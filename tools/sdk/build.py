"""build.py OUT.app SOURCE.c|.cpp|.o [...] [--base ADDR] [--stack BYTES] [--heap BYTES] [--api N] [--g2] [--vram]
            [--module] [--xip] [--xip-base ADDR] [--no-printf-float] [--libc nano|full] [--cflag FLAG ...] [--cxxflag FLAG ...] [--ldflag FLAG ...] [-I DIR ...] [-j JOBS] [--sources FILE]

Builds a loadable program for the CMM2 with the firmware's own toolchain (the folder in ARM_TOOLCHAIN_DIR, else
CMakePresets.json's, else the PATH's): compiles the sources with crt0.c and syscalls.c for the M7 (double-precision
hard float; C++ as C++14 without exceptions or RTTI, and then linked by g++ for its library), links them with app.ld at
--base (default 0xD1800000, where the G2 loads a program, so nothing is moved; a G1 moves it), and makes OUT.app with
mkimage.py.
--xip builds instead a runtime the firmware carries in its own flash and runs in place (as bcrun, apps/cc/make.py):
linked with xip.ld at --xip-base (default cmm2img.h's CMM2XIP_FLASH, mmbrun's place) and CMM2XIP_RAM, with crt0.c's cmm2xip_header first, and OUT (.bin) is
the flash's bytes, for a C file the firmware is built with. It takes the firmware's libm (fwlibm.c), not its own.
--no-printf-float leaves out newlib's printf of floats (dtoa, 4 KB), which a program that formats its own does not
need: printf's %f is otherwise always there.
--libc full links the whole newlib instead of newlib-nano (the default): its wide-character printf and file functions
(swprintf, fwprintf, fgetws), which newlib-nano leaves out, for a program that needs them (sc-im). It costs some code
space. Neither newlib converts UTF-8 as STM32CubeIDE's toolchain builds them (no _MB_CAPABLE): setlocale takes "C"
only, and a byte is a character, as Latin-1 (12 Nov 2026).
The ELF and its map are left beside OUT.app. --base 0x60000000 makes a copy that the loader must relocate (a pointer
it missed would then fault at once, the FMC bank there being unused): the test of the relocations.

The objects go in OUT's stem + ".obj/", each named by its source's path (engines share file names), and are compiled
JOBS at a time (default: every core). A source is compiled again only when it, or a header it read (gcc -MMD), is
newer than its object, or the flags have changed. --cflag goes to C and C++, --cxxflag to C++ only, --ldflag to the
link. --sources names a file of sources, one a line, for lists too long for a command line. An object (.o) among
the sources is linked as it is.
"""
import argparse, concurrent.futures, hashlib, json, os, shutil, subprocess, sys

SDK = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(SDK))
EXE = '.exe' if os.name == 'nt' else ''


def toolchain():
    """the folder of arm-none-eabi-gcc: the environment's ARM_TOOLCHAIN_DIR (as the firmware's README has it), else
    CMakePresets.json's, else wherever the PATH finds it"""
    dirs = [os.environ.get('ARM_TOOLCHAIN_DIR', '')]
    try:
        dirs.append(json.load(open(os.path.join(REPO, 'CMakePresets.json')))['configurePresets'][0]['cacheVariables']
                    ['ARM_TOOLCHAIN_DIR'])
    except (OSError, ValueError, KeyError, IndexError):
        pass
    for d in dirs:
        if d and os.path.exists(os.path.join(d, 'arm-none-eabi-gcc' + EXE)):
            return d
    found = shutil.which('arm-none-eabi-gcc')
    if found:
        return os.path.dirname(found)
    sys.exit('arm-none-eabi-gcc not found: set ARM_TOOLCHAIN_DIR to its folder, or put it on the PATH')


tooldir = toolchain()
GCC = os.path.join(tooldir, 'arm-none-eabi-gcc' + EXE)
GXX = os.path.join(tooldir, 'arm-none-eabi-g++' + EXE)

NANO = ['--specs=nano.specs']                   # the C library, unless --libc full: newlib-nano's headers and archive
FLAGS = ['-mcpu=cortex-m7', '-mfpu=fpv5-d16', '-mfloat-abi=hard', '-mthumb',
         '-O2', '-g0', '-Wall', '-ffunction-sections', '-fdata-sections',
         '-fno-exceptions', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables', '-I', SDK]
CFLAGS = FLAGS + ['-std=gnu11']
CXXFLAGS = FLAGS + ['-std=gnu++14', '-fno-rtti', '-fno-threadsafe-statics', '-fno-use-cxa-atexit']


def objname(objdir, src):
    s = os.path.abspath(src)
    return os.path.join(objdir, os.path.splitext(os.path.basename(s))[0] + '_' +
                        hashlib.md5(s.lower().encode()).hexdigest()[:8] + '.o')


def stale(o):
    """True if the object is missing or older than its source or any header its .d names"""
    d = o[:-2] + '.d'
    if not os.path.exists(o) or not os.path.exists(d):
        return True
    t = os.path.getmtime(o)
    deps = open(d).read().replace('\\\n', ' ').split('.o:', 1)[-1]  # (after the target: D: in a path is no end)
    for dep in deps.replace('\\ ', '\0').split():
        dep = dep.replace('\0', ' ')
        if not os.path.exists(dep) or os.path.getmtime(dep) > t:
            return True
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('sources', nargs='*')
    ap.add_argument('--sources', dest='listfile')
    ap.add_argument('--base', default='0xD1800000')
    ap.add_argument('--stack', default='0')
    ap.add_argument('--heap', default='0')
    ap.add_argument('--api', default='1')
    ap.add_argument('--g2', action='store_true')
    ap.add_argument('--vram', action='store_true')
    ap.add_argument('--fixed', action='store_true')
    ap.add_argument('--module', action='store_true')               # a module (CMM2IMG_KIND_MODULE): crt0.c's
                                                                    # entry calls module_main(), not main()
    ap.add_argument('--xip', action='store_true')
    ap.add_argument('--xip-base', default='0x08141000')
    ap.add_argument('--axi', action='append', default=[])         # a source's name (no folder or extension): its code,
                                                                    # constants and data in the AXI RAM (app.ld's .axi)
    ap.add_argument('--itcm', action='append', default=[])        # NAME:f1,f2,... (or NAME: all its code): those
                                                                    # functions of a source's in the ITCM above the
                                                                    # firmware's code (app.ld's .itcm)
    ap.add_argument('--no-printf-float', dest='pfloat', action='store_false')
    ap.add_argument('--libc', choices=('nano', 'full'), default='nano')
    ap.add_argument('--cflag', action='append', default=[])
    ap.add_argument('--cxxflag', action='append', default=[])
    ap.add_argument('--ldflag', action='append', default=[])
    ap.add_argument('-I', dest='incs', action='append', default=[])
    ap.add_argument('-j', dest='jobs', type=int, default=os.cpu_count() or 4)
    a = ap.parse_args()
    libc = NANO if a.libc == 'nano' else []                         # (compile and link alike: the headers match the archive)
    if a.xip:                                                       # (in the flags' key: a change rebuilds everything)
        a.cflag += ['-DCMM2_XIP', '-DCMM2_XIP_API=' + a.api, '-DCMM2_XIP_STACK=' + str(int(a.stack, 0)),
                    '-DCMM2_XIP_HEAP=' + str(int(a.heap, 0))]
    if a.module:
        a.cflag += ['-DCMM2_MODULE']
    stem = os.path.splitext(a.out)[0]
    objdir = stem + '.obj'
    os.makedirs(objdir, exist_ok=True)
    incs = ['-I' + i for i in a.incs]
    sources = list(a.sources)
    if a.listfile:
        sources += [l.strip() for l in open(a.listfile) if l.strip() and not l.startswith('#')]
    sources += [os.path.join(SDK, 'crt0.c'), os.path.join(SDK, 'syscalls.c')]
    if a.xip:
        sources.append(os.path.join(SDK, 'fwlibm.c'))
    # the flags as they were: a change rebuilds everything
    key = hashlib.md5(json.dumps([CFLAGS, CXXFLAGS, libc, a.cflag, a.cxxflag, incs]).encode()).hexdigest()
    keyfile = os.path.join(objdir, 'flags.md5')
    same = os.path.exists(keyfile) and open(keyfile).read() == key
    cxx = any(s.lower().endswith(('.cpp', '.cc')) for s in sources)
    jobs, objs = [], []
    for s in sources:
        if s.lower().endswith('.o'):
            objs.append(os.path.abspath(s))
            continue
        # each source to an object by its own compiler (g++ would take every .c after the first as C++, -x c or not)
        isc = not s.lower().endswith(('.cpp', '.cc'))
        o = objname(objdir, s)
        objs.append(o)
        if not same or stale(o):
            cmd = (([GCC] + CFLAGS) if isc else ([GXX] + CXXFLAGS + a.cxxflag)) + libc + a.cflag + incs + \
                  ['-MMD', '-MF', o[:-2] + '.d', '-c', s, '-o', o]
            jobs.append((s, cmd))
    if jobs:
        print('compiling %d of %d sources, %d at a time' % (len(jobs), len(sources), a.jobs), flush=True)
    failed = []
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
        for (s, cmd), r in zip(jobs, ex.map(lambda j: subprocess.run(j[1], capture_output=True, text=True), jobs)):
            if r.stdout or r.stderr:
                sys.stdout.write(r.stdout + r.stderr)
            if r.returncode:
                failed.append(s)
    if failed:
        sys.exit('%d failed: %s' % (len(failed), ' '.join(os.path.basename(f) for f in failed[:20])))
    open(keyfile, 'w').write(key)
    rsp = stem + '.rsp'
    open(rsp, 'w').write('\n'.join('"%s"' % o.replace('\\', '/') for o in objs) + '\n')
    if (a.axi or a.itcm) and (a.xip or not a.g2):
        sys.exit('--axi and --itcm are for a G2 program (--g2), not --xip: a G1 shows its display from the AXI RAM')
    # app.ld's .itcm: each function by its section (-ffunction-sections: .text.NAME, and GCC's clones .text.NAME.part.0
    # and the like); copied with .axi's by cmm2_axi_load
    itcm = []
    for spec in a.itcm:                         # (NAME alone: all its code)
        obj, _, fns = spec.partition(':')
        secs = ' '.join('.text.%s .text.%s.*' % (f, f) for f in fns.split(',') if f) if fns else '.text .text.*'
        itcm.append('        */%s_????????.o(%s)\n' % (obj, secs))
    open(os.path.join(objdir, 'cmm2itcm.ld'), 'w').write(''.join(itcm))
    # app.ld's .axi: each object named (objname's NAME_xxxxxxxx.o) by its sections (gcc's -ffunction-sections names),
    # its .bss too, which the image then carries as zeros (cmm2_axi_load copies them with the rest)
    open(os.path.join(objdir, 'cmm2axi.ld'), 'w').write(''.join(
        '        */%s_????????.o(.text .text.* .rodata .rodata.* .data .data.* .bss .bss.* COMMON)\n' % n for n in a.axi))
    layout = (['-T', os.path.join(SDK, 'xip.ld'), '-Wl,--defsym=__xip_base=' + a.xip_base] if a.xip else
              ['-T', os.path.join(SDK, 'app.ld'), '-Wl,--defsym=__app_base=' + a.base, '-Wl,--emit-relocs',
               '-L' + objdir])
    cmd = ([GXX if cxx else GCC] + FLAGS + libc + a.cflag + ['@' + rsp] +
           ['-nostartfiles'] + layout +
           ['-Wl,--gc-sections', '-Wl,-Map=' + stem + '.map', '-Wl,--no-warn-rwx-segments',
            '-o', stem + '.elf'] + (['-u', '_printf_float'] if a.pfloat else []) + a.ldflag +
           ['-Wl,--start-group', '-lc', '-lm', '-lgcc', '-Wl,--end-group'])
    r = subprocess.run(cmd)
    if r.returncode:
        sys.exit(r.returncode)
    if a.xip:                                                       # the flash's bytes, from CMM2XIP_FLASH
        r = subprocess.run([os.path.join(tooldir, 'arm-none-eabi-objcopy' + EXE), '-O', 'binary', stem + '.elf', a.out])
        if r.returncode or open(a.out, 'rb').read(8) != b'CMM2XIP\0':
            sys.exit('%s: no cmm2xip_header at its start' % a.out)
        print('%s: %d bytes in the flash' % (a.out, os.path.getsize(a.out)))
        sys.exit(0)
    r = subprocess.run([sys.executable, '-I', os.path.join(SDK, 'mkimage.py'), stem + '.elf', a.out,
                        '--stack', str(int(a.stack, 0)), '--heap', str(int(a.heap, 0)), '--api', a.api] +
                       (['--g2'] if a.g2 else []) + (['--vram'] if a.vram else []) + (['--fixed'] if a.fixed else []) +
                       (['--module'] if a.module else []))
    sys.exit(r.returncode)


if __name__ == '__main__':
    main()
