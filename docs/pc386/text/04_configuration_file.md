# The Configuration File

Each PC is described by a configuration file: a text file, usually ending .ini, in the emulator's folder. It names the BIOS files and the disk images, and sets the PC's memory and processor. You can keep several, one for each PC, and start any of them with RUN "pc386.app", name.ini.

## The Format

The file has sections, each headed by its name in square brackets, and in each section lines of the form name = value. A line starting with a semicolon is a comment. This is pc386.ini, the FreeDOS PC:

```
; pc386.ini: the PC that RUN "pc386.app" starts
[pc]
bios = bios.bin
vga_bios = vgabios.bin
mem_size = 4M
vga_mem_size = 256K
hda = freedos.img
fill_cmos = 1

[cpu]
gen = 3
fpu = 1
```

File names are found in the current directory, or can be given in full (A:/pc386/freedos.img). Sizes are a number followed by K or M, in capitals.

## The [pc] Section

{widths: 14,38}
| Name | What it sets |
| bios | The BIOS file: bios.bin. |
| vga_bios | The VGA BIOS file: vgabios.bin. |
| mem_size | The PC's memory, for example 4M. If there is not room for that much, the PC gets as much as fits, and the first line on the serial console says how much. |
| vga_mem_size | The VGA card's memory: 256K, as on a VGA card of the time. More lets the PC use larger VESA modes, but it comes from the same memory as the PC's own. |
| hda, hdb, hdc, hdd | The PC's first to fourth hard disks: image files. hda is the disk DOS starts from, C:. |
| cda, cdb, cdc, cdd | A CD-ROM drive in place of that hard disk, holding an image file (.iso). This has not been tried on the Colour Maximite 2. |
| fda, fdb | The floppy images in drives A: and B: when the PC starts. |
| fill_cmos | 1 puts the hard disks' sizes in the PC's set-up (its CMOS memory), as a BIOS set-up program would. Keep it at 1. |

## The [cpu] Section

{widths: 14,38}
| Name | What it sets |
| gen | 3: a 386. 4, 5 or 6: the processor reports itself as a 486, a Pentium or a Pentium Pro, for programs that check. |
| fpu | 1: the PC has a maths coprocessor (an x87). 0: none. |

## The [pc386] Section

This section is the Colour Maximite 2 emulator's own.

{widths: 14,38}
| Name | What it sets |
| floppies | The floppy images that drive A: is given in turn, separated by spaces or commas, up to 16. The word none is an empty drive. The first is in A: when the PC starts, unless fda names one. Ctrl+Alt+Page Down and Ctrl+Alt+Page Up change between them. |
| new_disk | A size, for example 64M. If the image that hda names is not on the card, an empty one of this size is made when the PC starts. |

## Which Drive the PC Starts From

As on a real PC, the BIOS starts from drive A: if there is a disk in it, and otherwise from the first hard disk. So a list of floppies that starts with none starts the PC from its hard disk, with the floppies ready to be put in later. A list that starts with a disk starts the PC from that disk, as an installation does.

As the PC starts, SeaBIOS shows "Press ESC for boot menu" for a moment: pressing Esc then lets you choose the drive to start from.

## Examples

A PC for installing MS-DOS from its three Setup disks onto a new 64 MB hard disk (Installing MS-DOS):

```
; msdos.ini: MS-DOS installed from its Setup disks onto msdos.img
[pc]
bios = bios.bin
vga_bios = vgabios.bin
mem_size = 4M
vga_mem_size = 256K
hda = msdos.img
fill_cmos = 1

[cpu]
gen = 3
fpu = 1

[pc386]
floppies = DISK1.IMG DISK2.IMG DISK3.IMG none
new_disk = 64M
```

The same PC once MS-DOS is installed. It starts from C:, with Windows 3.1's six disks ready to be put in drive A:

```
; msdosc.ini: MS-DOS from C:, Windows 3.1's disks to hand
[pc]
bios = bios.bin
vga_bios = vgabios.bin
mem_size = 4M
vga_mem_size = 256K
hda = msdos.img
fill_cmos = 1

[cpu]
gen = 3
fpu = 1

[pc386]
floppies = none WIN1.IMG WIN2.IMG WIN3.IMG WIN4.IMG WIN5.IMG WIN6.IMG
```

A PC with no hard disk, started from a floppy:

```
[pc]
bios = bios.bin
vga_bios = vgabios.bin
mem_size = 4M
vga_mem_size = 256K
fda = BOOT.IMG

[cpu]
gen = 3
fpu = 1
```
