' steptime.bas - STEPPER's 200 kHz tick on the CMM2 (Src/stepper_cmm2.c, tools/port_stepper.py; 25 Oct 2026): the step
' pulse's high time (one tick, 5 us), the low time between pulses at the top rate (one step every two ticks: 100 kHz),
' that rate, and the CPU the tick takes. X STEP on pin 8 (GP0) wired to pin 15, read by PULSIN and counted (COUNT3).
' (The PicoMite's StepperTiming.bas times its pads with PIO.)
Option Explicit
Option Default Integer
Dim tests, pass, i, mn, mx, w, n, t, t0, t1, t2

Sub Start(vmax, accel)
  STEPPER INIT
  STEPPER AXIS X, 8, 22, , 0, 80, vmax, accel
  STEPPER POSITION X, 0
  STEPPER RUN
  STEPPER GCODE G90
End Sub

' the step pulse's high time: 20 pulses at 800 steps/s
SetPin 15, DIn
Start 6000, 500
STEPPER GCODE G1, X, 100, F, 600
Pause 500
mn = 1000 : mx = 0
For i = 1 To 20
  w = Pulsin(15, 1)
  If w < mn Then mn = w
  If w > mx Then mx = w
Next
Check "step pulse high time 4 to 7 us (one 5 us tick; PULSIN's 1 us resolution)", mn >= 4 And mx <= 7, 1
STEPPER CLOSE

' the top rate: one step every two ticks
Start 600000, 50000
STEPPER GCODE G1, X, 10000, F, 600000                               ' (80 steps/mm at 10000 mm/s: over the cap)
Pause 500
mn = 1000
For i = 1 To 20
  w = Pulsin(15, 0)
  If w < mn Then mn = w
Next
Check "low time between pulses at the top rate: at least 4 us", mn >= 4, 1
STEPPER CLOSE
SetPin 15, CIn
Start 600000, 50000
STEPPER GCODE G1, X, 10000, F, 600000
Pause 500
Pin(15) = 0 : Pause 500 : n = Pin(15)
Check "the top step rate: 95000 to 101000 steps/s (one step every two 200 kHz ticks)", n >= 47500 And n <= 50500, 1
STEPPER CLOSE

' the CPU the tick takes
t = Timer : For i = 1 To 300000 : Next : t0 = Timer - t
STEPPER INIT
t = Timer : For i = 1 To 300000 : Next : t1 = Timer - t
STEPPER CLOSE
Start 600000, 50000
STEPPER GCODE G1, X, 10000, F, 600000
Pause 500
t = Timer : For i = 1 To 300000 : Next : t2 = Timer - t
STEPPER CLOSE
Check "the tick idle takes under 10% of the CPU", (t1 - t0) * 10 < t1, 1
Check "stepping at 100 kHz, under 35%", (t2 - t0) * 100 < t2 * 35, 1

SetPin 15, Off
Print "steptime:"; pass; " of"; tests; " passed"
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
