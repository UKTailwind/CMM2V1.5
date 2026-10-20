# Appendix D – SPI Communications

## SPI Communications

The Serial Peripheral Interface (SPI) communications protocol is used to send and receive data between integrated circuits. The command SPI refers to channel 1 and SPI2 refers to channel 2. SPI2 is not listed below however it is available on the Colour Maximite 2 and has an identical syntax (its function is SPI2()).

## I/O Pins

The SPI OPEN command will automatically configure the relevant I/O pins on the rear I/O connector (listed in the chapter Hardware Features). MISO stands for Master In Slave Out and because the Colour Maximite 2 is always the master that pin will be configured as an input. Similarly MOSI stands for Master Out Slave In and that pin will be configured as an output.

When the SPI CLOSE command is used these pins will be returned to a "not configured" state. They can then be configured as per normal using SETPIN.

## SPI Open

To use the SPI function the SPI channel must be first opened. The syntax for opening the SPI channel is:

```
SPI OPEN speed, mode [, bits]
```

Where:

- 'speed' is the speed of the clock. This can be 25000000, 12500000, 6250000, 3125000, 1562500, 781250, 390625 or 195315 (ie, 25MHz, 12.5MHz, 6.25MHz, 3.125MHz, 1562.5KHz, 781.25KHz, 390.625KHz or 195.3125KHz). For any other values the firmware will select the next valid speed that is equal or slower than the speed requested.
- 'mode' is a single numeric digit representing the transmission mode – see Transmission Format below.
- 'bits' is the number of bits to send/receive. This can be from 4 to 32 and is 8 if it is left out.

It is the responsibility of the program to separately manipulate the CS (chip select) pin if required.

## Transmission Format

The most significant bit is sent and received first. The format of the transmission can be specified by the 'mode' as shown below. Mode 0 is the most common format.

{widths: 10,70,10,10}
| Mode | Description | CPOL | CPHA |
|---|---|---|---|
| 0 | Clock is active high, data is captured on the rising edge and output on the falling edge | 0 | 0 |
| 1 | Clock is active high, data is captured on the falling edge and output on the rising edge | 0 | 1 |
| 2 | Clock is active low, data is captured on the falling edge and output on the rising edge | 1 | 0 |
| 3 | Clock is active low, data is captured on the rising edge and output on the falling edge | 1 | 1 |

For a more complete explanation see: http://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus

## Standard Send/Receive

When the SPI channel is open data can be sent and received using the SPI function. The syntax is:

```
received_data = SPI(data_to_send)
```

Note that a single SPI transaction will send data while simultaneously receiving data from the slave. 'data_to_send' is the data to send and the function will return the data received during the transaction. 'data_to_send' can be an integer or a floating point variable or a constant.

If you do not want to send any data (ie, you wish to receive only) any number (eg, zero) can be used for the data to send. Similarly if you do not want to use the data received it can be assigned to a variable and ignored.

## Bulk Send/Receive

Data can also be sent in bulk:

```
SPI WRITE nbr, data1, data2, data3, … etc
SPI WRITE nbr, string$
SPI WRITE nbr, array()
```

In the first method 'nbr' is the number of data items to send and the data is the expressions in the argument list (ie, 'data1', 'data2' etc). The data can be an integer or a floating point variable or a constant.

In the second or third method listed above the data to be sent is contained in the 'string$' or the contents of 'array()' (which must be a single dimension array of integer or floating point numbers). The string length, or the size of the array must be the same or greater than nbr. Any data returned from the slave is discarded.

Data can also be received in bulk:

```
SPI READ nbr, array()
```

Where 'nbr' is the number of data items to be received and array() is a single dimension integer array where the received data items will be saved. This command sends zeros while reading the data from the slave.

## SPI Close

If required the SPI channel can be closed as follows (the I/O pins will be set to inactive):

```
SPI CLOSE
```

## Examples

The following example shows how to use the SPI port for general I/O. It will send a command 80 (hex) and receive two bytes from the slave SPI device using the standard send/receive function:

```
PIN(10) = 1 : SETPIN 10, DOUT    ' pin 10 will be used as the enable signal
SPI OPEN 5000000, 3, 8           ' speed is 5 MHz and the data size is 8 bits
PIN(10) = 0                      ' assert the enable line (active low)
junk = SPI(&H80)                 ' send the command and ignore the return
byte1 = SPI(0)                   ' get the first byte from the slave
byte2 = SPI(0)                   ' get the second byte from the slave
PIN(10) = 1                      ' deselect the slave
SPI CLOSE                        ' and close the channel
```

The following is similar to the example given above but this time the transfer is made using the bulk send/receive commands:

```
OPTION BASE 1                    ' our array will start with the index 1
DIM data%(2)                     ' define the array for receiving the data
PIN(10) = 1 : SETPIN 10, DOUT    ' pin 10 will be used as the enable signal
SPI OPEN 5000000, 3, 8           ' speed is 5 MHz, 8 bits data
PIN(10) = 0                      ' assert the enable line (active low)
SPI WRITE 1, &H80                ' send the command
SPI READ 2, data%()              ' get two bytes from the slave
PIN(10) = 1                      ' deselect the slave
SPI CLOSE                        ' and close the channel
```
