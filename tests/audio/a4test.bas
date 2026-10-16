' a4test.bas - the PicoMite's BBC sound: PLAY BBC SOUND, PLAY BBC ENVELOPE and MM.INFO(BBC BUFFER)
' (docs/Audio_Review_and_Merge_Plan.md A4). Needs A:/snd/ts_wav.wav from gen_audio.py.
Dim integer i, n, ok
Dim float t
Const DOR1 = &H4000742C

Print "-- one note: pitch 89 is A4, 440 Hz; duration 20 is 1 s"
Print "BBC BUFFER before any PLAY BBC: "; MM.Info(BBC BUFFER)
t = Timer
Play BBC Sound 1, -15, 89, 20
Pause 100
Print "playing: "; MM.Info(SOUND); "  "; level$(); "  "; Str$(freq(), 0, 0); " Hz"
Do While MM.Info(SOUND) <> "OFF" And Timer - t < 5000 : Loop
Print "to OFF: "; Str$((Timer - t) / 1000, 0, 2); " s (1 s and a buffer or two)"
Play BBC Sound 1, -15, 137, 20 : Pause 100 : Print "pitch 137 (137 is an octave up: 880): "; Str$(freq(), 0, 0); " Hz"
Play BBC Sound &H11, -15, 41, 20 : Pause 100 : Print "flushed to pitch 41 (A3, 220): "; Str$(freq(), 0, 0); " Hz"
Play Stop

Print "-- the queue"
Play BBC Sound 1, -10, 89, 2
For i = 1 To 4 : Play BBC Sound 1, -10, 89 + i * 4, 2 : Next
Print "free on channel 1 with one playing and four waiting: "; MM.Info(BBC BUFFER 1); ", channel 2: "; MM.Info(BBC BUFFER 2); ", emptiest: "; MM.Info(BBC BUFFER)
t = Timer
For i = 1 To 20 : Play BBC Sound 2, -10, 100, 1 : Next
Print "20 notes of 0.05 s on channel 2 queued in "; Str$((Timer - t) / 1000, 0, 2); " s (the last few wait for room)"
Do While MM.Info(SOUND) <> "OFF" And Timer - t < 5000 : Loop
Print "all played by "; Str$((Timer - t) / 1000, 0, 2); " s (21 x 0.05)"

Print "-- an envelope: attack to 126, decay to 60, release"
Play BBC Envelope 1, 1, 0, 0, 0, 0, 0, 0, 20, -5, 0, -10, 126, 60
Play BBC Sound 1, 1, 89, 20
Pause 30 : Print "attack: "; level$();
Pause 150 : Print "  sustain: "; level$();
Pause 1100 : Print "  released: "; level$(); "  "; MM.Info(SOUND)
Play Stop
Play BBC Sound 1, 1, 89, 4 : Pause 100 : Print "the envelope after PLAY STOP: "; level$() : Play Stop

Print "-- noise, PAUSE, errors"
Play BBC Sound 0, -15, 6, 10 : Pause 100 : Print "white noise: "; level$()
Play Pause : Print "paused: "; MM.Info(SOUND) : Play Resume : Print "resumed: "; MM.Info(SOUND)
Play Stop
On Error Skip : Play BBC Sound 1, -16, 89, 10
Print "amplitude -16: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
On Error Skip : Play BBC Envelope 17, 1, 0, 0, 0, 0, 0, 0, 20, -5, 0, -10, 126, 60
Print "envelope 17: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Play Wav "A:/snd/ts_wav.wav"
On Error Skip : Play BBC Sound 1, -15, 89, 10
Print "during a WAV: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Play Stop

Print "-- a note queued just as the engine runs dry"
ok = 0
For i = 0 To 40
  Play BBC Sound 1, -15, 89, 1
  Pause 40 + i * 2
  Play BBC Sound 1, -15, 101, 1
  t = Timer
  Do While MM.Info(SOUND) <> "OFF" And Timer - t < 2000 : Loop
  If MM.Info(SOUND) = "OFF" Then ok = ok + 1
Next
Print ok; " of 41 ended cleanly"
Play Wav "A:/snd/ts_wav.wav" : Pause 200 : Print "a WAV afterwards: "; MM.Info(SOUND) : Play Stop
Print "done"
End

Function level$()
  Local integer lo = 4095, hi = 0, y, k
  For k = 1 To 5000
    y = Peek(WORD DOR1) And &HFFF
    If y < lo Then lo = y
    If y > hi Then hi = y
  Next
  level$ = "DAC " + Str$(lo) + " to " + Str$(hi)
End Function

' the frequency of a square at the DAC: its crossings of 2000 in 500 ms
Function freq()
  Local integer c, h, l, y
  Local float t0
  y = Peek(WORD DOR1) And &HFFF : l = (y > 2000)
  t0 = Timer
  Do While Timer - t0 < 500
    y = Peek(WORD DOR1) And &HFFF : h = (y > 2000)
    If h <> l Then c = c + 1 : l = h
  Loop
  freq = c / 2 / 0.5
End Function
