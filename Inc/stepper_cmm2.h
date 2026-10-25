/* stepper_cmm2.h - the pico SDK's names the PicoMite's stepper code uses (Src/stepper.c and Inc/stepper.h, made from
   its io/stepper.c and io/stepper.h by tools/port_stepper.py), on the Colour Maximite 2 (Peter, 25 Oct 2026):
   - the pins by GP number, on the pins External.c's GPPin[] gives them, with its 64-bit masks (gpio_set_mask64 ...);
   - the 100 kHz tick on TIM1, the only interrupt at NVIC level 0 (stepper_cmm2.c; the others are at IRQ_PRI, one
     level down);
   - the critical sections mask only that tick, not every interrupt: each audio sample here is written by an
     interrupt, every 23 us at 44.1 kHz with no FIFO, and the PicoMite's planner can keep interrupts off for ~50 us;
   - serialtx (TMC22xx) times its bits with the cycle counter, all interrupts off across a frame, as there. */
#ifndef STEPPER_CMM2_H
#define STEPPER_CMM2_H
#include <stdint.h>
#include <stdbool.h>

typedef unsigned int uint;

#define GPIO_IN  0
#define GPIO_OUT 1
#define gpio_init(gp) ((void)(gp))                  // (ExtCfg sets the pin up here)
extern void gpio_put(uint32_t gp, int value);       // External.c, by GP number
extern void gpio_set_dir(uint32_t gp, int out);
extern void gpio_set_mask64(uint64_t mask);
extern void gpio_clr_mask64(uint64_t mask);
extern uint64_t gpio_get_all64(void);

extern void uSec(unsigned int us);
#define busy_wait_us(us) uSec(us)
#define sleep_us(us) uSec(us)

extern uint32_t save_and_disable_interrupts(void);  // the stepper's tick masked (stepper_cmm2.c)
extern void restore_interrupts(uint32_t state);

extern void StepperTimerStart(void);                // TIM1 at 200 kHz, its interrupt at NVIC level 0
extern void StepperTimerStop(void);
#define StepperTimerAck() ((void)0)                 // (TIM1_UP_IRQHandler clears the flag before calling the ISR)

extern void serialtx(uint64_t gpmask, unsigned char *string, int baud);   // string[0]: the byte count, then the bytes (8N1)

#define __get_current_exception() __get_IPSR()      // (non-zero in an interrupt)
#define tight_loop_contents() ((void)0)
extern double getfloat(char *p, double min, double max);   // the PicoMite's: a number, checked (stepper_cmm2.c)
extern void stepper_timer_isr(void);                // the 100 kHz tick (stepper.c, in ITCM)
extern bool StepperHeapBlock(const void *p);        // a page of the on-chip or the SDRAM heap (stepper_cmm2.c)
#endif
