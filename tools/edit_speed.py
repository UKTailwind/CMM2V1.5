"""edit_speed.py [LINES]: the editor's response times on a large file (Editor_Port_Plan.md, E6), on the board on COM4,
at the file's top and at its end.

The file is LINES lines (4000 by default: 284 KB) of 'Print "line nnnnn ..."', sent to A:/e/big.bas and deleted after.
Each action is a key sent; its time is from the send to the last byte the board sends back for it (read until the
console has been quiet for 60 ms), so it includes the serial line: about 60 ms for a key, about 150 ms for a full
redraw (3.8 KB). The output is the same at the top and at the end, so the difference is the editor's own work there.
Before E6 a redraw at the end of 284 KB took 0.4 s more than at the top; after, the same."""
import os, sys, time, tempfile, contextlib, io
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vt100drive, zmodem_pc

LINES = int(sys.argv[1]) if len(sys.argv) > 1 else 4000
text = ''.join('Print "line %05d %s"\r\n' % (i, 'abcdefghij' * 5) for i in range(LINES))
t = vt100drive.Term()
t.cmd('chdir "A:/e"', settle=True)
d = tempfile.mkdtemp()
open(os.path.join(d, 'big.bas'), 'w', newline='').write(text)
t.cmd('if mm.info(exists file "big.bas") then kill "big.bas"', settle=True)
t.b.s.write(b'ZMODEM RECEIVE\r')
time.sleep(0.5)
with contextlib.redirect_stdout(io.StringIO()):
    zmodem_pc.send(t.b.s, t.b.baud, [os.path.join(d, 'big.bas')], 32768)
t.settle(0.5)
t.b.s.write(b'\r')
t.settle(0.5)
print('file: %d lines, %d bytes' % (LINES, len(text)))


def timed(label, data, quiet=0.06, limit=10.0):
    t.settle(0.3)
    n = 0
    t0 = time.time()
    t.b.s.write(data)
    last = t0
    while time.time() - last < quiet and time.time() - t0 < limit:
        x = t.b._read()
        if x:
            t.screen.feed(x)
            n += len(x)
            last = time.time()
    ms = (last - t0) * 1000
    print('  %-22s %7.1f ms  %6d bytes' % (label, ms, n))
    return ms


t.cmd('EDIT "big.bas"', settle=True)
t0 = time.time()
t.settle(1.0, limit=60)
print('opened: %r' % t.screen.status()[:50])
for where in ('top', 'end'):
    if where == 'end':
        t.key('end')
        t.key('end')
        t.settle(1.0, limit=60)
    else:
        t.key('down', 20)                               # (mid-screen: no scrolling for a keystroke)
    print(where, t.screen.status()[-20:])
    timed('keystroke (x)', b'x')
    timed('backspace', b'\x08')
    timed('right arrow', b'\x1b[C')
    timed('left arrow', b'\x1b[D')
    timed('enter', b'\r')
    timed('backspace (join)', b'\x08')
    timed('up arrow', b'\x1b[A')
    timed('down arrow', b'\x1b[B')
    timed('pgup', b'\x1b[5~')
    timed('pgup (page)', b'\x1b[5~')
    timed('pgdn', b'\x1b[6~')
    timed('pgdn (page)', b'\x1b[6~')
    if where == 'top':                                  # a search from here to a line near the end (its Enter)
        t.key('f3')
        t.type('line %05d' % (LINES - 10))
        timed('search to near the end', b'\r')
t.send(b'\x1b', settle=False)
time.sleep(1.0)
t.settle(0.5)
if 'discard' in t.screen.message():
    t.send(b'Y', settle=False)
    time.sleep(0.3)
    t.send(b'\r', settle=False)
t.prompt(30)
t.cmd('kill "big.bas"', settle=True)
t.close()
