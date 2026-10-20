"""refblocks.py: the entries of a reference text file (docs/manual/ref/NAME.txt, reftext.py) as blocks of lines, for
the edits of the reference pass (docs/Manual_Plan.md, M4).

    python refblocks.py remove NAME LINE [LINE ...]     removes the entries starting at those lines
    python refblocks.py show NAME LINE [LINE ...]       prints them

A removed entry's group stays whole: when the first row of a group (===) goes and the row after it (=+=) stays, that
row starts the group. In Python, load() and save() give and take the blocks: (line, [text lines]), the header first
with line 0."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from reftext import REF


def path(name):
    return os.path.join(REF, name + '.txt')


def load(name):
    blocks, cur = [], (0, [])
    for n, l in enumerate(open(path(name), encoding='utf-8').read().split('\n'), 1):
        if l.startswith('=== ') or l.startswith('=+= '):
            blocks.append(cur)
            cur = (n, [l])
        else:
            cur[1].append(l)
    blocks.append(cur)
    return blocks


def save(name, blocks):
    open(path(name), 'w', encoding='utf-8', newline='\n').write('\n'.join(l for _, b in blocks for l in b))


def remove(name, lines):
    blocks = load(name)
    starts = {n for n, _ in blocks}
    missing = set(lines) - starts
    if missing:
        sys.exit('%s: no entry starts at line %s' % (name, ', '.join(map(str, sorted(missing)))))
    out, head_gone = [], False
    for n, b in blocks:
        if n in lines:
            if b[0].startswith('=== '):
                head_gone = True
            continue
        if b and head_gone and b[0].startswith('=+= '):
            b = ['=== ' + b[0][4:]] + b[1:]
        if n:
            head_gone = False if b[0].startswith('=== ') else head_gone
        out.append((n, b))
    save(name, out)
    print('%s: %d entries removed' % (name, len(lines)))


def text_blocks(text):
    """the blocks of a piece of text in the same format (staged entries): a leading part (comments) first, with line 0"""
    blocks, cur = [], (0, [])
    for l in text.split('\n'):
        if l.startswith('=== ') or l.startswith('=+= '):
            blocks.append(cur)
            cur = (-1, [l])
        else:
            cur[1].append(l)
    blocks.append(cur)
    return blocks


def find(blocks, first):
    """the index of the entry whose first line is (or starts with) first"""
    hits = [i for i, (n, b) in enumerate(blocks) if n and b[0] == first]
    if not hits:
        hits = [i for i, (n, b) in enumerate(blocks) if n and b[0].startswith(first)]
    if len(hits) != 1:
        sys.exit('%d entries start with %r' % (len(hits), first))
    return hits[0]


def group(blocks, i):
    """the indexes of entry i's group: it and the =+= rows after it"""
    j = i + 1
    while j < len(blocks) and blocks[j][1] and blocks[j][1][0].startswith('=+= '):
        j += 1
    return i, j


def replace_groups(blocks, indexes, text):
    """the groups at those indexes taken out, and the text's blocks put where the first was"""
    spans = sorted(group(blocks, i) for i in indexes)
    new = [b for b in text_blocks(text) if b[1] and any(l.strip() for l in b[1])]
    at = spans[0][0]
    for a, z in reversed(spans):
        del blocks[a:z]
    blocks[at:at] = new
    return blocks


def insert(blocks, i, text):
    """the text's blocks put before entry i"""
    new = [b for b in text_blocks(text) if b[1] and any(l.strip() for l in b[1])]
    blocks[i:i] = new
    return blocks


if __name__ == '__main__':
    cmd, name, nums = sys.argv[1], sys.argv[2], [int(x) for x in sys.argv[3:]]
    if cmd == 'remove':
        remove(name, set(nums))
    elif cmd == 'show':
        for n, b in load(name):
            if n in nums:
                print('\n'.join(b))
