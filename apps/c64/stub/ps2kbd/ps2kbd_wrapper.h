/* ps2kbd/ps2kbd_wrapper.h: no PS/2 keyboard here, the USB keyboard comes by usbhid_wrapper.c. input_rp2350.cpp is built with its PS/2 code, as its USB code
   uses that code's variables, and this keyboard never has a key */
#ifndef CMM2_PS2KBD_WRAPPER_H
#define CMM2_PS2KBD_WRAPPER_H
#include <stdbool.h>
#include <stdint.h>
static inline void ps2kbd_init(void) { }
static inline void ps2kbd_tick(void) { }
static inline int ps2kbd_get_key(int *pressed, unsigned char *key) { (void)pressed; (void)key; return 0; }
static inline uint8_t ps2kbd_get_modifiers(void) { return 0; }
static inline bool ps2kbd_is_reset_combo(void) { return false; }
static inline uint8_t ps2kbd_get_arrow_state(void) { return 0; }
#endif
