# Appendix H – Cyclic Redundancy Check (CRC)

## Cyclic Redundancy Check (CRC)

The purpose of this description is not to explain or examine the mathematics behind CRCs but merely to explain the benefits of using them and how they may be used in MMBasic.

A cyclic redundancy check (CRC) is a strong algebraic error-detecting code commonly used in digital networks and storage devices to detect accidental changes to the data. Blocks of data have a short check value attached, this is the CRC. A CRC is based on the remainder of a polynomial division of their contents. This technique was invented by W. Wesley Peterson in 1961 and further developed by the CCITT.

Note: A CRC is not an error correction code it is just for error detection.

The advantages of using a CRC when sending or saving data are that it is a fast and efficient method for detecting errors in data transmission and can detect errors that occur during transmission, and storage caused by things such as noise, interference or distortion.

There are simpler methods of error detection including the use of ODD/EVEN parity for ASCII transmission and the use of a checksum. A checksum is simply an addition of all of the bits transmitted in a block of data, usually the carry bit is ignored and the resultant value is truncated to 8 or 16 bits which is appended to the end of the block of data. Neither of these methods are particularly secure.

The more bits in the CRC, the more errors it will detect so a 16 bit CRC will be more secure than an 8 bit CRC and so on. While a CRC will not catch all errors, it is much more secure than a simple checksum.

## Using a CRC

Suppose we want to send a string via some medium. It could be data from your weather station which is located up a pole which is sent via radio to your base station for example. A simple example using the MODBUS CRC:

```
' A simple demonstration of using a CRC
' the data to send
a$="123456789"
' calculate the CRC
b% = MATH(CRC16 a$,,&h8005,&hffff,0,1,1)
' convert the CRC to a string
acrc$ = CHR$(b% AND &HFF) + CHR$((b%>>8) AND &HFF)
' add the CRC to the data to send
txd$ = a$ + acrc$
' then transmit the data
' here the data is printed for demonstration
PRINT "The data to be transmitted"
PrintStr(txd$)
' check the received CRC and data
c% = MATH(CRC16 txd$,,&h8005,&hffff,0,1,1)
check$ = CHR$(c% AND &HFF) + CHR$((c%>>8) AND &HFF)
PRINT
PRINT "The CRC of the received data including the"
PRINT "received CRC - the result should be zero"
PrintStr(check$)

' Print a string as HEX numbers (for debug)
SUB PrintStr(b$)
  FOR i = 1 TO LEN(b$)
    PRINT HEX$(ASC(MID$(b$,i,1)),2); ", ";
  NEXT i
  PRINT
END SUB
```

The CRC value is transmitted along with the data to the receiver. The receiver can verify the received data by removing the CRC then recalculate the CRC and compare that with the received CRC. Or, more simply, recalculate the CRC for the whole received message and verify that the result is zero as demonstrated above. If the CRC does not check then the receiver should reply with a negative acknowledgement and request a re-transmission of the data.

## The MMBasic CRC Function

```
MATH(CRCn data [,length] [,polynome] [,startmask] [,endmask] [,reverseIn] [,reverseOut])
```

'n' is 8, 12, 16 or 32. Please see the entry for MATH() in the Functions table. Here are some examples for commonly used CRC calculations from Volhout's demonstration program that may be useful:

```
' CCITT CRC16
CRC% = MATH(CRC16 data$,,,&hffff)
' MODBUS CRC16
CRC% = MATH(CRC16 data$,,&h8005,&hffff,0,1,1)
' XMODEM CRC16
CRC% = MATH(CRC16 data$)
' MAXIM CRC8
CRC% = MATH(CRC8 data$,,&h31,,,1,1)
' standard CRC32
CRC% = MATH(CRC32 data$,,,&hffffffff,&hffffffff,1,1)
```

## Demonstration Program

```
' MATH CRC Demonstration program
' Based on the program "MATH CRC evaluation" written by Volhout
OPTION BASE 1
'test string
a$="123456789"
l%=LEN(a$)
PRINT "Test string "; a$
PRINT
'convert test string to array
DIM b%(l%)
FOR i=1 TO l% : b%(i)=ASC(MID$(a$,i,1)) : NEXT i
'perform CRC validation
DIM CRC%
'check CCITT CRC16
CRC% = MATH(CRC16 b%(),l%,,&hffff)
PRINT "CRC16-CCITT  "; HEX$(CRC%)
'check MODBUS CRC16
CRC% = MATH(CRC16 b%(),l%,&h8005,&hffff,0,1,1)
PRINT "CRC16-MODBUS "; HEX$(CRC%)
'check XMODEM CRC16
CRC% = MATH(CRC16 b%(),l%)
PRINT "CRC16-XMODEM "; HEX$(CRC%)
'check MAXIM CRC8 (used in DALLAS single wire devices)
CRC% = MATH(CRC8 b%(),l%,&h31,,,1,1)
PRINT "CRC8-MAXIM   "; HEX$(CRC%)
'check standard CRC32
CRC% = MATH(CRC32 b%(),l%,,&hffffffff,&hffffffff,1,1)
PRINT "CRC32        "; HEX$(CRC%)
```

On the Colour Maximite 2 this prints:

```
CRC16-CCITT  29B1
CRC16-MODBUS 4B37
CRC16-XMODEM 31C3
CRC8-MAXIM   A1
CRC32        CBF43926
```

Some useful links:

- The author of this CRC code: https://github.com/RobTillaart/CRC
- Explanations of CRCs: http://www.sunshine2k.de/articles/coding/crc/understanding_crc.html#ch1 and https://en.wikipedia.org/wiki/Cyclic_redundancy_check
- On line CRC calculators: http://zorc.breitbandkatze.de/crc.html, https://crccalc.com and https://www.lddgo.net/en/encrypt/crc
