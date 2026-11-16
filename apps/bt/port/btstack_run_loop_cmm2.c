/*
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* btstack_run_loop_cmm2.c: pico-sdk's btstack_run_loop_async_context.c for the Colour Maximite 2, polled. BTstack's work
   (its data sources, the callbacks queued for the main thread and its timers) is done only when some is pending, as
   pico-sdk's async context does it: when a timer is due, when a callback has been queued, or when the chip has said
   that Bluetooth has data (cyw43_bluetooth_hci_process, from the driver's poll). BTstack's embedded run loop polls its
   data sources on every pass instead, and the CYW43 transport's read wakes the chip's Bluetooth core and interrupts it
   each time: done as fast as the CMM2 can loop, that kept the core from ever answering (bt.app, 16 Nov 2026).

   btstack_run_loop_cmm2_poll() does the pending work; the program's loop (bt.app) or the firmware's poll calls it after
   the driver's poll. */
#include <stdbool.h>
#include <stdint.h>
#include "btstack_run_loop.h"
#include "btstack_run_loop_base.h"
#include "btstack_run_loop_cmm2.h"
#include "cmm2.h"

static volatile bool work_pending;
static bool timer_set;
static uint32_t timer_at;                       // (when the next timer is due, in ms, if timer_set)

static uint32_t now_ms(void) {
    return (uint32_t)(cmm2->micros() / 1000u);
}

static void cmm2_init(void) {
    btstack_run_loop_base_init();
    work_pending = false;
    timer_set = false;
}

static void cmm2_add_data_source(btstack_data_source_t *ds) {
    btstack_run_loop_base_add_data_source(ds);
}

static bool cmm2_remove_data_source(btstack_data_source_t *ds) {
    return btstack_run_loop_base_remove_data_source(ds);
}

static void cmm2_enable_data_source_callbacks(btstack_data_source_t *ds, uint16_t callbacks) {
    btstack_run_loop_base_enable_data_source_callbacks(ds, callbacks);
}

static void cmm2_disable_data_source_callbacks(btstack_data_source_t *ds, uint16_t callbacks) {
    btstack_run_loop_base_disable_data_source_callbacks(ds, callbacks);
}

static void cmm2_set_timer(btstack_timer_source_t *ts, uint32_t timeout_in_ms) {
    ts->timeout = now_ms() + timeout_in_ms + 1;
    work_pending = true;
}

static void cmm2_add_timer(btstack_timer_source_t *timer) {
    btstack_run_loop_base_add_timer(timer);
    work_pending = true;
}

static bool cmm2_remove_timer(btstack_timer_source_t *timer) {
    return btstack_run_loop_base_remove_timer(timer);
}

static void cmm2_dump_timer(void) {
    btstack_run_loop_base_dump_timer();
}

static uint32_t cmm2_get_time_ms(void) {
    return now_ms();
}

static void cmm2_execute(void) {                // (not used: the CMM2 polls)
    for (;;)
        btstack_run_loop_cmm2_poll();
}

static void cmm2_trigger_exit(void) {
}

static void cmm2_execute_on_main_thread(btstack_context_callback_registration_t *callback_registration) {
    btstack_run_loop_base_add_callback(callback_registration);
    work_pending = true;
}

static void cmm2_poll_data_sources_from_irq(void) {
    work_pending = true;
}

static const btstack_run_loop_t btstack_run_loop_cmm2 = {
    &cmm2_init,
    &cmm2_add_data_source,
    &cmm2_remove_data_source,
    &cmm2_enable_data_source_callbacks,
    &cmm2_disable_data_source_callbacks,
    &cmm2_set_timer,
    &cmm2_add_timer,
    &cmm2_remove_timer,
    &cmm2_execute,
    &cmm2_dump_timer,
    &cmm2_get_time_ms,
    &cmm2_poll_data_sources_from_irq,
    &cmm2_execute_on_main_thread,
    &cmm2_trigger_exit,
};

const btstack_run_loop_t *btstack_run_loop_cmm2_get_instance(void) {
    return &btstack_run_loop_cmm2;
}

// pico-sdk's btstack_work_pending, when there is work: data sources, then callbacks, then timers, and the time of the next
void btstack_run_loop_cmm2_poll(void) {
    uint32_t now = now_ms();
    if (!work_pending && !(timer_set && (int32_t)(now - timer_at) >= 0))
        return;
    work_pending = false;
    btstack_run_loop_base_poll_data_sources();
    btstack_run_loop_base_execute_callbacks();
    btstack_run_loop_base_process_timers(now_ms());
    now = now_ms();
    int ms = btstack_run_loop_base_get_time_until_timeout(now);
    timer_set = ms != -1;
    if (timer_set)
        timer_at = now + (uint32_t)ms;
}
