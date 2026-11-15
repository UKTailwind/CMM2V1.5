/* mbedtls_cmm2.c: what the PicoMite's net/mbedtls_config.h (copied as it is beside this file) expects from its
   firmware, for wifi.app. MIT License, as cmm2img.h.
     CallocMemory, FreeMemory    MBEDTLS_PLATFORM_CALLOC_MACRO and _FREE_MACRO: MMBasic's heap on the PicoMite, the
                                 program's C heap here (in the firmware, the network block)
     picomite_mbedtls_time       MBEDTLS_PLATFORM_TIME_MACRO: seconds since 1970 from the CMM2's clock (the RTC), so
                                 a certificate's dates check against the real time once the clock is set
     mbedtls_ms_time             MBEDTLS_PLATFORM_MS_TIME_ALT: milliseconds since start-up
     mbedtls_hardware_poll       MBEDTLS_ENTROPY_HARDWARE_ALT: the STM32's true random number generator, as pico-sdk's
                                 pico_mbedtls.c takes the RP2040's */
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"
#include "mbedtls/build_info.h"
#include "mbedtls/platform_time.h"

uint32_t cyw43_cmm2_rand(void);                 // (cyw43_arch_cmm2.c: the RNG)

void *CallocMemory(size_t num, size_t size) {
    return calloc(num, size);
}

void FreeMemory(void *addr) {
    free(addr);
}

static long long days_from_civil(int y, int m, int d) {   // (Howard Hinnant's)
    y -= m <= 2;
    long long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = (unsigned)(y - era * 400);
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (long long)doe - 719468;
}

long long picomite_mbedtls_time(long long *tp) {
    int t[6];
    cmm2->datetime(t);                          // year, month, day, hour, minute, second
    long long s = days_from_civil(t[0], t[1], t[2]) * 86400LL + t[3] * 3600 + t[4] * 60 + t[5];
    if (tp) {
        *tp = s;
    }
    return s;
}

mbedtls_ms_time_t mbedtls_ms_time(void) {
    return (mbedtls_ms_time_t)(cmm2->micros() / 1000u);
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
