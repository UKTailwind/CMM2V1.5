"""md2bb.py IN.md OUT.txt: a release note's Markdown as the Back Shed forum's BBCode (the docs/Release_Notes_*.backshed.txt
copies). Headings: the title centred, size 5, blue; ## size 4, blue, after two blank lines; ### size 3. A line that is
all *italic* is [I]. List items are bullets; items indented two spaces are hollow bullets after four non-breaking spaces,
which the forum keeps. **bold** is [B], `code` is Courier. The blank line after a heading is dropped."""
import re
import sys


def inline(s):
    s = re.sub(r'\*\*(.+?)\*\*', r'[B]\1[/B]', s)
    s = re.sub(r'`([^`]+)`', r'[FONT=Courier]\1[/FONT]', s)
    return s


def convert(md):
    out = []
    lines = md.replace('\r\n', '\n').split('\n')
    skip_blank = False
    for line in lines:
        if skip_blank and line.strip() == '':
            skip_blank = False
            continue
        skip_blank = False
        if line.startswith('# '):
            out.append('[CENTER][SIZE=5][COLOR=BLUE][B]%s[/B][/COLOR][/SIZE][/CENTER]' % inline(line[2:]))
            skip_blank = True
        elif line.startswith('## '):
            while out and out[-1] == '':
                out.pop()
            if out:
                out += ['', '']
            out.append('[SIZE=4][COLOR=BLUE][B]%s[/B][/COLOR][/SIZE]' % inline(line[3:]))
            skip_blank = True
        elif line.startswith('### '):
            out.append('[SIZE=3][B]%s[/B][/SIZE]' % inline(line[4:]))
            skip_blank = True
        elif re.match(r'^\*[^*].*\*$', line):
            out.append('[I]%s[/I]' % inline(line[1:-1]))
        elif line.startswith('  - '):
            out.append('\u00a0' * 4 + '\u25e6 ' + inline(line[4:]))      # (non-breaking spaces: the forum keeps them)
        elif line.startswith('- '):
            out.append('\u2022 ' + inline(line[2:]))
        else:
            out.append(inline(line))
    while out and out[-1] == '':
        out.pop()
    return '\n'.join(out) + '\n'


if __name__ == '__main__':
    md = open(sys.argv[1], encoding='utf-8').read()
    open(sys.argv[2], 'w', encoding='utf-8', newline='\n').write(convert(md))
