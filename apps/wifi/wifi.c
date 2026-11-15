/* wifi.c: step 1 of docs/Web_Plan.md, the CYW43439 on Peter's RM2 module driven from a loadable program, with the
   firmware as it is: cyw43-driver (as the PicoMite builds it) over SPI2 in half-duplex (port/cyw43_bus_cmm2.c).
   MIT License, as cmm2img.h.

     RUN "wifi.app", test [n]       the bus alone: the radio powered up, the test register in 16-bit mode, then the
                                    chip in 32-bit high-speed mode (as cyw43_ll.c sets it) and n rounds of a random
                                    word written to the R/W test register and read back and the test register read
                                    (default 10000), timed
     RUN "wifi.app", scan           the chip's firmware loaded (the blob built in), its MAC address, and the networks
                                    it hears

   The pins (rear I/O header): PB13 SPI2 clock (pin 40), PB15 data (38), PI3 CS (33), PI8 WL_ON (32). They are put
   back as they were at the end, with the radio off. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "cmm2.h"
#include "cyw43.h"
#include "cyw43_country.h"
#include "cyw43_internal.h"
#include "cyw43_spi.h"

uint32_t cyw43_cmm2_bus_errors(void);

static cyw43_int_t tself;                       // (the bus test's own state: cyw43_state stays untouched)

static int bus_test(int n) {
    cyw43_int_t *self = &tself;
    if (cyw43_spi_init(self) != 0) {
        printf("SPI2 set-up failed\n");
        return 1;
    }
    cyw43_spi_gpio_setup();
    cyw43_spi_reset();
    uint32_t v = 0;
    int tries;
    for (tries = 1; tries <= 10; tries++) {
        v = read_reg_u32_swap(self, BUS_FUNCTION, SPI_READ_TEST_REGISTER);
        if (v == 0xFEEDBEADu) {
            break;
        }
        cyw43_delay_ms(1);
    }
    printf("Test register, 16-bit mode: %08lX (try %d)\n", (unsigned long)v, tries);
    if (v != 0xFEEDBEADu) {
        cyw43_spi_deinit(self);
        return 1;
    }
    // as cyw43_ll_bus_init: 32-bit words, big-endian, high-speed mode, the IRQ active high
    uint32_t val = WORD_LENGTH_32 | ENDIAN_BIG | HIGH_SPEED_MODE | WAKE_UP | 0x4 << (8 * SPI_RESPONSE_DELAY) |
                   INTR_WITH_STATUS << (8 * SPI_STATUS_ENABLE) | INTERRUPT_POLARITY_HIGH;
    write_reg_u32_swap(self, BUS_FUNCTION, SPI_BUS_CONTROL, val);
    printf("Bus control: %08lX\n", (unsigned long)cyw43_read_reg_u32(self, BUS_FUNCTION, SPI_BUS_CONTROL));
    v = cyw43_read_reg_u32(self, BUS_FUNCTION, SPI_READ_TEST_REGISTER);
    printf("Test register, 32-bit mode: %08lX\n", (unsigned long)v);
    int bad = 0;
    uint32_t x = 12345;
    uint64_t t0 = cmm2->micros();
    for (int i = 0; i < n; i++) {
        x = x * 1664525u + 1013904223u;
        cyw43_write_reg_u32(self, BUS_FUNCTION, 0x18, x);           // the R/W test register
        if (cyw43_read_reg_u32(self, BUS_FUNCTION, 0x18) != x) {
            bad++;
        }
        if (cyw43_read_reg_u32(self, BUS_FUNCTION, SPI_READ_TEST_REGISTER) != 0xFEEDBEADu) {
            bad++;
        }
        if ((i & 255) == 0 && cmm2->poll()) {
            n = i + 1;
            break;
        }
    }
    uint64_t dt = cmm2->micros() - t0;
    printf("%d rounds (%d transfers) in %lu ms: %d errors, %lu bus errors, %lu transfers a second\n", n, 3 * n,
           (unsigned long)(dt / 1000), bad, (unsigned long)cyw43_cmm2_bus_errors(),
           (unsigned long)(dt ? 3ull * n * 1000000ull / dt : 0));
    cyw43_spi_deinit(self);
    return bad != 0;
}

#define MAXNETS 64
static uint8_t seen[MAXNETS][6];
static int nseen;

static int scan_cb(void *env, const cyw43_ev_scan_result_t *r) {
    (void)env;
    for (int i = 0; i < nseen; i++) {
        if (!memcmp(seen[i], r->bssid, 6)) {
            return 0;
        }
    }
    if (nseen < MAXNETS) {
        memcpy(seen[nseen++], r->bssid, 6);
    }
    char ssid[33];
    int len = r->ssid_len > 32 ? 32 : r->ssid_len;
    memcpy(ssid, r->ssid, len);
    ssid[len] = 0;
    const char *auth = r->auth_mode == 0 ? "open" : (r->auth_mode & 4) ? "WPA2" : (r->auth_mode & 2) ? "WPA" :
                       (r->auth_mode & 1) ? "WEP" : "?";
    printf("  %-32s  ch %2u  %4d dBm  %-4s  %02X:%02X:%02X:%02X:%02X:%02X\n", ssid, r->channel, r->rssi, auth,
           r->bssid[0], r->bssid[1], r->bssid[2], r->bssid[3], r->bssid[4], r->bssid[5]);
    return 0;
}

static int scan(void) {
    cyw43_init(&cyw43_state);
    uint64_t t0 = cmm2->micros();
    cyw43_wifi_set_up(&cyw43_state, CYW43_ITF_STA, true, CYW43_COUNTRY_WORLDWIDE);
    uint64_t t1 = cmm2->micros();
    uint8_t mac[6];
    if (!(cyw43_state.itf_state & (1 << CYW43_ITF_STA)) || cyw43_wifi_get_mac(&cyw43_state, CYW43_ITF_STA, mac) != 0) {
        printf("The radio did not start (%lu bus errors)\n", (unsigned long)cyw43_cmm2_bus_errors());
        cyw43_deinit(&cyw43_state);
        return 1;
    }
    printf("Radio up in %lu ms, MAC %02X:%02X:%02X:%02X:%02X:%02X\n", (unsigned long)((t1 - t0) / 1000), mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
    cyw43_wifi_scan_options_t opts = {0};
    int r = cyw43_wifi_scan(&cyw43_state, &opts, NULL, scan_cb);
    if (r != 0) {
        printf("Scan failed: %d\n", r);
    } else {
        printf("Networks:\n");
        uint64_t ts = cmm2->micros();
        while (cyw43_wifi_scan_active(&cyw43_state)) {
            cyw43_cmm2_poll();
            if (cmm2->poll()) {
                break;
            }
        }
        printf("%d found in %lu ms; %lu bus errors\n", nseen, (unsigned long)((cmm2->micros() - ts) / 1000),
               (unsigned long)cyw43_cmm2_bus_errors());
    }
    cyw43_deinit(&cyw43_state);
    return r != 0;
}

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "scan";
    int rc = 0;
    cyw43_cmm2_save_pins();
    if (!strcasecmp(cmd, "test")) {
        rc = bus_test(argc > 2 ? atoi(argv[2]) : 10000);
    } else if (!strcasecmp(cmd, "scan")) {
        rc = scan();
    } else {
        printf("RUN \"wifi.app\", test [n] | scan\n");
    }
    cyw43_cmm2_restore_pins();
    return rc;
}
