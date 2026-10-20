# Appendix K – Loading the Firmware

## Loading the Firmware

The ARM Cortex-M7 processor includes its own programmer/bootloader so the Colour Maximite 2 firmware can be easily loaded via USB using a personal computer or laptop (special hardware is not needed). Just follow these steps. Note that the same firmware file will work on either the Generation 1 or 2 versions of the Colour Maximite 2 (the hardware is automatically detected).

If you have a version using the Waveshare CPU board:

- Plug the Waveshare CPU board into its position on the motherboard.
- Remove all the jumpers on the Waveshare CPU board.
- Set the power switch to "5VIN".
- Set the BOOT CONFIG switch to "SYSTEM".

If you have a version without the Waveshare module (ie, the Generation 2 design) the programming function is provided by a set of three jumper pins labelled "PGM" and "RUN". Place a jumper on the common and "PGM" pins.

Make sure that the Type-B USB power cable to the Colour Maximite 2 is disconnected.

Using a USB Type-A to Type-A cable connect the USB Keyboard port on the Colour Maximite 2 to a USB port on your desktop computer. This will power up the Colour Maximite 2 regardless of the position of the power switch. You should also hear a sound from your computer as it connects to the Colour Maximite 2.

Go to https://www.st.com/en/development-tools/stm32cubeprog.html and download the STM32CubeProgrammer software. This is free software but STM do require you to have an STM account or provide your name and email address. They will email you a link to download the software. Then install this software on your computer (Windows, Linux and macOS are supported).

Run the STM32CubeProgrammer software on your computer. On the top right of the program window select USB as the communications method. If the program does not recognise the USB connection click on the small blue circle to the right of the Port drop down list to refresh the entry. Your screen should look like the illustration below (the USB port number may vary).

![USB selected](pdf:177:437){width=8}

Click on the "Connect" button. You should then see a series of messages as shown in the screenshot below finishing with the message "Data read successfully". Any messages in red will indicate an error.

![Connected](pdf:177:436){width=12}

Click on the download button on the left side of the STM32CubeProgrammer window and the software will switch to the "Erasing and Programming" mode as shown below.

![Erasing and Programming](pdf:178:443){width=12}

Use the "Browse" button to select the firmware file (it will have an extension of .bin). Tick the "Verify programming" checkbox. Finally, click on the "Start Programming" button.

The STM32CubeProgrammer software will then program the firmware into the flash memory on the ARM Cortex-M7 in the Colour Maximite 2 (the STM32CubeProgrammer software calls this "downloading"). After a short time a dialog box will pop up saying that "File download completed". Do not do anything at this point as the software will then start reading back the firmware programmed into the flash. When this has completed successfully another dialog box will pop up saying "Download verified successfully" as shown below.

![Download verified](pdf:178:444){width=15}

The whole operation will take under a minute and any messages in red will indicate an error. Then:

- Dismiss all the dialog boxes and close the STM32CubeProgrammer software.
- On the Waveshare module set the BOOT CONFIG switch to "Flash" or on versions without the Waveshare module the jumper should be between the common and the "RUN" pins.
- Remove the USB Type-A to Type-A cable from the USB Keyboard port.
- Plug in the VGA monitor and USB keyboard.
- Plug the Colour Maximite 2 into power and set the front panel power switch to ON.

On power up you should now see on the VGA monitor the Maximite logo and the version number of the firmware that you have just loaded.

If your monitor remains blank check that you are not still in programming mode (ie, reset the BOOT CONFIG switch or the programming jumper). If you do not have a VGA monitor you can use the serial console (over USB) to check the firmware installation. This is described in the section Power & Serial Console Connector in the chapter Hardware Features.

When MMBasic is first loaded (and whenever the clock has stopped, for example because its battery has failed) it will ask for the keyboard type, the time and the date. On a firmware upgrade MMBasic keeps these options and does not ask for them again: the keyboard type, the clock's settings (OPTION RTC CALIBRATE, OPTION RTC DRIVE and OPTION DS3231), the console port, the SD card speed, the default video mode and the editor's font, the mouse and OPTION GUI CONTROLS. The other options return to their defaults (the console's baud rate among them) and can be set again with the relevant OPTION commands. Saved variables (VAR SAVE) are cleared by an upgrade.

If you wish to load another version of the firmware (either earlier or later) this can be done by repeating the steps above.

## Alternative Method

An alternative method of loading the firmware is to use serial transfer over USB. To do this you must first install the serial over USB driver on your desktop computer as described in the section Power & Serial Console Connector in the chapter Hardware Features.

If you have a version using the Waveshare CPU board:

- Remove all the jumpers on the Waveshare CPU board.
- Set the power switch to "5VIN".
- Set the BOOT CONFIG switch to "System".

If your version does not use the Waveshare CPU board:

- Place a jumper between the common and "PGM" pins on the mainboard.

Then:

- Connect your desktop computer to the USB Type-B connector on the Colour Maximite 2.
- Set the front panel power switch to ON and/or press the reset button on the Waveshare CPU board.

This should power up the Colour Maximite 2 which will then connect to your desktop computer via USB. In Windows the connection will appear in Device Manager as "USB Serial Port" (the COM number will probably be different):

![Device Manager](pdf:179:458){width=7}

Install the STM32CubeProgrammer software on your computer as described above. Run the software and select UART in the top right corner (as illustrated below). Then select the correct COM port number as reported in the Windows Device Manager. Finally make sure that the baudrate is set to 115200 baud and the parity set to even:

![UART settings](pdf:180:450){width=7}

From then on the process is the same as that described above when using a direct USB connection via the keyboard port:

- Click on "Connect".
- Select "Erase & Programming" mode.
- Browse for the firmware file.
- Tick the "Verify programming" checkbox.
- Click on "Program".

The whole operation will take about 5 minutes.

Note that after the initial firmware has been loaded you can use the UPDATE FIRMWARE command to place the ARM Cortex-M7 into download mode (equivalent to setting the BOOT CONFIG switch to "SYSTEM" or the jumper to "PGM") thereby avoiding opening the case.

When the programming/verify has completed set the BOOT CONFIG switch to "Flash" on the CPU board (or return the jumpers to the "RUN" position) and press the RESET button.

## Third Method

If you have a version using the Waveshare board it is also possible to program the firmware using the micro USB connector on this board. This method is not recommended as the Waveshare module must be removed to access this connector and doing this too many times will inevitably damage the module's pins. However, this is a handy method of testing the Waveshare board - if you can load the firmware without error it is a good indication that the module is working correctly.

To use this method remove the Waveshare CPU module from the motherboard, place shorting jumpers on all header pins except PA9-VBUS, set the power switch to "USB" and the BOOT CONFIG switch to "SYSTEM". Plug a USB cable into the micro USB connector on the top of the module and the other end into your desktop computer - both LEDs on the module should illuminate and it should connect to your computer.

Then, using the steps listed for loading the firmware via the USB keyboard port, load and verify the Colour Maximite 2 firmware using this USB cable and your desktop computer.

## Linux and the Raspberry Pi

Loading the firmware from a Linux computer and/or the Raspberry Pi has some special considerations and these are explained here: https://www.thebackshed.com/forum/ViewTopic.php?FID=16&TID=12171
