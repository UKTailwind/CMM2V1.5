"""inventory.py [OUT]: what this build's BASIC has, for the manual's audit (docs/Manual_Plan.md, M-A1).

Read from the source, not from a manual:
- the command table and the token table of Inc/allcommands.h: each keyword, its type flags and its handler;
- the MM. names that tokenise() folds into ~( (overlaid_functions, Src/Functions.c);
- the function families folded into one token (FoldList, Src/MMBasic.c): BIN$( is BASE$(2, and so on;
- for each handler, its sub-keywords: the literals it passes to checkstring(), in its own body and in the functions it
  calls (two calls deep), each with the function and line it is in. A literal found through a call is marked "via".

Writes OUT (docs/manual/audit/firmware.json by default) and prints a summary. The sub-keywords are a superset: a
helper shared by two handlers gives its literals to both, which the cross-reference (crossref.py) shows."""
import json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'docs', 'manual', 'audit', 'firmware.json')
SRC = os.path.join(ROOT, 'Src')

# helpers that every handler calls and that read no sub-keyword of their own
STOP = {'getargs', 'getint', 'getinteger', 'getnumber', 'getstring', 'getCstring', 'getFstring', 'evaluate', 'error',
        'checkstring', 'skipspace', 'GetTempMemory', 'GetMemory', 'FreeMemory', 'findvar', 'makeargs', 'getvalue',
        'MMPrintString', 'PRet', 'PInt', 'PFlt', 'strcpy', 'strcat', 'strlen', 'memcpy', 'memset', 'sprintf',
        'getalpha', 'checkend', 'CheckAbort', 'routinechecks', 'IntToStr', 'FloatToStr', 'ExecuteProgram'}


def read(path):
    return open(path, encoding='latin-1').read()


def tables():
    """the two tables of allcommands.h: (table, name, flags, precedence, handler, line)"""
    rows, which = [], None
    for n, line in enumerate(read(os.path.join(ROOT, 'Inc', 'allcommands.h')).split('\n'), 1):
        s = line.strip()
        if s.startswith('#ifdef INCLUDE_COMMAND_TABLE'):
            which = 'command'
        elif s.startswith('#ifdef INCLUDE_TOKEN_TABLE'):
            which = 'token'
        elif s.startswith('#endif'):
            which = None
        elif which and s.startswith('{'):
            m = re.match(r'\{\s*"((?:[^"\\]|\\.)*)"\s*,\s*([^,]+),\s*(\d+)\s*,\s*(\w+)', s)
            if m:
                name = m.group(1).replace('\\\\', '\\')
                flags = [f.strip() for f in m.group(2).split('|')]
                rows.append({'table': which, 'name': name, 'flags': flags, 'prec': int(m.group(3)),
                             'handler': m.group(4), 'line': n})
    return rows


def uncomment(text):
    """the text with its comments blanked (/* */ and //), its strings and its line numbers kept: a checkstring in
    code that is commented out (ONEWIRE SEARCH) is not in the build"""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c and text[j] != '\n':
                j += 2 if text[j] == '\\' else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r'[^\n]', ' ', text[i:j]))
            i = j
        elif text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            i = j
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def functions():
    """every top-level C function in Src/*.c: name -> (file, first line, body), comments blanked"""
    defs = {}
    head = re.compile(r'^[A-Za-z_][\w \t\*]*?\b(\w+)\s*\(([^;{}]*)\)\s*(\{.*|//.*)?$')   # (a one-line body too)
    for fn in sorted(os.listdir(SRC)):
        if not fn.endswith('.c'):
            continue
        lines = uncomment(read(os.path.join(SRC, fn))).split('\n')
        i = 0
        while i < len(lines):
            m = head.match(lines[i])
            if m and m.group(1) not in ('if', 'while', 'for', 'switch', 'return') and not lines[i].startswith(' '):
                j = i if (m.group(3) or '').startswith('{') else i + 1
                if j < len(lines) and '{' in lines[j]:
                    depth, k, body = 0, j, []
                    while k < len(lines):
                        code = re.sub(r'"(?:[^"\\]|\\.)*"|\'(?:[^\'\\]|\\.)*\'|//.*', '', lines[k])
                        depth += code.count('{') - code.count('}')
                        body.append(lines[k])
                        if depth <= 0 and k > j - 1 and '{' in ''.join(body):
                            break
                        k += 1
                    defs.setdefault(m.group(1), (fn, i + 1, body))
                    i = k
            i += 1
    return defs


CHECK = re.compile(r'checkstring\s*\(\s*[^,]+,\s*(?:\(\s*(?:unsigned\s+)?char\s*\*\s*\)\s*)?"((?:[^"\\]|\\.)*)"')
CHECK_ARRAY = re.compile(r'checkstring\s*\(\s*[^,]+,\s*(?:\(\s*(?:const\s+)?(?:unsigned\s+)?char\s*\*\s*\)\s*)?(\w+)\s*\[')
CALL = re.compile(r'\b([A-Za-z_]\w*)\s*\(')


def subkeywords(handler, defs, depth=2):
    """the checkstring literals reached from handler: [{'kw', 'func', 'file', 'line', 'via'}]"""
    found, seen = [], set()

    def walk(fname, level, via):
        if fname in seen or fname not in defs:
            return
        seen.add(fname)
        fn, start, body = defs[fname]
        text = '\n'.join(body)
        for off, line in enumerate(body):
            for kw in CHECK.findall(line):
                found.append({'kw': kw, 'func': fname, 'file': fn, 'line': start + off, 'via': via})
            for arr in CHECK_ARRAY.findall(line):      # checkstring(p, fkn[k]): the array's literals (OPTION F15)
                m = re.search(r'\b%s\s*\[\s*\]\s*=\s*\{([^}]*)\}' % re.escape(arr), text)
                for kw in re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1)) if m else []:
                    found.append({'kw': kw, 'func': fname, 'file': fn, 'line': start + off, 'via': via})
        if level < depth:
            for off, line in enumerate(body):
                code = re.sub(r'"(?:[^"\\]|\\.)*"|//.*', '', line)
                for callee in CALL.findall(code):
                    if callee not in STOP and callee != fname:
                        walk(callee, level + 1, via + [fname])

    walk(handler, 0, [])
    out, keys = [], set()
    for f in found:                                     # one row a keyword, the nearest place it is read first
        if f['kw'] not in keys:
            keys.add(f['kw'])
            out.append(f)
    return out


def main():
    rows = tables()
    defs = functions()
    fsrc = read(os.path.join(SRC, 'Functions.c'))
    m = re.search(r'overlaid_functions\[\]\s*=\s*\{(.*?)\};', fsrc, re.S)
    mmvars = re.findall(r'"([^"]+)"', m.group(1)) if m else []
    msrc = read(os.path.join(SRC, 'MMBasic.c'))
    m = re.search(r'FoldList\[\]\s*=\s*\{(.*?)\};', msrc, re.S)
    folds = re.findall(r'"([^"]+)"', m.group(1)) if m else []
    for r in rows:
        r['subkeywords'] = subkeywords(r['handler'], defs) if r['handler'] not in ('cmd_null', 'op_invalid') else []
        d = defs.get(r['handler'])
        r['where'] = '%s:%d' % (d[0], d[1]) if d else None
    # the words tokenise() writes as another command (SPRITE is BLIT, WII is CONTROLLER, CAT is INC ...), and its
    # text substitutions (MM.INFO$ is MM.INFO)
    aliases = [{'alias': a, 'is': b} for a, b in re.findall(
        r'checkstring\(p,\s*"([^"]+)"\)\)\s*!=\s*NULL\)\s*\{\s*match_i\s*=\s*GetCommandValue\("([^"]+)"\)', msrc)]
    if re.search(r"if\(\*p == '\?'\)\s*\{\s*match_i = GetCommandValue\(\"Print\"\)", msrc):
        aliases.insert(0, {'alias': '?', 'is': 'Print'})
    aliases += [{'alias': a, 'is': b} for a, b in re.findall(r'str_replace\(inpbuf,\s*"(MM\.[^"]+)",\s*"([^"]+)"', msrc)]
    # the loader's directives (#DEFINE, #COMMENT START, #INCLUDE ...): FileIO.c reads them after a '#'
    iosrc = read(os.path.join(SRC, 'FileIO.c'))
    directives = sorted({'#' + d.strip() for d in re.findall(
        r'(?:checkstring\(&sbuff\[1\],\s*"|cmpstr\(")([A-Z][A-Z ]*)"', iosrc)})
    inv = {'version': re.search(r'#define VERSION\s+"([^"]+)"', read(os.path.join(ROOT, 'Inc', 'Version.h'))).group(1),
           'keywords': rows, 'mm_variables': mmvars, 'folded_functions': folds, 'aliases': aliases,
           'directives': directives}
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    json.dump(inv, open(OUT, 'w', encoding='utf-8', newline='\n'), indent=1)
    kinds = {}
    for r in rows:
        k = 'operator' if 'T_OPER' in r['flags'] else 'function' if ('T_FUN' in r['flags'] or 'T_FNA' in r['flags']) \
            and r['table'] == 'token' else 'command' if r['table'] == 'command' else 'other'
        kinds[k] = kinds.get(k, 0) + 1
    nsub = sum(len(r['subkeywords']) for r in rows)
    missing = [r['name'] for r in rows if not r['where'] and r['handler'] not in ('cmd_null', 'op_invalid')]
    print('version %s: %s, %d MM. variables, %d folded functions, %d sub-keywords, %d aliases, %d directives' % (
        inv['version'], ', '.join('%d %ss' % (v, k) for k, v in sorted(kinds.items())), len(mmvars), len(folds), nsub,
        len(aliases), len(directives)))
    print('aliases:', ', '.join('%s=%s' % (a['alias'], a['is']) for a in aliases), '; directives:', ', '.join(directives))
    if missing:
        print('handlers not found:', ', '.join(missing))
    print('written', OUT)


if __name__ == '__main__':
    main()
