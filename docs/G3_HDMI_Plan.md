# Colour Maximite 2 G3: HDMI through an MS7210

*8 November 2026; full colour levels on a G3, and the H743's own options for the narrow modes (measured on a G1), added
13 November; checked against v8-dev (a78205c) on 14 November, the day the first G3 arrived (the section "Checked
before the start"). A plan for recognising a G3 and configuring its MS7210, from the MS7210's published drivers and the
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
  - Its I2C_Start, I2C_Send_Byte, I2C_Read_Byte and I2C_Stop can be used for the MS7210 as they are.
  - **Its I2C_Wait_Ack cannot.** With no acknowledge it waits 250 ms (250 times MM_Delay(1)) before giving up, and
    DS3231_RD_Reg and DS3231_WR_Reg ignore what it returns and send the rest anyway. A probe on a G2, where nothing
    answers at 0xB2, would cost 250 ms an address: up to a second at every start-up with the 0x56 and clock retries
    below. So MS7210.c has its own acknowledge check, which waits a few microseconds and stops the transfer at the
    first NACK.
  - Both lines are push-pull outputs (DEV_Set_GPIOMode, with the pull-up on). I2C_Send_Byte drives SDA high after its
    eighth bit, before I2C_Wait_Ack turns it into an input, so for a microsecond or two the H743 drives SDA high against
    the slave's acknowledge. The DS3231 has lived with that. If the MS7210's transfers prove unreliable, its routines
    can drive SDA open-drain (GPIO_MODE_OUTPUT_OD) instead.
  - The bus is used only in the foreground: copy_clock (OPTION DS3231 ON) at start-up and hourly from routinechecks.
    The MS7210's accesses must stay in the foreground too.
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
| 19 | 640x512, doubled | 54 MHz | 844 x 1066 | Not standard | **1280x1024 at 60 Hz** |

Mode 19 was added on 9 November for the Amiga (half of mode 15's clock, each row twice), unannounced, and was to be
removed before 7.00.00; it is still in v8-dev. Its 54 MHz is inside the MS7210's range, so the chip's x2 is inside every
specification, as for mode 12.

Two loadable programs set a timing of their own over MODE 1's page: `amiga.app` (by default) and `bbc.app` show CEA 576p,
720x576 at 50 Hz with a 27 MHz clock, by writing PLL3 and the LTDC's timing registers directly. That is a standard
format (video code 17), fine for HDMI. They change only the sync polarity bits of GCR and put back the whole of GCR at
the end, so a G3's DE polarity survives them. But they change the clock behind setmode's back (step 3).

OPTION PICOMITE's smaller modes divide the clock the same way (ModeHs in `MX_LTDC_Init`), so they are in the same
position as modes 3 and 7.

**So the modes fall into three groups:**
1. **Already standard (1, 8, 9, 10, 11, 15, 16):** nothing beyond the PHY's band and the infoframe.
2. **Exactly half a standard clock (7, 12, 13, 19, and 3 apart from its height):** a clock at twice the pixel clock on
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
       - Mode 19 on PG7: R 12 and P 6 (108 MHz) as they are.
     - **PLL3_Config never turns P on.** setmode calls it with `DIVIDER_P_UPDATE|DIVIDER_Q_UPDATE|DIVIDER_R_UPDATE`,
       which is 0|1|2 = 3, and that falls to the branch that enables only R. A G3 sets DIVP3EN there, with PLL3 still
       off. Under OPTION PICOMITE setmode multiplies P, Q and R by ModeHs, so P = R/2 has to be made after that.
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
| DE polarity | LTDC_DEPOLARITY_AL, which puts DE **high** in the picture: RM0433's DEPOL is the polarity of *not* DE | Unchanged: the MS7210 wants DE high (the SDK's DRM_BUS_FLAG_DE_HIGH). Measured on the G3, 14 Nov: with DEPOL set the pin was high 27% of the time, in the blanking, and there was no picture |
| Clock edge | LTDC_PCPOLARITY_IPC | Unchanged, with 0x1281 = **0x00** (the clock as it comes, no delay; Peter, 14 Nov, in 1080p: of all eight settings only the two extremes were bad). Register 0x1281 (REG_PA_S) inverts the clock (bit 2) and delays it by 0 to 3 steps (bits 1-0); the FPGA boards write 0x04, inverted with no delay. The datasheet asks for 0.879 ns of setup and 0.22 ns of hold, and a clock duty of 40-60%. Tuned on the board at the highest clock, 148.5 MHz, where a pixel is 6.7 ns |
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
- The G3 has no VGA socket: the resistor DAC is gone with it (Peter, 14 Nov). So nothing else on the colour lines
  draws current that depends on the colour.

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

ReadBuffer8 (OtherDisplays.c) was a fifth: it scaled a CLUT colour up by 255/192 under LOW and 255/224 under MEDIUM
for PIXEL(, SAVE IMAGE and Print Screen. It no longer reads the option (Peter, 14 Nov: below), so it needs nothing.

The 12-bit depth needs nothing: it never had the option's alpha, and HIGH's BIT_4 is already 1:1.

The loadable programs need nothing either. Their palettes (the video_palette service) go into the CLUT as given, and
their 16 and 32-bit modes get the layer's alpha from setmode.

**When it takes effect:**
- G3Hardware is set by the probe in InitDisplayOther: before its setmode if the chip answers without a clock, after
  it if not ("Where the probe goes", below). Either way it comes before the choice of the BIT tables, so the tables
  come out right.
- If the probe has to come after the first setmode and it finds a G3, setmode runs once more, forced, to load the
  full CLUT and the layers' alpha.
- Nothing has reached the HDMI monitor by then, as the MS7210 is set up only after the probe.

**What a program sees:**
- Drawing, MAP and COLOUR MAP are unchanged, and so is PIXEL( in the 4, 1, 16 and 32-bit depths: those hold or
  return the colours as the program gave them.
- **PIXEL( in an 8-bit mode returns the value held in the CLUT, on every board** (Peter, 14 Nov: "this should be the
  case for all versions"). SAVE IMAGE and Print Screen get the same, as all three read through ReadBuffer8.
  - Done on v8-dev on 14 Nov, ahead of the G3 work: ReadBuffer8 no longer scales the CLUT's colour up by OPTION VGA
    OUTPUT's top.
  - Under HIGH, the default, nothing changes, as its divisor was 255: white reads &HE0E0E0.
  - Under MEDIUM white now reads &HE0E0E0 and under LOW &HC0C0C0, where both read &HFFFFFF before.
  - A colour set by MAP, which goes into the CLUT as it is, now reads back as it was set. Before, under LOW or MEDIUM
    a channel over the option's top overflowed: &HFF read back as &H52 under LOW.
- On a G3 the CLUT holds the full levels, so white reads &HFFFFFF. The mid levels move a little: &H68 to &H6D, &HB0
  to &HB6, &HD0 to &HDB. A golden that reads 8-bit pixels back needs a G3 result of its own, which goldens.py gives it
  by MM.DEVICE$.

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
- So when step 3 gives 720p and 1080p their codes (4 and 16), the infoframe is to say full range as well.
- **Where the field is (the SDK's ms7210drv_hd_tx_shell_set_video_infoframe, checked 14 Nov):** the MS7210 takes the
  AVI infoframe as raw bytes.
  - Clear bit 6 of 0x050E, then write the type 0x82 to 0x050A, the version 0x02 to 0x0509 and the length 0x0D to
    0x050B.
  - Then 14 bytes to 0x050D, one after another: the checksum (0x100 minus the sum of the header and data bytes), then
    data bytes 1 to 13. Then set bit 6 of 0x050E again. The FPGA table does exactly this.
  - Q is data byte 3, bits 3-2: 10 for full range. The SDK leaves it 0, and so does the FPGA table.
  - Data byte 1's scan field (bits 1-0) is 01, overscan, in the FPGA table. The SDK sends overscan for the CE formats
    and underscan (10) for the IT ones, and a computer's picture wants underscan, so that a TV that obeys it shows the
    edges. Data byte 3's bit 7, ITC (IT content), tells a TV the same, and many then turn off their picture
    processing.
  - Data byte 4 is the video code and data byte 5's bits 3-0 the pixel repetition.
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
   - Checked 14 Nov: MX_GPIO_Init runs (twice) just after the system clock, and the first setmode, in
     InitDisplayOther, comes after the SDRAM set-up, the pin 40 check and the console. Nothing guarantees 10 ms
     between them, so the release keeps the tick at which PG3 went low and waits out what is left of 11 ms.
1. Only when G1Hardware is 0, read registers 0x0000 to 0x0002 at 0xB2, and at 0x56 if nothing answers, with the short
   acknowledge check (above).
2. If nothing answers, drive PG7 as LTDC_CLK (the 800x600 clock is already running), wait 10 ms and read again. That
   covers a chip that needs a clock. On a G2 the clock would then run for about 10 ms, once, before going back to
   analog.
3. Found: set a new flag, G3Hardware, and run the MS7210's init for the mode in use. Not found: carry on as a G2.

**Where the probe goes depends on step 1's answer to the clock question.**
- If the chip answers with no clock, which is likely, the probe goes at the start of InitDisplayOther, before its
  setmode. Then the first setmode already has the G3's LTDC settings and full levels, the BIT tables are chosen right
  the first time, and the forced second setmode (below) is not needed.
- Only if it needs a clock does the probe go after the first setmode, with step 2's retry and the second setmode.

How it is reported (Peter, 14 Nov: MM.DEVICE$ "Colour Maximite 2 G3"):
- MM.DEVICE$ (MM_Misc.c), the start-up banner (main.c, on the screen and the serial console) and the head of OPTION
  LIST (MiscSTM32.c) all name the board. The banner and OPTION LIST print the same MES_SIGNON strings (Version.h), each
  choosing between four of them in its own expression.
  - So the name and clock line come from one routine, which the three call. G3 strings are added beside the G2's:
    "480MHz Colour Maximite 2 G3".
  - Until then a G3 says "Colour Maximite 2 G2", as the current firmware does.
  - tools/goldens.py files its results under the board MM.DEVICE$ names, so a G3 gets goldens of its own, which suits
    the 8-bit PIXEL( difference (below).
  - PicoMite programs look for "VGA" or "HDMI" in MM.DEVICE$ (picofrog). The CMM2's names have neither, with or
    without "G3", so nothing changes for them.
- VGA-only options noted as having no effect on a G3: OPTION VGA OUTPUT, as a G3 always drives full levels (above),
  and the offsets.

## Checked before the start (14 November)

The plan was read against v8-dev (a78205c) and the MS7210 sources on the day the first G3 arrived.

**Still as the plan says:**
- The pins: HAL_LTDC_MspInit maps every data line in the schematic's order. PE4 (B0) is driven on a G2 (7941b98), so
  on a G3 too. PG7 is analog except in mode 18, and DE is active low.
- PG3, PC8 and PD11-PD13 are used by nothing in the firmware.
- Mode 8 is 640x480 at 60 Hz, 25.2 MHz.
- The places that read Option.colourmap for the levels are those in the table above, and ReadBuffer8, which no longer
  does. The only other readers are OPTION LIST and the option itself, which stay.

**Corrected or added in the sections above:**
- Mode 19, which the table lacked: a doubled mode, half of VESA 1280x1024 at 60 Hz.
- amiga.app and bbc.app, which set 576p themselves (step 3).
- The I2C acknowledge wait of 250 ms, which the probe must not use.
- PLL3_Config, which never turns P on.
- Where the probe goes, once step 1 has answered the clock question.
- The AVI infoframe's layout, so the quantisation range and underscan can be set.
- What happens to a test program's settings when it ends, and so a way to an HDMI console before the firmware has the
  G3.

**Peter's answers (14 Nov):**
1. A G3 has no VGA socket: it went with the resistor DAC.
2. TVs: 800x600 at 75 Hz stays the start-up mode for now, to be revisited later (Risks).
3. PIXEL( returns the value held in the CLUT, on every board. Done on v8-dev the same day (above).
4. MM.DEVICE$ says "Colour Maximite 2 G3". The banner and OPTION LIST print the same name.

## Results on the G3 (14 November)

The first G3 arrived on 14 November. Peter fitted it in the G2's place (console COM16, the same Nucleo ST-Link) and it
was flashed with v8-dev (8.00.00b1, dae6cde). Steps 1 to 3 were done from ms7210test.app with the firmware unchanged.

- **The bus:** rtc_i2c_scan.bas finds the MS7210 at &H59 (8-bit &HB2), ID A2 20 0A, and a DS3231 at &H68.
- **The clock question:** the chip answers on I2C with PG7 analog, so a G3 is recognised before the first setmode.
- **The reset matters:** after power-up, with RSTN on its pull-up alone, the chip's registers were not its defaults:
  0x0080 and 0x1202 were 2 (YUV 4:2:2 in) and 0x092A was 0x40 (the HDMI clock halved). After PG3 low for 20 ms they
  read 0, the defaults, which is what RGB888 needs. So the start-up's PG3 reset is required, not a nicety. The test
  program writes the SDK's RGB settings anyway (0x0080, 0x1202, 0x1201, 0x00C0, 0x1200, 0x120B, 0x1265, 0x1262,
  0x0079, 0x092A), as a guard.
- **DE:** the firmware's own setting is right (the table above). The first attempt set GCR DEPOL and got no picture.
- **The first picture:** the FPGA table (0x1281 0x00), the oscillator at 27 MHz, the low band, on Peter's monitor in
  MODE 1. The test card: every data line right, full white beside &HE0E0E0, the greys distinct.
- **The clock edge (step 2):** in 1080p (148.8 MHz, PLL3 R 5 as it is), 0x1281 stepped through all eight settings
  with the card up: only the two extremes bad. 0x00 chosen. The duty-cycle change for mode 16 (VCO 592, R 4) is not
  needed.
- **Mode changes (step 3):** within the low band (1 to 8) nothing has to be written. From the low band to the high
  band (to 1080p) and back (1080p to 1) the HDMI PLL loses lock unless re-triggered; the re-trigger alone recovers it,
  even with the low band's drive. The rule for the firmware: on every change of pixel clock, mute, the band, the
  re-trigger (10 ms), the infoframe, unmute.
- **Group 1:** modes 1, 8, 9, 10, 11, 15, 16 and 18 all lock (0x091C bit 7) with the input timing stable (0x051D bit
  4, which is the video timing despite the SDK's register name CTS_STABLE). Peter: "the image is stunningly good".
- **Open:**
  - HPD: 0x0502 reads 0x80 (bit 0 clear) with the monitor connected and showing the picture. It read 0x01 once, with
    the chip in its bad power-up state. So bit 0 is not a usable hot-plug signal as things are; to find in step 1.4.
  - INT# (PC8) goes low after the init with 0x0500 = 0xA0 or 0xA8 and the mask 0x0501 at 0: what it reports is to be
    found (step 1.4).
  - The chip's clock meter reads 0.6-1% low (its RC oscillator at "27 MHz"): diagnostics only.

**Into the firmware (step 6 for group 1, 14 November):** Src/MS7210.c (in bank 2) with PG3's reset at start-up (low in
MX_GPIO_Init, high after RAMSETUP once 11 ms old, the probe 50 ms after, before InitDisplayOther), its own open-drain
I2C, the init above with a 20 ms settle (the FPGA table's 350 ms is not needed) and the clock-change sequence at the end
of every MX_LTDC_Init; G3Hardware; PG7 always LTDC_CLK (VERY_HIGH) and the colours, DE and syncs HIGH on a G3; the
offsets ignored (OFS); VgaMap() with FULL on a G3 (Clut8, VgaLevel, VgaAlpha, InitDisplayOther's tables);
"Colour Maximite 2 G3" in MM.DEVICE$, and the banner and OPTION LIST from BoardSignOn(). Checked on the G3: the banner,
MM.DEVICE$, 8-bit white &HFFFFFF, the chip locked with nothing run but the firmware.
- **Resolved:** three restarts out of about 25 on this firmware went into ST's system bootloader (PC 0x1FF0A4F4, USART1),
  not the firmware: the first start after its flash, and two OPTION DISK LOADs. _excep_code was RESTART_NOAUTORUN, so
  the firmware did not jump there (only RESTART_BOOT0 does): the chip started from system memory, as with BOOT0 high.
  CPU RESTART 11 of 11, OPTION DISK LOAD 8 of 8 (COM16 open or closed) and every ST-Link reset were fine. **The
  cause:** a jumper missing on the board (Peter, asked about BOOT0's wiring); with it fitted, 10 OPTION DISK LOADs and
  5 CPU RESTARTs all came back. Not the firmware.

**Step 4 (14 November): the clock doubled on PG7.** Mode 7 first from ms7210test.app (`mode 7 x2`): PLL3 VCO 816, R 52
(15.69 MHz), P 26; SAI1 from pll3_p, block A a master transmitter, MCKDIV 0; PG7 AF6. The chip counts 31.2 MHz, its PLL
locks, and Peter's monitor reports 640x480 at 75 Hz: "Good picture". 0x1281 swept with the card up: only the two
extremes bad, as in 1080p, so 0x00 for every mode. Then into the firmware: MS7210_Pll in setmode (after OPTION
PICOMITE's divide, before PLL3_Config) gives modes 3, 7, 13 and 19 P = R/2 (3 and 7 with R 52), PLL3_Config enables P
on a G3, and MS7210_Mode turns SAI1's MCLK on PG7 on in those modes (off in the others). From BASIC's MODE, Peter's
monitor: 7 640x480 at 75 Hz, 13 800x600, 19 1280x1024 at 60 Hz (SAI1 at 108 MHz), 3 640x400. Modes 1 to 18 in turn,
the monitor's reports: 2 and 3 640x400, 4 480x432, 5 240x432, 6 256x480, 12 960x1080, 13 800x600, 14 960x540, 17
384x480, the standard ones as before: every mode shows on it, the four under 25 MHz (4, 5, 6, 17) included, fed to the
chip as they are. Mode 12 is not yet doubled (148 MHz on PG7, or the chip's repetition). A TV is still to be tried.
- **Modes 5, 6 and 17 (14 November):** a second monitor took none of them as they were. Mode 5 is mode 4's line at
  half the clock, so it is doubled (R 70, P 35: 628 clocks at 23.54 MHz to mode 4's 630), as mode 4 itself works.
  Modes 6 and 17 are VESA 640x480 at 75 Hz with fewer, longer pixels: 336 and 504 clocks a line where VESA has 840,
  so 2.5 and 5/3 VESA pixels each (256 x 2.5 = 384 x 5/3 = 640). A VGA monitor samples them at its 31.5 MHz, each
  pixel 2-3 or 2-2-1 screen pixels wide. The chip is given the same: P = R / 2.5 (VCO 816, R 65 = 12.55 MHz, P 26 =
  31.38 MHz) and R / (5/3) (VCO 664, R 35 = 18.97 MHz, P 21 = 31.62 MHz), tried first from ms7210test.app
  (`mode m pll N R P`). Both monitors: 6 and 17 640x480, 5 as 4. "All work, as expected" (Peter).
- **OPTION PICOMITE's modes (14 November):** the divided ones (ModeHs 2 or 4) get P = R / ModeHs, the base mode's
  own clock, on PG7, so the chip has the base mode's standard timing, each pixel 2 or 4 times. Peter drove every
  resolution and PicoMite MODE: all as the base modes, but OPTION RESOLUTION 640,315000, which gives 60 Hz where the
  PicoMite gives 75 (not the G3: the speed has been ignored since mode 8 became 60 Hz on 9 November).
- **The apps (14 November):** bbc.app and amiga.app 576p (their own PLL3 and timing, sent as they are), cpc 640x480 at
  60 Hz (MODE 8), nes 640x480 at 75 Hz (MODE 7 doubled), doom 640x400 at 75 Hz (MODE 3 doubled). From the console bbc,
  amiga and cpc leave on Ctrl-], nes and doom on Ctrl-C.
- **Mode 12 (14 November):** doubled to 1080p at 60 Hz: VCO 592, R 8 (74.0 MHz, -0.34%), P 4, so SAI1's MCLK on PG7 is
  148.0 MHz (within its 150 MHz kernel clock, beyond the 49 MHz ST characterises). Peter's monitor: 1080p.
- **Modes 4 and 5 (14 November):** VGA's 640x480 at 75 Hz with pixels 4/3 and 8/3 of VESA's (630 and 314 clocks a
  line), 432 of the 480 lines shown; a VGA monitor sees no DE. On a G3 the chip is given 31.62 MHz (VCO 664, P 21; R 28
  and 56), mode 5's line made 24 + 45 + 240 + 6 = 315 clocks (840 at 8/3, VESA's porches exactly), and DE covers the
  480 lines (mode 7's vertical) with the picture 24 lines down as the layer's window, the rest the background colour;
  GETSCANLINE still counts from the picture. Peter, both monitors: 640x480 at 75 Hz.
- **Hot plug (14 November): no code.** Switching the monitor off and on works in every mode: HPD drops and comes back
  and the chip carries on (its PHY's output enabled by hand, not by HPD), as it did when the firmware set it up with no
  monitor and one was plugged in later. Unplugging and plugging the HDMI cable resets the G3: MM.INFO(BOOT) "Switch",
  a reset-pin reset, not a brown-out, so a glitch on NRST as the cable's shield meets the monitor's ground (Peter: a
  different ground potential). The board comes back through its start-up with the picture. It happens on unplugging
  only, and it is not the firmware: a fault loops in its handler rather than resetting, no hardware watchdog runs,
  and PC8 (INT#) is on no interrupt; and it still happened with the MS7210 held in reset (PG3 low) and with the
  ST-Link's SWD cable off. The likely cause: the G3 connects the board's 5V to the HDMI connector's pin 18 directly
  (Peter's PicoMite board has a diode), so a monitor's own 5V and ground share the board's supply while the cable is
  in, and unplugging steps it. Peter, 14 Nov: the next G3 gets a diode and a fuse on pin 18 (no room to change this
  one). The planned INT/HPD polling is left out.
- **Flashing a G3 with the ST-Link failed** ("Unable to get core ID", "failed to download Sector[0]") every time the
  MS7210 was sending HDMI, which an H743 reset does not stop; with the HDMI cable out (or the G3 power-cycled) it
  worked first time. Peter has ground loops between USB hubs; the HDMI cable to the monitor closes another. Later
  flashes failed with the cable out as well, and one wrote the whole image but could not read it back to verify; a
  read-back by -u afterwards matched the image (but for the firmware's own first-boot word at 0x080F6F40 and its
  options at 0x08140000). So: check a flash by reading it back, not by -v alone, until the ground loops are sorted.

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

### With the firmware as it is

A G3 running the current firmware (V8 or RC6) works as a G2 in every way but the picture: the SD card, the RTC, the
SDRAM and USB are a G2's. **It shows nothing on HDMI**, for three reasons:
- Nothing sets up the MS7210 over I2C.
- PG7, the pixel clock, is driven only in mode 18 (HAL_LTDC_MspInit). In every other mode it is analog.
- DE is active low (MX_LTDC_Init), and the MS7210 wants it high.

So the first checks are made from the serial console (COM4 on the G2 position), and MM.DEVICE$ says "Colour Maximite 2
G2".

### Step 1: experiment from a loadable program, without reflashing

A test program (`tools/sdk/examples/ms7210test.c`, beside ltdctest.c and built the same way) can do all the bring-up
while the firmware stays as it is:
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

Before it, `tools/rtc_i2c_scan.bas` lists every device on the RTC's bus from BASIC, with any firmware (BASIC's I2C
commands drive only the I/O connector's buses, so it drives PD4 and PD5 through the GPIO registers and puts them
back). It reads the MS7210's ID at &H59 (8-bit &HB2) and the DS3231's time at &H68. On the G2 (14 Nov) it found the
DS3231 alone, in 0.2 s.

What the firmware does around such a program (checked 14 Nov):
- Every setmode that changes the mode or depth runs MX_LTDC_Init, and through HAL_LTDC_Init HAL_LTDC_MspInit. That
  puts DE back to active low and PG7 back to analog. So the program sets them again after each video_mode call.
- A setmode to the mode and depth already showing returns early, without touching the LTDC. The loader puts BASIC's
  mode back only if the program changed it, and the end of a BASIC program asks for OPTION DEFAULT MODE, the same
  mode.
- So a program that leaves BASIC's MODE 1,8 alone leaves the picture on when it ends. Until the firmware has the
  G3, `RUN "ms7210test.app", on` gives an HDMI console at the prompt, until the next MODE that changes the mode.

If the monitor refuses 800x600 at 75 Hz, MODE 8 is the one to try: VESA 640x480 at 60 Hz since 9 November, which is
CEA's video code 1, the one format every HDMI sink must take.

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
   - Include `amiga.app` and `bbc.app`, which change PLL3 and the timing to 576p themselves, and back at their end.
   - If the chip follows a new clock by itself, they need nothing.
   - If it needs the PLL re-trigger, the fix belongs in one place, not in each program. Either the firmware's
     hot-plug poll notices the change (the clock meter at 0x001E, or "timing stable" at 0x051D) and re-triggers, or
     a new service lets a program say that it has changed the timing.
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
  - The register access over Analog_I2C.c, with its own short acknowledge check (above).
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
  a 60 Hz mode such as 800x600 at 40 MHz. Peter, 14 Nov: 800x600 at 75 Hz stays for the moment, to be revisited
  later.
- **148.5 MHz** on the board's traces and the H743's pins, with the edge tuning of step 2. If 1080p will not come
  clean, mode 16 may have to be refused on a G3.
- **No datasheet:** the register meanings come from MacroSilicon's SDK and the FPGA table. Anything they do not use,
  we cannot rely on.
