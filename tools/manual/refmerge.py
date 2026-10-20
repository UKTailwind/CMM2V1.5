"""refmerge.py STAGE.txt [...]: applies staged changes to the reference text files (docs/manual/ref/), for the reference
pass of M4 (docs/Manual_Plan.md). A staged file holds actions, each followed by its entries in the text format of
reftext.py:

    # ACTION REPLACE commands.txt 1234 "=== DRIVE drive$"       the group at that line (a "=+= " line: that row only)
    # ACTION REMOVE commands.txt 1234 "=== DRIVE drive$"        removes it the same way
    # ACTION ADD commands.txt BEFORE 1234 "=== DUMP ..."        puts the entries before the entry at that line

Line numbers are those of the files when the actions were written, and each is checked against its quoted first line.
The actions of all the files given are applied together, from the bottom of each text file up, so that one action does
not move the lines of another. Comment lines ("# ") in the entries are dropped."""
import re, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import refblocks as rb

ACT = re.compile(r'^# ACTION (REPLACE|REMOVE|ADD) (\w+)\.txt (?:BEFORE )?(\d+) "(.*)"\s*$')


def actions(path):
    out, cur = [], None
    for l in open(path, encoding='utf-8').read().split('\n'):
        m = ACT.match(l)
        if m:
            cur = {'op': m.group(1), 'file': m.group(2), 'line': int(m.group(3)), 'first': m.group(4), 'text': []}
            out.append(cur)
        elif l.startswith('# ACTION'):
            sys.exit('%s: an action not understood: %s' % (path, l))
        elif cur is not None and not l.startswith('# ') and l != '#':
            cur['text'].append(l)
    return out


def main(paths):
    acts = [a for p in paths for a in actions(p)]
    files = {}
    for a in acts:
        files.setdefault(a['file'], []).append(a)
    for name, lst in files.items():
        blocks = rb.load(name)
        at = {n: i for i, (n, b) in enumerate(blocks) if n}
        for a in sorted(lst, key=lambda a: (-a['line'], a['op'] == 'ADD')):           # (at one line: the replace before the add)
            i = at.get(a['line'])
            if i is None:
                sys.exit('%s.txt: no entry at line %d (%s)' % (name, a['line'], a['first']))
            if blocks[i][1][0].strip() != a['first'].strip():
                sys.exit('%s.txt:%d is %r, not %r' % (name, a['line'], blocks[i][1][0], a['first']))
            text = '\n'.join(a['text'])
            if a['op'] == 'ADD':
                rb.insert(blocks, i, text)
                continue
            if blocks[i][1][0].startswith('=== '):
                a0, z = rb.group(blocks, i)
            else:
                a0, z = i, i + 1
            new = [b for b in rb.text_blocks(text) if b[1] and any(l.strip() for l in b[1])] if a['op'] == 'REPLACE' else []
            blocks[a0:z] = new
        rb.save(name, blocks)
        print('%s.txt: %d actions' % (name, len(lst)))


if __name__ == '__main__':
    main(sys.argv[1:])
