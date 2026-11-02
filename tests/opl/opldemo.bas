' opldemo.bas: PLAY OPL on a G2 (dev firmware, 2 Nov 2026). A scale on one voice, four chords on three voices,
' then a BASIC loop timed with the synth on and off (its refills run between statements).
PLAY STOP
PLAY OPL
PLAY OPL WRITE &H105,1, &HBD,0                  ' OPL3 mode, no rhythm
DIM m(2) = (0, 1, 2)                            ' channels 0-2: modulator operators (carrier = +3)
FOR c = 0 TO 2
  PLAY OPL WRITE &H20+m(c),&H21, &H23+m(c),&H21, &H40+m(c),&H18, &H43+m(c),0, &H60+m(c),&HF2, &H63+m(c),&HF2, &H80+m(c),&H07, &H83+m(c),&H07, &HC0+c,&H36
NEXT

PRINT "A scale of C major"
DIM s(7) = (261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25)
FOR i = 0 TO 7
  NoteOn 0, s(i)
  PAUSE 300
  NoteOff 0
  PAUSE 50
NEXT

PRINT "Chords: C, F, G, C"
DIM ch(3, 2) = (261.63, 329.63, 392.00,  349.23, 440.00, 523.25,  392.00, 493.88, 587.33,  261.63, 329.63, 392.00)
FOR i = 0 TO 3
  FOR c = 0 TO 2 : NoteOn c, ch(i, c) : NEXT
  PAUSE 900
  FOR c = 0 TO 2 : NoteOff c : NEXT
  PAUSE 100
NEXT
PAUSE 500

t = TIMER : FOR i = 1 TO 200000 : NEXT : a = TIMER - t
PLAY STOP
t = TIMER : FOR i = 1 TO 200000 : NEXT : b = TIMER - t
PRINT "200000 empty FOR loops: "; a; " ms with PLAY OPL on, "; b; " ms with it off"
PRINT "BASIC keeps"; INT(b / a * 100 + 0.5); "% of its speed while the synth plays"
END

' a note on channel c (0-8) at f Hz: F-number = f * 2^(20 - block) / 49716, the block raised until it fits 10 bits
SUB NoteOn c, f
  LOCAL b = 4, n = INT(f * 65536 / 49716 + 0.5)
  DO WHILE n > 1023 : n = n \ 2 : b = b + 1 : LOOP
  PLAY OPL WRITE &HA0+c, n AND 255, &HB0+c, &H20 OR (b << 2) OR (n >> 8)
END SUB

SUB NoteOff c
  PLAY OPL WRITE &HB0+c, 0
END SUB
