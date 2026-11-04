# Disk Images

The PC's disks are image files on the SD card: each holds every sector of a disk, in order, and nothing else. Images of this kind are also called raw images, and other PC emulators use them too.

## Hard Disk Images

A hard disk image holds a whole disk: its partition table and its partitions. There are two ways to get one:

- **Make an empty one.** Name it with hda and give its size with new_disk (The Configuration File). The emulator writes the empty image to the card as the PC starts. This takes a while, and the serial console shows a dot for each 4 MB. The PC then sees a new, empty disk, which DOS prepares with FDISK and FORMAT, or MS-DOS Setup prepares for itself.
- **Copy one.** Use freedos.img, or an image made by another emulator that writes raw images.

64 MB is plenty for MS-DOS, Windows 3.1 and a good number of programs. A DOS partition can be up to 2 GB, but a large image takes a long time to make and to copy.

The images are ordinary files, so they can be copied and backed up with the card in a PC. Make a copy before you try something that might spoil a disk. Programs on a PC can open an image to read its files: on Windows, 7-Zip opens one, and OSFMount can show one as a drive. Only do this while the emulator is not running.

## Floppy Images

A floppy image is a disk of one of the PC's usual sizes:

{widths: 18,34}
| Disk | Image size in bytes |
| 3.5 inch, 1.44 MB | 1,474,560 |
| 3.5 inch, 720 KB | 737,280 |
| 3.5 inch, 2.88 MB | 2,949,120 |
| 5.25 inch, 1.2 MB | 1,228,800 |
| 5.25 inch, 360 KB | 368,640 |

The emulator also takes the larger formats some disks were written in, such as Microsoft's 1.68 MB DMF disks. A file of any other size cannot be used: the note at the top of the screen then says "cannot be used".

### Making an Image of a Disk

Put the disk in a PC's floppy drive and copy it, sector by sector, into a file with a disk-imaging program. On Linux:

```
dd if=/dev/fd0 of=DISK1.IMG bs=512
```

On Windows, a program such as WinImage or RawWrite for Windows makes an image of a disk.

Note that many USB floppy drives read only 1.44 MB disks and not 720 KB ones. A 720 KB disk has one square hole; a 1.44 MB disk has a second hole in the opposite corner.

A program whose disk is copy-protected (Lemmings is one) checks for something on the original disk that an image does not keep, so it will not run from an image.

### Making an Image from Files

An image can also be made from files: an empty 1.44 MB disk with the files copied onto it. This is the way to take files into the PC. On Windows, WinImage does it. On Linux, mtools does:

```
mformat -C -f 1440 -i FILES.IMG ::
mcopy -i FILES.IMG setup.exe readme.txt ::
```

Put FILES.IMG in the emulator's folder and in the list of floppies. In the PC, put it in drive A: with Ctrl+Alt+Page Down and copy the files from A:.
