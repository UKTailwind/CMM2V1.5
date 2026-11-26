/***********************************************************************************************************************
affine_host.c - the core of FRAMEBUFFER AFFINE and SPAN (Src/Warp.c: affine_run and span_run) checked on a PC, pixel by
pixel, against a model written from the rules of the PicoMite's docs/FRAMEBUFFER_AFFINE_SPAN_Design.md (sections 4 and
8) in 128-bit arithmetic, with what the CMM2 adds: a source and a destination of different sizes, 32-bit pixels, the
bits of a pixel that the transparent value is compared with (the 12-bit depth's alpha, a 32-bit pixel's RGB), and pages
that store each row twice (the first copy read, both written).

Every byte of the destination is compared, its second copies and the unused half of an odd 4-bit row's last byte too,
and the buffers are malloc'ed at their exact size so that the address sanitizer sees a read or write past either.

    gcc -O2 -g -fsanitize=address,undefined -fno-sanitize-recover=all -o affine_host tests/host/affine_host.c
    ./affine_host [cases] [seed]          (default 300000 cases, seed 2026)

Prints the number of cases of each kind and "AFFINE AND SPAN HOST TEST PASSED", or the first case that is wrong.
***********************************************************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WARP_HOST_TEST
#define WARP_CODE
static long routine_calls;
static void routinechecks(int n)
{
    (void)n;
    routine_calls++;
}
#include "../../Src/Warp.c"

typedef __int128 i128;

static uint64_t rs = 2026;
static uint64_t rnd(void) // xorshift64*
{
    rs ^= rs >> 12, rs ^= rs << 25, rs ^= rs >> 27;
    return rs * 0x2545F4914F6CDD1DULL;
}

static int64_t ri(int64_t lo, int64_t hi) // lo to hi
{
    return lo + (int64_t)(rnd() % (uint64_t)(hi - lo + 1));
}

static int chance(int pc)
{
    return (int)(rnd() % 100) < pc;
}

// a page as the firmware holds it: w by h pixels, its rows stored twice if dbl
typedef struct
{
    uint8_t *p;
    int w, h, bpp, rb, stride, dbl;
} page_t;

static int rowbytes(int w, int bpp)
{
    return bpp == 4 ? (w + 1) >> 1 : w * (bpp >> 3);
}

static void page_new(page_t *g, int w, int h, int bpp, int dbl)
{
    g->w = w, g->h = h, g->bpp = bpp, g->dbl = dbl;
    g->rb = rowbytes(w, bpp);
    g->stride = g->rb * (dbl ? 2 : 1);
    g->p = malloc((size_t)g->stride * h);
    for (int y = 0; y < h; y++)
    {
        uint8_t *r = g->p + y * g->stride;
        for (int i = 0; i < g->rb; i++)
            r[i] = chance(60) ? (uint8_t)(rnd() & 0x77) : (uint8_t)rnd(); // (few values: transparent ones are met)
        if (dbl)
            memcpy(r + g->rb, r, g->rb);
    }
}

static int pget(const page_t *g, int x, int y)
{
    return warp_get(g->p + y * g->stride, x, g->bpp);
}

static void pput(page_t *g, int x, int y, int c) // both copies of a doubled row
{
    warp_put(g->p + y * g->stride, x, g->bpp, c);
    if (g->dbl)
        warp_put(g->p + y * g->stride + g->rb, x, g->bpp, c);
}

static i128 floordiv(i128 a, i128 b) // b > 0
{
    i128 q = a / b;
    return a % b < 0 ? q - 1 : q;
}

static i128 clampi(i128 a, i128 m)
{
    return a > m ? m : a < -m ? -m : a;
}

static int transparent(int c, int trans, int tmask)
{
    return trans >= 0 && (c & tmask) == trans;
}

// the design's section 4, on a copy of the destination
static void affine_model(page_t *want, const page_t *src, const affine_t *a)
{
    for (int y = 0; y < want->h; y++)
    {
        i128 rel = (i128)y - a->first;
        if (rel < 0 || rel >= (i128)a->n * a->step)
            continue;
        int b = (int)(rel / a->step), j = (int)(rel % a->step);
        i128 u = a->u[b], v = a->v[b], du = a->du[b], dv = a->dv[b];
        if (a->edge != WARP_WRAP)
        {
            u = clampi(u, (i128)1 << 60), v = clampi(v, (i128)1 << 60);
            du = clampi(du, (i128)1 << 35), dv = clampi(dv, (i128)1 << 35);
        }
        for (int x = 0; x < want->w; x++)
        {
            i128 i = (i128)x - a->left;
            if (i < 0 || i >= a->width)
                continue;
            i128 sx = floordiv(u - j * dv + i * du, 65536), sy = floordiv(v + j * du + i * dv, 65536);
            if (a->edge == WARP_WRAP)
            {
                sx = ((sx % src->w) + src->w) % src->w;
                sy = ((sy % src->h) + src->h) % src->h;
            }
            else if (sx < 0 || sx >= src->w || sy < 0 || sy >= src->h)
            {
                if (a->edge >= 0)
                    pput(want, x, y, a->edge);
                continue;
            }
            int c = pget(src, (int)sx, (int)sy);
            if (!transparent(c, a->trans, a->tmask))
                pput(want, x, y, c);
        }
    }
}

// section 8, and the CMM2's rule for a source that is the smaller: a place it does not have is left
static void span_model(page_t *want, const page_t *src, const span_t *a, int same)
{
    for (int y = 0; y < want->h; y++)
    {
        i128 rel = (i128)y - a->first;
        if (rel < 0 || rel >= (i128)a->n * a->step)
            continue;
        int b = (int)(rel / a->step);
        for (int x = 0; x < want->w; x++)
        {
            int inside = a->lf[b] <= x && x <= a->rt[b];
            if (inside != a->outside)
            {
                if (!same && x < src->w && y < src->h)
                {
                    int c = pget(src, x, y);
                    if (!transparent(c, a->trans, a->tmask))
                        pput(want, x, y, c);
                }
            }
            else if (a->fill >= 0)
                pput(want, x, y, a->fill);
        }
    }
}

static int64_t wild(void) // any 64-bit value, the ends among them
{
    int k = (int)ri(0, 9);
    return k == 0 ? INT64_MIN : k == 1 ? INT64_MAX : k == 2 ? INT64_MIN + ri(0, 9) : k == 3 ? INT64_MAX - ri(0, 9)
                                                                                           : (int64_t)rnd();
}

#define FP 65536
// element k of AFFINE's four arrays, by the case's kind (the board test's six, and one more)
static void setval(int kind, int W, int H, int64_t *u, int64_t *v, int64_t *du, int64_t *dv)
{
    switch (kind)
    {
    case 0: // the picture as it is, moved
        *du = FP, *dv = 0, *u = ri(-W, W) * FP, *v = ri(-H, H) * FP;
        break;
    case 1: // any small step
        *du = ri(-3 * FP, 3 * FP), *dv = ri(-3 * FP, 3 * FP);
        *u = ri(-4 * (int64_t)W * FP, 4 * (int64_t)W * FP), *v = ri(-4 * (int64_t)H * FP, 4 * (int64_t)H * FP);
        break;
    case 2: // a quarter turn
        *du = 0, *dv = FP * (2 * ri(0, 1) - 1), *u = ri(-W, W) * FP, *v = ri(-H, H) * FP;
        break;
    case 3: // far away, with long steps
        *du = ri(-((int64_t)1 << 30), (int64_t)1 << 30), *dv = ri(-((int64_t)1 << 30), (int64_t)1 << 30);
        *u = ri(-((int64_t)1 << 44), (int64_t)1 << 44), *v = ri(-((int64_t)1 << 44), (int64_t)1 << 44);
        break;
    case 4: // steps of a few thousandths of a pixel
        *du = ri(-300, 300), *dv = ri(-300, 300), *u = ri(0, (int64_t)W * FP), *v = ri(0, (int64_t)H * FP);
        break;
    case 5: // a mirrored row
        *du = -FP, *dv = 0, *u = ri(-W, 2 * W) * FP + FP / 2, *v = ri(-H, 2 * H) * FP;
        break;
    case 6: // near the limits of section 4, and past them
        *du = chance(50) ? wild() : ri(-((int64_t)1 << 36), (int64_t)1 << 36);
        *dv = chance(50) ? wild() : ri(-((int64_t)1 << 36), (int64_t)1 << 36);
        *u = chance(50) ? wild() : ri(-((int64_t)1 << 61), (int64_t)1 << 61);
        *v = chance(50) ? wild() : ri(-((int64_t)1 << 61), (int64_t)1 << 61);
        break;
    default: // a slow turn about a place inside the source: most of the window in it
        *du = ri(FP / 4, 2 * FP), *dv = ri(-FP, FP), *u = ri(0, (int64_t)W * FP), *v = ri(0, (int64_t)H * FP);
        break;
    }
}

static int far(int size) // first or left: on the buffer, just off it, or as far as the commands allow
{
    int k = (int)ri(0, 9);
    return k < 5 ? (int)ri(-size, size) : k < 8 ? (int)ri(-3, 3) : k == 8 ? (int)ri(-WARP_FAR, WARP_FAR)
                                                                         : (chance(50) ? WARP_FAR : -WARP_FAR);
}

int main(int argc, char **argv)
{
    long cases = argc > 1 ? atol(argv[1]) : 300000, naff = 0, nspan = 0, ndiff = 0, ndbl = 0, npix = 0;
    if (argc > 2)
        rs = strtoull(argv[2], NULL, 0);
    (void)warp_run, (void)warp_bands; // (WARPH and WARPV: checked on a board, tests/cmm2/warpn.bas)
    static const int depths[4] = {4, 8, 16, 32};
    for (long cs = 1; cs <= cases; cs++)
    {
        uint64_t seed = rs;
        int bpp = depths[ri(0, 3)], tmask = bpp == 4 ? 0xF : bpp == 8 ? 0xFF : bpp == 16 ? 0xFFFF : 0xFFFFFF;
        int alpha12 = bpp == 16 && chance(40); // the 12-bit depth: its pixels whose alpha is 0 are the transparent ones
        if (alpha12)
            tmask = 0xF000;
        int dw = (int)ri(1, 40), dh = (int)ri(1, 30), sw = dw, sh = dh;
        if (chance(50))
            sw = (int)ri(1, 50), sh = (int)ri(1, 40), ndiff++;
        int aff = chance(60), same = !aff && chance(20);
        page_t src, dst, want;
        page_new(&src, sw, sh, bpp, chance(25));
        page_new(&dst, dw, dh, bpp, chance(25));
        ndbl += src.dbl || dst.dbl;
        want = dst;
        want.p = malloc((size_t)dst.stride * dh);
        memcpy(want.p, dst.p, (size_t)dst.stride * dh);
        int step = chance(60) ? (int)ri(1, dh) : 1, n = (int)ri(1, dh / step + 3);
        int edge = chance(50) ? WARP_WRAP : chance(30) ? WARP_LEAVE : (int)(rnd() & (bpp == 32 ? 0x0FFFFFFF : tmask | 0xFFF));
        if (bpp < 16 && edge >= 0)
            edge &= tmask;
        int trans = chance(30) ? (int)(rnd() & 0x77777777 & tmask) : -1;
        if (alpha12 && trans >= 0)
            trans = 0;
        int64_t *arr = malloc(sizeof(int64_t) * n * 4);
        if (aff)
        {
            affine_t a = {.dst = dst.p, .src = src.p, .sw = sw, .sh = sh, .ss = src.stride, .dw = dw, .dh = dh,
                          .ds = dst.stride, .dup = dst.dbl ? dst.rb : 0, .bpp = bpp};
            int kind = (int)ri(0, 7);
            for (int k = 0; k < n; k++)
                setval(kind, sw, sh, &arr[k], &arr[n + k], &arr[2 * n + k], &arr[3 * n + k]);
            a.n = n, a.step = step, a.first = chance(60) ? far(dh) : 0;
            a.left = chance(50) ? far(dw) : 0;
            a.width = chance(50) ? (int)ri(1, dw + 20) : chance(10) ? WARP_FAR : dw;
            a.u = arr, a.v = arr + n, a.du = arr + 2 * n, a.dv = arr + 3 * n;
            a.edge = edge, a.trans = trans, a.tmask = tmask;
            affine_model(&want, &src, &a);
            affine_run(&a);
            naff++;
        }
        else
        {
            span_t a = {.dst = dst.p, .src = same ? NULL : src.p, .sw = sw, .sh = sh, .ss = src.stride, .dw = dw, .dh = dh,
                        .ds = dst.stride, .dup = dst.dbl ? dst.rb : 0, .bpp = bpp};
            for (int k = 0; k < n; k++)
            {
                arr[k] = chance(10) ? wild() : ri(-10, dw + 10);
                arr[n + k] = chance(10) ? wild() : ri(-10, dw + 10);
            }
            a.n = n, a.step = step, a.first = chance(60) ? far(dh) : 0;
            a.lf = arr, a.rt = arr + n;
            a.fill = edge == WARP_WRAP ? -1 : edge, a.trans = trans, a.tmask = tmask, a.outside = chance(50);
            span_model(&want, &src, &a, same);
            span_run(&a);
            nspan++;
        }
        npix += dw * dh;
        if (memcmp(want.p, dst.p, (size_t)dst.stride * dh))
        {
            int at = 0;
            while (want.p[at] == dst.p[at])
                at++;
            int y = at / dst.stride, o = at % dst.stride, copy = o >= dst.rb;
            printf("WRONG: case %ld (seed %llu) %s %d bits%s, source %dx%d%s, destination %dx%d%s, n %d step %d edge %d "
                   "trans %d\n  row %d, byte %d of its %s copy: got %02X, the model has %02X\n",
                   cs, (unsigned long long)seed, aff ? "AFFINE" : same ? "SPAN on one buffer" : "SPAN", bpp,
                   alpha12 ? " (12-bit alpha)" : "", sw, sh, src.dbl ? " doubled" : "", dw, dh, dst.dbl ? " doubled" : "",
                   n, step, edge, trans, y, copy ? o - dst.rb : o, copy ? "second" : "first", dst.p[at], want.p[at]);
            free(arr), free(src.p), free(dst.p), free(want.p);
            return 1;
        }
        free(arr), free(src.p), free(dst.p), free(want.p);
    }
    printf("%ld AFFINE and %ld SPAN cases (%ld with a source of another size, %ld with doubled rows), %ld pixels, "
           "%ld routine checks\n",
           naff, nspan, ndiff, ndbl, npix, routine_calls);
    printf("AFFINE AND SPAN HOST TEST PASSED\n");
    return 0;
}
