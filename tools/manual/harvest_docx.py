"""harvest_docx.py MANUAL.docx OUT.json: the reference entries of a manual in Geoff Graham's template
(docs/Manual_Plan.md, M-A2): the PicoMite manual and the CMM2's 2020 Word file.

The reference sections (Predefined Read Only Variables, Operators, Options, Commands, Functions, Obsolete Commands
and Functions) are two-column tables: the syntax on the left, the description on the right. Each row becomes an entry
{section, name, syntax, desc, table, row}. The name is the syntax's leading keywords: the words before the first
argument, as a reader would look them up ("ADC OPEN", "MM.INFO(", "OPTION AUTORUN"). A row whose left cell is empty
continues the entry above it.

Also writes, beside OUT, the chapters: each Heading 1 with its Heading 2s, in order (OUT with _chapters.json)."""
import json, os, re, sys
import docx
from docx.oxml.ns import qn

REFERENCE = ('predefined read only variables', 'operators', 'options', 'commands', 'functions',
             'obsolete commands and functions')


def text_of(cell):
    return [p.text.replace(' ', ' ').strip() for p in cell.paragraphs]


def entry_name(syntax):
    """the leading keywords of a syntax line: up to the first lower-case word, bracketed part or argument"""
    s = syntax.strip()
    m = re.match(r"((?:MM\.)?[A-Z][A-Z0-9_$.#%!&]*(?:\(|\$)?(?:[ \t]+(?:[A-Z][A-Z0-9_$.]*\(?|\$))*)", s)
    if not m:
        return s.split(' ')[0] if s else ''
    name = m.group(1).strip()
    return re.sub(r'\s+', ' ', name)


def harvest(path):
    d = docx.Document(path)
    body = d.element.body
    entries, chapters = [], []
    h1 = h2 = None
    tno = 0
    for el in body.iterchildren():
        if el.tag == qn('w:p'):
            style = el.find(qn('w:pPr'))
            sname = ''
            if style is not None and style.find(qn('w:pStyle')) is not None:
                sname = style.find(qn('w:pStyle')).get(qn('w:val'))
            txt = ''.join(t.text or '' for t in el.iter(qn('w:t'))).strip()
            sid = d.styles.get_by_id(sname, 1) if sname else None
            label = sid.name if sid is not None else sname
            if label == 'Heading 1' and txt:
                h1, h2 = txt, None
                chapters.append({'chapter': txt, 'sections': []})
            elif label == 'Heading 2' and txt and chapters:
                h2 = txt
                chapters[-1]['sections'].append(txt)
        elif el.tag == qn('w:tbl'):
            tno += 1
            section = (h1 or '').lower()
            sec = next((r for r in REFERENCE if section.startswith(r)), None)
            if not sec:
                continue
            table = docx.table.Table(el, d)
            for rno, row in enumerate(table.rows):
                cells = row.cells
                if len(cells) < 2:
                    continue
                left = [t for t in text_of(cells[0]) if t]
                right = [t for t in text_of(cells[-1]) if t]
                if cells[0]._tc is cells[-1]._tc:            # (a merged row: a group label)
                    continue
                if not left and entries and entries[-1]['table'] == tno:
                    entries[-1]['desc'] += right
                    continue
                if not left:
                    continue
                entries.append({'section': sec, 'name': entry_name(left[0]), 'syntax': left, 'desc': right,
                                'table': tno, 'row': rno, 'heading2': h2})
    return entries, chapters


def main():
    src, out = sys.argv[1], sys.argv[2]
    entries, chapters = harvest(src)
    json.dump(entries, open(out, 'w', encoding='utf-8', newline='\n'), indent=1, ensure_ascii=False)
    json.dump(chapters, open(out.replace('.json', '_chapters.json'), 'w', encoding='utf-8', newline='\n'), indent=1,
              ensure_ascii=False)
    count = {}
    for e in entries:
        count[e['section']] = count.get(e['section'], 0) + 1
    print('%s: %d entries (%s), %d chapters' % (os.path.basename(src), len(entries),
          ', '.join('%s %d' % kv for kv in count.items()), len(chapters)))


if __name__ == '__main__':
    main()
