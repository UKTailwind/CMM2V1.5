/* pico/stdlib.h: what frank-c64 asks of the Pico SDK, for the CMM2: the time (c64_cmm2.c), and the RP2350's clock, GPIO, interrupts and watchdog, which here do
   nothing. The other headers of stub/ include this one. */
#ifndef CMM2_PICO_STDLIB_H
#define CMM2_PICO_STDLIB_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int uint;
typedef uint64_t absolute_time_t;

void sleep_ms(uint32_t ms);
void sleep_us(uint64_t us);
uint64_t time_us_64(void);
void tight_loop_contents(void);
static inline absolute_time_t get_absolute_time(void) { return time_us_64(); }
static inline uint32_t to_ms_since_boot(absolute_time_t t) { return (uint32_t)(t / 1000); }
static inline uint64_t to_us_since_boot(absolute_time_t t) { return t; }
static inline uint32_t time_us_32(void) { return (uint32_t)time_us_64(); }

#define __not_in_flash(...)
#define __not_in_flash_func(f) f
#define __no_inline_not_in_flash_func(f) f
#define __time_critical_func(f) f
#ifndef __aligned
#define __aligned(n) __attribute__((aligned(n)))
#endif
#define __dmb() __asm volatile("dmb" ::: "memory")
static inline uint32_t save_and_disable_interrupts(void) { return 0; }
static inline void restore_interrupts(uint32_t s) { (void)s; }
static inline uint get_core_num(void) { return 0; }

#define GPIO_IN 0
#define GPIO_OUT 1
#define PICO_DEFAULT_LED_PIN 25
#define gpio_init(p) do { } while(0)
#define gpio_set_dir(p, d) do { } while(0)
#define gpio_put(p, v) do { } while(0)
#define gpio_pull_up(p) do { } while(0)
#define gpio_pull_down(p) do { } while(0)
#define clk_sys 0
#define clock_get_hz(c) 480000000u

#define watchdog_update() do { } while(0)
#define watchdog_enable(ms, pause) do { } while(0)
#define watchdog_disable() do { } while(0)
#define watchdog_caused_reboot() false
#define watchdog_reboot(a, b, c) do { } while(0)

#define multicore_lockout_start_blocking() do { } while(0)
#define multicore_lockout_end_blocking() do { } while(0)

#ifdef __cplusplus
}
#endif
#endif
