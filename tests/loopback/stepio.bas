' stepio.bas - STEPPER's inputs and outputs (the PicoMite's stepper, Src/stepper.c; its StepperPullTest.bas, 25 Oct
' 2026, with real trips through the loop-back wires). X STEP on pin 8 (GP0) wired to pin 15 (COUNT3), X DIR on pin 22;
' pin 7 wired to pin 27 and pin 13 to pin 28, which BASIC drives as an E-STOP and a limit switch (27 and 28 have the
' board's I2C2 pull-ups). Pins 32, 33, 26, 21 and 37 are not wired: their pulls alone decide the inputs' levels.
' PIN() refuses the pins STEPPER reserves, so these are read from their GPIO port's registers (as the PicoMite's test
' reads its pads): IDR, the level, and PUPDR, the pull.
Option Explicit
Option Default Integer
Dim tests, pass, t, n

Function PBase(p)                                                   ' the port of a pin used here (Inc/IOPorts.h)
  Select Case p
    Case 37 : PBase = &H58020000                                    ' GPIOA
    Case 21 : PBase = &H58020400                                    ' GPIOB
    Case 26 : PBase = &H58020800                                    ' GPIOC
    Case 32, 33 : PBase = &H58022000                                ' GPIOI
  End Select
End Function
Function PBit(p)
  Select Case p
    Case 37 : PBit = 1
    Case 21 : PBit = 4
    Case 26 : PBit = 5
    Case 32 : PBit = 8
    Case 33 : PBit = 3
  End Select
End Function
Function Lvl(p)                                                     ' the pin's level (IDR)
  Lvl = (Peek(Word PBase(p) + &H10) >> PBit(p)) And 1
End Function
Function Pull(p)                                                    ' its pull (PUPDR): 1 up, 2 down, 0 none
  Pull = (Peek(Word PBase(p) + &H0C) >> (2 * PBit(p))) And 3
End Function

Sub Wait
  t = Timer
  Do : Loop Until (Peek(STEPPER ACTIVE) = 0 And Peek(STEPPER BUFFER) = MM.Code) Or Timer - t > 30000
End Sub

Print "-- the pulls and the outputs' first levels (the PicoMite's 69faf14 and 37c5d1c)"
STEPPER INIT 0.5, 32, 32
Check "E-STOP on pin 32: pulled up", Pull(32) * 10 + Lvl(32), 11
STEPPER AXIS X, 8, 22, 21, 0, 80, 6000, 500
Check "enable on pin 21 after STEPPER AXIS: high, the driver off", Lvl(21), 1
STEPPER ENABLE X, 1
Check "  after STEPPER ENABLE X, 1: low", Lvl(21), 0
STEPPER ENABLE X, 0
Check "  after STEPPER ENABLE X, 0: high", Lvl(21), 1
STEPPER HWLIMITS 33, 26, 37
Check "active-low limits on pins 33, 26, 37: pulled up and high", Pull(33) + Pull(26) + Pull(37) + 10 * (Lvl(33) + Lvl(26) + Lvl(37)), 33
STEPPER CLOSE
STEPPER INIT
STEPPER AXIS X, 8, 22, , 0, 80, 6000, 500
STEPPER HWLIMITS 33, 26, 37, 0, 0, 0, 7
Check "active-high limits (invert mask 7): pulled down and low", Pull(33) + Pull(26) + Pull(37) + 10 * (Lvl(33) + Lvl(26) + Lvl(37)), 6
STEPPER CLOSE

STEPPER INIT
STEPPER AXIS X, 8, 22, , 0, 80, 6000, 500
STEPPER POSITION X, 0
STEPPER SPINDLE 32
Check "spindle on pin 32, active high: off after STEPPER SPINDLE", Lvl(32), 0
STEPPER RUN
STEPPER GCODE M03 : Pause 50
Check "  on after M03", Lvl(32), 1
STEPPER GCODE M05 : Pause 50
Check "  off after M05", Lvl(32), 0
STEPPER CLOSE
STEPPER INIT
STEPPER AXIS X, 8, 22, , 0, 80, 6000, 500
STEPPER POSITION X, 0
STEPPER SPINDLE 32, 1
Check "inverted (active low): off, high, after STEPPER SPINDLE", Lvl(32), 1
STEPPER RUN
STEPPER GCODE M03 : Pause 50
Check "  on, low, after M03", Lvl(32), 0
STEPPER GCODE M05 : Pause 50
Check "  off after M05", Lvl(32), 1
STEPPER CLOSE

Print "-- an E-STOP and a limit switch tripped part way through a move"
SetPin 15, CIn
SetPin 7, DOut : Pin(7) = 1
STEPPER INIT 0.5, 32, 27
STEPPER AXIS X, 8, 22, , 0, 80, 6000, 500
STEPPER POSITION X, 0
STEPPER RUN
STEPPER GCODE G90
Pin(15) = 0
STEPPER GCODE G1, X, 100, F, 600                                   ' 8000 steps over 10 s
Pause 1000
Pin(7) = 0                                                          ' the E-STOP pressed
Pause 200
n = Pin(15)
Check "E-STOP: the move stops part way (some pulses, not all 8000)", n > 100 And n < 2000, 1
Check "  no more pulses after it", Pin(15), n
Check "  PEEK(STEPPER STATUS) has E-STOP (bit 6)", (Peek(STEPPER STATUS) >> 6) And 1, 1
Check "  and nothing active", Peek(STEPPER ACTIVE), 0
Pin(7) = 1
STEPPER POSITION X, 5                                               ' (the trip is reported, then the command runs: the
Check "  the command after the trip runs: X is 5", Peek(STEPPER X), 5   ' PicoMite's #137, 27b13db)
STEPPER CLOSE

SetPin 13, DOut : Pin(13) = 1
STEPPER INIT
STEPPER AXIS X, 8, 22, , 0, 80, 6000, 500
STEPPER HWLIMITS 28, 33, 26
STEPPER POSITION X, 100
STEPPER RUN
STEPPER GCODE G90
Pin(15) = 0
STEPPER GCODE G1, X, 0, F, 600
Pause 1000
Pin(13) = 0                                                         ' X_MIN reached
Pause 200
n = Pin(15)
Check "X_MIN: the move stops part way", n > 100 And n < 2000, 1
Check "  no more pulses after it", Pin(15), n
Check "  PEEK(STEPPER STATUS) has X_MIN (bit 0)", Peek(STEPPER STATUS) And 1, 1
Pin(13) = 1
STEPPER STATUS
STEPPER CLOSE

Print "-- G28 with an unconnected X_MIN (pin 33)"
STEPPER INIT
STEPPER AXIS X, 8, 22, , 0, 2, 60000, 1000
STEPPER HWLIMITS 33, 26, 37
STEPPER POSITION X, 0
On Error Skip
STEPPER GCODE G28, X, 1
Check "G28 runs to its end and stops with an error", MM.ErrNo <> 0, 1
Print "  ("; MM.ErrMsg$; ")"
STEPPER CLOSE

SetPin 15, Off : SetPin 7, Off : SetPin 13, Off
Print "stepio:"; pass; " of"; tests; " passed"
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
