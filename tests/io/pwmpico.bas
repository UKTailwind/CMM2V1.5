' pwmpico.bas - PWM and SERVO with OPTION PICOMITE: the PicoMite's arguments (Src/PWM.c), read back from the timers as
' pwm.bas reads them. It drives the PWM pins (12, 22, 36, 31), so it runs on a board where they are free (the G1):
'   python -I tools/goldens.py COM33 OUT --baud 115200 --corpus tests/io --dir A:/i --only pwmpico --nomodemap
'     --pre "OPTION PICOMITE HDMI"
' and OPTION PICOMITE NONE after. PWM's 5th argument is phase (centre), its 6th defer (only 0); SERVO takes positions,
' -20 to 120, at 50Hz; OFF; SETPIN GPn, PWM before it, as a PicoMite program has it.
Option Explicit
Const T3 = &H40000400, T8 = &H40010400
Dim integer clk = MM.Info(CPUSPEED) / 2

Print "-- PWM"
SetPin GP2, PWM
SetPin GP3, PWM1B
PWM 1, 1000, 25, 50
show "1, 1000, 25, 50", T3, 3
PWM 1, 1000, 25, 50, 1
show "1, 1000, 25, 50, 1 (phase)", T3, 3
PWM 1, 1000, 25, 50, 0, 0
show "1, 1000, 25, 50, 0, 0", T3, 3
On Error Skip 1
PWM 1, 1000, 25, 50, 0, 1
Print "defer 1: "; MM.ErrMsg$
PWM 1, OFF
show "1, OFF", T3, 3
PWM 1, 1000, , 50
show "1, 1000, , 50", T3, 3
PWM 1, OFF
SetPin GP4, PWM
SetPin GP5, PWM
PWM 2, 100, -10, 90
show "2, 100, -10, 90", T8, 2
PWM 2, OFF
On Error Skip 1
PWM 0, 1000, 50
Print "PWM 0: "; MM.ErrMsg$

Print "-- SERVO"
Servo 1, 50
show "1, 50", T3, 3
Servo 1, 0, 100
show "1, 0, 100", T3, 3
Servo 1, -20, 120
show "1, -20, 120", T3, 3
Servo 1, , 30
show "1, , 30", T3, 3
On Error Skip 1
Servo 1, 121
Print "SERVO 1, 121: "; MM.ErrMsg$
On Error Skip 1
Servo 1, 1, 2, 3
Print "SERVO 1, 1, 2, 3: "; MM.ErrMsg$
Servo 1, OFF
show "1, OFF", T3, 3
Servo 2, 50, 25
show "2, 50, 25", T8, 2
Servo 2, OFF
Print "end"

' the timer: on or stopped, its frequency and counting, and each output's duty (% of the period, - inverted) or off
Sub show(t$, b%, n%)
  Local integer psc = PEEK(WORD b% + &H28), arr = PEEK(WORD b% + &H2C), cr1 = PEEK(WORD b%), ccer = PEEK(WORD b% + &H20)
  Local integer centre = (cr1 >> 5) And 3, c, i
  Print t$; ":";
  If (cr1 And 1) = 0 Then Print " stopped": Exit Sub
  Print " f="; Str$(clk / (psc + 1) / (arr + 1) / Choice(centre, 2, 1), 0, 1); Choice(centre, " centre", "");
  For i = 0 To n% - 1
    c = PEEK(WORD b% + &H34 + 4 * i)
    If (ccer >> (4 * i)) And 1 Then
      Print " "; Chr$(65 + i); "="; Choice((ccer >> (4 * i + 1)) And 1, "-", ""); Str$(c * 100 / Choice(centre, arr, arr + 1), 0, 1);
    Else
      Print " "; Chr$(65 + i); " off";
    EndIf
  Next
  Print
End Sub
