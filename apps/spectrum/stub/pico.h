/* pico.h: what pico-spec takes from the Pico SDK's headers. The attributes place code and data on an RP2040, and mean nothing here */
#ifndef CMM2_PICO_H
#define CMM2_PICO_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#ifndef __not_in_flash_func
#define __not_in_flash_func(f) f
#define __no_inline_not_in_flash_func(f) f
#define __time_critical_func(f) f
#define __not_in_flash(x)
#define __in_flash(...)
#define __scratch_x(x)
#define __scratch_y(x)
#define __uninitialized_ram(x) x
#endif
#ifndef __unused
#define __unused __attribute__((unused))
#endif
#ifndef __aligned
#define __aligned(n) __attribute__((aligned(n)))
#endif
#define __force_inline inline __attribute__((always_inline))
#define __printflike(a, b)
#define __packed __attribute__((packed))
typedef unsigned int uint;
#define VGA_BASE_PIN 0
/* the pins the menu's Hardware Info lists: the board has none of them */
#define KBD_CLOCK_PIN 0
#define KBD_DATA_PIN 0
#define PWM_PIN0 0
#define PWM_PIN1 0
#define SDCARD_PIN_SPI0_MISO 0
#define SDCARD_PIN_SPI0_MOSI 0
#define SDCARD_PIN_SPI0_SCK 0
#define SDCARD_PIN_SPI0_CS 0
#define PICO_GIT_BRANCH "main"
#define PICO_GIT_COMMIT "7b8f48f"
#define PICO_BUILD_NAME "CMM2"
#ifdef __cplusplus
extern "C" {
#endif
uint64_t time_us_64(void);
static inline uint32_t time_us_32(void) { return (uint32_t)time_us_64(); }
void sleep_us(uint64_t us);
void sleep_ms(uint32_t ms);
void tight_loop_contents(void);
typedef uint64_t absolute_time_t;
static inline absolute_time_t get_absolute_time(void) { return time_us_64(); }
static inline uint32_t to_ms_since_boot(absolute_time_t t) { return (uint32_t)(t / 1000); }
static inline uint64_t to_us_since_boot(absolute_time_t t) { return t; }
#ifdef __cplusplus
}
#endif
#endif
