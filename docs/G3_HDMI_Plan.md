# Colour Maximite 2 G3: HDMI through an MS7210

*8 November 2026; full colour levels on a G3, and the H743's own options for the narrow modes (measured on a G1), added
13 November. A plan for recognising a G3 and configuring its MS7210, from the MS7210's published drivers and the
firmware as it stands. Nothing here has been tried on a G3 yet.*

## The G3, as Peter describes it

- A G2 with a MacroSilicon MS7210 that turns the LTDC's digital RGB888 into HDMI.
- The pixel clock is on PG7 (LTDC_CLK) and DE on PF10 (LTDC_DE). The other LTDC pins are as now.
- The MS7210 is on the same I2C channel as the RTC, which is how a G3 is to be recognised.
- It comes up in 800x600.

Peter's schematic (8 Nov) confirms the wiring:

| MS7210 | Wired to |
|---|---|
| D0-D7 | Blue: PE4 (B0), PG12, PD6, PG11, PI4, PI5, PI6, PI7 (B7) |
| D8-D15 | Green: PE5 (G0), PE6, PH13, PG10, PH15, PI0, PI1, PI2 (G7) |
| D16-D23 | Red: PH2 (R0), PH3, PH8, PH9, PH10, PC0, PB1, PG6 (R7) |
| PCLK | PG7 (LTDC_CLK), through 47 ohms |
| DE | PF10 (LTDC_DE) |
| HSYNC, VSYNC | PI10, PI9, each through 47 ohms |
| SDA, SCL | PD4, PD5: the RTC's bus |
| RSTN | PG3, with 27K to VCC |
| INT | PC8, with 27K to VCC |
| SA | Not connected: the address is 0xB2 |
| SCLK_MCLK, WS, I2SDATA0 | PD13, PD12, PD11 (phase 2). I2SDATA1-3 are not connected |
| HPD, DDC, CEC | The HDMI socket. DDC has 1.5K pull-ups to +5V, and CEC 27K to a supply fed through a diode |
| REXT | 5.6K to ground |

The data lines are in the chip's own order for 24-bit RGB, so no channel swap is needed.

Peter's open questions:
- Does the MS7210 need configuring again when the resolution changes?
- Will it cope with the line-duplicated modes, or do we need the PicoMite's pixel doubling as well?
- Does it need a bit clock before it will answer on I2C?

## What the sources say

The sources are MacroSilicon's datasheet and four working sets of code. MacroSilicon's own SDK is the most complete,
and the FPGA boards' init table is the closest to the G3, as those boards also feed the chip parallel RGB with HS, VS
and DE.

- **The datasheet:** MacroSilicon's, in Chinese (HJ-BMJL-PD-002, revision B/0, 22 pages), from Peter's Downloads.
  - It gives the pins, the input timing, the power-up and the address, but not the registers.
  - It is marked as MacroSilicon's property, not to be passed on, so it stays out of the repository.

- MacroSilicon's SDK (V1.0.0, 2020) inside D-Robotics' RDK X5 kernel:
  `drivers/gpu/drm/bridge/macrosilicon/ms-ms7210.c` and `ms7210/`.
- A standalone Linux driver: avmwww/ms7210-linux-driver.
- Loongson's driver, with a register header: `drivers/gpu/drm/loongson/bridge/ms7210_*` in LS2K0300-linux-4.19.
- Several FPGA boards (Pango) that drive it from RGB888 with HS, VS and DE, all with the same `ms7210_ctl.v`: a fixed
  table of 49 register writes.

### Talking to it

- **Address:** set by the SA pin (datasheet).
  - SA pulled up to DVDD, or left floating, gives 0xB2 in 8-bit form (0x59 in 7-bit form).
  - SA tied to ground gives 0x56 (0x2B).
  - The G3 has no strap, so its MS7210 is at **0xB2**.
  - The DS3231 is at 0xD0, so the two cannot clash.
- **Pins:**
  - The I2C pull-ups, and those on RSTN and INT, must go to the same supply as DVDD, the chip's I/O supply (1.8, 2.5
    or 3.3 V).
  - INT ("INT#", active low) goes to PC8 on the G3, with a 27K pull-up to VCC (Peter, 8 Nov).
- **24-bit RGB on the data pins:**
  - D0-D7 take blue (B0-B7), D8-D15 green and D16-D23 red.
  - The G3 is wired exactly so (the schematic, above), so the chip's channel and bit swaps (REG_DVIN_SWAP, 0x1201)
    are not needed.
- **Registers:** a 16-bit register number, sent **low byte first**, then 8 bits of data. A write is: start, B2, low,
  high, value, stop. A read is: start, B2, low, high, repeated start, B3, the byte with no acknowledge, stop.
- **Recognising it:**
  - Registers 0x0000, 0x0001 and 0x0002 read **A2 20 0A**.
  - The FPGA boards also write 0x5A to register 0x0003 and read it back before anything else.
  - Either test is unambiguous.
- **The RTC's bus:**
  - It is bit-banged in `Src/Analog_I2C.c`, SCL on PD5 and SDA on PD4, at a few hundred kHz, which suits the MS7210.
  - Its routines (I2C_Start, I2C_Send_Byte, I2C_Read_Byte, I2C_Stop) can be used for the MS7210 as they are.
  - A register write takes about 150 us, so the whole init is under 10 ms.

### Does it need a bit clock to answer on I2C?

Probably not, but nothing proves it.
- The chip has its own oscillator. The SDK sets "RC25M = 27M", and its pixel-clock meter counts against a 27 MHz
  crystal, MS7210_EXT_XTAL.
- Both the SDK and the FPGA boards read the chip ID or the scratch register before any video is set up.

The G3 has no crystal (Peter, 8 Nov), so the chip runs from that internal oscillator.
- The datasheet's block diagram shows it, RC25M, and lists a built-in oscillator.
- Its pinout has no crystal pins at all.

That makes it more likely still that I2C needs no pixel clock. Detection is planned to work either way (below).

### Power-up and reset (datasheet, section 12)

There are no special requirements.
- The supplies should come up within 10 ms of each other.
- Reset should come once the 1.2 V and 3.3 V supplies are stable, held low for more than 10 ms.
- The datasheet's reference circuit has a pull-up on RSTN.

### Does it need configuring again when the resolution changes?

Yes, but only a little:
- **The input timing:** with HS, VS and DE all wired, the chip takes its timing from them.
  - The FPGA table never writes the input timing registers (0x120C to 0x1223).
  - The SDK's timing regeneration is switched off in this mode: `dvin_deregen_en` is cleared for HSVSDE.
  - So the totals, porches and sync widths do not have to be told to it.
- **The HDMI PLL** runs in hardware automatic mode. Even so, the SDK reprograms the PHY and re-triggers the PLL
  (register 0x0910) whenever the pixel clock changes, then waits 10 ms.
- **The PHY's drive settings** change at a 100 MHz pixel clock:

  | Pixel clock | 0x0906 bit 2 | 0x0920 | 0x0926 | 0x0927 | 0x0928 | 0x0929 |
  |---|---|---|---|---|---|---|
  | 100 MHz or less | 0 | bit 6 clear | 0x11 | 0x01 | 0x11 | 0x01 |
  | Over 100 MHz | 1 | bit 6 set | 0xDD | 0x0D | 0x88 | 0x08 |

  The FPGA table uses the second row, as it drives 1080p. On the CMM2, modes 15 and 16 are over 100 MHz, and so is
  mode 12 if its pixels are repeated (below).
- **The AVI infoframe** (registers 0x0509 to 0x050E) carries the video format code and the pixel repetition. The FPGA
  boards send code 0, which is "not a standard format", for everything, and monitors accept that. A TV may want the
  right code for 720p (4) and 1080p (16).
- **A change of mode** therefore means:
  1. Mute (0x0507).
  2. Change the LTDC as now.
  3. Write the PHY's band, re-trigger the PLL and write the infoframe.
  4. Wait 10 to 20 ms.
  5. Unmute.

  That is about 15 register writes. Whether the PLL follows a clock change without the re-trigger is one of the
  first experiments, as the answer decides how little a mode change can get away with.

### Will it cope with the line-duplicated modes?

**The doubled lines are not a problem.**
- A doubled mode stores each line twice in memory (`copies` 2), so the LTDC sends an ordinary frame of full height.
- Mode 7, for example, goes out with 480 active lines.

**The horizontal size is the problem.**
- The narrow modes get their wide pixels by running the LTDC at a fraction of the clock. Mode 7's line is 420 clocks
  at 15.7 MHz, exactly half of VESA 640x480 at 75 Hz (840 at 31.5 MHz). That was mode 8's timing until 9 Nov; mode 8
  is now 640x480 at 60 Hz.
- A VGA monitor just sees wider pixels.
- HDMI cannot carry a pixel clock under 25 MHz, and a sink checks the timing it is given.

**The datasheet's input range is 25 to 300 MHz** (table 11.1, for SDR; the input timing figures in that table come
from simulation, not measurement). Feeding the chip the narrow modes' own clocks is therefore outside the datasheet:
- 15.7 MHz for modes 3 and 7.
- 11.8 to 23.5 MHz for modes 4, 5, 6 and 17.
- 24.75 MHz for mode 13, just under.

**The MS7210 has HDMI pixel repetition in hardware.**
- The SDK's `u8_clk_rpt`, `ms7210drv_hd_tx_shell_set_clk_repeat` (register 0x0508 bits 5-4, 2 to 10 times) and the
  doubled internal clock (0x092A bit 2) take a line of half the width at half the clock and send each pixel twice.
- For the modes whose clock is exactly half a standard one, that gives a standard HDMI signal with no change to the
  firmware's drawing.
- Whether a sink is happy depends on the AVI's repetition field: for VESA formats it should probably say "none".
  This has to be tried.
- **It is outside the datasheet's input range:** fed this way, the chip runs below the 25 MHz minimum. The SDK
  still uses this path for 480i and 576i, whose clock is 13.5 MHz, so the chip probably copes. It has to be tried,
  and the software doubling below is the fallback.

**The modes, as they would reach HDMI:**

| Mode | Picture | LTDC pixel clock | Line, frame | As it is | With the chip's x2 |
|---|---|---|---|---|---|
| 1 | 800x600 | 49.5 MHz | 1056 x 625, 75 Hz | VESA, fine | |
| 2 | 640x400 | 31.4 MHz | 840 x 500, DE 400 lines | Clock fine, but DE 400 of a 480 frame | |
| 3 | 320x200, doubled | 15.7 MHz | 420 x 500, DE 400 lines | Clock too low | 640 wide; still DE 400 |
| 4 | 480x432 | 23.5 MHz | 630 x 500, DE 432 lines | Clock too low | 960 x 432: not standard |
| 5 | 240x216, doubled | 11.8 MHz | 314 x 500, DE 432 lines | Clock too low | Still under 25 MHz |
| 6 | 256x240, doubled | 12.6 MHz | 336 x 500 | Clock too low | 672 total: not standard |
| 7 | 320x240, doubled | 15.7 MHz | 420 x 500 | Clock too low | **640x480 at 75 Hz** |
| 8 | 640x480 | 25.2 MHz | 800 x 525, 60 Hz (since 9 Nov) | VESA, fine | |
| 9 | 1024x768 | 64.7 MHz | 1344 x 806, 60 Hz | VESA, fine | |
| 10 | 848x480 | 33.7 MHz | 1088 x 517, 60 Hz | VESA, fine | |
| 11 | 1280x720 | 74.2 MHz | 1650 x 750, 60 Hz | CEA 720p, fine | |
| 12 | 960x540, doubled | 74.2 MHz | 1100 x 1125 | Not standard | **1920x1080 at 60 Hz** |
| 13 | 400x300, doubled | 24.75 MHz | 528 x 625 | Clock just too low | **800x600 at 75 Hz** |
| 14 | 960x540 | 37.8 MHz | 1100 x 569, 60 Hz | Not standard | Not standard |
| 15 | 1280x1024 | 108 MHz | 1688 x 1066, 60 Hz | VESA, fine (high band) | |
| 16 | 1920x1080 | 148.8 MHz | 2200 x 1125, 60 Hz | CEA 1080p, fine (high band) | |
| 17 | 384x240, doubled | 18.9 MHz | 504 x 500 | Clock too low | 768 x 480: not standard |
| 18 | 1024x600 | 50.5 MHz | 1344 x 628, 60 Hz | Not standard, common on monitors | |

OPTION PICOMITE's smaller modes divide the clock the same way (ModeHs in `MX_LTDC_Init`), so they are in the same
position as modes 3 and 7.

**So the modes fall into three groups:**
1. **Already standard (1, 8, 9, 10, 11, 15, 16):** nothing beyond the PHY's band and the infoframe.
2. **Exactly half a standard clock (7, 12, 13, and 3 apart from its height):** a clock at twice the pixel clock on
   PG7 (below), or the chip's x2 repetition, should make them standard. The firmware's drawing is unchanged.
3. **The rest (2, 3, 4, 5, 6, 14, 17, 18):** odd widths, odd clocks, or a DE that covers only part of the frame. For
   each, try these in order:
   - **Send it as it is.** Many monitors take odd timings over HDMI as they do over VGA, but not under 25 MHz, and
     probably not on a TV.
   - **Put it inside a standard frame,** with black borders. The LTDC's active area becomes the standard size and the
     layer's window sits in the middle; the LTDC fills the rest with its background colour. Combined with the x2
     repetition, mode 6 would show as 512x480 inside 640x480, and mode 5 as 480x432 inside it. That changes how big
     the picture looks compared with VGA, which stretched it to fill the screen.
   - **Double the pixels in software, as the PicoMite does.**
     - It is the last resort, as it costs the most: a full-width page made from the narrow one at every frame
       (measured below).
     - The firmware already has something of the kind in the merge of the 4-bit and 1-bit modes (Rgb121.c).

As Peter says, which group-3 approach each mode gets is a matter of experiment on real monitors and a TV.

### What the H743 can do itself (measured on a G1, 13 Nov)

The PicoMite makes its narrow modes in its line interrupt: it steps the frame pointer every second line and doubles
the pixels in software. The question was what the LTDC offers for the same.

**The LTDC has no scaling of any kind.**
- Its registers (`stm32h743xx.h`) are:
  - The timings.
  - Two layers, each with a window, a pitch and a line count.
  - Eight pixel formats, none under 8 bits a pixel: ARGB8888, RGB888, RGB565, ARGB1555, ARGB4444, L8, AL44 and AL88.
  - A CLUT, colour keying, blending and dithering.
  - A line interrupt, and shadow registers reloaded at the vertical blanking or at once.
  - The current position (CPSR) and status (CDSR).
- There is no pixel repetition, line repetition or scaler, and no format that turns one byte into two pixels.
- So the horizontal doubling cannot come from the LTDC itself.

**How it was measured.** `tools/sdk/examples/ltdctest.c` runs in MODE 7,8 on a G1, without a monitor.
- It reads the colour pins' GPIO input registers beside LTDC_CPSR, so it knows the colour of every line the LTDC
  sends.
- Each line of a test buffer is one colour, so a line's source is known from its colour.
- The same program reads SAI1's clock on PG7, and times the DMA2D and the CPU.

**What the LTDC does, as measured:**
- **A pitch of 0 repeats one line** for the whole frame, with no FIFO underrun or transfer error. (The flags were
  read inside the frame, as the firmware's LTDC handler clears them.) The manual sets no minimum on the pitch: it is
  "the increment from the start of one line of pixels to the start of the next".
- **After CFBLNR lines the LTDC fetches no more and shows the last line again** for the rest of the window. CFBLNR 1
  repeats line 0, and 240 in a 480-line window gives lines 0-239 and then line 239 again. The manual says too few
  lines gives a FIFO underrun, but none was flagged.
- **Each line's address is CFBAR + n x pitch**, where n is the line within the layer. A new CFBAR reloaded at once
  (SRCR IMR) mid-frame is used from the next line the LTDC fetches.
- **The LTDC fetches line n + 1 during line n.** This is with 8-bit pixels, 320 bytes a line, from the SDRAM. Of mode
  7's 420 clocks a line:
  - A write between about clocks 220 and 310 of line n lands cleanly on line n + 2.
  - An earlier write splits line n + 1.
  - A later one splits line n + 2.
- **Lines doubled from a page that holds each line once works.**
  - Pitch 0, with CFBAR set once every two lines in that window, gave 480 lines, each source line exactly twice, and
    no line split.
  - So the PicoMite's way of stepping the pointer every second line works on the LTDC.
  - **The line interrupt comes at the wrong moment.** Its flag is set at clock 0-15 of the line, during HSYNC, every
    time (LIPCR counts in CPSR's lines). That is while the next line is being fetched, so a write from it splits
    that line.
  - The line interrupt can also trigger the MDMA (`ltdc_li_it`, MDMA request 16), so the write would need no CPU, but
    it would still come at the wrong moment.
  - So each pair of lines would need a second, timed write: a busy wait of half a line, or a timer or DMA locked to
    the frame. The window also depends on the depth and on where the page is.
- **HDMI does not need this.** The doubled modes already store each line twice (`copies` 2), so the LTDC sends
  full-height frames. Single-copy pages would halve their memory and drawing time, which is a project of its own.

**Pixel doubling, the options measured:**
1. **A sampling clock at twice the pixel clock, on PG7. Recommended, and new.**
   - **The idea:**
     - PG7 has SAI1_MCLK_A on AF6 as well as LTDC_CLK on AF14 (the Linux kernel's H743 pin table, and the
       datasheet's pin list).
     - SAI1's kernel clock can be PLL3's P output, and PLL3's P divider can be set to half of R. The LTDC's pixel
       clock is R.
     - So PG7 can carry a clock at exactly twice the LTDC's pixel clock, from the same VCO, and the MS7210 samples
       each LTDC pixel twice.
     - DE, HSYNC and VSYNC come from the LTDC, so they are twice as long in the MS7210's clocks. Mode 7's 420 x 500 at
       15.7 MHz arrives as 840 x 500 at 31.4 MHz, which is VESA 640x480 at 75 Hz.
     - It costs no CPU and no memory, works in every colour depth, and leaves the firmware's drawing as it is. The
       chip's input clock stays inside its 25-300 MHz range.
     - It leaves the HDMI sound alone. That is SAI2, on its own clock selector from PLL1Q (G3_HDMI_Audio_Plan.md,
       "Beside the video's SAI1"). SAI1 is then not available for sound.
   - **Measured on the G1:**
     - **MCLK runs only while the SAI block is enabled.**
       - The H743 has no MCKEN. RM0433 never mentions it; the CMSIS header defines bit 27 for later H7 parts, and on
         this chip it reads back 0.
       - The clock generator runs with the block a master and NOMCK (bit 19) 0, which the header calls NODIV.
       - Block A as a master transmitter with nothing to send gives a steady MCLK, and its FIFO underruns harmlessly.
       - PG7 is the only SAI1 pin given a function.
     - **PLL3's P output is off** (PLLCFGR DIVP3EN 0) as the firmware leaves it, and it can be switched on only with
       PLL3 stopped, so PLL3_Config in setmode would do it.
     - **MCLK is pll3_p / MCKDIV, and both 0 and 1 mean undivided.** With an 8 MHz kernel clock it counted 7.9995 MHz
       at MCKDIV 0 and 1, 2.6665 at 3, and 0.1270 at 63. The manual says the same ("MCKDIV = 0 gives the same result
       as MCKDIV = 1"), and that an odd MCKDIV makes MCLK's duty cycle other than 50%.
     - **It is phase-locked to the pixel clock.** With MCLK's period set to 21 pixels, its rising edges fell at the
       same place in every frame for 3 million samples. That held with P = R, and with P = R/2 (32.0 and 16.0 MHz).
   - **Not measurable on a G1:**
     - Where MCLK's edges fall against the LTDC's data. The G3 sets the MS7210's sampling edge with register 0x1281,
       as in step 2.
     - Whether that phase is the same after every restart of PLL3, which is every mode change.
   - **Limits:**
     - **SAI1's kernel clock may run at up to 150 MHz**, the same as the LTDC's (RM0433 table 60, VOS0 and VOS1). So
       every mode's doubled clock is allowed, mode 12's 148.5 MHz included.
     - The datasheet characterises MCLK only as an audio clock, up to 256 x Fs (49.152 MHz at 192 kHz). Modes 3 and 7
       need 31.4 MHz, mode 4 47.1 and mode 13 49.5; mode 12's 148.5 is beyond what is characterised, though not beyond
       what is allowed. PG7 carries 148.8 MHz as LTDC_CLK in mode 16 anyway.
     - **The PLL's dividers must be even for a duty cycle close to 50%** (table 60's note: "(DIV[P/Q/R] + 1) shall be
       even", the register value plus 1). The MS7210 asks for 40-60%. P passes through the SAI undivided, so:
       - Modes 3 and 7: VCO 816, R 52, 15.69 MHz (-0.37%), P 26 (31.385 MHz, the old mode 8's).
       - Mode 13: R 32 and P 16 (49.5 MHz) as they are.
       - Mode 5's 11.8 MHz is still under 25 MHz when doubled, so it needs P = R/4 with P even: VCO 664, R 56
         (11.857 MHz, +0.38%), P 14 (47.4 MHz).
       - Mode 12 on PG7: VCO 592, R 8 (74.0 MHz, -0.34%), P 4 (148.0 MHz).
2. **The MS7210's own repetition** (above): the chip is fed the narrow clock, under its 25 MHz minimum for every mode
   but 12. For mode 12 it is inside every specification, so it is tried there beside the clock on PG7. It is also
   the alternative if the clock on PG7 will not sample cleanly.
3. **The DMA2D, for the 8-bit modes only.**
   - The trick: the DMA2D converts L8 to RGB565 through a CLUT whose entry i is the RGB565 colour whose two bytes are
     both i, so each byte comes out twice.
   - Two passes, each writing every other line, make a 640x480 L8 display buffer from the 320x240 page.
   - Measured: 1.6 ms a frame, bit for bit right, with no CPU. The DMA2D's conversion to RGB565 truncates: the manual
     does not say so, but the result is exact.
   - The cost is a 300 KB buffer, the SDRAM's bandwidth, and the LTDC at the full clock. It is like the Rgb121.c
     merge.
4. **The CPU**, a frame at a time, from the SDRAM to the SDRAM:
   - 3.3 ms for an 8-bit page and 6.4 ms for a 16-bit one. That is 25% and 48% of the CPU at 75 Hz.
   - The DMA2D cannot double 16-bit pixels usefully: with lines of one pixel it took 68 ms a frame.
5. **The firmware drawing a full 640x480 page** (Peter's alternative):
   - Every routine writes four pixels for one, and pages double in size, so there are half as many.
   - PEEK and POKE of the page's memory change, and so does the loadable programs' layout (`copies` 2).
   - It is the most invasive, and is needed only if 1 and 2 both fail.

**So, in step 4, try the clock on PG7 first.** Try the chip's repetition beside it for mode 12, and keep it as the
alternative. The DMA2D is the fallback for the 8-bit modes.

*The behaviour above was measured on a G1 with RM0433 Rev 9 (October 2026) beside it. Where the two differ (the
line count), the measurement is given.*

## What changes on a G3's LTDC

| Setting | Today (VGA) | G3 (HDMI) |
|---|---|---|
| PG7, LTDC_CLK | Driven only in mode 18 (kept off otherwise, as noise beside the video lines) | Always driven: LTDC_CLK (AF14), or SAI1_MCLK_A (AF6) at twice the pixel clock in the halved modes (above) |
| PF10, LTDC_DE | Mapped | Always driven |
| DE polarity | Active low (LTDC_DEPOLARITY_AL) | Active high: the SDK declares DRM_BUS_FLAG_DE_HIGH |
| Clock edge | LTDC_PCPOLARITY_IPC | To be found. The SDK samples on the falling edge. Register 0x1281 (REG_PA_S) inverts the clock (bit 2) and delays it by 0 to 3 steps (bits 1-0); the FPGA boards write 0x04, inverted with no delay, which is the place to start. The datasheet asks for 0.879 ns of setup and 0.22 ns of hold, and a clock duty of 40-60%. Tuned on the board at the highest clock, 148.5 MHz, where a pixel is 6.7 ns |
| Pin speed | Colours MEDIUM and syncs LOW, slowed on 23 Oct for the G1's VGA | HIGH or VERY_HIGH for the clock and data, to meet 148.5 MHz |
| Clock duty | Any: VGA has no use for the clock | 40-60% (MS7210). An odd PLL3 divider is not 50% (RM0433 table 60). Mode 16's R 5 is about 40/60 at 148.8 MHz, at the limit, so on a G3 use VCO 592 with R 4 (148.0 MHz, -0.34%). Modes 3, 4, 7, 8, 9, 11 and 12 have odd dividers of 11 or more (45/55 or closer) |
| PE4, LTDC_B0 | Left unconfigured by a second line in HAL_LTDC_MspInit, a bug as the G2 wires it (Peter, 8 Nov); fixed on 8 Nov for a G2 | Driven (AF14), as on a G2: the G3 wires it to the MS7210 |
| Option.offsets (OPTION VGA's centring) | Moves the picture | Should be 0: DE places the picture, and a shift only moves DE within the line |
| Colour levels (OPTION VGA OUTPUT) | HIGH (the default) and MEDIUM top each colour at 0xE0, LOW at 0xC0 | Full, 0xFF, whatever the option says (below) |

### Full colour levels on a G3

OPTION VGA OUTPUT never lets the outputs reach full level. HIGH and MEDIUM top each colour at 0xE0, and LOW at 0xC0.
- The reason is the resistor DAC. Its current into the monitor's 75 ohm loads grows with the brightness, and a screen
  of white moved the supply enough to jitter the picture (Peter, 23 Oct; the comment above VgaLevel in ltdc.c).
- On a G3 the LTDC's pins drive the MS7210's digital inputs, which draw no current that depends on the colour.
- **So a G3 drives every colour to its full level, whatever OPTION VGA OUTPUT says.**
- This assumes the G3 has no VGA resistors left on the same lines. If it keeps them for a VGA socket, the jitter
  could come back on that socket.

**One switch, at the root:**
- A fourth map, FULL (3), beside LOW (0), MEDIUM (1) and HIGH (2).
- One function in ltdc.c, `VgaMap()`, gives the map in use: FULL when G3Hardware is set, otherwise Option.colourmap.
- Every place that reads Option.colourmap for the levels calls it instead. These are all of them:

| Where | What it sets | With FULL |
|---|---|---|
| reset_CLUT and setmode (ltdc.c) | The 8-bit CLUT | The RGB332 levels: red and green 00 24 49 6D 92 B6 DB FF, blue 00 55 AA FF. Filled by a loop, not a fourth 1 KB table, as flash is tight. The two copies of the table choice become one routine |
| VgaLevel (ltdc.c) | The 4-bit modes' 16 colours (Rgb121Clut, MAP) and the 1-bit tiles' (MonoClut) | The colour unchanged. Its cache (VgaCurveFor) is keyed on VgaMap() |
| VgaAlpha (ltdc.c) | The 16 and 32-bit layers' constant alpha | 255 |
| InitDisplayOther (OtherDisplays.c) | The BIT and RBIT tables of the 16 and 12-bit depths, and through them the sprites' (SpriteLevels) | HIGH's, which are already 1:1 |
| ReadBuffer8 (OtherDisplays.c) | The divisor that scales a CLUT colour back for PIXEL(, SAVE IMAGE and Print Screen | 255 |

The 12-bit depth needs nothing: it never had the option's alpha, and HIGH's BIT_4 is already 1:1.

The loadable programs need nothing either. Their palettes (the video_palette service) go into the CLUT as given, and
their 16 and 32-bit modes get the layer's alpha from setmode.

**When it takes effect:**
- G3Hardware is set by the probe, after the first setmode (below).
- The probe goes in InitDisplayOther, between its setmode and its choice of the BIT tables, so the tables come out
  right.
- If it finds a G3, setmode runs once more, forced, to load the full CLUT and the layers' alpha.
- Nothing has reached the HDMI monitor by then, as the MS7210 is set up only after the probe.

**What a program sees:**
- Drawing, MAP and COLOUR MAP are unchanged, and so is PIXEL( in the 4, 1, 16 and 32-bit depths: those hold or
  return the colours as the program gave them.
- PIXEL( in an 8-bit mode returns the CLUT's colour, scaled back. Under HIGH that is the table's own, so white reads
  &HE0E0E0.
- On a G3 white reads &HFFFFFF, as LOW and MEDIUM already give it. The mid levels move a little: &H68 to &H6D, &HB0
  to &HB6, &HD0 to &HDB.
- SAVE IMAGE and Print Screen of an 8-bit screen get the brighter colours, which is what the monitor shows.
- Recommended: leave it so, as these are the colours on the screen. A golden that reads 8-bit pixels back then needs a
  G3 result of its own.
- The other way, ReadBuffer8 giving HIGH's levels on a G3, keeps every program's results the same as a G2's but
  reports colours the screen does not show. Peter's choice.

**OPTION VGA OUTPUT on a G3:**
- Still accepted and saved, so a set of options moved to a G2 (OPTION DISK SAVE and LOAD) keeps its meaning.
- It has no effect on the G3 itself.

**On a TV: the quantisation range.**
- HDMI carries RGB as full range (0 to 255) or limited range (16 to 235).
- A sink assumes full range for the IT formats: VESA, and video code 0, which the FPGA boards send.
- It assumes limited range for the CE formats, 720p and 1080p among them, unless the AVI infoframe's Q field (data
  byte 3, bits 3-2) says full.
- Taken as limited, the full levels are stretched: in the 16 and 32-bit depths everything under 16 shows as black and
  everything over 235 as white, and the 8-bit depth's colours look harsher.
- So when step 3 gives 720p and 1080p their codes (4 and 16), the infoframe is to say full range as well. Where the
  MS7210 keeps that field is to be found in the SDK's AVI code.
- Only a TV whose EDID says so obeys Q. Others have a menu setting, such as "HDMI black level" or "RGB range".

## Recognising a G3

`getVersion()` in main.c tells a G1 from a G2 by PF9 (low on a G2). A G3 is a G2 that has the MS7210, so:

0. **Release the reset early.** The MS7210's reset is on PG3 (Peter, 8 Nov), and a chip held in reset will not answer.
   - When G1Hardware is 0, drive PG3 **low** in MX_GPIO_Init, early in the start-up, then **high** later, at least
     10 ms on (the datasheet's reset), before the first setmode. That gives the chip a proper reset without slowing
     the start-up, and it is held low for its 10 ms while the H743 sets up its clocks and SDRAM. By the time of the
     probe, it has been out of reset for far longer than the SDK's 50 ms.
   - This means PG3 is driven on every G2 too, before anything knows it is a G3. The firmware has no use for PG3:
     it is not in the I/O connector's pin table (IOPorts.h). The comment "PG3 COUNT4" on the EXTI3 handler in
     stm32h7xx_it.c is stale, as COUNT4 is PC4.
   - PG3 is not connected on a G2 (Peter, 8 Nov), so driving it there is harmless.
   - The SDK's longer reset (100 ms low, then 50 ms) is kept in reserve, for a G3 whose chip misbehaves after the
     short one.
1. Only when G1Hardware is 0, after the first setmode at start-up, read registers 0x0000 to 0x0002 at 0xB2, and at
   0x56 if nothing answers.
2. If nothing answers, drive PG7 as LTDC_CLK (the 800x600 clock is already running), wait 10 ms and read again. That
   covers a chip that needs a clock. On a G2 the clock would then run for about 10 ms, once, before going back to
   analog.
3. Found: set a new flag, G3Hardware, and run the MS7210's init for the mode in use. Not found: carry on as a G2.

Things to report it, to be agreed:
- MM.DEVICE$ ("Colour Maximite 2 G3"), the start-up banner and OPTION LIST.
- VGA-only options noted as having no effect on a G3: OPTION VGA OUTPUT, as a G3 always drives full levels (above),
  and the offsets.

## The plan

### Step 0: the board (Peter's answers, 8 Nov)

1. **Reset: on PG3, with a 27K pull-up to VCC,** as in the datasheet's reference circuit. PG3 is not connected on a
   G2.
   - The chip comes out of reset by itself at power-up, and runs until the firmware pulls PG3 low.
   - The start-up's low-then-high then gives it a proper reset once the supplies are stable, as the datasheet asks.
     It also does so after every reset of the H743 alone (a flash, Ctrl-Alt-Del or a watchdog), which leaves the
     chip powered and as it was.
   - Driving PG3 low takes 0.12 mA through the pull-up, so push-pull and open-drain both do.
   - The pull-up goes to VCC, so DVDD must be the same 3.3 V, as the datasheet requires of RSTN's pull-up.
2. **Crystal: none.** The chip runs from its internal oscillator, which the datasheet shows as RC25M.
   - The SDK sets it to 27 MHz (register 0x0060 = 0x81, 0x0061 = 0x54), so the init does the same.
   - The pixel-clock meter (0x001E) is scaled to a 27 MHz crystal, so its readings are only approximate here. That
     is enough for diagnostics.
3. **Address strap: none,** so SA floats and the chip is at **0xB2** (datasheet). The probe still tries 0x56 after
   it, in case a later board ties SA to ground.
4. **B0 (PE4): wired to the MS7210.** A G3 drives PE4 as LTDC_B0 (AF14). Nothing else in the firmware uses PE4, and
   it is not on the I/O connector.
5. **HPD and CEC: to the HDMI socket,** and CEC is not available to the firmware. A hot plug reaches the H743
   through the chip's INT (item 7), or by reading HPD over I2C (0x0502 bit 0).
6. **I2S: wired to PD11, PD12 and PD13.** HDMI audio is a later project. Nothing in the firmware uses those pins or
   SAI2. On the H743 they should be SAI2's block A (data, frame and bit clock, AF10), to be checked against the
   datasheet then. There is no MCLK line, so the MS7210 would have to make its audio clock from the bit clock: its
   audio PLL (0x0400 on) is set up in the SDK.
7. **INT: wired to PC8, with a 27K pull-up to VCC.**
   - PC8 is free: it is not on the I/O connector, and nothing in the firmware uses it. Its EXTI line is free too:
     EXTI9_5_IRQHandler serves only pin 7.
   - The datasheet calls it "INT#", so it is active low.
   - Which events pull it low is not documented. The SDK names the HDMI shell's interrupt register (0x0500) and
     its mask (0x0501) but never uses them, and the FPGA table's "INT EN" comment sits on a PHY register (0x0920).
     So this is found in step 1.
   - Read PC8 by polling, not by interrupt. The chip's status is read over the bit-banged I2C, which takes
     milliseconds and is shared with the RTC, so that read belongs in the foreground, not in an interrupt handler.

### Step 1: experiment from a loadable program, without reflashing

A test program (`apps/ms7210test`) can do all the bring-up while the firmware stays as it is:
- It bit-bangs PD4/PD5 itself, as Analog_I2C.c does.
- It sets PG7 and PF10's alternate functions, and LTDC's DE polarity and clock edge, by writing the registers
  directly.
- It changes modes through the video_mode service.

Each experiment is then a quick rebuild and copy, not a firmware flash, and the findings go into the firmware once.
`tools/sdk/examples/ltdctest.c` already drives the LTDC, PLL3, SAI1, PG7 and the DMA2D this way (above), so the test
program can start from it. It would:
1. Read the ID bytes with and without the clock on PG7. This answers the clock question.
2. Write the FPGA table, with the PHY's low band for 800x600, and turn the picture on.
3. Dump the status registers:
   - HPD (0x0502)
   - PLL lock (0x091C bit 7)
   - The measured pixel clock (0x001E)
   - "Timing stable" (0x051D bit 4)
4. Find what drives INT on PC8:
   1. Set bits in the interrupt mask (0x0501).
   2. Plug and unplug the cable.
   3. Watch PC8 and the interrupt register (0x0500).
   4. Find how a write to 0x0500 clears it.
5. Show colour bars at full level, through the video_palette service, which puts colours into the CLUT as given:
   - 24 bars, each with one of the 24 data bits set. That checks every data line's wiring.
   - Full white, &HFFFFFF, beside the firmware's &HE0E0E0. That shows the full levels before the firmware has them.

**Done when:** there's a stable 800x600 picture on Peter's monitor, and we know whether the chip needs a clock.

### Step 2: the clock edge, at the top speed

1. In MODE 16 (148.8 MHz), step the LTDC's clock edge and register 0x1281 until a test pattern of fine detail shows
   no sparkles.
2. Take the middle of the good range, then check it at MODE 9.

**Done when:** every standard mode in group 1 is clean.

### Step 3: what a mode change needs

1. Try a mode change with nothing written to the MS7210.
2. Then with only the PLL re-trigger.
3. Then with the full sequence: band, re-trigger and infoframe.
4. Keep the least that is reliable across 20 changes each way, between the high and low bands as well.
5. On the TV, in 720p and 1080p, check the darkest and brightest greys of a 32-bit ramp. If they are lost in black
   and white, the infoframe's quantisation range needs setting to full (above).

**Done when:** group 1 works on a monitor, and 720p and 1080p on a TV with black and white where they should be.

### Step 4: the halved modes

1. **Modes 7 and 13 with the clock on PG7 at twice the pixel clock** (SAI1_MCLK_A, measured on the G1 above):
   - PLL3's P output on, with P = R/2. Mode 7 takes VCO 816, R 52 and P 26.
   - SAI1 from pll3_p, block A enabled as a master transmitter with MCKDIV 0, and PG7 on AF6.
   - Step the MS7210's sampling edge (0x1281) as in step 2. Each LTDC pixel is two of its clocks, so one edge should
     be clean.
   - Check that the edge stays right over 20 mode changes, as PLL3 restarts each time.
   - Then OPTION PICOMITE's halved modes, mode 3, and mode 5 with P = R/4.
2. **Mode 12, and anything the clock on PG7 will not do, with the chip's x2 repetition.** Try the AVI's repetition
   field both set and clear.
   - Mode 12 feeds the chip 74.25 MHz, within its range. Doubled on PG7 it needs 148.0 MHz from the SAI (VCO 592,
     R 8, P 4). That is within SAI1's 150 MHz kernel clock, but beyond the 49.152 MHz the datasheet characterises
     for MCLK, so try both.
   - Fed this way, modes 7 and 13 give the chip 15.7 and 24.75 MHz, below the datasheet's 25 MHz. Watch them for a
     picture that breaks up or drops out, especially on a cold board.
3. Then mode 3, with DE widened to 480 lines and the picture in the middle.

**Done when:** they show as 640x480, 800x600 and 1080p on a monitor and a TV. If neither the clock on PG7 nor the
chip's repetition will do it, the DMA2D doubling is the fallback for the 8-bit modes, and the rest is decided mode by
mode.

### Step 5: group 3

1. Each of modes 2, 3, 4, 5, 6, 14, 17 and 18 as it is, then in a standard frame with borders.
2. Peter judges the result on his monitor and a TV.

**Done when:** each mode has a decision.

### Step 6: into the firmware

- **`Src/MS7210.c`:**
  - The register access over Analog_I2C.c.
  - The detection.
  - The init.
  - The mode hook, called from setmode after MX_LTDC_Init.
  - A hot-plug check, and the init run again when a cable is plugged in:
    - PC8 read in the main loop, which is one GPIO read. The chip's status is read over I2C only when INT is low.
    - If INT does not report a plug (step 1), HPD is polled over I2C about every 500 ms instead.
- **The G3's settings in ltdc.c** (from the table above), under G3Hardware.
- **The full colour levels:** VgaMap(), the FULL CLUT and the places in its table (above).
- **Regression:** the G2 and the G1 must be unchanged. The goldens run on both, and the existing modes are checked by
  eye on the G2's VGA, under OPTION VGA OUTPUT LOW, MEDIUM and HIGH. On those boards VgaMap() returns Option.colourmap,
  so nothing should change.
- **The manual:**
  - The G3 in the hardware chapter, and which modes look different on HDMI.
  - OPTION VGA OUTPUT: no effect on a G3, which always drives full levels.

### Later

- **HDMI audio** through the MS7210's I2S input, which the G3 wires to PD11, PD12 and PD13 (SAI2 block A, see
  step 0). The SDK sets the audio in the same output configuration.
- **The monitor's EDID**, read through the MS7210's DDC (the SDK's `ms7210_hdtx_edid_get`), could choose or check a
  mode.
- **HDCP:** not needed.

## Risks

- **TVs:** HDMI requires a TV to accept only 640x480 at 60 Hz and the CEA formats. The CMM2's 75 Hz VESA modes,
  including the default 800x600 at 75 Hz, will suit monitors but may not suit TVs. If TVs matter, a G3 could start in
  a 60 Hz mode such as 800x600 at 40 MHz. That is Peter's decision.
- **148.5 MHz** on the board's traces and the H743's pins, with the edge tuning of step 2. If 1080p will not come
  clean, mode 16 may have to be refused on a G3.
- **No datasheet:** the register meanings come from MacroSilicon's SDK and the FPGA table. Anything they do not use,
  we cannot rely on.
