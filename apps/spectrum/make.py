"""make.py: builds apps/spectrum/out/spectrum.app, a Sinclair ZX Spectrum for the CMM2: pico-spec (DnCraptor's port of ESPectrum to the RP2040 and RP2350), fetched the first
time into apps/spectrum/pico-spec (git ignores it) at the commit below, patches/*.patch applied to it in order. Its emulator, tape and snapshot code and its on-screen menus are
built as they are, as the RP2040's build, which leaves out the network, General Sound and the rest that needs the RP2350. spectrum_cmm2.cpp stands in for its drivers
(the display, the sound stream, the USB keyboard and gamepads, on the loader's services), ff_cmm2.c for FatFs, and stub/ for the Pico SDK (Peter, 5 Nov 2026).

The program is GPL-3.0, as pico-spec is: its source must go with any copy given out (pico-spec's, at that commit, and this directory). The ROMs built into it are pico-spec's
and are not under the GPL: the README of the programs' zip says whose they are.

Built --vram, so that a G1 runs it too.

    python apps/spectrum/make.py
then put out/spectrum.app in A:/spectrum with what it is to load, and at the CMM2's prompt:
    CHDIR "A:/spectrum"
    RUN "spectrum.app"
"""
import glob, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'pico-spec')
URL = 'https://github.com/DnCraptor/pico-spec.git'
COMMIT = '7b8f48f70437deb2230bf7b92a1515d99c8a21e2'   # 30 Jul 2026

DEFINES = ['CMM2_APP', 'PICO_RP2040=1', 'VGA_HDMI', 'CPU_MHZ=480', 'FLASH_SIZE=4096', 'CONFIG_BOARD_TAG="cmm2"', 'PORT_VERSION="1.2.28"', 'PORT_VERSION_LEN=6',
           'IDE_PORT_TRACE=0', 'FDD_PORT_TRACE=0', 'RTC_PORT_TRACE=0', 'PROFI_PORT_TRACE=0', 'SND_PORT_TRACE=0', 'PERF_TRACE=0', 'MEM_ACCESS_TRACE=0',
           'GS_PERF_TRACE=0', 'GS_DEBUG_TRACE=0', 'ZIFI_TRACE=0', 'ZIFI_NET_CLIENT=0', 'DEFAULT_THROTTLING=0', 'KBDUSB', 'USE_NESPAD=1', 'NES_GPIO_CLK=0', 'NES_GPIO_DATA=0', 'NES_GPIO_LAT=0']


OUT = ('pwm_audio.cpp', 'UsbMsc.cpp', 'hid_app.cpp', 'hid_ri.c', 'hid_rip.c', 'joy_hid.cpp', 'xinput_host.c', 'PinSerialData_595.c')   # (the drivers)


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
    """its CMakeLists.txt's list, but for main.cpp and what stands for a driver"""
    src = os.path.join(SRC, 'src')
    out = glob.glob(os.path.join(src, '**', '*.cpp'), recursive=True) + glob.glob(os.path.join(src, '**', '*.c'), recursive=True)
    out += glob.glob(os.path.join(SRC, 'drivers', 'picomp3lib', '*.c'))
    out = [f.replace('\\', '/') for f in out]
    out = [f for f in out if os.path.basename(f) not in OUT and '/src/GS/' not in f]
    out += [os.path.join(HERE, 'spectrum_cmm2.cpp').replace('\\', '/'), os.path.join(HERE, 'ff_cmm2.c').replace('\\', '/')]
    return sorted(out)


def includes():
    return [os.path.join(HERE, 'stub'), os.path.join(SRC, 'src'), os.path.join(SRC, 'src', 'roms'), os.path.join(SRC, 'drivers', 'picomp3lib')]


def main():
    if not os.path.isdir(SRC):
        subprocess.run(['git', 'clone', '-q', URL, SRC], check=True)
        subprocess.run(['git', '-C', SRC, 'checkout', '-q', COMMIT], check=True)
    head = subprocess.run(['git', '-C', SRC, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != COMMIT:
        sys.exit('%s is at %s, not %s' % (SRC, head, COMMIT))
    patch()
    os.makedirs(os.path.join(HERE, 'out'), exist_ok=True)
    listfile = os.path.join(HERE, 'out', 'sources.txt')
    open(listfile, 'w').write('\n'.join(sources()) + '\n')
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'spectrum.app'),
           '--sources', listfile, '--api', '13', '--vram', '--stack', '0x10000', '--heap', str(2 << 20)]
    for d in includes():
        cmd += ['-I', d]
    cmd += ['--cflag=-D' + d for d in DEFINES] + ['--cflag=-w', '--cxxflag=-std=gnu++17']
    cmd += ['--cflag=' + a for a in sys.argv[1:] if a.startswith('-')]
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
