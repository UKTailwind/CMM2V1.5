/* hardware/clocks.h: for the menu's Hardware Info */
#ifndef CMM2_CLOCKS_H
#define CMM2_CLOCKS_H
#include "pico.h"
#define clk_sys 0
#define KHZ 1000
#define MHZ 1000000
static inline uint32_t clock_get_hz(int clock) { (void)clock; return 480000000u; }
static inline bool set_sys_clock_khz(uint32_t khz, bool required) { (void)khz; (void)required; return true; }
#endif
