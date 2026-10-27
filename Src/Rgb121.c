/***********************************************************************************************************************
The PicoMite's RGB121 display on the CMM2: MODE n,4 (18 Oct, Peter's design, for the PicoMite games).

The pages of a 4-bit mode hold the PicoMite's RGB121 pixels as its framebuffers do: two to a byte, the left pixel in the
low nibble, each row (width + 1) / 2 bytes, so a program's PEEKs and POKEs at MM.INFO(WRITEBUFF) act as on the PicoMite.
The LTDC shows none of them. It shows an 8-bit page outside the page table (Rgb121Display, first in AXI SRAM, each row
twice where the mode doubles rows) whose CLUT entries 0 to 15 are the 16 colours, which MAP changes. At the end of each
frame the LTDC's line interrupt asks for the merge, which runs at a low priority (CEC_IRQn, a peripheral not used, as a
software interrupt: below the audio and everything else): N, with the layer L over it and the top layer T over that
where they are created, a pixel of a layer's transparent colour showing what is under it (the PicoMite's VGA scanline
loop), each pixel a byte of the display.

The PicoMite's framebuffers are pages 0 to 4 (N, L, F, 2, T: Rgb121.h), laid out by pagegen after the display, in AXI
SRAM and then SDRAM. FRAMEBUFFER CREATE, CREATE 2, LAYER [t], LAYER TOP [t], WRITE, COPY, MERGE, WAIT and CLOSE act as
the PicoMite's (graphics/FrameBuffer.c) on those pages; a buffer not created is N, as the PicoMite's pointer to it is
DisplayBuf. Pages 5 on are more 4-bit pages, for PAGE WRITE, PAGE COPY, BLIT and the like. Nothing assumes a size: any
mode whose display fits AXI SRAM can be 4-bit (Rgb121ModeFits), MODE 7,4 being the PicoMite's MODE 2 and MODE 8,4 its
MODE 3.

The drawing set (DrawPixel4 and the others, set by setmode as the 8, 16 and 32-bit ones are) works on the write page and
honours OPTION Y AXIS INVERT as they do. The "fast" buffers (BLIT READ, SPRITE, PAGE SCROLL) hold a byte a pixel, its
RGB121 value, so the 8-bit sprite code serves a 4-bit page as it is, colour 0 (black) the transparent one as there.

The 1-bit depth (MODE n,1, the PicoMite's MODE 1: Mono.c) shares the display, the software interrupt and the
FRAMEBUFFER code here. Its merge (MonoMergeRun) is N ORed with L, each pixel its tile's colour, and its MERGE is F OR 2.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Rgb121.h"
#include "Mono.h"

extern volatile uint8_t pagesetdone;
extern int ModeXres(int mode), ModeYres(int mode);

uint8_t *Rgb121Display;
int Rgb121DisplayScale = 1;
volatile int Rgb121Active;
int Rgb121Buffers;
int Rgb121Transparent, Rgb121TransparentTop;

#define RGB121MAXW 1024                                             // the widest mode the merge's row buffers take
#define RoundUp8K(a) (((a) + 8 * 1024 - 1) & ~(8 * 1024 - 1))

static inline int Stride(int page) { return VideoColour == 1 ? PageTable[page].xmax >> 3 : (PageTable[page].xmax + 1) >> 1; }
static inline uint8_t *Row(int page, int y) { return PageTable[page].address + y * Stride(page); }
static inline void Put4(uint8_t *row, int x, int c) {
    uint8_t *p = row + (x >> 1);
    if(x & 1) *p = (*p & 0x0F) | (c << 4);
    else *p = (*p & 0xF0) | c;
}
static inline int Get4(const uint8_t *row, int x) { return (row[x >> 1] >> ((x & 1) << 2)) & 0xF; }

// pixels x1 to x2 of a row in colour c (0-15): the odd and even ends as nibbles, the bytes between at once
static void Run4(uint8_t *row, int x1, int x2, int c) {
    if(x1 & 1) Put4(row, x1++, c);
    if(x1 > x2) return;
    if(!(x2 & 1)) Put4(row, x2--, c);
    if(x1 > x2) return;
    memset(row + (x1 >> 1), c | (c << 4), ((x2 - x1) >> 1) + 1);
}

/* w pixels of row s from pixel sx to row d at pixel dx, a pixel of value t (0-15) left alone (-1 for none). Rows of the
   same parity without transparency are moved a byte at a time; d may be s (a row moved along itself). The PicoMite's
   blit121, a row of it. */
void Rgb121Row(uint8_t *d, int dx, const uint8_t *s, int sx, int w, int t) {
    if(w <= 0) return;
    int right = d == s && dx > sx;                                  // along itself to the right: from the end
    if(t < 0 && !((sx ^ dx) & 1)) {
        int head = sx & 1, hs = sx >> 1, hd = dx >> 1, n, tail, ts, td;
        if(head) { sx++; dx++; w--; }                               // an odd start: a high nibble
        n = w >> 1;
        tail = w & 1;                                               // an odd end: a low nibble
        ts = (sx >> 1) + n;
        td = (dx >> 1) + n;
        if(right && tail) d[td] = (d[td] & 0xF0) | (s[ts] & 0x0F);
        if(!right && head) d[hd] = (d[hd] & 0x0F) | (s[hs] & 0xF0);
        if(n) memmove(d + (dx >> 1), s + (sx >> 1), n);
        if(right && head) d[hd] = (d[hd] & 0x0F) | (s[hs] & 0xF0);
        if(!right && tail) d[td] = (d[td] & 0xF0) | (s[ts] & 0x0F);
        return;
    }
    if(right) {
        for(int i = w - 1; i >= 0; i--) { int v = Get4(s, sx + i); if(v != t) Put4(d, dx + i, v); }
    } else {
        for(int i = 0; i < w; i++) { int v = Get4(s, sx + i); if(v != t) Put4(d, dx + i, v); }
    }
}

/***********************************************************************************************************************
The mode
***********************************************************************************************************************/
// whether a display w x h (each row 'scale' times) can be 4-bit: in AXI SRAM, its rows in the merge's buffers
int Rgb121SizeFits(int w, int h, int scale) {
    return w <= RGB121MAXW && (w & 3) == 0 && w * h * scale <= 512 * 1024;
}

// whether a mode can be 4-bit
int Rgb121ModeFits(int mode) {
    return Rgb121SizeFits(ModeXres(mode), ModeYres(mode), ModeRowScale(mode));
}

// pagegen's start for a 4-bit mode (or 8-bit rows shown four times): the display at 'start' (AXI SRAM), the
// framebuffers none; the pages follow it
uint32_t Rgb121Layout(int mode, uint32_t start) {
    Rgb121Display = (uint8_t *)start;
    Rgb121DisplayScale = ModeRowScale(mode);
    Rgb121Buffers = 0;
    Rgb121Transparent = Rgb121TransparentTop = 0;
    return start + RoundUp8K(ModeXres(mode) * ModeYres(mode) * Rgb121DisplayScale);
}

// the PicoMite's 16 colours for the mode being set (setmode), which COLOUR MAP, PIXEL( and the 4-bit CLUT use: its VGA
// colours, and its HDMI ones in an OPTION PICOMITE HDMI mode, whose greens differ (0x55 and 0xAA: its MAP16DEF)
void Rgb121Palette(void) {
    const int *pal = (PicoActive && PicoFlavour() == PICO_HDMI) ? Rgb121Hdmi : Rgb121Vga;
    for(int i = 0; i < 16; i++) Rgb121Colours[i] = pal[i];
}

// the 16 colours a 4-bit mode shows, as the program gave them (the PicoMite's MapRGB): MAP n = c stages one, MAP SET
// shows them, MAP RESET, GREYSCALE and the mode's set-up show them all. SAVE IMAGE, SAVE PNG and Print Screen save
// these (Rgb121Shown); the CLUT has them at OPTION VGA OUTPUT's levels, and ReadBuffer4 the colour before MAP
static uint32_t MapRGB[16], RemapRGB[16];
void SetMapRGB(int slot, uint32_t colour) {                         // MAP n = colour: staged only
    if(slot >= 0 && slot < 16) RemapRGB[slot] = colour & 0xFFFFFF;
}
void CommitMapRGB(void) {                                           // MAP SET
    for(int i = 0; i < 16; i++) MapRGB[i] = RemapRGB[i];
}
// a row of n colours from ReadBuffer4 (blue, green, red) in the colours the screen shows them
void Rgb121Shown(char *p, int n) {
    for(int k = 0; k < n * 3; k += 3) {
        uint32_t c = MapRGB[RGB121((uint32_t)(uint8_t)p[k + 2] << 16 | (uint32_t)(uint8_t)p[k + 1] << 8 | (uint8_t)p[k])];
        p[k] = c;
        p[k + 1] = c >> 8;
        p[k + 2] = c >> 16;
    }
}

// the CLUT: entries 0 to 15 the PicoMite's colours at OPTION VGA OUTPUT's levels (VgaLevel: COLOUR MAP and PIXEL( give
// Rgb121Colours'), the others black (MODE n,4 and MAP RESET)
void Rgb121Clut(void) {
    Rgb121Palette();
    for(int i = 0; i < 256; i++) CLUT[i] = i < 16 ? VgaLevel((uint32_t)Rgb121Colours[i]) : 0;
    for(int i = 0; i < 16; i++) SetMapRGB(i, (uint32_t)Rgb121Colours[i]);
    CommitMapRGB();
}

// a program's end with a PicoMite mode showing (PicoMode.c): its framebuffers closed, as the PicoMite's do_end does
void Rgb121CloseAll(void) {
    if(VideoColour >= 8) return;
    Rgb121Buffers = 0;
    Rgb121Transparent = Rgb121TransparentTop = 0;
    HRes = PageTable[RGB121_N].xmax;
    VRes = PageTable[RGB121_N].ymax;
    MonoDirty = 1;
}

/***********************************************************************************************************************
GUI CURSOR in a 4 or 1-bit mode: drawn over the merge's display, at xcursor, ycursor (Draw.c), so the program's pages
are not touched: the PicoMite's programs copy F to N under it (MMbg). Each of its rows goes in as the merge writes that
row, which is before the LTDC shows it: drawn after the whole merge it came too late for the top of the screen, which the
LTDC had shown by then (Peter: above about line 200 it disappeared). The shapes are the 8-bit depth's (a byte a pixel,
not 0 in the cursor), in the colour's CLUT entry. The 4-bit merge runs every frame; the 1-bit one is asked for
(MonoDirty) when the cursor moves.
***********************************************************************************************************************/
static volatile int OverlayOn, OverlayEntry;
void CursorOverlaySet(int on) {
    OverlayOn = on;
    MonoDirty = 1;
}
void CursorOverlayColour(int c) {
    OverlayEntry = VideoColour == 1 ? MonoColourEntry(c) : RGB121(c);   // (4-bit: entries 0-15 are the 16 colours)
}
static struct { int x, y, w, h; const uint8_t *s; uint8_t e; } Ov;   // the cursor for this merge (OvStart): h 0 none
static void OvStart(void) {
    Ov.h = 0;
    if(!OverlayOn || !cursorenable) return;
    Ov.x = xcursor; Ov.y = ycursor; Ov.w = wcursor; Ov.h = hcursor;
    Ov.s = cursor == 1 ? cross8 : cursor8;
    Ov.e = OverlayEntry;
}
// the cursor's part of page row y, on its display rows (from the merge's loop, once it has written them)
static inline void OvRow(int y, int w, int scale) {
    if(y < Ov.y || y >= Ov.y + Ov.h) return;
    const uint8_t *s = Ov.s + (y - Ov.y) * Ov.w;
    for(int k = 0; k < scale; k++) {
        uint8_t *row = Rgb121Display + (y * scale + k) * w;
        for(int x = 0; x < Ov.w; x++) {
            int xx = Ov.x + x;
            if(xx >= 0 && xx < w && s[x]) row[xx] = Ov.e;
        }
    }
}

/***********************************************************************************************************************
The merge: N, then L and T over it where they are created, onto the display, a byte a pixel
***********************************************************************************************************************/
// the layer word v over the word f: each nibble of v that is not t's (t in every nibble) comes from v (FrameBuffer.c's
// merge_words: a lane of ((z & low) + low) | z has its top bit set exactly when z's lane is not zero)
static inline uint32_t Over(uint32_t f, uint32_t v, uint32_t t) {
    uint32_t z = v ^ t;
    uint32_t mask = (((((z & 0x77777777u) + 0x77777777u) | z) & 0x88888888u) >> 3) * 0xFu;
    return (v & mask) | (f & ~mask);
}

static uint32_t MergeN[RGB121MAXW / 8], MergeL[RGB121MAXW / 8];
static void __attribute__((noinline)) ITCM_CODE Rgb121Merge(void) {   // (not inlined: in ITCM)
    int w = PageTable[RGB121_N].xmax, h = PageTable[RGB121_N].ymax, scale = Rgb121DisplayScale;
    int bytes = (w + 1) >> 1, words = (bytes + 3) >> 2, dwords = w >> 2, i;
    int layer = (Rgb121Buffers >> RGB121_L) & 1, top = (Rgb121Buffers >> RGB121_T) & 1;
    uint32_t tl = (uint32_t)Rgb121Transparent * 0x11111111u, tt = (uint32_t)Rgb121TransparentTop * 0x11111111u;
    const uint8_t *n = PageTable[RGB121_N].address, *l = PageTable[RGB121_L].address, *t = PageTable[RGB121_T].address;
    uint32_t *d = (uint32_t *)Rgb121Display;
    if(n == NULL || d == NULL || w > RGB121MAXW) return;
    if(l == NULL) layer = 0;
    if(t == NULL) top = 0;
    OvStart();
    for(int y = 0; y < h; y++) {
        mycpy(MergeN, n, bytes);                                    // (a row at a time from AXI SRAM, which is not cached)
        n += bytes;
        if(layer) {
            mycpy(MergeL, l, bytes);
            l += bytes;
            for(i = 0; i < words; i++) MergeN[i] = Over(MergeN[i], MergeL[i], tl);
        }
        if(top) {
            mycpy(MergeL, t, bytes);
            t += bytes;
            for(i = 0; i < words; i++) MergeN[i] = Over(MergeN[i], MergeL[i], tt);
        }
        uint32_t *d0 = d;                                           // the row, and its copies where rows are repeated
        d += dwords * scale;
        for(i = 0; 2 * i < dwords; i++) {                           // a word of 8 pixels, two words of bytes
            uint32_t v = MergeN[i], a, b;
            a = v & 0xFFFF;
            a = (a | (a << 8)) & 0x00FF00FF;
            a = (a | (a << 4)) & 0x0F0F0F0F;
            for(int k = 0; k < scale; k++) d0[k * dwords + 2 * i] = a;
            if(2 * i + 1 < dwords) {
                b = v >> 16;
                b = (b | (b << 8)) & 0x00FF00FF;
                b = (b | (b << 4)) & 0x0F0F0F0F;
                for(int k = 0; k < scale; k++) d0[k * dwords + 2 * i + 1] = b;
            }
        }
        OvRow(y, w, scale);
    }
}

/* The 1-bit merge (MODE n,1: Mono.c): the CLUT entries given colours since the last frame first, while the LTDC is not
   reading the CLUT (the vertical blanking). Then, if something was drawn, or every 16th frame for what a program wrote
   to a page itself, N with L ORed in where it is created: each byte 8 pixels of one tile, a set pixel its foreground's
   entry, the others its background's (a nibble's bits spread to the bytes of a word by a multiply). */
static void __attribute__((noinline)) ITCM_CODE MonoMergeRun(void) {
    static int idle;
    for(int i = 0; i < 8; i++) {
        uint32_t m = MonoClutNew[i];
        MonoClutNew[i] = 0;
        while(m) {
            int k = i * 32 + __builtin_ctz(m);
            m &= m - 1;
            LTDC_Layer1->CLUTWR = ((uint32_t)k << 24) | (CLUT[k] & 0xFFFFFF);
        }
    }
    if(!MonoDirty && ++idle < 16) return;
    idle = 0;
    MonoDirty = MonoDirty > 1 ? MonoDirty - 1 : 0;                  // (2, a POKE's: set before its write, so the next too)
    int w = PageTable[RGB121_N].xmax, h = PageTable[RGB121_N].ymax, scale = Rgb121DisplayScale;
    int bytes = w >> 3, words = (bytes + 3) >> 2, dwords = w >> 2, i, tc = 0, trow = 0;
    int layer = (Rgb121Buffers >> RGB121_L) & 1;
    const uint8_t *n = PageTable[RGB121_N].address, *l = PageTable[RGB121_L].address;
    uint8_t *rn = (uint8_t *)MergeN, *tf = (uint8_t *)MergeL + 256, *tb = (uint8_t *)MergeN + 256;
    uint32_t *d = (uint32_t *)Rgb121Display;
    if(n == NULL || d == NULL || bytes > 240 || MonoTileF == NULL) return;
    if(l == NULL) layer = 0;
    OvStart();
    for(int y = 0; y < h; y++) {
        if(tc == 0) {                                               // a new row of tiles
            mycpy(tf, MonoTileF + trow * MonoTilesX, bytes);
            mycpy(tb, MonoTileB + trow * MonoTilesX, bytes);
            trow++;
        }
        if(++tc == MonoTileH) tc = 0;
        mycpy(rn, n, bytes);
        n += bytes;
        if(layer) {
            mycpy(MergeL, l, bytes);
            l += bytes;
            for(i = 0; i < words; i++) MergeN[i] |= MergeL[i];
        }
        uint32_t *d0 = d;                                           // the row, and its copies where rows are repeated
        d += dwords * scale;
        for(i = 0; i < bytes; i++) {
            uint32_t v = rn[i], f = tf[i] * 0x01010101u, b = tb[i] * 0x01010101u, x = f ^ b;
            uint32_t lo = (((v & 0xF) * 0x00204081u) & 0x01010101u) * 0xFFu;
            uint32_t hi = (((v >> 4) * 0x00204081u) & 0x01010101u) * 0xFFu;
            for(int k = 0; k < scale; k++) {
                d0[k * dwords + 2 * i] = b ^ (x & lo);
                d0[k * dwords + 2 * i + 1] = b ^ (x & hi);
            }
        }
        OvRow(y, w, scale);
    }
}

/* The 8-bit merge: OPTION PICOMITE's MODE 5 at 1024x600, 1280x720 and 1024x768 shows each row four times, which an
   8-bit page cannot hold (it stores a row twice at most), so page 0's rows are copied to the display that many times */
static void __attribute__((noinline)) ITCM_CODE Merge8Run(void) {
    int w = PageTable[0].xmax, h = PageTable[0].ymax, scale = Rgb121DisplayScale, dwords = w >> 2;
    const uint8_t *n = PageTable[0].address;
    uint32_t *d = (uint32_t *)Rgb121Display;
    if(n == NULL || d == NULL || w > (int)sizeof(MergeN)) return;
    for(int y = 0; y < h; y++) {
        mycpy(MergeN, n, w);
        n += w;
        for(int k = 0; k < scale; k++, d += dwords) mycpy(d, MergeN, w);
    }
}

// the software interrupt the LTDC's line interrupt pends at the end of each frame
void CEC_IRQHandler(void) {
    if(!Rgb121Active) return;
    if(VideoColour == 1) MonoMergeRun();
    else if(VideoColour == 8) Merge8Run();
    else Rgb121Merge();
}
void Rgb121MergeRequest(void) {
    NVIC_SetPendingIRQ(CEC_IRQn);
}

/***********************************************************************************************************************
The drawing set, on the write page (the read page for reading)
***********************************************************************************************************************/
void DrawPixel4(int x, int y, int c) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax;
    if(optiony) y = maxH - 1 - y;
    if(x < 0 || x >= maxW || y < 0 || y >= maxH) return;
    Put4(Row(WritePage, y), x, RGB121(c));
}

void DrawRectangle4(int x1, int y1, int x2, int y2, int c) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax, t;
    if(optiony) { y1 = maxH - 1 - y1; y2 = maxH - 1 - y2; }
    if((x1 < 0 && x2 < 0) || (y1 < 0 && y2 < 0) || (x1 >= maxW && x2 >= maxW) || (y1 >= maxH && y2 >= maxH)) return;
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    if(x1 < 0) x1 = 0;
    if(x2 >= maxW) x2 = maxW - 1;
    if(y1 < 0) y1 = 0;
    if(y2 >= maxH) y2 = maxH - 1;
    c = RGB121(c);
    for(int y = y1; y <= y2; y++) Run4(Row(WritePage, y), x1, x2, c);
}

void DrawBitmap4(int x1, int y1, int width, int height, int scale, int fc, int bc, unsigned char *bitmap) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax;
    int f = RGB121(fc), b = RGB121(bc);
    if(optiony) y1 = maxH - 1 - y1;                                 // (as DrawBitmap8: the text is not turned over)
    if(x1 >= maxW || y1 >= maxH || x1 + width * scale < 0 || y1 + height * scale < 0) return;
    for(int i = 0; i < height; i++) {
        for(int j = 0; j < scale; j++) {
            int y = y1 + i * scale + j;
            if(y < 0) continue;
            if(y >= maxH) return;
            uint8_t *row = Row(WritePage, y);
            for(int k = 0; k < width; k++) {
                int set = (bitmap[((i * width) + k) / 8] >> (((height * width) - ((i * width) + k) - 1) % 8)) & 1;
                if(!set && bc < 0) continue;
                for(int m = 0; m < scale; m++) {
                    int x = x1 + k * scale + m;
                    if(x >= 0 && x < maxW) Put4(row, x, set ? f : b);
                }
            }
        }
    }
}

// a buffer of colours (DrawBuffer8's forms: BGR, RGB with skip & 2, 4 bytes with skip & 1, RGB565 with skip & 8, a
// pixel under skip & 0xF0's alpha left alone with skip & 4) onto the page as RGB121
void DrawBuffer4(int x1, int y1, int x2, int y2, char *p, int skip) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax, t, deftrans = skip & 0xF0;
    int step = (skip & 8) ? 2 : ((skip & 1) ? 4 : 3);
    union { char rgbbytes[4]; uint16_t argb[2]; uint32_t rgb; } c;
    if(optiony) { y1 = maxH - 1 - y1; y2 = maxH - 1 - y2; }
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(int y = y1; y <= y2; y++) {
        routinechecks(1);
        uint8_t *row = (y >= 0 && y < maxH) ? Row(WritePage, y) : NULL;
        for(int x = x1; x <= x2; x++) {
            if(row == NULL || x < 0 || x >= maxW) { p += step; continue; }
            c.rgb = 0;
            if(skip & 2) {
                c.rgbbytes[2] = *p++;
                c.rgbbytes[1] = *p++;
                c.rgbbytes[0] = *p++;
                if(skip & 1) c.rgbbytes[3] = (*p++) & 0xF0;
            } else if(skip & 8) {
                c.rgbbytes[0] = *p++;
                c.rgbbytes[1] = *p++;
            } else {
                c.rgbbytes[0] = *p++;
                c.rgbbytes[1] = *p++;
                c.rgbbytes[2] = *p++;
                if(skip & 1) p++;
            }
            if(skip & 8) Put4(row, x, ((c.argb[0] >> 12) & 8) | ((c.argb[0] >> 8) & 6) | ((c.argb[0] >> 4) & 1));
            else if((uint8_t)c.rgbbytes[3] >= deftrans || (skip & 4) == 0) Put4(row, x, RGB121(c.rgb));
        }
    }
}

// the page's pixels as colours, three bytes each (blue, green, red): the PicoMite's colours, MAP not applied (its ReadBuffer16)
void ReadBuffer4(int x1, int y1, int x2, int y2, char *p) {
    int maxW = PageTable[ReadPage].xmax, maxH = PageTable[ReadPage].ymax, t;
    if(optiony) { y1 = maxH - 1 - y1; y2 = maxH - 1 - y2; }
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(int y = y1; y <= y2; y++) {
        routinechecks(1);
        for(int x = x1; x <= x2; x++) {
            if(x >= 0 && x < maxW && y >= 0 && y < maxH) {
                int c = Rgb121Colours[Get4(Row(ReadPage, y), x)];
                *p++ = c & 0xFF;
                *p++ = (c >> 8) & 0xFF;
                *p++ = (c >> 16) & 0xFF;
            } else p += 3;
        }
    }
}

// the "fast" buffers: a byte a pixel, its RGB121 value
void ReadBufferFast4(int x1, int y1, int x2, int y2, char *p) {
    int maxW = PageTable[ReadPage].xmax, maxH = PageTable[ReadPage].ymax, t;
    if(optiony) { y1 = maxH - 1 - y1; y2 = maxH - 1 - y2; }
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(int y = y1; y <= y2; y++) {
        routinechecks(1);
        const uint8_t *row = (y >= 0 && y < maxH) ? Row(ReadPage, y) : NULL;
        for(int x = x1; x <= x2; x++, p++) if(row != NULL && x >= 0 && x < maxW) *p = Get4(row, x);
    }
}

void DrawBufferFast4(int x1, int y1, int x2, int y2, char *p) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax, t;
    if(optiony) { y1 = maxH - 1 - y1; y2 = maxH - 1 - y2; }
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(int y = y1; y <= y2; y++) {
        routinechecks(1);
        uint8_t *row = (y >= 0 && y < maxH) ? Row(WritePage, y) : NULL;
        for(int x = x1; x <= x2; x++, p++) if(row != NULL && x >= 0 && x < maxW) Put4(row, x, *p & 0xF);
    }
}

// the page moved up (lines > 0) or down, as ScrollBuff8V
void ScrollBuff4V(int lines, int blank) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax, n = Stride(WritePage), y;
    maxH = ConsoleVRes();                                       // (the whole rows above the on-screen keyboard)
    if(lines > 0) {
        for(y = 0; y < maxH - lines - (ShortScroll ? gui_font_height : 0); y++) mycopy(Row(WritePage, y), Row(WritePage, y + lines), n);
        if(blank) {
            if(ShortScroll) DrawRectangle(0, maxH - lines - gui_font_height, maxW - 1, maxH - gui_font_height - 1, gui_bcolour);
            else DrawRectangle(0, maxH - lines, maxW - 1, maxH - 1, gui_bcolour);
        }
    } else if(lines < 0) {
        lines = -lines;
        for(y = maxH - 1; y >= lines; y--) mycopy(Row(WritePage, y), Row(WritePage, y - lines), n);
        if(blank) DrawRectangle(0, 0, maxW - 1, lines - 1, gui_bcolour);
    }
}

// the page moved right (pixels > 0) or left, as ScrollBuff8H (what is uncovered is left as it was)
void ScrollBuff4H(int pixels) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax;
    if(!pixels) return;
    for(int y = 0; y < maxH; y++) {
        uint8_t *row = Row(WritePage, y);
        if(pixels > 0) Rgb121Row(row, pixels, row, 0, maxW - pixels, -1);
        else Rgb121Row(row, 0, row, -pixels, maxW + pixels, -1);
    }
}

void Rgb121Pointers(void) {
    DrawPixel = DrawPixel4;
    DrawRectangle = DrawRectangle4;
    DrawBitmap = DrawBitmap4;
    DrawBuffer = DrawBuffer4;
    ReadBuffer = ReadBuffer4;
    ScrollLCD = ScrollBuff4V;
    ScrollBufferV = ScrollBuff4V;
    ScrollBufferH = ScrollBuff4H;
    DrawBufferFast = DrawBufferFast4;
    ReadBufferFast = ReadBufferFast4;
    BlitShowBuffer = BlitShowBuff;                                  // (a byte a pixel, as the 8-bit modes' buffers)
    HAL_NVIC_SetPriority(CEC_IRQn, IRQ_PRI(6), 0);                           // the merge: below everything
    HAL_NVIC_EnableIRQ(CEC_IRQn);
}

// DrawPixelFast, ReadPixelFast and DrawHLineFast's pixels in a 4-bit mode (the page's coordinates, c its RGB121 value)
void Rgb121Set(int x, int y, int c) { Put4(Row(WritePage, y), x, c & 0xF); }
int Rgb121Get(int x, int y) { return Get4(Row(ReadPage, y), x); }
void Rgb121HLine(int x1, int x2, int y, int c) { Run4(Row(WritePage, y), x1, x2, c & 0xF); }

// PAGE COPY (PageCopy): the whole page, or its pixels that are not black
void Rgb121PageCopy(int from, int to, int transparent) {
    int w = PageTable[from].xmax < PageTable[to].xmax ? PageTable[from].xmax : PageTable[to].xmax;
    int h = PageTable[from].ymax < PageTable[to].ymax ? PageTable[from].ymax : PageTable[to].ymax;
    if(from == to) return;
    if(!transparent && PageTable[from].xmax == PageTable[to].xmax) {
        mycopy(PageTable[to].address, PageTable[from].address, Stride(from) * h);
        return;
    }
    for(int y = 0; y < h; y++) {
        Rgb121Row(Row(to, y), 0, Row(from, y), 0, w, transparent ? 0 : -1);
        if(!(y & 15)) routinechecks(1);
    }
}

/* BLIT x1, y1, x2, y2, w, h [, page] [, orientation] (cmd_blit, the rectangle clipped to both pages): from the read
   page to the write page, mirrored left to right with orientation bit 0, top to bottom with bit 1, black left alone
   with bit 2, as MoveBufferNormal */
void Rgb121Blit(int x1, int y1, int x2, int y2, int w, int h, int flip) {
    int t = (flip & 4) ? 0 : -1, y;
    if(optiony) {
        y1 = PageTable[ReadPage].ymax - y1 - h;
        y2 = PageTable[WritePage].ymax - y2 - h;
    }
    if(!(flip & 3)) {
        if(ReadPage == WritePage && y2 > y1) {                      // (down the page: from the bottom row)
            for(y = h - 1; y >= 0; y--) Rgb121Row(Row(WritePage, y2 + y), x2, Row(ReadPage, y1 + y), x1, w, t);
        } else {
            for(y = 0; y < h; y++) Rgb121Row(Row(WritePage, y2 + y), x2, Row(ReadPage, y1 + y), x1, w, t);
        }
        return;
    }
    uint8_t *buf = GetTempMemory(w * h);                            // (turned: the rectangle is read first)
    for(y = 0; y < h; y++) {
        const uint8_t *s = Row(ReadPage, y1 + y);
        for(int x = 0; x < w; x++) buf[y * w + x] = Get4(s, x1 + x);
    }
    for(y = 0; y < h; y++) {
        const uint8_t *s = buf + ((flip & 2) ? h - 1 - y : y) * w;
        uint8_t *d = Row(WritePage, y2 + y);
        for(int x = 0; x < w; x++) {
            int v = s[(flip & 1) ? w - 1 - x : x];
            if(v != t) Put4(d, x2 + x, v);
        }
    }
}

/***********************************************************************************************************************
The PicoMite's framebuffers
***********************************************************************************************************************/
// a framebuffer letter (N, L, F, 2, T) as its page, -1 if it is not one; in a 4 or 1-bit mode a buffer not created is
// N, as the PicoMite's pointer to it is DisplayBuf (the other modes: the pages the game ports put them on,
// Game_Porting_Log.md)
int Rgb121Letter(char *p) {
    int page;
    if(checkword(p, "N")) page = RGB121_N;      // (the letter alone: N(1) is an expression, the PicoMite's #112)
    else if(checkword(p, "L")) page = RGB121_L;
    else if(checkword(p, "F")) page = RGB121_F;
    else if(checkword(p, "2")) page = RGB121_2;
    else if(checkword(p, "T")) page = RGB121_T;
    else return -1;
    if(VideoColour < 8 && !(Rgb121Buffers & (1 << page))) page = RGB121_N;
    return page;
}

// CLS without a colour on a layer clears it to its transparent colour, as the PicoMite's; 1 if it did
int Rgb121Cls(void) {
    int t;
    if(VideoColour != 4) return 0;
    if(WritePage == RGB121_L && (Rgb121Buffers & (1 << RGB121_L))) t = Rgb121Transparent;
    else if(WritePage == RGB121_T && (Rgb121Buffers & (1 << RGB121_T))) t = Rgb121TransparentTop;
    else return 0;
    memset(PageTable[WritePage].address, t | (t << 4), Stride(WritePage) * PageTable[WritePage].ymax);
    return 1;
}

static int FbCreated(int page) { return page == RGB121_N || (Rgb121Buffers & (1 << page)); }

// to the end of the frame (",B"): the merge has then been made from what was drawn before
static void WaitFrame(void) {
    pagesetdone = 0;
    while(!pagesetdone) CheckAbort();
}

static void FbCreate(int page, int colour, char *exists) {
    if(FbCreated(page)) error(exists);
    if(page > LastPage || PageTable[page].address == NULL) error("Not enough memory for the framebuffer");
    memset(PageTable[page].address, colour | (colour << 4), Stride(page) * PageTable[page].ymax);
    Rgb121Buffers |= 1 << page;                                     // (cleared first: the merge never shows it otherwise)
}

static void FbClose(int page) {
    if(page == RGB121_N || !FbCreated(page)) return;
    Rgb121Buffers &= ~(1 << page);
    if(WritePage == page) WritePage = RGB121_N;
    if(ReadPage == page) ReadPage = RGB121_N;
    if(page == RGB121_L) Rgb121Transparent = 0;
    if(page == RGB121_T) Rgb121TransparentTop = 0;
}

static void FbWrite(int page) {
    WritePage = ReadPage = page;
    HRes = PageTable[page].xmax;
    VRes = PageTable[page].ymax;
}

/* FRAMEBUFFER in a 4 or 1-bit mode: the PicoMite's (FrameBuffer.c, its VGA and HDMI versions), its messages. In a
   1-bit mode a layer has no transparent colour (L is ORed into N: its argument is not read), and MERGE is N = F OR 2. */
void Rgb121Framebuffer(void) {
    char *p;
    MonoDirty = 1;                                                  // (a 1-bit mode's merge: N or L may change)
    if((p = checkstring(cmdline, "CREATE 2"))) {
        FbCreate(RGB121_2, 0, "Framebuffer 2 already exists");
    } else if((p = checkstring(cmdline, "CREATE"))) {
        checkend(p);                                                // (no size: F is the size of the screen)
        FbCreate(RGB121_F, 0, "Framebuffer already exists");
    } else if((p = checkstring(cmdline, "LAYER TOP"))) {
        int t = 0;
        getargs(&p, 1, ",");
        if(argc == 1 && VideoColour == 4) t = getint(argv[0], 0, 15);
        if(FbCreated(RGB121_T)) error("Framebuffer already exists");
        Rgb121TransparentTop = t;
        FbCreate(RGB121_T, t, "Framebuffer already exists");
    } else if((p = checkstring(cmdline, "LAYER"))) {
        int t = 0;
        getargs(&p, 1, ",");
        if(argc == 1 && VideoColour == 4) t = getint(argv[0], 0, 15);
        if(FbCreated(RGB121_L)) error("Framebuffer already exists");
        Rgb121Transparent = t;
        FbCreate(RGB121_L, t, "Framebuffer already exists");
    } else if((p = checkstring(cmdline, "CLOSE"))) {
        int page = -1;                                              // (F, L, T or 2; anything else closes them all)
        if(checkstring(p, "F")) page = RGB121_F;
        else if(checkstring(p, "L")) page = RGB121_L;
        else if(checkstring(p, "T")) page = RGB121_T;
        else if(checkstring(p, "2")) page = RGB121_2;
        if(page > 0) FbClose(page);
        else { FbClose(RGB121_L); FbClose(RGB121_F); FbClose(RGB121_2); FbClose(RGB121_T); FbWrite(RGB121_N); }
    } else if((p = checkstring(cmdline, "WRITE"))) {
        char *q;
        int page = RGB121_N;
        if(checkword(p, "N")) q = "N";
        else if(checkword(p, "L")) q = "L";
        else if(checkword(p, "F")) q = "F";
        else if(checkword(p, "2")) q = "2";
        else if(checkword(p, "T")) q = "T";
        else {
            getargs(&p, 1, ",");
            if(argc != 1) error("Syntax");
            q = getCstring(argv[0]);
        }
        if(strcasecmp(q, "N") == 0) page = RGB121_N;
        else if(strcasecmp(q, "L") == 0) page = RGB121_L;
        else if(strcasecmp(q, "F") == 0) page = RGB121_F;
        else if(strcasecmp(q, "2") == 0) page = RGB121_2;
        else if(strcasecmp(q, "T") == 0) page = RGB121_T;
        else error("Syntax");
        if(!FbCreated(page)) {
            if(page == RGB121_L) error("Layer not created");
            if(page == RGB121_F) error("Frame buffer not created");
            if(page == RGB121_2) error("Frame buffer 2 not created");
            error("Layer Top not created");
        }
        FbWrite(page);
    } else if((p = checkstring(cmdline, "MERGE"))) {
        /* FRAMEBUFFER MERGE [colour] [,B]: 2 over F, its pixels of 'colour' transparent, into N (the PicoMite's
           RP2350 MERGE); B first waits for the frame's end */
        int colour = 0, words;
        if(!FbCreated(RGB121_F)) error("Frame buffer not created");
        if(!FbCreated(RGB121_2)) error("Frame buffer 2 not created");
        getargs(&p, 3, ",");
        if(argc >= 1 && *argv[0] && VideoColour == 4) colour = getint(argv[0], 0, 15);
        if(argc == 3) {
            if(!checkstring(argv[2], "B")) error("Syntax");
            WaitFrame();
        }
        uint32_t *d = (uint32_t *)PageTable[RGB121_N].address, *f = (uint32_t *)PageTable[RGB121_F].address;
        uint32_t *s = (uint32_t *)PageTable[RGB121_2].address, t = (uint32_t)colour * 0x11111111u;
        words = (Stride(RGB121_N) * PageTable[RGB121_N].ymax + 3) >> 2;
        if(VideoColour == 1) for(int i = 0; i < words; i++) d[i] = f[i] | s[i];
        else for(int i = 0; i < words; i++) d[i] = Over(f[i], s[i], t);
    } else if((p = checkstring(cmdline, "WAIT"))) {
        WaitFrame();
    } else if((p = checkstring(cmdline, "COPY"))) {
        int s, d = RGB121_N;
        getargs(&p, 5, ",");
        if(!(argc == 3 || argc == 5)) error("Syntax");
        if((s = Rgb121Letter(argv[0])) < 0 || (d = Rgb121Letter(argv[2])) < 0) error("Syntax");
        if(argc == 5) {
            if(!checkstring(argv[4], "B")) error("Syntax");
            WaitFrame();
        }
        if(s == d) error("Buffer not created");
        mycopy(PageTable[d].address, PageTable[s].address, Stride(d) * PageTable[d].ymax);
    } else error("Syntax");
}

/* BLIT FRAMEBUFFER from, to, x1, y1, x2, y2, w, h [, colour]: a rectangle of one framebuffer into another, a pixel of
   'colour' (0-15) left alone (the PicoMite's, its blit121) */
void Rgb121BlitFramebuffer(char *p) {
    int s, d = RGB121_N, x1, y1, x2, y2, w, h, blank = -1;
    if(VideoColour != 4) error("Invalid for this screen mode");     // (the PicoMite's MODE 1: "Not available in mode 1")
    getargs(&p, 17, ",");
    if(argc < 15) error("Syntax");
    if((s = Rgb121Letter(argv[0])) < 0 || (d = Rgb121Letter(argv[2])) < 0) error("Syntax");
    if(s == d) error("Same framebuffer");
    x1 = getinteger(argv[4]);
    y1 = getinteger(argv[6]);
    x2 = getinteger(argv[8]);
    y2 = getinteger(argv[10]);
    w = getinteger(argv[12]);
    h = getinteger(argv[14]);
    if(argc == 17) blank = getint(argv[16], -1, 15);
    if(x1 < 0 || y1 < 0 || x1 + w > PageTable[s].xmax || y1 + h > PageTable[s].ymax) error("Number out of bounds");
    if(x2 < 0) { x1 -= x2; w += x2; x2 = 0; }                       // (clipped to the destination, as blit121)
    if(y2 < 0) { y1 -= y2; h += y2; y2 = 0; }
    if(x2 + w > PageTable[d].xmax) w = PageTable[d].xmax - x2;
    if(y2 + h > PageTable[d].ymax) h = PageTable[d].ymax - y2;
    for(int y = 0; y < h; y++) Rgb121Row(Row(d, y2 + y), x2, Row(s, y1 + y), x1, w, blank);
}

/* BLIT RESIZE src, dst, sx, sy, sw, sh, dx, dy, dw, dh [, transparent]: the source rectangle scaled into the
   destination's by nearest neighbour, as the PicoMite's (its Blit.c, MODE 2 and 3: here MODE n,4). The source must be
   inside its buffer (error 21); the destination is clipped; a pixel of the transparent colour (0-15) is not drawn. When
   one buffer is both and the rectangles overlap the source is copied first. */
void Rgb121BlitResize(char *p) {
    int s, d = RGB121_N, sx, sy, sw, sh, dx, dy, dw, dh, t = -1, x0, y0, x1, y1, ss, ox = 0;
    uint8_t *copy = NULL;
    if(VideoColour != 4) error("Invalid for this screen mode");     // (as BLIT FRAMEBUFFER: the PicoMite's is 4-bit only)
    getargs(&p, 21, ",");
    if(!(argc == 19 || argc == 21)) error("Syntax");
    if((s = Rgb121Letter(argv[0])) < 0 || (d = Rgb121Letter(argv[2])) < 0) error("Syntax");
    sx = getinteger(argv[4]);
    sy = getinteger(argv[6]);
    sw = getinteger(argv[8]);
    sh = getinteger(argv[10]);
    dx = getinteger(argv[12]);
    dy = getinteger(argv[14]);
    dw = getinteger(argv[16]);
    dh = getinteger(argv[18]);
    if(argc == 21) t = getint(argv[20], -1, 15);
    if(sw < 1 || sh < 1 || dw < 1 || dh < 1) return;
    if(sx < 0 || sy < 0 || sx + sw > PageTable[s].xmax || sy + sh > PageTable[s].ymax) StandardError(21);
    x0 = dx < 0 ? 0 : dx;
    y0 = dy < 0 ? 0 : dy;
    x1 = dx + dw > PageTable[d].xmax ? PageTable[d].xmax : dx + dw;
    y1 = dy + dh > PageTable[d].ymax ? PageTable[d].ymax : dy + dh;
    if(x0 >= x1 || y0 >= y1) return;
    ss = (sw + 1) >> 1;
    if(s == d && x0 < sx + sw && x1 > sx && y0 < sy + sh && y1 > sy) {
        copy = GetTempMemory(ss * sh);
        for(int y = 0; y < sh; y++)
            for(int x = 0; x < sw; x++) Put4(copy + y * ss, x, Get4(Row(s, sy + y), sx + x));
    } else ox = sx;
    int64_t ystep = ((int64_t)sh << 16) / dh, yfp = (((int64_t)(y0 - dy) * sh) << 16) / dh;
    int64_t xstep = ((int64_t)sw << 16) / dw, xfp0 = (((int64_t)(x0 - dx) * sw) << 16) / dw;
    for(int y = y0; y < y1; y++, yfp += ystep) {
        int py = (int)(yfp >> 16);
        if(py >= sh) py = sh - 1;
        const uint8_t *srow = copy ? copy + py * ss : Row(s, sy + py);
        uint8_t *drow = Row(d, y);
        int64_t xfp = xfp0;
        for(int x = x0; x < x1; x++, xfp += xstep) {
            int px = (int)(xfp >> 16), v;
            if(px >= sw) px = sw - 1;
            v = Get4(srow, ox + px);
            if(v != t) Put4(drow, x, v);
        }
    }
}
