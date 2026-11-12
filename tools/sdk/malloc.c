/* malloc.c: the C library's malloc for a loadable program (tools/sdk), dlmalloc in place of newlib's. MIT License, as
   cmm2img.h; dlmalloc.c is Doug Lea's dlmalloc 2.8.6, unchanged, which he put in the public domain (CC0).

   newlib-nano's free() looks through its list of free blocks, in address order, for the place of each one: fine for a
   few hundred, but a program that frees hundreds of thousands (WordGrinder's Lua, closing a state that holds a 171,840
   word dictionary) ran for hours (12 Nov 2026). dlmalloc keeps its free blocks in bins by size, so malloc and free
   take about the same time however many there are, and it takes memory from sbrk in pieces anywhere, each one joined
   to the last when it follows on: the start block's heap first, and then blocks of MMBasic's heap (_sbrk, below).
   sbrk and cmm2_heap_take are here, not in syscalls.c, so that a program built without this file does not link:
   newlib-nano's malloc on this sbrk would take a piece from elsewhere as the continuation of its last free chunk.

   newlib's own functions (stdio's buffers, strdup, C++'s new) call the reentrant entry points, _malloc_r and the rest:
   all of them are here, so none of newlib's allocator is linked, nano or whole (--libc full). Its malloc(), free(),
   calloc() and the other plain names stay newlib's, and call these. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>                 /* newlib's struct mallinfo, which dlmalloc fills in */
#include "cmm2.h"

/* malloc's memory: sbrk, which dlmalloc (below) asks for a piece at a time, taking each answer as the next piece of
   its heap, joined to the one before when it follows on. First the start block's heap, from the program's .bss up to
   its top, which cmm2_heap_take lowers. When that is used up, blocks of MMBasic's own heap, which the alloc service
   lends (as Quake and the Amiga take theirs) and the loader frees when the program ends: on a G2 at the command
   prompt about 15 MB more than the 7 MB or so of its own (12 Nov 2026, Peter: "the G2 should have memory you can use
   in the MMBasic heap"). A block is a megabyte, or the piece asked for if that is more, or half and so on down to the
   piece when there is not a megabyte to have (a program has 64 blocks at most); its rest is there for the next
   pieces. KEEP stays free in MMBasic's heap, for the firmware while the program runs (its files, BASIC commands,
   sounds). */
#define BLOCK 0x100000u
#define KEEP  0x80000u
static char *abrk;                                                  // malloc's top in the start block's heap
static char *blo, *bbrk, *bend;                                     // and in the last block of MMBasic's it has had
static int in_block;                                                // the last piece was from that block

/* size bytes (rounded up to 8) taken from the top of the heap, for a program's own allocator; malloc keeps the rest.
   NULL if malloc has already taken too much. */
void *cmm2_heap_take(uint32_t size) {
    char *top = (char *)cmm2_start->heap + cmm2_start->heap_size;
    size = (size + 7) & ~7u;
    if(abrk == NULL) abrk = cmm2_start->heap;
    if(size > (uint32_t)(top - abrk)) return NULL;
    cmm2_start->heap_size -= size;
    return top - size;
}

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

/* sbrk(0) is where the last piece ended (dlmalloc's sbrk(n), sbrk(0) is the piece it has), and a piece comes from
   there if there is room, else from the start block's heap or the block: so the start block's heap is used however
   large the first request (an 8 MB one went to a block, and the start block's heap was then left unused) */
void *_sbrk(ptrdiff_t incr) {
    char *aend, *b, *prev;
    uint32_t size;
    if(abrk == NULL) abrk = cmm2_start->heap;
    aend = (char *)cmm2_start->heap + cmm2_start->heap_size;
    if(incr <= 0) {                                                 // (back from the top of the last piece's place)
        char **top = in_block ? &bbrk : &abrk, *lo = in_block ? blo : (char *)cmm2_start->heap;
        if(incr < lo - *top) {
            errno = ENOMEM;
            return (void *)-1;
        }
        prev = *top;
        *top += incr;
        return prev;
    }
    if(in_block && incr <= bend - bbrk) {
        prev = bbrk;
        bbrk += incr;
        return prev;
    }
    if(incr <= aend - abrk) {
        in_block = 0;
        prev = abrk;
        abrk += incr;
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

#define STRUCT_MALLINFO_DECLARED 1
#define MALLINFO_FIELD_TYPE size_t
#define USE_DL_PREFIX 1             /* dlmalloc's functions are dlmalloc(), dlfree()... */
#define DLMALLOC_EXPORT static      /* and are this file's alone (apps that build a dlmalloc of their own keep it) */
#define HAVE_MMAP 0
#define HAVE_MREMAP 0
#define HAVE_MORECORE 1
#define MORECORE _sbrk
#define MORECORE_CONTIGUOUS 0       /* sbrk's pieces may be anywhere */
#define MORECORE_CANNOT_TRIM 1      /* and are never given back before the program ends */
#define DEFAULT_GRANULARITY ((size_t)64U * (size_t)1024U)
#define malloc_getpagesize ((size_t)4096U)
#define USE_LOCKS 0                 /* (one thread; a frame hook or sound callback must not malloc) */
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
#include "dlmalloc.c"                      /* (the warnings it gives as it is, off to its end and after) */

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
