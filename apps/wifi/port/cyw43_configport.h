/*
 * cyw43_configport.h for the Colour Maximite 2 (apps/wifi): pico-sdk's (src/rp2_common/pico_cyw43_driver/include,
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd., SPDX-License-Identifier: BSD-3-Clause), its settings kept and the
 * RP2040's GPIO, timers and async context replaced by the CMM2's.
 *
 * The pins (Peter's RM2 module on the rear I/O header, docs/Web_Plan.md) are not RP2040 GPIO numbers but indexes,
 * which cyw43_driver_cmm2.c maps to the STM32's ports:
 *   CYW43_PIN_WL_REG_ON    PI8, MMBasic pin 32: WL_ON and BT_ON
 *   CYW43_PIN_WL_CS        PI3, pin 33
 *   CYW43_PIN_WL_HOST_WAKE PB15, pin 38: the gSPI data line, the chip's IRQ on it (through 10K) while CS is high
 * The clock is PB13 (pin 40), SPI2's SCK; the data line is SPI2's MOSI in half-duplex (cyw43_bus_cmm2.c).
 */
#ifndef _CYW43_CONFIGPORT_H
#define _CYW43_CONFIGPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int uint;                      // (pico.h's, used by the copied code)
#ifndef MIN
#define MIN(a, b) ((b) > (a) ? (a) : (b))     // (pico.h's)
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef __unused
#define __unused __attribute__((unused))
#endif

#ifndef CYW43_HOST_NAME
#define CYW43_HOST_NAME "CMM2"
#endif

#ifndef CYW43_GPIO
#define CYW43_GPIO 1
#endif

#ifndef CYW43_LOGIC_DEBUG
#define CYW43_LOGIC_DEBUG 0
#endif

#ifndef CYW43_USE_OTP_MAC
#define CYW43_USE_OTP_MAC 1
#endif

#ifndef CYW43_NO_NETUTILS
#define CYW43_NO_NETUTILS 1
#endif

#ifndef CYW43_IOCTL_TIMEOUT_US
#define CYW43_IOCTL_TIMEOUT_US 1000000
#endif

#ifndef CYW43_USE_STATS
#define CYW43_USE_STATS 0
#endif

// todo should this be user settable?
#ifndef CYW43_HAL_MAC_WLAN0
#define CYW43_HAL_MAC_WLAN0 0
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef CYW43_USE_SPI
#define CYW43_USE_SPI 1
#endif

#ifndef CYW43_SPI_PIO
#define CYW43_SPI_PIO 1                         // (cyw43_bus_cmm2.c keeps pico-sdk's guard: it is the bus, as the PIO's was)
#endif

#ifndef CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE
#if CYW43_ENABLE_BLUETOOTH
#define CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE "wb43439A0_7_95_49_00_combined.h"
#else
#define CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE "w43439A0_7_95_49_00_combined.h"
#endif
#endif

#ifndef CYW43_WIFI_NVRAM_INCLUDE_FILE
#define CYW43_WIFI_NVRAM_INCLUDE_FILE "wifi_nvram_43439.h"
#endif

// pico-sdk's error codes (pico/error.h), which the PicoMite's net code meets through cyw43_arch
#define PICO_OK                    0
#define PICO_ERROR_NONE            0
#define PICO_ERROR_GENERIC        -1
#define PICO_ERROR_TIMEOUT        -2
#define PICO_ERROR_NO_DATA        -3
#define PICO_ERROR_NOT_PERMITTED  -4
#define PICO_ERROR_INVALID_ARG    -5
#define PICO_ERROR_IO             -6
#define PICO_ERROR_BADAUTH        -7
#define PICO_ERROR_CONNECT_FAILED -8
#define PICO_ERROR_INSUFFICIENT_RESOURCES -9

// Note, these are negated, because cyw43_driver negates them before returning!
#define CYW43_EPERM            (-PICO_ERROR_NOT_PERMITTED) // Operation not permitted
#define CYW43_EIO              (-PICO_ERROR_IO) // I/O error
#define CYW43_EINVAL           (-PICO_ERROR_INVALID_ARG) // Invalid argument
#define CYW43_ETIMEDOUT        (-PICO_ERROR_TIMEOUT) // Connection timed out

#ifndef CYW43_WL_GPIO_COUNT
#define CYW43_WL_GPIO_COUNT 3
#endif

#define CYW43_NUM_GPIOS        CYW43_WL_GPIO_COUNT

#define cyw43_hal_pin_obj_t uint

// get the number of elements in a fixed-size array
#define CYW43_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

uint32_t cyw43_hal_ticks_us(void);
uint32_t cyw43_hal_ticks_ms(void);

#define CYW43_PIN_WL_REG_ON     0
#define CYW43_PIN_WL_CS         1
#define CYW43_PIN_WL_HOST_WAKE  2
#define CYW43_PIN_WL_DATA_IN    CYW43_PIN_WL_HOST_WAKE
#define CYW43_PIN_WL_DATA_OUT   CYW43_PIN_WL_HOST_WAKE

int cyw43_hal_pin_read(cyw43_hal_pin_obj_t pin);
void cyw43_hal_pin_low(cyw43_hal_pin_obj_t pin);
void cyw43_hal_pin_high(cyw43_hal_pin_obj_t pin);

#define CYW43_HAL_PIN_MODE_INPUT           (0)
#define CYW43_HAL_PIN_MODE_OUTPUT          (1)

#define CYW43_HAL_PIN_PULL_NONE            (0)
#define CYW43_HAL_PIN_PULL_UP              (1)
#define CYW43_HAL_PIN_PULL_DOWN            (2)

void cyw43_hal_pin_config(cyw43_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint32_t alt);

void cyw43_hal_get_mac(int idx, uint8_t buf[6]);

void cyw43_hal_generate_laa_mac(int idx, uint8_t buf[6]);

// One thread, polled: nothing to lock
#define CYW43_THREAD_ENTER
#define CYW43_THREAD_EXIT
#define cyw43_arch_lwip_check() ((void)0)
#define CYW43_THREAD_LOCK_CHECK

void cyw43_await_background_or_timeout_us(uint32_t timeout_us);
// todo not 100% sure about the timeouts here; MP uses __WFI which will always wakeup periodically
#define CYW43_SDPCM_SEND_COMMON_WAIT cyw43_await_background_or_timeout_us(1000);
#define CYW43_DO_IOCTL_WAIT cyw43_await_background_or_timeout_us(1000);

void cyw43_delay_ms(uint32_t ms);

void cyw43_delay_us(uint32_t us);

void cyw43_schedule_internal_poll_dispatch(void (*func)(void));

void cyw43_post_poll_hook(void);

#define CYW43_POST_POLL_HOOK cyw43_post_poll_hook();

// Allow malloc and free to be changed
#ifndef cyw43_malloc
#define cyw43_malloc malloc
#endif
#ifndef cyw43_free
#define cyw43_free free
#endif

#if !defined CYW43_LWIP && defined CYW43_LWIP_DEFAULT
#define CYW43_LWIP CYW43_LWIP_DEFAULT
#endif

// The CMM2's own: the poll that pico_cyw43_arch_poll's async context does (cyw43_driver_cmm2.c), called from the
// program's loop: cyw43_poll when the chip asks (its IRQ high on the data line), when the driver has scheduled it,
// and every 50 ms while it is awake (cyw43_sleep).
void cyw43_cmm2_poll(void);
// The four pins and SPI2 as they were before the program, and the radio off (at the end)
void cyw43_cmm2_save_pins(void);
void cyw43_cmm2_restore_pins(void);

#ifdef __cplusplus
}
#endif

#endif
