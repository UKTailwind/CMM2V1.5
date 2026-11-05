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

## Getting a Program into the PC

A program reaches the PC on a floppy image. You put its files in an image on another computer, give the image to the emulator, and copy the files to the PC's hard disk from DOS.

### Check That It Is a DOS Program

The PC runs programs written for DOS, and programs for Windows 3.1 if that is installed. A program for Windows 95 or later does not run, even one that works in a command window. The name can mislead: a "DOS version" of a modern program is often a Windows program for the Command Prompt. DOS stops such a program with one of these messages:

```
This program cannot be run in DOS mode.
This is a Windows NT character-mode executable
```

### Making the Image

Make an empty 1.44 MB image and copy the files onto it. On Linux, or on Windows with WSL (the Windows Subsystem for Linux), mtools does this. Install it with sudo apt install mtools, then:

```
mformat -C -f 1440 -i FILES.IMG ::
mcopy -i FILES.IMG setup.exe readme.txt ::
mdir -i FILES.IMG ::
```

The last line lists what is on the image. Run mcopy again to add more files.

DOS file names have up to 8 characters, a dot and up to 3 more. Give a file with a longer name its DOS name as it is copied:

```
mcopy -i FILES.IMG "Long File Name.exe" ::PROGRAM.EXE
```

On Windows, WinImage also makes images from files.

An image holds about 1.4 MB. A larger program needs several images, used one after the other.

### Copying the Files to the PC

1. Copy FILES.IMG to the emulator's folder on the SD card.

2. In the configuration file you start the PC with, add its name to the list of floppies, straight after none:

```
floppies = none FILES.IMG
```

Keep none first: if the PC starts with a disk in drive A:, it tries to start from that disk (The Configuration File).

3. Start the PC:

```
CHDIR "A:/pc386"
RUN "pc386.app", msdosc.ini
```

4. At the DOS prompt, press Ctrl+Alt+Page Down until the top of the screen shows "A: FILES.IMG".

5. Copy the files to a folder of their own on the hard disk:

```
MD C:\PROGRAM
COPY A:\*.* C:\PROGRAM
```

6. Run the program from there:

```
CD \PROGRAM
SETUP
```

The files are now on the PC's hard disk and stay there. Wait until the PC has stopped writing before you leave the emulator (Leaving, in Getting Started).
