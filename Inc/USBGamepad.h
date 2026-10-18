/* USBGamepad.h - USB gamepads (the PicoMite's): the calls between the HID host (USBKeyboard.c) and the gamepads
   (USBGamepad.c). */
#ifndef USBGAMEPAD_H
#define USBGAMEPAD_H
#include <stdint.h>
#include <stdbool.h>

#define GAMEPAD_PS4  128                            // DEVICE(GAMEPAD n, T), as on the PicoMite
#define GAMEPAD_PS3  129
#define GAMEPAD_SNES 130                            // a generic gamepad, decoded by its table entry
#define GAMEPAD_XBOX 131
#define GAMEPAD_SWITCH 133                          // the CMM2's: a Switch Pro Controller, or a pad in its mode (132 is the PicoMite's UNKNOWN)
#define GAMEPAD_XINPUT 134                          // the CMM2's: an Xbox 360 or Xbox One/Series pad (USBXInput.c); T gives XBOX

bool USBGamepadMount(uint8_t dev_addr, uint8_t instance, int slot);    // claims the slot if it is a known gamepad
void USBGamepadReport(uint8_t const *report, uint16_t len, int slot);
void USBGamepadDetached(int slot);
void USBGamepadTask(void);                          // the PS4's light bar and motors (from hid_app_task)
void USBGamepadService(void);                       // GAMEPAD MONITOR's print (from CheckKeyboard, outside tuh_task)
void USBGamepadDevice(char *tp);                    // DEVICE(GAMEPAD n, funct)
int USBGamepadXInputMount(uint8_t daddr, bool one); // an Xbox pad (USBXInput.c; one: GIP): its HID slot, -1 if none free
void USBGamepadXInputReport(uint8_t const *report, uint16_t len, int slot);
void USBGamepadXInputDetached(int slot);
void XInputRumble(int slot, uint8_t left, uint8_t right);   // (USBXInput.c)
bool XInputClaimed(uint8_t daddr);
int USBGamepadSlot(void);                           // a gamepad's HID slot, counted as a device (USBKeyboard.c); -1 none
void USBGamepadSlotFree(int slot);
char *GamepadInterrupt(void);                       // check_interrupt: GAMEPAD INTERRUPT ENABLE's, when due
void GamepadInterruptsOff(void);                    // at the start and end of a run
#endif
