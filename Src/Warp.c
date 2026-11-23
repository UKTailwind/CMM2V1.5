/***********************************************************************************************************************
FRAMEBUFFER WARPH and WARPV, and PAGE WARPH and WARPV: each row (WARPH) or column (WARPV) of one buffer copied to another,
moved by its own offset from an integer array. For racing roads, parallax, wobble and column effects. Peter's design of
23 Nov 2026 (the PicoMite's docs/FRAMEBUFFER_WARP_Design.md), and the PicoMite's graphics/Warp.c, whose core is here.

    FRAMEBUFFER WARPH from, to, xoff%() [, step] [, first] [, phase] [, edge] [, transparent] [, rows%()]
    FRAMEBUFFER WARPV from, to, yoff%() [, step] [, first] [, phase] [, edge] [, transparent]
    PAGE WARPH from TO to, xoff%() [, step] [, first] [, phase] [, edge] [, transparent] [, rows%()]
    PAGE WARPV from TO to, yoff%() [, step] [, first] [, phase] [, edge] [, transparent]

Band b is the step rows (WARPH) or columns (WARPV) of the destination from first + b * step, the last cut at its edge,
and it takes element (b + phase) MOD n of the array (n elements, n bands). A destination pixel (x, y) is the source's
(x - off, sy) in WARPH, sy being y or from rows%(), and (x, y + off) in WARPV: positive moves right, and up, as PAGE
SCROLL does. With no edge the source wraps round; edge -1 leaves the pixels nothing moves onto, and a colour fills them
(the pixel BOX would draw). A source pixel of the transparent value is not written.

The core is the PicoMite's, checked there against a pixel model, with what the CMM2 adds:
- 32-bit pixels, compared with the transparent colour without their alpha, as PIXEL( and FILL read them;
- in the 12-bit modes the transparent pixels are those whose alpha is 0, whatever colour the argument gives (Peter,
  23 Nov: the argument turns it on; 0 will do);
- the pages whose rows are stored twice (PageTable expand): the first copy read, both written;
- a source and destination of different sizes (the world framebuffer F of the 8, 12, 16 and 32-bit modes can be larger
  than the screen, and PAGE RESIZE makes pages smaller): the offsets wrap at the source's width or height, and the
  columns (WARPV) or rows (WARPH without rows%()) used must all be in the source;
- routinechecks between rows, as the page copies call it (a 32-bit 800x600 page is 1.9 MB).

The buffers: in the 4-bit modes the PicoMite's letters N, L, F, 2 and T (pages 0 to 4, Rgb121.h), one not created
refused with FRAMEBUFFER WRITE's message; in the 8, 12, 16 and 32-bit modes N is page 0 and F the world framebuffer
(WPN), as FRAMEBUFFER WRITE takes them there (Peter: not Rgb121Letter, which maps F to page 2 for TILEMAP and the image
slots). The PAGE form takes page numbers in every mode. The 1-bit modes are refused, as BLIT FRAMEBUFFER is.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Rgb121.h"

extern volatile uint8_t pagesetdone;

#define WARP_CODE __attribute__((section(".extra"), noinline)) // (the end of .text: nothing before it moves)
#define WARP_WRAP (-2)  // edge: wrap round (the default)
#define WARP_LEAVE (-1) // edge: leave the destination's pixel

typedef struct
{
    uint8_t *dst;
    const uint8_t *src;
    int sw, sh, ss;        // the source's width and height in pixels, and the bytes from one of its rows to the next
    int dw, dh, ds;        // the destination's
    int dup;               // the destination's rows stored twice: a row's bytes, copied to the copy after it (0: not)
    int bpp;               // bits a pixel: 4, 8, 16 (the 12-bit modes too) or 32
    int vertical;          // WARPV
    int n, step, first;    // the bands
    const int *boff;       // each band's offset: 0 to size-1 when wrapping, else clamped (warp_bands)
    const int *brow;       // WARPH: each band's first source row, -1 to leave it; NULL for none
    int edge, trans;       // WARP_WRAP, WARP_LEAVE or a pixel value; -1, or what a pixel ANDed with tmask must be
    int tmask;             // the bits of a pixel compared with trans: all of them, the RGB of a 32-bit pixel, the
                           // alpha of a 12-bit one (Peter, 23 Nov: a pixel of alpha 0 is the transparent one)
} warp_t;

// the 4-bit pixels are RGB121, two to a byte, the left one in the low nibble. Always inlined: at -Os a call for
// each pixel cost more than the pixel (on the PicoMite, WARPV with step 1 took 37 ms in MODE 2)
#define WARP_INLINE static inline __attribute__((always_inline))
WARP_INLINE int warp_get(const uint8_t *row, int x, int bpp)
{
    if (bpp == 4)
        return (row[x >> 1] >> ((x & 1) << 2)) & 0xF;
    if (bpp == 8)
        return row[x];
    if (bpp == 16)
        return ((const uint16_t *)row)[x];
    return (int)((const uint32_t *)row)[x];
}

WARP_INLINE void warp_put(uint8_t *row, int x, int bpp, int c)
{
    if (bpp == 4)
    {
        uint8_t *p = row + (x >> 1);
        *p = (x & 1) ? (uint8_t)((*p & 0x0F) | (c << 4)) : (uint8_t)((*p & 0xF0) | c);
    }
    else if (bpp == 8)
        row[x] = (uint8_t)c;
    else if (bpp == 16)
        ((uint16_t *)row)[x] = (uint16_t)c;
    else
        ((uint32_t *)row)[x] = (uint32_t)c;
}

// a source row when wrapping: y + an offset of 0 to sh-1, so below 2 * sh unless the destination is the taller
WARP_INLINE int warp_wrapy(int sy, int sh)
{
    if (sy >= sh)
    {
        sy -= sh;
        if (sy >= sh)
            sy %= sh;
    }
    return sy;
}

// a destination row finished: its copy where the page stores each row twice, and the audio and USB seen to now and then
WARP_INLINE void warp_done(const warp_t *a, uint8_t *d, int y)
{
    if (a->dup)
        memcpy(d + a->dup, d, a->dup);
    if ((y & 15) == 15)
        routinechecks(1);
}

// w pixels from pixel sx of a source row to pixel dx of a destination row, those whose bits m are t (-1: none) not
// written (m: the 16 and 32-bit depths; a 4 or 8-bit pixel is compared whole)
static void WARP_CODE warp_seg(uint8_t *d, int dx, const uint8_t *s, int sx, int w, int bpp, int t, int m)
{
    if (w <= 0)
        return;
    if (bpp == 4)
    {
        int c;
        if (dx & 1) // an odd start: the destination's high nibble alone
        {
            if ((c = warp_get(s, sx, 4)) != t)
                warp_put(d, dx, 4, c);
            dx++, sx++, w--;
        }
        uint8_t *q = d + (dx >> 1); // dx is even now: whole destination bytes
        const uint8_t *r = s + (sx >> 1);
        int n = w >> 1, odd = sx & 1;
        if (t < 0 && !odd)
            memcpy(q, r, n);
        else if (t < 0) // an odd source start: each byte from two (its pixel is the high nibble)
            for (int i = 0; i < n; i++)
                q[i] = (uint8_t)((r[i] >> 4) | (r[i + 1] << 4));
        else
            for (int i = 0; i < n; i++)
            {
                // as above, with a transparent nibble masked out, so the byte is merged whole
                uint8_t v = odd ? (uint8_t)((r[i] >> 4) | (r[i + 1] << 4)) : r[i], m = 0xFF;
                if ((v & 0x0F) == t)
                    m = 0xF0;
                if ((v >> 4) == t)
                    m &= 0x0F;
                q[i] = (uint8_t)((q[i] & ~m) | (v & m));
            }
        if ((w & 1) && (c = warp_get(s, sx + w - 1, 4)) != t) // an odd end: the low nibble alone
            warp_put(d, dx + w - 1, 4, c);
        return;
    }
    if (t < 0)
        memcpy(d + dx * (bpp >> 3), s + sx * (bpp >> 3), w * (bpp >> 3));
    else if (bpp == 8)
    {
        const uint8_t *p = s + sx;
        uint8_t *q = d + dx;
        for (int i = 0; i < w; i++)
            if (p[i] != t)
                q[i] = p[i];
    }
    else if (bpp == 16)
    {
        const uint16_t *p = (const uint16_t *)s + sx;
        uint16_t *q = (uint16_t *)d + dx;
        for (int i = 0; i < w; i++)
            if ((p[i] & m) != t)
                q[i] = p[i];
    }
    else
    {
        const uint32_t *p = (const uint32_t *)s + sx;
        uint32_t *q = (uint32_t *)d + dx;
        for (int i = 0; i < w; i++)
            if ((int)(p[i] & m) != t)
                q[i] = p[i];
    }
}

/* WARPV: row by row from the top, so the destination is written down the screen. A band's pixels keep their x, so
   a 4-bit band moves whole bytes between its ends. Made once for each depth (bpp is a constant in each): a call or a
   memcpy for each band of each row cost more than its pixels (on the PicoMite step 1 took 37 ms in MODE 2, the
   general code 14 ms at step 2) */
WARP_INLINE void warp_v(const warp_t *a, int bpp)
{
    // in locals: a store through a byte pointer could change *a, so the fields would be read again for every pixel
    const uint8_t *src = a->src;
    const int *boff = a->boff;
    int sh = a->sh, ss = a->ss, dh = a->dh, ds = a->ds, wd = a->dw, n = a->n, step = a->step, edge = a->edge,
        trans = a->trans, tmask = a->tmask;
    if (bpp == 4 && edge == WARP_WRAP && trans < 0 && !((a->first | step) & 1))
    {
        // the usual 4-bit wave: an even first column and step, so each band is whole bytes of one source row
        for (int y = 0; y < dh; y++)
        {
            uint8_t *d = a->dst + y * ds;
            for (int b = 0, x = a->first; b < n; b++, x += step)
            {
                int k = x >> 1, e = (x + step > wd ? wd : x + step) >> 1;
                const uint8_t *s = src + warp_wrapy(y + boff[b], sh) * ss;
                for (; k < e; k++)
                    d[k] = s[k];
            }
            warp_done(a, d, y);
        }
        return;
    }
    for (int y = 0; y < dh; y++)
    {
        uint8_t *d = a->dst + y * ds;
        for (int b = 0, x = a->first; b < n; b++, x += step)
        {
            int e = x + step > wd ? wd : x + step, sy = y + boff[b], i = x, c;
            if (edge == WARP_WRAP)
                sy = warp_wrapy(sy, sh);
            else if (sy < 0 || sy >= sh)
            {
                if (edge >= 0)
                    for (; i < e; i++)
                        warp_put(d, i, bpp, edge);
                continue;
            }
            const uint8_t *s = src + sy * ss;
            if (bpp == 4 && trans < 0)
            {
                if (i & 1) // an odd start: the high nibble alone
                {
                    warp_put(d, i, 4, warp_get(s, i, 4));
                    i++;
                }
                for (; i + 1 < e; i += 2)
                    d[i >> 1] = s[i >> 1];
                if (i < e) // an odd end: the low nibble alone
                    warp_put(d, i, 4, warp_get(s, i, 4));
            }
            else
                for (; i < e; i++)
                    if (((c = warp_get(s, i, bpp)) & tmask) != trans)
                        warp_put(d, i, bpp, c);
        }
        warp_done(a, d, y);
    }
}

/* WARPV with step 1, a column a band, where warp_v's band overhead is paid for each pixel. In the 4-bit wrapping
   copy (the usual wave) each destination byte is made from two columns at once */
WARP_INLINE void warp_v1(const warp_t *a, int bpp)
{
    const uint8_t *src = a->src; // (in locals, as in warp_v)
    const int *boff = a->boff;
    int sh = a->sh, ss = a->ss, dh = a->dh, ds = a->ds, n = a->n, edge = a->edge, trans = a->trans, tmask = a->tmask;
    for (int y = 0; y < dh; y++)
    {
        uint8_t *d = a->dst + y * ds;
        int b = 0, x = a->first;
        if (bpp == 4 && edge == WARP_WRAP && trans < 0)
        {
            if (x & 1) // an odd first column: the high nibble alone
            {
                int sy = warp_wrapy(y + boff[b++], sh);
                warp_put(d, x, 4, warp_get(src + sy * ss, x, 4));
                x++;
            }
            for (; b + 1 < n; b += 2, x += 2)
            {
                int s0 = warp_wrapy(y + boff[b], sh), s1 = warp_wrapy(y + boff[b + 1], sh);
                d[x >> 1] = (uint8_t)((src[s0 * ss + (x >> 1)] & 0x0F) | (src[s1 * ss + (x >> 1)] & 0xF0));
            }
        }
        for (; b < n; b++, x++) // the general case, and a 4-bit odd last column
        {
            int sy = y + boff[b], c;
            if (edge == WARP_WRAP)
                sy = warp_wrapy(sy, sh);
            else if (sy < 0 || sy >= sh)
            {
                if (edge >= 0)
                    warp_put(d, x, bpp, edge);
                continue;
            }
            if (((c = warp_get(src + sy * ss, x, bpp)) & tmask) != trans)
                warp_put(d, x, bpp, c);
        }
        warp_done(a, d, y);
    }
}

static void WARP_CODE warp_fill(uint8_t *row, int x, int w, int bpp, int c)
{
    if (bpp == 8 && w > 0)
        memset(row + x, c, w);
    else
        for (int i = 0; i < w; i++)
            warp_put(row, x + i, bpp, c);
}

/* one row of WARPH: the destination's pixel x is the source's x - o. Wrapping, o is 0 to the source's width - 1 and
   the source is taken round as often as the destination's width needs (twice at most when they are the same width,
   as on the PicoMite); with an edge o is -(source width) to the destination's width, and what no source pixel moves
   onto is left or filled */
static void WARP_CODE warp_row(const warp_t *a, uint8_t *d, const uint8_t *s, int o)
{
    int w = a->dw, sw = a->sw;
    if (a->edge == WARP_WRAP)
    {
        for (int x = 0, sx = o ? sw - o : 0, k; x < w; x += k, sx = 0)
        {
            k = sw - sx;
            if (k > w - x)
                k = w - x;
            warp_seg(d, x, s, sx, k, a->bpp, a->trans, a->tmask);
        }
        return;
    }
    int x0 = o > 0 ? o : 0, x1 = o + sw < w ? o + sw : w; // the destination's pixels that have a source pixel
    if (x1 <= x0)
        x0 = x1 = w; // (none: all of it is the edge's)
    else
        warp_seg(d, x0, s, x0 - o, x1 - x0, a->bpp, a->trans, a->tmask);
    if (a->edge >= 0)
    {
        warp_fill(d, 0, x0 < w ? x0 : w, a->bpp, a->edge);
        warp_fill(d, x1, w - x1, a->bpp, a->edge);
    }
}

static void WARP_CODE warp_run(const warp_t *a)
{
    if (!a->vertical)
    {
        int rows = 0;
        for (int b = 0; b < a->n; b++)
        {
            int y0 = a->first + b * a->step, y1 = y0 + a->step, r = a->brow ? a->brow[b] : 0;
            if (a->brow && r < 0)
                continue; // rows%() -1: the band is left as it is
            if (y1 > a->dh)
                y1 = a->dh;
            for (int y = y0; y < y1; y++)
            {
                int sy = y;
                if (a->brow) // the band's next source rows, round to the top
                    sy = warp_wrapy(r + y - y0, a->sh);
                uint8_t *d = a->dst + y * a->ds;
                warp_row(a, d, a->src + sy * a->ss, a->boff[b]);
                warp_done(a, d, rows++);
            }
        }
        return;
    }
    if (a->step == 1)
    {
        if (a->bpp == 4)
            warp_v1(a, 4);
        else if (a->bpp == 8)
            warp_v1(a, 8);
        else if (a->bpp == 16)
            warp_v1(a, 16);
        else
            warp_v1(a, 32);
    }
    else if (a->bpp == 4)
        warp_v(a, 4);
    else if (a->bpp == 8)
        warp_v(a, 8);
    else if (a->bpp == 16)
        warp_v(a, 16);
    else
        warp_v(a, 32);
}

/* the band offsets and rows from the arrays: band b takes element (b + phase) MOD n; an offset is reduced to 0 to
   size-1 when wrapping and clamped to lo..hi otherwise (anything further moves the whole band off: -size..size on
   the PicoMite, whose buffers are all the screen's size) */
static void WARP_CODE warp_bands(int *boff, int *brow, const int64_t *off, const int64_t *rows, int n, int64_t phase,
                                 int size, int lo, int hi, int wrap)
{
    int p = (int)(phase % n);
    if (p < 0)
        p += n;
    for (int b = 0, e = p; b < n; b++, e = (e + 1 == n ? 0 : e + 1))
    {
        int64_t o = off[e];
        if (wrap)
        {
            o %= size;
            if (o < 0)
                o += size;
        }
        else if (o > hi)
            o = hi;
        else if (o < lo)
            o = lo;
        boff[b] = (int)o;
        if (brow)
            brow[b] = (int)rows[e];
    }
}

// a FRAMEBUFFER letter as its page, refused if it was not created (FRAMEBUFFER WRITE's messages)
static int WARP_CODE warp_letter(char *p)
{
    if (VideoColour == 4)
    {
        int page = RGB121_N;
        const char *msg = "";
        if (checkword(p, "N"))
            return RGB121_N;
        if (checkword(p, "L"))
            page = RGB121_L, msg = "Layer not created";
        else if (checkword(p, "F"))
            page = RGB121_F, msg = "Frame buffer not created";
        else if (checkword(p, "2"))
            page = RGB121_2, msg = "Frame buffer 2 not created";
        else if (checkword(p, "T"))
            page = RGB121_T, msg = "Layer Top not created";
        else
            SyntaxError();
        if (!(Rgb121Buffers & (1 << page)))
            error((char *)msg);
        return page;
    }
    if (checkword(p, "N")) // the display: FRAMEBUFFER WRITE N
        return 0;
    if (checkword(p, "F")) // the world framebuffer
    {
        if (PageTable[WPN].address == NULL)
            error("Framebuffer not created");
        return WPN;
    }
    SyntaxError();
    return 0;
}

// a colour as the page holds it: the pixel BOX draws with it (DrawRectangle4, 8, 16 and 32)
static int WARP_CODE warp_native(int c)
{
    if (VideoColour == 4)
        return RGB121(c);
    if (VideoColour == 8)
        return RGB332(c);
    if (VideoColour == 16)
        return (BIT_5[(c & 0xFF0000) >> 19] << 11) | (BIT_6[(c & 0xFF00) >> 10] << 5) | BIT_5[(c & 0xFF) >> 3];
    if (VideoColour == 12)
        return (BIT_4[(c & 0xFF0000) >> 20] << 8) | (BIT_4[(c & 0xFF00) >> 12] << 4) | BIT_4[(c & 0xFF) >> 4] |
               ((c & 0xF000000) >> 12);
    return c;
}

// the bytes of one row of a page as it is stored (once)
static int WARP_CODE warp_rowbytes(int page)
{
    int w = PageTable[page].xmax;
    return VideoColour == 4 ? (w + 1) >> 1 : VideoColour == 8 ? w : VideoColour == 32 ? w * 4 : w * 2;
}

/* FRAMEBUFFER WARPH and WARPV (pages 0: p after the subcommand, the buffers as letters) and PAGE WARPH and WARPV
   (pages 1: from TO to). Everything is checked before the first pixel is written */
static void WARP_CODE Warp(char *p, int vertical, int pages)
{
    char sep[3] = {tokenTO, ',', 0};
    getargs(&p, 17, pages ? sep : ",");
    int dims[MAXDIM];
    warp_t a;
    int64_t *off, *rows = NULL, phase = 0;
    int from, to, cursorpage = VideoColour == 12 ? 1 : 0, cursorhidden = 0;
    if (argc < 5 || (vertical && argc > 15)) // (an empty last argument, a trailing comma, taken as the PicoMite takes it)
        SyntaxError();
    if (pages) // (from TO to, or a comma for TO, as PAGE COPY takes it)
        for (int i = 1; i < argc; i += 2)
            if (*argv[i] != ',' && (i != 1 || *argv[i] != tokenTO))
                SyntaxError();
    if (VideoColour == 1)
        error("Invalid for this screen mode"); // (the PicoMite's MODE 1: "Not available in mode 1")
    a.bpp = VideoColour == 12 ? 16 : VideoColour;
    if (pages)
    {
        from = getint(argv[0], 0, LastPage);
        to = getint(argv[2], 0, LastPage);
        if (from == to)
            error("Same page");
    }
    else
    {
        from = warp_letter(argv[0]);
        to = warp_letter(argv[2]);
        if (from == to)
            error("Same framebuffer");
    }
    a.src = PageTable[from].address;
    a.dst = PageTable[to].address;
    a.sw = PageTable[from].xmax;
    a.sh = PageTable[from].ymax;
    a.dw = PageTable[to].xmax;
    a.dh = PageTable[to].ymax;
    a.ss = warp_rowbytes(from) * (PageTable[from].expand ? 2 : 1); // (the first of a doubled row's copies)
    a.dup = PageTable[to].expand ? warp_rowbytes(to) : 0;
    a.ds = warp_rowbytes(to) + a.dup;
    a.vertical = vertical;
    int bands = vertical ? a.dw : a.dh; // the bands' dimension: the destination's
    a.n = parseintegerarray(argv[4], &off, 3, 1, dims, false, NULL);
    a.step = (argc >= 7 && *argv[6]) ? getint(argv[6], 1, bands) : 1;
    a.first = (argc >= 9 && *argv[8]) ? getint(argv[8], 0, bands - 1) : 0;
    if (argc >= 11 && *argv[10])
        phase = getinteger(argv[10]);
    a.edge = WARP_WRAP;
    if (argc >= 13 && *argv[12])
    {
        int c = getColour(argv[12], 1);
        a.edge = c < 0 ? WARP_LEAVE : warp_native(c);
    }
    a.trans = -1;
    a.tmask = VideoColour == 4 ? 0xF : VideoColour == 8 ? 0xFF : VideoColour == 16 ? 0xFFFF : VideoColour == 12 ? 0xF000
                                                                                       : 0xFFFFFF;
    if (argc >= 15 && *argv[14]) // as FRAMEBUFFER MERGE takes it: a 4 or 8-bit pixel value, else a colour
    {
        if (VideoColour == 4)
            a.trans = getint(argv[14], 0, 15);
        else if (VideoColour == 8)
            a.trans = getint(argv[14], 0, 255);
        else
        {
            a.trans = warp_native(getColour(argv[14], 0)) & a.tmask; // (32-bit: its RGB; 16-bit: RGB565)
            if (VideoColour == 12)
                a.trans = 0; // 12-bit: the source pixels whose alpha is 0, whatever colour was given (Peter)
        }
    }
    if ((int64_t)(a.n - 1) * a.step + a.first >= bands) // every element must start on the buffer
        error(vertical ? "Array has % elements, step % from column % fits %"
                       : "Array has % elements, step % from row % fits %",
              a.n, a.step, a.first, (bands - a.first + a.step - 1) / a.step);
    if (argc >= 17 && *argv[16])
    {
        int m = parseintegerarray(argv[16], &rows, 9, 1, dims, false, NULL);
        if (m != a.n)
            error("Row array has % elements, offset array has %", m, a.n);
        for (int i = 0; i < m; i++)
            if (rows[i] < -1 || rows[i] >= a.sh)
                error("Row array element % is ~: rows are -1 to %", i + OptionBase, rows[i], a.sh - 1);
    }
    int last = a.first + a.n * a.step; // (one past the last row or column the bands cover)
    if (last > bands)
        last = bands;
    if (vertical && last > a.sw) // the columns must be in the source: different sizes (the CMM2's world, PAGE RESIZE)
        error("Columns % to % are not all in the source (% wide)", a.first, last - 1, a.sw);
    if (!vertical && !rows && last > a.sh)
        error("Rows % to % are not all in the source (% high)", a.first, last - 1, a.sh);
    int *boff = (int *)GetTempMemory(a.n * sizeof(int) * (rows ? 2 : 1));
    a.boff = boff;
    a.brow = rows ? boff + a.n : NULL;
    if (vertical)
        warp_bands(boff, NULL, off, NULL, a.n, phase, a.sh, -a.dh, a.sh, a.edge == WARP_WRAP);
    else
        warp_bands(boff, rows ? boff + a.n : NULL, off, rows, a.n, phase, a.sw, -a.sw, a.dw, a.edge == WARP_WRAP);
    // the GUI cursor off the pages first, in the modes that draw it into them (PAGE COPY's rule); the 4 and 1-bit
    // modes draw it over the merge's display, which the pages never hold
    if (cursoron && VideoColour >= 8 && (from == cursorpage || to == cursorpage))
    {
        uint8_t readsave = ReadPage, writesave = WritePage;
        ReadPage = WritePage = cursorpage;
        hidecursor(0);
        cursorhidden = 1;
        ReadPage = readsave;
        WritePage = writesave;
    }
    warp_run(&a);
    if (cursorhidden)
    {
        uint8_t readsave = ReadPage, writesave = WritePage;
        ReadPage = WritePage = cursorpage;
        showcursor(0, xcursor, ycursor);
        ReadPage = readsave;
        WritePage = writesave;
    }
}

/* the subcommands from cmd_framebuffer, Rgb121Framebuffer and cmd_page (form: Rgb121.h), here so that those files,
   whose code is on hot paths, grow by a call each (the I-cache: added code that moves the 1 ms housekeeping makes
   every statement slower). 1 if cmd was one of them */
int WARP_CODE WarpCommand(char *cmd, int form)
{
    char *p;
    int vertical;
    if ((p = checkstring(cmd, "WARPH")))
        vertical = 0;
    else if ((p = checkstring(cmd, "WARPV")))
        vertical = 1;
    else if (form == WARP_FRAMEBUFFER && (p = checkstring(cmd, "WAIT")))
    {
        // FRAMEBUFFER WAIT in the 8, 12, 16 and 32-bit modes: to the end of the frame, as the 4-bit modes' (Rgb121.c)
        pagesetdone = 0;
        while (!pagesetdone)
            CheckAbort();
        return 1;
    }
    else
        return 0;
    if (form != WARP_TEST)
        Warp(p, vertical, form == WARP_PAGE);
    return 1;
}
