/* wifi.c: step 1 of docs/Web_Plan.md, the CYW43439 on Peter's RM2 module driven from a loadable program, with the
   firmware as it is: cyw43-driver (as the PicoMite builds it) over SPI2 in half-duplex (port/cyw43_bus_cmm2.c).
   MIT License, as cmm2img.h.

     RUN "wifi.app", test [n]       the bus alone: the radio powered up, the test register in 16-bit mode, then the
                                    chip in 32-bit high-speed mode (as cyw43_ll.c sets it) and n rounds of a random
                                    word written to the R/W test register and read back and the test register read
                                    (default 10000), timed
     RUN "wifi.app", scan           the chip's firmware loaded (the blob built in), its MAC address, and the networks
                                    it hears
     RUN "wifi.app", join [ssid pw] the network joined (WPA2, or open with no password), an address by DHCP, the
                                    gateway pinged; then, until a key, it answers pings and serves two TCP ports for
                                    timing: 5001 sends to whatever connects (as fast as it can, until it closes), 5002
                                    takes whatever is sent and throws it away. Each connection's bytes and rate are
                                    printed when it closes. With no ssid, A:/wifi/wifi.cfg gives it: the SSID on its
                                    first line, the password on its second.

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
#include "pico/cyw43_arch.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/raw.h"
#include "lwip/tcp.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip4.h"

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
    cyw43_arch_init();
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

// ---- join: DHCP, ping, and two TCP ports for timing ----

#define PING_ID 0xC2C2
static struct raw_pcb *ping_pcb;
static uint16_t ping_seq;
static uint64_t ping_sent_us;
static volatile int ping_got;
static uint32_t ping_rtt_us;

static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr) {
    (void)arg; (void)pcb; (void)addr;
    if (p->tot_len >= IP_HLEN + sizeof(struct icmp_echo_hdr)) {
        uint16_t hl = IPH_HL_BYTES((struct ip_hdr *)p->payload);
        if (pbuf_remove_header(p, hl) == 0) {
            struct icmp_echo_hdr *e = (struct icmp_echo_hdr *)p->payload;
            if (e->type == ICMP_ER && e->id == PING_ID && lwip_ntohs(e->seqno) == ping_seq) {
                ping_rtt_us = (uint32_t)(cmm2->micros() - ping_sent_us);
                ping_got = 1;
                pbuf_free(p);
                return 1;                       // eaten
            }
            pbuf_add_header(p, hl);
        }
    }
    return 0;
}

static int ping_once(const ip_addr_t *to, uint32_t timeout_ms) {
    const int len = sizeof(struct icmp_echo_hdr) + 32;
    struct pbuf *p = pbuf_alloc(PBUF_IP, len, PBUF_RAM);
    if (!p) {
        return -1;
    }
    struct icmp_echo_hdr *e = (struct icmp_echo_hdr *)p->payload;
    ICMPH_TYPE_SET(e, ICMP_ECHO);
    ICMPH_CODE_SET(e, 0);
    e->chksum = 0;
    e->id = PING_ID;
    e->seqno = lwip_htons(++ping_seq);
    for (int i = 0; i < 32; i++) {
        ((uint8_t *)e)[sizeof(*e) + i] = (uint8_t)i;
    }
    e->chksum = inet_chksum(e, len);
    ping_got = 0;
    ping_sent_us = cmm2->micros();
    raw_sendto(ping_pcb, p, to);
    pbuf_free(p);
    while (!ping_got && cmm2->micros() - ping_sent_us < timeout_ms * 1000ull) {
        cyw43_arch_poll();
    }
    return ping_got ? (int)ping_rtt_us : -1;
}

static uint8_t txbuf[4 * TCP_MSS];

typedef struct {
    struct tcp_pcb *pcb;
    uint64_t bytes, t0;
    int port;
} conn_t;
static conn_t conns[2];                         // [0] 5001, sending; [1] 5002, receiving

static void conn_end(conn_t *c, const char *why) {
    uint64_t dt = cmm2->micros() - c->t0;
    printf("Port %d: %s, %llu bytes in %lu ms = %lu KB/s\n", c->port, why, (unsigned long long)c->bytes,
           (unsigned long)(dt / 1000), (unsigned long)(dt ? c->bytes * 1000000ull / dt / 1024 : 0));
    if (c->pcb) {
        tcp_arg(c->pcb, NULL);
        tcp_sent(c->pcb, NULL);
        tcp_recv(c->pcb, NULL);
        tcp_err(c->pcb, NULL);
        if (tcp_close(c->pcb) != ERR_OK) {
            tcp_abort(c->pcb);
        }
        c->pcb = NULL;
    }
}

static void tx_fill(struct tcp_pcb *pcb) {
    for (;;) {
        u16_t n = tcp_sndbuf(pcb);
        if (n > sizeof(txbuf)) {
            n = sizeof(txbuf);
        }
        if (n < TCP_MSS || tcp_sndqueuelen(pcb) >= TCP_SND_QUEUELEN - 4) {
            break;
        }
        if (tcp_write(pcb, txbuf, n, 0) != ERR_OK) {
            break;
        }
    }
    tcp_output(pcb);
}

static err_t tx_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    conn_t *c = (conn_t *)arg;
    c->bytes += len;
    tx_fill(pcb);
    return ERR_OK;
}

static err_t conn_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    conn_t *c = (conn_t *)arg;
    (void)err;
    if (p == NULL) {
        conn_end(c, "closed by the PC");
        return ERR_OK;
    }
    if (c->port == 5002) {
        c->bytes += p->tot_len;
    }
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static void conn_err(void *arg, err_t err) {
    conn_t *c = (conn_t *)arg;
    c->pcb = NULL;                              // (lwIP has freed it)
    conn_end(c, err == ERR_RST ? "reset" : "error");
}

static err_t conn_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    conn_t *c = (conn_t *)arg;
    if (err != ERR_OK || pcb == NULL) {
        return ERR_VAL;
    }
    if (c->pcb) {                               // one at a time on each port
        tcp_abort(pcb);
        return ERR_ABRT;
    }
    c->pcb = pcb;
    c->bytes = 0;
    c->t0 = cmm2->micros();
    tcp_arg(pcb, c);
    tcp_recv(pcb, conn_recv);
    tcp_err(pcb, conn_err);
    if (c->port == 5001) {
        tcp_sent(pcb, tx_sent);
        tx_fill(pcb);
    }
    printf("Port %d: connection from %s\n", c->port, ipaddr_ntoa(&pcb->remote_ip));
    return ERR_OK;
}

static int listen_on(conn_t *c, int port) {
    c->port = port;
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb || tcp_bind(pcb, IP_ADDR_ANY, port) != ERR_OK) {
        return -1;
    }
    pcb = tcp_listen(pcb);
    tcp_arg(pcb, c);
    tcp_accept(pcb, conn_accept);
    return 0;
}

static int read_cfg(char *ssid, char *pw, int n) {
    FILE *f = fopen("A:/wifi/wifi.cfg", "r");
    if (!f) {
        return -1;
    }
    ssid[0] = pw[0] = 0;
    if (fgets(ssid, n, f)) {
        if (!fgets(pw, n, f)) {
            pw[0] = 0;
        }
    }
    fclose(f);
    ssid[strcspn(ssid, "\r\n")] = 0;
    pw[strcspn(pw, "\r\n")] = 0;
    return ssid[0] ? 0 : -1;
}

static int join(const char *ssid, const char *pw) {
    for (unsigned i = 0; i < sizeof(txbuf); i++) {
        txbuf[i] = (uint8_t)(i * 7);
    }
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();
    if (!(cyw43_state.itf_state & (1 << CYW43_ITF_STA))) {
        printf("The radio did not start (%lu bus errors)\n", (unsigned long)cyw43_cmm2_bus_errors());
        cyw43_arch_deinit();
        return 1;
    }
    printf("Joining %s\n", ssid);
    uint64_t t0 = cmm2->micros();
    int r = cyw43_arch_wifi_connect_timeout_ms(ssid, *pw ? pw : NULL, *pw ? CYW43_AUTH_WPA2_MIXED_PSK : CYW43_AUTH_OPEN, 30000);
    if (r != 0) {
        printf("Not joined: %d (%s)\n", r, cyw43_tcpip_link_status_name(cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA)));
        cyw43_arch_deinit();
        return 1;
    }
    struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
    int32_t rssi = 0;
    cyw43_wifi_get_rssi(&cyw43_state, &rssi);
    printf("Joined in %lu ms: address %s", (unsigned long)((cmm2->micros() - t0) / 1000), ip4addr_ntoa(netif_ip4_addr(n)));
    printf(", mask %s", ip4addr_ntoa(netif_ip4_netmask(n)));
    printf(", gateway %s, %ld dBm\n", ip4addr_ntoa(netif_ip4_gw(n)), (long)rssi);

    ping_pcb = raw_new(IP_PROTO_ICMP);
    raw_recv(ping_pcb, ping_recv, NULL);
    raw_bind(ping_pcb, IP_ADDR_ANY);
    ip_addr_t gw;
    ip_addr_copy_from_ip4(gw, *netif_ip4_gw(n));
    for (int i = 0; i < 4; i++) {
        int us = ping_once(&gw, 1000);
        if (us < 0) {
            printf("Ping %s: no answer\n", ipaddr_ntoa(&gw));
        } else {
            printf("Ping %s: %d.%d ms\n", ipaddr_ntoa(&gw), us / 1000, (us % 1000) / 100);
        }
    }

    if (listen_on(&conns[0], 5001) || listen_on(&conns[1], 5002)) {
        printf("TCP listen failed\n");
    }
    printf("Serving at %s: ping it; TCP 5001 sends, 5002 receives. Any key ends.\n", ip4addr_ntoa(netif_ip4_addr(n)));
    while (cmm2->con_getc() < 0) {
        cyw43_arch_poll();
        if (cmm2->poll()) {
            break;
        }
    }
    for (int i = 0; i < 2; i++) {
        if (conns[i].pcb) {
            conn_end(&conns[i], "ended");
        }
    }
    printf("%lu bus errors\n", (unsigned long)cyw43_cmm2_bus_errors());
    cyw43_arch_deinit();
    return 0;
}

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "scan";
    int rc = 0;
    cyw43_cmm2_save_pins();
    if (!strcasecmp(cmd, "test")) {
        rc = bus_test(argc > 2 ? atoi(argv[2]) : 10000);
    } else if (!strcasecmp(cmd, "scan")) {
        rc = scan();
    } else if (!strcasecmp(cmd, "join")) {
        static char ssid[64], pw[80];
        if (argc > 2) {
            strncpy(ssid, argv[2], sizeof(ssid) - 1);
            strncpy(pw, argc > 3 ? argv[3] : "", sizeof(pw) - 1);
            rc = join(ssid, pw);
        } else if (read_cfg(ssid, pw, sizeof(ssid)) == 0) {
            rc = join(ssid, pw);
        } else {
            printf("No ssid given, and no A:/wifi/wifi.cfg\n");
            rc = 1;
        }
    } else {
        printf("RUN \"wifi.app\", test [n] | scan | join [ssid password]\n");
    }
    cyw43_cmm2_restore_pins();
    return rc;
}
