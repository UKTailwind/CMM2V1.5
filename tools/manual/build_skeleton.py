"""build_skeleton.py: step M2 of docs/Manual_Plan.md. Makes docs/manual/Colour_Maximite_2_User_Manual.docx from the
PicoMite manual (the base, M-D1):

- the title page as Peter decided (M-D3): Colour Maximite 2, MMBasic Ver 7.00.00 alpha 1, the firmware it matches,
  Geoff Graham, "Updated for MMBasic 7.00 by Peter Mather";
- the chapters in the order of docs/manual/audit/chapters.md (CHAPTERS below). A chapter kept or merged keeps the
  PicoMite manual's text; one to be replaced or written new is its heading only; dropped chapters go;
- a to-do note under each chapter heading, saying what M3/M4 do to it. Every note starts with TODO (below) and is
  highlighted, so that it is seen in the document and tools/manual/check.py can count what is left;
- the footer's "PicoMite User Manual" as "Colour Maximite 2 User Manual".

The table of contents is a field, updated by Word (tools/manual/word.ps1). Run once: after M2 the manual is edited
in place, a change at a time."""
import copy, os, re, sys
import docx
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PICO = r'D:\Dropbox\PicoMite\PicoMite\docs\PicoMite_User_Manual.docx'
OUT = os.path.join(ROOT, 'docs', 'manual', 'Colour_Maximite_2_User_Manual.docx')
TODO = '[MANUAL TODO] '
DASH = '\u2013'

# (new heading, the PicoMite chapter it starts from or None, action, what to do: docs/manual/audit/chapters.md)
CHAPTERS = [
    ('Introduction', None, 'replace', 'from the b14 manual p7: what the CMM2 is, and what 7.00 brings.'),
    ('Quick Start Tutorial', None, 'replace', 'from the b14 manual p8.'),
    ('Colour Maximite 2 Generation 1', None, 'replace', 'from the b14 manual p10; the mouse interface (PS/2 and USB).'),
    ('Colour Maximite 2 Generation 2', None, 'replace', 'from the b14 manual p12.'),
    ('Hardware Features', None, 'replace', 'from the b14 manual p14; the USB socket (hub, mice, gamepads, serial, '
     'flash drive; no low-speed device behind a hub).'),
    ('Serial Console', 'Serial Console', 'merge', 'with the b14 manual p14 (the connector); the default speed is 460800.'),
    ('Using MMBasic', 'Using MMBasic', 'merge', 'with the b14 manual p19: programs run from the SD card, the status '
     'line, timing.'),
    ('Full Screen Editor', 'Full Screen Editor', 'new', 'the PicoMite text as the base, with the CMM2\'s keys and '
     'features (docs/Editor_Port_Plan.md, E9).'),
    ('File Manager', 'File Manager', 'keep', 'with the CMM2\'s differences (parity plan P4).'),
    ('Variables and Expressions', 'Variables and Expressions', 'keep', 'checked against the build.'),
    ('Subroutines and Functions', 'Subroutines and Functions', 'keep', 'checked against the build.'),
    ('Video Modes and Pages', None, 'new', 'MODE 1-18 and colour depths, pages, MODE n,4 with FRAMEBUFFER, image '
     'slots, the PicoMite mode map (b14 p56 and p12).'),
    ('Keyboard, Mouse and Gamepad', 'Keyboard/Mouse/Gamepad', 'merge', 'the USB sections for the CMM2 (mouse channel '
     '2, gamepads 3-4, COM4-COM7); the b14 manual\'s mouse channels and Wii controllers; without the PS/2 keyboard '
     'and the on-screen keyboard.'),
    ('Program and Data Storage', 'Program and Data Storage', 'merge', 'with the b14 manual p43: A: (and B:), C:, '
     'the program flash and LIBRARY; no flash slots or flash filesystem.'),
    ('Sound Output', 'Sound Output', 'merge', 'with the b14 manual p47: the DAC output; without PWM, VS1053, MCP48n2 '
     'and Bluetooth.'),
    ('Using the I/O Pins', 'Using the I/O pins', 'merge', 'with the b14 manual p49: the CMM2 connector and pins.'),
    ('Special Device Support', None, 'replace', 'from the b14 manual p52.'),
    ('Graphics Functions', 'Graphics Functions', 'merge', 'with the b14 manual p38.'),
    ('GUI Controls', 'GUI Integration', 'merge', 'with the b14 manual p38 (GUI Controls); CTRLVAL( and MSGBOX( are in '
     'the build.'),
    ('Game Playing Features', None, 'replace', 'from the b14 manual p56, with USB gamepads, TILEMAP and image slots.'),
    ('Long Strings', 'Long Strings', 'keep', 'checked against the build.'),
    ('Porting Programs', None, 'merge', 'the b14 manual p58, and a new section: running PicoMite programs on the CMM2.'),
    ('MMBasic Characteristics', 'MMBasic Characteristics', 'merge', 'with the b14 manual p63: the memory map, brought '
     'up to date.'),
    ('Predefined Read Only Variables', 'Predefined Read Only Variables', 'reference', 'entry by entry from '
     'docs/manual/audit/keywords.md (M4).'),
    ('Operators', None, 'replace', 'from the b14 manual p70.'),
    ('Options', 'Options', 'reference', 'entry by entry from docs/manual/audit/keywords.md (M4).'),
    ('Commands', 'Commands', 'reference', 'entry by entry from docs/manual/audit/keywords.md (M4).'),
    ('Functions', 'Functions', 'reference', 'entry by entry from docs/manual/audit/keywords.md (M4).'),
    ('Obsolete Commands and Functions', 'Obsolete Commands and Functions', 'reference', 'entry by entry (M4).'),
    ('Appendix A ' + DASH + ' Serial Communications', 'Appendix A', 'merge', 'with the b14 manual p155: COM1-COM3 on '
     'the connector, COM4-COM7 on USB.'),
    ('Appendix B ' + DASH + ' I2C Communications', None, 'replace', 'from the b14 manual p158.'),
    ('Appendix C ' + DASH + ' 1-Wire Communications', None, 'replace', 'from the b14 manual p160.'),
    ('Appendix D ' + DASH + ' SPI Communications', None, 'replace', 'from the b14 manual p161.'),
    ('Appendix E ' + DASH + ' Regular Expressions', 'Appendix E', 'merge', 'with the b14 manual p170.'),
    ('Appendix F ' + DASH + ' Sprites', None, 'replace', 'from the b14 manual p163 (the CMM2\'s BLIT and SPRITE).'),
    ('Appendix G ' + DASH + ' Turtle Graphics', None, 'replace', 'from the b14 manual p165 (the build has the '
     'CMM2\'s turtle.c).'),
    ('Appendix H ' + DASH + ' Cyclic Redundancy Check (CRC)', None, 'replace', 'from the b14 manual p167.'),
    ('Appendix I ' + DASH + ' Special Keyboard Keys', None, 'replace', 'from the b14 manual p172, with the codes of '
     'KeyboardMap.c.'),
    ('Appendix J ' + DASH + ' CAN Support', None, 'replace', 'from the b14 manual p173.'),
    ('Appendix K ' + DASH + ' Loading the Firmware', None, 'replace', 'from the b14 manual p177.'),
    ('Appendix L ' + DASH + ' Programming in BASIC - A Tutorial', 'Appendix J', 'keep', 'checked against the build.'),
    ('Appendix M ' + DASH + ' Firmware Applicability', None, 'new', 'the CMM2\'s closed list of labels '
     '(docs/Manual_Plan.md).'),
    ('Appendix N ' + DASH + ' Where Each Keyword Is Documented', None, 'new', 'generated from the reference sections.'),
    ('Appendix O ' + DASH + ' Companion Manuals', 'Appendix M', 'keep', 'only those that apply to the build.'),
]
DROPPED = ('Introduction', 'Firmware Versions and Files', 'First Steps', 'Hardware Details', 'Calculator Immediate Mode', 'Video Output',
           'Special Device Support', 'Display Panels', 'WiFi and Internet Functions', 'Bluetooth Versions',
           'Appendix B', 'Appendix C', 'Appendix D', 'Appendix F', 'Appendix G', 'Appendix H', 'Appendix I',
           'Appendix K', 'Appendix L')
ACTION = {'replace': 'To be written', 'new': 'To be written', 'merge': 'To be merged', 'keep': 'To be checked',
          'reference': 'To be audited'}


def ptext(el):
    return ''.join(t.text or '' for t in el.iter(qn('w:t'))).strip()


def set_text(p, text):
    """a paragraph's text replaced, keeping its first run's formatting"""
    runs = p.findall(qn('w:r'))
    for r in runs[1:]:
        p.remove(r)
    for h in p.findall(qn('w:hyperlink')):
        p.remove(h)
    if not runs:
        r = OxmlElement('w:r')
        p.append(r)
        runs = [r]
    for t in runs[0].findall(qn('w:t')):
        runs[0].remove(t)
    t = OxmlElement('w:t')
    t.text = text
    t.set(qn('xml:space'), 'preserve')
    runs[0].append(t)


def note(text):
    """a highlighted to-do paragraph"""
    p = OxmlElement('w:p')
    r = OxmlElement('w:r')
    rpr = OxmlElement('w:rPr')
    hl = OxmlElement('w:highlight')
    hl.set(qn('w:val'), 'yellow')
    rpr.append(hl)
    r.append(rpr)
    t = OxmlElement('w:t')
    t.text = TODO + text
    t.set(qn('xml:space'), 'preserve')
    r.append(t)
    p.append(r)
    return p


def main():
    d = docx.Document(PICO)
    sid = {s.style_id: s.name for s in d.styles}
    body = d.element.body
    els = list(body.iterchildren())
    final = els[-1]

    def is_h1(el):
        if el.tag != qn('w:p'):
            return False
        ppr = el.find(qn('w:pPr'))
        st = ppr.find(qn('w:pStyle')).get(qn('w:val')) if ppr is not None and ppr.find(qn('w:pStyle')) is not None else ''
        return sid.get(st) == 'Heading 1'

    starts = [i for i, el in enumerate(els[:-1]) if is_h1(el)]
    pio = next(i for i, el in enumerate(els) if i > starts[0] and ptext(el).startswith('Appendix F ' + DASH))
    starts = sorted(starts + [pio])         # (F's heading is unstyled; its first line after the contents, whose own
                                            # "Appendix F" line must stay: it holds part of the contents field)
    front = els[:starts[0]]
    chapters = {}
    for n, i in enumerate(starts):
        j = starts[n + 1] if n + 1 < len(starts) else len(els) - 1
        chapters[ptext(els[i])] = els[i:j]

    def chapter(prefix):
        hits = [k for k in chapters if k == prefix or k.startswith(prefix + ' ' + DASH) or k.startswith(prefix + ' \u2014')]
        if len(hits) != 1:
            sys.exit('no single PicoMite chapter for %r: %r' % (prefix, hits))
        return hits[0]

    template_h1 = copy.deepcopy(chapters[chapter('Introduction')][0])
    for b in list(template_h1.iter(qn('w:bookmarkStart'))) + list(template_h1.iter(qn('w:bookmarkEnd'))):
        b.getparent().remove(b)
    used = set()
    for el in els:
        body.remove(el)
    # the title page (M-D3)
    title = {'PicoMite': 'Colour Maximite 2', 'Ver 7.0.00': 'MMBasic Ver 7.00.00 alpha 1',
             'MMBasic BASIC Interpreter': '', 'For the': 'Matches firmware 7.00.00b1',
             'Raspberry Pi Pico': '', 'Raspberry Pi Pico 2': '', 'Raspberry Pi Pico W': 'Geoff Graham',
             'http://geoffg.net/picomite.html': 'http://geoffg.net/maximite.html'}
    for el in front[:24]:
        t = ptext(el)
        if t in title:
            set_text(el, title[t])
        elif t.startswith('Raspberry Pi Pico 2 W'):
            set_text(el, 'Updated for MMBasic 7.00 by Peter Mather')
        elif t.startswith('(13 September 2026)'):
            set_text(el, '(20 October 2026)')
    for el in front:
        body.append(el)
        if ptext(el) == 'About':
            body.append(note('The About, Support and Copyright sections: the CMM2\'s from the b14 manual p2, with '
                             'Gerry Allardice\'s b14 work and the libraries this build uses (TinyUSB, FatFs R0.16, '
                             'dr_libs, cJSON).'))
    for head, src, action, what in CHAPTERS:
        if src:
            key = chapter(src)
            used.add(key)
            part = chapters[key]
            set_text(part[0], head)
            body.append(part[0])
            body.append(note('%s: %s' % (ACTION[action], what)))
            for el in part[1:]:
                body.append(el)
        else:
            h = copy.deepcopy(template_h1)
            set_text(h, head)
            body.append(h)
            body.append(note('%s: %s' % (ACTION[action], what)))
    for k in DROPPED:
        used.add(chapter(k))
    left = [k for k in chapters if k not in used]
    if left:
        sys.exit('PicoMite chapters neither placed nor dropped: %r' % left)
    body.append(final)
    # the footer, in every section's footers and headers
    for s in d.sections:
        for part in (s.footer, s.first_page_footer, s.even_page_footer, s.header, s.first_page_header):
            for t in part._element.iter(qn('w:t')):
                if t.text and 'PicoMite User Manual' in t.text:
                    t.text = t.text.replace('PicoMite User Manual', 'Colour Maximite 2 User Manual')
    d.core_properties.title = 'Colour Maximite 2 User Manual'
    d.core_properties.subject = 'MMBasic Ver 7.00.00 alpha 1'
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    d.save(OUT)
    print('written', OUT, '-', len(CHAPTERS), 'chapters,', len(DROPPED), 'PicoMite chapters dropped')


if __name__ == '__main__':
    main()
