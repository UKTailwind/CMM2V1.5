# Introduction

The Colour Maximite 2 is a small self contained computer inspired by the home computers of the early 80's such as the Tandy TRS-80, Commodore 64 and Apple II. It includes its own BASIC interpreter and powers up in under a second into the BASIC interpreter (there is no operating system to boot). The emphasis is on ease of use and, as a result, a first time user could enter a small program and have it running within minutes.

While the concept is borrowed from computers of the 80's the technology used is very much up to date. The CPU that powers the Colour Maximite 2 is an ARM Cortex-M7 32-bit RISC processor running at up to 480MHz with 2MB flash memory and 1MB RAM. This processor includes its own video controller and generates a VGA output at resolutions up to 1920x1080 pixels and with up to 24-bit colour.

The Colour Maximite 2 is designed to be simple and fun. The assembly instructions and the firmware are completely free and the parts can be found from multiple sources. It can be assembled in an hour or two and will provide endless hours of fun.

The basic features of the Colour Maximite 2 are:

- **Low cost affordable fun.** The firmware (including the BASIC interpreter) is completely free. The main PCB is easy to assemble with thru hole components. The CPU and support circuits are contained on a fully assembled plug in board costing US$30. The firmware can be loaded using free software so a programmer or special equipment is not required to get started.
- **Instant startup into the BASIC interpreter.** Program space is 516KB, enough for huge and complex programs (typically 25,000 lines or more), while the general RAM used for arrays and buffers is over 5MB on the Generation 1 and over 24MB on the Generation 2 (enough for enormous arrays).
- **Full featured BASIC interpreter** with double precision floating point, 64-bit integers and string variables, long variable names, arrays of floats, integers or strings with multiple dimensions, user defined structures, extensive string handling and user defined subroutines and functions. Typically it will execute a program at 270,000 lines per second.
- **Rock solid VGA output** (or HDMI with an inexpensive converter), with 18 program selectable video modes from 1920x1080 pixels to 240x216 pixels and up to 24-bit colour (16 million colours).
- **USB keyboard, mouse and gamepads.** The keyboard can be wireless (with a USB dongle) or wired and have UK, US, DE, FR, ES or BE key mappings. With a USB hub the same socket also takes a mouse, gamepads, USB serial devices and a USB flash drive. A PS2 mouse or a dual USB/PS2 mouse can be used as well.
- **Stereo audio output** can play WAV, FLAC and MP3 files, computer generated music (MOD format), BBC Micro style sound, and robot speech and sound effects as well as generate precise sine wave tones.
- **A full screen editor** is built into the firmware. It includes advanced features such as colour coded syntax, search and replace, cut, copy and paste with a clipboard, lines longer than the screen, automatic indenting and full mouse support. With one key press the program can be saved and run. If an error occurs the EDIT command will return to the editor with the cursor placed on the line that caused the error.
- **Full support for SD cards** including editing and running programs on the SD card as well as opening files for reading, writing or random access. Cards up to 128GB formatted in FAT16, FAT32 or exFAT are supported and the files can also be read and written on personal computers running Windows, Linux or the Mac operating system. A USB flash drive can be used as a second drive (C:). A two-panel file manager (FM) is included in MMBasic.
- **Programs can be easily transferred** from another computer (Windows, Mac or Linux) using the SD card, a USB flash drive, the XModem or ZModem protocols or by streaming the program over the serial console input.
- **Extensive features for creating computer games.** These include multiple video planes, support for Blits, Sprites and tile maps, the ability to create computer generated music, sound effects and computer generated speech. The Colour Maximite 2 includes full support for the Wii Nunchuk and Wii Classic games controllers and for USB gamepads.
- **Battery backed clock** will keep the correct time, even with the power disconnected.
- **Twenty eight input/output pins** with 12 capable of analog input. Built in support for an IR remote control and temperature and humidity sensors. Communications protocols include I2C, asynchronous serial, RS232, IEEE 485, SPI, 1-Wire and CAN. These can be used to communicate with many sensors (temperature, humidity, acceleration, etc) as well as for sending data to test equipment.
- **Power** is 5 volts at 220mA typical from a USB port or charger.

## What is New in Version 7.00

Version 7.00 brings MMBasic on the Colour Maximite 2 up to date with MMBasic version 7 on the PicoMite. The language is the same on both computers, apart from the hardware that one of them does not have, so most programs written for the PicoMite can be run on the Colour Maximite 2 with few changes (see the chapter Porting Programs). In particular:

- **The interpreter** is the PicoMite's version 7. It runs programs faster and adds user defined structures (TYPE), a program library (LIBRARY), CHAIN, SAVE CONTEXT and LOAD CONTEXT, single element arrays, the full MATH command and many other commands and functions.
- **USB devices** use the PicoMite's USB support: a hub, mice, gamepads (including PlayStation, Xbox and Nintendo Switch Pro controllers), USB serial devices as COM4 to COM7 and a USB flash drive as the C: drive.
- **The file manager (FM)** is the PicoMite's two-panel file manager, which copies between the SD card and a USB flash drive.
- **The full screen editor** is the PicoMite's, with the Colour Maximite 2's mouse support and extra keys.
- **Files** can be transferred with the ZModem protocol as well as XModem.
- **Graphics** gain the PicoMite's tile maps (TILEMAP), image slots, its 16 colour display (MODE n,4) and its MODE 1 (MODE n,1), so that games written for the PicoMite need few changes.

!NOTE This manual is for version 7.00.00b3, the third beta of version 7.00. Chapters that have not yet been revised line by line for this version say so at their start.
