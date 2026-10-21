# Colour Maximite 2: MMBasic 7.00.00b2 (beta) release notes, from 5.07.02

*21 October 2026. Compared with MMBasic 5.07.02, the last official release. The changes since the previous beta, 7.00.00b1, have notes of their own.*

MMBasic 7.00 brings the Colour Maximite 2 up to the PicoMite's MMBasic version 7. The language is the same on both computers apart from the hardware one of them lacks, so most PicoMite programs run with few changes, and most Colour Maximite 2 programs run unchanged. Everything the Colour Maximite 2 already did is kept: the 18 video modes and their colour depths, pages, IMAGE, BLIT and sprites, the 3D engine, CAN, the DAC and ADC commands, the Wii controllers, speech and the GUI controls.

This release also takes in Gerry Allardice's work on the 6.00.00 betas (up to 6.00.00b14): his fixes, and the PicoMite features he had brought across.

This is a beta: a work-in-progress build for users to try. Please report problems on the Back Shed forum. The parts that have not yet been tested with real hardware are listed below; reports from people who have that hardware will help most.

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

- **Options kept:** the keyboard layout, the clock's settings, the console port, the SD card speed, the default video mode, the editor's font, the mouse and its sensitivity, and OPTION MAXCTRLS.
- **Options reset:** every other option returns to its default, including OPTION AUTORUN.
- **Erased:** the program held in flash, and any variables saved with VAR SAVE. A program that ran at power-up has to be run again from its file, and OPTION AUTORUN set again.
- **Not affected:** programs and files on the SD card.

The serial console runs at 115200 baud after the update. A Generation 2 can run it at 460800 (OPTION BAUDRATE 460800), which makes the editor and file transfers much faster; the Generation 1's USB to serial converter cannot run at 460800.

You can go back to an earlier firmware the same way, should you need to.

## What is new

### The language

- **Structures**: TYPE ... END TYPE, structure variables and arrays, members in every form (`p.x`, `pts(i).corner(2).y`), structures as parameters and FUNCTION results, initial values in DIM, the STRUCT command and STRUCT() function, and LIST TYPE.
- **Arrays**: single element arrays (DIM a(0)), REDIM [PRESERVE], whole-array assignment `a() = b()`, FUNCTIONs that return an array, the ARRAY commands, and a member of every element of a structure array (`BOX b().x, b().y, ...`) in the graphics commands and MATH.
- **Program structure**: LIBRARY (a library of SUBs and FUNCTIONs in flash or RAM), CHAIN (run another program, keeping the global variables), SAVE CONTEXT and LOAD CONTEXT, `/* */` block comments, and up to 1024 global variables.
- **Errors and endings**: MM.ERRLINE, ON ERROR RESTART, SUB MM.END (run when the program ends), END NOEND and END cmd$.
- **Strings and bits**: MID$ insertion (`MID$(s$, n, 0) = x$`) and replacements of another length, LMID and LINPUT( for long strings, and BIT, BYTE and FLAG with FLAGS and MM.FLAGS.
- **MATH** is the PicoMite's complete command and function, with CLAMP, SINC and the reworked CROSSING and SENSORFUSION, on integer, float and string arrays.
- **Other additions**: RANDOMIZE, FLUSH, CWD$, SYNC, INTERRUPT (for CSUBs), SETTICK FAST, OPTION ESCAPE OFF, OPTION CONSOLE NONE, COLOUR MAP, MAP GREYSCALE, LOAD IMAGE, DRIVE, MM.INFO(HEAPI) and (HEAPS), MM.ONEWIRE, and the regular expressions' `{n}`, `\b` and `\B`.
- `&h`, `&o`, `&b` and the hex digits a-f may be written in lower case.
- A command can take up to 75 arguments, and a DATA statement up to 75 values, as the PicoMite's: the limit was 32. A SUB or FUNCTION can have up to 38 parameters (16 before).
- The PicoMite's stack check: a recursion too deep stops with "Stack overflow" before it can overwrite other memory, a recursive FUNCTION goes about 40 levels deep, and the prompt warns when a program came close to the limit. MM.INFO(HEAPEND) is the limit.

### Speed

Programs run faster: the solar eclipse benchmark takes 6.0 s against 7.4 s before. SUB and FUNCTION calls, IF, DO and FOR loops, and programs with interrupts armed gain the most.

### The compiler

- OPTION COMPILE ON runs programs from a compiled form, as the PicoMite does: the solar eclipse benchmark takes 1.7 s, and tight loops run up to ten times faster.
- The program is compiled when it is run, if it has changed; any statement the compiler does not handle runs as before, and LIST, EDIT, errors, TRACE and interrupts behave as without it.
- OPTION COMPILE SHADOW checks every compiled calculation against the interpreter. MM.INFO$(COMPILE) reports what the last RUN did.
- The compiled program has 1 MB of RAM of its own. On the Generation 2 half of it is taken from the variable memory (512 KB less than before). On the Generation 1 all of it is, but only while the option is on: the first OPTION COMPILE ON after a restart restarts the CMM2 to make the area.

### CSUBs

CSUBs built for the PicoMite run unaltered when they use only its call table. The PicoMite's `mmb2csub` tool can build CSUBs for the Colour Maximite 2 (`--target cmm2`), which use its floating point hardware and run many times faster than the same code interpreted.

### The editor, the file manager and the console

- **The editor** is the PicoMite's full screen editor, with help on F11, a mouse, Print Screen, a clipboard of any size, and a save that cannot lose your edits (a failed save leaves the editor open with the text).
- **FM** is the PicoMite's two-panel file manager (F1 at the prompt types it). It copies between the SD card and a USB flash drive.
- **ZMODEM SEND and ZMODEM RECEIVE** transfer files over the serial console, faster than XMODEM.
- **HELP** reads A:/help.txt when the card has one.
- **OPTION F15, F16, F19 and F20** set strings for Shift-F3, F4, F7 and F8; Shift-F5 clears the screen and Shift-F6 sets up the terminal.
- **Print Screen** saves the screen as A:/SCREENnnn.BMP, at the prompt, in the editor and in FM.

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
- Volume changes, stops and pauses are smooth (no clicks), and PLAY SOUND's square and sawtooth are band-limited.

### Communications and I/O

- SPI, I2C and ONEWIRE's READ and WRITE take their data as the PicoMite does: a list, a whole array or a string.
- I2C option 1 keeps the bus (a repeated start); a timeout of 0 means no timeout; MM.I2C is 2 after a timeout.
- ADC START can scale each channel to a range, and ADC TRIGGER can give up after a number of samples.
- SETPIN CIN can count either edge or both, with a pull-up or a pull-down; TEMPR takes a conversion time.
- OPEN COMn: takes a transmit interrupt, called when everything queued has been sent.

### Flash and RAM slots

- The PicoMite's flash slots 1 to 3, with FLASH SAVE, OVERWRITE, LOAD, RUN, CHAIN, LIST and ERASE, and LOAD IMAGE and DISK LOAD into them.
- The flash slots are kept on the SD card (in the hidden directory A:/.flash), and a slot's program runs from RAM, so no flash memory is written.
- The PicoMite's RAM slots 1 to 5 (the image slots 4 to 8), with RAM SAVE, OVERWRITE, LOAD, RUN, CHAIN, LIST and ERASE, and RAM FILE LOAD, which loads a program into a RAM slot while another runs.
- The RAM slots are kept through RUN, CHAIN and NEW until RAM ERASE or a restart, as the PicoMite's are.
- MM.INFO(FLASH) gives the slot whose program is running.

### Libraries

FatFs R0.16 (with exFAT and its security fixes), current dr_wav, dr_mp3 and dr_flac (with their fixes for damaged files), cJSON 1.7.19, and ST's STM32H7 drivers 1.11.6.

## What behaves differently

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
  - MATH RANDOMIZE n now repeats MATH(RAND)'s sequence.
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
- MM.VER is 7.000001.
- A minus sign in front of a value now applies after a power that follows it, as in mathematics and most languages: -2^2 is -4 (it was 4), and -X^2 is -(X^2). Write (-2)^2 for the old result.

## Fixed from 5.07.02

### The interpreter

- A line longer than the screen, or one that grew past 255 characters when substituted, could hang the board. The console now keeps a line of up to 255 characters.
- `IF c THEN ERROR "x" ELSE ...` stopped with "Invalid character".
- A FUNCTION called as a statement, or a SUB used in an expression, ran part way and failed inside itself.
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

### Graphics

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
- DAC START, ADC START, IR and KEYPAD refuse a LOCAL array, which the interrupt went on using after its SUB ended.
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

## Not there yet

- The PicoMite compatibility switch (OPTION PICOMITE), which will let PicoMite programs with conflicting keywords run unchanged.
- RAY and FRAME.
- YMODEM, OPTION DISK SAVE/LOAD, and a mouse in the file manager.

## Not yet tested with real hardware

- SPI and I2C with real devices, and I2C option 1's repeated start.
- I2C with a Wii controller on the same bus.
- CAN on a bus (sending and receiving are tested in the internal loop back).
- PWM channels 3 to 6.
- The ADC.
- The serial ports COM1 to COM3.
- RTC calibration.
- The COUNT, ADC, DAC and IR interrupts.
- Xbox controllers.

## Known problems

- An armed interrupt pin slows the program down, because the pin is polled after every statement.
- The keyboard layout list names IT, but it cannot be selected.

## The manual

The Colour Maximite 2 User Manual for this release is posted with it. Its chapters and its reference sections (the commands, functions, options and read-only variables) have been checked against this firmware. Appendix N lists where each keyword is described.
