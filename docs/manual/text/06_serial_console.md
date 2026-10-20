# Serial Console

The serial console is a method of connecting your desktop or laptop computer to the Colour Maximite 2 and the MMBasic console. With access to the console you can enter and edit programs, run them, transfer files, etc.

Normally the VGA screen and the USB keyboard are used as the console, but the serial console works alongside them and just as well, including when using the File Manager and the Full Screen Editor. The only thing that the serial console does not show is graphics. It is particularly useful for transferring programs to and from your desktop computer (with the XModem and ZModem protocols), and for working on the Colour Maximite 2 from your desktop computer.

## USB Serial Port

The serial console uses the USB Type-B connector on the rear of the Colour Maximite 2, which is also its power connector. A USB to serial converter chip on the board (a Microchip MCP2221A on the first generation design, a CH340 on the second) makes it appear to your desktop computer as a normal serial port.

Windows 10 and later include the required USB device driver. For other operating systems (ie, Linux, Mac, Windows 8 and earlier) and the MCP2221A go to the Microchip website https://www.microchip.com/wwwproducts/en/MCP2221A for instructions (select the Documents tab). Support for the CH340 is included in most operating systems.

In Windows the Colour Maximite 2 will appear in Device Manager as "USB Serial Port" as illustrated below (the COM number will probably be different).

![Device Manager](pdf:15:458){width=8}

The MCP2221A converter chip tells the host by default that it only needs 100mA on the 5V pin and in some rare cases this can cause trouble (ie, failure to power up, random restarts, etc). The fix is to go to https://www.microchip.com/wwwproducts/en/MCP2221A and download the "MCP2221 Utility" (it is under the Documents tab) and use that to change the MCP2221A to request 500mA by default.

## Terminal Emulator

You also need a terminal emulator running on your desktop or laptop computer. This is a program that acts like an old fashioned computer terminal where it will display text received from a remote computer and any key presses will be sent to the remote computer over the serial link. The terminal emulator that you use should support VT100 emulation as this is required by the editor and file manager built into MMBasic.

For Windows users it is recommended that you use Tera Term as this has a good VT100 emulator and is known to work with the XModem and ZModem protocols which you can use to transfer programs to and from the Colour Maximite 2. Tera Term can be downloaded from: http://tera-term.en.lo4d.com.

The terminal emulator and the serial port that it is using should be set to the Colour Maximite 2 standard of 115200 baud, 8 data bits, no parity and one stop bit. The speed can be changed with the OPTION BAUDRATE command (the terminal emulator must then be set to match). A Generation 2 runs well at 460800 baud, which makes the editor and file transfers much faster; the USB to serial converter of a Generation 1 cannot run at that speed.

If you are using Tera Term do not set a delay between characters and if you are using Putty set the backspace key to generate the backspace character.

## The Console

Once you have connected your terminal emulator to the serial port you should be able to press return on your keyboard and see the MMBasic prompt, which is the greater than symbol (eg, ">"). If you reset the Colour Maximite 2 you will also see the MMBasic banner.

![Tera Term](docs/manual/images/serial_console_3.png){width=8}

This is the console and you use it to issue commands to configure MMBasic, load the BASIC program, edit and run it. MMBasic also uses the console to display any error messages.

The OPTION CONSOLE command sets where the console is: BOTH (the screen and the serial console, the default), SCREEN or SERIAL. See the chapter Options for the details.

## Apple Macintosh

The Apple Macintosh (OS X) is somewhat easier as it has the device driver and terminal emulator built in. First start the application 'Terminal' and at the prompt list the connected serial devices by typing in:

```
ls /dev/tty.*
```

The USB to serial converter will be listed as something like /dev/tty.usbmodem12345. While still at the Terminal prompt you can run the terminal emulator at 115200 baud by using the command:

```
screen /dev/tty.usbmodem12345 115200
```

By default the function keys will not be correctly defined for use in the built in program editor so you will have to use the control sequences as defined in the chapter Full Screen Editor of this manual. To avoid this you can configure the terminal emulator to generate these codes when the appropriate function keys are pressed.

Documentation for the screen command is here: https://www.systutorials.com/docs/linux/man/1-screen/

## Linux

For Linux see these posts:

- https://www.thebackshed.com/forum/ViewTopic.php?FID=16&TID=12171
- https://www.thebackshed.com/forum/ViewTopic.php?TID=14157&PID=175474#175474#175466

## Android

For Android devices see this post: https://www.thebackshed.com/forum/ViewTopic.php?FID=16&TID=17476&LastEntry=Y#230521#230517
