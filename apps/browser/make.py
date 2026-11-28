"""make.py: browser.app, NetSurf's framebuffer front end for the Colour Maximite 2 G2 and G3 (docs/Web_Browser_Study.md),
with libcurl, mbedTLS and zlib from apps/curl (its make.py, sources and port/) for HTTP and HTTPS.

    python apps/browser/make.py [--gen]

1. host/gen.sh, run in WSL the first time (or with --gen), fetches NetSurf and its libraries at pinned commits, applies
   patches/, builds them natively to make their generated sources, and copies the trees, the generated files, a
   manifest of each component's sources and flags, and the resources for the card into src/ (git ignores it).
2. Each component is compiled with its own flags from the manifest (their headers share names, so no one set of
   include folders serves them all), the paths made Windows ones, into out/obj/<component>.
3. tools/sdk/build.py links them with libcurl's objects into out/browser.app (services version 21, G2 and G3 only).

Changes for the CMM2: libnsfb gains the screen's surface, port/cmm2_surface.c; NetSurf's main is renamed netsurf_main
and called by port/cmm2_main.c; the curl fetcher is added (the native build has no libcurl); the resource path is
/browser/res on the current drive; the text is drawn by FreeType (patches/netsurf-0004) in DejaVu's fonts, from
/browser/fonts. On the card: A:/browser/browser.app, A:/browser/res (out/res: src/res and port/Choices),
A:/browser/fonts (out/fonts: eight DejaVu faces and their licence), A:/browser/cacert.pem.
    CHDIR "A:/browser"
    RUN "browser.app", --shot shot.png https://example.com/
"""
import concurrent.futures, hashlib, importlib.util, json, os, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(HERE, 'src')
OUT = os.path.join(HERE, 'out')
PORT = os.path.join(HERE, 'port')
SDK = os.path.join(REPO, 'tools', 'sdk')

sys.path.insert(0, SDK)
import build                                    # (tools/sdk/build.py: its compiler, flags and staleness test)


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


curl = load(os.path.join(REPO, 'apps', 'curl', 'make.py'), 'curl_make')

LIBS = ['libwapcaplet', 'libparserutils', 'libhubbub', 'libcss', 'libdom', 'libnsbmp', 'libnsgif', 'libnsfb',
        'libnsutils']
LEAVE_OUT = {                                   # sources the native build had that the CMM2 does without
    'libdom': ['bindings/xml/expat_xmlparser.c'],                       # (XML: expat; NetSurf parses HTML)
    'libnsfb': ['src/surface/wld.c'],                                    # (Wayland)
    'netsurf': ['frontends/framebuffer/font_internal.c',                # (the built-in bitmap font: FreeType's
                'build/Linux-framebuffer/font-ns-sans.c'],              # instead)
}
ADD = {                                         # sources added: (file, extra flags)
    'libnsfb': [(os.path.join(PORT, 'cmm2_surface.c'), [])],
    'netsurf': [('frontends/framebuffer/font_freetype.c', []),
                ('content/fetchers/curl.c', []), ('content/handlers/image/png.c', []),
                ('content/handlers/image/jpeg.c', []), (os.path.join(PORT, 'cmm2_main.c'), []),
                (os.path.join(PORT, 'iconv.c'), []), (os.path.join(PORT, 'posix.c'), []),
                (os.path.join(PORT, 'cmm2_download.c'), [])],
}
ZLIB_GZ = ['gzlib.c', 'gzread.c', 'gzclose.c']   # (zlib's gzip file reading, for utils/hashtable.c; apps/curl's zlib)

ARCHIVES = {                                    # what NetSurf's native build went without: folder: (url, SHA-256)
    'libpng-1.6.59': ('https://download.sourceforge.net/libpng/libpng-1.6.59.tar.xz',
                      'd80dd2a38a37f803cb9b6ac7b14bd6e74ddc3b654780a8380bdf93523fdb4389'),
    'jpeg-9f': ('https://www.ijg.org/files/jpegsrc.v9f.tar.gz',                   # (the IJG's own: jconfig.txt is its
                '04705c110cb2469caa79fb71fba3d7bf834914706e9641a4589485c1f832565b'),   # jconfig.h, no CMake)
    'freetype-2.15.0': ('https://download.savannah.gnu.org/releases/freetype/freetype-2.15.0.tar.xz',
                        '93cd47eb719c325107f2d3cf087d06299e944c3b48a40b55537302304dea1dd5'),
    'dejavu-fonts-ttf-2.37': ('https://github.com/dejavu-fonts/dejavu-fonts/releases/download/version_2_37/'
                              'dejavu-fonts-ttf-2.37.tar.bz2',
                              'fa9ca4d13871dd122f61258a80d01751d603b4d3ee14095d65453b4e846e17d7'),
}
PNG = os.path.join(SRC, 'libpng-1.6.59')
JPEG = os.path.join(SRC, 'jpeg-9f')
FT = os.path.join(SRC, 'freetype-2.15.0')
DEJAVU = os.path.join(SRC, 'dejavu-fonts-ttf-2.37')
# FreeType (step 4 of the study): TrueType, hinted and anti-aliased, its options and modules port/freetype's
FT_SOURCES = ['src/base/ftsystem.c', 'src/base/ftinit.c', 'src/base/ftdebug.c', 'src/base/ftbase.c', 'src/base/ftbbox.c',
              'src/base/ftbitmap.c', 'src/base/ftglyph.c', 'src/truetype/truetype.c', 'src/sfnt/sfnt.c',
              'src/smooth/smooth.c', 'src/cache/ftcache.c']
FT_FLAGS = ['-DFT_CONFIG_OPTIONS_H=<ftoption_cmm2.h>', '-DFT_CONFIG_MODULES_H=<ftmodule_cmm2.h>',
            '-I' + os.path.join(PORT, 'freetype'), '-I' + os.path.join(FT, 'include')]
# the fonts on the card (A:/browser/fonts): the faces NetSurf's framebuffer front end asks for by name
# (NETSURF_FB_FONT_*), DejaVu's licence with them
FONTS = ['DejaVuSans.ttf', 'DejaVuSans-Bold.ttf', 'DejaVuSans-Oblique.ttf', 'DejaVuSans-BoldOblique.ttf',
         'DejaVuSerif.ttf', 'DejaVuSerif-Bold.ttf', 'DejaVuSansMono.ttf', 'DejaVuSansMono-Bold.ttf']
PNG_SOURCES = ['png.c', 'pngerror.c', 'pngget.c', 'pngmem.c', 'pngpread.c', 'pngread.c', 'pngrio.c', 'pngrtran.c',
               'pngrutil.c', 'pngset.c', 'pngtrans.c']
JPEG_SOURCES = ['jaricom.c', 'jcomapi.c', 'jdapimin.c', 'jdapistd.c', 'jdarith.c', 'jdatasrc.c', 'jdcoefct.c',
                'jdcolor.c', 'jddctmgr.c', 'jdhuff.c', 'jdinput.c', 'jdmainct.c', 'jdmarker.c', 'jdmaster.c', 'jdmerge.c',
                'jdpostct.c', 'jdsample.c', 'jerror.c', 'jidctflt.c', 'jidctfst.c', 'jidctint.c', 'jmemmgr.c',
                'jmemnobs.c', 'jquant1.c', 'jquant2.c', 'jutils.c']   # (decoding only)


def fetch_archives():
    import tarfile, urllib.request
    for name, (url, sha) in ARCHIVES.items():
        folder = os.path.join(SRC, name)
        if not os.path.isdir(folder):
            arc = os.path.join(SRC, os.path.basename(url))
            if not os.path.exists(arc):
                print('fetching', url, flush=True)
                urllib.request.urlretrieve(url, arc)
            got = hashlib.sha256(open(arc, 'rb').read()).hexdigest()
            if got != sha:
                sys.exit('%s: SHA-256 %s, not %s' % (arc, got, sha))
            with tarfile.open(arc) as t:
                t.extractall(SRC, filter='data')
    for src, dst in ((os.path.join(PNG, 'scripts', 'pnglibconf.h.prebuilt'), os.path.join(PNG, 'pnglibconf.h')),
                     (os.path.join(JPEG, 'jconfig.txt'), os.path.join(JPEG, 'jconfig.h'))):
        if not os.path.exists(dst):
            shutil.copyfile(src, dst)
PER_FILE = {('netsurf', 'frontends/framebuffer/gui.c'): ['-Dmain=netsurf_main']}
NETSURF_DEFINES = {                             # the native build's, changed for the CMM2
    '-DNETSURF_FB_RESPATH': '-DNETSURF_FB_RESPATH="/browser/res"',
    '-DNETSURF_FB_FONTPATH': '-DNETSURF_FB_FONTPATH="/browser/fonts"',
}


def wsl_path(p):
    p = os.path.abspath(p).replace('\\', '/')
    return '/mnt/' + p[0].lower() + p[2:]


def gen(force):
    if os.path.exists(os.path.join(SRC, 'manifest.json')) and not force:
        return
    env = dict(os.environ, MSYS_NO_PATHCONV='1')
    r = subprocess.run(['wsl.exe', 'bash', wsl_path(os.path.join(HERE, 'host', 'gen.sh')), wsl_path(SRC),
                        wsl_path(os.path.join(HERE, 'patches'))], env=env)
    if r.returncode or not os.path.exists(os.path.join(SRC, 'manifest.json')):
        sys.exit('host/gen.sh failed (it runs in WSL: see its output above)')


def flags_for(comp, m):
    """the manifest's flags with the native build's paths made ours, and every library's include folder"""
    out = []
    for f in m[comp]['flags']:
        if f.startswith('-I'):
            p = f[2:].rstrip('/')
            if '/ns-cmm2/inst/' in p or p.startswith('/usr/'):
                continue                        # (installed headers: the libraries' own include folders below)
            if p.startswith('/'):
                p = os.path.join(SRC, p.split('/ns-cmm2/', 1)[1])
            else:
                p = os.path.join(SRC, comp, p)
            out.append('-I' + os.path.normpath(p))
        elif comp == 'netsurf' and f.split('=', 1)[0] in NETSURF_DEFINES:
            out.append(NETSURF_DEFINES[f.split('=', 1)[0]])
        elif f in ('-UWITH_JPEG', '-UWITH_PNG'):
            continue                            # (the native build's: the CMM2 has both, below)
        else:
            out.append(f)
    out += ['-D__CMM2__',                       # (patches/netsurf-0002: utils/config.h's CMM2 section)
            '-I' + os.path.join(SRC, 'include'),                         # (the libraries' headers as installed)
            '-I' + os.path.join(PORT, 'include')]                        # (endian.h, sys/utsname.h)
    if comp == 'netsurf':
        out += ['-DWITH_CURL', '-I' + os.path.join(curl.CURL, 'include'), '-I' + curl.ZLIB,
                '-I' + os.path.join(curl.PORT, 'include'),               # (sys/socket.h, netinet/in.h...: apps/curl's)
                '-DWITH_PNG', '-DWITH_JPEG', '-I' + PNG, '-I' + JPEG,
                '-DFB_USE_FREETYPE'] + FT_FLAGS
    if comp == 'libnsfb':
        out += ['-I' + os.path.join(SRC, 'libnsfb', 'src')]
    return out


def objpath(comp, src):
    return build.objname(os.path.join(OUT, 'obj', comp), src)   # (its name and a hash of its path: two base64.c...)


def compile_all():
    m = json.load(open(os.path.join(SRC, 'manifest.json')))
    jobs, objs = [], []
    units = {}
    for comp in LIBS + ['netsurf']:
        srcs = [(os.path.join(SRC, comp, s), []) for s in m[comp]['sources'] if s not in LEAVE_OUT.get(comp, [])]
        srcs += [(s if os.path.isabs(s) else os.path.join(SRC, comp, s), f) for s, f in ADD.get(comp, [])]
        units[comp] = (srcs, flags_for(comp, m))
    units['curl'] = ([(s, []) for s in curl.sources(tool=False)] +
                     [(os.path.join(curl.ZLIB, f), ['-DZ_HAVE_UNISTD_H', '-DNO_GZCOMPRESS']) for f in ZLIB_GZ] +
                     [(os.path.join(PORT, 'cmm2_ca.c'), [])],           # (the CA bundle parsed once: libcurl's flags)
                     curl.FLAGS + sum([['-I', i] for i in curl.includes()], []))
    units['libpng'] = ([(os.path.join(PNG, f), []) for f in PNG_SOURCES],
                       ['-I' + PNG, '-I' + curl.ZLIB, '-DPNG_ARM_NEON_OPT=0'])   # (no NEON on a Cortex-M7)
    units['libjpeg'] = ([(os.path.join(JPEG, f), []) for f in JPEG_SOURCES], ['-I' + JPEG])
    units['freetype'] = ([(os.path.join(FT, f), []) for f in FT_SOURCES], ['-DFT2_BUILD_LIBRARY'] + FT_FLAGS)
    units['malloc'] = ([(os.path.join(PORT, 'browser_malloc.c'), [])], [])   # (dlmalloc, into MMBasic's heap)
    for comp, (srcs, flags) in units.items():
        odir = os.path.join(OUT, 'obj', comp)
        os.makedirs(odir, exist_ok=True)
        key = hashlib.md5(json.dumps([build.CFLAGS, build.NANO, flags, [e for _, e in srcs], sorted(PER_FILE.items())],
                                     default=str).encode()).hexdigest()   # (per-file flags too)
        keyfile = os.path.join(odir, 'flags.md5')
        same = os.path.exists(keyfile) and open(keyfile).read() == key
        for s, extra in srcs:
            o = objpath(comp, s)
            objs.append(o)
            rel = os.path.relpath(s, os.path.join(SRC, comp)).replace('\\', '/')
            extra = extra + PER_FILE.get((comp, rel), [])
            if not same or build.stale(o):
                cmd = [build.GCC] + build.CFLAGS + build.NANO + flags + extra + ['-MMD', '-MF', o[:-2] + '.d', '-c', s,
                                                                                 '-o', o]
                jobs.append((comp, s, cmd))
        open(keyfile, 'w').write(key)           # (written now: a failed source stays stale by its missing object)
    if jobs:
        print('compiling %d of %d sources' % (len(jobs), len(objs)), flush=True)
    failed = []
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 4) as ex:
        for (comp, s, cmd), r in zip(jobs, ex.map(lambda j: subprocess.run(j[2], capture_output=True, text=True), jobs)):
            msg = (r.stdout + r.stderr).strip()
            if msg:
                print(msg)
            if r.returncode:
                failed.append('%s/%s' % (comp, os.path.basename(s)))
    if failed:
        sys.exit('%d failed: %s' % (len(failed), ' '.join(failed[:30])))
    return objs


def card_files():
    res = os.path.join(OUT, 'res')
    if os.path.isdir(res):
        shutil.rmtree(res)
    shutil.copytree(os.path.join(SRC, 'res'), res)
    shutil.copyfile(os.path.join(PORT, 'Choices'), os.path.join(res, 'Choices'))
    fonts = os.path.join(OUT, 'fonts')
    os.makedirs(fonts, exist_ok=True)
    for f in FONTS:
        shutil.copyfile(os.path.join(DEJAVU, 'ttf', f), os.path.join(fonts, f))
    shutil.copyfile(os.path.join(DEJAVU, 'LICENSE'), os.path.join(fonts, 'LICENSE'))


def main():
    gen('--gen' in sys.argv)
    curl.fetch_all()
    fetch_archives()
    objs = compile_all()
    lst = os.path.join(OUT, 'browser.objects')
    open(lst, 'w').write('\n'.join(objs) + '\n')
    cmd = [sys.executable, '-I', os.path.join(SDK, 'build.py'), os.path.join(OUT, 'browser.app'), '--sources', lst,
           '--api', '21', '--g2', '--pages', '--stack', str(1024 * 1024)] + \
          ['--ldflag=' + f for f in curl.LDFLAGS + ['-Wl,--wrap=free', '-Wl,--wrap=realloc', '-Wl,--wrap=malloc',
                                                    '-Wl,--wrap=calloc', '-Wl,--wrap=socket',
                                                    '-Wl,--wrap=connect', '-Wl,--wrap=closesocket',
                                                    '-Wl,--wrap=getsockopt', '-Wl,--wrap=getaddrinfo',
                                                    '-Wl,--wrap=close']]
    # (--pages: a G2 or G3 lends the display memory not shown, as heap2, the big blocks' pool in port/browser_malloc.c.
    # A 1 MB stack: NetSurf's layout recurses deeply. malloc, calloc, realloc and free wrapped: port/browser_malloc.c
    # notes their callers, so that a request not met, or a block given back that was never given out, names its code.
    # socket, connect, closesocket, getsockopt and getaddrinfo wrapped: port/cmm2_main.c's --cmm2-netlog. close
    # wrapped: port/posix.c, so that NetSurf's close() of a socket closes it)
    subprocess.run(cmd, check=True)
    card_files()


if __name__ == '__main__':
    main()
