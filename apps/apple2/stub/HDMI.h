/* HDMI.h: the part of frank-apple's display driver its start screen calls: the 320 x 240 picture in 4 bits a pixel that the start screen and the disk menu
   draw in (apple_cmm2.c shows it) */
#ifndef CMM2_HDMI_H
#define CMM2_HDMI_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint8_t *graphics_get_buffer(void);
#ifdef __cplusplus
}
#endif
#endif
