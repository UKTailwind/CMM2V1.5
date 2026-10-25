' gpnames.bas - the PicoMite's GP pin names without OPTION PICOMITE (25 Oct 2026: in every mode, not only under the
' option). GP16 and GP17 are pins 3 and 5 (I2C), which nothing else uses here.
Option Explicit
Option Default Integer
Dim tests, pass

Check "MM.DEVICE$ is the CMM2's (no OPTION PICOMITE)", Instr(MM.Device$, "Colour Maximite 2") = 1, 1
Check "MM.INFO(PINNO GP16)", MM.Info(PINNO GP16), 3
Check "MM.INFO(PINNO with GP2 as a string)", MM.Info(PINNO "GP2"), 12
Check "MM.INFO(MAX GP): 37, the highest GP with a pin here", MM.Info(MAX GP), 37
SetPin GP16, DOut
Pin(GP16) = 1
Check "SETPIN GP16, DOUT and PIN(GP16) = 1: pin 3 high", Pin(3), 1
Pin("GP16") = 0
Check "PIN() = 0 with GP16 as a string: low", Pin(GP16), 0
SetPin "GP" + Str$(17), DOut
Port(GP16, 2) = &B10
Check "PORT(GP16, 2) = &B10: GP17 (pin 5) high, GP16 low", Pin(5) * 10 + Pin(3), 10
Check "MM.INFO$(PIN GP16)", MM.Info$(PIN GP16) = "In Use", 1
SetPin GP16, Off : SetPin GP17, Off
On Error Skip 1
SetPin GP23, DIn
CheckS "a GP with no pin", MM.ErrMsg$, "GP23 is not available on the Colour Maximite 2"
On Error Skip 1
SetPin GP48, DIn
CheckS "past GP47", MM.ErrMsg$, "Invalid GPIO"

Print "gpnames:"; pass; " of"; tests; " passed"
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
