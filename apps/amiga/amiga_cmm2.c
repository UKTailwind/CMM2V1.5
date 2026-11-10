/* amiga_cmm2.c: an Amiga 500 on the Colour Maximite 2, frank-amiga's emulator (rh1tech's port of Matt Parsons's Omega:
   Musashi's 68000 and the OCS chipset, Agnus's DMA slot by slot) on one core, by the loader's services. Peter, 9 Nov
   2026: "let's get it running and then we can start really tuning". This is the first stage: frank-amiga's native
   (desktop) core as frank's native runner builds it, with the RP2350 board's halves not used; this file is its host
   (what native/main_native.c, host_native.c and Presentation.c are on a PC).

     RUN "amiga.app" [, disk.adf [disk2.adf ...]] [-kick file] [-ext file] [-click frame] [-fast] [-stats]

   - The machine: an A500 with 512 KB of chip RAM and 512 KB of slow RAM, PAL, no Fast RAM, its Kickstart from the
     folder (kick.rom, or -kick; 256 or 512 KB, a user's own: none is built in). The disks are DF0's, the first in it at
     power-on; F11 puts the next one in (the last back to the first).
   - The display: MODE 1 (800 x 600 at 60 Hz) in depth 12, ARGB4444 on one layer, the layer cut to a 640 x 512 window
     in the middle (below): the core draws 0x0RGB pixels, the Amiga's own colours, into a raster of 640 x 512 (a beam
     row each, low resolution pixels two wide), and at each frame's end the DMA2D copies its picture into the page not
     being shown, each Amiga line on two of the monitor's, while the next frame is made; the frame hook shows that page
     at the next blanking. Two rasters, so that one is copied while the
     core draws the other. The placing of the picture is Presentation.c's (frank-amiga's native host), but its
     reconstruction of rows wrapped at the bottom, which is not done.
   - The sound: Paula's lines mixed at the stream's 44.1 kHz (AUDIO_RATE), stereo, through a ring to the stream.
   - The keyboard: by place, the HID codes of the keys held (keys_raw) to the Amiga's raw codes, one change a frame as the
     keyboard's handshake allows; Insert is HELP, the left and right Windows keys the Amiga keys. F11 the next disk, F12
     (or Ctrl+Alt+End) back to MMBasic. The serial console types too: letters, digits, space, Enter; Ctrl-N the next
     disk, Ctrl-] back to MMBasic.
   - The mouse in port 1, the first USB gamepad a joystick in port 2 (any face button fire).
   - -click N: the left button pressed at frame N, for a Kickstart patched with Guardian, whose boot-block question
     waits for it. -fast: no pacing to 50 frames a second. -stats: the time a frame takes, each 50, on the console. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "cmm2.h"
#undef printf
#undef puts
#include "Host.h"
#include "../omega/CPU.h"
#include "../omega/Chipset.h"
#include "../omega/CIA.h"
#include "../omega/DMA.h"
#include "../omega/Floppy.h"
#include "../omega/Audio.h"
#include "../omega/Expansion.h"
#include "../omega/Picasso2.h"
#include "../omega/VideoStandard.h"
#include "../omega/DisplayLayout.h"
#include "../omega/m68k.h"

/* ---- memory (amiga_mem.h) ---- */
unsigned char amiga_chip[0x200000] __attribute__((aligned(32)));
unsigned char amiga_slow[0x80000] __attribute__((aligned(32)));
unsigned char amiga_ext[0x80000] __attribute__((aligned(32)));
unsigned char amiga_rom[0x80000] __attribute__((aligned(32)));
unsigned char amiga_none[64] __attribute__((aligned(32)));
uint8_t *cmm2_mfm_image;                        /* Floppy.c's DF0 as MFM, 2.1 MB: MMBasic's heap (read while a track loads) */

/* what native/main_native.c and host_native.c define for the rest */
int screenWidth = 640, screenHeight = 200, disass = 0;
unsigned long native_frame_counter;
void frankRegLog(const char *name, uint16_t value) { (void)name; (void)value; }
ssize_t pread(int fd, void *b, size_t n, off_t o) { return lseek(fd, o, SEEK_SET) < 0 ? -1 : read(fd, b, n); }   /* (hdf_native.c's IDE disk) */
ssize_t pwrite(int fd, const void *b, size_t n, off_t o) { return lseek(fd, o, SEEK_SET) < 0 ? -1 : write(fd, b, n); }
extern uint32_t nativeChipMask, nativeSlowSize;
extern int extRomPresent, p2DebugBlits;
extern int dmaRunSlice(int *balance);
void memoryNativeInit(void);

/* ---- the processor's AXI RAM for the 68000's opcode table (-DAXI_TABLE, patches/0001) ----
   MPU region 12 (the firmware's main.c) makes the 512 KB at 0x24000000 shareable, which the M7's data cache does not cache;
   with page 0 in SDRAM nothing uses it, so for the run it is ordinary write-back memory, and the firmware's setting comes
   back, the cache cleaned, before MMBasic has it again (the benchmark: 3.5% faster than in SDRAM) */
#define MPU_RNR  (*(volatile uint32_t *)0xE000ED98u)
#define MPU_RBAR (*(volatile uint32_t *)0xE000ED9Cu)
#define MPU_RASR (*(volatile uint32_t *)0xE000EDA0u)
#define DCCIMVAC (*(volatile uint32_t *)0xE000EF70u)
#define DCCMVAC  (*(volatile uint32_t *)0xE000EF68u)
static uint32_t axi_rasr;
static int axi_on(void) {
    MPU_RNR = 12;
    if ((MPU_RBAR & ~0x1Fu) != 0x24000000u) return 0;
    axi_rasr = MPU_RASR;
    uint32_t r = (axi_rasr & ~((7u << 19) | (1u << 18) | (1u << 17) | (1u << 16))) | (1u << 19) | (1u << 17) | (1u << 16);
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RASR = r;
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
    return 1;
}
/* The program's 8 MB (the loader's MPU region at 0xD1800000) is write-back without write-allocate (TEX 0, C, B): a store
   that misses the cache goes to the SDRAM by itself, 38 MB/s with the display and the DMA2D (the raster's clear took 17 ms a
   frame). With write-allocate (TEX 1) the line comes into the cache and is written back whole. For the run, put back at the
   end (a test of what it is worth: 9 Nov 2026) */
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
}
static void axi_off(void) {
    for (uint32_t a = 0x24000000u; a < 0x24080000u; a += 32) DCCIMVAC = a;
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RNR = 12;
    MPU_RASR = axi_rasr;
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
}

/* ---- options ---- */
#define MAXDISKS 8
static const char *disk[MAXDISKS];
static int ndisks, curdisk, opt_fast, opt_stats, getenv_flag_wa;
static int opt_noflip, opt_keys, opt_nocopy;    /* (-noflip: every frame copied into page 0, the LTDC's address never changed;
                                                   -keys: the keyboard's reports and the mouse's buttons on the console;
                                                   -nocopy: a test card drawn in page 0 once, nothing copied, the emulation
                                                   running: whether the picture moves with the emulation alone) */
static long opt_click = -1, opt_freeze = -1;    /* (-freeze N: the emulation stopped after frame N, its last picture
                                                   copied and shown 50 times a second: whether it moves with the copies alone) */
static const char *kick = "kick.rom", *ext;
static volatile int quit;

/* ---- the display ---- */
#define RW HOST_RASTER_W                        /* 640 */
#define RH HOST_RASTER_H                        /* 512 (PAL) */
/* MODE 1 (800 x 600 at 60 Hz, which every CMM2 monitor takes), its layer cut to a window of 640 x 512 in the middle: a
   page is that window, one line of 1280 bytes for each of the monitor's, an Amiga line on two of them. The LTDC reads only
   the window (39 MB/s from the SDRAM); outside it the LTDC's background colour, the Amiga's border colour, costs nothing.
   MODE 19 (1280 x 1024 at half the clock, each line read twice: 79 MB/s) was given up: with the emulator's own SDRAM
   traffic the LTDC ran dry a hundred times in 45 s and the monitors lost their sync (Peter, 9 Nov 2026) */
#define PW 640
#define PH 512
#define LTDC_BPCR    (*(volatile uint32_t *)0x5000100Cu)
#define LTDC_SRCR    (*(volatile uint32_t *)0x50001024u)
#define LTDC_BCCR    (*(volatile uint32_t *)0x5000102Cu)
#define LTDC_L1WHPCR (*(volatile uint32_t *)0x50001088u)
#define LTDC_L1WVPCR (*(volatile uint32_t *)0x5000108Cu)
#define LTDC_L1CFBAR (*(volatile uint32_t *)0x500010ACu)
#define LTDC_L1CFBLR (*(volatile uint32_t *)0x500010B0u)
#define LTDC_L1CFBLNR (*(volatile uint32_t *)0x500010B4u)
static uint32_t ltdc_saved[6];                  /* the layer as MMBasic's mode set it, put back at the end */
static volatile uint32_t border_rgb;            /* the Amiga's border colour as the LTDC's background (RGB888) */
static host_pixel_t *raster[2];                 /* the core draws into raster[cur] */
static int cur;
/* Three pages: page[0] MMBasic's page 0, page[1] in this program's memory, page[2] a block of MMBasic's heap (if there is
   room; else two). A frame's copies go to a page neither shown nor waiting to be, so the emulation never waits for a
   flip, which comes only at one of the monitor's 60 blankings a second (with two pages it waited up to 9 ms a frame);
   a finished page not yet shown when the next is finished is dropped */
#define NPAGES 3
static uint16_t *page[NPAGES];
static int npages = 2;
static volatile int shown;                      /* the page on the screen */
static volatile int flip_to = -1;               /* the page the frame hook shows next, once the copies are done */
static uint16_t page_border[NPAGES] = {0xFFFF, 0xFFFF, 0xFFFF};
static int page_geom[NPAGES] = {-1, -1, -1};
/* a raster is cleared to the border colour only when the colour or the screen's layout changed since its last clear
   (a steady screen draws the same pixels each frame), and once a second besides (anything left over goes): the clear
   took 4.7 ms a frame */
static struct { uint16_t bc, ddfstrt, ddfstop, diwstrt, diwstop, bplcon0; unsigned long at; } cleared[2];
Host_t host;

static volatile int npend, ipend;               /* (the copies waiting to go to the DMA2D: below) */
static void hook(void) {                        /* the start of each frame's blanking (PendSV): a finished page shown */
    LTDC_BCCR = border_rgb;
    if (flip_to >= 0 && ipend == npend && !cmm2->dma2d_busy()) {
        LTDC_L1CFBAR = (uint32_t)page[flip_to];
        LTDC_SRCR = 1;                          /* (reload now: in the blanking) */
        shown = flip_to;
        flip_to = -1;
    }
}

/* the layer cut to the window, page 0 in it; and put back as it was */
static void window_on(void) {
    ltdc_saved[0] = LTDC_L1WHPCR; ltdc_saved[1] = LTDC_L1WVPCR; ltdc_saved[2] = LTDC_L1CFBAR;
    ltdc_saved[3] = LTDC_L1CFBLR; ltdc_saved[4] = LTDC_L1CFBLNR; ltdc_saved[5] = LTDC_BCCR;
    const uint32_t ahbp = LTDC_BPCR >> 16 & 0xFFF, avbp = LTDC_BPCR & 0x7FF;
    const uint32_t x0 = ahbp + 1 + (800 - PW) / 2, y0 = avbp + 1 + (600 - PH) / 2;
    LTDC_L1WHPCR = (x0 + PW - 1) << 16 | x0;
    LTDC_L1WVPCR = (y0 + PH - 1) << 16 | y0;
    LTDC_L1CFBLR = (uint32_t)(PW * 2) << 16 | (PW * 2 + 7);
    LTDC_L1CFBLNR = PH;
    LTDC_L1CFBAR = (uint32_t)page[0];
    LTDC_BCCR = 0;
    LTDC_SRCR = 1;
}
static void window_off(void) {
    LTDC_L1WHPCR = ltdc_saved[0]; LTDC_L1WVPCR = ltdc_saved[1]; LTDC_L1CFBAR = ltdc_saved[2];
    LTDC_L1CFBLR = ltdc_saved[3]; LTDC_L1CFBLNR = ltdc_saved[4]; LTDC_BCCR = ltdc_saved[5];
    LTDC_SRCR = 1;
}

static void fill16(uint16_t *p, uint16_t c, size_t n) {
    uint32_t c2 = (uint32_t)c << 16 | c;
    if (((uintptr_t)p & 2) && n) { *p++ = c; n--; }
    uint32_t *q = (uint32_t *)p;
    for (size_t i = 0; i < n / 2; i++) q[i] = c2;
    if (n & 1) p[n - 1] = c;
}

/* the border around the copied picture, in a page: rows [0, y0) and [y1, PH) and the columns [0, x0) and [x1, PW) */
static void border(int pg, uint16_t c, int x0, int x1, int y0, int y1) {
    uint16_t *p = page[pg];
    for (int y = 0; y < PH; y++) {
        uint16_t *row = p + y * PW;
        if (y < y0 || y >= y1) fill16(row, c, PW);
        else { if (x0 > 0) fill16(row, c, x0); if (x1 < PW) fill16(row + x1, c, PW - x1); }
    }
    if (pg)                                     /* (memory the cache may hold: written back for the LTDC) */
        for (uintptr_t a = (uintptr_t)p & ~31u; a < (uintptr_t)(p + PH * PW); a += 32) DCCMVAC = (uint32_t)a;
    __asm volatile ("dsb 0xF" ::: "memory");
}

/* raster rows [sy0, sy1) (each step rows apart) to the page's lines from y (two lines each, the monitor's), raster
   columns [sx, sx + w) to page columns from x */
/* The copies a frame needs (two, or four for a rotated full-width screen) wait here and go to the service's queue, which
   holds three (the one being done among them), as it has room: from present() and between the core's slices, so the core
   goes on meanwhile. Room is looked for with dma2d_busy before dma2d_copy is called: dma2d_copy writes the source back
   from the cache before it looks for room, so calling it again and again while the queue was full cleaned the raster over
   and over (9.6% of the time) */
static struct { const void *s; void *d; uint32_t sp, dp, w, h; } pend[8];
static void pump(void) {
    while (ipend < npend && cmm2->dma2d_busy() < 3) {
        if (cmm2->dma2d_copy(pend[ipend].s, pend[ipend].sp, pend[ipend].d, pend[ipend].dp, pend[ipend].w, pend[ipend].h) < 0) break;
        ipend++;
    }
}
static void copy_rows(int pg, int sy0, int nrows, int step, int y, int sx, int x, int w) {
    if (nrows <= 0 || w <= 0) return;
    for (int k = 0; k < 2 && npend < 8; k++) {  /* the two monitor lines of each raster row */
        pend[npend].s = raster[cur] + sy0 * RW + sx;
        pend[npend].d = page[pg] + (y + k) * PW + x;
        pend[npend].sp = (uint32_t)(step * RW * 2);
        pend[npend].dp = (uint32_t)(2 * PW * 2);
        pend[npend].w = (uint32_t)(w * 2);
        pend[npend].h = (uint32_t)nrows;
        npend++;
    }
}

/* the finished frame in raster[cur] to the page not shown (Presentation.c's hostPresentFrame, by the DMA2D) */
static uint64_t us_wait, us_clear;              /* (-stats: where a frame's time goes) */
static void present(void) {
    uint64_t t0 = cmm2->micros();
    /* the last frame's copies done (they had a frame), and with two pages its page shown too */
    while (ipend < npend || cmm2->dma2d_busy() || (npages == 2 && flip_to >= 0)) {
        pump();
        if (opt_fast) break;
    }
    us_wait += cmm2->micros() - t0;
    npend = ipend = 0;
    if (opt_nocopy) { cur ^= 1; host.pixels = raster[cur]; return; }
    int pg = 0;
    if (!opt_noflip)
        while (pg == shown || pg == flip_to) pg++;  /* (a page neither shown nor waiting to be) */
    const uint16_t bc = (uint16_t)internal.palette[0];
    border_rgb = (uint32_t)((bc >> 8 & 15) * 17) << 16 | (uint32_t)((bc >> 4 & 15) * 17) << 8 | (uint32_t)((bc & 15) * 17);
    const int diw_start = omegaDiwVerticalStart(chipset.diwstrt);
    const int viewport_y_offset = (OMEGA_VIDEO_STANDARD == OMEGA_VIDEO_PAL && diw_start < 64) ? HOST_CONTENT_Y : OMEGA_VIDEO_VIEWPORT_Y_OFFSET;
    /* the other raster, for the next frame, cleared to the border colour first: its copies were done a frame ago, and the
       DMA2D, idle now, does not compete for the SDRAM (with these copies running, the clear took 17 ms). Only the rows this
       frame shows (the next is laid out the same, or nearly: a change shows for a frame), about half of the 512 */
    t0 = cmm2->micros();
    {
        const int o = cur ^ 1;
        if (cleared[o].bc != bc || cleared[o].ddfstrt != chipset.ddfstrt || cleared[o].ddfstop != chipset.ddfstop ||
            cleared[o].diwstrt != chipset.diwstrt || cleared[o].diwstop != chipset.diwstop || cleared[o].bplcon0 != chipset.bplcon0 ||
            native_frame_counter - cleared[o].at >= 50) {
            int r0 = 0, r1 = RH;
            if (!omegaDdfIsFullWidth(chipset.ddfstrt)) {
                while (HOST_CONTENT_Y + 2 * r0 - viewport_y_offset < 0) r0++;
                r1 = r0;
                while (r1 < RH && HOST_CONTENT_Y + 2 * r1 - viewport_y_offset + 1 < PH) r1++;
            }
            fill16(raster[o] + r0 * RW, bc, (size_t)RW * (r1 - r0));
            cleared[o].bc = bc; cleared[o].ddfstrt = chipset.ddfstrt; cleared[o].ddfstop = chipset.ddfstop;
            cleared[o].diwstrt = chipset.diwstrt; cleared[o].diwstop = chipset.diwstop; cleared[o].bplcon0 = chipset.bplcon0;
            cleared[o].at = native_frame_counter;
        }
    }
    us_clear += cmm2->micros() - t0;
    int geom;
    if (omegaDdfIsFullWidth(chipset.ddfstrt)) {
        const int rot = omegaDdfRowRotation(chipset.ddfstrt, chipset.ddfstop);
        const int alt = host.displayIsLores && omegaLoresUsesAlternateRasterRows(chipset.diwstrt, chipset.diwstop);
        const int step = alt ? 2 : 1, y_offset = host.displayIsLores && !alt ? 64 : 0;
        int rows = RH / step;
        if (rows > (PH - y_offset) / 2) rows = (PH - y_offset) / 2;
        geom = 1 << 20 | y_offset << 10 | rows;
        if (page_border[pg] != bc || page_geom[pg] != geom) { border(pg, bc, 0, PW, y_offset, y_offset + 2 * rows); page_border[pg] = bc; page_geom[pg] = geom; }
        /* each raster row on page rows y_offset + 2y and +1 (the monitor's lines) */
        const int y = y_offset;
        if (rot) {
            copy_rows(pg, 0, rows, step, y, rot, 0, RW - rot);
            copy_rows(pg, 0, rows, step, y, 0, RW - rot, rot);
        } else
            copy_rows(pg, 0, rows, step, y, 0, 0, RW);
    } else {
        /* raster row sy on page rows dy = HOST_CONTENT_Y + 2 sy - viewport_y_offset and dy + 1, its columns from
           HOST_FETCH_LEAD + HOST_VISIBLE_X0 on page columns HOST_VISIBLE_X0 to HOST_VISIBLE_X1 */
        int sy0 = 0;
        while (HOST_CONTENT_Y + 2 * sy0 - viewport_y_offset < 0) sy0++;
        int sy1 = sy0;
        while (sy1 < RH && HOST_CONTENT_Y + 2 * sy1 - viewport_y_offset + 1 < PH) sy1++;
        const int dy0 = HOST_CONTENT_Y + 2 * sy0 - viewport_y_offset;
        geom = dy0 << 10 | (sy1 - sy0);
        if (page_border[pg] != bc || page_geom[pg] != geom) {
            border(pg, bc, HOST_VISIBLE_X0, HOST_VISIBLE_X1, dy0, dy0 + 2 * (sy1 - sy0));
            page_border[pg] = bc; page_geom[pg] = geom;
        }
        copy_rows(pg, sy0, sy1 - sy0, 1, dy0, HOST_FETCH_LEAD + HOST_VISIBLE_X0, HOST_VISIBLE_X0, HOST_VISIBLE_X1 - HOST_VISIBLE_X0);
    }
    pump();
    if (!opt_noflip) flip_to = pg;
    cur ^= 1;
    host.pixels = raster[cur];
}

void hostInit(void) {
    for (int i = 0; i < 2; i++) {
        raster[i] = (host_pixel_t *)(((uintptr_t)malloc(RW * RH * 2 + 32) + 31) & ~(uintptr_t)31);
        if (!raster[i]) { printf("no memory for the raster\n"); exit(1); }
        memset(raster[i], 0, RW * RH * 2);
    }
    host.pixels = raster[cur];
    host.rasterRow = 0;
    host.rasterX = 0;
    host.displayIsLores = 0;
}
void hostClearRaster(uint32_t *render_fb, uint32_t border) { (void)render_fb; (void)border; }

/* ---- the sound: Paula's lines mixed (AudioMix.c) at 44.1 kHz, through a ring to the stream ---- */
#define RING 8192                               /* stereo frames, a power of 2 */
static int16_t ring[RING * 2];
static volatile uint32_t ring_in, ring_out;
static int audio_ok;
int hostAudioLine(const uint32_t *words, int count) {
    audioMixLine(words, count);
    return 1;
}
void hostAudioOut(const int16_t *samples, int frames) {
    if (!audio_ok) return;
    for (int i = 0; i < frames; i++) {
        if (ring_in - ring_out >= RING) break;  /* (full: the emulation ahead of the sound, which -fast allows) */
        const uint32_t k = ring_in & (RING - 1);
        ring[2 * k] = samples ? samples[2 * i] : 0;
        ring[2 * k + 1] = samples ? samples[2 * i + 1] : 0;
        ring_in++;
    }
}
static void fill(int16_t *buf, uint32_t frames) {   /* the stream's (PendSV) */
    for (uint32_t i = 0; i < frames; i++) {
        if (ring_out != ring_in) {
            const uint32_t k = ring_out & (RING - 1);
            buf[2 * i] = ring[2 * k]; buf[2 * i + 1] = ring[2 * k + 1];
            ring_out++;
        } else
            buf[2 * i] = buf[2 * i + 1] = 0;
    }
}

/* ---- the keyboard ---- */
void pressKey(uint16_t keyCode)   { (void)keyCode; }
void releaseKey(uint16_t keyCode) { (void)keyCode; }
void toggleLEDs(void)             { }

/* HID usage (4-0x65) to the Amiga's raw key code, by place (0xFF: none) */
static const uint8_t hid2amiga[0x66] = {
    [0x04] = 0x20, [0x05] = 0x35, [0x06] = 0x33, [0x07] = 0x22, [0x08] = 0x12, [0x09] = 0x23, [0x0A] = 0x24, [0x0B] = 0x25,   /* A-H */
    [0x0C] = 0x17, [0x0D] = 0x26, [0x0E] = 0x27, [0x0F] = 0x28, [0x10] = 0x37, [0x11] = 0x36, [0x12] = 0x18, [0x13] = 0x19,   /* I-P */
    [0x14] = 0x10, [0x15] = 0x13, [0x16] = 0x21, [0x17] = 0x14, [0x18] = 0x16, [0x19] = 0x34, [0x1A] = 0x11, [0x1B] = 0x32,   /* Q-X */
    [0x1C] = 0x15, [0x1D] = 0x31,                                                                                          /* Y, Z */
    [0x1E] = 0x01, [0x1F] = 0x02, [0x20] = 0x03, [0x21] = 0x04, [0x22] = 0x05, [0x23] = 0x06, [0x24] = 0x07, [0x25] = 0x08,   /* 1-8 */
    [0x26] = 0x09, [0x27] = 0x0A,                                                                                          /* 9, 0 */
    [0x28] = 0x44, [0x29] = 0x45, [0x2A] = 0x41, [0x2B] = 0x42, [0x2C] = 0x40,                                             /* Return Esc BS Tab Space */
    [0x2D] = 0x0B, [0x2E] = 0x0C, [0x2F] = 0x1A, [0x30] = 0x1B, [0x31] = 0x0D, [0x32] = 0x2B,                              /* - = [ ] \ # */
    [0x33] = 0x29, [0x34] = 0x2A, [0x35] = 0x00, [0x36] = 0x38, [0x37] = 0x39, [0x38] = 0x3A, [0x39] = 0x62,               /* ; ' ` , . / Caps */
    [0x3A] = 0x50, [0x3B] = 0x51, [0x3C] = 0x52, [0x3D] = 0x53, [0x3E] = 0x54, [0x3F] = 0x55, [0x40] = 0x56, [0x41] = 0x57,   /* F1-F8 */
    [0x42] = 0x58, [0x43] = 0x59,                                                                                          /* F9, F10 */
    [0x49] = 0x5F, [0x4C] = 0x46,                                                                                          /* Insert: HELP, Delete */
    [0x4F] = 0x4E, [0x50] = 0x4F, [0x51] = 0x4D, [0x52] = 0x4C,                                                            /* the arrows */
    [0x54] = 0x5C, [0x55] = 0x5D, [0x56] = 0x4A, [0x57] = 0x5E, [0x58] = 0x43,                                             /* keypad / * - + Enter */
    [0x59] = 0x1D, [0x5A] = 0x1E, [0x5B] = 0x1F, [0x5C] = 0x2D, [0x5D] = 0x2E, [0x5E] = 0x2F, [0x5F] = 0x3D, [0x60] = 0x3E,   /* keypad 1-8 */
    [0x61] = 0x3F, [0x62] = 0x0F, [0x63] = 0x3C, [0x64] = 0x30, [0x65] = 0x67,                                             /* 9 0 . <> Menu */
};
static const uint8_t mod2amiga[8] = {0x63, 0x60, 0x64, 0x66, 0x63, 0x61, 0x65, 0x67};   /* HID's modifier bits */

#define KQ 64
static uint8_t keyq[KQ];                        /* the Amiga's codes, bit 7 set for a release */
static uint32_t kq_in, kq_out;
static void key_event(uint8_t code, int up) { if (kq_in - kq_out < KQ) keyq[kq_in++ % KQ] = code | (up ? 0x80 : 0); }
static uint8_t held[256];                       /* the Amiga codes held, as keys_raw gave them */

static void next_disk(void);
static void keyboard(void) {
    uint8_t r[8], now[256];
    memset(now, 0, sizeof now);
    static int f11_was;
    int f11 = 0;
    const int n = cmm2->keys_raw(r);
    if (opt_keys) {                             /* (-keys: each change of the report on the console) */
        static uint8_t last[8];
        static int last_n = -99;
        if (n != last_n || memcmp(r, last, 8)) {
            printf("amiga: keys_raw %d: %02X %02X %02X %02X %02X %02X %02X %02X\n", n, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
            memcpy(last, r, 8); last_n = n;
        }
        static int lastk[7] = {-1};
        int k[7];
        cmm2->keys_held(k);
        if (memcmp(k, lastk, sizeof k)) {
            printf("amiga: keys_held %X %X %X %X %X %X, mods %X\n", k[0], k[1], k[2], k[3], k[4], k[5], k[6]);
            memcpy(lastk, k, sizeof k);
        }
    }
    if (n >= 0) {
        for (int b = 0; b < 8; b++) if (r[0] & (1 << b)) now[mod2amiga[b]] = 1;
        for (int i = 2; i < 8; i++) {
            const uint8_t u = r[i];
            if (u == 0x44) { f11 = 1; continue; }                                               /* F11: the next disk */
            if (u == 0x45 || (u == 0x4D && (r[0] & 0x11) && (r[0] & 0x44))) { quit = 1; continue; }   /* F12, Ctrl+Alt+End */
            if (u < sizeof hid2amiga && hid2amiga[u]) now[hid2amiga[u]] = 1;
            else if (u == 0x35) now[0x00] = 1;  /* (` is the Amiga code 0) */
        }
    }
    if (f11 && !f11_was) next_disk();
    f11_was = f11;
    for (int c = 0; c < 0x68; c++)
        if (now[c] != held[c]) { key_event((uint8_t)c, !now[c]); held[c] = now[c]; }
    /* the serial console: letters, digits, space, Enter; Ctrl-N the next disk, Ctrl-] back to MMBasic */
    for (int k; (k = cmm2->con_getc()) >= 0; ) {
        static const char *row1 = "1234567890", *rowq = "qwertyuiop", *rowa = "asdfghjkl", *rowz = "zxcvbnm";
        int code = -1;
        if (k == 0x1D) { quit = 1; continue; }
        if (k == 0x0E) { next_disk(); continue; }
        if (k >= 'A' && k <= 'Z') k += 32;
        const char *p;
        if ((p = strchr(row1, k)) && k) code = 0x01 + (int)(p - row1);
        else if ((p = strchr(rowq, k)) && k) code = 0x10 + (int)(p - rowq);
        else if ((p = strchr(rowa, k)) && k) code = 0x20 + (int)(p - rowa);
        else if ((p = strchr(rowz, k)) && k) code = 0x31 + (int)(p - rowz);
        else if (k == ' ') code = 0x40;
        else if (k == '\r' || k == '\n') code = 0x44;
        else if (k == 8 || k == 127) code = 0x41;
        else if (k == 27) code = 0x45;
        if (code >= 0) { key_event((uint8_t)code, 0); key_event((uint8_t)code, 1); }
    }
}

/* one key change a frame to the Amiga (CIA-A's serial port, as main_native.c's OMEGA_KEYS) */
static void key_send(void) {
    if (kq_out == kq_in) return;
    const uint8_t e = keyq[kq_out++ % KQ];
    const uint8_t code = e & 0x7F;
    CIAWrite(&CIAA, 0xC, (uint8_t)~((code << 1) | (e >> 7)));
    keyboardInt();
}

/* ---- the mouse (port 1) and a joystick (port 2) ---- */
static int mouse_here, mouse_cx, mouse_cy;
static void ports(void) {
    int m[4];
    const int have_mouse = cmm2->mouse(m);
    if (opt_keys) {                             /* (-keys: the mouse's buttons and presence on the console as they change) */
        static int last = -1;
        const int now = have_mouse ? (m[2] & 7) | 8 : 0;
        if (now != last) { printf("amiga: mouse %s, buttons %d\n", have_mouse ? "here" : "none", m[2] & 7); last = now; }
    }
    if (have_mouse) {
        if (!mouse_here) { mouse_here = 1; mouse_cx = 320; mouse_cy = 256; cmm2->mouse_set(mouse_cx, mouse_cy); }
        else {
            int dx = m[0] - mouse_cx, dy = m[1] - mouse_cy;
            if (dx > 100) dx = 100; if (dx < -100) dx = -100;
            if (dy > 100) dy = 100; if (dy < -100) dy = -100;
            const uint8_t h = (uint8_t)((chipset.joy0dat & 0xFF) + dx), v = (uint8_t)((chipset.joy0dat >> 8) + dy);
            chipset.joy0dat = (uint16_t)(v << 8 | h);
            if (dx || dy) cmm2->mouse_set(mouse_cx, mouse_cy);
        }
        if (m[2] & 1) CIAA.pra &= (uint8_t)~0x40; else CIAA.pra |= 0x40;
        if (m[2] & 2) chipset.potinp &= (uint16_t)~0x0400; else chipset.potinp |= 0x0400;
    }
    int lx = 128, ly = 128;
    const int b = cmm2->gamepad(1, &lx, &ly);
    int up = 0, down = 0, left = 0, right = 0, fire = 0;
    if (b >= 0) {
        up = (b >> 7 & 1) || ly < 64; down = (b >> 5 & 1) || ly > 192;
        left = (b >> 8 & 1) || lx < 64; right = (b >> 6 & 1) || lx > 192;
        fire = (b >> 10 & 15) != 0;
    }
    const int y1 = left, x1 = right, y0 = up ^ left, x0 = down ^ right;
    chipset.joy1dat = (uint16_t)(y1 << 9 | y0 << 8 | x1 << 1 | x0);
    if (fire) CIAA.pra &= (uint8_t)~0x80; else CIAA.pra |= 0x80;
}

/* ---- the disks ---- */
static int load_adf(const char *path) {
    const int fd = open(path, O_RDONLY);
    if (fd < 0) { printf("amiga: cannot open %s\n", path); return 0; }
    ADF2MFM(fd, df[0].mfmData);
    close(fd);
    return 1;
}
static long swap_at = -1;
static void next_disk(void) {
    if (ndisks < 2 || swap_at >= 0) return;
    curdisk = (curdisk + 1) % ndisks;
    if (df[0].hasDisk) floppyInsert(0);         /* (toggles: out) */
    swap_at = (long)native_frame_counter + 25;  /* in again half a second later, as main_native.c's OMEGA_DF0_SWAP */
    printf("amiga: DF0 %s\n", disk[curdisk]);
}

/* -nocopy's test card in page 0: a white frame at the page's edges, eight colour bars, a grey grid each 32 pixels */
static void test_card(void) {
    static const uint16_t bar[8] = {0xFFF, 0xFF0, 0x0FF, 0x0F0, 0xF0F, 0xF00, 0x00F, 0x000};
    for (int y = 0; y < PH; y++) {
        uint16_t *row = page[0] + y * PW;
        for (int x = 0; x < PW; x++) {
            uint16_t c = bar[x * 8 / PW];
            if (y >= PH / 2) c = (x % 32 == 0 || y % 32 == 0) ? 0x888 : 0x000;
            if (x < 2 || x >= PW - 2 || y < 2 || y >= PH - 2) c = 0xFFF;
            row[x] = c;
        }
    }
    __asm volatile ("dsb 0xF" ::: "memory");
}

/* ---- each frame (the core calls hostDisplay at its end) ---- */
static uint64_t t_next, t_last, t_stats, frame_us;
static int nstats;
static void frozen(void) {                      /* -freeze: the last picture, again and again, until F12 or Ctrl-] */
    printf("amiga: frozen at frame %lu\n", native_frame_counter);
    while (!quit) {
        t_next += 20032;
        while (cmm2->micros() < t_next) { }
        cur ^= 1;                               /* (the finished raster again: present() copies raster[cur], then turns to the other) */
        present();
        cmm2->poll();
        cmm2->break_take();
        keyboard();
        for (int k; (k = cmm2->con_getc()) >= 0; ) if (k == 0x1D) quit = 1;
    }
}
void hostDisplay(void) {
    native_frame_counter++;
    present();
    cmm2->poll();                               /* the USB devices seen to (the keyboard, the mouse, the pads: without it, none) */
    cmm2->break_take();                         /* (Ctrl-C is the Amiga's Ctrl and C, not the end of the program) */
    keyboard();
    key_send();
    ports();
    if (opt_click >= 0) {                       /* -click: Guardian's question answered */
        if ((long)native_frame_counter == opt_click) CIAA.pra &= (uint8_t)~0x40;
        if ((long)native_frame_counter == opt_click + 10) CIAA.pra |= 0x40;
    }
    if (swap_at >= 0 && (long)native_frame_counter >= swap_at) {
        if (load_adf(disk[curdisk]) && !df[0].hasDisk) floppyInsert(0);
        swap_at = -1;
    }
    uint64_t now = cmm2->micros();
    if (opt_stats) {
        frame_us += now - t_last;
        if (++nstats == 50) {
            printf("amiga: frame %lu, %.2f ms a frame (%.1f%% of 20): waiting %.2f, clearing %.2f\n", native_frame_counter,
                   frame_us / 50000.0, frame_us / 10000.0, us_wait / 50000.0, us_clear / 50000.0);
            nstats = 0; frame_us = 0; us_wait = us_clear = 0;
        }
    }
    if (!opt_fast) {                            /* 50 frames a second (PAL: 49.92) */
        t_next += 20032;
        if (t_next > now) { while (cmm2->micros() < t_next) { } }
        else if (now - t_next > 100000) t_next = now;   /* (behind: from here, not catching up) */
    }
    t_last = cmm2->micros();
    if (opt_freeze >= 0 && (long)native_frame_counter >= opt_freeze) { t_next = t_last; frozen(); }
}

void native_dump_ppm(const char *path) { (void)path; }
unsigned long native_nonblack_pixels(void) { return 0; }

/* ---- the Kickstart ---- */
static int load_rom(const char *path, uint8_t *dst, int ext_rom) {
    FILE *f = fopen(path, "rb");
    if (!f) { printf("amiga: no %s\n", path); return 0; }
    memset(dst, 0, 0x80000);
    const size_t n = fread(dst, 1, 0x80000, f);
    fclose(f);
    if (n != 0x40000 && n != 0x80000) { printf("amiga: %s is %u bytes (256 or 512 KB)\n", path, (unsigned)n); return 0; }
    if (n == 0x40000 && !ext_rom) memcpy(dst + 0x40000, dst, 0x40000);   /* (a 256 KB Kickstart mirrored) */
    return 1;
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-kick") && i + 1 < argc) kick = argv[++i];
        else if (!strcmp(argv[i], "-ext") && i + 1 < argc) ext = argv[++i];
        else if (!strcmp(argv[i], "-click") && i + 1 < argc) opt_click = atol(argv[++i]);
        else if (!strcmp(argv[i], "-fast")) opt_fast = 1;
        else if (!strcmp(argv[i], "-stats")) opt_stats = 1;
        else if (!strcmp(argv[i], "-wa")) getenv_flag_wa = 1;
        else if (!strcmp(argv[i], "-noflip")) opt_noflip = 1;
        else if (!strcmp(argv[i], "-keys")) opt_keys = 1;
        else if (!strcmp(argv[i], "-nocopy")) opt_nocopy = opt_noflip = 1;
        else if (!strcmp(argv[i], "-freeze") && i + 1 < argc) opt_freeze = atol(argv[++i]);
        else if (argv[i][0] != '-' && ndisks < MAXDISKS) disk[ndisks++] = argv[i];
    }
    printf("amiga: frank-amiga (Omega) on the Colour Maximite 2\n");
    if (!load_rom(kick, amiga_rom, 0)) return 1;
    if (ext) { if (!load_rom(ext, amiga_ext, 1)) return 1; extRomPresent = 1; }
    if (!(cmm2_mfm_image = cmm2->alloc(12798 * 82 * 2))) { printf("amiga: no room for the disk\n"); return 1; }

    cmm2_video v;
    if (cmm2->video_mode(1, 12, &v) < 0) { printf("amiga: MODE 1 in depth 12 needs services version 17 (firmware after 7.00.00RC4)\n"); return 1; }
    page[0] = (uint16_t *)v.fb;                 /* (MMBasic's page 0, 800 x 600, holds a 640 x 512 page) */
    page[1] = (uint16_t *)(((uintptr_t)malloc(PW * PH * 2 + 32) + 31) & ~(uintptr_t)31);
    if (!page[1]) { printf("amiga: no memory for the second page\n"); return 1; }
    memset(page[1], 0, PW * PH * 2);
    {                                           /* a third page, from MMBasic's heap if it has room */
        void *b = cmm2->alloc(PW * PH * 2 + 32);
        if (b) { page[2] = (uint16_t *)(((uintptr_t)b + 31) & ~(uintptr_t)31); memset(page[2], 0, PW * PH * 2); npages = 3; }
        else printf("amiga: two pages (no room for a third)\n");
    }
    if (opt_nocopy) test_card();
    if (getenv_flag_wa) wa_on();
    if (!axi_on()) { printf("amiga: the AXI RAM is not free\n"); return 1; }
    window_on();

    /* DF0 (main_native.c's order: the drive identified, the disk in at power-on) */
    floppyInit(0);
    if (ndisks && load_adf(disk[0])) {
        df[0].hasDisk = 1;
        df[0].pra &= 0xFB;                      /* /CHNG 0: changed */
        df[0].pra &= 0xDF;                      /* /DKRDY 0: ready */
    } else {                                    /* (an empty drive: a sync word on each track, so trackdisk gives up) */
        uint8_t *mfm = df[0].mfmData;
        for (int cyl = 0; cyl < 82; cyl++)
            for (int side = 0; side < 2; side++) { mfm[cyl * (12798 * 2) + side * 12798] = 0x44; mfm[cyl * (12798 * 2) + side * 12798 + 1] = 0x89; }
    }
    floppyInit(1); floppyInit(2); floppyInit(3);
    hostInit();
    nativeChipMask = 0x7FFFFu;                  /* the A500: 512 KB chip, mirrored, and 512 KB slow RAM */
    nativeSlowSize = 0x80000u;
    expansionFastRamSize = 0;
    expansionPicasso2Vram = 0;
    p2DebugBlits = 0;
    memoryNativeInit();
    cpu_init();
    ChipsetInit();

    const int rate = cmm2->audio_start(fill);
    audio_ok = rate > AUDIO_RATE - 200 && rate < AUDIO_RATE + 200;   /* (the timer makes 44,101 Hz: a few in a million) */
    if (!audio_ok) printf("amiga: the stream is %d Hz, not %d: no sound\n", rate, AUDIO_RATE);
    cmm2->con_screen(0);                        /* (the console's text to the serial port only: the screen is the Amiga's) */
    cmm2->frame_hook(hook);
    t_next = t_last = cmm2->micros();
    int balance = 0;
    while (!quit)
    {
        for (int i = 0; i < 200 && !quit; ) i += dmaRunSlice(&balance);
        if (ipend < npend) pump();              /* (the frame's copies still waiting for room) */
    }
    cmm2->frame_hook(NULL);
    while (cmm2->dma2d_busy()) { }
    window_off();
    axi_off();
    wa_off();
    printf("amiga: %lu frames\n", native_frame_counter);
    return 0;
}
