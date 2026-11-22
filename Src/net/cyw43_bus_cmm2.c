/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * cyw43_bus_cmm2.c: pico-sdk's cyw43_bus_pio_spi.c (src/rp2_common/pico_cyw43_driver, the PicoMite's pico-sdk) for
 * the Colour Maximite 2's firmware, as apps/wifi's (step 1 of docs/Web_Plan.md). The gSPI framing below make_cmd() is copied as it is. Only what drove the RP2040's PIO and
 * DMA is new: the transfer is SPI2 of the STM32H743 in half-duplex (CFG2.COMM = 3, one data line), polled.
 *
 * Measured from BASIC on the G3 (15 Nov 2026, tests/wifi): the chip in high-speed mode, which cyw43_ll.c sets, changes
 * the line on the rising edge, and SPI2 reads it in SPI mode 0, sampling on that same edge while the chip's output
 * delay holds the bit, as the PIO's spi_gap01_sample0 does. No errors at 24 MHz (SPI2's kernel clock, PLL1Q, 48 MHz
 * on a 480 MHz board, /2; a 400 MHz board's PLL1Q is 40 MHz, so 20 MHz there).
 *
 * The wire carries the buffer's bytes in memory order, each MSB first (the PIO's DMA swapped each word's bytes and
 * shifted it out MSB first). SPI2 sends 32-bit frames MSB first, so each word is byte-reversed (__REV) on the way out
 * and on the way in. All lengths are multiples of 4, as pico-sdk asserts.
 *
 * Pins: PB13 SPI2_SCK (AF5), PB15 SPI2_MOSI (AF5) during a transfer and an input (pulled down, as pico-sdk does)
 * between transfers, so that cyw43_hal_pin_read() sees the chip's IRQ; PI3 CS; PI8 WL_ON (cyw43_driver_cmm2.c).
 *
 * A G3 V1.2 (main.c RM2OnBoard, docs/G3_V1.2_Header_Plan.md) has the module on the board, on SPI6: PG13 SCK and PG14
 * the data line (both AF5), CS PF7, WL_ON PF8. SPI6 is one of the H743's limited SPIs: frames of 16 bits at most, an
 * 8-byte FIFO and at most 1023 frames a transfer (TSIZE). So each word, byte-reversed as above, goes as two 16-bit
 * frames, its high half first, and the wire carries the same bits; a transfer of more than SPI16_PART_WORDS words is
 * made in parts, with CS held low. SPI6's kernel clock is the D3 bus clock (120 MHz on a 480 MHz board), divided to
 * 30 MHz at most.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "stm32h7xx_hal.h"
#include "Loader.h"                             // (MicrosNow)
#include "cyw43.h"
#include "cyw43_internal.h"
#include "cyw43_spi.h"
#include "cyw43_debug_pins.h"

#if CYW43_SPI_PIO

#define IRQ_SAMPLE_DELAY_NS 100

#if !CYW43_USE_SPI
#error CYW43_USE_SPI should be true
#endif

#define DUMP_SPI_TRANSACTIONS(A)

//#define SWAP32(A) ((((A) & 0xff000000U) >> 8) | (((A) & 0xff0000U) << 8) | (((A) & 0xff00U) >> 8) | (((A) & 0xffU) << 8))
static inline __attribute__((always_inline)) uint32_t __swap16x2(uint32_t a) {
    __asm ("rev16 %0, %0" : "+l" (a) : : );
    return a;
}
#define SWAP32(a) __swap16x2(a)

#define SPI2_TIMEOUT_US 20000                    // (a transfer is at most 2 KB: 700 us at 24 MHz)
#define SPI6_MAX_HZ 30000000u                    // a G3 V1.2's bus clock at most (docs/G3_V1.2_Header_Plan.md)
#define SPI16_PART_WORDS 511u                    // a limited SPI's TSIZE: 1023 frames at most, two to a word

void cyw43_cmm2_pin_mode(GPIO_TypeDef *port, int bit, int moder);   // cyw43_driver_cmm2.c: 0 input, 1 output, 2 AF

extern int RM2OnBoard;                           // main.c: a G3 V1.2, the module on the board, on SPI6
#define DATA_PORT (RM2OnBoard ? GPIOG : GPIOB)   // the data line: PG14 on the board, PB15 on the add-on
#define DATA_BIT (RM2OnBoard ? 14 : 15)

typedef struct {
    uint32_t errors;
} bus_data_t;

static bus_data_t bus_data_instance;
static uint32_t cpu_mhz = 480;

// a G3 V1.2: SPI6, as SPI2 is set below but for the frames (16 bits) and the clock (pclk4, divided to SPI6_MAX_HZ)
static void spi6_init(void) {
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOFEN | RCC_AHB4ENR_GPIOGEN;
    RCC->D3CCIPR &= ~RCC_D3CCIPR_SPI6SEL;        // SPI6's kernel clock: pclk4, the D3 bus clock (its reset value)
    RCC->APB4ENR |= RCC_APB4ENR_SPI6EN;
    (void)RCC->APB4ENR;
    RCC->APB4RSTR |= RCC_APB4RSTR_SPI6RST;
    RCC->APB4RSTR &= ~RCC_APB4RSTR_SPI6RST;

    // PG13 (SCK) and PG14 (the data line): AF5, medium speed; PG14 pulled down
    GPIOG->AFR[1] = (GPIOG->AFR[1] & ~((0xFu << 20) | (0xFu << 24))) | (5u << 20) | (5u << 24);
    GPIOG->OSPEEDR = (GPIOG->OSPEEDR & ~((3u << 26) | (3u << 28))) | (1u << 26) | (1u << 28);
    GPIOG->OTYPER &= ~((1u << 13) | (1u << 14));
    GPIOG->PUPDR = (GPIOG->PUPDR & ~((3u << 26) | (3u << 28))) | (2u << 28);
    cyw43_cmm2_pin_mode(GPIOG, 13, 2);

    uint32_t mbr = 0, pclk = HAL_RCCEx_GetD3PCLK1Freq();   // the divider: 2 << mbr
    while (mbr < 7 && (pclk >> (mbr + 1)) > SPI6_MAX_HZ) mbr++;
    SPI6->CR1 = SPI_CR1_SSI;
    SPI6->CFG1 = (mbr << SPI_CFG1_MBR_Pos) | (15u << SPI_CFG1_DSIZE_Pos);
    SPI6->CFG2 = SPI_CFG2_AFCNTR | SPI_CFG2_SSM | SPI_CFG2_MASTER | SPI_CFG2_COMM_0 | SPI_CFG2_COMM_1;
}

int cyw43_spi_init(cyw43_int_t *self) {
    // Only does something if CYW43_LOGIC_DEBUG=1
    logic_debug_init();

    assert(!self->bus_data);
    self->bus_data = &bus_data_instance;

    cpu_mhz = SystemCoreClock / 1000000u;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;   // the cycle counter, for ns_delay
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    if (RM2OnBoard) {
        spi6_init();
        return 0;
    }
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOIEN;
    RCC->APB1LENR |= RCC_APB1LENR_SPI2EN;
    (void)RCC->APB1LENR;
    RCC->APB1LRSTR |= RCC_APB1LRSTR_SPI2RST;
    RCC->APB1LRSTR &= ~RCC_APB1LRSTR_SPI2RST;

    // PB13 (SCK) and PB15 (the data line): AF5, medium speed; PB15 pulled down, as pico-sdk sets the data pin
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~((0xFu << 20) | (0xFu << 28))) | (5u << 20) | (5u << 28);
    GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~((3u << 26) | (3u << 30))) | (1u << 26) | (1u << 30);
    GPIOB->OTYPER &= ~((1u << 13) | (1u << 15));
    GPIOB->PUPDR = (GPIOB->PUPDR & ~((3u << 26) | (3u << 30))) | (2u << 30);
    cyw43_cmm2_pin_mode(GPIOB, 13, 2);

    // SPI2: master, software NSS, half-duplex, mode 0, MSB first, 32-bit frames, /2; AFCNTR keeps SCK low (and the
    // data line driven while it transmits) when it is off between phases
    SPI2->CR1 = SPI_CR1_SSI;
    SPI2->CFG1 = (0u << SPI_CFG1_MBR_Pos) | (31u << SPI_CFG1_DSIZE_Pos);
    SPI2->CFG2 = SPI_CFG2_AFCNTR | SPI_CFG2_SSM | SPI_CFG2_MASTER | SPI_CFG2_COMM_0 | SPI_CFG2_COMM_1;
    return 0;
}

void cyw43_spi_deinit(cyw43_int_t *self) {
    if (self->bus_data) {
        (RM2OnBoard ? SPI6 : SPI2)->CR1 = SPI_CR1_SSI;
        self->bus_data = NULL;
    }
}

static void cs_set(bool value) {
    if (RM2OnBoard) GPIOF->BSRR = value ? (1u << 7) : (1u << (7 + 16));
    else GPIOI->BSRR = value ? (1u << 3) : (1u << (3 + 16));
}

static void ns_delay(uint32_t ns) {
    uint32_t cycles = ns * cpu_mhz / 1000u;
    uint32_t t0 = DWT->CYCCNT;
    while (DWT->CYCCNT - t0 < cycles) ;
}

static void start_spi_comms(cyw43_int_t *self) {
    (void)self;
    cyw43_cmm2_pin_mode(DATA_PORT, DATA_BIT, 2); // the data line to the SPI
    // Pull CS low
    cs_set(false);
}

// we need to atomically de-assert CS and enable IRQ
static void stop_spi_comms(void) {
    // from this point a positive edge will cause an IRQ to be pending
    cs_set(true);
    cyw43_cmm2_pin_mode(DATA_PORT, DATA_BIT, 0); // the data line an input: the chip's IRQ
    // we need to wait a bit in case the irq line is incorrectly high
    ns_delay(IRQ_SAMPLE_DELAY_NS);
}

// n words out: SPI2 transmitting (HDDIR), TSIZE n, filled as the FIFO takes them
static int spi2_send(const uint32_t *w, uint32_t n) {
    SPI2->CR1 = SPI_CR1_SSI | SPI_CR1_HDDIR;
    SPI2->CR2 = n;
    SPI2->CR1 = SPI_CR1_SSI | SPI_CR1_HDDIR | SPI_CR1_SPE;
    uint32_t i = 0;
    while (i < n && (SPI2->SR & SPI_SR_TXP)) SPI2->TXDR = __REV(w[i++]);
    SPI2->CR1 = SPI_CR1_SSI | SPI_CR1_HDDIR | SPI_CR1_SPE | SPI_CR1_CSTART;
    uint64_t t0 = MicrosNow();
    int ret = 0;
    for (;;) {
        uint32_t sr = SPI2->SR;
        if (i < n) {
            if (sr & SPI_SR_TXP) { SPI2->TXDR = __REV(w[i++]); continue; }
        } else if (sr & SPI_SR_EOT) break;
        if (MicrosNow() - t0 > SPI2_TIMEOUT_US) { ret = -CYW43_EIO; break; }
    }
    SPI2->IFCR = 0xFF8;
    SPI2->CR1 = SPI_CR1_SSI;                     // off, and the receive direction: the line let go
    return ret;
}

// n words in: SPI2 receiving, TSIZE n; MASRX stops the clock rather than overrun when an interrupt holds the CPU
static int spi2_receive(uint32_t *w, uint32_t n) {
    SPI2->CR1 = SPI_CR1_SSI | SPI_CR1_MASRX;
    SPI2->CR2 = n;
    SPI2->CR1 = SPI_CR1_SSI | SPI_CR1_MASRX | SPI_CR1_SPE;
    SPI2->CR1 = SPI_CR1_SSI | SPI_CR1_MASRX | SPI_CR1_SPE | SPI_CR1_CSTART;
    uint32_t i = 0;
    uint64_t t0 = MicrosNow();
    int ret = 0;
    for (;;) {
        uint32_t sr = SPI2->SR;
        if (i < n) {
            if (sr & SPI_SR_RXP) { w[i++] = __REV(SPI2->RXDR); continue; }
            if (sr & SPI_SR_SUSP) {              // suspended with the FIFO read: go on
                SPI2->IFCR = SPI_IFCR_SUSPC;
                SPI2->CR1 = SPI_CR1_SSI | SPI_CR1_MASRX | SPI_CR1_SPE | SPI_CR1_CSTART;
            }
        } else if (sr & SPI_SR_EOT) break;
        if (MicrosNow() - t0 > SPI2_TIMEOUT_US) { ret = -CYW43_EIO; break; }
    }
    SPI2->IFCR = 0xFF8;
    SPI2->CR1 = SPI_CR1_SSI;
    return ret;
}

// A limited SPI (a G3 V1.2's SPI6): the same transfers in 16-bit frames, two to a word, its high half first, and in
// parts of at most SPI16_PART_WORDS words (TSIZE). Frame i of w: half of the byte-reversed word i / 2.
static inline uint16_t half_out(const uint32_t *w, uint32_t i) {
    uint32_t v = __REV(w[i >> 1]);
    return (i & 1) ? (uint16_t)v : (uint16_t)(v >> 16);
}

static int spi16_send(SPI_TypeDef *s, const uint32_t *w, uint32_t n) {
    volatile uint16_t *txdr = (volatile uint16_t *)&s->TXDR;    // (one frame a write)
    while (n) {
        uint32_t part = n < SPI16_PART_WORDS ? n : SPI16_PART_WORDS, f = 2 * part, i = 0;
        s->CR1 = SPI_CR1_SSI | SPI_CR1_HDDIR;
        s->CR2 = f;
        s->CR1 = SPI_CR1_SSI | SPI_CR1_HDDIR | SPI_CR1_SPE;
        while (i < f && (s->SR & SPI_SR_TXP)) { *txdr = half_out(w, i); i++; }
        s->CR1 = SPI_CR1_SSI | SPI_CR1_HDDIR | SPI_CR1_SPE | SPI_CR1_CSTART;
        uint64_t t0 = MicrosNow();
        int ret = 0;
        for (;;) {
            uint32_t sr = s->SR;
            if (i < f) {
                if (sr & SPI_SR_TXP) { *txdr = half_out(w, i); i++; continue; }
            } else if (sr & SPI_SR_EOT) break;
            if (MicrosNow() - t0 > SPI2_TIMEOUT_US) { ret = -CYW43_EIO; break; }
        }
        s->IFCR = 0xFF8;
        s->CR1 = SPI_CR1_SSI;
        if (ret) return ret;
        w += part;
        n -= part;
    }
    return 0;
}

static int spi16_receive(SPI_TypeDef *s, uint32_t *w, uint32_t n) {
    volatile uint16_t *rxdr = (volatile uint16_t *)&s->RXDR;    // (one frame a read)
    while (n) {
        uint32_t part = n < SPI16_PART_WORDS ? n : SPI16_PART_WORDS, f = 2 * part, i = 0, hi = 0;
        s->CR1 = SPI_CR1_SSI | SPI_CR1_MASRX;
        s->CR2 = f;
        s->CR1 = SPI_CR1_SSI | SPI_CR1_MASRX | SPI_CR1_SPE;
        s->CR1 = SPI_CR1_SSI | SPI_CR1_MASRX | SPI_CR1_SPE | SPI_CR1_CSTART;
        uint64_t t0 = MicrosNow();
        int ret = 0;
        for (;;) {
            uint32_t sr = s->SR;
            if (i < f) {
                if (sr & SPI_SR_RXP) {
                    uint32_t v = *rxdr;
                    if (i & 1) w[i >> 1] = __REV((hi << 16) | v);
                    else hi = v;
                    i++;
                    continue;
                }
                if (sr & SPI_SR_SUSP) {          // suspended with the FIFO read: go on
                    s->IFCR = SPI_IFCR_SUSPC;
                    s->CR1 = SPI_CR1_SSI | SPI_CR1_MASRX | SPI_CR1_SPE | SPI_CR1_CSTART;
                }
            } else if (sr & SPI_SR_EOT) break;
            if (MicrosNow() - t0 > SPI2_TIMEOUT_US) { ret = -CYW43_EIO; break; }
        }
        s->IFCR = 0xFF8;
        s->CR1 = SPI_CR1_SSI;
        if (ret) return ret;
        w += part;
        n -= part;
    }
    return 0;
}

static int bus_send(const uint32_t *w, uint32_t n) {
    return RM2OnBoard ? spi16_send(SPI6, w, n) : spi2_send(w, n);
}

static int bus_receive(uint32_t *w, uint32_t n) {
    return RM2OnBoard ? spi16_receive(SPI6, w, n) : spi2_receive(w, n);
}

int cyw43_spi_transfer(cyw43_int_t *self, const uint8_t *tx, size_t tx_length, uint8_t *rx,
                       size_t rx_length) {

    if ((tx == NULL) && (rx == NULL)) {
        return CYW43_FAIL_FAST_CHECK(-CYW43_EINVAL);
    }

    bus_data_t *bus_data = (bus_data_t *)self->bus_data;
    int ret = 0;
    start_spi_comms(self);
    if (rx != NULL) {
        if (tx == NULL) {
            tx = rx;
            assert(tx_length && tx_length < rx_length);
        }
        assert(!(tx_length & 3));
        assert(!(((uintptr_t)tx) & 3));
        assert(!(((uintptr_t)rx) & 3));
        assert(!(rx_length & 3));

        ret = bus_send((const uint32_t *)tx, tx_length / 4);
        if (ret == 0) {
            ret = bus_receive((uint32_t *)(rx + tx_length), rx_length / 4 - tx_length / 4);
        }

        memset(rx, 0, tx_length); // make sure we don't have garbage in what would have been returned data if using real SPI
    } else if (tx != NULL) {
        assert(!(((uintptr_t)tx) & 3));
        assert(!(tx_length & 3));
        ret = bus_send((const uint32_t *)tx, tx_length / 4);
    }

    stop_spi_comms();
    if (ret) {
        bus_data->errors++;
    }
    return ret;
}

// Initialise our gpios
void cyw43_spi_gpio_setup(void) {
    // Setup CYW43_PIN_WL_REG_ON
    cyw43_hal_pin_config(CYW43_PIN_WL_REG_ON, CYW43_HAL_PIN_MODE_OUTPUT, CYW43_HAL_PIN_PULL_UP, 0);

    // Setup DO, DI and IRQ: the data line low while the chip powers up
    DATA_PORT->BSRR = 1u << (DATA_BIT + 16);
    cyw43_cmm2_pin_mode(DATA_PORT, DATA_BIT, 1);

    // Setup CS
    cs_set(true);
    if (RM2OnBoard) cyw43_cmm2_pin_mode(GPIOF, 7, 1);
    else cyw43_cmm2_pin_mode(GPIOI, 3, 1);
}

// Reset wifi chip
void cyw43_spi_reset(void) {
    cyw43_hal_pin_low(CYW43_PIN_WL_REG_ON); // off
    cyw43_delay_ms(20);
    cyw43_hal_pin_high(CYW43_PIN_WL_REG_ON); // on
    cyw43_delay_ms(250);

    // Setup IRQ - also used for DO, DI
    cyw43_cmm2_pin_mode(DATA_PORT, DATA_BIT, 0);
}

uint32_t cyw43_cmm2_bus_errors(void) {
    return bus_data_instance.errors;
}

// ---- from here on, pico-sdk's cyw43_bus_pio_spi.c as it is (its lines 371-547) ----

static inline uint32_t make_cmd(bool write, bool inc, uint32_t fn, uint32_t addr, uint32_t sz) {
    return write << 31 | inc << 30 | fn << 28 | (addr & 0x1ffff) << 11 | sz;
}

#if CYW43_VERBOSE_DEBUG
static const char *func_name(int fn) {
    switch (fn)
    {
        case BUS_FUNCTION:
            return "BUS_FUNCTION";
        case BACKPLANE_FUNCTION:
            return "BACKPLANE_FUNCTION";
        case WLAN_FUNCTION:
            return "WLAN_FUNCTION";
        default:
            return "UNKNOWN";
    }
}
#endif

uint32_t read_reg_u32_swap(cyw43_int_t *self, uint32_t fn, uint32_t reg) {
    uint32_t buf[2] = {0};
    assert(fn != BACKPLANE_FUNCTION);
    buf[0] = SWAP32(make_cmd(false, true, fn, reg, 4));
    int ret = cyw43_spi_transfer(self, NULL, 4, (uint8_t *)buf, 8);
    if (ret != 0) {
        return ret;
    }
    return SWAP32(buf[1]);
}

static inline uint32_t _cyw43_read_reg(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint size) {
    // Padding plus max read size of 32 bits + another 4?
    static_assert(CYW43_BACKPLANE_READ_PAD_LEN_BYTES % 4 == 0, "");
    int index = (CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4) + 1 + 1;
    uint32_t buf32[index];
    uint8_t *buf = (uint8_t *)buf32;
    const uint32_t padding = (fn == BACKPLANE_FUNCTION) ? CYW43_BACKPLANE_READ_PAD_LEN_BYTES : 0; // Add response delay
    buf32[0] = make_cmd(false, true, fn, reg, size);

    if (fn == BACKPLANE_FUNCTION) {
        logic_debug_set(pin_BACKPLANE_READ, 1);
    }
    int ret = cyw43_spi_transfer(self, NULL, 4, buf, 8 + padding);
    if (fn == BACKPLANE_FUNCTION) {
        logic_debug_set(pin_BACKPLANE_READ, 0);
    }

    if (ret != 0) {
        return ret;
    }
    uint32_t result = buf32[padding > 0 ? index - 1 : 1];
    CYW43_VDEBUG("cyw43_read_reg_u%d %s 0x%lx=0x%lx\n", size * 8, func_name(fn), reg, result);
    return result;
}

uint32_t cyw43_read_reg_u32(cyw43_int_t *self, uint32_t fn, uint32_t reg) {
    return _cyw43_read_reg(self, fn, reg, 4);
}

int cyw43_read_reg_u16(cyw43_int_t *self, uint32_t fn, uint32_t reg) {
    return _cyw43_read_reg(self, fn, reg, 2);
}

int cyw43_read_reg_u8(cyw43_int_t *self, uint32_t fn, uint32_t reg) {
    return _cyw43_read_reg(self, fn, reg, 1);
}

// This is only used to switch the word order on boot
int write_reg_u32_swap(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint32_t val) {
    uint32_t buf[2];
    // Boots up in little endian so command needs swapping too
    buf[0] = SWAP32(make_cmd(true, true, fn, reg, 4));
    buf[1] = SWAP32(val);
    int ret = cyw43_spi_transfer(self, (uint8_t *)buf, 8, NULL, 0);
    CYW43_VDEBUG("write_reg_u32_swap %s 0x%lx=0x%lx\n", func_name(fn), reg, val);
    return ret;
}

static inline int _cyw43_write_reg(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint32_t val, uint size) {
    uint32_t buf[2];
    buf[0] = make_cmd(true, true, fn, reg, size);
    buf[1] = val;

    if (fn == BACKPLANE_FUNCTION) {
        logic_debug_set(pin_BACKPLANE_WRITE, 1);
    }

    int ret = cyw43_spi_transfer(self, (uint8_t *)buf, 8, NULL, 0);

    if (fn == BACKPLANE_FUNCTION) {
        logic_debug_set(pin_BACKPLANE_WRITE, 0);
    }

    CYW43_VDEBUG("cyw43_write_reg_u%d %s 0x%lx=0x%lx\n", size * 8, func_name(fn), reg, val);
    return ret;
}

int cyw43_write_reg_u32(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint32_t val) {
    return _cyw43_write_reg(self, fn, reg, val, 4);
}

int cyw43_write_reg_u16(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint16_t val) {
    return _cyw43_write_reg(self, fn, reg, val, 2);
}

int cyw43_write_reg_u8(cyw43_int_t *self, uint32_t fn, uint32_t reg, uint32_t val) {
    return _cyw43_write_reg(self, fn, reg, val, 1);
}

#if CYW43_BUS_MAX_BLOCK_SIZE > 0x7f8
#error Block size is wrong for SPI
#endif

int cyw43_read_bytes(cyw43_int_t *self, uint32_t fn, uint32_t addr, size_t len, uint8_t *buf) {
    assert(fn != BACKPLANE_FUNCTION || (len <= CYW43_BUS_MAX_BLOCK_SIZE));
    const uint32_t padding = (fn == BACKPLANE_FUNCTION) ? CYW43_BACKPLANE_READ_PAD_LEN_BYTES : 0; // Add response delay
    size_t aligned_len = (len + 3) & ~3;
    assert(aligned_len > 0 && aligned_len <= 0x7f8);
    assert(buf == self->spid_buf || buf < self->spid_buf || buf >= (self->spid_buf + sizeof(self->spid_buf)));
    self->spi_header[padding > 0 ? 0 : (CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4)] = make_cmd(false, true, fn, addr, len);
    if (fn == WLAN_FUNCTION) {
        logic_debug_set(pin_WIFI_RX, 1);
    }
    int ret = cyw43_spi_transfer(self, NULL, 4, (uint8_t *)&self->spi_header[padding > 0 ? 0 : (CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4)], aligned_len + 4 + padding);
    if (fn == WLAN_FUNCTION) {
        logic_debug_set(pin_WIFI_RX, 0);
    }
    if (ret != 0) {
        CYW43_PRINTF("cyw43_read_bytes error %d", ret);
        return ret;
    }
    if (buf != self->spid_buf) { // avoid a copy in the usual case just to add the header
        memcpy(buf, self->spid_buf, len);
    }
    return 0;
}

// See whd_bus_spi_transfer_bytes
// Note, uses spid_buf if src isn't using it already
// Apart from firmware download this appears to only be used for wlan functions?
int cyw43_write_bytes(cyw43_int_t *self, uint32_t fn, uint32_t addr, size_t len, const uint8_t *src) {
    assert(fn != BACKPLANE_FUNCTION || (len <= CYW43_BUS_MAX_BLOCK_SIZE));
    const size_t aligned_len = (len + 3) & ~3u;
    assert(aligned_len > 0 && aligned_len <= 0x7f8);
    if (fn == WLAN_FUNCTION) {
        // Wait for FIFO to be ready to accept data
        int f2_ready_attempts = 1000;
        while (f2_ready_attempts-- > 0) {
            uint32_t bus_status = cyw43_read_reg_u32(self, BUS_FUNCTION, SPI_STATUS_REGISTER);
            if (bus_status & STATUS_F2_RX_READY) {
                logic_debug_set(pin_F2_RX_READY_WAIT, 0);
                break;
            } else {
                logic_debug_set(pin_F2_RX_READY_WAIT, 1);
            }
        }
        if (f2_ready_attempts <= 0) {
            CYW43_PRINTF("F2 not ready\n");
            return CYW43_FAIL_FAST_CHECK(-CYW43_EIO);
        }
    }
    if (src == self->spid_buf) { // avoid a copy in the usual case just to add the header
        self->spi_header[(CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4)] = make_cmd(true, true, fn, addr, len);
        logic_debug_set(pin_WIFI_TX, 1);
        int res = cyw43_spi_transfer(self, (uint8_t *)&self->spi_header[(CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4)], aligned_len + 4, NULL, 0);
        logic_debug_set(pin_WIFI_TX, 0);
        return res;
    } else {
        // todo: would be nice to get rid of this. Only used for firmware download?
        assert(src < self->spid_buf || src >= (self->spid_buf + sizeof(self->spid_buf)));
        self->spi_header[(CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4)] = make_cmd(true, true, fn, addr, len);
        memcpy(self->spid_buf, src, len);
        return cyw43_spi_transfer(self, (uint8_t *)&self->spi_header[(CYW43_BACKPLANE_READ_PAD_LEN_BYTES / 4)], aligned_len + 4, NULL, 0);
    }
}
#endif
