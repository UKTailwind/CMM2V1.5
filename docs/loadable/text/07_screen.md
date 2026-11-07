# The Screen

A program draws by writing pixels into display memory: the firmware sets up the video mode and gives the program the address and layout of the page that is shown. There are two ways to use it. The program can write straight into the page (Doom does), or it can keep its picture in its own memory and have it copied to the page at each frame, by the processor or the DMA2D (MicroPython does). When the program ends, MMBasic's video mode, font, colours and palette are put back.

## Setting a Mode

```
int video_mode(int mode, int depth, cmm2_video *v);         /* version 2 */

typedef struct {
    uint8_t *fb;                /* the first pixel of the page shown */
    uint32_t width, height;     /* in pixels */
    uint32_t pitch;             /* bytes from a line's start to the next's */
    uint32_t bpp;               /* bits a pixel: 8 or 16 */
    uint32_t copies;            /* 1, or 2 in a mode whose lines are shown twice */
} cmm2_video;
```

video_mode sets MMBasic's MODE mode,depth, clears the page and fills in v. It returns 0, or -1 if the mode or depth is not valid on this computer (as MODE would refuse it: MODE 16 needs a Generation 2; or, on a Generation 1 that has lent the program its display memory, a mode whose page does not fit in the internal RAM: see The Display Memory Lent in the chapter How a Program Runs), or -2 if MMBasic has a frame buffer of another depth. The depth is 8 (256 colours from the palette) or, from version 6, 16 (RGB565: two bytes a pixel, 65,536 colours; not in modes 15 and 16).

The modes are MMBasic's:

{widths: 10,25,10,25,30}
| Mode | Size | Mode | Size | Notes |
|---|---|---|---|---|
| 1 | 800 x 600 | 10 | 848 x 480 | |
| 2 | 640 x 400 | 11 | 1280 x 720 | |
| 3 | 320 x 200 | 12 | 960 x 540 | lines shown twice: 3, 5, 6, 7, 12, 13 and 17 |
| 4 | 480 x 432 | 13 | 400 x 300 | |
| 5 | 240 x 216 | 14 | 960 x 540 | |
| 6 | 256 x 240 | 15 | 1280 x 1024 | 8-bit only |
| 7 | 320 x 240 | 16 | 1920 x 1080 | Generation 2, 8-bit only |
| 8 | 640 x 480 | 17 | 384 x 240 | |
| 9 | 1024 x 768 | 18 | 1024 x 600 | |

Each pixel is a byte in depth 8 and two bytes (red in the top five bits, green in the middle six, blue in the bottom five) in depth 16. Line y starts at v.fb + y * v.pitch.

**Modes whose lines are shown twice.** The low resolution modes (3, 5, 6, 7, 12, 13 and 17) are shown with each line repeated, and their page holds each line twice: v.copies is 2, v.pitch is twice a line's width, and line y must be written at v.fb + y * v.pitch and again just after it. vidtest.c and Doom do it this way:

```
static void line(int y, const uint8_t *px) {          /* depth 8 */
    uint8_t *p = v.fb + y * v.pitch;
    memcpy(p, px, v.width);
    if(v.copies == 2) memcpy(p + v.width, px, v.width);
}
```

A mode change makes the monitor find the new picture, which takes a second or two (the firmware cannot shorten it), so a program sets its mode once at the start.

The page is not held in the processor's data cache, so what the program writes is on the screen at once. It is slower to write than the program's own memory, a byte at a time most of all: build a line or a frame in the program's memory and copy it, with memcpy() or the DMA2D.

## The Palette

```
int video_palette(int start, int count, const uint8_t *rgb);   /* version 2 */
```

In depth 8 each pixel is an index into a palette of 256 colours. video_palette sets count of them from start, from red, green and blue bytes (three for each colour), and returns 0, or -1 if the depth is not 8 or the range is wrong. It waits for the start of the next frame and loads the colours then, so the change is seen whole, and a palette changed every frame (for fades and colour cycling) costs nothing more. Doom loads its palette this way when it changes. MMBasic's palette is put back when the program ends.

## Waiting for a Frame

```
void video_wait(void);                                   /* version 2 */
```

video_wait returns at the start of the display's next frame (or after 50ms, whatever happens). The frame rate depends on the mode (MODE 3 runs at 75 frames a second, MODE 18 at 60), and a program can time its frames with micros() to find it. Drawing just after video_wait, while the screen's first lines are being sent, gives a picture without tearing for a program that draws in the shown page.

## A Display in the Program's Memory

Version 6 adds the services for a program that keeps its picture in its own memory and has it copied to the screen at each frame, as the Pico Computer 3's hdmi module does (MicroPython's display is built this way):

```
int  video_scaled(int mode, int depth, int scale, cmm2_video *v);
void frame_hook(void (*fn)(void));
int  dma2d_copy(const void *src, uint32_t src_pitch, void *dst, uint32_t dst_pitch,
                uint32_t width, uint32_t height);
int  dma2d_busy(void);
```

video_scaled is video_mode with each pixel shown scale times in each direction (scale is 1 or 2). With 2 the display is half the mode's width and height (MODE 18 at 2 gives 512 x 300), and its page holds each line twice, as in the modes whose lines are shown twice (v.copies is 2). It is refused in those modes, which are doubled already.

frame_hook asks for fn to be called at the start of each frame's blanking interval, the moment to copy a frame to the page without tearing, and frame_hook(NULL) stops it. fn is called in an interrupt and must follow the rules in Code Called From an Interrupt in the chapter Sound and Music: be quick, and call no services but dma2d_copy and dma2d_busy.

dma2d_copy has the processor's DMA2D copy a rectangle while the program goes on: width bytes from each of height lines, the source's lines src_pitch bytes apart and the destination's dst_pitch. Up to three copies wait their turn, each starting when the one before ends. It returns 0, or -1 if three are waiting already or a size, pitch or address is odd. It writes the source from the data cache to memory first, so the copy sees what the program last wrote; the program must not change the source until the copy is done. dma2d_busy returns how many copies are not yet done. A frame copied this way costs the processor very little. This example keeps a 320 x 240 picture in the program's memory and shows it in MODE 7, whose page holds each line twice, so it is copied twice, to the first and the second copy of each line:

```
static uint8_t frame[240][320];               /* the picture, depth 8 */
static cmm2_video v;

static void hook(void) {                      /* each frame, in an interrupt */
    cmm2->dma2d_copy(frame, 320, v.fb, v.pitch, 320, 240);
    cmm2->dma2d_copy(frame, 320, v.fb + v.width, v.pitch, 320, 240);
}

int main(int argc, char **argv) {
    if(cmm2->version < 6 || cmm2->video_mode(7, 8, &v) < 0) return 1;
    cmm2->frame_hook(hook);
    while(!cmm2->poll()) {
        draw(frame);                          /* the program's drawing */
    }
    cmm2->frame_hook(NULL);
    return 0;
}
```

A copy in progress while the program draws the next frame shows a mixture of the two: double buffering (two frames, drawing one while the other is copied) avoids that.

When the program ends its frame hook is removed and copies still waiting are dropped.

## Drawing

Version 10 adds drawing by MMBasic's own routines, the ones its CLS, PIXEL, LINE, BOX, RBOX, CIRCLE, TRIANGLE, TEXT, FONT, COLOUR and BLIT READ and WRITE commands use, on the display video_mode set (or MMBasic's, if the program set none):

```
int gfx_cls(int colour);
int gfx_pixel(int x, int y, int colour);
int gfx_line(int x1, int y1, int x2, int y2, int width, int colour);
int gfx_box(int x, int y, int w, int h, int width, int colour, int fill);
int gfx_rbox(int x, int y, int w, int h, int radius, int colour, int fill);
int gfx_circle(int x, int y, int r, int width, float aspect, int colour, int fill);
int gfx_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int colour, int fill);
int gfx_text(int x, int y, const char *s, const char *just, int font, int scale, int colour, int bg);
int gfx_font(int font, int scale, int *w, int *h);
int gfx_colour(int colour, int bg);
int gfx_read(int x, int y, int w, int h, uint8_t *bgr);
int gfx_write(int x, int y, int w, int h, const uint8_t *bgr);
int gfx_save(const char *name);
```

Colours are MMBasic's: RGB888 (0xRRGGBB, with MMBasic's transparency nibble in bits 24-27, as its RGB() gives them), made the mode's own, so the same calls draw in every mode and depth. A fill or background of -1 is none. The arguments are the commands': a box's w and h are its size, aspect 1 makes a circle round (2 an ellipse twice as wide), and gfx_text's just is TEXT's ("" for the default, "CM" centred on x and y...), its font 1-16 and scale 1-15, or 0 for the console's (FONT's). gfx_font sets the console's font, as FONT does (font 0 leaves it), and gives the size of its characters in pixels; gfx_colour sets the console's colours, as COLOUR does. gfx_read gives an area's pixels as 3 bytes each, blue, green and red (BLIT READ's buffer), and gfx_write puts them back. gfx_save saves the screen as a PNG, as SAVE PNG and Print Screen do (in about 0.4 seconds for 800 x 600), or as a BMP if the name ends in .BMP.

Each returns 0, or -4 when MMBasic's command would have given an error (a font that is not loaded, a justification it does not know, a folder that is not there), with the message in io_error(). Drawing this way is slower than writing the page directly, a call a shape, but the shapes, the fonts and the colours are MMBasic's own. A program can mix the two: the page video_mode gives is the one these draw on.

```
cmm2_video v;
if(cmm2->version < 10 || cmm2->video_mode(1, 8, &v) < 0) return 1;
cmm2->gfx_cls(0);
cmm2->gfx_box(20, 20, 160, 100, 3, 0x0FFFFFFF, 0x0FFF0000);   /* white, filled red */
cmm2->gfx_circle(400, 70, 50, 2, 1.0f, 0x0F00FF00, -1);       /* a green ring */
cmm2->gfx_text(400, 200, "Hello", "CM", 1, 3, 0x0FFFFFFF, -1);
cmm2->gfx_save("A:/hello.png");
```
