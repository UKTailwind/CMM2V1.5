/* ImageSlots.h - the PicoMite's image slots (FLASH LOAD IMAGE) and the 4-bit writer into a page (ImageSlots.c) */
#ifndef IMAGESLOTS_H
#define IMAGESLOTS_H
#include <stdint.h>
#define MAXIMAGESLOTS 8
#define MAXFLASHSLOTS 3                                             // the flash slots, 1 to 3 (then the RAM slots)
extern int FlashLoad;
extern char *ProgFlash;
extern int ProgFromSlot;
void SlotProgramCheck(int slot);
char *SlotProgram(int slot);
void SlotProgramLeave(void);
void FlashRunSlot(int slot, int chain);
int RamSlotBlocks(void **blk, int *size);
int TileMapUses(const uint8_t *img);
uint8_t *ImageSlotAddress(int slot);
uint8_t *ImageSlotPixels(int slot, int *w, int *h, const char *none);
void ImageSlotBlit(const uint8_t *src, int srcw, int srch, int sx, int sy, int w, int h, int page, int dx, int dy, int transparent);
void ImageSlotsForget(void);
void BlitFlash(char *p);
void TileMapForget(void);
extern const int Rgb121Colours[16];
#endif
