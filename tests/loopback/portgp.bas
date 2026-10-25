' portgp.bas - PORT and PORT() through the 64-bit GPIO masks (gpio_put_masked64 and gpio_get_all64, External.c;
' 25 Oct 2026). Needs GP0, GP1 and GP3-GP7 wired to GP8, GP9 and GP11-GP15 (pin 8 to 15, 10 to 24, 22 to 38, 36 to 35,
' 31 to 18, 7 to 27 and 13 to 28). GP10 is pin 40, the factory reset
' pin (held low at power-up it erases the options and the program), so GP2 and GP10 are not wired or used.
Option Explicit
Dim integer tests, pass, i, v, bad, a, b
Dim integer o(6) = (0, 1, 3, 4, 5, 6, 7)
Dim float t

For i = 0 To 6
  SetPin "GP" + Str$(o(i)), DOUT
  SetPin "GP" + Str$(o(i) + 8), DIN
Next
Port(GP0, 2, GP3, 5) = 0 : a = Port(GP8, 2, GP11, 5)
Port(GP0, 2, GP3, 5) = 127 : b = Port(GP8, 2, GP11, 5)
If a <> 0 Or b <> 127 Then
  Print "GP0, GP1, GP3-GP7 are not wired to GP8, GP9, GP11-GP15: read"; a; " and"; b; " for 0 and 127"
  End
EndIf

bad = 0
For v = 0 To 127
  Port(GP0, 2, GP3, 5) = v
  If Port(GP8, 2, GP11, 5) <> v Then bad = bad + 1
Next
Check "PORT(GP0, 2, GP3, 5) = v read back as PORT(GP8, 2, GP11, 5), v = 0 to 127: the values that differ", bad, 0
bad = 0
For v = 0 To 127
  Port(GP0, 2, GP3, 5) = v
  If Port(GP0, 2, GP3, 5) <> v Then bad = bad + 1
Next
Check "and as PORT(GP0, 2, GP3, 5), the outputs themselves", bad, 0
bad = 0
For i = 0 To 6
  Port(GP0, 2, GP3, 5) = 1 << i
  For v = 0 To 6
    If Pin("GP" + Str$(o(v) + 8)) <> (v = i) Then bad = bad + 1
  Next
Next
Check "one bit at a time, each input by PIN(): the pins that differ", bad, 0
bad = 0
For v = 0 To 31
  Port(GP3, 5) = v
  If Port(GP11, 5) <> v Then bad = bad + 1
Next
Check "one group of five consecutive GPs: PORT(GP3, 5) and PORT(GP11, 5)", bad, 0
Port(GP0, 2, GP3, 5) = 0
Port(7, 2) = &B10                                                   ' pins 7 and 8: GP6 and GP0
Check "pin numbers: PORT(7, 2) = &B10 sets pin 8 (GP0) and clears pin 7 (GP6)", Port(GP8, 2, GP11, 5), 1
Port(GP0, 1, GP0, 1) = &B10
Check "a pin given twice takes the last value", Pin(GP8), 1
On Error Skip 1
Port(GP8, 1) = 1
CheckS "an input", MM.ErrMsg$, "Invalid output pin"
On Error Skip 1
Port(GP23, 1) = 1
CheckS "a GP with no pin", MM.ErrMsg$, "GP23 is not available on the Colour Maximite 2"
t = Timer
For i = 1 To 10000 : Port(GP0, 2, GP3, 5) = i : Next
t = Timer - t
Check "10000 PORT writes in under a second", t < 1000, 1

For i = 0 To 6 : SetPin "GP" + Str$(o(i)), OFF : SetPin "GP" + Str$(o(i) + 8), OFF : Next
Print "portgp:"; pass; " of"; tests; " passed"
End

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
