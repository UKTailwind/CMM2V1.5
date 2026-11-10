# Colour Maximite 2: MMBasic 7.00.00RC5 (release candidate) release notes

*10 November 2026. Compared with 7.00.00RC4, the last version released. The full notes, from 5.07.02, are in Release_Notes_7.00.00.md.*

This is the sixth release candidate of MMBasic 7.00 for the Colour Maximite 2. Unless problems are found in it, it will become 7.00.00. Please report problems on the Back Shed forum.

It adds two emulators to the programs, an Amiga 500 and an Apple //e. For the Amiga, the firmware gives loadable programs their sound by DMA, a copy without the cache's write-back, and the fast internal memory. MODE 8 now runs at 60Hz.

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

Copy the new CMM2_Programs_7.00.00RC5.zip to the card as well: the amiga and apple2 folders are new.

## What is new

- **An Amiga 500 emulator** (Generation 2), in the folder amiga: pico-amiga, drewpo28's emulator for the RP2350, with Karl Stenerud's Musashi 68000. It is an Amiga 500 with 1MB of memory and its original chipset, a PAL Amiga at its own 50 frames a second in 720 x 576, with its sound; -ntsc gives an NTSC Amiga at 60 frames a second in 640 x 480. The mouse is the Amiga's mouse and USB gamepads its joysticks. F11 brings up a menu of the disks on the card at any time, to boot one, to put it in the drive when a game asks for its next disk, or to put it in DF1, the second drive; it makes blank disks too. What the Amiga writes to a disk goes into its .adf file on the card, as onto a real disk, so saved games and your own work last; a read-only file, or -wp, is a write-protected disk. Ctrl+Alt+Del resets the Amiga. The heaviest demos tried keep up, with the sound on.

  It needs a Kickstart ROM and disk images of your own: Kickstart is still copyright and does not come with it. If you have no Amiga of your own to take them from, Cloanto's Amiga Forever (www.amigaforever.com) is the legal way to get them, and costs little: its Value Edition has Kickstart 1.3, the Workbench 1.3 disk and over 50 games and demos. Copy Shared\rom\amiga-os-130.rom from its Amiga Files folder to A:/amiga, keeping its name, and the .adf disk images you want, then choose a disk from the menu:

  ```
  CHDIR "A:/amiga"
  RUN "amiga.app"
  ```

  The App User Guide has a chapter on it, with how to take Amiga Forever's games and demos out of their .rp9 files.

- **An Apple //e emulator,** in the folder apple2: frank-apple, rh1tech's port of Michel Pollet's MII. An enhanced Apple //e with 128K, an 80-column card, two Disk II drives and two hard disks, at the Apple's 60 frames a second in MODE 8, with its screen at full resolution. The Apple's ROMs are built in. The App User Guide has a chapter on it.

- **MODE 8 is 640 x 480 at 60Hz,** the standard VGA timing, where it was 75Hz. Its GETSCANLINE counts to 524. The other modes are unchanged.

- **For programmers:**
  - Version 17 of the services: video_mode and video_scaled take depth 12, a screen of 4,096 colours as ARGB4444 on one layer, each pixel 0x0RGB as an Amiga's colour registers hold them.
  - Version 18: dma2d_copy_nc, a DMA2D copy that does not write its source back from the cache first, for a second copy of the same rows.
  - The sound stream (audio_start) now goes to the DACs by DMA, with no interrupt for each sample: in the Amiga emulator the sound's cost fell from 0.75ms a frame to 0.15. Programs need no change.
  - build.py's new options --axi and --itcm put a program's hottest code and data in the processor's AXI RAM and in the ITCM above the firmware's code (0xA000 to 0xFFFF), on a Generation 2. The firmware keeps that ITCM free from this version on.

## What behaves differently

- MODE 8 runs at 60Hz, not 75Hz. A program that times itself by the frame (with the frame interrupt of MODE, or GETSCANLINE) runs at 60 frames a second in MODE 8.
- MM.VER is 7.000055 for RC5.

## The programs tested

On a Generation 2 unless it says otherwise:

- **amiga:**
  - Mental Hangover (Scoopex) and Arte (Sanity), the heaviest demos tried, at 50 frames a second all through with the sound on.
  - The Workbench 1.3 desktop with the mouse and keyboard.
  - Games from Amiga Forever, each for a minute: Arkanoid, Aunt Arctic Adventure, Blasteroids, Datastorm, Exile, Gotcha, Lethal Xcess, Obsession Pinball, Pac-Mania, Rage, Statix, Toobin and Zoom! load and run at 50 frames a second. X-Balls and Logic load slowly through AmigaDOS, and had not finished in that minute. Tie Break shows only a black screen (not looked into yet).
  - Demos: 242 (Virtual Dreams and Fairlight) at 50 frames a second; D.O.S. (Andromeda) falls just short in a few places, at worst 21.9ms for a 20ms frame.
  - The keyboard, the mouse and a gamepad by hand, in Workbench and in games, and the disk menu.
  - Saving: a blank disk made from the menu and put in DF1; AmigaDOS wrote a file and a folder to it, and the .adf file, copied from the card to a PC, has them. With -wp the Amiga reported the disk write-protected, and the file was unchanged.
  - Its picture's checksum, taken every 500 frames through 8,000 frames of Arte, is the same on this firmware as on the development version it was measured on.
- **apple2:** on a Generation 1 with this firmware, Applesoft and hi-res graphics, with the Apple at its full speed (1,023kHz); before, on 7.00.00RC4, lo-res, mixed, 80 columns and double hi-res (a2audit's video tests find their colours right), the disk menu and a Disk II boot.
- **The sound by DMA** on a Generation 1: Doom's timed demo with its music and effects at 38.4 frames a second, as before; the DACs given back to MMBasic after a program (PLAY TONE), on both generations.
- **MODE 8** on a Generation 1: 800 x 525 at 25.2MHz, 60.03Hz, and GETSCANLINE up to 524.
- Every other program in the zip is unchanged: rebuilt with this version's tools, they are the same files.

## Known problems

- **amiga:**
  - It runs on a Generation 2 only.
  - Interlaced screens show every other line.
  - The chipset does not report collisions between sprites and the playfield, which a few games use.
  - Titles made for later Amigas (the A1200 and A4000) and those on hard disk images do not run.
  - Tie Break, from Amiga Forever, shows only a black screen.
- **apple2:** it has been tried on a Generation 1 only, and through the serial console: its sound, a USB keyboard, a gamepad, saving to a disk and the hard disks have not been tried yet.

The known problems of 7.00.00RC4 remain.

## The manual

The User Manual gives MODE 8's new timing. Colour_Maximite_2_Loadable_Programs.pdf adds services versions 17 and 18, the sound by DMA, and --axi and --itcm, and Colour_Maximite_2_App_User_Guide.pdf the Amiga and Apple //e emulators.

## Licences

pico-amiga is under the GNU GPL version 3, and Musashi under the MIT licence. frank-apple and MII are under the MIT licence, and the Apple's ROMs built into them are Apple's. No Amiga software comes with the emulator. The licences folder of the zip has them, and its README lists every program's licence.
