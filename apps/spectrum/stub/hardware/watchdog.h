/* hardware/watchdog.h: how pico-spec reboots, after a change of machine: here the program starts again (spectrum_cmm2.cpp) */
#ifndef CMM2_WATCHDOG_H
#define CMM2_WATCHDOG_H
#include "pico.h"
#ifdef __cplusplus
extern "C" {
#endif
void cmm2_restart(void);
static struct { uint32_t scratch[8]; } cmm2_watchdog;
#define watchdog_hw (&cmm2_watchdog)
static inline void watchdog_enable(uint32_t ms, bool pause) { (void)ms; (void)pause; cmm2_restart(); }
static inline void watchdog_reboot(uint32_t a, uint32_t b, uint32_t ms) { (void)a; (void)b; (void)ms; cmm2_restart(); }
static inline bool watchdog_caused_reboot(void) { return false; }
#ifdef __cplusplus
}
#endif
#endif
