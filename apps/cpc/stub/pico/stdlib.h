/* pico/stdlib.h: what frank-cpc asks of the Pico SDK, for the CMM2: the time, and the RP2350's clock, GPIO and watchdog, which here do nothing. The other headers of stub/
   include this one. */
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

void sleep_ms(uint32_t ms);
void sleep_us(uint64_t us);
uint64_t time_us_64(void);
void tight_loop_contents(void);                 /* (cpc_frame_sync's wait for the next frame: -stats counts it) */
int getchar_timeout_us(uint32_t timeout_us);    /* (cpc_serial_console.c's lines: cpc_cmm2.c, Ctrl-P) */
#define PICO_ERROR_TIMEOUT (-1)

#define GPIO_IN 0
#define gpio_init(p) do { } while(0)
#define gpio_set_dir(p, d) do { } while(0)
#define gpio_pull_down(p) do { } while(0)
#define clk_sys 0
#define clock_get_hz(c) 480000000u

static struct { uint32_t scratch[4], ctrl; } cmm2_watchdog;   /* (the serial console's STATUS) */
#define watchdog_hw (&cmm2_watchdog)

#ifdef __cplusplus
}
#endif
#endif
