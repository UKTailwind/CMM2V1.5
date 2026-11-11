"""make.py: builds apps/trs80/out/trs80.app, the TRS-80 Model I, III, 4 and 4P for the CMM2: sdltrs (Mark Grebe's and Jens Guenther's SDL port of
Tim Mann's xtrs), fetched the first time into apps/trs80/sdltrs (git ignores it) at the commit below, patches/*.patch applied to it in order. All of
it is built as it is, its screen code, menus and keyboard tables included, but for its debugger (debug.c, dis.c) and its X clipboard
(PasteManager.c): sdl_cmm2.c is the part of SDL 1.2 it uses, on the loader's services, and sdl/ that part's headers (Peter, 11 Nov 2026).

The program is BSD 2-Clause licensed, as sdltrs is (its LICENSE; xtrs's own permissive licence is its xtrs.LICENSE): the notices go with any copy.
No Tandy ROM is built in or given out with it. sdltrs's own stand-ins are: for the Model 4P, Peter Cervasio's free boot ROM, which starts a
Model 4 disk; for the Models I, III and 4, a screen that says no ROM is installed. The user's own ROM files are used when they are in the folder it
is started in: level2.rom (Model I), model3.rom (Models III and 4), model4p.rom.

Built --vram, so that a G1 runs it too: MODE 8's and MODE 1's pages in 8 bits fit a G1's internal RAM.

    python apps/trs80/make.py
then put out/trs80.app in A:/trs80, with the ROMs and the disk and cassette images (.dsk, .dmk, .jv1, .jv3, .cas, .cmd), and at the CMM2's prompt:
    CHDIR "A:/trs80"
    RUN "trs80.app"
"""
import glob, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'sdltrs')
URL = 'https://gitlab.com/jengun/sdltrs.git'
COMMIT = '6053aec7c8e3af70d16dcc796cfac2ee4d32ef70'      # branch sdl2, 10 Nov 2026

# src/*.c that are not built: files other files include (the fonts, the CRC), the debugger (ZBX), the X clipboard, and the serial port on a
# Unix tty (uart_cmm2.c is that file on the CMM2's serial ports)
NOT_BUILT = {'trs_chars.c', 'crc.c', 'debug.c', 'dis.c', 'PasteManager.c', 'trs_uart.c'}


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
    out = [f for f in glob.glob(os.path.join(SRC, 'src', '*.c')) if os.path.basename(f) not in NOT_BUILT]
    out += [os.path.join(HERE, f) for f in ('sdl_cmm2.c', 'uart_cmm2.c')]
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
    inc = [os.path.join(HERE, 'sdl'), os.path.join(SRC, 'src')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'trs80.app'),
           '--sources', listfile, '--api', '13', '--vram', '--stack', '0x10000', '--heap', str(3 << 20)]
    for d in inc:
        cmd += ['-I', d]
    # NOX: sdltrs's build for SDL 1.2 without X (no clipboard)
    # (sys/types.h first: newlib's dirent.h needs its ssize_t)
    cmd += ['--cflag=-DNOX', '--cflag=-O2', '--cflag=-fno-strict-aliasing', '--cflag=-Wno-unused-parameter',
            '--cflag=-include', '--cflag=sys/types.h']
    cmd += ['--cflag=' + a for a in sys.argv[1:] if a.startswith('-')]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
