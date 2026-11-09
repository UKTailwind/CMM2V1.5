"""make.py: builds apps/apple2/out/apple2.app, an Apple //e for the CMM2: frank-apple (rh1tech's RP2350 port of Michel Pollet's MII), fetched the first time
into apps/apple2/frank-apple (git ignores it) at the commit below, patches/*.patch applied to it in order. Its emulator (MII's 65C02, memory, soft switches,
Disk II, SmartPort, paddles), its disk loader, disk menu and start screen are built as they are, but for its main.c, the RP2350's start-up and second core,
and mii_audio_i2s.c, the RP2350's sound: apple_cmm2.c has what the rest needs of those, and stands in for the drivers (the display, the speaker, the USB
keyboard and gamepads, on the loader's services), apple_video.c draws the screen at the Apple's full resolution, ff_cmm2.c stands in for FatFs, and stub/
for the Pico SDK (Peter, 9 Nov 2026).

The program is MIT licensed, as frank-apple and MII are (frank-apple's usbhid wrapper, GPL-2.0, is not built). The Apple //e Enhanced ROMs (the computer's
and its characters') and the Disk II's boot ROM are frank-apple's, built in.

Built --vram, so that a G1 runs it too: MODE 8's page, 640 x 480 in 8 bits, fits a G1's internal RAM.

    python apps/apple2/make.py
then put out/apple2.app in A:/apple2 and the disk images (.dsk, .do, .po, .nib, .woz) in it as well (frank-apple's /apple is this folder), and at the
CMM2's prompt:
    CHDIR "A:/apple2"
    RUN "apple2.app"
"""
import glob, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'frank-apple')
URL = 'https://github.com/rh1tech/frank-apple.git'
COMMIT = '1420e27abd555473817d94672b265d89e60b07f0'

# its CMakeLists.txt's, for an RP2350 board (the board's pins are not used here), without PSRAM (the second drive's disk goes to the heap: patches/), and
# printf named apple_printf in its sources, so that its messages (one for each step of a disk's head) go to the console only when asked for (-log)
DEFINES = ['CMM2_APP', 'BOARD_M2', 'CPU_CLOCK_MHZ=378', 'MII_65C02_DIRECT_ACCESS=1', 'MII_RP2350=1', 'PICO_RP2350=1', 'VIDEO_HDMI', 'VIDEO=HDMI',
           'ENABLE_PS2_KEYBOARD=0', 'ENABLE_DEBUG_LOGS=0', '_GNU_SOURCE', 'printf=apple_printf']


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
    """the add_executable of its CMakeLists.txt, but for main.c and mii_audio_i2s.c"""
    src = os.path.join(SRC, 'src')
    out = [os.path.join(src, f + '.c') for f in ('mii_65c02', 'mii_bank', 'mii_video', 'mii_rom', 'mii', 'mii_rom_iiee', 'mii_rom_iiee_video',
                                                 'disk_loader', 'disk_ui', 'mii_startscreen', 'mii_analog', 'mii_slot', 'mii_dd_stub', 'mii_floppy',
                                                 'mii_disk2', 'mii_dsk', 'mii_nib', 'mii_woz', 'mii_vcd_stub', 'mii_rom_disk2_p5', 'mii_smartport',
                                                 'mii_audio')]
    out += [os.path.join(HERE, f) for f in ('apple_cmm2.c', 'apple_video.c', 'ff_cmm2.c')]
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
    inc = [os.path.join(HERE, 'stub'), os.path.join(HERE), os.path.join(SRC, 'src'), os.path.join(SRC, 'drivers')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'apple2.app'),
           '--sources', listfile, '--api', '13', '--vram', '--stack', '0x10000', '--heap', str(2 << 20)]
    for d in inc:
        cmd += ['-I', d]
    cmd += ['--cflag=-D' + d for d in DEFINES] + ['--cflag=-w', '--cflag=-O3', '--cflag=-fno-strict-aliasing']   # (its CMakeLists.txt's)
    cmd += ['--cflag=' + a for a in sys.argv[1:] if a.startswith('-')]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
