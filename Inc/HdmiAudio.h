/* HdmiAudio.h: a G3's sound through HDMI (HdmiAudio.c), for Hardware_Includes.h and the audio interrupt
   (stm32h7xx_it.c, which has no Hardware_Includes.h) */
#ifndef HDMIAUDIO_H
#define HDMIAUDIO_H
#include <stdint.h>
// a G3's sound through HDMI (HdmiAudio.c): HdmiRate is the SAI's rate, 0 on a G1 or a G2 (or if it cannot run).
// With it on, iconvert and i2convert keep 16-bit samples at full level for HDMI and the interrupt makes the DACs'
// values from them (Audio.c, stm32h7xx_it.c). HdmiPut: TIM4's interrupt's pair, put in the ring (the D2 SRAM's
// last 3 KB; the 1 KB below it is the SAI's DMA buffer)
extern uint32_t HdmiRate, HdmiK;
extern volatile uint32_t HdmiRingW, HdmiRingR;
extern uint32_t *volatile HdmiStream;
extern int HdmiStreamFrames;
extern uint32_t HdmiPsc(uint32_t rate, uint32_t psc);
extern void HdmiAudioStart(void);
extern void HdmiConvert(int16_t *o, const int16_t *s, int count, int ramped);
#define HDMI_RING  ((uint32_t *)0x30047400)
#define HDMI_RINGN 768
static inline void HdmiPut(int32_t l, int32_t r) {
    uint32_t w = HdmiRingW, n = w + 1 < HDMI_RINGN ? w + 1 : 0;
    if(n == HdmiRingR) return;                              // (full: dropped)
    HDMI_RING[w] = (uint16_t)__SSAT(l, 16) | (uint32_t)__SSAT(r, 16) << 16;
    HdmiRingW = n;
}
// OPTION AUDIO BLUETOOTH (Src/net/bt/BTMite_cmm2.c): the same pairs to a ring of Bluetooth's own, BT_RINGN of them, which
// the speaker's encoder takes at 44.1 kHz (bt_audio_pull). NULL until the speaker's sound is first asked for. Never
// full: the interrupt writes on, and a reader it overtakes starts again behind it.
#define BT_RINGN 4096
extern uint32_t *volatile BtRing;
extern volatile uint32_t BtRingW;
static inline void BtPut(int32_t l, int32_t r) {
    uint32_t w = BtRingW;
    BtRing[w] = (uint16_t)__SSAT(l, 16) | (uint32_t)__SSAT(r, 16) << 16;
    BtRingW = (w + 1) & (BT_RINGN - 1);
}
#define HDMI_DAC(v)  (2000 + ((v) * 1900) / 32768)           // a G3's 16-bit sample to the DACs' value, as iconvert's
#define HDMI_DAC2(v) (2000 + ((v) * 950) / 32768)            // two added (MOD and EFFECT, i2convert's), each full level
#endif
