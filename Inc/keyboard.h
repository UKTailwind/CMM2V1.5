/*
 * keyboard.h
 *
 *  Created on: 2 Jul 2018
 *      Author: Peter
 *
 * The USB keyboard, through the TinyUSB host stack (USBKeyboard.c) and the
 * PicoMite's report decoder (KeyboardMap.c).  This header is included
 * everywhere through Hardware_Includes.h, so it declares only plain types:
 * the decoder's own header, KeyboardMap.h, is included after tusb.h by the
 * two files that need it.
 */

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <stdint.h>
#include <stdbool.h>
#include "USBMouse.h"

/* One HID interface the host is servicing (the PicoMite's struct s_HID,
   keyboard fields only for now).  report_timer counts milliseconds since
   the last report (USBKeyboardTick); hid_app_task() asks for the next
   report when it reaches report_rate. */
typedef struct s_HID
{
	uint8_t Device_address;
	uint8_t Device_instance;
	uint8_t Device_type;            /* HID_ITF_PROTOCOL_KEYBOARD or _MOUSE, a GAMEPAD_ type (USBGamepad.h), 0 when free */
	uint8_t report_rate;
	int16_t report_timer;
	uint16_t vid;
	uint16_t pid;
	bool active;
	bool report_requested;
	bool notfirsttime;
	uint8_t sendlights;             /* the boot keyboard's LED output report */
	uint8_t mouse_type;             /* a mouse: its report layout (USBMouse.c) */
	mouse_info_t mouse_info;
	uint8_t report[65];             /* the last report: length, then its bytes */
} a_HID;

#define HID_SLOTS 4
extern volatile struct s_HID HID[HID_SLOTS];

extern int caps_lock;
extern int num_lock;
extern int scroll_lock;
extern int KeyDown[7];
extern int keyselect;
extern volatile int keyboardseen;

void USBKeyboardInit(void);         /* bring the host up (OPTION USBKEYBOARD set) */
void CheckKeyboard(void);           /* run the host stack and the keyboard; from routinechecks() */
void USBKeyboardTick(void);         /* every millisecond, from Timer1msHandler() */
void clearrepeat(void);             /* forget the held key, so it does not repeat */
void USR_KEYBRD_ProcessData(uint8_t data);

#endif /* KEYBOARD_H_ */
