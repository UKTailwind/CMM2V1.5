"""extract.py "Chapter heading" OUT.md: a chapter of the manual as the text chapter.py writes (docs/Manual_Plan.md, M3).

For a chapter to be merged or checked: its present text (the PicoMite manual's) comes out as docs/manual/text/NN.md,
is edited there, and goes back with chapter.py. What it reads:
- Heading 2 and 3, paragraphs with their bold, italic and code (Courier New, Lucida Console, Consolas) runs;
- a paragraph all in a code font is code; consecutive ones make a block;
- a numbered or bulleted paragraph is a bullet;
- a paragraph with tabs is a ">>" line (an indented list such as the shortcut keys);
- a table is a pipe table with its column widths;
- a picture (inline or floating) is saved in docs/manual/images/ and referred to with its width;
- a text box, which the format cannot hold, becomes a !TODO note with its text, to be placed by hand.
A [MANUAL TODO] note becomes a !TODO line again."""
import os, re, sys
import docx
from docx.oxml.ns import qn

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MANUAL = os.path.join(ROOT, 'docs', 'manual', 'Colour_Maximite_2_User_Manual.docx')
IMAGES = os.path.join(ROOT, 'docs', 'manual', 'images')
CODE_FONTS = ('Courier New', 'Lucida Console', 'Consolas', 'Courier')
A = '{http://schemas.openxmlformats.org/drawingml/2006/main}'
WP = '{http://schemas.openxmlformats.org/drawingml/2006/wordprocessingDrawing}'
R = '{http://schemas.openxmlformats.org/officeDocument/2006/relationships}'


def font(r):
    rpr = r.find(qn('w:rPr'))
    f = rpr.find(qn('w:rFonts')) if rpr is not None else None
    return (f.get(qn('w:ascii')) or f.get(qn('w:hAnsi')) or '') if f is not None else ''


def flag(r, tag):
    rpr = r.find(qn('w:rPr'))
    if rpr is None:
        return False
    el = rpr.find(qn(tag))
    return el is not None and el.get(qn('w:val')) not in ('0', 'false')


def runs_of(p):
    """the paragraph's runs, those inside hyperlinks and smart tags too, in order"""
    for el in p.iter(qn('w:r')):
        if el.getparent().tag in (qn('w:p'), qn('w:hyperlink'), qn('w:smartTag'), qn('w:fldSimple'),
                                  qn('w:ins'), qn('w:sdtContent')):
            yield el


def run_text(r):
    out = []
    for el in r:
        if el.tag == qn('w:t'):
            out.append(el.text or '')
        elif el.tag == qn('w:tab'):
            out.append('\t')
        elif el.tag in (qn('w:br'), qn('w:cr')):
            out.append(' ')
    return ''.join(out)


def inline(p, code_all=None):
    """the paragraph's text with **bold**, *italic* and `code` marks; (text, all its text is code)"""
    parts, codechars, allchars = [], 0, 0
    for r in runs_of(p):
        t = run_text(r)
        if not t:
            continue
        allchars += len(t.strip())
        f = font(r)
        if any(f.startswith(c) for c in CODE_FONTS):
            codechars += len(t.strip())
            parts.append(('code', t))
        elif flag(r, 'w:b'):
            parts.append(('b', t))
        elif flag(r, 'w:i'):
            parts.append(('i', t))
        else:
            parts.append(('', t))
    is_code = allchars > 0 and codechars == allchars
    out = []
    for kind, t in parts:                               # (marks only round the words, not their spaces)
        if is_code or kind == '' or not t.strip():
            out.append(t)
            continue
        lead, core, trail = re.match(r'^(\s*)(.*?)(\s*)$', t, re.S).groups()
        mark = {'b': '**', 'i': '*', 'code': '`'}[kind]
        out.append(lead + mark + core + mark + trail)
    s = ''.join(out)
    s = s.replace('****', '').replace('``', '')         # (two marked runs side by side become one)
    return s, is_code


class Out:
    def __init__(self, slug):
        self.lines, self.code, self.slug, self.nimg = [], [], slug, 0

    def flush_code(self):
        if self.code:
            self.lines += ['```'] + self.code + ['```', '']
            self.code = []

    def add(self, s):
        self.flush_code()
        self.lines += [s, '']

    def picture(self, d, el):
        for blip in el.iter(A + 'blip'):
            rid = blip.get(R + 'embed')
            if not rid:
                continue
            part = d.part.related_parts[rid]
            ext = os.path.splitext(part.partname)[1].lower()
            if ext not in ('.png', '.jpg', '.jpeg', '.gif', '.bmp'):
                self.add('!TODO a picture in the %s format (%s), to be replaced by hand' % (ext, part.partname))
                continue
            self.nimg += 1
            name = '%s_%d%s' % (self.slug, self.nimg, ext)
            os.makedirs(IMAGES, exist_ok=True)
            open(os.path.join(IMAGES, name), 'wb').write(part.blob)
            ext_el = next(el.iter(WP + 'extent'), None)
            cm = round(int(ext_el.get('cx')) / 360000, 1) if ext_el is not None else 10
            self.add('![](docs/manual/images/%s){width=%s}' % (name, cm))


def table(t):
    grid = [int(g.get(qn('w:w')) or 0) for g in t.iter(qn('w:gridCol'))]
    rows = []
    for tr in t.iter(qn('w:tr')):
        row = []
        for tc in tr.iter(qn('w:tc')):
            text = ' '.join(inline(p)[0].strip() for p in tc.iter(qn('w:p')) if inline(p)[0].strip())
            row.append(text.replace('|', '\\|').replace('\t', ' '))
            pr = tc.find(qn('w:tcPr'))
            span = pr.find(qn('w:gridSpan')) if pr is not None else None
            for _ in range(int(span.get(qn('w:val'))) - 1 if span is not None else 0):
                row.append('')
        rows.append(row)
    n = max(len(r) for r in rows) if rows else 0
    out = []
    if grid and len(grid) == n and sum(grid):
        out.append('{widths: %s}' % ','.join(str(round(100 * g / sum(grid))) for g in grid))
    for k, r in enumerate(rows):
        out.append('| ' + ' | '.join(r + [''] * (n - len(r))) + ' |')
        if k == 0:
            out.append('|' + '---|' * n)
    return out


def main():
    title, dest = sys.argv[1], sys.argv[2]
    d = docx.Document(MANUAL)
    sid = {s.style_id: s.name for s in d.styles}

    def style(el):
        ppr = el.find(qn('w:pPr'))
        st = ppr.find(qn('w:pStyle')) if ppr is not None else None
        return sid.get(st.get(qn('w:val')), '') if st is not None else ''

    els = list(d.element.body.iterchildren())
    start = [i for i, e in enumerate(els) if e.tag == qn('w:p') and style(e) == 'Heading 1'
             and ''.join(t.text or '' for t in e.iter(qn('w:t'))).strip() == title]
    if len(start) != 1:
        sys.exit('%d headings "%s"' % (len(start), title))
    slug = re.sub(r'[^a-z0-9]+', '_', title.lower()).strip('_')[:30]
    o = Out(slug)
    o.lines = ['# ' + title, '']
    for el in els[start[0] + 1:]:
        if el.tag == qn('w:p') and style(el) == 'Heading 1':
            break
        if el.tag == qn('w:sectPr'):
            break
        if el.tag == qn('w:tbl'):
            o.flush_code()
            o.lines += table(el) + ['']
            continue
        if el.tag != qn('w:p'):
            continue
        for tb in el.iter(qn('w:txbxContent')):
            text = ' '.join(inline(p)[0].strip() for p in tb.iter(qn('w:p')) if inline(p)[0].strip())
            if text:
                o.add('!TODO a text box, to be placed by hand: ' + text)
        if list(el.iter(A + 'blip')):
            o.picture(d, el)
        st = style(el)
        text, is_code = inline(el)
        if text.startswith('[MANUAL TODO] '):
            o.add('!TODO ' + text[len('[MANUAL TODO] '):])
            continue
        if st.startswith('Heading 2'):
            o.add('## ' + text.strip().replace('**', ''))
        elif st.startswith('Heading 3'):
            o.add('### ' + text.strip().replace('**', ''))
        elif is_code:
            o.code.append(text.rstrip().replace('\t', '    '))
        elif not text.strip():
            o.flush_code()
        elif el.find(qn('w:pPr')) is not None and el.find(qn('w:pPr')).find(qn('w:numPr')) is not None:
            o.flush_code()
            o.lines.append('- ' + text.strip())
            o.lines.append('')
        elif '\t' in text.strip():
            o.flush_code()
            o.lines.append('>> ' + re.sub(r'\t+', '\t', text.strip()))
        else:
            o.add(re.sub(r'[ \t]+', ' ', text.strip()))
    o.flush_code()
    s = re.sub(r'\n{3,}', '\n\n', '\n'.join(o.lines)).strip() + '\n'
    s = re.sub(r'(^>> .*\n)\n(?=>> )', r'\1', s, flags=re.M)   # (a list of tabbed lines stays together)
    open(dest, 'w', encoding='utf-8', newline='\n').write(s)
    print('%s: %d lines, %d pictures' % (dest, s.count('\n'), o.nimg))


if __name__ == '__main__':
    main()
