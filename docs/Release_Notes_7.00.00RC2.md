# Colour Maximite 2: MMBasic 7.00.00RC2 (release candidate) release notes

*5 November 2026. Compared with 7.00.00RC0, the last version released: 7.00.00RC1 was prepared but not released, and its changes are here. The full notes, compared with MMBasic 5.07.02, are posted with these.*

This is the third release candidate of MMBasic 7.00 for the Colour Maximite 2. Unless problems are found in it, it will become 7.00.00. Please report problems on the Back Shed forum.

It adds a C compiler on the Colour Maximite 2 itself, which also compiles MMBasic programs; pc386, a 386 PC that runs FreeDOS, MS-DOS and Windows 3.1; a NES emulator; and Heretic. It also makes OPTION SD TIMING FAST much faster, and fixes OPTION OVERCLOCK, which stopped a Generation 1 from responding.

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

The flash is laid out differently in this version, so the first start after the update does some work. It takes a few seconds longer than usual: please let it finish.

- **Options kept:** all of them, from 7.00.00b4 or later. Earlier notes said this of the betas and RC0, and it was not so: an update then reset most of them. This one keeps them. An OPTION OVERCLOCK ON saved by an earlier version is left out on a machine where it no longer applies (see below).
- **Kept:** the library, the variables saved with VAR SAVE, the flash slots (which are on the SD card), a program held in flash with OPTION FLASH 3 to 6, and the programs and files on the SD card.
- **Not kept:** the program held in flash with the default OPTION FLASH 2, as the firmware now uses that part of the flash. RUN loads it again from its file as usual, and a program run at power up by OPTION AUTORUN has to be run once from its file.

**From this version on, writing the firmware erases the options.** They now share a sector of the flash with the runtime of compiled programs. The first update, to this version, moves them for you. For later ones, use the new OPTION DISK SAVE before you update and OPTION DISK LOAD afterwards. The keyboard layout and the time are kept without it.

## What is new

- **The C compiler.** CC compiles a program on the Colour Maximite 2, with no PC:

  ```
  CC "hello.c"
  RUN "hello.bc"
  ```

  - It is the Fuzix Compiler Kit, with the bytecode and Arm (Thumb-2) code generator written for the Pico Computer 3. A small program compiles in under a second, and its floating point runs on the processor's double precision hardware.
  - RUN "hello.c" compiles the file if it needs it, and runs it. A compiled program can use the graphics, sound, keyboard, mouse and I/O pins through cmm2.h.
  - The compiler is in the cc folder of CMM2_Programs_7.00.00RC2.zip, which goes on the card as A:/cc. What runs a compiled program is in the firmware, so a compiled program needs nothing from the card.
- **Compiled MMBasic.** CC compiles an MMBasic program too:

  ```
  CC "game.bas"
  RUN "game.b.bc"
  ```

  - The program is translated to C and compiled. Your .bas file is not changed, and RUN "game.bas" still runs it with the interpreter.
  - A program that spends its time calculating gains the most. A 3,200-line program that predicts eclipses took 6.16 seconds with the interpreter, 1.71 with OPTION COMPILE ON and 0.93 compiled, and its output was the same.
  - A game gains less, as its drawing takes the same time however it is run. Wolfenstein 3D ran at 28.5 frames a second with the interpreter, 79.7 with OPTION COMPILE ON and 90.2 compiled. Into the Darkness, which spends most of its time drawing, ran at 17.8, 37.5 and 37.9.
  - A line that CC cannot translate is left out and reported, so read what it says and compare the result with the BASIC program.
  - A new manual, Colour Maximite 2 C Compiler, describes all of this.
- **pc386, a 386 PC.** It is tiny386, Chunhui He's 386 PC emulator, with a front end for the Colour Maximite 2:

  ```
  CHDIR "A:/pc386"
  RUN "pc386.app"
  ```

  - The PC has a 386 processor with a maths coprocessor, 4MB of memory (2.75MB on a Generation 1), a VGA card shown at 640 x 480 in 256 colours, a Sound Blaster 16, an Adlib and the PC speaker. The Colour Maximite 2's USB keyboard and its USB or PS/2 mouse are the PC's.
  - Its hard disks and floppy disks are image files on the SD card. Ctrl+Alt+Page Down and Page Up change the disk in drive A: from a list, as you would change disks when an installation asks for the next one. Ctrl+Alt+End leaves.
  - It comes with FreeDOS 1.3 installed on a hard disk image, ready to run. MS-DOS 6.2 and Windows 3.1 install from images of your own disks, and run with mouse and sound.
  - FreeDOS starts in about 35 seconds on a Generation 2, and Windows 3.1 is usable. On a Generation 1 of revision Y, OPTION OVERCLOCK ON (below) makes it as fast as on a Generation 2.
  - A new manual, Colour Maximite 2 386 PC Emulator, describes its keys, the configuration file of each PC, disk images, moving a PC to another Colour Maximite 2, getting your own programs onto its disks, and installing MS-DOS and Windows 3.1.
- **A NES emulator.** It is frank-nes, Mikhail Matveev's emulator, on the QuickNES core by Shay Green:

  ```
  CHDIR "A:/nes"
  RUN "nes.app"
  ```

  - It runs at full speed on both generations, with about 80% of the processor to spare, in MODE 7,8 with the Colour Maximite 2's own sound.
  - It uses USB gamepads, a DualShock 3 or a generic pad among them, and the USB keyboard. Ctrl+Alt+End leaves, as it does pc386.
  - Two free games come with it, Nova the Squirrel and Thwaite, in the nes folder of the zip. Any other .nes file you have the right to use goes in A:/nes.
- **Heretic.** Raven Software's game, through frank-heretic by Mikhail Matveev, on the same layer as Doom, with its music on the firmware's FM synthesiser:

  ```
  CHDIR "A:/heretic"
  RUN "heretic.app"
  ```

  - It comes with Heretic's shareware episode, in the heretic folder of the zip. If you have the full game, put its heretic.wad in the folder, and it is used in place of the shareware.
- **OPTION DISK SAVE fname$ and OPTION DISK LOAD fname$** save the options to a file and load them back, with the flash library if there is one.
- **OPTION SD TIMING FAST** now drives the SD card with the processor's SPI hardware directly. A 32MB copy on a Generation 2 takes 37 seconds, against 51 seconds with RC0's FAST and 59 seconds with NORMAL.
- **Loadable programs: services version 13.**
  - keys_raw gives a program the USB keyboard's own key codes, the keys' places before any keyboard layout.
  - gamepad gives a program the USB gamepads, as DEVICE(GAMEPAD n, B) gives them in BASIC.
  - Drawing by MMBasic's own routines (gfx_cls to gfx_save), and the firmware's memory routines and maths library.
  - basic_cmd, basic_eval, basic_prep and basic_exec give a program MMBasic's statements and expressions.
  - A file opened to read and write in place now seeks as fast as one opened only to read.
  - The Loadable Programs manual describes them.

## What behaves differently

- **OPTION OVERCLOCK ON** now runs an original Colour Maximite 2 whose processor is of revision Y at 480MHz, in place of 400MHz: the speed at which every other Colour Maximite 2 runs. It gives an error on a Generation 2 ("G1 only") and on an original that runs at 480MHz already.
- **The flash library and OPTION FLASH** are one sector higher in the flash. Their numbers are as they were, and the first start after the update moves what they hold.

## Fixed since 7.00.00RC0

- **OPTION OVERCLOCK ON stopped a Generation 1 from responding.** It started and showed its screen, but took no keys from the keyboard or the serial console. At 504MHz its processor ran 26% above its rating, and computed wrong values.
- **An update reset most of the options**, although the notes of the betas and RC0 said that they were all kept. This update keeps them, as above.
- **A loadable program found no PS/2 mouse** unless a BASIC program had used it first, because the mouse had not answered at power up. ScummVM had no mouse. The program now starts it, as MM.MOUSE does.
- **The PS/2 mouse** now starts each byte afresh if 5 milliseconds pass without the rest of it, as the PicoMite's does, so that one missed clock pulse no longer garbles the bytes after it.

## The programs tested

- The regression tests in tests/cmm2 (113 programs) on a Generation 2: the same results as 7.00.00b5. Three programs whose expected output was recorded with OPTION DEFAULT MODE 8 differ with the board's own setting (DEFAULT MODE 1,8).
- The update itself, on both generations: from RC0 on a Generation 2, with a library and a CSUB program at OPTION FLASH 3, and with OPTION AUTORUN and VAR SAVE; on a Generation 1 from a firmware of the old layout, with its own options; from an options record of the older form; and a second update of this version over itself, with OPTION DISK SAVE and LOAD.
- The C compiler on both generations: compiling and running C and MMBasic, with Wolfenstein 3D and the eclipse program on both, and Into the Darkness on a Generation 2. Earlier in its development its C89 conformance tests gave the same results as the compiler on a PC.
- The NES emulator: its speed on both generations; on a Generation 2, a DualShock 3 and a generic pad, Nova the Squirrel, and the CPU test programs nestest, official_only and cpu_timing_test, which it passes.
- pc386 on both generations: FreeDOS; MS-DOS 6.2 installed from its Setup disks; Windows 3.1 installed from its six disks and run with sound, a USB mouse and a PS/2 mouse; Turbo Pascal 5.5.
- Heretic with its shareware episode, on both generations.
- MicroPython, Doom, ScummVM and pc386 from the zip on this firmware, on both generations.

## Known problems

- **With a USB touch screen connected, a mouse click can be taken as a swipe** by the GUI controls' gestures, as the click's end is taken from the touch screen.
- An armed interrupt pin slows the program down, because the pin is polled after every statement.
- In pc386, a program protected by a check of its original floppy disk (Lemmings is one) does not run from a disk image.
- **A very large MMBasic program** (4,300 lines) compiles on a Generation 2 but stops with an error on a Generation 1, which has less memory for the compiler.
- A compiled Petscii Robots may pause its music at times.
- **The NES emulator:** a low speed USB pad cannot go through a hub, as the firmware does not support that. A pad already plugged in when the Colour Maximite 2 was restarted by a reset, not a power cycle, has been seen to give no response until it was plugged in again. QuickNES fails the most exacting timing tests (apu_test 4 of 8, ppu_vbl_nmi 3 of 10), which show in few games.

## The manual

The User Manual describes:

- OPTION DISK SAVE and LOAD, OPTION SD TIMING and OPTION OVERCLOCK as they now are, in the options;
- CC, for C and for MMBasic, in the commands and the chapter Using MMBasic;
- pc386, among the programs that come with the firmware, in the chapter Using MMBasic;
- the flash as it now is, in the chapter MMBasic Characteristics.

Appendix O lists the two new companion manuals, Colour_Maximite_2_C_Compiler.pdf and Colour_Maximite_2_386_PC_Emulator.pdf, posted with this release. Colour_Maximite_2_Loadable_Programs.pdf adds the services up to version 13.

## Licences

The compiler is free software under the GNU General Public License version 3. It is a separate program, cc.app, and is not part of the firmware. Its source is in github.com/UKTailwind/FUZIX, branch cmm2. The runtime of compiled programs, in the firmware, is not under that licence, and the translator from MMBasic to C and its runtime are under the MMBasic licence.

Heretic's program is under the GNU General Public License version 2. Its source is github.com/rh1tech/frank-heretic at the commit named in apps/heretic/make.py, with the files in apps/heretic. The shareware episode is not free software: it is copyright Raven Software and id Software, and comes unchanged, with its LICENSE.DOC, which allows it to be passed on free of charge. It may not be sold.

The NES emulator is under the GNU General Public License version 3, and its QuickNES core under the GNU LGPL 2.1 or later. Its source is github.com/rh1tech/frank-nes at the commit named in apps/nes/make.py, with the patch and the files in apps/nes. Nova the Squirrel's code is under the GNU General Public License version 3 or later, and its graphics and levels under Creative Commons BY-NC-SA 4.0. Thwaite is under the GNU General Public License version 3 or later.

pc386 is tiny386, under the BSD 3-Clause licence, whose notice follows. Its emulated devices come from QEMU and TinyEMU (MIT licence), its Adlib's FM synthesis is fmopl (GNU LGPL 2.1 or later), its BIOS files are SeaBIOS and SeaVGABIOS (GNU LGPL 3), and FreeDOS is under the licences of its programs, most of them the GNU General Public License.

> **tiny386:** Copyright (c) 2024-2025, Chunhui He. All rights reserved.
>
> Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
>
> - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
> - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
> - Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
>
> THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The licences of the programs from RC0 (MicroPython, Doom, ScummVM, ymfm and Freedoom) are unchanged.
