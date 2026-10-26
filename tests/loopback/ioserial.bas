' ioserial.bas - the I/O items of 26 Oct 2026 (docs/PicoMite_Parity_ToDo.md) on the loop-back wiring (README.md):
' SETPIN ARAW (pin 8, GP0, an output, to pin 15, GP8, analog); BITSTREAM's mode and two-pin forms on pins 22 and 36
' (read back on 38 and 35): each stream's length by TIMER (right under 50ms with interrupts off) and the pins' last levels; DEVICE SERIALTX on
' pin 24 (GP9) read by COM1 on pin 10; DEVICE SERIALRX on pin 38 from PWM 1B on pin 22.
' Run it in a CMM2 mode (no OPTION PICOMITE): in the PicoMite's MODE 1 the display's merge, held while BITSTREAM has
' interrupts off, is done when they are on again, before TIMER is read, which adds about 3ms to a stream's time.
Option Explicit
Option Default Integer
Dim tests, pass, hi, lo, mx, n, st, i, us, tries
Dim float v
Dim r$ Length 64, r3$ Length 3, e$
Dim integer a(9), b(9), c(1), d(9)

mx = 65535 : If Instr(MM.Device$, "PicoMite") Then mx = 4095

' ---- SETPIN ARAW
SetPin 8, DOut
SetPin 15, ARaw
Pin(8) = 1 : Pause 2 : hi = Pin(15)
Pin(8) = 0 : Pause 2 : lo = Pin(15)
Check "ARAW: the pin high gives near the top count (" + Str$(mx) + ")", hi > mx * 0.97 And hi <= mx, 1
Check "  low, near 0", lo < mx * 0.03, 1
Pin(8) = 1 : Pause 2
Check "  an integer, not volts", Instr(Str$(Pin(15)), "."), 0
SetPin 15, Off
SetPin 15, AIn : Pause 2 : v = Pin(15)
Check "AIN still gives volts (3.0 to 3.4)", v > 3 And v < 3.4, 1
SetPin 15, Off
On Error Skip 1 : SetPin 15, ARaw, 12
CheckS "ARAW takes no bit count", MM.ErrMsg$, "Syntax"
SetPin 15, Off : SetPin 8, Off

' ---- BITSTREAM
For i = 0 To 9 : a(i) = 1000 : b(i) = 2000 : Next
SetPin 38, DIn : SetPin 35, DIn
SetPin 22, DOut : Pin(22) = 0
us = Stream("BITSTREAM 22, 10, a()")
Check "one pin, 10 changes of 1000us: 10ms (+-2%)", Abs(us - 10000) < 200, 1
Check "  an even count leaves it low", Pin(38), 0
us = Stream("BITSTREAM 22, 9, a(), 0")
Check "  9 with mode 0 given: high", Pin(38), 1
SetPin 22, Off
us = Stream("BITSTREAM 22, 10, a(), 1")
Check "mode 1 (open-collector): 10 waits of 1000us (+-2%)", Abs(us - 10000) < 200, 1
Check "  released and pulled up at the end: high", Pin(38), 1
On Error Skip 1 : BITSTREAM 22, 9, a(), 1
CheckS "  an odd count", MM.ErrMsg$, "Open-collector mode requires even number of transitions"
SetPin 22, Off
SetPin 22, DOut : Pin(22) = 0 : SetPin 36, DOut : Pin(36) = 0
us = Stream("BITSTREAM 22, 10, a(), 0, 36, 7, b()")
Check "two pins: 10 x 1000us and 7 x 2000us take 14ms (+-2%)", Abs(us - 14000) < 280, 1
Check "  pin 22 changed 10 times (low), pin 36 7 (high)", Pin(38) * 10 + Pin(35), 1
c(0) = 1000 : c(1) = 1000 : d(0) = 3000
For i = 0 To 2
  Pin(22) = 0
  us = Stream("BITSTREAM 22, 2, c(), 0, 22, 1, d(), 0, " + Str$(i))
  n = n * 10 + Pin(38)
Next
Check "one pin, high 1-2ms and from 3ms in one, from 3ms in the other: XOR, AND, OR end 1, 0, 1", n, 101
On Error Skip 1 : BITSTREAM 22, 2, c(), 0, 36, 1, d(), 0, 1
CheckS "  the logic with two pins", MM.ErrMsg$, "Logic parameter only valid when both pins are the same"
c(0) = 60000 : c(1) = 60000 : Pin(22) = 0
us = Stream("BITSTREAM 22, 2, c()")
Check "times above 50000us (they never ended) end: 2 x 60000us, low again", Pin(38), 0
SetPin 22, Off : SetPin 36, Off : SetPin 38, Off : SetPin 35, Off

' ---- DEVICE SERIALTX, read by COM1
Open "COM1:9600" As #1
Pause 50 : e$ = Input$(255, #1)
Device SerialTX 24, 9600, "Hello"
Pause 50 : e$ = Input$(255, #1)
CheckS "SERIALTX at 9600 to COM1", e$, "Hello"
Close #1
Open "COM1:115200" As #1
Pause 50 : e$ = Input$(255, #1)
Device SerialTX 24, 115200, "AbCdEfG"
Pause 50 : e$ = Input$(255, #1)
CheckS "  at 115200", e$, "AbCdEfG"
Close #1
SetPin 24, Off

' ---- DEVICE SERIALRX on pin 38, from PWM 1B on pin 22 (COM1 sends a byte an interrupt, so only one while SERIALRX
' has them off). A square wave at half the baud rate is a stream of "U" (&H55, its bits alternate); one at a tenth,
' 90% high, a stream of &HFF. A start in the low part is a "Framing error", and it is tried again after a pause.
SetPin 22, Off : SetPin 38, Off
Device SerialRX 38, 9600, r$, 50, st
CheckS "nothing sent: timeout, status -1, empty", Str$(st) + "[" + r$ + "]", "-1[]"
SetPin 38, Off
Pwm 1, 4800, 50, 50
RxP 9600, 4, ""
CheckS "SERIALRX at 9600, 4 characters from a square wave: UUUU, status 2", r$ + Str$(st), "UUUU2"
Pwm 1, 57600, 50, 50
RxP 115200, 6, ""
CheckS "  at 115200: UUUUUU, status 2", r$ + Str$(st), "UUUUUU2"
Pwm 1, 960, 90, 90
RxP 9600, 64, Chr$(255)
CheckS "  to a terminator (&HFF): one character, status 3", Str$(Len(r$)) + Str$(Asc(r$)) + Str$(st), "12553"
Pwm 1, 4800, 50, 50
tries = 0
Do
  On Error Skip 1
  Device SerialRX 38, 9600, r3$, 100, st
  tries = tries + 1
  Pause Rnd * 3
Loop While MM.ErrMsg$ = "Framing error" And tries < 12
CheckS "  into a LENGTH 3 string, no count given: UUU, status 2", r3$ + Str$(st), "UUU2"
Pwm 1, Stop
SetPin 38, Off

Print "ioserial:"; pass; " of"; tests; " passed"
End

' how long a BITSTREAM command took, in microseconds, by TIMER: its counter runs on while interrupts are off, and a
' 50ms overflow missed then is counted when they are on again (two are not: so under 50ms)
Function Stream(cmd$)
  Local float t0
  t0 = Timer
  Execute cmd$
  Stream = (Timer - t0) * 1000
End Function

' SERIALRX on pin 38 into r$ (nbr, terminators), tried again after a framing error
Sub RxP(baud, nbr, t$)
  Local k
  For k = 1 To 12
    On Error Skip 1
    Device SerialRX 38, baud, r$, 200, st, nbr, t$                ' (no terminators when t$ is empty)
    If MM.ErrMsg$ <> "Framing error" Then Exit Sub
    Pause Rnd * 3                                                   ' (each try starts at another point of the wave: tried
  Next                                                              ' at once, it came back to the same low part)
End Sub

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
