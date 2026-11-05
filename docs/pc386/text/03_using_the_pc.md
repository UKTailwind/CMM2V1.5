# Using the PC

## Starting a PC

```
RUN "pc386.app" [, file.ini] [-stats]
```

Without a file name, the emulator starts the PC that pc386.ini describes. The configuration file and the files it names are found in the current directory, so CHDIR to the emulator's folder first, or give their full names (A:/pc386/bios.bin).

With -stats, the serial console shows every 5 seconds how fast the PC is running, in millions of instructions a second (MIPS), and how much of the time goes on drawing the screen. Two more words are there for finding faults: -keys shows each report from the keyboard, and -mouse each movement of the mouse.

## The Emulator's Keys

Three keys, held with Ctrl and Alt, are the emulator's own. The PC does not see them:

{widths: 18,34}
| Keys | What they do |
| Ctrl+Alt+End | Leave the emulator and go back to MMBasic. |
| Ctrl+Alt+Page Down | Put the next floppy image of the list in drive A:. |
| Ctrl+Alt+Page Up | Put the previous floppy image in drive A:. |

Either Ctrl key and either Alt key will do. The keypad's 1 (End), 3 (Page Down) and 9 (Page Up) work too, whether Num Lock is on or off.

Every other key goes to the PC, Ctrl+Alt+Delete included: it restarts the PC, as it would a real one. Ctrl+C goes to the PC as well; it does not stop the emulator.

## The Keyboard

The PC is told which key was pressed, by its place on the keyboard, as a real PC's keyboard tells it. DOS turns the keys into characters with its own keyboard layout, which is the US layout unless you load another. For a UK keyboard, for example, put this in AUTOEXEC.BAT:

```
KEYB UK
```

## Changing Floppy Disks

The configuration file can give drive A: a list of floppy images (see The Configuration File). The first is in the drive when the PC starts. Ctrl+Alt+Page Down puts the next one in, and Ctrl+Alt+Page Up the one before, going round the list. The word none in the list is an empty drive.

Each time, the top left corner of the screen shows for 3 seconds what is in the drive, for example:

```
A: DISK2.IMG
```

To the PC this is a disk taken out of the drive and another put in. Use it when an installation program asks for its next disk: change the disk, then press Enter.

## The Serial Console

While the emulator runs, a terminal on the serial console can type into the PC: letters, digits, punctuation, Enter, Tab, Backspace and Esc, as on a US keyboard. What it types while a key of the USB keyboard is held is left out. These keys are the emulator's:

{widths: 10,42}
| Key | What it does |
| Ctrl-] | Leave the emulator. |
| Ctrl-N | Put the next floppy image of the list in drive A:. |
| Ctrl-T | Print the PC's text screen on the terminal. |
| Ctrl-A | Hold Alt for the next key: Ctrl-A then F opens a Windows program's File menu. |

The emulator also prints its messages there, such as which floppy image is in drive A:.

## The Mouse

The Colour Maximite 2's mouse is the PC's PS/2 mouse: a USB mouse, or a PS/2 mouse set up with OPTION MOUSE. DOS programs need a mouse driver to use it: the FreeDOS disk loads CuteMouse, and with MS-DOS you can load CuteMouse or Microsoft's MOUSE.COM. Windows 3.1 has its own driver, which it chooses as it is installed.

## The Serial Port

A line in the [pc386] section of the configuration file gives the PC a serial port:

```
com1 = 9600
```

The PC's COM1 (port 3F8, IRQ 4) is then the Colour Maximite 2's COM1, on its rear connector:

{widths: 10,42}
| Pin | Signal |
| 8 | GP0: what the PC sends. |
| 10 | GP1: what the PC receives. |
| 9 | Ground. |

These are 3.3 V signals and they are not 5 V tolerant. Connect them to another 3.3 V serial port, such as a 3.3 V USB serial adapter, and not straight to an RS-232 port.

The speed is the one in the configuration file, with 8 data bits, no parity and 1 stop bit. What a program in the PC sets makes no difference, so set the other end to the speed in the file.

There are no handshake lines, and neither the PC nor the Colour Maximite 2 can hold the other end back. What arrives waits in a buffer of 1024 bytes until the PC takes it, and anything more is lost. A program's XOFF goes out as an ordinary character, and stops the other end only if that end obeys it.

- A simple DOS program that took the port by interrupts kept up at 115200 baud.
- Windows 3.1's Terminal is slower. Choose COM1 when it first asks for a port. Its Transfers menu sent and received files by XModem without a fault at 9600 and 38400 baud, at about 800 and 2,000 bytes a second.
- Text shown in Terminal, or saved with Receive Text File, goes at only about 260 characters a second. Terminal sends XOFF within about 900 characters, so the other end has to obey it. For a long text, use XModem.

## The Screen

The screen is 640 x 480 in 256 colours (MODE 8,8) on every Colour Maximite 2:

- Text is drawn with characters 8 pixels wide, so 80 columns fill the width of the screen.
- A graphics mode up to 640 x 480 is shown whole, in the middle of the screen. The 320 x 200 modes of DOS games are shown twice as wide and twice as tall.
- A larger VESA mode shows only its middle 640 x 480.
- The colours are the PC's own, as its VGA card has them. The VESA modes of more than 256 colours are shown in 256 fixed colours, so their colours are close but not exact.

## Sound

The Sound Blaster 16, the Adlib and the PC speaker are mixed into the Colour Maximite 2's audio output, at the level set by PLAY VOLUME. DOS programs find the Sound Blaster at these settings, which their set-up programs ask for:

{widths: 18,34}
| Setting | Value |
| Port | 220 |
| IRQ | 5 |
| DMA | 1 (high DMA 5) |

Many programs read them from the BLASTER variable, which AUTOEXEC.BAT can set:

```
SET BLASTER=A220 I5 D1 H5 T6
```

Programs that play music on an Adlib or a Sound Blaster's FM synthesiser find it at port 388.

## Making It Faster

- OPTION SD TIMING FAST makes the SD card, and so the PC's disks, much faster.
- On a G1 whose processor is of revision Y, OPTION OVERCLOCK ON runs the processor at 480MHz in place of 400MHz.
