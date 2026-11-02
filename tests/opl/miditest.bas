' miditest.bas: PLAY MIDI, PLAY MIDIFILE and PLAY OPL file$ on a G2 (dev firmware, 2 Nov 2026). About 75 s of sound;
' each step says what it plays. Needs A:/opl (d_intro.mid, d_e1m1.mid, WONDERIN.WLF, YsBattle.vgm, samurai.dro).
CLS
PRINT "1. PLAY MIDI TEST: the tune kept in flash (Freedoom's FreeDM title, 11 s)"
PLAY MIDI TEST
PAUSE 300 : PRINT "   MM.INFO(SOUND) is "; MM.INFO(SOUND); ", MM.INFO(TRACK) "; MM.INFO(TRACK)
DO WHILE MM.INFO(SOUND) <> "OFF" : LOOP

PRINT "2. PLAY MIDIFILE with an interrupt at its end (Freedoom's intro, 7 s)"
done = 0
PLAY MIDIFILE "A:/opl/d_intro.mid", TuneEnded
DO WHILE done = 0 : LOOP
PRINT "   its interrupt ran; MM.INFO(SOUND) is "; MM.INFO(SOUND)

PRINT "3. PLAY MIDIFILE: Freedoom E1M1 for 12 s, then PLAY STOP"
PLAY MIDIFILE "A:/opl/d_e1m1.mid" : PAUSE 12000 : PLAY STOP

PRINT "4. PLAY OPL file$: the Wolfenstein 3D IMF, 10 s"
PLAY OPL "A:/opl/WONDERIN.WLF" : PAUSE 300 : PRINT "   MM.INFO(SOUND) is "; MM.INFO(SOUND)
PAUSE 9700 : PLAY STOP

PRINT "5. PLAY OPL file$: YsBattle.vgm, paused for 2 s after 5 s"
PLAY OPL "A:/opl/YsBattle.vgm" : PAUSE 5000
PLAY PAUSE : PRINT "   MM.INFO(SOUND) is "; MM.INFO(SOUND) : PAUSE 2000
PLAY RESUME : PAUSE 5000 : PLAY STOP

PRINT "6. PLAY OPL file$: samurai.dro, two OPL2 chips, 8 s"
PLAY OPL "A:/opl/samurai.dro" : PAUSE 8000 : PLAY STOP

PRINT "7. PLAY MIDI in real time: a scale on the piano, the same on an organ (program 19), then drums"
PLAY MIDI
PRINT "   MM.INFO(SOUND) is "; MM.INFO(SOUND)
FOR n = 60 TO 72 : PLAY NOTE ON 0, n, 100 : PAUSE 150 : PLAY NOTE OFF 0, n : NEXT
PLAY MIDI CMD &HC0, 19
FOR n = 60 TO 72 : PLAY NOTE ON 0, n, 100 : PAUSE 150 : PLAY NOTE OFF 0, n : NEXT
FOR i = 1 TO 8
  PLAY NOTE ON 9, 36, 110 : PAUSE 120 : PLAY NOTE ON 9, 42, 80 : PAUSE 120
  PLAY NOTE ON 9, 38, 110 : PAUSE 120 : PLAY NOTE ON 9, 42, 80 : PAUSE 120
NEXT
PAUSE 1000 : PLAY STOP
PRINT "Done"
END

SUB TuneEnded
  done = 1
END SUB
