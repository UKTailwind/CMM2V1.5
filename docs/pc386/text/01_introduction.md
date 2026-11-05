# Introduction

pc386 turns the Colour Maximite 2 into a 386 PC. It is a loadable program: MMBasic runs it with RUN "pc386.app", and it gives the screen, the keyboard, the mouse and the sound to the PC until you leave it. The PC runs FreeDOS, MS-DOS, Windows 3.1 and the DOS programs and games of their time. On a G2 it also runs Windows 95, slowly.

The emulator is tiny386, written by Chunhui He, with a front end that joins it to the Colour Maximite 2's display, keyboard, mouse, sound and SD card. It runs on both the original Colour Maximite 2 (the G1) and the Generation 2 (the G2).

## What the PC Has

{widths: 12,40}
| Part | What the PC has |
| Processor | An Intel 80386 with an x87 maths coprocessor. The configuration file can make it report itself as a 486 or a Pentium instead. |
| Memory | 4 MB as the example files set it, or as much as fits: 2,816 KB on a G1. |
| Display | A VGA card with VESA modes, shown at 640 x 480 in 256 colours. |
| Hard disks | Up to four IDE drives, each an image file on the SD card. |
| CD-ROM drives | In place of a hard disk, each an image file (.iso) on the SD card. |
| Floppy drives | A: and B:, each an image file. A: can be given a list of images to change between, as you would change disks. |
| Keyboard | The Colour Maximite 2's USB keyboard, as a PS/2 keyboard. |
| Mouse | The Colour Maximite 2's USB or PS/2 mouse, as a PS/2 mouse. |
| Serial port | COM1, if the configuration file asks for it: the Colour Maximite 2's own COM1. |
| Sound | A Sound Blaster 16 (port 220, IRQ 5, DMA 1 and 5), an Adlib FM synthesiser (port 388) and the PC speaker, through the Colour Maximite 2's audio output. |
| BIOS | SeaBIOS and SeaVGABIOS. |

The PC has no network card, no parallel port and no MIDI (MPU-401) port.

## Speed

The Colour Maximite 2 emulates each of the 386's instructions in software. On a G2 the PC runs about 5 million instructions a second in a simple loop, and 1 to 2.5 million in Windows. FreeDOS starts in about 35 seconds, and Windows 3.1 is usable, though slower than a real 386.

A G1 runs at 400MHz if its processor is of revision Y, which makes the PC slower. OPTION OVERCLOCK ON runs such a G1 at 480MHz, and the PC is then about as fast as on a G2. The start-up message shows which speed a Colour Maximite 2 runs at.

## What You Need

- The Colour Maximite 2 firmware, MMBasic 7.00.00RC1 or later (loadable-program services of version 9 or later).
- The emulator's files on the SD card (the next chapter).
- For MS-DOS and Windows 3.1, your own copies of their disks, as image files. FreeDOS, which is free, comes ready installed on a disk image.
- A USB keyboard. A mouse is needed for Windows.
