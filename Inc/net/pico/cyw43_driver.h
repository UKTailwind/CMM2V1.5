/* pico/cyw43_driver.h for the Colour Maximite 2's firmware: pico-sdk's declares cyw43_driver_init and _deinit for its
   async context. The CMM2 has none (Src/net/cyw43_driver_cmm2.c is polled), so nothing is needed here; WiFi.c includes
   it as the PicoMite builds it. */
#ifndef _PICO_CYW43_DRIVER_H
#define _PICO_CYW43_DRIVER_H
#include "cyw43.h"
#endif
