' a6test.bas - the output of A6 (docs/Audio_Review_and_Merge_Plan.md): volume ramps, the glide to the midpoint on
' PAUSE, the ramp on stop, and polyBLEP square edges, all read at the DAC (DOR1). Needs A:/snd/ts_wav.wav.
Dim integer i, n, v(399)
Dim float t, tt(399)
Const DOR1 = &H4000742C

Print "-- PAUSE: the distance from 2000 after it, by time"
Play Tone 100, 100 : Pause 300
Do : Loop Until Abs((Peek(WORD DOR1) And &HFFF) - 2000) > 1500      ' near a peak of the tone
Play Pause
t = Timer
For i = 0 To 399 : v(i) = Abs((Peek(WORD DOR1) And &HFFF) - 2000) : tt(i) = Timer - t : Next
showpath
Play Stop

Print "-- PLAY VOLUME 100 to 0 on a tone: the peak in each 2.5 ms (the old firmware stepped at once)"
Play Volume 100, 100 : Play Tone 440, 440 : Pause 200
Play Volume 0, 0
peaks 16, 2.5
Play Stop : Play Volume 100, 100
Print "-- on a WAV, 100 to 10: the peak in each 20 ms (the buffers already converted play first)"
Play Wav "A:/snd/ts_wav.wav" : Pause 300
Play Volume 10, 10
peaks 10, 20
Play Stop : Play Volume 100, 100
Print "-- PLAY SOUND channel 25 to 5: the peak in each 5 ms"
Play Sound 1, B, S, 440, 25 : Pause 200
Play Sound 1, B, S, 440, 5
peaks 10, 5
Play Stop

Print "-- square edges: samples between the extremes of a 5 kHz square (none on a naive square)"
Play Sound 1, B, Q, 5000, 25 : Pause 100
lo = 4095 : hi = 0 : n = 0
For i = 1 To 20000 : y = Peek(WORD DOR1) And &HFFF : If y < lo Then lo = y
If y > hi Then hi = y
Next
For i = 1 To 20000 : y = Peek(WORD DOR1) And &HFFF : If y > lo + 20 And y < hi - 20 Then n = n + 1
Next
Print "extremes "; lo; " and "; hi; ", "; n; " of 20000 samples between"
Play Stop
Play Sound 1, B, S, 5000, 25 : Pause 100
Print "a sine for comparison: ";
n = 0 : For i = 1 To 20000 : y = Peek(WORD DOR1) And &HFFF : If y > lo + 20 And y < hi - 20 Then n = n + 1
Next
Print n; " between"
Play Stop

Print "-- stop: 50 tones stopped near a peak"
t = Timer
For i = 1 To 50
  Play Tone 100, 100 : Pause 3
  Do : Loop Until Abs((Peek(WORD DOR1) And &HFFF) - 2000) > 1500
  Play Stop
Next
Print "after the last: DAC "; Peek(WORD DOR1) And &HFFF; ", "; Str$((Timer - t) / 50, 0, 1); " ms a tone (3 ms pause, up to 10 ms waiting for a peak, the ramp)"
Print "done"
End

Sub showpath
  Local integer k, j
  Local float ms = 0
  Print "  ms: distance";
  For k = 0 To 399
    If tt(k) >= ms Then Print "  "; Str$(tt(k), 0, 2); ": "; v(k); : ms = ms + 1
    If v(k) = 0 Then Print "  0 at "; Str$(tt(k), 0, 2); " ms"; : Exit For
  Next
  Print
End Sub

' the peak distance from 2000 in each of n windows of w ms
Sub peaks n, w
  Local integer k, p, y
  Local float t0
  Print "  ";
  For k = 1 To n
    p = 0 : t0 = Timer
    Do While Timer - t0 < w
      y = Abs((Peek(WORD DOR1) And &HFFF) - 2000) : If y > p Then p = y
    Loop
    Print p; " ";
  Next
  Print
End Sub
