"""pdcurses.py: PDCursesMod for the CMM2's loadable programs, shared by the apps that use curses (apps/wordgrinder,
apps/scim) and by make.py here, which builds PDCursesMod's own demos.

Bill Gray's PDCursesMod (public domain) is fetched the first time into apps/pdcurses/PDCursesMod at the commit below
(git ignores it), and patches/*.patch applied to it in order, if there are any. Its own code is built as it is: the
library (pdcurses/*.c) and, of its Linux framebuffer port (fb/), the drawing (pdcdisp.c: each character drawn into
an 8-bit page, bold, italic, underline and the rest made from the one font, and the mouse pointer), the screen size,
the settings and the clipboard. cmm2/ is the rest of a port, on the loader's services: the screen (MODE, the palette,
the fonts), the keys and the mouse, the time. The fonts are Spleen's (Frederic Cambus, BSD-2-Clause), fetched into
apps/pdcurses/spleen, made here into PSF2 fonts with their Unicode tables, as PDCursesMod's psf.c reads them.

    import pdcurses                         (sys.path having apps/pdcurses)
    pdcurses.prepare()                      the fetches, the patches and out/cmm2_fonts.c: once, then when they change
    pdcurses.sources()                      the C files to build, the fonts' among them
    pdcurses.flags()                        build.py's options: -I and the defines (wide characters as UTF-8, the mouse)

Peter, 12 Nov 2026 (docs/Office_Apps_Plan.md). PDCursesMod's own files are public domain; cmm2/ is too.
"""
import glob, hashlib, os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
PDC = os.path.join(HERE, 'PDCursesMod')
PDC_URL = 'https://github.com/Bill-Gray/PDCursesMod.git'
PDC_COMMIT = 'd494b49c29706802a0095d64333a45a3295c89a0'       # 4.5.4, 30 Sep 2026
SPLEEN = os.path.join(HERE, 'spleen')
SPLEEN_URL = 'https://github.com/fcambus/spleen.git'
SPLEEN_COMMIT = '5116f8eac7f65da2cfa169626ec43b5d041665fe'    # as apps/vt100's
OUT = os.path.join(HERE, 'out')
FONTS_C = os.path.join(OUT, 'cmm2_fonts.c')

# the fonts, in cmm2_fonts[]'s order (cmm2/pdcscrn.c picks by name, PDC_FONT, or by the screen's width)
SIZES = [(6, 12), (8, 16), (12, 24), (16, 32)]

# fb/ files built as they are (the others are cmm2/'s)
FB_FILES = ['pdcdisp.c', 'pdcgetsc.c', 'pdcsetsc.c', 'pdcclip.c']


def fetch(path, url, commit):
    if not os.path.isdir(path):                 # (that commit alone: a whole history will not unpack in a Dropbox folder)
        subprocess.run(['git', 'init', '-q', path], check=True)
        subprocess.run(['git', '-C', path, 'fetch', '-q', '--depth', '1', url, commit], check=True)
        subprocess.run(['git', '-C', path, 'checkout', '-q', 'FETCH_HEAD'], check=True)
    head = subprocess.run(['git', '-C', path, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    if head != commit:                          # (first: in a folder with a broken .git, git -C falls through to the CMM2 repository)
        sys.exit('%s is at %s, not %s' % (path, head, commit))


def patch():
    """the patches applied to PDCursesMod's files, once: again (from the commit) only when they change"""
    patches = sorted(glob.glob(os.path.join(HERE, 'patches', '*.patch')))
    want = hashlib.sha1(b''.join(open(p, 'rb').read() for p in patches)).hexdigest()
    stamp = os.path.join(PDC, '.cmm2-patches')
    if os.path.exists(stamp) and open(stamp).read().strip() == want:
        return
    subprocess.run(['git', '-C', PDC, 'checkout', '-q', '--', '.'], check=True)
    for p in patches:
        subprocess.run(['git', '-C', PDC, 'apply', p], check=True)
    open(stamp, 'w').write(want + '\n')


def read_bdf(path, w, h):
    """{code: h rows of w-bit integers, the left pixel the top bit} (as apps/vt100's make.py reads Spleen)"""
    text = open(path, encoding='latin-1').read()
    box = re.search(r'FONTBOUNDINGBOX (\d+) (\d+) (-?\d+) (-?\d+)', text)
    fx, fy = int(box.group(3)), int(box.group(4))
    out = {}
    for m in re.finditer(r'ENCODING (-?\d+).*?BBX (\d+) (\d+) (-?\d+) (-?\d+).*?BITMAP\n(.*?)ENDCHAR', text, re.S):
        code, bw, bh, bx, by = (int(m.group(i)) for i in range(1, 6))
        if code < 0:
            continue
        rows = [0] * h
        data = m.group(6).split()
        top = h - (by - fy) - bh                # the glyph's first row in the cell
        for i, hexrow in enumerate(data):
            bits = int(hexrow, 16) >> (len(hexrow) * 4 - bw)
            if 0 <= top + i < h:
                shift = w - bw - (bx - fx)
                rows[top + i] = (bits << shift if shift >= 0 else bits >> -shift) & ((1 << w) - 1)
        out[code] = rows
    return out


def scan_line(w, h, n):
    """the VT100's horizontal line at scan n of 9 (U+23BA-23BD: curses's ACS_S1, S3, S7 and S9), which Spleen lacks"""
    rows = [0] * h
    rows[min(h - 1, (2 * n - 1) * h // 18)] = (1 << w) - 1
    return rows


def block(w, h, x0, y0, x1, y1):
    """the part of the cell from (x0, y0) to (x1, y1), fractions of it, set"""
    rows = [0] * h
    for y in range(round(y0 * h), round(y1 * h)):
        for x in range(round(x0 * w), round(x1 * w)):
            rows[y] |= 1 << (w - 1 - x)
    return rows


def shade(w, h, n):
    """U+2591-2593: a quarter, half or three quarters of the pixels set"""
    rows = [0] * h
    for y in range(h):
        for x in range(w):
            if (n == 2 and (x + y) % 2 == 0) or (n == 1 and x % 2 == 0 and y % 2 == 0) or \
               (n == 3 and not (x % 2 == 0 and y % 2 == 0)):
                rows[y] |= 1 << (w - 1 - x)
    return rows


def poly(w, h, pts, filled):
    """a polygon in the cell, its corners as fractions of it: filled, or its outline a pixel wide"""
    p = [(x * (w - 1), y * (h - 1)) for x, y in pts]
    rows = [0] * h

    def put(x, y):
        if 0 <= x < w and 0 <= y < h:
            rows[y] |= 1 << (w - 1 - x)
    if filled:
        for y in range(h):
            for x in range(w):
                inside = False
                for i in range(len(p)):
                    (x1, y1), (x2, y2) = p[i], p[i - 1]
                    if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
                        inside = not inside
                if inside:
                    put(x, y)
    for i in range(len(p)):                     # the edges, so that a small shape keeps its corners
        (x1, y1), (x2, y2) = p[i - 1], p[i]
        n = int(max(abs(x2 - x1), abs(y2 - y1))) + 1
        for j in range(n + 1):
            put(round(x1 + (x2 - x1) * j / n), round(y1 + (y2 - y1) * j / n))
    return rows


# shapes WordGrinder draws (its document's ends, its menus' and file browser's marks) that some sizes lack
SHAPES = {0x25B2: ([(.5, .25), (.85, .7), (.15, .7)], True), 0x25BC: ([(.15, .3), (.85, .3), (.5, .75)], True),
          0x25B6: ([(.2, .2), (.8, .5), (.2, .8)], True), 0x25BA: ([(.2, .3), (.8, .5), (.2, .7)], True),
          0x25B7: ([(.2, .2), (.8, .5), (.2, .8)], False), 0x25C7: ([(.5, .2), (.85, .5), (.5, .8), (.15, .5)], False)}

# characters curses draws with (its ACS and WACS, and fb/pdcdisp.c's cursors: the half and full blocks, the square)
# that a Spleen size lacks: made, or another of its glyphs used, as ncurses falls back for a terminal without them
DOUBLES = {0x2550: 0x2500, 0x2551: 0x2502}
for _lo, _single in ((0x2552, 0x250C), (0x2555, 0x2510), (0x2558, 0x2514), (0x255B, 0x2518), (0x255E, 0x251C),
                     (0x2561, 0x2524), (0x2564, 0x252C), (0x2567, 0x2534), (0x256A, 0x253C)):
    for _i in range(3):
        DOUBLES[_lo + _i] = _single
STANDINS = {0x2190: '<', 0x2191: '^', 0x2192: '>', 0x2193: 'v', 0x3C0: '*', 0x2264: '<', 0x2265: '>', 0x25AA: 0x25A0,
            0x2013: '-', 0x2014: '-', 0x2018: "'", 0x2019: "'", 0x201C: '"', 0x201D: '"', 0x2022: 0xB7, 0x2026: '_'}


def complete(g, w, h):
    made = {0x2580: block(w, h, 0, 0, 1, .5), 0x2584: block(w, h, 0, .5, 1, 1), 0x2588: block(w, h, 0, 0, 1, 1),
            0x258C: block(w, h, 0, 0, .5, 1), 0x2590: block(w, h, .5, 0, 1, 1), 0x25A0: block(w, h, .25, .25, .75, .75),
            0x2591: shade(w, h, 1), 0x2592: shade(w, h, 2), 0x2593: shade(w, h, 3), 0x2594: block(w, h, 0, 0, 1, .125)}
    for cp, rows in made.items():
        g.setdefault(cp, rows)
    for cp, (pts, filled) in SHAPES.items():
        if cp not in g:
            g[cp] = poly(w, h, pts, filled)
    if 0x2260 not in g and ord('=') in g and ord('/') in g:     # not equal: = with / through it
        g[0x2260] = [a | b for a, b in zip(g[ord('=')], g[ord('/')])]
    for cp, to in list(DOUBLES.items()) + list(STANDINS.items()):
        to = ord(to) if isinstance(to, str) else to
        if cp not in g and to in g:
            g[cp] = g[to]


def psf2(glyphs, w, h):
    """a PSF2 font with its Unicode table: one glyph per code, in code order"""
    codes = sorted(glyphs)
    bpr = (w + 7) // 8
    data = bytearray()
    for c in codes:
        for r in glyphs[c]:
            data += (r << (8 * bpr - w)).to_bytes(bpr, 'big')
    table = bytearray()
    for c in codes:
        table += chr(c).encode('utf-8') + b'\xff'
    head = (b'\x72\xb5\x4a\x86' + (0).to_bytes(4, 'little') + (32).to_bytes(4, 'little') + (1).to_bytes(4, 'little') +
            len(codes).to_bytes(4, 'little') + (bpr * h).to_bytes(4, 'little') + h.to_bytes(4, 'little') +
            w.to_bytes(4, 'little'))
    return bytes(head + data + table)


def make_fonts():
    os.makedirs(OUT, exist_ok=True)
    lines = ['/* cmm2_fonts.c: made by apps/pdcurses/pdcurses.py from Spleen (Frederic Cambus, BSD-2-Clause: its LICENSE).',
             '   Each a PSF2 font with its Unicode table, as PDCursesMod\'s psf.c reads one. Do not edit. */',
             '#include "cmm2_fonts.h"', '']
    table = []
    for w, h in SIZES:
        g = read_bdf(os.path.join(SPLEEN, 'spleen-%dx%d.bdf' % (w, h)), w, h)
        for n, cp in ((1, 0x23BA), (3, 0x23BB), (7, 0x23BC), (9, 0x23BD)):
            g.setdefault(cp, scan_line(w, h, n))
        complete(g, w, h)
        b = psf2(g, w, h)
        name = 'font_%dx%d' % (w, h)
        lines.append('static const unsigned char %s[%d] = {' % (name, len(b)))
        for i in range(0, len(b), 24):
            lines.append('    ' + ','.join('%d' % x for x in b[i:i + 24]) + ',')
        lines.append('};')
        lines.append('')
        table.append('    {%d, %d, "%dx%d", %s, sizeof(%s)},' % (w, h, w, h, name, name))
    lines.append('const struct cmm2_font cmm2_fonts[] = {')
    lines += table
    lines.append('};')
    lines.append('const int cmm2_n_fonts = %d;' % len(SIZES))
    text = '\n'.join(lines) + '\n'
    if not os.path.exists(FONTS_C) or open(FONTS_C).read() != text:
        open(FONTS_C, 'w', newline='\n').write(text)


def prepare():
    fetch(PDC, PDC_URL, PDC_COMMIT)
    fetch(SPLEEN, SPLEEN_URL, SPLEEN_COMMIT)
    patch()
    make_fonts()


def sources():
    out = sorted(glob.glob(os.path.join(PDC, 'pdcurses', '*.c')))
    out += [os.path.join(PDC, 'fb', f) for f in FB_FILES]
    out += sorted(glob.glob(os.path.join(HERE, 'cmm2', '*.c')))
    out.append(FONTS_C)
    return out


def flags():
    """build.py's options for every source of a program that uses PDCurses"""
    return ['-I', PDC, '-I', os.path.join(HERE, 'cmm2'), '-I', os.path.join(HERE, 'cmm2', 'include'),
            '--cflag=-DPDC_WIDE', '--cflag=-DPDC_FORCE_UTF8', '--cflag=-DHAVE_MOUSE']
    # (a program written for ncurses's mouse calls, MEVENT and getmouse(&event), adds --cflag=-DPDC_NCMOUSE: sc-im)
