/*
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* pico-sdk 2.3.1's pico_cyw43_driver/btstack_cyw43.c on the Colour Maximite 2 (docs/Bluetooth_Plan.md, step 3), changed
   only so: BTstack's run loop is the CMM2's polled one (btstack_run_loop_cmm2.c), not the async context's; the bond
   store's flash bank is the PicoMite's, on Option.bt_tlv (Src/net/bt/picomite/BTKeyboard.c's pico_flash_bank_instance,
   which the PicoMite links in with --wrap); and pico-sdk's panic() is here, as an MMBasic error. */

#include "ble/le_device_db_tlv.h"
#include "classic/btstack_link_key_db_tlv.h"
#include "btstack_tlv.h"
#include "btstack_tlv_flash_bank.h"
#include "btstack_memory.h"
#include "hci.h"

#if WANT_HCI_DUMP
#include "hci_dump.h"
#ifdef ENABLE_SEGGER_RTT
#include "hci_dump_segger_rtt_stdout.h"
#else
#include "hci_dump_embedded_stdout.h"
#endif
#endif

#include "pico/btstack_hci_transport_cyw43.h"
#include "btstack_run_loop_cmm2.h"
#include "pico/btstack_cyw43.h"
#include <stdarg.h>
#include <stdio.h>

const hal_flash_bank_t *pico_flash_bank_instance(void);   // (BTKeyboard.c: the bank over Option.bt_tlv)

static void setup_tlv(void) {
    static btstack_tlv_flash_bank_t btstack_tlv_flash_bank_context;
    const hal_flash_bank_t *hal_flash_bank_impl = pico_flash_bank_instance();

    const btstack_tlv_t *btstack_tlv_impl = btstack_tlv_flash_bank_init_instance(
            &btstack_tlv_flash_bank_context,
            hal_flash_bank_impl,
            NULL);

    // setup global TLV
    btstack_tlv_set_instance(btstack_tlv_impl, &btstack_tlv_flash_bank_context);
#ifdef ENABLE_CLASSIC
    const btstack_link_key_db_t *btstack_link_key_db = btstack_link_key_db_tlv_get_instance(btstack_tlv_impl, &btstack_tlv_flash_bank_context);
    hci_set_link_key_db(btstack_link_key_db);
#endif
#ifdef ENABLE_BLE
    // configure LE Device DB for TLV
    le_device_db_tlv_configure(btstack_tlv_impl, &btstack_tlv_flash_bank_context);
#endif
}

bool btstack_cyw43_init(async_context_t *context) {
    // Initialise bluetooth
    btstack_memory_init();
    (void)context;
    btstack_run_loop_init(btstack_run_loop_cmm2_get_instance());

#if WANT_HCI_DUMP
#ifdef ENABLE_SEGGER_RTT
    hci_dump_init(hci_dump_segger_rtt_stdout_get_instance());
#else
    hci_dump_init(hci_dump_embedded_stdout_get_instance());
#endif
#endif

    hci_init(hci_transport_cyw43_instance(), NULL);

    // setup TLV storage
    setup_tlv();
    return true;
}

void btstack_cyw43_deinit(__unused async_context_t *context) {
    hci_power_control(HCI_POWER_OFF);
    hci_close();
    btstack_run_loop_deinit();
    btstack_memory_deinit();
}

// pico-sdk's panic(), which its Bluetooth glue calls when the chip will not do as it is told: an MMBasic error
void panic(const char *fmt, ...) {
    extern void error(char *msg, ...);
    static char buf[80];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    error("Bluetooth: $", buf);
}
