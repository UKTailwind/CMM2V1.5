' stepaudio.bas - listen: a 440 Hz tone with the stepper idle, stepping flat out and planning hard (25 Oct 2026). On the
' CMM2 the stepper's critical sections mask only its own 200 kHz tick, which is above every other interrupt; each audio
' sample is written by an interrupt (TIM6), so any glitch heard is the tick holding that interrupt off. The stepper
' takes priority: audio may suffer, motion must not. Uses X STEP on pin 8 and Y STEP on pin 10 (no motors needed).
' Not checked by the harness: run it and listen.
Option Explicit
Option Default Integer
Dim k, t
Dim float a, x, y, px, py

Sub Phase(n$, secs)
  Print n$ : Text MM.HRes \ 2, MM.VRes \ 2, Space$(60), CM : Text MM.HRes \ 2, MM.VRes \ 2, n$, CM
  t = Timer
End Sub

CLS
Play Tone 440, 440
Phase "1 of 4: the tone alone (10 s)", 10
Pause 10000

STEPPER INIT 0.5, 64
STEPPER AXIS X, 8, 22, , 0, 80, 600000, 50000
STEPPER AXIS Y, 10, 36, , 0, 80, 600000, 50000
STEPPER POSITION X, 0
STEPPER POSITION Y, 0
Phase "2 of 4: the 200 kHz tick running, no motion (10 s)", 10
Pause 10000

STEPPER RUN
STEPPER GCODE G90
Phase "3 of 4: stepping at 100 kHz (10 s)", 10
STEPPER GCODE G1, X, 10000, F, 600000
Pause 10000
STEPPER ESTOP
STEPPER CLOSE

STEPPER INIT 0.5, 64
STEPPER AXIS X, 8, 22, , 0, 80, 60000, 5000
STEPPER AXIS Y, 10, 36, , 0, 80, 60000, 5000
STEPPER POSITION X, 55
STEPPER POSITION Y, 50
STEPPER RUN
STEPPER GCODE G90
STEPPER GCODE G64
Phase "4 of 4: short blended arcs and lines queued as fast as they go (10 s)", 10
k = 0 : px = 55 : py = 50                                          ' (round a circle of radius 5 about 50,50, clockwise)
Do While Timer - t < 10000
  If MM.Code > 0 Then
    k = k + 1
    a = Rad(-10 * k)
    x = 50 + 5 * Cos(a) : y = 50 + 5 * Sin(a)
    If k Mod 2 Then
      STEPPER GCODE G1, X, x, Y, y, F, 6000
    Else
      STEPPER GCODE G2, X, x, Y, y, I, 50 - px, J, 50 - py, F, 6000
    EndIf
    px = x : py = y
  EndIf
Loop
STEPPER ESTOP
STEPPER CLOSE
Play Stop
Print "Done:"; k; " moves queued in phase 4"
