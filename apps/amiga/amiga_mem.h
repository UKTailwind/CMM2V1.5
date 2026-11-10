/* amiga_mem.h: the Omega core's 16 MB low16Meg (an array on a PC) in the CMM2's memory. Only these parts of the
   Amiga's 24-bit space are memory that frank-amiga's native code reads or writes as low16Meg: chip RAM (2 MB, as far as
   its masks reach; the A500 has 512 KB, mirrored), slow RAM at 0xC00000 (512 KB), the extended ROM at 0xE00000 (512 KB,
   AROS's second half) and the Kickstart at 0xF80000 (512 KB). patches/0001-cmm2.patch turns each low16Meg[x] into AM(x).
   The core's diagnostics (printf in memory_native.c and the rest) are dropped: the PC runner prints a line for some
   register accesses each time they happen (Lotus: 14,000 in a race); amiga_cmm2.c, whose lines are wanted, #undefs it.
   (Peter, 9 Nov 2026) */
#ifndef AMIGA_MEM_H
#define AMIGA_MEM_H
#include <stdint.h>
#include <stdio.h>
extern unsigned char amiga_chip[], amiga_slow[], amiga_ext[], amiga_rom[], amiga_none[];
static inline unsigned char *amiga_map(uint32_t a) {
    if (a < 0x200000u) return amiga_chip + a;
    if (a >= 0xF80000u) return amiga_rom + ((a - 0xF80000u) & 0x7FFFFu);
    if (a - 0xE00000u < 0x80000u) return amiga_ext + (a - 0xE00000u);
    if (a - 0xC00000u < 0x80000u) return amiga_slow + (a - 0xC00000u);
    return amiga_none;                          /* (nothing else is low16Meg memory in this configuration) */
}
#define AM(a) (*amiga_map((uint32_t)(a)))
#ifndef AMIGA_LOUD
#define printf(...) ((void)0)
#define puts(s) ((void)0)
#endif
#endif
