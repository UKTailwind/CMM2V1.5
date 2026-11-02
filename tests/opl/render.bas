' render.bas: the OPL comparison set, 30 s each, into A:/opl/wav by oplplay.app (made by make_opl_bas.py)
ON ERROR SKIP : MKDIR "A:/opl/wav"
t = TIMER
PRINT "1 of 20: Freedoom E1M1 (MIDI): Nuked-OPL3, 18 voices"
RUN "A:/apps/oplplay.app", A:/opl/d_e1m1.mid A:/opl/wav/01_d_e1m1_nuked.wav nuked 30
PRINT "2 of 20: Freedoom E1M1 (MIDI): ymfm, 18 voices"
RUN "A:/apps/oplplay.app", A:/opl/d_e1m1.mid A:/opl/wav/02_d_e1m1_ymfm.wav ymfm 30
PRINT "3 of 20: Freedoom E1M1 (MIDI): emu8950, 9 voices (OPL2)"
RUN "A:/apps/oplplay.app", A:/opl/d_e1m1.mid A:/opl/wav/03_d_e1m1_emu8950.wav emu8950 30
PRINT "4 of 20: Freedoom MAP01 (MIDI): Nuked-OPL3"
RUN "A:/apps/oplplay.app", A:/opl/d_map01.mid A:/opl/wav/04_d_map01_nuked.wav nuked 30
PRINT "5 of 20: Freedoom MAP01 (MIDI): ymfm"
RUN "A:/apps/oplplay.app", A:/opl/d_map01.mid A:/opl/wav/05_d_map01_ymfm.wav ymfm 30
PRINT "6 of 20: Freedoom E2M1 (MIDI): Nuked-OPL3"
RUN "A:/apps/oplplay.app", A:/opl/d_e2m1.mid A:/opl/wav/06_d_e2m1_nuked.wav nuked 30
PRINT "7 of 20: GRABBAG.MID (MIDI): Nuked-OPL3"
RUN "A:/apps/oplplay.app", A:/opl/GRABBAG.MID A:/opl/wav/07_grabbag_nuked.wav nuked 30
PRINT "8 of 20: WONDERIN.WLF (Wolfenstein 3D IMF): Nuked-OPL3"
RUN "A:/apps/oplplay.app", A:/opl/WONDERIN.WLF A:/opl/wav/08_wonderin_nuked.wav nuked 30
PRINT "9 of 20: WONDERIN.WLF (Wolfenstein 3D IMF): ymfm"
RUN "A:/apps/oplplay.app", A:/opl/WONDERIN.WLF A:/opl/wav/09_wonderin_ymfm.wav ymfm 30
PRINT "10 of 20: WONDERIN.WLF (Wolfenstein 3D IMF): emu8950"
RUN "A:/apps/oplplay.app", A:/opl/WONDERIN.WLF A:/opl/wav/10_wonderin_emu8950.wav emu8950 30
PRINT "11 of 20: YsBattle.vgm (VGM, OPL2): Nuked-OPL3"
RUN "A:/apps/oplplay.app", A:/opl/YsBattle.vgm A:/opl/wav/11_ysbattle_nuked.wav nuked 30
PRINT "12 of 20: YsBattle.vgm (VGM, OPL2): ymfm"
RUN "A:/apps/oplplay.app", A:/opl/YsBattle.vgm A:/opl/wav/12_ysbattle_ymfm.wav ymfm 30
PRINT "13 of 20: YsBattle.vgm (VGM, OPL2): emu8950"
RUN "A:/apps/oplplay.app", A:/opl/YsBattle.vgm A:/opl/wav/13_ysbattle_emu8950.wav emu8950 30
PRINT "14 of 20: BeyondSN.vgm (VGM, OPL3): Nuked-OPL3"
RUN "A:/apps/oplplay.app", A:/opl/BeyondSN.vgm A:/opl/wav/14_beyondsn_nuked.wav nuked 30
PRINT "15 of 20: BeyondSN.vgm (VGM, OPL3): ymfm"
RUN "A:/apps/oplplay.app", A:/opl/BeyondSN.vgm A:/opl/wav/15_beyondsn_ymfm.wav ymfm 30
PRINT "16 of 20: doofus.dro (DOSBox capture, DRO 0.1): Nuked-OPL3"
RUN "A:/apps/oplplay.app", A:/opl/doofus.dro A:/opl/wav/16_doofus_nuked.wav nuked 30
PRINT "17 of 20: dro_v2.dro (DOSBox capture, DRO 2.0): Nuked-OPL3"
RUN "A:/apps/oplplay.app", A:/opl/dro_v2.dro A:/opl/wav/17_dro_v2_nuked.wav nuked 30
PRINT "18 of 20: samurai.dro (DRO, two OPL2s): Nuked-OPL3"
RUN "A:/apps/oplplay.app", A:/opl/samurai.dro A:/opl/wav/18_samurai_nuked.wav nuked 30
PRINT "19 of 20: MainBGM5.vgm (VGM, two OPL2s): ymfm"
RUN "A:/apps/oplplay.app", A:/opl/MainBGM5.vgm A:/opl/wav/19_mainbgm5_ymfm.wav ymfm 30
PRINT "20 of 20: RRThiel.vgm (VGM, OPL YM3526): emu8950"
RUN "A:/apps/oplplay.app", A:/opl/RRThiel.vgm A:/opl/wav/20_rrthiel_emu8950.wav emu8950 30
PRINT "all rendered in"; INT((TIMER - t) / 1000); " s"
