# Getting Started

This chapter puts the emulator on the SD card and starts the FreeDOS PC that comes with it.

## The Files

The emulator's files are in the pc386 folder of the firmware's release:

{widths: 14,38}
| File | What it is |
| pc386.app | The emulator. |
| bios.bin | The PC's BIOS (SeaBIOS). |
| vgabios.bin | The VGA card's BIOS (SeaVGABIOS). |
| freedos.img | A 32 MB hard disk with FreeDOS 1.3 installed, its mouse driver included. |
| pc386.ini | The configuration file of the FreeDOS PC. |

Make a folder pc386 on the SD card and copy the files into it. A card reader on a PC is the quickest way: disk images are tens of megabytes, which take many minutes over the serial console.

## The First Run

At the command prompt:

```
CHDIR "A:/pc386"
RUN "pc386.app"
```

The screen changes to 640 x 480, SeaBIOS starts, and FreeDOS starts from the hard disk image. It loads its mouse driver and ends at the C:\> prompt, ready for commands. The serial console shows a few lines from the emulator, such as:

```
pc386: 4096 KB of RAM, 640x480, pc386.ini
```

That is the PC's memory, the screen and the configuration file it was started with.

## Leaving

Press Ctrl+Alt+End to leave the emulator and go back to the MMBasic prompt. This switches the PC off at once, as if its power had been cut. Before you leave:

- Finish what the PC is doing, and leave Windows if it is running.
- Wait until the PC has stopped writing to its disks.

The PC's disks are the image files on the SD card, so everything written to them is kept for the next time.

## Starting Again

The next time, the same two commands start the same PC with its disks as you left them. To start a different PC, name its configuration file:

```
RUN "pc386.app", msdos.ini
```

## Moving a PC to Another Colour Maximite 2

A PC is a few files in a folder, so copying them moves it, with everything installed on its hard disk. A G1 and a G2 use the same files.

### The Files You Need

These five files are the least a PC needs. Copy them into one folder on the other computer's SD card:

{widths: 18,34}
| File | What it is |
| pc386.app | The emulator. |
| bios.bin | The PC's BIOS. |
| vgabios.bin | The VGA card's BIOS. |
| The configuration file | The PC's .ini file: pc386.ini for the FreeDOS PC, or your own, such as msdosc.ini. |
| The hard disk image | The file its hda line names: freedos.img, or your own, such as msdos.img. |

Floppy images are needed only if the configuration file lists them and you want to use them.

### Moving It

1. Leave the emulator (Leaving, above), so that the hard disk image is complete.

2. Check that the other computer has MMBasic 7.00.00RC1 or later. An older firmware stops with "Needs a newer firmware".

3. Copy the five files into a folder such as pc386 on the other computer's SD card. Use a card reader: a 64 MB image takes about half an hour over the serial console.

4. In the configuration file, take out of the floppies line every image you did not copy. If none is left, delete the line.

5. Start the PC, naming its configuration file:

```
CHDIR "A:/pc386"
RUN "pc386.app", msdosc.ini
```

The emulator looks for all its files in the current directory, so CHDIR comes first. If the configuration file is called pc386.ini, RUN "pc386.app" alone is enough.

The OPTIONs of the first computer do not move with the files. Set OPTION SD TIMING FAST on the other computer too, and OPTION OVERCLOCK ON if it is a G1 of revision Y (Using the PC).

A hard disk image holds everything installed on it. FreeDOS can be passed on to other people. MS-DOS, Windows and other commercial programs cannot, so an image with them on it is for your own computers.
