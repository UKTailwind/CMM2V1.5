/* The firmware's OPL3 synth (G2 only): ymfm's YMF262 behind C, Src/OPLChip.cpp. A chip is OPLChipSize() bytes the
   caller provides (the heap), made by OPLChipInit and ended by OPLChipDone; it makes 16-bit stereo at the rate given. */
#ifndef OPLCHIP_HEADER
#define OPLCHIP_HEADER
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint32_t OPLChipSize(void);
void OPLChipInit(void *chip, uint32_t rate);                         // made and reset: silent, OPL2 mode
void OPLChipDone(void *chip);
void OPLChipWrite(void *chip, uint32_t reg, uint32_t value);        // reg 0-0x1FF (0x100 up: the second bank)
void OPLChipGenerate(void *chip, int16_t *buf, uint32_t frames);    // frames of left, right
#ifdef __cplusplus
}
#endif
#endif
