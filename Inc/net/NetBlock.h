/* NetBlock.h: the network's memory on the Colour Maximite 2 (docs/Web_Plan.md: the network block, Peter 15 Nov 2026).
   128 KB of SDRAM, taken at start-up only when OPTION WIFI is set: on a G1 the top of the display memory (0xD02E0000),
   on a G2 or G3 the top of MMBasic's heap, below the executable window. lwIP's heap is laid over it (lwipopts.h:
   LWIP_RAM_HEAP_POINTER), its pools come from that heap, and so do the net code's own long-lived blocks. Read and
   write only: nothing runs from it. NULL while Wi-Fi is off. */
#ifndef NETBLOCK_H
#define NETBLOCK_H
#include <stdint.h>

#define NET_BLOCK_SIZE 0x20000                  // (a plain number: lwIP's #if tests MEM_SIZE, made from it)
#define NET_BLOCK_G1   0xD02E0000               // a G1's: the display memory's top 128 KB

extern uint8_t *NetBlock;

#endif
