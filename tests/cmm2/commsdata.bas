' commsdata.bas - the data of SPI, I2C and ONEWIRE's READ and WRITE as the PicoMite takes it (a list, a whole array
' or a string: MMBasic.c's GetCommsTxData and friends), I2C option 1 and MM.I2C, SETTICK FAST, the turtle's colour
' spellings, WS2812 type W with one LED and a long chain, and CAN OPEN's speeds (20 Oct 2026). Nothing need be wired:
' I2C goes to address &H33, where no device answers, and SPI's data and ONEWIRE's pin 7 are not looked at.
Option Explicit
Dim integer tests, pass, n%, i, ok, ticks, t0, fast
Dim float x, y, z, v
Dim s$, r$
Dim t$ Length 4
Dim integer a%(7), b%(7), c%(3), big%(399)
Dim f!(7)
Const K = 1
For i = 0 To 7 : a%(i) = i + 1 : f!(i) = i + 0.6 : Next
s$ = "ABCDEFGH"

' ---- SPI ----------------------------------------------------------------------------------------------------------
SPI Open 1000000, 0, 8
On Error Skip 3
SPI Write 3, 1, 2, 3
SPI Write 3, a%()
SPI Write 3, f!()
CheckS "SPI WRITE: a list, an integer array, a float array", ErrText$(), ""
On Error Skip 1
SPI Write 3, s$
CheckS "SPI WRITE: a string", ErrText$(), ""
On Error Skip 1
SPI Write 4, a%(6)
CheckS "SPI WRITE: an element is one item (as the PicoMite)", ErrText$(), "Argument count"
SPI Read 3, x, y, z
Check "SPI READ: a list of variables", (x >= 0 And x <= 255 And y >= 0 And y <= 255 And z >= 0 And z <= 255), 1
SPI Read 3, r$
Check "SPI READ: a string, its length set", Len(r$), 3
SPI Read 3, b%()
SPI Read 3, f!()
Check "SPI READ: a whole array of either type", (b%(2) <= 255 And f!(2) <= 255), 1
SPI Read 2, b%(5), f!(6)
SPI Read 1, v
Check "SPI READ: array elements in a list, and one variable", (b%(5) <= 255 And f!(6) <= 255 And v <= 255), 1
On Error Skip 1
SPI Read 3
CheckS "SPI READ with nothing to read into", ErrText$(), "Invalid syntax"
On Error Skip 1
SPI Read 3, v
CheckS "SPI READ of three into one variable", ErrText$(), "Invalid variable"
On Error Skip 1
SPI Read 300, r$
CheckS "SPI READ of more than a string holds", ErrText$(), "Number out of bounds"
On Error Skip 1
SPI Read 5, t$
CheckS "SPI READ past a string's LENGTH", ErrText$(), "Number out of bounds"
On Error Skip 1
SPI Read 20, c%()
CheckS "SPI READ past an array's end", ErrText$(), "Insufficient space in array"
On Error Skip 1
SPI Read 1, K
CheckS "SPI READ into a constant", ErrText$(), "Cannot change a constant"
SPI Close

' ---- I2C ----------------------------------------------------------------------------------------------------------
I2C Open 100, 100
t0 = Timer
I2C Write &H33, 0, 3, 1, 2, 3
Check "I2C WRITE to no device: MM.I2C 1 (a NACK)", MM.I2C, 1
I2C Write &H33, 1, 3, a%()
Check "and with option 1 (keep the bus): the NACK frees it", MM.I2C, 1
I2C Check &H33
Check "I2C CHECK after it finds no device", (MM.I2C <> 0), 1
I2C Read &H33, 0, 3, r$
Check "I2C READ into a string: MM.I2C 1", MM.I2C, 1
Check "the string's length set", Len(r$), 3
I2C Read &H33, 1, 2, x, y
Check "I2C READ into a list, option 1", MM.I2C, 1
I2C Read &H33, 0, 4, f!()
I2C Write &H33, 2, 1, 5
Check "I2C WRITE takes option 2 (the PicoMite's bit 1)", MM.I2C, 1
Check "eight NACKs take no time (no timeouts)", (Timer - t0 < 100), 1
On Error Skip 1
I2C Write &H33, 4, 1, 5
CheckS "I2C WRITE option 4", ErrText$(), "4 is invalid (valid is 0 to 3)"
On Error Skip 1
I2C Write &H33, 0, 2, 1
CheckS "I2C WRITE of two with one item", ErrText$(), "Argument count"
On Error Skip 1
I2C Write &H33, 0, 9, s$
CheckS "I2C WRITE of more than a string has", ErrText$(), "Insufficient data"
On Error Skip 1
I2C Read &H33, 0, 3, x
CheckS "I2C READ of three into one variable", ErrText$(), "Invalid variable"
On Error Skip 1
I2C Read &H33, 0, 5, t$
CheckS "I2C READ past a string's LENGTH", ErrText$(), "Number out of bounds"
On Error Skip 1
I2C Read &H33, 0, 2, b%(), x
CheckS "I2C READ: a whole array is not a list item", ErrText$(), "Invalid variable"
I2C Close

' ---- ONEWIRE (pin 7) -----------------------------------------------------------------------------------------------
On Error Skip 4
OneWire Write 7, 0, 3, 1, 2, 3
OneWire Write 7, 0, 3, a%()
OneWire Write 7, 0, 3, f!()
OneWire Write 7, 0, 3, s$
CheckS "ONEWIRE WRITE: a list, both arrays, a string (it took a list only)", ErrText$(), ""
OneWire Read 7, 4, 3, x, y, z
Check "ONEWIRE READ: a list of bits", (x <= 1 And y <= 1 And z <= 1), 1
OneWire Read 7, 0, 3, r$
Check "ONEWIRE READ: a string", Len(r$), 3
OneWire Read 7, 0, 3, b%()
OneWire Read 7, 0, 2, b%(4), f!(5)
Check "ONEWIRE READ: an array, and elements in a list (it refused elements)", (b%(2) <= 255 And b%(4) <= 255 And f!(5) <= 255), 1
s$ = "ab"
On Error Skip 1
OneWire Write 7, 0, 4, s$
CheckS "ONEWIRE WRITE of more than a string has", ErrText$(), "Insufficient data"

' ---- WS2812 (pin 7) -----------------------------------------------------------------------------------------------
On Error Skip 1
WS2812 W, 7, 1, &HFFFFFFFF
CheckS "WS2812 type W, one LED: all 32 bits (it took 24)", ErrText$(), ""
On Error Skip 1
WS2812 O, 7, 1, &H1000000
CheckS "WS2812 type O, one LED: 24 bits", ErrText$(), "16777216 is invalid (valid is 0 to 16777215)"
For i = 0 To 399 : big%(i) = i * 4097 And &HFFFFFF : Next
WS2812 S, 7, 400, big%()
WS2812 W, 7, 400, big%()
Check "WS2812, 400 LEDs (3 and 4 bytes an LED: it allocated one)", 1, 1
SetPin 7, Off

' ---- SETTICK FAST -------------------------------------------------------------------------------------------------
SetTick Fast 10000, FastInt
t0 = Timer
Do While Timer - t0 < 500 : Loop
SetTick Fast 0, 0
n% = ticks
Check "SETTICK FAST 10000 for 0.5 s: about 5000 ticks (it was unreachable)", (n% > 4900 And n% < 5100), 1
Pause 50
Check "SETTICK FAST 0, 0 stops it", ticks, n%
fast = 10
On Error Skip 1
SetTick fast, SlowInt, 2
CheckS "SETTICK with a variable called FAST is the period", ErrText$(), ""
SetTick 0, 0, 2

' ---- TURTLE -------------------------------------------------------------------------------------------------------
On Error Skip 7
Turtle Reset
Turtle Pen Color RGB(red)
Turtle PenColour RGB(green)
Turtle Fill Color RGB(blue)
Turtle FillColour RGB(white)
Turtle Cursor Color RGB(red)
Turtle CursorColour RGB(red)
CheckS "TURTLE PEN COLOR, PENCOLOUR, FILL COLOR, FILLCOLOUR, CURSOR COLOR, CURSORCOLOUR", ErrText$(), ""
CLS

' ---- CAN ----------------------------------------------------------------------------------------------------------
On Error Skip 1
CAN Open 1, 123, 0
CheckS "CAN OPEN's speeds", ErrText$(), "Valid speeds are 0,125000,250000,500000,1000000"

Print "commsdata:"; pass; " of"; tests; " passed"
End

Sub FastInt
  ticks = ticks + 1
End Sub
Sub SlowInt
End Sub

' the error message without "Error in line n: "
Function ErrText$()
  Local integer c
  c = Instr(MM.ErrMsg$, ": ")
  If c Then ErrText$ = Mid$(MM.ErrMsg$, c + 2) Else ErrText$ = MM.ErrMsg$
End Function

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub CheckS(n$, got$, want$)
  tests = tests + 1
  If got$ = want$ Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got "; got$; " want "; want$
  EndIf
End Sub
