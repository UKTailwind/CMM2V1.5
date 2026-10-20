# Appendix C – 1-Wire Communications

## 1-Wire Communications

The 1-Wire protocol was developed by Dallas Semiconductor to communicate with chips using a single signalling line. This implementation was written for MMBasic by Gerard Sexton.

There are three commands that you can use:

{widths: 45,55}
| Command | Description |
|---|---|
| ONEWIRE RESET pin | Reset the 1-Wire bus |
| ONEWIRE WRITE pin, flag, length, data [, data…] | Send a number of bytes |
| ONEWIRE READ pin, flag, length, data [, data…] | Get a number of bytes |

Where:

- pin - The I/O pin (located in the rear connector) to use. It can be any pin capable of digital I/O.
- flag - A combination of the following options: 1 - Send reset before command; 2 - Send reset after command; 4 - Only send/recv a bit instead of a byte of data; 8 - Invoke a strong pullup after the command (the pin will be set high and open drain disabled)
- length - Length of data to send or receive
- data - The data to send (WRITE) or where the received data goes (READ), in any one of three forms: a list of items, one for each byte (or bit), whose number must agree with the length parameter (for WRITE each item is a constant, variable or expression; for READ each is a numeric variable; array elements such as a(3) are allowed); an array, floating point or integer, specified with empty brackets (eg, array()), used from its first element; or a string variable (for WRITE its first 'length' characters are sent; for READ the bytes received are stored as its characters and its length is set to 'length').

And the automatic variable:

- MM.ONEWIRE - Returns true if a device was found

After the command is executed, the I/O pin will be set to the not configured state unless flag option 8 is used.

When a reset is requested the automatic variable MM.ONEWIRE will return true if a device was found. This will occur with the ONEWIRE RESET command and the ONEWIRE READ and ONEWIRE WRITE commands if a reset was requested (flag = 1 or 2).

The 1-Wire protocol is often used in communicating with the DS18B20 temperature measuring sensor and to help in that regard MMBasic includes the TEMPR() function which provides a convenient method of directly reading the temperature of a DS18B20 without using these functions.
