# MicroPython

MicroPython is the Python language for small computers. This is the MicroPython of the Pico Computer 3, so programs written for that run here too.

## Starting It

micropython.app goes in the root of the card. At the MMBasic prompt:

```
RUN "A:/micropython.app"
```

It runs boot.py and main.py if they are in the current folder, and then shows its >>> prompt. To run one program and go straight back to MMBasic, name it:

```
RUN "A:/micropython.app", myprog.py
```

## Using It

The prompt appears on the screen, and on the serial console. Type Python at it:

```
>>> print("Hello", 6 * 7)
Hello 42
```

These are ready at the prompt as it starts:

{widths: 16,36}
| Name | What it does |
| ls(), cd(), cat() | List a folder, change folder, show a file |
| edit() | The editor |
| run() | Run a program |
| fm() | The file manager |
| plot(), Turtle() | Drawing and turtle graphics |
| play(), tone() | Sound |
| draw_jpg() | Show a JPEG picture |
| keydown(), mouse() | Read the keyboard and the mouse |

## The Keys

{widths: 16,36}
| Keys | What they do |
| Ctrl-C | Cancels a line, or stops a running program |
| Ctrl-D | A soft reset, which starts MicroPython again |

To leave MicroPython and go back to MMBasic, type:

```
import machine
machine.reset()
```

## Your Files

The SD card is at / and also at /sd. A USB flash drive is at /usb when one is plugged in. Modules of your own can go in /lib.

## What It Has

- The Pico Computer 3's modules for the display, the keyboard, the mouse, sound and pictures.
- SQLite, the database, and ulab, for arrays and fast numerical work.
- The machine module, for the rear I/O pins, by the PicoMite's GP numbers.

Python runs about twice as fast here as it does on the RP2350.
