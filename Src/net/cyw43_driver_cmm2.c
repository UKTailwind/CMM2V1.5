/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * cyw43_driver_cmm2.c: what pico-sdk's cyw43_driver.c (src/rp2_common/pico_cyw43_driver) gives the driver, for the
 * Colour Maximite 2's firmware (as apps/wifi's). pico-sdk runs cyw43_poll from an async context woken by a GPIO interrupt on the chip's IRQ; here
 * there is one thread, polled, as pico_cyw43_arch_lwip_poll is on the PicoMite: cyw43_cmm2_poll(), called from the
 * firmware's ProcessWeb (cyw43_arch_poll), runs cyw43_poll when the IRQ (the data line while CS is high) is up, when the driver has asked for
 * it, and every 50 ms while it is awake (cyw43_sleep), as pico-sdk's sleep worker does.
 *
 * Also the pins' HAL (cyw43_configport.h's indexes to the STM32's ports), the time and the MAC address.
 */

#include <string.h>
#include "stm32h7xx_hal.h"
#include "Loader.h"                             // (MicrosNow)
#include "cyw43.h"

#ifndef CYW43_SLEEP_CHECK_MS
#define CYW43_SLEEP_CHECK_MS 50
#endif

static volatile bool poll_pending;
static uint32_t last_poll_ms;

typedef struct { GPIO_TypeDef *port; uint8_t bit; } wl_pin_t;
static const wl_pin_t pins_addon[] = {          // the add-on module, on the rear connector
    {GPIOI, 8},                                  // CYW43_PIN_WL_REG_ON: PI8, MMBasic pin 32
    {GPIOI, 3},                                  // CYW43_PIN_WL_CS: PI3, pin 33
    {GPIOB, 15},                                 // CYW43_PIN_WL_HOST_WAKE: PB15, pin 38, the data line
}, pins_onboard[] = {                            // a G3 V1.2's module, on the board (main.c RM2OnBoard)
    {GPIOF, 8},                                  // CYW43_PIN_WL_REG_ON: PF8
    {GPIOF, 7},                                  // CYW43_PIN_WL_CS: PF7
    {GPIOG, 14},                                 // CYW43_PIN_WL_HOST_WAKE: PG14, the data line
};
extern int RM2OnBoard;
#define pins (RM2OnBoard ? pins_onboard : pins_addon)

void cyw43_cmm2_pin_mode(GPIO_TypeDef *port, int bit, int moder) {   // 0 input, 1 output, 2 alternate function
    port->MODER = (port->MODER & ~(3u << (2 * bit))) | ((uint32_t)moder << (2 * bit));
}

int cyw43_hal_pin_read(cyw43_hal_pin_obj_t pin) {
    return (pins[pin].port->IDR >> pins[pin].bit) & 1;
}

void cyw43_hal_pin_low(cyw43_hal_pin_obj_t pin) {
    pins[pin].port->BSRR = 1u << (pins[pin].bit + 16);
}

void cyw43_hal_pin_high(cyw43_hal_pin_obj_t pin) {
    pins[pin].port->BSRR = 1u << pins[pin].bit;
}

void cyw43_hal_pin_config(cyw43_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint32_t alt) {
    (void)alt;
    GPIO_TypeDef *p = pins[pin].port;
    int b = pins[pin].bit;
    uint32_t pupd = pull == CYW43_HAL_PIN_PULL_UP ? 1u : pull == CYW43_HAL_PIN_PULL_DOWN ? 2u : 0u;
    p->PUPDR = (p->PUPDR & ~(3u << (2 * b))) | (pupd << (2 * b));
    if (mode == CYW43_HAL_PIN_MODE_OUTPUT) {
        p->OTYPER &= ~(1u << b);
    }
    cyw43_cmm2_pin_mode(p, b, mode == CYW43_HAL_PIN_MODE_OUTPUT ? 1 : 0);
}

uint32_t cyw43_hal_ticks_us(void) {
    return (uint32_t)MicrosNow();
}

uint32_t cyw43_hal_ticks_ms(void) {
    return (uint32_t)(MicrosNow() / 1000u);
}

void cyw43_delay_us(uint32_t us) {
    uint64_t end = MicrosNow() + us;
    while (MicrosNow() < end) {
    }
}

void cyw43_delay_ms(uint32_t ms) {
    cyw43_delay_us(ms * 1000u);
}

// pico-sdk waits for the async context's work (the IRQ) or the time; here the IRQ is read from the data line
void cyw43_await_background_or_timeout_us(uint32_t timeout_us) {
    uint64_t t0 = MicrosNow();
    while (MicrosNow() - t0 < timeout_us) {
        if (poll_pending || cyw43_hal_pin_read(CYW43_PIN_WL_HOST_WAKE)) {
            return;
        }
    }
}

void cyw43_schedule_internal_poll_dispatch(void (*func)(void)) {
    assert(func == cyw43_poll);
    (void)func;
    poll_pending = true;
}

void cyw43_post_poll_hook(void) {
}

// The chip has something for the driver: its IRQ is up (the data line, while CS is high), or the driver asked for a poll
int cyw43_cmm2_waiting(void) {
    return cyw43_poll != NULL && (poll_pending || cyw43_hal_pin_read(CYW43_PIN_WL_HOST_WAKE));
}

void cyw43_cmm2_poll(void) {
    if (cyw43_poll == NULL) {
        return;
    }
    uint32_t now = cyw43_hal_ticks_ms();
    if (!poll_pending && !cyw43_hal_pin_read(CYW43_PIN_WL_HOST_WAKE) &&
        !(cyw43_sleep && now - last_poll_ms >= CYW43_SLEEP_CHECK_MS)) {
        return;
    }
    poll_pending = false;
    last_poll_ms = now;
    if (cyw43_sleep > 0) {
        cyw43_sleep--;
    }
    cyw43_poll();
}

// A locally administered MAC from the STM32's unique ID, if the chip's OTP has none (as pico-sdk's from the board id)
void cyw43_hal_generate_laa_mac(__unused int idx, uint8_t buf[6]) {
    CYW43_DEBUG("Warning. No mac in otp. Generating mac from board id\n");
    const uint8_t *uid = (const uint8_t *)UID_BASE;
    memcpy(buf, uid + 2, 6);
    buf[0] &= (uint8_t)~0x1; // unicast
    buf[0] |= 0x2; // locally administered
}

void cyw43_hal_get_mac(__unused int idx, uint8_t buf[6]) {
    memcpy(buf, cyw43_state.mac, 6);
}

#if !CYW43_LWIP
static void no_lwip_fail(void) {
    printf("cyw43 has no ethernet interface\n");
}
void __attribute__((weak)) cyw43_cb_tcpip_init(__unused cyw43_t *self, __unused int itf) {
}
void __attribute__((weak)) cyw43_cb_tcpip_deinit(__unused cyw43_t *self, __unused int itf) {
}
void __attribute__((weak)) cyw43_cb_tcpip_set_link_up(__unused cyw43_t *self, __unused int itf) {
    no_lwip_fail();
}
void __attribute__((weak)) cyw43_cb_tcpip_set_link_down(__unused cyw43_t *self, __unused int itf) {
    no_lwip_fail();
}
void __attribute__((weak)) cyw43_cb_process_ethernet(__unused void *cb_data, __unused int itf, __unused size_t len, __unused const uint8_t *buf) {
    no_lwip_fail();
}
int __attribute__((weak)) cyw43_tcpip_link_status(__unused cyw43_t *self, int __unused itf) {
    no_lwip_fail();
    return 0;
}
#endif
