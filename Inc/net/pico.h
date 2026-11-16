/* pico.h: what pico-sdk's Bluetooth glue (Src/net/bt) takes from pico-sdk's pico.h on the Colour Maximite 2: the
   standard types, and panic() (Src/net/bt/btstack_cyw43.c: the message, as an MMBasic error). */
#ifndef CMM2_PICO_H
#define CMM2_PICO_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#ifndef __unused
#define __unused __attribute__((unused))
#endif
void panic(const char *fmt, ...);
#endif
