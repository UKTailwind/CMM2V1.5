# Colour Maximite 2 Generation 2

This is an upgraded design with three main improvements over the first generation:

- It uses a larger RAM chip (32MB vs 8MB) which is used to support the top VGA resolution of 1920x1080 pixels, 24-bit colour depth and provide more RAM for BASIC programs.
- An 8-bit digital to analog converter is used for each colour channel allowing the Generation 2 computer to display true colour 24-bit images.
- The PCB is a 4 layer design with all components mounted on the top side. This makes it more suitable for machine assembly.

This version also has connectors for a second Wii controller and a PS2 mouse and the rear I/O connector is inverted to make it more convenient to use connectors and cables designed to suit the Raspberry Pi Hats.

This diagram lists the key hardware features:

![Generation 2 (top)](pdf:12:34){width=17}

![Generation 2 (bottom)](pdf:12:35){width=17}

## Mouse Interface

The Generation 2 design supports two types of mouse interface on its rear USB mouse socket (the lower socket).

The first is a dual USB/PS2 mouse, used in PS2 mode. Most wired mice will automatically switch between USB and PS2 modes and many come with a USB to PS2 adapter. This adapter simply signals the mouse to switch to PS2 mode via a pull-up resistor and provides the physical PS2 connector. The recommended mouse is the Microsoft Basic Optical Mouse v2.0, which is low cost and widely available.

On the latest board, the G2 V2.0, the mouse plugs straight into the lower USB socket on the rear of the Colour Maximite 2 and is used in PS2 mode. On earlier Generation 2 boards IC5 must not be populated, and all four solder jumpers marked "select USB or PS2 mouse" (diagram above) must be joined with a solder blob; the mouse then plugs into the same socket. A USB to PS2 adapter is not required (the Colour Maximite 2 will automatically switch the mouse into PS2 mode). The mouse is enabled with OPTION MOUSE 0.

The second type, on earlier Generation 2 boards only, is a full USB mouse through a Hobbytronics mouse controller chip (https://www.hobbytronics.co.uk/usb-host-soic) installed at the position for IC5 along with the associated supporting components. The latest board, the G2 V2.0, does not support the Hobbytronic interface, and Hobbytronics no longer appears to be trading.

A USB mouse can also be plugged into the USB keyboard socket on the front panel (see USB Connector in the chapter Hardware Features).

MMBasic has built in support for a mouse via the MOUSE() function. The mouse can also be used in the editor to select text and in the file manager to select files, etc. A full description is provided in the chapter Hardware Features.

## Power Switch Orientation

Near the power switch there is a three pin jumper marked PWR which is used to select the orientation of the power switch. If the two pins towards the back of the Colour Maximite 2 are connected the power switch will operate as expected in the UK, Australia and New Zealand (down is on). If the two pins towards the front of the computer are connected the power switch will operate as expected in North America (up is on).

## High Accuracy Real Time Clock

The basic design uses a miniature 32KHz crystal and the real time clock hardware built onto the ARM Cortex-M7 processor to keep the date and time. Optionally you can add an external DS3231MZ+ real time clock to the PCB (illustrated in the above photo) and it will be kept alive by the coin battery on the PCB. This clock will typically lose or gain only a couple of seconds in a week.

To enable this time source the command is: OPTION DS3231 ON

Once enabled the time and date can be set and retrieved from the DS3231MZ+ using the normal MMBasic DATE/TIME commands and functions.

## ESP-01 Module

There is a footprint on the PCB for connecting an ESP-01 WiFi module to COM2. The firmware allows COM2 to be used as a serial console (OPTION CONSOLE PORT) so if the correct USB/UART code is loaded into the ESP-01 it will be possible to access the console of the Colour Maximite 2 remotely.

MMBasic does not include any Internet functions using this device but communications can be established using the normal serial communications protocol included in MMBasic.

## Comparing Page Buffers between Gen 1 and Gen 2

![Page buffers, 1](pdf:13:41){width=17}

![Page buffers, 2](pdf:13:39){width=17}

![Page buffers, 3](pdf:13:40){width=17}

The other features of the Generation 2 board are common with the first generation Colour Maximite 2 and are described in the chapter Hardware Features.
