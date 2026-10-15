' bounds.bas - arguments read or written past their end, and LOCALs kept past their SUB: the fixes from the review
' of the array parsing (LGETSTR$, LINSTR, LONGSTRING COPY, BLIT LOADARRAY, WS2812, I2C WRITE, SPI WRITE and READ,
' DAC START, IR).
Option Explicit
Dim integer tests, pass, n%
Dim float nf
Dim s$
Dim integer a%(10), b%(10), c%(3), w%(7)
Dim f!(8)

' ---- long strings -------------------------------------------------------------------------------------------
LongString Clear a%()
s$ = LGetStr$(a%(), 5, 3)
CheckS "LGETSTR$ from past the end is empty (it wrote about 4 GB)", s$, ""
LongString Append a%(), "hello world"
a%(0) = 5                                   ' the long string is now "hello"; " world" is still in the array
Check "LINSTR's regex stops at the long string's length (w.r matched past it)", LInStr(a%(), "w.r", 1, nf), 0
Check "LINSTR's regex inside it", LInStr(a%(), "l+", 1, n%), 3
Check "and the match's length into an integer variable (it needed a float)", n%, 2
LongString Append b%(), String$(30, "x")
On Error Skip 1
LongString Copy c%(), b%()
CheckS "LONGSTRING COPY of 30 bytes into 24 bytes of room (it allowed 32)", ErrText$(), "Destination array too small"

' ---- an array from an element -------------------------------------------------------------------------------
On Error Skip 1
Blit LoadArray #1, 2, 2, f!(5)
CheckS "BLIT LOADARRAY takes its array whole (from f!(5) it read past the end)", ErrText$(), "Invalid syntax"
On Error Skip 1
Bitbang WS2812 O, 1, 4, w%(6)
CheckS "so does WS2812", ErrText$(), "Invalid syntax"
On Error Skip 1
Bitbang WS2812 O, 1, 9, w%()
CheckS "and it keeps its size check", ErrText$(), "Array size"

' ---- I2C and SPI ----------------------------------------------------------------------------------------------
I2C Open 100, 1000
s$ = "AB"
On Error Skip 1
I2C Write &H50, 0, 4, s$
CheckS "I2C WRITE of a string shorter than the count (it sent what followed it)", ErrText$(), "Insufficient data"
I2C Close
SPI Open 1000000, 0, 8
On Error Skip 1
SPI Write 4, w%(6)
CheckS "SPI WRITE counts from the element given (w%(6) of eight read past the end)", ErrText$(), "Insufficient data"
On Error Skip 1
SPI Read 0, w%()
CheckS "SPI READ of no bytes", ErrText$(), "Number out of bounds"
SPI Close

' ---- a LOCAL kept past its SUB --------------------------------------------------------------------------------
LocalUsers

Print "bounds:"; pass; " of"; tests; " passed"
End

Sub LocalUsers
  Local integer d%(9), dev, cmd
  On Error Skip 1
  DAC Start 1000, d%()
  CheckS "DAC START with a LOCAL array (the DAC interrupt read it after the SUB)", ErrText$(), "Must be a global variable"
  On Error Skip 1
  IR dev, cmd, IrInt
  CheckS "IR with LOCAL variables (the timer interrupt wrote through them)", ErrText$(), "Must be a global variable"
End Sub
Sub IrInt
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
