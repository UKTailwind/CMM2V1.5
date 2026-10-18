/* USBMouse.h - a USB mouse (the PicoMite's): the report layout its descriptor gives, and the calls between the HID
   host (USBKeyboard.c) and the mouse (USBMouse.c). */
#ifndef USBMOUSE_H
#define USBMOUSE_H
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MOUSE_TYPE_UNKNOWN = 0,
    MOUSE_TYPE_STANDARD_8BIT = 1,                   // 4 bytes: buttons, X, Y, wheel
    MOUSE_TYPE_HIGHRES_12BIT = 2,                   // 5 bytes: buttons, 12-bit X/Y packed, wheel
    MOUSE_TYPE_GAMING_16BIT = 3                     // 6+ bytes: buttons, 16-bit X, 16-bit Y, wheel, ...
} mouse_report_type_t;

// one Input field of the mouse report, from the report descriptor (bit_offset is in the payload, after any report ID)
typedef struct {
    bool present;
    uint16_t bit_offset;
    uint8_t bit_size;
    bool is_signed;                                 // Logical Minimum < 0 on its Input item
    uint8_t report_id;                              // the report it belongs to (0: no IDs used)
} mouse_field_t;

typedef struct {
    mouse_report_type_t type;                       // a summary only: reports are decoded by the fields below
    uint8_t report_length;
    uint8_t x_bits, y_bits, button_count;
    bool has_wheel, has_pan;
    uint8_t wheel_byte_offset;
    bool uses_report_id;
    uint8_t report_id;
    bool layout_valid;                              // X and Y found in one report
    uint8_t min_len;                                // the payload bytes needed to decode
    mouse_field_t f_buttons;                        // the run of 1-bit buttons, as one bitmap
    mouse_field_t f_x, f_y, f_wheel, f_pan;
} mouse_info_t;

#define USBMOUSE_CHANNEL 2                          // a USB mouse is channel 2, as on the PicoMite
extern volatile int UsbMouse;                       // its HID slot + 1 while one is attached, else 0
mouse_report_type_t analyze_mouse_descriptor(const uint8_t *desc_report, uint16_t desc_len, mouse_info_t *info);
void USBMouseReport(uint8_t const *report, uint16_t len, int slot);
void USBMouseAttached(int slot);
void USBMouseDetached(void);
int MouseOpenChannel(void);                         // the mouse in use: 0 PS/2, 1-3 I2C, 2 also USB; -1 none
extern char *MouseIntAny;                           // MOUSE INTERRUPT ENABLE n, sub
extern volatile int MouseIntAnyFound;
#endif
