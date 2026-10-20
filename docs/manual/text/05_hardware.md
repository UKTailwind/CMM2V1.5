# Hardware Features

The Colour Maximite 2 is based on an ARM Cortex-M7 32-bit processor which provides nearly all of the services for the user. This includes the flash memory (where the BASIC interpreter is installed), RAM and a sophisticated video display controller which generates the high quality VGA image with many advanced features useful to games programmers.

## STM32H743IIT6 Cortex-M7 32-bit RISC CPU

This is a high performance 32-bit ARM processor with 2MB of flash and 1MB of RAM. It includes a sophisticated video display controller which is used to generate a stable VGA output. The MMBasic firmware is loaded onto the flash memory of this chip and provides the Colour Maximite 2 with its personality.

There are two versions of this chip, Rev Y with a maximum speed of 400MHz and Rev V with a speed of 480MHz. Most vendors do not allow you to specify the version in advance so, depending on the supply pipeline, you may get either. MMBasic will automatically accommodate either revision and you can tell which one you have with the MM.INFO(CPUSPEED) function which will return 400000000 for the Rev Y chip and 480000000 for the Rev V chip. Other than this there is no difference when used in the Colour Maximite 2.

## VGA Monitor Connector

This is the main video output and it generates standard VGA signals in a variety of resolutions and number of colours as determined by the MODE command. These range from 800x600 pixels (the default at power up) up to 1280x1024 and down to 240x216 pixels with 256, 4096 or 65536 colours. Most modes work perfectly with monitors that have an aspect ratio of 4:3 or widescreen monitors that can switch to that ratio (most widescreen monitors will do this automatically). The Generation 2 Colour Maximite 2 can go to 1920x1080 pixels and 24-bit colour depth at some resolutions.

Note that on first setup some monitors may truncate the text on the margins or show an image that seems to shimmer or flicker. In most cases this can be fixed by pressing the auto setup button on the monitor or, failing that, using the monitor's image setup mode to adjust parameters such as the clock, phase and position.

If an HDMI output is required it is recommended that an inexpensive VGA to HDMI converter be used. These cost about US$10+ on eBay and will also encode the audio from the computer so that it can play through the monitor's speakers.

## USB Connector

This is the Type A USB connector on the front panel. It accepts a standard USB keyboard, including most that have a wireless dongle for the USB connection and keyboards with a built-in mouse or hub. With a USB hub it also accepts:

- a USB mouse, which is mouse channel 2 (see Mouse Support below);
- up to two USB gamepads, which are game controller channels 3 and 4 (see the chapter Keyboard, Mouse and Gamepad);
- USB serial devices, such as USB to serial adapters and GPS receivers, which appear as the serial ports COM4 to COM7;
- a USB flash drive, which appears as drive C:.

The processor's USB controller cannot work with a low-speed USB device through a hub. Many inexpensive keyboards and mice are low-speed devices. Such a device works when it is plugged straight into the socket; through a hub MMBasic refuses it with the message "USB low-speed device behind a hub is not supported: plug it in directly". Full-speed keyboards and mice work through a hub.

When a keyboard is connected the SD card activity LED will be illuminated. On the first access of the SD card the LED will revert to its normal action (illuminated during SD card access).

The firmware can accommodate a number of different keyboard layouts: UK, US, German (DE), French (FR), Spanish (ES) and Belgian (BE). The layout is asked for when MMBasic is first loaded and can be changed with the OPTION USBKEYBOARD command.

## Power & Serial Console Connector

This USB B connector is for power and the serial console over USB. The power requirement is 5V at 160mA to 250mA (typical). This is within the capabilities of most USB chargers however some PCs (especially older laptops) may have trouble supplying this. If your Colour Maximite 2 is suffering from intermittent issues such as reboots, errors reading the SD card, etc then it would be worth changing the power supply to one with a much higher capacity (for example, 2 amps or more).

The serial console is available if the Colour Maximite 2 is connected via this port to a personal computer: it appears there as a normal serial port, at 460800 baud. The console is used to write and debug BASIC programs and configure the computer. How to connect to it and set up a terminal emulator is described in the chapter Serial Console.

## Mouse Support

A USB mouse plugged into the USB socket on the front panel (directly, or through a hub if it is a full-speed mouse) is mouse channel 2. It needs no configuration: the editor uses it whenever it is connected. OPTION MOUSE SENSITIVITY n divides its movement by n: a number above 1 slows the mouse down and a number below 1 speeds it up (0 or 1 leaves it as it is).

A PS2 mouse or the Hobbytronic USB mouse interface is configured for the editor with this command (use channel 0 for a PS2 mouse):

```
OPTION MOUSE channel [,sensitivity]
```

The optional sensitivity parameter allows you to tune the sensitivity of the Hobbytronic interface by specifying a number between 0 and 10. Zero indicates that the default automatic sensitivity should be used while other values will explicitly set the sensitivity as per the Hobbytronic documentation.

For a PS2 mouse the sensitivity can be set between 0 and 8. Values of 1-4 set the resolution of the mouse to 1, 2, 4 or 8 counts/mm. Values 5-8 do the same but in addition enable mouse scaling which gives a non-linear relationship between speed and count (note that not all PS2 mice implement these settings).

OPTION MOUSE OFF stops the editor using a PS2 or Hobbytronic mouse.

Within the editor the mouse works as you would expect. For example, you can use the mouse to point and select text, scroll using the scroll wheel, and so on. For the details see the chapter Full Screen Editor.

Within a MMBasic program the mouse can be enabled for access with various sensitivity settings and interrupts on left and/or right clicks. Specialised built in functions can be used to determine the current mouse position in pixels and detect single or double clicks.

## Audio Connector

This is a 3.5mm stereo phono socket. The tip is the left channel, the ring is the right channel while the sleeve is ground. The signal level at full volume is about 1V RMS (approx 3V peak to peak). MMBasic can generate audio in many formats ranging from simple sine wave tones through to playing WAV, FLAC, and MP3 files.

Note:

- The output is high impedance suitable for feeding into an amplifier. It cannot directly drive a loudspeaker, headphones or any low impedance load and might be damaged if that was attempted.
- If the audio is garbled and/or lacks normal bass it might indicate that your amplifier has a low input impedance. Try adding a 4.7K resistor in series with the output (the Generation 2 board has these permanently installed).
- There is a DC offset on the output. Most amplifiers have an input capacitor so this has no effect but, if not, it may be necessary to add a 4.7µF capacitor in series with the output (positive leg to the Colour Maximite 2).

## Power & SD Card Activity LEDs

The power indicator (green) is illuminated whenever power is applied.

The SD Card activity indicator (red) will illuminate briefly when reading or writing from/to the SD card. The SD card should not be removed when this LED shows activity. The red LED will also illuminate when a USB keyboard is plugged in and successfully communicates with MMBasic - it then reverts to normal activity on the first read/write to the SD card.

## SD Card Connector

The SD card connector on the front panel is the main storage for the Colour Maximite 2. All programs reside on the SD card so it must be present for most operations. This is different from the original Colour Maximite where the SD card was not necessarily required.

The Colour Maximite 2 will support cards up to 128GB. Cards larger than 32GB should be formatted as exFAT and cards 32GB or less formatted as FAT32. Note that very small cards may not be reliable and exFAT is much slower than FAT32. BASIC programs are quite small so an 8GB or 16GB card formatted as FAT32 is ample and also readily available at a cheap price.

The SD card is drive A:. It can also be called B: (as the PicoMite names its SD card). A USB flash drive is drive C:.

## External I/O Connector

This 40-pin ribbon connector provides 28 input/output pins which can be controlled from within the BASIC program plus 3.3V and 5.0V outputs for powering external circuitry plus a number of ground pins.

The total current drawn from all 3.3V pins should be limited to 100mA. Exceeding this may cause the voltage regulator on the CPU module to enter thermal shutdown. The capacity of the 5V supply is limited only by your USB power supply.

If more input/output pins are required an I/O expander such as the Microchip MCP23017 can be used.

### Colour Maximite 2 Generation 1

This is the image of the connector as seen from the rear of the first generation Colour Maximite 2:

![Generation 1 connector](pdf:16:51){width=15}

These numbers are used as reference in the table below and by MMBasic to refer to an I/O pin. They are not the connector manufacturer's standard pin numbers.

The pin layout and positioning of special functions on the external I/O connector is compatible with the Raspberry Pi allowing Pi HATs to be connected if required.

Note that most Raspberry Pi cables for use with solderless breadboards need to be inverted when plugged into the Colour Maximite 2's rear I/O connector (ie, the key tab on the connector needs to be on the bottom). If this is not done the pin legends on the breadboard connector will be wrong. This is not a problem if the Maximite connector is unshrouded otherwise you may need to cut a new keyway on the bottom of the shroud with side cutters (not hard to do).

### Colour Maximite 2 Generation 2

The second generation Colour Maximite 2 has these pins reversed so that the issue described above (inverted connector) is avoided. On this version Raspberry Pi cables can plug directly in - there is no need to cut a second keyway.

This is the image of the connector as seen from the rear of the second generation Colour Maximite 2:

![Generation 2 connector](pdf:17:55){width=15}

These are the connector manufacturer's standard pin numbers. These numbers are used as reference in the table below and by MMBasic to refer to an I/O pin.

### I/O Pin Capabilities

The capabilities of each pin are (5V: the pin tolerates inputs up to 5.3V):

{widths: 6,38,6,6,38,6}
| Pin | Features | 5V | Pin | Features | 5V |
|---|---|---|---|---|---|
| 1 | 3.3 Volt Power |  | 2 | 5.0 Volt Power |  |
| 3 | I2C SDA / CAN1 Tx | 5V | 4 | 5.0 Volt Power |  |
| 5 | I2C SCK / CAN1 Rx | 5V | 6 | Ground |  |
| 7 | Analog Input or COUNT 1 |  | 8 | Analog Input or COM1: TX |  |
| 9 | Ground |  | 10 | Analog Input or COM1: RX |  |
| 11 | COM2: RX | 5V | 12 | Analog Input or PWM 1A |  |
| 13 | Analog Input or COUNT 2 |  | 14 | Ground |  |
| 15 | Analog Input or COUNT 3 |  | 16 | Analog Input or COM2: TX |  |
| 17 | 3.3 Volt Power |  | 18 | FAST COUNT | 5V |
| 19 | SPI MOSI / CAN2 Rx | 5V | 20 | Ground |  |
| 21 | SPI MISO | 5V | 22 | Analog Input or PWM 1B |  |
| 23 | SPI CLOCK | 5V | 24 | Analog Input or COUNT 4 |  |
| 25 | Ground |  | 26 | Analog Input |  |
| 27 | I2C2 SDA | 5V | 28 | I2C2 SCK | 5V |
| 29 | Analog Input or PWM 1C |  | 30 | Ground |  |
| 31 | PWM 2B | 5V | 32 | General I/O | 5V |
| 33 | General I/O | 5V | 34 | Ground |  |
| 35 | SPI2 MISO | 5V | 36 | PWM 2A | 5V |
| 37 | Analog Input or COM1 DE |  | 38 | SPI2 MOSI | 5V |
| 39 | Ground |  | 40 | SPI2 CLOCK / CAN2 Tx | 5V |

Except for the power and ground pins all pins can be used for digital I/O using the PIN() function and command and using the pin number as the reference. For example pin 3 can be set to an output using SETPIN 3, DOUT and then the pin set high (ie, to 3.3V) using the command PIN(3) = 1.

All pins marked 5V can tolerate inputs up to 5.3V; all other pins can tolerate up to 3.6V.

Pin 40 can be used to completely reset the Colour Maximite 2 to its "factory default" condition. If that pin is connected to ground (ie, pin 39) on power up all options will be reset to their defaults and any program in program memory erased. Note that external circuitry connected to this pin (eg, a capacitor) must not look like a short circuit at power up as this might trigger a reset.

There are 12 pins marked as supporting analog input and these can be used for measuring voltages. The other special capabilities (eg, COM2: RX, etc) are described in the relevant section of this manual.

Both the data line (SDA) and clock (SCL) for both I2C ports have 10K pullup resistors (to 3.3V) installed on the motherboard so external pullup resistors are not required. Note that these may interfere with the operation of these pins if they are used as general purpose inputs.

## Wii Nunchuk and Wii Classic Connector

The Colour Maximite 2 includes connectors on the front panel for either the Wii Nunchuk (illustrated below) or the Wii Classic game controllers and MMBasic includes commands and functions to work with both of them. Many games written for the Colour Maximite 2 will use either controller to control the game play so they are a useful addition if you plan on playing some games.

![Wii Nunchuk](pdf:18:58){width=4}

For more details on the Nunchuk and Classic search the Internet for "Nintendo Nunchuk" or "Nintendo Classic". Clones of both of these can be purchased cheaply for US$10 to US$40.

The connector used by the Wii controllers has a large plastic clip on top and this must be uppermost when it is plugged in. Take care as it is possible to insert the connector upside down and that may damage the controller or the Colour Maximite 2.

## Temperature Sensor

These solder pads are for an optional Dallas DS18B20 temperature sensor. Support for the DS18B20 is built into MMBasic - see the chapter Special Device Support in this manual for the details. The signal line for this sensor is pin 42. If this sensor is installed the associated pullup resistor (4.7KΩ) on the motherboard must also be installed.

## IR Receiver

These solder pads are for an IR remote control receiver which will allow the Colour Maximite 2 to be controlled via a standard NEC or Sony infra red remote control transmitter. See the chapter Special Device Support for details of the receivers that can be fitted and how to use them in your program. The signal pin for the receiver can also be used as a digital I/O pin as pin 41.

## Firmware Upload Select

This is either a switch or a jumper. Normally the switch should be set in the "Flash" position or, if a jumper is used, it should be in the "RUN" position.

When uploading new firmware the switch is set in the "System" position and the jumper should be moved to the "PRG" position. See Appendix K (Loading the Firmware) at the end of this manual for the details.

## Backup Battery

The CR1220 coin cell battery on the motherboard keeps the internal ARM Cortex-M7 real time clock running while the power is off and also keeps a bank of 4KB RAM alive at the same time. The real time clock is used to provide the correct time to MMBasic on startup and the battery backed RAM is used to store saved variables (see VAR SAVE). The options are kept in the flash memory and do not need the battery. The life of this battery will be about 3 to 4 years of normal use.

## Reset Switch

This is used to reset the Colour Maximite 2 and start the bootup sequence as if the power had been cycled.
