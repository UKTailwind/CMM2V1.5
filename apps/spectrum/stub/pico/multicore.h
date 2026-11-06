#ifndef CMM2_MULTICORE_H
#define CMM2_MULTICORE_H
#include "pico.h"
struct semaphore { int count; };
static inline void sem_init(struct semaphore *s, int a, int b) { (void)s; (void)a; (void)b; }
static inline void sem_acquire_blocking(struct semaphore *s) { (void)s; }
static inline void sem_release(struct semaphore *s) { (void)s; }
/* the second core drew the picture and ran the sound: the firmware's frame hook and sound stream do that here, so "starting it" is setting them up */
void cmm2_core1(void);
#define multicore_launch_core1(fn) cmm2_core1()
static inline void multicore_lockout_victim_init(void) { }
static inline void multicore_lockout_start_blocking(void) { }
static inline void multicore_lockout_end_blocking(void) { }
#endif
