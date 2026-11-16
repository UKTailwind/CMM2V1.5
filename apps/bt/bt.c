/* bt.c: step 1 of docs/Bluetooth_Plan.md, the Bluetooth half of the CYW43439 on Peter's RM2 module brought up from a
   loadable program, with the firmware as it is: cyw43-driver with CYW43_ENABLE_BLUETOOTH (the combined WiFi and
   Bluetooth firmware, then the Bluetooth patch over the shared bus: pico-sdk's cybt_shared_bus), BTstack's HCI over
   pico-sdk's CYW43 transport, on BTstack's embedded run loop. MIT License, as cmm2img.h.

     RUN "bt.app", scan [seconds]   Bluetooth started (the times it took), the radio's address, then a Classic inquiry
                                    and an LE scan together for 'seconds' (default 10): each device heard, Classic or
                                    LE, with its address, signal strength, class of device and name (Classic devices
                                    that gave no name are asked for it afterwards, as the PicoMite's BLUETOOTH SCAN
                                    does). Ctrl-C ends it early.
     RUN "bt.app", kbd address [seconds]  step 2: a Bluetooth Classic keyboard (its address from scan) connected as
                                    the PicoMite's BTClassicKeyboard.c connects one: report protocol, Passkey Entry (the
                                    six digits shown here are typed on the keyboard, then Enter); then each report it
                                    sends, raw and with the keys newly down, until Ctrl-C or 'seconds' (default 120).
                                    The pairing is kept in RAM only, so it is made again each time.
     RUN "bt.app", ble [seconds]    step 2's other half: the first BLE keyboard (or mouse) advertising the HID service
                                    connected as the PicoMite's BTKeyboard.c connects one (LE Secure Connections or
                                    legacy, a passkey shown if it asks for one), its HID service read over GATT, then
                                    each report, until Ctrl-C or 'seconds' (default 120).

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
#include "classic/hid_host.h"
#include "ble/sm.h"
#include "ble/gatt_client.h"
#include "ble/gatt-service/hids_host.h"
#include "bluetooth_gatt.h"
#include "classic/sdp_server.h"
#include "l2cap.h"
#include "btstack_util.h"
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

/* ---- step 2: a Classic keyboard, as the PicoMite's BTClassicKeyboard.c connects one ---- */
static uint8_t kb_descriptor[384];             // (hid_host keeps the pointer: BTClassicKeyboard.c's CK_DESCRIPTOR_SIZE)
static bd_addr_t kb_addr;
static uint16_t kb_cid;
static int kb_state;                            // 0 waiting for HCI, 1 connecting, 2 connected, 3 closed or failed
static uint32_t kb_reports;
static uint8_t kb_last[8];                      // (the last report's key slots, for the keys newly down)
static btstack_packet_callback_registration_t kb_hci_cb;

static char kb_ascii(uint8_t k, int shift) {    // (US layout, for the test's output only)
    static const char lo[] = "abcdefghijklmnopqrstuvwxyz1234567890\n\033\b\t -=[]\\#;'`,./";
    static const char hi[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()\n\033\b\t _+{}|~:\"~<>?";
    if (k >= 0x04 && k <= 0x38) return (shift ? hi : lo)[k - 4];
    return 0;
}

// a keyboard report's body (modifiers, reserved, six key slots, as the boot layout): the keys newly down
static void kb_keys(const uint8_t *k) {
    int shift = (k[0] & 0x22) != 0;
    for (int i = 2; i < 8; i++) {
        if (!k[i] || memchr(kb_last, k[i], 6)) continue;
        char c = kb_ascii(k[i], shift);
        if (c == '\n') printf(" <Enter>");
        else if (c >= ' ') printf(" '%c'", c);
        else printf(" key %02X", k[i]);
    }
    memcpy(kb_last, &k[2], 6);
}

static void kb_report(const uint8_t *r, uint16_t len) {
    kb_reports++;
    printf("[report");
    for (int i = 0; i < len && i < 16; i++) printf(" %02X", r[i]);
    printf("]");
    // Classic, report protocol: the HIDP header (A1, input data), report ID 1, then the boot layout
    if (len >= 10 && r[0] == 0xA1 && r[1] == 1) kb_keys(&r[2]);
    printf("\n");
}

static void kb_hci_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (type != HCI_EVENT_PACKET) return;
    bd_addr_t a;
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING && kb_state == 0) {
            t_up = cmm2->micros();
            printf("Bluetooth up in %lu ms; connecting to %s...\n", (unsigned long)((t_up - t_start) / 1000),
                   bd_addr_to_str(kb_addr));
            // a display, and protection against eavesdropping: Passkey Entry (BTClassicKeyboard.c, bt_ckbd_connect)
            gap_ssp_set_io_capability(SSP_IO_CAPABILITY_DISPLAY_ONLY);
            gap_ssp_set_authentication_requirement(SSP_IO_AUTHREQ_MITM_PROTECTION_REQUIRED_GENERAL_BONDING);
            uint8_t st = hid_host_connect(kb_addr, HID_PROTOCOL_MODE_REPORT, &kb_cid);
            if (st != ERROR_CODE_SUCCESS) {
                printf("hid_host_connect: Bluetooth error 0x%02X\n", st);
                kb_state = 3;
            } else {
                kb_state = 1;
            }
        }
        break;
    case HCI_EVENT_USER_PASSKEY_NOTIFICATION:
        printf("\n  Type %06lu then Enter on the keyboard\n\n",
               (unsigned long)hci_event_user_passkey_notification_get_numeric_value(packet));
        break;
    case HCI_EVENT_PIN_CODE_REQUEST: {           // legacy pairing: the PIN is ours to choose, typed on the keyboard
        hci_event_pin_code_request_get_bd_addr(packet, a);
        uint32_t pin = (uint32_t)(cmm2->micros() * 2654435761u) % 1000000u;
        char s[8];
        snprintf(s, sizeof(s), "%06lu", (unsigned long)pin);
        gap_pin_code_response(a, s);
        printf("\n  Type %s then Enter on the keyboard (PIN)\n\n", s);
        break;
    }
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:   // Just Works, if the keyboard insists on it
        hci_event_user_confirmation_request_get_bd_addr(packet, a);
        gap_ssp_confirmation_response(a);
        printf("(Just Works pairing)\n");
        break;
    default:
        break;
    }
}

static void kb_hid_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (type != HCI_EVENT_PACKET || hci_event_packet_get_type(packet) != HCI_EVENT_HID_META) return;
    switch (hci_event_hid_meta_get_subevent_code(packet)) {
    case HID_SUBEVENT_INCOMING_CONNECTION:
        hid_host_accept_connection(hid_subevent_incoming_connection_get_hid_cid(packet), HID_PROTOCOL_MODE_REPORT);
        break;
    case HID_SUBEVENT_CONNECTION_OPENED: {
        uint8_t st = hid_subevent_connection_opened_get_status(packet);
        if (st == ERROR_CODE_PAGE_TIMEOUT) {    // (asleep, or not yet in pairing mode: asked again from kbd's loop)
            static int told;
            if (!told++) printf("No answer: put the keyboard in pairing mode (asking again until the time is up)\n");
            kb_state = 4;
        } else if (st != ERROR_CODE_SUCCESS) {
            printf("Connection failed: Bluetooth error 0x%02X\n", st);
            kb_state = 3;
        } else {
            printf("Connected in %lu ms: type on the keyboard (Ctrl-C at the CMM2 ends)\n",
                   (unsigned long)((cmm2->micros() - t_up) / 1000));
            kb_state = 2;
        }
        break;
    }
    case HID_SUBEVENT_DESCRIPTOR_AVAILABLE: {
        uint16_t cid = hid_subevent_descriptor_available_get_hid_cid(packet);
        printf("HID descriptor: status 0x%02X, %u bytes\n", hid_subevent_descriptor_available_get_status(packet),
               hid_descriptor_storage_get_descriptor_len(cid));
        break;
    }
    case HID_SUBEVENT_REPORT:
        kb_report(hid_subevent_report_get_report(packet), hid_subevent_report_get_report_len(packet));
        break;
    case HID_SUBEVENT_CONNECTION_CLOSED:
        printf("Disconnected\n");
        kb_state = 3;
        break;
    default:
        break;
    }
}

static int kbd(const char *addr, uint32_t secs) {
    if (!sscanf_bd_addr(addr, kb_addr)) {
        printf("Not an address: %s\n", addr);
        return 1;
    }
    cyw43_arch_init();
    btstack_memory_init();
    btstack_run_loop_init(btstack_run_loop_cmm2_get_instance());
#ifdef BT_APP_DUMP
    hci_dump_init(hci_dump_embedded_stdout_get_instance());
#endif
    hci_init(hci_transport_cyw43_instance(), NULL);
    setup_tlv();
    // the PicoMite's Classic set-up (BTAudio.c's bt_audio_init, less the speaker's parts)
    hci_set_master_slave_policy(0);
    gap_set_local_name("Colour Maximite 2");
    gap_set_class_of_device(0x000104);          // (a desktop computer)
    gap_ssp_set_auto_accept(0);
    gap_set_required_encryption_key_size(7);
    l2cap_init();
    sdp_init();
    hid_host_init(kb_descriptor, sizeof(kb_descriptor));
    hid_host_register_packet_handler(&kb_hid_handler);
    kb_hci_cb.callback = &kb_hci_handler;
    hci_add_event_handler(&kb_hci_cb);
    printf("Starting Bluetooth...\n");
    t_start = cmm2->micros();
    hci_power_control(HCI_POWER_ON);
    uint64_t limit = t_start + (uint64_t)secs * 1000000u;
    while (kb_state != 3) {
        cyw43_cmm2_poll();
        btstack_run_loop_cmm2_poll();
        if (kb_state == 4) {                    // (no answer to the page: again)
            uint8_t st = hid_host_connect(kb_addr, HID_PROTOCOL_MODE_REPORT, &kb_cid);
            kb_state = st == ERROR_CODE_SUCCESS ? 1 : 3;
            if (st != ERROR_CODE_SUCCESS) printf("hid_host_connect: Bluetooth error 0x%02X\n", st);
        }
        if (cmm2->poll()) break;
        if (cmm2->micros() > limit) {
            printf("Time up (%lu s)\n", (unsigned long)secs);
            break;
        }
    }
    printf("%lu reports; %lu bus errors\n", (unsigned long)kb_reports, (unsigned long)cyw43_cmm2_bus_errors());
    if (kb_state == 2) hid_host_disconnect(kb_cid);
    uint64_t t = cmm2->micros();
    while (cmm2->micros() - t < 300000) {       // (the disconnection sent before the radio goes off)
        cyw43_cmm2_poll();
        btstack_run_loop_cmm2_poll();
    }
    hci_power_control(HCI_POWER_OFF);
    hci_close();
    cyw43_arch_deinit();
    return kb_reports ? 0 : 1;
}

/* ---- step 2's other half: a BLE keyboard, as the PicoMite's BTKeyboard.c finds and connects one ---- */
static uint8_t le_descriptor[512];             // (hids_host keeps the pointer)
static hci_con_handle_t le_handle = HCI_CON_HANDLE_INVALID;
static uint16_t le_cid;
static int le_state;                            // 0 waiting, 1 scanning, 2 connecting, 3 connected, 4 pairing, 5 HID, 6 ready, 9 end
static uint64_t le_connected_at;
static int le_pairing_started;
static bd_addr_t le_addr, le_failed;           // (the device connecting, and the last that failed)
static uint64_t le_failed_at;
static btstack_packet_callback_registration_t le_hci_cb, le_sm_cb;

// the keys, as the PicoMite takes them: every GATT notification on the keyboard's link (BTKeyboard.c's
// bth_raw_notification_handler), whose input reports carry no report ID; 8 bytes the boot layout, 7 bytes the same
// without its reserved byte (a Logitech Pebble Keys 2 K380s, in BTKeyboard.c)
static gatt_client_notification_t le_listener;
static void le_notification_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (type != HCI_EVENT_PACKET || hci_event_packet_get_type(packet) != GATT_EVENT_NOTIFICATION) return;
    uint16_t len = gatt_event_notification_get_value_length(packet);
    const uint8_t *v = gatt_event_notification_get_value(packet);
    kb_reports++;
    printf("[notification 0x%04X:", gatt_event_notification_get_value_handle(packet));
    for (int i = 0; i < len && i < 16; i++) printf(" %02X", v[i]);
    printf("]");
    if (len == 8) {
        kb_keys(v);
    } else if (len == 7) {
        uint8_t boot[8] = {v[0], 0, v[1], v[2], v[3], v[4], v[5], v[6]};
        kb_keys(boot);
    }
    printf("\n");
}

static int ad_has_uuid16(const uint8_t *ad, uint8_t len, uint16_t uuid) {
    ad_context_t it;
    for (ad_iterator_init(&it, len, ad); ad_iterator_has_more(&it); ad_iterator_next(&it)) {
        uint8_t t = ad_iterator_get_data_type(&it);
        if (t != BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS &&
            t != BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS) continue;
        const uint8_t *d = ad_iterator_get_data(&it);
        for (int i = 0; i + 1 < ad_iterator_get_data_len(&it); i += 2)
            if (little_endian_read_16(d, i) == uuid) return 1;
    }
    return 0;
}

static void le_hids_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (type != HCI_EVENT_PACKET || hci_event_packet_get_type(packet) != HCI_EVENT_GATTSERVICE_META) return;
    switch (hci_event_gattservice_meta_get_subevent_code(packet)) {
    case GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED: {
        uint8_t st = gattservice_subevent_hid_service_connected_get_status(packet);
        if (st != ERROR_CODE_SUCCESS) {
            printf("HID service: Bluetooth error 0x%02X\n", st);
            le_state = 9;
        } else {
            printf("HID service connected (%u instance(s)) %lu ms after the link: type on the keyboard (Ctrl-C at the "
                   "CMM2 ends)\n", gattservice_subevent_hid_service_connected_get_num_instances(packet),
                   (unsigned long)((cmm2->micros() - le_connected_at) / 1000));
            le_state = 6;
            gatt_client_listen_for_characteristic_value_updates(&le_listener, &le_notification_handler, le_handle, NULL);
        }
        break;
    }
    case GATTSERVICE_SUBEVENT_HID_REPORT: {
        const uint8_t *r = gattservice_subevent_hid_report_get_report(packet);
        uint16_t len = gattservice_subevent_hid_report_get_report_len(packet);
        kb_reports++;
        printf("[report id %u:", gattservice_subevent_hid_report_get_report_id(packet));
        for (int i = 0; i < len && i < 16; i++) printf(" %02X", r[i]);
        printf("]");
        if (len == 8) kb_keys(r);           // (a keyboard's input report: the boot layout)
        printf("\n");
        break;
    }
    default:
        break;
    }
}

static void le_handler(uint8_t type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (type != HCI_EVENT_PACKET) return;
    bd_addr_t a;
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING && le_state == 0) {
            t_up = cmm2->micros();
            printf("Bluetooth up in %lu ms; looking for a BLE keyboard (one advertising the HID service): put it in "
                   "pairing mode\n", (unsigned long)((t_up - t_start) / 1000));
            gap_set_scan_parameters(1, 0x0030, 0x0030);
            gap_start_scan();
            le_state = 1;
        }
        break;
    case GAP_EVENT_ADVERTISING_REPORT: {
        if (le_state != 1) break;
        const uint8_t *ad = gap_event_advertising_report_get_data(packet);
        uint8_t len = gap_event_advertising_report_get_data_length(packet);
        if (!ad_has_uuid16(ad, len, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE)) break;
        // distant strangers left alone, as BTKeyboard.c's BTH_SCAN_RSSI_MIN (a speaker's BLE side that advertises the HID
        // service for its buttons, 3 m away at -88, was taken and failed again and again: 0x3E)
        if ((int8_t)gap_event_advertising_report_get_rssi(packet) < -70) break;
        gap_event_advertising_report_get_address(packet, a);
        if (!memcmp(a, le_failed, 6) && cmm2->micros() - le_failed_at < 10000000u) break;   // (it failed: not again yet)
        bd_addr_type_t at = (bd_addr_type_t)gap_event_advertising_report_get_address_type(packet);
        char name[40] = "";
        ad_context_t it;
        for (ad_iterator_init(&it, len, ad); ad_iterator_has_more(&it); ad_iterator_next(&it)) {
            uint8_t t = ad_iterator_get_data_type(&it);
            if (t == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME || t == BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME) {
                int n = ad_iterator_get_data_len(&it);
                if (n > 39) n = 39;
                memcpy(name, ad_iterator_get_data(&it), n);
                name[n] = 0;
            }
        }
        printf("HID advertiser %s (type %u, RSSI %d) \"%s\": connecting\n", bd_addr_to_str(a), at,
               (int8_t)gap_event_advertising_report_get_rssi(packet), name);
        gap_stop_scan();
        memcpy(le_addr, a, 6);
        gap_connect(a, at);
        le_state = 2;
        break;
    }
    case HCI_EVENT_LE_META:
        if (hci_event_le_meta_get_subevent_code(packet) != HCI_SUBEVENT_LE_CONNECTION_COMPLETE || le_state != 2) break;
        if (hci_subevent_le_connection_complete_get_status(packet)) {
            printf("LE connection failed: Bluetooth error 0x%02X; looking again\n",
                   hci_subevent_le_connection_complete_get_status(packet));
            gap_start_scan();
            le_state = 1;
            break;
        }
        le_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
        le_connected_at = cmm2->micros();
        le_pairing_started = 0;
        le_state = 3;
        printf("LE connected; waiting for the keyboard to ask for pairing\n");
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (hci_event_disconnection_complete_get_connection_handle(packet) != le_handle) break;
        printf("Disconnected (reason 0x%02X)\n", hci_event_disconnection_complete_get_reason(packet));
        if (le_state != 6) {                    // (never got as far as its reports: left alone for a while)
            memcpy(le_failed, le_addr, 6);
            le_failed_at = cmm2->micros();
        }
        le_handle = HCI_CON_HANDLE_INVALID;
        le_state = le_state == 6 ? 9 : 1;
        if (le_state == 1) gap_start_scan();
        break;
    case SM_EVENT_JUST_WORKS_REQUEST:
        printf("(Just Works pairing)\n");
        sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
        break;
    case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
        printf("(Numeric Comparison %06lu, confirmed)\n",
               (unsigned long)sm_event_numeric_comparison_request_get_passkey(packet));
        sm_numeric_comparison_confirm(sm_event_numeric_comparison_request_get_handle(packet));
        break;
    case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
        printf("\n  Type %06lu then Enter on the keyboard\n\n",
               (unsigned long)sm_event_passkey_display_number_get_passkey(packet));
        break;
    case SM_EVENT_PAIRING_STARTED:
        le_pairing_started = 1;
        le_state = 4;
        printf("Pairing...\n");
        break;
    case SM_EVENT_PAIRING_COMPLETE: {
        uint8_t st = sm_event_pairing_complete_get_status(packet);
        if (st != ERROR_CODE_SUCCESS) {
            printf("Pairing failed: status 0x%02X, reason 0x%02X\n", st, sm_event_pairing_complete_get_reason(packet));
            gap_disconnect(sm_event_pairing_complete_get_handle(packet));
            break;
        }
        printf("Paired, %lu ms after the link; reading the HID service...\n",
               (unsigned long)((cmm2->micros() - le_connected_at) / 1000));
        le_state = 5;
        uint8_t hs = hids_host_connect(le_handle, &le_hids_handler, HID_PROTOCOL_MODE_REPORT, &le_cid);
        if (hs != ERROR_CODE_SUCCESS) printf("hids_host_connect: Bluetooth error 0x%02X\n", hs);
        break;
    }
    default:
        break;
    }
}

static int ble(uint32_t secs) {
    cyw43_arch_init();
    btstack_memory_init();
    btstack_run_loop_init(btstack_run_loop_cmm2_get_instance());
#ifdef BT_APP_DUMP
    hci_dump_init(hci_dump_embedded_stdout_get_instance());
#endif
    hci_init(hci_transport_cyw43_instance(), NULL);
    setup_tlv();
    // the PicoMite's BLE host set-up (BTKeyboard.c, bt_keyboard_init)
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_KEYBOARD_DISPLAY);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING | SM_AUTHREQ_SECURE_CONNECTION);
    sm_set_secure_connections_only_mode(false);
    sm_set_encryption_key_size_range(7, 16);
    gatt_client_init();
    hids_host_init(le_descriptor, sizeof(le_descriptor));
    le_hci_cb.callback = &le_handler;
    hci_add_event_handler(&le_hci_cb);
    le_sm_cb.callback = &le_handler;
    sm_add_event_handler(&le_sm_cb);
    printf("Starting Bluetooth...\n");
    t_start = cmm2->micros();
    hci_power_control(HCI_POWER_ON);
    uint64_t limit = t_start + (uint64_t)secs * 1000000u;
    while (le_state != 9) {
        cyw43_cmm2_poll();
        btstack_run_loop_cmm2_poll();
        // the keyboard did not ask for pairing within 2 s: ask it (BTKeyboard.c's pair_kickoff_timeout)
        if (le_state == 3 && !le_pairing_started && cmm2->micros() - le_connected_at > 2000000u) {
            printf("No request from the keyboard: asking it to pair\n");
            le_pairing_started = 1;
            sm_request_pairing(le_handle);
        }
        if (cmm2->poll()) break;
        if (cmm2->micros() > limit) {
            printf("Time up (%lu s)\n", (unsigned long)secs);
            break;
        }
    }
    printf("%lu reports; %lu bus errors\n", (unsigned long)kb_reports, (unsigned long)cyw43_cmm2_bus_errors());
    if (le_handle != HCI_CON_HANDLE_INVALID) gap_disconnect(le_handle);
    uint64_t t = cmm2->micros();
    while (cmm2->micros() - t < 300000) {
        cyw43_cmm2_poll();
        btstack_run_loop_cmm2_poll();
    }
    hci_power_control(HCI_POWER_OFF);
    hci_close();
    cyw43_arch_deinit();
    return kb_reports ? 0 : 1;
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
    } else if (!strcasecmp(cmd, "kbd") && argc > 2) {
        rc = kbd(argv[2], argc > 3 ? (uint32_t)atoi(argv[3]) : 120);
    } else if (!strcasecmp(cmd, "ble")) {
        rc = ble(argc > 2 ? (uint32_t)atoi(argv[2]) : 120);
    } else {
        printf("RUN \"bt.app\", scan [seconds] | kbd address [seconds] | ble [seconds]\n");
    }
    cyw43_cmm2_restore_pins();
    return rc;
}
