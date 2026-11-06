"""make.py: builds apps/cpc/out/cpc.app, an Amstrad CPC 464, 664 and 6128 for the CMM2: frank-cpc (rh1tech's RP2350 port of Caprice32), fetched the first time into
apps/cpc/frank-cpc (git ignores it) at the commit below, patches/*.patch applied to it in order. Its emulator, settings, loader and on-screen menus are built as they are, but
for frank-cpc's own main.c, which is the RP2350's start-up: cpc_cmm2.c has what the rest needs of it, and stands in for the drivers (the display, the sound stream, the USB
keyboard and gamepads, on the loader's services), ff_cmm2.c for FatFs, and stub/ for the Pico SDK (Peter, 6 Nov 2026).

Left out by Peter's decision: IPF disk images (src/capsimg, src/cpc_ipf*: capsimg's licence is non-commercial and does not fit the GPL). .DSK, .CDT, .CAS and .CPR stay.

The program is GPL-3.0, as frank-cpc is: its source must go with any copy given out (frank-cpc's, at that commit, and this directory). The CPC's four ROMs are not in it: they
are the files cpc464.rom, cpc664.rom, cpc6128.rom and amsdos.rom of frank-cpc's sdcard/cpc.zip, read from the card.

Built --vram, so that a G1 runs it too: MODE 8's page, 640 x 480 in 8 bits, fits a G1's internal RAM.

    python apps/cpc/make.py
then put out/cpc.app in A:/cpc, the ROMs in A:/cpc/rom and what it is to load in A:/cpc/disk (frank-cpc's /cpc is this folder), and at the CMM2's prompt:
    CHDIR "A:/cpc"
    RUN "cpc.app"
"""
import glob, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'frank-cpc')
URL = 'https://github.com/rh1tech/frank-cpc.git'
COMMIT = '342b0882ffb9162fba8284b16e766ce3c84598cc'

DEFINES = ['CMM2_APP', 'PLATFORM_M2=1', 'PICO_BUILD=1', 'LSB_FIRST=1', 'HDMI_PIO=1', 'USB_HID_ENABLED=1', 'FRANK_CPC_VERSION="1.00"']


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
    """CAP32_ENGINE, CPC_PLATFORM, PICO_PORT and CPC_STUBS of its CMakeLists.txt, but for IPF and main.c; and the USB wrapper"""
    src = os.path.join(SRC, 'src')
    out = [os.path.join(src, 'cap32', f + '.cpp') for f in ('z80', 'cap32', 'crtc', 'psg', 'fdc', 'disk', 'tape', 'tape_fastload', 'asic')]
    out += [os.path.join(src, f) for f in ('cpc_adapter.cpp', 'cpc_cartridge.cpp', 'cpc_dandanator.cpp', 'tape_rec.c', 'tape_gpio.c', 'printer.c', 'dialogs.c')]
    out += [os.path.join(src, 'Pico', f + '.c') for f in ('platform', 'cpc_settings', 'cpc_loader', 'cpc_tape_loader', 'cpc_ui', 'cpc_autotype', 'cpc_serial_console',
                                                          'cpc_boot', 'ui_draw', 'ui_font')]
    out += [os.path.join(SRC, 'drivers', 'usbhid', 'usbhid_wrapper.c')]
    out += [os.path.join(HERE, 'cpc_cmm2.c'), os.path.join(HERE, 'ff_cmm2.c')]
    return sorted(out)


def main():
    if not os.path.isdir(SRC):                  # (that commit alone: the whole history will not unpack in a Dropbox folder)
        subprocess.run(['git', 'init', '-q', SRC], check=True)
        subprocess.run(['git', '-C', SRC, 'fetch', '-q', '--depth', '1', URL, COMMIT], check=True)
        subprocess.run(['git', '-C', SRC, 'checkout', '-q', 'FETCH_HEAD'], check=True)
    head = subprocess.run(['git', '-C', SRC, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != COMMIT:
        sys.exit('%s is at %s, not %s' % (SRC, head, COMMIT))
    patch()
    os.makedirs(os.path.join(HERE, 'out'), exist_ok=True)
    listfile = os.path.join(HERE, 'out', 'sources.txt')
    open(listfile, 'w').write('\n'.join(sources()) + '\n')
    inc = [os.path.join(HERE, 'stub'), os.path.join(SRC, 'src'), os.path.join(SRC, 'src', 'cap32'), os.path.join(SRC, 'src', 'Pico'), os.path.join(SRC, 'drivers'),
           os.path.join(SRC, 'drivers', 'usbhid'), os.path.join(SRC, 'drivers', 'nespad')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'cpc.app'),
           '--sources', listfile, '--api', '13', '--vram', '--stack', '0x10000', '--heap', str(3 << 20)]
    for d in inc:
        cmd += ['-I', d]
    cmd += ['--cflag=-D' + d for d in DEFINES] + ['--cflag=-w', '--cflag=-O3', '--cflag=-fno-strict-aliasing']   # (its CMakeLists.txt's)
    cmd += ['--cflag=' + a for a in sys.argv[1:] if a.startswith('-')]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
