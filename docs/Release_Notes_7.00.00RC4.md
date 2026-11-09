# Colour Maximite 2: MMBasic 7.00.00RC4 (release candidate) release notes

*9 November 2026. Compared with 7.00.00RC3, the last version released. The full notes, from 5.07.02, are in Release_Notes_7.00.00.md.*

This is the fifth release candidate of MMBasic 7.00 for the Colour Maximite 2. Unless problems are found in it, it will become 7.00.00. Please report problems on the Back Shed forum.

It adds an Atari 8-bit emulator to the programs, lets a program set OPTION PICOMITE for itself alone, gives programs a gamepad's right stick and analogue triggers, and accepts the USB serial bridge on a Generation 1 (the Microbridge).

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

**Writing the firmware erases the options.** Save them first, and load them again afterwards:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

The console runs at 115200 baud until the options are loaded. The library, the variables saved with VAR SAVE and the files on the SD card are kept.

Copy the new CMM2_Programs_7.00.00RC4.zip to the card as well: atari800 is new, and the c64 folder was added to RC3's zip on 8 November.

## What is new

- **An Atari 8-bit emulator.** atari800 7.2.1, the Atari800 development team's emulator, in the folder atari800: the Atari 400 and 800, the XL and XE computers and the 5200 console, at the Atari's own speed in MODE 7, with its own menus (F1) for disks, cartridges, tapes and the machine. AltirraOS and Altirra BASIC, Avery Lee's free replacements for Atari's ROMs, are built in, so it needs nothing else; Atari's own ROMs are used if you put them in its folder. USB gamepads are its joysticks, with a single pad's right stick as the second joystick for twin-stick games, and the mouse can be paddles, an ST or Amiga mouse, a trak-ball, a light pen or a Koala pad. The keys follow OPTION USBKEYBOARD. The App User Guide has a chapter on it.

  ```
  CHDIR "A:/atari800"
  RUN "atari800.app"
  ```

  It has been tried on a Generation 2 only so far.

- **The Commodore 64 emulator,** added to RC3's zip on 8 November, is in this zip too: frank-c64, a PAL C64 with its 1541 disk drive, in the folder c64.

- **zeptoforth 1.17.2,** Travis Bemann's release of 9 November, in place of the development version in RC3's zip: its fixed-point exp and expm1 are more accurate, and its own fix for the cursor on leaving zeptoed replaces ours. Its saved Forth systems are new, so copy the whole zeptoforth folder.

- **OPTION PICOMITE in a program is for that program.** At the command prompt the option is saved, as before. Run in a program it now works, where it gave the error "Invalid in a program", and lasts until the program ends: the display changes at once to the PicoMite's MODE 1 (or, with NONE, to the Colour Maximite 2's mode), and when the program ends, with END, an error or Ctrl-C, the saved setting is in force again. So a PicoMite program can start with OPTION PICOMITE HDMI and run the same whether or not the option is saved. It is not saved even if the program saves other options, and OPTION LIST shows the saved setting.

- **EDIT's mark mode:** End pressed twice (or Ctrl-K twice) marks to the end of the program, as it moves there in edit mode. End once still marks to the end of the line.

- **For programmers:** version 16 of the services adds gamepad_axes, which gives a pad's right stick and its analogue L2 and R2 as well as the buttons and the left stick. atari800 uses it for twin-stick games, and works without it on older firmware.

## Fixed since 7.00.00RC3

- **The Generation 1's USB serial bridge (the Microbridge, a programmed PIC) was not accepted as a USB serial device** by another Colour Maximite 2: it refuses the request that sets DTR and RTS, and OPEN gave the error "did not accept DTR/RTS". A device that refuses it is now opened without it. An attempt that had failed also left the USB host's set-up unfinished.
- **EDIT's mark mode drew the top line over the status line** after a scroll that left the start of the selection above the screen, and the status line gave the line below the cursor after a scroll.
- **atari800 (in the zip, not the firmware): any use of a tape crashed the emulated Atari under AltirraOS.** atari800 patches the operating system's tape routine at a fixed address, which was wrong for the AltirraOS it ships with; this release's atari800 has it corrected.

## What behaves differently

- OPTION PICOMITE is accepted in a program (it gave "Invalid in a program").
- MM.VER is 7.000054 for RC4. A release will count as 99 in the last two digits, so 7.00.00 will be 7.000099, above every beta and release candidate of the same version.

## The programs tested

On a Generation 2 unless it says otherwise:

- atari800: Altirra's own test suite for emulators, Acid800, gives the same result on the Colour Maximite 2 as atari800 on a PC, test for test. BASIC with Altirra's ROMs and with Atari's; a program (.xex), a disk image (.atr), an 8K cartridge (.car) and a 5200 cartridge (Acid5200); a tape saved with CSAVE and loaded back with CLOAD; boot tapes, fast and at real speed. Two USB gamepads, one pad's two sticks in a twin-stick game, and the mouse as paddles. Sound, the keyboard and the menus.
- zeptoforth 1.17.2: its 108 double-precision checks, the solar eclipse program's output identical to MMBasic's byte for byte, and the checks of its Colour Maximite 2 words (files, graphics, the documentation and a stress test), all passed.
- OPTION PICOMITE set in a program, with the option saved and not saved: MM.DEVICE$ during and after, and OPTION LIST.
- gamepad_axes with a DualShock 3 and a DualShock 4.
- A Generation 1's Microbridge on a Generation 2's USB socket, with the VT100 terminal at 115200 baud.

## Known problems

- **atari800:** with its SIO patch off (tapes at real speed), a tape that has a program file after its boot loader stops with a checksum error under AltirraOS; with Atari's own XL operating system it loads. With the SIO patch on, as it starts, both load at once.
- **The MCP2221A USB to serial bridge** (used on some Generation 1 boards) loses characters above 57600 baud on a USB socket, as it has no flow control. The manual says so.

The known problems of 7.00.00RC3 remain.

## The manual

The User Manual describes OPTION PICOMITE in a program, and mark mode's End End. Colour_Maximite_2_Loadable_Programs.pdf adds services version 16, and Colour_Maximite_2_App_User_Guide.pdf the Commodore 64 and Atari emulators.

## Licences

atari800 is under the GNU GPL version 2 or later. AltirraOS and Altirra BASIC, built into it, are copyright Avery Lee and may be copied and passed on with their notice. frank-c64 and Frodo are under the GNU GPL version 2 or later, and the C64's ROMs built into it are Commodore's. The licences folder of the zip has them, and its README lists every program's licence.
