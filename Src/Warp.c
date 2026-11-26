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

FRAMEBUFFER and PAGE AFFINE and SPAN (the PicoMite's docs/FRAMEBUFFER_AFFINE_SPAN_Design.md, agreed by Peter on 25 Nov
2026, and its graphics/Warp.c again) are here too, on the same pieces and with the same buffers.

    FRAMEBUFFER AFFINE from, to, u%(), v%(), du%(), dv%() [, step] [, first] [, left] [, width] [, edge] [, transparent]
    FRAMEBUFFER SPAN from, to, left%(), right%() [, step] [, first] [, fill] [, transparent] [, outside]
    PAGE AFFINE from TO to, u%(), ...          PAGE SPAN from TO to, left%(), ...

AFFINE reads each row of the destination from the source along a straight line: column i of row j of band b shows the
source's pixel at (u - j*dv + i*du, v + j*du + i*dv) / 65536, rounded down, so one element turns and scales a whole
picture and an element a row makes a floor in perspective. Its edge and transparent are WARPH's. SPAN copies, on each
row of band b, the columns left(b) to right(b) (or, with outside, the others); the pixels it does not copy are left, or
get the fill colour. What the CMM2 adds to these two, besides the depths and the doubled rows above:
- the source and the destination can be of different sizes. AFFINE's places wrap at the source's size, or are outside
  it there (edge), and its bands and window are the destination's: a map larger than the screen (the world framebuffer
  F, up to 4096 x 2048) is the larger world that the PicoMite's design leaves for later. SPAN copies a pixel from the
  same place in the source, and a place that the source does not have is left as it is.

The core (all but the commands at the end) is plain C, so that tests/host/affine_host.c can check it on a PC against a
pixel-by-pixel model of the design (WARP_HOST_TEST).
***********************************************************************************************************************/
#ifndef WARP_HOST_TEST
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Rgb121.h"

extern volatile uint8_t pagesetdone;

#define WARP_CODE __attribute__((section(".extra"), noinline)) // (the end of .text: nothing before it moves)
#endif
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

// a destination row finished: its copy where the page stores each row twice (dup: a row's bytes, 0 if it does not),
// and the audio and USB seen to now and then
WARP_INLINE void warp_rowdone(uint8_t *d, int dup, int y)
{
    if (dup)
        memcpy(d + dup, d, dup);
    if ((y & 15) == 15)
        routinechecks(1);
}

WARP_INLINE void warp_done(const warp_t *a, uint8_t *d, int y)
{
    warp_rowdone(d, a->dup, y);
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
    if (w <= 0)
        return;
    if (bpp == 8)
        memset(row + x, c, w);
    else if (bpp == 4) // whole bytes between an odd start and an odd end (SPAN fills most of a screen)
    {
        if (x & 1)
        {
            warp_put(row, x, 4, c);
            x++, w--;
        }
        memset(row + (x >> 1), c * 0x11, w >> 1);
        if (w & 1)
            warp_put(row, x + w - 1, 4, c);
    }
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

/* ---------------------------------------------------------------------------------------------------------------------
FRAMEBUFFER SPAN: band b is the step rows of the destination from first + b * step (those off it are not drawn), and its
span is the columns lf[b] to rt[b], cut to the row and empty when lf > rt. The span's pixels (outside: the row's others)
are copied from the same place in the source, those of the transparent value excepted; the row's other pixels get the
fill colour, if there is one. The source may be the destination (src NULL): nothing is copied then, and only the fill
is drawn. The CMM2's: a place that a smaller source does not have is left as it is.
--------------------------------------------------------------------------------------------------------------------- */
typedef struct
{
    uint8_t *dst;
    const uint8_t *src;       // NULL: the destination itself
    int sw, sh, ss;           // as warp_t
    int dw, dh, ds, dup, bpp;
    int n, step, first;       // the bands
    const int64_t *lf, *rt;   // each band's first and last column
    int fill, trans, tmask;   // -1 or a pixel value; -1 or a pixel's bits tmask (as warp_t)
    int outside;              // 0 or 1
} span_t;

// the columns x0 to x1-1 of a row from the same columns of the source's row s (NULL: it has no such row)
static void WARP_CODE span_copy(const span_t *a, uint8_t *d, const uint8_t *s, int x0, int x1)
{
    if (x1 > a->sw)
        x1 = a->sw;
    if (s && x0 < x1)
        warp_seg(d, x0, s, x0, x1 - x0, a->bpp, a->trans, a->tmask);
}

static void WARP_CODE span_run(const span_t *a)
{
    int w = a->dw, bpp = a->bpp, rows = 0;
    for (int b = 0; b < a->n; b++)
    {
        int64_t y0 = a->first + (int64_t)b * a->step, y1 = y0 + a->step;
        if (y0 >= a->dh)
            break;
        if (y1 <= 0)
            continue;
        if (y0 < 0)
            y0 = 0;
        if (y1 > a->dh)
            y1 = a->dh;
        int64_t l = a->lf[b], r = a->rt[b];
        int lo = l < 0 ? 0 : l > w ? w : (int)l, hi = r < 0 ? 0 : r >= w ? w : (int)r + 1; // the span is lo to hi-1
        if (hi < lo)
            hi = lo;
        for (int y = (int)y0; y < (int)y1; y++)
        {
            uint8_t *d = a->dst + y * a->ds;
            const uint8_t *s = (a->src && y < a->sh) ? a->src + y * a->ss : NULL;
            if (!a->outside)
            {
                span_copy(a, d, s, lo, hi);
                if (a->fill >= 0)
                {
                    warp_fill(d, 0, lo, bpp, a->fill);
                    warp_fill(d, hi, w - hi, bpp, a->fill);
                }
            }
            else
            {
                span_copy(a, d, s, 0, lo);
                span_copy(a, d, s, hi, w);
                if (a->fill >= 0)
                    warp_fill(d, lo, hi - lo, bpp, a->fill);
            }
            warp_rowdone(d, a->dup, rows++);
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------------
FRAMEBUFFER AFFINE: band b is the step rows of the destination from first + b * step, and the window is the width
columns from left; what is off the destination is not drawn. Column i of row j of the band shows the source's pixel
(sx, sy):

    U = u[b] - j * dv[b] + i * du[b]      sx = U / 65536, rounded down
    V = v[b] + j * du[b] + i * dv[b]      sy = V / 65536, rounded down

With no edge (WARP_WRAP) sx and sy are taken round the source, and any value is allowed. With an edge a pixel whose
place is outside the source is left (WARP_LEAVE) or gets the edge colour, and a place beyond +-AFF_POS or a step beyond
+-AFF_STEP is taken as that limit, so that no sum here can overflow. A source pixel of the transparent value is not
written. The source's size is its own (the CMM2's: sw by sh, below 32768 each way so that its size in 1/65536 pixel and
the sum of two places fit 32 bits; the largest page is 4096 x 2048).
--------------------------------------------------------------------------------------------------------------------- */
#define AFF_POS ((int64_t)1 << 60)
#define AFF_STEP ((int64_t)1 << 35)
#define WARP_FAR (1 << 24) // first, left and width go no further than this either way

typedef struct
{
    uint8_t *dst;
    const uint8_t *src;
    int sw, sh, ss;                  // as warp_t
    int dw, dh, ds, dup, bpp;
    int n, step, first, left, width; // the bands, and the window's first column and its width
    const int64_t *u, *v, *du, *dv;  // each band: the place its first pixel shows, and the step along a row
    int edge, trans, tmask;          // as warp_t
} affine_t;

static int64_t WARP_CODE aff_mod(int64_t a, int64_t m) // 0 to m-1
{
    a %= m;
    return a < 0 ? a + m : a;
}

static int64_t WARP_CODE aff_floor(int64_t a, int64_t b) // a / b rounded down, for b > 0
{
    int64_t q = a / b;
    return a % b < 0 ? q - 1 : q;
}

static int64_t WARP_CODE aff_clamp(int64_t a, int64_t m)
{
    return a > m ? m : a < -m ? -m : a;
}

// the columns *i0 to *i1-1 cut down to those i for which f + i * d is 0 to m-1 (a straight line: one run of them)
static void WARP_CODE aff_inside(int64_t f, int64_t d, int64_t m, int64_t *i0, int64_t *i1)
{
    int64_t lo, hi;
    if (d == 0)
    {
        if (f < 0 || f >= m)
            *i1 = *i0;
        return;
    }
    if (d > 0)
        lo = -aff_floor(f, d), hi = aff_floor(m - 1 - f, d);
    else
        lo = -aff_floor(m - 1 - f, -d), hi = aff_floor(f, -d);
    if (lo > *i0)
        *i0 = lo;
    if (hi + 1 < *i1)
        *i1 = hi + 1;
}

typedef struct
{
    uint32_t du, dv, mw, mh; // the steps along a row, and the source's size: all in 1/65536 pixel
    const uint8_t *src;
    int stride, trans, tmask;
} aff_step_t;

/* cnt pixels of a destination row from pixel x: each is the source's pixel at (U >> 16, V >> 16), and U and V then
   step by du and dv round the source. All four are 0 to mw-1 or mh-1, so a step is an addition and one compare, and
   no pixel needs a division. Made once for each depth, each a function of its own: inlined into affine_run the loop
   had no registers left and read its steps from the stack for every pixel (on the PicoMite, 5.5 ms a MODE 2 screen
   against 4.8). */
WARP_INLINE void aff_row(uint8_t *d, int x, int cnt, uint32_t U, uint32_t V, const aff_step_t *s, int bpp)
{
    uint32_t du = s->du, dv = s->dv, mw = s->mw, mh = s->mh; // in locals, as in warp_v
    const uint8_t *src = s->src;
    int stride = s->stride, trans = s->trans, tmask = s->tmask;
#define AFF_NEXT(c)                                           \
    {                                                         \
        c = warp_get(src + (V >> 16) * stride, U >> 16, bpp); \
        if ((U += du) >= mw)                                  \
            U -= mw;                                          \
        if ((V += dv) >= mh)                                  \
            V -= mh;                                          \
    }
    int c, c2;
    if (bpp == 4 && trans < 0) // two pixels gathered for each whole byte of the destination
    {
        if (cnt && (x & 1))
        {
            AFF_NEXT(c);
            warp_put(d, x, 4, c);
            x++, cnt--;
        }
        uint8_t *q = d + (x >> 1);
        for (; cnt >= 2; cnt -= 2)
        {
            AFF_NEXT(c);
            AFF_NEXT(c2);
            *q++ = (uint8_t)(c | (c2 << 4));
        }
        if (cnt)
        {
            AFF_NEXT(c);
            *q = (uint8_t)((*q & 0xF0) | c);
        }
        return;
    }
    for (; cnt > 0; cnt--, x++)
    {
        AFF_NEXT(c);
        if ((c & tmask) != trans)
            warp_put(d, x, bpp, c);
    }
#undef AFF_NEXT
}

static void WARP_CODE aff_row4(uint8_t *d, int x, int cnt, uint32_t U, uint32_t V, const aff_step_t *s)
{
    aff_row(d, x, cnt, U, V, s, 4);
}

static void WARP_CODE aff_row8(uint8_t *d, int x, int cnt, uint32_t U, uint32_t V, const aff_step_t *s)
{
    aff_row(d, x, cnt, U, V, s, 8);
}

static void WARP_CODE aff_row16(uint8_t *d, int x, int cnt, uint32_t U, uint32_t V, const aff_step_t *s)
{
    aff_row(d, x, cnt, U, V, s, 16);
}

static void WARP_CODE aff_row32(uint8_t *d, int x, int cnt, uint32_t U, uint32_t V, const aff_step_t *s)
{
    aff_row(d, x, cnt, U, V, s, 32);
}

static void WARP_CODE affine_run(const affine_t *a)
{
    int w = a->dw, h = a->dh, wrap = a->edge == WARP_WRAP, rows = 0;
    int64_t mw = (int64_t)a->sw << 16, mh = (int64_t)a->sh << 16;
    aff_step_t st = {0, 0, (uint32_t)mw, (uint32_t)mh, a->src, a->ss, a->trans, a->tmask};
    int64_t ia = a->left < 0 ? -(int64_t)a->left : 0, ib = a->width; // the window's columns that are on the buffer
    if (ib > (int64_t)w - a->left)
        ib = (int64_t)w - a->left;
    if (ia >= ib)
        return;
    for (int b = 0; b < a->n; b++)
    {
        int64_t y0 = a->first + (int64_t)b * a->step;
        if (y0 >= h)
            break;
        if (y0 + a->step <= 0)
            continue;
        int64_t u = a->u[b], v = a->v[b], du = a->du[b], dv = a->dv[b];
        int64_t dvu = dv, duv = du; // the steps down the band: dv as U takes it, du as V does
        if (wrap)
        {
            u = aff_mod(u, mw), v = aff_mod(v, mh);
            dvu = aff_mod(dv, mw), duv = aff_mod(du, mh);
        }
        else
        {
            u = aff_clamp(u, AFF_POS), v = aff_clamp(v, AFF_POS);
            dvu = dv = aff_clamp(dv, AFF_STEP), duv = du = aff_clamp(du, AFF_STEP);
        }
        // the steps along a row, round the source. With an edge these serve only for a run of two pixels or more
        // inside the source, whose step is then smaller than the source: the same step, a negative one made positive
        st.du = (uint32_t)aff_mod(du, mw), st.dv = (uint32_t)aff_mod(dv, mh);
        int j1 = y0 + a->step > h ? (int)(h - y0) : a->step;
        for (int j = y0 < 0 ? (int)-y0 : 0; j < j1; j++)
        {
            uint8_t *d = a->dst + (int)(y0 + j) * a->ds;
            int64_t U = u - j * dvu, V = v + j * duv, i0 = ia, i1 = ib; // the row's column 0
            if (wrap)
            {
                U = aff_mod(U + i0 * st.du, mw);
                V = aff_mod(V + i0 * st.dv, mh);
            }
            else
            {
                aff_inside(U, du, mw, &i0, &i1);
                aff_inside(V, dv, mh, &i0, &i1);
                if (i0 >= i1)
                    i0 = i1 = ia; // none of the row is in the source
                if (a->edge >= 0)
                {
                    warp_fill(d, a->left + (int)ia, (int)(i0 - ia), a->bpp, a->edge);
                    warp_fill(d, a->left + (int)i1, (int)(ib - i1), a->bpp, a->edge);
                }
                U += i0 * du, V += i0 * dv;
            }
            int x = a->left + (int)i0, cnt = (int)(i1 - i0);
            if (a->bpp == 4)
                aff_row4(d, x, cnt, (uint32_t)U, (uint32_t)V, &st);
            else if (a->bpp == 8)
                aff_row8(d, x, cnt, (uint32_t)U, (uint32_t)V, &st);
            else if (a->bpp == 16)
                aff_row16(d, x, cnt, (uint32_t)U, (uint32_t)V, &st);
            else
                aff_row32(d, x, cnt, (uint32_t)U, (uint32_t)V, &st);
            warp_rowdone(d, a->dup, rows++);
        }
    }
}

#ifndef WARP_HOST_TEST
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

// the two buffers of a command, and what the cores need to know of them (warp_t's fields of the same names)
typedef struct
{
    int from, to; // the pages
    uint8_t *dst;
    const uint8_t *src;
    int sw, sh, ss, dw, dh, ds, dup, bpp, tmask;
} warp_pair_t;

#define WARP_GEOMETRY(a, g)                                                                                         \
    ((a).src = (g).src, (a).dst = (g).dst, (a).sw = (g).sw, (a).sh = (g).sh, (a).ss = (g).ss, (a).dw = (g).dw,       \
     (a).dh = (g).dh, (a).ds = (g).ds, (a).dup = (g).dup, (a).bpp = (g).bpp, (a).tmask = (g).tmask)

/* a command's from and to: FRAMEBUFFER letters, or (pages) the page numbers of PAGE ... from TO to. Refused in a 1-bit
   mode, and when they are one buffer unless the command can do with one (same: SPAN) */
static void WARP_CODE warp_pair(warp_pair_t *g, char *f, char *t, int pages, int same)
{
    if (VideoColour == 1)
        error("Invalid for this screen mode"); // (the PicoMite's MODE 1: "Not available in mode 1")
    if (pages)
    {
        g->from = getint(f, 0, LastPage);
        g->to = getint(t, 0, LastPage);
    }
    else
    {
        g->from = warp_letter(f);
        g->to = warp_letter(t);
    }
    if (g->from == g->to && !same)
        error(pages ? "Same page" : "Same framebuffer");
    g->bpp = VideoColour == 12 ? 16 : VideoColour;
    g->src = PageTable[g->from].address;
    g->dst = PageTable[g->to].address;
    g->sw = PageTable[g->from].xmax;
    g->sh = PageTable[g->from].ymax;
    g->dw = PageTable[g->to].xmax;
    g->dh = PageTable[g->to].ymax;
    g->ss = warp_rowbytes(g->from) * (PageTable[g->from].expand ? 2 : 1); // (the first of a doubled row's copies)
    g->dup = PageTable[g->to].expand ? warp_rowbytes(g->to) : 0;
    g->ds = warp_rowbytes(g->to) + g->dup;
    g->tmask = VideoColour == 4 ? 0xF : VideoColour == 8 ? 0xFF : VideoColour == 16 ? 0xFFFF : VideoColour == 12 ? 0xF000
                                                                                        : 0xFFFFFF;
}

// the PAGE form's separators: from TO to (or a comma for TO, as PAGE COPY takes it), and commas after
static void WARP_CODE warp_seps(char **argv, int argc)
{
    for (int i = 1; i < argc; i += 2)
        if (*argv[i] != ',' && (i != 1 || *argv[i] != tokenTO))
            SyntaxError();
}

// an edge or fill argument: -1 (WARP_LEAVE), or a colour as the pixel that BOX would draw with it
static int WARP_CODE warp_edge(char *p)
{
    int c = getColour(p, 1);
    return c < 0 ? WARP_LEAVE : warp_native(c);
}

// a transparent argument, as FRAMEBUFFER MERGE takes it: a 4 or 8-bit pixel value, else a colour (32-bit: its RGB;
// 16-bit: RGB565; 12-bit: the source pixels whose alpha is 0, whatever colour was given (Peter))
static int WARP_CODE warp_trans(char *p, int tmask)
{
    if (VideoColour == 4)
        return getint(p, 0, 15);
    if (VideoColour == 8)
        return getint(p, 0, 255);
    int t = warp_native(getColour(p, 0)) & tmask;
    return VideoColour == 12 ? 0 : t;
}

/* the GUI cursor off the pages before a command writes (show 0: returns 1 if it was taken off) and back after (show 1),
   in the modes that draw it into them (PAGE COPY's rule); the 4 and 1-bit modes draw it over the merge's display,
   which the pages never hold */
static int WARP_CODE warp_cursor(const warp_pair_t *g, int show)
{
    int cursorpage = VideoColour == 12 ? 1 : 0;
    if (!show && !(cursoron && VideoColour >= 8 && (g->from == cursorpage || g->to == cursorpage)))
        return 0;
    uint8_t readsave = ReadPage, writesave = WritePage;
    ReadPage = WritePage = cursorpage;
    if (show)
        showcursor(0, xcursor, ycursor);
    else
        hidecursor(0);
    ReadPage = readsave;
    WritePage = writesave;
    return 1;
}

/* FRAMEBUFFER WARPH and WARPV (pages 0: p after the subcommand, the buffers as letters) and PAGE WARPH and WARPV
   (pages 1: from TO to). Everything is checked before the first pixel is written */
static void WARP_CODE Warp(char *p, int vertical, int pages)
{
    char sep[3] = {tokenTO, ',', 0};
    getargs(&p, 17, pages ? sep : ",");
    int dims[MAXDIM];
    warp_t a;
    warp_pair_t g;
    int64_t *off, *rows = NULL, phase = 0;
    if (argc < 5 || (vertical && argc > 15)) // (an empty last argument, a trailing comma, taken as the PicoMite takes it)
        SyntaxError();
    if (pages)
        warp_seps(argv, argc);
    warp_pair(&g, argv[0], argv[2], pages, 0);
    WARP_GEOMETRY(a, g);
    a.vertical = vertical;
    int bands = vertical ? a.dw : a.dh; // the bands' dimension: the destination's
    a.n = parseintegerarray(argv[4], &off, 3, 1, dims, false, NULL);
    a.step = (argc >= 7 && *argv[6]) ? getint(argv[6], 1, bands) : 1;
    a.first = (argc >= 9 && *argv[8]) ? getint(argv[8], 0, bands - 1) : 0;
    if (argc >= 11 && *argv[10])
        phase = getinteger(argv[10]);
    a.edge = (argc >= 13 && *argv[12]) ? warp_edge(argv[12]) : WARP_WRAP;
    a.trans = (argc >= 15 && *argv[14]) ? warp_trans(argv[14], a.tmask) : -1;
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
    int cursorhidden = warp_cursor(&g, 0);
    warp_run(&a);
    if (cursorhidden)
        warp_cursor(&g, 1);
}

/* FRAMEBUFFER SPAN and PAGE SPAN (pages, as Warp). No value of the arrays is an error, and the bands may start above
   the buffer or run past its end (the design's section 8) */
static void WARP_CODE Span(char *p, int pages)
{
    char sep[3] = {tokenTO, ',', 0};
    getargs(&p, 17, pages ? sep : ",");
    int dims[MAXDIM];
    span_t a;
    warp_pair_t g;
    int64_t *lf, *rt;
    if (argc < 7)
        SyntaxError();
    if (pages)
        warp_seps(argv, argc);
    warp_pair(&g, argv[0], argv[2], pages, 1);
    WARP_GEOMETRY(a, g);
    if (g.from == g.to)
        a.src = NULL; // one buffer: nothing is copied, and only the fill is drawn
    a.n = parseintegerarray(argv[4], &lf, 3, 1, dims, false, NULL);
    int m = parseintegerarray(argv[6], &rt, 4, 1, dims, false, NULL);
    if (m != a.n)
        error("Right array has % elements, left array has %", m, a.n);
    a.lf = lf;
    a.rt = rt;
    a.step = (argc >= 9 && *argv[8]) ? getint(argv[8], 1, a.dh) : 1;
    a.first = (argc >= 11 && *argv[10]) ? getint(argv[10], -WARP_FAR, WARP_FAR) : 0;
    a.fill = (argc >= 13 && *argv[12]) ? warp_edge(argv[12]) : -1;
    a.trans = (argc >= 15 && *argv[14]) ? warp_trans(argv[14], a.tmask) : -1;
    a.outside = (argc >= 17 && *argv[16]) ? getint(argv[16], 0, 1) : 0;
    int cursorhidden = warp_cursor(&g, 0);
    span_run(&a);
    if (cursorhidden)
        warp_cursor(&g, 1);
}

/* FRAMEBUFFER AFFINE and PAGE AFFINE (pages, as Warp). No value of the arrays is an error, and the window may lie
   partly or wholly off the destination (the design's section 4) */
static void WARP_CODE Affine(char *p, int pages)
{
    char sep[3] = {tokenTO, ',', 0};
    getargs(&p, 23, pages ? sep : ",");
    int dims[MAXDIM];
    affine_t a;
    warp_pair_t g;
    int64_t *q[4];
    if (argc < 11)
        SyntaxError();
    if (pages)
        warp_seps(argv, argc);
    warp_pair(&g, argv[0], argv[2], pages, 0);
    WARP_GEOMETRY(a, g);
    for (int i = 0; i < 4; i++) // u, v, du, dv
    {
        int m = parseintegerarray(argv[4 + 2 * i], &q[i], 3 + i, 1, dims, false, NULL);
        if (i == 0)
            a.n = m;
        else if (m != a.n)
            error("Array % has % elements, array 1 has %", i + 1, m, a.n);
    }
    a.u = q[0], a.v = q[1], a.du = q[2], a.dv = q[3];
    a.step = (argc >= 13 && *argv[12]) ? getint(argv[12], 1, a.dh) : 1;
    a.first = (argc >= 15 && *argv[14]) ? getint(argv[14], -WARP_FAR, WARP_FAR) : 0;
    a.left = (argc >= 17 && *argv[16]) ? getint(argv[16], -WARP_FAR, WARP_FAR) : 0;
    a.width = (argc >= 19 && *argv[18]) ? getint(argv[18], 1, WARP_FAR) : a.dw;
    a.edge = (argc >= 21 && *argv[20]) ? warp_edge(argv[20]) : WARP_WRAP;
    a.trans = (argc >= 23 && *argv[22]) ? warp_trans(argv[22], a.tmask) : -1;
    int cursorhidden = warp_cursor(&g, 0);
    affine_run(&a);
    if (cursorhidden)
        warp_cursor(&g, 1);
}

/* the subcommands from cmd_framebuffer, Rgb121Framebuffer and cmd_page (form: Rgb121.h), here so that those files,
   whose code is on hot paths, grow by a call each (the I-cache: added code that moves the 1 ms housekeeping makes
   every statement slower). 1 if cmd was one of them */
int WARP_CODE WarpCommand(char *cmd, int form)
{
    char *p;
    int which; // 0 WARPH, 1 WARPV, 2 AFFINE, 3 SPAN
    if ((p = checkstring(cmd, "WARPH")))
        which = 0;
    else if ((p = checkstring(cmd, "WARPV")))
        which = 1;
    else if ((p = checkstring(cmd, "AFFINE")))
        which = 2;
    else if ((p = checkstring(cmd, "SPAN")))
        which = 3;
    else if (form == WARP_FRAMEBUFFER && (p = checkstring(cmd, "WAIT")))
    {
        // FRAMEBUFFER WAIT in the 8, 12, 16 and 32-bit modes: to the end of the frame, as the 4-bit modes' (Rgb121.c)
        ThreadFrameWait();                                          // (the other threads run meanwhile: Thread.c)
        return 1;
    }
    else
        return 0;
    if (form == WARP_TEST)
        return 1;
    if (which == 2)
        Affine(p, form == WARP_PAGE);
    else if (which == 3)
        Span(p, form == WARP_PAGE);
    else
        Warp(p, which, form == WARP_PAGE);
    return 1;
}
#endif
