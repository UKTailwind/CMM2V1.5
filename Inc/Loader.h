/* Loadable programs: Src/Loader.c. RUN "name.app" runs one; docs/Loader_Plan.md has the design. */
#ifndef LOADER_HEADER
#define LOADER_HEADER
#include <stdint.h>
extern int LoaderRun(char *name);                   // RUN's hook: 1 if it ran a program (name NULL: RUN "name.app")
extern uint64_t MicrosNow(void);                    // microseconds since start-up (TIM16 and uSecTimer, read in step)
extern void *LoaderAlloc(uint32_t size);            // a block of the heap for a program, or NULL; freed when it ends
extern void LoaderRelease(void *p);
extern void (*volatile LoaderFrameFn)(void);      // a program's frame hook (version 6): the LTDC's interrupt pends it
extern volatile int LoaderFramePend;
extern void LoaderDMA2D(void);                     // the DMA2D's interrupt: the program's copies
// modules (CMM2IMG_KIND_MODULE, 7.00.00RC6): one at a time, beside the BASIC program, gone with the heap (SQL.c's SQLite); a G2
// loads one in the applications' 8 MB (a program run there closes it first: an error from a BASIC program), a G1 in the heap
extern int LoaderModExec;                          // 1 while a module is in the heap: the heap executable (main.c)
extern const void *LoaderModule(char *fname, uint32_t heap);   // fname loaded (or the one loaded): its table, or an error
extern const void *LoaderModuleHead(void);         // the table of the module loaded, or NULL
extern uint32_t LoaderModuleHeapSize(void);        // its own heap's bytes (heap 0 to LoaderModule on a G2: all there is)
extern void LoaderModuleForget(void);              // the module gone (HeapWipe): its forget() first
extern void *LoaderModFn(const void *fn);          // fn, to be called on the module's stack: cast to its type and call
#endif
