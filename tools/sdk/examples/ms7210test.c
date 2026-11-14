/* ms7210test.c: the G3's MS7210 (the LTDC's RGB888 in, HDMI out) brought up from a loadable program, with the firmware
   as it is (docs/G3_HDMI_Plan.md, step 1). The register meanings are MacroSilicon's SDK's and the FPGA boards' 49-write
   table's (ms7210_ctl.v), as the plan gives them. MIT License, as cmm2img.h.

     RUN "ms7210test.app"                     id, then status
     RUN "ms7210test.app", id                 the chip's ID at 0xB2 and 0x56, without the pixel clock on PG7 and with it
     RUN "ms7210test.app", reset              PG3 (RSTN) low for 20 ms: the ID read during it and after
     RUN "ms7210test.app", on notrim          the G3's LTDC settings and the chip's init in BASIC's mode. The picture
                                              stays on after the program ends, until a MODE changes the mode. notrim:
                                              the internal oscillator left as it is (not set to 27 MHz)
     RUN "ms7210test.app", status             HPD, PLL lock, the clock the chip counts, timing stable, INT (PC8)
     RUN "ms7210test.app", bars m             the test card in MODE m,8 (1 if not given) until a key
     RUN "ms7210test.app", mode m what        MODE m,8, then none, trig or full (the default) written to the chip, and
                                              the test card until a key; BASIC's mode back at the end
     RUN "ms7210test.app", int s mask         PC8 (INT#), 0x0500 and HPD watched for s seconds (30): plug and unplug.
                                              mask (hex) written to 0x0501 first
     RUN "ms7210test.app", edge pc pa speed   the LTDC's clock polarity (GCR PCPOL, 0 or 1), the chip's sampling (0x1281,
                                              0-7: bit 2 inverts, bits 1-0 delay) and the pins' speed (0-3)
     RUN "ms7210test.app", avi vic q s itc    the AVI infoframe: video code, Q (0 default, 2 full range), S (1
                                              overscan, 2 underscan), ITC (1: IT content)
     RUN "ms7210test.app", reg r v            register r read, or v written to it and read back (hex)
     RUN "ms7210test.app", dump r n           n registers from r (hex; 16 if not given)

   The test card (8-bit, through the palette, so at full level whatever OPTION VGA OUTPUT says): the top half 24 bars,
   each one of the 24 data lines alone, R7 at the left to B0 at the right, so a missing or crossed line shows; then
   &HFFFFFF, the firmware's &HE0E0E0 and alternate black and white columns (the finest detail, for the clock edge); at
   the bottom the 16 darkest greys and the 16 brightest (a TV that takes the levels as limited range loses them).

   Everything but the chip's registers and the LTDC's settings is put back at the end: PD4/PD5 (the RTC's bus, used
   here open drain), PC8 and PG3. The firmware's next change of mode puts the LTDC back as a G2's.
   Built with: python build.py out/ms7210test.app examples/ms7210test.c --api 2
               -I ../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I ../../Drivers/CMSIS/Include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"
#include "stm32h743xx.h"

/* --- I2C on PD4 (SDA) and PD5 (SCL), bit-banged open drain: a NACK ends a transfer at once (Analog_I2C.c's
   I2C_Wait_Ack waits 250 ms for one) --- */
#define SDA 4
#define SCL 5
static uint32_t pd_moder, pd_otyper, pd_pupdr, pd_odr;
static int nacks;
static void hd(void) { cmm2->delay_us(2); }                        // half a bit: about 100 kHz with the calls
static inline void sda(int v) { GPIOD->BSRR = v ? (1u << SDA) : (1u << (SDA + 16)); }
static inline void scl(int v) { GPIOD->BSRR = v ? (1u << SCL) : (1u << (SCL + 16)); }
static inline int sda_in(void) { return (GPIOD->IDR >> SDA) & 1; }
static void scl_up(void) {                                         // (a slave may hold SCL low: up to 1 ms)
    scl(1);
    uint64_t t = cmm2->micros();
    while(!(GPIOD->IDR & (1u << SCL)) && cmm2->micros() - t < 1000) ;
}
static void start(void) { sda(1); hd(); scl_up(); hd(); sda(0); hd(); scl(0); hd(); }
static void stop(void) { sda(0); hd(); scl_up(); hd(); sda(1); hd(); }
static int put(uint8_t b) {                                        // 1: acknowledged
    for(int i = 0; i < 8; i++, b <<= 1) { sda(b & 0x80); hd(); scl_up(); hd(); scl(0); }
    sda(1); hd(); scl_up(); hd();
    int ack = !sda_in();
    scl(0); hd();
    return ack;
}
static uint8_t get(int ack) {
    uint8_t b = 0;
    sda(1);
    for(int i = 0; i < 8; i++) { hd(); scl_up(); hd(); b = (uint8_t)(b << 1 | sda_in()); scl(0); }
    sda(!ack); hd(); scl_up(); hd(); scl(0); sda(1); hd();
    return b;
}
static void i2c_open(void) {
    pd_moder = GPIOD->MODER; pd_otyper = GPIOD->OTYPER; pd_pupdr = GPIOD->PUPDR; pd_odr = GPIOD->ODR;
    GPIOD->BSRR = (1u << SDA) | (1u << SCL);                       // released
    GPIOD->OTYPER |= (1u << SDA) | (1u << SCL);                    // open drain: a 1 leaves the line to the pull-ups
    GPIOD->PUPDR = (GPIOD->PUPDR & ~(0xFu << (2 * SDA))) | (5u << (2 * SDA));   // (and the internal pull-ups on)
    GPIOD->MODER = (GPIOD->MODER & ~(0xFu << (2 * SDA))) | (5u << (2 * SDA));   // outputs
    for(int i = 0; i < 9 && !sda_in(); i++) { scl(0); hd(); scl_up(); hd(); }   // a slave left holding SDA: clocked out
    stop();
}
static void i2c_close(void) {
    GPIOD->BSRR = ((pd_odr >> SDA & 1) ? 1u << SDA : 1u << (SDA + 16)) | ((pd_odr >> SCL & 1) ? 1u << SCL : 1u << (SCL + 16));
    GPIOD->MODER = (GPIOD->MODER & ~(0xFu << (2 * SDA))) | (pd_moder & (0xFu << (2 * SDA)));
    GPIOD->PUPDR = (GPIOD->PUPDR & ~(0xFu << (2 * SDA))) | (pd_pupdr & (0xFu << (2 * SDA)));
    GPIOD->OTYPER = (GPIOD->OTYPER & ~(3u << SDA)) | (pd_otyper & (3u << SDA));
}

/* --- the MS7210's registers: a 16-bit number, low byte first, then the data --- */
static int dev = 0xB2;                                             // the address in 8-bit form (0x56 with SA grounded)
static int wr(uint16_t r, uint8_t v) {
    start();
    int ok = put((uint8_t)dev) && put(r & 0xFF) && put(r >> 8) && put(v);
    stop();
    if(!ok) nacks++;
    return ok ? 0 : -1;
}
static int rd(uint16_t r) {                                        // the byte, or -1 if nothing answered
    start();
    if(!(put((uint8_t)dev) && put(r & 0xFF) && put(r >> 8))) { stop(); nacks++; return -1; }
    start();                                                       // (a repeated start)
    if(!put((uint8_t)(dev | 1))) { stop(); nacks++; return -1; }
    int v = get(0);
    stop();
    return v;
}
static void ms(int n) { cmm2->delay_us((uint32_t)n * 1000); }

static int id_at(int a) {                                          // 1: the MS7210 is at a
    dev = a;
    int i0 = rd(0x0000), i1 = rd(0x0001), i2 = rd(0x0002);
    if(i0 < 0) { printf("  0x%02X: no answer\n", a); return 0; }
    int s0 = rd(0x0003);
    wr(0x0003, 0x5A);                                              // the FPGA boards' check: the scratch register
    int s1 = rd(0x0003);
    if(s0 >= 0) wr(0x0003, (uint8_t)s0);
    printf("  0x%02X: ID %02X %02X %02X%s; 0x0003 %02X, 0x5A written, read %02X\n", a, i0, i1, i2,
           (i0 == 0xA2 && i1 == 0x20 && i2 == 0x0A) ? " (MS7210)" : " (not the MS7210's A2 20 0A)", s0 & 0xFF, s1 & 0xFF);
    return i0 == 0xA2 && i1 == 0x20 && i2 == 0x0A;
}
static int find(void) {
    if(id_at(0xB2) || id_at(0x56)) return 1;
    dev = 0xB2;
    return 0;
}

/* --- the LTDC as a G3 wants it --- */
static const struct { GPIO_TypeDef *port; uint8_t pin; } ltdc_pins[] = {   // HAL_LTDC_MspInit's, but PG7
    {GPIOE, 4}, {GPIOE, 5}, {GPIOE, 6}, {GPIOI, 9}, {GPIOI, 10}, {GPIOF, 10}, {GPIOC, 0}, {GPIOH, 2}, {GPIOH, 3},
    {GPIOB, 1}, {GPIOH, 8}, {GPIOH, 9}, {GPIOH, 10}, {GPIOG, 6}, {GPIOH, 13}, {GPIOH, 15}, {GPIOI, 0}, {GPIOI, 1},
    {GPIOI, 2}, {GPIOD, 6}, {GPIOG, 10}, {GPIOG, 11}, {GPIOG, 12}, {GPIOI, 4}, {GPIOI, 5}, {GPIOI, 6}, {GPIOI, 7}};
static int speed = 2;                                              // the pins' speed: 2 HIGH (the firmware has MEDIUM and LOW)
static const char *pg7_mode(void) {
    uint32_t m = GPIOG->MODER >> 14 & 3, af = GPIOG->AFR[0] >> 28;
    return m == 3 ? "analog" : m == 2 ? (af == 14 ? "LTDC_CLK" : af == 6 ? "SAI1_MCLK_A" : "another function") : "a GPIO";
}
static void pg7_ltdc(void) {                                       // PG7 as LTDC_CLK (AF14), very high speed
    GPIOG->AFR[0] = (GPIOG->AFR[0] & ~(0xFu << 28)) | (14u << 28);
    GPIOG->OSPEEDR |= 3u << 14;
    GPIOG->MODER = (GPIOG->MODER & ~(3u << 14)) | (2u << 14);
}
static void g3_ltdc(void) {                                        // over the firmware's: faster pins, PG7 driven, DE high
    for(unsigned i = 0; i < sizeof(ltdc_pins) / sizeof(ltdc_pins[0]); i++) {
        GPIO_TypeDef *p = ltdc_pins[i].port;
        int n = ltdc_pins[i].pin;
        p->OSPEEDR = (p->OSPEEDR & ~(3u << (2 * n))) | ((uint32_t)speed << (2 * n));
    }
    pg7_ltdc();
    LTDC->GCR |= LTDC_GCR_DEPOL;                                   // (GCR's polarities are not shadowed)
}
static uint32_t pixel_hz(void) {                                   // PLL3's R: 8 MHz / M * N / R (no FRACN)
    uint32_t m = (RCC->PLLCKSELR >> 20) & 0x3F, divr = RCC->PLL3DIVR;
    uint32_t n = (divr & 0x1FF) + 1, r = ((divr >> 24) & 0x7F) + 1;
    return m ? (uint32_t)(8000000ull * n / m / r) : 0;
}

/* --- the chip's set-up --- */
static int avi_vic = 0, avi_q = 0, avi_s = 1, avi_itc = 0, avi_rpt = 0;   // the FPGA table's: code 0, Q default, overscan
static void infoframe(void) {                                      // the AVI infoframe as raw bytes (the SDK's
    uint8_t pb[14] = {0};                                          // ms7210drv_hd_tx_shell_set_video_infoframe)
    int wide = avi_vic == 4 || avi_vic == 5 || avi_vic == 16 || avi_vic == 18 || avi_vic == 19 || avi_vic == 20;
    pb[1] = (uint8_t)(0x10 | (avi_s & 3));                         // RGB, active format present, the scan
    pb[2] = (uint8_t)((wide ? 0x80 : 0x40) | (wide ? 0x20 : 0x10) | 0x08);   // 709 or 601, 16:9 or 4:3, as the picture
    pb[3] = (uint8_t)((avi_itc ? 0x80 : 0) | (avi_q & 3) << 2);
    pb[4] = (uint8_t)avi_vic;
    pb[5] = (uint8_t)(avi_rpt & 0xF);
    uint8_t sum = 0x82 + 0x02 + 0x0D;
    for(int i = 1; i < 14; i++) sum = (uint8_t)(sum + pb[i]);
    pb[0] = (uint8_t)(0x100 - sum);
    wr(0x050E, 0x00);                                              // (as the FPGA table: the packet off while it is written)
    wr(0x050A, 0x82); wr(0x0509, 0x02); wr(0x050B, 0x0D);
    for(int i = 0; i < 14; i++) wr(0x050D, pb[i]);
    wr(0x050E, 0x40);
    printf("AVI: code %d, Q %d, S %d, ITC %d, repetition %d (bytes", avi_vic, avi_q, avi_s, avi_itc, avi_rpt);
    for(int i = 0; i < 6; i++) printf(" %02X", pb[i]);
    printf(")\n");
}
static void band(uint32_t hz) {                                    // the PHY's drive for the clock (the SDK's phy_init)
    int high = hz > 100000000, v;
    if((v = rd(0x0906)) >= 0) wr(0x0906, (uint8_t)(high ? v | 0x04 : v & ~0x04));
    if((v = rd(0x0920)) >= 0) wr(0x0920, (uint8_t)(high ? v | 0x40 : v & ~0x40));
    wr(0x0926, high ? 0xDD : 0x11); wr(0x0927, high ? 0x0D : 0x01);
    wr(0x0928, high ? 0x88 : 0x11); wr(0x0929, high ? 0x08 : 0x01);
}
static void trigger(void) { wr(0x0910, 0x01); ms(10); }            // the HDMI PLL re-triggered, 10 ms to settle
static const uint16_t table_a[][2] = {                             // the FPGA table's first 19 writes (0x1281 to 0x05C0)
    {0x1281, 0x04}, {0x0016, 0x04}, {0x0009, 0x01}, {0x0007, 0x09}, {0x0008, 0xF0}, {0x000A, 0xF0}, {0x0006, 0x11},
    {0x0531, 0x84}, {0x0900, 0x20}, {0x0901, 0x47}, {0x0904, 0x09}, {0x0923, 0x07}, {0x0924, 0x44}, {0x0925, 0x44},
    {0x090F, 0x80}, {0x091F, 0x07}, {0x0920, 0x1E}, {0x0018, 0x20}, {0x05C0, 0xFE}};
static void chip_init(int trim) {
    uint32_t hz = pixel_hz();
    nacks = 0;
    if(trim) { wr(0x0060, 0x81); wr(0x0061, 0x54); }              // the internal oscillator to 27 MHz (the SDK's)
    for(unsigned i = 0; i < sizeof(table_a) / sizeof(table_a[0]); i++) wr(table_a[i][0], (uint8_t)table_a[i][1]);
    ms(350);                                                       // (the FPGA boards wait 3.2M of their clocks here)
    wr(0x000B, 0x00);
    wr(0x0507, 0x06);                                              // muted
    band(hz);                                                      // (the FPGA table: the high band, for its 1080p)
    trigger();
    wr(0x000B, 0x11);
    infoframe();
    wr(0x0507, 0x00);                                              // unmuted
    wr(0x0019, 0x07);                                              // the pixel-clock meter on (the SDK's)
    printf("init for %.3f MHz (%s band)%s: %d writes not acknowledged\n", hz / 1e6, hz > 100000000 ? "high" : "low",
           trim ? ", oscillator set to 27 MHz" : "", nacks);
}

/* --- what the chip says --- */
static int pc8(void) {                                             // INT#, active low (27K to VCC): PC8 read as an input
    uint32_t m = GPIOC->MODER, p = GPIOC->PUPDR;
    GPIOC->PUPDR = p & ~(3u << 16);
    GPIOC->MODER = m & ~(3u << 16);
    cmm2->delay_us(5);
    int v = (GPIOC->IDR >> 8) & 1;
    GPIOC->MODER = m;
    GPIOC->PUPDR = p;
    return v;
}
static void status(void) {
    wr(0x0019, 0x07);                                              // (the meter on, in case the init has not been run)
    ms(20);
    int hpd = rd(0x0502), pll = rd(0x091C), st = rd(0x051D), lo = rd(0x001E), hi = rd(0x001F);
    int mute = rd(0x0507), pa = rd(0x1281), irq = rd(0x0500), mask = rd(0x0501), osc0 = rd(0x0060), osc1 = rd(0x0061);
    if(hpd < 0) { printf("status: the chip does not answer at 0x%02X\n", dev); return; }
    printf("HPD %s (0x0502 %02X), HDMI PLL %s (0x091C %02X), timing %s (0x051D %02X)\n", hpd & 1 ? "plugged" : "none",
           hpd, pll & 0x80 ? "locked" : "NOT locked", pll, st & 0x10 ? "stable" : "NOT stable", st);
    printf("pixel clock: the LTDC's %.3f MHz; the chip counts %.2f MHz (its meter, against its own oscillator: 0x0060-1 "
           "%02X %02X)\n", pixel_hz() / 1e6, ((hi << 8 | lo) * 2700u / 4096u) / 100.0, osc0, osc1);
    printf("mute 0x0507 %02X, sampling 0x1281 %02X, interrupt 0x0500 %02X mask 0x0501 %02X, INT# (PC8) %s\n", mute, pa,
           irq, mask, pc8() ? "high" : "LOW");
    printf("LTDC: DE active %s, clock %s, PG7 %s, %ux%u active\n", LTDC->GCR & LTDC_GCR_DEPOL ? "high" : "low (a G2's)",
           LTDC->GCR & LTDC_GCR_PCPOL ? "inverted" : "as it comes", pg7_mode(),
           (unsigned)(((LTDC->AWCR >> 16) & 0xFFF) - ((LTDC->BPCR >> 16) & 0xFFF)),
           (unsigned)((LTDC->AWCR & 0x7FF) - (LTDC->BPCR & 0x7FF)));
}

/* --- the test card --- */
static void card(const cmm2_video *v) {
    static uint8_t pal[64 * 3];
    memset(pal, 0, sizeof(pal));
    for(int k = 0; k < 24; k++) {                                  // 1-24: R7 ... R0, G7 ... G0, B7 ... B0, each alone
        uint32_t c = 1u << (23 - k);
        pal[(1 + k) * 3] = (uint8_t)(c >> 16); pal[(1 + k) * 3 + 1] = (uint8_t)(c >> 8); pal[(1 + k) * 3 + 2] = (uint8_t)c;
    }
    memset(&pal[25 * 3], 0xFF, 3);                                 // 25: full white
    memset(&pal[26 * 3], 0xE0, 3);                                 // 26: the firmware's white under OPTION VGA OUTPUT HIGH
    for(int i = 0; i < 16; i++) {                                  // 32-47: greys 0-15, 48-63: greys 240-255
        memset(&pal[(32 + i) * 3], i, 3);
        memset(&pal[(48 + i) * 3], 240 + i, 3);
    }
    cmm2->video_palette(0, 64, pal);
    uint32_t w = v->width, h = v->height;
    for(uint32_t y = 0; y < h; y++) {
        uint8_t *line = v->fb + y * v->pitch;
        for(uint32_t x = 0; x < w; x++) {
            uint8_t c;
            if(y < h / 2) c = (uint8_t)(1 + x * 24 / w);
            else if(y < h * 3 / 4) c = x < w / 3 ? 25 : x < 2 * w / 3 ? 26 : (x & 1 ? 0 : 25);
            else c = (uint8_t)(32 + x * 32 / w);
            line[x] = c;
        }
        if(v->copies == 2) memcpy(line + w, line, w);
    }
    SCB_CleanDCache_by_Addr((uint32_t *)((uint32_t)v->fb & ~31u), (int32_t)(v->pitch * h + 64));
}
static void wait_key(void) {
    printf("(any key to end)\n");
    while(cmm2->con_getc() < 0 && !cmm2->poll()) ms(10);
}
/* MODE m,8, the G3's settings again (setmode put the G2's back if the mode changed), and then what is written to the
   chip: 0 nothing, 1 the PLL re-trigger, 2 mute, band, re-trigger, infoframe, unmute; 3 the last if the clock changed */
static void show(int mode, int what) {
    cmm2_video v;
    uint32_t before = pixel_hz();
    if(cmm2->video_mode(mode, 8, &v) < 0) { printf("MODE %d,8 refused\n", mode); return; }
    g3_ltdc();
    uint32_t hz = pixel_hz();
    if(what == 3) what = hz != before ? 2 : 0;
    if(what == 1) trigger();
    if(what == 2) { wr(0x0507, 0x06); band(hz); trigger(); infoframe(); wr(0x0507, 0x00); }
    printf("MODE %d,8: %ux%u%s, %.3f MHz; written to the chip: %s\n", mode, (unsigned)v.width, (unsigned)v.height,
           v.copies == 2 ? " (lines twice)" : "", hz / 1e6, what == 2 ? "band, re-trigger, infoframe" : what ? "re-trigger" : "nothing");
    card(&v);
    ms(200);
    status();
    wait_key();
}

/* --- INT#: what drives it --- */
static void watch(int secs, int mask) {
    if(mask >= 0) { wr(0x0501, (uint8_t)mask); printf("0x0501 set to %02X\n", mask); }
    printf("watching INT# (PC8), 0x0500 and HPD for %d s: plug and unplug the cable (Ctrl-C ends it)\n", secs);
    uint64_t t0 = cmm2->micros(), next = 0;
    int lp = -1, la = -1, lh = -1;
    while(cmm2->micros() - t0 < (uint64_t)secs * 1000000 && !cmm2->poll()) {
        int p = pc8();
        double t = (cmm2->micros() - t0) / 1e6;
        if(p != lp) { printf("%7.2f s: INT# %s\n", t, p ? "high" : "LOW"); lp = p; }
        if(cmm2->micros() >= next) {
            int a = rd(0x0500), h = rd(0x0502);
            if(a != la || h != lh) { printf("%7.2f s: 0x0500 %02X, 0x0502 %02X (HPD %d)\n", t, a, h, h & 1); la = a; lh = h; }
            next = cmm2->micros() + 200000;
        }
        ms(2);
    }
    int a = rd(0x0500);
    if(a > 0) {                                                    // how it clears: a 1 written to each bit set, then 0s
        wr(0x0500, (uint8_t)a);
        int b = rd(0x0500);
        printf("0x0500 %02X, written back: now %02X, INT# %s\n", a, b, pc8() ? "high" : "LOW");
        if(b > 0) { wr(0x0500, 0); printf("0x0500 written 00: now %02X, INT# %s\n", rd(0x0500), pc8() ? "high" : "LOW"); }
    }
}

/* --- PG3, the chip's reset (27K to VCC) --- */
static void reset_chip(void) {
    uint32_t m = GPIOG->MODER, ot = GPIOG->OTYPER;
    GPIOG->BSRR = 1u << (3 + 16);
    GPIOG->OTYPER = ot & ~(1u << 3);
    GPIOG->MODER = (m & ~(3u << 6)) | (1u << 6);                   // PG3 driven low
    ms(20);
    printf("PG3 low for 20 ms (the chip should not answer):\n");
    find();
    GPIOG->BSRR = 1u << 3;
    ms(50);
    GPIOG->MODER = m;                                              // (the pull-up holds it high)
    GPIOG->OTYPER = ot;
    printf("PG3 released, 50 ms on (the chip's registers are now its defaults: `on` sets it up again):\n");
    find();
}

static int arg(int argc, char **argv, int i, int base, int dflt) {
    return i < argc ? (int)strtol(argv[i], NULL, base) : dflt;
}

int main(int argc, char **argv) {
    const char *c = argc > 1 ? argv[1] : "";
    i2c_open();
    if(!*c || !strcmp(c, "id")) {
        uint32_t m = GPIOG->MODER, afr = GPIOG->AFR[0], sp = GPIOG->OSPEEDR;
        printf("PG7 is %s. Without the pixel clock on PG7:\n", pg7_mode());
        GPIOG->MODER = m | (3u << 14);                             // analog: no clock
        ms(20);
        int a = find();
        printf("With the pixel clock (%.3f MHz) on PG7 as LTDC_CLK:\n", pixel_hz() / 1e6);
        pg7_ltdc();
        ms(20);
        int b = find();
        GPIOG->MODER = m; GPIOG->AFR[0] = afr; GPIOG->OSPEEDR = sp;
        printf("So the chip %s\n", a ? "answers without a clock" : b ? "answers only with the clock" : "does not answer");
        if(!*c && (a || b)) status();
    } else if(!strcmp(c, "reset")) {
        reset_chip();
    } else if(!strcmp(c, "on")) {
        int trim = !(argc > 2 && !strcmp(argv[2], "notrim"));
        if(!find()) {
            printf("trying with the clock on PG7\n");
            pg7_ltdc();
            ms(20);
            if(!find()) { printf("no MS7210\n"); i2c_close(); return 1; }
        }
        g3_ltdc();
        chip_init(trim);
        ms(100);
        status();
    } else if(!strcmp(c, "status")) {
        if(find()) status();
    } else if(!strcmp(c, "bars")) {
        if(find()) show(arg(argc, argv, 2, 10, 1), 3);
    } else if(!strcmp(c, "mode")) {
        const char *w = argc > 3 ? argv[3] : "full";
        if(find()) show(arg(argc, argv, 2, 10, 1), !strcmp(w, "none") ? 0 : !strcmp(w, "trig") ? 1 : 2);
    } else if(!strcmp(c, "int")) {
        if(find()) watch(arg(argc, argv, 2, 10, 30), arg(argc, argv, 3, 16, -1));
    } else if(!strcmp(c, "edge")) {
        if(find()) {
            if(argc > 2) LTDC->GCR = arg(argc, argv, 2, 10, 0) ? LTDC->GCR | LTDC_GCR_PCPOL : LTDC->GCR & ~LTDC_GCR_PCPOL;
            if(argc > 3) wr(0x1281, (uint8_t)(arg(argc, argv, 3, 10, 4) & 7));
            if(argc > 4) { speed = arg(argc, argv, 4, 10, 2) & 3; g3_ltdc(); }
            status();
        }
    } else if(!strcmp(c, "avi")) {
        if(find()) {
            avi_vic = arg(argc, argv, 2, 10, 0); avi_q = arg(argc, argv, 3, 10, 0);
            avi_s = arg(argc, argv, 4, 10, 1); avi_itc = arg(argc, argv, 5, 10, 0);
            infoframe();
        }
    } else if(!strcmp(c, "reg")) {
        if(find() && argc > 2) {
            uint16_t r = (uint16_t)arg(argc, argv, 2, 16, 0);
            if(argc > 3) {
                uint8_t w = (uint8_t)arg(argc, argv, 3, 16, 0);
                printf("0x%04X: %02X written%s\n", r, w, wr(r, w) ? ", not acknowledged" : "");
            }
            int v = rd(r);
            if(v < 0) printf("0x%04X: no answer\n", r);
            else printf("0x%04X reads %02X\n", r, v);
        }
    } else if(!strcmp(c, "dump")) {
        if(find() && argc > 2) {
            int r = arg(argc, argv, 2, 16, 0), n = arg(argc, argv, 3, 16, 16);
            for(int i = 0; i < n; i++) {
                if(i % 16 == 0) printf("%s0x%04X:", i ? "\n" : "", r + i);
                printf(" %02X", rd((uint16_t)(r + i)) & 0xFF);
            }
            printf("\n");
        }
    } else printf("ms7210test: id, reset, on, status, bars, mode, int, edge, avi, reg or dump\n");
    i2c_close();
    return 0;
}
