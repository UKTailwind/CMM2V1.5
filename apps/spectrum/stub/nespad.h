/* nespad.h: pico-spec's NES pads: here the USB gamepads, read by nespad_read (spectrum_cmm2.cpp) */
#ifndef CMM2_NESPAD_H
#define CMM2_NESPAD_H
#include "pico.h"
#define DPAD_LEFT 0x001000
#define DPAD_RIGHT 0x004000
#define DPAD_DOWN 0x000400
#define DPAD_UP 0x000100
#define DPAD_START 0x000040
#define DPAD_SELECT 0x000010
#define DPAD_B 0x000004
#define DPAD_A 0x000001
#define DPAD_Y 0x010000
#define DPAD_X 0x040000
#define DPAD_LT 0x100000
#define DPAD_RT 0x400000
#ifdef __cplusplus
extern "C" {
#endif
extern uint32_t nespad_state, nespad_state2;
bool nespad_begin(uint32_t cpu_khz, uint8_t clk, uint8_t data, uint8_t lat);
void nespad_read(void);
#ifdef __cplusplus
}
#endif
#endif
