/* amiga_cmm2.c: an Amiga 500 on the Colour Maximite 2, pico-amiga's emulator (drewpo28's line-based core for the
   RP2350: Musashi's 68000, the OCS chipset a line at a time) on one core, by the loader's services. This file is the
   platform, what src/pico is on the RP2350 boards (Peter, 9 Nov 2026: "what about taking the emulator from the other
   source").

     RUN "amiga.app" [, disk.adf [disk2.adf ...]] [-df1 disk.adf] [-wp] [-kick file] [-ext file] [-ntsc | -pal60] [-nosound]
                       [-click frame] [-fast] [-stats] [-keys] [-nowa] [-dt N] [-qos N]

   By default (Peter, 10 Nov 2026: "Ensure that the default when you run the app makes sense PAL, sound on etc."): a
   PAL Amiga in 576p with sound, its Kickstart amiga-os-130.rom. -ntsc: an NTSC Amiga in MODE 8 at 60 Hz; -pal60: a PAL
   Amiga in MODE 8 at 60 Hz, a frame a monitor frame (pico-amiga's own way: 20% fast), for a monitor that takes no
   50 Hz; -nosound. (-pal and -sound, once needed, are taken and change nothing.)

   - The machine: an A500 with 512 KB of chip RAM and 512 KB of slow RAM, PAL (-ntsc: NTSC), its Kickstart from the folder
     (amiga-os-130.rom, Amiga Forever's Kickstart 1.3, or -kick; 256 or 512 KB, a user's own: none is built in). The
     first disk given is in DF0 at power-on. F11 brings up the disk menu (below) at any time, and a program started
     with no disk starts with it. DF1, an external drive, takes a second disk (-df1, or the menu). What the Amiga writes to
     a disk goes into its .adf file on the card as well, so it lasts, as on a real disk; a read-only file, or any with
     -wp, is a write-protected disk.
   - The display, NTSC: MODE 8 (640 x 480 at 60 Hz, the Amiga's own rate) in depth 12, ARGB4444 on one layer, cut to a
     window of 640 x 474: the core's picture (237 rows of the NTSC beam, each two of the monitor's lines, 640 of its 720
     pixels: a low resolution pixel two wide), each frame shown at the monitor's next. -pal: 576p, 720 x 576 at 50 Hz (the
     program's own timing over MODE 1's page: below), all 288 rows, each frame shown once too. As the frame is made its rows are put out as 0x0RGB
     pixels (the core's amiga_fb_line), in batches in the AXI RAM, and the DMA2D copies each batch into the page not
     shown, each row on two lines (below); the frame hook shows that page at a blanking once its copies are done.
     Outside the window, the LTDC's background colour: the colour at the picture's top left, the border's. MODE 19 and
     MODE 1's 800 x 600 at 75 Hz read too much from the SDRAM for the LTDC to keep up, and the monitors lost their sync;
     MODE 8 at 60 Hz (Peter, 9 Nov 2026: "If we change to NTSC mode then everything can work in 640x480 with massively
     reduced bus bandwidth") has not lost it. An interlaced row's second line is shown as its first (for now).
   - The sound (-nosound: none): Paula at 31,528 Hz NTSC, 31,300 PAL (the core's two samples a line),
     stereo, taken from its ring at the stream's 44.1 kHz.
   - The keyboard: by place, the HID codes of the keys held (keys_raw) to the Amiga's raw codes; Insert is HELP, the left
     and right Windows keys the Amiga keys. F11 the disk menu, F12 (or Ctrl+Alt+End) back to MMBasic, Ctrl+Alt+Del (or
     Ctrl and both Amiga keys, the Amiga's own) a reset. The serial console types too: letters, digits, space, Enter;
     Ctrl-D the disk menu, Ctrl-N the next disk given, Ctrl-] back to MMBasic.
   - The mouse in port 1, the first USB gamepad (of the firmware's HID slots 1-8) a joystick in port 2, a second one in
     port 1 (any face button fire).
   - -click N: the left button pressed at frame N. -fast: no pacing. -stats: the time a frame takes, each 50, and where
     it went (the core's buckets), on the console. -keys: each key's codes on the console. -nowa, -dt, -qos: below. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "cmm2.h"
#include "amiga.h"
#include "uifont.h"                             /* pico-amiga's own menu font (src/pico/ui/UiFont.cpp's), by make.py */

/* The processor's AXI RAM (512 KB at 0x24000000) holds the emulator's code and constants: Musashi's handlers, their
   index and cycle tables, and the chips' (make.py's AXI list, build.py --axi), copied there by cmm2_axi_load() once the
   display is in the SDRAM (Peter, 9 Nov 2026: "put the opcode tables and handlers in the 512K of video memory and run
   the display from sdram"). In the SDRAM they missed the M7's caches at nearly every 68000 instruction. */

/* Write-allocate (-nowa: not; Peter found that worse, 9 Nov 2026): the program's 8 MB (the loader's MPU region at 0xD1800000) write-back with write-allocate (TEX 1) for the run, in
   place of no write-allocate (TEX 0): a store that misses the cache brings the line in, so the framebuffer's row Denise
   has just written is in the cache when it is put out. Put back, the cache written back, at the end */
#define MPU_RNR  (*(volatile uint32_t *)0xE000ED98u)
#define MPU_RBAR (*(volatile uint32_t *)0xE000ED9Cu)
#define MPU_RASR (*(volatile uint32_t *)0xE000EDA0u)
#define DCCMVAC  (*(volatile uint32_t *)0xE000EF68u)
static int wa_region = -1;
static uint32_t wa_rasr;
static void wa_on(void) {
    for (int r = 0; r < 16; r++) {
        MPU_RNR = (uint32_t)r;
        if ((MPU_RBAR & ~0x1Fu) == 0xD1800000u && (MPU_RASR & 1u)) wa_region = r;
    }
    if (wa_region < 0) return;
    MPU_RNR = (uint32_t)wa_region;
    wa_rasr = MPU_RASR;
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RASR = (wa_rasr & ~(7u << 19)) | (1u << 19);
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
}
static void wa_off(void) {
    if (wa_region < 0) return;
    for (uint32_t a = 0xD1800000u; a < 0xD2000000u; a += 32) DCCMVAC = a;
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RNR = (uint32_t)wa_region;
    MPU_RASR = wa_rasr;
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
    wa_region = -1;
}

/* ---- options ---- */
#define MAXDISKS 8
static const char *disk[MAXDISKS];
static int ndisks, curdisk, opt_fast, opt_stats, opt_keys, opt_wa = 1, opt_sound = 1;
/* -noprof: -stats's frame times without the core's buckets (its clock read at each of their changes, some 15 a
   line, costs time of its own); -mute: the sound made and sent but each sample 0 (its cost measured in silence) */
static int opt_noprof, opt_mute;
/* -crc N: each N frames the finished page's checksum (FNV-1a of its pixels) on the console: the same in two builds if
   a change made for speed changed nothing the Amiga shows */
static int opt_crc;
/* -dt N: the DMA2D waits N of its clock's cycles between its transfers on the bus (its AMTCR; 0: none), so that its
   copies of a batch, 63 KB written as fast as it can, leave the LTDC its share of the SDRAM: without, the LTDC ran dry
   while Lotus's demo turned its car, and the monitor lost its sync (Peter, 9 Nov 2026). The DMA2D is the program's */
static int opt_dt = 32;
#define DMA2D_AMTCR (*(volatile uint32_t *)0x5200104Cu)
static uint32_t amtcr_saved;
/* -qos N: the LTDC's reads first at the AXI interconnect (its INI 6 read QoS, 0-15; all are 0 from reset) */
static int opt_qos = -1;
#define AXI_INI6_READ_QOS (*(volatile uint32_t *)0x51047100u)
static uint32_t qos_saved;
static long opt_click = -1;
static const char *kick = "amiga-os-130.rom", *ext;   /* Kickstart 1.3 by Amiga Forever's name for it */
static volatile int quit;
static unsigned long frame;

/* ---- the machine's memory ---- */
static uint8_t chip_ram[512 * 1024] __attribute__((aligned(32)));
static uint8_t slow_ram[512 * 1024] __attribute__((aligned(32)));
/* the core's framebuffer: AMIGA_ROW_RING rows in the AXI RAM after the batches (patches/0001: each row made in one of
   them and put out at its line's end), not 288 rows (245 KB) in the SDRAM, where a finished row had left the cache by
   the time it was read (2.5 ms a frame to read them) */
static amiga_row_t *ring;
static amiga_row_t blank;                       /* a row a frame does not draw (a short frame's last): the border's */
static uint16_t track[AMIGA_TRACK_WORDS];
#define AFRAMES 8192                            /* the core's sound ring, stereo frames (a power of 2) */
static int16_t aring[AFRAMES * 2];

/* ---- the display ---- */
/* The window and the page. NTSC (the default): MODE 8, 640 x 480 at 60 Hz, the Amiga's own rate (each frame shown once,
   at a blanking), the window 640 x 474: 237 rows (an NTSC frame's lines 26 to 262), each row's 640 pixels from its 41st
   (the standard screen's 320 low resolution pixels and no more). PAL (-pal): 576p over MODE 1's page (timing_576p),
   the whole screen, all 288 rows of 720 pixels (Peter, 9 Nov 2026: "If we change to NTSC mode then everything can work in
   640x480 with massively reduced bus bandwidth": the LTDC reads 36 MB/s, 50 at most, where MODE 1's read 62 and 99) */
/* -pal60: a PAL Amiga on the NTSC display, as pico-amiga's 640 x 480 at 60 Hz (Peter, 10 Nov 2026: "we should try the
   pico-amiga approach next 640x512 windowed onto 640x480 as an experiment"): one PAL frame (313 lines) a display frame,
   so 20% faster than a real one, the window the standard 320 x 256 screen's middle 240 rows (the framebuffer's 26 to 265:
   its row 18 is the screen's first) and 320 columns. A PAL title's waits for a line past NTSC's 262 come to an end */
static int opt_pal = 1, opt_576 = 1;            /* a PAL Amiga (not -ntsc); shown in 576p (not -pal60 or -ntsc) */
static int PW = 640, PH = 474, ROWS = 237, SX = 40, SY = 0, SCR_W = 640, SCR_H = 480, VMODE = 8;
#define LTDC_BPCR     (*(volatile uint32_t *)0x5000100Cu)
#define LTDC_SRCR     (*(volatile uint32_t *)0x50001024u)
#define LTDC_BCCR     (*(volatile uint32_t *)0x5000102Cu)
#define LTDC_L1WHPCR  (*(volatile uint32_t *)0x50001088u)
#define LTDC_L1WVPCR  (*(volatile uint32_t *)0x5000108Cu)
#define LTDC_L1CFBAR  (*(volatile uint32_t *)0x500010ACu)
#define LTDC_L1CFBLR  (*(volatile uint32_t *)0x500010B0u)
#define LTDC_L1CFBLNR (*(volatile uint32_t *)0x500010B4u)
static uint32_t ltdc_saved[6];
static volatile uint32_t border_rgb;
#define NPAGES 3
static uint16_t *page[NPAGES];
static int npages = 2;
static volatile int shown, flip_to = -1;
static volatile uint32_t issued;                /* the copies given to the DMA2D, ever */
static volatile uint32_t frame_mark;            /* issued, once the copies of the page waiting to be shown were */
static uint32_t completed(void) {               /* the copies done, ever (issued first: a copy issued meanwhile makes it
                                                   less, never more) */
    const uint32_t i = issued;
    return i - (uint32_t)cmm2->dma2d_busy();
}

static volatile uint32_t flips, late_flips, late_max;   /* (-stats: flips made in the picture's lines, not the blanking) */
static void hook(void) {                        /* the start of each frame's blanking (PendSV): a finished page shown */
    LTDC_BCCR = border_rgb;
    if (flip_to >= 0 && (int32_t)(completed() - frame_mark) >= 0) {
        const uint32_t y = *(volatile uint32_t *)0x50001044u & 0xFFFF;   /* (LTDC_CPSR's line) */
        const uint32_t avbp = LTDC_BPCR & 0x7FF, aah = *(volatile uint32_t *)0x50001010u & 0x7FF;
        LTDC_L1CFBAR = (uint32_t)page[flip_to];
        LTDC_SRCR = 1;                          /* (reload now: in the blanking) */
        shown = flip_to;
        flip_to = -1;
        flips++;
        if (y > avbp && y <= aah) { late_flips++; if (y - avbp > late_max) late_max = y - avbp; }
    }
}
static void window_on(void) {                   /* the layer cut to the window, page 0 in it */
    ltdc_saved[0] = LTDC_L1WHPCR; ltdc_saved[1] = LTDC_L1WVPCR; ltdc_saved[2] = LTDC_L1CFBAR;
    ltdc_saved[3] = LTDC_L1CFBLR; ltdc_saved[4] = LTDC_L1CFBLNR; ltdc_saved[5] = LTDC_BCCR;
    const uint32_t ahbp = LTDC_BPCR >> 16 & 0xFFF, avbp = LTDC_BPCR & 0x7FF;
    const uint32_t x0 = ahbp + 1 + (SCR_W - PW) / 2, y0 = avbp + 1 + (SCR_H - PH) / 2;
    LTDC_L1WHPCR = (x0 + PW - 1) << 16 | x0;
    LTDC_L1WVPCR = (y0 + PH - 1) << 16 | y0;
    LTDC_L1CFBLR = (uint32_t)(PW * 2) << 16 | (PW * 2 + 7);
    LTDC_L1CFBLNR = PH;
    LTDC_L1CFBAR = (uint32_t)page[0];
    LTDC_BCCR = 0;
    LTDC_SRCR = 1;
}
static void window_off(void) {                  /* and as MMBasic's mode had it */
    LTDC_L1WHPCR = ltdc_saved[0]; LTDC_L1WVPCR = ltdc_saved[1]; LTDC_L1CFBAR = ltdc_saved[2];
    LTDC_L1CFBLR = ltdc_saved[3]; LTDC_L1CFBLNR = ltdc_saved[4]; LTDC_BCCR = ltdc_saved[5];
    LTDC_SRCR = 1;
}

/* -pal: 576p, 720 x 576 at 50.00 Hz (CEA-861's: 864 x 625, 27.000 MHz, both syncs low), the PAL picture's own size and
   rate, set over MODE 1's page by the program itself, as vtest.app did (Peter, 10 Nov 2026: "Try the 720x576"): each
   frame shown once, as NTSC's are. In MODE 1 (800 x 600 at 75 Hz, twice the SDRAM's reads) the emulation fell 5% behind
   and the sound with it. PLL3 (the pixel clock: 8 MHz x 81 / 24, its input as MODE 1 has it) and the timing are put
   back at the end. The firmware's frame interrupt stays at MODE 1's line 624 (ltdc.c: its AccumulatedActiveH + 1),
   576p's last, in its blanking */
#define LTDC_SSCR     (*(volatile uint32_t *)0x50001008u)
#define LTDC_AWCR     (*(volatile uint32_t *)0x50001010u)
#define LTDC_TWCR     (*(volatile uint32_t *)0x50001014u)
#define LTDC_GCR      (*(volatile uint32_t *)0x50001018u)
#define LTDC_CPSR     (*(volatile uint32_t *)0x50001044u)
#define RCC_CR        (*(volatile uint32_t *)0x58024400u)
#define RCC_PLL3DIVR  (*(volatile uint32_t *)0x58024440u)
static uint32_t tm_saved[6];                    /* SSCR, BPCR, AWCR, TWCR, GCR, PLL3DIVR */
static int tm_set;
static int pll3_set(uint32_t divr) {            /* PLL3 off, its dividers, on again (as ltdc.c's PLL3_Config) */
    RCC_CR &= ~(1u << 28);
    for (int i = 0; RCC_CR & (1u << 29); i++) if (i > 1000000) return 0;
    RCC_PLL3DIVR = divr;
    RCC_CR |= 1u << 28;
    for (int i = 0; !(RCC_CR & (1u << 29)); i++) if (i > 1000000) return 0;
    return 1;
}
static void blank_wait(void) {                  /* (the LTDC's line past the picture, or 40 ms) */
    const uint64_t end = cmm2->micros() + 40000;
    while (cmm2->micros() < end && (LTDC_CPSR & 0xFFFF) <= (LTDC_AWCR & 0x7FF)) { }
}
static int timing_576p(void) {
    tm_saved[0] = LTDC_SSCR; tm_saved[1] = LTDC_BPCR; tm_saved[2] = LTDC_AWCR; tm_saved[3] = LTDC_TWCR;
    tm_saved[4] = LTDC_GCR; tm_saved[5] = RCC_PLL3DIVR;
    const uint32_t hsw = 64, hbp = 68, hfp = 12, vsw = 5, vbp = 39, vfp = 5, w = 720, h = 576;
    const uint32_t ahbp = hsw + hbp - 1, avbp = vsw + vbp - 1, aaw = ahbp + w, aah = avbp + h;
    blank_wait();
    if (!pll3_set((tm_saved[5] & ~(0x1FFu | (0x7Fu << 24))) | (81u - 1) | ((24u - 1) << 24))) { pll3_set(tm_saved[5]); return 0; }
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
    if (!tm_set) return;
    blank_wait();
    pll3_set(tm_saved[5]);
    LTDC_SSCR = tm_saved[0]; LTDC_BPCR = tm_saved[1]; LTDC_AWCR = tm_saved[2]; LTDC_TWCR = tm_saved[3];
    LTDC_GCR = tm_saved[4];
    LTDC_SRCR = 1;
    tm_set = 0;
}

/* The rows put out as the frame goes on: as Denise finishes each (row_done: patches/0001's AMIGA_ROW_DONE_ALL), its 720
   pixels go into one of two batches of 22 rows in the AXI RAM after the code and variables (62 KB), and a full batch
   goes to the page being made by two DMA2D copies, each row on the page's even line and on its odd one, while the
   emulation goes on; a batch's buffer is written again once its copies are done. The rows a frame does not reach (a short
   frame's last) go out as the border at its end. Room in the DMA2D's queue (three) is looked for
   first: dma2d_copy writes the source back from the cache before it looks */
extern char __axi_end[];
#define CH 22                                   /* rows a batch */
static uint16_t *batch[2];
static uint32_t buf_mark[2];                    /* issued, once the last copies from each buffer were */
static int next_row;                            /* the frame's next row to put out */
static int back_pg = 1;                         /* the page the frame goes to */
static uint64_t us_wait, us_out, us_queue, us_copy, us_read;
#define T() (*(volatile uint32_t *)0xE0001004u)   /* (the cycle counter: micros(), a service, cost 5% at each row) */
static void copy_out(const uint16_t *src, int row0, int n) {
    for (int k = 0; k < 2; k++) {
        uint32_t t0 = T();
        while (cmm2->dma2d_busy() >= 3) { }
        us_queue += T() - t0;
        t0 = T();
        /* (the second copy of the batch by dma2d_copy_nc when the firmware has it, services 18: the first has just
           written the batch back from the cache, and nothing has written to it since) */
        if (k && cmm2->version >= 18)
            cmm2->dma2d_copy_nc(src + SX, AMIGA_OUT_W * 2, page[back_pg] + (2 * row0 + k) * PW, 2 * PW * 2, PW * 2, (uint32_t)n);
        else
            cmm2->dma2d_copy(src + SX, AMIGA_OUT_W * 2, page[back_pg] + (2 * row0 + k) * PW, 2 * PW * 2, PW * 2, (uint32_t)n);
        us_copy += T() - t0;
        issued++;
    }
}
/* a row's 720 pixels, as amiga_fb_line makes them (its first line), but 32 bits at a time for the two kinds of row most
   screens are: a low resolution row (each 0x0RGB twice) and a high resolution one (each index through the row's
   palette); the interlaced kind by amiga_fb_line. The row (852 bytes, the framebuffer 32-aligned) and its pixels are
   4-aligned, as out is */
/* each halfword of v twice, by PKHBT and PKHTB (one instruction each: GCC makes three or four of a | a << 16 and
   never these from C) */
static inline uint32_t dup_lo(uint32_t v) { uint32_t r; __asm__("pkhbt %0, %1, %1, lsl #16" : "=r"(r) : "r"(v)); return r; }
static inline uint32_t dup_hi(uint32_t v) { uint32_t r; __asm__("pkhtb %0, %1, %1, asr #16" : "=r"(r) : "r"(v)); return r; }
static void fb_line_fast(const amiga_row_t *row, uint16_t *out) {
    uint32_t *o = (uint32_t *)out;
    if (row->mode == AMIGA_ROW_DIRECT) {
        const uint32_t *s = (const uint32_t *)row->px.rgb;
        for (int x = 0; x < AMIGA_FB_W / 2; x++) {
            const uint32_t v = s[x];
            o[2 * x] = dup_lo(v);
            o[2 * x + 1] = dup_hi(v);
        }
    } else if (row->mode == AMIGA_ROW_IDX8) {
        const uint32_t *s = (const uint32_t *)row->px.idx;
        const uint16_t *pal = row->pal;
        for (int x = 0; x < AMIGA_OUT_W / 4; x++) {
            const uint32_t v = s[x];
            o[2 * x] = pal[v & 63] | (uint32_t)pal[v >> 8 & 63] << 16;
            o[2 * x + 1] = pal[v >> 16 & 63] | (uint32_t)pal[v >> 24 & 63] << 16;
        }
    } else
        amiga_fb_line(row, row->pad & 1, out);  /* (interlaced: the line of the field drawn, patches/0001's w_begin) */
}
static uint16_t border_px;                      /* the border's colour, 0x0RGB: the picture's top left */
static const amiga_row_t *blank_row(void) {
    blank.mode = AMIGA_ROW_DIRECT;
    for (int x = 0; x < AMIGA_FB_W; x++) blank.px.rgb[x] = border_px;
    return &blank;
}
static void put_row(int row, const amiga_row_t *r) {
    const int b = (row / CH) & 1, i = row % CH;
    if (i == 0) {                               /* a batch begins: its buffer's last copies done */
        const uint32_t t0 = T();
        while ((int32_t)(completed() - buf_mark[b]) < 0) { }
        us_wait += T() - t0;
    }
    uint16_t *dst = batch[b] + i * AMIGA_OUT_W;
#ifdef ROW_READ_TEST
    {                                           /* (a test: the time to read the row alone, before it is put out) */
        static volatile uint32_t sink;
        const uint32_t t0 = T(), *w = (const uint32_t *)r;
        uint32_t s = 0;
        for (unsigned k = 0; k < sizeof *r / 4; k++) s += w[k];
        sink = s;
        us_read += T() - t0;
    }
#endif
    fb_line_fast(r, dst);
    if (row == 0) {                             /* the border: the picture's top left */
        border_px = dst[0];
        border_rgb = (uint32_t)((dst[0] >> 8 & 15) * 17) << 16 | (uint32_t)((dst[0] >> 4 & 15) * 17) << 8 | (uint32_t)((dst[0] & 15) * 17);
    }
    if (i == CH - 1 || row == ROWS - 1) {
        copy_out(batch[b], row - i, i + 1);
        buf_mark[b] = issued;
    }
}
static void mid_frame_poll(void);
static void row_done(void *ctx, int row, amiga_row_t *r) {
    (void)ctx;
    mid_frame_poll();
    row -= SY;                                  /* (the window's first row: -pal60's) */
    if (row < next_row || row >= ROWS) return;
    const uint32_t t0 = T();
    while (next_row < row) { put_row(next_row, blank_row()); next_row++; }   /* (rows not drawn: the border) */
    put_row(row, r);
    next_row = row + 1;
    us_out += T() - t0;
}
static void present(void) {                     /* the frame's end: its last rows out, its page shown once copied */
    uint32_t t0 = T();
    while (next_row < ROWS) { put_row(next_row, blank_row()); next_row++; }
    us_out += T() - t0;
    t0 = T();
    /* The frame before shown first (at a blanking), so that none is dropped and the emulation goes no faster than
       the display's 60 a second (576p's 50 with -pal); this one is made while that one waits, in the third page, so that a frame which takes
       longer than a display frame is shown at the next blanking and costs only its own overrun (waiting for each frame to
       be shown before the next was begun made each such frame take two: Lotus's race at 30 frames a second) */
    if (!opt_fast) while (flip_to >= 0 && !quit) { }
    if (opt_crc > 0 && frame % (unsigned long)opt_crc == 0) {
        while ((int32_t)(completed() - issued) < 0) { }
        const uint32_t *w = (const uint32_t *)page[back_pg];
        uint32_t h = 2166136261u;
        for (int i = 0; i < PW * PH / 2; i++) h = (h ^ w[i]) * 16777619u;
        printf("amiga: frame %lu crc %08lX\n", frame, (unsigned long)h);
    }
    frame_mark = issued;
    __asm volatile ("dmb 0xF" ::: "memory");
    flip_to = back_pg;                          /* (PAL: one not shown yet when this comes is not shown) */
    if (npages == 2) while (flip_to >= 0 && !opt_fast) { }   /* (two pages: the next frame's is the one shown till then) */
    int pg = 0;
    while (pg == shown || pg == flip_to) pg++;  /* (a page neither shown nor waiting to be) */
    back_pg = pg;
    next_row = 0;
    us_wait += T() - t0;
}

/* ---- the sound: the core's ring (two samples a line: 31,300 Hz PAL, 31,528 NTSC) to the stream (44.1 kHz), each sample
   between two of the ring's ---- */
static int audio_rate, core_rate = 31528;
/* The ring's frame the stream is at: its whole part (as the core's count, which goes on for ever) and its fraction (16
   bits) apart. As one 16.16 word the whole part went round at 65,536, two seconds in, and from then on each sample was
   "too far behind" and the ring's newest but 1,500 taken for the whole of the stream's block: a low crackle and no
   music (Peter, 10 Nov 2026) */
static uint32_t a_k, a_f;
static int a_started;
#define A_LAG 1500                              /* frames behind the core: 48 ms */
static void fill(int16_t *buf, uint32_t frames) {   /* the stream's (PendSV) */
    const uint32_t wp = amiga_audio_write_pos();
    if (!a_started) {
        if (wp < A_LAG) { memset(buf, 0, frames * 4); return; }
        a_k = wp - A_LAG; a_f = 0;
        a_started = 1;
    }
    /* the core makes its samples at the display's rate (NTSC: 2 x 263 lines x 60.03 frames, 31,576 a second), not quite
       core_rate's: the step is trimmed (up to 0.4%) to hold the lag near A_LAG, or the ring went round each two minutes */
    int32_t err = (int32_t)(wp - a_k) - A_LAG;
    if (err > 1600) err = 1600; if (err < -1600) err = -1600;
    const uint32_t step = (uint32_t)((((uint64_t)core_rate << 16) / (uint32_t)audio_rate) + err / 8);
    static int16_t last_l, last_r;
    for (uint32_t i = 0; i < frames; i++) {
        if (wp - a_k > AFRAMES - 512) { a_k = wp - A_LAG; a_f = 0; }   /* (too far behind: the ring went round) */
        if ((int32_t)(wp - a_k) < 2) { buf[2 * i] = last_l; buf[2 * i + 1] = last_r; continue; }   /* (caught up: the core is slow) */
        const uint32_t j0 = a_k & (AFRAMES - 1), j1 = (a_k + 1) & (AFRAMES - 1);
        buf[2 * i] = last_l = (int16_t)(aring[2 * j0] + (((aring[2 * j1] - aring[2 * j0]) * (int32_t)a_f) >> 16));
        buf[2 * i + 1] = last_r = (int16_t)(aring[2 * j0 + 1] + (((aring[2 * j1 + 1] - aring[2 * j0 + 1]) * (int32_t)a_f) >> 16));
        a_f += step;
        a_k += a_f >> 16;
        a_f &= 0xFFFF;
    }
    if (opt_mute) memset(buf, 0, frames * 4);
}

/* ---- the keyboard ---- */
/* HID usage (4-0x65) to the Amiga's raw key code, by place (0: none, but ` is the Amiga's 0: below) */
static const uint8_t hid2amiga[0x66] = {
    [0x04] = 0x20, [0x05] = 0x35, [0x06] = 0x33, [0x07] = 0x22, [0x08] = 0x12, [0x09] = 0x23, [0x0A] = 0x24, [0x0B] = 0x25,   /* A-H */
    [0x0C] = 0x17, [0x0D] = 0x26, [0x0E] = 0x27, [0x0F] = 0x28, [0x10] = 0x37, [0x11] = 0x36, [0x12] = 0x18, [0x13] = 0x19,   /* I-P */
    [0x14] = 0x10, [0x15] = 0x13, [0x16] = 0x21, [0x17] = 0x14, [0x18] = 0x16, [0x19] = 0x34, [0x1A] = 0x11, [0x1B] = 0x32,   /* Q-X */
    [0x1C] = 0x15, [0x1D] = 0x31,                                                                                          /* Y, Z */
    [0x1E] = 0x01, [0x1F] = 0x02, [0x20] = 0x03, [0x21] = 0x04, [0x22] = 0x05, [0x23] = 0x06, [0x24] = 0x07, [0x25] = 0x08,   /* 1-8 */
    [0x26] = 0x09, [0x27] = 0x0A,                                                                                          /* 9, 0 */
    [0x28] = 0x44, [0x29] = 0x45, [0x2A] = 0x41, [0x2B] = 0x42, [0x2C] = 0x40,                                             /* Return Esc BS Tab Space */
    [0x2D] = 0x0B, [0x2E] = 0x0C, [0x2F] = 0x1A, [0x30] = 0x1B, [0x31] = 0x0D, [0x32] = 0x2B,                              /* - = [ ] \ # */
    [0x33] = 0x29, [0x34] = 0x2A, [0x36] = 0x38, [0x37] = 0x39, [0x38] = 0x3A, [0x39] = 0x62,                              /* ; ' , . / Caps */
    [0x3A] = 0x50, [0x3B] = 0x51, [0x3C] = 0x52, [0x3D] = 0x53, [0x3E] = 0x54, [0x3F] = 0x55, [0x40] = 0x56, [0x41] = 0x57,   /* F1-F8 */
    [0x42] = 0x58, [0x43] = 0x59,                                                                                          /* F9, F10 */
    [0x49] = 0x5F, [0x4C] = 0x46,                                                                                          /* Insert: HELP, Delete */
    [0x4F] = 0x4E, [0x50] = 0x4F, [0x51] = 0x4D, [0x52] = 0x4C,                                                            /* the arrows */
    [0x54] = 0x5C, [0x55] = 0x5D, [0x56] = 0x4A, [0x57] = 0x5E, [0x58] = 0x43,                                             /* keypad / * - + Enter */
    [0x59] = 0x1D, [0x5A] = 0x1E, [0x5B] = 0x1F, [0x5C] = 0x2D, [0x5D] = 0x2E, [0x5E] = 0x2F, [0x5F] = 0x3D, [0x60] = 0x3E,   /* keypad 1-8 */
    [0x61] = 0x3F, [0x62] = 0x0F, [0x63] = 0x3C, [0x64] = 0x30, [0x65] = 0x67,                                             /* 9 0 . <> Menu */
};
static const uint8_t mod2amiga[8] = {0x63, 0x60, 0x64, 0x66, 0x63, 0x61, 0x65, 0x67};   /* HID's modifier bits */
static uint8_t held[0x68];                      /* the Amiga keys held, as keys_raw gave them */

static void next_disk(void);
static void menu_open(void);
static uint8_t mute[0x68];                      /* keys held as the disk menu went: not the Amiga's till let go (its Enter) */
static int mute_all, f11_was;
static void keyboard(void) {
    uint8_t r[8], now[0x68];
    memset(now, 0, sizeof now);
    static int rst_was;
    int f11 = 0, rst = 0;
    const int n = cmm2->keys_raw(r);
    if (opt_keys) {                             /* (-keys: each change of the report on the console) */
        static uint8_t last[8];
        static int last_n = -99;
        if (n != last_n || memcmp(r, last, 8)) {
            printf("amiga: keys_raw %d: %02X %02X %02X %02X %02X %02X %02X %02X\n", n, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
            memcpy(last, r, 8); last_n = n;
        }
    }
    if (n >= 0) {
        const int ctrl = r[0] & 0x11, alt = r[0] & 0x44;
        for (int b = 0; b < 8; b++) if (r[0] & (1 << b)) now[mod2amiga[b]] = 1;
        for (int i = 2; i < 8; i++) {
            const uint8_t u = r[i];
            if (u == 0x44) { f11 = 1; continue; }                                               /* F11: the disk menu */
            if (u == 0x45 || (u == 0x4D && ctrl && alt)) { quit = 1; continue; }                /* F12, Ctrl+Alt+End */
            if (u == 0x4C && ctrl && alt) { rst = 1; continue; }   /* Ctrl+Alt+Del: a reset, as in the C64 app (Peter, 10 Nov 2026) */
            if (u == 0x35) now[0x00] = 1;       /* (` is the Amiga's code 0) */
            else if (u < sizeof hid2amiga && hid2amiga[u]) now[hid2amiga[u]] = 1;
        }
        if (ctrl && (r[0] & 0x08) && (r[0] & 0x80)) rst = 1;   /* Ctrl and both Amiga keys: the Amiga's own reset */
    }
    if (rst && !rst_was && !opt_crc) { amiga_reset(false); printf("amiga: reset\n"); }   /* (a warm one, as the keyboard's) */
    rst_was = rst;
    if (mute_all) { memcpy(mute, now, sizeof mute); mute_all = 0; }
    for (int c = 0; c < 0x68; c++) {
        if (mute[c]) { if (now[c]) continue; mute[c] = 0; }
        if (now[c] != held[c]) { if (!opt_crc) amiga_key((uint8_t)c, now[c] != 0); held[c] = now[c]; }   /* (-crc: none) */
    }
    /* the serial console: letters, digits, space, Enter; Ctrl-N the next disk, Ctrl-] back to MMBasic. A USB key's
       character reaches the console's queue as well (KeyboardMap.c): the queue is emptied, not read, while a USB key is
       held and at the poll after, as the C64 and Atari apps do (Peter, 9 Nov 2026: in Notepad "every keypress on the
       keyboard repeated twice") */
    static int quiet;
    const int ok = n <= 0 && quiet;
    quiet = n <= 0;
    for (int k; (k = cmm2->con_getc()) >= 0; ) {
        if (!ok) continue;
        static const char *row1 = "1234567890", *rowq = "qwertyuiop", *rowa = "asdfghjkl", *rowz = "zxcvbnm";
        int code = -1;
        if (k == 0x1D) { quit = 1; continue; }
        if (k == 0x04) { menu_open(); break; }  /* (Ctrl-D: the disk menu) */
        if (k == 0x0E) { next_disk(); continue; }
        if (k >= 'A' && k <= 'Z') k += 32;
        const char *p;
        if (k && (p = strchr(row1, k))) code = 0x01 + (int)(p - row1);
        else if (k && (p = strchr(rowq, k))) code = 0x10 + (int)(p - rowq);
        else if (k && (p = strchr(rowa, k))) code = 0x20 + (int)(p - rowa);
        else if (k && (p = strchr(rowz, k))) code = 0x31 + (int)(p - rowz);
        else if (k == ' ') code = 0x40;
        else if (k == '\r' || k == '\n') code = 0x44;
        else if (k == 8 || k == 127) code = 0x41;
        else if (k == 27) code = 0x45;
        if (code >= 0) { amiga_key((uint8_t)code, true); amiga_key((uint8_t)code, false); }
    }
    const int open = f11 && !f11_was;
    f11_was = f11;
    if (open) menu_open();
}

/* ---- the mouse (port 1) and a joystick (port 2) ---- */
/* The mouse as the C64 app's 1351 is (Peter, 10 Nov 2026: in Workbench "Fast movements of the mouse can be missed or
   move backwards"): the firmware keeps the pointer clamped to its screen, so it is read every 2 ms of the frame, with
   the USB polled first (the Quake port's lesson: once a frame, a fast mouse's reports were lost), and put back in the
   middle each time, its movement kept as a backlog. The Amiga's counters are 8 bits and its OS reads them once a frame:
   more than about 127 between two readings would look like a move the other way, so at most 100 goes to them a
   frame and the rest the next (a backlog of more than four frames' worth is let go) */
#define MOUSE_FRAME 100
#define MOUSE_BACKLOG 400
static int mouse_here, mouse_bx, mouse_by, mouse_btn, click_down;
static uint32_t poll_t, poll_cyc;
static void mouse_task(void) {
    int m[4];
    if (!cmm2->mouse(m)) { mouse_here = 0; mouse_btn = 0; return; }
    if (!mouse_here) { mouse_here = 1; cmm2->mouse_set(320, 256); return; }   /* (first seen: from the middle) */
    const int dx = m[0] - 320, dy = m[1] - 256;
    if (dx || dy) {
        cmm2->mouse_set(320, 256);
        mouse_bx += dx; mouse_by += dy;
        if (mouse_bx > MOUSE_BACKLOG) mouse_bx = MOUSE_BACKLOG; if (mouse_bx < -MOUSE_BACKLOG) mouse_bx = -MOUSE_BACKLOG;
        if (mouse_by > MOUSE_BACKLOG) mouse_by = MOUSE_BACKLOG; if (mouse_by < -MOUSE_BACKLOG) mouse_by = -MOUSE_BACKLOG;
    }
    mouse_btn = m[2];
}
static void mid_frame_poll(void) {              /* (row_done's: each 2 ms of the frame) */
    const uint32_t now = T();
    if (now - poll_t < poll_cyc) return;
    poll_t = now;
    cmm2->poll();
    mouse_task();
}
static void ports(void) {
    mouse_task();
    int dx = mouse_bx, dy = mouse_by;
    if (dx > MOUSE_FRAME) dx = MOUSE_FRAME; if (dx < -MOUSE_FRAME) dx = -MOUSE_FRAME;
    if (dy > MOUSE_FRAME) dy = MOUSE_FRAME; if (dy < -MOUSE_FRAME) dy = -MOUSE_FRAME;
    mouse_bx -= dx; mouse_by -= dy;
    if (opt_crc) return;                        /* (-crc: the Amiga sees no mouse or pad, so that a hand on them changes no
                                                   frame of a run whose checksums are compared) */
    amiga_mouse(dx, dy, (mouse_btn & 1) || click_down, (mouse_btn & 2) != 0);
    /* the pads: the first in the firmware's HID slots (1-8: the keyboard and the mouse have slots too) the joystick in
       port 2, a second one the joystick in port 1 (a two-player game's), each the d-pad or the left stick, any face
       button fire (DEVICE(GAMEPAD n, B)'s bitmap: up 7, down 5, left 8, right 6, the face buttons 10-13) */
    int n = 0;
    for (int slot = 1; slot <= 8 && n < 2; slot++) {
        int lx = 128, ly = 128, dirs = 0;
        const int b = cmm2->gamepad(slot, &lx, &ly);
        if (b < 0) continue;
        if ((b >> 7 & 1) || ly < 64) dirs |= 1;
        if ((b >> 5 & 1) || ly > 192) dirs |= 2;
        if ((b >> 8 & 1) || lx < 64) dirs |= 4;
        if ((b >> 6 & 1) || lx > 192) dirs |= 8;
        amiga_joystick(n == 0 ? 1 : 0, (uint8_t)dirs, (b >> 10 & 15) != 0);
        n++;
    }
    if (n == 0) amiga_joystick(1, 0, false);
}

/* ---- the disks (Peter, 10 Nov 2026: "We say that anything saved is lost. This would be very bad for someone doing any
   program development - can this be solved"): DF0 and DF1, each image held in memory for the reads, and what the Amiga
   writes put into the image and into its .adf file on the card too, as pico-amiga's own platform does
   (src/pico/machine.cpp's adf_write), so that it lasts, as on a real disk. The file is synced once the frame is done
   (sync_disks), not at each of a track's eleven sectors. A file that cannot be opened for writing (read-only), or any
   with -wp, is a write-protected disk. One file is never in both drives: two writers ---- */
#define DISK_BYTES 901120                       /* an ADF: 80 cylinders, 2 sides, 11 sectors of 512 bytes */
typedef struct { uint8_t *img; int fh, dirty; char path[256]; } drive_t;   /* fh: the file open for writing (-1: none) */
static drive_t drv[2] = { { NULL, -1, 0, "" }, { NULL, -1, 0, "" } };
static int opt_wp;
static const char *opt_df1;
static char mmsg[64];                           /* the disk menu's message (a disk that did not load) */
static char cwd0[192] = "A:/";                  /* the folder the program started in */
static int disk_read(void *ctx, uint32_t off, void *dst, uint32_t len) {
    const drive_t *d = ctx;
    if (off + len > DISK_BYTES) return -1;
    memcpy(dst, d->img + off, len);
    return 0;
}
static int disk_write(void *ctx, uint32_t off, const void *src, uint32_t len) {
    drive_t *d = ctx;
    if (off + len > DISK_BYTES) return -1;
    memcpy(d->img + off, src, len);
    if (d->fh >= 0) {
        if (cmm2->file_seek(d->fh, off) < 0 || cmm2->file_write(d->fh, src, len) != (int32_t)len) {
            printf("amiga: DF%d: writing to %s failed\n", (int)(d - drv), d->path);
            return -1;
        }
        d->dirty = 1;
    }
    return 0;
}
static void sync_disks(void) {
    for (int i = 0; i < 2; i++)
        if (drv[i].dirty) { cmm2->file_sync(drv[i].fh); drv[i].dirty = 0; }
}
static void drives_close(void) {                /* (the files only: at the end, when the core's code may have gone) */
    sync_disks();
    for (int i = 0; i < 2; i++)
        if (drv[i].fh >= 0) { cmm2->file_close(drv[i].fh); drv[i].fh = -1; }
}
static void path_join(char *out, size_t n, const char *dir, const char *name) {
    const size_t l = strlen(dir);
    snprintf(out, n, "%s%s%s", dir, l && dir[l - 1] == '/' ? "" : "/", name);
}
static void path_abs(char *out, size_t n, const char *p) {   /* a name as given (from the folder started in, or A:/...) in full */
    if (p[0] && p[1] == ':') snprintf(out, n, "%s", p);
    else if (p[0] == '/') snprintf(out, n, "%c:%s", cwd0[0], p);
    else { while (p[0] == '.' && p[1] == '/') p += 2; path_join(out, n, cwd0, p); }
}
static const char *path_dir(char *dir, size_t n, const char *path) {   /* path's folder into dir (A:/'s / kept), its name returned */
    const char *s = strrchr(path, '/');
    if (!s) { snprintf(dir, n, "%s", cwd0); return path; }
    snprintf(dir, n, "%.*s", (int)(s - path) + (s == path + 2), path);
    return s + 1;
}
static void drive_out(int n) {                  /* drive n's disk out, its file closed */
    drive_t *d = &drv[n];
    amiga_eject_disk(n);
    if (d->fh >= 0) {
        if (d->dirty) cmm2->file_sync(d->fh);
        cmm2->file_close(d->fh);
    }
    d->fh = -1;
    d->dirty = 0;
    d->path[0] = 0;
}
/* path in drive n: as a disk change (the core keeps the drive empty for 2 s first if a disk was in, for Kickstart to see
   it), or (boot) the Amiga restarted with it in DF0, as at power-on */
static int drive_in(int n, const char *path, int boot) {
    char full[256];
    path_abs(full, sizeof full, path);
    const char *b = strrchr(full, '/');
    b = b ? b + 1 : full;
    if (drv[1 - n].path[0] && !strcasecmp(drv[1 - n].path, full)) drive_out(1 - n);   /* (out of the other drive) */
    drive_out(n);
    drive_t *d = &drv[n];
    if (!d->img && !(d->img = cmm2->alloc(DISK_BYTES)) && !(d->img = malloc(DISK_BYTES))) {
        printf("amiga: no room for a disk in DF%d\n", n);
        snprintf(mmsg, sizeof mmsg, "No room for a disk in DF%d", n);
        return 0;
    }
    int fh = opt_wp ? -1 : cmm2->file_open(full, CMM2_O_READ | CMM2_O_WRITE);
    const int rw = fh >= 0;
    if (!rw) fh = cmm2->file_open(full, CMM2_O_READ);
    if (fh < 0) {
        printf("amiga: cannot open %s\n", full);
        snprintf(mmsg, sizeof mmsg, "Cannot open %s", b);
        return 0;
    }
    if (cmm2->file_size(fh) < DISK_BYTES || cmm2->file_read(fh, d->img, DISK_BYTES) != DISK_BYTES) {
        cmm2->file_close(fh);
        printf("amiga: %s is not an ADF (880 KB)\n", full);
        snprintf(mmsg, sizeof mmsg, "%s is not a disk image", b);
        return 0;
    }
    if (rw) d->fh = fh;
    else cmm2->file_close(fh);
    snprintf(d->path, sizeof d->path, "%s", full);
    amiga_disk_t ad = { disk_read, rw ? disk_write : NULL, d, DISK_BYTES };
    amiga_insert_disk(n, &ad);
    if (boot) amiga_reset(true);
    printf("amiga: DF%d %s%s%s\n", n, full, rw ? "" : " (write-protected)", boot ? ", the Amiga restarted" : "");
    return 1;
}
static void next_disk(void) {                   /* (Ctrl-N on the serial console: the next disk given, in DF0) */
    if (ndisks < 2) return;
    curdisk = (curdisk + 1) % ndisks;
    drive_in(0, disk[curdisk], 0);
}

/* A blank disk, pico-amiga's machine_format_adf (src/pico/machine.cpp): AmigaDOS's OFS, not bootable, its root block and
   bitmap made, the volume named, dated by the clock; written 22 blocks at a time */
static void put32(uint8_t *b, int off, uint32_t v) {
    b[off] = (uint8_t)(v >> 24); b[off + 1] = (uint8_t)(v >> 16);
    b[off + 2] = (uint8_t)(v >> 8); b[off + 3] = (uint8_t)v;
}
static void block_sum(uint8_t *b, int off) {    /* AmigaDOS block checksum: the longs sum to 0 */
    uint32_t s = 0;
    for (int i = 0; i < 512; i += 4)
        s += ((uint32_t)b[i] << 24) | ((uint32_t)b[i + 1] << 16) | ((uint32_t)b[i + 2] << 8) | b[i + 3];
    put32(b, off, (uint32_t)-s);
}
static uint32_t amiga_days(int y, int m, int d) {   /* days since 1 January 1978, AmigaDOS's dates */
    static const int before[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    if (y < 1978 || m < 1 || m > 12 || d < 1) return 0;
    uint32_t days = 0;
    for (int i = 1978; i < y; i++) days += 365 + ((i % 4 == 0 && i % 100 != 0) || i % 400 == 0);
    return days + (uint32_t)(before[m - 1] + d - 1) + (m > 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0));
}
static int format_adf(const char *path, const char *volume) {
    const int fh = cmm2->file_open(path, CMM2_O_WRITE | CMM2_O_CREATE);
    if (fh < 0) return 0;
    int t[6];
    cmm2->datetime(t);
    const uint32_t days = amiga_days(t[0], t[1], t[2]), mins = (uint32_t)(t[3] * 60 + t[4]), ticks = (uint32_t)t[5] * 50;
    const size_t vlen = strlen(volume) < 30 ? strlen(volume) : 30;
    static const int dates[3] = { 0x1A4, 0x1D8, 0x1E4 };   /* root, volume, creation */
    static uint8_t chunk[22 * 512];
    int ok = 1;
    for (int n = 0; n < 1760 && ok; n++) {
        uint8_t *blk = chunk + (n % 22) * 512;
        memset(blk, 0, 512);
        if (n == 0) {
            memcpy(blk, "DOS", 4);                          /* DOS\0: OFS, not bootable */
            put32(blk, 8, 880);
        } else if (n == 880) {                              /* the root block */
            put32(blk, 0x000, 2);                           /* T_HEADER */
            put32(blk, 0x00C, 72);                          /* hash table size */
            put32(blk, 0x138, 0xFFFFFFFFu);                 /* bitmap valid */
            put32(blk, 0x13C, 881);                         /* bitmap block */
            for (int i = 0; i < 3; i++) { put32(blk, dates[i], days); put32(blk, dates[i] + 4, mins); put32(blk, dates[i] + 8, ticks); }
            blk[0x1B0] = (uint8_t)vlen;
            memcpy(blk + 0x1B1, volume, vlen);
            put32(blk, 0x1FC, 1);                           /* ST_ROOT */
            block_sum(blk, 0x014);
        } else if (n == 881) {                              /* the bitmap: 1 = free */
            for (int b = 2; b < 1760; b++)
                if (b != 880 && b != 881) blk[4 + (b - 2) / 32 * 4 + 3 - ((b - 2) % 32) / 8] |= (uint8_t)(1u << ((b - 2) % 8));
            block_sum(blk, 0);
        }
        if (n % 22 == 21) ok = cmm2->file_write(fh, chunk, sizeof chunk) == (int32_t)sizeof chunk;
    }
    cmm2->file_close(fh);
    if (!ok) cmm2->file_remove(path);
    return ok;
}

/* ---- the disk menu (Peter, 10 Nov 2026: "Can we modify the app so that a function key gives a dropdown of disks. This
   would be much more user friendly. The commodore 64 emulator does this well", "And, of course a way to return to the
   menu after playing a game"): F11 at any time, and at the start when DF0 has no disk. The Amiga stops while it is
   up, and its sound with it. It lists a folder's disk images (.adf) and folders, the folder it was left in (at first
   DF0's, or the one the program started in), the disks in DF0 and DF1 marked, and a new blank disk to make there. A
   disk chosen is booted (the Amiga restarted with it in DF0, as at power-on), put in DF0 as a disk change (a game's
   next disk), or put in DF1 (a work disk beside Workbench). Drawn over the picture on the page shown, a line at a time
   (each pixel written once: no flicker), in pico-amiga's own menu font (its 6 x 10, out/uifont.h) at twice its size, in
   Workbench 1.3's colours. Its keys come from the console's queue: the USB keyboard's codes, with their repeat, and
   the serial console's bytes (a terminal's arrows as ESC [ A ...); the first gamepad's up, down and fire too ---- */
#define MENU_MAX 300
enum { E_FILE, E_DIR, E_NEW, E_UP };            /* (sorted the other way: .., a new disk, the folders, the disks) */
typedef struct { char name[100]; uint8_t kind; } ment_t;
static ment_t ments[MENU_MAX];
static int ment_n, ment_disks, msel, mtop, menu_on, mdirty, mact, mact_sel, menu_hold_f11, mpad_was;
static char mdir[192];
#define MFW 12                                  /* a character's cell: the font's 6 x 10, twice */
#define MFH 20
#define M_BLUE   0x05A                          /* (Workbench 1.3's colours) */
#define M_WHITE  0xFFF
#define M_BLACK  0x002
#define M_ORANGE 0xF80
#define M_LIGHT  0x9BE
static int mcols, mlines, mrows, mbx, mby, mbw, mbh;
static uint16_t mline[AMIGA_OUT_W];
static char mtx[32][64], mrx[32][8];
static uint16_t mfg[32], mbg[32];

static int is_root(const char *d) { return d[0] && d[1] == ':' && (!d[2] || (d[2] == '/' && !d[3])); }
static int is_adf(const char *n) { const size_t l = strlen(n); return l > 4 && !strcasecmp(n + l - 4, ".adf"); }
static int ment_cmp(const void *a, const void *b) {
    const ment_t *x = a, *y = b;
    if (x->kind != y->kind) return (int)y->kind - (int)x->kind;
    return strcasecmp(x->name, y->name);
}
static void menu_scroll(void) {
    if (msel >= ment_n) msel = ment_n - 1;
    if (msel < 0) msel = 0;
    if (msel < mtop) mtop = msel;
    if (msel >= mtop + mrows) mtop = msel - mrows + 1;
    if (mtop > ment_n - mrows) mtop = ment_n - mrows;
    if (mtop < 0) mtop = 0;
}
static void menu_list(const char *select) {     /* mdir's folders and disk images, select (a name in it) chosen */
    ment_n = ment_disks = 0;
    if (!is_root(mdir)) { strcpy(ments[ment_n].name, ".."); ments[ment_n++].kind = E_UP; }
    ments[ment_n].name[0] = 0;
    ments[ment_n++].kind = E_NEW;
    const int h = cmm2->dir_open(mdir);
    if (h < 0) snprintf(mmsg, sizeof mmsg, "Cannot read that folder");
    else {
        static cmm2_dirent e;
        while (ment_n < MENU_MAX && cmm2->dir_read(h, &e) > 0) {
            if (e.name[0] == '.' || (!e.is_dir && !is_adf(e.name)) || strlen(e.name) >= sizeof ments->name) continue;   /* (._ files a Mac leaves) */
            strcpy(ments[ment_n].name, e.name);
            ments[ment_n++].kind = e.is_dir ? E_DIR : E_FILE;
            ment_disks++;
        }
        cmm2->dir_close(h);
    }
    qsort(ments, (size_t)ment_n, sizeof *ments, ment_cmp);
    msel = 0;
    for (int i = 0; i < ment_n; i++) if (ments[i].kind <= E_DIR) { msel = i; break; }   /* (the first folder or disk) */
    if (select) for (int i = 0; i < ment_n; i++) if (ments[i].kind <= E_DIR && !strcasecmp(ments[i].name, select)) { msel = i; break; }
    mtop = 0;
    mact = 0;
    menu_scroll();
}
static void menu_size(void) {                   /* the box: as much of the page as fits, centred */
    mcols = (PW - 64) / MFW; if (mcols > 56) mcols = 56;
    mlines = (PH - 40) / MFH; if (mlines > 26) mlines = 26;
    mrows = mlines - 7;
    mbw = mcols * MFW + 16; mbh = mlines * MFH + 12;
    mbx = (PW - mbw) / 2; mby = (PH - mbh) / 2;
}
static void mset(int i, uint16_t bg, uint16_t fg, const char *text, const char *right) {
    mbg[i] = bg; mfg[i] = fg;
    snprintf(mtx[i], sizeof mtx[i], "%s", text ? text : "");
    snprintf(mrx[i], sizeof mrx[i], "%s", right ? right : "");
}
static void mglyphs(int gy, const char *s, int col, int end, uint16_t fg) {   /* s's glyph row gy (0-9) into mline */
    for (; *s && col < end; s++, col++) {
        unsigned ch = (uint8_t)*s;
        if (ch < 0x20 || ch > 0x87) ch = '?';
        const unsigned bits = ui_font6x10[(ch - 0x20) * 10 + gy];
        if (!bits) continue;
        uint16_t *q = mline + 8 + col * MFW;
        for (int b = 0; b < 6; b++) if (bits & (0x20u >> b)) q[2 * b] = q[2 * b + 1] = fg;
    }
}
static void menu_draw(void) {
    char t[128];
    /* the lines: the title, DF0, DF1, a gap, the list (or what to do with the disk chosen), a gap, two of help */
    const char *d = mdir;
    const int dmax = mcols - 18;
    if ((int)strlen(d) > dmax) d += strlen(d) - dmax;
    snprintf(t, sizeof t, " Amiga disks in %s%s", d != mdir ? "..." : "", d);
    mset(0, M_WHITE, M_BLUE, t, NULL);
    for (int n = 0; n < 2; n++) {
        const char *p = drv[n].path, *b = strrchr(p, '/');
        snprintf(t, sizeof t, " DF%d: %s%s", n, p[0] ? (b ? b + 1 : p) : "empty", p[0] && drv[n].fh < 0 ? " (write-protected)" : "");
        mset(1 + n, M_BLUE, M_LIGHT, t, NULL);
    }
    mset(3, M_BLUE, M_WHITE, NULL, NULL);
    static const char *const act[4] = { "Boot it: the Amiga restarted with it in DF0", "Put it in DF0 (a disk change)",
                                        "Put it in DF1, the second drive", "Cancel" };
    for (int k = 0; k < mrows; k++) {
        const int i = 4 + k;
        if (mact) {
            if (k == 1) { snprintf(t, sizeof t, "  %s", ments[msel].name); mset(i, M_BLUE, M_WHITE, t, NULL); }
            else if (k >= 3 && k < 7) {
                const int on = k - 3 == mact_sel;
                snprintf(t, sizeof t, "  %s", act[k - 3]);
                mset(i, on ? M_ORANGE : M_BLUE, on ? M_BLACK : M_WHITE, t, NULL);
            } else mset(i, M_BLUE, M_WHITE, NULL, NULL);
            continue;
        }
        const int j = mtop + k;
        if (j >= ment_n) {
            mset(i, M_BLUE, M_LIGHT, j == ment_n && !ment_disks ? "  No disk images (.adf) or folders here" : NULL, NULL);
            continue;
        }
        const ment_t *e = &ments[j];
        char full[300];
        path_join(full, sizeof full, mdir, e->name);
        const char *tag = e->kind != E_FILE ? NULL : !strcasecmp(full, drv[0].path) ? "DF0" : !strcasecmp(full, drv[1].path) ? "DF1" : NULL;
        if (e->kind == E_UP) snprintf(t, sizeof t, "  .. (the folder above)");
        else if (e->kind == E_NEW) snprintf(t, sizeof t, "  [ A new blank disk, made here ]");
        else snprintf(t, sizeof t, "  %s%s", e->name, e->kind == E_DIR ? "/" : "");
        const int on = j == msel;
        mset(i, on ? M_ORANGE : M_BLUE, on ? M_BLACK : e->kind == E_FILE ? M_WHITE : M_LIGHT, t, tag);
    }
    mset(mlines - 3, M_BLUE, M_WHITE, NULL, NULL);
    if (mmsg[0]) { snprintf(t, sizeof t, " %s", mmsg); mset(mlines - 2, M_BLUE, M_ORANGE, t, NULL); }
    else mset(mlines - 2, M_BLUE, M_LIGHT, mact ? " Enter: do it   Esc: back to the list" : " Enter: choose   Backspace: up a folder", NULL);
    mset(mlines - 1, M_BLUE, M_LIGHT, mact ? " F11: back to the Amiga   F12: quit" : " F11 or Esc: back to the Amiga   F12: quit", NULL);
    /* the pixels, a line at a time: a black edge 2 wide, 4 of blue inside it, the lines of text */
    uint16_t *const pg = page[shown];
    for (int y = 0; y < mbh; y++) {
        const int edge = y < 2 || y >= mbh - 2;
        for (int x = 0; x < mbw; x++) mline[x] = edge || x < 2 || x >= mbw - 2 ? M_BLACK : M_BLUE;
        if (!edge && y >= 6 && y < 6 + mlines * MFH) {
            const int i = (y - 6) / MFH, gy = (y - 6) % MFH / 2;
            if (mbg[i] != M_BLUE) {             /* (the title's bar, the chosen line's) */
                const int x0 = i == 0 ? 2 : 6, x1 = i == 0 ? mbw - 2 : mbw - 6;
                for (int x = x0; x < x1; x++) mline[x] = mbg[i];
            }
            const int rl = (int)strlen(mrx[i]);
            mglyphs(gy, mtx[i], 0, rl ? mcols - rl - 2 : mcols, mfg[i]);
            if (rl) mglyphs(gy, mrx[i], mcols - rl - 1, mcols, mfg[i] == M_BLACK ? M_BLACK : M_ORANGE);
        }
        memcpy(pg + (mby + y) * PW + mbx, mline, (size_t)mbw * 2);
    }
    for (uintptr_t a = (uintptr_t)(pg + mby * PW) & ~(uintptr_t)31; a < (uintptr_t)(pg + (mby + mbh) * PW); a += 32) DCCMVAC = (uint32_t)a;
    __asm volatile ("dsb 0xF" ::: "memory");   /* (the cache written back: the LTDC reads the memory) */
}
static void menu_open(void) {
    if (menu_on || opt_crc) return;
    while (flip_to >= 0 && !quit) { }           /* (the frame made last shown: the menu goes over it) */
    while (cmm2->dma2d_busy()) { }
    for (int c = 0; c < 0x68; c++) if (held[c]) { amiga_key((uint8_t)c, false); held[c] = 0; }   /* (the Amiga's keys let go) */
    while (cmm2->con_getc() >= 0) { }           /* (F11's own code in the console's queue) */
    menu_hold_f11 = 1;                          /* (F11's repeat is not F11 again) */
    mpad_was = 7;                               /* (a button held already is not pressed) */
    menu_size();
    char keep[100] = "", dd[192];
    const int first = !mdir[0];
    if (first) snprintf(mdir, sizeof mdir, "%s", cwd0);
    else if (ment_n && msel < ment_n && ments[msel].kind <= E_DIR) snprintf(keep, sizeof keep, "%s", ments[msel].name);
    if (drv[0].path[0]) {                       /* (DF0's disk chosen when it is in the folder: at first, its folder) */
        const char *nm = path_dir(dd, sizeof dd, drv[0].path);
        if (first) snprintf(mdir, sizeof mdir, "%s", dd);
        if (!strcasecmp(dd, mdir)) snprintf(keep, sizeof keep, "%s", nm);
    }
    menu_list(keep[0] ? keep : NULL);
    menu_on = mdirty = 1;
    printf("amiga: the disk menu\n");
}
static void menu_close(void) {
    menu_on = 0;
    mmsg[0] = 0;
    mute_all = 1;                               /* (the keys held now are not the Amiga's till let go) */
    f11_was = 1;
    while (cmm2->con_getc() >= 0) { }
    mouse_bx = mouse_by = 0;                    /* (the mouse's moves in the menu are not the Amiga's) */
}
static void menu_up(void) {
    if (is_root(mdir)) return;
    char name[100], dd[192];
    snprintf(name, sizeof name, "%s", path_dir(dd, sizeof dd, mdir));
    snprintf(mdir, sizeof mdir, "%s", dd);
    menu_list(name);
}
static void menu_new(void) {                    /* a blank disk, workN.adf named WorkN, in the folder; then where to put it */
    char name[32], vol[16], full[300];
    cmm2_stat st;
    int n;
    for (n = 1; n < 100; n++) {
        snprintf(name, sizeof name, "work%d.adf", n);
        path_join(full, sizeof full, mdir, name);
        if (cmm2->file_stat(full, &st) < 0) break;
    }
    if (n == 100) { snprintf(mmsg, sizeof mmsg, "work1.adf to work99.adf are all here"); return; }
    snprintf(vol, sizeof vol, "Work%d", n);
    if (!format_adf(full, vol)) { snprintf(mmsg, sizeof mmsg, "Could not make %s here", name); return; }
    printf("amiga: made %s, a blank disk named %s\n", full, vol);
    menu_list(name);
    mact = 1;
    mact_sel = drv[0].path[0] ? 2 : 1;          /* (DF1 beside the disk in DF0, else DF0) */
    snprintf(mmsg, sizeof mmsg, "Made %s, a blank disk named %s", name, vol);
}
static void menu_use(int act) {                 /* 0: boot it; 1: in DF0; 2: in DF1 */
    char p[300];
    path_join(p, sizeof p, mdir, ments[msel].name);
    mact = 0;
    if (drive_in(act == 2, p, act == 0)) menu_close();
}
static void menu_do(int k) {                    /* a key: the console's codes (0x80-0x83 the arrows, 0x86-0x89 Home End PgUp PgDn) */
    if (!k) return;
    if (k == 0x9B) { if (!menu_hold_f11) menu_close(); return; }   /* F11 */
    if (k == 0x9C || k == 0x1D) { quit = 1; return; }   /* F12, the serial console's Ctrl-] */
    mdirty = 1;
    mmsg[0] = 0;
    const int enter = k == '\r' || k == '\n';
    if (mact) {
        if (k == 0x80 && mact_sel > 0) mact_sel--;
        else if (k == 0x81 && mact_sel < 3) mact_sel++;
        else if (k == 27 || k == 8 || k == 0x7F || k == 0x82) mact = 0;
        else if (enter || k == 0x83) { if (mact_sel == 3) mact = 0; else menu_use(mact_sel); }
        return;
    }
    if (k == 0x80) msel--;
    else if (k == 0x81) msel++;
    else if (k == 0x88) msel -= mrows - 1;
    else if (k == 0x89) msel += mrows - 1;
    else if (k == 0x86) msel = 0;
    else if (k == 0x87) msel = ment_n - 1;
    else if (k == 27) { menu_close(); return; }
    else if (k == 8 || k == 0x7F || k == 0x82) menu_up();
    else if ((enter || k == 0x83) && ment_n) {
        const ment_t *e = &ments[msel];
        if (e->kind == E_UP) menu_up();
        else if (e->kind == E_NEW) menu_new();
        else if (e->kind == E_DIR) {
            char sub[192];
            path_join(sub, sizeof sub, mdir, e->name);
            snprintf(mdir, sizeof mdir, "%s", sub);
            menu_list(NULL);
        } else { mact = 1; mact_sel = 0; }
    } else if (k > ' ' && k < 0x7F) {          /* a letter or digit: the next name that starts with it */
        for (int i = 1; i <= ment_n; i++) {
            const int j = (msel + i) % ment_n;
            if (tolower((unsigned char)ments[j].name[0]) == tolower(k)) { msel = j; break; }
        }
    }
    menu_scroll();
}
static int menu_key(void) {                     /* the next key, a terminal's ESC [ sequences as the USB keyboard's codes */
    static int after_cr;
    const int k = cmm2->con_getc();
    /* (Enter, CR and LF together, is one key: OPTION CRLF has the USB keyboard's Enter give both (KeyboardMap.c), and
       the LF was taken as a second Enter, which opened the folder's first disk or booted the disk chosen; Peter, 10 Nov
       2026: "from the amiga folder I select demoes folder. It only shows the first one until I press escape") */
    if (k >= 0) { const int lf = k == '\n' && after_cr; after_cr = k == '\r'; if (lf) return 0; }
    if (k != 27) return k;
    const int k2 = cmm2->con_getc();
    if (k2 < 0) return 27;                      /* (Esc alone) */
    if (k2 != '[' && k2 != 'O') return 0;
    int k3 = cmm2->con_getc();
    switch (k3) {
    case 'A': return 0x80; case 'B': return 0x81; case 'C': return 0x83; case 'D': return 0x82;
    case 'H': return 0x86; case 'F': return 0x87;
    }
    if (k3 < '0' || k3 > '9') return 0;
    int v = k3 - '0';
    while ((k3 = cmm2->con_getc()) >= '0' && k3 <= '9') v = v * 10 + k3 - '0';
    switch (v) {
    case 1: case 7: return 0x86; case 4: case 8: return 0x87; case 5: return 0x88; case 6: return 0x89;
    case 23: return 0x9B; case 24: return 0x9C;
    }
    return 0;
}
static void menu_pad(void) {                    /* the first gamepad: up and down (held: again each 100 ms after 400), fire */
    static int rep;
    int now = 0;
    for (int slot = 1; slot <= 8; slot++) {
        int lx = 128, ly = 128;
        const int b = cmm2->gamepad(slot, &lx, &ly);
        if (b < 0) continue;
        now = ((b >> 7 & 1) || ly < 64) | (((b >> 5 & 1) || ly > 192) << 1) | (((b >> 10 & 15) != 0) << 2);
        break;
    }
    rep = now == mpad_was && (now & 3) ? rep + 1 : 0;
    const int fresh = now & ~mpad_was, again = rep > 20 && rep % 5 == 0;
    mpad_was = now;
    if ((fresh & 1) || ((now & 1) && again)) menu_do(0x80);
    if ((fresh & 2) || ((now & 2) && again)) menu_do(0x81);
    if (fresh & 4) menu_do('\r');
}
static void menu_frame(void) {                  /* the main loop's, while the menu is up: a display frame's time */
    const uint64_t t_end = cmm2->micros() + 20000;
    cmm2->poll();
    cmm2->break_take();
    uint8_t r[8];
    int f11 = 0;
    if (cmm2->keys_raw(r) > 0)
        for (int i = 2; i < 8; i++) {
            if (r[i] == 0x44) f11 = 1;
            if (r[i] == 0x4D && (r[0] & 0x11) && (r[0] & 0x44)) quit = 1;   /* (Ctrl+Alt+End) */
        }
    if (!f11) menu_hold_f11 = 0;
    for (int k; menu_on && !quit && (k = menu_key()) >= 0; ) menu_do(k);
    if (menu_on) menu_pad();
    mouse_task();                               /* (the pointer kept in the middle) */
    if (menu_on && mdirty) { menu_draw(); mdirty = 0; }
    while (cmm2->micros() < t_end && !quit) { }
}

/* ---- the Kickstart ---- */
static uint8_t *load_rom(const char *path, uint32_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) { printf("amiga: no %s here (a Kickstart of your own: see the App User Guide's Amiga chapter)\n", path); return NULL; }
    uint8_t *p = malloc(0x80000);
    const size_t n = p ? fread(p, 1, 0x80000, f) : 0;
    fclose(f);
    if (n != 0x40000 && n != 0x80000) { printf("amiga: %s is %u bytes (256 or 512 KB)\n", path, (unsigned)n); return NULL; }
    *size = (uint32_t)n;
    return p;
}

/* the profile's clock: the M7's cycle counter */
#define DEMCR      (*(volatile uint32_t *)0xE000EDFCu)
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000u)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004u)
static uint32_t cycles(void) { return DWT_CYCCNT; }

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-kick") && i + 1 < argc) kick = argv[++i];
        else if (!strcmp(argv[i], "-ext") && i + 1 < argc) ext = argv[++i];
        else if (!strcmp(argv[i], "-click") && i + 1 < argc) opt_click = atol(argv[++i]);
        else if (!strcmp(argv[i], "-fast")) opt_fast = 1;
        else if (!strcmp(argv[i], "-stats")) opt_stats = 1;
        else if (!strcmp(argv[i], "-noprof")) opt_noprof = 1;
        else if (!strcmp(argv[i], "-mute")) opt_mute = 1;
        else if (!strcmp(argv[i], "-crc") && i + 1 < argc) opt_crc = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-keys")) opt_keys = 1;
        else if (!strcmp(argv[i], "-nowa")) opt_wa = 0;
        else if (!strcmp(argv[i], "-dt") && i + 1 < argc) opt_dt = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-qos") && i + 1 < argc) opt_qos = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-sound")) opt_sound = 1;
        else if (!strcmp(argv[i], "-nosound")) opt_sound = 0;
        else if (!strcmp(argv[i], "-wp")) opt_wp = 1;
        else if (!strcmp(argv[i], "-df1") && i + 1 < argc) opt_df1 = argv[++i];
        else if (!strcmp(argv[i], "-pal")) opt_pal = opt_576 = 1;
        else if (!strcmp(argv[i], "-pal60")) opt_pal = 1, opt_576 = 0;
        else if (!strcmp(argv[i], "-ntsc")) opt_pal = opt_576 = 0;
        else if (argv[i][0] != '-' && ndisks < MAXDISKS) disk[ndisks++] = argv[i];
    }
    if (opt_576) { PW = AMIGA_OUT_W; PH = 2 * AMIGA_FB_H; ROWS = AMIGA_FB_H; SX = 0; SCR_W = 720; SCR_H = 576; VMODE = 1; core_rate = 31300; }
    else if (opt_pal) { PH = 480; ROWS = 240; SY = 26; core_rate = (int)(2LL * 313 * 25212000 / (800 * 525)); }   /* (37,578 a second: the pitch 20% up, as the tempo) */
    printf("amiga: pico-amiga on the Colour Maximite 2 (%s)\n", opt_576 ? "PAL" : opt_pal ? "PAL at 60 Hz" : "NTSC");
    amiga_config_t cfg;
    memset(&cfg, 0, sizeof cfg);
    if (!(cfg.rom = load_rom(kick, &cfg.rom_size))) return 1;
    if (ext && !(cfg.ext_rom = load_rom(ext, &cfg.ext_size))) return 1;
    if (!getcwd(cwd0, sizeof cwd0)) strcpy(cwd0, "A:/");
    if (!(drv[0].img = cmm2->alloc(DISK_BYTES))) { printf("amiga: no room for a disk\n"); return 1; }

    cmm2_video v;
    if (cmm2->video_mode(VMODE, 12, &v) < 0) { printf("amiga: MODE %d in depth 12 refused\n", VMODE); return 1; }
    page[0] = (uint16_t *)v.fb;                 /* (MMBasic's page 0, the screen's size, holds the window's page) */
    /* The other two pages from MMBasic's heap, not this program's 8 MB: that is the SDRAM's fourth bank (8 MB each), where
       the Amiga's chip and slow RAM are, and the LTDC reading a page there while the 68000 works in them made each change
       of row in the bank wait (Lotus's turning car: the LTDC ran dry). This program's memory only if the heap has no room */
    for (int i = 1; i < NPAGES; i++) {
        void *b = cmm2->alloc(PW * PH * 2 + 32);
        if (!b && i == 1) b = malloc(PW * PH * 2 + 32);
        if (!b) { printf("amiga: two pages (no room for a third)\n"); break; }
        page[i] = (uint16_t *)(((uintptr_t)b + 31) & ~(uintptr_t)31);
        memset(page[i], 0, PW * PH * 2);
        npages = i + 1;
    }
    if (npages < 2) { printf("amiga: no memory for the second page\n"); return 1; }
    if (opt_stats) printf("amiga: pages at %p %p %p\n", (void *)page[0], (void *)page[1], (void *)page[2]);
    if (cmm2_axi_load()) { printf("amiga: the AXI RAM is not free\n"); return 1; }
    batch[0] = (uint16_t *)(((uintptr_t)__axi_end + 31) & ~(uintptr_t)31);
    batch[1] = batch[0] + CH * AMIGA_OUT_W;
    ring = (amiga_row_t *)(batch[1] + CH * AMIGA_OUT_W); /* (32-aligned: 2 x 22 x 1440 bytes after the 32-aligned batches) */
    if ((uintptr_t)(ring + AMIGA_ROW_RING) > 0x24080000u) { cmm2_axi_unload(); printf("amiga: no room in the AXI RAM for the rows\n"); return 1; }
    if (opt_576 && !timing_576p()) { cmm2_axi_unload(); printf("amiga: PLL3 did not lock for 576p\n"); return 1; }
    window_on();
    if (opt_wa) wa_on();
    qos_saved = AXI_INI6_READ_QOS;
    if (opt_qos >= 0) AXI_INI6_READ_QOS = (uint32_t)(opt_qos & 15);
    amtcr_saved = DMA2D_AMTCR;
    DMA2D_AMTCR = opt_dt > 0 ? (uint32_t)(opt_dt > 255 ? 255 : opt_dt) << 8 | 1u : 0u;

    DEMCR |= 1u << 24;                          /* (the cycle counter on, for the profile) */
    DWT_CTRL |= 1u;
    cfg.chip_ram = chip_ram; cfg.chip_size = sizeof chip_ram;
    cfg.slow_ram = slow_ram; cfg.slow_size = sizeof slow_ram;
    cfg.chipset = AMIGA_OCS;
    cfg.ntsc = !opt_pal;
    memset(ring, 0, sizeof *ring * AMIGA_ROW_RING);
    cfg.framebuffer = ring;
    cfg.audio_ring = aring; cfg.audio_frames = AFRAMES;
    cfg.track = track;
    cfg.row_done = row_done;
    if (opt_stats && !opt_noprof) cfg.prof_clock = cycles;
    amiga_init(&cfg);
    amiga_set_drives(2);                        /* DF0 and DF1: the A500's own drive and an external one */
    cmm2->con_screen(0);                        /* (the console's text to the serial port only: the screen is the Amiga's) */
    if (ndisks) drive_in(0, disk[0], 0);        /* (one that will not load: the menu comes up, and says why) */
    if (opt_df1) drive_in(1, opt_df1, 0);
    amiga_reset(true);

    if (opt_sound) {                            /* (on unless -nosound: off on 9 Nov 2026 while it was "dire", Peter; its
                                                   reader fixed and the stream by DMA since, 10 Nov) */
        audio_rate = cmm2->audio_start(fill);
        if (audio_rate < 8000) printf("amiga: no sound (%d)\n", audio_rate);
    }
    cmm2->frame_hook(hook);
    const uint32_t hz = cmm2->cpu_hz();
    poll_cyc = hz / 500;                        /* (the mouse and the USB each 2 ms: mid_frame_poll) */
    uint64_t t_next = cmm2->micros(), t_last = t_next, frame_us = 0;
    int nstats = 0;
    if (!drv[0].path[0]) menu_open();           /* (no disk in DF0: the menu first) */
    while (!quit) {
        if (menu_on) { menu_frame(); t_last = cmm2->micros(); continue; }
        amiga_run_frame();
        frame++;
        present();
        sync_disks();                           /* (what the Amiga wrote this frame, on the card) */
        cmm2->poll();                           /* the USB devices seen to (the keyboard, the mouse, the pads) */
        cmm2->break_take();                     /* (Ctrl-C is the Amiga's Ctrl and C, not the end of the program) */
        keyboard();
        if (opt_click >= 0) click_down = (long)frame >= opt_click && (long)frame < opt_click + 10;
        ports();
        uint64_t now = cmm2->micros();
        if (opt_stats) {
            frame_us += now - t_last;
            if (++nstats == 50) {
                amiga_profile_t pr;
                amiga_get_profile(&pr, true);
                const double k = 1000.0 / hz / (pr.frames ? pr.frames : 1);   /* (ticks a frame to ms) */
                const double budget_ms = opt_576 ? 20.0 : 800.0 * 525 / 25212.0;   /* (a display frame: 576p's, MODE 8's) */
                printf("amiga: frame %lu, %.2f ms a frame (%.1f%% of %.2f): cpu %.2f copper %.2f render %.2f (fetch %.2f) blit %.2f"
                       " paula %.2f line %.2f disk %.2f other %.2f; waiting %.2f, rows out %.2f (queue %.2f, copy calls %.2f, read %.2f); flips %u, %u late (line %u)\n", frame, frame_us / 50000.0,
                       frame_us / 500.0 / budget_ms, budget_ms, pr.ticks[AMIGA_PROF_CPU] * k, pr.ticks[AMIGA_PROF_COPPER] * k, pr.ticks[AMIGA_PROF_RENDER] * k,
                       pr.ticks[AMIGA_PROF_BPL] * k, pr.ticks[AMIGA_PROF_BLIT] * k, pr.ticks[AMIGA_PROF_PAULA] * k,
                       pr.ticks[AMIGA_PROF_LINE] * k, pr.ticks[AMIGA_PROF_DISK] * k, pr.ticks[AMIGA_PROF_OTHER] * k,
                       us_wait * k, us_out * k, us_queue * k, us_copy * k, us_read * k, (unsigned)flips, (unsigned)late_flips, (unsigned)late_max);
                nstats = 0; frame_us = 0; us_wait = us_out = us_queue = us_copy = us_read = 0; flips = late_flips = late_max = 0;
            }
        }
        t_last = cmm2->micros();
    }
    cmm2->frame_hook(NULL);
    while (cmm2->dma2d_busy()) { }
    drives_close();
    window_off();
    timing_back();
    wa_off();
    DMA2D_AMTCR = amtcr_saved;
    AXI_INI6_READ_QOS = qos_saved;
    cmm2_axi_unload();
    printf("amiga: %lu frames\n", frame);
    return 0;
}
