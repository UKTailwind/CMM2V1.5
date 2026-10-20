# Appendix B – I2C Communications

## I2C Communications

The Colour Maximite 2 implements three I2C channels, two on the rear I/O connector and the third dedicated to the front panel Wii connector. All operate in master mode (slave mode is not available).

The I2C command refers to channel 1 while the commands I2C2 and I2C3 refer to channels 2 and 3 using the same syntax. There are five commands that can be used:

{widths: 30,70}
| Command | Description |
|---|---|
| I2C OPEN speed, timeout | Enables the I2C module in master mode. 'speed' is the clock speed (in KHz) to use and must be one of 100, 400 or 1000. 'timeout' is a value in milliseconds after which the master send and receive commands will be interrupted if they have not completed. The minimum value is 100. A value of zero will disable the timeout (though this is not recommended). |
| I2C WRITE addr, option, sendlen, senddata [,senddata ....] | Send data to the I2C slave device. 'addr' is the slave's I2C address. 'option' can be 0 or 1 (see Keeping the Bus below). 'sendlen' is the number of bytes to send (1 to 256). 'senddata' is the data to be sent (all values sent will be between 0 and 255) in any one of three forms: a list of constants, variables or expressions, one for each byte, where array elements such as a(3) are allowed (example: I2C WRITE &H6F, 0, 3, &H23, &H43, &H25); a one dimensional array specified with empty brackets, when 'sendlen' elements of the array will be sent starting with the first (example: I2C WRITE &H6F, 0, 3, ARRAY()); or a string variable (not a constant) at least 'sendlen' characters long, when its first 'sendlen' characters are sent (example: I2C WRITE &H6F, 0, 3, STRING$). |
| I2C READ addr, option, rcvlen, rcvbuf [,rcvbuf ....] | Get data from the I2C slave device. 'addr' is the slave's I2C address. 'option' can be 0 or 1 (see Keeping the Bus below). 'rcvlen' is the number of bytes to receive. 'rcvbuf' is where the received data goes, in any one of three forms: a list of numeric variables, one for each byte, where array elements such as a(3) are allowed (a single variable is a list of one, and 'rcvlen' must then be 1); a one dimensional array of numbers specified with empty brackets, when received bytes will be stored in sequential elements of the array starting with the first (example: I2C READ &H6F, 0, 3, ARRAY()); or a string variable, when bytes will be stored as sequential characters and the string set to 'rcvlen' characters ('rcvlen' can then be at most 255, and no more than a LENGTH given to the string). |
| I2C CHECK addr | Checks whether a device answers at the I2C address 'addr'. MM.I2C is set to 0 if a device answers and to a non zero value if none does. The channel must be open (by I2C OPEN or by a Wii controller or mouse using it). A bus kept by option 1 is released first. |
| I2C CLOSE | Disables the master I2C module and returns the I/O pins to a "not configured" state. They can then be configured using SETPIN. A bus kept by option 1 is released first. |

Following an I2C write or read command the automatic variable MM.I2C will be set to indicate the result of the operation as follows:

- 0 = The command completed without error.
- 1 = Received a NACK response
- 2 = Command timed out

The Wii controllers (CONTROLLER NUNCHUK, CONTROLLER CLASSIC) and an I2C mouse (CONTROLLER MOUSE) use the same channels in the background. A program's I2C commands on a channel that one of them is using wait for the controller's transfer under way to finish, so the two do not collide.

## Keeping the Bus

An 'option' of 0 ends each I2C WRITE and READ with a stop condition, which frees the bus. An 'option' of 1 keeps control of the bus: no stop condition is sent, and the next I2C WRITE or READ on the channel begins with a repeated start instead of a start. Many devices need this to read a register: its number is written with option 1, then the register is read with option 0, which ends with the stop. For example, to read the seconds register (0) of a DS3231 real time clock on channel 2:

```
I2C2 OPEN 100, 1000
I2C2 WRITE &H68, 1, 1, 0       ' the register number, keeping the bus
I2C2 READ &H68, 0, 1, secs     ' a repeated start, the read, and the stop
I2C2 CLOSE
PRINT HEX$(secs)
```

A bus that is kept is released by the next WRITE or READ with option 0, by I2C CHECK or I2C CLOSE, and at the end of the program. A Wii controller or mouse on the channel is not read while a program keeps its bus.

## 7-Bit Addressing

The standard addresses used in these commands are 7-bit addresses (without the read/write bit). MMBasic will add the read/write bit and manipulate it accordingly during transfers.

Some vendors provide 8-bit addresses which include the read/write bit. You can determine if this is the case because they will provide one address for writing to the slave device and another for reading from the slave. In these situations you should only use the top seven bits of the address. For example: If the read address is 9B (hex) and the write address is 9A (hex) then using only the top seven bits will give you an address of 4D (hex).

Another indicator that a vendor is using 8-bit addresses instead of 7-bit addresses is to check the address range. All 7-bit addresses should be in the range of 08 to 77 (hex). If your slave address is greater than this range then probably your vendor has specified an 8-bit address.

## I/O Pins

Refer to the rear panel I/O connector diagram in the chapter Hardware Features for the pin numbers used for the I2C channels 1 and 2. Their signals are marked as data line (SDA) and clock (SCL). I2C channel 3 is routed to the Wii connector on the front panel. When the I2C CLOSE command is used the I/O pins are reset to a "not configured" state. They can then be configured as per normal using SETPIN.

Both the data line (SDA) and clock (SCL) for all three I2C ports have 10K pullup resistors installed on the motherboard so external pullup resistors are not required. These should be considered if these pins are to be used as general purpose I/O pins.

When running the I2C bus at above 100 kHz the cabling between the devices becomes important. Ideally the cables should be as short as possible (to reduce capacitance) and also the data and clock lines should not run next to each other but have a ground wire between them (to reduce crosstalk).

If the data line is not stable when the clock is high, or the clock line is jittery, the I2C peripherals can get "confused" and end up locking the bus (normally by holding the clock line low). If you do not need the higher speeds then operating at 100 kHz is the safest choice.

## Example

As an example, the following program will read and display the current time (hours and minutes) maintained by a PCF8563 real time clock chip connected to I2C channel 2:

```
DIM AS INTEGER RData(2)        ' this will hold received data
I2C2 OPEN 100, 1000            ' open the I2C channel
I2C2 WRITE &H51, 0, 1, 3       ' set the first register to 3
I2C2 READ &H51, 0, 2, RData()  ' read two registers
I2C2 CLOSE                     ' close the I2C channel
PRINT "Time is " RData(1) ":" RData(0)
```
