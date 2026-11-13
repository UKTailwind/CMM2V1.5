/* wg_malloc.c: WordGrinder's malloc (apps/wordgrinder), dlmalloc in place of newlib-nano's. MIT License, as WordGrinder;
   dlmalloc.c is Doug Lea's dlmalloc 2.8.6, unchanged, which he put in the public domain (CC0).

   WordGrinder's spelling loads the whole word list into a Lua table: about 16 MB for its British dictionary of
   171,840 words, more than the program's own heap (about 7 MB on a G2). So when that is used up, this malloc goes on
   in blocks of MMBasic's heap, which the alloc service lends (as Quake and the Amiga take theirs) and the loader frees
   when the program ends: on a G2 at the command prompt about 15 MB more (Peter, 12 Nov 2026: "The G2 should have
   memory you can use in the MMBasic heap"). And newlib-nano's free() searches its whole list of free blocks for each
   one: closing the Lua state at the end, some 350,000 objects, took hours. dlmalloc keeps its free blocks in bins by
   size.

   It is WordGrinder's alone (Peter, 12 Nov 2026): as the SDK's malloc for every program it was no gain to the others,
   and zeptoforth's solar eclipse ran 8% slower with its start-up buffers placed by it. The SDK and its sbrk are as
   they were; this file gives newlib its reentrant entry points (_malloc_r and the rest), so newlib's own allocator is
   not linked into WordGrinder. */
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
   room; then blocks of MMBasic's heap, a megabyte, or the piece asked for if that is more, or half and so on down to
   the piece when there is not a megabyte to have (a program has 64 blocks at most); a block's rest is there for the
   next pieces. KEEP stays free in MMBasic's heap, for the firmware while the program runs (files, BASIC commands). */
#define BLOCK 0x100000u
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

static void *morecore(ptrdiff_t incr) {
    char *b, *prev;
    uint32_t size;
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
    for(size = (uint32_t)incr > BLOCK ? (uint32_t)incr : BLOCK; (b = borrow(size)) == NULL;
        size = size / 2 > (uint32_t)incr ? size / 2 : (uint32_t)incr)
        if(size == (uint32_t)incr) {
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
   first, so that the message is seen, and it waits for a key, as apps/pdcurses' at_exit does: WordGrinder set a
   video mode, so the firmware clears the screen as the program ends (Loader.c's ld_screenback), and the message would
   go with it (13 Nov 2026, review of 1468d08). _exit, not exit: the heap is damaged, so atexit's work is skipped. */
static void __attribute__((noreturn)) heap_damaged(void) {
    if(cmm2->version >= 4) cmm2->con_screen(1);
    if(cmm2->version >= 10) cmm2->gfx_cls(0);
    cmm2->con_puts("\r\nWordGrinder: its memory is damaged (dlmalloc found a bad block), so it stops here.\r\n"
                   "Press a key for MMBasic.\r\n");
    while(cmm2->con_getc() < 0 && !cmm2->break_take()) cmm2->poll();
    _exit(1);
}
#define ABORT heap_damaged()

#define STRUCT_MALLINFO_DECLARED 1
#define MALLINFO_FIELD_TYPE size_t
#define USE_DL_PREFIX 1             /* dlmalloc's functions are dlmalloc(), dlfree()... */
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

void *_malloc_r(struct _reent *r, size_t n) { (void)r; return dlmalloc(n); }
void _free_r(struct _reent *r, void *p) { (void)r; dlfree(p); }
void *_calloc_r(struct _reent *r, size_t n, size_t size) { (void)r; return dlcalloc(n, size); }
void *_realloc_r(struct _reent *r, void *p, size_t n) { (void)r; return dlrealloc(p, n); }
void *_memalign_r(struct _reent *r, size_t align, size_t n) { (void)r; return dlmemalign(align, n); }
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
