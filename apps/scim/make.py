"""make.py: builds apps/scim/out/scim.app, sc-im (Andres Martinelli's spreadsheet, from Nathan Klemm's and the other sc
programs, BSD 4-clause) on the CMM2's curses (apps/pdcurses: PDCursesMod with a CMM2 back end). sc-im is fetched the
first time into apps/scim/sc-im at the commit below (git ignores it); patches/*.patch are applied to it in order.

Built as its src/Makefile builds it: every .c of src/ and its folders actions, cmds, formats and utils, and gram.c. The
grammar's files (gram.c, y.tab.h, experres.h, statres.h) are in apps/scim/gen, made by gen.sh in WSL with bison, again
only when gram.y or the sed scripts change, so that a build needs no bison.

sc-im reads and writes UTF-8 (Peter, 12 Nov 2026), so it is built with the Arm GNU Toolchain (the same GCC 13.3),
whose newlib converts UTF-8 (_MB_CAPABLE), and with the whole newlib (--libc full: swprintf and the rest): the folder
in SCIM_TOOLCHAIN_DIR, else the Arm GNU Toolchain's usual one. The other apps keep STM32CubeIDE's toolchain.
compat/ has what sc-im includes that a loadable program has not: curses.h (ncurses's mouse calls for sc-im alone),
dlfcn.h (no shared libraries: its C triggers say so) and sys/ioctl.h (included, not used).
Left out, as the plan says (docs/Office_Apps_Plan.md): threads and with them the autobackup, gnuplot, XLS, XLSX, ODS
and Lua. A shell command (the pager, an editor, the clipboard, plots) cannot run on the CMM2.

    python apps/scim/make.py
then put out/sd/scim on the SD card as A:/scim and, at the CMM2's prompt:
    RUN "A:/scim/scim.app" [, budget.sc] [mode=N] [font=WxH]
"""
import glob, hashlib, os, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, 'apps', 'pdcurses'))
import pdcurses

SRC = os.path.join(HERE, 'sc-im')
URL = 'https://github.com/andmarti1424/sc-im.git'
COMMIT = '8cd391b19d4e17ca9a4cdb5e412f67ac587f2257'          # main, 28 Oct 2026 (the last tag is v0.8.5)
GEN = os.path.join(HERE, 'gen')
OUT = os.path.join(HERE, 'out')
TOOLCHAIN = os.environ.get('SCIM_TOOLCHAIN_DIR', r'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\13.3 rel1\bin')

# POSIX regex for sc-im's searches (regcomp, regexec): newlib ships <regex.h> but builds its code (Henry Spencer's, BSD
# 3-clause, in libc/posix) only for Cygwin and the like. So the code matching that header is fetched from the newlib
# the toolchain has, 4.4.0 (its tag's commit), into apps/scim/regex (git ignores it), each file checked by its hash.
REGEX = os.path.join(HERE, 'regex')
REGEX_URL = ('https://sourceware.org/git/?p=newlib-cygwin.git;a=blob_plain;f=newlib/libc/posix/%s;'
             'hb=26f7004bf73c421c3fd5e5a6ccf470d05337b435')
REGEX_FILES = {
    'regcomp.c': '0e6b2b661de5cce2f483f96043eaf79f3ca9e3cc107431cf5c7d22cf8f86e5bc',
    'regexec.c': '08291ea51b16c225bbc934d77350cba2f3c8566766becaa08b50d344406b9394',
    'regerror.c': '8c0c67142f65bd676cf98a0efedf02eaa9a928cd3354af0167d2bdaa15ccf41e',
    'regfree.c': '9732fc0265b7b4a3f2fdcc091525da5ccf8bbe6b0b1eed44da51a997f0827d53',
    'engine.c': '7f7bdd5e079b33c9e1d9165e22ab60805adab0e10316180e2752f3cfede38841',
    'collcmp.c': 'ae45fd0428c5857a54f829f459faeeb49764c50fcd9b7f95b18123d23311f37a',
    'cclass.h': '18b957a837986f0277c2d996e9e4219e2daa00c17e934f7518e17bca09ec5627',
    'cname.h': 'b9b60a3dc4e397afe38426511960c84b83175b1cb4dbdae383109df1020ad94f',
    'regex2.h': '45a705c03dac4f187f2c96bb28ddfd8956774509e32cc94aa8176cf23dd28829',
    'utils.h': 'f5c44445db8db15454bb3a0f2daf607aad9c0bfec3df1a6b31183510385b4374',
    'collate.h': '0d2b88ae99eaa0bfccdd46e47b8e41cac723a896aa0ff8cad336b8316d6fe567',
}


def fetch_regex():
    import urllib.request
    os.makedirs(REGEX, exist_ok=True)
    for f, want in REGEX_FILES.items():
        path = os.path.join(REGEX, f)
        if os.path.exists(path) and hashlib.sha256(open(path, 'rb').read()).hexdigest() == want:
            continue
        data = urllib.request.urlopen(REGEX_URL % f, timeout=60).read()
        if hashlib.sha256(data).hexdigest() != want:
            sys.exit('newlib\'s %s is not the file expected' % f)
        open(path, 'wb').write(data)


def fetch(path, url, commit):
    if not os.path.isdir(path):                 # (that commit alone: a whole history will not unpack in a Dropbox folder)
        subprocess.run(['git', 'init', '-q', path], check=True)
        subprocess.run(['git', '-C', path, 'fetch', '-q', '--depth', '1', url, commit], check=True)
        subprocess.run(['git', '-C', path, 'checkout', '-q', 'FETCH_HEAD'], check=True)
    head = subprocess.run(['git', '-C', path, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != commit:                          # (first: in a folder with a broken .git, git -C falls through to the CMM2 repository)
        sys.exit('%s is at %s, not %s' % (path, head, commit))


def patch():
    """the patches applied to sc-im's files, once: again (from the commit) only when they change"""
    patches = sorted(glob.glob(os.path.join(HERE, 'patches', '*.patch')))
    want = hashlib.sha1(b''.join(open(p, 'rb').read() for p in patches)).hexdigest()
    stamp = os.path.join(SRC, '.cmm2-patches')
    if os.path.exists(stamp) and open(stamp).read().strip() == want:
        return
    subprocess.run(['git', '-C', SRC, 'checkout', '-q', '--', '.'], check=True)
    for p in patches:
        subprocess.run(['git', '-C', SRC, 'apply', p], check=True)
    open(stamp, 'w').write(want + '\n')


def wsl_path(p):
    p = os.path.abspath(p).replace('\\', '/')
    return '/mnt/%s%s' % (p[0].lower(), p[2:]) if p[1] == ':' else p


def grammar():
    """gen/: the grammar's files, made again (gen.sh in WSL) only when what they are made from changes"""
    src = os.path.join(SRC, 'src')
    want = hashlib.sha1(b''.join(open(os.path.join(src, f), 'rb').read() for f in ('gram.y', 'eres.sed', 'sres.sed'))
                        ).hexdigest()
    stamp = os.path.join(GEN, 'made-from.sha1')
    if os.path.exists(stamp) and open(stamp).read().strip() == want:
        return
    os.makedirs(GEN, exist_ok=True)
    env = dict(os.environ, MSYS_NO_PATHCONV='1')
    subprocess.run(['wsl', '-e', 'bash', wsl_path(os.path.join(HERE, 'gen.sh')), wsl_path(src), wsl_path(GEN)],
                   check=True, env=env)
    open(stamp, 'w', newline='\n').write(want + '\n')


def grammar_headers():
    """the grammar's headers copied beside sc-im's sources, where they include them from (cmds/cmds.c: ../y.tab.h)"""
    for f in ('y.tab.h', 'experres.h', 'statres.h'):
        data = open(os.path.join(GEN, f), 'rb').read()
        dst = os.path.join(SRC, 'src', f)
        if not os.path.exists(dst) or open(dst, 'rb').read() != data:
            open(dst, 'wb').write(data)


def scim_main():
    """out/scim_main.c: sc-im's main.c, its main renamed scim_main for cmm2_main.c to call. Only there: build.py builds
    every source, crt0.c too, with the same flags"""
    path = os.path.join(OUT, 'scim_main.c')
    text = '/* made by make.py: sc-im\'s src/main.c, its main renamed */\n#define main scim_main\n#include "main.c"\n'
    if not os.path.exists(path) or open(path).read() != text:
        open(path, 'w', newline='\n').write(text)
    return path


def main():
    fetch(SRC, URL, COMMIT)
    patch()
    grammar()
    grammar_headers()
    fetch_regex()
    pdcurses.prepare()
    os.makedirs(OUT, exist_ok=True)
    src = os.path.join(SRC, 'src')
    srcs = sorted(f for f in glob.glob(os.path.join(src, '*.c')) if os.path.basename(f) != 'main.c')
    for d in ('actions', 'cmds', 'formats', 'utils'):
        srcs += sorted(glob.glob(os.path.join(src, d, '*.c')))
    srcs += [os.path.join(GEN, 'gram.c'), scim_main(), os.path.join(HERE, 'cmm2_main.c'),
             os.path.join(HERE, 'compat', 'compat.c')]
    srcs += [os.path.join(REGEX, f) for f in ('regcomp.c', 'regexec.c', 'regerror.c', 'regfree.c', 'collcmp.c')]
    srcs += pdcurses.sources()
    listfile = os.path.join(OUT, 'sources.txt')
    open(listfile, 'w').write('\n'.join(srcs) + '\n')
    defines = ['-DNCURSES', '-D_XOPEN_SOURCE_EXTENDED', '-D_GNU_SOURCE', '-DUSECOLORS', '-DUNDO', '-DMAXROWS=65536',
               '-DUSELOCALE', '-DMOUSE', '-DNO_WORDEXP',
               '-DSNAME="sc-im"', '-DHELP_PATH=getenv("HOME")', '-DLIBDIR=""',
               '-DDFLT_PAGER=""', '-DDFLT_EDITOR=""',
               '-DHISTORY_FILE="sc-iminfo"', '-DHISTORY_DIR="."', '-DINS_HISTORY_FILE="sc-iminfo"',
               '-DCONFIG_FILE="scimrc"', '-DCONFIG_DIR="."',
               '-DDEFAULT_COPY_TO_CLIPBOARD_CMD=""', '-DDEFAULT_PASTE_FROM_CLIPBOARD_CMD=""',
               '-DDEFAULT_OPEN_FILE_UNDER_CURSOR_CMD=""']
    cmd = [sys.executable, os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(OUT, 'scim.app'),
           '--sources', listfile, '--api', '12', '--libc', 'full', '--stack', '131072',
           '-I', os.path.join(HERE, 'compat'), '-I', src] + ['--cflag=' + d for d in defines] + pdcurses.flags()
    env = dict(os.environ, ARM_TOOLCHAIN_DIR=TOOLCHAIN)
    r = subprocess.run(cmd, env=env).returncode
    if r == 0:
        card()
    sys.exit(r)


def card():
    """out/sd/scim, the SD card's A:/scim: the program, and sc-im's help file under the name it looks for"""
    sd = os.path.join(OUT, 'sd', 'scim')
    os.makedirs(sd, exist_ok=True)
    shutil.copy(os.path.join(OUT, 'scim.app'), sd)
    shutil.copy(os.path.join(SRC, 'src', 'doc'), os.path.join(sd, 'sc-im_help'))
    print('%s: %s' % (sd, ' '.join(sorted(os.listdir(sd)))))


if __name__ == '__main__':
    main()
