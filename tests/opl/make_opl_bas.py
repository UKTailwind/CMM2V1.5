"""make_opl_bas.py OUTDIR: writes render.bas and jukebox.bas (the OPL comparison set) into OUTDIR."""
import os, sys

SECS = 30
SET = [  # file, emulator, title
    ('d_e1m1.mid', 'nuked', 'Freedoom E1M1 (MIDI): Nuked-OPL3, 18 voices'),
    ('d_e1m1.mid', 'ymfm', 'Freedoom E1M1 (MIDI): ymfm, 18 voices'),
    ('d_e1m1.mid', 'emu8950', 'Freedoom E1M1 (MIDI): emu8950, 9 voices (OPL2)'),
    ('d_map01.mid', 'nuked', 'Freedoom MAP01 (MIDI): Nuked-OPL3'),
    ('d_map01.mid', 'ymfm', 'Freedoom MAP01 (MIDI): ymfm'),
    ('d_e2m1.mid', 'nuked', 'Freedoom E2M1 (MIDI): Nuked-OPL3'),
    ('GRABBAG.MID', 'nuked', 'GRABBAG.MID (MIDI): Nuked-OPL3'),
    ('WONDERIN.WLF', 'nuked', 'WONDERIN.WLF (Wolfenstein 3D IMF): Nuked-OPL3'),
    ('WONDERIN.WLF', 'ymfm', 'WONDERIN.WLF (Wolfenstein 3D IMF): ymfm'),
    ('WONDERIN.WLF', 'emu8950', 'WONDERIN.WLF (Wolfenstein 3D IMF): emu8950'),
    ('YsBattle.vgm', 'nuked', 'YsBattle.vgm (VGM, OPL2): Nuked-OPL3'),
    ('YsBattle.vgm', 'ymfm', 'YsBattle.vgm (VGM, OPL2): ymfm'),
    ('YsBattle.vgm', 'emu8950', 'YsBattle.vgm (VGM, OPL2): emu8950'),
    ('BeyondSN.vgm', 'nuked', 'BeyondSN.vgm (VGM, OPL3): Nuked-OPL3'),
    ('BeyondSN.vgm', 'ymfm', 'BeyondSN.vgm (VGM, OPL3): ymfm'),
    ('doofus.dro', 'nuked', 'doofus.dro (DOSBox capture, DRO 0.1): Nuked-OPL3'),
    ('dro_v2.dro', 'nuked', 'dro_v2.dro (DOSBox capture, DRO 2.0): Nuked-OPL3'),
    ('samurai.dro', 'nuked', 'samurai.dro (DRO, two OPL2s): Nuked-OPL3'),
    ('MainBGM5.vgm', 'ymfm', 'MainBGM5.vgm (VGM, two OPL2s): ymfm'),
    ('RRThiel.vgm', 'emu8950', 'RRThiel.vgm (VGM, OPL YM3526): emu8950'),
]


def wavname(i, f, e):
    return '%02d_%s_%s.wav' % (i + 1, f.rsplit('.', 1)[0].lower(), e)


out = sys.argv[1]
ALL = list(range(len(SET)))


def render(name, pick):
  with open(os.path.join(out, name), 'w', newline='\r\n') as b:
    b.write("' %s: the OPL comparison set, %d s each, into A:/opl/wav by oplplay.app (made by make_opl_bas.py)\n" % (name, SECS))
    b.write('ON ERROR SKIP : MKDIR "A:/opl/wav"\n')
    b.write('t = TIMER\n')
    for i in pick:
        f, e, title = SET[i]
        b.write('PRINT "%d of %d: %s"\n' % (i + 1, len(SET), title))
        b.write('RUN "A:/apps/oplplay.app", A:/opl/%s A:/opl/wav/%s %s %d\n' % (f, wavname(i, f, e), e, SECS))
    b.write('PRINT "all rendered in"; INT((TIMER - t) / 1000); " s"\n')


def jukebox(name, pick):
  with open(os.path.join(out, name), 'w', newline='\r\n') as b:
    b.write("' %s: the OPL comparison set (render.bas made it), each with its title. A key skips to the next,\n" % name)
    b.write("' Q stops. The level of emu8950 and ymfm is matched to Nuked-OPL3's.\n")
    b.write('n = %d\n' % len(pick))
    b.write('PLAY STOP\n')
    b.write('FOR i = 1 TO n\n')
    b.write('  READ f$, t$\n')
    b.write('  CLS : PRINT : PRINT "  OPL comparison: piece"; i; " of"; n : PRINT : PRINT "  "; t$ : PRINT\n')
    b.write('  PRINT i; ": "; t$\n')
    b.write('  PRINT "  (a key: the next, Q: stop)"\n')
    b.write('  PLAY WAV "A:/opl/wav/" + f$\n')
    b.write('  DO WHILE MM.INFO(SOUND) <> "OFF"\n')
    b.write('    k$ = INKEY$\n')
    b.write('    IF k$ <> "" THEN PLAY STOP : IF UCASE$(k$) = "Q" THEN END\n')
    b.write('    PAUSE 50\n')
    b.write('  LOOP\n')
    b.write('  PAUSE 800\n')
    b.write('NEXT\n')
    b.write('PRINT "  The end"\n')
    b.write('END\n')
    for i in pick:
        f, e, title = SET[i]
        b.write('DATA "%s", "%s"\n' % (wavname(i, f, e), title))


render('render.bas', ALL)
jukebox('jukebox.bas', ALL)
GOOD = [0, 1, 2, 3, 4, 5, 6, 16, 17, 18, 19]                # (the first run: 8-16 were cut short by Ctrl-C)
REDO = [7, 8, 9, 10, 11, 12, 13, 14, 15]
jukebox('jukebox1.bas', GOOD)
render('render2.bas', REDO)
jukebox('jukebox2.bas', REDO)
print('wrote render.bas, jukebox.bas, jukebox1.bas, render2.bas, jukebox2.bas')
