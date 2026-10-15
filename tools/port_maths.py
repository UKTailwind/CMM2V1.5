"""port_maths.py PICO_MATHS_C OUT_C - the PicoMite's core/MATHS.c rewritten for the CMM2.

Mechanical changes only; the CMM2-specific code is added by hand afterwards:
  - #ifdef rp2350 / #else / #endif: the rp2350 branch is kept (int dims, as the CMM2's);
    the includes of hardware/sync.h and pico/rand.h are dropped
  - #ifdef STRUCTENABLED: kept (the CMM2 has structures)
  - g_VarIndex -> VarIndex, g_OptionBase -> OptionBase
  - getcsargs(x, n) -> getargs(x, n, ",")
  - GetTempMainMemory -> GetTempMemory
  - unsigned char * -> char * (the CMM2's interpreter API takes char *), and the casts of
    string literals dropped
"""
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
d = open(src, encoding='latin-1').read().replace('\r\n', '\n')

# ---- includes ------------------------------------------------------------------------------
d = d.replace('#include "hardware/sync.h"\n', '')
d = d.replace('#ifdef rp2350\n#include "pico/rand.h"\n#endif\n', '')

# ---- conditional blocks --------------------------------------------------------------------
def resolve(text, name, keep_first):
    out = []
    lines = text.split('\n')
    i = 0
    while i < len(lines):
        l = lines[i]
        if l.strip() == '#ifdef ' + name:
            depth = 1
            first, second = [], []
            cur = first
            i += 1
            while i < len(lines):
                s = lines[i].strip()
                if s.startswith('#if'):
                    depth += 1
                elif s == '#endif':
                    depth -= 1
                    if depth == 0:
                        break
                elif s == '#else' and depth == 1:
                    cur = second
                    i += 1
                    continue
                cur.append(lines[i])
                i += 1
            out += resolve('\n'.join(first if keep_first else second), name, keep_first).split('\n') if (first if keep_first else second) else []
            i += 1
            continue
        out.append(l)
        i += 1
    return '\n'.join(out)

d = resolve(d, 'rp2350', True)
d = resolve(d, 'STRUCTENABLED', True)

# ---- names ---------------------------------------------------------------------------------
d = re.sub(r'\bg_VarIndex\b', 'VarIndex', d)
d = re.sub(r'\bg_OptionBase\b', 'OptionBase', d)
d = re.sub(r'\bGetTempMainMemory\b', 'GetTempMemory', d)
d = re.sub(r'getcsargs\(([^;]*?),\s*([^,;()]+)\)\s*;', r'getargs(\1, \2, ",");', d)

# ---- char types ----------------------------------------------------------------------------
d = d.replace('(unsigned char *)"', '"')
d = re.sub(r'\(unsigned char \*\)\s*', '(char *)', d)
d = re.sub(r'\bunsigned char \*', 'char *', d)
d = re.sub(r'\bunsigned char \*\*', 'char **', d)

open(dst, 'w', encoding='latin-1', newline='\r\n').write(d)
print('written', dst, d.count('\n') + 1, 'lines')
for w in ('getcsargs', 'g_', 'time_us_', 'get_rand_32', 'IntReady', 'save_and_disable', 'rp2350', 'STRUCTENABLED'):
    n = len(re.findall(r'\b' + re.escape(w), d))
    if n:
        print('  still', n, 'x', w)
