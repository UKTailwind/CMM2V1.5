/* USBTouch.h - the PicoMite's USB touch screen (its Hardware_Includes.h and graphics/Draw.h), copied by
   tools/port_usbtouch.py: do not edit, run it again. Included by keyboard.h (struct s_HID's touch fields). */
#ifndef USBTOUCH_H_
#define USBTOUCH_H_

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * Type definitions - USB multi-touch digitizer
 * ============================================================================
 * Windows Precision Touchscreen / generic multi-touch HID. Captured from
 * the report descriptor at enumeration; process_touch_report() then uses
 * these offsets to decode each input report.
 */
#define MAX_TOUCH_CONTACTS 10 /* HID spec allows more but 10 is the \
								 practical cap for consumer screens */

typedef struct
{
	bool valid; /* descriptor looked like a touch screen */
	bool uses_report_id;
	uint8_t report_id;			  /* touch reports often share endpoint with config */
	uint16_t report_length_bytes; /* total report length (incl. report-id byte) */
	uint8_t max_contacts;		  /* number of Finger collections found */

	/* Contact Count (Digitizer usage 0x54) — appears once per report. */
	uint16_t contact_count_bit_offset; /* relative to start of report payload */
	uint8_t contact_count_bits;

	/* The descriptor declares N identical "Finger" collections back-to-
	   back. We capture the first one's layout and the stride to step
	   between subsequent contacts. */
	uint16_t first_contact_bit_offset;
	uint16_t contact_stride_bits;

	/* Offsets WITHIN a single contact block (relative to its start). */
	uint8_t tip_switch_bit_offset;
	uint8_t in_range_bit_offset;
	uint8_t contact_id_bit_offset;
	uint8_t contact_id_bits;
	uint8_t x_bit_offset;
	uint8_t x_bits;
	uint8_t y_bit_offset;
	uint8_t y_bits;

	int32_t x_logical_max; /* for scaling raw → display coords */
	int32_t y_logical_max;

	/* Single-contact pointer fallback. Dual-mode digitizers emit the
	   multitouch Finger report (report_id above) only for 2+ contacts and
	   fall back to a Generic Desktop Mouse collection — its own report ID,
	   one button (= tip) + absolute X/Y — for a single finger. Captured so
	   single touches still register when the multitouch report never comes.
	   Devices with no such collection leave has_pointer_fallback=false. */
	/* Windows/HID "Device Configuration" → "Device Mode" (Input Mode, usage
	   0x52) Feature report. When present, a panel that defaults to mouse /
	   non-reporting mode is switched to multi-touch by writing 0x02 to this
	   feature (the standard hid-multitouch mechanism). Captured report ID. */
	bool has_input_mode;
	uint8_t input_mode_report_id;

	/* Windows touch "bring-up" Feature reports. Some panels (ILITEK ILI2132)
	   that declare no Input Mode only enter digitizer mode after the host
	   performs these GET_FEATURE reads, the way Windows does. Report IDs are
	   captured from the descriptor by usage (0x55 Contact Count Maximum,
	   0xC5 Microsoft certification blob) rather than hardcoded, so the
	   handshake works on any panel that carries them. 0 = not present. */
	uint8_t contact_count_max_report_id;
	uint8_t cert_blob_report_id;

	bool has_pointer_fallback;
	uint8_t pointer_report_id;
	uint16_t pointer_button_bit_offset; /* button 1 = tip, relative to payload */
	uint16_t pointer_x_bit_offset;
	uint8_t pointer_x_bits;
	uint16_t pointer_y_bit_offset;
	uint8_t pointer_y_bits;
	int32_t pointer_x_logical_max;
	int32_t pointer_y_logical_max;
	uint16_t pointer_report_length_bytes;
} touch_info_t;

typedef struct
{
	int16_t x;	   /* raw logical X (0..x_logical_max) */
	int16_t y;	   /* raw logical Y (0..y_logical_max) */
	uint8_t id;	   /* contact identifier, stable across reports for one finger */
	bool tip;	   /* finger touching surface */
	bool in_range; /* finger detected (may be hovering) */
} touch_contact_t;

typedef struct
{
	uint8_t count; /* live contacts in this report */
	touch_contact_t contacts[MAX_TOUCH_CONTACTS];
} touch_report_t;

extern volatile bool usb_touch_present;
extern volatile bool usb_touch_active;
extern volatile int16_t usb_touch_x;
extern volatile int16_t usb_touch_y;
extern volatile uint64_t usb_touch_last_us;
extern volatile bool usb_touch_active2;
extern volatile int16_t usb_touch_x2;
extern volatile int16_t usb_touch_y2;
extern volatile uint8_t usb_touch_count;
extern volatile int16_t usb_touch_xn[MAX_TOUCH_CONTACTS];
extern volatile int16_t usb_touch_yn[MAX_TOUCH_CONTACTS];

#define TOUCH_GESTURES
#ifdef TOUCH_GESTURES
void touch_gesture_on_down(int16_t x, int16_t y);
void touch_gesture_on_up(int16_t end_x, int16_t end_y);
void touch_gesture_tick(int16_t cur_x, int16_t cur_y, bool is_down);
void touch_gesture_pinch_start(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void touch_gesture_pinch_end(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

/* Latched gesture state — all cleared on matching-accessor read. */
extern int touch_swipe_dir;     /* 0=none, 1=L, 2=R, 3=U, 4=D */
extern int touch_tap;           /* 0 / 1 */
extern int touch_longpress;     /* 0 / 1 */
extern int touch_doubletap;     /* 0 / 1 */
extern int touch_pinch_dir;     /* 0=none, 1=expand, 2=contract */
extern int touch_rotate_dir;    /* 0=none, 1=CW, 2=CCW */
extern int touch_twotap;        /* 0 / 1 */
#endif

/* the CMM2's callbacks call these (Src/USBKeyboard.c, Src/USBGamepad.c) */
bool USBTouchMount(uint8_t dev_addr, uint8_t instance, uint8_t const *desc_report, uint16_t desc_len, int slot);
void USBTouchUnmount(uint8_t dev_addr, uint8_t instance, int i);
void USBTouchReport(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len, int n);
void USBTouchTask(int i);
void USBTouchGetReportComplete(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len);
#endif
