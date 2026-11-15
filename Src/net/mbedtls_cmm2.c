/* mbedtls_cmm2.c: what the PicoMite's net/mbedtls_config.h (Inc/net, copied as it is) expects from the firmware, on the
   Colour Maximite 2 (docs/Web_Plan.md, step 2).
     CallocMemory                MBEDTLS_PLATFORM_CALLOC_MACRO: MMBasic's heap, as the PicoMite's (Memory.c there:
                                 GetMemory, which zeroes the block and raises "Not enough memory" when there is none).
                                 A TLS session's memory never outlives a RUN. FreeMemory is MMBasic's own (Memory.c).
     (picomite_mbedtls_time and mbedtls_ms_time are the PicoMite's own, in its WiFi.c)
     mbedtls_hardware_poll       MBEDTLS_ENTROPY_HARDWARE_ALT: the STM32's true random number generator, as pico-sdk's
                                 pico_mbedtls.c takes the RP2040's */
#include <stdint.h>
#include <string.h>
#include "mbedtls/build_info.h"

extern void *GetMemory(size_t size);
uint32_t cyw43_cmm2_rand(void);                 // (cyw43_arch_cmm2.c: the RNG)

void *CallocMemory(size_t num, size_t size) {
    return GetMemory(num * size);
}

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
    (void)data;
    *olen = 0;
    while (*olen < len) {
        uint32_t r = cyw43_cmm2_rand();
        size_t n = len - *olen < sizeof(r) ? len - *olen : sizeof(r);
        memcpy(output + *olen, &r, n);
        *olen += n;
    }
    return 0;
}
