# Introduction

Your Colour Maximite 2 can run much more than MMBasic. This guide covers the programs that come with the firmware: games, emulators, a C compiler, MicroPython, Forth and a terminal. It tells you how to install each one, start it, use it and leave it.

You do not need to be a programmer to use any of them, apart from the C compiler, MicroPython and zeptoforth. If you want to write programs of this kind yourself, that is what the companion manual *Colour Maximite 2 Loadable Programs* is for.

## What Is Here

{widths: 13,27,12}
| Program | What it does | Folder |
| MicroPython | Python, with the screen, sound and I/O pins | A:/ |
| Doom | The classic game, with the free Freedoom game data | A:/Doom |
| Heretic | Raven Software's game, with its shareware episode | A:/heretic |
| Quake | id Software's game, with its shareware episode | A:/quake |
| ScummVM | Classic adventure games from LucasArts, Sierra and others | A:/quest |
| NES | A Nintendo Entertainment System emulator, with two free games | A:/nes |
| ZX Spectrum | A 48K and 128K Spectrum emulator | A:/spectrum |
| Amstrad CPC | A 464, 664 and 6128 emulator | A:/cpc |
| Commodore 64 | A C64 emulator, with its 1541 disk drive | A:/c64 |
| Atari 8-bit | The Atari 400/800, XL/XE and 5200 emulator atari800 | A:/atari800 |
| 386 PC | A PC that runs FreeDOS, MS-DOS, Windows 3.1 and Windows 95 | A:/pc386 |
| C compiler | Compiles C and MMBasic programs on the machine itself | A:/cc |
| VT100 terminal | Makes the Colour Maximite 2 the screen and keyboard of a PicoMite or other computer | A:/vt100 |
| zeptoforth | A Forth with tasks, double precision and a screen editor | A:/zeptoforth |

All of them run on both the Generation 1 and the Generation 2, from the same files, except zeptoforth, which has a file for each, and Quake, which needs a Generation 2.

## Installing the Programs

The programs are in the file CMM2_Programs_7.00.00RC4.zip, which is posted with the firmware.

1. Update the firmware first. Some of these programs need the firmware of the same release.

2. Unzip the file on a PC.

3. Copy everything in it to the root of the SD card, keeping the folders. Use a card reader: some of the files are tens of megabytes, which would take many minutes over the serial console.

You need only copy the folders you want. If you do not want Doom, leave out the Doom folder. Keep the licences folder with the programs if you pass them on to anyone else.

## Starting a Program

A program is a file ending .app, and MMBasic runs it with RUN. Change to its folder first, as most of them look there for their files and keep their settings there:

```
CHDIR "A:/nes"
RUN "nes.app"
```

Words after a comma are passed to the program:

```
RUN "scummvm.app", dott
```

Do not put them in quotes. Each chapter of this guide says what its program accepts.

A BASIC program can start one too. MMBasic carries on with the next statement when it ends, with the program's variables as they were.

## Leaving a Program

Programs differ in how you leave them, as an emulator has to pass most keys on to the machine it is emulating:

{widths: 16,36}
| Program | How to leave it |
| MicroPython | machine.reset() at its prompt |
| Doom, Heretic, Quake | Ctrl-C |
| ScummVM | Ctrl-C. In a game, Ctrl+Alt+Del goes back to the list of games |
| NES | Ctrl+Alt+End, or Ctrl-C |
| ZX Spectrum, Amstrad CPC, Commodore 64, Atari 8-bit | Ctrl+Alt+End (F9 too on the Atari). Ctrl-C is a key of the emulated machine |
| 386 PC | Ctrl+Alt+End. See its chapter first, as this switches the PC off |
| VT100 terminal | Shift-F12, then Quit. Ctrl-C goes to the computer at the other end |

When a program ends, MMBasic tidies up after it: its files are closed, its sound is stopped, its I/O pins are released, and the video mode, font, colours and palette are put back.

## What the Programs Use

**The keyboard and mouse.** All the programs use a USB keyboard. Those that use a mouse take a USB mouse, or a PS/2 mouse set up with OPTION MOUSE.

**USB gamepads.** The NES, Spectrum, CPC, C64 and Atari emulators use them. A DualShock 3 sends nothing until its PS button has been pressed. A generic pad is usually a low speed device, and has to go straight into the Colour Maximite 2, not through a hub.

**The screen.** A program sets the video mode it needs, and MMBasic puts yours back when it ends. Your monitor may take a moment to lock on each time.

**Memory.** A Generation 2 keeps 8MB of its memory for these programs. A Generation 1 lends a program the largest free part of MMBasic's memory while it runs, and some programs borrow the display memory too, so on a Generation 1 the screen can change as a program starts.

## If a Program Will Not Start

{widths: 18,34}
| Message | What it means |
| Needs a newer firmware | The program needs a later version of the firmware than you have. Update it. |
| Not a loadable program | The file is not a program, or is not the one you meant. |
| The program file is damaged | Copy it to the card again. |
| Needs a G2 | The program needs a Generation 2. Of the programs in this guide, only Quake does. |
| Needs n KB, n KB free in one block | There is not enough memory. Restart the Colour Maximite 2 and try it from the command prompt, not from inside a BASIC program. On a Generation 1, OPTION RAM leaves less: use OPTION FLASH. |

If a program cannot find its files, you have probably started it from the wrong folder. CHDIR to its folder and try again.
