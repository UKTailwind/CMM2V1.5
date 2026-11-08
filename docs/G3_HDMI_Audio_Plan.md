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
- **The peripheral:** those three pins should be SAI2's block A, at AF10: PD11 SD_A, PD12 FS_A, PD13 SCK_A. The HAL
  has GPIO_AF10_SAI2. This is to be checked against the H743 datasheet's alternate-function table.
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
- **Ready for the DAC:** the samples reach the interrupt already as DAC values. They are 12-bit, centred on 2000,
  with PLAY VOLUME applied (iconvert, i2convert and the mapping table in Audio.c).
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

## The design

### One output, two destinations

- **AudioOut:** a small inline function, `AudioOut(left, right)`, takes the place of the sound's DAC writes. Those are
  the writes in Audio_Interrupt and in StopAudio's ramp down.
  - It always writes the DAC exactly as the code does today. On a G1 and a G2 that is all it does.
  - On a G3 it also puts the pair into a ring, written only by the TIM4 interrupt and read only by the SAI's DMA
    interrupt. That costs a few cycles a sample.
- **TIM4 as it is:** TIM4 stays at each source's rate, so none of the twelve sources changes.

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
  - A resampler from TIM4's actual rate to the SAI's 44,117.6 Hz. TIM4's actual rate is PSpeedDiv / (PSC + 1), the
    prescaler being an integer. Linear interpolation to start with.
  - A slow correction on how full the ring is, nudging the step by up to 0.1%, so that TIM4's clock and the SAI's
    never drift apart.
  - The DAC's 12-bit values become 16-bit samples: (v - 2000) x 16.
- **Silence:** when nothing is playing, the SAI goes on sending silence. A sink that loses its sound can take up to a
  second to bring it back, and would cut off the start of the next sound.

### Priorities

- TIM4's interrupt stays at the top priority, IRQ_PRI(0), so the DAC's timing is unchanged.
- The DMA's interrupt is lower. A fill of 256 frames takes a few tens of microseconds.

### Quality

- **First:** the DAC's 12 bits, so about 72 dB, as the sound is now.
- **Later, if wanted:** 16-bit samples for the sources that have them: WAV, FLAC, MP3, MOD and a program's stream.
  An `AudioOut16` would sit beside `AudioOut`, with the volume applied in 16 bits.

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

The SAI, its DMA, the ring, the resampler and the correction, with `AudioOut` in Audio_Interrupt. Then each source:
1. PLAY TONE.
2. WAV at 44.1, 22.05, 48 and 8 kHz.
3. MP3, FLAC and MOD.
4. SOUND, TTS, and OPL and MIDI.
5. ARRAY and SAMPLE.
6. A loadable program's stream, such as Doom's.

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

- The 16-bit path.
- 48 kHz sources sent at 48 kHz. The SAI cannot make 48 kHz exactly from PLL1Q, so for now they are resampled to
  44.1 kHz.

## Risks

- **A sink and 44,117.6 Hz:** 400 ppm fast is inside the usual tolerance. It is to be heard on Peter's TV and monitor.
- **A monitor with no speakers, or a DVI one:** it gives silence, and the DAC's sound is there regardless.
- **The resampler at low rates,** an 8 kHz WAV for example: linear interpolation adds some aliasing. That is good
  enough to start with, and a better filter can follow.
- **TIM4's inexact rate:** TIM4's rate is not exact, as its prescaler is an integer. The correction absorbs that, but
  it has to be proved over a long run (step 3).
- **Phase 1 first:** this phase needs phase 1's MS7210 driver, and its HDMI mode, first.
