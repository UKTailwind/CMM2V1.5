# Colour Maximite 2: MMBasic 7.00.00b5 (beta) release notes

*2 November 2026. Compared with 7.00.00b4, the previous beta. The full notes, compared with MMBasic 5.07.02, are posted with these.*

This beta comes from running published Colour Maximite 2 programs on MMBasic 7.00: Wolf3D, Gauntlet, Into the Darkness, Z-MIM, NOMIS, BoxDot, Cuno, the Welcome Tape and SP Tools. It fixes the problems they found in the firmware. It also lets a program use a new command's name as a variable, and adds RUN with a string expression, MM.MOUSE and the rest of the PicoMite's MM.INFO keys. With OPTION COMPILE ON, the graphics commands and most built-in functions are now compiled too.

This is a beta: a work-in-progress build for users to try. Please report problems on the Back Shed forum.

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

- **Options kept:** all of them.
- **Kept:** the program held in flash, the library, the variables saved with VAR SAVE, and the programs and files on the SD card.

## What is new

- **OPTION COMPILE ON compiles more of a program:** the graphics commands LINE, BOX, RBOX, CIRCLE, ARC, TRIANGLE, PIXEL, TEXT and COLOUR, and most built-in functions. Before, a statement with one of them in it ran as text.
  - The functions include LEFT$, MID$, INSTR, STR$, HEX$, RGB, PIXEL(), MAX, MIN, PEEK and MM.INFO().
  - The compiled code works out their arguments and hands the command or function the values, so a program that draws a lot gains most. For example, a Game of Life program that draws with PIXEL and LINE runs 2.4 times faster compiled than it did with 7.00.00b4: a row takes 7.8 ms, against 18.8 ms before and 20.2 ms without the option.
- **RUN f$:** the program's name can be a string expression, as on the PicoMite. For example: RUN f$, RUN "GAME" + STR$(n), or RUN MM.INFO$(PATH) + "MENU", -x.
  - The name is read as an expression when it has a string variable or function in it, or when something other than a comma follows a quoted name.
  - Anything else is passed to the program in memory, as before: RUN -f still runs it with "-f" in MM.CMDLINE$.
  - The text after the comma is still passed to the program as it is written.
- **MM.MOUSE** gives the channel of the mouse that is connected, for CONTROLLER MOUSE OPEN, MOUSE() and DEVICE(MOUSE ...), so a program need not be written for one kind of mouse.
  - It is 2 for a USB mouse.
  - Otherwise it is the channel of a mouse already open, or the channel set with OPTION MOUSE.
  - It is -1 if there is no mouse.
  - For example: `m = MM.MOUSE : CONTROLLER MOUSE OPEN m, LeftClick : x = MOUSE(X, m)`.
- **MM.INFO()** has the PicoMite's:
  - **LINE:** the number of the program line being run, as a string. It is the number an error there would report. It is "LIBRARY" in the library's code and "UNKNOWN" at the command prompt.
  - **CALLTABLE:** the address of the table of firmware routines that a CSUB calls, in the PicoMite's layout.
  - **MAX VARS:** the most variables a program can have.
  - **SYSTEM HEAP:** the free space of the firmware's own memory, which the audio, the JSON functions and the file system use.
  - **OPTION WIDTH and OPTION HEIGHT:** the console's columns and rows.
  - **OPTION AUDIO:** "DAC". With OPTION PICOMITE it is "PWM", the PicoMite's name for audio that works as the Colour Maximite 2's does.
  - **OPTION KEYBOARD:** the keyboard layout under the PicoMite's name for it. This is "GR" for the German layout, and "" when the keyboard is disabled.

## What behaves differently

- **A variable can have the name of a command,** so a program written before the command was added runs as it did. This was found with NOMIS: its `flash = 1` gave "Syntax" once FLASH became a command.
  - At the start of a statement, `name = value` (or `name$`, `name%` or `name!`) assigns to the variable.
  - The exceptions are TIMER, TIME$, DATE$, FLAGS and CASE, where `NAME =` is the command itself, and REM, RUN and DATA.
  - An array cannot have a command's name, as `IF (x) = y THEN` is the IF command.
  - A name that is also a keyword or a function (FOR, GOTO, SIN) is still not allowed.
- **A mode change puts the mouse at the centre of the new screen,** and the GUI cursor's position too.
  - Before, a USB mouse was centred only when it was plugged in. A program's cursor started at the centre of the start-up mode, such as (400, 300), and the mouse could be off a smaller screen until it moved.
  - An I2C mouse is told the new screen's size as well. Before, it kept to the old screen's size until it was opened again.
- **DEVICE(MOUSE n, ...), MOUSE INTERRUPT and MOUSE SET** accept a channel from 0 to 4. They act on the mouse in use, whatever the channel.
- **GETSCANLINE in the 4-bit and 1-bit depths:** in the modes whose lines are drawn twice (3, 5, 6, 7, 12, 13 and 17), it now counts the mode's own lines, as the other depths always have. For example, MODE 7,4 now counts to 249, not 499.
- **GETSCANLINE with OPTION PICOMITE:** the doubled modes count the monitor's lines, as on the PicoMite, so MODE 2 counts to 499.
- **Print Screen and Ctrl-\\** save the screen as it is shown:
  - the page on display, when a program shows one page while it draws on another;
  - in the 12-bit depth, both layers blended over the background colour.

  Before, they saved the page being drawn on, and in the 12-bit depth only page 0.

## Fixed since 7.00.00b4

- **MM.INFO(SOUND)** gives "MODFILE" again while a module plays, and a bare "PAUSED" while anything is paused, as on 6.00. The 7.00 betas gave the PicoMite's "MOD" and "PAUSED WAV" and the like.
  - A game that checks for "MODFILE" before it plays a sound effect stopped its music at the first effect. Wolf3D, Gauntlet and Into the Darkness do this, and Into the Darkness then stopped with "Samples play over MOD file".
  - With OPTION PICOMITE, it still gives the PicoMite's strings.
- **A FOR or DO loop straight after a single-line IF's THEN or ELSE,** inside another loop, made the outer loop end at the wrong NEXT or LOOP. When the IF was false, this gave "Cannot find a matching FOR". Gauntlet has such a line. For example:

  ```
  For a = 1 To 2
    If k(a) > 0 Then For i = 1 To k(a) : Print "*"; : Next i
  Next a
  ```

- **MM.INFO(FONTHEIGHT) and MM.FONTHEIGHT in font 7** now give 10, the spacing of its lines on the screen, instead of 8, the height of its characters. A program that placed text by them, such as Z-MIM with *FONT SMALL, drew it partly over the line before.
- **A restart could leave the Colour Maximite 2 hung,** with no display and no console, until it was reset. This happened if a Ctrl-C arrived on the console, or from a keyboard, while it started up in a new video mode, such as after OPTION DEFAULT MODE.
- **An error naming a variable whose name is 32 characters long,** the longest allowed, printed stray characters after the name. This affected "Different type already declared", "Global variable already declared", "Local variable already declared" and "is not declared".
- **MM.ERRNO and MM.ERRMSG$** are kept at the command prompt after a program stops with an error. Before, the status line cleared them within a second.
- **The file manager:** holding Up or Down on a USB keyboard now repeats, to scroll a long list. Page Up and Page Down still move one page per key press.
- **A file command on C: with no USB flash drive** (MKDIR, OPEN and the like) gave "SD Card not found". It now gives "USB drive not present", as DRIVE "C:" does (MM.ERRNO 21).
- **USB messages:** a message such as "Multi-touch Connected" could be printed in a program's output. This happened when the program ran another one with RUN, while the next program was loading. Such messages are now only shown at the command prompt.

## The programs tested

These run on this beta. The ones marked with a change need it to their own code, which their authors have been sent or which is described on the Back Shed forum:

- **Wolf3D, Gauntlet and Into the Darkness:** their music now plays on through the sound effects. Into the Darkness needs no change; Wolf3D and Gauntlet have fixes of their own on GitHub.
- **Z-MIM:** runs, and 1.8 times faster with OPTION COMPILE ON. Its fixes are on GitHub.
- **The Welcome Tape and SP Tools:** run, including the programs they start with RUN. The Welcome Tape's fixes are on GitHub.
- **NOMIS:** runs unchanged now that `flash` can be a variable. It needs MODE 1 (800 x 600), the default mode. To use any kind of mouse, change it to use MM.MOUSE.
- **BoxDot:** expects its mouse on channel 3. Change it to use MM.MOUSE.
- **Cuno:** asks for MODE 15, which is 1280 x 1024 on the Colour Maximite 2, but is laid out for 1920 x 1080. Change it to MODE 16 (a Generation 2 with the Rev V processor), and to use MM.MOUSE.

## Known problems

- With a USB touch screen connected and OPTION MAXCTRLS above 0, every statement checks for the GUI's touch input, so programs run slower: the solar eclipse benchmark takes 2.2 s compiled instead of 1.7 s, and 6.4 s as text instead of 6.2 s. OPTION MAXCTRLS 0 avoids it when no GUI controls are needed.

## The manual

The User Manual describes all of the above:

- RUN, in the commands;
- what OPTION COMPILE compiles, in the options;
- MM.MOUSE, and the new MM.INFO keywords, in the read-only variables and the reference section on MM.INFO;
- variables with a command's name, in the chapters Variables and Expressions and MMBasic Characteristics;
- MM.MOUSE and the mouse's centring, in the chapter Keyboard, Mouse and Gamepad, and under MODE;
- MM.INFO(SOUND), in the chapter Sound Output;
- GETSCANLINE, in the functions and in the chapter Video Modes and Pages.
