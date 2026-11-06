/* pico/bootrom.h: there is no USB boot mode: the program starts again */
#include "hardware/watchdog.h"
#define reset_usb_boot(a, b) cmm2_restart()
