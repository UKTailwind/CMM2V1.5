"""v8notes.py: docs/Release_Notes_8.00.00.md, the full notes of version 8: each beta's notes (docs/Release_Notes_8.00.00bN.md),
newest first, their headings one level down. It is the release's Release_Notes_<version>_full.md asset
(docs/Release_Checklist.md, section 8). Run it again after a beta's notes change, or add the next beta to BETAS."""
import os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BETAS = ['8.00.00b2', '8.00.00b1']                       # newest first
OUT = os.path.join(ROOT, 'docs', 'Release_Notes_8.00.00.md')

parts = ['# Colour Maximite 2: MMBasic 8.00 release notes\n',
         '*The notes of every 8.00 beta, newest first. 8.00.00b1\'s are compared with 7.00.00RC6, the last release of '
         'version 7; each later beta\'s with the beta before it. Made by tools/v8notes.py from docs/Release_Notes_8.00.00bN.md.*\n']
for v in BETAS:
    text = open(os.path.join(ROOT, 'docs', 'Release_Notes_%s.md' % v), encoding='utf-8').read()
    lines = text.split('\n')
    assert lines[0].startswith('# '), v
    body = '\n'.join(lines[1:]).strip('\n')
    date = re.match(r'\*(\d+ \w+ \d{4})\.', body)
    body = re.sub(r'(?m)^(#+) ', lambda m: '#' + m.group(1) + ' ', body)       # (no code block here starts with #)
    parts.append('## %s%s\n\n%s\n' % (v, ' (%s)' % date.group(1) if date else '', body))
open(OUT, 'w', encoding='utf-8', newline='\n').write('\n'.join(parts))
print(OUT)
