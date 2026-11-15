/* mbedtls_cmm2.c: what the PicoMite's net/mbedtls_config.h (Inc/net, copied as it is) expects from the firmware, on the
   Colour Maximite 2 (docs/Web_Plan.md, step 2).
     CallocMemory                MBEDTLS_PLATFORM_CALLOC_MACRO: MMBasic's heap, as the PicoMite's (Memory.c there:
                                 GetMemory, which zeroes the block and raises "Not enough memory" when there is none).
                                 A TLS session's memory never outlives a RUN. FreeMemory is MMBasic's own (Memory.c).
     picomite_mbedtls_time       MBEDTLS_PLATFORM_TIME_MACRO: seconds since 1970 from the CMM2's clock (the RTC, or WEB
                                 NTP's time), so a certificate's dates check against the real time once the clock is set
     mbedtls_ms_time             MBEDTLS_PLATFORM_MS_TIME_ALT: milliseconds since start-up
     mbedtls_hardware_poll       MBEDTLS_ENTROPY_HARDWARE_ALT: the STM32's true random number generator, as pico-sdk's
                                 pico_mbedtls.c takes the RP2040's */
#include <stdint.h>
#include <string.h>
#include "mbedtls/build_info.h"
#include "mbedtls/platform_time.h"
#include "Loader.h"                             // (MicrosNow)

extern void *GetMemory(size_t size);
extern volatile int second, minute, hour, day, month, year;
uint32_t cyw43_cmm2_rand(void);                 // (cyw43_arch_cmm2.c: the RNG)

void *CallocMemory(size_t num, size_t size) {
    return GetMemory(num * size);
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
    int y, mo, d, h, mi, s;
    do {                                        // (the counters move in the 1 ms tick: read until a second is whole)
        s = second; y = year; mo = month; d = day; h = hour; mi = minute;
    } while (s != second);
    long long t = days_from_civil(y, mo, d) * 86400LL + h * 3600 + mi * 60 + s;
    if (tp) {
        *tp = t;
    }
    return t;
}

mbedtls_ms_time_t mbedtls_ms_time(void) {
    return (mbedtls_ms_time_t)(MicrosNow() / 1000u);
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
