/* stepper_cmm2.c - what the pico SDK gives the PicoMite's stepper code (Src/stepper.c, made by tools/port_stepper.py),
   on the Colour Maximite 2 (Peter, 25 Oct 2026). See Inc/stepper_cmm2.h. */
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "stepper.h"

/* The tick: ISR_FREQ (stepper.h: 100 kHz, as the PicoMite's) from TIM1, unused by anything else, its update interrupt
   at NVIC level 0, above every other interrupt (they are at IRQ_PRI(n), Configuration.h), as the PicoMite puts its PWM
   slice 10 wrap. TIM1's clock is APB2's, doubled when APB2 is divided (240 MHz at 480 MHz). */
static uint32_t StepperTimerClock(void) {
    uint32_t f = HAL_RCC_GetPCLK2Freq();
    if((RCC->D2CFGR & RCC_D2CFGR_D2PPRE2) != RCC_APB2_DIV1) f *= 2;
    return f;
}

void StepperTimerStart(void) {
    __HAL_RCC_TIM1_CLK_ENABLE();
    TIM1->CR1 = 0;
    TIM1->DIER = 0;
    TIM1->PSC = 0;
    TIM1->ARR = StepperTimerClock() / ISR_FREQ - 1;                 // (ISR_FREQ, stepper.h: what the planner's rates are in)
    TIM1->CNT = 0;
    TIM1->EGR = TIM_EGR_UG;                                         // (the prescaler and reload loaded)
    TIM1->SR = 0;
    NVIC_ClearPendingIRQ(TIM1_UP_IRQn);
    HAL_NVIC_SetPriority(TIM1_UP_IRQn, 0, 0);                       // the only interrupt at level 0
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
    TIM1->DIER = TIM_DIER_UIE;
    TIM1->CR1 = TIM_CR1_CEN;
}

void StepperTimerStop(void) {
    TIM1->CR1 = 0;
    TIM1->DIER = 0;
    HAL_NVIC_DisableIRQ(TIM1_UP_IRQn);
    TIM1->SR = 0;
    NVIC_ClearPendingIRQ(TIM1_UP_IRQn);
}

void ITCM_CODE TIM1_UP_IRQHandler(void) {
    TIM1->SR = ~TIM_SR_UIF;                                         // (rc_w0: only UIF cleared)
    stepper_timer_isr();
}

/* The stepper's critical sections guard what the planner shares with the tick: the tick alone is masked, so audio,
   USB and the serial ports go on (save_and_disable_interrupts' state: whether it was enabled). */
uint32_t save_and_disable_interrupts(void) {
    uint32_t was = NVIC_GetEnableIRQ(TIM1_UP_IRQn);
    NVIC_DisableIRQ(TIM1_UP_IRQn);
    __DSB();
    __ISB();
    return was;
}

void restore_interrupts(uint32_t was) {
    if(was) NVIC_EnableIRQ(TIM1_UP_IRQn);
}

// the PicoMite's getfloat: a number from 'min' to 'max'
double getfloat(char *p, double min, double max) {
    MMFLOAT f = getnumber(p);
    if(f < min || f > max) {
        char a[24], b[24], c[24];
        FloatToStr(a, f, 0, STR_AUTO_PRECISION, ' ');
        FloatToStr(b, min, 0, STR_AUTO_PRECISION, ' ');
        FloatToStr(c, max, 0, STR_AUTO_PRECISION, ' ');
        error("$ is invalid (valid is $ to $)", a, b, c);
    }
    return f;
}

/* The tick parks the arc buffers it has finished with for the main loop to free (stepper.c's stepper_retired); a
   node is checked before it is freed: a page of the on-chip heap or of the SDRAM heap (both of 256-byte pages). */
bool StepperHeapBlock(const void *p) {
    uintptr_t a = (uintptr_t)p;
    if(a & (RAMPAGESIZE - 1)) return false;
    return (a >= (uintptr_t)RAMBASE && a < RAMEND) || (a >= (uintptr_t)SDRAMBASE && a < (uintptr_t)SDRAMEND);
}

/* TMC22xx's transmitter: 8N1, the low bit first, string[0] the byte count, on the GPs in gpmask, the bits timed by the
   cycle counter from the start of the frame (so they do not drift), every interrupt off across it as on the PicoMite
   (a 9-byte frame at 115200: 0.8 ms). */
void serialtx(uint64_t gpmask, unsigned char *string, int baud) {
    uint32_t bit = SystemCoreClock / baud, t, primask;
    int n = (unsigned char)string[0], i, k;
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55;                                          // (the M7's DWT is locked until this is written)
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    primask = __get_PRIMASK();
    __disable_irq();
    t = DWT->CYCCNT;
    for(i = 1; i <= n; i++) {
        uint32_t b = ((uint32_t)(unsigned char)string[i] << 1) | 0x200;   // start bit 0, 8 data bits, stop bit 1
        for(k = 0; k < 10; k++, b >>= 1) {
            gpio_put_masked64(gpmask, (b & 1) ? gpmask : 0);
            t += bit;
            while((int32_t)(DWT->CYCCNT - t) < 0);
        }
    }
    __set_PRIMASK(primask);
}
