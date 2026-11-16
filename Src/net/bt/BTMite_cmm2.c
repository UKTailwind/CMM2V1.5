/* BTMite_cmm2.c: the Colour Maximite 2's side of the PicoMite's Bluetooth host (Src/net/bt/picomite, copied by
   tools/port_bt.py; docs/Bluetooth_Plan.md, step 3): what the PicoMite's own core gives that code, done the CMM2's way.
   Inc/net/BTMite_cmm2.h maps the PicoMite's names onto these.

     process_mouse_input       a Bluetooth mouse, or a keyboard's touchpad: the CMM2's USB mouse movement (USBMouse.c),
                               on channel 2, so DEVICE(MOUSE 2), MM.MOUSE and the GUI cursor follow it
     process_consumer_report   the media keys, as the PicoMite's (input/KeyboardMap.c): codes 0x8A-0x90 into the keys
     BtNunstruct and the rest  the PicoMite's nunstruct[] for DEVICE(GAMEPAD): BTKeyboard.c's game controller ([3])
     bt_audio_pull and the rest   OPTION AUDIO BLUETOOTH's sound: the audio interrupt's pairs (HdmiAudio.h's BtPut)
     BtMouse                   a Bluetooth mouse or touchpad is connected (USBMouse.c's MouseOpenChannel)
     BtSaveOptions             the pairings' save (BTKeyboard.c's bt_tlv_save, by tools/port_bt.py) */
#include "BTMite_cmm2.h"                         // (first, as the PicoMite's files have it: Inc/net/BTMite_cmm2.h)
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "picomite/BTKeyboard.h"
#include "picomite/BTClassicKeyboard.h"
#include "HdmiAudio.h"
#include "btstack.h"
#include "btstack_memory.h"

/* ---- the mouse ---- */
void BtMouseInput(int dx, int dy, int8_t wheel, uint8_t buttons);   // (USBMouse.c)

void process_mouse_input(int16_t x_delta, int16_t y_delta, int8_t wheel_delta, uint8_t buttons, uint8_t n) {
    (void)n;                                                        // (the PicoMite's slot 2: the CMM2's channel 2 too)
    BtMouseInput(x_delta, y_delta, wheel_delta, buttons);
}

int BtMouse(void) {
    return bt_keyboard_has_pointer() || bt_ckbd_has_pointer();
}

/* ---- the pairings' save ----
   BTKeyboard.c saves Option.bt_tlv from the Bluetooth poll once BTstack has stopped writing it. SaveOptions rewrites
   sector 2 with mmbrun after the options, which needs a 128 KB copy of it in RAM: on a G1 running a program that is the
   heap's, and there may be none. Then the pairings stay in RAM (LoadOptions keeps them while bt_tlv_dirty is set) and
   the save is tried again every 2 s, not an error that ends the program. */
void BtSaveOptions(void) {
    extern int OptionSaveSoft, OptionSaveFailed;
    extern volatile bool bt_tlv_dirty;
    static uint32_t failed_ms;
    static int waiting;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (waiting && now - failed_ms < 2000) {
        bt_tlv_dirty = true;
        return;
    }
    OptionSaveFailed = 0;
    OptionSaveSoft = 1;
    SaveOptions(1);
    waiting = OptionSaveFailed;
    if (waiting) {
        failed_ms = now;
        bt_tlv_dirty = true;
    }
}

/* ---- BTstack's memory given back (cyw43_arch_cmm2.c's cyw43_arch_bt_forget: a G1 restarting its radio) ---- */
void BtFreeMemory(void) {
    hci_deinit();                                                   // (hci_stack, from malloc)
    btstack_memory_deinit();                                        // (the pools: every buffer btstack_memory gave)
    uint32_t *r = BtRing;
    BtRing = NULL;                                                  // (the audio interrupt stops filling it first)
    free(r);
}

/* ---- the media keys: the PicoMite's process_consumer_report (input/KeyboardMap.c), as it is ---- */
bool process_consumer_report(const uint8_t *report, uint16_t len) {
    if (len != 2)
        return false; /* not a single-usage consumer report */
    uint16_t usage = (uint16_t)report[0] | ((uint16_t)report[1] << 8);
    uint8_t code;
    switch (usage) {
    case 0x0000:           /* all media keys released - nothing to emit */
        return true;
    case 0x00E9:           /* Volume Increment */
        code = MM_KEY_VOLUME_UP;
        break;
    case 0x00EA:           /* Volume Decrement */
        code = MM_KEY_VOLUME_DOWN;
        break;
    case 0x00E2:           /* Mute */
        code = MM_KEY_MUTE;
        break;
    case 0x00CD:           /* Play/Pause */
    case 0x00B0:           /* Play */
    case 0x00B1:           /* Pause */
        code = MM_KEY_PLAY_PAUSE;
        break;
    case 0x00B7:           /* Stop */
        code = MM_KEY_STOP;
        break;
    case 0x00B5:           /* Scan Next Track */
    case 0x00B3:           /* Fast Forward */
        code = MM_KEY_NEXT_TRACK;
        break;
    case 0x00B6:           /* Scan Previous Track */
    case 0x00B4:           /* Rewind */
        code = MM_KEY_PREV_TRACK;
        break;
    default:
        return false;      /* unrecognised - let the caller log it raw */
    }
    USR_KEYBRD_ProcessData(code);
    return true;
}

/* ---- DEVICE(GAMEPAD)'s state for the Bluetooth game controller (BTKeyboard.c fills [3]) ---- */
static volatile struct s_nunstruct BtNun[4];
volatile struct s_nunstruct *const BtNunstruct = BtNun;
volatile int BtNunfoundc[4];
char *BtNunInterruptc[4];

/* ---- OPTION AUDIO BLUETOOTH's sound ----
   The PicoMite's bt_audio_pull steps PLAY's buffers itself (misc/SDCard.c). The CMM2's sound is made by TIM4's audio
   interrupt at the sound's own rate (stm32h7xx_it.c), as for the DACs and a G3's HDMI: with OPTION AUDIO BLUETOOTH it
   puts each pair in BtRing too (HdmiAudio.h's BtPut), and this takes them at the speaker's rate, interpolating
   between them at the two clocks' ratio (both PLL1's: they never drift). The reader keeps BT_TARGET pairs behind the
   interrupt: a sound starts once that many are there, which covers the encoder's ticks coming late (the Bluetooth poll
   runs between statements). With the ring empty (the sound ended) the output decays to silence over a few
   milliseconds, not a step (a click). A loadable program's stream (Loader.c's DMA, no interrupt) is not heard. */
#define BT_TARGET 1536                                              // 35 ms at 44.1 kHz
uint32_t *volatile BtRing;                                          // the pairs (HdmiAudio.h), from newlib's heap
volatile uint32_t BtRingW;                                          // the interrupt's next write
volatile uint32_t bt_audio_underruns;                               // (BLUETOOTH STATUS)

// PLAY has a sound running that the audio interrupt makes (the speaker's stream is suspended after OPTION AUDIO
// BLUETOOTH's idle seconds without one: BTAudio.c)
bool bt_audio_playing(void) {
    switch (CurrentlyPlaying) {
    case P_TONE: case P_SOUND: case P_WAV: case P_FLAC: case P_MP3: case P_MOD: case P_TTS: case P_ARRAY:
    case P_SAMPLE: case P_BBC: case P_OPL:
        return true;
    default:
        return false;
    }
}

// frames stereo frames at out_rate (BTAudio.c: its encoder's tick, or the sound thrown away in real time while no
// speaker takes it)
void bt_audio_pull(int16_t *pcm, int frames, uint32_t out_rate) {
    static uint32_t r, frac;                                        // the ring's next read, and 16.16 from prev to cur
    static int32_t prev_l, prev_r, cur_l, cur_r;
    static int primed;                                              // the ring is being played (it had reached BT_TARGET)
    uint32_t *ring = BtRing, step = 0;
    if (ring == NULL && AUDIO_BLUETOOTH) {                          // first asked for: the ring made, which the audio
        ring = malloc(BT_RINGN * sizeof(uint32_t));                 // interrupt fills from now on
        if (ring != NULL) {
            BtRingW = r = 0;
            BtRing = ring;
        }
    }
    if (ring != NULL) {
        uint32_t fill = (BtRingW - r) & (BT_RINGN - 1);
        if (!primed && fill >= BT_TARGET) primed = 1;
        else if (primed && fill > BT_RINGN - 256) r = (BtRingW - BT_TARGET) & (BT_RINGN - 1);   // (overtaken: behind it again)
        // the interrupt's rate: TIM4's clock (PeripheralBusSpeed) / (PSC + 1) / (ARR + 1), as HdmiAudio.c's step has it
        step = (uint32_t)(((uint64_t)PeripheralBusSpeed << 16) / ((uint64_t)(TIM4->PSC + 1) * (TIM4->ARR + 1) * out_rate));
    }
    for (int i = 0; i < frames; i++) {
        if (primed) {
            frac += step;
            while (frac >= 0x10000) {
                frac -= 0x10000;
                if (r == BtRingW) {                                 // empty: the sound ended, or a gap in it
                    primed = 0;
                    if (bt_audio_playing()) bt_audio_underruns++;
                    break;
                }
                uint32_t v = ring[r];
                r = (r + 1) & (BT_RINGN - 1);
                prev_l = cur_l;
                prev_r = cur_r;
                cur_l = (int16_t)v;
                cur_r = (int32_t)v >> 16;
            }
        }
        if (!primed) {
            cur_l = cur_l * 63 / 64;
            cur_r = cur_r * 63 / 64;
            prev_l = cur_l;
            prev_r = cur_r;
            frac = 0;
        }
        pcm[2 * i] = (int16_t)(prev_l + (int32_t)(((int64_t)(cur_l - prev_l) * frac) >> 16));
        pcm[2 * i + 1] = (int16_t)(prev_r + (int32_t)(((int64_t)(cur_r - prev_r) * frac) >> 16));
    }
}
