/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* cyw43_arch_cmm2.c: pico-sdk's pico_cyw43_arch (cyw43_arch.c, cyw43_arch_poll.c) and pico_lwip's lwip_nosys.c for
   the Colour Maximite 2's firmware (as apps/wifi's), in poll mode: cyw43_arch_poll() is cyw43_cmm2_poll() (the driver's poll when the chip asks)
   then lwIP's timers, where pico-sdk ran both from its async context. The connect loop is pico-sdk's. Also sys_now()
   for lwIP and its random numbers from the STM32's RNG. */
#include <string.h>
#include "stm32h7xx_hal.h"
#include "Loader.h"                             // (MicrosNow)
#include "pico/cyw43_arch.h"
#if CYW43_LWIP
#include "lwip/init.h"
#include "lwip/timeouts.h"
#endif

#include "NetBlock.h"
#if CYW43_ENABLE_BLUETOOTH
#include "pico/btstack_cyw43.h"
#include "btstack_run_loop_cmm2.h"
int RadioBt(void);                              // (OPTION RADIO: Bluetooth's half on; Src/net/WebMite_cmm2.c)
static bool bt_inited;
#endif

uint8_t *NetBlock;                              // the network block (NetBlock.h), set when OPTION WIFI starts it

static uint32_t country_code = PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE;

#if CYW43_LWIP
// lwip_nosys.c's: lwIP runs only from the program's loop, never from an interrupt, so nothing to protect
sys_prot_t sys_arch_protect(void) {
    return 0;
}

void sys_arch_unprotect(__unused sys_prot_t pval) {
}

uint32_t sys_now(void) {
    return (uint32_t)(MicrosNow() / 1000u);
}
#endif

uint32_t cyw43_cmm2_rand(void) {
    if (!(RNG->CR & RNG_CR_RNGEN)) {
        RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;      // (its clock: HSI48, as the firmware sets it)
        (void)RCC->AHB2ENR;
        RNG->CR |= RNG_CR_RNGEN;
    }
    uint64_t t0 = MicrosNow();
    while (!(RNG->SR & RNG_SR_DRDY)) {
        if (MicrosNow() - t0 > 100) {
            return (uint32_t)MicrosNow() * 2654435761u;
        }
    }
    return RNG->DR;
}

int cyw43_arch_init(void) {
    cyw43_init(&cyw43_state);
#if CYW43_LWIP
    static bool done_lwip_init;
    if (!done_lwip_init && NetBlock != NULL) {  // (lwIP's heap is the network block: none with the WiFi's half off)
        lwip_init();
        done_lwip_init = true;
    }
#endif
#if CYW43_ENABLE_BLUETOOTH
    // as pico-sdk's cyw43_arch_init (cyw43_arch_poll.c): BTstack set up on the same chip, with OPTION RADIO's half on
    if (RadioBt() && !bt_inited)
        bt_inited = btstack_cyw43_init(NULL);
#endif
    return 0;
}

int cyw43_arch_init_with_country(uint32_t country) {
    country_code = country;
    return cyw43_arch_init();
}

void cyw43_arch_deinit(void) {
#if CYW43_ENABLE_BLUETOOTH
    if (bt_inited) {
        btstack_cyw43_deinit(NULL);
        bt_inited = false;
    }
#endif
    cyw43_deinit(&cyw43_state);
}

// Before a G1 starts its radio again after lending its display memory (WebGiveBack, which wipes the radio's data):
// BTstack's memory from newlib's heap given back (hci_stack and its pools; BTMite_cmm2.c's speaker ring), which the
// wipe would lose track of, a leak at every such program. Nothing is said to the chip, which the restart resets.
void cyw43_arch_bt_forget(void) {
#if CYW43_ENABLE_BLUETOOTH
    extern void BtFreeMemory(void);
    if (bt_inited) {
        BtFreeMemory();
        bt_inited = false;
    }
#endif
}

uint32_t cyw43_arch_get_country_code(void) {
    return country_code;
}

void cyw43_arch_enable_sta_mode(void) {
    assert(cyw43_is_initialized(&cyw43_state));
    cyw43_wifi_set_up(&cyw43_state, CYW43_ITF_STA, true, cyw43_arch_get_country_code());
}

void cyw43_arch_disable_sta_mode(void) {
    assert(cyw43_is_initialized(&cyw43_state));
    if (cyw43_state.itf_state & (1 << CYW43_ITF_STA)) {
        cyw43_cb_tcpip_deinit(&cyw43_state, CYW43_ITF_STA);
        cyw43_state.itf_state &= ~(1 << CYW43_ITF_STA);
    }
    if (cyw43_state.wifi_join_state) {
        cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
    }
}

#if CYW43_LWIP
const char *cyw43_tcpip_link_status_name(int status) {
    switch (status) {
    case CYW43_LINK_DOWN:
        return "link down";
    case CYW43_LINK_JOIN:
        return "joining";
    case CYW43_LINK_NOIP:
        return "no ip";
    case CYW43_LINK_UP:
        return "link up";
    case CYW43_LINK_FAIL:
        return "link fail";
    case CYW43_LINK_NONET:
        return "network fail";
    case CYW43_LINK_BADAUTH:
        return "bad auth";
    }
    return "unknown";
}
#endif

int cyw43_arch_wifi_connect_async(const char *ssid, const char *pw, uint32_t auth) {
    if (!pw) auth = CYW43_AUTH_OPEN;
    // Connect to wireless
    return cyw43_wifi_join(&cyw43_state, strlen(ssid), (const uint8_t *)ssid, pw ? strlen(pw) : 0, (const uint8_t *)pw, auth, NULL, CYW43_CHANNEL_NONE);
}

#if CYW43_LWIP
int cyw43_arch_wifi_connect_timeout_ms(const char *ssid, const char *pw, uint32_t auth, uint32_t timeout_ms) {
    uint64_t until = MicrosNow() + (uint64_t)timeout_ms * 1000u;
    int err = cyw43_arch_wifi_connect_async(ssid, pw, auth);
    if (err) return err;

    int status = CYW43_LINK_UP + 1;
    while(status >= 0 && status != CYW43_LINK_UP) {
        int new_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
        // If there was no network, keep trying
        if (new_status == CYW43_LINK_NONET) {
            new_status = CYW43_LINK_JOIN;
            err = cyw43_arch_wifi_connect_async(ssid, pw, auth);
            if (err) return err;
        }
        if (new_status != status) {
            status = new_status;
        }
        if (MicrosNow() >= until) {
            return PICO_ERROR_TIMEOUT;
        }
        // Do polling
        cyw43_arch_poll();
    }
    // Turn status into a pico_error_codes, CYW43_LINK_NONET shouldn't happen as we fail with PICO_ERROR_TIMEOUT instead
    assert(status == CYW43_LINK_UP || status == CYW43_LINK_BADAUTH || status == CYW43_LINK_FAIL);
    if (status == CYW43_LINK_UP) {
        return PICO_OK; // success
    } else if (status == CYW43_LINK_BADAUTH) {
        return PICO_ERROR_BADAUTH;
    } else {
        return PICO_ERROR_CONNECT_FAILED;
    }
}
#endif

void cyw43_arch_gpio_put(uint wl_gpio, bool value) {
    cyw43_gpio_set(&cyw43_state, (int)wl_gpio, value);
}

bool cyw43_arch_gpio_get(uint wl_gpio) {
    bool value = false;
    cyw43_gpio_get(&cyw43_state, (int)wl_gpio, &value);
    return value;
}

void cyw43_arch_poll(void) {
    cyw43_cmm2_poll();
#if CYW43_LWIP
    if (NetBlock != NULL)
        sys_check_timeouts();
#endif
#if CYW43_ENABLE_BLUETOOTH
    // BTstack's pending work (btstack_run_loop_cmm2.c), as pico-sdk's async context does it after the driver's poll:
    // its data sources when the chip has flagged Bluetooth data, its callbacks and its timers
    if (bt_inited)
        btstack_run_loop_cmm2_poll();
#endif
}
