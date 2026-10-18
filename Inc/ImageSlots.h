/* ImageSlots.h - the PicoMite's image slots (FLASH LOAD IMAGE) and the 4-bit writer into a page (ImageSlots.c) */
#ifndef IMAGESLOTS_H
#define IMAGESLOTS_H
#include <stdint.h>
#define MAXIMAGESLOTS 8
uint8_t *ImageSlotAddress(int slot);
uint8_t *ImageSlotPixels(int slot, int *w, int *h);
void ImageSlotBlit(const uint8_t *src, int srcw, int srch, int sx, int sy, int w, int h, int page, int dx, int dy, int transparent);
void ImageSlotsForget(void);
void BlitFlash(char *p);
void TileMapForget(void);
extern const int Rgb121Colours[16];
#endif
