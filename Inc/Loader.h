/* Loadable programs (G2 only): Src/Loader.c. RUN "name.app" runs one; docs/Loader_Plan.md has the design. */
#ifndef LOADER_HEADER
#define LOADER_HEADER
#include <stdint.h>
extern int LoaderRun(char *name);                   // RUN's hook: 1 if it ran a program (name NULL: RUN "name.app")
extern uint64_t MicrosNow(void);                    // microseconds since start-up (TIM16 and uSecTimer, read in step)
extern void *LoaderAlloc(uint32_t size);            // a block of the heap for a program, or NULL; freed when it ends
extern void LoaderRelease(void *p);
#endif
