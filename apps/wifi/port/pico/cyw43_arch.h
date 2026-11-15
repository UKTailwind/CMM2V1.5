/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* pico/cyw43_arch.h for the Colour Maximite 2 (apps/wifi/port): the calls of pico-sdk's pico_cyw43_arch that the
   PicoMite's net files make, with pico-sdk's names and meanings, in poll mode (pico_cyw43_arch_lwip_poll) and with no
   async context. cyw43_arch_cmm2.c has them. */
#ifndef _PICO_CYW43_ARCH_H
#define _PICO_CYW43_ARCH_H

#include "cyw43.h"
#include "cyw43_country.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE
#define PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE CYW43_COUNTRY_WORLDWIDE
#endif

#define CYW43_WL_GPIO_LED_PIN 0                 // (the Pico W's LED, on the chip's GPIO 0: nothing on the RM2 module)

int cyw43_arch_init(void);
int cyw43_arch_init_with_country(uint32_t country);
void cyw43_arch_deinit(void);
void cyw43_arch_enable_sta_mode(void);
void cyw43_arch_disable_sta_mode(void);
int cyw43_arch_wifi_connect_async(const char *ssid, const char *pw, uint32_t auth);
int cyw43_arch_wifi_connect_timeout_ms(const char *ssid, const char *pw, uint32_t auth, uint32_t timeout_ms);
uint32_t cyw43_arch_get_country_code(void);
void cyw43_arch_gpio_put(uint wl_gpio, bool value);
bool cyw43_arch_gpio_get(uint wl_gpio);
void cyw43_arch_poll(void);
const char *cyw43_tcpip_link_status_name(int status);

// one thread: lwIP needs no lock
static inline void cyw43_arch_lwip_begin(void) {
}
static inline void cyw43_arch_lwip_end(void) {
}

#ifdef __cplusplus
}
#endif

#endif
