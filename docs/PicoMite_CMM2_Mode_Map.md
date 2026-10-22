# PicoMite display modes on the CMM2 (OPTION PICOMITE)

22 Oct 2026. Every display mode of the PicoMite's VGA and HDMI builds, the CMM2 mode each one becomes under OPTION
PICOMITE, and the decisions so far.

Sources:
- the PicoMite tree at 49f45c4;
- the CMM2 tree at main.

The facts were gathered from the code, with file:line references, and the ones the mapping rests on were checked
again. Refresh rates were computed from the timing values in the code, not measured.

## 1. Decisions

Peter, 22 Oct 2026:

1. **OPTION PICOMITE VGA | HDMI | NONE**, NONE by default. A program's MODE numbers mean different things on the
   PicoMite's VGA and HDMI builds, so the option says which one the program was written for. NONE: MODE is the CMM2's,
   as today.
2. **A size the CMM2 does not have becomes the next bigger one:** the smallest CMM2 resolution (by pixel count) at least
   as wide and at least as tall. Everything the program draws fits.
3. **MM.HRES and MM.VRES report the PicoMite's size.** The program sees exactly the PicoMite's screen:
   - drawing is clipped to that area, so what the PicoMite would cut off does not show in the spare space;
   - the console's text wraps and scrolls at that width and height;
   - CLS clears the whole screen, so nothing is left in the spare border.
4. **The PicoMite's area is at the top left** of the larger screen. There is no offset ("adding an offset just adds
   complexity").
5. **MODE 1 is to be discussed** (section 5).
6. **The resolution** (Peter, 22 Oct 2026, the open questions answered):
   - OPTION PICOMITE HDMI or VGA takes the PicoMite's resolution as a second argument, with 640x480 the default
     (`OPTION PICOMITE HDMI, 1280x720`).
   - **The RESOLUTION command is to be implemented as well**, because it can be run in a program as MODE can. It
     changes the resolution the mapping uses, and so the CMM2 mode, and leaves MODE 1, as the PicoMite's RESOLUTION
     does (Draw.c:5185-5251).
7. **A program with no MODE statement runs in MODE 1** (mapped as section 5 decides), as on a PicoMite booted in its
   default mode. What that looks like is part of the MODE 1 discussion.
8. **The end of a program:**
   - On the PicoMite the program's mode stays at the prompt (Commands.c:4107-4109), until EDIT or FM is entered;
     those go back to MODE 1.
   - Under OPTION PICOMITE the CMM2 does the same: the program's mode (n,4 or whichever it was mapped to) stays at the
     prompt, and EDIT and FM go back to the CMM2's own display. Today the CMM2 goes back to its default mode when a
     program ends (main.c:885).
9. **MODE always clears the screen and closes the framebuffers**, as the PicoMite's does (Draw.c:4932-4934), even
   with the same mode and depth. The CMM2's MODE does not clear then (ltdc.c:240-254).

Open:

- **RP2040 against RP2350 VGA.** The RP2350 VGA build is a superset of the RP2040's: MODE 3, and 800x600 and 848x480.
  A program for the RP2040 uses only modes and sizes the RP2350 has, so one VGA setting covers both.

## 2. The PicoMite's modes

MODE exists only on the VGA and HDMI builds (AllCommands.h:606-616). There are 5 modes on HDMI, 3 on RP2350 VGA and 2
on RP2040 VGA (configuration.h:71, 209, 252). Sizes are set in ResetDisplay (graphics/Draw.c:6320-6381).

**Colour in each mode:**
- MODE 1: 1 bit per pixel plus tile colours (TILE, MODE 1 only).
- MODE 2 and 3: 4 bits per pixel, 16 colours. The colours are RGB121 on VGA. On HDMI they are a palette of RGB555, or
  of RGB332 at the RGB332 resolutions.
- MODE 4: RGB555, on HDMI at 640x480 and 720x400 only (Draw.c:4951-4957).
- MODE 5: 8 bits per pixel, a 256-entry palette. At RGB332 resolutions it is raw RGB332, because MAP has no effect
  there (PicoMite #123).

**Layers the scanout shows** (L is the layer, T the top layer):
- MODE 1: L ORed in, with no transparent colour.
- MODE 2: L, plus T on the RP2350 and at HDMI's 640x480, 720x400 and the 4x4 resolutions.
- MODE 3: L only. T can be created but is never shown.
- MODE 4: L, whose transparent colour is an RGB colour.
- MODE 5: L, plus T where MODE 2 has it.

### VGA (RP2040: MODE 1-2 at 640x480 and 720x400; RP2350: MODE 1-3 at all four)

| OPTION RESOLUTION | MODE 1 | MODE 2 | MODE 3 (RP2350) |
|---|---|---|---|
| 640x480 (the default, 75 Hz at 315 MHz; 60 Hz at 252 or 378 MHz) | 640x480 | 320x240 | 640x480 |
| 720x400 | 720x400 | 360x200 | 720x400 |
| 800x600 (RP2350) | 800x600 | 400x300 | 800x600 |
| 848x480 (RP2350) | 848x480 | 424x240 | 848x480 |

### HDMI, full builds (HDMI, HDMIUSB, PC3)

| OPTION RESOLUTION | MODE 1 | MODE 2 | MODE 3 | MODE 4 | MODE 5 |
|---|---|---|---|---|---|
| 640x480 (RGB555 output) | 640x480 | 320x240 | 640x480 | 320x240 | 320x240 |
| 720x400 (RGB555 output) | 720x400 | 360x200 | 720x400 | 360x200 | 360x200 |
| 800x600 | 800x600 | 400x300 | 800x600 | - | 400x300 |
| 848x480 | 848x480 | 424x240 | 848x480 | - | 424x240 |
| 800x480 | 800x480 | 400x240 | 800x480 | - | 400x240 |
| 1024x600 | 1024x600 | 256x150 (4x4) | 512x300 | - | 256x150 |
| 1280x720 | 1280x720 | 320x180 (4x4) | 640x360 | - | 320x180 |
| 1024x768 | 1024x768 | 256x192 (4x4) | 512x384 | - | 256x192 |

The cut-down HDMI builds (HDMIBTH, HDMIWEB) are RGB332 throughout:
- 640x480 and 720x400: MODE 1, 2 and 5;
- 800x600, 848x480 and 800x480: MODE 1 and 2 (but see PicoMite #122);
- 1024x600: MODE 1, 2, 3 and 5.

**Default fonts** (Draw.c:4994-5028):
- MODE 1 and 3: font 1 (8x12);
- MODE 2, 4 and 5: font 7 (6x8);
- at 1280x720 and 1024x768, MODE 1 uses font 3 (16x24).

The CMM2's fonts 1, 3 and 7 are the same sizes, except that the VGA PicoMite's font 3 is a 16x16 font (arial_bold,
graphics/Draw.c:87-95): OPTION PICOMITE VGA would need it as font 3. The PicoMite's fonts 8 (4x6) and 9 (8x10) are
built into the CMM2 since 22 Oct 2026.

The **default 16 colours differ** between the builds: VGA's greens are 0x40 and 0x80 (Draw.c:255), HDMI's 0x55 and 0xAA
(MAP16DEF, HDMI.c:79). The CMM2's 4-bit colours are VGA's (ImageSlots.c:70-71), so OPTION PICOMITE HDMI should start
from HDMI's.

## 3. The CMM2's modes

Resolutions are in ltdc.c:61-62. MODE's checks are in cmd_mode (MiscSTM32.c:1307-1339).

- **Depths** are 4 (the PicoMite's RGB121, Rgb121.c), 8, 12, 16 and 32 (G2 only).
- **4-bit** is allowed only where Rgb121ModeFits allows it: modes 1-8, 10, 13, 14 and 17.
- **Layers:** 12-bit has two layers with alpha. In 4-bit the PicoMite's own N, L, F, 2 and T pages are merged at each
  frame, T included, in every mode. 8, 16 and 32-bit have no layers.

| MODE | Size | Depths | The monitor sees |
|---|---|---|---|
| 1 | 800x600 | 4, 8, 12, 16, 32 | 800x600 @ 60 |
| 2 | 640x400 | all | 640x480 @ 75, letterboxed |
| 3 | 320x200 | all | 640x480 @ 75, 2x2, letterboxed |
| 4 | 480x432 | all | 640x480 @ 75 |
| 5 | 240x216 | all | 640x480 @ 75 |
| 6 | 256x240 | all | 640x480 @ 75 |
| 7 | 320x240 | all | 640x480 @ 75, 2x2 |
| 8 | 640x480 | all | 640x480 @ 75 |
| 9 | 1024x768 | 8, 16 | 1024x768 @ 60 |
| 10 | 848x480 | all | 848x480 @ 60 |
| 11 | 1280x720 | 8, 16 | 720p60 |
| 12 | 960x540 | 8, 16 | 1080p60, 2x2 |
| 13 | 400x300 | all | 800x600 @ 60, 2x2 |
| 14 | 960x540 | all | 1920x540 (not every monitor) |
| 15 | 1280x1024 | 8 | 1280x1024 @ 60 |
| 16 | 1920x1080 | 8 (G2 Rev V only) | 1080p60 |
| 17 | 384x240 | all | 640x480 @ 75 |
| 18 | 1024x600 | 8, 12, 16, 32 | 1024x600 @ 68 (computed) |

"all" = 4, 8, 12, 16 and 32 (G2).

## 4. The mapping

Depth:
- a PicoMite 4-bit mode becomes the CMM2's 4-bit depth: the same pixels, and the PicoMite's FRAMEBUFFER, L and T;
- 8-bit becomes 8-bit (RGB332 and MAP);
- RGB555 becomes 16-bit (RGB565);
- MODE 1 is in section 5.

Size: the same size where the CMM2 has it, otherwise the next bigger (decision 2). "Spare" is what the larger CMM2
screen has beyond the PicoMite's area.

| PicoMite size (where) | CMM2 MODE | Spare | Notes |
|---|---|---|---|
| 320x240 (MODE 2, 4, 5 at 640x480) | 7,4 / 7,16 / 7,8 | - | Exact. MODE 4's RGB-keyed layer has no 16-bit equivalent; MODE 5's L and T have none in 8-bit. |
| 640x480 (MODE 3 at 640x480) | 8,4 | - | Exact. The CMM2 also shows T. |
| 360x200 (MODE 2, 4, 5 at 720x400) | 17,4 / 17,16 / 17,8 | 24 x 40 | 384x240 |
| 720x400 (MODE 3 at 720x400) | 10,4 | 128 x 80 | 848x480 |
| 400x300 (MODE 2, 5 at 800x600) | 13,4 / 13,8 | - | Exact |
| 800x600 (MODE 3 at 800x600) | 1,4 | - | Exact |
| 424x240 (MODE 2, 5 at 848x480) | 4,4 / 4,8 | 56 x 192 | 480x432: the only mode that contains it below 640x400 |
| 848x480 (MODE 3 at 848x480) | 10,4 | - | Exact |
| 400x240 (MODE 2, 5 at 800x480, HDMI) | 13,4 / 13,8 | 0 x 60 | 400x300 |
| 800x480 (MODE 3 at 800x480, HDMI) | 10,4 | 48 x 0 | 848x480 |
| 256x150 (MODE 2, 5 at 1024x600, HDMI) | 6,4 / 6,8 | 0 x 90 | 256x240 |
| 512x300 (MODE 3 at 1024x600, HDMI) | 2,4 | 128 x 100 | 640x400 |
| 320x180 (MODE 2, 5 at 1280x720, HDMI) | 3,4 / 3,8 | 0 x 20 | 320x200 |
| 640x360 (MODE 3 at 1280x720, HDMI) | 2,4 | 0 x 40 | 640x400 |
| 256x192 (MODE 2, 5 at 1024x768, HDMI) | 6,4 / 6,8 | 0 x 48 | 256x240 |
| 512x384 (MODE 3 at 1024x768, HDMI) | 2,4 | 128 x 16 | 640x400 |

MODE 1, at full resolution, would go to:

| Size | CMM2 MODE | Note |
|---|---|---|
| 640x480 | 8 | exact |
| 720x400 | 10 | 848x480, next bigger |
| 800x600 | 1 | exact |
| 848x480 | 10 | exact |
| 800x480 | 10 | 848x480, next bigger |
| 1024x600 | 18 | exact; no 4-bit there |
| 1280x720 | 11 | exact; 8 or 16-bit only |
| 1024x768 | 9 | exact; 8 or 16-bit only |

**What still differs, whatever the mapping:**
- **Frame rates:** 640x480 at 252 or 378 MHz is 60 Hz on the PicoMite and 75 Hz here; 800x600 is 56 against 60;
  1024x768 is 70 against 60; 1024x600 is 60 against 68. These matter to FRAMEBUFFER WAIT and to the ,B of FRAMEBUFFER
  COPY.
- **PEEK and POKE** of the display (MM.INFO(WRITEBUFF)): the layout matches in 4-bit only. The CMM2's 8, 12 and 16-bit
  page 0 doubles its rows in modes 3, 5, 6, 7, 12, 13 and 17.
- **MAP in 4-bit** takes any colour here; on VGA it picks among the 16 hardware colours.

## 5. MODE 1, to be discussed

MODE 1 is the PicoMite's default mode, so it is also what a program with no MODE statement runs in.

**What it is:**
- one bit per pixel;
- TILE: the screen is cells 8 pixels wide and a font height tall (TILE HEIGHT, 8 to the height), each with its own
  foreground and background colour (RGB121 on VGA, RGB555 or RGB332 on HDMI);
- graphics in any non-zero colour only set the bit (DrawPixel2, Draw.c:5328-5345), which shows in the cell's
  foreground colour;
- text that lines up with the cells sets their colours (Draw.c:5518-5545);
- CLS resets every cell (Draw.c:695-733);
- MAP is refused;
- FRAMEBUFFER L is ORed in;
- a program can PEEK and POKE the 1-bit buffer.

**Options:**

| Option | For | Against |
|---|---|---|
| A. 8-bit (n,8): graphics and text in their own colours, TILE ignored | Every MODE 1 size has 8-bit; simplest | TILE does nothing; graphics show their own colour, not the cell's; no 1-bit buffer; no L or F |
| B. 4-bit (n,4), with TILE emulated: a 1-bit shadow of what is set, and each cell redrawn in its colours when TILE or the cell's pixels change | VGA's cell colours are RGB121 already; the PicoMite's FRAMEBUFFER, L and F come with it | No 4-bit at 1024x600, 1280x720 or 1024x768 (8-bit there); TILE emulation is new work; L is overlaid, not ORed |
| C. 8 or 16-bit with TILE emulated the same way | Every size; 16-bit has HDMI's RGB555 cell colours exactly at 640x480 | The most work |

In 1-bit drawing every non-zero colour is "set", so emulating TILE means drawing in the cell's colour, not the
program's. Either all drawing routines go through a MODE 1 path, or each cell is redrawn after drawing. The second is
simpler; the first is faster.

## 6. LCD builds (no MODE)

The PicoMite's LCD builds take the panel's size (OPTION LCDPANEL, SPI-LCD.c:60-118). By decision 2 they would map like
this:
- 320x240: 7,16. VIRTUAL_C, which is 16 colours: 7,4.
- 480x320: 4 (480x432).
- 800x480: 10.
- 240x240: 6 (256x240).
- 160x128: 5 (240x216).
- VIRTUAL_M and VGA222_640 (640x480): 8.

OPTION PICOMITE has no value for them yet.

## 7. Found on the way

**Filed against the PicoMite:**
- #122: HDMIBTH/HDMIWEB undo OPTION RESOLUTION 800x600, 848x480 and 800x480 at the restart it causes.
- #123: HDMI MODE 5's MAP has no effect at the RGB332 resolutions.

**In the CMM2 tree, not yet corrected:**
- docs/PicoMite_Parity_Audit_and_Plan.md §6.3: line 293 says the PicoMite's MODE 3 shows L and T (it shows L only);
  line 301 says 1024x600 has no CMM2 match (MODE 18 has it); line 534's 4-bit list leaves out mode 14.
- docs/PicoMite_CMM2_Dialect_Conflicts.md:525-543: proposes 7,12 or 7,8 for MODE 2, since replaced by 7,4, and has the
  same 1024x600 row.
- Src/ltdc.c's comments: line 320 calls mode 4 60 Hz (it is 75); line 374 gives mode 10 39.375 MHz (it is 33.75);
  line 446 copies mode 17's "75 Hz 18.9MHz" for mode 18 (it is 58.5 MHz, about 68 Hz).
- The manual: the monitor-timing list (12_video_modes.md:28) leaves out mode 18. The page table (line 77) gives the
  G1's mode 18 8-bit pages as 0-6, where the allocation gives 0-4. That is not checked on a G1.
