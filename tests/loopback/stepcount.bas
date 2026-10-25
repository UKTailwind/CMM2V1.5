' stepcount.bas - STEPPER's step pulses counted (the PicoMite's stepper, Src/stepper.c; 25 Oct 2026): X STEP on pin 8
' (GP0) wired to pin 15 (COUNT3), Y STEP on pin 10 (GP1) wired to pin 24 (COUNT4), X DIR on pin 22 (GP3) wired to pin
' 38, Y DIR on pin 36 (GP4) wired to pin 35. Lines and arcs at 80 steps/mm: the pulses each axis gave, its DIR at the
' end and its position. (The PicoMite's StepperTiming reads the pads with PIO; this counts them on the wires.)
Option Explicit
Option Default Integer
Dim tests, pass, t
Dim float x, y

SetPin 15, CIn : SetPin 24, CIn
SetPin 38, DIn : SetPin 35, DIn

Sub Start
  STEPPER INIT
  STEPPER AXIS X, 8, 22, , 0, 80, 6000, 500
  STEPPER AXIS Y, 10, 36, , 0, 80, 6000, 500
  STEPPER POSITION X, 50
  STEPPER POSITION Y, 60
  STEPPER RUN
  STEPPER GCODE G90
  Pin(15) = 0 : Pin(24) = 0                                         ' (the counts zeroed)
End Sub

Sub Wait
  t = Timer
  Do : Loop Until (Peek(STEPPER ACTIVE) = 0 And Peek(STEPPER BUFFER) = MM.Code) Or Timer - t > 30000
End Sub

Start
STEPPER GCODE G1, X, 60, F, 1200
Wait
Check "G1 X 50 to 60: X pulses", Pin(15), 800
Check "  Y pulses", Pin(24), 0
Check "  X DIR high (positive)", Pin(38), 1
STEPPER GCODE G1, X, 40
Wait
Check "G1 X 60 to 40: X pulses in all", Pin(15), 2400
Check "  X DIR low (negative)", Pin(38), 0
STEPPER GCODE G1, X, 50
STEPPER GCODE G1, Y, 50
Wait
Check "G1 X 40 to 50 and Y 60 to 50: X pulses in all", Pin(15), 3200
Check "  Y pulses", Pin(24), 800
Check "  Y DIR low", Pin(35), 0
STEPPER CLOSE

Start
STEPPER GCODE G1, X, 60, F, 1200
STEPPER GCODE G1, X, 40
STEPPER GCODE G1, X, 50
For t = 1 To 2
  STEPPER GCODE G2, X, 50, Y, 40, I, 0, J, -10
  STEPPER GCODE G2, X, 50, Y, 60, I, 0, J, 10
Next
Wait
Check "the PicoMite's test 2 (lines and four semicircles): X pulses", Pin(15), 9600
x = Peek(STEPPER X) : y = Peek(STEPPER Y)
Check "  back at X 50", x = 50, 1
Check "  and Y 60", y = 60, 1
STEPPER CLOSE

Start
STEPPER GCODE G91
STEPPER GCODE G1, X, 0.4358, F, 600
For t = 2 To 10 : STEPPER GCODE G1, X, 0.4358 : Next
Wait
Check "ten G91 moves of 0.4358 mm: X pulses (4.358 mm: 348.64, on the nearest step)", Pin(15), 349
STEPPER CLOSE

SetPin 15, Off : SetPin 24, Off : SetPin 38, Off : SetPin 35, Off
Print "stepcount:"; pass; " of"; tests; " passed"
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
