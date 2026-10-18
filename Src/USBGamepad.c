/***********************************************************************************************************************
USB gamepads (the PicoMite's, 18 Oct: Peter). A gamepad takes a HID slot as on the PicoMite (the first free of 3 and 4,
then any), and its channel is the slot + 1: DEVICE(GAMEPAD n, funct) reads it, GAMEPAD INTERRUPT ENABLE n, sub [, mask]
calls sub when a button in the mask changes. Recognised by VID and PID: the Sony DualShock 4 and its clones (PS4), the
DualShock 3 (PS3), three XBox-style pads (XBOX), and the generic pads of the table below, or of GAMEPAD CONFIGURE (SNES).
The CMM2 adds the Nintendo Switch Pro Controller (SWITCH, 133), as which many third-party pads present themselves in
their Switch mode (an EasySMX 9013 Pro, 18 Oct): see process_switch_pro().
GAMEPAD MONITOR claims any other HID interface as a generic pad and prints its reports as they change (SILENT: prints
nothing, and DEVICE(GAMEPAD n, RAW) gives the report), so that a CONFIGURE can be worked out for it.

The button bitmap (DEVICE(GAMEPAD n, B)), as on the PicoMite and the Wii Classic:
  bit 0 R/R1, 1 start/options, 2 home/PS, 3 select/share, 4 L/L1, 5 down, 6 right, 7 up, 8 left, 9 R2, 10 x/triangle,
  11 a/circle, 12 y/square, 13 b/cross, 14 L2, 15 the PS4's touchpad.

Differences from the PicoMite's code, each a fault there (PicoMite issues #43-#45):
- the DS4's d-pad gives down and left for SW, and up and left for NW (it gave down, and down and left; #43);
- a CONFIGURE code of 128-135 (pressed when bit code-128 is clear) tests that bit (it shifted by the code itself, and 128
  was refused), and a code that is not one of 0-7, 64, 128-135, 192 is refused by CONFIGURE, not by an error() raised
  from inside the USB stack when a report arrives; an index past the end of the report is never pressed (#44);
- an unplugged pad's buttons and sticks read 0 (they kept their last values: a button held as it went stayed pressed; #45);
- a DualShock 3 is switched on as Linux does it (feature reports F2 and F5), or it reports no buttons or sticks.
GAMEPAD MONITOR stays on until a reset, as on the PicoMite (#46).  A DualShock 3 (one, 18 Oct) sends only its buttons'
pressures until its PS button is pressed; then everything.  L3 and R3 have no bit in the map (bit 15 is the PS4's
touchpad), on either board.  Nothing prints from inside the USB stack: the connect messages go through USBMessage(), the monitor's lines to a buffer
that USBGamepadService() prints once tuh_task() has returned.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "tusb.h"
#include "keyboard.h"
#include "USBGamepad.h"

extern void USBMessage(const char *m);

// one button of a generic pad: the report byte it is in (0xFF: none), and how it is read: 0-7 that bit set, 128-135 bit
// (code - 128) clear, 64 the byte below 64, 192 the byte above 192
typedef struct { uint8_t index, code; } gp_button_t;
typedef struct { uint16_t vid, pid; gp_button_t b[16]; } gp_decode_t;    // b[i]: the bitmap's bit i

#define NB {0xFF, 0}
static const gp_decode_t GenericPads[] = {          // the PicoMite's table, in the bitmap's order
    {0x0810, 0xE501, {{6, 1}, {6, 5}, NB, {6, 4}, {6, 0}, {4, 192}, {3, 192}, {4, 64}, {3, 64}, NB, {5, 4}, {5, 5}, {5, 7}, {5, 6}, NB, NB}},
    {0x0079, 0x0011, {{6, 1}, {6, 5}, NB, {6, 4}, {6, 0}, {4, 192}, {3, 192}, {4, 64}, {3, 64}, NB, {5, 4}, {5, 5}, {5, 7}, {5, 6}, NB, NB}},
    {0x081F, 0xE401, {{6, 1}, {6, 5}, NB, {6, 4}, {6, 0}, {1, 192}, {0, 192}, {1, 64}, {0, 64}, NB, {5, 4}, {5, 5}, {5, 7}, {5, 6}, NB, NB}},
    {0x1C59, 0x0026, {{6, 1}, {6, 3}, NB, {6, 2}, {6, 0}, {1, 192}, {0, 192}, {1, 64}, {0, 64}, NB, {5, 7}, {5, 6}, {5, 4}, {5, 5}, NB, NB}},
    {0x06A3, 0x0107, {{3, 7}, {3, 5}, NB, {3, 4}, {3, 6}, {1, 192}, {0, 192}, {1, 64}, {0, 64}, {3, 7}, {3, 1}, {3, 3}, {3, 0}, {3, 2}, NB, NB}},
    {0x11FF, 0x3331, {{6, 3}, {6, 1}, {6, 6}, {6, 0}, {6, 2}, {1, 192}, {0, 192}, {1, 64}, {0, 64}, {6, 5}, {5, 4}, {5, 6}, {5, 5}, {5, 7}, {6, 4}, {6, 7}}},
    {0x0583, 0x2060, {{2, 5}, {2, 7}, NB, {2, 6}, {2, 4}, {1, 192}, {0, 192}, {1, 64}, {0, 64}, {3, 5}, {2, 2}, {2, 0}, {2, 3}, {2, 1}, {3, 4}, NB}},
    {0, 0, {NB}}
};
static gp_decode_t MyPad;                           // GAMEPAD CONFIGURE's
static bool monitor = false, nooutput = false;      // GAMEPAD MONITOR [SILENT]

typedef struct {
    int lx, ly, rx, ry, l, r;                       // the sticks, and the analogue L2 and R2
    uint16_t b;                                     // the buttons' bitmap
    uint16_t mask;                                  // the buttons whose change calls GAMEPAD INTERRUPT ENABLE's sub
    int16_t gyro[3], accs[3];                       // the PS4's
    uint8_t type;                                   // GAMEPAD_PS4 ... ; 0 when there is none
    uint8_t hat;                                    // the PS4's d-pad, 0-7 from north clockwise; 0xFF centred or none
    uint8_t motorleft, motorright, red, green, blue;    // GAMEPAD HAPTIC and COLOUR, for a PS4
    uint8_t init_step, packet;                      // a Switch Pro Controller's set-up (USBGamepadTask), its subcommands' count
    uint32_t init_time;
    char *interrupt;
    volatile int found;
    gp_decode_t decode;                             // a generic pad's
} gamepad_t;
static volatile gamepad_t Pad[HID_SLOTS + 1];       // by channel, 1-4

static char monbuf[1024];                           // GAMEPAD MONITOR's lines, printed by USBGamepadService()
static void MonAdd(const char *s) {
    if(strlen(monbuf) + strlen(s) < sizeof(monbuf)) strcat(monbuf, s);
}
static void MonHex(unsigned v, int digits) {
    char s[8];
    for(int i = digits - 1; i >= 0; i--) { s[i] = "0123456789ABCDEF"[v & 15]; v >>= 4; }
    s[digits] = 0;
    MonAdd(s);
}

static bool is_sony_ds4(uint16_t vid, uint16_t pid) {
    return (vid == 0x054c && (pid == 0x09cc || pid == 0x05c4))    // Sony DualShock4
        || (vid == 0x0f0d && pid == 0x005e)                         // Hori FC4
        || (vid == 0x0f0d && pid == 0x00ee)                         // Hori PS4 Mini (PS4-099U)
        || (vid == 0x1f4f && pid == 0x1002);                        // ASW GG xrd controller
}
static bool is_sony_ds3(uint16_t vid, uint16_t pid) {
    return vid == 0x054c && pid == 0x0268;                          // Sony DualShock3
}
static bool is_xbox(uint16_t vid, uint16_t pid) {
    return (vid == 0x11c0 && pid == 0x5500)                         // EasySMX Wireless, Android mode
        || (vid == 0x11c1 && pid == 0x9101)                         // EasySMX Wireless, PC mode, D-input
        || (vid == 0x2F24 && pid == 0x0048);
}
static bool is_switch_pro(uint16_t vid, uint16_t pid) {
    return vid == 0x057E && pid == 0x2009;                          // Nintendo Switch Pro Controller, and pads in its mode
}
static const gp_decode_t *generic_decode(uint16_t vid, uint16_t pid) {
    if(MyPad.vid == vid && MyPad.pid == pid && (vid || pid)) return &MyPad;
    for(int i = 0; GenericPads[i].pid; i++)
        if(GenericPads[i].vid == vid && GenericPads[i].pid == pid) return &GenericPads[i];
    return NULL;
}

// the new bitmap: a changed button in the mask calls GAMEPAD INTERRUPT ENABLE's sub
static void SetButtons(volatile gamepad_t *g, uint16_t b) {
    if((b ^ g->b) & g->mask) {
        g->found = 1;
        if(g->interrupt != NULL) IntSignal();
    }
    g->b = b;
}

// A DualShock 3 sends its buttons and sticks only once it has been switched on: Linux's hid-sony reads its feature
// reports F2 (17 bytes) and then F5 (8 bytes) at connection (sixaxis_set_operational_usb). Before that its reports carry
// the buttons' pressures only (seen 18 Oct: bytes 14-25 moved, the button and stick bytes stayed at 0 and 128). Linux's
// third step, a one-byte output report, is left out: some clones (SHANWAN) then rumble without stopping.
static uint8_t ds3_feature[20];
void tuh_hid_get_report_complete_cb(uint8_t dev_addr, uint8_t idx, uint8_t report_id, uint8_t report_type, uint16_t len) {
    (void)report_type; (void)len;
    if(report_id != 0xF2) return;
    for(int i = 0; i < HID_SLOTS; i++)
        if(HID[i].active && HID[i].Device_address == dev_addr && HID[i].Device_instance == idx && HID[i].Device_type == GAMEPAD_PS3)
            tuh_hid_get_report(dev_addr, idx, 0xF5, HID_REPORT_TYPE_FEATURE, ds3_feature, 8);
}

bool USBGamepadMount(uint8_t dev_addr, uint8_t instance, int slot) {
    uint16_t vid, pid;
    char m[96];
    int ch = slot + 1;
    const gp_decode_t *d = NULL;
    uint8_t type;
    tuh_vid_pid_get(dev_addr, &vid, &pid);
    if(is_sony_ds4(vid, pid)) type = GAMEPAD_PS4, strcpy(m, "PS4 Controller");
    else if(is_sony_ds3(vid, pid)) type = GAMEPAD_PS3, strcpy(m, "PS3 Controller");
    else if(is_xbox(vid, pid)) type = GAMEPAD_XBOX, strcpy(m, "XBox Controller");
    else if(is_switch_pro(vid, pid)) type = GAMEPAD_SWITCH, strcpy(m, "Switch Pro Controller");
    else if((d = generic_decode(vid, pid)) != NULL || monitor) type = GAMEPAD_SNES, strcpy(m, "Generic Gamepad");
    else return false;                                              // not a gamepad: its slot stays free
    volatile gamepad_t *g = &Pad[ch];
    g->lx = g->ly = g->rx = g->ry = g->l = g->r = 0;
    g->b = 0; g->found = 0; g->hat = 0xFF;
    memset((void *)g->gyro, 0, sizeof(g->gyro));
    memset((void *)g->accs, 0, sizeof(g->accs));
    g->motorleft = g->motorright = 0;
    g->init_step = type == GAMEPAD_SWITCH ? 1 : 0;                  // its USB set-up (USBGamepadTask)
    g->init_time = HAL_GetTick();
    g->packet = 0;
    if(d) memcpy((void *)&g->decode, d, sizeof(gp_decode_t));
    else memset((void *)&g->decode, 0, sizeof(gp_decode_t));
    g->type = type;
    HID[slot].vid = vid;
    HID[slot].pid = pid;
    HID[slot].Device_address = dev_addr;
    HID[slot].Device_instance = instance;
    HID[slot].Device_type = type;
    HID[slot].report_rate = 20;                                     // ms between reports
    HID[slot].report_timer = -(10 + (slot + 2) * 500);
    if(type == GAMEPAD_SWITCH) {                                    // read from the start: it answers each set-up command
        HID[slot].report_rate = 8;                                  // with a report, and drops off the bus about 2 s after
        HID[slot].report_timer = 8;                                 // connecting if they are not read; then the full reports
    }                                                               // come about every 15 ms
    HID[slot].report_requested = false;
    HID[slot].active = true;
    if(type == GAMEPAD_PS3) tuh_hid_get_report(dev_addr, instance, 0xF2, HID_REPORT_TYPE_FEATURE, ds3_feature, 17);
    strcat(m, " Connected on channel ");
    IntToStr(m + strlen(m), ch, 10);
    if(type != GAMEPAD_PS4 && type != GAMEPAD_PS3) {                // (CONFIGURE needs them)
        strcat(m, " (vid &H");
        IntToStr(m + strlen(m), vid, 16);
        strcat(m, ", pid &H");
        IntToStr(m + strlen(m), pid, 16);
        strcat(m, ")");
    }
    strcat(m, "\r\n");
    if(monitor) MonAdd(m);                                          // printed while a program runs too
    else {
        strcat(m, "> ");
        USBMessage(m);
    }
    return true;
}

void USBGamepadDetached(int slot) {
    volatile gamepad_t *g = &Pad[slot + 1];
    char m[48];
    strcpy(m, g->type == GAMEPAD_PS4 ? "PS4 Controller" : g->type == GAMEPAD_PS3 ? "PS3 Controller" :
              g->type == GAMEPAD_XBOX ? "XBox Controller" : g->type == GAMEPAD_SWITCH ? "Switch Pro Controller" : "Generic Gamepad");
    strcat(m, " Disconnected\r\n> ");
    USBMessage(m);
    g->lx = g->ly = g->rx = g->ry = g->l = g->r = 0;               // nothing held (the interrupt and its mask stay)
    g->b = 0; g->type = 0; g->hat = 0xFF;
    memset((void *)g->gyro, 0, sizeof(g->gyro));
    memset((void *)g->accs, 0, sizeof(g->accs));
}

static bool pressed(uint8_t const *report, uint16_t len, gp_button_t k) {
    if(k.index == 0xFF || k.index >= len) return false;
    uint8_t v = report[k.index];
    if(k.code == 192) return v > 192;
    if(k.code == 64) return v < 64;
    if(k.code < 8) return v & (1 << k.code);
    if(k.code >= 128 && k.code < 136) return !(v & (1 << (k.code - 128)));
    return false;
}

static void process_generic(uint8_t const *report, uint16_t len, int ch) {
    volatile gamepad_t *g = &Pad[ch];
    if(len > 64) len = 64;
    if(monitor) {
        static uint8_t last[64];
        if(!nooutput && memcmp(report, last, len)) {
            MonHex(HID[ch - 1].vid, 4); MonAdd(","); MonHex(HID[ch - 1].pid, 4); MonAdd("\r\n");
            for(int i = 0; i < len; i++) { if(i) MonAdd(","); MonHex(last[i], 2); }
            MonAdd("\r\n");
            for(int i = 0; i < len; i++) { if(i) MonAdd(","); MonHex(report[i], 2); }
            MonAdd("\r\n");
        }
        memcpy(last, report, len);
        return;
    }
    if(!g->decode.vid && !g->decode.pid) return;
    uint16_t b = 0;
    for(int i = 0; i < 16; i++) if(pressed(report, len, g->decode.b[i])) b |= 1 << i;
    SetButtons(g, b);
}

static void process_xbox(uint8_t const *report, uint16_t len, int ch) {
    volatile gamepad_t *g = &Pad[ch];
    uint16_t b = 0;
    if(len != 9) return;
    if(report[0] & 0x10) b |= 0x0400;                               // y/triangle
    if(report[0] & 0x02) b |= 0x0800;                               // b/circle
    if(report[0] & 0x08) b |= 0x1000;                               // x/square
    if(report[0] & 0x01) b |= 0x2000;                               // a/cross
    if(report[1] & 0x08) b |= 0x0002;                               // start
    if(report[1] & 0x04) b |= 0x0008;                               // home
    if(report[1] & 0x10) b |= 0x0004;                               // select
    if(report[0] & 0x80) b |= 0x0001;                               // R/R1
    if(report[0] & 0x40) b |= 0x0010;                               // L/L1
    if(report[2] == 0x4) b |= 0x20;                                 // down
    if(report[2] == 0x2) b |= 0x40;                                 // right
    if(report[2] == 0x0) b |= 0x80;                                 // up
    if(report[2] == 0x6) b |= 0x100;                                // left
    g->lx = report[3];
    g->ly = report[4];
    g->rx = report[5];
    g->ry = report[6];
    g->l = report[8];
    g->r = report[7];
    SetButtons(g, b);
}

static void process_sony_ds3(uint8_t const *report, uint16_t len, int ch) {
    volatile gamepad_t *g = &Pad[ch];
    uint16_t b = 0;
    if(len < 20) return;
    if(report[3] & 0x08) b |= 1;
    if(report[2] & 0x08) b |= 1 << 1;
    if(report[4] & 0x01) b |= 1 << 2;
    if(report[2] & 0x01) b |= 1 << 3;
    if(report[3] & 0x04) b |= 1 << 4;
    if(report[2] & 0x40) b |= 1 << 5;
    if(report[2] & 0x20) b |= 1 << 6;
    if(report[2] & 0x10) b |= 1 << 7;
    if(report[2] & 0x80) b |= 1 << 8;
    if(report[3] & 0x02) b |= 1 << 9;
    if(report[3] & 0x10) b |= 1 << 10;
    if(report[3] & 0x20) b |= 1 << 11;
    if(report[3] & 0x80) b |= 1 << 12;
    if(report[3] & 0x40) b |= 1 << 13;
    if(report[3] & 0x01) b |= 1 << 14;
    g->lx = report[6];
    g->ly = report[7];
    g->rx = report[8];
    g->ry = report[9];
    g->l = report[18];
    g->r = report[19];
    SetButtons(g, b);
}

typedef struct TU_ATTR_PACKED {                     // the DS4's input report 1, after its ID
    uint8_t x, y, z, rz;                            // the sticks
    struct { uint8_t dpad : 4; uint8_t square : 1; uint8_t cross : 1; uint8_t circle : 1; uint8_t triangle : 1; };
    struct { uint8_t l1 : 1; uint8_t r1 : 1; uint8_t l2 : 1; uint8_t r2 : 1; uint8_t share : 1; uint8_t option : 1; uint8_t l3 : 1; uint8_t r3 : 1; };
    struct { uint8_t ps : 1; uint8_t tpad : 1; uint8_t counter : 6; };
    uint8_t l2_trigger, r2_trigger;
    uint16_t timestamp;
    uint8_t battery;
    int16_t gyro[3], accel[3];
} sony_ds4_report_t;

typedef struct TU_ATTR_PACKED {                     // the DS4's output report 5
    uint8_t set_rumble : 1; uint8_t set_led : 1; uint8_t set_led_blink : 1; uint8_t set_ext_write : 1;
    uint8_t set_left_volume : 1; uint8_t set_right_volume : 1; uint8_t set_mic_volume : 1; uint8_t set_speaker_volume : 1;
    uint8_t set_flags2;
    uint8_t reserved;
    uint8_t motor_right, motor_left;
    uint8_t lightbar_red, lightbar_green, lightbar_blue, lightbar_blink_on, lightbar_blink_off;
    uint8_t ext_data[8];
    uint8_t volume_left, volume_right, volume_mic, volume_speaker;
    uint8_t other[9];
} sony_ds4_output_report_t;

static void process_sony_ds4(uint8_t const *report, uint16_t len, int ch) {
    volatile gamepad_t *g = &Pad[ch];
    // the d-pad, 0-7 from north clockwise, as up/right/down/left: N, NE, E, SE, S, SW, W, NW
    static const uint16_t dpad[8] = {1 << 7, (1 << 7) | (1 << 6), 1 << 6, (1 << 5) | (1 << 6), 1 << 5, (1 << 5) | (1 << 8), 1 << 8, (1 << 7) | (1 << 8)};
    sony_ds4_report_t r;
    if(len < 1 + sizeof(r) || report[0] != 1) return;               // the buttons are all in report 1
    memcpy(&r, report + 1, sizeof(r));
    uint16_t b = 0;
    if(r.r1) b |= 1;
    if(r.option) b |= 1 << 1;
    if(r.ps) b |= 1 << 2;
    if(r.share) b |= 1 << 3;
    if(r.l1) b |= 1 << 4;
    if(r.dpad < 8) b |= dpad[r.dpad];
    if(r.r2) b |= 1 << 9;
    if(r.triangle) b |= 1 << 10;
    if(r.circle) b |= 1 << 11;
    if(r.square) b |= 1 << 12;
    if(r.cross) b |= 1 << 13;
    if(r.l2) b |= 1 << 14;
    if(r.tpad) b |= 1 << 15;
    g->hat = r.dpad < 8 ? r.dpad : 0xFF;
    g->lx = r.x;
    g->ly = r.y;
    g->rx = r.z;
    g->ry = r.rz;
    g->l = r.l2_trigger;
    g->r = r.r2_trigger;
    memcpy((void *)g->gyro, r.gyro, sizeof(r.gyro));
    memcpy((void *)g->accs, r.accel, sizeof(r.accel));
    SetButtons(g, b);
}

// A Switch Pro Controller, as Linux's hid-nintendo drives one on USB. At connection it is sent output report 0x80 with
// the commands 02 (handshake), 03 (3 Mbit), 02 and 04 (no timeout: without it the pad drops off the bus and comes back
// every few seconds, still waiting for a host on its Bluetooth link), then subcommand 03 with 30 (the full input
// report, 0x30, about every 15 ms) and subcommand 30 with 01 (player 1's light). One step per 100 ms (USBGamepadTask);
// the replies (reports 0x81 and 0x21) are not waited for. Report 0x30: the buttons in bytes 3-5, the sticks 12 bits
// each in bytes 6-8 and 9-11. Mapped by position, as the PicoMite maps every pad: X the top face button (bit 10), A
// the right (11), Y the left (12), B the bottom (13); + START, - SELECT, HOME, Capture as the touchpad (15); ZL and ZR
// are L2 and R2, read 255 or 0 as the analogue L and R. The sticks 0-255, up and left 0, as the others.
static void process_switch_pro(uint8_t const *report, uint16_t len, int ch) {
    volatile gamepad_t *g = &Pad[ch];
    if(len < 12 || report[0] != 0x30) return;
    uint8_t r = report[3], s = report[4], l = report[5];
    uint16_t b = 0;
    if(r & 0x40) b |= 1;                                            // R
    if(s & 0x02) b |= 1 << 1;                                       // +
    if(s & 0x10) b |= 1 << 2;                                       // HOME
    if(s & 0x01) b |= 1 << 3;                                       // -
    if(l & 0x40) b |= 1 << 4;                                       // L
    if(l & 0x01) b |= 1 << 5;                                       // down
    if(l & 0x04) b |= 1 << 6;                                       // right
    if(l & 0x02) b |= 1 << 7;                                       // up
    if(l & 0x08) b |= 1 << 8;                                       // left
    if(r & 0x80) b |= 1 << 9;                                       // ZR
    if(r & 0x02) b |= 1 << 10;                                      // X
    if(r & 0x08) b |= 1 << 11;                                      // A
    if(r & 0x01) b |= 1 << 12;                                      // Y
    if(r & 0x04) b |= 1 << 13;                                      // B
    if(l & 0x80) b |= 1 << 14;                                      // ZL
    if(s & 0x20) b |= 1 << 15;                                      // Capture
    g->lx = (report[6] | ((report[7] & 0x0F) << 8)) >> 4;
    g->ly = 255 - ((((report[7] >> 4) | (report[8] << 4)) >> 4) & 0xFF);
    g->rx = (report[9] | ((report[10] & 0x0F) << 8)) >> 4;
    g->ry = 255 - ((((report[10] >> 4) | (report[11] << 4)) >> 4) & 0xFF);
    g->l = (l & 0x80) ? 255 : 0;
    g->r = (r & 0x80) ? 255 : 0;
    static const uint8_t hat[16] = {0xFF, 0, 2, 1, 4, 0xFF, 3, 0xFF, 6, 7, 0xFF, 0xFF, 5, 0xFF, 0xFF, 0xFF}; // by up|right<<1|down<<2|left<<3
    g->hat = hat[((l >> 1) & 1) | (((l >> 2) & 1) << 1) | ((l & 1) << 2) | (((l >> 3) & 1) << 3)];
    SetButtons(g, b);
}

static void switch_pro_step(int slot) {
    volatile gamepad_t *g = &Pad[slot + 1];
    uint8_t a = HID[slot].Device_address, i = HID[slot].Device_instance;
    static const uint8_t usbcmd[4] = {0x02, 0x03, 0x02, 0x04};    // handshake, 3 Mbit, handshake, no timeout
    if(g->init_step >= 1 && g->init_step <= 4) {
        if(!tuh_hid_send_report(a, i, 0x80, &usbcmd[g->init_step - 1], 1)) return;     // (busy: next time)
    } else if(g->init_step == 5 || g->init_step == 6) {
        // output report 1: a packet number, a neutral rumble, the subcommand and its argument
        uint8_t sub[11] = {0, 0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40, 0, 0};
        sub[0] = g->packet++ & 0x0F;
        sub[9] = g->init_step == 5 ? 0x03 : 0x30;                   // the input report mode, the player lights
        sub[10] = g->init_step == 5 ? 0x30 : 0x01;                  // the full report, player 1
        if(!tuh_hid_send_report(a, i, 0x01, sub, sizeof(sub))) return;
    }
    g->init_step = g->init_step >= 6 ? 0 : g->init_step + 1;
    g->init_time = HAL_GetTick();
}

void USBGamepadReport(uint8_t const *report, uint16_t len, int slot) {
    switch(Pad[slot + 1].type) {
        case GAMEPAD_PS4:  process_sony_ds4(report, len, slot + 1); break;
        case GAMEPAD_PS3:  process_sony_ds3(report, len, slot + 1); break;
        case GAMEPAD_XBOX: process_xbox(report, len, slot + 1); break;
        case GAMEPAD_SNES: process_generic(report, len, slot + 1); break;
        case GAMEPAD_SWITCH: process_switch_pro(report, len, slot + 1); break;
    }
}

// every 50 ms, a PS4's light bar and motors (GAMEPAD COLOUR and HAPTIC), as on the PicoMite
void USBGamepadTask(void) {
    static uint32_t last;
    for(int i = 0; i < HID_SLOTS; i++)                              // a Switch Pro Controller's set-up, a step per 100 ms
        if(HID[i].active && HID[i].Device_type == GAMEPAD_SWITCH && Pad[i + 1].init_step &&
           HAL_GetTick() - Pad[i + 1].init_time >= 100 && tuh_hid_send_ready(HID[i].Device_address, HID[i].Device_instance))
            switch_pro_step(i);
    if(HAL_GetTick() - last < 50) return;
    last = HAL_GetTick();
    for(int i = 0; i < HID_SLOTS; i++) {
        volatile gamepad_t *g = &Pad[i + 1];
        if(!HID[i].active || HID[i].Device_type != GAMEPAD_PS4) continue;
        sony_ds4_output_report_t out = {0};
        out.set_rumble = 1;
        out.motor_left = g->motorleft;
        out.motor_right = g->motorright;
        out.set_led = 1;
        out.lightbar_red = g->red;
        out.lightbar_green = g->green;
        out.lightbar_blue = g->blue;
        tuh_hid_send_report(HID[i].Device_address, HID[i].Device_instance, 5, &out, sizeof(out));
    }
}

void USBGamepadService(void) {
    if(monbuf[0]) {
        char m[sizeof(monbuf)];
        strcpy(m, monbuf);
        monbuf[0] = 0;
        MMPrintString(m);
    }
}

// DEVICE(GAMEPAD n, LX|LY|RX|RY|L|R|B|H|GX|GY|GZ|AX|AY|AZ|T|RAW), as on the PicoMite (another letter: 0)
void USBGamepadDevice(char *tp) {
    getargs(&tp, 3, ",");
    if(argc != 3) error("Syntax");
    int n = getint(argv[0], 1, HID_SLOTS);
    volatile gamepad_t *g = &Pad[n];
    char *k = argv[2];
    targ = T_INT;
    if(checkstring(k, "LX")) iret = g->lx;
    else if(checkstring(k, "LY")) iret = g->ly;
    else if(checkstring(k, "RX")) iret = g->rx;
    else if(checkstring(k, "RY")) iret = g->ry;
    else if(checkstring(k, "L")) iret = g->l;
    else if(checkstring(k, "R")) iret = g->r;
    else if(checkstring(k, "B")) iret = g->b;
    else if(checkstring(k, "H")) iret = g->type ? g->hat : 0xFF;   // (0xFF: centred, or no pad)
    else if(checkstring(k, "GX")) iret = g->gyro[0];
    else if(checkstring(k, "GY")) iret = g->gyro[1];
    else if(checkstring(k, "GZ")) iret = g->gyro[2];
    else if(checkstring(k, "AX")) iret = g->accs[0];
    else if(checkstring(k, "AY")) iret = g->accs[1];
    else if(checkstring(k, "AZ")) iret = g->accs[2];
    else if(checkstring(k, "T")) iret = g->type;
    else if(checkstring(k, "RAW")) {                                // the last report: its length, then its bytes
        sret = GetTempStrMemory();
        memcpy(sret, (void *)HID[n - 1].report, HID[n - 1].report[0] + 1);
        targ = T_STR;
    }
    else iret = 0;
}

char *GamepadInterrupt(void) {
    for(int n = 1; n <= HID_SLOTS; n++)
        if(Pad[n].interrupt != NULL && Pad[n].found) {
            Pad[n].found = 0;
            return Pad[n].interrupt;
        }
    return NULL;
}

void GamepadInterruptsOff(void) {
    for(int n = 1; n <= HID_SLOTS; n++) {
        Pad[n].interrupt = NULL;
        Pad[n].found = 0;
        Pad[n].mask = 0;
    }
}

// GAMEPAD INTERRUPT ENABLE n, sub [, mask] | INTERRUPT DISABLE n | MONITOR [SILENT] | CONFIGURE vid, pid, 16 x (index,
// code) | HAPTIC n, left, right | COLOUR n, colour (the last two a PS4's)
void cmd_gamepad(void) {
    char *tp;
    int n;
    if((tp = checkstring(cmdline, "INTERRUPT ENABLE"))) {
        getargs(&tp, 5, ",");
        if(!(argc == 3 || argc == 5)) error("Syntax");
        n = getint(argv[0], 1, HID_SLOTS);
        Pad[n].interrupt = GetIntAddress(argv[2]);
        Pad[n].found = 0;
        Pad[n].mask = argc == 5 ? getint(argv[4], 0, 0xFFFF) : 0xFFFF;
        IntSignal();
    } else if((tp = checkstring(cmdline, "INTERRUPT DISABLE"))) {
        getargs(&tp, 1, ",");
        if(argc != 1) error("Syntax");
        n = getint(argv[0], 1, HID_SLOTS);
        Pad[n].interrupt = NULL;
        Pad[n].found = 0;
    } else if((tp = checkstring(cmdline, "MONITOR SILENT"))) {
        monitor = nooutput = true;
    } else if((tp = checkstring(cmdline, "MONITOR"))) {
        monitor = true;
    } else if((tp = checkstring(cmdline, "CONFIGURE"))) {
        getargs(&tp, 67, ",");
        if(argc != 67) error("Syntax");
        gp_decode_t d;
        d.vid = getint(argv[0], 0, 0xFFFF);
        d.pid = getint(argv[2], 0, 0xFFFF);
        for(int i = 0; i < 16; i++) {
            d.b[i].index = getint(argv[4 + i * 4], 0, 255);
            d.b[i].code = getint(argv[6 + i * 4], 0, 255);
            int c = d.b[i].code;
            if(d.b[i].index != 0xFF && !(c < 8 || c == 64 || c == 192 || (c >= 128 && c < 136)))
                error("Invalid code % for button %", c, i);
        }
        memcpy(&MyPad, &d, sizeof(d));
    } else if((tp = checkstring(cmdline, "HAPTIC"))) {
        getargs(&tp, 5, ",");
        if(argc != 5) error("Syntax");
        n = getint(argv[0], 1, HID_SLOTS);
        if(Pad[n].type != GAMEPAD_PS4) error("PS4 only");
        Pad[n].motorleft = getint(argv[2], 0, 255);
        Pad[n].motorright = getint(argv[4], 0, 255);
    } else if((tp = checkstring(cmdline, "COLOUR")) || (tp = checkstring(cmdline, "COLOR"))) {
        getargs(&tp, 3, ",");
        if(argc != 3) error("Syntax");
        n = getint(argv[0], 1, HID_SLOTS);
        if(Pad[n].type != GAMEPAD_PS4) error("PS4 only");
        int colour = getint(argv[2], 0, 0xFFFFFF);
        Pad[n].red = colour >> 16;
        Pad[n].green = (colour >> 8) & 0xFF;
        Pad[n].blue = colour & 0xFF;
    } else error("Syntax");
}
