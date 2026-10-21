"""appendix_n.py: Appendix N of the manual, where each keyword is documented, made from the build's keywords
(docs/manual/audit/firmware.json, inventory.py) and the reference sections (docs/manual/ref/*.txt). Writes
docs/manual/text/44_appendix_n_index.md for chapter.py, and prints the keywords it found nowhere.

A keyword is documented in a reference section when an entry's syntax line starts with it (the tokeniser's aliases
read through, as refcheck.py does); the operators are in the chapter Operators, and the words that are parts of a
statement (THEN, TO, STEP...) with that statement. Some features have a companion manual as well (Appendix O)."""
import json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import refblocks as rb
from refcheck import plain, keyword_of, FW
from reftext import SECTIONS

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, 'docs', 'manual', 'text', '44_appendix_n_index.md')
NAMES = {'variables': 'Predefined Read Only Variables', 'options': 'Options', 'commands': 'Commands',
         'functions': 'Functions', 'obsolete': 'Obsolete Commands and Functions'}
PART_OF = {'ELSE': 'IF', 'ELSEIF': 'IF', 'ENDIF': 'IF', 'THEN': 'IF', 'STEP': 'FOR', 'TO': 'FOR', 'NEXT': 'FOR',
           'UNTIL': 'DO', 'WHILE': 'DO', 'LOOP': 'DO', 'GOSUB': 'ON', 'GOTO': 'ON', 'CASE': 'SELECT CASE',
           'END SELECT': 'SELECT CASE', 'CASE ELSE': 'SELECT CASE', 'END SUB': 'SUB', 'END FUNCTION': 'FUNCTION',
           'END CSUB': 'CSUB', 'END TYPE': 'TYPE', 'EXIT FOR': 'EXIT', 'EXIT SUB': 'EXIT', 'EXIT FUNCTION': 'EXIT'}
OVERRIDE = {'BLIT': 'Commands', 'SPRITE': 'Commands (the sprite commands: SPRITE is another name for BLIT), and Functions, SPRITE('}
PLACED = {'@(': 'Commands, with PRINT', 'AS': 'Commands, with DIM, LOCAL, OPEN and the other statements that use it',
          'SPI(': 'Functions', 'SPI2(': 'Functions', 'SPRITE(': 'Functions'}      # (entries the matcher cannot read)
INTERNAL = {'', '~(', 'SCHANGE$(', 'TOPBOTTOM(', 'BITBYTEFLAG('}            # the tokeniser's folds: never typed
COMPANION = {'DRAW3D': 'The 3D engine.pdf', 'DRAW3D(': 'The 3D engine.pdf', 'GUI': 'GUI Controls and Programming.pdf',
             'CTRLVAL(': 'GUI Controls and Programming.pdf', 'MSGBOX(': 'GUI Controls and Programming.pdf',
             'TYPE': 'MMBasic_Structures_Manual.pdf', 'STRUCT': 'MMBasic_Structures_Manual.pdf',
             'STRUCT(': 'MMBasic_Structures_Manual.pdf', 'FM': 'FM_User_Manual.pdf',
             'TILEMAP': 'TILEMAP_User_Manual.pdf', 'TILEMAP(': 'TILEMAP_User_Manual.pdf',
             'RAY': 'Appendix P', 'RAY(': 'Appendix P', 'TURTLE': 'Appendix G',
             'STAR': 'Appendix Q', 'ASTRO': 'Appendix Q', 'LOCATION': 'Appendix Q', 'SLEW': 'Appendix Q'}   # (and this manual's own appendices)


def documented():
    """keyword -> the reference sections with an entry for it"""
    where = {}
    for name in SECTIONS:
        for n, b in rb.load(name):
            if not n:
                continue
            for l in b:
                if l == '---' or l.startswith('--- '):
                    break
                t = plain(l[4:] if l[:4] in ('=== ', '=+= ') else l)
                if not t or t.lower() == 'or':
                    continue
                kw, _ = keyword_of(t)
                if kw:
                    where.setdefault(kw, set()).add(NAMES[name])
    return where


def main():
    where = documented()
    aliases = {a['alias'].upper(): a['is'].upper() for a in FW['aliases']}
    rows, missing = [], []
    def add(kw, place):
        rows.append((kw, place))
    for k in FW['keywords']:
        name = k['name'].upper()
        flags = k['flags']
        if 'T_OPER' in flags:
            add(name, 'the chapter Operators')
            continue
        if name in INTERNAL:
            continue
        if name in OVERRIDE:
            place = OVERRIDE[name]
        elif name in where:
            place = ', '.join(sorted(where[name]))
        elif name in PLACED:
            place = PLACED[name]
        elif name in PART_OF and PART_OF[name] in where:
            place = 'Commands, with %s' % PART_OF[name]
        else:
            missing.append(name)
            continue
        if name in COMPANION:
            place += '; ' + COMPANION[name]
        add(name, place)
    for f in FW['folded_functions']:
        name = (f if isinstance(f, str) else f['name']).upper()
        if name in where:
            add(name, ', '.join(sorted(where[name])) + ('; ' + COMPANION[name] if name in COMPANION else ''))
        else:
            missing.append(name)
    for v in FW['mm_variables']:
        if v.upper() in where:
            add(v.upper(), ', '.join(sorted(where[v.upper()])))
        elif v.upper() == 'MM.END':
            add('MM.END', 'the chapter Using MMBasic, and Commands, with END')
        else:
            missing.append(v.upper())
    for dct in FW['directives']:
        name = dct.upper()
        key = next((w for w in where if w.startswith('#') and name.startswith(w)), None)
        if name in where or key:
            add(name, ', '.join(sorted(where.get(name) or where[key])))
        else:
            missing.append(name)
    for a, k in aliases.items():
        if a in OVERRIDE:
            add(a, OVERRIDE[a])
        elif a not in [r[0] for r in rows]:
            add(a, 'another name for %s (%s)' % (k, ', '.join(sorted(where.get(k) or where.get(k + '(') or {'Commands'}))))
    seen, out = set(), []
    for kw, place in sorted(rows, key=lambda r: (re.sub(r'^[^A-Z0-9#]+', '', r[0]) or r[0], r[0])):
        if kw not in seen:
            seen.add(kw)
            out.append('| %s | %s |' % (kw.replace('|', '\\|'), place))
    text = ['# Appendix N – Where Each Keyword Is Documented', '',
            'Every keyword of the language, and the section of this manual where it is described. Where a feature also '
            'has a manual of its own, that is named too (see Appendix O). The options are in the section Options, '
            'under OPTION.', '', '{widths: 30,70}', '| Keyword | Where it is documented |', '|---|---|'] + out + ['']
    open(OUT, 'w', encoding='utf-8', newline='\n').write('\n'.join(text))
    print('%d keywords written to %s' % (len(out), OUT))
    if missing:
        print('found nowhere (%d): %s' % (len(missing), ', '.join(sorted(set(missing)))))


if __name__ == '__main__':
    main()
