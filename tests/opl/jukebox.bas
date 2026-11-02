' jukebox.bas: the OPL comparison set (render.bas made it), each with its title. A key skips to the next,
' Q stops. The level of emu8950 and ymfm is matched to Nuked-OPL3's.
n = 20
PLAY STOP
FOR i = 1 TO n
  READ f$, t$
  CLS : PRINT : PRINT "  OPL comparison: piece"; i; " of"; n : PRINT : PRINT "  "; t$ : PRINT
  PRINT i; ": "; t$
  PRINT "  (a key: the next, Q: stop)"
  PLAY WAV "A:/opl/wav/" + f$
  DO WHILE MM.INFO(SOUND) <> "OFF"
    k$ = INKEY$
    IF k$ <> "" THEN PLAY STOP : IF UCASE$(k$) = "Q" THEN END
    PAUSE 50
  LOOP
  PAUSE 800
NEXT
PRINT "  The end"
END
DATA "01_d_e1m1_nuked.wav", "Freedoom E1M1 (MIDI): Nuked-OPL3, 18 voices"
DATA "02_d_e1m1_ymfm.wav", "Freedoom E1M1 (MIDI): ymfm, 18 voices"
DATA "03_d_e1m1_emu8950.wav", "Freedoom E1M1 (MIDI): emu8950, 9 voices (OPL2)"
DATA "04_d_map01_nuked.wav", "Freedoom MAP01 (MIDI): Nuked-OPL3"
DATA "05_d_map01_ymfm.wav", "Freedoom MAP01 (MIDI): ymfm"
DATA "06_d_e2m1_nuked.wav", "Freedoom E2M1 (MIDI): Nuked-OPL3"
DATA "07_grabbag_nuked.wav", "GRABBAG.MID (MIDI): Nuked-OPL3"
DATA "08_wonderin_nuked.wav", "WONDERIN.WLF (Wolfenstein 3D IMF): Nuked-OPL3"
DATA "09_wonderin_ymfm.wav", "WONDERIN.WLF (Wolfenstein 3D IMF): ymfm"
DATA "10_wonderin_emu8950.wav", "WONDERIN.WLF (Wolfenstein 3D IMF): emu8950"
DATA "11_ysbattle_nuked.wav", "YsBattle.vgm (VGM, OPL2): Nuked-OPL3"
DATA "12_ysbattle_ymfm.wav", "YsBattle.vgm (VGM, OPL2): ymfm"
DATA "13_ysbattle_emu8950.wav", "YsBattle.vgm (VGM, OPL2): emu8950"
DATA "14_beyondsn_nuked.wav", "BeyondSN.vgm (VGM, OPL3): Nuked-OPL3"
DATA "15_beyondsn_ymfm.wav", "BeyondSN.vgm (VGM, OPL3): ymfm"
DATA "16_doofus_nuked.wav", "doofus.dro (DOSBox capture, DRO 0.1): Nuked-OPL3"
DATA "17_dro_v2_nuked.wav", "dro_v2.dro (DOSBox capture, DRO 2.0): Nuked-OPL3"
DATA "18_samurai_nuked.wav", "samurai.dro (DRO, two OPL2s): Nuked-OPL3"
DATA "19_mainbgm5_ymfm.wav", "MainBGM5.vgm (VGM, two OPL2s): ymfm"
DATA "20_rrthiel_emu8950.wav", "RRThiel.vgm (VGM, OPL YM3526): emu8950"
