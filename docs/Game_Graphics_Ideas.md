# Game graphics for BASIC: ideas from the Amiga emulator

*10 November 2026. A note for later, not a plan yet: what the Amiga's chips do that MMBasic on the Colour Maximite 2
could offer BASIC games. Peter: "The amiga code does some very clever stuff in the blitter and other areas. Is there
anything that could be useful in Basic to expand the game programming capabilities there?" Nothing here has been
tried. It waits until the G3 work is done (the last section).*

## What BASIC has already

Several of the Amiga's tricks have a CMM2 equivalent already:

- **The blitter's cookie-cut and logic:** BLIT, with a transparent colour (BLIT FRAMEBUFFER, BLIT FLASH, BLIT MEMORY),
  and BOX AND_PIXELS and PAGE AND_PIXELS, OR_PIXELS and XOR_PIXELS.
- **Sprites:** SPRITE, 64 software sprites with collisions and SPRITE INTERRUPT (docs/Sprite_Plan.md for its refactor).
- **Two playfields:** two display layers (PAGE DISPLAY n, layer; FRAMEBUFFER LAYER), but they cannot scroll apart.
- **Tiles:** TILEMAP, the PicoMite's tile map engine (Src/TileMap.c, ported for Picanoid and Exile). Its tilesets
  are image slots in RGB121, 16 colours, and the processor draws them (ImageSlotBlit).
- **Paula's job:** PLAY MODFILE, PLAY SOUND and PLAY EFFECT.
- **Timing:** MODE's frame interrupt, and GETSCANLINE.

The gap is that everything that moves the picture copies pixels: PAGE SCROLL and PAGE COPY move whole pages, and
FRAMEBUFFER WINDOW copies a window. The Amiga mostly moves where the display reads from instead, and copies only
what is new.

## 1. Scrolling by moving the display's window (first choice)

**The Amiga:** the bitplane pointers and modulo let the display start reading anywhere in a bitmap larger than the
screen. A game scrolls a map by changing the pointers in the vertical blank (and BPLCON1 for the last pixel).

**The CMM2:** the LTDC can do the same. Set a layer's start address (LxCFBAR) and its line length in memory (the
pitch, LxCFBLR) to show a screen-sized window into a page wider and taller than the screen; PAGE RESIZE makes such
pages. Moving the window costs a register write in the vertical blank, where PAGE DISPLAY already changes pages
(Src/ltdc.c). The LTDC reads only the window, so the SDRAM bandwidth is unchanged.

**For BASIC:** for example PAGE DISPLAY n, x, y (or a new PAGE VIEW). Each of the LTDC's two layers has its own
window, so a background and a foreground can scroll at different speeds: the Amiga's dual playfield parallax.

**To settle:**
- the alignment of the start address at each depth (RGB121 holds two pixels a byte: even x only);
- the modes whose lines are drawn twice (ModeRowScale), and the PicoMite modes over them (Rgb121.c);
- what MM.HRES, MM.VRES, GETSCANLINE and the frame interrupt report with a window;
- drawing commands still address the whole page, as now.

**Effort:** low. **Risk:** low.

## 2. A copper list: changes at chosen lines

**The Amiga:** the copper writes chip registers at chosen beam positions. That gives sky gradients and raster bars,
more colours on the screen than the mode has, a status panel with its own palette and scroll, and split screens.

**The CMM2:** a list of (line, change) entries that the firmware applies from the LTDC's line interrupt (LTDC_LIPCR),
moved on to the next entry's line each time, every frame. The changes could be:
- a palette entry in the 8-bit modes (the layer's CLUT, LxCLUTWR);
- a layer's window position (from 1), for a fixed status bar or parallax bands;
- the background colour (BCCR), behind the transparent pixels.

BASIC cannot run its own code at each line (far too slow), so the list is data that BASIC sets, for example
COPPER line, PALETTE n, rgb, COPPER line, VIEW layer, x, y, and COPPER CLEAR.

**To settle, by experiment first:**
- whether the CLUT and the window can change mid-frame without a glitch: the shadow registers' immediate reload (SRCR
  IMR) part-way down the screen, and RM0433's rules for writing the CLUT;
- the interrupt's cost (about a microsecond a change) and its jitter against the USB and sound interrupts;
- the firmware's own frame interrupt also uses LIPCR (ltdc.c), so the two must share it.

**Effort:** medium. **Risk:** unknown until the experiment.

## 3. TILEMAP made faster and fuller

**The Amiga:** games draw their worlds from tiles with the blitter, and as the screen scrolls, they draw only the
column or row of tiles that has just come into view.

**The CMM2:** TILEMAP is there. It could gain:
- tilesets at the screen's own depth (8 and 16-bit), not only RGB121's 16 colours;
- tiles drawn by the DMA2D, which works beside the processor (as dma2d_copy does for loadable programs, and for the
  Amiga app's rows);
- with 1, a draw of just the edge column into the unseen part of a wide page, which gives an endless scrolling map
  with BASIC doing only the game.

**Effort:** medium. It builds on 1.

## 4. Translucent blits

The DMA2D blends a foreground over a background, with a constant or a per-pixel alpha, in hardware. A BLIT option
with an alpha would give shadows, fades and see-through effects in the 16 and 32-bit depths. The 12-bit depth's top
layer has a per-pixel alpha already (ARGB4444).

**Effort:** low to medium.

## 5. MMBasic's own sound by DMA

In 7.00.00RC5 the loadable programs' sound stream went to the DACs by DMA (DACDMAStart in Src/Audio.c, DMA1 stream 0,
triggered by TIM4): in the Amiga emulator the sound's cost fell from 0.75 ms to 0.15 ms in each 20 ms frame. MMBasic's
own PLAY commands still take an interrupt for every sample (TIM4_IRQHandler, then Audio_Interrupt), several percent
of the processor at 44.1 kHz while music plays. Filling a DMA buffer instead would give that time back to BASIC games.
It belongs with the G3's I2S audio below, which will need DMA buffers anyway, and whose plan sends the sound to the
DACs and to HDMI together (docs/G3_HDMI_Audio_Plan.md).

**Effort:** medium. **Risk:** PLAY's many sources (tones, WAV, FLAC, MP3, MOD, sound effects, the FM synthesiser)
must all fill the buffer.

## Order, and where it waits

The order: 1 first, as a prototype on the G2 with a BASIC demo of a large page scrolling under a parallax layer, for
Peter to judge; then 3; then 2, after its experiment; then 4. Item 5 goes with the G3's audio.

All of it is V8 work, on v8-dev: V7 is closed. These are new CMM2 commands, so none changes how a PicoMite program
runs. Their names are to be agreed with Peter, and with the PicoMite's where it has an equivalent.

**The next tasks come first** (Peter, 10 November 2026). When the G3 boards arrive, in about five days (around 15
November):

- the G3's HDMI output (docs/G3_HDMI_Plan.md);
- its I2S audio over HDMI (docs/G3_HDMI_Audio_Plan.md);
- the web support, with Peter's Wi-Fi module (docs/Web_Plan.md).

This note waits until those are done.
