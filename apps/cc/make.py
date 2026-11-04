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
Fuzix's C library.

    python apps/cc/make.py [cc|bcrun]
then put both in a folder on the SD card, with the headers (Applications/CC/hosttest/ctest-include) in its include/
folder, and at the CMM2's prompt: RUN "cc.app", prog.c   then   RUN "bcrun.app", prog.bc [args]
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
COMMIT = '803848948c3f98fdb6477820931f2a866f812545'   # branch cmm2, 4 Nov 2026 (from pc3's 6d662a677)
SPARSE = ['Applications/CC', 'Applications/cpp', 'Applications/mmb2c', 'Library/libs/fnum.c']

# the passes: their sources (Makefile.common and Makefile.host's lists) and flags (the host build's: tables as big as
# a PC's, which the CMM2's memory allows)
PASSFLAGS = ['-DCPU_armm0', '-DNO_FLOAT', '-DBIG_TABLES', '-include', 'unistd.h', '-I', CC]
C1 = '''body.c declaration.c enum.c error.c expression.c header.c idxdata.c initializer.c label.c lex.c main.c primary.c
        stackframe.c storage.c struct.c switch.c symbol.c tree.c type.c type_iterator.c target-thumb.c'''.split()
PASSES = [('cpp', CPP, 'cpp.c hash.c main.c token1.c token2.c'.split(), ['-I', CPP]),
          ('cc0', CC, ['frontend.c'], PASSFLAGS),
          ('cc1', CC, C1, PASSFLAGS),
          ('cc2', CC, ['backend.c', 'backend-bcode.c'], PASSFLAGS)]


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


def bcrun(extra):
    sdk([os.path.join(OUT, 'bcrun.app'), os.path.join(CC, 'bcrun.c'), os.path.join(CC, 'cmm2', 'bcrun_cmm2.c'),
         os.path.join(SRC, 'Library', 'libs', 'fnum.c'), '--stack', '0x40000', '-I', CC, '-I', os.path.join(CC, 'cmm2'),
         '--cflag=-DCMM2'] + ['--cflag=' + a for a in extra])


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
        ['--stack', '0x40000', '-I', CC, '-I', os.path.join(CC, 'cmm2'), '--cflag=-DCMM2'] +
        ['--cflag=' + a for a in extra])


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
        bcrun(extra)
    if not what or 'cc' in what:
        cc(extra)


if __name__ == '__main__':
    main()
