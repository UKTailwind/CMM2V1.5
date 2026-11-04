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
