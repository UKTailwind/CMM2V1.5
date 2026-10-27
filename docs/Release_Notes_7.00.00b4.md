# Colour Maximite 2: MMBasic 7.00.00b4 (beta) release notes

*27 October 2026. Compared with 7.00.00b3, the previous beta. The full notes, compared with MMBasic 5.07.02, are posted with these.*

This beta brings USB touch screens to the Colour Maximite 2. The touch screens work with the GUI controls, report gestures, and can show an on-screen keyboard. It adds the PicoMite's STEPPER command, which drives stepper motors from G-code. It also adds SAVE PNG with a Print Screen key, the PicoMite's GP pin names in every mode, and most of the PicoMite's remaining commands and options.

This is a beta: a work-in-progress build for users to try. Please report problems on the Back Shed forum.

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

- **Options kept:** all of them. The new options (the F1 and F5 to F9 keys and OPTION SCREEN KEYBOARD) start unset.
- **Kept:** the program held in flash, the library, the variables saved with VAR SAVE, and the programs and files on the SD card.

## What is new

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

### STEPPER

- **The PicoMite's STEPPER command** drives up to three stepper motor axes (X, Y and Z) and a rotary A axis from G-code: acceleration planning, arcs, corner blending, homing, limit switches and an emergency stop.
- **TMC2209 and TMC2208 drivers** can be set up and read over a serial line (the TMC22xx command).
- PEEK(STEPPER ...) and MM.CODE report its state.
- **It is described in full in Stepper_Reference.pdf,** posted with this release.

### Screen captures

- **SAVE PNG** saves the screen as a PNG file.
- **Print Screen and Ctrl-\\** save the screen as A:/SCREEN_date_time.PNG at the command prompt and in the editor and file manager.
- In a running program they are ordinary keys unless the program sets OPTION PRINTSCREEN ON. OPTION PRINTSCREEN FORMAT PNG | BMP chooses the file type.
- In MODE n,4 a capture saves the colours shown, after MAP and MAP SET.

### The I/O pins

- **The PicoMite's pin names** (GP0 and up) work in every mode, not only with OPTION PICOMITE.
- **The PicoMite's 64-bit GPIO masks** work on GP numbers, and PORT and PORT() go through them.
- **BITSTREAM** takes the PicoMite's fourth argument (its mode) and its two-pin form.
- **DEVICE SERIALRX and DEVICE SERIALTX** are serial ports on any pins.
- **SETPIN pin, ARAW** takes a single raw reading.

### The PicoMite's other commands, functions and options

- BLIT COMPRESSED, BLIT RESIZE, SAVE COMPRESSED IMAGE and AUTOSAVE APPEND.
- LIST VARIABLES, LIST OPTIONS, LIST PINS and LIST COLLISIONS.
- PEEK(BP var), PEEK(WP var) and PEEK(SP var), which step through memory.
- MM.USB (the last USB key's code), MM.DISPLAY, MM.INFO$(PLATFORM) ("CMM2") and MM.INFO$(USBDRIVE).
- MM.INFO(RESET) names a brown-out, and MM.INFO(BOOT) is another name for it.
- CPU SLEEP s.
- **Options:**
  - OPTION DEFAULT COLOURS fc [, bc];
  - OPTION KEYBOARD nn [, capslock] [, numlock] [, repeatstart] [, repeatrate];
  - OPTION F1 and OPTION F5 to F9;
  - OPTION LOGGING ON | OFF, which appends each error to A:/log.txt;
  - OPTION AUTORUN n [, NORESET], which runs program slot n at power-up.
- **Mode 18** (1024x600) uses the PicoMite's 1024x600 timing.

### OPTION PICOMITE

- MM.INFO(CPUSPEED) is a string.
- KEYDOWN's shifted function keys add &H40.
- LOAD PNG's fourth argument is the alpha cut-off.
- GETSCANLINE counts from the first visible line.
- MM.INFO(KEYBOARD) reports the on-screen keyboard ("On" or "Off").
- With no resolution given, OPTION PICOMITE uses OPTION DEFAULT MODE's, and OPTION LIST shows the PicoMite's resolution and mode.
- Enter is a carriage return alone, as on the PicoMite.

## What behaves differently

- **TOUCH() and CLICK()** are now separate functions, as on the PicoMite. A program saved with TOUCH() lists it as TOUCH(), and CLICK() takes only the GUI's keywords; the gestures are TOUCH()'s.
- **GUI CURSOR's errors:** SHOW, UNLINK MOUSE, and x, y with the cursor off no longer give an error. LINK MOUSE gives "No mouse connected" when there is no mouse.
- **GUI CLICK with no position** presses at the cursor's point, not where the mouse is.
- **RBOX's radius** is limited to half the shorter side, and a filled box keeps its whole outline.
- **Running out of memory** gives "Not enough memory for n bytes".
- **The MATH complex functions and the FFTs** with one argument give "Argument count".
- **MsgBox** cannot be opened by a click from GUI CLICK or GUI CLICK PIN. Nothing could move the pointer to its buttons, so it gives an error instead of waiting.
- **Interrupt events** that happened before their interrupt was set up are no longer reported to it: Nunchuk and Classic buttons, mouse buttons and the frame interrupt.
- **The DS3231 clock** is copied to the internal clock every hour; before, it was every 100 hours.

## Fixed since 7.00.00b3

### The interpreter

- After an interrupt that ended in an error had cut a PAUSE short, the next run's first PAUSE returned at once and its SETTICK interrupt was not called.
- An array parameter given no argument printed stray characters in its error message.
- /\* ... \*/ block comments work as the manual describes.
- "#COMMENT END" with any number of spaces ends the comment block.
- RESTORE of a name that is both a label and a variable goes to the label.
- A DIM that ran out of memory still counted the variable in MM.INFO(VARCNT).
- READ with no program reads the library's DATA.
- LONGSTRING MID with a very large count is cut to the string.
- MATH(C_DIV) is accurate to the last digit.
- A library loaded into RAM ends with the program. FLASH LOAD n and RAM LOAD n followed by LIST list the new program.
- FLASH LOAD IMAGE and FLASH DISK LOAD into a RAM slot keep its old contents until the new ones are read.
- A program's font left selected is reset to the default font at the next RUN.
- LIBRARY LOAD may come before OPTION SYMBOLS OFF.
- A program line that is too long says "Line is too long".

### Graphics and the screen

- GUI CURSOR works in the 4-bit and 1-bit colour depths.
- DRAW3D: HIDE after a hidden object, each face's lighting, CREATE's checks of the faces, and the argument counts.

### USB, the mouse and the editor

- GUI CLICK PIN was left set after RUN: its button then read as pressed, and the mouse could not click.
- The G2's console no longer drops every second character after the console is restarted.
- The Switch Pro Controller is type 135 in DEVICE(GAMEPAD n, T), as in MM.INFO(USB n).
- A USB device that the USB host has no room for is reported as "[USB EP_FULL ...]".
- The editor and the file manager turn an IR remote off before they clear the variables it writes to.
- The editor saves an empty file as an empty file, and an import that runs out of memory says how much was read.

## Not there yet

- The PicoMite's FRAMEBUFFER layers in the 8, 16 and 32-bit colour depths.
- GETSCANLINE in the 4-bit doubled modes counting as the PicoMite does.
- OPTION AUTORUN's NORESET as the PicoMite has it, and OPTION RTC AUTO.
- RUN with a variable or an expression.
- Some MM.INFO keys: LINE, AUTORUN, CALLTABLE, MAX VARS, SYSTEM HEAP, and OPTION AUDIO, WIDTH, HEIGHT and KEYBOARD.
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

- A touch monitor that is also the Colour Maximite 2's display may reset its touch controller when the screen's resolution changes (MODE). Touches are not seen for a moment while it reconnects.
- An armed interrupt pin slows the program down, because the pin is polled after every statement.
- The keyboard layout list names IT, but it cannot be selected.

## The manual

The Colour Maximite 2 User Manual for this release is posted with it. It describes everything above:

- the touch screens and the on-screen keyboard, in the chapter Keyboard, Mouse and Gamepad;
- TOUCH(), CLICK(), GUI CURSOR, GUI CLICK, GUI TEST, KEYBOARD and OPTION SCREEN KEYBOARD, in the reference sections;
- the editor's swipes and taps, in the chapter Full Screen Editor.

STEPPER is described in Stepper_Reference.pdf.
