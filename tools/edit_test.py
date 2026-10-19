"""edit_test.py [--port COM4] [--only 01,05] [--show]: the editor's test scripts (tests/editor/NN_*.txt) on the board.

Each script drives EDIT on the serial console (tools/vt100drive.py) and checks the screen, the console's output and
the files the editor leaves on the card. The scripts were written against kilo (Editor_Port_Plan.md, E0), and the
PicoMite editor's port must pass them before it replaces kilo (S). They run in A:/e, made if it is not there.

A script is one step a line ('#' starts a comment; the first comment line is the script's title):
  put NAME LOCAL     send tests/editor/LOCAL to the card as NAME (ZMODEM)
  kill NAME...       delete each NAME on the card, if it is there
  cmd TEXT           TEXT and Enter at the prompt (the console output is collected from here)
  type TEXT          TEXT a character at a time; Python escapes (\\t \\r \\xNN) work
  typen N TEXT       TEXT N times over (a long line)
  key NAME [N]       a key, N times (vt100drive.keybytes: f1, ^K, shift-tab, ...)
  wait SECS          read for SECS seconds
  prompt             wait for the prompt (the editor has exited)
  get NAME LOCAL     NAME, fetched by ZMODEM, has the bytes of tests/editor/LOCAL
  exists NAME        the card has NAME
  missing NAME       it has not
  screen TEXT        the screen shows TEXT;           noscreen TEXT: it does not
  status TEXT        the status bar (the second-last row) holds TEXT
  message TEXT       the message line (the last row) holds TEXT
  output TEXT        the console output since the last cmd holds TEXT;   nooutput TEXT: it does not
  line N TEXT        row N of the screen starts with TEXT (rows from 0)
  colour NAME TEXT   every character of TEXT, where it first is on the screen, has the foreground NAME: the colour
                     coding's VT100 colours (COLOURS: normal, comment, keyword, quote, number)
  show               print the screen (for writing a script)
  requires EXPR      the script runs only where the board prints 1 for EXPR, and is otherwise reported as SKIP:
                     what it needs that is not always there (a USB drive: mm.info(exists dir "C:/"))
  known TEXT         kilo fails this script, for the reason TEXT: its failure is reported as KNOWN, not FAIL (the
                     script still says what the editor should do, and the new editor has to pass it)
A failed check is reported and the script goes on; a step that cannot be done (no prompt, a transfer that fails, a
key when the editor has closed) ends the script, and the board is brought back to the prompt (Esc three times, then
Ctrl-C) for the next one.
"""
import contextlib
import io
import argparse
import codecs
import glob
import os
import shutil
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vt100drive
import zmodem_pc

HERE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tests', 'editor')
CARDDIR = 'A:/e'
# the colour coding on a terminal (Inc/Editor_cmm2.h, VT100_C_*): SGR foreground codes
COLOURS = {'normal': 37, 'comment': 33, 'keyword': 36, 'quote': 35, 'number': 32}


class Abort(Exception):
    pass


class Skip(Exception):
    pass


class Run:
    def __init__(self, term, show):
        self.t = term
        self.show = show
        self.fails = []

    def check(self, ok, what):
        if not ok:
            self.fails.append(what)

    def at_prompt(self, line, timeout=20.0):
        """a command at the prompt; its output"""
        self.t.cmd(line)
        try:
            return self.t.prompt(timeout)
        except TimeoutError as e:
            raise Abort(str(e))

    def fresh_prompt(self):
        """after a transfer: Enter, and the prompt it brings"""
        self.t.settle(0.5)
        self.at_prompt('')

    def recover(self):
        """back to the prompt, whatever was open: Esc, Y and Enter leave the PicoMite editor ("Exit and discard all
        changes (Y/N)"), three more Escs leave kilo (and FM), then Ctrl-C and Enter"""
        self.t.send(b'\x1b', settle=False)
        time.sleep(0.4)
        self.t.send(b'Y\r', settle=False)
        time.sleep(0.6)
        for _ in range(3):
            self.t.send(b'\x1b', settle=False)
            time.sleep(0.3)
        self.t.settle(0.5)
        self.t.b.s.write(b'\x03')
        time.sleep(0.3)
        self.t.mark()
        self.t.b.s.write(b'\r')
        try:
            self.t.prompt(10)
        except TimeoutError:
            pass

    def put(self, name, local):
        d = tempfile.mkdtemp()
        try:
            shutil.copyfile(os.path.join(HERE, local), os.path.join(d, name))
            self.kill([name])
            self.t.mark()
            self.t.b.s.write(b'ZMODEM RECEIVE\r')
            time.sleep(0.5)
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    zmodem_pc.send(self.t.b.s, self.t.b.baud, [os.path.join(d, name)], 32768)
            except SystemExit as e:
                raise Abort('put %s: %s' % (name, e))
            self.fresh_prompt()
        finally:
            shutil.rmtree(d, ignore_errors=True)

    def zmodem_started(self, timeout=10.0):
        """wait for the board's ZMODEM sender to begin (its ZRQINIT, '**' ZDLE 'B'): the receiver's handshake sent
        before that would reach the prompt as typed text, where '**...' is RUN of a name that is not one"""
        t0, seen = time.time(), b''
        while time.time() - t0 < timeout:
            seen += self.t.b.s.read(64)
            if b'**\x18B' in seen:
                return True
        return False

    def get(self, name):
        d = tempfile.mkdtemp()
        try:
            self.t.mark()
            self.t.settle(0.3)
            self.t.b.s.write(('ZMODEM SEND "%s"\r' % name).encode('latin-1'))
            if not self.zmodem_started():
                raise Abort('get %s: the board did not start sending' % name)
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    zmodem_pc.recv(self.t.b.s, self.t.b.baud, d)
            except SystemExit as e:
                raise Abort('get %s: %s' % (name, e))
            self.fresh_prompt()                         # (the receiver may have read the prompt)
            files = os.listdir(d)
            if len(files) != 1:
                raise Abort('get %s: %d files came' % (name, len(files)))
            return open(os.path.join(d, files[0]), 'rb').read()
        finally:
            shutil.rmtree(d, ignore_errors=True)

    def exists(self, name):
        out = self.at_prompt('print mm.info(exists file "%s")' % name)
        return out.replace('\r', '').split('\n')[1].strip() == '1' if '\n' in out else False

    def kill(self, names):
        for n in names:
            self.at_prompt('if mm.info(exists file "%s") then kill "%s"' % (n, n))

    def step(self, line):
        verb, _, arg = line.partition(' ')
        t = self.t
        if verb == 'put':
            name, local = arg.split()
            self.put(name, local)
        elif verb == 'kill':
            self.kill(arg.split())
        elif verb == 'cmd':
            t.cmd(arg)
            t.settle(0.5)
        elif verb in ('type', 'typen', 'key'):
            if t.at_prompt():                           # (the editor has closed: keys now would be commands)
                last = [l for l in t.text().replace('\r', '').split('\n') if l.strip() and l.strip() != '>']
                raise Abort('the editor has closed' + (': ' + last[-1].strip() if last else ''))
            if verb == 'type':
                t.type(codecs.decode(arg.encode('latin-1'), 'unicode_escape'))
            elif verb == 'typen':
                n, _, text = arg.partition(' ')
                t.type(codecs.decode(text.encode('latin-1'), 'unicode_escape') * int(n))
            else:
                p = arg.split()
                t.key(p[0], int(p[1]) if len(p) > 1 else 1)
        elif verb == 'requires':
            out = self.at_prompt('print ' + arg).replace('\r', '').split('\n')
            if len(out) < 2 or out[1].strip() != '1':
                raise Skip(arg)
        elif verb == 'known':
            self.known = arg
        elif verb == 'wait':
            t0 = time.time()
            while time.time() - t0 < float(arg):
                t.read()
        elif verb == 'prompt':
            try:
                t.prompt()
            except TimeoutError as e:
                raise Abort(str(e))
        elif verb == 'get':
            name, local = arg.split()
            want = open(os.path.join(HERE, local), 'rb').read()
            got = self.get(name)
            if got != want:
                self.fails.append('get %s: %d bytes, not the %d of %s; first difference at %d' % (
                    name, len(got), len(want), local,
                    next((i for i in range(min(len(got), len(want))) if got[i] != want[i]), min(len(got), len(want)))))
                with open(os.path.join(tempfile.gettempdir(), 'edit_test_' + name), 'wb') as f:
                    f.write(got)
        elif verb == 'exists':
            self.check(self.exists(arg), 'exists ' + arg)
        elif verb == 'missing':
            self.check(not self.exists(arg), 'missing ' + arg)
        elif verb == 'screen':
            t.settle(0.3)
            self.check(t.screen.find(arg) is not None, 'screen: ' + arg)
        elif verb == 'noscreen':
            t.settle(0.3)
            self.check(t.screen.find(arg) is None, 'noscreen: ' + arg)
        elif verb == 'status':
            t.settle(0.3)
            self.check(arg in t.screen.status(), 'status: %r not in %r' % (arg, t.screen.status()))
        elif verb == 'message':
            t.settle(0.3)
            self.check(arg in t.screen.message(), 'message: %r not in %r' % (arg, t.screen.message()))
        elif verb == 'output':
            self.check(arg in t.text(), 'output: ' + arg)
        elif verb == 'nooutput':
            self.check(arg not in t.text(), 'nooutput: ' + arg)
        elif verb == 'line':
            n, _, text = arg.partition(' ')
            t.settle(0.3)
            self.check(t.screen.line(int(n)).startswith(text), 'line %s: %r, not %r' % (n, t.screen.line(int(n)), text))
        elif verb == 'colour':
            name, _, text = arg.partition(' ')
            t.settle(0.3)
            at = t.screen.find(text)
            if at is None:
                self.fails.append('colour %s: %r is not on the screen' % (name, text))
            else:
                r, c = at
                got = [t.screen.at[r][c + i][0] for i in range(len(text))]
                self.check(all(g == COLOURS[name] for g in got), 'colour %s %r (%d): %s' % (
                    name, text, COLOURS[name], got))
        elif verb == 'show':
            t.settle(0.3)
            t.screen.show()
        else:
            raise Abort('no step ' + verb)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port', default='COM4')
    ap.add_argument('--only', default='')
    ap.add_argument('--show', action='store_true', help='print the screen when a script fails')
    a = ap.parse_args()
    only = [x for x in a.only.split(',') if x]
    scripts = sorted(glob.glob(os.path.join(HERE, '[0-9][0-9]*.txt')))     # (the scripts: their number first)
    if only:
        scripts = [s for s in scripts if any(os.path.basename(s).startswith(o) for o in only)]
    term = vt100drive.Term(a.port, 460800)
    run = Run(term, a.show)
    run.recover()
    run.at_prompt('if not mm.info(exists dir "%s") then mkdir "%s"' % (CARDDIR, CARDDIR))
    run.at_prompt('chdir "%s"' % CARDDIR)
    passed = known = skipped = 0
    for path in scripts:
        name = os.path.basename(path)[:-4]
        lines = [l.rstrip('\n') for l in open(path, encoding='latin-1')]
        title = next((l.lstrip('# ').strip() for l in lines if l.startswith('#')), '')
        run.fails = []
        run.known = ''
        t0 = time.time()
        try:
            for l in lines:
                l = l.strip()
                if l and not l.startswith('#'):
                    run.step(l)
        except Skip as e:
            print('SKIP  %-22s         %s (it needs %s)' % (name, title, e))
            skipped += 1
            continue
        except Abort as e:
            run.fails.append('ABORT: ' + str(e).split('\n')[0])
            run.recover()
            try:
                run.at_prompt('chdir "%s"' % CARDDIR)
            except Abort as e2:
                print('the board did not come back to the prompt: ' + str(e2).split('\n')[0])
                run.recover()
        secs = time.time() - t0
        if run.fails:
            print('%s %-22s %5.1fs  %s' % ('KNOWN' if run.known else 'FAIL ', name, secs, title))
            if run.known:
                print('       (known: %s)' % run.known)
                known += 1
            for f in run.fails:
                print('       ' + f)
            if a.show:
                term.screen.show()
        else:
            passed += 1
            print('PASS  %-22s %5.1fs  %s%s' % (name, secs, title, ' (was known: %s)' % run.known if run.known else ''))
    print('%d of %d passed%s%s' % (passed, len(scripts), ', %d known failures' % known if known else '',
                                   ', %d skipped' % skipped if skipped else ''))
    term.close()
    sys.exit(0 if passed + known + skipped == len(scripts) else 1)


if __name__ == '__main__':
    main()
