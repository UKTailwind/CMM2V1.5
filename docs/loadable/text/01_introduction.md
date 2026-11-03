# Introduction

A loadable program is a program written in C or C++, compiled on a PC for the Colour Maximite 2's processor and run from the SD card or a USB flash drive with MMBasic's RUN command:

```
RUN "A:/apps/hello.app", any words
```

It runs at the full speed of the processor, an STM32H743 (an Arm Cortex-M7 at 480MHz, or 400MHz with the earlier revision of the chip that many Generation 1 boards have, with a double precision floating point unit). MMBasic stays where it is in flash and the program reaches the computer's hardware through a table of services that the firmware gives it: the console, the clock, the files on the SD card and USB flash drive, the screen, the keyboard and mouse, the sound output, an FM synthesiser and the I/O pins. When the program ends MMBasic tidies up after it and carries on: at the command prompt it returns to the prompt, and in a BASIC program it goes on with the next statement, with the BASIC program and its variables as they were.

This makes the Colour Maximite 2 a platform for software that would be too slow or too large in BASIC, and for software that already exists in C: games, emulators and other programming languages.

## What Has Been Done This Way

Three large programs are included with the firmware, and they show what is possible:

- **MicroPython**, the Python 3 of microcontrollers, as on the Pico Computer 3: its display, keyboard, mouse, sound and image modules, SQLite and ulab (a NumPy for MicroPython), and the I/O pins through the machine module. It runs Python about twice as fast as the RP2350 does.
- **Doom**, through doomgeneric, with the Freedoom game data: the original 320 x 200 game with its sound effects and its music on the FM synthesiser, at about 50 frames a second on a Generation 2.
- **ScummVM**, the interpreter for the classic adventure games of LucasArts, Sierra and others (from frank-quest, its port to the RP2350): the AGI, SCI, SCUMM, Gobliiins and Kyrandia engines, with music and speech. It needs a Generation 2.

Each was ported with the tools and services described in this guide, and the chapter Porting Existing Software describes how.

## What You Need

- A Colour Maximite 2, Generation 1 or 2, with firmware 7.00.00RC0 or later.
- A PC running Windows, Linux or macOS with Python 3 and the Arm GNU toolchain for bare metal targets (arm-none-eabi-gcc). The firmware is built with GCC 13.3 (ST's or Arm's own) and that is the version to use.
- The firmware's repository, which holds the tools (tools/sdk), the header that describes the services (Inc/cmm2img.h) and the example programs: `git clone https://github.com/UKTailwind/CMM2V1.5`
- A way to put files on the SD card: a card reader, or XMODEM or ZMODEM over the serial console.

## Generation 1 and Generation 2

The same program runs on both generations. The differences that matter to a program are its speed and its memory:

{widths: 30,35,35}
| | Generation 1 | Generation 2 |
|---|---|---|
| Processor clock | 400MHz or 480MHz, by the chip's revision | 480MHz |
| Memory for the program | about 4MB, borrowed from MMBasic's heap while the program runs | 8MB set aside for it |
| Where the program is loaded | where the free memory is: it is moved there | 0xD1800000, where it was linked |
| Video modes | as for BASIC: no MODE 16 and no 32-bit colour | all |

A program too large for a Generation 1 is built with the --g2 option, and a Generation 1 then refuses it with the error "Needs a G2". ScummVM is built this way.

## This Guide

The chapter Getting Started builds and runs a first program. How a Program Runs describes what the firmware does with a program from RUN to its end, and Building Programs the tools that make one. The C Library covers the standard C functions a program can use. The Services and the four chapters after it describe the services, with examples: the screen, the keyboard and mouse, sound and music, and the I/O pins. The last chapters cover performance, debugging and porting existing software. The appendices list the services, describe the program file's format and give the licences.
