/* pico/async_context.h on the Colour Maximite 2: the CMM2 polls the radio from its one thread (the firmware's poll,
   Src/net/cyw43_arch_cmm2.c), with no async context, so the lock the PicoMite's Bluetooth code takes around its
   btstack calls (bta_lock, ck_lock and the like) has nothing to guard against: these do nothing, as
   cyw43_arch_lwip_begin and _end do (pico/cyw43_arch.h). */
#ifndef _PICO_ASYNC_CONTEXT_H
#define _PICO_ASYNC_CONTEXT_H

typedef struct async_context async_context_t;
static inline void async_context_acquire_lock_blocking(async_context_t *context) {
    (void)context;
}
static inline void async_context_release_lock(async_context_t *context) {
    (void)context;
}

#endif
