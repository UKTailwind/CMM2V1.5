/* Mono.h - the PicoMite's MODE 1: MODE n,1, its 1-bit pages, their tiles' colours and the merge onto the display
   (Mono.c, the merge in Rgb121.c) */
#ifndef MONO_H
#define MONO_H
#include <stdint.h>

// a colour as a 1-bit page holds it: set unless black (the PicoMite's MODE 1 drawing)
#define MONO1(c) (((c) & 0xFFFFFF) != 0)

#define MONOMAXW 1920                           // the widest mode (a row of its pages is 240 bytes)
#define MONOMINTILEH 8                          // TILE HEIGHT's least (the PicoMite HDMI's)

extern uint8_t *MonoTileF, *MonoTileB;          // each tile's foreground and background colour: CLUT entries
extern int MonoTileH, MonoTilesX, MonoTilesY;   // TILE HEIGHT, the tiles across and down
extern volatile int MonoDirty;                  // something was drawn: the merge runs at the frame's end
extern volatile uint32_t MonoClutNew[8];        // CLUT entries given colours: written to the LTDC at the frame's end

uint32_t MonoLayout(int mode, uint32_t start, uint32_t *sdram);
void MonoClut(void);
void MonoStart(void);
void MonoTilesReset(void);
void MonoPointers(void);
void MonoMerge(void);
void MonoClearScreen(int c);
void MonoSet(int x, int y, int c);
int MonoGet(int x, int y);
void MonoHLine(int x1, int x2, int y, int c);
void MonoPageCopy(int from, int to, int transparent);
void MonoBlit(int x1, int y1, int x2, int y2, int w, int h, int flip);
void MonoRow(uint8_t *d, int dx, const uint8_t *s, int sx, int w, int t);
int MonoTileColour(int x, int y, int set);
#endif
