"""crossref.py: the manual audit's cross-reference (docs/Manual_Plan.md, M-A3), from docs/manual/audit/:
firmware.json (inventory.py), pico.json and cmm2_2020.json (harvest_docx.py), cmm2_b14.json (harvest_pdf.py).

Writes docs/manual/audit/keywords.md:
1. every keyword of the build, with where each manual documents it, then its sub-keywords the same way. The class is
   a suggestion from where it is documented (P: the PicoMite manual only, C: the b14 manual only, P+C: both, U: neither);
   the audit confirms it against the handler and corrects it (PD, CD, X: docs/Manual_Plan.md).
2. the entries of the PicoMite and b14 manuals for keywords, or sub-keywords, that the build does not have: the
   candidates for X.

A keyword is documented by an entry whose name is the keyword or starts with it; a sub-keyword by an entry of its
keyword whose syntax has the sub-keyword as a word. Both are looser than a reader would be: the audit decides."""
import json, os, re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
AUDIT = os.path.join(ROOT, 'docs', 'manual', 'audit')
MANUALS = (('pico', 'PicoMite 7.0'), ('cmm2_b14', 'CMM2 b14'), ('cmm2_2020', 'CMM2 2020'))


def norm(s):
    return re.sub(r'\s+', ' ', s.strip().upper())


def load(name):
    return json.load(open(os.path.join(AUDIT, name + '.json'), encoding='utf-8'))


def kind(r):
    if 'T_OPER' in r['flags']:
        return 'operator'
    if r['table'] == 'command':
        return 'command'
    if 'T_FUN' in r['flags'] or 'T_FNA' in r['flags']:
        return 'function' + (' and command' if 'T_CMD' in r['flags'] else '')
    return 'command' if 'T_CMD' in r['flags'] else 'other'


def bare(k):
    return k[:-1] if k.endswith('(') else k


INTERNAL = {'BITBYTEFLAG(', 'SCHANGE$(', 'TOPBOTTOM(', '~(', ''}     # the tokeniser's own tokens, never typed


def starts(text, key):
    """text starts with keyword key, followed by nothing a name could go on with"""
    if not text.startswith(key):
        return False
    rest = text[len(key):]
    return key.endswith('(') or not rest or not re.match(r'[A-Z0-9_.$%!]', rest)


def entries_for(key, entries, kind=''):
    """the entries that document keyword key (normalised): by name, or by any syntax line that starts with it (CASE
    in SELECT CASE's entry, MIN( in MAX('s); an MM. variable anywhere in a syntax line (MM.HRES and MM.VRES)"""
    out = []
    word = re.compile(r'(?<![A-Z0-9_.])' + re.escape(key) + r'(?![A-Z0-9_])')
    for e in entries:
        n = norm(e['name'])
        syn = [norm(s) for s in e['syntax']]
        if kind == 'operator':
            if e['section'] == 'operators' and any(word.search(s) for s in syn + [norm(d) for d in e['desc']]):
                out.append(e)
        elif starts(n, key) or bare(n) == bare(key) or any(starts(s, key) for s in syn) or \
                (key.startswith('MM.') and any(word.search(s) for s in syn)):
            out.append(e)
    return out


def where(e):
    return 'p%d' % e['page'] if 'page' in e else 't%d' % e['table']


def has_word(sub, ents):
    pat = re.compile(r'(?<![A-Z0-9_])' + re.escape(norm(sub)) + r'(?![A-Z0-9_])')
    return [e for e in ents if any(pat.search(norm(s)) for s in e['syntax'])]


def main():
    fw = load('firmware')
    man = {m: load(m) for m, _ in MANUALS}
    rows, out = [], []
    keys = []
    for r in fw['keywords']:
        if 'T_NA' in r['flags']:
            continue
        keys.append((norm(r['name']), 'internal' if norm(r['name']) in INTERNAL else kind(r), r))
    for v in fw['mm_variables']:
        keys.append((norm(v), 'variable (folded)', None))
    for f in fw['folded_functions']:
        keys.append((norm(f), 'function (folded)', None))
    for d in fw['directives']:
        keys.append((norm(d), 'directive', None))
    for a in fw['aliases']:
        keys.append((norm(a['alias']), 'alias of ' + norm(a['is']), None))
    summary = {}
    for key, k, r in keys:
        if k == 'internal':
            rows.append('| %s | internal | %s | - | - | - | - |  |' % (key or '(LET)', r['where'] or ''))
            continue
        found = {m: entries_for(key, man[m], k) for m, _ in MANUALS}
        sug = {(True, True): 'P+C', (True, False): 'P', (False, True): 'C', (False, False): 'U'}[
            (bool(found['pico']), bool(found['cmm2_b14'] or found['cmm2_2020']))]
        summary[sug] = summary.get(sug, 0) + 1
        cells = []
        for m, _ in MANUALS:
            f = found[m]
            cells.append(', '.join(sorted({where(e) for e in f}))[:40] if f else '-')
        rows.append('| **%s** | %s | %s | %s | %s |  |' % (key.replace('|', '\\|'), k, r['where'] if r else '',
                                                        ' | '.join(cells), sug))
        if r:
            for s in r['subkeywords']:
                sub = {m: has_word(s['kw'], found[m]) for m, _ in MANUALS}
                ssug = {(True, True): 'P+C', (True, False): 'P', (False, True): 'C', (False, False): 'U'}[
                    (bool(sub['pico']), bool(sub['cmm2_b14'] or sub['cmm2_2020']))]
                rows.append('| &nbsp;&nbsp;%s | sub-keyword | %s:%d%s | %s | %s |  |' % (
                    s['kw'].replace('|', '\\|'), s['file'], s['line'], ' (%s)' % s['func'] if s['via'] else '',
                    ' | '.join('yes' if sub[m] else '-' for m, _ in MANUALS), ssug))
    out.append('# The manual audit: every keyword of the build against the manuals\n')
    out.append('Made by `tools/manual/crossref.py` from the firmware %s (`firmware.json`) and the harvests of the PicoMite '
               'manual 7.0 (`pico.json`), the CMM2 manual 6.00.00 b14 (`cmm2_b14.json`, pages) and the CMM2 manual 5.05.04 '
               '(`cmm2_2020.json`, tables). Run it again after any of them changes; the Audit column is kept by hand '
               '(docs/Manual_Plan.md, M-A3).\n' % fw['version'])
    out.append('Keywords by where they are documented (a suggestion, to be confirmed): ' +
               ', '.join('%s %d' % kv for kv in sorted(summary.items())) + '.\n')
    out.append('## 1. The build\'s keywords\n')
    out.append('| Keyword | Kind | Handler | %s | Suggested | Audit |' % ' | '.join(t for _, t in MANUALS))
    out.append('|---|---|---|---|---|---|---|---|')
    out += rows
    # 2. the entries for what the build lacks
    fwkeys = {}
    for key, k, r in keys:
        fwkeys[key] = r
    alias = {norm(a['alias']): norm(a['is']) for a in fw['aliases']}
    out.append('\n## 2. Manual entries for what the build does not have (candidates for X)\n')
    out.append('An entry\'s name is read through the tokeniser\'s aliases first (SPRITE is BLIT, WII is CONTROLLER, CAT '
               'is INC). Rows whose left cell is a single letter or starts in lower case are parts of a description '
               '(a table inside it), not entries, and are left out.\n')
    for m, title in MANUALS[:2]:
        missing, subs = [], []
        for e in man[m]:
            n = norm(e['name'])
            if len(n) < 2 or not re.match(r'[A-Z#]', e['name'].strip()):
                continue
            for a, b in alias.items():
                if starts(n, a):
                    n = b + n[len(a):]
                    break
            best = None
            words = n.split(' ')
            for i in range(len(words), 0, -1):          # the longest run of leading words that is a keyword
                cand = ' '.join(words[:i])
                for c in (cand, cand + '(', bare(cand)):
                    if c in fwkeys:
                        best = c
                        break
                if best:
                    break
            if not best:
                missing.append('%s (%s)' % (e['name'], where(e)))
            elif fwkeys[best] and len(words) > len(best.split(' ')):
                nxt = words[len(best.split(' '))]
                kws = {norm(s['kw']).split(' ')[0] for s in fwkeys[best]['subkeywords']}
                if kws and re.match(r'^[A-Z][A-Z0-9_]+$', nxt) and nxt not in kws:
                    subs.append('%s (%s)' % (e['name'], where(e)))
        out.append('**%s**: %d entries name no keyword of the build:\n' % (title, len(missing)))
        out.append(', '.join(sorted(set(missing))) + '\n')
        out.append('**%s**: %d entries name a sub-keyword the build\'s handler does not read:\n' % (title, len(subs)))
        out.append(', '.join(sorted(set(subs))) + '\n')
    open(os.path.join(AUDIT, 'keywords.md'), 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
    print('keywords:', ', '.join('%s %d' % kv for kv in sorted(summary.items())), '- written', os.path.join(AUDIT, 'keywords.md'))


if __name__ == '__main__':
    main()
