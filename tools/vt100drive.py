"""vt100drive.py: drive a full-screen program (EDIT, FM) on the CMM2's serial console, and read its screen.

As a library (tools/edit_test.py):
    t = Term('COM4', 460800)
    t.cmd('EDIT "t.bas"'); t.settle()          # a line at the prompt; then read until the output goes quiet
    t.key('f1'); t.type('PRINT 1')             # keys as a terminal sends them, text a character at a time
    t.screen.find('PRINT')                     # (row, col) of a text on the screen, or None
    t.screen.status()                          # the editor's status bar (the second-last row)
    t.prompt()                                 # wait for the prompt; the output since the last mark()

As a command (the FM tests of 19 Oct):
    python -I tools/vt100drive.py STEP...
      cmd:TEXT  type:TEXT  key:NAME  wait:SECS  show  prompt

Keys. Those a terminal has are sent as it sends them (VT100: ESC [ A, ESC [ 1 1 ~ for F1, ...). Those it has not,
the shifted keys of the CMM2's own keyboard, are sent as the codes that keyboard puts in the console queue
(Hardware_Includes.h: SHIFT_TAB 0x9F, SHIFT_DEL 0xA0, DOWNSEL 0xA1, RIGHTSEL 0xA3): the serial console passes bytes
of 0x80 and over through unchanged (checked 19 Oct), so the editor cannot tell them from the keyboard's.

The screen is a small VT100: cursor moves (H f A B C D, ESC 7/8, ESC[s/u), erase (2J, K), SGR (foreground,
background and reverse, kept for each cell), the size from the terminal resize the CMM2 asks for (ESC[8;rows;colst).
"""
import re
import sys
import time
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import goldens

KEYS = {
    'up': b'\x1b[A', 'down': b'\x1b[B', 'right': b'\x1b[C', 'left': b'\x1b[D',
    'home': b'\x1b[1~', 'ins': b'\x1b[2~', 'del': b'\x1b[3~', 'end': b'\x1b[4~', 'pgup': b'\x1b[5~', 'pgdn': b'\x1b[6~',
    'f1': b'\x1b[11~', 'f2': b'\x1b[12~', 'f3': b'\x1b[13~', 'f4': b'\x1b[14~', 'f5': b'\x1b[15~',
    'f6': b'\x1b[17~', 'f7': b'\x1b[18~', 'f8': b'\x1b[19~', 'f9': b'\x1b[20~', 'f10': b'\x1b[21~',
    'f11': b'\x1b[23~', 'f12': b'\x1b[24~',
    'tab': b'\t', 'enter': b'\r', 'esc': b'\x1b', 'bksp': b'\x08',
    # the CMM2 keyboard's own codes (no VT100 sequence)
    'shift-tab': b'\x9f', 'shift-del': b'\xa0', 'shift-down': b'\xa1', 'shift-left': b'\xa2', 'shift-right': b'\xa3',
}


def keybytes(name):
    """the bytes for a key name: KEYS, ^X for a control key, or 0xNN for a raw code"""
    k = name.lower()
    if k in KEYS:
        return KEYS[k]
    if len(k) == 2 and k[0] == '^':
        return bytes([ord(k[1].upper()) & 0x1f])
    if k.startswith('0x'):
        return bytes([int(k, 16)])
    raise KeyError('no key ' + name)


class Screen:
    """what the terminal would show: characters, and for each cell (fg, bg, reverse)"""
    def __init__(self, rows=40, cols=100):
        self.resize(rows, cols)
        self.pending = ''

    def resize(self, rows, cols):
        self.rows, self.cols = rows, cols
        self.ch = [[' '] * cols for _ in range(rows)]
        self.at = [[(None, None, False)] * cols for _ in range(rows)]
        self.r = self.c = 0
        self.fg, self.bg, self.rev = None, None, False
        self.saved = (0, 0)

    def clear(self):
        self.ch = [[' '] * self.cols for _ in range(self.rows)]
        self.at = [[(None, None, False)] * self.cols for _ in range(self.rows)]

    def put(self, ch):
        if 0 <= self.r < self.rows and 0 <= self.c < self.cols:
            self.ch[self.r][self.c] = ch
            self.at[self.r][self.c] = (self.fg, self.bg, self.rev)
        self.c += 1

    def sgr(self, params):
        p = [int(x) for x in params.split(';') if x.isdigit()] or [0]
        for n in p:
            if n == 0:
                self.fg, self.bg, self.rev = None, None, False
            elif n == 7:
                self.rev = True
            elif n == 27:
                self.rev = False
            elif 30 <= n <= 37 or 90 <= n <= 97:
                self.fg = n
            elif n == 39:
                self.fg = None
            elif 40 <= n <= 47 or 100 <= n <= 107:
                self.bg = n
            elif n == 49:
                self.bg = None

    def feed(self, text):
        s = self.pending + text
        self.pending = ''
        i = 0
        while i < len(s):
            ch = s[i]
            if ch == '\x1b':
                m = re.match(r'\x1b\[([?0-9;]*)([A-Za-z@])', s[i:])
                if m:
                    params, f = m.group(1), m.group(2)
                    i += m.end()
                    if f in 'Hf':
                        p = [int(x) if x else 1 for x in params.split(';')] if params else [1, 1]
                        self.r, self.c = p[0] - 1, (p[1] - 1 if len(p) > 1 else 0)
                    elif f == 'J':
                        if params in ('2', '3'):
                            self.clear()
                    elif f == 'K':
                        if 0 <= self.r < self.rows:
                            for c in range(max(self.c, 0), self.cols):
                                self.ch[self.r][c] = ' '
                                self.at[self.r][c] = (None, self.bg, False)
                    elif f == 'm':
                        self.sgr(params)
                    elif f == 't':
                        p = params.split(';')
                        if len(p) == 3 and p[0] == '8':
                            self.resize(int(p[1]), int(p[2]))
                    elif f == 's':
                        self.saved = (self.r, self.c)
                    elif f == 'u':
                        self.r, self.c = self.saved
                    elif f in 'ABCD':
                        n = int(params or 1)
                        self.r += {'A': -n, 'B': n}.get(f, 0)
                        self.c += {'C': n, 'D': -n}.get(f, 0)
                    continue
                if s[i + 1:i + 2] in ('7', '8'):
                    if s[i + 1] == '7':
                        self.saved = (self.r, self.c)
                    else:
                        self.r, self.c = self.saved
                    i += 2
                    continue
                if len(s) - i < 16 and not re.search(r'[A-Za-z@~]', s[i + 1:]):
                    self.pending = s[i:]                # (the rest of the sequence comes with the next read)
                    return
                i += 1
                continue
            if ch == '\r':
                self.c = 0
            elif ch == '\n':
                self.r += 1
                if self.r >= self.rows:
                    self.ch.pop(0)
                    self.ch.append([' '] * self.cols)
                    self.at.pop(0)
                    self.at.append([(None, None, False)] * self.cols)
                    self.r = self.rows - 1
            elif ch == '\b':
                self.c = max(0, self.c - 1)
            elif ch >= ' ':
                self.put(ch)
            i += 1

    def line(self, r):
        return ''.join(self.ch[r]).rstrip() if 0 <= r < self.rows else ''

    def lines(self):
        return [self.line(r) for r in range(self.rows)]

    def find(self, text):
        for r in range(self.rows):
            c = ''.join(self.ch[r]).find(text)
            if c >= 0:
                return r, c
        return None

    def status(self):
        """the editor's status bar: the second-last row"""
        return self.line(self.rows - 2)

    def message(self):
        """the editor's message line: the last row"""
        return self.line(self.rows - 1)

    def show(self, out=sys.stdout):
        last = max([r for r in range(self.rows) if self.line(r)] or [0])
        out.write('+' + '-' * self.cols + '+  (%dx%d, cursor %d,%d)\n' % (self.cols, self.rows, self.r, self.c))
        for r in range(last + 1):
            marks = []
            for m in re.finditer(r'R+', ''.join('R' if a[2] else ' ' for a in self.at[r])):
                marks.append('rev@%d+%d' % (m.start(), len(m.group(0))))
            for m in re.finditer(r'(B+)', ''.join('B' if a[1] in (46, 47) else ' ' for a in self.at[r])):
                marks.append('bg@%d+%d' % (m.start(), len(m.group(0))))
            out.write('|' + ''.join(self.ch[r]) + '|%2d' % r + ('  <' + ','.join(marks) + '>' if marks else '') + '\n')
        out.write('+' + '-' * self.cols + '+\n')


class Term:
    """the board's console with a Screen fed by everything it sends"""
    def __init__(self, port='COM4', baud=460800):
        self.b = goldens.Board(port, baud)
        self.screen = Screen()
        self.log = ''                                   # the console output since mark()
        self.b.drain(0.2)

    def close(self):
        self.b.close()

    def read(self):
        t = self.b._read()
        if t:
            self.screen.feed(t)
            self.log += t
        return t

    def mark(self):
        self.log = ''

    def text(self):
        """the output since mark(), without escape sequences"""
        return goldens.ANSI.sub('', self.log)

    def settle(self, quiet=0.4, limit=30.0):
        """read until the console has been quiet for quiet seconds (at most limit); the seconds it took"""
        t0 = last = time.time()
        while time.time() - last < quiet and time.time() - t0 < limit:
            if self.read():
                last = time.time()
        return last - t0

    def send(self, data, settle=True):
        self.b.s.write(data)
        if settle:
            self.settle()

    def cmd(self, line, settle=False):
        self.mark()
        self.send(line.encode('latin-1') + b'\r', settle)

    def type(self, text, gap=0.02):
        for ch in text:
            self.b.s.write(ch.encode('latin-1'))
            time.sleep(gap)
            self.read()
        self.settle()

    def key(self, name, n=1):
        for _ in range(n):
            self.send(keybytes(name), settle=False)
            time.sleep(0.05)
            self.read()
        self.settle()

    def at_prompt(self):
        """the screen's cursor is after a '> ' alone on its row (EDIT clears the screen and homes the cursor before
        the prompt, so no newline comes before it)"""
        s = self.screen
        return s.c == 2 and s.line(s.r) == '>'

    def prompt(self, timeout=20.0):
        """wait for the '> ' prompt; the output since mark(), as text"""
        t0 = time.time()
        while time.time() - t0 < timeout:
            self.read()
            clean = self.text()
            if clean.endswith('\n> ') or clean.endswith('\n>') or clean in ('> ', '>') or \
                    (clean.endswith('> ') and self.at_prompt()):
                time.sleep(0.05)
                if not self.read():
                    return clean
        raise TimeoutError('no prompt after %.0fs; last output:\n%s' % (timeout, self.text()[-600:]))


def main():
    t = Term()
    for step in sys.argv[1:]:
        kind, _, arg = step.partition(':')
        if kind == 'cmd':
            t.cmd(arg, settle=True)
        elif kind == 'type':
            t.type(arg)
        elif kind == 'key':
            t.key(arg)
        elif kind == 'wait':
            t0 = time.time()
            while time.time() - t0 < float(arg):
                t.read()
        elif kind == 'show':
            t.settle(0.3)
            t.screen.show()
        elif kind == 'prompt':
            print(t.prompt()[-600:])
        else:
            sys.exit('no step ' + step)
    t.close()


if __name__ == '__main__':
    main()
