' realcmd.bas - ERASE, I2C2, I2C3, SPI2, HUMID, DHT22, WS2812 and BITSTREAM are commands of their own (M3 of the b14
' merge): until then the tokeniser rewrote them into CLEAR VARS, I2C 2, BITBANG DHT22 and so on, and LIST showed the
' rewritten text.  Each runs here; the ones that need hardware are given arguments that fail in a known way.
' LIST FLASH of this program must show the commands as typed.
Option Explicit
Dim integer a = 1, b = 2, c = 3, n
Dim arr(3)
Dim string e$
Erase a
On Error Skip 1
n = a
e$ = MM.ErrMsg$ : Print "erase: "; e$
Erase b, c
On Error Skip 1
n = c
e$ = MM.ErrMsg$ : Print "erase two: "; e$
Dim integer a = 10
Print "made again"; a
On Error Skip 1
I2C Close
e$ = MM.ErrMsg$ : Print "i2c: "; e$
On Error Skip 1
I2C2 Close
e$ = MM.ErrMsg$ : Print "i2c2: "; e$
On Error Skip 1
I2C3 Close
e$ = MM.ErrMsg$ : Print "i2c3: "; e$
On Error Skip 1
SPI2 Close
e$ = MM.ErrMsg$ : Print "spi2: "; e$
On Error Skip 1
SPI Close
e$ = MM.ErrMsg$ : Print "spi: "; e$
On Error Skip 1
Humid 0, arr(0), arr(1)
e$ = MM.ErrMsg$ : Print "humid: "; e$
On Error Skip 1
DHT22 0, arr(0), arr(1)
e$ = MM.ErrMsg$ : Print "dht22: "; e$
On Error Skip 1
WS2812 O, 0, 1, arr()
e$ = MM.ErrMsg$ : Print "ws2812: "; e$
On Error Skip 1
Bitstream 0, 2, arr()
e$ = MM.ErrMsg$ : Print "bitstream: "; e$
On Error Skip 1
Bitbang Bitstream 0, 2, arr()
e$ = MM.ErrMsg$ : Print "bitbang bitstream: "; e$
Print "done"
