' sndinfo.bas - MM.INFO(SOUND)'s strings without OPTION PICOMITE (1 Nov 2026): the CMM2's, as 6.00 gave them, "MODFILE"
' and a bare "PAUSED" for any pause (Wolf3D, Gauntlet and Into the Darkness test for "MODFILE" before an EFFECT). A
' tone needs no file; under OPTION PICOMITE it is "PAUSED TONE" (tests/picomite/pmdialect).
Print "["; MM.Info$(Sound); "]"
Play Tone 440, 440, 2000 : Pause 50
Print "["; MM.Info$(Sound); "]"
Play Pause
Print "["; MM.Info$(Sound); "]"
Play Resume
Print "["; MM.Info(Sound); "]"
Play Stop
Print "["; MM.Info(Sound); "]"
