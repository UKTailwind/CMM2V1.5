# Colour Maximite 2: MMBasic 7.00.00b5 (beta) release notes

*DRAFT, started 1 November 2026. Compared with 7.00.00b4, the previous beta.*

This is a beta: a work-in-progress build for users to try. Please report problems on the Back Shed forum.

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

- **Options kept:** all of them.
- **Kept:** the program held in flash, the library, the variables saved with VAR SAVE, and the programs and files on the SD card.

## What is new

- **RUN f$:** the program's name can be a string expression, as on the PicoMite. For example: RUN f$, RUN "GAME" + STR$(n), or RUN MM.INFO$(PATH) + "MENU", -x.
  - The name is read as an expression when it has a string variable or function in it, or when something other than a comma follows a quoted name.
  - Anything else is passed to the program in memory, as before: RUN -f still runs it with "-f" in MM.CMDLINE$.
  - The text after the comma is still passed to the program as it is written.
- **MM.INFO()** has the PicoMite's:
  - **LINE:** the number of the program line being run, as a string. It is the number an error there would report. It is "LIBRARY" in the library's code and "UNKNOWN" at the command prompt.
  - **CALLTABLE:** the address of the table of firmware routines that a CSUB calls, in the PicoMite's layout.
  - **MAX VARS:** the most variables a program can have.
  - **SYSTEM HEAP:** the free space of the firmware's own memory, which the audio, the JSON functions and the file system use.
  - **OPTION WIDTH and OPTION HEIGHT:** the console's columns and rows.
  - **OPTION AUDIO:** "DAC". With OPTION PICOMITE it is "PWM", the PicoMite's name for audio that works as the Colour Maximite 2's does.
  - **OPTION KEYBOARD:** the keyboard layout under the PicoMite's name for it. This is "GR" for the German layout, and "" when the keyboard is disabled.

## What behaves differently

- **GETSCANLINE in the 4-bit and 1-bit depths:** in the modes whose lines are drawn twice (3, 5, 6, 7, 12, 13 and 17), it now counts the mode's own lines, as the other depths always have. For example, MODE 7,4 now counts to 249, not 499.
- **GETSCANLINE with OPTION PICOMITE:** the doubled modes count the monitor's lines, as on the PicoMite, so MODE 2 counts to 499.
- **Print Screen and Ctrl-\\** save the screen as it is shown:
  - the page on display, when a program shows one page while it draws on another;
  - in the 12-bit depth, both layers blended over the background colour.

  Before, they saved the page being drawn on, and in the 12-bit depth only page 0.

## Fixed since 7.00.00b4

- **A FOR or DO loop straight after a single-line IF's THEN or ELSE,** inside another loop, made the outer loop end at the wrong NEXT or LOOP. When the IF was false, this gave "Cannot find a matching FOR". For example:

  ```
  For a = 1 To 2
    If k(a) > 0 Then For i = 1 To k(a) : Print "*"; : Next i
  Next a
  ```

- **MM.ERRNO and MM.ERRMSG$** are kept at the command prompt after a program stops with an error. Before, the status line cleared them within a second.
- **The file manager:** holding Up or Down on a USB keyboard now repeats, to scroll a long list. Page Up and Page Down still move one page per key press.
- **A file command on C: with no USB flash drive** (MKDIR, OPEN and the like) gave "SD Card not found". It now gives "USB drive not present", as DRIVE "C:" does (MM.ERRNO 21).
- **MM.INFO(FONTHEIGHT) and MM.FONTHEIGHT in font 7** now give 10, the spacing of its lines on the screen, instead of 8, the height of its characters. A program that placed text by them, such as Z-MIM with *FONT SMALL, drew it partly over the line before.
- **USB messages:** a message such as "Multi-touch Connected" could be printed in a program's output. This happened when the program ran another one with RUN, while the next program was loading. Such messages are now only shown at the command prompt.

## The manual

The User Manual describes all of the above:

- RUN, in the commands;
- the new MM.INFO keywords, in the reference section on MM.INFO;
- GETSCANLINE, in the functions and in the chapter Video Modes and Pages.
