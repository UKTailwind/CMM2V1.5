# Colour Maximite 2: MMBasic 7.00.00RC0 (release candidate) release notes

*3 November 2026. Compared with 7.00.00b5, the previous beta. The full notes, compared with MMBasic 5.07.02, are posted with these.*

This is the first release candidate of MMBasic 7.00 for the Colour Maximite 2. Unless problems are found in it, it will become 7.00.00. Please report problems on the Back Shed forum.

It adds two things. The Colour Maximite 2 can now run programs written in C and C++, compiled for its processor: MicroPython, Doom and ScummVM come with it, and a new manual explains how to write more. And it has an FM synthesiser, an emulated OPL3, which plays MIDI files, the PicoMite's PLAY MIDI and PLAY NOTE commands without a VS1053 module, and the music of DOS games.

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

- **Options kept:** all of them. A setting of OPTION FLASH 1 becomes OPTION FLASH 2 (see below).
- **Kept:** the library, the variables saved with VAR SAVE, the flash slots (which are on the SD card), and the programs and files on the SD card.
- **Not kept:** the program held in flash, as the firmware now uses that part of the flash. RUN loads it again from its file as usual. A program run at power up by OPTION AUTORUN has to be run once with RUN after the update.

## What is new

- **Loadable programs.** A file ending .APP is a program written in C or C++ and compiled for the Colour Maximite 2's processor. RUN runs it:

  ```
  RUN "A:/micropython.app"
  CHDIR "A:/Doom" : RUN "doom.app"
  ```

  - It runs at the full speed of the processor and takes over the computer while it runs, through services the firmware gives it: the console, files, the screen, the keyboard and mouse, sound, the FM synthesiser and the I/O pins.
  - The words after RUN's comma are its arguments. When it ends, MMBasic carries on: at the command prompt it returns to the prompt, and in a BASIC program it goes on with the next statement, the program's variables as they were.
  - MMBasic tidies up after it: its files are closed, its sound stopped, its I/O pins released, and the video mode, font, colours and palette put back.
  - Ctrl-C asks it to end, and then stops the BASIC program that ran it too.
  - A Generation 2 keeps 8MB of its memory for loadable programs. A Generation 1 lends a program the largest free part of MMBasic's heap (about 4MB at the command prompt) while it runs, and a program built for it, as Doom and ScummVM are, the 3MB of display memory as well: about 7MB in all. The same program file runs on both generations.
  - Errors: "Not a loadable program", "Needs a newer firmware", "Needs a G2", "Needs n KB, n KB free", "The program file is damaged".
- **Three programs come with the firmware,** in CMM2_Programs_7.00.00RC0.zip, to copy to the SD card:
  - **MicroPython,** as on the Pico Computer 3: its display, keyboard, mouse, audio and image modules, SQLite, ulab and the machine module on the rear I/O pins. It runs Python about twice as fast as the RP2350 does.
  - **Doom,** with Freedoom, a free replacement for its game data: about 50 frames a second on a Generation 2 and 35 on a Generation 1, with its sound effects and its music on the FM synthesiser.
  - **ScummVM,** for classic adventure games (AGI, SCI, SCUMM, Gobliiins and Kyrandia), with three free games: the Freddy Pharkas demo, DG: The Adventure Game and Another DG Game. It runs on both generations. Games you own go in folders of A:/quest whose names the zip's README.txt lists, and `RUN "scummvm.app", dott` starts one at once.
- **A new manual, Colour Maximite 2 Loadable Programs,** is for programmers: the tools that build a program (in the repository's tools/sdk, with the free Arm GNU toolchain), how the firmware runs one, every service with examples, and how MicroPython, Doom and ScummVM were ported.
- **An FM synthesiser:** an emulation of the Yamaha YMF262 (the OPL3 of the Sound Blaster cards), by ymfm.
  - **PLAY MIDIFILE file$ [, interrupt]** plays a Standard MIDI File.
  - **PLAY MIDI** turns it into a General MIDI instrument for **PLAY NOTE ON** and **PLAY NOTE OFF** and **PLAY MIDI CMD**, the PicoMite's commands for its VS1053 module. **PLAY MIDI TEST** plays a tune kept in the firmware.
  - Each of the 128 General MIDI instruments and each drum is an FM instrument of the GENMIDI bank in the firmware (Freedoom's, the bank Doom's music was written for), so MIDI music sounds as it did on a DOS game's Sound Blaster.
  - **PLAY OPL file$ [, interrupt]** plays music recorded from an OPL chip: IMF (Wolfenstein 3D, Commander Keen), DOSBox's DRO and VGM files.
  - **PLAY OPL** on its own, **PLAY OPL WRITE** and **PLAY OPL RESET** let a program write the chip's registers itself, and **PLAY OPL TEST** measures its share of the processor.
  - It plays in the background as the other sounds do, and PLAY PAUSE, RESUME, STOP and VOLUME work on it. MM.INFO$(SOUND) gives "OPL" or "MIDI", and MM.INFO$(TRACK) the file's name.
  - It takes a share of the processor while it plays: about 20% with no notes sounding and 30% to 55% for typical music on a Generation 2. A BASIC program keeps about three quarters of its speed.

## What behaves differently

- **OPTION FLASH** takes 2 to 6, and 2 is the default. The firmware has grown into the first part of the flash that programs used.
- **MM.INFO(HEAP)** on a Generation 2 is 8MB less (about 15.9MB), as that memory is kept for loadable programs.

## Fixed since 7.00.00b5

- **With a USB touch screen and OPTION MAXCTRLS above 0, programs ran slower,** as every statement checked for the GUI's touch input. The GUI is now seen to once a millisecond: the solar eclipse benchmark takes 1.70 s compiled with the GUI running, as it does without it.
- **RUN's arguments after a ':' or in a single-line IF** reach the program as they are written. In `CHDIR "A:/Doom" : RUN "doom.app", -timedemo demo1` the program was given them in capitals and with the minus sign changed (MMBasic's stored form of the line), and so was MM.CMDLINE$ after such a RUN in a program. RUN at the start of a line was always right.
- **After PLAY WAV (or FLAC, MP3) and PLAY STOP,** the sound output kept the number of the file it had closed. A file the program opened next could be given that number, and the next sound command closed it.

## The programs tested

- The regression tests: tests/cmm2 (113 programs), the PicoMite's tests, the goldens (202 programs) and the Testfiles (66), each with OPTION COMPILE OFF, ON and SHADOW; SP Tools; the editor's and the file manager's tests. All give the results they gave with 7.00.00b5, except the free memory that three of the Testfiles print, now 8MB less on a Generation 2.
- On a Generation 1, tests/cmm2 runs as on a Generation 2, apart from what the Generation 1's hardware does not have (32-bit colour, the mouse in use) and rgb121 (see Known problems).
- MicroPython, Doom and ScummVM on both generations (ScummVM on the Generation 2), including the full Day of the Tentacle.

## Known problems

- **On a Generation 1, the test program rgb121 stops responding in MODE 8,4** when it shows three layers (FRAMEBUFFER LAYER TOP) and waits for a frame. It is in 7.00.00b5 too. A reset is needed.
- **With a USB touch screen connected, a mouse click can be taken as a swipe** by the GUI controls' gestures, as the click's end is taken from the touch screen.
- An armed interrupt pin slows the program down, because the pin is polled after every statement.

## The manual

The User Manual describes all of the above:

- RUN of a .APP file, in the commands, and the chapter Using MMBasic's new section Loadable Programs;
- the FM synthesiser, in the chapter Sound Output's new section FM Synthesis and MIDI, and PLAY MIDI, PLAY NOTE, PLAY MIDI CMD, PLAY MIDI TEST, PLAY MIDIFILE and PLAY OPL in the commands;
- MM.INFO$(SOUND) and MM.INFO$(TRACK), in the read-only variables;
- the memory kept for loadable programs, in the chapter MMBasic Characteristics.

Appendix O lists the new companion manual, Colour_Maximite_2_Loadable_Programs.pdf, posted with this release.

## Licences

The FM synthesiser is ymfm, and its MIDI instrument bank and PLAY MIDI TEST's tune come from Freedoom. Both are under the BSD 3-Clause licence, whose notices follow:

> **ymfm:** Copyright (c) 2021, Aaron Giles. All rights reserved.
>
> **Freedoom:** Copyright (c) 2001-2024 Contributors to the Freedoom project. All rights reserved.
>
> Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
>
> 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
> 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
> 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
>
> THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The programs in CMM2_Programs_7.00.00RC0.zip have their own licences, listed in its README.txt: Doom and ScummVM are under the GNU GPL, and their source is in the firmware's repository (apps/doom and apps/scummvm) with the upstream projects at the commits named there.
