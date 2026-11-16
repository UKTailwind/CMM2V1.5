/* bt_heap.c: Bluetooth's heap (Inc/net/bt_heap.h): the first-fit allocator of Kernighan and Ritchie (The C Programming
   Language, 8.7) over BtBlock, a free list in address order whose neighbours join when freed, in 8-byte units.
   Started again over the same block at each radio start (WebStart), which is all a G1's restart after a --vram program
   needs: its wipe of the radio's data forgets every allocation with the list. With the block full, or before it is
   set, newlib's malloc stands in (free tells the two apart by address). */
#include "bt_heap.h"
#undef malloc                                   // (newlib's, here: this file is the macros' target)
#undef calloc
#undef realloc
#undef free

typedef union bt_hdr {
    struct {
        union bt_hdr *next;                     // the next free block, by address (the list is a circle through base)
        uint32_t units;                         // this block's size in units, the header's included
    } s;
    uint64_t align;
} bt_hdr_t;

static bt_hdr_t base;                           // the list's start: no size, below every block (D2 RAM < the SDRAM)
static bt_hdr_t *freep;                         // where the last search ended
static uint8_t *lo, *hi;                        // the block

void BtHeapInit(void *block, uint32_t size) {
    bt_hdr_t *p = (bt_hdr_t *)block;
    lo = (uint8_t *)block;
    hi = lo + size;
    p->s.units = size / sizeof(bt_hdr_t);
    p->s.next = &base;
    base.s.next = p;
    base.s.units = 0;
    freep = &base;
}

void *BtMalloc(size_t n) {
    if (lo == NULL || n == 0) return malloc(n ? n : 1);
    uint32_t units = (n + sizeof(bt_hdr_t) - 1) / sizeof(bt_hdr_t) + 1;
    bt_hdr_t *prev = freep, *p;
    for (p = prev->s.next;; prev = p, p = p->s.next) {
        if (p->s.units >= units) {
            if (p->s.units == units) {
                prev->s.next = p->s.next;
            } else {                            // (its top end given)
                p->s.units -= units;
                p += p->s.units;
                p->s.units = units;
            }
            freep = prev;
            return (void *)(p + 1);
        }
        if (p == freep) break;                  // round the list once: no room
    }
    return malloc(n);
}

void BtFree(void *ap) {
    if (ap == NULL) return;
    if ((uint8_t *)ap < lo || (uint8_t *)ap >= hi) {
        free(ap);
        return;
    }
    bt_hdr_t *bp = (bt_hdr_t *)ap - 1, *p;
    for (p = freep; !(bp > p && bp < p->s.next); p = p->s.next)
        if (p >= p->s.next && (bp > p || bp < p->s.next)) break;    // (at the list's end or start)
    if (bp + bp->s.units == p->s.next) {        // joined to the next
        bp->s.units += p->s.next->s.units;
        bp->s.next = p->s.next->s.next;
    } else {
        bp->s.next = p->s.next;
    }
    if (p + p->s.units == bp) {                 // and to the one before
        p->s.units += bp->s.units;
        p->s.next = bp->s.next;
    } else {
        p->s.next = bp;
    }
    freep = p;
}

void *BtCalloc(size_t n, size_t s) {
    if (s && n > (size_t)-1 / s) return NULL;
    void *p = BtMalloc(n * s);
    if (p) memset(p, 0, n * s);
    return p;
}

void *BtRealloc(void *ap, size_t n) {
    if (ap == NULL) return BtMalloc(n);
    if ((uint8_t *)ap < lo || (uint8_t *)ap >= hi) return realloc(ap, n);
    size_t have = (((bt_hdr_t *)ap - 1)->s.units - 1) * sizeof(bt_hdr_t);
    if (n <= have) return ap;
    void *p = BtMalloc(n);
    if (p) {
        memcpy(p, ap, have);
        BtFree(ap);
    }
    return p;
}

uint32_t BtHeapFree(void) {
    uint32_t f = 0;
    if (lo == NULL) return 0;
    for (bt_hdr_t *p = base.s.next; p != &base; p = p->s.next) f += p->s.units * sizeof(bt_hdr_t);
    return f;
}
