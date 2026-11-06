#ifndef CMM2_SYNC_H
#define CMM2_SYNC_H
#include "pico.h"
static inline uint32_t save_and_disable_interrupts(void) { return 0; }
static inline void restore_interrupts(uint32_t s) { (void)s; }
#endif
