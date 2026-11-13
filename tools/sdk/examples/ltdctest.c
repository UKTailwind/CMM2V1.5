/* ltdctest.c: what the LTDC, SAI1 and DMA2D can do for the doubled modes on HDMI (docs/G3_HDMI_Plan.md), measured on a
   G1 or G2 without a monitor: the colour pins' input registers are read beside LTDC_CPSR, the line and pixel the LTDC is
   on, so each line's colour is known. MIT License, as cmm2img.h.

     RUN "ltdctest.app"            all the tests, in MODE 7,8
     RUN "ltdctest.app", name ...  only those named: base pitch0 lnr1 lnr240 cfbar cfbarmid double double2 window lif
                                   mclk mclk2 mclkdbg mclkdiv dma2d cost

   Each line of a test buffer is one colour: red is the buffer's line number (0-255) and green the marker &HA5, so the
   blanking (black) is told from the picture. The LTDC's registers, PLL3, PG7 and SAI1 are put back as they were.
   Built with: python build.py out/ltdctest.app examples/ltdctest.c --api 2
               -I ../../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I ../../Drivers/CMSIS/Include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"
#include "stm32h743xx.h"

#define W 320                                                      // MODE 7: 320 wide, each line shown twice
#define H 480                                                      // the lines the LTDC sends
#define MARK 0xA5
#define CX(c) ((c) >> 16)
#define CY(c) ((c) & 0xFFFF)

static cmm2_video v;
static int16_t seen[1300], seen2[1300];                            // the red seen on each line (CPSR's Y), and a second
static int ax0, ax1, ay0, ay1;                                     // value if it changed within the line; the picture
static uint8_t *A, *B, *S;                                         // area in CPSR's terms
static uint32_t save_cfbar, save_cfblr, save_cfblnr;

static inline uint32_t cpsr(void) { return LTDC->CPSR; }
static inline void pins(uint32_t *r, uint32_t *g) {
    uint32_t h = GPIOH->IDR, c = GPIOC->IDR, b = GPIOB->IDR, gg = GPIOG->IDR, e = GPIOE->IDR, i = GPIOI->IDR;
    *r = (h >> 2 & 1) | (h >> 3 & 1) << 1 | (h >> 8 & 1) << 2 | (h >> 9 & 1) << 3 | (h >> 10 & 1) << 4 | (c & 1) << 5 |
         (b >> 1 & 1) << 6 | (gg >> 6 & 1) << 7;                    // R0-R7: PH2 PH3 PH8 PH9 PH10 PC0 PB1 PG6
    *g = (e >> 5 & 1) | (e >> 6 & 1) << 1 | (h >> 13 & 1) << 2 | (gg >> 10 & 1) << 3 | (h >> 15 & 1) << 4 | (i & 1) << 5 |
         (i >> 1 & 1) << 6 | (i >> 2 & 1) << 7;                     // G0-G7: PE5 PE6 PH13 PG10 PH15 PI0 PI1 PI2
}
static void clean(void *p, uint32_t n) { SCB_CleanDCache_by_Addr((uint32_t *)((uint32_t)p & ~31u), (int32_t)n + 64); }
static void reload(uint32_t how) {                                 // LTDC_SRCR_IMR or LTDC_SRCR_VBR, and wait for it
    LTDC->SRCR = how;
    uint64_t t = cmm2->micros();
    while((LTDC->SRCR & (LTDC_SRCR_IMR | LTDC_SRCR_VBR)) && cmm2->micros() - t < 100000) ;
}
static void layer(uint8_t *fb, uint32_t pitch, uint32_t lines) {   // layer 1 at the next frame
    LTDC_Layer1->CFBAR = (uint32_t)fb;
    LTDC_Layer1->CFBLR = (pitch << 16) | (save_cfblr & 0x1FFF);
    LTDC_Layer1->CFBLNR = lines;
    reload(LTDC_SRCR_VBR);
    cmm2->video_wait();
}
static void restore(void) {
    LTDC_Layer1->CFBAR = save_cfbar;
    LTDC_Layer1->CFBLR = save_cfblr;
    LTDC_Layer1->CFBLNR = save_cfblnr;
    reload(LTDC_SRCR_VBR);
}

static void calibrate(void) {                                      // where the picture is in CPSR's terms
    ax0 = ay0 = 0xFFFF; ax1 = ay1 = 0;
    for(int k = 0; k < 400000; k++) {
        uint32_t c = cpsr(), r, g;
        pins(&r, &g);
        uint32_t c2 = cpsr();
        if(g != MARK || CY(c) != CY(c2)) continue;
        if((int)CX(c2) < ax0) ax0 = CX(c2);
        if((int)CX(c) > ax1) ax1 = CX(c);
        if((int)CY(c) < ay0) ay0 = CY(c);
        if((int)CY(c) > ay1) ay1 = CY(c);
    }
}

// one frame sampled: hook(y, x) is called at every sample (and may change the layer), the colours are recorded
static void (*hook)(int y, int x);
static uint32_t frame_isr;                                         // LTDC_ISR's FIFO underrun and transfer error bits
static void frame(void) {                                          // over the frame (the firmware's handler clears them)
    int last = -1, inside = 0;
    for(int y = 0; y < 1300; y++) seen[y] = seen2[y] = -1;
    for(;;) {                                                       // into the vertical blanking
        int y = CY(cpsr());
        if(y < ay0 || y > ay1) break;
    }
    __disable_irq();
    LTDC->ICR = LTDC_ICR_CFUIF | LTDC_ICR_CTERRIF;
    for(;;) {
        uint32_t c = cpsr(), r, g;
        int y = CY(c);
        if(hook) hook(y, CX(c));
        if(y != last) {
            last = y;
            if(y >= ay0 && y <= ay1) inside = 1;
            else if(inside) break;                                  // (the picture's end, past it or wrapped)
            continue;
        }
        pins(&r, &g);
        uint32_t c2 = cpsr();
        if(CY(c2) != (uint32_t)y || g != MARK || (int)CX(c) < ax0 + 6 || (int)CX(c2) > ax1 - 6) continue;
        if(seen[y] < 0) seen[y] = r;
        else if(seen[y] != (int)r) seen2[y] = r;
    }
    frame_isr = LTDC->ISR & (LTDC_ISR_FUIF | LTDC_ISR_TERRIF);
    __enable_irq();
}

static int (*expect)(int n);
static void report(const char *name) {
    int bad = 0, mixed = 0, unseen = 0, first = -1, runs = 0, n, s;
    for(n = 0; n <= ay1 - ay0; n++) {
        int y = ay0 + n;
        if(seen[y] < 0) { unseen++; continue; }
        if(seen2[y] >= 0) mixed++;
        if(expect && seen[y] != expect(n)) { bad++; if(first < 0) first = n; }
    }
    printf("%s: %d lines, %d unexpected", name, ay1 - ay0 + 1, bad);
    if(first >= 0) printf(" (first: line %d shows %d, expected %d)", first, seen[ay0 + first], expect(first));
    printf(", %d changed within the line, %d not seen\n", mixed, unseen);
    for(n = 0; n <= ay1 - ay0 && runs < 14; n = s + 1, runs++) {   // the lines as runs of a constant step
        int y = ay0 + n, d;
        s = n;
        if(n < ay1 - ay0 && seen[y] >= 0 && seen[y + 1] >= 0) {
            d = seen[y + 1] - seen[y];
            while(s < ay1 - ay0 && seen[ay0 + s] >= 0 && seen[ay0 + s + 1] >= 0 && seen[ay0 + s + 1] - seen[ay0 + s] == d &&
                  seen2[ay0 + s + 1] < 0) s++;
        } else d = 0;
        printf("   lines %3d-%3d: %3d..%3d step %d%s\n", n, s, seen[ay0 + n], seen[ay0 + s], d,
               seen2[ay0 + n] >= 0 ? " (changed within the line)" : "");
    }
    if(n <= ay1 - ay0) printf("   ...\n");
}

static int e_line(int n) { return n & 0xFF; }
static int e_zero(int n) { (void)n; return 0; }
static int e_255(int n) { (void)n; return 255; }
static int e_half(int n) { return n / 2; }

static int trig_y, trig_x, trig_done, trig_cx;
static void h_switch(int y, int x) {                               // to B at line trig_y, once x is reached
    if(!trig_done && y == trig_y && x >= trig_x) {
        trig_cx = x;
        LTDC_Layer1->CFBAR = (uint32_t)B;
        LTDC->SRCR = LTDC_SRCR_IMR;
        trig_done = 1;
    }
}
static int lead = 1, every2 = 0, hd_x = 0, hd_last = -1;
static void h_double(int y, int x) {                               // the line `lead` on from y is set up once x is reached
    if(y == hd_last || x < hd_x) return;
    hd_last = y;
    int n = y - ay0 + lead;
    if(n < 1 || n >= H) return;
    if(every2 && (n & 1)) return;                                  // (pitch 0 then shows the same line twice)
    LTDC_Layer1->CFBAR = (uint32_t)(S + (n / 2) * W);
    LTDC->SRCR = LTDC_SRCR_IMR;
}

// where in a line the LTDC's line interrupt comes: LIPCR moved to lines of the picture, its flag polled with the
// interrupts off (so the firmware's handler does not take it), CPSR read as it is set; LIPCR put back
static void lif(void) {
    uint32_t lipcr = LTDC->LIPCR;
    static const int at[] = {0, 1, 100, 200, 201, 479};
    printf("lif: the line interrupt (LIPCR) and CPSR when its flag is set (the picture is x %d-%d, y %d-%d)\n",
           ax0, ax1, ay0, ay1);
    for(unsigned k = 0; k < sizeof(at) / sizeof(at[0]); k++) {
        int target = ay0 + at[k], x[5], y[5];
        for(int rep = 0; rep < 5; rep++) {
            __disable_irq();
            LTDC->LIPCR = target;
            LTDC->ICR = LTDC_ICR_CLIF;
            while(CY(cpsr()) == (uint32_t)target) ;                   // (not already on the line)
            LTDC->ICR = LTDC_ICR_CLIF;
            uint32_t c = 0, n = 0;
            while(!(LTDC->ISR & LTDC_ISR_LIF) && ++n < 20000000) ;
            c = cpsr();
            LTDC->LIPCR = lipcr;
            LTDC->ICR = LTDC_ICR_CLIF;
            __enable_irq();
            x[rep] = n >= 20000000 ? -1 : (int)CX(c);
            y[rep] = (int)CY(c);
            cmm2->video_wait();
        }
        printf("   LIPCR %3d (picture line %3d): CPSR at the flag x %d %d %d %d %d, y %d %d %d %d %d\n", target, at[k],
               x[0], x[1], x[2], x[3], x[4], y[0], y[1], y[2], y[3], y[4]);
    }
    LTDC->LIPCR = lipcr;
}

static int want(int argc, char **argv, const char *name) {
    if(argc < 2) return 1;
    for(int i = 1; i < argc; i++) if(!strcmp(argv[i], name)) return 1;
    return 0;
}

// SAI1 block A as a master transmitter with nothing to send: the H743 puts MCLK out only while the block is enabled
// (CR1's MCKEN does not stick), at the kernel clock / MCKDIV (0: undivided)
static void sai_mclk(int div) {
    SAI1_Block_A->CR1 = 0;
    while(SAI1_Block_A->CR1 & SAI_xCR1_SAIEN) ;
    SAI1_Block_A->CR2 = 0;
    SAI1_Block_A->FRCR = 31 | (15UL << 8);                          // a 32-bit frame, FS 16 bits
    SAI1_Block_A->SLOTR = (1UL << 8) | (3UL << 16);                  // two slots, both on
    SAI1_Block_A->CR1 = ((uint32_t)div << SAI_xCR1_MCKDIV_Pos) | (4UL << 5);   // 16-bit data, master transmitter
    SAI1_Block_A->CR1 |= SAI_xCR1_SAIEN;
    cmm2->delay_us(200);
}
// SAI1's master clock on PG7 (AF6) from pll3_p_ck: its frequency counted, and its edges' place in the frame
static void mclk(int pll2x) {
    uint32_t d2 = RCC->D2CCIP1R, apb = RCC->APB2ENR, moder = GPIOG->MODER, afr = GPIOG->AFR[0], sp = GPIOG->OSPEEDR,
             cr1 = SAI1_Block_A->CR1, divr = RCC->PLL3DIVR;
    uint32_t cfgr = RCC->PLLCFGR;
    int n3 = (divr & 0x1FF) + 1, p3 = ((divr >> 9) & 0x7F) + 1, r3 = ((divr >> 24) & 0x7F) + 1;
    int restart = pll2x || !(cfgr & RCC_PLLCFGR_DIVP3EN);
    printf("PLL3: N %d, P %d, R %d (%.3f and %.3f MHz); PLLCFGR DIVP3EN %lu, DIVR3EN %lu\n", n3, p3, r3,
           8.0 * n3 / p3, 8.0 * n3 / r3, (unsigned long)((cfgr >> 22) & 1), (unsigned long)((cfgr >> 24) & 1));
    if(restart) {                                                  // P's output on (it can change only with PLL3 off),
        RCC->CR &= ~RCC_CR_PLL3ON;                                  // and with pll2x P = R/2: R 52 (16.0 MHz), P 26 (32.0)
        while(RCC->CR & RCC_CR_PLL3RDY) ;
        if(pll2x) RCC->PLL3DIVR = (divr & ~((0x7FUL << 9) | (0x7FUL << 24))) | ((26UL - 1) << 9) | ((52UL - 1) << 24);
        RCC->PLLCFGR = cfgr | RCC_PLLCFGR_DIVP3EN;
        RCC->CR |= RCC_CR_PLL3ON;
        while(!(RCC->CR & RCC_CR_PLL3RDY)) ;
        if(pll2x) p3 = 26, r3 = 52;
        printf("PLL3 restarted: P %d (%.3f MHz) output on, R %d (%.3f MHz)\n", p3, 8.0 * n3 / p3, r3, 8.0 * n3 / r3);
    }
    RCC->D2CCIP1R = (d2 & ~RCC_D2CCIP1R_SAI1SEL) | (2UL << RCC_D2CCIP1R_SAI1SEL_Pos);   // pll3_p_ck
    RCC->APB2ENR |= RCC_APB2ENR_SAI1EN;
    (void)RCC->APB2ENR;
    GPIOG->AFR[0] = (afr & ~(0xFUL << 28)) | (6UL << 28);
    GPIOG->OSPEEDR = sp | (3UL << 14);
    GPIOG->MODER = (moder & ~(3UL << 14)) | (2UL << 14);
    static const int divs[] = {0, 1, 2, 4, 10, 42, 63};
    for(unsigned k = 0; k < sizeof(divs) / sizeof(divs[0]); k++) {
        sai_mclk(divs[k]);
        uint32_t edges = 0, samples = 0, last = GPIOG->IDR & 0x80;
        uint64_t t0 = cmm2->micros();
        __disable_irq();
        while(samples < 2000000) {
            uint32_t now = GPIOG->IDR & 0x80;
            if(now && !last) edges++;
            last = now;
            samples++;
        }
        __enable_irq();
        uint64_t us = cmm2->micros() - t0;
        double f = edges / (double)us, fs = samples / (double)us;
        printf("MCKDIV %2d: %8.4f MHz counted (sampled at %.1f MHz%s); P/MCKDIV would be %.4f\n", divs[k], f, fs,
               f > fs / 3 ? ", too fast to count" : "", divs[k] ? 8.0 * n3 / p3 / divs[k] : 8.0 * n3 / p3);
    }
    // locked to the pixel clock? With MCKDIV = 21 R/P the clock's period is 21 pixels, and a frame is a whole number
    // of them (420 x 500 = 10000 x 21), so its rising edges fall at the same place in every frame if they are locked
    int md = 21 * r3 / p3, hist[21] = {0}, edges = 0;
    uint32_t tw = ((LTDC->TWCR >> 16) & 0xFFF) + 1;
    sai_mclk(md);
    uint32_t last = GPIOG->IDR & 0x80;
    __disable_irq();
    for(int k = 0; k < 3000000; k++) {
        uint32_t now = GPIOG->IDR & 0x80, c = cpsr();
        if(now && !last) { hist[(CY(c) * tw + CX(c)) % 21]++; edges++; }
        last = now;
    }
    __enable_irq();
    printf("MCKDIV %d (21 pixels): %d rising edges; their place in the frame, mod 21 pixels:\n  ", md, edges);
    for(int k = 0; k < 21; k++) printf("%d%s", hist[k], k < 20 ? " " : "\n");
    SAI1_Block_A->CR1 = 0;                                         // all as it was
    while(SAI1_Block_A->CR1 & SAI_xCR1_SAIEN) ;
    SAI1_Block_A->CR1 = cr1;
    GPIOG->MODER = moder;
    GPIOG->AFR[0] = afr;
    GPIOG->OSPEEDR = sp;
    RCC->APB2ENR = apb;
    RCC->D2CCIP1R = d2;
    if(restart) {
        RCC->CR &= ~RCC_CR_PLL3ON;
        while(RCC->CR & RCC_CR_PLL3RDY) ;
        RCC->PLL3DIVR = divr;
        RCC->PLLCFGR = cfgr;
        RCC->CR |= RCC_CR_PLL3ON;
        while(!(RCC->CR & RCC_CR_PLL3RDY)) ;
    }
}

// MCKDIV's meaning: SAI1 from pll3_p at 8 MHz (P 104), slow enough to count, timed by the cycle counter
static void mclkdiv(void) {
    uint32_t d2 = RCC->D2CCIP1R, apb = RCC->APB2ENR, moder = GPIOG->MODER, afr = GPIOG->AFR[0], sp = GPIOG->OSPEEDR,
             cr1 = SAI1_Block_A->CR1, divr = RCC->PLL3DIVR, cfgr = RCC->PLLCFGR;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    double hz = cmm2->cpu_hz();
    RCC->CR &= ~RCC_CR_PLL3ON;
    while(RCC->CR & RCC_CR_PLL3RDY) ;
    RCC->PLL3DIVR = (divr & ~(0x7FUL << 9)) | ((104UL - 1) << 9);
    RCC->PLLCFGR = cfgr | RCC_PLLCFGR_DIVP3EN;
    RCC->CR |= RCC_CR_PLL3ON;
    while(!(RCC->CR & RCC_CR_PLL3RDY)) ;
    RCC->D2CCIP1R = (d2 & ~RCC_D2CCIP1R_SAI1SEL) | (2UL << RCC_D2CCIP1R_SAI1SEL_Pos);
    RCC->APB2ENR |= RCC_APB2ENR_SAI1EN;
    (void)RCC->APB2ENR;
    GPIOG->AFR[0] = (afr & ~(0xFUL << 28)) | (6UL << 28);
    GPIOG->OSPEEDR = sp | (3UL << 14);
    GPIOG->MODER = (moder & ~(3UL << 14)) | (2UL << 14);
    static const int divs[] = {0, 1, 2, 3, 4, 10, 63};
    printf("mclkdiv: SAI1 kernel clock pll3_p = %.3f MHz, CPU %.0f MHz\n", 832.0 / 104, hz / 1e6);
    for(unsigned k = 0; k < sizeof(divs) / sizeof(divs[0]); k++) {
        sai_mclk(divs[k]);
        uint32_t edges = 0, last = GPIOG->IDR & 0x80, c0 = DWT->CYCCNT;
        __disable_irq();
        for(int n = 0; n < 2000000; n++) {
            uint32_t now = GPIOG->IDR & 0x80;
            if(now && !last) edges++;
            last = now;
        }
        __enable_irq();
        double s = (uint32_t)(DWT->CYCCNT - c0) / hz;
        printf("   MCKDIV %2d: %.4f MHz (sampled at %.1f MHz); 8/MCKDIV = %.4f\n", divs[k], edges / s / 1e6,
               2000000 / s / 1e6, 8.0 / (divs[k] ? divs[k] : 1));
    }
    SAI1_Block_A->CR1 = 0;
    while(SAI1_Block_A->CR1 & SAI_xCR1_SAIEN) ;
    SAI1_Block_A->CR1 = cr1;
    GPIOG->MODER = moder; GPIOG->AFR[0] = afr; GPIOG->OSPEEDR = sp;
    RCC->APB2ENR = apb; RCC->D2CCIP1R = d2;
    RCC->CR &= ~RCC_CR_PLL3ON;
    while(RCC->CR & RCC_CR_PLL3RDY) ;
    RCC->PLL3DIVR = divr;
    RCC->PLLCFGR = cfgr;
    RCC->CR |= RCC_CR_PLL3ON;
    while(!(RCC->CR & RCC_CR_PLL3RDY)) ;
}

// the DMA2D doubling an 8-bit page's pixels: L8 in through a CLUT whose entry i is the RGB565 colour whose two bytes
// are both i, RGB565 out, so each byte becomes two; each line written twice (two passes, every other line)
static void dma2d(void) {
    uint8_t *d = malloc(640 * 480 + 32), *src = v.fb;
    if(!d) { printf("dma2d: no memory\n"); return; }
    d = (uint8_t *)(((uint32_t)d + 31) & ~31u);
    volatile uint32_t *fgclut = (volatile uint32_t *)(DMA2D_BASE + 0x400);
    uint32_t ahb3 = RCC->AHB3ENR;
    printf("dma2d: its clock was %s\n", (ahb3 & RCC_AHB3ENR_DMA2DEN) ? "on" : "off");
    RCC->AHB3ENR = ahb3 | RCC_AHB3ENR_DMA2DEN;
    (void)RCC->AHB3ENR;
    while(DMA2D->CR & DMA2D_CR_START) ;
    for(int i = 0; i < 256; i++) {
        uint32_t r5 = i >> 3, g6 = ((i & 7) << 3) | (i >> 5), b5 = i & 31;
        fgclut[i] = (r5 << 3) << 16 | (g6 << 2) << 8 | (b5 << 3);
    }
    for(int y = 0; y < 240; y++) for(int x = 0; x < W; x++) src[y * 2 * W + x] = src[y * 2 * W + W + x] = (uint8_t)(x * 7 + y * 3);
    clean(src, 2 * W * 240);
    SCB_InvalidateDCache_by_Addr((uint32_t *)d, 640 * 480);
    uint64_t t0 = cmm2->micros();
    for(int pass = 0; pass < 2; pass++) {
        DMA2D->IFCR = 0x3F;
        DMA2D->FGPFCCR = (255UL << DMA2D_FGPFCCR_CS_Pos) | 5;          // L8, a 256-entry ARGB8888 CLUT
        DMA2D->OPFCCR = 2;                                          // RGB565
        DMA2D->FGMAR = (uint32_t)src;
        DMA2D->FGOR = W;                                            // (the page's second copy of each line skipped)
        DMA2D->OMAR = (uint32_t)(d + pass * 640);
        DMA2D->OOR = 320;                                           // (pixels of the output: every other line)
        DMA2D->NLR = (W << 16) | 240;
        DMA2D->CR = (1UL << 16) | DMA2D_CR_START;                   // memory to memory with pixel format conversion
        uint64_t t = cmm2->micros();
        while((DMA2D->CR & DMA2D_CR_START) && cmm2->micros() - t < 200000) ;
    }
    uint64_t us = cmm2->micros() - t0;
    uint32_t isr = DMA2D->ISR;
    RCC->AHB3ENR = ahb3;
    SCB_InvalidateDCache_by_Addr((uint32_t *)d, 640 * 480);
    int bad = 0;
    for(int y = 0; y < 480; y++) for(int x = 0; x < 640; x++) if(d[y * 640 + x] != src[(y / 2) * 2 * W + x / 2]) bad++;
    printf("dma2d: 320x240 L8 to 640x480 L8 in %lu us (ISR %08lx), %d bytes wrong of %d\n", (unsigned long)us,
           (unsigned long)isr, bad, 640 * 480);
}

// rising edges of PG7 counted for 1.5 million samples: MHz
static double pg7_mhz(double *fs) {
    uint32_t edges = 0, last = GPIOG->IDR & 0x80;
    uint64_t t0 = cmm2->micros();
    __disable_irq();
    for(int k = 0; k < 1500000; k++) {
        uint32_t now = GPIOG->IDR & 0x80;
        if(now && !last) edges++;
        last = now;
    }
    __enable_irq();
    uint64_t us = cmm2->micros() - t0;
    *fs = 1500000.0 / us;
    return edges / (double)us;
}
// why SAI1's MCLK does not reach PG7: the pin as an output, then SAI1 from pll3_p and pll1_q, with and without SAIEN
static void mclkdbg(void) {
    uint32_t d2 = RCC->D2CCIP1R, apb = RCC->APB2ENR, moder = GPIOG->MODER, afr = GPIOG->AFR[0], sp = GPIOG->OSPEEDR,
             cfgr = RCC->PLLCFGR;
    double fs, f;
    GPIOG->MODER = (moder & ~(3UL << 14)) | (1UL << 14);           // PG7 an output
    GPIOG->BSRR = 1UL << 7; cmm2->delay_us(10);
    int hi = (GPIOG->IDR >> 7) & 1;
    GPIOG->BSRR = 1UL << 23; cmm2->delay_us(10);
    int lo = (GPIOG->IDR >> 7) & 1;
    printf("mclkdbg: PG7 as an output reads %d when set, %d when cleared\n", hi, lo);
    if(!(cfgr & RCC_PLLCFGR_DIVP3EN)) {                             // P's output on
        RCC->CR &= ~RCC_CR_PLL3ON;
        while(RCC->CR & RCC_CR_PLL3RDY) ;
        RCC->PLLCFGR = cfgr | RCC_PLLCFGR_DIVP3EN;
        RCC->CR |= RCC_CR_PLL3ON;
        while(!(RCC->CR & RCC_CR_PLL3RDY)) ;
    }
    printf("   PLLCFGR %08lx (DIVP3EN %lu)\n", (unsigned long)RCC->PLLCFGR, (unsigned long)((RCC->PLLCFGR >> 22) & 1));
    RCC->APB2ENR |= RCC_APB2ENR_SAI1EN;
    (void)RCC->APB2ENR;
    GPIOG->AFR[0] = (afr & ~(0xFUL << 28)) | (6UL << 28);
    GPIOG->OSPEEDR = sp | (3UL << 14);
    GPIOG->MODER = (moder & ~(3UL << 14)) | (2UL << 14);
    static const int srcs[] = {2, 0};                               // pll3_p_ck, pll1_q_ck
    for(int s = 0; s < 2; s++) {
        RCC->D2CCIP1R = (d2 & ~RCC_D2CCIP1R_SAI1SEL) | ((uint32_t)srcs[s] << RCC_D2CCIP1R_SAI1SEL_Pos);
        for(int en = 0; en < 2; en++) {
            SAI1_Block_A->CR1 = 0;
            while(SAI1_Block_A->CR1 & SAI_xCR1_SAIEN) ;
            SAI1_Block_A->CR2 = 0;
            SAI1_Block_A->FRCR = 31 | (15UL << 8);                  // a 32-bit frame, FS 16 bits
            SAI1_Block_A->SLOTR = (1UL << 8) | (3UL << 16);          // two slots, both on
            SAI1_Block_A->CR1 = SAI_xCR1_MCKEN | (10UL << SAI_xCR1_MCKDIV_Pos) | (4UL << 5);   // 16-bit data, master TX
            if(en) SAI1_Block_A->CR1 |= SAI_xCR1_SAIEN;
            cmm2->delay_us(100);
            f = pg7_mhz(&fs);
            printf("   SAI1 from %s, SAIEN %d: CR1 %08lx SR %08lx, PG7 %.4f MHz (sampled at %.1f MHz)\n",
                   srcs[s] == 2 ? "pll3_p" : "pll1_q", en, (unsigned long)SAI1_Block_A->CR1,
                   (unsigned long)SAI1_Block_A->SR, f, fs);
        }
    }
    SAI1_Block_A->CR1 = 0;
    while(SAI1_Block_A->CR1 & SAI_xCR1_SAIEN) ;
    GPIOG->MODER = moder; GPIOG->AFR[0] = afr; GPIOG->OSPEEDR = sp;
    RCC->APB2ENR = apb; RCC->D2CCIP1R = d2;
    if(!(cfgr & RCC_PLLCFGR_DIVP3EN)) {
        RCC->CR &= ~RCC_CR_PLL3ON;
        while(RCC->CR & RCC_CR_PLL3RDY) ;
        RCC->PLLCFGR = cfgr;
        RCC->CR |= RCC_CR_PLL3ON;
        while(!(RCC->CR & RCC_CR_PLL3RDY)) ;
    }
}

// what doubling 16-bit and 8-bit pages costs: the DMA2D with lines of one pixel, and the CPU (both in the SDRAM)
static void cost(void) {
    uint16_t *s16 = malloc(W * H * 2 + 64), *d16 = malloc(640 * H * 2 + 64);
    if(!s16 || !d16) { printf("cost: no memory\n"); return; }
    s16 = (uint16_t *)(((uint32_t)s16 + 31) & ~31u); d16 = (uint16_t *)(((uint32_t)d16 + 31) & ~31u);
    for(int i = 0; i < W * H; i++) s16[i] = (uint16_t)(i * 2654435761u >> 16);
    clean(s16, W * H * 2);
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)d16, 640 * H * 2);
    uint32_t ahb3 = RCC->AHB3ENR;
    RCC->AHB3ENR = ahb3 | RCC_AHB3ENR_DMA2DEN;
    (void)RCC->AHB3ENR;
    uint64_t t0 = cmm2->micros();                                   // a doubled page is W x H pixels in a row; its pixel
    for(int pass = 0; pass < 2; pass++)                             // i goes to 2i and 2i + 1 of the 640-wide display
        for(int piece = 0; piece < 3; piece++) {
            DMA2D->IFCR = 0x3F;
            DMA2D->FGPFCCR = 2; DMA2D->OPFCCR = 2;                  // RGB565 to RGB565, no conversion
            DMA2D->FGMAR = (uint32_t)(s16 + piece * 51200); DMA2D->FGOR = 0;
            DMA2D->OMAR = (uint32_t)(d16 + 2 * piece * 51200 + pass); DMA2D->OOR = 1;
            DMA2D->NLR = (1UL << 16) | 51200;                       // 51,200 lines of one pixel
            DMA2D->CR = DMA2D_CR_START;                             // memory to memory
            uint64_t t = cmm2->micros();
            while((DMA2D->CR & DMA2D_CR_START) && cmm2->micros() - t < 500000) ;
        }
    uint64_t us = cmm2->micros() - t0;
    RCC->AHB3ENR = ahb3;
    SCB_InvalidateDCache_by_Addr((uint32_t *)d16, 640 * H * 2);
    int bad = 0;
    for(int i = 0; i < W * H; i++) if(d16[2 * i] != s16[i] || d16[2 * i + 1] != s16[i]) bad++;
    printf("cost: 16-bit, DMA2D with one-pixel lines: %lu us a frame, %d pixels wrong\n", (unsigned long)us, bad);
    memset(d16, 0, 640 * H * 2);
    t0 = cmm2->micros();
    uint32_t *w = (uint32_t *)d16;
    for(int i = 0; i < W * H; i += 4) {                             // the CPU: a word of two copies for each pixel
        uint32_t a = s16[i], b = s16[i + 1], c = s16[i + 2], d = s16[i + 3];
        w[i] = a | a << 16; w[i + 1] = b | b << 16; w[i + 2] = c | c << 16; w[i + 3] = d | d << 16;
    }
    SCB_CleanDCache_by_Addr((uint32_t *)d16, 640 * H * 2);
    us = cmm2->micros() - t0;
    bad = 0;
    for(int i = 0; i < W * H; i++) if(d16[2 * i] != s16[i] || d16[2 * i + 1] != s16[i]) bad++;
    printf("cost: 16-bit, the CPU: %lu us a frame, %d pixels wrong\n", (unsigned long)us, bad);
    uint8_t *s8 = (uint8_t *)s16;                                   // 8-bit: a halfword of two copies for each byte
    uint16_t *h8 = d16;
    t0 = cmm2->micros();
    for(int i = 0; i < W * H; i += 4) {
        uint32_t q = *(uint32_t *)(s8 + i), a = q & 0xFF, b = q >> 8 & 0xFF, c = q >> 16 & 0xFF, d = q >> 24;
        *(uint32_t *)(h8 + i) = (a | a << 8) | (b | b << 8) << 16;
        *(uint32_t *)(h8 + i + 2) = (c | c << 8) | (d | d << 8) << 16;
    }
    SCB_CleanDCache_by_Addr((uint32_t *)h8, 640 * H);
    us = cmm2->micros() - t0;
    bad = 0;
    for(int i = 0; i < W * H; i++) if(((uint8_t *)h8)[2 * i] != s8[i] || ((uint8_t *)h8)[2 * i + 1] != s8[i]) bad++;
    printf("cost: 8-bit, the CPU: %lu us a frame, %d pixels wrong\n", (unsigned long)us, bad);
}

int main(int argc, char **argv) {
    uint8_t rgb[768];
    if(cmm2->version < 2) { printf("services version %lu\n", (unsigned long)cmm2->version); return 1; }
    if(cmm2->video_mode(7, 8, &v) < 0) { printf("MODE 7,8 refused\n"); return 2; }
    for(int i = 0; i < 256; i++) { rgb[i * 3] = i; rgb[i * 3 + 1] = MARK; rgb[i * 3 + 2] = 0; }
    cmm2->video_palette(0, 256, rgb);
    save_cfbar = LTDC_Layer1->CFBAR; save_cfblr = LTDC_Layer1->CFBLR; save_cfblnr = LTDC_Layer1->CFBLNR;
    printf("LTDC: GCR %08lx SSCR %08lx BPCR %08lx AWCR %08lx TWCR %08lx; layer 1 CR %08lx PFCR %lu CFBAR %08lx "
           "CFBLR %08lx CFBLNR %lu\n", (unsigned long)LTDC->GCR, (unsigned long)LTDC->SSCR, (unsigned long)LTDC->BPCR,
           (unsigned long)LTDC->AWCR, (unsigned long)LTDC->TWCR, (unsigned long)LTDC_Layer1->CR,
           (unsigned long)LTDC_Layer1->PFCR, (unsigned long)save_cfbar, (unsigned long)save_cfblr,
           (unsigned long)save_cfblnr);
    A = malloc(W * H + 64); B = malloc(W * H + 64); S = malloc(W * 240 + 64);
    if(!A || !B || !S) { printf("no memory\n"); return 3; }
    A = (uint8_t *)(((uint32_t)A + 63) & ~63u); B = (uint8_t *)(((uint32_t)B + 63) & ~63u);
    S = (uint8_t *)(((uint32_t)S + 63) & ~63u);
    for(int y = 0; y < H; y++) { memset(A + y * W, y & 0xFF, W); memset(B + y * W, 255 - (y & 0xFF), W); }
    for(int y = 0; y < 240; y++) memset(S + y * W, y, W);
    clean(A, W * H); clean(B, W * H); clean(S, W * 240);

    layer(A, W, H);
    calibrate();
    printf("the picture in CPSR's terms: x %d-%d, y %d-%d\n", ax0, ax1, ay0, ay1);
    if(ay1 - ay0 + 1 != H) { printf("not %d lines: stopped\n", H); restore(); return 4; }

    printf("LTDC IER %08lx (FIFO underrun interrupt 2, transfer error 4: the firmware's handler clears their flags)\n",
           (unsigned long)LTDC->IER);
    if(want(argc, argv, "base")) {
        layer(A, W, H); hook = NULL; expect = e_line; frame(); report("base (A, pitch 320)");
        printf("   FIFO underrun %lu, transfer error %lu in the frame\n", (unsigned long)(frame_isr >> 1 & 1),
               (unsigned long)(frame_isr >> 2 & 1));
    }
    for(int k = 0; k < 4; k++) {                                   // B: line 0 is 255, so a line the LTDC did not fetch
        static const char *const nm[] = {"pitch0", "lnr1", "lnr240", "double"};   // (pixel 0) is told from it
        if(k == 3 || !want(argc, argv, nm[k])) continue;
        if(k == 0) layer(B, 0, H);
        else layer(B, W, k == 1 ? 1 : 240);
        hook = NULL; expect = e_255; frame();
        report(k == 0 ? "pitch0 (B, pitch 0, 480 lines)" : k == 1 ? "lnr1 (B, pitch 320, CFBLNR 1)" :
               "lnr240 (B, pitch 320, CFBLNR 240)");
        printf("   FIFO underrun %lu, transfer error %lu in the frame\n", (unsigned long)(frame_isr >> 1 & 1),
               (unsigned long)(frame_isr >> 2 & 1));
    }
    if(want(argc, argv, "cfbar") || want(argc, argv, "cfbarmid")) {
        for(int mid = 0; mid < 2; mid++) {
            if(!want(argc, argv, mid ? "cfbarmid" : "cfbar")) continue;
            layer(A, W, H);
            trig_y = ay0 + 200; trig_x = mid ? (ax0 + ax1) / 2 : 0; trig_done = 0; hook = h_switch; expect = NULL;
            frame();
            printf("(CFBAR to B with IMR on line 200, at x %d; A's line n is n, B's is 255 - n)\n", trig_cx);
            report(mid ? "cfbarmid" : "cfbar");
            hook = NULL;
            frame();
            report("   the frame after");
        }
    }
    for(int k = 0; k < 2; k++) {
        if(!want(argc, argv, k ? "double2" : "double")) continue;
        every2 = k;
        for(lead = 1; lead <= 2; lead++) {
            for(int mid = 0; mid < 2; mid++) {
                if(mid && lead == 1) continue;
                hd_x = mid ? (ax0 + ax1) / 2 : 0;
                LTDC_Layer1->CFBAR = (uint32_t)S;
                layer(S, 0, H); hook = h_double; expect = e_half; hd_last = -1;
                frame();
                char name[100];
                snprintf(name, sizeof(name), "%s (S, pitch 0, set %d line%s ahead, at x %d)", k ? "double2, every 2nd line" :
                         "double, every line", lead, lead > 1 ? "s" : "", hd_x);
                report(name);
                printf("   FIFO underrun %lu, transfer error %lu in the frame\n", (unsigned long)(frame_isr >> 1 & 1),
                       (unsigned long)(frame_isr >> 2 & 1));
                hook = NULL;
            }
        }
    }
    if(want(argc, argv, "window")) {                               // where in line 200 a change of CFBAR takes effect
        printf("window: CFBAR to B (IMR) on line 200 once x is reached; lines 197-205: A old, B new, M changed within\n");
        for(int xt = 0; xt < 420; xt += (xt >= 60 && xt < 400 ? 40 : 10)) {
            layer(A, W, H);
            trig_y = ay0 + 200; trig_x = xt; trig_done = 0; hook = h_switch; expect = NULL;
            frame();
            hook = NULL;
            char line[16];
            for(int n = 197; n <= 205; n++) {
                int y = ay0 + n;
                line[n - 197] = seen2[y] >= 0 ? 'M' : seen[y] == (n & 0xFF) ? 'A' : seen[y] == 255 - (n & 0xFF) ? 'B' : '?';
            }
            line[9] = 0;
            printf("   x %3d (written at %3d): %s\n", xt, trig_cx, line);
        }
    }
    restore();
    cmm2->video_wait();
    if(want(argc, argv, "lif")) lif();
    if(want(argc, argv, "mclk")) mclk(0);
    if(want(argc, argv, "mclk2")) mclk(1);
    if(want(argc, argv, "dma2d")) dma2d();
    if(want(argc, argv, "mclkdbg")) mclkdbg();
    if(want(argc, argv, "mclkdiv")) mclkdiv();
    if(want(argc, argv, "cost")) cost();
    return 0;
}
