# Colour Maximite 2: MMBasic 7.00.00RC3 (release candidate) release notes, from 5.07.02

*7 November 2026. Compared with MMBasic 5.07.02, the last official release. The changes since the last version released, 7.00.00RC2, have notes of their own.*

MMBasic 7.00 brings the Colour Maximite 2 up to the PicoMite's MMBasic version 7. The language is the same on both computers apart from the hardware one of them lacks, so most PicoMite programs run with few changes (and with OPTION PICOMITE, those written for a PicoMite with a VGA or HDMI screen run unchanged), and most Colour Maximite 2 programs run unchanged. Everything the Colour Maximite 2 already did is kept: the 18 video modes and their colour depths, pages, IMAGE, BLIT and sprites, the 3D engine, CAN, the DAC and ADC commands, the Wii controllers, speech and the GUI controls.

The betas added USB touch screens, with the GUI controls, gestures and an on-screen keyboard, and the PicoMite's STEPPER command for stepper motors driven by G-code, and they were checked by running published Colour Maximite 2 programs (Wolf3D, Gauntlet, Into the Darkness, Z-MIM, NOMIS, BoxDot, Cuno, the Welcome Tape and SP Tools). The release candidates add loadable programs, written in C or C++ and run with RUN (MicroPython, Doom, ScummVM and pc386, a 386 PC that runs DOS and Windows 3.1, come with them), and an FM synthesiser that plays MIDI and the music of DOS games.

This release also takes in Gerry Allardice's work on the 6.00.00 betas (up to 6.00.00b14): his fixes, and the PicoMite features he had brought across.

This is the fourth release candidate: unless problems are found in it, it will become 7.00.00. Please report problems on the Back Shed forum. The parts that have not yet been tested with real hardware are listed below; reports from people who have that hardware will help most.

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

- **Options kept:** the keyboard layout, the clock's settings, the console port, the SD card speed, the default video mode, the editor's font, the mouse and its sensitivity, and OPTION MAXCTRLS.
- **Options reset:** every other option returns to its default, including OPTION AUTORUN.
- **Erased:** the program held in flash, and any variables saved with VAR SAVE. A program that ran at power-up has to be run again from its file, and OPTION AUTORUN set again.
- **Not affected:** programs and files on the SD card.

This is what an update from 5.07.02 does. An update from 7.00.00b4 or later keeps all the options, the library and the variables: see the notes for 7.00.00RC2.

**From this version on, writing the firmware erases the options.** Use OPTION DISK SAVE before an update and OPTION DISK LOAD after it.

The serial console runs at 115200 baud after the update. A Generation 2 can run it at 460800 (OPTION BAUDRATE 460800), which makes the editor and file transfers much faster; the Generation 1's USB to serial converter cannot run at 460800.

You can go back to an earlier firmware the same way, should you need to.

## What is new

### Loadable programs

- A file ending .APP is a program written in C or C++ and compiled for the processor: `RUN "name.app" [, words]` runs it, at the full speed of the processor, and MMBasic carries on when it ends (at the prompt, or with the BASIC program's next statement), tidying up after it.
- It uses the computer through services the firmware gives it: the console, files, the screen, the keyboard and mouse, sound, the FM synthesiser and the I/O pins.
- MicroPython (as on the Pico Computer 3), Doom (with Freedoom), ScummVM (for classic adventure games) and pc386 (a 386 PC, tiny386 by Chunhui He, which runs FreeDOS, MS-DOS and Windows 3.1 with VGA graphics, a Sound Blaster 16 and a mouse), a NES emulator (frank-nes, with two free games) and Heretic (with its shareware episode) come with the firmware, in CMM2_Programs_7.00.00RC2.zip.
- A VT100 terminal (vt100.app) makes the Colour Maximite 2 the screen and keyboard of a PicoMite or other computer, on COM1, COM2 or a USB serial device. The manual Colour_Maximite_2_App_User_Guide.pdf covers all these programs.
- Quake (quake.app, on a Generation 2) is id Software's game with its shareware episode, at about 53 frames a second with the sound on. The manual Colour_Maximite_2_App_User_Guide.pdf has a chapter on it.
- zeptoforth (zeptoforth.app, and zeptoforth_g1.app on a Generation 1) is Travis Bemann's Forth, with preemptive tasks, double precision and the zeptoed editor, on the VT100 terminal's console. The manual Colour_Maximite_2_App_User_Guide.pdf has a chapter on it.
- A Generation 2 keeps 8MB of its memory for loadable programs; a Generation 1 lends a program the largest free part of MMBasic's heap while it runs, and a program built for it (Doom and ScummVM are) the display memory as well. The same program file runs on both.
- The companion manual Colour Maximite 2 Loadable Programs describes how to write them, with the tools in the repository's tools/sdk, and Colour Maximite 2 386 PC Emulator how to use pc386.

### The C compiler

- CC compiles a program on the Colour Maximite 2, with no PC: the Fuzix Compiler Kit, with the bytecode and Arm code generator written for the Pico Computer 3. `CC "hello.c"` makes hello.bc, and `RUN "hello.bc"` runs it. RUN "hello.c" compiles it first if it needs it. A compiled program can use the graphics, sound, keyboard, mouse and I/O pins through cmm2.h.
- CC compiles MMBasic programs too: `CC "game.bas"` translates the program to C and compiles it to game.b.bc. The .bas file is not changed. A 3,200-line calculation that took 6.16 seconds with the interpreter and 1.71 seconds with OPTION COMPILE ON took 0.93 seconds.
- What runs a compiled program is in the firmware. The compiler is in the cc folder of the zip, which goes on the card as A:/cc. The companion manual Colour Maximite 2 C Compiler describes it.

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
- MM.INFO(CPUSPEED) is a string.
- KEYDOWN's shifted function keys add &H40.
- LOAD PNG's fourth argument is the alpha cut-off.
- GETSCANLINE counts from the first visible line, and in the doubled modes it counts the monitor's lines.
- MM.INFO(SOUND) gives the PicoMite's strings ("MOD", "PAUSED WAV" and so on), and MM.INFO(OPTION AUDIO) is "PWM".
- MM.INFO(FONTHEIGHT) of font 7 is 8, its characters' height (10, its line spacing, without OPTION PICOMITE).
- MM.INFO(KEYBOARD) reports the on-screen keyboard ("On" or "Off").
- With no resolution given, OPTION PICOMITE uses OPTION DEFAULT MODE's, and OPTION LIST shows the PicoMite's resolution and mode.
- Enter is a carriage return alone, as on the PicoMite.
- The manual's Running PicoMite Programs (in Video Modes and Pages) and PicoMite Pin Names (in Using the I/O Pins) have the details.

### The language

- **Structures**: TYPE ... END TYPE, structure variables and arrays, members in every form (`p.x`, `pts(i).corner(2).y`), structures as parameters and FUNCTION results, initial values in DIM, the STRUCT command and STRUCT() function, and LIST TYPE.
- **Arrays**: single element arrays (DIM a(0)), REDIM [PRESERVE], whole-array assignment `a() = b()`, FUNCTIONs that return an array, the ARRAY commands, and a member of every element of a structure array (`BOX b().x, b().y, ...`) in the graphics commands and MATH.
- **Program structure**: LIBRARY (a library of SUBs and FUNCTIONs in flash or RAM), CHAIN (run another program, keeping the global variables), SAVE CONTEXT and LOAD CONTEXT, `/* */` block comments, up to 1024 global variables, and RUN with a string expression for the program's name (RUN f$).
- **Errors and endings**: MM.ERRLINE, ON ERROR RESTART, SUB MM.END (run when the program ends), END NOEND and END cmd$.
- **Strings and bits**: MID$ insertion (`MID$(s$, n, 0) = x$`) and replacements of another length, LMID and LINPUT( for long strings, and BIT, BYTE and FLAG with FLAGS and MM.FLAGS.
- **MATH** is the PicoMite's complete command and function, with CLAMP, SINC and the reworked CROSSING and SENSORFUSION, on integer, float and string arrays.
- **Other additions**: RANDOMIZE, FLUSH, CWD$, SYNC, INTERRUPT (for CSUBs), SETTICK FAST, OPTION ESCAPE OFF, OPTION CONSOLE NONE, COLOUR MAP, MAP GREYSCALE, LOAD IMAGE, DRIVE, MM.INFO(HEAPI) and (HEAPS), MM.ONEWIRE, and the regular expressions' `{n}`, `\b` and `\B`.
- `&h`, `&o`, `&b` and the hex digits a-f may be written in lower case.
- A command can take up to 75 arguments, and a DATA statement up to 75 values, as the PicoMite's: the limit was 32. A SUB or FUNCTION can have up to 38 parameters (16 before).
- The PicoMite's stack check: a recursion too deep stops with "Stack overflow" before it can overwrite other memory, a recursive FUNCTION goes about 40 levels deep, and the prompt warns when a program came close to the limit. MM.INFO(HEAPEND) is the limit.

### Speed

Programs run faster: the solar eclipse benchmark takes 6.0 s against 7.4 s before. SUB and FUNCTION calls, IF, DO and FOR loops, and programs with interrupts armed gain the most.

OPTION SD TIMING FAST drives the SD card with the processor's SPI hardware directly: a 32MB copy on a Generation 2 takes 37 seconds, against 59 seconds with NORMAL.

### The compiler

- OPTION COMPILE ON runs programs from a compiled form, as the PicoMite does: the solar eclipse benchmark takes 1.7 s, and tight loops run up to ten times faster.
- The graphics commands LINE, BOX, RBOX, CIRCLE, ARC, TRIANGLE, PIXEL, TEXT and COLOUR, and most built-in functions, are compiled too: the compiled code works out their arguments and hands them the values. A Game of Life program that draws with PIXEL and LINE runs 2.6 times faster compiled than as text.
- The program is compiled when it is run, if it has changed; any statement the compiler does not handle runs as before, and LIST, EDIT, errors, TRACE and interrupts behave as without it.
- OPTION COMPILE SHADOW checks every compiled calculation against the interpreter. MM.INFO$(COMPILE) reports what the last RUN did.
- The compiled program has 1 MB of RAM of its own. On the Generation 2 half of it is taken from the variable memory (512 KB less than before). On the Generation 1 all of it is, but only while the option is on: the first OPTION COMPILE ON after a restart restarts the CMM2 to make the area.

### CSUBs

CSUBs built for the PicoMite run unaltered when they use only its call table. The PicoMite's `mmb2csub` tool can build CSUBs for the Colour Maximite 2 (`--target cmm2`), which use its floating point hardware and run many times faster than the same code interpreted.

### The editor, the file manager and the console

- **The editor** is the PicoMite's full screen editor, with help on F11, a mouse, Print Screen, a clipboard of any size, and a save that cannot lose your edits (a failed save leaves the editor open with the text).
- **FM** is the PicoMite's two-panel file manager (F1 at the prompt types it). It copies between the SD card and a USB flash drive, and takes the mouse: a click selects, a double click opens, runs or shows, a right click marks, the wheel moves the selection, and the top line's labels can be clicked.
- **The PS2 mouse**, with OPTION MOUSE 0, is the computer's mouse, as a USB mouse is: it starts with the computer, or when a mouse is first needed, and keeps running when a program ends.
- **ZMODEM SEND and ZMODEM RECEIVE** transfer files over the serial console, faster than XMODEM.
- **HELP** reads A:/help.txt when the card has one.
- **OPTION F15, F16, F19 and F20** set strings for Shift-F3, F4, F7 and F8; Shift-F5 clears the screen and Shift-F6 sets up the terminal.
- **Print Screen** saves the screen at the prompt, in the editor and in FM (see Screen captures, below).

### USB

The USB socket takes a hub, and these devices:

- keyboards (a low-speed device must be plugged in directly, not through a hub);
- a USB mouse, as mouse channel 2: DEVICE(MOUSE ...), MOUSE INTERRUPT, MOUSE SET and OPTION MOUSE SENSITIVITY;
- gamepads: the DualShock 3 and 4, the Switch Pro controller, Xbox 360 and Xbox One/Series controllers, and generic pads, with GAMEPAD and DEVICE(GAMEPAD ...);
- USB serial adaptors, as COM4 to COM7 (FTDI, CP210x, CH34x and PL2303);
- a USB flash drive, as the C: drive (FAT or exFAT).

The SD card is both A: and B:, so PicoMite programs that name B: work. COPY and KILL take wildcards at the prompt.

### Graphics

- MODE n,4: a 4-bit colour depth that gives the PicoMite's 16 colour display, with its FRAMEBUFFER, layers and PEEK/POKE addresses, so that PicoMite games run.
- MODE n,1: a 1-bit colour depth, the PicoMite's MODE 1, with coloured tiles 8 pixels wide: TILE, TILE RESET, TILE HEIGHT and MM.INFO(TILE HEIGHT).
- RAY and RAY(, the PicoMite's raycasting engine, in every colour depth and in full colour (Appendix P).
- Fonts 8 and 9, the PicoMite's 4 x 6 and 8 x 10 fonts.
- A steadier VGA picture (the pixel clock from the crystal undivided, slower sync edges), and OPTION VGA OUTPUT in every colour depth but 12 bits.
- TILEMAP, image slots (FLASH LOAD IMAGE, FLASH DISK LOAD), BLIT FLASH and BLIT FRAMEBUFFER.
- FILL (flood fill), BEZIER, a smoother ARC, and BOX with arrays of x and y and one size.
- LOAD JPG with dithering, offsets and scaling; LOAD PNG of any size, a line at a time; LOAD BMP of every common kind (palette, RLE, 16-bit, top-down).
- FRAMEBUFFER CREATE without a size makes one the size of the screen.
- 25 colour names for RGB().
- GUI CURSOR LINK MOUSE works with or without GUI controls.
- The PicoMite's GUI LISTBOX (a list to choose from, which scrolls) and GUI SLIDER controls, and GUI CLICK, which clicks the controls from a program or with a button on a pin. TOUCH() is accepted as another name for CLICK().

### Sprites

- The PicoMite's additions:
  - static objects: SPRITE STATIC, STATIC CLEAR, STINTERRUPT and NOSTINTERRUPT, and SPRITE(ST …), invisible rectangles for the solid parts of the scenery that sprites collide with;
  - SPRITE(B), a sprite's collision with what is drawn on the screen, pixel by pixel, and from which side;
  - SPRITE SHOW, SHOW SAFE and SWAP orientations 4 to 7, which draw every pixel;
  - SPRITE SET TRANSPARENT;
  - SPRITE LOADBMP, also BLIT LOAD;
  - SPRITE LOADPNG of part of an image, with the transparent pixels given a colour;
  - SPRITE LOAD's palette argument.
- Sprites are kept through a change of mode: their images are full colour in every colour depth, so they can be loaded once and shown in any mode.
- SPRITE LOADPNG reads every kind of PNG that LOAD PNG reads, a line at a time.
- SPRITE MOVE and SHOW are faster in most colour depths: MOVE by 9% to 27%.
- A program's SPRITE commands list as SPRITE (they listed as BLIT).

### Sound

- FLAC and MP3 are decoded to 16 bits.
- PLAY ARRAY, PLAY SAMPLE (with envelopes), PLAY RELEASE and PLAY LOAD SOUND.
- BBC Micro sound: PLAY BBC SOUND and PLAY BBC ENVELOPE.
- PLAY MODFILE can play once and call an interrupt at its end, and PLAY EFFECT has its own interrupt.
- MM.INFO(SOUND) also reports ARRAY, SAMPLE and BBC sound.
- An FM synthesiser (ymfm's emulation of the OPL3): PLAY MIDIFILE plays MIDI files; PLAY MIDI, PLAY NOTE ON and OFF, PLAY MIDI CMD and PLAY MIDI TEST are the PicoMite's commands for its VS1053 module, with no module needed; PLAY OPL plays IMF, DRO and VGM music recorded from an OPL chip, and PLAY OPL WRITE writes its registers. MM.INFO(SOUND) reports OPL and MIDI.
- Volume changes, stops and pauses are smooth (no clicks), and PLAY SOUND's square and sawtooth are band-limited.

### Communications and I/O

- SPI, I2C and ONEWIRE's READ and WRITE take their data as the PicoMite does: a list, a whole array or a string.
- I2C option 1 keeps the bus (a repeated start); a timeout of 0 means no timeout; MM.I2C is 2 after a timeout.
- ADC START can scale each channel to a range, and ADC TRIGGER can give up after a number of samples.
- SETPIN CIN can count either edge or both, with a pull-up or a pull-down; TEMPR takes a conversion time.
- OPEN COMn: takes a transmit interrupt, called when everything queued has been sent.

### Astronomy

- STAR, ASTRO, LOCATION and SLEW, the PicoMite's: the positions of 83 named stars and other objects, the Moon and the planets, from a location and the time (Appendix Q).
- GPS(SIDEREAL) and GPS(JULIAN).

### The PicoMite's other forms

- DEVICE, the PicoMite's prefix for its device commands: DEVICE WII [CLASSIC], NUNCHUCK, MOUSE, GAMEPAD, HUMID, WS2812 and BITSTREAM, and DEVICE(WII …), DEVICE(CLASSIC …) and DEVICE(NUNCHUCK …).
- CONTROLLER (WII) takes the PicoMite's WII OPEN, WII CLOSE and WII NUNCHUCK OPEN and CLOSE.
- MM.INFO$(LCDPANEL) returns "MODE" and the mode's number, as the PicoMite's VGA and HDMI versions do.
- MM.INFO(DISPLAY) returns the current mode's number, so that a program can tell a VGA or HDMI screen from an LCD panel.
- OPTION NOCHECK ON or OFF is accepted and does nothing.
- PWM channel, OFF and SERVO channel, OFF stop a channel, as STOP does.

### Flash and RAM slots

- The PicoMite's flash slots 1 to 3, with FLASH SAVE, OVERWRITE, LOAD, RUN, CHAIN, LIST and ERASE, and LOAD IMAGE and DISK LOAD into them.
- The flash slots are kept on the SD card (in the hidden directory A:/.flash), and a slot's program runs from RAM, so no flash memory is written.
- The PicoMite's RAM slots 1 to 5 (the image slots 4 to 8), with RAM SAVE, OVERWRITE, LOAD, RUN, CHAIN, LIST and ERASE, and RAM FILE LOAD, which loads a program into a RAM slot while another runs.
- The RAM slots are kept through RUN, CHAIN and NEW until RAM ERASE or a restart, as the PicoMite's are.
- MM.INFO(FLASH) gives the slot whose program is running.

### Libraries

FatFs R0.16 (with exFAT and its security fixes), current dr_wav, dr_mp3 and dr_flac (with their fixes for damaged files), cJSON 1.7.19, and ST's STM32H7 drivers 1.11.6.

### USB touch screens

- **Connecting one:** a USB touch screen is recognised as soon as it is plugged in, as on the PicoMite. It can be a touch monitor, or a touch panel with a USB controller.
  - Touch screens made for Windows work.
  - A multi-touch screen reports up to ten fingers, and a single-touch one works as a pointer.
  - It is given channel 4, and MM.INFO(USB n) returns 133 for a multi-touch screen and 134 for a single-touch one.
- **The TOUCH() function** reads it, as on the PicoMite:
  - where the fingers are: X, Y, X2, Y2, and XN n and YN n (XN 0 gives how many fingers are down);
  - whether the screen is touched: DOWN and UP;
  - the GUI's REF, LASTREF, LASTX and LASTY;
  - the gestures: the swipes (SWL, SWR, SWU, SWD and SWIPE), TAP, DTAP, HOLD, a pinch (EXPAND, CONTRACT and PINCH), a twist (CW, CCW and ROTATE) and a two-finger tap (TTAP).
- **The GUI controls** respond to a touch as they do to a mouse click. GUI TEST TOUCH draws a dot wherever the screen is touched, and GUI TEST LCDPANEL draws circles and reports how many a second.
- **The full screen editor:** a swipe left, right, down or up is Home, End, Page Down or Page Up, and a tap moves the cursor there.
- **The WCH touch controller:** some monitors use the WingCoolTouch/WCH controller (USB ID 27C0:0859), such as the Corsair Xeneon Edge and the Verbatim HD14T. It gives its multi-touch reports only to a computer it takes for a Windows one. MMBasic now reads it as Windows does, so all ten fingers are seen.

### The on-screen keyboard

- **OPTION SCREEN KEYBOARD n [, M]** shows a keyboard on a touch screen, as on the PicoMite.
  - ‘n’ is the percentage of the screen's height it takes, from 0 to 50; 30 is a good size.
  - It types at the command prompt and in the editor and the file manager, and for INKEY$ and INPUT.
  - It has letter, symbol and function-key pages, and arrow keys.
- **Its modifier keys:** without M, Sh, Ctl, Sym and Fn latch for the next key. With M they are held: hold Sh with one finger and tap a key with another.
- **When it is shown:** automatically at the command prompt and in the editor and file manager, with the status line just above it. While a program runs it is removed. A program shows it with KEYBOARD ON and removes it with KEYBOARD OFF.
- It is not available in MODE n,1.

### The mouse, GUI CURSOR and GUI CLICK

- **GUI CURSOR** works as on the PicoMite:
  - GUI CURSOR x, y sets the cursor's point even while it is off or hidden, and GUI CLICK presses there.
  - GUI CURSOR ON with new arguments changes the shape, point or colour while the cursor is already on.
  - GUI CURSOR COLOUR is kept while the cursor is off.
  - GUI CURSOR LINK MOUSE moves the mouse to the cursor.
  - GUI CURSOR LOAD makes the loaded cursor the current one at once.
- **The GUI controls' pointer:** it is shown while a mouse is plugged in, as if the program had used GUI CURSOR ON and GUI CURSOR LINK MOUSE. So GUI CURSOR's commands work on it.
- **GUI CLICK** presses at the cursor's point and moves the mouse there.
- **CLICK()** is the PicoMite's: DOWN, UP, X, Y, REF, LASTREF, LASTX and LASTY, for a mouse, GUI CLICK or GUI CLICK PIN.
- **MM.MOUSE** gives the channel of the mouse to use: 2 for a USB mouse, otherwise a mouse already open or the channel set with OPTION MOUSE, and -1 if there is none. DEVICE(MOUSE n, ...), MOUSE INTERRUPT and MOUSE SET take any channel from 0 to 4 and act on the mouse in use.

### STEPPER

- **The PicoMite's STEPPER command** drives up to three stepper motor axes (X, Y and Z) and a rotary A axis from G-code: acceleration planning, arcs, corner blending, homing, limit switches and an emergency stop.
- **TMC2209 and TMC2208 drivers** can be set up and read over a serial line (the TMC22xx command).
- PEEK(STEPPER ...) and MM.CODE report its state.
- **It is described in full in Stepper_Reference.pdf,** posted with this release.

### Screen captures

- **SAVE PNG** saves the screen as a PNG file.
- **Print Screen and Ctrl-\\** save the screen as A:/SCREEN_date_time.PNG at the command prompt and in the editor and file manager.
- In a running program they are ordinary keys unless the program sets OPTION PRINTSCREEN ON. OPTION PRINTSCREEN FORMAT PNG | BMP chooses the file type.
- A capture saves the screen as it is shown: the page on display, and in the 12-bit depth both layers over the background colour. In MODE n,4 it saves the colours shown, after MAP and MAP SET.

### The I/O pins

- **The PicoMite's pin names** (GP0 and up) work in every mode, not only with OPTION PICOMITE.
- **The PicoMite's 64-bit GPIO masks** work on GP numbers, and PORT and PORT() go through them.
- **BITSTREAM** takes the PicoMite's fourth argument (its mode) and its two-pin form.
- **DEVICE SERIALRX and DEVICE SERIALTX** are serial ports on any pins.
- **SETPIN pin, ARAW** takes a single raw reading.

### The PicoMite's other commands, functions and options

- BLIT COMPRESSED, BLIT RESIZE, SAVE COMPRESSED IMAGE and AUTOSAVE APPEND.
- AUTOSAVE ends with Ctrl-Z as well as F1, F2 and F6: Ctrl-Z saves and loads the program as F1 does, as on the PicoMite.
- LIST VARIABLES, LIST OPTIONS, LIST PINS and LIST COLLISIONS.
- PEEK(BP var), PEEK(WP var) and PEEK(SP var), which step through memory.
- MM.USB (the last USB key's code), MM.DISPLAY, MM.INFO$(PLATFORM) ("CMM2") and MM.INFO$(USBDRIVE).
- MM.INFO(RESET) names a brown-out, and MM.INFO(BOOT) is another name for it.
- MM.INFO(LINE), MM.INFO(CALLTABLE), MM.INFO(MAX VARS), MM.INFO(SYSTEM HEAP), and MM.INFO(OPTION WIDTH), (OPTION HEIGHT), (OPTION AUDIO) and (OPTION KEYBOARD).
- CPU SLEEP s.
- **Options:**
  - OPTION DEFAULT COLOURS fc [, bc];
  - OPTION KEYBOARD nn [, capslock] [, numlock] [, repeatstart] [, repeatrate];
  - OPTION F1 and OPTION F5 to F9;
  - OPTION LOGGING ON | OFF, which appends each error to A:/log.txt;
  - OPTION AUTORUN n [, NORESET], which runs program slot n at power-up.
- **Mode 18** (1024x600) uses the PicoMite's 1024x600 timing.

## What behaves differently

- OPTION OVERCLOCK ON runs an original Colour Maximite 2 whose processor is of revision Y at 480MHz, in place of 400MHz, and gives an error on other machines, which run at 480MHz already. It no longer runs a machine at 504MHz, beyond the processor's ratings: that stopped a Generation 1 from responding.
- FILES lists the files, as LIST FILES does; FM is the file manager.
- The editor's keys are the PicoMite's.
- MM.ERRMSG$ is the bare message, without "Error in line n:"; MM.ERRLINE gives the line.
- PRINT's comma moves to the next 8-column stop (it was 14).
- A PRINT that fails part way prints nothing.
- A single array element in SPI, I2C or ONEWIRE WRITE is one item: `SPI WRITE 4, a(2)` sends a(2) only. Use `a()` to send a whole array.
- An array named bit(), byte() or flag() is now the function, and a statement that starts with a variable called FLAGS is the command.
- DIM a(0) makes a one element array, and BOUND(a(), n) past the array's dimensions is an error.
- UCASE$( and LCASE$( with a second argument are an error.
- MATH:
  - MATH RANDOMIZE n now repeats MATH(RAND)'s sequence, the PicoMite's for the same seed.
  - Integer POWER, ADD and SCALE round once, after the arithmetic.
  - SENSORFUSION and Q_CREATE follow OPTION ANGLE.
- A program's OPTION EXPLICIT stays in force at the command prompt after the program ends, until NEW or RUN.
- A: and B: are both the SD card; C: is a USB flash drive.
- NEW turns OPTION AUTORUN off, as the manual always said.
- OPTION USBKEYBOARD DISABLE ignores a USB keyboard.
- With OPTION SLEEP, any key wakes the monitor, while a program runs too.
- MM.WATCHDOG stays true until a program ends, so a program run by OPTION AUTORUN after a watchdog restart sees it.
- GUI controls, like the editor, use a USB mouse without OPTION MOUSE, as soon as it is plugged in; a program can create its controls before there is a mouse.
- In the 32-bit colour depth RGB(r, g, b, t) keeps the top 4 bits of t.
- Sprites, as on the PicoMite:
  - SPRITE LOADPNG's third argument is the colour of the transparent pixels; the cut-off is the fourth, from 1 to 254. The old cut-off n is the new cut-off 16 × n - 1: `SPRITE LOADPNG 1, f$, 8` is now `SPRITE LOADPNG 1, f$, , 127`. With none given, a pixel whose alpha value is above 30 is now solid (it had to be at least 128).
  - Black in a sprite is transparent, whatever loaded it: in a sprite file a 0, and a solid black pixel of a PNG (a negative third argument gives these a colour). SPRITE SET TRANSPARENT chooses another transparent colour, and orientations 4 to 7 draw every pixel. In the colour depths below 32 bits, colours too dark to be told from black are transparent too.
  - BLIT WRITE copies every pixel unless its orientation is 4 to 7; SPRITE WRITE leaves out the transparent ones, as before.
  - BLIT LOAD loads a BMP file; SPRITE LOAD loads a sprite file, as before.
  - SPRITE(A) gives the address of an image of four bytes a pixel in every colour depth.
- One mouse is used at a time.
- MM.VER is 7.000005.
- Modes 1 and 13 are 800 x 600 at 75Hz (they were 60Hz), and most modes' pixel clocks moved by up to 0.42%.
- In the 16 and 32-bit colour depths full white is 7/8 of the old level (3/4 with OPTION VGA OUTPUT LOW), as in the 8-bit depth.
- A variable can have the name of a command, so a program that used a name before it became a command (FLASH, STAR, LOCATION) runs as it did: at the start of a statement, `name = value` (or `name$`, `name%` or `name!`) assigns to the variable. The exceptions are TIMER, TIME$, DATE$, FLAGS and CASE, where `NAME =` is the command itself, and REM, RUN and DATA. An array cannot have a command's name, and a name that is also a keyword or a function (FOR, GOTO, SIN) is still not allowed.
- A mode change puts the mouse, and the GUI cursor's position, at the centre of the new screen. An I2C mouse is told the new screen's size.
- OPTION PROFILING in a program is accepted and does nothing (it saved the option and restarted the computer). At the prompt it is still the line profiler.
- An empty duty cycle or pulse width in PWM or SERVO (eg, PWM 1, 1000, , 50) leaves that output as it is.
- A minus sign in front of a value now applies after a power that follows it, as in mathematics and most languages: -2^2 is -4 (it was 4), and -X^2 is -(X^2). Write (-2)^2 for the old result.
- **GUI CURSOR's errors:** SHOW, UNLINK MOUSE, and x, y with the cursor off no longer give an error. LINK MOUSE gives "No mouse connected" when there is no mouse.
- **RBOX's radius** is limited to half the shorter side, and a filled box keeps its whole outline.
- **Running out of memory** gives "Not enough memory for n bytes".
- **The MATH complex functions and the FFTs** with one argument give "Argument count".
- **MsgBox** cannot be opened by a click from GUI CLICK or GUI CLICK PIN. Nothing could move the pointer to its buttons, so it gives an error instead of waiting.
- **Interrupt events** that happened before their interrupt was set up are no longer reported to it: Nunchuk and Classic buttons, mouse buttons and the frame interrupt.
- **The DS3231 clock** is copied to the internal clock every hour; before, it was every 100 hours.

## Fixed from 5.07.02

### The interpreter

- **RUN's arguments after a ':' or in a single-line IF** reach the program as they are written. In `CHDIR "A:/Doom" : RUN "doom.app", -timedemo demo1` the program was given them in capitals and with the minus sign changed (MMBasic's stored form of the line), and so was MM.CMDLINE$ after such a RUN in a program. RUN at the start of a line was always right.
- A line longer than the screen, or one that grew past 255 characters when substituted, could hang the board. The console now keeps a line of up to 255 characters.
- `IF c THEN ERROR "x" ELSE ...` stopped with "Invalid character".
- A FUNCTION called as a statement, or a SUB used in an expression, ran part way and failed inside itself.
- A keyword in lower case inside DATA, such as rgb(red), stopped with "Invalid colour".
- Variables and names:
  - STATIC variables of different SUBs could share one variable.
  - Deep recursion ran out of local variables.
  - 512 or more SUB, FUNCTION and label names hung the board.
  - VAR RESTORE lost a variable's declared type.
- Numbers: MAX and MIN of very negative numbers, ABS of a large float, integer `^` (now fast), subscripts beyond 32 bits, and the printing of INF, NaN and the most negative integer.
- Loops:
  - `NEXT JI` closed `FOR I`.
  - DO UNTIL at the top of a loop looped for ever.
  - A DO loop in a recursive SUB used its caller's condition.
- Errors:
  - ON ERROR SKIP was used up by a FUNCTION's statements, by an earlier ON ERROR, or by an interrupt.
  - ON ERROR IGNORE was dropped by an interrupt, and an interrupt SUB inherited the error state.
  - "Not enough memory" in a SUB under ON ERROR SKIP cleared every variable.
  - The prompt kept a program's ON ERROR IGNORE.
  - An error inside a CASE body could lose its line number.
- The program's text:
  - A string without its closing quote restarted the processor.
  - Text inside quotes was changed by the line substitutions (`PRINT "ERASE"` printed CLEAR VARS), and a #DEFINE applied inside strings and parts of words.
  - A #COMMENT START/END block still acted on # lines, and a line beginning MMDEBUGGER was dropped.
  - A RUN line in a program could be stored damaged.
  - `BIN$ (5)` with a space failed.
  - TRACE skipped labels wrongly.
- Calls:
  - A FUNCTION's negative argument, `f(-24)`, gave "Dimensions".
  - CALL split its arguments inside the name's expression.
  - An EXECUTE run from inside another overwrote it.
  - The CSUB interrupt vectors were not cleared at RUN, so a pin edge could call code that had gone.
- Running and ending:
  - MM.CMDLINE$ was cut at a `|`, and `*"a name"` was split at its space.
  - A program that runs off its last line now runs MM.END.
  - RUN resets MM.ERRNO, and MM.INFO$(RESET) has no CR LF.
  - CONTINUE after Ctrl-C or an error inside a SUB lost the SUB's local variables, and its END SUB then cleared every variable.
  - GOSUB to a label or line that does not exist left a return behind it.
- RESTORE of a name that is both a label and a variable goes to the label.
- A DIM that ran out of memory still counted the variable in MM.INFO(VARCNT).

### Strings, numbers and MATH

- Dates and times:
  - DATETIME$(n) refused every number.
  - EPOCH did not check the minutes.
  - DATE$ = accepted years the clock cannot hold.
  - TIME$ = sec across midnight left the date wrong.
- FORMAT$, STR2BIN, LGETSTR$, and LONGSTRING LOAD, LEFT, RIGHT and MID could write past their buffers.
- CHOICE truncated its condition, and TRIM$ refused a lower-case where$.
- Regular expressions: `{n}` matched n or more, and LINSTR's could read past the string.
- JSON$: an index after the first was always 0, and a long string could be read past its end.
- MATH:
  - The fixes in the PicoMite's MATH: M_INVERSE and the determinant's memory, CRC12, Q_CREATE, PID, WINDOW, rounding, SLICE and INSERT, CHI, INTERPOLATE and FFT.
  - MATH RANDOMIZE 0 made MATH(RAND) return 0 for ever, and MATH(RAND) could return 1.
  - AES128 and BASE64 could overrun a short string.
  - The complex functions and FFT check their arguments.
  - CRC8 and CRC32 apply the endmask after reverseOut.
- LONGSTRING MID with a very large count is cut to the string.
- MATH(C_DIV) is accurate to the last digit.

### Graphics

- A Generation 2 left the lowest bit of blue (PE4) undriven: its set-up for the video named the pin and then left it out.
- FRAMEBUFFER:
  - CREATE without a size faulted, and CREATE over a framebuffer kept the old backup.
  - Drawing after PAGE WRITE or FRAMEBUFFER WRITE used the page's width as its height.
- PAGE:
  - AND_PIXELS, OR_PIXELS, XOR_PIXELS, MERGE, STITCH and SCROLL used the write page's size, not their pages'.
  - COPY's deferred copies.
  - DISPLAY -1 left the display off after the program, and mode 17's page limit.
- IMAGE in the 32-bit depth and with the Y axis up.
- LINE PLOT with a start or a step, CIRCLE with an aspect over 1 (it left gaps), and the % of font 7.
- PIXEL FILL in the 32-bit depth and off the page, and PIXEL() in the 32-bit depth.
- BLIT MEMORY with an odd width.
- LOAD BMP read the pixels from the wrong place in some files; LOAD PNG's cut-off of 1; SAVE IMAGE with the Y axis up.
- GUI CURSOR:
  - LOAD without an extension wrote into the program.
  - ON leaked memory, COLOR was refused, and OFF now unlinks the mouse.
  - A linked cursor puts its point, not its corner, at the mouse.
- DRAW3D CLOSE ALL, LIGHT, SET FLAGS and RESET.
- The VGA picture jittered on some monitors and through video capture devices, most in mode 11 (1280 x 720).
- Sound stopped while LOAD BMP decoded a large image.
- DRAW3D: HIDE after a hidden object, each face's lighting, CREATE's checks of the faces, and the argument counts.

### Sprites

- SPRITE LOAD without an extension wrote into the program, and it read one sprite too many.
- SPRITE COPY to sprites 32 and up.
- SPRITE SWAP with # before the second number, and SPRITE CLOSE after a SWAP could corrupt memory.
- SPRITE SCROLL's fill colour.
- Edge collisions could overrun.
- A change of colour depth kept the sprites in the old depth's pixels, and a change of resolution kept them shown with backgrounds from the cleared screen. Now a change of mode keeps them, no longer shown, to be shown again.
- After SPRITE MOVE, SCROLL or RESTORE, later collisions could be missed and SPRITE(T) was wrong; a sprite on layer 0 met only other layer 0 sprites; and a sprite with the most collisions recorded was left out of SPRITE(C, 0 …).
- The details of a collision could be cleared before the collision interrupt ran.
- The collision state outlived a RUN, so a program's first SPRITE(T) could show the last program's.
- SPRITE WRITE of a sprite that was shown moved it and overwrote its stored background.
- SPRITE SHOW SAFE of a shown sprite ignored a new layer.
- SPRITE HIDE SAFE and SHOW SAFE of a sprite at the bottom of the list left the others hidden until the next MOVE.
- In the 12-bit colour depth a SPRITE LOADARRAY sprite was invisible.

### Sound

- Mono FLAC and MP3 played at double speed.
- PLAY TONE's and PLAY TTS's completion interrupts never fired, and a directory's fired after each track.
- Directories: more than 20 tracks overran, a directory given with its drive failed, and MM.INFO(TRACK) could fault.
- PLAY SOUND's "Invalid type" never fired, and PLAY SOUND now works while paused.
- A module that cannot be loaded is refused.
- DAC START f, a(), b(), int ignored b().

### Files

- The FatFs and audio library fixes for damaged files and cards.
- DIR$ on the USB drive said "SD card not found" with no card in.
- LIST "file" did not add .BAS.
- MEMORY INPUT of fewer bytes than the file holds gave "End of file".

### Communications and I/O

- CAN:
  - OPEN at 1000000 bit/s set no timing.
  - FD bit rate switching was the wrong way round, and a Classical open after an FD one ran as FD.
  - Filters 21 to 31 could not be set.
  - The remote-frame flag.
  - CAN SEND and READ placed FD bytes differently.
  - RUN now closes the CAN.
- PWM:
  - A duty change on output B or C went to output A.
  - A duty change lost an inverted output's inversion.
  - Controller 2 was disturbed by controller 1.
  - With no duty cycle for 1A, pin 12 was held high (PWM 1, 1000, , 50, and SERVO 1, 50).
  - Controller 2 could switch on other outputs of its timer and invert 2A.
  - SERVO's pulses were inverted after a PWM with a negative duty cycle on the same output.
  - A change between centred and ordinary pulses at the same frequency kept the old kind.
- PULSE with a pin number out of range read past the end of a table.
- Serial ports: opening COM3 disturbed COM2, and the COM1 to COM3 transmit interrupts never fired.
- I2C:
  - A transfer could fail while a Wii controller was being polled.
  - I2C2 and I2C3 OPEN reserved the wrong pin.
  - I2C WRITE of a string checks its length.
  - A READ into another type wrote through stale pointers.
- ADC:
  - With a trigger it wrote to arrays it was not given.
  - ADC OPEN's pin check could unconfigure a COM pin.
  - The 400 MHz limit was never applied.
- ADC START, BLIT LOADARRAY and WS2812 read or wrote past an array given from an element.
- DAC START, ADC START and IR refuse a LOCAL array, which the interrupt went on using after its SUB ended.
- SPI READ refuses a count below 1.
- WS2812 type W, and chains of more than 85 LEDs.
- GPS: GPS(TIME) and GPS(DATE) before the first fix, a sentence without a good checksum, and PRINT #GPS's checksum.
- PORT() reads output pins, and DISTANCE is right at 480 MHz.

### Options and the console

- OPTION F11 and F12 strings of more than 24 characters overflowed, and OPTION SEARCH PATH was limited to 23 characters.
- OPTION LIST shows each option as the command that sets it, and OPTION KEYBOARD REPEAT only when it is changed.
- MM.INFO(OPTION CONSOLE PORT), and MM.INFO$(OPTION CONSOLE) for NONE.
- MOUSE(D) reported a double click for ever.
- EDIT stopped with "Mouse not connected" when the mouse that OPTION MOUSE names was not plugged in; it now works without the mouse.
- AUTORUN of a program without END did not tidy up after it.
- With OPTION FLASH 6, loading a program of more than 128 KB erased the saved options (and more than 256 KB with OPTION FLASH 5).
- A Ctrl-C that arrived while the computer restarted in a new video mode (after OPTION DEFAULT MODE, for example) could leave it hung, with no display and no console.

## Not there yet

- The PicoMite's FRAMEBUFFER layers in the 8, 16 and 32-bit colour depths.
- OPTION AUTORUN's NORESET as the PicoMite has it, and OPTION RTC AUTO.
- MM.INFO(AUTORUN), the PicoMite's, on its own (MM.INFO(OPTION AUTORUN) is there).
- ONESHOT, ADC RUN, TRIANGLE SAVE and RESTORE, and BLIT MEMORY332.

The PicoMite's FRAME command, YMODEM, OPTION DISK SAVE/LOAD, I2C SLAVE, SAVE PERSISTENT, OPTION CONTINUATION LINES, MANDELBROT and WATCHDOG HW are not planned for the Colour Maximite 2.

## Not yet tested with real hardware

- SPI and I2C with real devices, and I2C option 1's repeated start.
- I2C with a Wii controller on the same bus.
- CAN on a bus (sending and receiving are tested in the internal loop back).
- PWM and SERVO with an instrument (their timers are checked by reading them back).
- The ADC.
- The serial ports COM1 to COM3, and DEVICE SERIALRX and SERIALTX.
- RTC calibration.
- The COUNT, ADC, DAC and IR interrupts.
- STEPPER with motors (its timing and pins are tested on the loop-back wiring).
- The Switch Pro Controller, Xbox controllers, and an IR remote left running into the editor or the file manager.
- More USB devices than the USB host's endpoint table holds.

## Known problems

- A Generation 1 has less memory for a program run from a compiled BASIC program, or with OPTION RAM, because the BASIC program's memory is then in the way. If a large program will not start, run it from the command prompt, and use OPTION FLASH in place of OPTION RAM.
- A touch monitor that is also the Colour Maximite 2's display may reset its touch controller when the screen's resolution changes (MODE). Touches are not seen for a moment while it reconnects.
- An armed interrupt pin slows the program down, because the pin is polled after every statement.
- With a USB touch screen connected, a mouse click can be taken as a swipe by the GUI controls' gestures.
- The keyboard layout list names IT, but it cannot be selected.
- In pc386, a program protected by a check of its original floppy disk (Lemmings is one) does not run from a disk image.

## The manual

The Colour Maximite 2 User Manual for this release is posted with it. Its chapters and its reference sections (the commands, functions, options and read-only variables) have been checked against this firmware. Appendix N lists where each keyword is described.

This release's manual also describes:

- loadable programs, in the commands (RUN) and the chapter Using MMBasic;
- OPTION DISK SAVE and LOAD, OPTION SD TIMING and OPTION OVERCLOCK as they now are, in the options;
- CC, for C and for MMBasic, in the commands and the chapter Using MMBasic;
- the FM synthesiser, in the chapter Sound Output and in the commands (PLAY MIDI, PLAY NOTE, PLAY MIDI CMD, PLAY MIDI TEST, PLAY MIDIFILE and PLAY OPL);
- the touch screens and the on-screen keyboard, in the chapter Keyboard, Mouse and Gamepad;
- TOUCH(), CLICK(), GUI CURSOR, GUI CLICK, GUI TEST, KEYBOARD and OPTION SCREEN KEYBOARD, in the reference sections;
- the editor's swipes and taps, in the chapter Full Screen Editor;
- RUN with a string expression, MM.MOUSE and the new MM.INFO keys, in the reference sections;
- variables with a command's name, in the chapters Variables and Expressions and MMBasic Characteristics;
- MM.MOUSE and the mouse's centring on a mode change, in the chapter Keyboard, Mouse and Gamepad.

STEPPER is described in Stepper_Reference.pdf, writing loadable programs in Colour_Maximite_2_Loadable_Programs.pdf, compiling C and MMBasic in Colour_Maximite_2_C_Compiler.pdf, and pc386 in Colour_Maximite_2_386_PC_Emulator.pdf, all posted with the manual.
