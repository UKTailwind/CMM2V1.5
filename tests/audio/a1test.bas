' a1test.bas - the audio fixes of A1 (docs/Audio_Review_and_Merge_Plan.md section 2). It needs the files of
' tests/audio/gen_audio.py in A:/snd and Peter's audio card (A:/SOUNDS, A:/mp3, A:/fLAC, A:/MUSICS, the root's
' files); tests/audio/README.md
Dim integer fired, n
Dim float t
Dim string s$, first$, last$, f$
Chdir "A:/"

Print "-- 2a TONE and TTS completion interrupts"
fired = 0 : t = Timer
Play Tone 440, 440, 500, done
Do While fired = 0 And Timer - t < 3000 : Loop
Print "tone of 500 ms: "; Choice(fired, Str$(Timer - t, 0, 0) + " ms", "no interrupt")
fired = 0 : t = Timer
Play TTS "hello world",,,,, done
Do While fired = 0 And Timer - t < 6000 : Loop
Print "tts: "; Choice(fired, Str$(Timer - t, 0, 0) + " ms", "no interrupt"); "   sound "; MM.Info(SOUND)

Print "-- 2b, 2e, 2f: PLAY WAV of a directory of 53 (the playlist holds 20), one interrupt at the end"
fired = 0 : n = 0 : first$ = "" : last$ = "" : t = Timer
Play Wav "SOUNDS", done
Do While fired = 0 And Timer - t < 180000
  s$ = MM.Info(TRACK)
  If s$ <> last$ And s$ <> "OFF" Then n = n + 1 : last$ = s$
  If first$ = "" Then first$ = s$
Loop
Print "tracks seen "; n; "  first "; first$; "  last "; last$
Print "interrupts "; fired; " after "; Str$((Timer - t) / 1000, 0, 1); " s";
Pause 1000 : Print ", and 1 s later "; fired

Print "-- the playlist's directory: empty, full path, relative, from the root, current"
On Error Skip : Mkdir "A:/snd/empty"
Chdir "A:/snd"
On Error Skip : Play Wav "empty"
Print "  empty: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error") : Play Stop
Chdir "A:/"
Play Wav "A:/SOUNDS" : Pause 200 : Print "  full path: "; MM.Info(TRACK) : Play Stop
Chdir "A:/MUSICS"
Play Wav "../SOUNDS" : Pause 200 : Print "  relative: "; MM.Info(TRACK) : Play Stop
Play Mp3 "/mp3" : Pause 500 : Print "  from the root: "; MM.Info(TRACK) : Play Stop
Chdir "A:/fLAC"
Play Flac "" : Pause 500 : Print "  current: "; MM.Info(TRACK) : Play Stop
Chdir "A:/"

Print "-- 2f MM.INFO(TRACK) for a single file: ";
Play Wav "A:/Crash.wav" : Pause 100 : Print MM.Info(TRACK) : Play Stop

Print "-- PLAY STOP does not fire the interrupt: ";
fired = 0 : Play Wav "A:/Bombora.wav", done : Pause 300 : Play Stop : Pause 300 : Print fired; " interrupts"
Print "-- a finished WAV's handler does not fire again at a tone's end: ";
fired = 0 : t = Timer : Play Wav "A:/SOUNDS/1up.wav", done
Do While fired = 0 And Timer - t < 10000 : Loop
Play Tone 600, 600, 200 : Pause 600 : Print fired; " interrupts (1 is right)"

Print "-- 2g, 2h PLAY SOUND"
Play Sound 1, B, S, 440
On Error Skip : Play Sound 1, B, X, 440
Print "an invalid type: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error"); "   sound "; MM.Info(SOUND)
Play Pause
On Error Skip : Play Sound 2, B, S, 660
Print "PLAY SOUND while paused: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error"); "   sound "; MM.Info(SOUND)
Play Resume : Pause 300 : Print "resumed: "; MM.Info(SOUND) : Play Stop

Print "-- 2i a failed TTS, then PLAY"
On Error Skip : Play TTS Phonetic "1234"
Print "TTS PHONETIC 1234: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Play Stop
Play Tone 440, 440, 500 : Pause 100 : Print "a tone afterwards: "; MM.Info(SOUND) : Play Stop

Print "-- 2n files it cannot play, then PLAY"
On Error Skip : Play Wav "A:/alley.mod"
Print "WAV of a MOD: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error") : Play Stop
On Error Skip : Play Wav "A:/sr004-01-24192.wav"
Print "WAV at 192 kHz: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error") : Play Stop
On Error Skip : Play Flac "A:/alley.mod"
Print "FLAC of a MOD: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error") : Play Stop
On Error Skip : Play Mp3 "A:/alley.mod"
Print "MP3 of a MOD: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error") : Play Stop
Play Tone 440, 440, 500 : Pause 100 : Print "a tone afterwards: "; MM.Info(SOUND) : Play Stop

Print "-- 96 kHz and 24 bits"
Restore hirate
Do
  Read f$ : If f$ = "" Then Exit Do
  On Error Skip
  If Instr(f$, ".flac") Then Play Flac f$ Else Play Wav f$
  If MM.ErrNo Then Print f$; ": "; MM.ErrMsg$ Else Pause 1500 : Print f$; ": "; MM.Info(SOUND)
  Play Stop
Loop

Print "-- 2k, 2m MOD"
Play Modfile "A:/alley.mod" : Pause 500 : Print "alley.mod: "; MM.Info(SOUND)
On Error Skip : Play Modsample 1, 1, 0
Print "MODSAMPLE at volume 0: "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
Pause 300 : Play Stop
fired = 0 : t = Timer
Play Modfile "A:/alley.mod" : Play Effect "A:/snd/tm44_wav.wav", done
Do While fired = 0 And Timer - t < 10000 : Loop
Print "mono EFFECT of 1 s over MOD: "; Choice(fired, Str$(Timer - t, 0, 0) + " ms to its interrupt", "no interrupt"); "   sound "; MM.Info(SOUND)
Play Effect "A:/snd/ts_wav.wav" : Pause 300 : Print "stereo EFFECT: "; MM.Info(SOUND) : Play Stop
Restore notmod
Do
  Read f$ : If f$ = "" Then Exit Do
  On Error Skip : Play Modfile f$
  Print f$; ": "; Choice(MM.ErrNo, MM.ErrMsg$, MM.Info(SOUND))
  Pause 300 : Play Stop
Loop
Print "modules that must play:"
Restore goodmod
Do
  Read f$ : If f$ = "" Then Exit Do
  On Error Skip : Play Modfile f$
  Print "  "; f$; ": "; Choice(MM.ErrNo, MM.ErrMsg$, MM.Info(SOUND))
  Pause 500 : Play Stop
Loop
Play Tone 440, 440, 500 : Pause 100 : Print "a tone afterwards: "; MM.Info(SOUND) : Play Stop
Print "done"
End

Sub done
  fired = fired + 1
End Sub

hirate:
Data "A:/F96.flac", "A:/Sample_BeeMoved_96kHz24bit.flac", "A:/sr005-01-2496.wav", "A:/48.wav", ""
notmod:
Data "A:/Crash.wav", "A:/TESTIT.MOD", "A:/test.flac", ""
goodmod:
Data "A:/PTETRIS.MOD", "A:/RISsound.mod", "A:/song.mod", "A:/stksll2.mod", "A:/the_wanderer_2.mod"
Data "A:/pseudolukian_-_touch.mod", "A:/TESTMOD.MOD", "A:/music/get_psyched-sfx.mod"
Data "A:/MUSICS/alieni_vol_1.mod", "A:/MUSICS/alone.mod", "A:/MUSICS/draclev1.mod", "A:/MUSICS/envie.mod"
Data "A:/MUSICS/jaktar-dungeons-1.mod", "A:/MUSICS/mahoney-questioff.mod", "A:/MUSICS/pirate_army_of_darkness.mod"
Data "A:/MUSICS/quest.mod", "A:/MUSICS/viking_battle.mod", ""
