# Colour Maximite 2 Generation 1

This version uses a plug in CPU module made by Waveshare. The module carries the ARM Cortex-M7 32-bit RISC processor, 8MB SDRAM and various support components. The plug in board concept eliminates the need to solder the ARM CPU with its 176 pins and 0.2mm gap between pins and is therefore suitable for home construction.

The CPU module has two connectors on its top, one a micro USB the other a multi pin header. Both of these are only needed for firmware development and are not used in normal operation as a BASIC computer. For normal operation all the jumpers on this board should be removed, the power switch should be set to "5VIN" and the BOOT CONFIG switch should be set to "Flash".

The following image illustrates the major connectors and features of this design:

![Generation 1 (top)](pdf:10:26){width=14}

![Generation 1 (bottom)](pdf:10:27){width=14}

Many suppliers are offering a fully assembled version of this design. These do not use the plug in module concept, instead the ARM Cortex-M7 processor and its supporting components are soldered directly to the main PCB. This makes sense for a machine assembled board and is cheaper to supply.

The layout of this board is essentially the same except that there are no jumpers as on the Waveshare board. Some items are positioned differently as noted in the image below.

![Generation 1, fully assembled](pdf:10:28){width=12}

## External Oscillator

The main processor clock comes from an 8 MHz crystal on the Waveshare board which is driven by the on chip oscillator in the ARM processor. This arrangement can have some jitter on the clock which in turn may cause jitter or instability in the VGA image. However, in most cases, this design works well with the maximum 800x600 pixel VGA video resolution provided by the early firmware releases.

However, with firmware upgrades, it is now possible to generate a VGA image at much higher resolutions and these may display an instability caused by this jitter. The solution is to fit an external 8MHz oscillator to replace the crystal and oscillator used by the Waveshare board. If the higher resolutions are not required then, in most cases, this modification will not be required. This issue does not affect the single board versions (ie, fully assembled without the Waveshare module) as they use an external oscillator module.

Note that the following will require a level of skill in soldering surface mount parts. The following parts are required:

- 1 x SMD 1206 100nF ceramic capacitor
- 1 x 8MHz oscillator in 5.0 x 7.0 mm SMD package (QX7 XO 25ppm). Eg, RS Stock Nbr 813-6194.

The solder pads for these are located under the Waveshare board near the left hand 80 pin connector. The oscillator has a dot identifying pin 1 and this needs to be aligned with a small dot on the PCB silkscreen. Take care to not damage the 80-pin connector nearby and use the minimum of solder so that the solder does not short to the case of the oscillator. The capacitor should then be installed near the oscillator.

Nothing more is required and it is not necessary to remove the 8 MHz crystal from the Waveshare board. On power up the Colour Maximite 2 should operate as normal.

## Mouse Interface

A PS2 mouse or a dual USB/PS2 mouse with a PS2 adapter can be connected via the rear I/O connector. The mouse CLOCK (pin 5) connects to pin 33 of the I/O connector and mouse DATA (pin 1) connects to pin 32. Both must have 4.7KΩ pullup resistors to +5V. The mouse GND and +5V pins should be connected to the ground and 5V pins on the I/O connector.

![PS2 mouse connections](pdf:11:31){width=7}

A USB mouse can also be plugged into the USB socket on the front panel (see USB Connector in the chapter Hardware Features).

MMBasic has built in support for the mouse via the MOUSE() function. The mouse can also be used in the editor to select text, etc. A full description is provided in the chapter Hardware Features.

The other features of this board are common with the Colour Maximite 2 Generation 2 and are described in the chapter Hardware Features.
