/* apple_video.h: the Apple's screen drawn at its full resolution (apple_video.c) */
#ifndef APPLE_VIDEO_H
#define APPLE_VIDEO_H
#include <stdint.h>
#include "mii.h"

#define APPLE_W 560                             /* the pixels apple_video_render draws on each of the Apple's 192 lines */
#define APPLE_H 192
#define LOW 64                                  /* the palette: MII's colours from 0, their dimmer ones from LOW, the two masked monos at MASKED */
#define MASKED 126

/* the Apple's 192 lines, 560 pixels each, pitch bytes apart, from its memory and soft switches as they are now */
void apple_video_render(mii_t *mii, uint8_t *frame, int pitch);
/* the palette's first 128 entries, 3 bytes each (R, G, B), from the video's colour set */
void apple_video_palette(const mii_video_t *video, uint8_t *rgb);
#endif
