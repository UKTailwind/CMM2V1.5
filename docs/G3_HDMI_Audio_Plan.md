# Colour Maximite 2 G3: sound through HDMI

*8 November 2026. The second phase of the G3, after [G3_HDMI_Plan.md](G3_HDMI_Plan.md): sound sent to the HDMI
screen as I2S through the MS7210, beside the DAC sound the G2 has now. Nothing here has been tried on a G3 yet.*

## What Peter asked

- I2S audio over HDMI, as a second phase after the picture.
- At first, a choice on the G3 between the DAC and HDMI. Then, as sending to both costs almost nothing: "Just run
  both always" (Peter, 8 Nov). So a G3 plays every sound through the DAC and through HDMI at once, and there is
  nothing to choose.

## Where we start

### The board

- **The wiring** (Peter's schematic, 8 Nov):
  - PD13 goes to SCLK_MCLK (pin 7).
  - PD12 goes to WS (pin 6).
  - PD11 goes to I2SDATA0 (pin 5).
  - I2SDATA1 to I2SDATA3 are not connected, which suits two channels.
- **The peripheral:** those three pins are SAI2's block A, at AF10: PD11 SD_A, PD12 FS_A, PD13 SCK_A. The HAL has
  GPIO_AF10_SAI2. Checked on 13 Nov: the H743 datasheet's pin list gives SAI2_SD_A, SAI2_FS_A and SAI2_SCK_A on
  those pins, and the Linux kernel's H743 pin table puts them on AF10.
- **Nothing in the way:**
  - The firmware does not use SAI2 or those pins.
  - Of the DMA controllers' streams, the firmware uses only DMA2 stream 0, for memory-to-memory copies. DMA1 is free.

### How the firmware makes sound now

- **One interrupt:** all twelve sources end in the TIM4 interrupt, Audio_Interrupt in stm32h7xx_it.c. They are
  PLAY TONE, WAV, MP3, FLAC, MOD, SOUND, the BBC sound, TTS, ARRAY, SAMPLE, OPL and MIDI, and a loadable program's
  stream. The interrupt writes DAC1's two channels once per sample; there are 25 such writes.
- **Each source's own rate:** TIM4 runs at the source's own sample rate, set by its prescaler for each sound:
  - a WAV, MP3 or FLAC file's own rate;
  - 44.1 kHz for MOD, SOUND, tones and the program stream;
  - 22.05 kHz for speech;
  - PLAY ARRAY's and SAMPLE's own rates.
- **Where the 16 bits go:** the samples are 16-bit when they are decoded, and are cut to 12 bits before the interrupt
  sees them. How depends on the source:
  - **WAV, MP3 and FLAC:** iconvert (Audio.c) turns each buffer's signed 16-bit samples into DAC values in the same
    buffer as it is filled. These are 12-bit, centred on 2000, with PLAY VOLUME applied (the mapping table), so the
    16 bits are gone before the interrupt runs.
  - **MOD, PLAY OPL and a program's stream:** i2convert keeps them signed 16-bit, but at half level, so that a sound
    effect (PLAY EFFECT's WAV) can be added to them. The interrupt adds the two and makes the DAC value.
  - **Made at 12 bits, with no 16 bits to keep:** PLAY TONE, SOUND, the BBC sound, speech, and PLAY ARRAY and SAMPLE,
    whose arrays hold DAC values.
- **Other DAC uses:** the DAC also has uses that are not sound, and these stay on the DAC:
  - the DAC command's output and waveform (MM_Custom.c, and the d1point table in the interrupt);
  - the level set at power-up (main.c).

### What the MS7210 needs for I2S (MacroSilicon's SDK)

- **I2S from its pins:**
  - 0x0008 bits 1-0 = 0: the audio clock from the SCK pin.
  - 0x00A5 bits 4-3 = 0: I2S, not S/PDIF.
- **The format:**
  - 0x0530 bit 0 = 0: I2S samples.
  - 0x0531 bits 6-0: the rate code. 44.1 and 48 kHz both use the 48 kHz code; the SDK notes "44.1K, N value 7007
    can't use".
  - 0x0535 bits 5-4: 16 bits.
  - 0x0532 = 0x01: two channels.
- **The audio infoframe:**
  - Type 0x84, two channels, written through 0x050A, 0x0509, 0x050B and 0x050D.
  - Then enabled with 0x050E bit 5.
- **Reset and unmute:** the audio is held in reset (0x0506 bits 2-1) while all of that is written, then released,
  and unmuted (0x0507 bit 2).
- **HDMI mode, not DVI** (0x0503 bit 0 clear): a DVI signal carries no sound. Phase 1 must therefore leave the chip in
  HDMI mode.
- **Re-sent at each mode change:** the video's sequence resets the chip's HDMI section (the SDK's "shell"), and the
  audio with it, so the audio set-up has to be sent again.
- **The frame:** the chip has one clock pin, SCK_MCLK, so it takes its audio clock from the bit clock and measures the
  rate from it. A standard I2S frame of 64 bits (two 32-bit slots) is therefore the safe choice. A frame of another
  length would throw that measurement off.

## The audio clock

SAI2 has to make the I2S bit clock, and so needs a clock that divides down to 64 times the sample rate:

| SAI2's clock | Rate it gives | The catch |
|---|---|---|
| **PLL1Q, 48 MHz** (USB's), divided by 17, then by the frame's 64 | **44,117.6 Hz, 0.04% fast** | None: PLL1Q is already 48 MHz, and the SAI only takes from it |
| PLL2P, 60 MHz (the ADC's) | 44.6 kHz with a standard frame, 1.2% fast | Too far off, or a frame of another length |
| PLL3P | Up to 0.7% off | PLL3 is the LTDC's, and its VCO changes with every mode. The sound would also break at each mode change |
| PLL2, retuned by its fractional part | Exact | It moves the SDRAM's, the UARTs' and the ADC's clocks as well |

**PLL1Q it is.** 0.04% (400 ppm) is inside the 0.1% that ordinary consumer audio is allowed (IEC 60958's level II).
The channel status says 44.1 kHz, and the sink rebuilds the clock from what the chip measures.
- PLL1Q is 48 MHz only on a board running at 480 MHz. At 400 MHz, USB takes HSI48 instead.
- A G3 runs at 480 MHz, as a G2 of revision V does. If a G3 ever runs at 400 MHz, this choice is made again.

**Beside the video's SAI1** (13 Nov). The video plan may use SAI1 to put a clock at twice the pixel clock on PG7
for the halved modes (G3_HDMI_Plan.md, "What the H743 can do itself"). That takes nothing from this plan:
- **Separate peripherals:** SAI1 block A gives only its master clock on PG7 (AF6), and SAI2 block A is the sound on
  PD11-13 (AF10).
- **Separate clock selectors and PLLs:** SAI1's clock is chosen by RCC_D2CCIP1R's SAI1SEL (bits 2-0, pll3_p, which
  changes with every mode). SAI2's is chosen by SAI23SEL (bits 8-6, pll1_q here, which never changes).
- **What it rules out:**
  - SAI1 for sound, both of its blocks, as they share pll3_p.
  - DFSDM1's audio clock, which also follows SAI1SEL. The firmware does not use DFSDM1.
  - SAI3 (which shares SAI23SEL with SAI2) and SAI4 (with its own selectors) stay free.
- **Keep SAI2 asynchronous:** never synchronise it to SAI1 (SAI_GCR SYNCIN), whose clock stops at every mode change.
  Asynchronous is the reset state.
- **Write RCC_D2CCIP1R a field at a time** (read, modify, write), as both selectors are in it.
- **The chip's audio clock regeneration is unchanged:** the MS7210 measures the rate from SCK and counts its CTS
  against the pixel clock it is given. In a halved mode that is PG7's doubled clock, which is a standard pixel clock
  to HDMI. PLL1 and PLL3 run from the same 8 MHz crystal, so CTS is steady.
- **Room for the sound:** the doubled modes' blanking is twice as many of the chip's clocks too, so a halved mode has
  the audio room of the standard mode it becomes, such as 640x480 at 75 Hz.

**TIM4 locked to the SAI.** The two clocks share PLL1:
- **The SAI's clock:** PLL1Q, from PLL1's 960 MHz VCO (PLLN 120, PLLQ 20).
- **TIM4's clock:** the timer clock comes from the same VCO through the system clock. TIM4 counts at 120 MHz
  (PSpeedDiv) and its period is 2.
- **The rate:** with its prescaler at 2,720 (PSC 2719), TIM4 runs at 120,000,000 / 2,720 = 44,117.647 Hz, exactly the
  SAI's rate. It never drifts from it, as both come from one oscillator.
- **Today:** a 44.1 kHz file sets PSC+1 to 2,721, which gives 44,101.4 Hz.

So on a G3:
- **The 44.1 kHz family:** for 44.1, 22.05 and 11.025 kHz sources TIM4 is set to 44,117.6 Hz, or a half or a quarter
  of it. Each sample TIM4 takes goes to the SAI as it is: no resampling, and no correction on the ring's fill, as the
  two cannot drift.
- **The DAC:** it plays those sources 0.04% fast as well (0.7 cent), as HDMI does, with the two in step.
- **Other rates** (48, 32, 16 and 8 kHz, PLAY ARRAY's and SAMPLE's own) are resampled to the SAI's rate, with the
  slow correction below. This is done in 32 bits, so the 16-bit resolution is kept, though the samples are new ones.

## The design

### One output, two destinations

- **AudioOut:** a small inline function, `AudioOut(left, right, hleft, hright)`, takes the place of the sound's DAC
  writes. Those are the writes in Audio_Interrupt and in StopAudio's ramp down.
  - It always writes the DAC exactly as the code does today, from the first two. On a G1 and a G2 that is all it does.
  - On a G3 it also puts the 16-bit pair (the last two) into a ring, written only by the TIM4 interrupt and read only
    by the SAI's DMA interrupt. That costs a few cycles a sample.
- **TIM4 as it is, but for its rate on a G3:** TIM4 still runs the sources, so none of the twelve changes. For the
  44.1 kHz family a G3 sets it to the SAI's rate (The audio clock, above).

### Full resolution for 16-bit sources

A 16-bit source reaches HDMI with all 16 bits, from the first version, not as a later addition (Peter, 8 Nov).
- **The 16-bit sources:**
  - WAV (16-bit files, and 8-bit ones, which the decoder widens);
  - MP3 and FLAC;
  - MOD;
  - PLAY OPL and MIDI;
  - a loadable program's stream (Doom, Quake, ScummVM...).
- **A 16-bit twin of each buffer, on a G3 only:**
  - As iconvert and i2convert convert a buffer, they also write each sample, with PLAY VOLUME applied in 16 bits, to
    a twin buffer at the same index.
  - At PLAY VOLUME 100 the twin holds the decoder's samples unchanged.
  - The interrupt reads the twin at the position it reads the DAC's buffer, and passes those samples to AudioOut as
    the 16-bit pair.
  - The DAC's buffers, and so the DAC's sound, are as they are now.
  - The twins are four buffers of WAV_BUFFER_SIZE, 32 KB, from the heap while a 16-bit source plays.
- **Mixing:** while PLAY EFFECT plays over MOD, OPL or a program's stream, the HDMI side adds the two twins at full
  level and clips at 16 bits. The DAC side still halves them, as now. With one source playing, nothing is added and
  the samples are unchanged.
- **The 12-bit sources** (tone, SOUND, BBC, speech, ARRAY, SAMPLE) are widened: (v - 2000) x 16.
- **Bit for bit:** a 44.1 kHz 16-bit WAV or FLAC at PLAY VOLUME 100 reaches the MS7210 with every sample as it is in
  the file, as no resampling is needed (The audio clock). At other rates the resampler keeps 16-bit precision.

### From the ring to the SAI

- **SAI2 block A:**
  - Master transmitter, I2S standard, with two 32-bit slots carrying 16-bit data (HAL_SAI_InitProtocol with
    SAI_I2S_STANDARD and SAI_PROTOCOL_DATASIZE_16BITEXTENDED).
  - No master clock (NOMCK), MCKDIV 17, taking its clock from pll1_q_ck.
- **The DMA:**
  - A DMA1 stream, circular, in two halves of 256 frames (5.8 ms each).
  - Its half-transfer and transfer-complete interrupts each fill the half just sent.
  - The buffer is cleaned from the data cache before the DMA reads it, or put in memory that is not cached.
- **The fill:**
  - **The 44.1 kHz family:** the ring's 16-bit pairs are copied as they are, each repeated twice at 22.05 kHz and
    four times at 11.025 kHz. TIM4 is locked to the SAI, so the ring's fill stays where it is.
  - **Other rates:** a resampler from TIM4's actual rate to the SAI's 44,117.6 Hz, in 32 bits. TIM4's actual rate is
    PSpeedDiv / (PSC + 1), the prescaler being an integer. Linear interpolation to start with.
  - **The correction:** for those, a slow correction on how full the ring is, nudging the step by up to 0.1%, keeps
    TIM4 and the SAI from drifting apart.
- **Silence:** when nothing is playing, the SAI goes on sending silence. A sink that loses its sound can take up to a
  second to bring it back, and would cut off the start of the next sound.

### Priorities

- TIM4's interrupt stays at the top priority, IRQ_PRI(0), so the DAC's timing is unchanged.
- The DMA's interrupt is lower. A fill of 256 frames takes a few tens of microseconds.

### Quality

- **16-bit sources:** all 16 bits, about 96 dB. A 44.1 kHz file at full volume arrives bit for bit (Full resolution
  for 16-bit sources, above).
- **12-bit sources:** the DAC's 12 bits, about 72 dB, as the sound is now. They are made at 12 bits.

### Both, always

- **No option:** a G3 sends every sound to the DAC and to HDMI together, so there is no command, no saved option and
  nothing in OPTION LIST.
- **When the SAI runs:** whenever a G3's MS7210 is set up in HDMI mode, from start-up, sending silence between sounds.
- **PLAY VOLUME:** it sets both, as the samples carry the volume already.
- **No speakers:** a screen with no speakers, or a DVI one, simply plays nothing, and the DAC carries on.
- **MM.INFO(AUDIO):** it answers "DAC" as now (or "PWM" under OPTION PICOMITE), as the DAC is still there on every
  board.

## Steps

### Step 1: from a test program, without reflashing (this makes sound)

As in the video plan's step 1, a loadable program sets up SAI2, the DMA and the MS7210's audio registers by writing
them directly, and sends a 1 kHz tone straight into the DMA's buffer.

**Done when:**
- A clean tone comes from the TV's or monitor's speakers.
- The rate measures 44,117.6 Hz, from PD12's frame clock on a scope or the chip's own measurement.
- The sound survives a mode change, and a cable unplugged and plugged back.

### Step 2: in the firmware, on a G3

The SAI, its DMA, the ring, the resampler and the correction, with `AudioOut` in Audio_Interrupt. Also, from the
start, TIM4 locked to the SAI for the 44.1 kHz family and the 16-bit twins in iconvert and i2convert. Then each source:
1. PLAY TONE.
2. WAV at 44.1, 22.05, 48 and 8 kHz.
3. MP3, FLAC and MOD.
4. SOUND, TTS, and OPL and MIDI.
5. ARRAY and SAMPLE.
6. A loadable program's stream, such as Doom's.

**Bit for bit:** the 16 bits are proved without a recorder.
- **The check:** a test build keeps the first 65,536 frames it hands to the SAI while a 44.1 kHz 16-bit WAV plays at
  PLAY VOLUME 100, and a program compares them with the file's own samples. They must be identical.
- **The same:** for a FLAC of the same music, and for an MP3 against the decoder's output (an MP3 is not the WAV's
  samples).
- **At PLAY VOLUME 50:** each sample must be the file's scaled in 16 bits, not a 12-bit value widened.

### Step 3: long runs

- **An hour of music:** an hour of MOD or MP3 with no click, the ring's fill logged throughout to show the correction
  holding.
- **Changes while playing:** a mode change, and a hot plug, while a sound plays.

### Step 4: checks and the manual

- **The DAC on a G3:** it must sound the same while HDMI plays beside it. Check for clicks from the extra work in the
  interrupt.
- **The manual:** the G3's sound comes out of both.
- **Regression:** a G2's and a G1's sound must be unchanged, as their path is untouched. The goldens are run on both.

### Later

- 48 kHz sources sent at 48 kHz. The SAI cannot make 48 kHz exactly from PLL1Q, so for now they are resampled to
  44.1 kHz.

## Risks

- **A sink and 44,117.6 Hz:** 400 ppm fast is inside the usual tolerance. It is to be heard on Peter's TV and monitor.
- **A monitor with no speakers, or a DVI one:** it gives silence, and the DAC's sound is there regardless.
- **The resampler at low rates,** an 8 kHz WAV for example: linear interpolation adds some aliasing. That is good
  enough to start with, and a better filter can follow.
- **TIM4's inexact rate:** for the 44.1 kHz family it is exact, locked to the SAI. For other rates TIM4's rate is not
  exact, as its prescaler is an integer. The correction absorbs that, but it has to be proved over a long run
  (step 3).
- **The DAC 0.04% fast on a G3:** with TIM4 locked to the SAI, a 44.1 kHz source plays at 44,117.6 Hz on the DAC too
  (0.7 cent). It is inaudible, and a G1's and a G2's rates are unchanged. If it ever matters, those sources can be
  resampled like the rest, at the cost of bit-for-bit output.
- **The twins' time:** iconvert and i2convert do a second multiply for each sample on a G3, outside the interrupt.
  This is to be timed with a 48 kHz FLAC, the heaviest decode.
- **Phase 1 first:** this phase needs phase 1's MS7210 driver, and its HDMI mode, first.
