"""make.py: builds apps/zeptoforth/out/zeptoforth.app, zeptoforth (Travis Bemann's Forth for the Cortex-M, MIT licence) as a loadable program: a proof of concept, the kernel
alone, on a G2. The kernel is fetched the first time into apps/zeptoforth/zeptoforth (git ignores it) at the commit below, patches/*.patch applied to it in order.

Its STM32F746 port is the nearest to the CMM2, a Cortex-M7, and the kernel is assembled from it, with the files in port/ found first: what a port supplies (the memory, the
console, the flash layer, the top file), as the rules for a program need it. variables.s and expose.s come from the F746's own folder, as does the search that finds
../common and ../m4_m7. zf_cmm2.c starts it and calls the services for it. zf_services.c is the Forth words' way to the services, and comes after the kernel on the link line
so that changing it moves no kernel word and a saved image stays good.

    python apps/zeptoforth/make.py
then put out/zeptoforth.app, or on a G1 out/zeptoforth_g1.app, in A:/zeptoforth, and at the CMM2's prompt:
    CHDIR "A:/zeptoforth"
    RUN "zeptoforth.app"
It starts as a bare kernel: forth/setup.fs is the library, typed in by a loader that waits for each line's ACK, then save-image keeps it as zeptoforth.img or zeptoforth_g1.img.
    python apps/zeptoforth/make.py package
gathers what goes in the programs' zip.
"""
import glob, hashlib, importlib.util, os, shutil, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'zeptoforth')
URL = 'https://github.com/tabemann/zeptoforth.git'
COMMIT = 'b8991416484a73fd6cb4069fa5d3d8ab2dc864d9'   # 1 Nov 2026
VERSION = '1.17.2-dev'


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


def kernel_info():
    """src/common/kernel_info.s, as its Makefile writes it: the kernel's platform, version and date"""
    out = []
    for word, text in (('kernel-platform', 'cmm2'), ('kernel-version', VERSION), ('kernel-date', time.strftime('%d %b %Y'))):
        label = '_' + word.replace('-', '_')
        out += ['        define_word "%s", visible_flag' % word, label + ':', '        push {lr}', '        string "%s"' % text,
                '        pop {pc}', '        end_inlined', '']
    open(os.path.join(SRC, 'src', 'common', 'kernel_info.s'), 'w', newline='\n').write('\n'.join(out))


def vt100():
    """apps/vt100's make.py: its terminal is the console, with libvterm and the fonts it fetches and makes"""
    spec = importlib.util.spec_from_file_location('vt100_make', os.path.join(REPO, 'apps', 'vt100', 'make.py'))
    vt = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(vt)
    vt.fetch(vt.VTERM, vt.VTERM_URL, vt.VTERM_COMMIT)
    vt.fetch(vt.SPLEEN, vt.SPLEEN_URL, vt.SPLEEN_COMMIT)
    vt.patch()
    os.makedirs(vt.OUT, exist_ok=True)
    vt.make_fonts()
    return vt


def build(vt, out, flags, first=[]):
    """the kernel first on the link line, so that changing the C moves no kernel word and a saved image stays good"""
    after = [os.path.join(HERE, f) for f in ('zf_cmm2.c', 'zf_services.c', 'zf_console.c')] + [s for s in vt.sources() if not s.endswith('vt100.c')]
    cmd = [sys.executable, '-I', os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', out)] + first + [os.path.join(HERE, 'port', 'zeptoforth.s')] + after + \
          ['--api', '13', '--stack', '0x10000', '-I', os.path.join(HERE, 'port'), '-I', os.path.join(SRC, 'src', 'stm32f746'),
           '-I', os.path.join(REPO, 'apps', 'vt100'), '-I', os.path.join(vt.VTERM, 'include'), '-I', os.path.join(vt.VTERM, 'src')] + flags
    return subprocess.run(cmd).returncode


def package():
    """out/package, as it goes in the programs' zip: what a user needs of this folder, and its licence. The saved images are made on a board of each kind (setup.fs, then save-image)
    and fetched from its card to out/ first; README.txt is the zip's README's section"""
    top = os.path.join(HERE, 'out', 'package')
    shutil.rmtree(top, ignore_errors=True)
    folder, licences = os.path.join(top, 'zeptoforth'), os.path.join(top, 'licences')
    os.makedirs(folder, exist_ok=True)              # (Dropbox may hold on to a folder just emptied)
    os.makedirs(licences, exist_ok=True)
    for f in ('out/zeptoforth.app', 'out/zeptoforth_g1.app', 'out/zeptoforth.img', 'out/zeptoforth_g1.img', 'WORDS.md', 'forth/float64_v.fs', 'test/float64_test.fs'):
        if not os.path.exists(os.path.join(HERE, f)):
            sys.exit('%s is not there' % f)
        shutil.copy(os.path.join(HERE, f), folder)
    shutil.copy(os.path.join(SRC, 'LICENSE'), os.path.join(licences, 'zeptoforth-LICENSE.txt'))
    print('out/package:', ' '.join(sorted(os.listdir(folder))))


def main():
    if sys.argv[1:] == ['package']:
        package()
        return
    if not os.path.isdir(SRC):                  # (that commit alone: the whole history will not unpack in a Dropbox folder)
        subprocess.run(['git', 'init', '-q', SRC], check=True)
        subprocess.run(['git', '-C', SRC, 'fetch', '-q', '--depth', '1', URL, COMMIT], check=True)
        subprocess.run(['git', '-C', SRC, 'checkout', '-q', 'FETCH_HEAD'], check=True)
    head = subprocess.run(['git', '-C', SRC, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != COMMIT:
        sys.exit('%s is at %s, not %s' % (SRC, head, COMMIT))
    patch()
    kernel_info()
    os.makedirs(os.path.join(HERE, 'out'), exist_ok=True)
    vt = vt100()
    r = build(vt, 'zeptoforth.app', ['--g2', '--heap', str(0x680000)])
    if r == 0:                                  # (a G1: linked in its heap, and moved there by zf_g1.c)
        r = build(vt, 'zeptoforth_g1.app', ['--base', '0xD0400000', '--fixed', '--heap', str(0x100000), '--cflag=-DZF_G1',
                                            '--cflag=-Wa,--defsym,ZF_G1=1', '--ldflag=-Wl,-e,zf_g1_start'], [os.path.join(HERE, 'zf_g1.c')])
    sys.exit(r)


if __name__ == '__main__':
    main()
