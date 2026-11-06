/* ps2kbd_mrmltr.h: pico-spec's keyboard driver: here tick() takes the firmware's USB keyboard report (spectrum_cmm2.cpp) */
#ifndef CMM2_PS2KBD_H
#define CMM2_PS2KBD_H
#include "tusb.h"
typedef void (*cmm2_kbd_cb)(hid_keyboard_report_t const *report, hid_keyboard_report_t const *prev);
#define pio1 0
class Ps2Kbd_Mrmltr {
    cmm2_kbd_cb cb;
public:
    Ps2Kbd_Mrmltr(int pio, int pin, cmm2_kbd_cb f) : cb(f) { (void)pio; (void)pin; }
    void init_gpio() { }
    void tick();
};
#endif
