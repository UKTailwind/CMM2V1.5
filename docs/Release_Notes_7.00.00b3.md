# Colour Maximite 2: MMBasic 7.00.00b3 (beta) release notes

*23 October 2026. Compared with 7.00.00b2, the previous beta. The full notes, compared with MMBasic 5.07.02, are posted with these.*

This beta adds the PicoMite compatibility switch, OPTION PICOMITE, with which programs written for a PicoMite with a VGA or HDMI screen run unchanged: its display modes and resolutions, its pin names, and its forms of SETPIN, PWM and SERVO. It also brings across the PicoMite's raycaster, its astronomy commands, its MODE 1 (a colour depth of 1 bit with coloured tiles) and two more fonts, gives the file manager the mouse, and steadies the VGA picture.

This is a beta: a work-in-progress build for users to try. Please report problems on the Back Shed forum.

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

As with 7.00.00b2:

- **Options kept:** the keyboard layout, the clock's settings, the console port, the SD card speed, the default video mode, the editor's font, the mouse and its sensitivity, and OPTION MAXCTRLS.
- **Options reset:** every other option returns to its default, including OPTION AUTORUN.
- **Erased:** the program held in flash, and any variables saved with VAR SAVE. A program that ran at power-up has to be run again from its file, and OPTION AUTORUN set again.
- **Not affected:** programs and files on the SD card.

## What is new

### OPTION PICOMITE

- OPTION PICOMITE device [, resolution] makes the Colour Maximite 2 behave as the PicoMite named, by the name its MM.DEVICE$ gives: PicoMiteVGA, PicoMiteVGAUSB, PicoMiteHDMI, PicoMiteHDMIUSB, PicoMitePC3, PicoMiteHDMIBTH or PicoMiteHDMIWEB, with RP2350A or RP2350B where the PicoMite has it. VGA and HDMI are short for PicoMiteVGAUSB RP2350B and PicoMiteHDMIUSB RP2350B. OPTION PICOMITE NONE, the default, turns it off.
- MM.DEVICE$ then returns that name, so a program that configures itself by it does so as on that PicoMite.
- The display:
  - MODE n with no colour depth is the PicoMite's MODE n (1 to 3 for VGA, 1 to 5 for HDMI), at the PicoMite's size, with its fonts and, in MODE 2 and 3, its 16 colours (the HDMI version's greens for an HDMI device).
  - ‘resolution’ is the PicoMite's OPTION RESOLUTION: 640x480 (the default), 720x400, 800x600 or 848x480, and for HDMI also 800x480, 1024x600, 1280x720 or 1024x768. RESOLUTION changes it in a program, as on the PicoMite HDMI.
  - Each resolution keeps one monitor timing for all its modes, as on the PicoMite, and the smaller modes have pixels of 2 x 2 or 4 x 4.
  - A program starts in MODE 1, and the mode it leaves stays at the prompt.
  - POKE DISPLAY VRES sets the height that is drawn on.
- The I/O pins:
  - The PicoMite's pin names (GP0 to GP22, GP26 to GP28, GP36 and GP37) stand for the pins of the rear connector with the same functions: COM1 on GP0 and GP1, PWM on GP2 to GP5, the counting inputs on GP6 to GP9, SPI, SPI2, I2C and I2C2 on their PicoMite pins, the analog inputs on GP26 to GP28. A name can be written as it is or as a string.
  - The PicoMite's SETPIN commands that give COM ports, I2C, SPI, PWM and IR their pins are accepted, checked as the PicoMite checks them.
  - MM.INFO(PINNO GPn) gives a name's pin, and MM.INFO(MAX GP) the PicoMite's highest GP.
  - PWM and SERVO take the PicoMite's arguments: PWM's fifth is its phase (centred pulses) and SERVO takes a position from -20 to 120.
- The manual's Running PicoMite Programs (in Video Modes and Pages) and PicoMite Pin Names (in Using the I/O Pins) have the details.

### The PicoMite's MODE 1 and tiles

- MODE n,1: a colour depth of 1 bit, as the PicoMite's MODE 1, in every mode. The screen is in tiles 8 pixels wide, each with a foreground and a background colour.
- TILE, TILE RESET and TILE HEIGHT, and MM.INFO(TILE HEIGHT), as the PicoMite HDMI's.
- PEEK and POKE of the screen act as on the PicoMite.

### The raycaster

- RAY and RAY(, the PicoMite's raycasting engine, in every colour depth and in full colour (Appendix P).

### Astronomy

- STAR, ASTRO, LOCATION and SLEW, the PicoMite's: the positions of 83 named stars and other objects, the Moon and the planets, from a location and the time (Appendix Q).
- GPS(SIDEREAL) and GPS(JULIAN).

### Fonts

- Fonts 8 and 9, the PicoMite's 4 x 6 and 8 x 10 fonts, are built in.

### The file manager and the mouse

- The file manager takes the mouse: a click selects, a double click opens, runs or shows, a right click marks, the wheel moves the selection, and the top line's labels can be clicked. A serial terminal's mouse clicks work too.
- With OPTION MOUSE 0 the PS2 mouse is the computer's mouse, as a USB mouse is: it starts when the computer starts, or when a mouse is first needed, and keeps running when a program ends.

### The PicoMite's other forms

- DEVICE, the PicoMite's prefix for its device commands: DEVICE WII [CLASSIC], NUNCHUCK, MOUSE, GAMEPAD, HUMID, WS2812 and BITSTREAM, and DEVICE(WII …), DEVICE(CLASSIC …) and DEVICE(NUNCHUCK …).
- CONTROLLER (WII) takes the PicoMite's WII OPEN, WII CLOSE and WII NUNCHUCK OPEN and CLOSE.
- MM.INFO$(LCDPANEL) returns "MODE" and the mode's number, as the PicoMite's VGA and HDMI versions do.
- MM.INFO(DISPLAY) returns the current mode's number, so that a program can tell a VGA or HDMI screen from an LCD panel.
- OPTION NOCHECK ON or OFF is accepted and does nothing.
- PWM channel, OFF and SERVO channel, OFF stop a channel, as STOP does.

### The VGA picture

- The picture is steadier: the pixel clock comes from the crystal undivided, with the PLL in its range in every mode, and the edges of the sync signals are slower. Video capture devices showed the difference most.
- OPTION VGA OUTPUT now applies to every colour depth but 12 bits: the 4-bit and 1-bit colours and the 16 and 32-bit pixels are driven to the same levels as the 8-bit depth's. LOW, which draws the least current from the outputs, steadies the picture further on a capture device.

## What behaves differently

- Modes 1 and 13 are 800 x 600 at 75Hz (they were 60Hz), which more monitors and capture devices lock to. With OPTION PICOMITE, the PicoMite's 800x600 resolution is shown at 75Hz too.
- Most modes' pixel clocks moved by up to 0.42% (mode 9, 1024 x 768, is now 64.73 MHz). Monitors take them as before.
- In the 16 and 32-bit colour depths full white is now 7/8 of the old level (3/4 with OPTION VGA OUTPUT LOW), as in the 8-bit depth.
- STAR, ASTRO, LOCATION and SLEW are keywords, and cannot be names of variables.
- MATH RANDOMIZE n gives the same numbers as the PicoMite for the same seed: different numbers from before.
- OPTION PROFILING in a program is accepted and does nothing, as the PicoMite's run-time profiler is not available. It used to save the option and restart the computer. At the prompt it is still the line profiler.
- An empty duty cycle or pulse width in PWM or SERVO (eg, PWM 1, 1000, , 50) leaves that output as it is.
- A USB serial device is read only while its COM port is open, and what it sent before the OPEN is thrown away.

## Fixed since 7.00.00b2

### The interpreter

- An assignment to an array element whose index has an '=' in it, such as a(CHOICE(k = 1, 1, 2)) = 5, took the '=' inside the brackets as its own. The same for MID$(, LMID(, BIT(, BYTE(, FLAG(, PIN(, PORT(, MAP( and CTRLVAL(.
- A FUNCTION's call as a graphics coordinate was taken as an array's index.
- RESTORE with an expression, a FUNCTION, an array's element or a label named like an array.
- DIM's LENGTH could not hold an '=' or an apostrophe, REDIM's bounds a function or brackets, and STRUCT SAVE and LOAD's file number a function.
- REDIM PRESERVE a(BOUND(a()) * 2) stopped with "Cannot find A".
- READ of whole arrays of short strings stopped with "Not enough internal memory".
- A keyword in lower case inside DATA, such as rgb(red), stopped with "Invalid colour".
- EXECUTE "runs = 5" was taken as RUN; EXECUTE of RUN "C:/x" ran A:/x; and an EXECUTE of RUN with an unclosed quote hung the computer.
- ERROR "50% off" garbled its message, and several messages showed their format codes.
- An option word was taken from inside an expression, such as the L of L(0) in PLAY SOUND or TRIM$. GUI CLICK PIN's INV never worked.
- A comment after FILES, RUN or HELP was taken as its argument (and was MM.CMDLINE$).
- CLOSE g was a syntax error, and CLOSE gpsf closed the GPS.
- MEMORY PACK, UNPACK, PRINT and INPUT with an address could crash.

### Graphics and the screen

- The VGA picture jittered on some monitors and through video capture devices, most in mode 11 (1280 x 720).
- Sound and USB stopped while LOAD BMP, SPRITE LOADBMP or FLASH LOAD IMAGE decoded a large image.

### I/O

- PWM 1 with no duty cycle for 1A held pin 12 high, and so did SERVO 1, 50.
- PWM 2 could switch on other outputs of its timer and invert 2A.
- SERVO's pulses were inverted after a PWM with a negative duty cycle on the same output.
- PWM changing between centred and ordinary pulses at the same frequency kept the old kind.
- PULSE with a pin number out of range read past the end of a table.

### USB, the mouse and the file manager

- A USB flash drive behind the hub with several other devices (a keyboard, a mouse and a USB serial adapter) was not mounted.
- CLOSE of a USB COM port soon after a long PRINT waited for ever.
- With OPTION MOUSE 0 and no PS2 mouse, a program's first GUI command stopped with "Mouse timeout".
- The file manager drew the right panel's selected line in the wrong colours after PgUp, PgDn, End and scrolling.
- A PS2 mouse's sensitivity of 1 to 4 gave a random speed.

## Not there yet

- The PicoMite's FRAMEBUFFER layers in the 8 and 16-bit colour depths (the PicoMite's MODE 4 and 5); they work in the 4-bit and 1-bit depths.
- With OPTION PICOMITE: MM.INFO(CPUSPEED) as a string, RUN with a variable, KEYDOWN's shifted function keys and GETSCANLINE's scaling.
- BITSTREAM's two-pin form, DEVICE SERIALRX and SERIALTX, ONESHOT and I2C SLAVE.
- The PicoMite's ADC command.

The PicoMite's FRAME command, YMODEM and OPTION DISK SAVE/LOAD are not planned for the Colour Maximite 2.

## Not yet tested with real hardware

- SPI and I2C with real devices, and I2C option 1's repeated start.
- I2C with a Wii controller on the same bus.
- CAN on a bus (sending and receiving are tested in the internal loop back).
- PWM and SERVO with an instrument (their timers are checked by reading them back).
- The ADC.
- The serial ports COM1 to COM3.
- RTC calibration.
- The COUNT, ADC, DAC and IR interrupts.
- Xbox controllers.

## Known problems

- An armed interrupt pin slows the program down, because the pin is polled after every statement.
- The keyboard layout list names IT, but it cannot be selected.

## The manual

The Colour Maximite 2 User Manual for this release is posted with it. It describes everything above: OPTION PICOMITE (Running PicoMite Programs in the chapter Video Modes and Pages, and PicoMite Pin Names in Using the I/O Pins), MODE n,1 and TILE, the raycaster (Appendix P), astronomy (Appendix Q), fonts 8 and 9, the file manager's mouse, and OPTION VGA OUTPUT. Appendix N lists where each keyword is described.
