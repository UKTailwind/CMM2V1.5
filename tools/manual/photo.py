"""photo.py OUT.png STEP...: a picture of the Colour Maximite 2's screen for the manual (docs/Manual_Plan.md), taken on
the board on COM4 with the Print Screen key (0x9D, which saves A:/SCREEN_yyyymmdd_hhmmss.PNG), fetched by ZMODEM and
saved as PNG.

    put:LOCAL[=NAME]    send a local file to the card's current directory (as NAME)
    cmd:TEXT            a line at the prompt (waits for the prompt to come back)
    run:TEXT            a line at the prompt that starts something (EDIT, FM): it does not wait for the prompt
    key:NAME[*N]        a key, N times (vt100drive's names: f1, esc, down, pgdn ...)
    type:TEXT           text, a character at a time
    wait:SECS           wait
    snap                Print Screen; it waits the 7 s a 640x480 save takes. The first is OUT, the next OUT_2 ...
After the steps the pictures are fetched and deleted from the card. Steps that leave something open must close it
themselves (key:esc), so that the fetch is typed at the prompt. Needs Pillow."""
import os, re, sys, time
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 'tools'))
import vt100drive
from edit_photos import put, fetch, clean, SHOTS


def main():
    out, steps = sys.argv[1], sys.argv[2:]
    t = vt100drive.Term()
    t.cmd('', settle=True)
    clean(t, SHOTS)
    snaps = 0
    for step in steps:
        kind, _, arg = step.partition(':')
        if kind == 'put':
            local, _, name = arg.partition('=')
            put(t, name or os.path.basename(local), open(local, encoding='latin-1').read().splitlines())
        elif kind == 'cmd':
            t.cmd(arg)
            t.prompt(30)
        elif kind == 'run':
            t.cmd(arg)
            t.settle(1.0, limit=20)
        elif kind == 'key':
            name, _, n = arg.partition('*')
            t.key(name, int(n or 1))
        elif kind == 'type':
            t.type(arg)
        elif kind == 'wait':
            time.sleep(float(arg))
            t.settle(0.3)
        elif kind == 'snap':
            t.send(b'\x9d', settle=False)
            time.sleep(7.0)
            t.settle(0.5)
            snaps += 1
        else:
            sys.exit('no step ' + step)
    t.cmd('', settle=True)
    for n in range(snaps):
        name = out if n == 0 else re.sub(r'(\.\w+)$', r'_%d\1' % (n + 1), out)
        img = fetch(t, n, name)
        print(name, img.size if img else 'NOT FETCHED')
    clean(t, SHOTS)
    t.close()


if __name__ == '__main__':
    main()
