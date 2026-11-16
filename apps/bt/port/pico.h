/* pico.h: what pico-sdk's Bluetooth glue (port/) takes from pico-sdk's pico.h on the Colour Maximite 2 (bt.app): the
   standard types, and panic(), which bt.c gives (a message, then back to MMBasic). */
#ifndef CMM2_PICO_H
#define CMM2_PICO_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
void panic(const char *fmt, ...);
#endif
