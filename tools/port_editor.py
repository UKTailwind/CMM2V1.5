"""port_editor.py PICO_EDITOR_C OUT_C - the PicoMite's editor (misc/Editor.c without its MMBASIC_FM part) for the CMM2.

Mechanical changes only, as tools/port_fm.py makes for FM; the CMM2's platform layer is Inc/Editor_cmm2.h, and what
the compiler then finds is fixed by hand in the output (Editor_Port_Plan.md, E1):
  - the editor's lines: from "these two are the only global variables" to the start of the MMBASIC_FM block, and from
    the end of that block to the end of the file (the FM part is Src/FileManager.c)
  - the conditional blocks resolved for the CMM2 (COND below): no VGA, HDMI, web, touch or GUI controls; PICOMITE is
    taken as true, which leaves out the VGA builds' tile mouse cursor (D2 gives the CMM2 its own mouse)
  - getcsargs(x, n) -> getargs(x, n, ","); unsigned char * -> char * (char is unsigned on the ARM)
  - Option.DISPLAY_CONSOLE -> (OptionConsole & 2); Option.ColourCode -> EdColourCode (the editor turns it off for a
    file that is not BASIC); Option.continuation -> 0 (the CMM2 has no continuation lines)
  - the display helpers renamed (EdPutS, EdCursor, EdDisplay: Console.c has the CMM2's MX470 ones), HRes and VRes
    the write page's size (EdHRes, EdVRes), the serial console's output through the CMM2's (EdSerialPutS, EdSerialPutC)
"""
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
lines = open(src, encoding='latin-1').read().replace('\r\n', '\n').split('\n')

# ---- the editor's lines ------------------------------------------------------------------------
first = next(i for i, l in enumerate(lines) if 'these two are the only global variables' in l)
fmstart = next(i for i, l in enumerate(lines) if l.strip() == '#ifdef MMBASIC_FM' and i > 800)
depth = 0
for fmend in range(fmstart, len(lines)):
    s = lines[fmend].strip()
    if s.startswith('#if'):
        depth += 1
    elif s.startswith('#endif'):
        depth -= 1
        if depth == 0:
            break
last = max(i for i, l in enumerate(lines) if l.strip() == '#endif')      # the file's last #endif (LITE's)
body = lines[first:fmstart] + lines[fmend + 1:last + 1]

# ---- conditionals -----------------------------------------------------------------------------
TRUE = {'USBKEYBOARD', 'STRUCTENABLED', 'MMBASIC_FM', 'PICOMITE', 'rp2350'}
FALSE = {'PICOMITEVGA', 'HDMI', 'PICOMITEWEB', 'PICOMITEHDMIWEB', 'PICOMITEMIN', 'GUICONTROLS', 'LITE', 'PICOMITEBT',
         'PICOMITERP2350'}


def cond(expr):
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
    out, stack = [], []
    def active():
        return all(s[0] is not False for s in stack)
    for l in ls:
        s = l.strip()
        m = re.match(r'#\s*(ifdef|ifndef|if)\b(.*)', s)
        if m:
            kind, expr = m.group(1), m.group(2)
            c = cond('defined(%s)' % expr.strip()) if kind == 'ifdef' else \
                cond('!defined(%s)' % expr.strip()) if kind == 'ifndef' else cond(expr)
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
d = re.sub(r'\(unsigned char \*\*\)\s*', '(char **)', d)
d = re.sub(r'\bunsigned char \*\*', 'char **', d)
d = re.sub(r'\bunsigned char \*', 'char *', d)
d = d.replace('Option.DISPLAY_CONSOLE', '(OptionConsole & 2)')
d = d.replace('Option.ColourCode', 'EdColourCode')
d = d.replace('Option.continuation', '0')
for a, b in (('MX470PutS', 'EdPutS'), ('MX470Cursor', 'EdCursor'), ('MX470Display', 'EdDisplay'),
             ('HRes', 'EdHRes'), ('VRes', 'EdVRes'), ('SSPrintString', 'EdSerialPutS'),
             ('SerialConsolePutC', 'EdSerialPutC'), ('mytoupper', 'toupper'),
             ('PrintStatus', 'EdPrintStatus')):
    d = re.sub(r'\b%s\b' % a, b, d)
d = d.replace('extern void routinechecks(void);\n', '')         # (the CMM2's takes an argument: Editor_cmm2.h)

head = '''/***********************************************************************************************************************
Editor.c - EDIT, the PicoMite's full-screen editor (misc/Editor.c without its MMBASIC_FM part), for the CMM2 in place
of kilo.c (Editor_Port_Plan.md; Peter, 19 Oct 2026). Made by tools/port_editor.py from the PicoMite's source, then
fixed by hand where the CMM2 differs; the platform layer is Inc/Editor_cmm2.h. Built with EDITOR_ENGINE (the CMake
preset "Editor") until it replaces kilo.c.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#ifdef EDITOR_ENGINE
#include "Editor_cmm2.h"

'''
open(dst, 'w', encoding='latin-1', newline='\n').write(head + d + '\n#endif // EDITOR_ENGINE\n')
print('written', dst, d.count('\n') + 1, 'lines')
for w in ('getcsargs', 'DISPLAY_TYPE', 'nunstruct', 'tilefcols', 'llist', 'SaveToProgMemory', 'StartEditPoint',
          'ProgMemory', 'drivecheck', 'FatFSFileSystem', 'ExistsFile', 'FileSize', 'StandardError', 'MX470Scroll',
          'tud_cdc', 'SaveContext', 'RestoreContext', 'fm_edit_wants_run', 'uSec', 'MMgetchar', 'routinechecks'):
    n = len(re.findall(r'\b' + re.escape(w), d))
    if n:
        print('  still', n, 'x', w)
