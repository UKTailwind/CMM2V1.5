/***********************************************************************************************************************
A USB mouse (the PicoMite's, 17 Oct: Peter). Channel 2, as on the PicoMite; one mouse at a time (Peter): while a PS/2 or
I2C mouse is open a USB mouse is ignored, and while a USB mouse is attached no other can be opened.

Its state is mousestruct[2], in the PicoMite's nunstruct meanings, which the CMM2's PS/2 and I2C mice now use too: ax, ay
the position (clamped to the screen), az the wheel's count, L R C the left, right and middle buttons, Z a double click
(read once), x0 the buttons' bitmap. The mouse is put in the report protocol (18 Oct, Peter: the wheel is only in its
reports) and its reports read by its descriptor's layout (analyze_mouse_descriptor), X and Y at its own width; a boot
report holds them in a byte each, which a fast mouse that sends the low byte of its count wraps (+130 arrives as -126).
It is asked for a report every 10 ms (USBKeyboard.c), as a PS/2 mouse streams. The movement is halved as on the
PicoMite and divided by OPTION MOUSE SENSITIVITY (unset or 0: 1), the fraction of a pixel a report leaves kept for the
next.

DEVICE(MOUSE n, X|Y|L|R|M|W|B|D|T) reads the mouse in use whatever it is (0 when there is none), so a PicoMite program
reads any of them on channel 2; MOUSE INTERRUPT ENABLE n, sub (any button change or double click), MOUSE INTERRUPT
DISABLE n and MOUSE SET n, x, y [, w] are the PicoMite's commands. The CMM2's MOUSE(k [, n]) and CONTROLLER MOUSE are
kept (CONTROLLER MOUSE OPEN 2 sets the USB mouse's interrupts; its left, right and left-up interrupts work for it too).
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "tusb.h"
#include "keyboard.h"
#include "USBMouse.h"
#include "USBGamepad.h"

extern volatile struct s_nunstruct mousestruct[4];
extern volatile int mouse0, mouse1, mouse2, mouse3;
extern volatile int mouse2foundz, mouse2foundc, mouse2leftup;
extern char *mouse2Interruptz, *mouse2Interruptc, *mouse2Interruptu;
extern volatile int TOUCH_DOWN, mouseupdated;
extern void USBMessage(const char *m);

volatile int UsbMouse = 0;
char *MouseIntAny = NULL;                                           // MOUSE INTERRUPT ENABLE 2, sub (the PicoMite's)
volatile int MouseIntAnyFound = 0;

// the mouse in use: 0 PS/2, 1-3 I2C, 2 also a USB mouse; -1 when there is none
int MouseOpenChannel(void) {
    if(UsbMouse) return USBMOUSE_CHANNEL;
    if(mouse0) return 0;
    if(mouse1) return 1;
    if(mouse2) return 2;
    if(mouse3) return 3;
    return -1;
}

/* ---- the PicoMite's report descriptor parser (USBKeyboard.c there) ---------------------------------------------------
   Walks the HID report descriptor once at enumeration and records the bit offset, width and signedness of each field
   (buttons, X, Y, wheel, AC pan) and the report ID that carries them; USBMouseReport then decodes by bit extraction.
   Understands usage queues, Usage Min/Max ranges, constant padding, 4-byte extended usages and per-report-ID bit spaces. */
#define MOUSE_MAX_USAGES 8
#define MOUSE_MAX_REPORT_IDS 8
typedef struct {
    uint8_t id;
    uint16_t bits;
} mouse_rbits_t;

static uint16_t mouse_rbits_get(const mouse_rbits_t *tab, uint8_t n, uint8_t id) {
    for(uint8_t k = 0; k < n; k++) if(tab[k].id == id) return tab[k].bits;
    return 0;
}

static void mouse_rbits_set(mouse_rbits_t *tab, uint8_t *n, uint8_t id, uint16_t bits) {
    for(uint8_t k = 0; k < *n; k++)
        if(tab[k].id == id) { tab[k].bits = bits; return; }
    if(*n < MOUSE_MAX_REPORT_IDS) { tab[*n].id = id; tab[*n].bits = bits; (*n)++; }
}

static void mouse_capture_field(mouse_field_t *f, uint16_t off, uint8_t bits, bool is_signed, uint8_t report_id) {
    if(!f->present) {
        f->present = true;
        f->bit_offset = off;
        f->bit_size = bits;
        f->is_signed = is_signed;
        f->report_id = report_id;
    }
}

// a field from a report's payload (bit n in byte n/8, bit n%8), sign-extended if signed
static int mouse_field_value(const uint8_t *p, const mouse_field_t *f) {
    uint32_t v = 0;
    if(!f->present) return 0;
    for(uint8_t i = 0; i < f->bit_size; i++) {
        uint16_t b = f->bit_offset + i;
        if(p[b >> 3] & (1u << (b & 7))) v |= 1u << i;
    }
    if(f->is_signed && f->bit_size < 32 && (v & (1u << (f->bit_size - 1)))) v |= ~((1u << f->bit_size) - 1);
    return (int)(int32_t)v;
}

mouse_report_type_t analyze_mouse_descriptor(const uint8_t *desc_report, uint16_t desc_len, mouse_info_t *info) {
    if(!info) return MOUSE_TYPE_UNKNOWN;
    memset(info, 0, sizeof(mouse_info_t));
    if(!desc_report || desc_len == 0) return MOUSE_TYPE_UNKNOWN;
    uint16_t usage_page = 0;                                        // the global items
    int32_t logical_min = 0;
    uint8_t report_size = 0, report_count = 0, cur_id = 0;
    bool saw_report_id = false;
    uint32_t usages[MOUSE_MAX_USAGES];                              // the local items: page << 16 | usage
    uint8_t n_usages = 0;
    uint32_t usage_min = 0;
    bool have_usage_range = false;
    uint16_t bit_pos = 0;                                           // within the current report ID's report
    mouse_rbits_t rbits[MOUSE_MAX_REPORT_IDS];
    uint8_t n_rbits = 0;
    for(uint16_t i = 0; i < desc_len;) {
        uint8_t prefix = desc_report[i++];
        if(prefix == 0xFE) {                                        // a long item: its size, then tag and data
            if(i < desc_len) i += 2 + desc_report[i];
            continue;
        }
        uint8_t bSize = prefix & 0x03;
        if(bSize == 3) bSize = 4;
        uint8_t bType = (prefix >> 2) & 0x03, bTag = (prefix >> 4) & 0x0F;
        uint32_t data = 0;
        for(uint8_t j = 0; j < bSize && i + j < desc_len; j++) data |= (uint32_t)desc_report[i + j] << (j * 8);
        int32_t sdata = (int32_t)data;
        if(bSize == 1) sdata = (int8_t)data;
        else if(bSize == 2) sdata = (int16_t)data;
        i += bSize;
        if(bType == 1) {                                            // Global
            switch(bTag) {
                case 0: usage_page = data; break;
                case 1: logical_min = sdata; break;
                case 7: report_size = data; break;
                case 8:                                             // Report ID: that report's own bit space
                    mouse_rbits_set(rbits, &n_rbits, cur_id, bit_pos);
                    saw_report_id = true;
                    cur_id = data;
                    bit_pos = mouse_rbits_get(rbits, n_rbits, cur_id);
                    break;
                case 9: report_count = data; break;
                default: break;
            }
        } else if(bType == 2) {                                     // Local
            if(bTag == 0) {                                         // Usage: a 4-byte one carries its page
                if(n_usages < MOUSE_MAX_USAGES) usages[n_usages++] = (bSize == 4) ? data : (((uint32_t)usage_page << 16) | data);
            } else if(bTag == 1) {
                usage_min = data;
                have_usage_range = true;
            }
        } else if(bType == 0) {                                     // Main
            if(bTag == 8) {                                         // Input
                bool constant = data & 0x01;
                if(!constant && report_size && report_count) {
                    for(uint16_t f = 0; f < report_count; f++) {
                        uint32_t u = 0;
                        if(n_usages) u = usages[f < n_usages ? f : n_usages - 1];
                        else if(have_usage_range) u = ((uint32_t)usage_page << 16) | (usage_min + f);
                        uint16_t page = u >> 16, usage = u & 0xFFFF, off = bit_pos + f * report_size;
                        bool sgn = logical_min < 0;
                        if(page == 0x01) {                          // Generic Desktop
                            if(usage == 0x30) mouse_capture_field(&info->f_x, off, report_size, sgn, cur_id);
                            else if(usage == 0x31) mouse_capture_field(&info->f_y, off, report_size, sgn, cur_id);
                            else if(usage == 0x38) mouse_capture_field(&info->f_wheel, off, report_size, sgn, cur_id);
                        } else if(page == 0x09) {                   // Buttons: the run of 1-bit fields as one bitmap
                            if(!info->f_buttons.present && report_size == 1) {
                                uint16_t nb = report_count - f;
                                mouse_capture_field(&info->f_buttons, off, nb > 8 ? 8 : (uint8_t)nb, false, cur_id);
                            }
                        } else if(page == 0x0C && usage == 0x0238) // Consumer AC Pan
                            mouse_capture_field(&info->f_pan, off, report_size, sgn, cur_id);
                    }
                }
                bit_pos += (uint16_t)report_size * report_count;
            }
            n_usages = 0;                                           // the local items apply to one Main item
            usage_min = 0;
            have_usage_range = false;
        }
    }
    mouse_rbits_set(rbits, &n_rbits, cur_id, bit_pos);
    if(info->f_x.present && info->f_y.present && info->f_y.report_id == info->f_x.report_id) {
        info->layout_valid = true;
        info->uses_report_id = saw_report_id;
        info->report_id = info->f_x.report_id;
        if(info->f_wheel.present && info->f_wheel.report_id != info->report_id) info->f_wheel.present = false;
        if(info->f_pan.present && info->f_pan.report_id != info->report_id) info->f_pan.present = false;
        if(info->f_buttons.present && info->f_buttons.report_id != info->report_id) info->f_buttons.present = false;
        uint16_t need = 0;
        const mouse_field_t *fields[5] = {&info->f_buttons, &info->f_x, &info->f_y, &info->f_wheel, &info->f_pan};
        for(int k = 0; k < 5; k++)
            if(fields[k]->present) {
                uint16_t end = fields[k]->bit_offset + fields[k]->bit_size;
                if(end > need) need = end;
            }
        info->min_len = (need + 7) / 8;
    }
    info->x_bits = info->f_x.bit_size;
    info->y_bits = info->f_y.bit_size;
    info->button_count = info->f_buttons.bit_size;
    info->has_wheel = info->f_wheel.present;
    info->wheel_byte_offset = info->f_wheel.bit_offset / 8;
    info->has_pan = info->f_pan.present;
    info->report_length = (mouse_rbits_get(rbits, n_rbits, info->report_id) + 7) / 8 + (info->uses_report_id ? 1 : 0);
    if(info->x_bits == 8 && info->y_bits == 8) info->type = MOUSE_TYPE_STANDARD_8BIT;
    else if(info->x_bits == 12 && info->y_bits == 12) info->type = MOUSE_TYPE_HIGHRES_12BIT;
    else if(info->x_bits == 16 && info->y_bits == 16) info->type = MOUSE_TYPE_GAMING_16BIT;
    else info->type = MOUSE_TYPE_UNKNOWN;
    return info->type;
}

/* ---- the state (the PicoMite's process_mouse_input, into mousestruct[2]) ------------------------------------------- */
static float RemX, RemY;                                            // the fractions of a pixel the last reports left
// a report's movement dx, dy in the mouse's counts, scale pixels a count
static void MouseInput(int dx, int dy, int8_t wheel, uint8_t buttons, float scale) {
    static uint64_t leftpress = 0;
    static uint8_t leftstate = 0;
    volatile struct s_nunstruct *m = &mousestruct[USBMOUSE_CHANNEL];
    uint64_t now = Getuptime();
    int oldL = m->L, oldR = m->R;
    if(now - leftpress > 500000) {                                  // a double click: press, release and press in 500 ms
        leftstate = 0;
        m->Z = 0;
    }
    if(leftstate == 0 && (buttons & 1)) { leftpress = now; leftstate = 1; }
    if(leftstate == 1 && !(buttons & 1)) { leftpress = now; leftstate = 2; }
    if(leftstate == 2 && (buttons & 1)) {
        leftpress = now;
        leftstate = 3;
        m->Z = 1;
        MouseIntAnyFound = 1;
        if(MouseIntAny != NULL) IntSignal();
    }
    m->L = (buttons & 1) ? 1 : 0;
    m->R = (buttons & 2) ? 1 : 0;
    m->C = (buttons & 4) ? 1 : 0;
    TOUCH_DOWN = m->L;                                              // (the GUI controls' touch, as the CMM2's other mice)
    int xmax = HRes > 0 ? HRes : 1024, ymax = VRes > 0 ? VRes : 1024;
    float fx = RemX + (float)dx * scale, fy = RemY + (float)dy * scale;
    int ix = (int)fx, iy = (int)fy;                                 // (whole pixels, toward zero: the rest is kept)
    RemX = fx - (float)ix;
    RemY = fy - (float)iy;
    m->ax += ix;
    if(m->ax >= xmax) { m->ax = xmax - 1; RemX = 0.0f; }
    if(m->ax < 0) { m->ax = 0; RemX = 0.0f; }
    m->ay += iy;
    if(m->ay >= ymax) { m->ay = ymax - 1; RemY = 0.0f; }
    if(m->ay < 0) { m->ay = 0; RemY = 0.0f; }
    m->az += wheel;
    if(m->x0 != buttons) {                                          // any button changed: MOUSE INTERRUPT ENABLE's interrupt
        MouseIntAnyFound = 1;
        if(MouseIntAny != NULL) IntSignal();
    }
    m->x0 = buttons;
    if(!oldL && m->L) { mouse2foundz = 1; if(mouse2Interruptz != NULL) IntSignal(); }   // CONTROLLER MOUSE OPEN 2's left,
    if(!oldR && m->R) { mouse2foundc = 1; if(mouse2Interruptc != NULL) IntSignal(); }   // right and left-up interrupts
    if(oldL && !m->L) { mouse2leftup = 1; if(mouse2Interruptu != NULL) IntSignal(); }
    int cw = FontTable[gui_font >> 4][0] * (gui_font & 0b1111), ch = FontTable[gui_font >> 4][1] * (gui_font & 0b1111);
    if(cw > 0) m->x1 = m->ax / cw;                                  // the character cell (the PicoMite's editor reads it)
    if(ch > 0) m->y1 = m->ay / ch;
    mouseupdated = 1;
}

// a report from the mouse in HID slot 'slot' (the interrupt's callback: nothing here prints)
void USBMouseReport(uint8_t const *report, uint16_t len, int slot) {
    mouse_info_t mi;
    float sp;
    int dx = 0, dy = 0, wheel = 0;
    uint8_t buttons = 0;
    if(report == NULL || len == 0 || UsbMouse != slot + 1) return;
    memcpy(&mi, (const void *)&HID[slot].mouse_info, sizeof(mouse_info_t));
    memcpy(&sp, (const void *)Option.mousespeed, sizeof(float));
    if(!(sp > 0.0f && sp <= 1000.0f)) sp = 1.0f;
    uint8_t proto = tuh_hid_get_protocol(HID[slot].Device_address, HID[slot].Device_instance);
    if(proto == HID_PROTOCOL_BOOT || !mi.layout_valid) {            // the boot layout: buttons, X, Y [, wheel]
        if(len < 3) return;
        buttons = report[0];
        dx = (int8_t)report[1];
        dy = (int8_t)report[2];
        if(len > 3) wheel = (int8_t)report[3];
    } else {
        if(mi.uses_report_id) {
            if(report[0] != mi.report_id) return;                   // another of the interface's reports
            report++;
            len--;
        }
        if(len < mi.min_len) return;
        buttons = (uint8_t)mouse_field_value(report, &mi.f_buttons);
        dx = mouse_field_value(report, &mi.f_x);
        dy = mouse_field_value(report, &mi.f_y);
        wheel = mouse_field_value(report, &mi.f_wheel);
    }
    MouseInput(dx, dy, (int8_t)wheel, buttons, 0.5f / sp);           // (halved, as the PicoMite's, and divided by the option)
}

// the mouse in HID slot 'slot' has been set up (USBKeyboard.c): it is channel 2 from now on
void USBMouseAttached(int slot) {
    volatile struct s_nunstruct *m = &mousestruct[USBMOUSE_CHANNEL];
    memset((void *)m, 0, sizeof(struct s_nunstruct));
    RemX = RemY = 0.0f;
    m->ax = (HRes > 0 ? HRes : 1024) / 2;                           // (the CMM2's PS/2 mouse starts at the centre too)
    m->ay = (VRes > 0 ? VRes : 1024) / 2;
    UsbMouse = slot + 1;
}

void USBMouseDetached(void) {
    volatile struct s_nunstruct *m = &mousestruct[USBMOUSE_CHANNEL];
    UsbMouse = 0;
    m->L = m->R = m->C = m->Z = 0;
    m->x0 = 0;
    TOUCH_DOWN = 0;
}

/* ---- the BASIC side --------------------------------------------------------------------------------------------- */

// MOUSE INTERRUPT ENABLE n, sub | MOUSE INTERRUPT DISABLE n | MOUSE SET n, x, y [, w] (the PicoMite's)
void cmd_usbmouse(void) {
    char *tp;
    int c;
    if((tp = checkstring(cmdline, "INTERRUPT ENABLE"))) {
        getargs(&tp, 3, ",");
        if(argc != 3) error("Syntax");
        getint(argv[0], 1, 4);
        MouseIntAny = GetIntAddress(argv[2]);
        IntSignal();
        return;
    }
    if((tp = checkstring(cmdline, "INTERRUPT DISABLE"))) {
        getargs(&tp, 1, ",");
        if(argc != 1) error("Syntax");
        getint(argv[0], 1, 4);
        MouseIntAny = NULL;
        return;
    }
    if((tp = checkstring(cmdline, "SET"))) {
        getargs(&tp, 7, ",");
        if(!(argc == 7 || argc == 5)) error("Syntax");
        getint(argv[0], 2, 2);
        if((c = MouseOpenChannel()) < 0) error("No mouse");
        mousestruct[c].ax = getint(argv[2], 0, HRes - 1);
        mousestruct[c].ay = getint(argv[4], 0, VRes - 1);
        if(argc == 7) mousestruct[c].az = getint(argv[6], -1000000, 1000000);
        return;
    }
    error("Syntax");
}

// DEVICE(MOUSE n, X|Y|L|R|M|W|B|D|T): the mouse in use, whatever it is (0 when there is none), as the PicoMite's
void fun_device_fn(void) {
    char *tp;
    if((tp = checkstring(ep, "MOUSE"))) {
        getargs(&tp, 3, ",");
        if(argc != 3) error("Syntax");
        getint(argv[0], 1, 4);
        int c = MouseOpenChannel();
        volatile struct s_nunstruct *m = &mousestruct[c < 0 ? 0 : c];
        char *k = argv[2];
        if(checkstring(k, "X")) iret = c < 0 ? 0 : m->ax;
        else if(checkstring(k, "Y")) iret = c < 0 ? 0 : m->ay;
        else if(checkstring(k, "L")) iret = c < 0 ? 0 : m->L;
        else if(checkstring(k, "R")) iret = c < 0 ? 0 : m->R;
        else if(checkstring(k, "M")) iret = c < 0 ? 0 : m->C;
        else if(checkstring(k, "W")) iret = c < 0 ? 0 : m->az;
        else if(checkstring(k, "B")) iret = c < 0 ? 0 : m->x0;
        else if(checkstring(k, "D")) {
            iret = c < 0 ? 0 : m->Z;
            if(c >= 0) m->Z = 0;
        } else if(checkstring(k, "T")) iret = c < 0 ? 0 : m->classic[0];
        else error("Syntax");
        targ = T_INT;
        return;
    }
    if((tp = checkstring(ep, "GAMEPAD"))) {                         // DEVICE(GAMEPAD n, funct) (USBGamepad.c)
        USBGamepadDevice(tp);
        return;
    }
    if((tp = checkstring(ep, "WII")) || (tp = checkstring(ep, "CLASSIC"))) {   // DEVICE(WII x), DEVICE(CLASSIC x): CLASSIC(x)
        ep = tp;                                                    // on the Wii connector, channel 3 (MiscSTM32.c)
        fun_classic();
        return;
    }
    if((tp = checkstring(ep, "NUNCHUCK"))) {                        // DEVICE(NUNCHUCK x): NUNCHUK(x) there
        ep = tp;
        fun_nunchuck();
        return;
    }
    error("Syntax");
}
