"""make.py: builds apps/amiga/out/amiga.app, an Amiga 500 for the CMM2: frank-amiga (rh1tech's port of Matt Parsons's Omega,
the 68000 by Musashi and the OCS chipset slot by slot) fetched the first time into apps/amiga/frank-amiga (git ignores it) at
the commit below, patches/*.patch applied to it in order. The native (desktop) build of its core is used, as its
native/build.sh makes it (no PICO_BUILD: the RP2350 board's two halves and their link are not), with amiga_cmm2.c as the
host in place of native/main_native.c, host_native.c and Presentation.c (Peter, 9 Nov 2026).

GPL-3.0-or-later, as frank-amiga is (its third_party/omega under MPL-2.0, Musashi under its own licence): its source must go
with any copy given out (frank-amiga's, at that commit, and this directory). No Kickstart is built in: the user's own.

    python apps/amiga/make.py
then put out/amiga.app in A:/amiga with a Kickstart (kick.rom) and disk images (.adf), and at the CMM2's prompt:
    CHDIR "A:/amiga"
    RUN "amiga.app", game.adf
"""
import glob, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'frank-amiga')
URL = 'https://github.com/rh1tech/frank-amiga.git'
COMMIT = '00da6a6d6975da2d1ae527135d4f973700ef452a'

DEFINES = ['OMEGA_VIDEO_STANDARD=1',            # PAL (native/build.sh's VIDEO=PAL)
           'OMEGA_SCREENSHOT_REGRESSION',       # (main_native.c's boot sampler out: not built here anyway)
           'P2_DEBUG_BLITS',                    # (as native/build.sh: the variable it defines; the card is off)
           'CMM2_PIXEL16',                      # 0x0RGB pixels, the screen's ARGB4444 (patches/0001)
           'AUDIO_RATE=44100',                  # Paula mixed at the firmware's stream rate
           'AXI_TABLE']                         # the 68000's opcode table in the AXI RAM (amiga_cmm2.c)


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


def sources():
    """native/build.sh's sources, but for main_native.c, host_native.c and Presentation.c (amiga_cmm2.c is the host)"""
    O = os.path.join(SRC, 'third_party', 'omega')
    N = os.path.join(SRC, 'native')
    out = [os.path.join(N, f) for f in ('memory_native.c', 'hdf_native.c', 'kicksmash_native.c')]
    out += [os.path.join(SRC, 'slave', 'src', 'platform', 'Planar.c'), os.path.join(SRC, 'common', 'amiga', 'Collision.c')]
    out += [os.path.join(O, f + '.c') for f in ('Audio', 'AudioMix', 'Blitter', 'Chipset', 'CIA', 'CPU', 'debug', 'DMA', 'Floppy',
                                                'Expansion', 'Gayle', 'Picasso2', 'KickSmash', 'm68kcpu', 'm68kdasm', 'm68kopac',
                                                'm68kopdm', 'm68kopnz', 'm68kops')]
    out += [os.path.join(HERE, 'amiga_cmm2.c')]
    return out


def main():
    if not os.path.isdir(SRC):                  # (that commit alone: the whole history will not unpack in a Dropbox folder)
        subprocess.run(['git', 'init', '-q', SRC], check=True)
        subprocess.run(['git', '-C', SRC, 'fetch', '-q', '--depth', '1', URL, COMMIT], check=True)
        subprocess.run(['git', '-C', SRC, 'checkout', '-q', 'FETCH_HEAD'], check=True)
    head = subprocess.run(['git', '-C', SRC, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != COMMIT:                          # (first: in a folder with a broken .git, git -C falls through to the CMM2 repository)
        sys.exit('%s is at %s, not %s' % (SRC, head, COMMIT))
    patch()
    os.makedirs(os.path.join(HERE, 'out'), exist_ok=True)
    listfile = os.path.join(HERE, 'out', 'sources.txt')
    open(listfile, 'w').write('\n'.join(sources()) + '\n')
    N = os.path.join(SRC, 'native')
    inc = [HERE, N, os.path.join(SRC, 'common', 'amiga'), os.path.join(SRC, 'slave', 'src', 'platform'), os.path.join(SRC, 'third_party', 'omega')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'amiga.app'),
           '--sources', listfile, '--api', '17', '--g2', '--stack', '0x10000', '--heap', str(3584 << 10)]
    for d in inc:
        cmd += ['-I', d]
    cmd += ['--cflag=-include', '--cflag=sys/types.h',                      # (newlib's dirent.h needs ssize_t)
            '--cflag=-include', '--cflag=' + os.path.join(N, 'sdl_shim.h'),   # (the native code's SDL atomics, no SDL)
            '--cflag=-include', '--cflag=' + os.path.join(HERE, 'amiga_mem.h')]
    cmd += ['--cflag=-D' + d for d in DEFINES] + ['--cflag=-std=gnu11', '--cflag=-w', '--cflag=-O2']
    cmd += ['--cflag=' + a for a in sys.argv[1:] if a.startswith('-')]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
