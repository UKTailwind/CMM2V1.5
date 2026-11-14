/* MS7210.c: the G3's MacroSilicon MS7210, which turns the LTDC's RGB888 (with HS, VS and DE) into HDMI
   (docs/G3_HDMI_Plan.md). Brought up on the first G3 on 14 Nov 2026 from tools/sdk/examples/ms7210test.c, whose
   findings these are; the register meanings are MacroSilicon's SDK's and the FPGA boards' table's (ms7210_ctl.v).

   - The chip is on the RTC's bit-banged bus (SCL PD5, SDA PD4) at 0xB2 (0x56 if SA is grounded). It is driven here
     open drain, with an acknowledge check that ends a transfer at the first NACK: Analog_I2C.c's waits 250 ms for one,
     and a G2, which has no MS7210, would wait at every start-up. The DS3231's routines set the pins their own way each
     time, and both run only in the foreground.
   - Its reset is PG3 (27K to VCC on a G3, not connected on a G2). On the first G3 the reset at power-up by the pull-up
     alone left registers that are not the defaults (YUV 4:2:2 in, the HDMI clock halved): so PG3 goes low early in
     the start-up (MX_GPIO_Init), high once that is 10 ms old, and the chip is looked for 50 ms after.
   - It answers with no pixel clock, so a G3 is known before the first setmode, which then has the G3's settings.
   - In bank 2 (the linker script's .bank2): only setmode and the start-up call it, in the foreground. Its delays are
     uSec's (TIM16), not HAL_GetTick's: a loaded program's video_mode runs setmode with BASEPRI raised. */
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "PicoMode.h"                                           // (ModeHs)

int G3Hardware;                                                     // 1: the MS7210 answered (a G3)
static int ready;                                                   // its init done (MS7210_Mode)
static int dev = 0xB2;                                              // its address, 8-bit form
static uint32_t pg3_low, pg3_high;                                  // HAL_GetTick's when PG3 went low and high

#define SDA GPIO_PIN_4                                              // PD4 and PD5 (main.h's SDA_Pin, SCL_Pin)
#define SCL GPIO_PIN_5
static void hd(void) { uSec(2); }                                   // half a bit: about 100 kHz
static void msdelay(int n) { while(n--) uSec(1000); }
static inline void sda(int v) { GPIOD->BSRR = v ? SDA : (uint32_t)SDA << 16; }
static inline void scl(int v) { GPIOD->BSRR = v ? SCL : (uint32_t)SCL << 16; }
static void scl_up(void) {                                          // (a slave may hold SCL low: up to 1 ms)
    scl(1);
    for(int i = 0; i < 500 && !(GPIOD->IDR & SCL); i++) uSec(2);
}
static void pins(void) {                                            // PD4, PD5: open-drain outputs, released, pull-ups on
    GPIOD->BSRR = SDA | SCL;
    GPIOD->OTYPER |= SDA | SCL;
    GPIOD->PUPDR = (GPIOD->PUPDR & ~(0xFu << 8)) | (0x5u << 8);
    GPIOD->MODER = (GPIOD->MODER & ~(0xFu << 8)) | (0x5u << 8);
}
static void start(void) { sda(1); hd(); scl_up(); hd(); sda(0); hd(); scl(0); hd(); }
static void stop(void) { sda(0); hd(); scl_up(); hd(); sda(1); hd(); }
static int put(uint8_t b) {                                         // 1: acknowledged
    for(int i = 0; i < 8; i++, b <<= 1) { sda(b & 0x80); hd(); scl_up(); hd(); scl(0); }
    sda(1); hd(); scl_up(); hd();
    int ack = !(GPIOD->IDR & SDA);
    scl(0); hd();
    return ack;
}
static uint8_t get(void) {                                          // a byte, not acknowledged (the last of a read)
    uint8_t b = 0;
    sda(1);
    for(int i = 0; i < 8; i++) { hd(); scl_up(); hd(); b = (uint8_t)(b << 1 | ((GPIOD->IDR & SDA) != 0)); scl(0); }
    hd(); scl_up(); hd(); scl(0); hd();
    return b;
}

/* a register: its 16-bit number low byte first */
static void wr(uint16_t r, uint8_t v) {
    pins();
    start();
    if(put((uint8_t)dev) && put(r & 0xFF) && put(r >> 8)) put(v);
    stop();
}
static int rd(uint16_t r) {                                         // the byte, or -1 if nothing answered
    int v = -1;
    pins();
    start();
    if(put((uint8_t)dev) && put(r & 0xFF) && put(r >> 8)) {
        start();                                                    // (a repeated start)
        if(put((uint8_t)(dev | 1))) v = get();
    }
    stop();
    return v;
}
static void clr(uint16_t r, uint8_t bits, uint8_t set) {            // the bits cleared, then set's set
    int v = rd(r);
    if(v >= 0) wr(r, (uint8_t)((v & ~bits) | set));
}

/* the clock the chip takes: PLL3's R output (the pixel clock), HSE / M * N / R (FRACN is never used), or in a halved
   mode its P output, twice R, on PG7 */
static int doubled;                                                 // this mode's chip clock is SAI1's MCLK (MS7210_Pll)
static uint32_t chip_hz(void) {
    uint32_t m = (RCC->PLLCKSELR >> 20) & 0x3F, d = RCC->PLL3DIVR;
    uint32_t div = doubled ? ((d >> 9) & 0x7F) + 1 : ((d >> 24) & 0x7F) + 1;
    return m ? (uint32_t)((uint64_t)HSE_VALUE * ((d & 0x1FF) + 1) / m / div) : 0;
}

/* The halved modes (docs/G3_HDMI_Plan.md, step 4): their line is half a standard one at half its clock, under HDMI's
   25 MHz. The chip is clocked at twice the pixel clock instead, so it takes each pixel twice and sends the standard
   format: mode 7 (320x240) as VESA 640x480 at 75 Hz (Peter, 14 Nov 2026, the first G3: "Good picture, monitor reports
   640x480 @75Hz"), 13 as 800x600 at 75, 3 as 640x400 in 640x480's frame. The twice is PLL3's P
   (R/2, R made even for 3 and 7), SAI1's kernel clock, whose master clock goes out on PG7 (AF6) while block A is
   enabled as a master transmitter with nothing to send. setmode's, before PLL3_Config: pll adjusted for mode */
void MS7210_Pll(RCC_PLL3InitTypeDef *pll, int mode) {
    doubled = 0;
    if(!G3Hardware) return;
    if(ModeHs > 1) {                                                // OPTION PICOMITE's smaller modes (PicoMode.c): the base
        pll->PLL3P = pll->PLL3R / ModeHs;                           // mode's clock divided by ModeHs (setmode multiplied P, Q
        doubled = 1;                                                // and R): P back to the base's, so the chip has the base
        return;                                                     // mode's standard timing, each pixel ModeHs times
    }
    switch(mode) {
        case 3: case 7:                                             // x2: VCO 816, R 52 (15.69 MHz, -0.37%: R even), P 26
            pll->PLL3N = 102; pll->PLL3R = 52; pll->PLL3P = 26; break;
        case 12:                                                    // x2: 1080p at 60 Hz (2200 x 1125). VCO 592, R 8 (74.0 MHz,
            pll->PLL3N = 74; pll->PLL3R = 8; pll->PLL3P = 4; break; // -0.34%: R even), P 4 = 148.0 MHz, within SAI1's 150
        case 13:                                                    // x2: R 32 as it is
            pll->PLL3P = pll->PLL3R / 2; break;
        /* Modes 4 and 5 are VGA's 640x480 at 75 Hz too, their pixels 4/3 and 8/3 of VESA's (630 and 315 clocks a line;
           ltdc.c makes mode 5's 315 and DE 480 lines on a G3), so the chip is given 31.5 MHz as for 6 and 17 (Peter, 14
           Nov 2026: 480x432 "is not any sort of standard"; mode 5 was doubled, as mode 4's line at 23.54 MHz) */
        case 4:                                                     // x4/3: VCO 664, R 28 (23.71 MHz, +0.7%), P 21 (31.62)
            pll->PLL3N = 83; pll->PLL3R = 28; pll->PLL3P = 21; break;
        case 5:                                                     // x8/3: VCO 664, R 56 (11.86 MHz, +0.7%), P 21 (31.62)
            pll->PLL3N = 83; pll->PLL3R = 56; pll->PLL3P = 21; break;
        /* Modes 6 and 17 are VESA 640x480 at 75 Hz (840 x 500 at 31.5 MHz) with fewer, longer pixels, 2.5 and 5/3 of
           VESA's (336 and 504 clocks a line): a VGA monitor samples them at its 31.5 MHz. The chip is given that clock:
           P = R / 2.5 and R / (5/3), the pixels 2-3 and 2-2-1 samples wide as a VGA monitor's are (Peter's monitor, 14
           Nov: 640x480 for both, where a second monitor took neither as they were) */
        case 6:                                                     // x2.5: VCO 816, R 65 (12.55 MHz, -0.4%), P 26 (31.38)
            pll->PLL3N = 102; pll->PLL3R = 65; pll->PLL3P = 26; break;
        case 17:                                                    // x5/3: VCO 664, R 35 (18.97 MHz, +0.3%), P 21 (31.62)
            pll->PLL3N = 83; pll->PLL3R = 35; pll->PLL3P = 21; break;
        default: return;
    }
    doubled = 1;                                                    // (the chip's clock is P, on PG7)
}
static void mclk(int on) {                                          // SAI1 block A's MCLK on PG7, from pll3_p, undivided
    if(on) {
        RCC->D2CCIP1R = (RCC->D2CCIP1R & ~RCC_D2CCIP1R_SAI1SEL) | (2UL << RCC_D2CCIP1R_SAI1SEL_Pos);
        RCC->APB2ENR |= RCC_APB2ENR_SAI1EN;
        (void)RCC->APB2ENR;
    } else if(!(RCC->APB2ENR & RCC_APB2ENR_SAI1EN)) return;
    SAI1_Block_A->CR1 = 0;
    for(int i = 0; i < 100000 && (SAI1_Block_A->CR1 & SAI_xCR1_SAIEN); i++) {}
    if(!on) return;
    SAI1_Block_A->CR2 = 0;
    SAI1_Block_A->FRCR = 31 | (15UL << 8);                          // a 32-bit frame, FS 16 bits
    SAI1_Block_A->SLOTR = (1UL << 8) | (3UL << 16);                 // two slots, both on
    SAI1_Block_A->CR1 = 4UL << 5;                                   // 16-bit data, master transmitter, MCKDIV 0 (undivided)
    SAI1_Block_A->CR1 |= SAI_xCR1_SAIEN;
    GPIOG->AFR[0] = (GPIOG->AFR[0] & ~(0xFu << 28)) | (6u << 28);   // PG7: SAI1_MCLK_A (HAL_LTDC_MspInit made it LTDC_CLK)
}
static void band(uint32_t hz) {                                     // the PHY's drive: over 100 MHz the SDK's high band
    int high = hz > 100000000;
    clr(0x0906, 0x04, high ? 0x04 : 0);
    clr(0x0920, 0x40, high ? 0x40 : 0);
    wr(0x0926, high ? 0xDD : 0x11); wr(0x0927, high ? 0x0D : 0x01);
    wr(0x0928, high ? 0x88 : 0x11); wr(0x0929, high ? 0x08 : 0x01);
}
static void infoframe(void) {                                       // the AVI infoframe, raw bytes as the FPGA table's:
    static const uint8_t pb[14] = {0x06, 0x11, 0x58};               // video code 0 (VESA), RGB, overscan, 601, 4:3; byte 0
    wr(0x050E, 0x00);                                               // the checksum. Underscan, IT content and the range for
    wr(0x050A, 0x82); wr(0x0509, 0x02); wr(0x050B, 0x0D);           // TVs are for later (the plan's step 3)
    for(int i = 0; i < 14; i++) wr(0x050D, pb[i]);
    wr(0x050E, 0x40);
}

/* the start-up, in main: PG3 low (MX_GPIO_Init, on a G2 or a G3; never a G1, whose PG3 may be used) */
void MS7210_ResetStart(void) {
    GPIO_InitTypeDef g = {0};
    GPIOG->BSRR = (uint32_t)GPIO_PIN_3 << 16;
    g.Pin = GPIO_PIN_3;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &g);
    pg3_low = HAL_GetTick();
    pg3_high = 0;
}
void MS7210_Release(void) {                                          // PG3 high, once it has been low 10 ms (the datasheet)
    if(G1Hardware || pg3_high) return;
    while(HAL_GetTick() - pg3_low < 11) {}
    GPIOG->BSRR = GPIO_PIN_3;
    pg3_high = HAL_GetTick() | 1;
}
int MS7210_Start(void) {                                            // the chip looked for, 50 ms after the reset: 1 if found
    static const uint8_t addr[2] = {0xB2, 0x56};
    G3Hardware = ready = 0;
    if(G1Hardware) return 0;
    MS7210_Release();
    while(HAL_GetTick() - pg3_high < 50) {}
    for(int i = 0; i < 2 && !G3Hardware; i++) {
        dev = addr[i];
        G3Hardware = (rd(0x0000) == 0xA2 && rd(0x0001) == 0x20 && rd(0x0002) == 0x0A);
    }
    return G3Hardware;
}

/* setmode's, at the end of MX_LTDC_Init: the first time the chip's init, after it what a change of the pixel clock
   needs (measured on the G3: a change between the PHY's bands loses the HDMI PLL's lock unless it is re-triggered;
   within a band nothing is needed, but the LTDC has just been stopped and started, so the same is done every time) */
static const uint16_t table[][2] = {                                // the FPGA boards' first 19 writes, but 0x1281:
    {0x1281, 0x00}, {0x0016, 0x04}, {0x0009, 0x01}, {0x0007, 0x09}, {0x0008, 0xF0}, {0x000A, 0xF0}, {0x0006, 0x11},
    {0x0531, 0x84}, {0x0900, 0x20}, {0x0901, 0x47}, {0x0904, 0x09}, {0x0923, 0x07}, {0x0924, 0x44}, {0x0925, 0x44},
    {0x090F, 0x80}, {0x091F, 0x07}, {0x0920, 0x1E}, {0x0018, 0x20}, {0x05C0, 0xFE}};   // the sampling: the clock as it
                                                                    // comes, no delay (Peter, 14 Nov, in 1080p: of the 8
                                                                    // settings only the two extremes were bad)
void MS7210_Mode(void) {
    if(!G3Hardware) return;
    mclk(doubled);                                                  // (off, if a halved mode had it on)
    uint32_t hz = chip_hz();
    if(!ready) {
        wr(0x0060, 0x81); wr(0x0061, 0x54);                         // the internal oscillator at 27 MHz (the SDK's)
        for(unsigned i = 0; i < sizeof(table) / sizeof(table[0]); i++) wr(table[i][0], (uint8_t)table[i][1]);
        clr(0x0080, 0x0F, 0);                                       // the SDK's for RGB 24-bit SDR with HS, VS and DE, which
        clr(0x1202, 0xEF, 0);                                       // a proper reset leaves (but the power-up's did not):
        clr(0x1201, 0x10, 0);                                       // RGB in and out, sequential data, the clock as it
        clr(0x00C0, 0x03, 0);                                       // comes, DE from its pin, none regenerated, not DE only
        clr(0x1200, 0x0C, 0x04);                                    // or BT.1004, the pads pulled down, the HDMI clock 1:1
        clr(0x120B, 0x01, 0);
        clr(0x1265, 0x01, 0);
        clr(0x1262, 0x01, 0);
        clr(0x0079, 0x30, 0x20);
        clr(0x092A, 0x44, 0);
        msdelay(20);                                                // (the PHY's PLL powered: the SDK waits 10 ms)
        wr(0x000B, 0x00);
        wr(0x0019, 0x07);                                           // (the chip's pixel-clock meter, for diagnostics)
    }
    wr(0x0507, 0x06);                                               // muted
    band(hz);
    wr(0x0910, 0x01);                                               // the HDMI PLL re-triggered
    msdelay(10);
    if(!ready) wr(0x000B, 0x11);                                    // (the shell out of reset)
    infoframe();
    wr(0x0507, 0x00);                                               // unmuted
    ready = 1;
}

/* the board's name with its clock, for the start-up banner and OPTION LIST */
const char *BoardSignOn(void) {
    int f = (SystemCoreClock == 480000000);
    if(G1Hardware) return f ? MES_SIGNON480 : MES_SIGNON400;
    if(G3Hardware) return f ? MES_SIGNON480G3 : MES_SIGNON400G3;
    return f ? MES_SIGNON480G2 : MES_SIGNON400G2;
}
