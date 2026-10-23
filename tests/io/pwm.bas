' pwm.bas - PWM and SERVO (Src/PWM.c), read back from the timers: TIM3 is PWM 1 (1A pin 12, 1B 22, 1C 29) and TIM8
' PWM 2 (2A 36, 2B 31). It drives those pins, so it runs on a board where they are free (the G1):
'   python -I tools/goldens.py COM33 OUT --baud 115200 --corpus tests/io --dir A:/i --only pwm
' with OPTION PICOMITE NONE (pwmpico.bas has the PicoMite's forms). An empty duty or width leaves its output as it is,
' and an output can be started after the others; OFF stops as STOP does; centre and up counting at one timer frequency.
Option Explicit
Const T3 = &H40000400, T8 = &H40010400
Dim integer clk = MM.Info(CPUSPEED) / 2
Dim integer pp(4) = (12, 22, 29, 36, 31)

Print "-- PWM 1"
PWM 1, 1000, 25
show "1, 1000, 25", T3, 3
PWM 1, 1000, 25, 50
show "1, 1000, 25, 50", T3, 3
PWM 1, 2000, , 75
show "1, 2000, , 75 (A's count kept)", T3, 3
PWM 1, 2000, 10, , 30
show "1, 2000, 10, , 30", T3, 3
pins
PWM 1, OFF
show "1, OFF", T3, 3
pins

Print "-- output A after B"
PWM 1, 1000, , 40
show "1, 1000, , 40", T3, 3
pins
PWM 1, 1000, 10, 40
show "1, 1000, 10, 40", T3, 3
pins
PWM 1, STOP

Print "-- centre"
PWM 1, 1000, 30, , , 1
show "1, 1000, 30, , , 1", T3, 3
PWM 1, 2000, 30
show "1, 2000, 30", T3, 3
PWM 1, 2000, 30, , , 1
show "1, 2000, 30, , , 1", T3, 3
PWM 1, STOP

Print "-- PWM 2"
PWM 2, 500, 10, -20
show "2, 500, 10, -20", T8, 2
PWM 2, 500, , 60
show "2, 500, , 60", T8, 2
pins
PWM 2, OFF
show "2, OFF", T8, 2

Print "-- SERVO"
Servo 1, 1.5
show "1, 1.5", T3, 3
Servo 1, 100, 1.2, 1.8
show "1, 100, 1.2, 1.8", T3, 3
Servo 1, , 2
show "1, , 2 (A's count kept)", T3, 3
Servo 1, OFF
Servo 1, 50
show "1, 50 (a frequency)", T3, 3
pins
Servo 1, STOP
Servo 2, 1, 2
show "2, 1, 2", T8, 2
Servo 2, STOP

Print "-- errors"
On Error Skip 1
PWM 3, 100, 50
Print "PWM 3: "; MM.ErrMsg$
On Error Skip 1
PWM 1, 100, 150
Print "PWM 1, 100, 150: "; MM.ErrMsg$
On Error Skip 1
Servo 1, 19
Print "SERVO 1, 19: "; MM.ErrMsg$
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

' the PWM pins' state: 12, 22, 29 (1A-1C), 36, 31 (2A, 2B)
Sub pins
  Local integer i
  Print "  pins";
  For i = 0 To 4
    Print " "; Str$(pp(i)); " "; MM.Info$(PIN pp(i));
  Next
  Print
End Sub
