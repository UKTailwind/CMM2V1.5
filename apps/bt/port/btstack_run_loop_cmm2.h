/* btstack_run_loop_cmm2.h: BTstack's run loop on the Colour Maximite 2 (btstack_run_loop_cmm2.c), polled. */
#ifndef BTSTACK_RUN_LOOP_CMM2_H
#define BTSTACK_RUN_LOOP_CMM2_H
#include "btstack_run_loop.h"

const btstack_run_loop_t *btstack_run_loop_cmm2_get_instance(void);
void btstack_run_loop_cmm2_poll(void);          // BTstack's pending work, if any (after the driver's poll)

#endif
