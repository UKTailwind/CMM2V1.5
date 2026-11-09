"""guide.py [loadable|pc386|cc|apps]: a companion manual, made in the user manual's template: Colour Maximite 2 Loadable
Programs (docs/loadable/, the default), Colour Maximite 2 386 PC Emulator (docs/pc386/), Colour Maximite 2 C Compiler
(docs/cc/) or Colour Maximite 2 App User Guide (docs/apps/). Its title page, styles,
page size and footer, with the guide's own front matter, a table of contents and a chapter for each file of the
guide's text/ (written by chapter.py --docx --rawcode). Run it again after an edit; then tools/manual/word.ps1
docs/loadable/Colour_Maximite_2_Loadable_Programs.docx docs/loadable/...pdf (or the pc386 guide's) makes the PDF.

The skeleton is made afresh each time from docs/manual/Colour_Maximite_2_User_Manual.docx:
- the title page's text replaced (TITLE below), its links taken out;
- the front matter cut to three of its headings, About This Guide, Support and Copyright, with the guide's text;
- the contents a new TOC field (Word fills it: word.ps1);
- the body emptied, then a Heading 1 for each chapter, copied from the manual's first (it starts a page);
- "Colour Maximite 2 User Manual" in the footers made the guide's name."""
import copy, glob, os, subprocess, sys
import docx
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_skeleton import set_text, ptext

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MANUAL = os.path.join(ROOT, 'docs', 'manual', 'Colour_Maximite_2_User_Manual.docx')
DIR = os.path.join(ROOT, 'docs', 'loadable')
OUT = os.path.join(DIR, 'Colour_Maximite_2_Loadable_Programs.docx')
NAME = 'Colour Maximite 2 Loadable Programs'
FIRMWARE = '7.00.00RC4'
DATE = '9 November 2026'

TITLE = {                                               # the title page: its paragraphs by their start
    'User Manual': 'Loadable Programs',
    'MMBasic Ver': 'Programs in C and C++',
    'Matches firmware': 'For firmware %s (services version 16)' % FIRMWARE,
    'Geoff Graham': 'Peter Mather',
    'For updates to this manual': 'The firmware, this guide and the tools that build a program are at',
    'http://geoffg.net': 'https://github.com/UKTailwind/CMM2V1.5',
}
ABOUT = ('This guide is for programmers who want to write programs in C or C++ for the Colour Maximite 2, or to port '
         'existing software to it, as loadable programs: files ending .APP that MMBasic runs with RUN "name.app". It '
         'covers the tools that build a program, how the firmware loads and runs one, the services that the firmware '
         'gives it, and the experience of porting MicroPython, Doom and ScummVM. It assumes a working knowledge of C '
         'and of the MMBasic user manual.')
SUPPORT = ('Questions should be raised on the Back Shed forum ( http://www.thebackshed.com/forum/Microcontrollers ), and '
           'problems with the tools or the services reported as issues of the repository above.')
COPYRIGHT = ('This guide is copyright 2026 Peter Mather and is distributed under a Creative Commons '
             'Attribution-NonCommercial-ShareAlike 3.0 Australia license (CC BY-NC-SA 3.0), as the user manual is. The '
             'files of the tools (tools/sdk) are under the MIT licence, so a program built with them can have any licence '
             'its author chooses (see Appendix C).')
SUBJECT = 'Loadable programs for the Colour Maximite 2'
TITLE_ADD = {}                                          # a line put on the title page before the paragraph that starts so

PC386 = {                                               # guide.py pc386: the 386 PC's guide, for users
    'DIR': os.path.join(ROOT, 'docs', 'pc386'),
    'OUT': os.path.join(ROOT, 'docs', 'pc386', 'Colour_Maximite_2_386_PC_Emulator.docx'),
    'NAME': 'Colour Maximite 2 386 PC Emulator',
    'DATE': '5 November 2026',
    'TITLE': {
        'User Manual': '386 PC Emulator',
        'MMBasic Ver': 'MS-DOS, FreeDOS and Windows 3.1 on the Colour Maximite 2',
        'Matches firmware': 'For firmware with loadable-program services version 9',
        'Geoff Graham': 'Port and guide: Peter Mather',
        'For updates to this manual': 'The firmware, the emulator and this guide are at',
        'http://geoffg.net': 'https://github.com/UKTailwind/CMM2V1.5',
    },
    'ABOUT': ('This guide is for users of pc386, a 386 PC that the Colour Maximite 2 runs as a loadable program. It '
              'covers the files the emulator needs, starting it, its keys, the configuration file that describes each '
              'PC, disk images, and installing MS-DOS and Windows 3.1 from their floppy disks. It assumes you can use '
              'the MMBasic command prompt and copy files to the SD card.'),
    'SUPPORT': ('Questions should be raised on the Back Shed forum ( http://www.thebackshed.com/forum/Microcontrollers ), '
                'and problems with the emulator reported as issues of the repository above.'),
    'COPYRIGHT': ('This guide is copyright 2026 Peter Mather and is distributed under a Creative Commons '
                  'Attribution-NonCommercial-ShareAlike 3.0 Australia license (CC BY-NC-SA 3.0), as the user manual is. '
                  'The emulator is tiny386, by Chunhui He, under the BSD 3-Clause licence; its other parts, the BIOS '
                  'files and FreeDOS have their own licences (see Appendix A).'),
    'SUBJECT': 'A 386 PC emulator for the Colour Maximite 2',
    'TITLE_ADD': {'Geoff Graham': 'Emulator: tiny386, by Chunhui He'},   # (his work, credited above the guide's author)
}


def style_of(el, sid):
    ppr = el.find(qn('w:pPr'))
    st = ppr.find(qn('w:pStyle')) if ppr is not None else None
    return sid.get(st.get(qn('w:val'))) if st is not None else None


def toc_paragraph():
    p = OxmlElement('w:p')
    for kind, text in (('begin', None), (None, ' TOC \\o "1-2" \\h \\z \\u '), ('separate', None),
                       (None, None), ('end', None)):
        r = OxmlElement('w:r')
        if kind:
            fc = OxmlElement('w:fldChar')
            fc.set(qn('w:fldCharType'), kind)
            r.append(fc)
        elif text:
            it = OxmlElement('w:instrText')
            it.set(qn('xml:space'), 'preserve')
            it.text = text
            r.append(it)
        else:
            t = OxmlElement('w:t')
            t.text = '(the contents: updated by Word)'
            r.append(t)
        p.append(r)
    return p


def skeleton(titles):
    d = docx.Document(MANUAL)
    sid = {s.style_id: s.name for s in d.styles}
    body = d.element.body
    els = list(body.iterchildren())
    contents = next(i for i, e in enumerate(els) if ptext(e) == 'Contents')
    first_h1 = next(i for i, e in enumerate(els) if e.tag == qn('w:p') and style_of(e, sid) == 'Heading 1')
    h1 = copy.deepcopy(els[first_h1])
    for b in list(h1.iter(qn('w:bookmarkStart'))) + list(h1.iter(qn('w:bookmarkEnd'))):
        b.getparent().remove(b)
    # the title page
    for e in els[:contents]:
        t = ptext(e)
        for start, extra in TITLE_ADD.items():
            if t.startswith(start):
                line = copy.deepcopy(e)
                set_text(line, extra)
                e.addprevious(line)
        for start, new in TITLE.items():
            if t.startswith(start):
                set_text(e, new)
        if t.startswith('and ') and 'mmbasic.com' in t:
            body.remove(e)
        elif t.startswith('(') and t.endswith(')') and t[1:2].isdigit():   # the date, "(2 November 2026)"
            set_text(e, '(' + DATE + ')')
    # the front matter: About, Support and Copyright, each heading with one paragraph after it
    front = [e for e in els[:contents] if e.getparent() is not None]
    keep = {}
    for i, e in enumerate(front):
        t = ptext(e)
        if t in ('About', 'Support', 'Copyright and Acknowledgments'):
            body_p = next(x for x in front[i + 1:] if ptext(x))
            keep[t] = (e, body_p)
    start = front.index(keep['About'][0])
    for e in front[start:]:
        if not any(e is a or e is b for a, b in keep.values()) and ptext(e):
            body.remove(e)
    set_text(keep['About'][0], 'About This Guide')
    set_text(keep['About'][1], ABOUT)
    set_text(keep['Support'][1], SUPPORT)
    set_text(keep['Copyright and Acknowledgments'][0], 'Copyright')
    set_text(keep['Copyright and Acknowledgments'][1], COPYRIGHT)
    # the contents and the body
    els = list(body.iterchildren())
    contents = next(i for i, e in enumerate(els) if ptext(e) == 'Contents')
    sect = els[-1]                                       # (the body's sectPr)
    for e in els[contents + 1:-1]:
        body.remove(e)
    anchor = els[contents]
    blank = OxmlElement('w:p')
    anchor.addnext(blank)
    toc = toc_paragraph()
    blank.addnext(toc)
    anchor = toc
    for t in titles:
        h = copy.deepcopy(h1)
        set_text(h, t)
        anchor.addnext(h)
        anchor = h
    # the footers
    for s in d.sections:
        for part in (s.footer, s.first_page_footer, s.even_page_footer, s.header):
            for p in part.paragraphs:
                for r in p.runs:
                    if 'Colour Maximite 2 User Manual' in r.text:
                        r.text = r.text.replace('Colour Maximite 2 User Manual', NAME)
    d.core_properties.title = NAME
    d.core_properties.subject = SUBJECT
    d.save(OUT)
    assert sect.tag == qn('w:sectPr')

CC = {                                                  # guide.py cc: the C compiler's guide, for users
    'DIR': os.path.join(ROOT, 'docs', 'cc'),
    'OUT': os.path.join(ROOT, 'docs', 'cc', 'Colour_Maximite_2_C_Compiler.docx'),
    'NAME': 'Colour Maximite 2 C Compiler',
    'DATE': '7 November 2026',
    'TITLE': {
        'User Manual': 'C Compiler',
        'MMBasic Ver': 'Compiling C and MMBasic programs on the Colour Maximite 2',
        'Matches firmware': 'For firmware 7.00.00RC2 and later',
        'Geoff Graham': 'Port and guide: Peter Mather',
        'For updates to this manual': 'The firmware, the compiler and this guide are at',
        'http://geoffg.net': 'https://github.com/UKTailwind/CMM2V1.5',
    },
    'ABOUT': ('This guide is for users of the CC command, which compiles programs written in C, and MMBasic programs, on '
              'the Colour Maximite 2 itself. It covers installing the compiler, compiling and running a program, the '
              'language and library, what is and is not translated from MMBasic, and the speed to expect. It assumes you '
              'can use the MMBasic command prompt and copy files to the SD card.'),
    'SUPPORT': ('Questions should be raised on the Back Shed forum ( http://www.thebackshed.com/forum/Microcontrollers ), '
                'and problems with the compiler reported as issues of the repository above.'),
    'COPYRIGHT': ('This guide is copyright 2026 Peter Mather and is distributed under a Creative Commons '
                  'Attribution-NonCommercial-ShareAlike 3.0 Australia license (CC BY-NC-SA 3.0), as the user manual is. '
                  'The compiler is the Fuzix Compiler Kit, under the GNU General Public License version 3; its other '
                  'parts have their own licences (see Appendix A).'),
    'SUBJECT': 'The C compiler and MMBasic translator for the Colour Maximite 2',
    'TITLE_ADD': {'Geoff Graham': 'Compiler: the Fuzix Compiler Kit'},
}


APPS = {                                                # guide.py apps: all the programs of the zip, for users
    'DIR': os.path.join(ROOT, 'docs', 'apps'),
    'OUT': os.path.join(ROOT, 'docs', 'apps', 'Colour_Maximite_2_App_User_Guide.docx'),
    'NAME': 'Colour Maximite 2 App User Guide',
    'DATE': '9 November 2026',
    'TITLE': {
        'User Manual': 'App User Guide',
        'MMBasic Ver': 'The programs that come with the Colour Maximite 2',
        'Matches firmware': 'For firmware 7.00.00RC3 and later',
        'Geoff Graham': 'Peter Mather',
        'For updates to this manual': 'The firmware, the programs and this guide are at',
        'http://geoffg.net': 'https://github.com/UKTailwind/CMM2V1.5',
    },
    'ABOUT': ('This guide is for anyone using the programs that come with the Colour Maximite 2: MicroPython, Doom, '
              'Heretic, Quake, ScummVM, the NES, ZX Spectrum, Amstrad CPC, Commodore 64, Atari 8-bit and Apple //e emulators, '
              'the 386 PC, the C compiler, the VT100 terminal and zeptoforth. It covers installing each one, starting it, its '
              'keys and leaving it. It assumes you can use the MMBasic command prompt and copy files to the SD card.'),
    'SUPPORT': ('Questions should be raised on the Back Shed forum ( http://www.thebackshed.com/forum/Microcontrollers ), '
                'and problems with the programs reported as issues of the repository above.'),
    'COPYRIGHT': ('This guide is copyright 2026 Peter Mather and is distributed under a Creative Commons '
                  'Attribution-NonCommercial-ShareAlike 3.0 Australia license (CC BY-NC-SA 3.0), as the user manual is. '
                  'The programs have their own licences (see Appendix A).'),
    'SUBJECT': 'The programs that come with the Colour Maximite 2',
    'TITLE_ADD': {},
}


def main():
    which = sys.argv[1] if len(sys.argv) > 1 else 'loadable'
    if which == 'pc386':
        globals().update(PC386)
    elif which == 'cc':
        globals().update(CC)
    elif which == 'apps':
        globals().update(APPS)
    elif which != 'loadable':
        sys.exit('guide.py [loadable|pc386|cc|apps]')
    os.makedirs(DIR, exist_ok=True)
    files = sorted(glob.glob(os.path.join(DIR, 'text', '*.md')))
    titles = [open(f, encoding='utf-8').readline()[2:].strip() for f in files]
    skeleton(titles)
    r = subprocess.run([sys.executable, '-I', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'chapter.py'),
                        '--docx', OUT, '--rawcode'] + (['--keep'] if which == 'apps' else []) + files)
    sys.exit(r.returncode)


if __name__ == '__main__':
    main()
