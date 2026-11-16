/* bt.c: step 1 of docs/Bluetooth_Plan.md, the Bluetooth half of the CYW43439 on Peter's RM2 module brought up from a
   loadable program, with the firmware as it is: cyw43-driver with CYW43_ENABLE_BLUETOOTH (the combined WiFi and
   Bluetooth firmware, then the Bluetooth patch over the shared bus: pico-sdk's cybt_shared_bus), BTstack's HCI over
   pico-sdk's CYW43 transport, on BTstack's embedded run loop. MIT License, as cmm2img.h.

     RUN "bt.app", scan [seconds]   Bluetooth started (the times it took), the radio's address, then a Classic inquiry
                                    and an LE scan together for 'seconds' (default 10): each device heard, Classic or
                                    LE, with its address, signal strength, class of device and name (Classic devices
                                    that gave no name are asked for it afterwards, as the PicoMite's BLUETOOTH SCAN
                                    does). Ctrl-C ends it early.

   The pins (rear I/O header) are put back as they were at the end, with the radio off. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "cmm2.h"
#include "cyw43.h"
#include "pico/cyw43_arch.h"
#include "btstack_config.h"
#include "btstack_defines.h"
#include "btstack_event.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "btstack_run_loop_cmm2.h"
#include "btstack_tlv.h"
#include "btstack_tlv_flash_bank.h"
#include "hal_flash_bank_memory.h"
#include "hal_time_ms.h"
#include "hal_cpu.h"
#include "hci.h"
#include "gap.h"
#include "ad_parser.h"
#include "bluetooth_data_types.h"
#include <stdarg.h>
#include "pico.h"
#include "ble/le_device_db_tlv.h"
#include "classic/btstack_link_key_db_tlv.h"
#include "pico/btstack_hci_transport_cyw43.h"
#ifdef BT_APP_DUMP
#include "hci_dump.h"
#include "hci_dump_embedded_stdout.h"
#endif

uint32_t cyw43_cmm2_bus_errors(void);
void cyw43_cmm2_poll(void);
void cyw43_cmm2_save_pins(void);
void cyw43_cmm2_restore_pins(void);

/* ---- the time, for BTstack's platform code (its run loop is port/btstack_run_loop_cmm2.c) ---- */
uint32_t hal_time_ms(void) {
    return (uint32_t)(cmm2->micros() / 1000u);
}

// pico-sdk's panic(), which its Bluetooth glue calls when the chip will not do as it is told: said, then back to MMBasic
void panic(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf("\nBluetooth: ");
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    cyw43_cmm2_restore_pins();
    cmm2->leave(1);
}

/* ---- the pairings: in RAM for this test (the firmware will keep them with the options, as the PicoMite does) ---- */
static uint8_t tlv_storage[2 * 1024];
static hal_flash_bank_memory_t hal_bank;
static btstack_tlv_flash_bank_t tlv_bank;

static void setup_tlv(void) {
    const hal_flash_bank_t *hal = hal_flash_bank_memory_init_instance(&hal_bank, tlv_storage, sizeof(tlv_storage));
    const btstack_tlv_t *tlv = btstack_tlv_flash_bank_init_instance(&tlv_bank, hal, &hal_bank);
    btstack_tlv_set_instance(tlv, &tlv_bank);
    hci_set_link_key_db(btstack_link_key_db_tlv_get_instance(tlv, &tlv_bank));
    le_device_db_tlv_configure(tlv, &tlv_bank);
}

/* ---- the devices heard ---- */
#define MAXDEV 48
typedef struct {
    bd_addr_t addr;
    uint8_t le, named, name_tried, psrm;
    int8_t rssi;
    uint32_t cod;
    uint16_t clock_offset;
    char name[40];
} dev_t;
static dev_t devs[MAXDEV];
static int ndev;
static int state;                               // 0 starting, 1 scanning, 2 asking names, 3 done
static uint64_t t_start, t_up;
static uint32_t scan_secs = 10;
static btstack_packet_callback_registration_t hci_cb;

static dev_t *find(const bd_addr_t a, int le) {
    for (int i = 0; i < ndev; i++)
        if (devs[i].le == le && !memcmp(devs[i].addr, a, 6)) return &devs[i];
    if (ndev == MAXDEV) return NULL;
    dev_t *d = &devs[ndev++];
    memset(d, 0, sizeof(*d));
    memcpy(d->addr, a, 6);
    d->le = (uint8_t)le;
    return d;
}

static void set_name(dev_t *d, const uint8_t *s, int n) {
    if (n > (int)sizeof(d->name) - 1) n = sizeof(d->name) - 1;
    memcpy(d->name, s, n);
    d->name[n] = 0;
    d->named = 1;
}

static const char *kind(uint32_t cod) {         // as the PicoMite's BLUETOOTH SCAN marks them
    uint32_t major = (cod >> 8) & 0x1F;
    if (major == 4) return "Audio";
    if (major == 5 && (cod & 0x40)) return "Keyboard";
    if (major == 5) return "Peripheral";
    if (major == 1) return "Computer";
    if (major == 2) return "Phone";
    return "";
}

static int ask_next_name(void) {
    for (int i = 0; i < ndev; i++) {
        dev_t *d = &devs[i];
        if (!d->le && !d->named && !d->name_tried) {
            d->name_tried = 1;
            gap_remote_name_request(d->addr, d->psrm, d->clock_offset | 0x8000);
            return 1;
        }
    }
    return 0;
}

static void packet_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (type != HCI_EVENT_PACKET) return;
    bd_addr_t a;
    dev_t *d;
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
#ifdef BT_APP_DUMP
        printf("[BTstack state %d after %lu ms]\n", btstack_event_state_get_state(packet),
               (unsigned long)((cmm2->micros() - t_start) / 1000));
#endif
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING && state == 0) {
            t_up = cmm2->micros();
            gap_local_bd_addr(a);
            printf("Bluetooth up in %lu ms: address %s, %lu bus errors\n", (unsigned long)((t_up - t_start) / 1000),
                   bd_addr_to_str(a), (unsigned long)cyw43_cmm2_bus_errors());
            printf("Scanning for %lu s (Classic inquiry and LE scan)...\n", (unsigned long)scan_secs);
            state = 1;
            gap_set_scan_parameters(1, 0x0030, 0x0030);     // (active, so names come in scan responses)
            gap_start_scan();
            gap_inquiry_start((uint8_t)((scan_secs * 100 + 127) / 128));
        }
        break;
    case GAP_EVENT_INQUIRY_RESULT:
        gap_event_inquiry_result_get_bd_addr(packet, a);
        if ((d = find(a, 0)) == NULL) break;
        d->cod = gap_event_inquiry_result_get_class_of_device(packet);
        d->psrm = gap_event_inquiry_result_get_page_scan_repetition_mode(packet);
        d->clock_offset = gap_event_inquiry_result_get_clock_offset(packet);
        if (gap_event_inquiry_result_get_rssi_available(packet)) d->rssi = (int8_t)gap_event_inquiry_result_get_rssi(packet);
        if (gap_event_inquiry_result_get_name_available(packet))
            set_name(d, gap_event_inquiry_result_get_name(packet), gap_event_inquiry_result_get_name_len(packet));
        break;
    case GAP_EVENT_INQUIRY_COMPLETE:
        if (state == 1) {
            gap_stop_scan();
            state = 2;
            if (!ask_next_name()) state = 3;
        }
        break;
    case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
        reverse_bd_addr(&packet[3], a);
        if (packet[2] == 0 && (d = find(a, 0)) != NULL) {
            const uint8_t *n = &packet[9];
            set_name(d, n, (int)strnlen((const char *)n, 248));
        }
        if (state == 2 && !ask_next_name()) state = 3;
        break;
    case GAP_EVENT_ADVERTISING_REPORT: {
        gap_event_advertising_report_get_address(packet, a);
        if ((d = find(a, 1)) == NULL) break;
        d->rssi = (int8_t)gap_event_advertising_report_get_rssi(packet);
        ad_context_t it;
        const uint8_t *ad = gap_event_advertising_report_get_data(packet);
        uint8_t len = gap_event_advertising_report_get_data_length(packet);
        for (ad_iterator_init(&it, len, ad); ad_iterator_has_more(&it); ad_iterator_next(&it)) {
            uint8_t t = ad_iterator_get_data_type(&it);
            if (t == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME || (t == BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME && !d->named))
                set_name(d, ad_iterator_get_data(&it), ad_iterator_get_data_len(&it));
        }
        break;
    }
    default:
        break;
    }
}

static int scan(void) {
    cyw43_arch_init();
    btstack_memory_init();
    btstack_run_loop_init(btstack_run_loop_cmm2_get_instance());
#ifdef BT_APP_DUMP
    hci_dump_init(hci_dump_embedded_stdout_get_instance());
#endif
    hci_init(hci_transport_cyw43_instance(), NULL);
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);   // (signal strength and names in the inquiry results, as BTAudio.c)
    setup_tlv();
    hci_cb.callback = &packet_handler;
    hci_add_event_handler(&hci_cb);
    printf("Starting Bluetooth (the radio's firmware, then the Bluetooth patch)...\n");
    t_start = cmm2->micros();
    int pc = hci_power_control(HCI_POWER_ON);
    if (pc) printf("hci_power_control: %d\n", pc);
    uint64_t limit = t_start + (uint64_t)(scan_secs + 40) * 1000000u;
    int stopped = 0;
    while (state != 3) {
        cyw43_cmm2_poll();                      // (the driver: Bluetooth's data flagged to BTstack when the chip says)
        btstack_run_loop_cmm2_poll();
        if (cmm2->poll()) {
            stopped = 1;
            break;
        }
        if (cmm2->micros() > limit) {
            printf("Timed out (state %d)\n", state);
            break;
        }
    }
    if (state == 0) {
        printf("Bluetooth did not start (%lu bus errors)\n", (unsigned long)cyw43_cmm2_bus_errors());
    } else {
        int nc = 0, nl = 0;
        printf("\n%-18s %-4s %5s  %-10s %s\n", "Address", "", "RSSI", "Kind", "Name");
        for (int i = 0; i < ndev; i++) {
            dev_t *d = &devs[i];
            printf("%-18s %-4s %5d  %-10s %s\n", bd_addr_to_str(d->addr), d->le ? "LE" : "BR", d->rssi,
                   d->le ? "" : kind(d->cod), d->named ? d->name : "");
            if (d->le) nl++; else nc++;
        }
        printf("%d Classic, %d LE in %lu ms%s; %lu bus errors\n", nc, nl,
               (unsigned long)((cmm2->micros() - t_up) / 1000), stopped ? " (stopped)" : "",
               (unsigned long)cyw43_cmm2_bus_errors());
    }
    hci_power_control(HCI_POWER_OFF);
    hci_close();
    cyw43_arch_deinit();
    return state == 3 ? 0 : 1;
}

int main(int argc, char **argv) {
    const char *cmd = argc > 1 ? argv[1] : "scan";
    int rc = 0;
    cyw43_cmm2_save_pins();
    if (!strcasecmp(cmd, "scan")) {
        if (argc > 2) scan_secs = (uint32_t)atoi(argv[2]);
        if (scan_secs < 2) scan_secs = 2;
        if (scan_secs > 60) scan_secs = 60;
        rc = scan();
    } else {
        printf("RUN \"bt.app\", scan [seconds]\n");
    }
    cyw43_cmm2_restore_pins();
    return rc;
}
