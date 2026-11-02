' render2.bas: the OPL comparison set, 30 s each, into A:/opl/wav by oplplay.app (made by make_opl_bas.py)
ON ERROR SKIP : MKDIR "A:/opl/wav"
t = TIMER
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
PRINT "all rendered in"; INT((TIMER - t) / 1000); " s"
