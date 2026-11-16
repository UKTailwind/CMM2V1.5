/* BTMite_cmm2.h: what the PicoMite's Bluetooth host (Src/net/bt/picomite, copied by tools/port_bt.py) has from the
   PicoMite's own headers and core, on the Colour Maximite 2 (docs/Bluetooth_Plan.md, step 3). CMakeLists.txt makes it
   the first include of those files only (-include), as WebMite_cmm2.h is the network files'.

   It is WebMite_cmm2.h (the CMM2 is a PicoMite PC3 here: PICOMITEWEB, the WiFi on the same chip) with:
     PICOMITEBTH, PICOMITEPC3       the PicoMite's Bluetooth host build, and the PC3's OPTION RADIO
     malloc, calloc, free           the C library's (newlib's arena, below MMBasic's SDRAM heap): WebMite_cmm2.h makes
                                    them lwIP's for the network code, whose block exists only with the WiFi on
     time_us_32(), to_ms_since_boot(), get_absolute_time()   the CMM2's microsecond clock (MicrosNow)
     cyw43_arch_async_context()     none (pico/async_context.h: its lock does nothing on the CMM2's one thread)
     AUDIO_BLUETOOTH, _IDLE         OPTION AUDIO BLUETOOTH (the PicoMite's io/Audio.h)
     PC3_WIFI_ON, PC3_BT_ON, PC3_RADIO_USED   OPTION RADIO (the PicoMite's Hardware_Includes.h)
     KBD_SOURCE_BT_LE, _CLASSIC     the PicoMite's input/KeyboardMap.h
     CONFIG_GR                      the CMM2's CONFIG_DE (OPTION KEYBOARD GR: German) */
#ifndef BTMITE_CMM2_H
#define BTMITE_CMM2_H

#include "WebMite_cmm2.h"
#undef calloc
#undef malloc
#undef free

#define PICOMITEBTH 1
#define PICOMITEPC3 1

#include "pico/async_context.h"
static inline async_context_t *cyw43_arch_async_context(void) { return (async_context_t *)0; }
#define time_us_32() ((uint32_t)MicrosNow())
static inline uint32_t to_ms_since_boot(absolute_time_t t) { return (uint32_t)(t / 1000u); }

/* ---- the PicoMite's io/Audio.h, Hardware_Includes.h and input/KeyboardMap.h ---- */
#define AUDIO_BLUETOOTH (Option.audio_bluetooth != 0)
#define AUDIO_BLUETOOTH_IDLE (Option.audio_bluetooth - 1)
#define RADIO_WIFI_OFF 1
#define RADIO_BT_OFF 2
#define PC3_WIFI_ON (!(Option.Radio & RADIO_WIFI_OFF))
#define PC3_BT_ON (!(Option.Radio & RADIO_BT_OFF))
#define PC3_RADIO_USED (PC3_WIFI_ON || PC3_BT_ON)
#define KBD_SOURCE_BT_LE 0xF0
#define KBD_SOURCE_BT_CLASSIC 0xF1
#define CONFIG_GR CONFIG_DE
#define MM_KEY_VOLUME_UP   0x8A
#define MM_KEY_VOLUME_DOWN 0x8B
#define MM_KEY_MUTE        0x8C
#define MM_KEY_PLAY_PAUSE  0x8D
#define MM_KEY_STOP        0x8E
#define MM_KEY_PREV_TRACK  0x8F
#define MM_KEY_NEXT_TRACK  0x90

/* ---- the PicoMite's core, on the CMM2 (Src/net/bt/BTMite_cmm2.c, Src/main.c) ---- */
#define MMputchar(c, ...) MMputchar(c)          // (the PicoMite's has a flush argument; the CMM2's flushes by itself)
#define __compiler_memory_barrier() __asm volatile ("" : : : "memory")
extern volatile int ConsoleRedirect;            // BLUETOOTH STATUS #n's output (Src/main.c, as the PicoMite's)
char *OutputRedirect(char *p);
// the PicoMite's io/Serial.c: its Bluetooth serial device is COM7; the CMM2's COM4 to COM7 are USB serial devices, so
// here it is COM8 (Src/Serial.c), open while its state's open is set
#define com7 (BtCom.open)
void SerialBTReceive(const uint8_t *data, int len);
// the PicoMite's I2C.h: DEVICE(GAMEPAD)'s state, of which BTKeyboard.c fills [3] with a Bluetooth game controller
#define nunstruct BtNunstruct
#define nunfoundc BtNunfoundc
#define nunInterruptc BtNunInterruptc
extern volatile struct s_nunstruct *const BtNunstruct;
extern volatile int BtNunfoundc[4];
extern char *BtNunInterruptc[4];
// the PicoMite's input/KeyboardMap.c and misc/SDCard.c (OPTION AUDIO BLUETOOTH's sound)
void BtSaveOptions(void);                       // (Src/net/bt/BTMite_cmm2.c: tools/port_bt.py's rewrite in BTKeyboard.c)
bool process_consumer_report(const uint8_t *report, uint16_t len);
void process_mouse_input(int16_t x_delta, int16_t y_delta, int8_t wheel_delta, uint8_t buttons, uint8_t n);
bool bt_audio_playing(void);
void bt_audio_pull(int16_t *pcm, int frames, uint32_t out_rate);
extern volatile uint32_t bt_audio_underruns;

#endif
