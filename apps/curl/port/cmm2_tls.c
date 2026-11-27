/* cmm2_tls.c: what mbedTLS asks of the CMM2 (port/mbedtls_user_config.h). MIT License, as cmm2img.h.
     mbedtls_hardware_poll   MBEDTLS_ENTROPY_HARDWARE_ALT: the STM32H743's true random number generator. A fault in
                             it (a seed or clock error, or no number in time) is an error, never a weaker source.
     mbedtls_ms_time         MBEDTLS_PLATFORM_MS_TIME_ALT: milliseconds since start-up. */
#include <string.h>
#include "cmm2.h"
#include "stm32h7xx.h"
#include "mbedtls/build_info.h"
#include "mbedtls/platform_time.h"
#include "mbedtls/entropy.h"

static int rng_word(uint32_t *w) {
    uint64_t t0;
    int tries;
    if(!(RCC->AHB2ENR & RCC_AHB2ENR_RNGEN)) {
        RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;      /* (its clock is HSI48, which the firmware runs for USB) */
        (void)RCC->AHB2ENR;
    }
    if(!(RNG->CR & RNG_CR_RNGEN)) RNG->CR |= RNG_CR_RNGEN;
    for(tries = 0; tries < 3; tries++) {
        if(RNG->SR & (RNG_SR_SEIS | RNG_SR_CEIS)) {   /* a seed or clock error: start it again (RM0433 34.3.7) */
            RNG->CR &= ~RNG_CR_RNGEN;
            RNG->SR &= ~(RNG_SR_SEIS | RNG_SR_CEIS);
            RNG->CR |= RNG_CR_RNGEN;
        }
        t0 = cmm2->micros();
        while(!(RNG->SR & RNG_SR_DRDY)) {
            if(cmm2->micros() - t0 > 1000) break;
        }
        if((RNG->SR & RNG_SR_DRDY) && !(RNG->SR & (RNG_SR_SECS | RNG_SR_CECS))) {
            *w = RNG->DR;
            return 0;
        }
    }
    return -1;
}

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
    uint32_t w;
    size_t n;
    (void)data;
    *olen = 0;
    while(*olen < len) {
        if(rng_word(&w)) return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
        n = len - *olen < sizeof(w) ? len - *olen : sizeof(w);
        memcpy(output + *olen, &w, n);
        *olen += n;
    }
    return 0;
}

mbedtls_ms_time_t mbedtls_ms_time(void) {
    return (mbedtls_ms_time_t)(cmm2->micros() / 1000u);
}
