/* browser_malloc.c: the browser's malloc (apps/browser), dlmalloc in place of newlib-nano's, as WordGrinder's
   (apps/wordgrinder/wg_malloc.c, which this copies: a change for one program stays in that program). dlmalloc.c is Doug
   Lea's dlmalloc 2.8.6, unchanged, which he put in the public domain (CC0). MIT License, as Inc/cmm2img.h.

   A page, its style sheets and its images can want more than the program's own heap (about 5.5 MB beside the browser's
   code in a G2's 8 MB: the Wikipedia page Colour_Maximite reached it, 27 Nov 2026). So when that is used up, this
   malloc goes on in blocks of MMBasic's heap, which the alloc service lends and the loader frees when the program
   ends: on a G2 or G3 at the command prompt about 15 MB more. dlmalloc keeps its free blocks in bins by size, where
   newlib-nano's free() searches a list. */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>                 /* newlib's struct mallinfo, which dlmalloc fills in */
#include <unistd.h>
#include "cmm2.h"

void *_sbrk(ptrdiff_t incr);        /* (the SDK's syscalls.c: the program's own heap, from its .bss up) */

/* dlmalloc's memory, a piece at a time, each answer taken as the next piece of its heap and joined to the one before
   when it follows on (sbrk(0) is where the last piece ended). The program's own heap by the SDK's sbrk while it has
   room; then blocks of MMBasic's heap, each the largest it can lend (borrow_most), at least the piece asked for (a
   program has 64 blocks at most); a block's rest is there for the next pieces. KEEP stays free in MMBasic's heap, for
   the firmware while the program runs (files, BASIC commands). */
#define KEEP  0x80000u
static char *blo, *bbrk, *bend;     /* the last block of MMBasic's heap had, and how much of it is used */
static int in_block;                /* the last piece was from that block */

/* size bytes of MMBasic's heap, if KEEP would still be free after */
static char *borrow(uint32_t size) {
    void *b = cmm2->alloc(size), *k;
    if(b == NULL) return NULL;
    if((k = cmm2->alloc(KEEP)) == NULL) {
        cmm2->release(b);
        return NULL;
    }
    cmm2->release(k);
    return b;
}

/* The largest block of MMBasic's heap that can be had with KEEP still free, of incr at least, found to 256 KB by a
   binary search up to 64 MB: dlmalloc cannot join blocks, so its free memory stays in as few pieces as there are blocks.
   In blocks of a megabyte a page's larger requests failed with most of the heap free (28 Nov 2026, the G3: 320
   requests not met with 4.9 MB in use of the 18.7 MB had). */
#define BIG (64u << 20)
#define STEP (256u << 10)
static char *borrow_most(uint32_t incr, uint32_t *got) {
    uint32_t lo, hi = BIG, size;
    char *b;
    if(incr > BIG || (b = borrow(incr)) == NULL) return NULL;
    cmm2->release(b);
    lo = incr;                                  /* (a size that can be had; hi one that cannot, or the most tried) */
    while(hi - lo > STEP) {
        size = lo + (((hi - lo) / 2) & ~(STEP - 1));
        if(size <= lo) size = lo + STEP;
        if((b = borrow(size)) != NULL) {
            cmm2->release(b);
            lo = size;
        } else hi = size;
    }
    if((b = borrow(lo)) == NULL) return NULL;
    *got = lo;
    {                                           /* (28 Nov 2026, on trial: each block of MMBasic's heap had) */
        extern void cmm2_note_block(void *b, uint32_t size);
        cmm2_note_block(b, lo);
    }
    return b;
}

/* a request of the big blocks' pool (below) under way: one it cannot meet is not out of memory, as the main heap is
   tried next; and the pool is not grown. dlmalloc grows any of its spaces by MORECORE, so a pool that filled took
   MMBasic's heap for itself, and blocks there, not where the pool was lent, went to the main heap's free: "a block
   given back" (28 Nov 2026, the first Wikipedia page) */
static int in_big;

static void *morecore(ptrdiff_t incr) {
    char *b, *prev;
    uint32_t size;
    if(in_big) {                    /* (the pool's own: it stays the memory lent, see above) */
        errno = ENOMEM;
        return (void *)-1;
    }
    if(incr == 0) return in_block ? bbrk : _sbrk(0);
    if(incr < 0) {                  /* (never: MORECORE_CANNOT_TRIM) */
        errno = ENOMEM;
        return (void *)-1;
    }
    if(in_block && incr <= bend - bbrk) {
        prev = bbrk;
        bbrk += incr;
        return prev;
    }
    if((prev = _sbrk(incr)) != (void *)-1) {
        in_block = 0;
        return prev;
    }
    if(bbrk && incr <= bend - bbrk) {
        in_block = 1;
        prev = bbrk;
        bbrk += incr;
        return prev;
    }
    if((b = borrow_most((uint32_t)incr, &size)) == NULL) {
        errno = ENOMEM;
        return (void *)-1;
    }
    in_block = 1;
    blo = b;
    bend = b + size;
    bbrk = b + incr;
    return b;
}

/* dlmalloc's ABORT, when it finds a block damaged, or is given one it never gave out: newlib's abort() ended the
   program with code 1 and nothing said (Peter's review, 12 Nov 2026). The console is put back over curses' screen
   first, so that the message is seen, and it waits for a key, as apps/pdcurses' at_exit does: the browser set a
   video mode, so the firmware clears the screen as the program ends (Loader.c's ld_screenback), and the message would
   go with it (13 Nov 2026, review of 1468d08). _exit, not exit: the heap is damaged, so atexit's work is skipped. */
/* (numbers into text without printf, which the damaged heap might not survive) */
static char *put_u(char *p, unsigned long v) {
    char t[12];
    int n = 0;
    do t[n++] = (char)('0' + v % 10); while((v /= 10) != 0);
    while(n) *p++ = t[--n];
    return p;
}

static char *put_s(char *p, const char *s) {
    while(*s) *p++ = *s++;
    return p;
}

static char *put_x(char *p, uint32_t v) {
    for(int i = 28; i >= 0; i -= 4) *p++ = "0123456789ABCDEF"[(v >> i) & 15];
    return p;
}

/* (borrow_most's, 28 Nov 2026, on trial: each block of MMBasic's heap the main heap has, on the console) */
void cmm2_note_block(void *b, uint32_t size) {
    char t[100], *p = t;
    p = put_s(p, "browser: the heap borrows &H");
    p = put_x(p, (uint32_t)(uintptr_t)b);
    p = put_s(p, ", ");
    p = put_u(p, (unsigned long)(size / 1024));
    p = put_s(p, " KB of MMBasic's heap\r\n");
    *p = 0;
    cmm2->con_puts(t);
}

/* the code that called free or realloc last (the link's --wrap=free and --wrap=realloc, make.py): a block given back
   that dlmalloc never gave out names its caller, for addr2line on out/browser.elf */
static void *last_caller;

/* what dlmalloc's check found (28 Nov 2026, a heap damaged after a page came up blank and Back was pressed): which
   check (its line in dlmalloc.c), the block, and the code that called free or realloc last */
static void __attribute__((noreturn)) heap_damaged_at(const char *what, void *blk, int line) {
    char b[240], *p = b;
    if(cmm2->version >= 4) cmm2->con_screen(1);
    if(cmm2->version >= 10) cmm2->gfx_cls(0);
    p = put_s(p, "\r\nThe browser: its memory is damaged (dlmalloc found a bad block), so it stops here.\r\n(");
    p = put_s(p, what);
    p = put_s(p, ", dlmalloc.c line ");
    p = put_u(p, (unsigned long)line);
    if(blk != NULL) {
        p = put_s(p, ", block &H");
        p = put_x(p, (uint32_t)(uintptr_t)blk);
    }
    p = put_s(p, ", the last free or realloc from &H");
    p = put_x(p, (uint32_t)(uintptr_t)last_caller);
    p = put_s(p, ")\r\nPress a key for MMBasic.\r\n");
    *p = 0;
    cmm2->con_puts(b);
    while(cmm2->con_getc() < 0 && !cmm2->break_take()) cmm2->poll();
    _exit(1);
}
#define ABORT heap_damaged_at("abort", NULL, __LINE__)
#define CORRUPTION_ERROR_ACTION(m) heap_damaged_at("its own lists", NULL, __LINE__)
#define USAGE_ERROR_ACTION(m, p) heap_damaged_at("a block given back", (p), __LINE__)

/* a request dlmalloc could not meet, on the console (the serial one while NetSurf has the screen), at most twice a
   second: a page that comes up blank, or goes wrong after, may have run out of memory */
static size_t dlmalloc_footprint(void);
static struct mallinfo dlmallinfo(void);
static size_t want;                /* the request being met (the wrappers below): out_of_memory says it */
static void *want_caller;           /* and the code that asked (--wrap=malloc, calloc and realloc) */
static unsigned long count;         /* the requests not met so far */
static unsigned long count_small;   /* and those of them under BIG_MIN */

/* the big blocks: a request of this or more is the pool's first (below), the images' bitmaps and the page's buffer.
   (64 KB filled the pool with the memory cache's pieces of the pages' data, and a 3.4 MB request found no room.) */
#define BIG_MIN (512u * 1024u)

/* (cmm2_main.c's cmm2_memory_poll: a page that ran out of memory is stopped, and the error page shown.) Only the
   requests under BIG_MIN count: a big one not met is an image's bitmap, which NetSurf goes without (the image not
   drawn), and the page stays. Nor do those made while cmm2_optional_alloc is set: the local history's thumbnail of a
   page, drawn when it is done (patches/netsurf-0006), which the page goes without. The error page for that took away
   a page that had been shown whole (28 Nov 2026, the Wikipedia page BASIC: "I saw the basic page and then it
   disappeared", Peter: the thumbnail's 3.4 MB buffer, now 220 KB) */
int cmm2_optional_alloc;

unsigned long cmm2_failed_requests(void) {
    return count_small;
}


static void out_of_memory(void) {
    static uint64_t last;
    if(in_big) {
        errno = ENOMEM;
        return;
    }
    char b[220], *p = b;
    errno = ENOMEM;
    count++;
    if(want < BIG_MIN && !cmm2_optional_alloc) count_small++;
    if(cmm2->micros() - last < 500000u) return;
    last = cmm2->micros();
    p = put_s(p, "browser: out of memory: ");
    p = put_u(p, (unsigned long)want);
    p = put_s(p, " bytes asked for from &H");
    p = put_x(p, (uint32_t)(uintptr_t)want_caller);
    p = put_s(p, " (");
    p = put_u(p, count);
    p = put_s(p, " requests not met so far); the heap has ");
    p = put_u(p, (unsigned long)(dlmalloc_footprint() / 1024));
    p = put_s(p, " KB from the system");
    if(count <= 3) {                            /* (what is free, and in how many pieces: the walk is long) */
        struct mallinfo m = dlmallinfo();
        p = put_s(p, ", ");
        p = put_u(p, (unsigned long)(m.uordblks / 1024));
        p = put_s(p, " KB in use, ");
        p = put_u(p, (unsigned long)(m.fordblks / 1024));
        p = put_s(p, " KB free in ");
        p = put_u(p, (unsigned long)m.ordblks);
        p = put_s(p, " pieces");
    }
    p = put_s(p, "\r\n");
    *p = 0;
    cmm2->con_puts(b);
}
#define MALLOC_FAILURE_ACTION out_of_memory()

#define STRUCT_MALLINFO_DECLARED 1
#define MALLINFO_FIELD_TYPE size_t
#define USE_DL_PREFIX 1             /* dlmalloc's functions are dlmalloc(), dlfree()... */
#define MSPACES 1                   /* and spaces of their own: the big blocks' pool, below */
#define DLMALLOC_EXPORT static      /* and are this file's alone */
#define HAVE_MMAP 0
#define HAVE_MREMAP 0
#define HAVE_MORECORE 1
#define MORECORE morecore
#define MORECORE_CONTIGUOUS 0       /* the pieces may be anywhere */
#define MORECORE_CANNOT_TRIM 1      /* and are never given back before the program ends */
#define DEFAULT_GRANULARITY ((size_t)64U * (size_t)1024U)
#define malloc_getpagesize ((size_t)4096U)
#define USE_LOCKS 0
#define NO_MALLOC_STATS 1           /* (malloc_stats below, by mallinfo) */
#define LACKS_SYS_MMAN_H 1
#define LACKS_SCHED_H 1
#define LACKS_TIME_H 1              /* (its magic number from an address, not from the clock's service) */

#undef M_TRIM_THRESHOLD             /* (mallopt's: dlmalloc's, the same numbers as newlib's) */
#undef M_MMAP_THRESHOLD
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#include "dlmalloc.c"               /* (the warnings it gives as it is, off to its end and after) */

void __real_free(void *p);
void *__real_realloc(void *p, size_t n);
void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t size);
void __wrap_free(void *p) { last_caller = __builtin_return_address(0); __real_free(p); }
void *__wrap_realloc(void *p, size_t n) {
    want_caller = last_caller = __builtin_return_address(0);
    return __real_realloc(p, n);
}
void *__wrap_malloc(size_t n) { want_caller = __builtin_return_address(0); return __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t size) { want_caller = __builtin_return_address(0); return __real_calloc(n, size); }

/* The big blocks' pool (28 Nov 2026, Peter: "free more memory by stealing video memory since we only need one page for
   the display"): a G2 or G3 lends a program built --pages the display memory not shown as heap2 (5.5 MB in MODE 9,16,
   Src/Loader.c). Requests of BIG_MIN and more come from a dlmalloc space of their own there: the images' bitmaps (one
   of 3.4 MB on the Wikipedia page Maximite), the page's buffer, and the pages' data as it grows. So they neither use
   up the main heap nor leave it in pieces: a 3.4 MB bitmap had failed with 14 MB of the main heap free. The main heap
   takes them when the pool is full, or not there (a G1, or a mode whose display memory cannot be lent). */
#include <string.h>
static mspace big;
static char *big_lo, *big_hi;
static int big_tried;
#define IN_BIG(p) ((char *)(p) >= big_lo && (char *)(p) < big_hi)

int cmm2_no_big;                    /* (cmm2_main.c's --cmm2-nobig: the pool left unused, for a test) */

static void big_init(void) {
    char b[120], *p = b;
    big_tried = 1;
    if(cmm2_start == NULL || cmm2_start->heap2 == NULL || cmm2_start->heap2_size < (1u << 20) || cmm2_no_big) return;
    big = create_mspace_with_base(cmm2_start->heap2, cmm2_start->heap2_size, 0);
    if(big != NULL) {
        big_lo = cmm2_start->heap2;
        big_hi = big_lo + cmm2_start->heap2_size;
    }
    p = put_s(p, "browser: big blocks in the display memory at &H");    /* (28 Nov 2026, on trial: what was lent) */
    p = put_x(p, (uint32_t)(uintptr_t)cmm2_start->heap2);
    p = put_s(p, ", ");
    p = put_u(p, (unsigned long)(cmm2_start->heap2_size / 1024));
    p = put_s(p, big != NULL ? " KB\r\n" : " KB: no space made there\r\n");
    *p = 0;
    cmm2->con_puts(b);
}

/* n bytes from the pool, or NULL (no out of memory said: the main heap is tried next) */
static void *big_alloc(size_t n) {
    void *p;
    if(n < BIG_MIN) return NULL;
    if(!big_tried) big_init();                  /* (at the first big request: after main has read its words) */
    if(big == NULL) return NULL;
    in_big = 1;
    p = mspace_malloc(big, n);
    in_big = 0;
    return p;
}

/* n bytes from the pool whatever their size, or else from the main heap: patches/netsurf-0004's fonts, each face's
   file whole, kept for the program's life (330 to 760 KB each, 4 MB for the eight). In the display memory they leave
   the main heap to the pages (28 Nov 2026, Peter: "can the fonts use the vram?"; the Wikipedia page BASIC, which fits
   the main heap with 0.4 MB to spare, did not when it was gone back to) */
void *cmm2_malloc_pool(size_t n) {
    void *p;
    if(!big_tried) big_init();
    if(big != NULL) {
        in_big = 1;
        p = mspace_malloc(big, n);
        in_big = 0;
        if(p != NULL) return p;
    }
    return malloc(n);
}

/* (cmm2_main.c's memory line: the pool's use, and its size; 0 and 0 if there is none) */
void cmm2_big_use(unsigned long *in_use, unsigned long *size) {
    *in_use = big != NULL ? (unsigned long)mspace_mallinfo(big).uordblks : 0;
    *size = big != NULL ? (unsigned long)(big_hi - big_lo) : 0;
}

void *_malloc_r(struct _reent *r, size_t n) {
    void *p;
    (void)r;
    want = n;
    return (p = big_alloc(n)) != NULL ? p : dlmalloc(n);
}

void _free_r(struct _reent *r, void *p) {
    (void)r;
    if(p != NULL && big != NULL && IN_BIG(p)) mspace_free(big, p);
    else dlfree(p);
}

void *_calloc_r(struct _reent *r, size_t n, size_t size) {
    void *p;
    (void)r;
    want = n * size;
    if(size != 0 && want / size == n && (p = big_alloc(want)) != NULL) return memset(p, 0, want);
    return dlcalloc(n, size);
}

/* a block grown past BIG_MIN moves to the pool; one in the pool that cannot grow there moves to the main heap */
void *_realloc_r(struct _reent *r, void *p, size_t n) {
    void *q;
    size_t had;
    (void)r;
    want = n;
    if(p == NULL) return _malloc_r(r, n);
    if(n == 0) {
        _free_r(r, p);
        return NULL;
    }
    had = dlmalloc_usable_size(p);              /* (either space's: its chunks are made alike) */
    if(big != NULL && IN_BIG(p)) {
        in_big = 1;
        q = mspace_realloc(big, p, n);
        in_big = 0;
        if(q != NULL) return q;
        if((q = dlmalloc(n)) == NULL) return NULL;
        memcpy(q, p, had < n ? had : n);
        mspace_free(big, p);
        return q;
    }
    if(n >= BIG_MIN && n > had && (q = big_alloc(n)) != NULL) {
        memcpy(q, p, had);
        dlfree(p);
        return q;
    }
    return dlrealloc(p, n);
}

void *_memalign_r(struct _reent *r, size_t align, size_t n) {
    void *p = NULL;
    (void)r;
    want = n;
    if(n >= BIG_MIN && !big_tried) big_init();
    if(big != NULL && n >= BIG_MIN) {
        in_big = 1;
        p = mspace_memalign(big, align, n);
        in_big = 0;
    }
    return p != NULL ? p : dlmemalign(align, n);
}
void *_valloc_r(struct _reent *r, size_t n) { (void)r; return dlvalloc(n); }
void *_pvalloc_r(struct _reent *r, size_t n) { (void)r; return dlpvalloc(n); }
size_t _malloc_usable_size_r(struct _reent *r, void *p) { (void)r; return dlmalloc_usable_size(p); }
int _malloc_trim_r(struct _reent *r, size_t pad) { (void)r; return dlmalloc_trim(pad); }
int _mallopt_r(struct _reent *r, int param, int value) { (void)r; return dlmallopt(param, value); }
struct mallinfo _mallinfo_r(struct _reent *r) { (void)r; return dlmallinfo(); }

/* mallinfo's sums, on stderr, as newlib's malloc_stats() gives them */
void _malloc_stats_r(struct _reent *r) {
    struct mallinfo m = dlmallinfo();
    (void)r;
    fprintf(stderr, "max system bytes = %10lu\nsystem bytes     = %10lu\nin use bytes     = %10lu\n",
            (unsigned long)dlmalloc_max_footprint(), (unsigned long)m.arena, (unsigned long)m.uordblks);
}
