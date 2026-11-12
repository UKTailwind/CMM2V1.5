/* bbc_cmm2.c: frank-micro's drivers on the Colour Maximite 2, by the loader's services (Peter, 11 Nov 2026). This file has what frank-micro's
   frank_platform.c, frank_gui.c and frank_audio.c give the rest of it (those files are the RP2350's start-up, its HDMI picture and its HDMI, I2S and PWM
   sound, and are not built): main, which starts B-em; the display; the sound; the USB keyboard and gamepads as frank-micro's USB HID driver hands them
   over; and the pace of the frames.

   - The display: one program for both boards (Peter, 12 Nov 2026: "go for the single app"), the board told at run time (a G2 runs it where it was
     linked). B-em draws each of the BBC's 256 lines as a row of 640 pixels (RGB555), and each is kept whole here (frank-micro's 320 x 256 halves
     MODE 0's): the BBC's eight colours as frank-micro has them (palette 0-7, which its monitor setting changes), MODE 7's teletext as frank-micro
     samples it, the six columns of each SAA5050 character at full intensity, each twice across. The 640 x 256 picture is shown at 640 x 512, each
     line twice, in 576p at 50 Hz on both boards (the BBC's own rate, each frame shown once, as the Amiga does). A G2: MODE 1,16's page with the
     display's timing changed, three pages flipped in the blanking, the lines put through the palette as RGB565 and copied by the DMA2D as the
     frame goes on. A G1: MODE 1,8's page (in its internal RAM) with the same timing, each frame drawn into one of three 8-bit pictures and the
     one last finished copied to the page in the blanking, so it is never shown half drawn. frank-micro's menus (F11 the discs, F12 the settings) draw at 320 x 256, over the picture, each pixel twice
     across.
   - The sound: B-em's SN76489 at 31,250 samples a second goes through a ring to the firmware's sound stream, at its rate by a phase step trimmed to
     hold the ring's level: the emulator keeps its own time, 50 frames a second.
   - The keyboard: the USB keyboard's keys as they are pressed and let go (its raw reports), through frank-micro's own USB path, so that frank-micro's
     keys work: F11 the disc menu, F12 the settings, Print Screen a screenshot (A:/bbc/screenshot), Ctrl+Alt+Delete starts the BBC again. What is on a
     key is what the BBC types, in any OPTION USBKEYBOARD layout (keys_by_character); CAPS LOCK is on Caps Lock, CTRL on Ctrl, COPY on End, f0-f9 on
     F1-F10, as B-em puts them.
     Pause/Break is the BBC's BREAK (Shift+Break boots the disc in drive 0). Ctrl+Alt+End leaves, as it does the other emulators.
   - The joystick: the first USB gamepad, as frank-micro has it, keys of the BBC by its gamepad setting (cursor keys, or Z X : / and RETURN).
   - The serial console types too, for remote use: each character as the BBC's keyboard makes it (" is SHIFT and 2, * SHIFT and :, small letters
     SHIFT and the letter, as with CAPS LOCK on), Return as RETURN, Backspace and Delete as DELETE, Ctrl+letter as CTRL and the letter; Ctrl-F then 0-9
     for f0-f9, u d l r for the cursor keys, p n for Page Up and Down, b for BREAK, B for CTRL and BREAK, m for the disc menu, s for the settings
     (Return, Escape and those keys are the menus' while one is up); Ctrl-] leaves.
   - The ROMs: frank-micro's are built in; the .rom and .bin files of A:/bbc/roms go into the free sideways RAM banks as the BBC starts (roms_load).
   - The discs: RUN "bbc.app", disc [disc] puts them in drives 0 and 1 (and SHIFT-BREAKs), F11's menu others. What the BBC writes to a disc goes
     to its image on the SD card at once (patches/0002), unless the file is read only or RUN's -ro is given: then the disc is write protected.
   RUN "bbc.app", -stats prints the frames a second, how much of the time the emulator has to spare, the sound ring's level and the heap in use, every
   5 s. */
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmm2.h"
#include "x_gui.h"
#include "HDMI.h"
#include "ff.h"
#include "frank_gui.h"
#include "frank_ui.h"
#include "frank_disc.h"
#include "frank_loader.h"
#include "frank_settings.h"
#include "frank_screenshot.h"
#include "usbhid_wrapper.h"
#include "nespad.h"

extern int _al_mangled_main(int argc, char **argv);   /* B-em's main (main.c: #define main _al_mangled_main) */
extern void key_down(int code);
extern void key_up(int code);
extern void m6502_reset(void);
extern void frank_gamepad_init(void);
extern bool g_frank_disc_readonly;              /* (frank_disc.c, patches/0002: the disc images opened for reading only) */

static bool has_arg(const char *a) {
    for(int i = 1; i < cmm2_start->argc; i++) if(strcmp(cmm2_start->argv[i], a) == 0) return true;
    return false;
}

/* where B-em reads an empty ROM slot (mem.c's g_garbage_read: the RP2350's flash) */
uint8_t cmm2_garbage[0x4000];

/* the Pico SDK's panic: B-em's, when it cannot go on */
void panic(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "bbc: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(3);
}

void watchdog_reboot(uint32_t pc, uint32_t sp, uint32_t delay_ms) { (void)pc; (void)sp; (void)delay_ms; exit(0); }   /* (not called: patches/) */

/* ---- the time ---- */
uint64_t time_us_64(void) { return cmm2->micros(); }
void sleep_us(uint64_t us) { usleep((useconds_t)us); }
void sleep_ms(uint32_t ms) { usleep((useconds_t)ms * 1000); }
void tight_loop_contents(void) { }

/* -stats: the frames, the time the emulator had to spare and the sound ring, since the last report */
static int stats;
static uint32_t stat_frames;
static uint64_t stat_idle, stat_from;
static volatile uint32_t ring_in, ring_out;     /* (the sound's: ring_out in samples, ring_phase its fraction) */
static int stat_peak;                           /* (the loudest of B-em's samples since the last report) */
static int audio_on;                            /* (the sound stream started) */

static void stat_frame(void) {
    uint64_t now = cmm2->micros();
    if(!stats) return;
    stat_frames++;
    if(now - stat_from >= 5000000) {
        uint32_t ms = (uint32_t)((now - stat_from) / 1000), idle = (uint32_t)(stat_idle / 1000);
        printf("bbc: %lu.%lu frames a second, %lu%% spare, sound ring %lu (peak %d, audio %d), heap %lu\n", (unsigned long)(stat_frames * 1000 / ms),
               (unsigned long)(stat_frames * 10000 / ms % 10), (unsigned long)(idle * 100 / ms), (unsigned long)(ring_in - ring_out), stat_peak,
               audio_on, (unsigned long)mallinfo().uordblks);
        stat_peak = 0;
        stat_frames = 0;
        stat_idle = 0;
        stat_from = now;
    }
}

/* ---- the display ---- */
#define PIC_W 640                               /* the BBC's picture: 256 lines, each shown twice */
#define PIC_H 256
#define UI_W 320                                /* frank-micro's menus: 320 x 256, over the picture */
#define UI_H 256
#define UI_CLEAR 255                            /* (a colour the menus do not use: where the picture shows through) */
static cmm2_video video;
static bool g2;                                 /* a G2: it runs the program where it was linked (CMM2IMG_BASE), where a G1 moves it */
static uint8_t __attribute__((aligned(4))) ui_fb[UI_W * UI_H];
static bool ui_frame;                           /* (the menus are drawn over this frame: ui_fb made at its start) */
static uint8_t palette[256 * 3], palette_shown[256 * 3];
static volatile bool palette_new;
static uint16_t pal565[256];                    /* (the G2's: the palette as RGB565) */
uint8_t *SCREEN[2];                             /* (frank_gui.h's: not used by the rest) */
volatile uint32_t current_buffer;

/* the G1, in 576p too (Peter, 12 Nov 2026: "It would be better to have them both in 576P"): MODE 1,8's page (in the internal RAM, as a G1
   shows it) with the G2's timing set over it, the layer cut to the picture, which is the page's first 640 x 512 bytes. The picture is made in
   one of three 8-bit buffers in turn, and the one last finished put on the page, each line twice, at the start of the display's blanking by the
   DMA2D (the even lines, then the odd, both ahead of the display), so it is never shown half drawn. A frame's end waits for the one before it
   to be taken, so the emulation goes at the display's 50 a second as on the G2; the buffer made next is the one taken two blankings before,
   its copy long done */
#define NPIC 3
static uint8_t *pic8[NPIC];
static int pic_n;                               /* the buffer being made */
static const void *volatile ready;              /* a whole picture not yet on the page */

static void frame8(void) {
    const void *s = ready;
    if(s == NULL) return;
    ready = NULL;
    cmm2->dma2d_copy(s, PIC_W, video.fb, 2 * PIC_W, PIC_W, PIC_H);
    cmm2->dma2d_copy(s, PIC_W, video.fb + PIC_W, 2 * PIC_W, PIC_W, PIC_H);
}

/* the G2, as the Amiga's (apps/amiga): 576p (720 x 576 at 50.00 Hz, the BBC's own rate, each frame shown once) set over MODE 1,16's page (in the
   SDRAM), the layer cut to the 640 x 512 picture in its middle. Three pages: the shown one, one waiting to be (flipped to in the blanking by
   the frame hook) and the one being made. Each line, through the palette as RGB565 (with the menus over it), goes into one of two batches of
   22, and a full batch to the page being made by two DMA2D copies, each line on the page's even line and its odd one, while the emulation
   goes on. The 8-bit picture frank-micro's screenshot takes is made only for a frame that is to be saved */
static uint8_t *shot;
static bool shot_frame;                         /* (this frame's 8-bit picture is being made) */

#define LTDC_SSCR     (*(volatile uint32_t *)0x50001008u)
#define LTDC_BPCR     (*(volatile uint32_t *)0x5000100Cu)
#define LTDC_AWCR     (*(volatile uint32_t *)0x50001010u)
#define LTDC_TWCR     (*(volatile uint32_t *)0x50001014u)
#define LTDC_GCR      (*(volatile uint32_t *)0x50001018u)
#define LTDC_SRCR     (*(volatile uint32_t *)0x50001024u)
#define LTDC_BCCR     (*(volatile uint32_t *)0x5000102Cu)
#define LTDC_CPSR     (*(volatile uint32_t *)0x50001044u)
#define LTDC_L1WHPCR  (*(volatile uint32_t *)0x50001088u)
#define LTDC_L1WVPCR  (*(volatile uint32_t *)0x5000108Cu)
#define LTDC_L1CFBAR  (*(volatile uint32_t *)0x500010ACu)
#define LTDC_L1CFBLR  (*(volatile uint32_t *)0x500010B0u)
#define LTDC_L1CFBLNR (*(volatile uint32_t *)0x500010B4u)
#define RCC_CR        (*(volatile uint32_t *)0x58024400u)
#define RCC_PLL3DIVR  (*(volatile uint32_t *)0x58024440u)
#define SCR_W 720
#define SCR_H 576
#define PW PIC_W                                /* the page: the picture's 640 x 512 */
#define PH (2 * PIC_H)
#define NPAGES 3
static uint16_t *page[NPAGES];
static int npages = 1;
static volatile int shown, flip_to = -1;
static volatile uint32_t issued;                /* the copies given to the DMA2D, ever */
static volatile uint32_t frame_mark;            /* issued, once the copies of the page waiting to be shown were */
static uint32_t completed(void) { const uint32_t i = issued; return i - (uint32_t)cmm2->dma2d_busy(); }

/* the start of each frame's blanking (PendSV): a finished page shown (the Amiga's hook) */
static void hook(void) {
    if(flip_to >= 0 && (int32_t)(completed() - frame_mark) >= 0) {
        LTDC_L1CFBAR = (uint32_t)page[flip_to];
        LTDC_SRCR = 1;                          /* (reload now: in the blanking) */
        shown = flip_to;
        flip_to = -1;
    }
}

/* 576p: CEA-861's 864 x 625 at 27.000 MHz, both syncs low; PLL3 (the pixel clock) 8 MHz x 81 / 24, its input as MODE 1 has it. The
   firmware's frame interrupt stays at MODE 1's line 624, 576p's last, in its blanking. All put back at the end (the Amiga's timing_576p) */
static uint32_t tm_saved[6], ltdc_saved[6];
static int tm_set, window_set;
static int pll3_set(uint32_t divr) {
    RCC_CR &= ~(1u << 28);
    for(int i = 0; RCC_CR & (1u << 29); i++) if(i > 1000000) return 0;
    RCC_PLL3DIVR = divr;
    RCC_CR |= 1u << 28;
    for(int i = 0; !(RCC_CR & (1u << 29)); i++) if(i > 1000000) return 0;
    return 1;
}
static void blank_wait(void) {                  /* (the LTDC's line past the picture, or 40 ms) */
    const uint64_t end = cmm2->micros() + 40000;
    while(cmm2->micros() < end && (LTDC_CPSR & 0xFFFF) <= (LTDC_AWCR & 0x7FF)) { }
}
static int timing_576p(void) {
    tm_saved[0] = LTDC_SSCR; tm_saved[1] = LTDC_BPCR; tm_saved[2] = LTDC_AWCR; tm_saved[3] = LTDC_TWCR;
    tm_saved[4] = LTDC_GCR; tm_saved[5] = RCC_PLL3DIVR;
    const uint32_t hsw = 64, hbp = 68, hfp = 12, vsw = 5, vbp = 39, vfp = 5, w = SCR_W, h = SCR_H;
    const uint32_t ahbp = hsw + hbp - 1, avbp = vsw + vbp - 1, aaw = ahbp + w, aah = avbp + h;
    blank_wait();
    if(!pll3_set((tm_saved[5] & ~(0x1FFu | (0x7Fu << 24))) | (81u - 1) | ((24u - 1) << 24))) { pll3_set(tm_saved[5]); return 0; }
    LTDC_SSCR = (hsw - 1) << 16 | (vsw - 1);
    LTDC_BPCR = ahbp << 16 | avbp;
    LTDC_AWCR = aaw << 16 | aah;
    LTDC_TWCR = (aaw + hfp) << 16 | (aah + vfp);
    LTDC_GCR &= ~(3u << 30);                    /* (HSync and VSync active low) */
    LTDC_SRCR = 1;
    tm_set = 1;
    return 1;
}
static void timing_back(void) {
    if(!tm_set) return;
    blank_wait();
    pll3_set(tm_saved[5]);
    LTDC_SSCR = tm_saved[0]; LTDC_BPCR = tm_saved[1]; LTDC_AWCR = tm_saved[2]; LTDC_TWCR = tm_saved[3];
    LTDC_GCR = tm_saved[4];
    LTDC_SRCR = 1;
    tm_set = 0;
}
static void window_on(uint32_t base, uint32_t bpp) {   /* the layer cut to the picture (bpp bytes a pixel), shown from base, the border black */
    ltdc_saved[0] = LTDC_L1WHPCR; ltdc_saved[1] = LTDC_L1WVPCR; ltdc_saved[2] = LTDC_L1CFBAR;
    ltdc_saved[3] = LTDC_L1CFBLR; ltdc_saved[4] = LTDC_L1CFBLNR; ltdc_saved[5] = LTDC_BCCR;
    const uint32_t ahbp = LTDC_BPCR >> 16 & 0xFFF, avbp = LTDC_BPCR & 0x7FF;
    const uint32_t x0 = ahbp + 1 + (SCR_W - PW) / 2, y0 = avbp + 1 + (SCR_H - PH) / 2;
    LTDC_L1WHPCR = (x0 + PW - 1) << 16 | x0;
    LTDC_L1WVPCR = (y0 + PH - 1) << 16 | y0;
    LTDC_L1CFBLR = (uint32_t)(PW * bpp) << 16 | (PW * bpp + 7);
    LTDC_L1CFBLNR = PH;
    LTDC_L1CFBAR = base;
    LTDC_BCCR = 0;
    LTDC_SRCR = 1;
    window_set = 1;
}
static void window_off(void) {
    if(!window_set) return;
    LTDC_L1WHPCR = ltdc_saved[0]; LTDC_L1WVPCR = ltdc_saved[1]; LTDC_L1CFBAR = ltdc_saved[2];
    LTDC_L1CFBLR = ltdc_saved[3]; LTDC_L1CFBLNR = ltdc_saved[4]; LTDC_BCCR = ltdc_saved[5];
    LTDC_SRCR = 1;
    window_set = 0;
}

/* the lines put out as the frame goes on (the Amiga's batches) */
#define CH 22
static uint16_t *batch[2];
static uint32_t buf_mark[2];                    /* issued, once the last copies from each batch were */
static int back_pg = 1;                         /* the page the frame goes to */

static void copy_out(const uint16_t *src, int row0, int n) {
    for(int k = 0; k < 2; k++) {
        while(cmm2->dma2d_busy() >= 3) { }
        if(k && cmm2->version >= 18)            /* (the second copy needs no write-back from the cache: the first's did it) */
            cmm2->dma2d_copy_nc(src, PW * 2, page[back_pg] + (2 * row0 + k) * PW, 2 * PW * 2, PW * 2, (uint32_t)n);
        else
            cmm2->dma2d_copy(src, PW * 2, page[back_pg] + (2 * row0 + k) * PW, 2 * PW * 2, PW * 2, (uint32_t)n);
        issued++;
    }
}

/* the frame's end: its page shown at a blanking once copied, after the one before it (so the emulation goes at the display's 50 a second,
   unless frank-micro's Limit Speed is off); the next made in the page neither shown nor waiting */
static void present(void) {
    uint64_t t0 = cmm2->micros();
    if(g_frank_limit_speed) while(flip_to >= 0) { }
    frame_mark = issued;
    __asm volatile ("dmb 0xF" ::: "memory");
    flip_to = back_pg;
    if(npages == 2) while(flip_to >= 0) { }    /* (two pages: the next frame's is the one shown till then) */
    int pg = 0;
    while(pg == shown || pg == flip_to) pg++;
    back_pg = pg;
    if(stats) stat_idle += cmm2->micros() - t0;
}

static void display_end(void) {                 /* (at exit: MMBasic's mode as it was) */
    cmm2->frame_hook(NULL);
    while(cmm2->dma2d_busy()) { }
    if(!g2 && video.fb)                         /* (a G1's picture is in MMBasic's own page: left there, MMBasic's palette showed it in blue, Peter) */
        memset(video.fb, 0, (size_t)video.pitch * video.height);
    window_off();
    timing_back();
}

/* the G2: write-allocate (RUN's -nowa: not), as the Amiga has it (apps/amiga): the program's 8 MB (the loader's MPU region at 0xD1800000, where
   the program, B-em's memory and the lines are) write-back with write-allocate (TEX 1) for the run, in place of no write-allocate (TEX 0), so
   that a store that misses the cache brings the line in. Put back, the cache written back, at the end */
#define MPU_RNR  (*(volatile uint32_t *)0xE000ED98u)
#define MPU_RBAR (*(volatile uint32_t *)0xE000ED9Cu)
#define MPU_RASR (*(volatile uint32_t *)0xE000EDA0u)
#define DCCMVAC  (*(volatile uint32_t *)0xE000EF68u)
static int wa_region = -1;
static uint32_t wa_rasr;
static void wa_on(void) {
    for(int r = 0; r < 16; r++) {
        MPU_RNR = (uint32_t)r;
        if((MPU_RBAR & ~0x1Fu) == CMM2IMG_BASE && (MPU_RASR & 1u)) wa_region = r;
    }
    if(wa_region < 0) return;
    MPU_RNR = (uint32_t)wa_region;
    wa_rasr = MPU_RASR;
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RASR = (wa_rasr & ~(7u << 19)) | (1u << 19);
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
}
static void wa_off(void) {
    if(wa_region < 0) return;
    for(uint32_t a = CMM2IMG_BASE; a < CMM2IMG_BASE + CMM2IMG_AREA; a += 32) DCCMVAC = a;
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RNR = (uint32_t)wa_region;
    MPU_RASR = wa_rasr;
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
    wa_region = -1;
}

/* the G2: the processor's AXI RAM (512 KB at 0x24000000, the firmware's MPU region 12) ordinary cached memory for the run, as crt0.c's
   cmm2_axi_load makes it, once MODE 1,16 has its page in the SDRAM: the line batches there, so that the DMA2D reads them from it and only
   writes the SDRAM (from the SDRAM to the SDRAM it was the frame's bottleneck, x_gui_end_scanline waiting on it 44% of the time). Given
   back, emptied from the cache, before the program ends */
#define DCCIMVAC (*(volatile uint32_t *)0xE000EF70u)
#define AXI_RAM 0x24000000u
static uint32_t axi_rasr;
static int axi_claimed;
static int axi_claim(void) {
    MPU_RNR = 12;
    if((MPU_RBAR & ~0x1Fu) != AXI_RAM || ((MPU_RASR >> 1) & 0x1F) != 18) return -1;
    axi_rasr = MPU_RASR;
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RASR = (axi_rasr & ~((1u << 28) | (7u << 19) | (1u << 18) | (1u << 17) | (1u << 16))) | (1u << 19) | (1u << 17) | (1u << 16);
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
    axi_claimed = 1;
    return 0;
}
static void axi_release(void) {
    if(!axi_claimed) return;
    memset((void *)AXI_RAM, 0, 2 * 22 * 640 * 2);  /* (the batches: MMBasic's 8-bit page is here, and would show them) */
    for(uint32_t a = AXI_RAM; a < AXI_RAM + 0x80000u; a += 32) DCCIMVAC = a;
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RNR = 12;
    MPU_RASR = axi_rasr;
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
    axi_claimed = 0;
}

static void *aligned32(size_t n) {              /* (from MMBasic's heap, else this program's: 32-aligned for the DMA2D and the cache) */
    void *b = cmm2->alloc(n + 32);
    if(!b) b = malloc(n + 32);
    return b ? (void *)(((uintptr_t)b + 31) & ~(uintptr_t)31) : NULL;
}

static int display_start(void) {
    g2 = (uintptr_t)display_start >= CMM2IMG_BASE && (uintptr_t)display_start < CMM2IMG_BASE + CMM2IMG_AREA;
    if(!g2) {
        if(cmm2->video_mode(1, 8, &video) < 0 || (pic8[0] = aligned32(NPIC * PIC_W * PIC_H)) == NULL) {
            fprintf(stderr, "bbc: cannot set MODE 1,8\n");
            return -1;
        }
        for(int i = 1; i < NPIC; i++) pic8[i] = pic8[0] + i * PIC_W * PIC_H;
        memset(pic8[0], 0, NPIC * PIC_W * PIC_H);
        memset(video.fb, 0, PW * PH);           /* (the page's first 640 x 512 bytes: the picture's, packed) */
        SCREEN[0] = pic8[0];
        SCREEN[1] = pic8[1];
        if(!timing_576p()) {
            fprintf(stderr, "bbc: PLL3 did not lock for 576p\n");
            return -1;
        }
        window_on((uint32_t)video.fb, 1);
        atexit(display_end);
        cmm2->frame_hook(frame8);
        return 0;
    }
    if(!has_arg("-nowa")) {                     /* (before display_end's atexit: put back after it) */
        wa_on();
        atexit(wa_off);
    }
    if(cmm2->video_mode(1, 16, &video) < 0) {
        fprintf(stderr, "bbc: cannot set MODE 1,16\n");
        return -1;
    }
    page[0] = (uint16_t *)video.fb;             /* (MMBasic's page 0, the screen's size, holds the picture's) */
    for(int i = 1; i < NPAGES; i++) {
        if((page[i] = aligned32(PW * PH * 2)) == NULL) break;
        memset(page[i], 0, PW * PH * 2);
        npages = i + 1;
    }
    if(axi_claim() == 0) {                      /* (after the mode, whose change clears the AXI RAM; given back after display_end) */
        atexit(axi_release);
        batch[0] = (uint16_t *)AXI_RAM;
    } else batch[0] = aligned32(2 * CH * PW * 2);
    shot = aligned32(PIC_W * PIC_H);
    if(npages < 2 || batch[0] == NULL || shot == NULL) {
        fprintf(stderr, "bbc: no room for the pages\n");
        return -1;
    }
    batch[1] = batch[0] + CH * PW;
    if(!timing_576p()) {
        fprintf(stderr, "bbc: PLL3 did not lock for 576p\n");
        return -1;
    }
    window_on((uint32_t)page[0], 2);
    atexit(display_end);
    cmm2->frame_hook(hook);
    return 0;
}

void graphics_set_palette(uint8_t i, uint32_t rgb) {
    palette[i * 3] = (uint8_t)(rgb >> 16);
    palette[i * 3 + 1] = (uint8_t)(rgb >> 8);
    palette[i * 3 + 2] = (uint8_t)rgb;
    pal565[i] = (uint16_t)((rgb >> 19 & 0x1F) << 11 | (rgb >> 10 & 0x3F) << 5 | (rgb >> 3 & 0x1F));
    palette_new = true;
}

uint32_t graphics_get_palette(uint8_t i) { return (uint32_t)palette[i * 3] << 16 | (uint32_t)palette[i * 3 + 1] << 8 | palette[i * 3 + 2]; }
void graphics_set_bgcolor(uint32_t rgb) { (void)rgb; }
void graphics_set_buffer(uint8_t *buffer) { (void)buffer; }

static void palette_show(void) {               /* (in the program: video_palette waits for the next frame) */
    if(!palette_new) return;
    palette_new = false;
    if(memcmp(palette, palette_shown, sizeof(palette)) == 0) return;
    memcpy(palette_shown, palette, sizeof(palette));
    if(!g2) cmm2->video_palette(0, 256, palette_shown);   /* (the G2's RGB565: pal565) */
}

/* frank_gui.c's MODE 7 line, at twice its width: each of the 40 characters is 16 of B-em's pixels, its six SAA5050 columns at the offsets where
   they are at full intensity (0, 2, 5, 8, 11, 14). There each pixel is one of the BBC's colours whole, so each column is drawn in its own
   colour (frank_gui.c's rule, by brightness, made a character on a coloured background all one colour), twice across, and the sides in the
   first column's: a letter's space, a joined graphic's left block. The BBC's colour of a pixel is the top bit of its red, green and blue
   (x_gui.h's RGB555: bits 15, 10 and 4), gathered for two pixels at a time by one multiply: shifted down 4, bits 11, 6, 0 and 27, 22, 16 times
   1 + 2^6 + 2^13 land on 11-13 and 27-29 as red, green, blue (the only two products on one bit, 6 and 22, carry into 7 and 23, clear of them).
   dst: the G2's RGB565 line or the G1's 8-bit one; idx: the 8-bit line as well (a G2's screenshot frame), or NULL */
static inline uint32_t two_bbc(uint32_t w) { return ((w >> 4) & 0x08410841u) * 0x2041u; }

static void row_teletext(const uint16_t *src, void *dst, uint8_t *idx) {
    for(int c = 0; c < PIC_W / 16; c++) {
        const uint16_t *cell = src + c * 16;
        uint32_t a = two_bbc(cell[0] | (uint32_t)cell[2] << 16), b = two_bbc(cell[5] | (uint32_t)cell[8] << 16),
                 d = two_bbc(cell[11] | (uint32_t)cell[14] << 16);
        uint8_t k[16];
        k[0] = k[1] = k[14] = k[15] = (uint8_t)(a >> 11 & 7);
        k[2] = k[3] = (uint8_t)(a >> 11 & 7);
        k[4] = k[5] = (uint8_t)(a >> 27 & 7);
        k[6] = k[7] = (uint8_t)(b >> 11 & 7);
        k[8] = k[9] = (uint8_t)(b >> 27 & 7);
        k[10] = k[11] = (uint8_t)(d >> 11 & 7);
        k[12] = k[13] = (uint8_t)(d >> 27 & 7);
        if(g2) {
            uint16_t *o = (uint16_t *)dst + c * 16;
            for(int i = 0; i < 16; i++) o[i] = pal565[k[i]];
            if(idx) memcpy(idx + c * 16, k, 16);
        } else memcpy((uint8_t *)dst + c * 16, k, 16);
    }
}

static void row_pixels(const uint16_t *src, void *dst, uint8_t *idx) {
    const uint32_t *s = (const uint32_t *)src;
    if(g2) {
        uint32_t *d = (uint32_t *)dst;
        for(int x = 0; x < PIC_W / 4; x++) {
            uint32_t a = two_bbc(s[2 * x]), b = two_bbc(s[2 * x + 1]);
            uint32_t i0 = a >> 11 & 7, i1 = a >> 27 & 7, i2 = b >> 11 & 7, i3 = b >> 27 & 7;
            d[2 * x] = pal565[i0] | (uint32_t)pal565[i1] << 16;
            d[2 * x + 1] = pal565[i2] | (uint32_t)pal565[i3] << 16;
            if(idx) ((uint32_t *)idx)[x] = i0 | i1 << 8 | i2 << 16 | i3 << 24;
        }
    } else {
        uint32_t *d = (uint32_t *)dst;
        for(int x = 0; x < PIC_W / 4; x++) {
            uint32_t a = two_bbc(s[2 * x]), b = two_bbc(s[2 * x + 1]);
            d[x] = (a >> 11 & 7) | (a >> 27 & 7) << 8 | (b >> 11 & 7) << 16 | (b >> 27 & 7) << 24;
        }
    }
}

/* a line of frank-micro's menus over the picture's: drawn at 320 x 256 at the frame's start, each pixel twice across where they draw */
static void ui_row(int y, void *dst) {
    const uint8_t *s = ui_fb + y * UI_W;
    if(g2) {
        uint16_t *d = (uint16_t *)dst;
        for(int x = 0; x < UI_W; x++) if(s[x] != UI_CLEAR) d[2 * x] = d[2 * x + 1] = pal565[s[x]];
    } else {
        uint8_t *d = (uint8_t *)dst;
        for(int x = 0; x < UI_W; x++) if(s[x] != UI_CLEAR) d[2 * x] = d[2 * x + 1] = s[x];
    }
}

/* B-em's lines (display.c, built X_GUI): frank_gui.c's bookkeeping, a frame of 256, numbered as scanvideo numbers them */
static uint16_t __attribute__((aligned(32))) row0[PIC_W], row1[PIC_W];
static struct scanvideo_scanline_buffer s_buffer;
static int s_scanline_number;
static uint32_t s_frame_number_part;

int x_gui_init(void) { return 0; }

struct scanvideo_scanline_buffer *x_gui_begin_scanline(void) {
    if(s_scanline_number++ == 0) {              /* a frame begins */
        current_buffer ^= 1u;
        pic_n = (pic_n + 1) % NPIC;             /* (the G1's: the one taken two blankings ago) */
        s_frame_number_part += 0x10000u;
        shot_frame = g2 && g_frank_screenshot_pending;
        ui_frame = frank_ui_is_visible();
        if(ui_frame) {
            memset(ui_fb, UI_CLEAR, sizeof(ui_fb));
            frank_ui_render(ui_fb, UI_W, UI_H);
        }
    }
    s_buffer.row0 = row0;
    s_buffer.row1 = s_buffer.double_height ? row1 : NULL;
    s_buffer.scanline_id = s_frame_number_part | (uint32_t)s_scanline_number;
    memset(row0, 0, sizeof(row0));
    if(s_buffer.row1) memset(row1, 0, sizeof(row1));
    return &s_buffer;
}

void x_gui_end_scanline(struct scanvideo_scanline_buffer *buffer) {
    int y = s_scanline_number - 1;
    if(y >= 0 && y < PIC_H) {
        void *d;
        uint8_t *idx = NULL;
        int b = 0, i = 0;
        if(g2) {
            b = (y / CH) & 1;
            i = y % CH;
            if(i == 0) while((int32_t)(completed() - buf_mark[b]) < 0) { }   /* (a batch begins: its last copies done) */
            d = batch[b] + i * PW;
            if(shot_frame) idx = shot + y * PIC_W;
        } else d = pic8[pic_n] + y * PIC_W;
        if(buffer->double_height) row_teletext(buffer->row0, d, idx);
        else row_pixels(buffer->row0, d, idx);
        if(ui_frame) ui_row(y, d);
        if(g2 && (i == CH - 1 || y == PIC_H - 1)) {
            copy_out(batch[b], y - i, i + 1);
            buf_mark[b] = issued;
        }
    }
    if(s_scanline_number >= PIC_H) {            /* the frame's end */
        s_scanline_number = 0;
        if(g2) {
            if(shot_frame) {                    /* (the BBC's picture, before the menus) */
                shot_frame = g_frank_screenshot_pending = false;
                frank_ui_toast(frank_screenshot_save(shot) == 0 ? "Screenshot saved" : "Screenshot failed");
            }
            present();
        } else {
            if(g_frank_screenshot_pending) {    /* (the G1's 8-bit picture is the shown one: the menus are on it) */
                g_frank_screenshot_pending = false;
                frank_ui_toast(frank_screenshot_save(pic8[pic_n]) == 0 ? "Screenshot saved" : "Screenshot failed");
            }
            uint64_t t0 = cmm2->micros();
            if(g_frank_limit_speed) while(ready) { }   /* (the last one taken: the display's 50 a second, as present on the G2) */
            ready = pic8[pic_n];
            if(stats) stat_idle += cmm2->micros() - t0;
        }
    }
}

void x_gui_refresh_menu_display(void) { }
void x_gui_set_force_aspect_ratio(bool far) { (void)far; }

/* ---- the sound ---- */
#define RING 8192                               /* a power of 2 */
#define RING_LEVEL 3000                         /* samples: a stream buffer of 1024, a frame of 625 and more */
#define RING_FULL 7000
#define BBC_HZ 31250                            /* B-em's samples a second, of the BBC's time */
#define CHUNK 256                               /* B-em's buffer: even */
#define SOUND_GAIN 8                            /* B-em's SN76489 makes about a tenth of full scale (frank-micro's HDMI: the set's volume) */
static int16_t ring[RING];
static uint32_t ring_phase, ring_step;
bool x_gui_audio_init_failed;
volatile int g_frank_audio_driver;              /* (frank_settings.h's: the RP2350's HDMI, I2S or PWM; one here) */

void frank_audio_set_driver(int drv) { g_frank_audio_driver = drv; }

/* in PendSV: no services. The emulator keeps its own time and the stream the board's, so the step is trimmed, by up to 1%, to hold the ring at RING_LEVEL */
static void fill(int16_t *buf, uint32_t frames) {
    static bool playing;
    static int16_t v;
    static int32_t smooth = RING_LEVEL << 4;
    uint32_t out = ring_out, in = ring_in, ph = ring_phase, level = in - out;
    smooth += (int32_t)level - (smooth >> 4);
    int32_t err = (smooth >> 4) - RING_LEVEL;
    if(err > RING_LEVEL) err = RING_LEVEL;
    if(err < -RING_LEVEL) err = -RING_LEVEL;
    uint32_t step = ring_step + (int32_t)(((int64_t)ring_step * err) / (RING_LEVEL * 100));
    if(!playing && level >= RING_LEVEL) playing = true;
    while(frames--) {
        if(playing) {
            if(out == in) playing = false;      /* (empty: waits for RING_LEVEL again) */
            else {
                v = ring[out & (RING - 1)];
                ph += step;
                out += ph >> 16;
                ph &= 0xFFFF;
                if((int32_t)(in - out) < 0) out = in;
            }
        }
        *buf++ = v;
        *buf++ = v;
    }
    ring_out = out;
    ring_phase = ph;
}

/* frank_audio.c's: B-em's one buffer of mono samples, taken and given back full */
static int16_t s_mono[CHUNK];
static mem_buffer_t s_mem = { .size = sizeof(s_mono), .bytes = (uint8_t *)s_mono, .flags = 0 };
static struct audio_format s_fmt;
static struct audio_buffer_format s_bfmt = { .format = &s_fmt, .sample_stride = 2 };
static struct audio_buffer s_buf = { .buffer = &s_mem, .format = &s_bfmt, .max_sample_count = CHUNK };
static struct audio_buffer_pool s_pool;

struct audio_buffer_pool *x_gui_audio_init(uint freq) {
    s_fmt.sample_freq = freq;
    s_fmt.format = AUDIO_BUFFER_FORMAT_PCM_S16;
    s_fmt.channel_count = 1;
    x_gui_audio_init_failed = false;
    int r = cmm2->audio_start(fill);
    audio_on = r > 0;
    if(audio_on) ring_step = (uint32_t)(((uint64_t)freq << 16) / (uint32_t)r);
    return &s_pool;
}

struct audio_buffer *take_audio_buffer(struct audio_buffer_pool *ac, bool block) {
    (void)ac; (void)block;
    s_buf.sample_count = 0;
    return &s_buf;
}

void give_audio_buffer(struct audio_buffer_pool *ac, struct audio_buffer *buffer) {
    const int16_t *src = (const int16_t *)buffer->buffer->bytes;
    int vol = g_frank_sound_on ? g_frank_volume : 0;
    (void)ac;
    if(vol > 100) vol = 100;
    for(uint32_t i = 0; i < buffer->sample_count; i++) {
        uint32_t in = ring_in;
        int a = src[i] < 0 ? -src[i] : src[i];
        if(a > stat_peak) stat_peak = a;
        if(!audio_on || in - ring_out >= RING_FULL) break;
        int32_t v = (int32_t)src[i] * vol * SOUND_GAIN / 100;   /* (B-em's four channels at their loudest are about 3,000) */
        ring[in & (RING - 1)] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
        ring_in = in + 1;
    }
}

/* B-em's disc noise streams samples from the RP2350's flash (sector_read's xip_stream): not here (frank_audio.c's) */
#include "xip_stream.h"
struct xip_stream_dma *xip_stream_head;
void xip_stream_init(void) { }
void xip_stream_dma_start(struct xip_stream_dma *dma) { if(dma) dma->state = COMPLETE; }
void xip_stream_dma_cancel(struct xip_stream_dma *dma) { if(dma) dma->state = NONE; }
void xip_stream_dma_poll(void) { }

/* ---- the keyboard: frank-micro's USB HID driver's key actions (usbhid.h), from the USB keyboard's raw reports ---- */
#define M_CTRL 0x11                             /* the report's modifier bits: left and right */
#define M_SHIFT 0x22
#define M_ALT 0x44
#define H_PAUSE 0x48                            /* HID usages */
#define H_END 0x4D
#define KEYQ 64
static struct { uint8_t hid, down; } keyq[KEYQ];
static uint32_t keyq_in, keyq_out;
static bool usb_keys_down;                      /* (a USB key's character reaches the serial console too: it is not typed twice) */

static void key_action(uint8_t hid, int down) {
    if(keyq_in - keyq_out >= KEYQ) return;
    keyq[keyq_in % KEYQ].hid = hid;
    keyq[keyq_in % KEYQ].down = (uint8_t)down;
    keyq_in++;
}

int usbhid_get_key_action(uint8_t *hid, int *down) {
    if(keyq_out == keyq_in) return 0;
    *hid = keyq[keyq_out % KEYQ].hid;
    *down = keyq[keyq_out % KEYQ].down;
    keyq_out++;
    return 1;
}

static bool in_report(const uint8_t *r, uint8_t h) {
    for(int i = 2; i < 8; i++) if(r[i] == h) return true;
    return false;
}

static void bbc_break(void) { m6502_reset(); }  /* (as frank-micro's SHIFT-BREAK autoboot: the 6502 reset, the BBC's memory as it is) */

/* each frame (frank_keyboard_poll): the USB devices seen to, the keys that went down and up since the last report as frank-micro's driver gives them
   (its shared Ctrl, Shift and Alt as 0xE0-0xE2), Pause as BREAK, Ctrl+Alt+End to leave */
static void typing_frame(void);
static void keys_by_character(uint8_t *r);
void usbhid_wrapper_init(void) { }
int usbhid_wrapper_keyboard_connected(void) { return 1; }
int usbhid_wrapper_gamepad_connected(void) { return 1; }
int usbhid_wrapper_get_key(int *pressed, unsigned char *key) { (void)pressed; (void)key; return 0; }

void usbhid_wrapper_tick(void) {
    static uint8_t last[8];
    uint8_t r[8];
    cmm2->poll();
    cmm2->break_take();                         /* (Ctrl-C is the BBC's) */
    memset(r, 0, sizeof(r));
    cmm2->keys_raw(r);
    if((r[0] & M_CTRL) && (r[0] & M_ALT) && in_report(r, H_END)) exit(0);   /* Ctrl+Alt+End leaves */
    keys_by_character(r);
    static const uint8_t mod[3] = { M_CTRL, M_SHIFT, M_ALT };
    for(int m = 0; m < 3; m++) if((r[0] & mod[m]) != (last[0] & mod[m])) key_action((uint8_t)(0xE0 + m), (r[0] & mod[m]) != 0);
    for(int i = 2; i < 8; i++) if(last[i] >= 4 && !in_report(r, last[i])) key_action(last[i], 0);
    for(int i = 2; i < 8; i++) if(r[i] >= 4 && !in_report(last, r[i])) {
        if(r[i] == H_PAUSE) bbc_break();
        else key_action(r[i], 1);
    }
    usb_keys_down = r[0] != 0 || r[2] != 0 || last[0] != 0 || last[2] != 0;
    memcpy(last, r, sizeof(last));
    typing_frame();
}

/* the first gamepad, as frank-micro's USB driver gives it (frank_keyboard.c's BTN_ bits), its left stick as the d-pad */
unsigned int usbhid_wrapper_get_joystick(void) {
    enum { BTN_LEFT = 0x0001, BTN_RIGHT = 0x0002, BTN_UP = 0x0004, BTN_DOWN = 0x0008, BTN_FIREA = 0x0010, BTN_FIREB = 0x0020, BTN_START = 0x0100,
           BTN_SELECT = 0x0200, BTN_FIREX = 0x0800, BTN_FIREY = 0x1000 };
    for(int slot = 1; slot <= 8; slot++) {
        int x = 128, y = 128, b = cmm2->gamepad(slot, &x, &y);
        unsigned int m = 0;
        if(b < 0) continue;
        if((b >> 7 & 1) || y < 64) m |= BTN_UP;
        if((b >> 5 & 1) || y > 192) m |= BTN_DOWN;
        if((b >> 8 & 1) || x < 64) m |= BTN_LEFT;
        if((b >> 6 & 1) || x > 192) m |= BTN_RIGHT;
        if(b >> 11 & 1) m |= BTN_FIREA;         /* a/circle */
        if(b >> 13 & 1) m |= BTN_FIREB;         /* b/cross */
        if(b >> 10 & 1) m |= BTN_FIREX;         /* x/triangle */
        if(b >> 12 & 1) m |= BTN_FIREY;         /* y/square */
        if(b >> 1 & 1) m |= BTN_START;
        if(b >> 3 & 1) m |= BTN_SELECT;
        if(stats) {                             /* (-stats: each change of the pad, as the firmware gives it) */
            static int last_b = -1, last_x = -1, last_y = -1, last_slot;
            if(b != last_b || slot != last_slot || (x >> 4) != (last_x >> 4) || (y >> 4) != (last_y >> 4)) {
                printf("bbc: pad %d buttons %04X stick %d,%d -> %04X\n", slot, b, x, y, m);
                last_b = b; last_x = x; last_y = y; last_slot = slot;
            }
        }
        return m;
    }
    return 0;
}

unsigned int usbhid_wrapper_get_joystick_idx(int idx) { return idx == 0 ? usbhid_wrapper_get_joystick() : 0; }

/* no wired NES pad (frank_keyboard.c's nespad) */
uint32_t nespad_state, nespad_state2;
bool nespad_begin(uint32_t cpu_khz, uint8_t clkPin, uint8_t dataPin, uint8_t dataPin2, uint8_t latPin) {
    (void)cpu_khz; (void)clkPin; (void)dataPin; (void)dataPin2; (void)latPin;
    return false;
}
void nespad_read(void) { }

/* ---- the serial console's typing: each character as the BBC's own keyboard makes it, by B-em's key codes (keyboard.c's allegro2bbc) ---- */
enum { AK_A = 1, AK_0 = 27, AK_1 = 28, AK_F1 = 47, AK_ESCAPE = 59, AK_TILDE = 60, AK_MINUS, AK_EQUALS, AK_BACKSPACE, AK_TAB, AK_OPENBRACE,
       AK_CLOSEBRACE, AK_ENTER, AK_SEMICOLON, AK_QUOTE, AK_BACKSLASH, AK_BACKSLASH2, AK_COMMA, AK_FULLSTOP, AK_SLASH, AK_SPACE,
       AK_LSHIFT = 215, AK_LCTRL = 217 };
#define S 0x100                                 /* with SHIFT */
#define C 0x200                                 /* with CTRL */

/* the BBC's key for character c (and SHIFT or CTRL), or 0 */
static int bbc_key(int c) {
    static const char sym[] = "!\"#$%&'()";      /* SHIFT and 1-9 */
    const char *q;
    if(c >= 'A' && c <= 'Z') return AK_A + c - 'A';              /* (CAPS LOCK on, as the BBC starts) */
    if(c >= 'a' && c <= 'z') return (AK_A + c - 'a') | S;
    if(c == '0') return AK_0;
    if(c >= '1' && c <= '9') return AK_1 + c - '1';
    if(c && (q = strchr(sym, c)) != NULL) return (AK_1 + (int)(q - sym)) | S;
    switch(c) {
        case ' ': return AK_SPACE;
        case '-': return AK_MINUS;
        case '=': return AK_MINUS | S;
        case '^': return AK_EQUALS;
        case '~': return AK_EQUALS | S;
        case '@': return AK_OPENBRACE;
        case '[': return AK_CLOSEBRACE;
        case '{': return AK_CLOSEBRACE | S;
        case ';': return AK_SEMICOLON;
        case '+': return AK_SEMICOLON | S;
        case ':': return AK_QUOTE;
        case '*': return AK_QUOTE | S;
        case ']': return AK_BACKSLASH;
        case '}': return AK_BACKSLASH | S;
        case '\\': return AK_BACKSLASH2;
        case '|': return AK_BACKSLASH2 | S;
        case '_': return AK_TILDE;
        case '`': case 163: return AK_TILDE | S;   /* (the BBC's pound sign, its character 96; 163 is a French keyboard's) */
        case ',': return AK_COMMA;
        case '<': return AK_COMMA | S;
        case '.': return AK_FULLSTOP;
        case '>': return AK_FULLSTOP | S;
        case '/': return AK_SLASH;
        case '?': return AK_SLASH | S;
        case 13: return AK_ENTER;
        case 8: case 127: return AK_BACKSPACE;
        case 9: return AK_TAB;
        case 27: return AK_ESCAPE;
    }
    if(c >= 1 && c <= 26) return (AK_A + c - 1) | C;
    return 0;
}

/* ---- the USB keyboard by character: what is on the key is what the BBC types, in any OPTION USBKEYBOARD layout (Peter, 12 Nov 2026), as the CPC's
   (apps/cpc). frank-micro's USB path takes a key by its place on a PC's keyboard and B-em puts the BBC's keys there (keyboard.c), so Shift and 8 was the
   BBC's SHIFT and 8, which is (. Here each key of the main block, as it goes down, becomes the place of the BBC key that types the character the
   firmware's layout makes of it (keys_held; bbc_key's table), and the BBC's SHIFT is as that character needs and not as the PC's is: * is SHIFT and the
   BBC's : key, @ its @ key with SHIFT up. Decided as the key goes down, so that a Shift let go first does not change a key still held. Letters keep
   the PC's Shift (CAPS LOCK is the BBC's own); the keypad, the arrows, the function keys, Return and the rest keep their places. As the CPC found, a key
   that needs SHIFT up is held back SHIFT_SETTLE frames from when it went up; with no Shift held there is no wait. RUN's -keys prints each key */
static int keytrace = -1;
#define SHIFT_SETTLE 3

static uint8_t ak_place(int ak) {               /* the USB key at the place of B-em's key ak (frank_keyboard.c's hid_to_xt and patches/0003) */
    if(ak >= AK_A && ak < AK_A + 26) return (uint8_t)(0x04 + ak - AK_A);
    if(ak == AK_0) return 0x27;
    if(ak >= AK_1 && ak < AK_1 + 9) return (uint8_t)(0x1E + ak - AK_1);
    switch(ak) {
        case AK_TILDE: return 0x35;
        case AK_MINUS: return 0x2D;
        case AK_EQUALS: return 0x2E;
        case AK_OPENBRACE: return 0x2F;
        case AK_CLOSEBRACE: return 0x30;
        case AK_BACKSLASH: return 0x31;
        case AK_SEMICOLON: return 0x33;
        case AK_QUOTE: return 0x34;
        case AK_COMMA: return 0x36;
        case AK_FULLSTOP: return 0x37;
        case AK_SLASH: return 0x38;
        case AK_BACKSLASH2: return 0x64;
    }
    return 0;
}

static void keys_by_character(uint8_t *r) {
    static uint8_t was[6], place[256];
    static int8_t shift[256];
    static int shift_up = SHIFT_SETTLE;         /* frames the BBC's SHIFT has been up */
    uint8_t out[6] = {0};
    int keys[7], total = 0, force = -1, i, fresh = 0, mods = r[0];
    if(keytrace < 0) keytrace = has_arg("-keys");
    for(i = 0; i < 6 && r[i + 2]; i++) total++;
    cmm2->keys_held(keys);
    for(i = 0; i < total; i++) {
        uint8_t h = r[i + 2];
        if(memchr(was, h, sizeof(was)) == NULL) {
            int c = keys[total - 1 - i], k = 0;   /* (keys_held has the last key of the report first) */
            uint8_t p = 0;
            bool letter = h >= 0x04 && h <= 0x1D;
            if(c >= 1 && c <= 26) c += 96;      /* (Ctrl with a letter) */
            if(h < 0x28 || (h >= 0x2D && h <= 0x38) || h == 0x64) {
                k = bbc_key(c);
                if(!(k & C)) p = ak_place(k & 0xFF);
            }
            place[h] = p ? p : h;
            shift[h] = !p || (p >= 0x04 && p <= 0x1D && letter) ? -1 : (k & S) != 0;
            fresh = 1;
            if(keytrace) printf("bbc: key %02X of %d: character %d, place %02X, shift %d\n", h, total, c, place[h], shift[h]);
        }
        out[i] = place[h];
        if(shift[h] >= 0) force = shift[h];
    }
    if(force == 0) r[0] &= ~M_SHIFT;
    else if(force == 1) r[0] |= 0x02;
    if(r[0] & M_SHIFT) shift_up = 0;
    else if(shift_up < SHIFT_SETTLE) {
        shift_up++;
        for(i = 0; i < total; i++) if(shift[r[i + 2]] == 0) out[i] = 0;     /* (the next frames offer it again) */
    }
    memcpy(was, r + 2, sizeof(was));
    memcpy(r + 2, out, sizeof(out));
    if(force >= 0) r[0] &= ~0x40;               /* (AltGr made the character) */
    if(keytrace && fresh) printf("bbc:   modifiers %02X became %02X, keys %02X %02X\n", mods, r[0], r[2], r[3]);
}

#define HOLD 3                                  /* frames a key is held, then frames up before the next */
#define GAP 2
static int typed_q[1024];
static uint32_t typed_in, typed_out;

static void typed(int k) {
    if(typed_in - typed_out < sizeof(typed_q) / sizeof(typed_q[0])) typed_q[typed_in++ % (sizeof(typed_q) / sizeof(typed_q[0]))] = k;
}

/* each frame: the console's characters queued (Ctrl-F then 0-9 f0-f9, u d l r the cursor keys, p n Page Up and Down, b BREAK, B CTRL and BREAK, m the disc menu,
   s the settings; Ctrl-] leaves), and the next typed key pressed and let go; while frank-micro's menus are up, Return, Escape, Backspace and
   those keys are the menu's */
enum { AK_PGUP = 80, AK_PGDN, AK_LEFT, AK_RIGHT, AK_UP, AK_DOWN };
static unsigned int menu_key(int k) {           /* (frank-micro's keysym for a typed key, or 0) */
    switch(k) {
        case AK_ENTER: return FRANK_KS_Return;
        case AK_ESCAPE: return FRANK_KS_Escape;
        case AK_BACKSPACE: return FRANK_KS_BackSpace;
        case AK_UP: return FRANK_KS_Up;
        case AK_DOWN: return FRANK_KS_Down;
        case AK_LEFT: return FRANK_KS_Left;
        case AK_RIGHT: return FRANK_KS_Right;
        case AK_PGUP: return FRANK_KS_Page_Up;
        case AK_PGDN: return FRANK_KS_Page_Down;
    }
    return 0;
}

static void typing_frame(void) {
    static int prefix, held, phase, last, ctrl_hold;
    int c;
    if(ctrl_hold && --ctrl_hold == 0) key_up(AK_LCTRL);
    while((c = cmm2->con_getc()) >= 0) {
        if(c == 0x1D) exit(0);                  /* Ctrl-], whatever the USB keyboard is doing */
        if(usb_keys_down) continue;             /* (the USB keyboard's own) */
        if(prefix) {
            static const char keys[] = "udlrpn";
            static const int codes[] = { AK_UP, AK_DOWN, AK_LEFT, AK_RIGHT, AK_PGUP, AK_PGDN };
            const char *q = c ? strchr(keys, c) : NULL;
            prefix = 0;
            if(c >= '0' && c <= '9') typed(AK_F1 + c - '0');
            else if(q) typed(codes[q - keys]);
            else if(c == 'b') typed(-1);
            else if(c == 'B') typed(-4);
            else if(c == 'm') typed(-2);
            else if(c == 's') typed(-3);
            continue;
        }
        if(c == 6) {
            prefix = 1;
            continue;
        }
        if(c == 0x1D) exit(0);                  /* Ctrl-] */
        if(c == 10 && last == 13) continue;     /* (CR LF: one RETURN) */
        last = c;
        if(c == 10) c = 13;
        int k = bbc_key(c);
        if(k) typed(k);
    }
    if(phase > 0) {
        if(--phase == GAP) {                    /* let go */
            key_up(held & 0xFF);
            if(held & S) key_up(AK_LSHIFT);
            if(held & C) key_up(AK_LCTRL);
        }
        return;
    }
    if(typed_out == typed_in) return;
    held = typed_q[typed_out++ % (sizeof(typed_q) / sizeof(typed_q[0]))];
    if(held == -1) {
        bbc_break();
        return;
    }
    if(held == -4) {                            /* CTRL and BREAK: the hard reset (a Master's ROMs in sideways RAM seen), CTRL held a second */
        key_down(AK_LCTRL);
        bbc_break();
        ctrl_hold = 50;
        return;
    }
    if(held == -2) {
        frank_ui_open_disk_menu();
        return;
    }
    if(held == -3) {
        frank_ui_toggle();
        return;
    }
    if(frank_ui_wants_keys()) {                 /* (the menu's) */
        unsigned int ks = menu_key(held & 0xFF);
        if(ks) frank_ui_handle_key(ks);
        return;
    }
    if(held & S) key_down(AK_LSHIFT);
    if(held & C) key_down(AK_LCTRL);
    key_down(held & 0xFF);
    phase = HOLD + GAP;
}

/* ---- the sideways ROMs of the folder roms (A:/bbc/roms; Peter, 12 Nov 2026) ----
   Each .rom or .bin file there, in the order of their names, goes into a free sideways RAM bank, from the lowest: the Master's 4 to 7, the Model B's
   from 0 (B-em's banks are frank-micro's heap blocks: NO_USE_RAM_ROMS, alloc_swram). An 8 KB ROM is put in twice, as a 16 KB socket shows it, a shorter
   one with &FF after it; a longer one is left out. Then the BBC is started again as from switching on (B-em's main_reset, Ctrl+Alt+Delete's), so that
   its MOS finds them, as it finds ROMs only then (the Master's after CTRL-BREAK too). Done before the first frame's end and again after the model is
   changed, which makes the banks afresh */
#include <strings.h>
#include "mem.h"
extern void main_reset(void);
extern int8_t curmodel;
#define ROMS_MAX 16

static int name_cmp(const void *a, const void *b) { return strcasecmp(*(char *const *)a, *(char *const *)b); }

static int roms_load(void) {
    char *names[ROMS_MAX];
    cmm2_dirent e;
    int n = 0, loaded = 0, slot = 0, d = cmm2->dir_open("roms");
    if(d < 0) return 0;
    while(cmm2->dir_read(d, &e) == 1) {
        const char *dot = strrchr(e.name, '.');
        if(e.is_dir || dot == NULL || (strcasecmp(dot, ".rom") != 0 && strcasecmp(dot, ".bin") != 0)) continue;
        if(n == ROMS_MAX) {
            printf("bbc: roms/%s left out: no more than %d are read\n", e.name, ROMS_MAX);
            continue;
        }
        if((names[n] = strdup(e.name)) != NULL) n++;
    }
    cmm2->dir_close(d);
    qsort(names, (size_t)n, sizeof(names[0]), name_cmp);
    for(int i = 0; i < n; i++) {
        char path[300];
        int h;
        int32_t size;
        while(slot < ROM_NSLOT && !(rom_slots[slot].swram && rom_slots[slot].data)) slot++;
        snprintf(path, sizeof(path), "roms/%s", names[i]);
        free(names[i]);
        if(slot == ROM_NSLOT) {
            printf("bbc: %s left out: no sideways RAM bank is free\n", path);
            continue;
        }
        if((h = cmm2->file_open(path, CMM2_O_READ)) < 0) {
            printf("bbc: cannot open %s\n", path);
            continue;
        }
        size = cmm2->file_size(h);
        if(size <= 0 || size > ROM_SIZE) {
            printf("bbc: %s left out: %ld bytes, not 1 to 16384\n", path, (long)size);
            cmm2->file_close(h);
            continue;
        }
        uint8_t *bank = (uint8_t *)rom_slots[slot].data;   /* (sideways RAM: frank-micro's heap) */
        memset(bank, 0xFF, ROM_SIZE);
        int32_t got = cmm2->file_read(h, bank, (uint32_t)size);
        cmm2->file_close(h);
        if(got != size) {
            printf("bbc: cannot read %s\n", path);
            memset(bank, 0xFF, ROM_SIZE);
            continue;
        }
        if(size == ROM_SIZE / 2) memcpy(bank + ROM_SIZE / 2, bank, ROM_SIZE / 2);
        if(bank[7] + 4 <= size && memcmp(bank + bank[7], "\0(C)", 4) == 0) {
            char title[64];
            int t = 0;
            for(int k = 9; k < bank[7] && t < (int)sizeof(title) - 1; k++) title[t++] = bank[k] >= 32 && bank[k] < 127 ? (char)bank[k] : ' ';
            title[t] = 0;
            printf("bbc: %s in bank %d: %s\n", path, slot, title);
        } else
            printf("bbc: %s in bank %d has no ROM header: the BBC will not see it\n", path, slot);
        slot++;
        loaded++;
    }
    return loaded;
}

static void roms_check(void) {
    static int roms_model = -1;
    if(curmodel == roms_model) return;
    roms_model = curmodel;
    int n = roms_load();
    if(n > 0) {
        char msg[40];
        main_reset();
        snprintf(msg, sizeof(msg), "%d ROM%s from roms", n, n == 1 ? "" : "s");
        frank_ui_toast(msg);
    }
}

/* ---- frank_platform.c's frank_perf_tick, called by al_wait_for_event once a frame of the BBC's (40,000 cycles, 20 ms): the pace is the
   display's, 576p's 50 frames a second on both boards (present on a G2, x_gui_end_scanline's wait on a G1) ---- */
void frank_perf_tick(void) {
    static bool inited;
    roms_check();                               /* (the first frame, and after a change of model) */
    if(!inited) {                               /* (B-em's discs are ready once main_init has run) */
        inited = true;
        frank_settings_apply_live();
        frank_disk_autoload();
        for(int i = 1, drive = 0; i < cmm2_start->argc && drive < 2; i++)   /* RUN "bbc.app", disc [disc]: drives 0 and 1 */
            if(cmm2_start->argv[i][0] != '-' && cmm2_start->argv[i][0] != ',') {
                char name[256];
                snprintf(name, sizeof(name), "%s", cmm2_start->argv[i]);
                size_t n = strlen(name);
                if(n && name[n - 1] == ',') name[n - 1] = 0;   /* (RUN's words are split at spaces: "disc," is the disc) */
                if(frank_disc_mount(drive, name) != 0) printf("bbc: cannot open %s\n", name);
                drive++;
            }
        if(frank_disc_name(0)) frank_disc_request_boot();
    }
    frank_disc_autoboot_tick();
    palette_show();
    stat_frame();
}

/* ---- the start ---- */
int main(int argc, char **argv) {
    static const uint32_t bbc[8] = { 0x000000, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF };
    static FATFS fs;
    (void)argc; (void)argv;
    stats = has_arg("-stats");
    g_frank_disc_readonly = has_arg("-ro");
    for(int i = 0; i < 256; i++) graphics_set_palette((uint8_t)i, bbc[i & 7]);   /* (frank_platform.c's) */
    frank_ui_init();                            /* (the menus' colours, 240-247) */
    if(display_start() < 0) return 1;
    cmm2->con_screen(0);                        /* (its messages to the serial console alone) */
    memset(cmm2_garbage, 0xFF, sizeof(cmm2_garbage));
    palette_show();
    frank_gamepad_init();
    f_mount(&fs, "", 1);
    frank_settings_load();
    stat_from = cmm2->micros();
    _al_mangled_main(0, NULL);
    return 0;
}
