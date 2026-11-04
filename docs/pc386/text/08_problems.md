# When Things Go Wrong

## Messages on the Serial Console

{widths: 20,32}
| Message | What to do |
| pc386: cannot open bios.bin | The emulator looks for its files in the current directory. CHDIR to its folder first, or give the files' full names in the configuration file. |
| pc386: cannot open file.ini | The configuration file is not in the current directory, or its name is mistyped. |
| pc386: file.ini: an error in line N | That line of the configuration file is not of the form name = value, or a section's name is not in square brackets. |
| pc386: making msdos.img, 64 MB ...: the card is full? | There was not room on the SD card for the new hard disk image. |
| pc386: the sound stream is at 44101 Hz, not 44100: its pitch is off | This is shown each time the emulator starts. The difference is far too small to hear. |
| A: name cannot be used | The floppy image is not on the card, or its size is not one of a floppy's (Disk Images). |

## The PC

- **The Ctrl+Alt keys do nothing.** Some keyboards do not report every combination of keys. Use the keypad's 1, 3 and 9 with Ctrl and Alt, or the serial console's Ctrl-] and Ctrl-N.
- **The mouse does not work in a DOS program.** Load a mouse driver: CuteMouse or MOUSE.COM.
- **A DOS program has no sound.** Run its set-up program and choose a Sound Blaster (16, Pro or plain) at port 220, IRQ 5, DMA 1 (high DMA 5), and an Adlib for music. Many programs also read the BLASTER variable (Using the PC).
- **A program says it needs a 486 or a Pentium.** Set gen = 4 or gen = 5 in the configuration file's [cpu] section. A program that uses instructions the emulator does not have may still not run.
- **A program asks for its original disk.** Its disk is copy-protected, and the protection is not in the image (Disk Images).
- **The PC is slow.** Set OPTION SD TIMING FAST; on a G1 of revision Y, set OPTION OVERCLOCK ON (Using the PC). A large Windows program is slow on any emulated 386.
- **DOS says a disk is damaged after the emulator was left.** The PC was switched off while it was writing (Leaving, in Getting Started). Run SCANDISK (MS-DOS) or CHKDSK on the disk, and keep copies of disk images you care about.
