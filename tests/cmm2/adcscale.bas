' adcscale.bas - ADC START's per-channel scale and offset, and ADC TRIGGER's timeout (M10 of the b14 merge, the
' PicoMite's).  Nothing is connected to the pins, so the readings are noise: a channel whose bottom and top are the same
' value reads that value exactly, every other range is checked by its bounds, and a trigger that may never come ends
' with its timeout.
Option Explicit
Dim float a(19), b(19), lo, hi
Dim integer i, ok
Dim string e$
ADC Open 2000, 7, 12
ADC Start a(), b()
ok = 1
For i = 0 To 19 : If a(i) < 0 Or a(i) > 3.31 Or b(i) < 0 Or b(i) > 3.31 Then ok = 0
Next
Print "volts: "; Choice(ok, "in 0 to 3.3", "OUT OF RANGE")
ADC Start a(), b(), , 2.5, 2.5, -7, -7
ok = 1
For i = 0 To 19 : If a(i) <> 2.5 Or b(i) <> -7 Then ok = 0
Next
Print "fixed: "; a(0); " "; b(0); " "; Choice(ok, "every sample", "NOT EVERY SAMPLE")
ADC Start a(), b(), , 0, 100, 1000, 0
ok = 1
For i = 0 To 19 : If a(i) < 0 Or a(i) > 100 Or b(i) < 0 Or b(i) > 1000 Then ok = 0
Next
Print "scaled: "; Choice(ok, "in 0 to 100 and 1000 to 0", "OUT OF RANGE")
ADC Start a()
ok = 1
For i = 0 To 19 : If a(i) < 0 Or a(i) > 3.31 Then ok = 0
Next
Print "volts again: "; Choice(ok, "in 0 to 3.3", "OUT OF RANGE")
ADC Trigger 1, 3.25, 30
ADC Start a()
Print "trigger timeout: completed"
On Error Skip 1
ADC Trigger 1, 4
e$ = MM.ErrMsg$ : Print "trigger voltage: "; e$
ADC Close
Print "done"
