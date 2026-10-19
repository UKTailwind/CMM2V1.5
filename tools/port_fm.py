"""port_fm.py PICO_EDITOR_C OUT_C - the PicoMite's file manager (FM, misc/Editor.c's MMBASIC_FM part) for the CMM2.

Mechanical changes only; the CMM2's platform layer is FileManager_cmm2.h, and anything the compiler then finds is
fixed by hand in the output:
  - the lines between "#ifdef MMBASIC_FM" and its "#endif" are taken
  - the conditional blocks are resolved for the CMM2 (COND below): rp2350, HAS_USB_MSC, USBKEYBOARD and NOVS1053
    are true; PICOMITEVGA, HDMI, PICOMITE, PICOMITEMIN, PICOMITEWEB and GUICONTROLS are false (the CMM2 sets its
    display up as EDIT does, and has no touch, web or VS1053)
  - getcsargs(x, n) -> getargs(x, n, ",")
  - unsigned char * -> char * (the CMM2's interpreter API takes char *), and the casts of string literals dropped
  - Option.DISPLAY_CONSOLE -> (OptionConsole & 2), the CMM2's display console
  - MMErrMsg[0] = 0 -> MMErrMsg = "" (the CMM2's MMErrMsg points to the message: it may be a constant string)
"""
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
lines = open(src, encoding='latin-1').read().replace('\r\n', '\n').split('\n')

# ---- the FM part: the largest of Editor.c's MMBASIC_FM blocks (the others are the editor's hooks for FM) ----------
blocks = []
for start in [i for i, l in enumerate(lines) if l.strip() == '#ifdef MMBASIC_FM']:
    depth = 0
    for i in range(start, len(lines)):
        s = lines[i].strip()
        if s.startswith('#if'):
            depth += 1
        elif s.startswith('#endif'):
            depth -= 1
            if depth == 0:
                blocks.append((i - start, start, i))
                break
_, start, end = max(blocks)
body = lines[start + 1:end]

# ---- conditionals -----------------------------------------------------------------------------
TRUE = {'rp2350', 'HAS_USB_MSC', 'USBKEYBOARD', 'NOVS1053', 'MMBASIC_FM'}
FALSE = {'PICOMITEVGA', 'HDMI', 'PICOMITE', 'PICOMITEMIN', 'PICOMITEWEB', 'GUICONTROLS'}


def cond(expr):
    """True/False for a condition made of the names above; None to keep the block as it is"""
    e = expr.strip()
    e = re.sub(r'defined\s*\(\s*(\w+)\s*\)', r'\1', e)
    e = re.sub(r'defined\s+(\w+)', r'\1', e)
    names = set(re.findall(r'[A-Za-z_]\w*', e))
    if not names or not names <= (TRUE | FALSE):
        return None
    py = e.replace('&&', ' and ').replace('||', ' or ')
    py = re.sub(r'!(?!=)', ' not ', py)
    for n in names:
        py = re.sub(r'\b%s\b' % n, 'True' if n in TRUE else 'False', py)
    return bool(eval(py))


def resolve(ls):
    out, stack = [], []          # stack entries: [state, taken] state: 'keep' (unresolved: copy the directives), True, False
    def active():
        return all(s[0] is not False for s in stack)
    for l in ls:
        s = l.strip()
        m = re.match(r'#\s*(ifdef|ifndef|if)\b(.*)', s)
        if m:
            kind, expr = m.group(1), m.group(2)
            if kind == 'ifdef':
                c = cond('defined(%s)' % expr.strip())
            elif kind == 'ifndef':
                c = cond('!defined(%s)' % expr.strip())
            else:
                c = cond(expr)
            if c is None:
                stack.append(['keep', False])
                if active():
                    out.append(l)
            else:
                stack.append([c, c])
            continue
        m = re.match(r'#\s*elif\b(.*)', s)
        if m and stack:
            top = stack[-1]
            if top[0] == 'keep':
                if active():
                    out.append(l)
                continue
            c = cond(m.group(1))
            if c is None:
                raise SystemExit('an #elif that cannot be resolved: ' + s)
            top[0] = (not top[1]) and c
            top[1] = top[1] or c
            continue
        if re.match(r'#\s*else\b', s) and stack:
            top = stack[-1]
            if top[0] == 'keep':
                if active():
                    out.append(l)
                continue
            top[0] = not top[1]
            top[1] = True
            continue
        if re.match(r'#\s*endif\b', s) and stack:
            top = stack.pop()
            if top[0] == 'keep' and active():
                out.append(l)
            continue
        if active():
            out.append(l)
    assert not stack, 'unbalanced conditionals'
    return out


body = resolve(body)
d = '\n'.join(body)

# ---- names and types ------------------------------------------------------------------------
d = re.sub(r'getcsargs\(([^;]*?),\s*([^,;()]+)\)\s*;', r'getargs(\1, \2, ",");', d)
d = d.replace('(unsigned char *)"', '"')
d = re.sub(r'\(unsigned char \*\)\s*', '(char *)', d)
d = re.sub(r'\bunsigned char \*\*', 'char **', d)
d = re.sub(r'\bunsigned char \*', 'char *', d)
d = d.replace('Option.DISPLAY_CONSOLE', '(OptionConsole & 2)')
d = re.sub(r'MMErrMsg\[0\]\s*=\s*0\s*;', 'MMErrMsg = "";', d)

head = '''/***********************************************************************************************************************
FileManager.c - FM, the PicoMite's two-panel file manager (misc/Editor.c, its MMBASIC_FM part), for the CMM2 (Peter,
19 Oct 2026). Made by tools/port_fm.py from the PicoMite's source, then fixed by hand where the CMM2 differs; the
platform layer is FileManager_cmm2.h. The CMM2's drives: A: (or B:, the PicoMite's name for it) the SD card, C: a
USB flash drive; the PicoMite's A:, its flash drive, does not exist here.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "FileManager_cmm2.h"

'''
open(dst, 'w', encoding='latin-1', newline='\n').write(head + d + '\n')
print('written', dst, d.count('\n') + 1, 'lines')
for w in ('lfs_', 'getcsargs', 'DISPLAY_TYPE', 'SCREENMODE', 'time_us_64', 'FatFSFileSystem', 'filesource'):
    n = len(re.findall(r'\b' + re.escape(w), d))
    if n:
        print('  still', n, 'x', w)
