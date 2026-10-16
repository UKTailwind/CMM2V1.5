' levels.bas - what each sound source sends to the DACs (DOR1 and DOR2 read while it plays), and the PicoMite's PAUSE,
' paused TONE and MM.INFO(SOUND) (docs/Audio_Review_and_Merge_Plan.md section 8). The tones from gen_audio.py are at
' half scale, so WAV, FLAC and MP3 should swing about +-950 about 2000 at VOLUME 100; TONE +-1900; idle 2000.
Dim integer mn, mx, v, i, n
Dim float tot
Dim string f$, k$
Const DOR1 = &H4000742C, DOR2 = &H40007430

Print "-- levels: source, channel 1 min/mean/max, channel 2 min/mean/max"
measure "idle"
Restore sources
Do
  Read f$, k$ : If f$ = "" Then Exit Do
  Select Case k$
    Case "WAV" : Play Wav f$
    Case "FLAC" : Play Flac f$
    Case "MP3" : Play Mp3 f$
  End Select
  Pause 300 : measure f$ : Play Stop
Loop
Play Tone 440, 440 : Pause 100 : measure "TONE 440" : Play Stop
Play Sound 1, B, S, 440 : Pause 100 : measure "SOUND 1 sine" : Play Stop
Play Volume 50, 50
Play Wav "A:/snd/ts_wav.wav" : Pause 300 : measure "ts_wav.wav at VOLUME 50" : Play Stop
Play TTS "hello world, this is a test of the speech output" : Pause 300 : measure "TTS at VOLUME 50" : Play Stop
Play Volume 100, 100
Play TTS "hello world, this is a test of the speech output" : Pause 300 : measure "TTS" : Play Stop
Play Modfile "A:/PTETRIS.MOD" : Pause 500 : measure "PTETRIS.MOD" : Play Stop
measure "idle after"

Print "-- PAUSE, PAUSE again, TONE while paused (the PicoMite's)"
Play Tone 440, 440 : Pause 100
Play Pause : Print "paused: "; MM.Info(SOUND)
On Error Skip : Play Pause
Print "PAUSE again: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error"); "   "; MM.Info(SOUND)
Play Tone 880, 880 : Pause 100
Print "PLAY TONE while paused: "; MM.Info(SOUND)
Play Resume : Pause 100 : Print "resumed: "; MM.Info(SOUND)
Play Stop
On Error Skip : Play Pause
Print "PAUSE with nothing playing: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")

Print "-- MM.INFO(SOUND)"
Play Wav "A:/snd/ts_wav.wav" : Pause 100 : Print MM.Info(SOUND); " ";
Play Pause : Print MM.Info(SOUND); " " : Play Stop
Play Flac "A:/snd/ts_flac.flac" : Pause 100 : Print MM.Info(SOUND); " ";
Play Pause : Print MM.Info(SOUND); " " : Play Stop
Play Mp3 "A:/snd/ts_mp3.mp3" : Pause 100 : Print MM.Info(SOUND); " ";
Play Pause : Print MM.Info(SOUND); " " : Play Stop
Play Modfile "A:/PTETRIS.MOD" : Pause 100 : Print MM.Info(SOUND); " ";
Play Pause : Print MM.Info(SOUND); " " : Play Stop
Play Sound 1, B, S, 440 : Pause 100 : Print MM.Info(SOUND); " ";
Play Pause : Print MM.Info(SOUND); " " : Play Stop
Play TTS "hello" : Pause 100 : Print MM.Info(SOUND); " ";
Play Stop : Print MM.Info(SOUND)
Play Tone 440, 440, 100 : Pause 300 : Print "a tone after its end: "; MM.Info(SOUND)
Print "done"
End

Sub measure s$
  Local integer a1 = 4095, b1 = 0, a2 = 4095, b2 = 0, x, y, j
  Local float t1, t2
  For j = 1 To 20000
    x = Peek(WORD DOR1) And &HFFF : y = Peek(WORD DOR2) And &HFFF
    t1 = t1 + x : t2 = t2 + y
    If x < a1 Then a1 = x
    If x > b1 Then b1 = x
    If y < a2 Then a2 = y
    If y > b2 Then b2 = y
  Next
  Print s$; ": "; a1; Str$(t1 / 20000, 5, 0); b1; "   "; a2; Str$(t2 / 20000, 5, 0); b2
End Sub

sources:
Data "A:/snd/ts_wav.wav", "WAV", "A:/snd/tm_wav.wav", "WAV", "A:/snd/ts_flac.flac", "FLAC", "A:/snd/tm_flac.flac", "FLAC"
Data "A:/snd/ts_mp3.mp3", "MP3", "A:/snd/tm_mp3.mp3", "MP3", "", ""
