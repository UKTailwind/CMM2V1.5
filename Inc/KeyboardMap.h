/*
 * KeyboardMap.h -- HID boot-keyboard report decoder and international
 * keymaps, from the PicoMite (input/KeyboardMap.h).  USBKeyboard.c hands
 * each 8-byte keyboard report to process_kbd_report(), which tracks keys
 * down and up, fills KeyDown[] for KEYDOWN(), handles Caps/Num/Scroll lock
 * and pushes the resulting characters into the console receive buffer.
 *
 * The PicoMite's Bluetooth keyboards, consumer-control (media key) reports
 * and mouse post-decode are not part of the CMM2 build.
 */

#ifndef KEYBOARDMAP_H
#define KEYBOARDMAP_H

#include <stdint.h>
#include <stdbool.h>

/* The 8-byte boot-keyboard report.  tinyusb's class/hid/hid.h declares the
   same struct; when it has been included first (tusb.h), ours is skipped.
   The sentinel was _TUSB_HID_H_ before tinyusb 0.19 and TUSB_HID_H_ since. */
#if !defined(TUSB_HID_H_) && !defined(_TUSB_HID_H_)
typedef struct {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} hid_keyboard_report_t;
#endif

/* A HID scancode and modifier byte through the layout chosen by
   OPTION USBKEYBOARD (UK/US/DE/FR/ES/BE): an ASCII or MMBasic special-key
   code, or 0 for no character. */
uint8_t APP_MapKeyToUsage(uint8_t keyCode, int modifier);

/* Push one character into the console receive buffer: the break key,
   ON KEY's key, OPTION ... CR/LF/CRLF for Enter, a 25 ms guard against
   a doubled report, and overflow (the oldest character is dropped). */
void USR_KEYBRD_ProcessData(uint8_t data);

/* Decode a boot-keyboard report from HID slot n (USBKeyboard.c's HID[]). */
void process_kbd_report(hid_keyboard_report_t const *report, uint8_t n);

/* The lock keys as a boot keyboard's LED output report holds them: bit 0
   Num Lock, bit 1 Caps Lock, bit 2 Scroll Lock. */
uint8_t kbd_lock_leds(void);

extern int caps_lock;
extern int num_lock;
extern int scroll_lock;

/* KeyDown[0..5] the characters of the keys held (up to six), KeyDown[6]
   the modifier bits.  Read by KEYDOWN(). */
extern int KeyDown[7];

/* The layout table in use, set when a keyboard is mounted. */
extern const int *keylayout;
extern const int UKkeyValue[], USkeyValue[], DEkeyValue[], FRkeyValue[], ESkeyValue[], BEkeyValue[];

extern int USBcode;                 /* the last key's HID scancode */
extern uint32_t repeattime;         /* ms to the next auto-repeat */

#endif /* KEYBOARDMAP_H */
