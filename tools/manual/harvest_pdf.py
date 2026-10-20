"""harvest_pdf.py MANUAL.pdf OUT.json: the reference entries of the CMM2's b14 manual, which we have only as a PDF
(docs/Manual_Plan.md, M-A2). The output has the same form as harvest_docx.py's, with the page in place of the table.

The PDF keeps the Word file's two-column tables as positions: the syntax starts at x = 51 pt, the description at
x = 210 pt. A block of left-column lines (each within 1.5 line heights of the one above) starts a row; the right-column
lines from there to the next block are its description, across a page break too. The small Calibri-Light labels
(the keyword groups' bookmarks) and the running header and footer are left out. The sections come from the PDF's
outline (its level-1 entries)."""
import json, os, re, sys
import fitz

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from harvest_docx import REFERENCE, entry_name

SPLIT = 200                                             # pt: left of it the syntax, right of it the description


def lines_of(page):
    out = []
    for blk in page.get_text('dict')['blocks']:
        for l in blk.get('lines', []):
            spans = l['spans']
            txt = ''.join(s['text'] for s in spans).replace(' ', ' ')
            font, size = spans[0]['font'], spans[0]['size']
            if size < 9 or font.startswith('Calibri-Light'):     # (header, footer, bookmark labels)
                continue
            out.append((l['bbox'][1], l['bbox'][0], txt.strip(), size))
    return sorted(out)


def harvest(path):
    d = fitz.open(path)
    toc = [t for t in d.get_toc() if t[0] == 1]
    sections = []
    for i, (lvl, title, page) in enumerate(toc):
        end = toc[i + 1][2] - 1 if i + 1 < len(toc) else d.page_count
        sec = next((r for r in REFERENCE if title.strip().lower().startswith(r)), None)
        if sec:
            sections.append((sec, page, end))
    entries = []
    for sec, first, last in sections:
        cur = None
        for pn in range(first - 1, last):
            prev_left = None
            for y, x, txt, size in lines_of(d[pn]):
                if pn == first - 1 and txt.lower() == sec:   # (the section's own heading)
                    continue
                if x < SPLIT:
                    if not txt:
                        continue
                    marked = bool(re.match(r'[-]', txt))   # (a symbol-font bullet before the syntax, on
                    txt = re.sub(r'^[-]\s*', '', txt)       # 22 rows: kept as 'marked', not in the name)
                    if cur is not None and prev_left is not None and y - prev_left < size * 1.5:
                        cur['syntax'].append(txt)               # (the syntax goes on)
                        cur['marked'] = cur['marked'] or marked
                    else:
                        cur = {'section': sec, 'name': entry_name(txt), 'syntax': [txt], 'desc': [], 'page': pn + 1,
                               'marked': marked}
                        entries.append(cur)
                    prev_left = y
                elif cur is not None and txt:
                    cur['desc'].append(txt)
    for e in entries:                                   # a description's lines joined into paragraphs: a line that
        paras, buf = [], ''                             # ends a sentence ends a paragraph (good enough for the audit)
        for t in e['desc']:
            buf = (buf + ' ' + t).strip()
            if t.endswith(('.', ':')) or len(t) < 40:
                paras.append(buf)
                buf = ''
        if buf:
            paras.append(buf)
        e['desc'] = paras
    return entries


def main():
    src, out = sys.argv[1], sys.argv[2]
    entries = harvest(src)
    json.dump(entries, open(out, 'w', encoding='utf-8', newline='\n'), indent=1, ensure_ascii=False)
    count = {}
    for e in entries:
        count[e['section']] = count.get(e['section'], 0) + 1
    print('%s: %d entries (%s)' % (os.path.basename(src), len(entries), ', '.join('%s %d' % kv for kv in count.items())))


if __name__ == '__main__':
    main()
