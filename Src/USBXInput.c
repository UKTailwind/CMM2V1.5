/***********************************************************************************************************************
Xbox controllers (18 Oct, Peter), the CMM2's. Their gamepad interfaces are vendor-specific, not HID, so TinyUSB's HID
driver does not take them and neither firmware had them. This is a TinyUSB application class driver
(usbh_app_driver_get_cb) for the two wired protocols, as Linux's xpad driver handles them:

- Xbox 360, and the PC pads that use its protocol (X-input): interface class 0xFF, subclass 0x5D, protocol 1. No set-up
  is needed; the "player 1" light is set. Its 20-byte input report: byte 0 the type (0), byte 1 the length (0x14);
  byte 2 up 01, down 02, left 04, right 08, START 10, BACK 20, the stick clicks 40 and 80; byte 3 LB 01, RB 02, the
  guide button 04, A 10, B 20, X 40, Y 80; bytes 4 and 5 LT and RT, 0-255; bytes 6-13 LX, LY, RX, RY, 16-bit signed,
  up positive. Rumble: 00 08 00 left right 00 00 00.
- Xbox One and Xbox Series X|S (GIP): class 0xFF, subclass 0x47, protocol 0xD0, interface 0. Nothing is sent until the
  host sends "power on", 05 20 seq 01 00 (and 05 20 seq 0F 06 to an Xbox One S, 045E:02EA, and an Elite Series 2,
  045E:0B00, as xpad does). Input report 0x20: byte 4 MENU 04, VIEW 08, A 10, B 20, X 40, Y 80; byte 5 up 01, down 02,
  left 04, right 08, LB 10, RB 20, the stick clicks 40 and 80; bytes 6-9 LT and RT, 0-1023; bytes 10-17 LX, LY, RX, RY,
  16-bit signed, up positive. The guide button comes alone, in report 0x07 (byte 4 bit 0), which the pad sends until
  it is acknowledged when byte 1 asks for that (0x30). Rumble: 09 00 seq 09 00 0F 00 00 left right FF 00 FF, 0-100.

Each pad gets a channel from USBGamepad.c as the other gamepads do (DEVICE(GAMEPAD n, T) 131, XBOX, as for the
PicoMite's XBox pads); GAMEPAD HAPTIC n, left, right drives its motors. An IN transfer is kept waiting; what goes out
(the set-up, acknowledgements, rumble) is queued and sent one at a time. The pads' other interfaces (a headset, the
security one, audio) are left unclaimed. Written from xpad and not yet run against a genuine Xbox controller: an
EasySMX 9013 Pro would not stay in its X-input mode on the CMM2 (18 Oct).
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "tusb.h"
#include "host/usbh_pvt.h"
#include "keyboard.h"
#include "USBGamepad.h"

#define XINPUT_MAX 2                                                // pads at once
#define XOUT_QUEUE 4                                                // packets waiting to go out, per pad
#define XOUT_LEN 16

typedef struct {
    uint8_t daddr;                                                  // 0: free
    uint8_t itf_num, ep_in, ep_out;
    uint16_t in_size;
    bool one;                                                       // GIP (Xbox One, Series X|S), else Xbox 360
    int8_t slot;                                                    // its HID slot (USBGamepad.c), -1 none
    uint8_t seq;                                                    // GIP's packet count
    uint8_t sent_l, sent_r;                                         // the motors as last sent
    uint8_t q_len[XOUT_QUEUE], q_head, q_count;
    uint8_t q[XOUT_QUEUE][XOUT_LEN];
} xinput_itf_t;

static xinput_itf_t xi[XINPUT_MAX];
CFG_TUSB_MEM_ALIGN static uint8_t xi_in[XINPUT_MAX][64];
CFG_TUSB_MEM_ALIGN static uint8_t xi_out[XINPUT_MAX][XOUT_LEN];

static xinput_itf_t *find(uint8_t daddr, uint8_t ep_addr) {
    for(int k = 0; k < XINPUT_MAX; k++)
        if(xi[k].daddr == daddr && (ep_addr == 0 || xi[k].ep_in == ep_addr || xi[k].ep_out == ep_addr)) return &xi[k];
    return NULL;
}

static bool xinput_in(xinput_itf_t *e) {                           // the next input report
    int k = e - xi;
    if(!usbh_edpt_claim(e->daddr, e->ep_in)) return false;
    if(!usbh_edpt_xfer(e->daddr, e->ep_in, xi_in[k], e->in_size)) {
        usbh_edpt_release(e->daddr, e->ep_in);
        return false;
    }
    return true;
}

// the next queued packet, if the OUT endpoint is free
static void xout_kick(xinput_itf_t *e) {
    int k = e - xi;
    if(!e->ep_out || !e->q_count) return;
    if(!usbh_edpt_claim(e->daddr, e->ep_out)) return;              // busy: its completion sends the next
    memcpy(xi_out[k], e->q[e->q_head], e->q_len[e->q_head]);
    if(!usbh_edpt_xfer(e->daddr, e->ep_out, xi_out[k], e->q_len[e->q_head])) {
        usbh_edpt_release(e->daddr, e->ep_out);
        return;
    }
    e->q_head = (e->q_head + 1) % XOUT_QUEUE;
    e->q_count--;
}

static bool xout(xinput_itf_t *e, const uint8_t *data, uint8_t len) {
    if(!e->ep_out || len > XOUT_LEN || e->q_count == XOUT_QUEUE) return false;
    int t = (e->q_head + e->q_count) % XOUT_QUEUE;
    memcpy(e->q[t], data, len);
    e->q_len[t] = len;
    e->q_count++;
    xout_kick(e);
    return true;
}

static bool xinput_init(void) {
    memset(xi, 0, sizeof(xi));
    return true;
}

static bool xinput_deinit(void) {
    return true;
}

// an Xbox gamepad interface: open its interrupt endpoints, and take the descriptors up to the next interface
static uint16_t xinput_open(uint8_t rhport, uint8_t daddr, const tusb_desc_interface_t *itf, uint16_t max_len) {
    (void)rhport;
    bool x360 = itf->bInterfaceClass == 0xFF && itf->bInterfaceSubClass == 0x5D && itf->bInterfaceProtocol == 0x01;
    bool one = itf->bInterfaceClass == 0xFF && itf->bInterfaceSubClass == 0x47 && itf->bInterfaceProtocol == 0xD0 &&
               itf->bInterfaceNumber == 0 && itf->bAlternateSetting == 0 && itf->bNumEndpoints >= 2;
    if(!x360 && !one) return 0;
    xinput_itf_t *e = find(0, 0);
    if(e == NULL) return 0;
    memset(e, 0, sizeof(*e));
    e->slot = -1;
    e->one = one;
    uint16_t len = tu_desc_len(itf);
    const uint8_t *p = tu_desc_next(itf);
    while(len < max_len && tu_desc_len(p) && tu_desc_type(p) != TUSB_DESC_INTERFACE) {
        if(tu_desc_type(p) == TUSB_DESC_ENDPOINT) {
            const tusb_desc_endpoint_t *ep = (const tusb_desc_endpoint_t *)p;
            if(ep->bmAttributes.xfer == TUSB_XFER_INTERRUPT && tuh_edpt_open(daddr, ep)) {
                if(tu_edpt_dir(ep->bEndpointAddress) == TUSB_DIR_IN) {
                    e->ep_in = ep->bEndpointAddress;
                    e->in_size = tu_min16(tu_edpt_packet_size(ep), sizeof(xi_in[0]));
                } else e->ep_out = ep->bEndpointAddress;
            }
        }
        len += tu_desc_len(p);
        p = tu_desc_next(p);
    }
    if(!e->ep_in) {
        memset(e, 0, sizeof(*e));
        return 0;
    }
    e->daddr = daddr;
    e->itf_num = itf->bInterfaceNumber;
    return len;
}

static bool xinput_set_config(uint8_t daddr, uint8_t itf_num) {
    xinput_itf_t *e = find(daddr, 0);
    if(e && e->itf_num == itf_num) {
        e->slot = USBGamepadXInputMount(daddr, e->one);
        if(e->one) {
            uint16_t vid = 0, pid = 0;
            tuh_vid_pid_get(daddr, &vid, &pid);
            const uint8_t power_on[5] = {0x05, 0x20, e->seq++, 0x01, 0x00};
            xout(e, power_on, sizeof(power_on));
            if(vid == 0x045E && (pid == 0x02EA || pid == 0x0B00)) {     // an Xbox One S, an Elite Series 2
                const uint8_t s_init[5] = {0x05, 0x20, e->seq++, 0x0F, 0x06};
                xout(e, s_init, sizeof(s_init));
            }
        } else {
            static const uint8_t led[3] = {0x01, 0x03, 0x02};     // the ring of lights: player 1
            xout(e, led, sizeof(led));
        }
        xinput_in(e);
    }
    usbh_driver_set_config_complete(daddr, itf_num);
    return true;
}

static bool xinput_xfer_cb(uint8_t daddr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
    xinput_itf_t *e = find(daddr, ep_addr);
    if(e == NULL) return false;
    if(ep_addr == e->ep_in) {
        const uint8_t *r = xi_in[e - xi];
        if(result == XFER_RESULT_SUCCESS && e->slot >= 0) {
            USBGamepadXInputReport(r, (uint16_t)xferred_bytes, e->slot);
            if(e->one && xferred_bytes >= 3 && r[0] == 0x07 && r[1] == 0x30) {    // the guide button: acknowledge it
                const uint8_t ack[13] = {0x01, 0x20, r[2], 0x09, 0x00, 0x07, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
                xout(e, ack, sizeof(ack));
            }
        }
        xinput_in(e);                                               // and wait for the next
    } else xout_kick(e);                                            // the OUT endpoint is free: the next packet
    return true;
}

static void xinput_close(uint8_t daddr) {
    xinput_itf_t *e;
    while((e = find(daddr, 0)) != NULL) {
        if(e->slot >= 0) USBGamepadXInputDetached(e->slot);
        memset(e, 0, sizeof(*e));
    }
}

static const usbh_class_driver_t xinput_driver = {
    .name = "XINPUT",
    .init = xinput_init,
    .deinit = xinput_deinit,
    .open = xinput_open,
    .set_config = xinput_set_config,
    .xfer_cb = xinput_xfer_cb,
    .close = xinput_close
};

usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return &xinput_driver;
}

// GAMEPAD HAPTIC on an Xbox pad: its motors, sent when they change (from USBGamepadTask, outside the callbacks)
void XInputRumble(int slot, uint8_t left, uint8_t right) {
    for(int k = 0; k < XINPUT_MAX; k++) {
        xinput_itf_t *e = &xi[k];
        if(!e->daddr || e->slot != slot || (e->sent_l == left && e->sent_r == right)) continue;
        bool ok;
        if(e->one) {
            const uint8_t rumble[13] = {0x09, 0x00, e->seq, 0x09, 0x00, 0x0F, 0x00, 0x00,
                                        (uint8_t)(left * 100 / 255), (uint8_t)(right * 100 / 255), 0xFF, 0x00, 0xFF};
            ok = xout(e, rumble, sizeof(rumble));
            if(ok) e->seq++;
        } else {
            const uint8_t rumble[8] = {0x00, 0x08, 0x00, left, right, 0x00, 0x00, 0x00};
            ok = xout(e, rumble, sizeof(rumble));
        }
        if(ok) {
            e->sent_l = left;
            e->sent_r = right;
        }
    }
}

// a device whose Xbox gamepad interface this driver has (tuh_mount_cb: is it one nothing else took?)
bool XInputClaimed(uint8_t daddr) {
    return daddr != 0 && find(daddr, 0) != NULL;
}
