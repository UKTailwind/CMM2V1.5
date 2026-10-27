"""bookmarks.py [DOCX [PDF]] [--validate]: a PDF bookmark for each entry of the manual's reference sections (Predefined
Read Only Variables, Options, Commands, Functions, Obsolete Commands and Functions), under each section's "Detailed
Listing" heading, as the PicoMite's manual has them (its docs/add_manual_bookmarks.py; Peter, 24 Oct 2026).

Word's export bookmarks only the paragraphs in a heading style, and the entries are rows of a table, so they are added
to the exported PDF afterwards: the names from the docx's tables (the table after each section's "Detailed Listing"
heading), each found in the PDF's left column in the document's order, the outline rebuilt with them as the heading's
children. Run again on a PDF that has them, it replaces them. word.ps1 runs it after each export.

As the PicoMite's, a name is the entry's first syntax line up to its arguments (OPTION BAUDRATE, ARC, MID$), the first
of the rows that give the same name (PRINT, PRINT @, PRINT #) only; an MM.INFO row keeps its key (MM.INFO(HEAP),
MM.INFO$(SDCARD)), so that each has its own bookmark. --validate reports what it finds and writes nothing.

The defaults are docs/manual/Colour_Maximite_2_User_Manual.docx and .pdf."""
import logging, os, re, shutil, sys
from docx import Document
from docx.oxml.ns import qn
import pypdf
from pypdf.generic import Fit, NameObject, NumberObject

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SECTIONS = ["Predefined Read Only Variables", "Options", "Commands", "Functions", "Obsolete Commands and Functions"]
NAME_X_MAX = 198                    # the name column: its fragments start left of this (the descriptions at 203 or more,
                                    # the Options' Permanent? ticks at 205)
KEEP_TRAIL = set("([,")             # an upper-case word before one of these is still the name (MID$( ..., ADC OPEN, ...)
TOP_PAD = 16                        # the view a line above the entry's first line


def runtext(r):
    return "".join(t.text or "" for t in r.findall(qn("w:t")))


def first_cell_text(tc):
    """the first non-empty paragraph of a name cell: (its first run's text, its whole text)"""
    for p in tc.findall(qn("w:p")):
        runs = [x for x in (runtext(r) for r in p.findall(qn("w:r"))) if x != ""]
        if runs and "".join(runs).strip():
            return runs[0], "".join(runs)
    return None, None


def clean_name(tc, use_full):
    r0, full = first_cell_text(tc)
    if r0 is None:
        return None
    s = ((full if use_full else r0) or full).replace("\xa0", " ").strip()
    m = re.match(r"(MM\.INFO\$?)\(\s*([A-Z][A-Z0-9 ]*?)(\s*)(?=[)a-z'0-9\[]|$)", s)   # ([: BBC BUFFER [n])
    if m and m.group(2).strip():                                     # an MM.INFO key: its own bookmark
        key = m.group(2).strip()
        if not m.group(3) and s[m.end():m.end() + 1].islower() and " " in key:
            key = key.rsplit(" ", 1)[0]                              # (PINNO GPn: GPn is the argument)
        return "%s(%s)" % (m.group(1), key)
    out = []
    for tok in s.split():
        m = re.match(r"[A-Z][A-Z0-9.$_]*", tok)
        if m and m.end() == len(tok):
            out.append(tok)                                          # a whole upper-case word
        elif m and tok[m.end()] in KEEP_TRAIL:
            out.append(m.group(0))                                   # NAME( ...: the name, and stop
            break
        else:
            break                                                    # a lower-case argument or a symbol: stop
    name = " ".join(out).strip()
    if name:
        return name
    return s.split()[0] if s[:1].isalnum() else s                    # (' and * and ?: their text)


def docx_sections(path):
    """[(section, [names])]: the table after each section's Detailed Listing heading, in the document's order"""
    body = Document(path).element.body
    section, armed, tables = None, False, {}
    for el in body.iterchildren():
        if el.tag == qn("w:p"):
            text = "".join(t.text or "" for t in el.iter(qn("w:t"))).strip()
            ps = el.find(qn("w:pPr"))
            style = ps.find(qn("w:pStyle")).get(qn("w:val")) if ps is not None and ps.find(qn("w:pStyle")) is not None else ""
            if style.lower().startswith("heading"):
                if text in SECTIONS:
                    section, armed = text, False
                elif text == "Detailed Listing" and section:
                    armed = True
                else:
                    section, armed = None, False
        elif el.tag == qn("w:tbl") and armed and section not in tables:
            tables[section] = el
            armed = False
    out = []
    for s in SECTIONS:
        if s not in tables:
            sys.exit("no table after %s's Detailed Listing" % s)
        names, seen = [], set()
        for tr in tables[s].findall(qn("w:tr")):
            tc = tr.find(qn("w:tc"))
            nm = clean_name(tc, s != "Options") if tc is not None else None   # (an option's value list: its first run)
            if nm and nm not in seen:
                names.append(nm)
                seen.add(nm)
        out.append((s, names))
    return out


def page_name_lines(reader):
    """for each page, its name-column lines: [(y, x, text)] from the top"""
    pages = []
    for page in reader.pages:
        frags = []

        def visit(text, cm, tm, fd, fs, frags=frags):
            if text.strip():
                frags.append((round(tm[5], 1), round(tm[4], 1), text))
        page.extract_text(visitor_text=visit)
        lines = {}
        for y, x, t in frags:
            lines.setdefault(y, []).append((x, t))
        out = []
        for y, parts in lines.items():
            parts.sort()
            namep = [p for p in parts if p[0] <= NAME_X_MAX]
            if namep:
                txt = "".join(p[1] for p in namep).replace("\xa0", " ").strip()
                if txt:
                    out.append((y, namep[0][0], txt))
        out.sort(key=lambda r: -r[0])
        pages.append(out)
    return pages


def ns(s):
    """the text without its spaces, in lower case (the PDF's text often loses the spaces between runs)"""
    return re.sub(r"\s+", "", s.replace("\xa0", " ")).lower()


def locate(sections, reader, parents):
    """{section: [(name, page, y)]} and the names not found: each matched in order from the section's Detailed Listing
    page, a name wrapped over up to four lines matched as one (OPTION DEFAULT / COLOURS)"""
    pages = page_name_lines(reader)
    stream = [(pi, y, ns(txt)) for pi, rows in enumerate(pages) for (y, x, txt) in rows]
    located, misses, pos = {}, [], 0
    for parent, names in sections:
        start = parents.get(parent)
        if start is None:
            misses += [(parent, n, "no Detailed Listing bookmark") for n in names]
            continue
        while pos < len(stream) and stream[pos][0] < start:
            pos += 1
        found = []
        for nm in names:
            want = ns(nm[:-1] if nm.startswith("MM.INFO") and nm.endswith(")") else nm)   # (a key's arguments follow it)
            hit, j = None, pos
            while j < len(stream):
                pi, y, nt = stream[j]
                if nt and nt.startswith(want):
                    hit = (pi, y, j + 1)
                    break
                if nt and want.startswith(nt):
                    acc, k = nt, j + 1
                    while k < len(stream) and len(acc) < len(want):
                        acc += stream[k][2]
                        k += 1
                        if acc.startswith(want):
                            hit = (pi, y, k)
                            break
                        if not want.startswith(acc[:len(want)]):
                            break
                    if hit:
                        break
                j += 1
            if hit:
                found.append((nm, hit[0], hit[1]))
                pos = hit[2]
            else:
                misses.append((parent, nm, "not found"))
        located[parent] = found
    return located, misses


def section_parents(reader):
    """the page of each section's Detailed Listing bookmark"""
    res = {}

    def rec(items, parent):
        for i, it in enumerate(items):
            if isinstance(it, list):
                continue
            title = str(it.title)
            child = items[i + 1] if i + 1 < len(items) and isinstance(items[i + 1], list) else None
            if title == "Detailed Listing" and parent in SECTIONS:
                res[parent] = reader.get_destination_page_number(it)
            if child:
                rec(child, title)
    rec(reader.outline, "ROOT")
    return res


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    validate = "--validate" in sys.argv
    docx_path = args[0] if args else os.path.join(ROOT, "docs", "manual", "Colour_Maximite_2_User_Manual.docx")
    pdf = args[1] if len(args) > 1 else os.path.splitext(docx_path)[0] + ".pdf"
    reader = pypdf.PdfReader(pdf)
    sections = docx_sections(docx_path)
    parents = section_parents(reader)
    located, misses = locate(sections, reader, parents)
    for parent, names in sections:
        print("%s: %d of %d located" % (parent, len(located.get(parent, [])), len(names)))
    for p, n, why in misses[:60]:
        print("  not found: [%s] %r (%s)" % (p, n, why))
    if validate:
        return 1 if misses else 0

    logging.getLogger("pypdf").setLevel(logging.ERROR)                # ("Annotation sizes differ" as the pages' links are
    writer = pypdf.PdfWriter()                                        # copied: they all are, with their targets)
    writer.append(reader, import_outline=False)

    def rebuild(items, wparent, parent_title):
        for i, it in enumerate(items):
            if isinstance(it, list):
                continue
            title = str(it.title)
            child = items[i + 1] if i + 1 < len(items) and isinstance(items[i + 1], list) else None
            pg = reader.get_destination_page_number(it)
            top = getattr(it, "top", None)
            node = writer.add_outline_item(title, pg, parent=wparent,
                                           fit=Fit.xyz(top=float(top)) if top is not None else Fit.fit(), is_open=False)
            if title == "Detailed Listing" and parent_title in located:   # the entries (in place of any from before)
                for nm, lpi, ly in located[parent_title]:
                    dst = min(ly + TOP_PAD, float(reader.pages[lpi].mediabox.top))
                    writer.add_outline_item(nm, lpi, parent=node, fit=Fit.xyz(top=dst), is_open=False)
            elif child:
                rebuild(child, node, title)
    rebuild(reader.outline, None, "ROOT")

    def fix_counts(node):                                            # every level closed until it is opened
        child, n = node.get("/First"), 0
        while child is not None:
            c = child.get_object()
            fix_counts(c)
            n += 1
            child = c.get("/Next")
        if n:
            node[NameObject("/Count")] = NumberObject(-n)
        return n
    outlines = writer._root_object["/Outlines"]
    outlines[NameObject("/Count")] = NumberObject(fix_counts(outlines))   # (the top level open)

    tmp = pdf + ".bookmarks.tmp"
    with open(tmp, "wb") as f:
        writer.write(f)
    shutil.move(tmp, pdf)
    print("%s: %d entry bookmarks" % (pdf, sum(len(v) for v in located.values())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
