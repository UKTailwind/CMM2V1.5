# The VT100 Terminal

This program makes the Colour Maximite 2 a terminal: the screen and keyboard of another computer. Plug a PicoMite into it and you can use the PicoMite's prompt, its editor and its file manager, in colour, with the mouse, without a PC.

It has a setup menu, scrollback, a log, and XMODEM for moving files. It is a VT100 and VT102, with xterm's 256 colours and mouse reports added, and VT52 mode. It uses libvterm, with the Spleen fonts.

It needs firmware 7.00.00RC3 or later for USB serial devices.

## Connecting

{widths: 16,36}
| Port | What it is |
| COM4 to COM7 | USB serial devices, such as a PicoMite or a USB serial adapter, in a USB socket or through a hub |
| COM1 | The serial port on GP0 and GP1 of the rear connector |
| COM2 | The serial port on GP36 and GP37 of the rear connector |

The rear connector's serial ports use 3.3 V signals and are not 5 V tolerant. Do not connect them to an RS-232 port.

When a USB serial device is plugged in, the MMBasic prompt shows which port it is, starting at COM4.

## Starting It

```
CHDIR "A:/vt100"
RUN "vt100.app", COM4 115200
```

The port and speed can be left out once they are saved in the setup menu. Any setting can be given on RUN's line by name, such as font=3.

The line at the bottom of the screen shows the port, its speed and the size of the terminal.

## Leaving It

Press **Shift-F12**, or Pause, to open the setup menu, then choose **Quit**.

**Ctrl-C does not leave the terminal.** It goes to the computer at the other end, as it should, to stop a program there.

## Using It With a PicoMite

Everything works as it does on a terminal on a PC:

- The PicoMite's prompt, EDIT and FM work with the arrow keys, the function keys and the mouse.
- When the PicoMite asks for a screen size, at EDIT, FM and OPTION DISPLAY, the terminal changes to it. It tries a smaller font in the same video mode first, so your monitor rarely has to lock on again.
- Colours are shown as the PicoMite sends them.

The block on the screen in EDIT is the mouse pointer, and the underline is the text cursor.

### If the PicoMite Restarts

Many PicoMite OPTION commands restart it, and it also drops off when you unplug it. You do not have to do anything: the status line says **COM4 disconnected, waiting for it to come back**, and when the device returns the terminal carries on by itself.

## The Keys

{widths: 20,32}
| Keys | What they do |
| Shift-F12, or Pause | The setup menu |
| Shift with Page Up, Page Down | Look back through what has scrolled off the screen |
| Shift with Home, End | The top of the scrollback, and back to the live screen |
| The mouse wheel | Scrollback too, unless the other computer is using the mouse |
| Any other key | Goes back to the live screen, and to the other computer |
| Ctrl-J | Sends a line feed, on its own |

## The Setup Menu

Up and Down choose a setting, and Left, Right or Enter change it. Esc goes back to the terminal. Choose **Save settings** to keep your changes, which go in vt100.cfg in the current folder.

{widths: 18,34}
| Setting | What it does |
| Port, Speed, Format | The connection. Format is the data bits, parity and stop bits |
| Screen | The video mode |
| Font | From 5 x 8 to 16 x 32. 8 x 14 is the default |
| Columns, Rows | The terminal's size. All uses the whole screen |
| Status line | The line at the bottom |
| Characters | How bytes above 127 are shown: CP437 (as a PicoMite has them), Latin-1 or UTF-8 |
| Text colour | White, green or amber, like a monitor of the time |
| Cursor | Underline, Block or Bar |
| Backspace sends | BS (8) or DEL (127) |
| Enter sends | CR, or CR LF. A PicoMite wants CR |
| LF received as | LF, or CR LF. This is for what arrives, and does not change what your keys send |
| Local echo | Shows what you type, for a computer that does not send it back |
| Text line delay | The pause after each line when sending a text file |
| Send text file... | Sends a file as if you had typed it |
| XMODEM send..., XMODEM receive... | Moving files (below) |
| Log to file... | Records everything that arrives in a file |
| Reset the terminal | Puts the terminal back as it starts |
| Save settings | Keeps these settings |
| Quit | Leaves the terminal |

## Moving Files With XMODEM

Files can be moved each way, between the SD card and a PicoMite or other computer.

### From the Colour Maximite 2 to a PicoMite

1. At the PicoMite's prompt, type XMODEM RECEIVE "name.txt" and press Enter.

2. Straight away press Shift-F12, choose **XMODEM send...** and press Enter.

3. Pick the file from the list and press Enter.

The PicoMite waits only a limited time, so go straight on from step 1 to step 2.

### From a PicoMite to the Colour Maximite 2

1. At the PicoMite's prompt, type XMODEM SEND "name.txt" and press Enter.

2. Press Shift-F12, choose **XMODEM receive...** and press Enter.

3. Type a name for the file and press Enter.

Esc stops a transfer. Retries in the progress display mean the line is having trouble, even if the file arrives whole.

### File Sizes

XMODEM sends files in blocks of 128 bytes with no length, so the last block is padded. A text file arrives exactly as it was. A binary file keeps the padding, and can arrive up to 127 bytes longer. Nothing is lost.
