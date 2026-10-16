' a5test.bas - PLAY MODFILE played once with an interrupt (hxcmod's noloop, the PicoMite's), and MM.INFO(TRACK) for
' MOD (docs/Audio_Review_and_Merge_Plan.md A5). Needs A:/snd/tiny.mod (2.56 s) and A:/snd/ts_wav.wav from gen_audio.py,
' and A:/PTETRIS.MOD.
Dim integer fired, efired, r
Dim float t, tm, te
Dim string f$ = "A:/snd/tiny.mod"
Const DOR1 = &H4000742C

Print "-- played once: tiny.mod is 2.56 s"
fired = 0 : t = Timer
Play Modfile f$, done
Pause 200 : Print "playing: "; MM.Info(SOUND); "  "; MM.Info(TRACK)
Do While fired = 0 And Timer - t < 8000 : Loop
Print "PLAY MODFILE f$, sub (the PicoMite's): "; Choice(fired, Str$((Timer - t) / 1000, 0, 2) + " s", "no interrupt"); "  then "; MM.Info(SOUND); "  DAC "; Peek(WORD DOR1) And &HFFF
fired = 0 : t = Timer
Play Modfile f$, 22050, done
Do While fired = 0 And Timer - t < 8000 : Loop
Print "PLAY MODFILE f$, 22050, sub: "; Choice(fired, Str$((Timer - t) / 1000, 0, 2) + " s", "no interrupt"); "  then "; MM.Info(SOUND)
fired = 0 : t = Timer
Play Modfile f$, , done
Do While fired = 0 And Timer - t < 8000 : Loop
Print "PLAY MODFILE f$, , sub: "; Choice(fired, Str$((Timer - t) / 1000, 0, 2) + " s", "no interrupt")
Pause 500 : Print "interrupts 0.5 s later: "; fired

Print "-- looped as before without an interrupt"
r = 22050
Play Modfile f$, r : Pause 4000 : Print "a rate in a variable, after 4 s: "; MM.Info(SOUND) : Play Stop
Play Modfile f$ : Pause 4000 : Print "no rate, after 4 s: "; MM.Info(SOUND) : Play Stop

Print "-- a real module, stopped"
fired = 0
Play Modfile "A:/PTETRIS.MOD", done : Pause 1000 : Print MM.Info(TRACK); "  "; MM.Info(SOUND)
Play Pause : Print "paused: "; MM.Info(TRACK); "  "; MM.Info(SOUND) : Play Resume
Play Stop : Pause 300 : Print "PLAY STOP fired "; fired; "  "; MM.Info(TRACK)

Print "-- EFFECT over a module played once (each has its own interrupt), then a WAV"
fired = 0 : t = Timer
Play Modfile f$, done : Play Effect "A:/snd/ts_wav.wav"
Do While fired = 0 And Timer - t < 8000 : Loop
Print "module, EFFECT without an interrupt: "; Choice(fired, Str$((Timer - t) / 1000, 0, 2) + " s", "no interrupt"); "  then "; MM.Info(SOUND)
fired = 0 : efired = 0 : tm = 0 : te = 0 : t = Timer
Play Modfile f$, done : Play Effect "A:/snd/ts_wav.wav", edone
Do While (fired = 0 Or efired = 0) And Timer - t < 8000 : Loop
Pause 300
Print "module "; fired; " at "; Str$(tm / 1000, 0, 2); " s, EFFECT of 2 s "; efired; " at "; Str$(te / 1000, 0, 2); " s"
On Error Skip : Play Modfile f$, 12345
Print "rate 12345: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Play Wav "A:/snd/ts_wav.wav" : Pause 200 : Print "a WAV afterwards: "; MM.Info(SOUND) : Play Stop
Print "done"
End

Sub done
  fired = fired + 1 : tm = Timer - t
End Sub

Sub edone
  efired = efired + 1 : te = Timer - t
End Sub
