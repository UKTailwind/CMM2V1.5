"""make.py: builds apps/quake/out/quake.app, Quake for the CMM2 (quake_cmm2.c). quakegeneric is fetched the first time into
apps/quake/quakegeneric (git ignores it), at the commit below, with patches/ applied, and built with its platform files replaced by quake_cmm2.c.
The program is GPL-2.0, as quakegeneric is: its source must go with any copy given out (quakegeneric's, at that commit,
id's sound files, and this directory). Any arguments are more compiler flags.

    python apps/quake/make.py
then put out/quake.app in A:/quake, with pak0.pak in A:/quake/id1, and at the CMM2's prompt: CHDIR "A:/quake": RUN "quake.app"
"""
import glob, hashlib, os, subprocess, sys, urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'quakegeneric')
URL = 'https://github.com/erysdren/quakegeneric.git'
COMMIT = '13052102577c629650cf07a46151a4b6e1b19c3c'   # 28 Jan 2025
NOT = '''quakegeneric quakegeneric_dos quakegeneric_null quakegeneric_sdl2 quakegeneric_w32 sys_null vid_null snd_null'''.split()
# quakegeneric has no sound: the mixer is id's own, from the source it came from
ID = 'https://raw.githubusercontent.com/id-Software/Quake/bf4ac424ce754894ac8f1dae6a3981954bc9852d/WinQuake/'
IDFILES = ['snd_dma.c', 'snd_mem.c', 'snd_mix.c']


def patch():
    """patches/*.patch applied to the commit's files, once: again (from the commit) only when they change"""
    patches = sorted(glob.glob(os.path.join(HERE, 'patches', '*.patch')))
    want = hashlib.sha1(b''.join(open(p, 'rb').read() for p in patches)).hexdigest()
    stamp = os.path.join(SRC, '.cmm2-patches')
    if os.path.exists(stamp) and open(stamp).read() == want:
        return
    subprocess.run(['git', '-C', SRC, 'checkout', '-q', '--', '.'], check=True)
    for p in patches:
        subprocess.run(['git', '-C', SRC, 'apply', '--whitespace=nowarn', p], check=True)
    open(stamp, 'w').write(want)


def main():
    if not os.path.isdir(os.path.join(SRC, '.git')):               # (by a shallow fetch, as a full clone fails in Dropbox)
        subprocess.run(['git', 'init', '-q', SRC], check=True)
        subprocess.run(['git', '-C', SRC, 'fetch', '-q', '--depth', '1', URL, COMMIT], check=True)
        subprocess.run(['git', '-C', SRC, 'checkout', '-q', 'FETCH_HEAD'], check=True)
    head = subprocess.run(['git', '-C', SRC, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != COMMIT:
        sys.exit('%s is at %s, not %s' % (SRC, head, COMMIT))
    patch()
    os.makedirs(os.path.join(HERE, 'out'), exist_ok=True)
    idsrc = os.path.join(HERE, 'id')
    os.makedirs(idsrc, exist_ok=True)
    for f in IDFILES:
        if not os.path.exists(os.path.join(idsrc, f)):
            urllib.request.urlretrieve(ID + f, os.path.join(idsrc, f))
    qg = os.path.join(SRC, 'source')
    srcs = [f for f in sorted(glob.glob(os.path.join(qg, '*.c'))) if os.path.basename(f)[:-2] not in NOT]
    srcs += [os.path.join(idsrc, f) for f in IDFILES] + [os.path.join(HERE, 'quake_cmm2.c')]
    # -fno-short-enums: Quake reads structures with enums in them straight from its files, where each is four bytes
    flags = ['-w', '-fno-strict-aliasing', '-fno-short-enums'] + sys.argv[1:]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'quake.app')]
    cmd += srcs + ['--api', '11', '--g2', '--stack', '0x60000', '--heap', str(2 << 20), '-I', qg]
    cmd += ['--cflag=' + f for f in flags] + ['--ldflag=-Wl,--wrap=S_ExtraUpdate', '--ldflag=-Wl,--no-enum-size-warning', '--ldflag=-Wl,-u,_scanf_float']   # (newlib-nano's fscanf has no %f without it: a saved game is read by it)
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
