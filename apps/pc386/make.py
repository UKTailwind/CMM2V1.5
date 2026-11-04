"""make.py: builds apps/pc386/out/pc386.app, a 386 PC for the CMM2: tiny386 (CPU, x87, VGA, IDE, keyboard and mouse,
Sound Blaster 16, Adlib, PC speaker) with the CMM2 layer pc386_cmm2.c (Peter, 3 Nov 2026). tiny386 is fetched the first
time into apps/pc386/tiny386 (git ignores it), at the commit below, patches/*.patch applied to it in order (each a fix to
tiny386 itself, a candidate for upstream), and built with the files its ESP32 build names, its front ends left out.
port.h is included first in every file (the clock tiny386 asks of POSIX), and stub/ holds the termios and ioctl headers
of the host console code it has but the CMM2 does not use.

tiny386's CPU and the project are BSD-3-Clause, the devices from QEMU and TinyEMU MIT, the Adlib's fmopl LGPL-2.1+; the
BIOS files it boots (SeaBIOS, LGPL-3) go on the SD card beside the program.

    python apps/pc386/make.py [interp|pd]
then put out/pc386.app in A:/pc386 with bios.bin, vgabios.bin, a disk image and pc386.ini, and at the CMM2's prompt:
CHDIR "A:/pc386" : RUN "pc386.app" (or RUN "pc386.app", other.ini). pc386.app has tiny386's fast path in front of its
interpreter (fast/fast.c in place of i386.c): FreeDOS boots in 35 s, not 52 (Peter: the default, 3 Nov 2026). interp
builds the interpreter alone, pd the fast path with its pre-decoding too (quicker in tight loops, slower to boot), as
out/pc386interp.app and pc386pd.app.
"""
import glob, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'tiny386')
URL = 'https://github.com/hchunhui/tiny386.git'
COMMIT = '2ba0f6aaf4d545a3775e9cb44bf721f3f5789367'   # 3 Nov 2026
FILES = '''adlib fmopl fpu i386 i8042 i8254 i8259 ide ini pc misc pci vga vgafont ne2000 i8257 sb16 pcspk'''.split()


def patch():
    """the patches applied to the commit's files, once: again (from the commit) only when they change"""
    patches = sorted(glob.glob(os.path.join(HERE, 'patches', '*.patch')))
    want = hashlib.sha1(b''.join(open(p, 'rb').read() for p in patches)).hexdigest()
    stamp = os.path.join(SRC, '.cmm2-patches')
    if os.path.exists(stamp) and open(stamp).read().strip() == want:
        return
    subprocess.run(['git', '-C', SRC, 'checkout', '-q', '--', '.'], check=True)
    for p in patches:
        subprocess.run(['git', '-C', SRC, 'apply', p], check=True)
    open(stamp, 'w').write(want + '\n')


def main():
    if not os.path.isdir(SRC):
        subprocess.run(['git', 'clone', '-q', URL, SRC], check=True)
        subprocess.run(['git', '-C', SRC, 'checkout', '-q', COMMIT], check=True)
    head = subprocess.run(['git', '-C', SRC, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != COMMIT:
        sys.exit('%s is at %s, not %s' % (SRC, head, COMMIT))
    patch()
    os.makedirs(os.path.join(HERE, 'out'), exist_ok=True)
    args = [a for a in sys.argv[1:] if not a.startswith('-')]     # (and flags for the compiler: -O3 and the like)
    cpu = args[0] if args else ''
    if cpu not in ('', 'interp', 'pd'):
        sys.exit('make.py [interp|pd] [-compiler-flag...]')
    srcs = [os.path.join(SRC, f + '.c') for f in FILES] + [os.path.join(HERE, 'pc386_cmm2.c')]
    flags = ['-include', os.path.join(HERE, 'port.h'), '-DBPP=8', '-DI386_ENABLE_FPU', '-DUSE_FMOPL',
             '-DI386_ALIGNED_ONLY', '-w']   # (the guest's unaligned words: single LDRs and STRs, never an LDRD, LDM, STRD or
                                            # STM, which a Cortex-M7 faults on unaligned)
    if cpu != 'interp':
        srcs[FILES.index('i386')] = os.path.join(SRC, 'fast', 'fast.c')
        flags += ['-DUSE_FAST'] + (['-DFAST_PD'] if cpu == 'pd' else [])
    flags += [a for a in sys.argv[1:] if a.startswith('-')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'),
           os.path.join(HERE, 'out', 'pc386%s.app' % cpu)]
    cmd += srcs + ['--api', '9', '--vram', '--stack', '0x10000', '--heap', str(5 << 20), '-I', SRC,
                   '-I', os.path.join(HERE, 'stub')]
    cmd += ['--cflag=' + f for f in flags]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
