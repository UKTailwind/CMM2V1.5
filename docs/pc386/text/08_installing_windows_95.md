# Installing Windows 95

Windows 95 runs on the G2. It is not fast, but it runs, with its mouse and sound. A G1 cannot run it: Windows 95 needs 4 MB of memory, and the PC on a G1 has 2,816 KB.

This chapter installs Windows 95 OSR2.5 from an image of its CD, which needs no MS-DOS first. Use an image of your own CD: this chapter calls it WIN95.ISO. Setup takes about three hours.

## Preparing

Copy WIN95.ISO to the emulator's folder. The CD has a start-up disk inside it, as a floppy image. Save that to the emulator's folder too, as W95BOOT.IMG: a program that reads ISO files can do this, and 7-Zip shows it in a folder called [BOOT]. The PC can start from the CD itself, but only until its hard disk is formatted, so Setup needs the floppy image.

Then make a configuration file, win95.ini. Its 5 MB is about as much as a G2 has room for:

```
; win95.ini: Windows 95 from its CD onto win95.img
[pc]
bios = bios.bin
vga_bios = vgabios.bin
mem_size = 5M
vga_mem_size = 256K
hda = win95.img
cdb = WIN95.ISO
fill_cmos = 1

[cpu]
gen = 3
fpu = 1

[pc386]
floppies = W95BOOT.IMG none
new_disk = 384M
```

## Running Setup

1. Start the PC with RUN "pc386.app", win95.ini. The emulator makes the hard disk image first, which takes a while.

2. The start-up disk loads its CD-ROM driver and runs Setup. A new hard disk has to be partitioned and formatted first: follow what the screen asks.

3. Choose Typical. When Setup asks what the PC has, tick the sound card and leave the network card.

4. Setup copies its files. This is the long part, over two hours, as the PC decompresses each file at the speed of a 386. Let it run.

5. When Setup is about to restart the PC, press Ctrl+Alt+Page Down to empty drive A:. If the start-up disk is still in the drive, Setup starts again.

6. The first start sets up the PC's hardware, then Control Panel and the Start menu, which takes about 20 minutes. There is little on the screen for long stretches: the SD card's light shows that it is still working.

7. Choose your time zone and answer anything else it asks. Windows 95 then starts.

Then change the list of floppies in win95.ini, or the start-up disk goes back in drive A: each time the emulator starts:

```
floppies = none W95BOOT.IMG
```

Windows 95 takes under six minutes to start after that. Use Shut Down on the Start menu before you leave the emulator: Windows 95 turns the PC off, and the emulator ends by itself.
