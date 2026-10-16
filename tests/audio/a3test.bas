' a3test.bas - the PicoMite's PLAY ARRAY, SAMPLE, RELEASE and LOAD SOUND, and PLAY SOUND's string forms, "M" and "U"
' (docs/Audio_Review_and_Merge_Plan.md A3). Needs A:/snd/ts_wav.wav from gen_audio.py.
Dim integer fired, i, j, v, s
Dim float t
Dim integer a%(1999), b%(1999), c%(10999), w%(99), x%(99), u%(1023), z%(499)
Const DOR1 = &H4000742C

' a 440 Hz sine at 8000 samples a second, four to an element: 8000 samples, 1 s
For i = 0 To 1999
  v = 0
  For j = 0 To 3
    s = Int(16000 * Sin(2 * Pi * 440 * (i * 4 + j) / 8000))
    v = v Or ((s And &HFFFF) << (16 * j))
  Next
  a%(i) = v : b%(i) = v
Next
' 44000 samples of silence for 44000 a second, and one cycle of a sine in 100 elements for PLAY SAMPLE
For i = 0 To 99 : w%(i) = Int(16000 * Sin(2 * Pi * i / 100)) : x%(i) = w%(i) : Next
' PLAY LOAD SOUND's table: a square wave, 2048 values of 3900 then 2048 of 100, four to an element
For i = 0 To 1023
  If i < 512 Then v = 3900 Else v = 100
  u%(i) = v Or (v << 16) Or (v << 32) Or (v << 48)
Next

Print "-- PLAY ARRAY: the time to its interrupt"
timearray 8000, 0, 8000, "8000 samples at 8000 a second (1 s)"
timearray 44000, 0, 44000, "44000 at 44000 a second (1 s), the silent array"
timearray 100, 0, 100, "100 at 100 a second (1 s)"
timearray 8000, 4000, 8000, "samples 4000 to 8000 at 8000 (0.5 s)"
Play Array a%(), b%(), 8000 : Pause 200 : Print "playing: "; MM.Info(SOUND); "  "; level$()
Play Pause : Print "paused: "; MM.Info(SOUND) : Play Resume : Print "resumed: "; MM.Info(SOUND)
On Error Skip : Play Wav "A:/snd/ts_wav.wav"
Print "WAV during ARRAY: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Play Stop
On Error Skip : Play Array a%(), z%(), 8000
Print "sizes differ: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
On Error Skip : Play Array a%(), b%(), 5
Print "5 a second: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
localarray
Play Stop
fired = 0 : t = Timer : Play Wav "A:/snd/ts_wav.wav", done
Do While fired = 0 And Timer - t < 6000 : Loop
Print "a WAV after ARRAY at 100 a second: "; Str$((Timer - t) / 1000, 0, 1); " s (about 2.1; 640 if the repeat were kept)"

Print "-- PLAY SAMPLE and RELEASE"
fired = 0
Play Sample w%(), x%(), 440, 20, 100, 50, 200, done
Pause 400 : Print "sustaining: "; MM.Info(SOUND); "  "; level$()
t = Timer : Play Release
Do While fired = 0 And Timer - t < 3000 : Loop
Print "RELEASE of 200 ms to its interrupt: "; Choice(fired, Str$(Timer - t, 0, 0) + " ms", "no interrupt"); "  "; MM.Info(SOUND)
fired = 0 : t = Timer
Play Sample w%(), x%(), 440, 50, 300, 0, 100, done
Do While fired = 0 And Timer - t < 3000 : Loop
Print "sustain 0 (50 + 300 ms) to its interrupt: "; Choice(fired, Str$(Timer - t, 0, 0) + " ms", "no interrupt")
On Error Skip : Play Release
Print "RELEASE with nothing playing: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Play Sample w%(), x%(), 440, 0, 0, 100, 0 : Pause 200 : Play Pause : Print "paused: "; MM.Info(SOUND)
Play Resume : Pause 100 : Print "resumed: "; MM.Info(SOUND); "  "; level$() : Play Stop

Print "-- PLAY LOAD SOUND and type U"
On Error Skip : Play Sound 1, B, U, 440
Print "U before loading: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error") : Play Stop
On Error Skip : Play Load Sound z%()
Print "500 elements: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Play Load Sound u%()
On Error Skip : Play Load Sound u%()
Print "loaded twice: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Play Sound 1, B, U, 440, 25 : Pause 100 : Print "U: "; MM.Info(SOUND); "  "; level$()
Play Sound 1, B, Q, 440, 25 : Pause 100 : Print "Q for comparison:  "; level$()
Play Stop
On Error Skip : Play Sound 1, B, U, 440
Print "U after PLAY STOP: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error") : Play Stop

Print "-- PLAY SOUND's position and type as strings"
p$ = "M" : k$ = "s"
Play Sound 1, p$, k$, 440 : Pause 100 : Print "M and s: "; MM.Info(SOUND); "  "; level$()
Play Sound 2, "L", "T", 660 : Pause 100 : Print "L and T: "; MM.Info(SOUND)
On Error Skip : Play Sound 1, "X", "S", 440
Print "position X: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
On Error Skip : Play Sound 1, "B", "Z", 440
Print "type Z: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
On Error Skip : Play Sound 1, "B", "SS", 440
Print "type SS: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Play Stop
Print "done"
End

Sub timearray rate, first, last, d$
  fired = 0 : t = Timer
  If rate = 44000 Then
    Play Array c%(), c%(), rate, first, last, done
  Else
    Play Array a%(), b%(), rate, first, last, done
  EndIf
  Do While fired = 0 And Timer - t < 5000 : Loop
  Print "  "; d$; ": "; Choice(fired, Str$((Timer - t) / 1000, 0, 2) + " s", "no interrupt"); "  then "; MM.Info(SOUND)
End Sub

Sub localarray
  Local integer q%(9)
  On Error Skip : Play Array q%(), q%(), 8000
  Print "a local array: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
End Sub

Function level$()
  Local integer lo = 4095, hi = 0, y, k
  For k = 1 To 5000
    y = Peek(WORD DOR1) And &HFFF
    If y < lo Then lo = y
    If y > hi Then hi = y
  Next
  level$ = "DAC " + Str$(lo) + " to " + Str$(hi)
End Function

Sub done
  fired = fired + 1
End Sub
