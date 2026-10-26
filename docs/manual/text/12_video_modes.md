# Video Modes and Pages

The video output of the Colour Maximite 2 is set by the MODE command:

```
MODE r [, bits [, bg [, int]]]
```

'r' is the screen resolution, a number from 1 to 18; 'bits' is the colour depth (see below); 'bg' is the background colour in the 12-bit mode; and 'int' is a subroutine that will be called at the start of the frame blanking (useful for updating the screen without tearing). The default at power up is MODE 1 with an 8-bit colour depth, or the mode set with OPTION DEFAULT MODE.

## Resolutions

{widths: 10,35,10,45}
| Mode | Resolution | Mode | Resolution |
|---|---|---|---|
| 1 | 800 x 600 pixels | 10 | 848 x 480 pixels (widescreen) |
| 2 | 640 x 400 pixels | 11 | 1280 x 720 pixels (widescreen) |
| 3 | 320 x 200 pixels | 12 | 960 x 540 pixels (widescreen, lines duplicated) |
| 4 | 480 x 432 pixels | 13 | 400 x 300 pixels |
| 5 | 240 x 216 pixels | 14 | 960 x 540 pixels (widescreen, lines not duplicated - not supported by all monitors) |
| 6 | 256 x 240 pixels | 15 | 1280 x 1024 pixels |
| 7 | 320 x 240 pixels | 16 | 1920 x 1080 pixels (widescreen, Generation 2 only) |
| 8 | 640 x 480 pixels | 17 | 384 x 240 pixels |
| 9 | 1024 x 768 pixels (no 12-bit mode) | 18 | 1024 x 600 pixels |

All resolutions except 10, 11, 12, 14 and 16 work perfectly with monitors that have an aspect ratio of 4:3 or widescreen monitors that can switch to that ratio (most widescreen monitors will do this automatically).

The monitor will see these signals: modes 1 and 13, 800 x 600 at 75Hz; modes 2, 3, 4, 5, 6, 7, 8 and 17, 640 x 480 at 75Hz; mode 9, 1024 x 768 at 60Hz; mode 10, 848 x 480 at 60Hz; mode 11, 1280 x 720 at 60Hz; mode 14, 1920 x 540 at 60Hz; modes 12 and 16, 1920 x 1080 at 60Hz; mode 15, 1280 x 1024 at 60Hz.

The GETSCANLINE function returns the line of the monitor that is being drawn. Its maximum value in each mode is: mode 1, 627; modes 2, 4 and 8, 499; modes 3, 5, 6, 7 and 17, 249; mode 9, 805; mode 10, 516; mode 11, 749; mode 12, 562; mode 13, 313; mode 14, 568; mode 15, 1065; mode 16, 1124; mode 18, 637.

## Colour Depths

{widths: 14,14,14,14,14,15,15}
| | 32-bit (G2) | 16-bit | 12-bit | 8-bit | 4-bit | 1-bit |
|---|---|---|---|---|---|---|
| Pixel format | ARGB8888 | RGB565 | ARGB4444 | RGB332 | RGB121 | Set or clear |
| Bits per pixel | 32 (4 bytes) | 16 (2 bytes) | 16 (2 bytes) | 8 (1 byte) | 4 (two pixels a byte) | 1 (eight pixels a byte) |
| Colours | 16,777,216 | 65,536 | 4,096 | 256 | 16 | Two in each tile, 256 on the screen |
| Transparency | None | None | 16 levels | None | In the layers | None (the layer is ORed) |
| Pages displayed | 1 | 1 | 2 + background | 1 | 1 to 3 (the layers) | 1 or 2 (the layer) |

The 32-bit depth is available on the Generation 2 only.

The 12-bit depth is particularly useful for games: it supports 4096 colours with an additional 4-bit alpha channel allowing 16 levels of transparency. It has two image layers (pages 0 and 1) plus a background colour ('bg' in the MODE command). Page 0 is the lower layer and page 1 the upper, so the stack is: background, page 0, page 1, with each one overwriting the previous in turn as defined by the transparency values of each individual pixel.

In the 8-bit depth each of the 256 colours can be changed with the MAP command.

The 4-bit depth is the PicoMite's 16 colour display: see The PicoMite's 16 Colour Display below. The 1-bit depth is the PicoMite's MODE 1: see The PicoMite's MODE 1 below.

## Pages

In addition to the page that is displayed there are many extra video pages that are available to the programmer for building images. The display always shows the contents of page 0 (and pages 0 and 1 in the 12-bit depth) unless overridden with the PAGE DISPLAY command. PAGE WRITE sets the page that the graphics commands draw on, and PAGE COPY copies one page to another, which can be done during the video blanking period to update the display without flashing and tearing. PAGE SCROLL scrolls a page, PAGE STITCH and PAGE MERGE combine pages, and PAGE RESIZE changes the size of a page.

The pages available in each mode and colour depth are (Generation 1 / Generation 2):

{widths: 8,13,16,16,16,15,16}
| Mode | 32-bit (G2) | 16-bit | 12-bit | 8-bit | 4-bit (G2) | 1-bit |
|---|---|---|---|---|---|---|
| 1 | 0 to 2 | 0-2 / 0-6 | 0-2 / 0-6 | 0-6 / 0-15 | 0 to 28 | 0-47 / 0-60 |
| 2 | 0 to 6 | 0-6 / 0-14 | 0-6 / 0-14 | 0-13 / 0-29 | 0 to 57 | 0-60 / 0-60 |
| 3 | 0 to 27 | 0-25 / 0-57 | 0-24 / 0-56 | 0-53 / 0-60 | 0 to 60 | 0-60 / 0-60 |
| 4 | 0 to 7 | 0-7 / 0-17 | 0-7 / 0-17 | 0-15 / 0-35 | 0 to 60 | 0-60 / 0-60 |
| 5 | 0 to 33 | 0-30 / 0-60 | 0-29 / 0-60 | 0-60 / 0-60 | 0 to 60 | 0-60 / 0-60 |
| 6 | 0 to 28 | 0-26 / 0-60 | 0-25 / 0-59 | 0-53 / 0-60 | 0 to 60 | 0-60 / 0-60 |
| 7 | 0 to 20 | 0-19 / 0-46 | 0-18 / 0-45 | 0-41 / 0-60 | 0 to 60 | 0-60 / 0-60 |
| 8 | 0 to 4 | 0-4 / 0-10 | 0-4 / 0-10 | 0-10 / 0-23 | 0 to 47 | 0-60 / 0-60 |
| 9 | - | 0-1 / 0-3 | - | 0-3 / 0-8 | - | 0-28 / 0-60 |
| 10 | 0 to 3 | 0-2 / 0-7 | 0-2 / 0-7 | 0-7 / 0-17 | 0 to 34 | 0-54 / 0-60 |
| 11 | - | 0 / 0-2 | - | 0-2 / 0-6 | - | 0-21 / 0-55 |
| 12 | - | 0 / 0-4 | - | 0-3 / 0-11 | - | 0-38 / 0-60 |
| 13 | 0 to 12 | 0-11 / 0-28 | 0-10 / 0-27 | 0-26 / 0-60 | 0 to 60 | 0-60 / 0-60 |
| 14 | 0 to 2 | 0-2 / 0-6 | 0-2 / 0-6 | 0-6 / 0-14 | 0 to 27 | 0-46 / 0-60 |
| 15 | - | - | - | 0-1 / 0-4 | - | 0-12 / 0-37 |
| 16 | - | - | - | - / 0-2 | - | - / 0-20 |
| 17 | 0 to 16 | 0-15 / 0-38 | 0-14 / 0-37 | 0-34 / 0-60 | 0 to 60 | 0-60 / 0-60 |
| 18 | 0 to 1 | 0-1 / 0-4 | 0-1 / 0-4 | 0-6 / 0-10 | - | 0-35 / 0-60 |

A "-" means that the combination is not available. The 4-bit figures are for the Generation 2 (the Generation 1, with less memory, has fewer). MM.INFO(MAX PAGES) returns the highest page number in the current mode.

## Video Memory

MM.INFO(PAGE ADDRESS n) and MM.INFO(MAX PAGES) are useful if you wish to PEEK or POKE the video memory. In all cases the memory is arranged as a two dimensional array x,y so, to get the address of a specific pixel on a specific page n:

- With an 8-bit colour depth you would use: add% = MM.INFO(PAGE ADDRESS n) + y * MM.HRES + x
- For the 12 or 16-bit colour depth you would use: add% = MM.INFO(PAGE ADDRESS n) + (y * MM.HRES + x) * 2
- With the 4-bit colour depth two pixels share a byte, the left pixel in the low four bits: add% = MM.INFO(PAGE ADDRESS n) + y * ((MM.HRES + 1) \ 2) + x \ 2
- With the 1-bit colour depth eight pixels share a byte, the leftmost in bit 0: add% = MM.INFO(PAGE ADDRESS n) + y * (MM.HRES \ 8) + x \ 8, and the pixel is bit x AND 7

For modes 3, 5, 6, 7, 12, 13 and 17 in the 8, 12, 16 and 32-bit depths each line in page 0 is duplicated to get square pixels so y needs to be multiplied by 2 for PEEK and both lines y * 2 and y * 2 + 1 need to be POKEd (4 and 1-bit pages hold each line once). For example, for an 8-bit colour depth:

```
add1% = MM.INFO(PAGE ADDRESS n) + (y * 2) * MM.HRES + x
add2% = MM.INFO(PAGE ADDRESS n) + (y * 2 + 1) * MM.HRES + x
```

This duplication will also apply to page 1 in 12-bit colour modes. For the 12 and 16-bit modes you can use POKE SHORT and PEEK(SHORT) which are designed for this purpose.

## The PicoMite's 16 Colour Display (MODE n,4)

A colour depth of 4 bits gives the display that the PicoMite's VGA and HDMI versions have in their 16 colour modes, so that programs written for the PicoMite (and their PEEKs and POKEs of the screen) work as they do there. MODE 7,4 (320 x 240) is the PicoMite's MODE 2 and MODE 8,4 (640 x 480) is its MODE 3. The 4-bit depth is available in the modes that the table above shows.

- The 16 colours are the PicoMite's RGB121 colours. The MAP command changes them (MAP(n) returns the PicoMite's value for colour n), and MAP RESET restores them.
- The pixels are held two to a byte, the left one in the low four bits, at MM.INFO(WRITEBUFF) as on the PicoMite.
- The FRAMEBUFFER command works as it does on the PicoMite: FRAMEBUFFER CREATE makes the framebuffer F, LAYER the layer L (shown over the display, with a transparent colour), LAYER TOP the top layer T, and CREATE 2 a second framebuffer; WRITE, COPY, MERGE, WAIT and CLOSE act on them. The framebuffers are pages 0 to 4 (N, L, F, 2 and T) and the pages after them are more 4-bit pages, for PAGE WRITE, PAGE COPY, BLIT and the like.
- BLIT, SPRITE and TILEMAP work on 4-bit pages, with colour 0 (black) as the transparent colour of a sprite.

## The PicoMite's MODE 1 (MODE n,1)

A colour depth of 1 bit gives the PicoMite's MODE 1, its default mode, so that programs written for it (and their PEEKs and POKEs of the screen) work as they do there. MODE 8,1 (640 x 480) is the PicoMite's MODE 1 on its VGA version, and on its HDMI version at 640 x 480. The 1-bit depth is available in every mode.

- A pixel is set or clear: drawn in any colour but black it is set, drawn in black it is cleared.
- The screen is divided into tiles 8 pixels wide and TILE HEIGHT tall (the height of the font when the mode was set). Each tile has a foreground colour, which its set pixels show, and a background colour, which the others show. The TILE command sets them, and so does text that lines up with the tiles: PRINT and TEXT in a font whose characters are whole tiles (such as font 1 with tiles 12 high), at a position that is a multiple of 8 across and of the tile height down, colour the tiles they cover. CLS gives every tile the colour COLOUR set on the colour it clears to.
- The pixels are held eight to a byte, the leftmost in bit 0, at MM.INFO(WRITEBUFF) as on the PicoMite. PIXEL( returns the colour a pixel shows.
- The tiles' colours are held to 15 bits, as on the PicoMite's HDMI version, and up to 256 different colours can be on the screen at once.
- The FRAMEBUFFER command works as it does on the PicoMite: CREATE makes the framebuffer F, LAYER the layer L, which is ORed with the display, CREATE 2 a second framebuffer, and MERGE makes the display F ORed with 2.
- BLIT, SPRITE and TILEMAP work on 1-bit pages, a pixel of a sprite set unless its colour is black or nearly black (black among the PicoMite's 16 colours). BLIT also moves the tiles' colours when the area lines up with the tiles.
- MAP, PAGE DISPLAY, the IMAGE commands, GUI CURSOR and RAY are not available.

In the other colour depths FRAMEBUFFER is the Colour Maximite 2's own: a working area larger than the screen, with FRAMEBUFFER CREATE width, height, WRITE, BACKUP, RESTORE, WINDOW and CLOSE (see the FRAMEBUFFER command). While a framebuffer exists, a MODE command that changes the colour depth is refused with the error "Colour mismatch with framebuffer": use FRAMEBUFFER CLOSE first.

## Image Slots

FLASH LOAD IMAGE n, file$ loads an image into image slot n (1 to 8), as the PicoMite's games do. Slots 4 to 8, the PicoMite's RAM slots, are held in RAM until RAM ERASE, an overwrite or a restart, as the PicoMite's are, so a program that loads its images every time it starts uses FLASH LOAD IMAGE's O (OVERWRITE). Slots 1 to 3, the PicoMite's flash slots, are kept on the SD card and read into RAM when they are first used (see FLASH SAVE). MM.INFO(FLASH ADDRESS n) returns a slot's address and BLIT FLASH draws from it. The image is held in the PicoMite's format (an 8 byte width and height, then two pixels a byte), so programs that PEEK it work as they do on the PicoMite.

## Running PicoMite Programs

A program written for the PicoMite selects its display with the PicoMite's MODE numbers, which mean different resolutions on the Colour Maximite 2 (MODE 1 here is 800 x 600). OPTION PICOMITE gives it the display it expects without changes:

- OPTION PICOMITE says which PicoMite the program was written for, by the name it returns in MM.DEVICE$ (eg, OPTION PICOMITE PicoMiteVGA RP2350B, or VGA or HDMI for short), and at which of its resolutions (640x480, 720x400, 800x600 or 848x480, and with HDMI also 800x480, 1024x600, 1280x720 and 1024x768). If none is given it is the resolution of OPTION DEFAULT MODE (eg, 1024x768 with DEFAULT MODE 9 and HDMI), so that the monitor keeps one resolution for the command prompt, the editor and the programs. MM.DEVICE$ then returns that name, so a program that configures itself by it does so as on that PicoMite.
- MODE n with no colour depth is then the PicoMite's MODE n: 1 to 3 on VGA, 1 to 5 on HDMI. MM.HRES and MM.VRES are the PicoMite's sizes, and so are its framebuffers and its screen memory for PEEK and POKE. A program that does not set a mode starts in MODE 1, as on the PicoMite, and so does a Colour Maximite 2 program while the option is set (OPTION PICOMITE NONE turns it off). MODE always clears the screen.
- Each PicoMite resolution is shown with one of the Colour Maximite 2's display timings, the same for all its modes, as on the PicoMite, so the monitor does not change when the mode does. In the smaller modes each pixel is 2 x 2 or 4 x 4, as on the PicoMite. MM.INFO(MODE) gives the Colour Maximite 2 mode whose timing is used.
- Two resolutions do not fill the screen: 720x400 and 800x480 are shown at the top left of 848 x 480. The rest of the screen is black and cannot be drawn on.
- PAGE DISPLAY is not available in MODE 5 at 1024x600, 1280x720 and 1024x768, whose rows are each shown four times by MMBasic.
- When the program ends its display stays at the command prompt until EDIT or FM is used, as on the PicoMite.
- With HDMI, RESOLUTION changes the resolution in a program, and the 16 colours of MODE 2 and 3 are the PicoMite HDMI's (its greens are 0x55 and 0xAA rather than VGA's 0x40 and 0x80). With VGA the tiles of MODE 1 are in VGA's 16 colours.
- MODE with a colour depth is still the Colour Maximite 2's MODE.

{widths: 16,14,14,14,14,14,14}
| PicoMite resolution | Timing (mode) | MODE 1 | MODE 2 | MODE 3 | MODE 4 (HDMI) | MODE 5 (HDMI) |
|---|---|---|---|---|---|---|
| 640x480 | 640 x 480 (8) | 640x480 | 320x240, 2x2 | 640x480 | 320x240, 2x2 | 320x240, 2x2 |
| 720x400 | 848 x 480 (10) | 720x400 | 360x200, 2x2 | 720x400 | 360x200, 2x2 | 360x200, 2x2 |
| 800x600 | 800 x 600 (1) | 800x600 | 400x300, 2x2 | 800x600 | - | 400x300, 2x2 |
| 848x480 | 848 x 480 (10) | 848x480 | 424x240, 2x2 | 848x480 | - | 424x240, 2x2 |
| 800x480 (HDMI) | 848 x 480 (10) | 800x480 | 400x240, 2x2 | 800x480 | - | 400x240, 2x2 |
| 1024x600 (HDMI) | 1024 x 600 (18) | 1024x600 | 256x150, 4x4 | 512x300, 2x2 | - | 256x150, 4x4 |
| 1280x720 (HDMI) | 1280 x 720 (11) | 1280x720 | 320x180, 4x4 | 640x360, 2x2 | - | 320x180, 4x4 |
| 1024x768 (HDMI) | 1024 x 768 (9) | 1024x768 | 256x192, 4x4 | 512x384, 2x2 | - | 256x192, 4x4 |

Each entry is the PicoMite's screen, with the size of its pixels on the monitor where they are larger than one; the timing is that of the Colour Maximite 2 mode given. MODE 1 is in the 1-bit colour depth, MODE 2 and 3 in the 4-bit depth, MODE 4 in the 16-bit depth and MODE 5 in the 8-bit depth. Without OPTION PICOMITE, change a program's MODE statements as follows so that it has the display it expects:

{widths: 30,30,40}
| PicoMite | Colour Maximite 2 | Display |
|---|---|---|
| MODE 1 | MODE 8,1 | 640 x 480, set or clear pixels in coloured tiles |
| MODE 2 | MODE 7,4 | 320 x 240, 16 colours |
| MODE 3 | MODE 8,4 | 640 x 480, 16 colours |
| MODE 4 | MODE 7,16 | 320 x 240, RGB555 on the PicoMite (16-bit here) |
| MODE 5 | MODE 7,8 | 320 x 240, 256 colours |

See also the chapter Porting Programs.
