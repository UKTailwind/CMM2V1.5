"""gen_csubtests.py DIR [--timing]   (python tools/gen_csubtests.py tests/cmm2)
Writes the tests of the two CSUB CallTables: CallTablePS in vector slot 7, the PicoMite's soft-float convention, for
a CSUB built for the PicoMite and run here unaltered; CallTableP in slot 8, hard-float, for one built for the CMM2.
  csubabi.bas            tools/csubabi.c built for the PicoMite (Cortex-M0+) and for the CMM2 (Cortex-M7): every
                         float and double slot, each result checked against the interpreter
  chess_ps.bas           the PicoMite's TSCP chess (Bas/chess), its bench: chesslib.bas's CSUB exactly as built for
                         the PicoMite, with the program around it made a console one (no FRAMEBUFFER or sprites)
  chess_m7.bas           the same, with search() converted here by mmb2csub --target cmm2
  julia_ps.bas           mmb2csub's Julia example (mmb2csub/examples/julia.bas), its CSUB as built for the PicoMite
  julia_m7.bas           the same routine converted here by mmb2csub --target cmm2
The node counts, the scores and the picture's checksum must agree between each pair, and with the interpreted
programs (chess takes far longer than a corpus run allows; julia about 26 s). --timing keeps the times in the output,
for benchmarking, and holds the Julia picture for 5 s; without it the output is the same every run, for the .expected
files. Measured 17 Oct: chess bench 31.99 s (PicoMite CSUB) and 31.43 s (CMM2 CSUB), 550,778 nodes; julia 25.95 s
interpreted, 0.986 s (PicoMite CSUB), 0.230 s (CMM2 CSUB), the same picture.
Needs the PicoMite tree (PICOMITE_TREE, default D:/Dropbox/PicoMite/PicoMite) for the programs and for its
user-tools (mmb2csub, armcfgen), and arm-none-eabi-gcc: ARM_TOOLCHAIN_DIR, else the presets' path, else the PATH."""
import json, os, re, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = sys.argv[1]
TIMING = '--timing' in sys.argv
PICO = os.environ.get('PICOMITE_TREE', r'D:\Dropbox\PicoMite\PicoMite')
TOOLS = os.path.join(PICO, 'user-tools')
gcc_dir = os.environ.get('ARM_TOOLCHAIN_DIR') or json.load(open(os.path.join(HERE, '..', 'CMakePresets.json')))[
    'configurePresets'][0]['cacheVariables'].get('ARM_TOOLCHAIN_DIR', '')
ENV = dict(os.environ)
if gcc_dir and os.path.isdir(gcc_dir):
    ENV['PATH'] = gcc_dir + os.pathsep + ENV['PATH']
PS_MARK = "   ' Cortex-M0+ (PicoMite), runs on the CMM2"                 # goldens.py runs a PicoMite CSUB so marked

def read(path):
    return open(path, 'rb').read().decode('latin-1').replace('\r\n', '\n')
def write(name, text):
    open(os.path.join(OUT, name), 'wb').write(text.encode('latin-1'))
    print(name, text.count('\n'), 'lines')
def run(args, cwd):
    r = subprocess.run([sys.executable, '-I', '-B'] + args, cwd=cwd, env=ENV, capture_output=True, text=True)
    if r.returncode:
        sys.exit('failed: %s\n%s%s' % (' '.join(args), r.stdout, r.stderr))
    return r.stdout
def to_cmm2(text, sub):
    """text, a whole program, with sub converted by mmb2csub --target cmm2 in a scratch directory"""
    w = tempfile.mkdtemp()
    open(os.path.join(w, 'p.bas'), 'wb').write(text.encode('latin-1'))
    run([os.path.join(TOOLS, 'mmb2csub.py'), 'p.bas', sub, '--target', 'cmm2', '--no-backup', '--no-original', '--no-c'], w)
    out = read(os.path.join(w, 'p.bas'))
    assert "' Cortex-M7 (CMM2)" in out
    return out
def one(text, a, b):
    assert text.count(a) == 1, a[:60]
    return text.replace(a, b)

# ---- csubabi.bas
def armcfgen(name, cpu):
    o = os.path.join(tempfile.mkdtemp(), name + '.txt')
    run([os.path.join(TOOLS, 'armcfgen.py'), '-c', os.path.join(HERE, 'csubabi.c'), '-I', PICO, '-O', '2', '-n', name,
         '-e', 'main', '-o', o] + (['--cpu', cpu] if cpu else []), HERE)
    t = read(o)
    t = t[t.index('CSUB ' + name.upper()):]
    t = t[:t.index('End CSUB') + len('End CSUB')].split('\n')
    t[0] = 'CSUB %s INTEGER, FLOAT, STRING, STRING' % name + (PS_MARK if not cpu else "   ' Cortex-M7 (CMM2)")
    return '\n'.join(t) + '\n'
ABI = r"""' csubabi.bas - the CallTable's float and double slots in both conventions. abiPS is built for the PicoMite
' (Cortex-M0+, soft-float: it finds CallTablePS through vector slot 7), abiM7 for the CMM2 (Cortex-M7, hard-float:
' CallTableP, slot 8), from the same C (tools/csubabi.c; made by tools/gen_csubtests.py). Each result must be what
' the interpreter gives, and the two CSUBs' results must be the same.
Option Explicit
Dim integer rp(23), rm(23), pass, total, i, cp, cm, cb
Dim float fp(18), fm(18)
Dim string s1p, s2p, s1m, s2m

Sub Check(n$, ok%)
  total = total + 1
  If ok% Then
    pass = pass + 1 : Print "ok   "; n$
  Else
    Print "FAIL "; n$
  EndIf
End Sub

' a single-precision float's bits as its value
Function FromBits(b%) As FLOAT
  Local integer e, m
  Local float v
  e = (b% >> 23) And 255 : m = b% And &H7FFFFF
  If e = 0 Then
    v = m / 2^149
  Else
    v = (1 + m / 2^23) * 2^(e - 127)
  EndIf
  If b% And &H80000000 Then v = -v
  FromBits = v
End Function

Function Near(a!, b!, tol!) As INTEGER
  Near = (Abs(a! - b!) <= tol! * Abs(b!))
End Function

' where the circle drew
Function PicSum() As INTEGER
  Local integer x, y, s
  For y = 50 To 150
    For x = 0 To 200
      If Pixel(x, y) <> 0 Then s = s + x * 1000 + y
    Next
  Next
  PicSum = s
End Function

Sub SetIn(r%(), f!())
  f!(0) = 2.5 : f!(1) = -0.75 : f!(18) = 0.5
  r%(0) = -123456789012 : r%(1) = &H3FC00000 : r%(2) = &HBE800000 : r%(3) = &H400921FB54442D18
End Sub

Sub CheckAll(w$, r%(), f!(), s1$, s2$, c%)
  Check w$ + " FMul", f!(2) = 2.5 * -0.75
  Check w$ + " FAdd", f!(3) = 2.5 + -0.75
  Check w$ + " FSub", f!(4) = 2.5 - -0.75
  Check w$ + " FDiv", f!(5) = 2.5 / -0.75
  Check w$ + " IntToFloat", f!(6) = -123456789012
  Check w$ + " LoadFloat", f!(7) = Pi
  Check w$ + " sin", f!(8) = Sin(2.5)
  Check w$ + " cos", f!(9) = Cos(2.5)
  Check w$ + " sqrt", f!(10) = Sqr(2.5)
  Check w$ + " atan2", Near(f!(11), Pi - Atn(2.5 / 0.75), 1e-14)
  Check w$ + " pow", Near(f!(12), 2.5 ^ -0.75, 1e-14)
  Check w$ + " log", f!(13) = Log(2.5)
  Check w$ + " tan", f!(14) = Tan(2.5)
  Check w$ + " StoD", f!(15) = 1.5
  Check w$ + " TimerVal", Abs(f!(16) - Timer) < 1000
  Check w$ + " RndVal", f!(17) >= 0 And f!(17) < 1
  Check w$ + " FCmp", r%(4) = 1 And r%(5) = -1 And r%(6) = 0
  Check w$ + " FloatToInt64", r%(7) = 3
  Check w$ + " SAdd", r%(8) = &H3FA00000
  Check w$ + " SSub", r%(9) = &H3FE00000
  Check w$ + " SMul", r%(10) = &HBEC00000
  Check w$ + " SDiv", r%(11) = &HC0C00000
  Check w$ + " SCmp", r%(12) = 1 And r%(13) = -1 And r%(14) = 0
  Check w$ + " sinf", Near(FromBits(r%(15)), Sin(1.5), 1e-6)
  Check w$ + " cosf", Near(FromBits(r%(16)), Cos(1.5), 1e-5)
  Check w$ + " sqrtf", Near(FromBits(r%(17)), Sqr(1.5), 1e-6)
  Check w$ + " atan2f", Near(FromBits(r%(18)), Pi - Atn(6), 1e-6)
  Check w$ + " powf", Near(FromBits(r%(19)), 1.5 ^ -0.25, 1e-6)
  Check w$ + " DtoS", r%(20) = &H40200000
  Check w$ + " StoI", r%(21) = 2
  Check w$ + " ItoS", Near(FromBits(r%(22)), -123456789012, 1e-7)
  Check w$ + " FloatToStr", s1$ = Str$(-0.75, 0, 4)
  Check w$ + " StrFormat", s2$ = Str$(2.5, 0, 6)
  Check w$ + " DrawCircle", c% = cb And cb <> 0
End Sub

CLS
Circle 100, 100, 40, 1, 0.5, &HFFFFFF
cb = PicSum()
CLS
SetIn rp(), fp() : abiPS rp(), fp(), s1p, s2p : cp = PicSum()
CLS
SetIn rm(), fm() : abiM7 rm(), fm(), s1m, s2m : cm = PicSum()
CLS
CheckAll "PicoMite", rp(), fp(), s1p, s2p, cp
CheckAll "CMM2", rm(), fm(), s1m, s2m, cm
' the two the same, but for TIMER and RND
For i = 0 To 23
  If rp(i) <> rm(i) Then Print "differ: r("; i; ")"; rp(i); rm(i) : total = total + 1
Next
For i = 0 To 15
  If fp(i) <> fm(i) Then Print "differ: f("; i; ")"; fp(i); fm(i) : total = total + 1
Next
Print "csubabi:"; pass; " of"; total; " passed"
End

"""
write('csubabi.bas', ABI + armcfgen('abiPS', None) + '\n' + armcfgen('abiM7', 'cortex-m7'))

# ---- chess
CHESS = os.path.join(PICO, 'Bas', 'chess')
def drop_block_comments(t):
    out, inside = [], False
    for l in t.split('\n'):
        s = l.strip()
        if not inside and s.startswith('/*'):
            inside = not s.endswith('*/') or s == '/*'
            continue
        if inside:
            if s.endswith('*/'): inside = False
            continue
        out.append(l)
    return '\n'.join(out)
def body(t, head, new):
    i = t.index('\n' + head + '\n') + 1
    j = t.index('\nEnd ', i)
    return t[:i] + head + '\n' + new + t[j:]
BENCH_TIME = '''oPrintR "Time: "+Str$(t)+" ms"
'''
BENCH_NPS = '''If t=0 Then
  oPrintR "(invalid)"
Else
  nps=nodes
  nps=nps*1000.0/t
  ' Score: 1.000 = my Athlon XP 2000+
  oPrint "Nodes per second: "+Str$(Int(nps+0.5))
  oPrintR " (Score: "+Str$(nps/243169.0,0,6)+")"
EndIf
'''
def console(t):
    t = drop_block_comments(t)
    t = re.sub(r'(?m)^LIBRARY LOAD .*\n', '', t)
    t = one(t, 'If Not Instr(MM.Info(lcdpanel),"MODE")) Then\nFRAMEBUFFER create\nFRAMEBUFFER write f\nElse\nMODE 2\nEndIf\n'
               'Colour RGB(white),RGB(black):CLS :Font 8\n', '')
    t = re.sub(r'(?m)^[ \t]*If Not Instr\(MM\.Info\(lcdpanel\),"MODE"\)\) Then FRAMEBUFFER copy f,n[ \t]*\n', '', t)
    assert 'lcdpanel' not in t
    t = body(t, 'Sub load_sprites', '')
    t = body(t, 'Sub oPrint s$', 'Print s$;\n')
    t = body(t, 'Sub oPrintR s$', 'Print s$\n')
    t = body(t, 'Sub draw_chessboard', '')
    t = one(t, "main 'start tscp's main loop\n", 'init_hash\ninit_board\nbench\nPrint "bench done"\n')
    if not TIMING:
        t = one(t, BENCH_TIME, '')
        t = one(t, BENCH_NPS, '')
    return t
hdr = "' %s: made by tools/gen_csubtests.py from the PicoMite's Bas/chess (TSCP 1.81c, MMBasic by Ceptimus)\n"
lib = read(os.path.join(CHESS, 'chesslib.bas'))
i = lib.index("' --- mmb2csub: search"); j = lib.index('End CSUB', i) + len('End CSUB')
a = 'CSUB searchK INTEGER, INTEGER, INTEGER, INTEGER, INTEGER\n'
search = one(lib[i:j], a, a[:-1] + PS_MARK + '\n') + '\n'           # (the mark is a comment: the code is as it was)
ps = console(read(os.path.join(CHESS, 'chesscsub.bas')))
assert '\nFunction search(' not in ps
write('chess_ps.bas', hdr % 'chess_ps.bas' + ps + '\n' + search)
plain = console(read(os.path.join(CHESS, 'chessiter.bas')))
m7 = to_cmm2(plain, 'search')
assert m7.count('\nFunction search(') == 1
write('chess_m7.bas', hdr % 'chess_m7.bas' + m7)

# ---- julia
JULIA = os.path.join(PICO, 'mmb2csub', 'examples')
JEND = 'Save Image "julia.bmp"\nDo\n  a$ = Inkey$\nLoop While a$ = ""\nend\n'
JNEW = ("t = Timer - t\n' the picture's checksum, for comparing the versions pixel for pixel\ns% = 0\n"
        "For y% = 0 To PixelHeight - 1\n  For x% = 0 To PixelWidth - 1\n    s% = (s% * 31 + Pixel(x%, y%)) And &HFFFFFFFFFFFF\n"
        "  Next x%\nNext y%\nPrint \"julia checksum\"; s%\n" +
        # (timed, the picture stays a while: a CSUB draws it before a monitor has settled on the new mode)
        ('Print "julia time"; Int(t); " ms"\nPause 5000\n' if TIMING else '') + 'end\n')
def jcmm2(t):
    t = one(t, 'Mode 2\n', 'Mode 7,8\n')                            # the CMM2's MAP MAXIMITE needs an 8-bit mode
    t = one(t, JEND, JNEW)
    return one(t, 'plotjulia PixelWidth,', 't = Timer\nplotjulia PixelWidth,')
jhdr = "' %s: made by tools/gen_csubtests.py from the PicoMite's mmb2csub/examples/julia.bas\n"
jps = read(os.path.join(JULIA, 'julia.bas'))
i = jps.index('\n/*\n'); j = jps.index('\n*/\n', i)
jps = jps[:i + 1] + jps[j + 4:]                                     # the original routine, in a block comment, goes
a = 'CSUB PlotJulia FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, INTEGER\n'
jps = one(jps, a, a[:-1] + PS_MARK + '\n')
write('julia_ps.bas', jhdr % 'julia_ps.bas' + jcmm2(jps))
write('julia_m7.bas', jhdr % 'julia_m7.bas' + to_cmm2(jcmm2(read(os.path.join(JULIA, 'julia.bas.bak'))), 'plotjulia'))
