"""make.py: builds apps/atari800/out/atari800.app, the Atari 8-bit computers (400/800, XL/XE) and the 5200 for the CMM2: atari800 7.2.1 (the
Atari800 development team's emulator), fetched the first time into apps/atari800/atari800 (git ignores it) at the commit below, patches/*.patch
applied to it in order. Its emulator and its menus (ui.c, ui_basic.c) are built as they are; a8_cmm2.c is the platform, as atari800's sdl/ is on a
PC (the display, the keys, the joysticks, the sound and the time, on the loader's services), and port/config.h what its configure would write
(Peter, 9 Nov 2026, for a user's request).

The program is GPL-2.0-or-later, as atari800 is: its source must go with any copy given out (atari800's, at that commit, and this directory). The
ROMs built in are AltirraOS and Altirra BASIC (Avery Lee's, under their own all-permissive licence, in atari800's src/roms): Atari's own ROMs are
not given out with it, and are used when they are in the folder it is started in (Peter, 9 Nov 2026).

Built --vram, so that a G1 runs it too: MODE 7's page, 320 x 240 in 8 bits with each line stored twice, fits a G1's internal RAM.

    python apps/atari800/make.py
then put out/atari800.app in A:/atari800, with the disk images, programs and cartridges (.atr, .xex, .car, .rom, .cas) and any Atari ROMs of
your own, and at the CMM2's prompt:
    CHDIR "A:/atari800"
    RUN "atari800.app"
"""
import glob, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'atari800')
URL = 'https://github.com/atari800/atari800.git'
COMMIT = '63a381884175e3cbfc745ff00dfb3d70d928eb50'      # ATARI800_7_2_1, 13 Sep 2026

# src/*.c that are other ports' (each with its main or its device), or features config.h leaves out
NOT_BUILT = {'atari_basic.c', 'atari_curses.c', 'atari_ps2.c', 'atari_rpi.c', 'atari_x11.c', 'sound_oss.c', 'joycfg.c', 'mkimg.c', 'netsio.c',
             'netsiowin.c', 'download.c', 'videomode.c', 'pal_blending.c', 'filter_ntsc.c', 'file_export.c', 'pokeyrec.c', 'af80.c', 'bit3.c',
             'xep80.c', 'xep80_fonts.c', 'pbi_bb.c', 'pbi_mio.c', 'pbi_proto80.c', 'pbi_scsi.c', 'pbi_xld.c', 'ide.c', 'rdevice.c', 'voicebox.c',
             'votrax.c', 'votraxsnd.c'}


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
    src = os.path.join(SRC, 'src')
    out = [f for f in glob.glob(os.path.join(src, '*.c')) if os.path.basename(f) not in NOT_BUILT]
    out += glob.glob(os.path.join(src, 'roms', '*.c'))
    out += [os.path.join(HERE, 'a8_cmm2.c')]
    return sorted(out)


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
    inc = [os.path.join(HERE, 'port'), os.path.join(SRC, 'src')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'atari800.app'),
           '--sources', listfile, '--api', '13', '--vram', '--stack', '0x10000', '--heap', str(2 << 20)]
    for d in inc:
        cmd += ['-I', d]
    cmd += ['--cflag=-DHAVE_CONFIG_H', '--cflag=-O3', '--cflag=-fno-strict-aliasing', '--cflag=-Wno-unused-parameter']
    cmd += ['--cflag=' + a for a in sys.argv[1:] if a.startswith('-')]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
