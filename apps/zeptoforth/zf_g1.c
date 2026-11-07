/* zf_g1.c: the start of the G1's build (make.py). A saved image holds code compiled against fixed addresses, and a G1's loader puts a program in the heap's largest free run, which
   moves with the options. So this build is linked at a fixed place in that run, has no relocations (mkimage.py --fixed), and starts here, wherever the loader put it: the program is
   moved up to where it is linked, and started there. MIT licence, as zeptoforth.

   Nothing here may use an address of the program until it has moved: only what the loader passed, and addresses reckoned from it. The function runs from the program's first bytes,
   so the move, working down from the top, does not reach it: the two places differ by at least 64 KB, the loader's step, or not at all. */
#include "cmm2.h"

#define SCB_CCSIDR  (*(volatile uint32_t *)0xE000ED80)
#define SCB_CSSELR  (*(volatile uint32_t *)0xE000ED84)
#define SCB_ICIALLU (*(volatile uint32_t *)0xE000EF50)
#define SCB_DCCSW   (*(volatile uint32_t *)0xE000EF6C)

extern char __image_start[], __image_end[], __bss_end__[];
extern const uint32_t zf_mem[5];
extern int _start(cmm2img_start *s);

int zf_g1_start(cmm2img_start *s) __attribute__((section(".text.entry"), used));

static inline void hex(cmm2img_start *s, uint32_t v) {
    int i;
    for(i = 28; i >= 0; i -= 4) {
        int d = v >> i & 15;
        s->svc->con_putc(d < 10 ? '0' + d : 'A' + d - 10);
    }
    s->svc->con_putc(' ');
}

int zf_g1_start(cmm2img_start *s) {
    static const char refused[] = "zeptoforth: the memory it is linked for is not free. It has, then needs: ";
    char *to = __image_start, *from = (char *)s->base;
    uint32_t heap_end = (uint32_t)s->heap + s->heap_size, ccsidr, sets, ways, w;
    uint32_t words = (__image_end - __image_start + 3) / 4;
    const uint32_t *mem = (const uint32_t *)(from + ((const char *)zf_mem - to));
    volatile uint32_t *src = (volatile uint32_t *)from, *dst = (volatile uint32_t *)to;
    int (*volatile go)(cmm2img_start *) = _start;
    if(from > to || heap_end < mem[3]) {
        s->svc->con_puts(from + (refused - to));
        hex(s, (uint32_t)from);
        hex(s, heap_end);
        hex(s, (uint32_t)to);
        hex(s, mem[3]);
        s->svc->con_putc('\r');
        s->svc->con_putc('\n');
        return 1;
    }
    if(from != to) {
        while(words--) dst[words] = src[words];
    }
    for(dst = (volatile uint32_t *)((uint32_t)(__image_end + 3) & ~3u); dst < (volatile uint32_t *)__bss_end__; dst++) *dst = 0;
    s->heap = (void *)(((uint32_t)__bss_end__ + 7) & ~7u);
    s->heap_size = heap_end - (uint32_t)s->heap;
    s->base = (uint32_t)to;
    SCB_CSSELR = 0;                             /* what the data cache holds out to the memory, and the instruction cache emptied: zf_sync_code's, which has not moved yet */
    __asm volatile("dsb" ::: "memory");
    ccsidr = SCB_CCSIDR;
    sets = ccsidr >> 13 & 0x7FFF;
    ways = ccsidr >> 3 & 0x3FF;
    do {
        w = ways;
        do {
            SCB_DCCSW = (sets << 5 & 0x3FE0) | (w << 30);
        } while(w--);
    } while(sets--);
    __asm volatile("dsb" ::: "memory");
    SCB_ICIALLU = 0;
    __asm volatile("dsb\n\tisb" ::: "memory");
    return go(s);
}
