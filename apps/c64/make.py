"""make.py: builds apps/c64/out/c64.app, a Commodore 64 for the CMM2: frank-c64 (rh1tech's RP2350 port of Christian Bauer's Frodo 4), fetched the first time into
apps/c64/frank-c64 (git ignores it) at the commit below, patches/*.patch applied to it in order. Its emulator (Frodo's line-based core, as frank-c64 builds it), its
settings, disk loader, disk menu and start screen are built as they are, but for its main_rp2350.c, the RP2350's start-up, and sid_i2s.cpp, the RP2350's sound: c64_cmm2.c
has what the rest needs of those, and stands in for the drivers (the display, the sound stream, the USB keyboard and gamepads, on the loader's services), ff_cmm2.c for
FatFs, and stub/ for the Pico SDK (Peter, 8 Nov 2026).

The program is GPL-2.0-or-later, as frank-c64 and Frodo are: its source must go with any copy given out (frank-c64's, at that commit, and this directory). The C64's
ROMs (BASIC, KERNAL, the characters and the 1541's) are frank-c64's, built in: Peter, 8 Nov 2026, "we can include the commodore roms as everyone else does".

Built --vram, so that a G1 runs it too: MODE 7's page, 320 x 240 in 8 bits with each line stored twice, fits a G1's internal RAM.

    python apps/c64/make.py
then put out/c64.app in A:/c64 and the disk images and programs (.d64, .d81, .prg, .crt) in it as well (frank-c64's /c64 is this folder), and at the CMM2's prompt:
    CHDIR "A:/c64"
    RUN "c64.app"
"""
import glob, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'frank-c64')
URL = 'https://github.com/rh1tech/frank-c64.git'
COMMIT = '97a3592a02f015419c2114c13ffbd8af1e226fe3'

# its CMakeLists.txt's, for an M2 board with HDMI (the board's pins are not used here)
DEFINES = ['CMM2_APP', 'BOARD_M2', 'CPU_CLOCK_MHZ=378', 'FLASH_MAX_FREQ_MHZ=66', 'FIRMWARE_VERSION="v1.08"', 'FRODO_RP2350=1', 'VIDEO_HDMI', 'VIDEO=HDMI',
           'FEATURE_AUDIO_I2S=1', 'PSRAM_MAX_FREQ_MHZ=133', 'ENABLE_PS2_KEYBOARD=1', 'ENABLE_DEBUG_LOGS=0', 'USB_HID_ENABLED=1', 'I2S_DATA_PIN=9', 'I2S_CLOCK_PIN_BASE=10']


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
    """the add_executable of its CMakeLists.txt, but for main_rp2350.c and sid_i2s.cpp; and the USB wrapper"""
    src = os.path.join(SRC, 'src')
    out = [os.path.join(src, 'rp2350', f) for f in ('Display_rp2350.cpp', 'C64_rp2350.cpp', 'Prefs_rp2350.cpp', 'ROM_data.cpp', 'SID_rp2350.cpp', 'input_rp2350.cpp',
                                                    'disk_loader.c', 'disk_ui.c', 'startscreen.c', 'fatfs_stdio.c', 'Tape_stub.cpp')]
    out += [os.path.join(src, f + '.cpp') for f in ('CPUC64', 'VIC', 'CIA', 'IEC', 'Cartridge', 'REU', '1541d64', '1541gcr', 'CPU1541', 'VIA')]
    out += [os.path.join(SRC, 'drivers', 'usbhid', 'usbhid_wrapper.c')]
    out += [os.path.join(HERE, 'c64_cmm2.c'), os.path.join(HERE, 'ff_cmm2.c')]
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
    inc = [os.path.join(HERE, 'stub'), os.path.join(SRC, 'src'), os.path.join(SRC, 'src', 'rp2350'), os.path.join(SRC, 'drivers'),
           os.path.join(SRC, 'drivers', 'usbhid')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'c64.app'),
           '--sources', listfile, '--api', '13', '--vram', '--stack', '0x10000', '--heap', str(2 << 20)]
    for d in inc:
        cmd += ['-I', d]
    cmd += ['--cflag=-D' + d for d in DEFINES] + ['--cflag=-w', '--cflag=-O3', '--cflag=-fno-strict-aliasing', '--cxxflag=-std=gnu++20']   # (its CMakeLists.txt's)
    cmd += ['--cflag=' + a for a in sys.argv[1:] if a.startswith('-')]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
