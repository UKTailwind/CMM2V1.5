# Special Device Support

To make it easier for a program to interact with the external world the Colour Maximite 2 includes drivers for a number of common peripheral devices.

## Infrared Remote Control Decoder

You can easily add a remote control to the Colour Maximite 2. The solder pads for the IR receiver are on the motherboard (near the Wii connector) and it is only a matter of soldering in a suitable receiver. With this done you can use the IR command to handle the device and key codes within your program.

It will work with any NEC or Sony compatible remote controls including ones that generate extended messages. Most cheap programmable remote controls will generate either protocol and using one of these you can add a sophisticated flair to your programs. The NEC protocol is also used by many other manufacturers including Apple, Pioneer, Sanyo, Akai and Toshiba so their branded remotes can be used.

NEC remotes use a 38kHz modulation of the IR signal and suitable receivers tuned to this frequency include the Vishay TSOP4838, Jaycar ZD1952 and Altronics Z1611A.

Sony remotes use a 40 kHz modulation and receivers for this frequency can be hard to find. Generally 38 kHz receivers will work but maximum sensitivity will be achieved with a 40 kHz receiver.

To setup the decoder you use the command:

```
IR dev, key, interrupt
```

Where dev is a variable that will be updated with the device code and key is the variable to be updated with the key code. Interrupt is the interrupt subroutine to call when a new key press has been detected. The IR decoding is done in the background and the program will continue after this command without interruption. This is an example of using the IR decoder:

```
IR DevCode, KeyCode, IR_Int        ' start the IR decoder
DO
  ' < body of the program >
LOOP

SUB IR_Int                         ' a key press has been detected
  PRINT "Received  device = " DevCode "  key = " KeyCode
END SUB
```

IR remote controls can address many different devices (VCR, TV, etc) so the program would normally examine the device code first to determine if the signal was intended for the program and, if it was, then take action based on the key pressed. There are many different devices and key codes so the best method of determining what codes your remote generates is to use the above program to discover the codes.

The decoder is stopped with IR CLOSE.

## Infrared Remote Control Transmitter

Using the IR SEND command you can transmit a 12 bit Sony infrared remote control signal. This is intended for communications with another Colour Maximite 2 or a Micromite but it will also work with Sony equipment that uses 12 bit codes. Note that all Sony products require that the message be sent three times with a 26mS delay between each message.

The circuit below illustrates what is required. The transistor is used to drive the infrared LED because the output of the I/O pin is limited to about 10mA. This circuit provides about 50mA to the LED.

![IR transmitter](clip:52:365,548,570,700){width=7}

To send a signal you use the command:

```
IR SEND pin, dev, key
```

Where pin is the I/O pin used, dev is the device code to send and key is the key code. Any I/O pin can be used and you do not have to set it up beforehand (the IR SEND command will automatically do that).

Note that the modulation frequency used is 38KHz and this matches the common IR receivers (described above) for maximum sensitivity when communicating to another Maximite or a Micromite.

## Measuring Temperature

The TEMPR() function will get the temperature from a DS18B20 temperature sensor. This device can be purchased on eBay for about $5 in a variety of packages including a waterproof probe version.

There is a position on the Colour Maximite 2's mother board for a TO-92 version of this sensor and this can be mounted so that the body of the sensor pokes out of a hole in the back panel to sense the ambient temperature. If this is installed the associated pullup resistor (4.7KΩ) on the motherboard must also be installed and pin 42 used in the function and command described below.

Sensors can also be connected to any of the pins on the external I/O connector. The DS18B20 should be powered separately by a 3V to 5V supply as shown below. Multiple sensors can be used but a separate I/O pin and a 4.7K pullup resistor is required for each one.

![DS18B20](clip:53:398,52,586,146){width=6}

To get the current temperature you just use the TEMPR() function in an expression. For example:

```
PRINT "Temperature: " TEMPR(pin)
```

Where 'pin' is the I/O pin to which the sensor is connected. You do not have to configure the I/O pin, that is handled by MMBasic.

The returned value is in degrees C with a resolution of 0.25 ºC and is accurate to ±0.5 ºC. If there is an error during the measurement the returned value will be 1000.

The time required for the overall measurement is 200ms and the running program will halt for this period while the measurement is being made. This also means that interrupts will be disabled for this period. If you do not want this you can separately trigger the conversion using the TEMPR START command then later use the TEMPR() function to retrieve the temperature reading. The TEMPR() function will always wait if the sensor is still making the measurement. For example:

```
TEMPR START 15
< do other tasks >
PRINT "Temperature: " TEMPR(15)
```

## Measuring Humidity and Temperature

The HUMID command will read the humidity and temperature from a DHT22 humidity/temperature sensor. This device is also sold as the RHT03 or AM2302 but all are compatible and can be purchased on eBay for under $5. The DHT11 sensor is also supported.

The DHT22 can be powered from 3.3V or 5V (5V is recommended) and it should have a pullup resistor on the data line as shown. This is suitable for long cable runs (up to 20 meters).

![DHT22](clip:53:330,474,590,572){width=7}

To get the temperature or humidity you use the HUMID command with three or four arguments as follows:

```
HUMID pin, tVar, hVar [,DHT11]
```

Where 'pin' is the I/O pin to which the sensor is connected. You can use any I/O pin but if the DHT22 is powered from 5V it must be 5V capable (ie, NOT a pin that supports analog input). The I/O pin will be automatically configured by MMBasic.

'tVar' is a floating point variable in which the temperature is returned and 'hVar' is a second variable for the humidity. Both of these variables must be declared first as floats (using DIM). The temperature is returned as degrees C with a resolution of one decimal place (eg, 23.4) and the humidity is returned as a percentage relative humidity (eg, 54.3).

If the optional DHT11 parameter is set to 1 then the command will use device timings suitable for that device. In this case the results will be returned with a resolution of 1 degree and 1% humidity. For example:

```
DIM FLOAT temp, humidity
HUMID pin, temp, humidity
PRINT "The temperature is" temp " and the humidity is" humidity
```

HUMID was called BITBANG DHT22 (or DHT22) in earlier versions of MMBasic for the Colour Maximite 2, and both are still accepted.

## Measuring Distance

Using a HC-SR04 ultrasonic sensor (illustrated below) and the built in DISTANCE() function you can measure the distance to a target.

![HC-SR04](pdf:54:147){width=5}

This device can be found on eBay for about $4 and it will measure the distance to a target from 3cm to 3m. It works by sending an ultrasonic sound pulse and measuring the time it takes for the echo to be returned.

Compatible sensors are the SRF05, SRF06, Parallax PING and the DYP-ME007 (which is waterproof and therefore good for monitoring the level of a water tank).

You use the DISTANCE function as follows:

```
d = DISTANCE(trig, echo)
```

Where trig is the I/O pin connected to the "trig" input of the sensor and echo is the pin connected the "echo" output of the sensor. You can also use 3-pin devices and in that case only one pin number is specified.

The value returned is the distance in centimetres to the target. The I/O pins are automatically configured by this function but note that they should be 5V capable as the HC-SR04 is a 5V device.

## WS2812 Support

The Colour Maximite 2 has built in support for the WS2812 multicolour LED chip. This chip needs a very specific timing to work properly and with the WS2812 command it is easy to control these devices with minimal effort.

This command will output the required signals needed to drive a chain of WS2812 LED chips connected to the pin specified and set the colours of each LED in the chain. The syntax of the command is:

```
WS2812 type, pin, nbr, colours%()
```

'type' is a single character specifying the type of chip being driven as follows:

- O = original WS2812
- B = WS2812B
- S = SK6812
- W = SK6812W (RGBW)

'nbr' is the number of LEDs in the chain (1 to 512). The colours%() array should be an integer array with at least as many elements as there are LEDs to be driven. For the first three types each element in the array should contain the colour in the normal RGB888 format (0 to &HFFFFFF); for type W it should contain an RGBW value (0 to &HFFFFFFFF). If only one LED is connected then a single integer can be used instead of the array, with the same range.

The pin is made a digital output if it is not one already. For example:

```
DIM b%(4)=(RGB(red), RGB(green), RGB(blue), RGB(yellow), RGB(cyan))
SETPIN 5, DOUT
WS2812 O, 5, 5, b%()
```

will output the specified colours to an array of five WS2812 LEDs daisy chained off pin 5.

This command was called BITBANG WS2812 in earlier versions of MMBasic for the Colour Maximite 2, and that spelling is still accepted. Note that the number of LEDs ('nbr') is now required.

## Hobbytronic Mouse Support

The Colour Maximite 2 has built in support for the Hobbytronic USB mouse interface (https://www.hobbytronics.co.uk/usb-host-mouse). This is enabled using the CONTROLLER MOUSE OPEN command and then the mouse position and buttons can be interrogated using the MOUSE function.

## CAN Support

MMBasic on the Colour Maximite 2 exposes the native support for CAN provided by the STM32H743 chip as the MMBasic CAN command. A CAN transceiver such as the SN65HVD230 CAN Transceiver is required to interface to the actual physical CAN bus. It connects to 3.3V and GND plus the nominated CAN Tx and CAN Rx pins on the Colour Maximite 2. The CANL and CANH connectors go to the physical CAN bus. You can exercise the CAN command in loopback mode without having the transceiver attached.

![CAN transceiver](pdf:55:150){width=5}

See Appendix J (CAN Support) for the details.
