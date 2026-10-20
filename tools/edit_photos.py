"""edit_photos.py OUTDIR [CONFIG...]: EDIT under each edit font, COLOURCODE mode and console (Editor_Port_Plan.md,
E1's gate), on the board on COM4, photographed with Print Screen.

Each config runs its set-up at the prompt, opens A:/e/p.bas (a colour sample, 60 short lines and a long one),
photographs the display (the Print Screen key, 0x9D, which saves A:/SCREENnnn.BMP) when open, after 30 lines down,
and in mark mode, leaves, puts the option back, and fetches the pictures by ZMODEM as OUTDIR/CONFIG_NN_LABEL.png.
For each picture it prints the bands of light rows (the status bar: there should be one). On the terminal it checks
the status bar and the size the editor asked for. A full-screen Print Screen takes 5-6 s and a picture about 35 s to
fetch, so the eight configs take about 20 minutes. Needs Pillow.

The screen-only console cannot be driven from here: with OPTION CONSOLE SCREEN the console UART takes no input. (Its
receive interrupt ignores bytes while OptionConsole has no serial bit; setting OptionConsole 2 with POKE, with the
UART left running, hung the board on the next byte.) It is checked by hand, with the USB keyboard."""
import contextlib, io, os, sys, tempfile, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vt100drive, zmodem_pc
from PIL import Image

SAMPLE = (["' colour sample", 'Dim s$ = Left$("abc", 2)', 'x = 123 + 4.5', 'Rem a rem line', '/* a block',
           'still comment', '*/', 'Print s$, x'] + ['line %03d' % i for i in range(1, 61)] +
          ['L' + ''.join(chr(65 + j % 26) for j in range(160))])
STEPS = ['snap:open', 'key:down*30', 'snap:down30', 'key:f4', 'key:down*3', 'key:right*5', 'snap:mark', 'key:esc']
CONFIGS = {
    'font_normal': (['option edit font normal'], [], True),
    'font_large': (['option edit font large'], ['option edit font normal'], True),
    'font_vlarge': (['option edit font very large'], ['option edit font normal'], True),
    'font_medium': (['option edit font medium'], ['option edit font normal'], True),
    'font_small': (['option edit font small'], ['option edit font normal'], True),
    'colour_off': (['option colourcode off'], ['option colourcode on'], True),
    'colour_reverse': (['option colourcode reverse'], ['option colourcode on'], True),
    'console_serial': (['option console serial'], ['option console both'], True),
}


def bars(img):
    w, h = img.size
    px = img.convert('L').load()
    rows = [sum(1 for x in range(0, w, 4) if px[x, y] > 160) > (w // 4) * 0.6 for y in range(h)]
    out, y = [], 0
    while y < h:
        if rows[y]:
            y0 = y
            while y < h and rows[y]:
                y += 1
            if y - y0 >= 4:
                out.append((y0, y - 1))
        y += 1
    return out


def clean(t, pattern):
    t.cmd('kill "%s", all' % pattern, settle=True)
    if 'sure' in t.text():
        t.send(b'Y', settle=False)
        time.sleep(0.2)
        t.mark()
        t.send(b'\r', settle=False)
        t.prompt(120)


def put(t, name, lines):
    d = tempfile.mkdtemp()
    open(os.path.join(d, name), 'wb').write(''.join(l + '\r\n' for l in lines).encode())
    t.cmd('if mm.info(exists file "%s") then kill "%s"' % (name, name), settle=True)
    t.b.s.write(b'ZMODEM RECEIVE\r')
    time.sleep(0.5)
    with contextlib.redirect_stdout(io.StringIO()):
        zmodem_pc.send(t.b.s, t.b.baud, [os.path.join(d, name)], 32768)
    t.settle(0.5)
    t.b.s.write(b'\r')
    t.settle(0.5)


def fetch(t, n, out):
    for attempt in range(2):
        dd = tempfile.mkdtemp()
        t.settle(0.5)
        t.b.s.write(('ZMODEM SEND "A:/SCREEN%03d.BMP"\r' % n).encode())
        t0, seen = time.time(), b''
        while time.time() - t0 < 10 and b'**\x18B' not in seen:
            seen += t.b.s.read(64)
        if b'**\x18B' not in seen:
            continue
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                zmodem_pc.recv(t.b.s, t.b.baud, dd)
        except SystemExit:
            t.settle(2.0)
            t.b.s.write(b'\x03\r')
            t.settle(1.0)
            continue
        if os.listdir(dd):
            t.settle(0.5)
            t.b.s.write(b'\r')
            t.settle(0.3)
            img = Image.open(os.path.join(dd, os.listdir(dd)[0]))
            img.save(out)
            return img
    return None


def run(name, outdir):
    setup, restore, serial = CONFIGS[name]
    t = vt100drive.Term()
    full = []                                           # everything the board sent in this config (the stray-key hunt)
    read = t.read
    def logged():
        x = read()
        if x:
            full.append(x)
        return x
    t.read = logged
    t.cmd('chdir "A:/e"', settle=True)
    put(t, 'p.bas', SAMPLE)
    clean(t, 'A:/SCREEN*.BMP')
    for c in setup:
        t.cmd(c, settle=True)
    t.mark()
    full.append('<<EDIT>>')
    t.cmd('EDIT "p.bas"', settle=True)
    t.settle(1.0)
    if serial and 'modified' in t.screen.status():
        print('STRAY %-15s the file is modified on entry; the log:' % name)
        print(repr(''.join(full)[-2500:]))
    if serial:
        ok = 'p.bas' in t.screen.status() and 'modified' not in t.screen.status()
        print('%-5s %-15s terminal %dx%d, status %r' % ('ok' if ok else 'CHECK', name, t.screen.cols, t.screen.rows,
                                                         t.screen.status()[:60]))
    else:
        print('%-5s %-15s terminal quiet: %d characters' % ('ok' if not t.log.strip() else 'CHECK', name, len(t.log)))
    labels = []
    for step in STEPS:
        kind, _, arg = step.partition(':')
        if kind == 'key':
            k = arg.split('*')
            t.key(k[0], int(k[1]) if len(k) > 1 else 1)
        elif kind == 'snap':
            t.settle(0.5)
            t.send(b'\x9d', settle=False)
            t0 = time.time()                            # (a full-screen SAVE IMAGE takes 5-6 s)
            while time.time() - t0 < 7.0:
                t.read()
            t.settle(0.5)
            labels.append(arg)
    t.send(b'\x1b', settle=False)                       # leave: the file is unchanged, but if it asks, discard
    t0 = time.time()
    while time.time() - t0 < (15 if serial else 4) and not (serial and ('discard' in t.screen.message() or
                                                                        t.at_prompt())):
        t.read()
    t.settle(0.5)
    if serial and 'discard' in t.screen.message():
        t.send(b'Y', settle=False)
        time.sleep(0.3)
        t.send(b'\r', settle=False)
        t.settle(1.0)
    if serial and not t.at_prompt():
        print('CHECK %-15s the editor did not close: %r' % (name, t.screen.message()))
        print('      the log since EDIT, the last 600:', repr(''.join(full).split('<<EDIT>>')[-1][-600:]))
        t0 = time.time()
        while time.time() - t0 < 40 and not t.at_prompt():
            x = t.b._read()
            if x:
                t.screen.feed(x)
                print('      %5.1f s later: %r' % (time.time() - t0, x[-120:]))
        t.close()
        return
    for c in restore:
        t.cmd(c, settle=True)
    t.b.s.write(b'\r')
    t.settle(0.5)
    if not serial:
        print('      %-15s after: %r' % (name, t.text()[-80:]))
    for n, label in enumerate(labels):
        out = os.path.join(outdir, '%s_%02d_%s.png' % (name, n, label))
        img = fetch(t, n, out)
        if img is None:
            print('      %-15s %s: no picture' % (name, label))
            continue
        print('      %-15s %02d %-8s bars %s' % (name, n, label, bars(img)))
    clean(t, 'A:/SCREEN*.BMP')
    t.close()


if __name__ == '__main__':                              # (tools/manual/photo.py uses put, fetch and clean)
    outdir = sys.argv[1]
    os.makedirs(outdir, exist_ok=True)
    for name in (sys.argv[2:] or CONFIGS):
        run(name, outdir)
