# Colour Maximite 2: MMBasic 7.00.00RC5 (release candidate) release notes

*10 November 2026. Compared with 7.00.00RC4, the last version released. The full notes, from 5.07.02, are in Release_Notes_7.00.00.md.*

This is the sixth release candidate of MMBasic 7.00 for the Colour Maximite 2. Unless problems are found in it, it will become 7.00.00. Please report problems on the Back Shed forum.

It adds two emulators to the programs, an Amiga 500 and an Apple //e, from 11 November 2026 a third, the TRS-80, from 12 November a fourth, the BBC Micro, on 12 November a word processor, WordGrinder, and on 13 November a spreadsheet, sc-im. For the Amiga, the firmware gives loadable programs their sound by DMA, a copy without the cache's write-back, and the fast internal memory. MODE 8 now runs at 60Hz.

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

Copy the new CMM2_Programs_7.00.00RC5.zip to the card as well: the amiga, apple2, trs80, bbc, wordgrinder and scim folders are new.

## What is new

- **An Amiga 500 emulator** (Generation 2), in the folder amiga: pico-amiga, drewpo28's emulator for the RP2350, with Karl Stenerud's Musashi 68000. It is an Amiga 500 with 1MB of memory and its original chipset, a PAL Amiga at its own 50 frames a second in 720 x 576, with its sound; -ntsc gives an NTSC Amiga at 60 frames a second in 640 x 480. The mouse is the Amiga's mouse and USB gamepads its joysticks. F11 brings up a menu of the disks on the card at any time, to boot one, to put it in the drive when a game asks for its next disk, or to put it in DF1, the second drive; it makes blank disks too. What the Amiga writes to a disk goes into its .adf file on the card, as onto a real disk, so saved games and your own work last; a read-only file, or -wp, is a write-protected disk. Ctrl+Alt+Del resets the Amiga. The heaviest demos tried keep up, with the sound on.

  It needs a Kickstart ROM and disk images of your own: Kickstart is still copyright and does not come with it. If you have no Amiga of your own to take them from, Cloanto's Amiga Forever (www.amigaforever.com) is the legal way to get them, and costs little: its Value Edition has Kickstart 1.3, the Workbench 1.3 disk and over 50 games and demos. Copy Shared\rom\amiga-os-130.rom from its Amiga Files folder to A:/amiga, keeping its name, and the .adf disk images you want, then choose a disk from the menu:

  ```
  CHDIR "A:/amiga"
  RUN "amiga.app"
  ```

  The App User Guide has a chapter on it, with how to take Amiga Forever's games and demos out of their .rp9 files.

  On 11 November 2026 the zip's amiga.app was replaced (the firmware is unchanged): a jump to an odd address now gives the 68000's address error, as some copy protections expect, and a disk's data now arrives as the disk turns, as some loaders expect. Super Cars (Gremlin), which stopped with Guru Meditation #0000000B, now loads.

  On 12 November 2026 it was replaced again (the firmware is unchanged): HAM7, the trick of seven bitplanes in HAM mode, where the Amiga fetches four planes and takes the other two from fixed words the program writes, is now shown as an Amiga shows it. Desire's Inside the Machine (2024) showed its HAM7 scenes, its metallic one among them, as a mush of pixels; they now look as they should.

- **An Apple //e emulator,** in the folder apple2: frank-apple, rh1tech's port of Michel Pollet's MII. An enhanced Apple //e with 128K, an 80-column card, two Disk II drives and two hard disks, at the Apple's 60 frames a second in MODE 8, with its screen at full resolution. The Apple's ROMs are built in. The App User Guide has a chapter on it.

- **A TRS-80 emulator,** in the folder trs80 (added to the programs on 11 November 2026: the firmware is unchanged): sdltrs, Mark Grebe's and Jens Guenther's emulator, from Tim Mann's xtrs. It is the TRS-80 Model I, III, 4 and 4P, with floppy and hard disks, cassettes, sound, a joystick, a mouse, a serial port and sdltrs's own menu (F7), on both generations. The Model 4's 80 x 24 screen fills MODE 8. What the TRS-80 writes to a disk goes into its file on the card. The ROM's MEMORY SIZE? and a DOS's date and time are answered for you, the last two from the clock.

  No Tandy ROM comes with it. The Model 4P needs none: its free boot ROM is built in, and starts a Model 4 system disk such as LS-DOS 6.3.1, which may be copied freely and is at www.tim-mann.org/trs80.html. The Models I, III and 4 need level2.rom or model3.rom of your own in the folder; copies are at github.com/asig/xtrs-roms (level2rom.hex and romimage.m3, to be renamed), to download and use at your own risk:

  ```
  CHDIR "A:/trs80"
  RUN "trs80.app", -model 4p -disk0 ld4-631.dsk
  ```

  The App User Guide has a chapter on it.

- **A BBC Micro emulator,** in the folder bbc (added to the programs on 12 November 2026: the firmware is unchanged): frank-micro, rh1tech's port of Tom Walker's B-em by way of Graham Sanderson's B-em for the Pico. It is the BBC Micro Model B and the BBC Master 128, at the BBC's own 50 frames a second in 720 x 576 on both generations, with the BBC's ROMs built in, two disc drives whose discs are written to their files on the card, sound and a gamepad. Discs (.ssd, .dsd, .adf, .adl) go in A:/bbc/disk and are chosen with F11. The USB keyboard types what is printed on its keys. Sideways ROM images in A:/bbc/roms go into its sideways RAM as it starts, on either model, and *SRLOAD loads one from a disc:

  ```
  CHDIR "A:/bbc"
  RUN "bbc.app", disk/elite.ssd
  ```

  The App User Guide has a chapter on it.

- **A word processor, WordGrinder,** in the folder wordgrinder (added to the programs on 12 November 2026: the firmware is unchanged): David Given's WordGrinder 0.8, for writing. It keeps to the words and their structure (headings, lists, quotations, bold, italic and underline) and exports them as ODT for Microsoft Word or LibreOffice, and as HTML, Markdown, LaTeX, Troff or plain text. It uses the video mode MMBasic is in, from 640 x 400 to 1920 x 1080, with a font chosen to suit; Alt+= and Alt+Minus change them as it runs. Its documents can be on the SD card or a USB flash drive. On a Generation 2 it checks spelling against a whole English dictionary, British or American, with the memory MMBasic lends it; on a Generation 1 there is not the room, and it says so and carries on. It runs on PDCursesMod, with a new screen and keyboard for the Colour Maximite 2:

  ```
  CHDIR "A:/wordgrinder"
  RUN "wordgrinder.app", letter.wg
  ```

  The App User Guide has a chapter on it.

- **A spreadsheet, sc-im,** in the folder scim (added to the programs on 13 November 2026: the firmware is unchanged): Andrés Martinelli's sc-im 0.8.6, a spreadsheet worked from the keyboard as the vi editor is. It has formulas with over 60 functions, several sheets in a file, number and date formats, sorting, filters, undo and colours. It keeps its sheets in its own text format (.sc), formulas and all, and reads and writes CSV files from and to Microsoft Excel and LibreOffice (CSV UTF-8 or Excel's plain CSV), tab-separated files and Markdown tables. A USB mouse moves to a cell with a click, types a cell's name into a formula, and scrolls with its wheel. Like WordGrinder, it uses the video mode MMBasic is in, and Alt+= and Alt+Minus change the mode and the font. Its CSV reading and writing were mended for Excel's files: the mark at the start of a CSV UTF-8 file, quotes inside text, and the accented letters of a plain CSV file:

  ```
  CHDIR "A:/scim"
  RUN "scim.app", budget.sc
  ```

  The App User Guide has a chapter on it, with a first sheet to make step by step.

- **Going up a folder in the Atari's and the TRS-80's lists of files** (on 12 November 2026 the zip's atari800.app and trs80.app were replaced: the firmware is unchanged). [..] at the top of each folder's list (<..> on the TRS-80) now goes up a folder, as far as A:/. Before, once in a folder, the Atari could go up only with Backspace, and the TRS-80 not at all.

- **MODE 8 is 640 x 480 at 60Hz,** the standard VGA timing, where it was 75Hz. Its GETSCANLINE counts to 524. The other modes are unchanged.

- **For programmers:**
  - Version 17 of the services: video_mode and video_scaled take depth 12, a screen of 4,096 colours as ARGB4444 on one layer, each pixel 0x0RGB as an Amiga's colour registers hold them.
  - Version 18: dma2d_copy_nc, a DMA2D copy that does not write its source back from the cache first, for a second copy of the same rows.
  - The sound stream (audio_start) now goes to the DACs by DMA, with no interrupt for each sample: in the Amiga emulator the sound's cost fell from 0.75ms a frame to 0.15. Programs need no change.
  - build.py's new options --axi and --itcm put a program's hottest code and data in the processor's AXI RAM and in the ITCM above the firmware's code (0xA000 to 0xFFFF), on a Generation 2. The firmware keeps that ITCM free from this version on.
  - From 12 November the tools' readdir gives ".." first in a folder below a drive's top folder, as Linux's does, and stat and opendir take names with "." and ".." in them, on exFAT cards too.

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
  - AmigaBASIC, from a Workbench 1.3 Extras disk in DF1, running its Demo program.
  - Saving: a blank disk made from the menu and put in DF1; AmigaDOS wrote a file and a folder to it, and the .adf file, copied from the card to a PC, has them. With -wp the Amiga reported the disk write-protected, and the file was unchanged.
  - Its picture's checksum, taken every 500 frames through 8,000 frames of Arte, is the same on this firmware as on the development version it was measured on.
  - Super Cars (Gremlin), with the amiga.app of 11 November: its title screen and results desk on pico-amiga's own PC build, and its intro on a Generation 2, where it had stopped with the guru. Ten other games and demos show the same screens, frame for frame, as before.
  - Desire's Inside the Machine (2024), with the amiga.app of 12 November, on a Generation 2 running the development firmware (the change is in the program alone): four minutes of it, its HAM7 scenes in their colours, as pico-amiga's own PC build draws them.
- **apple2:** on a Generation 1 with this firmware, Applesoft and hi-res graphics, with the Apple at its full speed (1,023kHz); before, on 7.00.00RC4, lo-res, mixed, 80 columns and double hi-res (a2audit's video tests find their colours right), the disk menu and a Disk II boot.
- **trs80:** on both generations, Level II BASIC on the Model I, Model III BASIC, LDOS 5.3.1 from disk on the Models I and III, and LS-DOS 6.3.1 on the Model 4 and on the Model 4P with the free boot ROM; the menu and its list of disks; sound; a USB keyboard, the mouse and a gamepad by hand; the MegaMem on a Generation 2. On a Generation 1 with this firmware as released, the Model I, the Model III with LDOS and the Model 4P with LS-DOS; and a BASIC program saved to a disk and loaded again after leaving.
- **atari800 and trs80,** with the apps of 12 November, on a Generation 2 running the development firmware (the change is in the programs alone): [..] and <..> at the top of each folder below A:/, going up a folder at a time to A:/, and none in A:/ itself; Backspace in the Atari's list.
- **bbc,** on both generations running the development firmware (it needs nothing new from it): BASIC; MODE 0, 2 and 7; Elite; Chuckie Egg with the keyboard and a gamepad; the sound; Wordwise-Plus 1.4F editing, previewing and saving its text to a disc; BASIC programs saved and loaded; *FORM; the roms folder and *SRLOAD on both models; changing the model; a UK USB keyboard.
- **wordgrinder,** on both generations running the development firmware (it needs nothing new from it): its own 65 tests, of which 64 pass (the other looks for a Windows install folder); typing, selecting with Shift and Ctrl, the menus and styles with a USB keyboard; documents saved and loaded on A: and C:, autosave, and an ODT export opened in Microsoft Word; changing the video mode and font with Alt+= and Alt+Minus; and, on a Generation 2, spelling with the whole British dictionary (ready in 6 seconds), and on a Generation 1 the message that it does not fit, with the document kept.
- **scim,** on both generations running the development firmware (it needs nothing new from it), with a USB keyboard and mouse: the chapter's first sheet; formulas, functions, dates and formats; text with accented letters, and Greek letters with Ctrl+K; the arrows, Page Up and Page Down, Home and End; a click to a cell and into a formula, and the wheel; Alt+= and Alt+Minus; a settings file (scimrc) and the history of what was typed; sheets saved and loaded as .sc and CSV; CSV files saved by Microsoft Excel (CSV UTF-8 and plain CSV, with dates, percentages, quotes and accented letters) opened, and sc-im's CSV files opened in Excel; :help; and the shell's commands (:!, :plot) saying they cannot run.
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
  - The first HAM7 scene of Desire's Inside the Machine runs at about 83% of the Amiga's speed (24ms a frame), as it did before it was shown right.
- **apple2:** it has been tried on a Generation 1 only, and through the serial console: its sound, a USB keyboard, a gamepad, saving to a disk and the hard disks have not been tried yet.
- **trs80:** cassette files, /CMD files, hard disks, the hi-res graphics boards, the printer, the serial port, saved states and games with a joystick have not been tried yet. A disk cannot be write-protected from its menu. The MegaMem needs a Generation 2.
- **bbc:** ADFS discs, the Printer DAC and screenshots have not been tried yet. The Model B runs the Master's 65C12, so a program that uses the original 6502's undocumented instructions may not run. Some early ROMs made for the Model B, Wordwise-Plus 1.4A among them, do not run on the Master.
- **wordgrinder:** it has no mouse (WordGrinder 0.8 has none) and does not print: export a document and print it from a PC. Ctrl+I and Ctrl+M need a USB keyboard, as a terminal on the serial console sends them as Tab and Enter. Checking spelling with a whole dictionary needs a Generation 2.
- **scim:** it has no charts, cannot run other programs, has no clipboard, cannot read or write .xlsx, .xls or .ods files (use CSV), and does not print. A CSV file holds each number as it is shown, so give a column the decimal places it needs before saving; numbers with a thousands separator, percentages and dates in a CSV file come in as text. The fonts lack many of the Greek and maths characters Ctrl+K can enter. Sorting, filters and subtotals have not been tried yet.

The known problems of 7.00.00RC4 remain.

## The manual

The User Manual gives MODE 8's new timing. Colour_Maximite_2_Loadable_Programs.pdf adds services versions 17 and 18, the sound by DMA, and --axi and --itcm, and Colour_Maximite_2_App_User_Guide.pdf the Amiga, Apple //e, TRS-80 and BBC Micro emulators, WordGrinder and sc-im.

## Licences

pico-amiga is under the GNU GPL version 3, and Musashi under the MIT licence. frank-apple and MII are under the MIT licence, and the Apple's ROMs built into them are Apple's. No Amiga software comes with the emulator. sdltrs is under the BSD 2-Clause licence, xtrs under its own, and the Model 4P boot ROM built in is Peter Cervasio's, free to copy with its notice; no Tandy ROM comes with it. frank-micro is under the GNU GPL version 3 and B-em under version 2 or later, and the BBC's ROMs built into it are Acorn's. WordGrinder is under the MIT licence, as are Lua and its other Lua parts; MiniZip and zlib are under the zlib licence, uthash under a BSD licence, PDCursesMod and dlmalloc are in the public domain, and its dictionaries, from SCOWL, are free to use with their notice. sc-im is under the BSD 4-Clause licence (this product includes software developed by Andrés G. Martinelli), and the regular expressions it has from newlib under BSD licences. The licences folder of the zip has them, and its README lists every program's licence.
