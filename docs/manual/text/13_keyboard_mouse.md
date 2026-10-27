# Keyboard, Mouse and Gamepad

The USB connector on the front panel of the Colour Maximite 2 is a USB host. It takes a keyboard and, through a USB hub, a mouse, a touch screen, up to two gamepads, up to four USB serial devices and a USB flash drive, all at the same time. A PS2 mouse can also be connected (see Mouse Support in the chapter Hardware Features), and the Wii Nunchuk and Wii Classic controllers plug into the connectors on the front panel.

The USB support is the PicoMite's, so programs written for the PicoMite read these devices the same way here.

## USB Hub

A USB hub can be plugged into the USB connector so that several devices can be used at once. The keyboard, the mouse and the gamepads are given a channel number: by default a keyboard is channel 1, a mouse channel 2, the first and second gamepads channels 3 and 4, and a touch screen channel 4 (or a free channel when 4 is in use). USB serial devices are numbered separately, as the serial ports COM4 to COM7.

The processor's USB controller cannot work with a low-speed USB device through a hub. Many inexpensive keyboards, mice and gamepads are low-speed devices. Such a device works when it is plugged straight into the USB connector; through a hub MMBasic refuses it with the message "USB low-speed device behind a hub is not supported: plug it in directly". Full-speed devices work through a hub.

When a device is connected or removed a message is shown on the console, for example "USB Keyboard Connected on channel 1" or "USB Mouse Disconnected". A device that MMBasic does not support is reported with its USB identity: "USB device (vid &H..., pid &H...) connected: not supported".

## USB Keyboard

When a USB keyboard is connected it will be immediately recognised (no configuration required). Keyboards with a wireless dongle work, and so do keyboards with a built-in hub or a built-in mouse.

The keyboard's layout is set with the command:

```
OPTION USBKEYBOARD layout
```

Where 'layout' is US, UK, DE (German), FR (French), ES (Spanish) or BE (Belgian). OPTION KEYBOARD REPEAT start, rate sets the time before a held key starts to repeat and the time between its repeats (in milliseconds). The PicoMite's OPTION KEYBOARD layout [, capslock] [, numlock] [, start] [, rate] sets them in one command, and whether Caps Lock and Num Lock are on when a keyboard is connected.

Note that the non US layouts map some of the special keys present on these keyboards but the corresponding special character may not display if it is not in the font used. Instead a standard ASCII character will be used.

The codes that the special keys generate (the function keys, the arrows and so on) are listed in Appendix I. KEYDOWN() returns the keys that are being held down, which is useful in games.

## USB Mouse

When a USB mouse is connected it will be immediately recognised (no configuration required) and MMBasic will allocate it to channel 2. It can be used in the editor (see the chapter Full Screen Editor) and read by a program.

Within a program the mouse is read with the DEVICE() function, as on the PicoMite:

```
' print any movement of the mouse on the console
Do
  mx = DEVICE(MOUSE 2, X)
  my = DEVICE(MOUSE 2, Y)
  If mx <> tx Or my <> ty Then Print mx, my
  tx = mx : ty = my
Loop
```

DEVICE(MOUSE 2, L), R and M return the state of the left, right and middle buttons, W the scroll wheel, and D a double click. The MOUSE INTERRUPT ENABLE command calls a subroutine when a button is clicked, and MOUSE SET sets the position of the mouse. The MOUSE() function (the Colour Maximite 2's) also reads it, as channel 2. OPTION MOUSE SENSITIVITY n divides the mouse's movement by n.

A program can also display a cursor on the screen and link it to the mouse using the GUI CURSOR command, and mouse clicks can be read with the CLICK() function - see the command and function listings for the details.

## PS2 and I2C Mice

A PS2 mouse (connected to the rear I/O connector on the Generation 1, or to the lower USB socket on the Generation 2) is mouse channel 0. Set with OPTION MOUSE 0 it is the computer's mouse, as a USB mouse is: programs read it with the MOUSE() function and link the GUI cursor to it without CONTROLLER MOUSE OPEN, which then only sets its interrupts. A Hobbytronic USB mouse interface is opened on an I2C channel with CONTROLLER MOUSE OPEN; the G2 V2.0 does not support it. See Mouse Support in the chapter Hardware Features.

## USB Touch Screen

A USB touch screen (a touch monitor, or a touch panel with a USB controller) is recognised as soon as it is plugged in, as on the PicoMite. Touch screens made for Windows work: a multi-touch screen reports up to ten fingers, and one with only single touch works as a pointer. It is given channel 4 (a free channel when 4 is in use), and the console shows its size, for example "Multi-touch Connected on channel 4 (10 contacts, 16383x9599)". MM.INFO(USB n) returns 133 for a multi-touch screen and 134 for a single-touch one.

A program reads it with the TOUCH() function: where the fingers are, whether the screen is touched, and the gestures (a swipe, a tap, a double tap, a long press, a pinch, a twist and a two-finger tap). The GUI controls respond to it as they do to the mouse (see the chapter GUI Controls), it types on the on-screen keyboard (below), and the editor uses it (see the chapter Full Screen Editor). GUI TEST TOUCH draws a dot wherever the screen is touched, to check one.

A touch monitor that is also the Colour Maximite 2's display may reset its touch controller when the screen's resolution changes (MODE): it disconnects and connects again, and touches are not seen for a moment.

The touch controller of some monitors (the WingCoolTouch/WCH controller, USB ID 27C0:0859, in the Corsair Xeneon Edge, the Verbatim HD14T and others) gives its multi-touch reports only to a computer it takes for one running Windows; MMBasic reads it as Windows does, so all ten fingers are seen.

## On-Screen Keyboard

With a USB touch screen MMBasic can show a keyboard at the bottom of the screen and type with it, as the PicoMite does. What is tapped arrives as it would from a USB keyboard: at the command prompt, in the editor and the file manager, and for INKEY$, INPUT and the like.

```
OPTION SCREEN KEYBOARD n [, M]
```

'n' is the percentage of the screen's height that the keyboard takes (0 to 50; 30 is a good size), and OPTION SCREEN KEYBOARD 0 removes it. The setting is kept in flash. The keyboard is shown at the command prompt and in the editor and the file manager, which keep above it; at the prompt the status line (OPTION STATUS) sits just above it. The text above the keyboard is a whole number of lines of the current font; any pixels left over are blank. While a program runs the keyboard is taken away and the program has the whole screen. A program shows it with KEYBOARD ON (the screen scrolls up to make room) and takes it away with KEYBOARD OFF, and when the program ends the keyboard is as it was at RUN. KEYBOARD OFF at the command prompt keeps it away until KEYBOARD ON or a restart. The keyboard is not available in MODE n,1.

It has four rows of twelve keys:

```
q   w   e   r   t   y   u   i   o   p   "   Bsp
a   s   d   f   g   h   j   k   l   ;   '   Ent
Sh  z   x   c   v   b   n   m   ,   .   /   Del
Sym Fn  Ctl Esc Tab   Space   <-  v   ^   ->
```

Sh gives capitals and the shifted punctuation, and Ctl a control key: Ctl then C stops a running program as Ctrl-C does. Sym changes the keys to digits and symbols, and Fn to the function keys (F1 to F12, Home, End, Page Up, Page Down, Insert and Delete) and more symbols. The arrows are the cursor keys.

Without M the modifiers latch: tap Sh, then 'a', to get 'A'; Sym and Fn stay until they are tapped again. With M (OPTION SCREEN KEYBOARD 30, M) they work as a keyboard's do: hold Sh with one finger and tap keys with another, and a modifier tapped on its own does nothing. This needs a multi-touch screen.

## USB Gamepads

Up to two USB gamepads can be connected through a hub (or one straight into the USB connector). The first is allocated to channel 3 and the second to channel 4. Within a program the gamepad is read using the DEVICE(GAMEPAD n, ...) function: LX, LY, RX and RY return the sticks (0 to 255), L and R the analog triggers, B the buttons as a bitmap, H the direction of the d-pad, T the type of gamepad, and on a PlayStation 4 controller GX, GY and GZ the gyroscope and AX, AY and AZ the accelerometer.

These gamepads are recognised:

- the PlayStation 4 DualShock 4 and its clones, and the PlayStation 3 DualShock 3 (which sends only its buttons until its PS button is pressed);
- the Nintendo Switch Pro Controller, and the gamepads that present themselves as one in their Switch mode;
- the Xbox 360, Xbox One and Xbox Series controllers;
- a number of generic gamepads, such as the USB versions of the Super Nintendo SNES controller.

The buttons of every gamepad are mapped onto DEVICE(GAMEPAD n, B) by their position, so a program works with any of them. On a Switch Pro Controller X, the top face button, is bit 10, A (right) bit 11, Y (left) bit 12 and B (bottom) bit 13; + is START, - SELECT and Capture the touchpad bit; ZL and ZR are L2 and R2 and read 255 or 0 as the analog L and R.

The GAMEPAD command sets up a gamepad: GAMEPAD INTERRUPT ENABLE calls a subroutine when a button is pressed, GAMEPAD HAPTIC drives a controller's vibration motors and GAMEPAD COLOUR sets the colour of a PlayStation 4 controller's light bar. GAMEPAD MONITOR shows what a gamepad sends and GAMEPAD CONFIGURE describes a generic gamepad that MMBasic does not recognise.

## USB Serial Devices (COM4 to COM7)

MMBasic can also communicate with USB serial devices. This includes USB to serial adapters based on the CH340, CP210x, FTDI and PL2303 chips as well as devices that present a USB serial interface of their own such as an Arduino, a Raspberry Pi Pico or a GPS receiver.

Up to four USB serial devices can be connected (via a hub) and they are allocated to the serial ports COM4, COM5, COM6 and COM7 in the order that they are plugged in. These are in addition to the three hardware serial ports COM1, COM2 and COM3. When a device is connected a message is shown on the console, eg, "USB serial device (CH34x) Connected as COM4".

The port is opened and used exactly like the hardware serial ports, eg:

```
OPEN "COM4:115200" AS #1
```

The baud rate and the 7BIT, EVEN, ODD and S2 options are passed to the USB device (where the device supports them); the DE, INV and OC options do not apply. A receive buffer size, an interrupt subroutine and an interrupt trigger level can be specified exactly as for COM1 to COM3 - see Appendix A (Serial Communications). The DTR and RTS signals are asserted while the port is open. A USB GPS receiver can be opened AS GPS like a GPS receiver on a hardware port.

If the USB device is unplugged while the port is open the port will remain open and communications will resume when the device is plugged back in. The CLOSE command closes the port and frees its receive buffer.

## USB Flash Drive

A USB flash drive (memory stick) can be connected, either directly or via a hub. The drive will be automatically recognised and the message "USB Flash Drive Connected" will be shown on the console.

The drive is accessed as drive C: and works alongside the SD card (A:, also called B:). All of the normal file commands can be used with it (FILES, RUN, EDIT, OPEN, COPY, etc) and it can be made the default drive with the command DRIVE "C:" or by simply typing C: at the command prompt. The file manager (FM) can copy files and directories between the SD card and the drive. The drive can be formatted as FAT16, FAT32 or exFAT.

## Wii Nunchuk and Classic Controllers

The Wii Nunchuk and the Wii Classic controllers plug into the connectors on the front panel, and up to three can be used (on the I2C channels 1 to 3). The Nunchuk controller is opened with the CONTROLLER NUNCHUK OPEN command and its status is queried using the NUNCHUK() function. The Classic controller is opened with the CONTROLLER CLASSIC OPEN command and its status is queried using the CLASSIC() function. See the chapter Game Playing Features.
