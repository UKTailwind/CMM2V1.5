"""make.py: builds apps/wordgrinder/out/wordgrinder.app, David Given's WordGrinder 0.8 word processor (MIT) on the CMM2's
curses (apps/pdcurses: PDCursesMod with a CMM2 back end). WordGrinder is fetched the first time into
apps/wordgrinder/wordgrinder at its 0.8 release, and zlib (Jean-loup Gailly and Mark Adler, zlib licence) into
apps/wordgrinder/zlib at 1.3.1 (git ignores both); patches/*.patch are applied to WordGrinder in order.

All of it is built as WordGrinder's own build.lua builds a static Unix binary with its curses front end: its C, the
Lua 5.1.5, lpeg, LuaBitOp, minizip and uthash it carries, and its Lua scripts as a table in the program
(out/luascripts.c, as its tools/multibin2c.lua makes it). zlib is built in, as the system's is not there. Its Lua
code is told it runs on "windows" (ARCH), whose file browser takes a drive letter and matches names whatever their
case, as FAT does; cmm2_main.c gives it the variables that need (Peter, 12 Nov 2026: docs/Office_Apps_Plan.md).

Built without --vram, so that a G1 keeps its modes above 960 x 540 (about 4.5 MB for the program there).

    python apps/wordgrinder/make.py
then put out/sd/wordgrinder on the SD card as A:/wordgrinder (wordgrinder.app, and WordGrinder's British and
American/Canadian dictionaries, from SCOWL, with SCOWL's licence) and, at the CMM2's prompt:
    RUN "A:/wordgrinder/wordgrinder.app" [, document.wg] [mode=N] [font=WxH]
A G2 holds a whole dictionary, in memory MMBasic's heap lends to WordGrinder's own malloc (wg_malloc.c: dlmalloc, Doug
Lea's, public domain, in dlmalloc.c); cmm2_main.c names the British one at the first start (Peter, 12 Nov 2026), and a
document's spelling is turned on in its settings.
"""
import glob, hashlib, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, 'apps', 'pdcurses'))
import pdcurses

SRC = os.path.join(HERE, 'wordgrinder')
URL = 'https://github.com/davidgiven/wordgrinder.git'
COMMIT = 'ada51b4d4192fcfaf53f778f126b81e9b901ad94'          # the 0.8 release, 23 Oct 2020
ZLIB = os.path.join(HERE, 'zlib')
ZLIB_URL = 'https://github.com/madler/zlib.git'
ZLIB_COMMIT = '51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf'     # 1.3.1
DICTIONARIES = ('british.dictionary', 'american-canadian.dictionary')
OUT = os.path.join(HERE, 'out')
VERSION, FILEFORMAT = '0.8', '8'                             # (WordGrinder's Makefile's)

# build.lua's list, in its order: each runs as it is loaded, and later ones use what earlier ones made
SCRIPTS = ['_prologue', 'events', 'main', 'xml', 'utils', 'redraw', 'settings', 'document', 'forms', 'ui', 'browser',
           'html', 'margin', 'xpattern', 'fileio', 'export', 'export/text', 'export/html', 'export/latex',
           'export/troff', 'export/opendocument', 'export/markdown', 'import', 'import/html', 'import/text',
           'import/opendocument', 'import/markdown', 'navigate', 'addons/goto', 'addons/autosave',
           'addons/docsetman', 'addons/scrapbook', 'addons/statusbar_charstyle', 'addons/statusbar_pagecount',
           'addons/statusbar_position', 'addons/statusbar_wordcount', 'addons/debug', 'addons/look-and-feel',
           'addons/keymapoverride', 'addons/smartquotes', 'addons/undo', 'addons/spillchocker', 'addons/templates',
           'addons/directories', 'addons/recents', 'menu', 'cli', 'lunamark/util', 'lunamark/entities',
           'lunamark/markdown']

LUA = ['lapi', 'lauxlib', 'lbaselib', 'lcode', 'ldblib', 'ldebug', 'ldo', 'ldump', 'lfunc', 'lgc', 'linit', 'liolib',
       'llex', 'lmathlib', 'lmem', 'loadlib', 'lobject', 'lopcodes', 'loslib', 'lparser', 'lstate', 'lstring',
       'lstrlib', 'ltable', 'ltablib', 'ltm', 'lundump', 'lvm', 'lzio', 'winshim']
ZLIB_FILES = ['adler32', 'compress', 'crc32', 'deflate', 'inffast', 'inflate', 'inftrees', 'trees', 'uncompr', 'zutil']


def fetch(path, url, commit):
    if not os.path.isdir(path):                 # (that commit alone: a whole history will not unpack in a Dropbox folder)
        subprocess.run(['git', 'init', '-q', path], check=True)
        subprocess.run(['git', '-C', path, 'fetch', '-q', '--depth', '1', url, commit], check=True)
        subprocess.run(['git', '-C', path, 'checkout', '-q', 'FETCH_HEAD'], check=True)
    head = subprocess.run(['git', '-C', path, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != commit:                          # (first: in a folder with a broken .git, git -C falls through to the CMM2 repository)
        sys.exit('%s is at %s, not %s' % (path, head, commit))


def patch():
    """the patches applied to WordGrinder's files, once: again (from the commit) only when they change"""
    patches = sorted(glob.glob(os.path.join(HERE, 'patches', '*.patch')))
    want = hashlib.sha1(b''.join(open(p, 'rb').read() for p in patches)).hexdigest()
    stamp = os.path.join(SRC, '.cmm2-patches')
    if os.path.exists(stamp) and open(stamp).read().strip() == want:
        return
    subprocess.run(['git', '-C', SRC, 'checkout', '-q', '--', '.'], check=True)
    for p in patches:
        subprocess.run(['git', '-C', SRC, 'apply', p], check=True)
    open(stamp, 'w').write(want + '\n')


def luascripts():
    """out/luascripts.c: the scripts as tools/multibin2c.lua makes them (script_table), each named as build.lua names it"""
    lines = ['#include "globals.h"']
    names = []
    for i, s in enumerate(SCRIPTS, 1):
        name = 'src/lua/%s.lua' % s
        data = open(os.path.join(SRC, name), 'rb').read()
        names.append(name)
        lines.append('')
        lines.append('/* This is %s */' % name)
        lines.append('static const char file_%d[] = {' % i)
        lines += [', '.join(str(b) for b in data[j:j + 16]) + ', ' for j in range(0, len(data), 16)]
        lines.append('};')
    lines.append('const FileDescriptor script_table[] = {')
    lines += ['{ file_%d, sizeof(file_%d), "%s" },' % (i, i, n) for i, n in enumerate(names, 1)]
    lines.append('{ NULL, 0 }')
    lines.append('};')
    text = '\n'.join(lines) + '\n'
    path = os.path.join(OUT, 'luascripts.c')
    if not os.path.exists(path) or open(path).read() != text:
        open(path, 'w', newline='\n').write(text)
    return path


def wg_main():
    """out/wg_main.c: WordGrinder's main.c, its main renamed wg_main for cmm2_main.c to call (as its Windows build
    renames it appMain). Only there: build.py builds every source, crt0.c too, with the same flags"""
    path = os.path.join(OUT, 'wg_main.c')
    text = '/* made by make.py: WordGrinder\'s src/c/main.c, its main renamed */\n#define main wg_main\n#include "main.c"\n'
    if not os.path.exists(path) or open(path).read() != text:
        open(path, 'w', newline='\n').write(text)
    return path


def main():
    fetch(SRC, URL, COMMIT)
    fetch(ZLIB, ZLIB_URL, ZLIB_COMMIT)
    patch()
    pdcurses.prepare()
    os.makedirs(OUT, exist_ok=True)
    c = os.path.join(SRC, 'src', 'c')
    emu = os.path.join(c, 'emu')
    srcs = [os.path.join(c, f + '.c') for f in ('utils', 'filesystem', 'zip', 'lua', 'word', 'screen')]
    srcs.append(wg_main())
    srcs.append(luascripts())
    srcs += [os.path.join(emu, 'wcwidth.c'), os.path.join(emu, 'luabitop', 'lua-bitop.c'), os.path.join(emu, 'tmpnam.c')]
    srcs += [os.path.join(emu, 'lua-5.1.5', f + '.c') for f in LUA]
    srcs.append(os.path.join(c, 'arch', 'unix', 'cursesw', 'dpy.c'))
    srcs += [os.path.join(emu, 'minizip', f + '.c') for f in ('ioapi', 'zip', 'unzip')]
    srcs += [os.path.join(emu, 'lpeg', f + '.c') for f in ('lpvm', 'lpcap', 'lptree', 'lpcode', 'lpprint')]
    srcs += [os.path.join(ZLIB, f + '.c') for f in ZLIB_FILES]
    srcs.append(os.path.join(HERE, 'cmm2_main.c'))
    srcs.append(os.path.join(HERE, 'wg_malloc.c'))              # (which includes dlmalloc.c)
    srcs += pdcurses.sources()
    listfile = os.path.join(OUT, 'sources.txt')
    open(listfile, 'w').write('\n'.join(srcs) + '\n')
    defines = ['-DVERSION="%s"' % VERSION, '-DFILEFORMAT=' + FILEFORMAT, '-DNOUNCRYPT', '-DNOCRYPT', '-DEMULATED_WCWIDTH',
               '-DNDEBUG', '-DWINSHIM', '-DLUA_USE_EMU_TMPNAM', '-DARCH="windows"',
               '-DUSE_FILE32API', '-DWG_GC_PAUSE=110', '-DWG_POLL_HOOK=PDC_cmm2_poll', '-Wno-unused-function']
    cmd = [sys.executable, os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(OUT, 'wordgrinder.app'),
           '--sources', listfile, '--api', '12', '--stack', '131072',
           '-I', c, '-I', os.path.join(emu, 'lua-5.1.5'), '-I', os.path.join(emu, 'luabitop'),
           '-I', os.path.join(emu, 'uthash'), '-I', os.path.join(emu, 'minizip'), '-I', os.path.join(emu, 'lpeg'),
           '-I', ZLIB] + ['--cflag=' + d for d in defines] + pdcurses.flags()
    r = subprocess.run(cmd).returncode
    if r == 0:
        card()
    sys.exit(r)


def card():
    """out/sd/wordgrinder, the SD card's A:/wordgrinder: the program, the dictionaries WordGrinder ships for Windows
    (extras/), and SCOWL's licence for them (licenses/COPYING.Scowl)"""
    import shutil
    sd = os.path.join(OUT, 'sd', 'wordgrinder')
    os.makedirs(sd, exist_ok=True)
    shutil.copy(os.path.join(OUT, 'wordgrinder.app'), sd)
    for f in DICTIONARIES:
        shutil.copy(os.path.join(SRC, 'extras', f), sd)
    shutil.copy(os.path.join(SRC, 'licenses', 'COPYING.Scowl'), sd)
    print('%s: %s' % (sd, ' '.join(sorted(os.listdir(sd)))))


if __name__ == '__main__':
    main()
