"""refcheck.py: each reference entry (docs/manual/ref/*.txt) against what the build has (docs/manual/audit/firmware.json,
inventory.py), for the reference pass of M4 (docs/Manual_Plan.md). Writes docs/manual/audit/refcheck.md.

Each syntax line of an entry (the lines of its left cell; "or" between them) is read for its keyword, the longest
keyword name it starts with (aliases read through), and then for its first sub-keyword: a word or words in capitals
after the keyword that the handler, or a function it calls, passes to checkstring(). An entry is:
- OK: every syntax line names a keyword of the build, and its sub-keyword if it has one;
- NO KEYWORD: no syntax line names a keyword of the build (a candidate to remove);
- NO SUB-KEYWORD: the keyword is there but a sub-keyword is not (the handler does not read it);
- PART: some syntax lines are OK and some are not.
The verdicts are suggestions: capitals in a syntax line can be an argument's name, and the inventory reads the
checkstring() literals only. The pass decides."""
import json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from reftext import parse, REF, SECTIONS

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
AUDIT = os.path.join(ROOT, 'docs', 'manual', 'audit')
FW = json.load(open(os.path.join(AUDIT, 'firmware.json'), encoding='utf-8'))


def plain(line):
    """a syntax line without its attributes, marks and escapes"""
    line = re.sub(r'^\{[^}]*\}', '', line)
    line = re.sub(r'<<([^|>]*)\|?([^>]*)>>', lambda m: m.group(2) or m.group(1), line)
    line = line.replace('\\n', ' ')
    line = re.sub(r'\\(.)', r'\1', line)
    return re.sub(r'\*\*|\*|__|`|\^\^', '', line).strip()


def table():
    kws = {}
    for k in FW['keywords']:
        name = k['name'].upper()
        kws.setdefault(name, set()).update(s['kw'].upper() for s in k['subkeywords'])
    for v in FW['mm_variables']:
        kws.setdefault(v.upper(), set())
    for f in FW.get('folded_functions', []):
        name = (f['name'] if isinstance(f, dict) else str(f)).upper()
        kws.setdefault(name, set())
    aliases = {}
    for a in FW['aliases']:
        if isinstance(a, dict):
            aliases[a['alias'].upper()] = a['is'].upper()
        elif isinstance(a, (list, tuple)):
            aliases[a[0].upper()] = a[1].upper()
    for d in FW.get('directives', []):
        kws.setdefault((d if isinstance(d, str) else d['name']).upper(), set())
    return kws, aliases


KWS, ALIASES = table()
NAMES = sorted(KWS, key=len, reverse=True)


def keyword_of(s):
    u = s.upper()
    for a, k in sorted(ALIASES.items(), key=lambda x: -len(x[0])):
        if u.startswith(a) and (len(u) == len(a) or not u[len(a)].isalnum()):
            u = k + u[len(a):]
            break
    for n in NAMES:
        if u.startswith(n) and (n.endswith('(') or len(u) == len(n) or not (u[len(n)].isalnum() or u[len(n)] in '$_.')):
            return n, s[len(n):] if s.upper().startswith(n) else u[len(n):]
    return None, s


def check_line(s):
    kw, rest = keyword_of(s)
    if kw is None:
        return 'NO KEYWORD', None
    rest = rest.strip()
    m = re.match(r'([A-Z][A-Z0-9_$]*(?: [A-Z][A-Z0-9_$]*)*)\b', rest)    # capitals: a sub-keyword, or several
    if not m or not KWS[kw]:
        return 'OK', kw
    words = m.group(1).split()
    for n in range(len(words), 0, -1):
        if ' '.join(words[:n]) in KWS[kw]:
            return 'OK', kw + ' ' + ' '.join(words[:n])
    if len(words[0]) == 1:                              # (a single capital: an argument, x, y, n...)
        return 'OK', kw
    return 'NO SUB-KEYWORD', kw + ' ' + words[0]


def main():
    out = ['# The reference entries against the build', '',
           'Made by `tools/manual/refcheck.py` from `docs/manual/ref/` and `firmware.json` (%s). Suggestions for the '
           'reference pass: the pass decides.' % FW['version'], '']
    totals = {}
    for name, section in SECTIONS.items():
        entries = parse(os.path.join(REF, name + '.txt'))
        rows = []
        for e in entries:
            lines = [plain(l) for l in e['left']]
            syn = [l for l in lines if l and l.lower() not in ('or', '{}')]
            if not syn:
                continue
            res = [(l,) + check_line(l) for l in syn]
            ok = [r for r in res if r[1] == 'OK']
            verdict = 'OK' if len(ok) == len(res) else ('PART' if ok else
                      ('NO KEYWORD' if all(r[1] == 'NO KEYWORD' for r in res) else 'NO SUB-KEYWORD'))
            totals[verdict] = totals.get(verdict, 0) + 1
            if verdict != 'OK':
                bad = '; '.join('%s (%s)' % (r[0][:60], r[1] if r[1] != 'NO SUB-KEYWORD' else 'no ' + r[2]) for r in res if r[1] != 'OK')
                rows.append('| %d | %s | %s | %s |' % (e['line'], verdict, syn[0][:70].replace('|', '/'), bad.replace('|', '/')))
        out += ['## %s (`%s.txt`)' % (section, name), '', '%d entries; not OK: %d' % (len(entries), len(rows)), '',
                '| Line | Verdict | First syntax line | What is missing |', '|---|---|---|---|'] + rows + ['']
    out.insert(4, 'Entries: ' + ', '.join('%s %d' % kv for kv in sorted(totals.items())) + '.')
    out.insert(5, '')
    open(os.path.join(AUDIT, 'refcheck.md'), 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
    print('refcheck.md:', totals)


if __name__ == '__main__':
    main()
