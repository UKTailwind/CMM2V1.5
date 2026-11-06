/* graphics.h: pico-spec's display driver, here spectrum_cmm2.cpp */
#ifndef CMM2_GRAPHICS_H
#define CMM2_GRAPHICS_H
#include "pico.h"
#ifdef __cplusplus
extern "C" {
#endif
extern int graphics_max_tft_freq_mhz;
void graphics_set_palette(uint8_t i, uint32_t rgb);
void graphics_set_buffer(uint8_t *buffer, uint16_t width, uint16_t height);
void graphics_init(void);
void graphics_set_pio_clk_div(float d);
void graphics_set_bgcolor(uint32_t rgb);
void graphics_set_flashmode(bool a, bool b);
#ifdef __cplusplus
}
#endif
#endif
