"""make.py: builds the C compiler for the CMM2 (Peter, 4 Nov 2026; the plan is docs/C_Compiler_Plan.md): apps/cc/out/
cc.app, the compiler, and bcrun.app, its runtime. The compiler is the Fuzix Compiler Kit with the Pico Computer 3's
bytecode and Thumb-2 target, from Peter's Fuzix fork: its branch cmm2 adds the CMM2's platform sections (bcrun.c and
ccbc.c under -DCMM2, and Applications/CC/cmm2). The fork is fetched the first time into apps/cc/fuzix (git ignores
it), sparse and shallow, at the commit below; a working copy there that is somewhere else is built as it is, with a
note.

cc.app is the driver, ccbc.c, with the passes linked in, as a loadable program cannot start another: cpp (Fuzix's
preprocessor), cc0, cc1 and cc2, each compiled as it is for any machine, linked on its own (ld -r), its main and its
calls of exit renamed (cpp_main..., cmm2_pass_exit) and everything else in it made local, so that the four never
meet. The driver calls each where Fuzix forks and execs it (cmm2/ccbc_cmm2.c). The passes are GPLv3, so cc.app is.

bcrun.app loads a compiled program (prog.bc), relocates it and runs it: its native Thumb-2 functions directly, the rest
as bytecode. The runtime is not under the compiler's GPLv3 (the kit's README); fnum.c, printf's %e %f %g, is from
Fuzix's C library. The firmware carries it with the BASIC runtime, as mmbrun, the one runtime for C and translated
BASIC programs alike (Peter, 4 Nov 2026: so that a compiled program needs nothing on the card), in bank 2's sector 2
after the options, and runs it there in place: built --xip (tools/sdk/build.py) as out/mmbrunx.bin, the flash's bytes at
cmm2img.h's CMM2XIP_FLASH; Src/MmbrunImage.c holds them, and this writes it whenever it builds mmbrun: the firmware is
then built again with it, and the two are committed together. out/bcrun.app and out/mmbrun.app, the same as loadable
programs, are for comparison runs from the card (RUN "mmbrun.app", "prog.bc").

    python apps/cc/make.py [cc|bcrun|mmbrun]
then put out/sd/cc on the SD card as A:/cc (cc.app, and the headers in include/), and at the CMM2's prompt:
CC "prog.c" and RUN "prog.bc" [, args], or RUN "prog.c" [, args] for both
"""
import concurrent.futures, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SDK = os.path.join(REPO, 'tools', 'sdk')
SRC = os.path.join(HERE, 'fuzix')
CC = os.path.join(SRC, 'Applications', 'CC')
CPP = os.path.join(SRC, 'Applications', 'cpp')
OUT = os.path.join(HERE, 'out')
URL = 'https://github.com/UKTailwind/FUZIX'
COMMIT = '552517b0a7c049d1277b0c0d4b5df9e05db13036'   # branch cmm2, 10 Nov 2026: a FOR ... NEXT in a single-line IF
                                                       # (FUZIX #44), MODE n the firmware's; from pc3's 6d662a677
SPARSE = ['Applications/CC', 'Applications/cpp', 'Applications/mmb2c', 'Library/libs/fnum.c']

# the passes: their sources (Makefile.common and Makefile.host's lists) and flags (the host build's: tables as big as
# a PC's, which the CMM2's memory allows)
PASSFLAGS = ['-DCPU_armm0', '-DNO_FLOAT', '-DBIG_TABLES', '-include', 'unistd.h', '-I', CC]
C1 = '''body.c declaration.c enum.c error.c expression.c header.c idxdata.c initializer.c label.c lex.c main.c primary.c
        stackframe.c storage.c struct.c switch.c symbol.c tree.c type.c type_iterator.c target-thumb.c'''.split()
MMBC = os.path.join(SRC, 'Applications', 'mmb2c', 'mmbc')
PASSES = [('cpp', CPP, 'cpp.c hash.c main.c token1.c token2.c'.split(), ['-I', CPP]),
          ('mmbc', MMBC, ('mmbc_main.c mmbc_lex.c mmbc_util.c mmbc_tab.c mmbc_sym.c mmbc_expr.c mmbc_builtin.c '
                          'mmbc_decl.c mmbc_walk.c mmbc_stmt.c mmbc_type.c mmbc_out.c mmbc_font.c').split(),
           ['-I', MMBC, '-DMMBC_ARENA', '-DMMBC_ARENA_MALLOC']),
          ('cc0', CC, ['frontend.c'], PASSFLAGS),
          ('cc1', CC, C1, PASSFLAGS),
          ('cc2', CC, ['backend.c', 'backend-bcode.c'], PASSFLAGS + ['-DTHUMB_VFP', '-DCMM2_TARGET', '-DARENA_TABLES', '-DARENA_MALLOC'])]   # (doubles inline in the
                                                                    # FPU, its tables on the heap: under 5 MB on a G1)


def git(*args, **kw):
    return subprocess.run(['git', '-C', SRC] + list(args), check=True, **kw)


def fetch():
    """the fork's files the build needs, at COMMIT: only those, and only that commit"""
    os.makedirs(SRC)
    git('init', '-q')
    git('remote', 'add', 'origin', URL)
    git('sparse-checkout', 'set', '--no-cone', *SPARSE)
    git('fetch', '-q', '--depth', '1', '--filter=blob:none', 'origin', COMMIT)
    git('checkout', '-q', '-b', 'cmm2', 'FETCH_HEAD')


def sdk(args):
    r = subprocess.run([sys.executable, '-I', os.path.join(SDK, 'build.py')] + args)
    if r.returncode:
        sys.exit(r.returncode)


def bcrun(extra, mm=False, xip=False):
    """bcrun.app, or with mm mmbrun.app: bcrun with the BASIC runtime (Applications/mmb2c's, its /dev/sys answered by
    cmm2/cmm2_sys.c), which runs what the translator makes and C alike; with xip (mm) mmbrunx.bin, to run in place
    from the firmware's flash (CMM2XIP_FLASH, xip.ld's default)"""
    srcs = [os.path.join(CC, 'bcrun.c'), os.path.join(CC, 'cmm2', 'bcrun_cmm2.c'),
            os.path.join(SRC, 'Library', 'libs', 'fnum.c')]
    flags = ['--cflag=-DCMM2']
    if mm:
        srcs.append(os.path.join(CC, 'cmm2', 'cmm2_sys.c'))
        flags += ['--cflag=-DCMM2_MM', '-I', os.path.join(SRC, 'Applications', 'mmb2c')]
    out = ('mmbrun' if mm else 'bcrun') + ('x.bin' if xip else '.app')
    sdk([os.path.join(OUT, out)] + srcs + (['--xip'] if xip else []) +
        ['--no-printf-float'] +                                     # (fnum.c's %f)
        ['--stack', '0x20000', '-I', CC, '-I', os.path.join(CC, 'cmm2')] + flags +
        ['--cflag=' + a for a in extra])                            # (the program's own stack is in its memory, not here)


def passobj(name, folder, files, flags, b):
    """one pass as one object, out/cc.passes/NAME.o, whose only global is NAME_main"""
    objdir = os.path.join(OUT, 'cc.passes', name)
    os.makedirs(objdir, exist_ok=True)
    jobs = [[b.GCC] + b.CFLAGS + flags + ['-w', '-c', os.path.join(folder, f),
                                          '-o', os.path.join(objdir, f[:-2] + '.o')] for f in files]
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 4) as ex:
        for cmd, r in zip(jobs, ex.map(lambda c: subprocess.run(c, capture_output=True, text=True), jobs)):
            if r.returncode:
                sys.exit(r.stdout + r.stderr)
    tools = os.path.dirname(b.GCC)
    whole = os.path.join(objdir, 'whole.o')
    subprocess.run([os.path.join(tools, 'arm-none-eabi-ld' + b.EXE), '-r', '-o', whole] +
                   [os.path.join(objdir, f[:-2] + '.o') for f in files], check=True)
    out = os.path.join(OUT, 'cc.passes', name + '.o')
    subprocess.run([os.path.join(tools, 'arm-none-eabi-objcopy' + b.EXE), '--redefine-sym', 'main=%s_main' % name,
                    '--redefine-sym', 'exit=cmm2_pass_exit', '-G', '%s_main' % name, whole, out], check=True)
    return out


def cc(extra):
    sys.path.insert(0, SDK)
    import build as b
    objs = [passobj(name, folder, files, flags, b) for name, folder, files, flags in PASSES]
    sdk([os.path.join(OUT, 'cc.app'), os.path.join(CC, 'ccbc.c'), os.path.join(CC, 'cmm2', 'ccbc_cmm2.c')] + objs +
        ['--stack', '0x10000', '-I', CC, '-I', os.path.join(CC, 'cmm2'), '--cflag=-DCMM2'] +
        ['--cflag=' + a for a in extra])                            # (the passes' recursion: cc1 32K on the PC3)


def main():
    if not os.path.isdir(SRC):
        fetch()
    head = git('rev-parse', 'HEAD', capture_output=True, text=True).stdout.strip()
    dirty = git('status', '--porcelain', capture_output=True, text=True).stdout.strip()
    if head != COMMIT or dirty:
        print('note: building the working copy in %s (%s%s), not %s' %
              (SRC, head[:9], ', changed' if dirty else '', COMMIT[:9]))
    os.makedirs(OUT, exist_ok=True)
    what = [a for a in sys.argv[1:] if not a.startswith('-')]
    extra = [a for a in sys.argv[1:] if a.startswith('-')]     # (flags for the compiler: -O3 and the like)
    if not what or 'bcrun' in what:
        bcrun(extra)                                                # (out/bcrun.app: for comparison runs)
    if not what or 'mmbrun' in what:
        bcrun(extra, mm=True)
        bcrun(extra, mm=True, xip=True)
        image(head, dirty)
    if not what or 'cc' in what:
        cc(extra)
    card()


def image(head, dirty, binname='mmbrunx.bin', out=os.path.join(REPO, 'Src', 'MmbrunImage.c'), name='MmbrunImage',
          section='.xip'):
    """Src/MmbrunImage.c: out/mmbrunx.bin's bytes as a constant in the firmware's .xip, at the flash address they were
    linked for, where the loader runs the runtime in place (Loader.c, ld_bc)"""
    data = open(os.path.join(OUT, binname), 'rb').read()
    rows = [', '.join('0x%02X' % b for b in data[i:i + 16]) for i in range(0, len(data), 16)]
    what = 'bcrun, the C compiler\'s runtime,' if name == 'BcrunImage' else 'mmbrun, bcrun with the BASIC runtime,'
    open(out, 'w', newline='\n').write(
        '/* %s.c: %s as the firmware carries it in bank 2\'s flash (Peter, 4 Nov 2026:\n'
        '   a compiled program then needs nothing on the card), linked for the place the link script\'s %s gives it\n'
        '   (cmm2img.h), where Loader.c runs it. Made by apps/cc/make.py from apps/cc/out/%s, built\n'
        '   from Peter\'s Fuzix fork, branch cmm2, at %s%s:\n'
        '   do not edit, but run make.py and build the firmware again. bcrun is the runtime of the Fuzix Compiler Kit\'s\n'
        '   PC3 target, not under the compiler\'s GPLv3 (the kit\'s README). */\n'
        '#include <stdint.h>\n\n'
        'const uint8_t %s[%d] __attribute__((section("%s"), used, aligned(4))) = {\n%s\n};\n'
        % (name, what, section, binname, head[:9], ' with changes' if dirty else '', name, len(data), section,
           ',\n'.join('    ' + r for r in rows)))
    print('%s: %s, %d bytes' % (out, binname, len(data)))


def card():
    """out/sd/cc, the SD card's A:/cc (the folder the firmware's CC and RUN of a .c use): cc.app (bcrun is in the
    firmware); in include/ the headers - ctest-include's (what the PC3's card has), then the CMM2's over them
    (cmm2/include: a time.h with clock, a stdlib.h with all that bcrun has, and cmm2.h, the services); and in
    examples/ the CMM2's examples"""
    import shutil
    sd = os.path.join(OUT, 'sd', 'cc')
    inc = os.path.join(sd, 'include')
    ex = os.path.join(sd, 'examples')
    os.makedirs(inc, exist_ok=True)
    os.makedirs(ex, exist_ok=True)
    if os.path.exists(os.path.join(sd, 'bcrun.app')):
        os.remove(os.path.join(sd, 'bcrun.app'))
    for f in ('cc.app', 'mmbrun.app'):
        if os.path.exists(os.path.join(OUT, f)):
            shutil.copy(os.path.join(OUT, f), sd)
    # as the PC3's card has them (Applications/CC/mkccimage.sh): ctest-include's, the four FCC-view headers by name, and
    # every mmb_*.h (what a translated program reads); then the CMM2's over them
    mmb = os.path.join(SRC, 'Applications', 'mmb2c')
    hdrs = [os.path.join(CC, 'hosttest', 'ctest-include', f) for f in os.listdir(os.path.join(CC, 'hosttest',
                                                                                            'ctest-include'))]
    hdrs += [os.path.join(mmb, 'fcc', 'include', f) for f in ('math.h', 'ctype.h', 'stdint.h', 'time.h')]
    hdrs += [os.path.join(mmb, f) for f in os.listdir(mmb) if f.startswith('mmb_')]
    hdrs += [os.path.join(CC, 'cmm2', 'include', f) for f in os.listdir(os.path.join(CC, 'cmm2', 'include'))]
    for h in hdrs:
        if h.endswith('.h'):
            shutil.copy(h, inc)
    for f in sorted(os.listdir(os.path.join(CC, 'cmm2', 'examples'))):
        shutil.copy(os.path.join(CC, 'cmm2', 'examples', f), ex)
    print('%s: %s, include/ %d headers, examples/ %s' % (sd, ' '.join(f for f in sorted(os.listdir(sd))
                                                                      if f.endswith('.app')),
                                                         len(os.listdir(inc)), ' '.join(sorted(os.listdir(ex)))))


if __name__ == '__main__':
    main()
