/***********************************************************************************************************************
The PicoMite's MODE 1 on the CMM2: MODE n,1 (22 Oct 2026, Peter's option B)

A page of a 1-bit mode holds the PicoMite's MODE 1 pixels as its framebuffer does: a bit a pixel, the leftmost of each
eight in bit 0, each row width / 8 bytes, so a program's PEEKs and POKEs at MM.INFO(WRITEBUFF) act as on the PicoMite.
The colour a pixel shows comes from its tile. The screen is tiles 8 pixels wide and TILE HEIGHT tall (the font's height
when the mode was set), each with a foreground colour, which its set pixels show, and a background colour, which the
others show. TILE sets them, and so does text that lines up with the tiles.

The LTDC shows none of the pages. As in the 4-bit mode (Rgb121.c) it shows an 8-bit display outside the page table,
which a merge writes at the end of each frame in which something was drawn: N with the layer L ORed into it, as the
PicoMite's scanout does, each pixel the CLUT entry of its tile's foreground or background colour. The tile colours are
kept as the PicoMite HDMI's are (RGB555), and each is given a CLUT entry the first time it is used. When all 256 are
taken, entries that no tile uses any more are given out again, and past that a colour gets the nearest entry.

The drawing set (DrawPixel1 and the others, the PicoMite's DrawPixel2 and the others in graphics/Draw.c) works on the
write page and honours OPTION Y AXIS INVERT. A colour that is not black sets a pixel, black clears it. The "fast" buffers
(BLIT READ, SPRITE, PAGE SCROLL) hold a byte a pixel, 1 or 0, so the 8-bit sprite code serves a 1-bit page as it is,
colour 0 the transparent one.

The PicoMite's framebuffers are pages 0 to 4 (N, L, F, 2, T), the 4-bit mode's FRAMEBUFFER code (Rgb121.c) working on
them in this depth too. T can be created and drawn on, but in MODE 1 the PicoMite does not show it, and neither does
this.

The tiles' colours are in AXI SRAM ahead of the display, so the merge reads them quickly. The display goes in AXI SRAM
where it fits and otherwise at the start of SDRAM (the large modes), as an 8-bit mode's page 0 does.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Rgb121.h"
#include "Mono.h"

extern int ModeXres(int mode), ModeYres(int mode);
extern void BlitShowBuff(int bnbr, int x1, int y1, int mode);

uint8_t *MonoTileF, *MonoTileB;
int MonoTileH = 12, MonoTilesX = 80, MonoTilesY = 40;
volatile int MonoDirty;
volatile uint32_t MonoClutNew[8];

#define RoundUp8K(a) (((a) + 8 * 1024 - 1) & ~(8 * 1024 - 1))

static inline int Stride(int page) { return PageTable[page].xmax >> 3; }
static inline uint8_t *Row(int page, int y) { return PageTable[page].address + y * Stride(page); }
static inline void Put1(uint8_t *row, int x, int c) {
    if(c) row[x >> 3] |= 1 << (x & 7);
    else row[x >> 3] &= ~(1 << (x & 7));
}
static inline int Get1(const uint8_t *row, int x) { return (row[x >> 3] >> (x & 7)) & 1; }

// pixels x1 to x2 of a row set (c not 0) or cleared: the ends' bytes masked, the bytes between at once
static void Run1(uint8_t *row, int x1, int x2, int c) {
    int b1 = x1 >> 3, b2 = x2 >> 3;
    uint8_t m1 = 0xFF << (x1 & 7), m2 = 0xFF >> (7 - (x2 & 7));
    if(b1 == b2) m1 &= m2;
    if(c) row[b1] |= m1;
    else row[b1] &= ~m1;
    if(b1 == b2) return;
    if(b2 - b1 > 1) memset(row + b1 + 1, c ? 0xFF : 0, b2 - b1 - 1);
    if(c) row[b2] |= m2;
    else row[b2] &= ~m2;
}

/* w pixels of row s from pixel sx to row d at pixel dx: all of them (t -1), only the set ones (t 0: a pixel of value t
   is left alone) or only the clear ones (t 1). The pixels are taken out first, so d may be s (a row moved along
   itself). */
void MonoRow(uint8_t *d, int dx, const uint8_t *s, int sx, int w, int t) {
    uint8_t v[MONOMAXW / 8 + 1];
    int sh = sx & 7, k;
    if(w <= 0) return;
    if(w > MONOMAXW) w = MONOMAXW;
    s += sx >> 3;
    for(k = 0; 8 * k < w; k++) {                                    // a byte of v, 8 pixels from sx on
        uint32_t b = s[k] >> sh;
        if(sh && 8 * k + 8 - sh < w) b |= s[k + 1] << (8 - sh);
        v[k] = b;
    }
    sh = dx & 7;
    d += dx >> 3;
    for(k = 0; w > 0; k++, w -= 8) {                                // into d: each byte of v over two of d
        uint32_t m = (w >= 8 ? 0xFF : (1u << w) - 1) << sh, b = ((uint32_t)v[k] << sh) & m;
        if(t == 0) { d[k] |= b; if(m >> 8) d[k + 1] |= b >> 8; }
        else if(t == 1) { d[k] &= ~(m & ~b); if(m >> 8) d[k + 1] &= ~((m & ~b) >> 8); }
        else { d[k] = (d[k] & ~m) | b; if(m >> 8) d[k + 1] = (d[k + 1] & ~(m >> 8)) | (b >> 8); }
    }
}

/***********************************************************************************************************************
The mode
***********************************************************************************************************************/
/* pagegen's start for a 1-bit mode: the tiles' colours at 'start' (AXI SRAM) for the least TILE HEIGHT, then the
   display there if it fits, else at *sdram, which then moves past it; the framebuffers none. The pages follow. */
uint32_t MonoLayout(int mode, uint32_t start, uint32_t *sdram) {
    int w = ModeXres(mode), h = ModeYres(mode), tiles;
    uint32_t display;
    Rgb121DisplayScale = ModeRowScale(mode);
    Rgb121Buffers = 0;
    Rgb121Transparent = Rgb121TransparentTop = 0;
    MonoTilesX = w / 8;
    MonoTileH = 12;                                                 // (until MonoStart: the font's height)
    MonoTilesY = (h + MonoTileH - 1) / MonoTileH;
    tiles = MonoTilesX * ((h + MONOMINTILEH - 1) / MONOMINTILEH);
    MonoTileF = (uint8_t *)start;
    MonoTileB = MonoTileF + tiles;
    start += RoundUp8K(2 * tiles);
    display = (uint32_t)w * h * Rgb121DisplayScale;
    if(start + display <= 0x24080000) {
        Rgb121Display = (uint8_t *)start;
        return start + RoundUp8K(display);
    }
    Rgb121Display = (uint8_t *)*sdram;
    *sdram += RoundUp8K(display);
    return start;
}

/***********************************************************************************************************************
The tiles' colours: CLUT entries
***********************************************************************************************************************/
static int ClutUsed;                                                // entries 0 to ClutUsed - 1 have been given colours
static uint32_t MonoRGB[256];                                       // each entry's colour (the CLUT has it at OPTION VGA
                                                                    // OUTPUT's levels: VgaLevel, ltdc.c)
static int Recent[4], RecentN;                                      // the last given out, kept until their tiles are set
static uint32_t CacheRGB[16];
static uint8_t CacheIndex[16];

// MODE n,1: no colour has an entry, each is black (the LTDC is given them all at the frame's end)
void MonoClut(void) {
    for(int i = 0; i < 256; i++) CLUT[i] = MonoRGB[i] = 0;
    for(int i = 0; i < 8; i++) MonoClutNew[i] = 0xFFFFFFFF;
    for(int i = 0; i < 16; i++) CacheRGB[i] = 0xFFFFFFFF;
    ClutUsed = 0;
}

// a colour as the PicoMite HDMI's tiles hold it (its RGB555()), made RGB888 again as its RGB555toRGB888() does; under
// OPTION PICOMITE VGA as its VGA tiles do, one of the 16 RGB121 colours
static uint32_t Colour555(uint32_t c) {
    uint32_t r = (c >> 19) & 31, g = (c >> 11) & 31, b = (c >> 3) & 31;
    if(PicoActive && PicoFlavour() == PICO_VGA) return Rgb121Vga[RGB121(c)];
    return (r << 3 | r >> 2) << 16 | (g << 3 | g >> 2) << 8 | (b << 3 | b >> 2);
}

// an entry no tile uses (and not one just given out), -1 if there is none
static int ClutFree(void) {
    uint32_t used[8] = {0};
    int n = MonoTilesX * MonoTilesY, i;
    for(i = 0; i < n; i++) {
        used[MonoTileF[i] >> 5] |= 1u << (MonoTileF[i] & 31);
        used[MonoTileB[i] >> 5] |= 1u << (MonoTileB[i] & 31);
    }
    for(i = 0; i < 4; i++) used[Recent[i] >> 5] |= 1u << (Recent[i] & 31);
    for(i = 0; i < 256; i++) if(!(used[i >> 5] & (1u << (i & 31)))) return i;
    return -1;
}

static int Nearest(uint32_t rgb) {
    int best = 0, bestd = 0x7FFFFFFF;
    for(int i = 0; i < 256; i++) {
        int dr = (int)((MonoRGB[i] >> 16) & 0xFF) - (int)((rgb >> 16) & 0xFF);
        int dg = (int)((MonoRGB[i] >> 8) & 0xFF) - (int)((rgb >> 8) & 0xFF);
        int db = (int)(MonoRGB[i] & 0xFF) - (int)(rgb & 0xFF);
        int dd = dr * dr + dg * dg + db * db;
        if(dd < bestd) { bestd = dd; best = i; }
    }
    return best;
}

// colour c's CLUT entry, given to it if it has none (written to the LTDC at the frame's end)
static int MonoIndex(int c) {
    uint32_t rgb = Colour555(c);
    int h = ((rgb >> 3) ^ (rgb >> 11) ^ (rgb >> 19)) & 15, i;
    if(CacheRGB[h] == rgb && MonoRGB[CacheIndex[h]] == rgb) i = CacheIndex[h];
    else {
        for(i = 0; i < ClutUsed; i++) if(MonoRGB[i] == rgb) break;
        if(i == ClutUsed) {
            if(ClutUsed < 256) i = ClutUsed++;
            else i = ClutFree();
            if(i < 0) i = Nearest(rgb);
            else {
                MonoRGB[i] = rgb;
                CLUT[i] = VgaLevel(rgb);
                MonoClutNew[i >> 5] |= 1u << (i & 31);
            }
        }
        CacheRGB[h] = rgb;
        CacheIndex[h] = i;
    }
    Recent[RecentN++ & 3] = i;
    return i;
}

// a colour's CLUT entry, for GUI CURSOR's pointer drawn over the display (Rgb121.c)
int MonoColourEntry(int c) {
    return MonoIndex(c);
}

// tiles x1 to x2 across and y1 to y2 down (clipped) given entries f and b (-1: the colour left as it is)
static void Tiles(int x1, int y1, int x2, int y2, int f, int b) {
    if(x1 < 0) x1 = 0;
    if(y1 < 0) y1 = 0;
    if(x2 >= MonoTilesX) x2 = MonoTilesX - 1;
    if(y2 >= MonoTilesY) y2 = MonoTilesY - 1;
    for(int y = y1; y <= y2; y++) {
        if(x2 < x1) break;
        if(f >= 0) memset(MonoTileF + y * MonoTilesX + x1, f, x2 - x1 + 1);
        if(b >= 0) memset(MonoTileB + y * MonoTilesX + x1, b, x2 - x1 + 1);
    }
    MonoDirty = 1;
}

static void TilesAll(int fc, int bc) {
    int f = MonoIndex(fc), b = MonoIndex(bc);
    Tiles(0, 0, MonoTilesX - 1, MonoTilesY - 1, f, b);
}

// the colour (RGB888) pixel x, y shows, set or not: its tile's foreground or background
int MonoTileColour(int x, int y, int set) {
    int t = (y / MonoTileH) * MonoTilesX + (x >> 3);
    return MonoRGB[set ? MonoTileF[t] : MonoTileB[t]];
}

/* the end of setmode, the mode set (and MODE n,1 again): TILE HEIGHT the font's height, no colour with an entry, every
   tile in the default colours, N cleared, as the PicoMite's setmode() */
void MonoStart(void) {
    MonoTileH = gui_font_height < MONOMINTILEH ? MONOMINTILEH : gui_font_height;
    MonoTilesY = (PageTable[RGB121_N].ymax + MonoTileH - 1) / MonoTileH;
    MonoClut();
    memset(PageTable[RGB121_N].address, 0, Stride(RGB121_N) * PageTable[RGB121_N].ymax);
    TilesAll(gui_fcolour, gui_bcolour);
}

// a program's end with a PicoMite mode showing (PicoMode.c): the tiles the font's height and the default colours, the
// pixels kept, as the PicoMite's setmode(mode, false)
void MonoTilesReset(void) {
    MonoTileH = gui_font_height < MONOMINTILEH ? MONOMINTILEH : gui_font_height;
    MonoTilesY = (PageTable[RGB121_N].ymax + MonoTileH - 1) / MonoTileH;
    TilesAll(Option.DefaultFC, Option.DefaultBC);
}

// ClearScreen in a 1-bit mode: N cleared and every tile in the foreground colour on c (the PicoMite's), a framebuffer
// filled as a rectangle is
void MonoClearScreen(int c) {
    if(WritePage != RGB121_N) {
        DrawRectangle(0, 0, PageTable[WritePage].xmax - 1, PageTable[WritePage].ymax - 1, c);
        return;
    }
    memset(PageTable[RGB121_N].address, 0, Stride(RGB121_N) * PageTable[RGB121_N].ymax);
    TilesAll(gui_fcolour, c);
}

/* TILE x, y [, fc] [, bc] [, w] [, h]: the colours of w x h tiles from tile x, y (fc or bc left out: left as it is);
   TILE RESET: every tile in the default colours; TILE HEIGHT h (8 to the screen's height): the tiles h pixels tall,
   the screen cleared. The PicoMite HDMI's (graphics/Draw.c, cmd_tile). */
void cmd_tile(void) {
    char *tp;
    if(VideoColour != 1) error("Invalid for this screen mode");
    if(checkstring(cmdline, "RESET")) {
        TilesAll(Option.DefaultFC, Option.DefaultBC);
    } else if((tp = checkstring(cmdline, "HEIGHT"))) {
        MonoTileH = getint(tp, MONOMINTILEH, PageTable[RGB121_N].ymax);
        MonoTilesY = (PageTable[RGB121_N].ymax + MonoTileH - 1) / MonoTileH;
        TilesAll(gui_fcolour, Option.DefaultBC);
        ClearScreen(Option.DefaultBC);
    } else {
        int x, y, f = -1, b = -1, xlen = 1, ylen = 1;
        getargs(&cmdline, 11, ",");
        if(argc < 5) error("Syntax");
        x = getint(argv[0], 0, MonoTilesX - 1);
        y = getint(argv[2], 0, MonoTilesY - 1);
        if(*argv[4]) f = MonoIndex(getColour(argv[4], 0));
        if(argc >= 7 && *argv[6]) b = MonoIndex(getColour(argv[6], 0));
        if(argc >= 9 && *argv[8]) xlen = getint(argv[8], 1, MonoTilesX - x);
        if(argc >= 11 && *argv[10]) ylen = getint(argv[10], 1, MonoTilesY - y);
        Tiles(x, y, x + xlen - 1, y + ylen - 1, f, b);
    }
}

/***********************************************************************************************************************
The drawing set, on the write page (the read page for reading)
***********************************************************************************************************************/
void DrawPixel1(int x, int y, int c) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax;
    if(optiony) y = maxH - 1 - y;
    if(x < 0 || x >= maxW || y < 0 || y >= maxH) return;
    Put1(Row(WritePage, y), x, MONO1(c));
    MonoDirty = 1;
}

void DrawRectangle1(int x1, int y1, int x2, int y2, int c) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax, t;
    if(optiony) { y1 = maxH - 1 - y1; y2 = maxH - 1 - y2; }
    if((x1 < 0 && x2 < 0) || (y1 < 0 && y2 < 0) || (x1 >= maxW && x2 >= maxW) || (y1 >= maxH && y2 >= maxH)) return;
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    if(x1 < 0) x1 = 0;
    if(x2 >= maxW) x2 = maxW - 1;
    if(y1 < 0) y1 = 0;
    if(y2 >= maxH) y2 = maxH - 1;
    c = MONO1(c);
    for(int y = y1; y <= y2; y++) Run1(Row(WritePage, y), x1, x2, c);
    MonoDirty = 1;
}

/* a bitmap (a character): its set bits in fc, the others in bc (-1: left alone). In line with the tiles, a whole number
   of them, it also gives them fc and bc (bc -1: their background left as it is; the PicoMite makes it white there). A
   black character on a colour is drawn as the clear pixels of set ones where it is not in line with them. The
   PicoMite's DrawBitmap2. */
void DrawBitmap1(int x1, int y1, int width, int height, int scale, int fc, int bc, unsigned char *bitmap) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax;
    if(optiony) y1 = maxH - 1 - y1;                                 // (as DrawBitmap8: the text is not turned over)
    if(x1 >= maxW || y1 >= maxH || x1 + width * scale < 0 || y1 + height * scale < 0) return;
    if(fc != -1) fc &= 0xFFFFFF;
    if(bc != -1) bc &= 0xFFFFFF;
    int tilematch = (x1 & 7) == 0 && y1 % MonoTileH == 0 && (width * scale) % 8 == 0 && (height * scale) % MonoTileH == 0;
    int inverse = fc == 0 && bc > 0 && !tilematch;
    int on = fc != 0 || bc > 0;                                     // a set bit's pixel (black on a colour in line: set,
    if(tilematch) {                                                 // in the tile's foreground of black)
        int f = MonoIndex(fc), b = bc >= 0 ? MonoIndex(bc) : -1;
        Tiles(x1 >> 3, y1 / MonoTileH, ((x1 + width * scale) >> 3) - 1, (y1 + height * scale) / MonoTileH - 1, f, b);
    }
    for(int i = 0; i < height; i++) {
        for(int j = 0; j < scale; j++) {
            int y = y1 + i * scale + j;
            if(y < 0) continue;
            if(y >= maxH) break;
            uint8_t *row = Row(WritePage, y);
            for(int k = 0; k < width; k++) {
                int set = (bitmap[((i * width) + k) / 8] >> (((height * width) - ((i * width) + k) - 1) % 8)) & 1;
                if(!inverse && !set && bc < 0) continue;
                int v = inverse ? !set : (set ? on : 0);
                for(int m = 0; m < scale; m++) {
                    int x = x1 + k * scale + m;
                    if(x >= 0 && x < maxW) Put1(row, x, v);
                }
            }
        }
    }
    MonoDirty = 1;
}

/* a buffer of colours (DrawBuffer8's forms: BGR, RGB with skip & 2, 4 bytes with skip & 1, RGB565 with skip & 8, a pixel
   under skip & 0xF0's alpha left alone with skip & 4) onto the page: set where a component is 0x40 or more (the
   PicoMite's DrawBuffer2) */
void DrawBuffer1(int x1, int y1, int x2, int y2, char *p, int skip) {
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
                uint32_t v = c.argb[0];
                c.rgb = ((v & 0xF800) << 8) | ((v & 0x7E0) << 5) | ((v & 0x1F) << 3);
            } else {
                c.rgbbytes[0] = *p++;
                c.rgbbytes[1] = *p++;
                c.rgbbytes[2] = *p++;
                if(skip & 1) p++;
            }
            if(!(skip & 8) && (skip & 4) && (uint8_t)c.rgbbytes[3] < deftrans) continue;
            Put1(row, x, (c.rgb & 0xC0C0C0) != 0);
        }
    }
    MonoDirty = 1;
}

// the page's pixels as the colours they show, three bytes each (blue, green, red): the PicoMite's ReadBuffer2
void ReadBuffer1(int x1, int y1, int x2, int y2, char *p) {
    int maxW = PageTable[ReadPage].xmax, maxH = PageTable[ReadPage].ymax, t;
    if(optiony) { y1 = maxH - 1 - y1; y2 = maxH - 1 - y2; }
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(int y = y1; y <= y2; y++) {
        routinechecks(1);
        for(int x = x1; x <= x2; x++) {
            if(x >= 0 && x < maxW && y >= 0 && y < maxH) {
                int c = MonoTileColour(x, y, Get1(Row(ReadPage, y), x));
                *p++ = c & 0xFF;
                *p++ = (c >> 8) & 0xFF;
                *p++ = (c >> 16) & 0xFF;
            } else p += 3;
        }
    }
}

// the "fast" buffers: a byte a pixel, 1 or 0
void ReadBufferFast1(int x1, int y1, int x2, int y2, char *p) {
    int maxW = PageTable[ReadPage].xmax, maxH = PageTable[ReadPage].ymax, t;
    if(optiony) { y1 = maxH - 1 - y1; y2 = maxH - 1 - y2; }
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(int y = y1; y <= y2; y++) {
        routinechecks(1);
        const uint8_t *row = (y >= 0 && y < maxH) ? Row(ReadPage, y) : NULL;
        for(int x = x1; x <= x2; x++, p++) if(row != NULL && x >= 0 && x < maxW) *p = Get1(row, x);
    }
}

void DrawBufferFast1(int x1, int y1, int x2, int y2, char *p) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax, t;
    if(optiony) { y1 = maxH - 1 - y1; y2 = maxH - 1 - y2; }
    if(x2 <= x1) { t = x1; x1 = x2; x2 = t; }
    if(y2 <= y1) { t = y1; y1 = y2; y2 = t; }
    for(int y = y1; y <= y2; y++) {
        routinechecks(1);
        uint8_t *row = (y >= 0 && y < maxH) ? Row(WritePage, y) : NULL;
        for(int x = x1; x <= x2; x++, p++) if(row != NULL && x >= 0 && x < maxW) Put1(row, x, *p != 0);
    }
    MonoDirty = 1;
}

// the page moved up (lines > 0) or down, what is uncovered cleared if blank (PAGE SCROLL, SPRITE SCROLL)
void ScrollBuff1V(int lines, int blank) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax, n = Stride(WritePage), y;
    maxH = ConsoleVRes();                                       // (the whole rows above the on-screen keyboard)
    if(lines > 0) {
        for(y = 0; y < maxH - lines - (ShortScroll ? gui_font_height : 0); y++) mycopy(Row(WritePage, y), Row(WritePage, y + lines), n);
        if(blank) {
            if(ShortScroll) DrawRectangle(0, maxH - lines - gui_font_height, maxW - 1, maxH - gui_font_height - 1, 0);
            else DrawRectangle(0, maxH - lines, maxW - 1, maxH - 1, 0);
        }
    } else if(lines < 0) {
        lines = -lines;
        for(y = maxH - 1; y >= lines; y--) mycopy(Row(WritePage, y), Row(WritePage, y - lines), n);
        if(blank) DrawRectangle(0, 0, maxW - 1, lines - 1, 0);
    }
    MonoDirty = 1;
}

// the console's scroll (ScrollLCD): the pixels, and the tiles' colours with them by whole tiles (the PicoMite's ScrollLCD2)
void ScrollLCD1(int lines, int blank) {
    int n = MonoTilesX, rows = MonoTilesY;
    rows -= (PageTable[WritePage].ymax * OptionVResreserved / 100) / MonoTileH;   // (the on-screen keyboard's tiles stay)
    if(lines != 0 && lines % MonoTileH == 0) {
        int offset = (lines > 0 ? lines : -lines) / MonoTileH;
        if(lines > 0) {
            for(int y = 0; y < rows - offset; y++) {
                memmove(MonoTileF + y * n, MonoTileF + (y + offset) * n, n);
                memmove(MonoTileB + y * n, MonoTileB + (y + offset) * n, n);
            }
        } else {
            for(int y = rows - 1; y >= offset; y--) {
                memmove(MonoTileF + y * n, MonoTileF + (y - offset) * n, n);
                memmove(MonoTileB + y * n, MonoTileB + (y - offset) * n, n);
            }
        }
    }
    ScrollBuff1V(lines, blank);
}

// the page moved right (pixels > 0) or left (what is uncovered is left as it was)
void ScrollBuff1H(int pixels) {
    int maxW = PageTable[WritePage].xmax, maxH = PageTable[WritePage].ymax;
    if(!pixels) return;
    for(int y = 0; y < maxH; y++) {
        uint8_t *row = Row(WritePage, y);
        if(pixels > 0) MonoRow(row, pixels, row, 0, maxW - pixels, -1);
        else MonoRow(row, 0, row, -pixels, maxW + pixels, -1);
    }
    MonoDirty = 1;
}

void MonoPointers(void) {
    DrawPixel = DrawPixel1;
    DrawRectangle = DrawRectangle1;
    DrawBitmap = DrawBitmap1;
    DrawBuffer = DrawBuffer1;
    ReadBuffer = ReadBuffer1;
    ScrollLCD = ScrollLCD1;
    ScrollBufferV = ScrollBuff1V;
    ScrollBufferH = ScrollBuff1H;
    DrawBufferFast = DrawBufferFast1;
    ReadBufferFast = ReadBufferFast1;
    BlitShowBuffer = BlitShowBuff;                                  // (a byte a pixel, as the 8-bit modes' buffers)
    HAL_NVIC_SetPriority(CEC_IRQn, IRQ_PRI(6), 0);                           // the merge: below everything (Rgb121.c)
    HAL_NVIC_EnableIRQ(CEC_IRQn);
}

// DrawPixelFast, ReadPixelFast and DrawHLineFast's pixels in a 1-bit mode (the page's coordinates, c 1 or 0)
void MonoSet(int x, int y, int c) { Put1(Row(WritePage, y), x, c & 1); MonoDirty = 1; }
int MonoGet(int x, int y) { return Get1(Row(ReadPage, y), x); }
void MonoHLine(int x1, int x2, int y, int c) { Run1(Row(WritePage, y), x1, x2, c & 1); MonoDirty = 1; }

// PAGE COPY (PageCopy): the whole page, or its set pixels ORed in
void MonoPageCopy(int from, int to, int transparent) {
    int w = PageTable[from].xmax < PageTable[to].xmax ? PageTable[from].xmax : PageTable[to].xmax;
    int h = PageTable[from].ymax < PageTable[to].ymax ? PageTable[from].ymax : PageTable[to].ymax;
    if(from == to) return;
    if(!transparent && PageTable[from].xmax == PageTable[to].xmax) mycopy(PageTable[to].address, PageTable[from].address, Stride(from) * h);
    else for(int y = 0; y < h; y++) {
        MonoRow(Row(to, y), 0, Row(from, y), 0, w, transparent ? 0 : -1);
        if(!(y & 15)) routinechecks(1);
    }
    MonoDirty = 1;
}

/* BLIT x1, y1, x2, y2, w, h [, page] [, orientation] (cmd_blit, the rectangle clipped to both pages): from the read
   page to the write page, mirrored left to right with orientation bit 0, top to bottom with bit 1, the clear pixels
   left alone with bit 2. In line with the tiles (and not turned) the tiles' colours go with it, as the PicoMite's. */
void MonoBlit(int x1, int y1, int x2, int y2, int w, int h, int flip) {
    int t = (flip & 4) ? 0 : -1, y;
    if(optiony) {
        y1 = PageTable[ReadPage].ymax - y1 - h;
        y2 = PageTable[WritePage].ymax - y2 - h;
    }
    if(!(flip & 3)) {
        if(ReadPage == WritePage && y2 > y1) {                      // (down the page: from the bottom row)
            for(y = h - 1; y >= 0; y--) MonoRow(Row(WritePage, y2 + y), x2, Row(ReadPage, y1 + y), x1, w, t);
        } else {
            for(y = 0; y < h; y++) MonoRow(Row(WritePage, y2 + y), x2, Row(ReadPage, y1 + y), x1, w, t);
        }
        int th = MonoTileH;
        if(!((x1 | x2 | w) & 7) && y1 % th == 0 && y2 % th == 0 && h % th == 0) {
            int tx1 = x1 >> 3, tx2 = x2 >> 3, ty1 = y1 / th, ty2 = y2 / th, xc = w >> 3, yc = h / th, n = MonoTilesX;
            for(int k = 0; k < yc; k++) {                           // (in the order the rows were moved)
                int yy = (ty2 > ty1) ? yc - 1 - k : k;
                memmove(MonoTileF + (ty2 + yy) * n + tx2, MonoTileF + (ty1 + yy) * n + tx1, xc);
                memmove(MonoTileB + (ty2 + yy) * n + tx2, MonoTileB + (ty1 + yy) * n + tx1, xc);
            }
        }
        MonoDirty = 1;
        return;
    }
    uint8_t *buf = GetTempMemory(w * h);                            // (turned: the rectangle is read first)
    for(y = 0; y < h; y++) {
        const uint8_t *s = Row(ReadPage, y1 + y);
        for(int x = 0; x < w; x++) buf[y * w + x] = Get1(s, x1 + x);
    }
    for(y = 0; y < h; y++) {
        const uint8_t *s = buf + ((flip & 2) ? h - 1 - y : y) * w;
        uint8_t *d = Row(WritePage, y2 + y);
        for(int x = 0; x < w; x++) {
            int v = s[(flip & 1) ? w - 1 - x : x];
            if(v != t) Put1(d, x2 + x, v);
        }
    }
    MonoDirty = 1;
}
