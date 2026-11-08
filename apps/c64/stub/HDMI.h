/* HDMI.h: the part of frank-c64's display driver the rest of it calls, on the loader's services (c64_cmm2.c) */
#ifndef CMM2_HDMI_H
#define CMM2_HDMI_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void graphics_set_palette(uint8_t i, uint32_t rgb);
uint32_t graphics_get_palette(uint8_t i);
uint8_t *graphics_get_buffer_line(int y);
#ifdef __cplusplus
}
#endif
#endif
