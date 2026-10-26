/* Rgb121.h - the PicoMite's RGB121 display: MODE n,4, its 4-bit pages, the merge onto the display and the PicoMite's
   FRAMEBUFFER (Rgb121.c), which MODE n,1 shares (Mono.h) */
#ifndef RGB121_H
#define RGB121_H
#include <stdint.h>

// a colour (RGB888) as the PicoMite's RGB121 value: bit 3 red, bits 2-1 green, bit 0 blue (its RGB121())
#define RGB121(c) ((((c) & 0x800000) >> 20) | (((c) & 0xC000) >> 13) | (((c) & 0x80) >> 7))
// a colour as an 8-bit page holds it (RGB332)
#define RGB332(c) ((((c) & 0xE00000) >> 16) | (((c) & 0xE000) >> 11) | (((c) & 0xC0) >> 6))
// a colour as a page of one byte or less a pixel holds it: RGB121 in MODE n,4, 1 or 0 in MODE n,1 (set unless black),
// RGB332 in the 8-bit modes
#define NATIVE8(c) (VideoColour == 4 ? RGB121(c) : VideoColour == 1 ? (((c) & 0xFFFFFF) != 0) : RGB332(c))

// the PicoMite's framebuffers in MODE n,4 are pages 0 to 4: N the display, L the layer, F the framebuffer, 2 the second
// framebuffer, T the top layer (a bit of Rgb121Buffers each, set while it is created)
#define RGB121_N 0
#define RGB121_L 1
#define RGB121_F 2
#define RGB121_2 3
#define RGB121_T 4

extern uint8_t *Rgb121Display;                  // the 8-bit page the LTDC shows, the merge's output (not in the page table)
extern int Rgb121DisplayScale;                  // 2 where the mode stores each row twice
extern volatile int Rgb121Active;               // the merge runs at each frame
extern int Rgb121Buffers;                       // the framebuffers created: bit RGB121_L and so on
extern int Rgb121Transparent, Rgb121TransparentTop; // the colours (0-15) L and T are transparent in

int Rgb121ModeFits(int mode);
int Rgb121SizeFits(int w, int h, int scale);
void Rgb121CloseAll(void);
uint32_t Rgb121Layout(int mode, uint32_t start);
void Rgb121Clut(void);
void SetMapRGB(int slot, uint32_t colour);      // the colours a 4-bit mode shows (MAP n = c, MAP SET): Rgb121.c
void CommitMapRGB(void);
void Rgb121Shown(char *p, int n);               // ReadBuffer4's colours as shown, after MAP (SAVE IMAGE and PNG)
void CursorOverlaySet(int on);                  // GUI CURSOR in a 4 or 1-bit mode: over the merge's display
void CursorOverlayColour(int c);
void Rgb121Palette(void);
void Rgb121Pointers(void);
void Rgb121Framebuffer(void);
int Rgb121Letter(char *p);
int Rgb121Cls(void);
void Rgb121Set(int x, int y, int c);
int Rgb121Get(int x, int y);
void Rgb121HLine(int x1, int x2, int y, int c);
void Rgb121PageCopy(int from, int to, int transparent);
void Rgb121Blit(int x1, int y1, int x2, int y2, int w, int h, int flip);
void Rgb121BlitFramebuffer(char *p);
void Rgb121BlitResize(char *p);
void Rgb121Row(uint8_t *d, int dx, const uint8_t *s, int sx, int w, int t);
void Rgb121MergeRequest(void);
#endif
