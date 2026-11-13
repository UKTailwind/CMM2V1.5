/***************************************************************************
CMM2 MMBasic
Memory.h

Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The name MMBasic be used when referring to the interpreter in any
   documentation and promotional material and the original copyright message
  be displayed  on the console at startup (additional copyright messages may
   be added).

5. All advertising materials mentioning features or use of this software must
   display the following acknowledgement: This product includes software
   developed by Geoff Graham and Peter Mather.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/



#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)
// General definitions used by other modules

#ifndef MEMORY_HEADER
#define MEMORY_HEADER

extern volatile char *StrTmp[];                                              // used to track temporary string space on the heap
extern int TempMemoryTop;                                           // this is the last index used for allocating temp memory
extern int TempMemoryIsChanged;						                // used to prevent unnecessary scanning of strtmp[]

typedef enum _M_Req {M_PROG, M_VAR} M_Req;

extern void m_alloc(int size);
//extern void *GetMemory(size_t msize);
extern void *GetTempMemory(int NbrBytes);
extern int MemSize(void *addr);                     // the heap memory allocated at an address (REDIM PRESERVE)
extern void *GetTempStrMemory(void);
extern void *GetMemory(size_t NbrBytes);
extern void *GetMemoryNull(size_t NbrBytes);                       // as GetMemory, NULL when there is no room
extern void *GetInternalMemory(size_t NbrBytes);
extern uint32_t HeapGen;                                // counts the heap's wipes (Memory.c)
extern void *GetInternalMemoryNull(size_t size);
//extern void *GetSystemMemory(int  msize);
extern void ClearTempMemory(void);
extern void ClearSpecificTempMemory(void *addr);
extern int TempMemoryMark(void);                                    // the temporary memory got since a mark freed (Memory.c)
extern void ClearTempMemoryTo(int mark);
extern void FreeMemory(void *addr);
extern void *GetFastTempMemory(size_t size);//extern void *GetSMemory(size_t msize);
extern void InitHeap(void);
extern void ProgramCachesForget(void);
extern void InitHeapKeeping(void **keep, int *size, int n);
extern char *HeapBottom(void);
extern int FreeSpaceOnHeap(void);
extern int FreeSpaceOnSHeap(void);
extern void *ReAllocMemory(void *addr, size_t msize);
extern void *GetStringMemory(void);
extern void *GetStringMemoryBottom(void);
//extern void *SDReAllocMemory(void *addr, size_t msize);
extern void FreeMemorySafe(void **addr);
extern void mycopysafe(void *out, const void *in, int n);
extern void *ReAllocInternalMemory(void *addr, size_t msize);
extern int G1Hardware;
//extern unsigned int _stack;
//extern unsigned int _splim;
//extern unsigned int _heap;
//extern unsigned int _min_stack_size;
//extern unsigned int _text_begin;
//extern int MemSize(void *addr);
//extern int SDMemSize(void *addr);
extern void FreeMemorySafe(void **addr);
extern unsigned int MBitsGet(void *addr);
extern unsigned int SDBitsGet(void *addr);
extern uint32_t MinHeap(void);
extern void *minMMheap;
//extern int FreeSpaceOnSDHeap(void);
// RAM parameters
// ==============
// The following settings will allow MMBasic to use all the free memory on the STM32.  If you need some RAM for
// other purposes you can declare the space needed as a static variable -or- allocate space to the heap (which
// will reduce the memory available to MMBasic) -or- change the definition of RAMEND.
// NOTE: MMBasic does not use the heap.  It has its own heap which is allocated out of its own memory space.
extern uint32_t RAMBASE;
#define RAMEND          	0x30040000
#define FUNBASE				0x30040000
#define FUNEND				(FUNBASE+36*(MAXSUBFUN+1))   // where funtbl ended (the name index replaced it): the buffers after it stay put; the 18 KB is free
#define SRAMBASE         	0x38000000
#define SRAMEND         	0x38010000
// below the heap on a G2 (Peter, 17 Oct): the OPTION RAM program at 0xD0700000 (MPU region 9: its CSUBs run there),
// then the compiler's stream (Stream.c), 1 MB, about three times a program of 340 KB. On a G1 (Peter, 20 Oct) the stream
// takes the bottom 1 MB of the heap, above the OPTION RAM program, while OPTION COMPILE is on: RBStreamSize, set at boot
// (main.c), is its size, 0 when a G1 does not compile.
extern uint32_t RBStreamSize;
#define RB_STREAM_BASE    	(G1Hardware ? (Option.ProgramStartCode<0 ? 0xD0380000: 0xD0300000) : 0xD0780000)
#define RB_STREAM_SIZE    	RBStreamSize
#define SDRAMBASE         	(G1Hardware ? (Option.ProgramStartCode<0 ? 0xD0380000: 0xD0300000) + RBStreamSize : 0xD0880000)
#define SDRAMEND         	(G1Hardware ? 0xD0800000-0x100 : 0xD1800000-0x100)
// the executable window: the top 512 KB of the SDRAM heap, below SDRAMEND (MPU region 15; the rest of the heap is
// not executable, which keeps speculative instruction fetches off the FMC).  GetMemory allocates below it, GetExecMemory
// in it.  At the top, because newlib's _sbrk grows up from SDRAMBASE.
#define SDRAMEXEC        	(G1Hardware ? 0xD0780000 : 0xD1780000)
#define SDRAMEXECSIZE    	0x80000
// loadable programs (Peter 2 Nov 2026): on a G2 the top 8 MB of the SDRAM, above the heap, kept for them for good.
// MPU region 8 makes it executable and write-back cached (main.c); RUN "x.app" loads a program at its start (Loader.c).
// A G1 has no room to keep: a program borrows the heap's largest free run while it runs, and LoaderExec makes regions
// 8 and 9 (the heap above 0xD0380000) executable meanwhile (MPU_Config_nCacheable). A program built --vram (3 Nov
// 2026) is lent the 3 MB of pages at 0xD0000000 too: its code there if it fits, and LoaderVram makes regions 1-6
// executable meanwhile, execute-never again after.
#define LOADER_BASE       	0xD1800000
#define LOADER_SIZE       	0x800000
#define LOADER_VRAM_BASE  	0xD0000000                      // a G1's display memory, lent (Loader.c, CMM2IMG_FLAG_VRAM)
#define LOADER_VRAM_SIZE  	0x300000
// the SDRAM heap's page map at 0x30000000 (Memory.c SDmap): 2 bits for each 256 bytes, so 16 KB maps 16 MB, more than a
// G2's 15.5 MB heap. The 16 KB above it, to RAMBASE's 0x30008000, is the RAM of the runtime the firmware runs in place
// from its flash (cmm2img.h CMM2XIP_RAM: bcrun's .data and .bss, Peter 4 Nov 2026)
#define SDMAP_SIZE        	0x4000
extern int LoaderExec, LoaderVram, LoaderNoSbrk, LoaderModExec;   // (LoaderModExec: a module in the heap, Loader.h)
extern volatile int LoaderActive;
extern int LargestFreeOnHeap(void);
extern int HeapBottomFree(void);
extern void *HeapClaimBottom(int size);
extern int SbrkUnused(void);                            // newlib's arena never grown (syscalls.c)
extern void HeapLowMark(void *was);
extern void *GetExecMemory(size_t size);
// other (minor) memory management parameters
#define RAMPAGESIZE        256                                         // the allocation granuality
#define SDRAMPAGESIZE        256                                      // the allocation granuality
#define PAGEBITS        2                                           // nbr of status bits per page of allocated memory, must be a power of 2

#define PUSED           0b01                                        // flag that indicates that the page is in use
#define PLAST           0b10                                        // flag to show that this is the last page in a single allocation

#define PAGESPERWORD    ((sizeof(unsigned int) * 8)/PAGEBITS)
#define MRoundUp(a)     (((a) + (RAMPAGESIZE - 1)) & (~(RAMPAGESIZE - 1)))// round up to the nearest page size
#define TBUFFSIZE	3200
#define LBUFFSIZE	6400
#define LBUFFSTART FUNEND+TBUFFSIZE+0x100
#define LBUFFEND LBUFFSTART+LBUFFSIZE
#endif
#endif

