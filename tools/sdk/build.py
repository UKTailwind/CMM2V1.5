"""build.py OUT.app SOURCE.c|.cpp [...] [--base ADDR] [--stack BYTES] [--heap BYTES] [--api N] [--cflag FLAG ...] [-I DIR ...]

Builds a loadable program for the CMM2 G2 with the firmware's own toolchain (CMakePresets.json's ARM_TOOLCHAIN_DIR):
compiles the sources with crt0.c and syscalls.c for the M7 (double-precision hard float; C++ as C++14 without
exceptions or RTTI, and then linked by g++ for its library), links them with app.ld at
--base (default 0xD1800000, where the G2 loads a program, so nothing is moved), and makes OUT.app with mkimage.py.
The ELF and its map are left beside OUT.app. --base 0x60000000 makes a copy that the loader must relocate (a pointer
it missed would then fault at once, the FMC bank there being unused): the test of the relocations.
"""
import argparse, json, os, subprocess, sys

SDK = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(SDK))
tooldir = json.load(open(os.path.join(REPO, 'CMakePresets.json')))['configurePresets'][0]['cacheVariables']['ARM_TOOLCHAIN_DIR']
EXE = '.exe' if os.name == 'nt' else ''
GCC = os.path.join(tooldir, 'arm-none-eabi-gcc' + EXE)
GXX = os.path.join(tooldir, 'arm-none-eabi-g++' + EXE)

FLAGS = ['-mcpu=cortex-m7', '-mfpu=fpv5-d16', '-mfloat-abi=hard', '-mthumb', '--specs=nano.specs',
         '-O2', '-g0', '-Wall', '-ffunction-sections', '-fdata-sections',
         '-fno-exceptions', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables',
         '-iquote', os.path.join(REPO, 'Inc'), '-I', SDK]
CFLAGS = FLAGS + ['-std=gnu11']
CXXFLAGS = FLAGS + ['-std=gnu++14', '-fno-rtti', '-fno-threadsafe-statics', '-fno-use-cxa-atexit']


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('sources', nargs='+')
    ap.add_argument('--base', default='0xD1800000')
    ap.add_argument('--stack', default='0')
    ap.add_argument('--heap', default='0')
    ap.add_argument('--api', default='1')
    ap.add_argument('--cflag', action='append', default=[])
    ap.add_argument('-I', dest='incs', action='append', default=[])
    a = ap.parse_args()
    stem = os.path.splitext(a.out)[0]
    incs = ['-I' + i for i in a.incs]
    objs, cxx = [], False
    for s in a.sources + [os.path.join(SDK, 'crt0.c'), os.path.join(SDK, 'syscalls.c')]:
        # each source to an object by its own compiler (g++ would take every .c after the first as C++, -x c or not)
        isc = not s.lower().endswith(('.cpp', '.cc'))
        cxx = cxx or not isc
        o = stem + '_' + os.path.splitext(os.path.basename(s))[0] + '.o'
        r = subprocess.run(([GCC] + CFLAGS if isc else [GXX] + CXXFLAGS) + a.cflag + incs + ['-c', s, '-o', o])
        if r.returncode:
            sys.exit(r.returncode)
        objs.append(o)
    cmd = ([GXX if cxx else GCC] + FLAGS + a.cflag + objs +
           ['-nostartfiles', '-T', os.path.join(SDK, 'app.ld'), '-Wl,--defsym=__app_base=' + a.base,
            '-Wl,--gc-sections', '-Wl,--emit-relocs', '-Wl,-Map=' + stem + '.map', '-Wl,--no-warn-rwx-segments',
            '-u', '_printf_float', '-o', stem + '.elf', '-Wl,--start-group', '-lc', '-lm', '-lgcc', '-Wl,--end-group'])
    r = subprocess.run(cmd)
    if r.returncode:
        sys.exit(r.returncode)
    r = subprocess.run([sys.executable, '-I', os.path.join(SDK, 'mkimage.py'), stem + '.elf', a.out,
                        '--stack', str(int(a.stack, 0)), '--heap', str(int(a.heap, 0)), '--api', a.api])
    sys.exit(r.returncode)


if __name__ == '__main__':
    main()
