/* bt_heap.h: Bluetooth's heap (Src/net/bt/bt_heap.c). BTstack, its glue (Src/net/bt) and the PicoMite's Bluetooth host
   (Src/net/bt/picomite, through BTMite_cmm2.h) take their memory with malloc, calloc, realloc and free, which this
   header, made their first include by CMakeLists.txt, sends to a block of Bluetooth's own in MMBasic's SDRAM heap
   (BT_BLOCK_SIZE, kept across RUN and NEW as the network block is: Src/net/WebMite_cmm2.c's BtBlock).

   Not newlib's heap: its arena grows up from the SDRAM heap's bottom, and a G1's loader joins the display memory with
   the heap's bottom for its largest programs (CC, the NES emulator) only while that arena is unused (Loader.c). */
#ifndef BT_HEAP_H
#define BT_HEAP_H
#include <stdlib.h>                             // (first: its prototypes must not meet the macros below)
#include <string.h>
#include <stdint.h>

#define BT_BLOCK_SIZE 0x10000                   // 64 KB: BTstack's pools and connections, and the speaker's 16 KB ring
void BtHeapInit(void *block, uint32_t size);
void *BtMalloc(size_t n);
void *BtCalloc(size_t n, size_t s);
void *BtRealloc(void *p, size_t n);
void BtFree(void *p);
uint32_t BtHeapFree(void);                      // (the bytes free in the block, for BLUETOOTH STATUS's check)
extern uint8_t *BtBlock;                        // (Src/net/WebMite_cmm2.c: NULL with Bluetooth's half off)

#ifndef BT_HEAP_NO_MACROS                        // (a file that only needs the declarations: Src/net/WebMite_cmm2.c)
#undef malloc
#undef calloc
#undef realloc
#undef free
#define malloc(n) BtMalloc(n)
#define calloc(n, s) BtCalloc(n, s)
#define realloc(p, n) BtRealloc(p, n)
#define free(p) BtFree(p)
#endif
#endif
