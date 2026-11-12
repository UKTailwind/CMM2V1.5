"""make.py: builds apps/pdcurses/out/pdcdemo.app, PDCursesMod's own demos on the CMM2's curses (pdcurses.py), to test
the port: its screen, colours, attributes, Unicode, keys, mouse and speed. Each demo is built as it is, its main
renamed (out/demos/demo_NAME.c includes it), and pdcdemo.c runs the one named after RUN's comma:

    python apps/pdcurses/make.py
then, with out/pdcdemo.app on the card:
    RUN "pdcdemo.app", testcurs                 (or newtest, widetest, ptest, worm..., mode=N font=WxH too)
    RUN "pdcdemo.app"                           lists them

Public domain, as PDCursesMod.
"""
import os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import pdcurses

# PDCursesMod's demos/, each built as it is (picsview needs pictures, and the rest are not curses programs)
DEMOS = ['testcurs', 'newtest', 'widetest', 'ptest', 'worm', 'rain', 'firework', 'xmas', 'speed', 'tuidemo', 'ozdemo',
         'calendar', 'mbrot', 'init_col', 'test_pan']


def main():
    pdcurses.prepare()
    gen = os.path.join(HERE, 'out', 'demos')
    os.makedirs(gen, exist_ok=True)
    srcs = []
    for d in DEMOS:
        p = os.path.join(gen, 'demo_%s.c' % d)
        text = '/* made by make.py: PDCursesMod\'s demos/%s.c, its main renamed */\n#define main %s_main\n' \
               '#include "demos/%s.c"\n' % (d, d, d)
        if not os.path.exists(p) or open(p).read() != text:
            open(p, 'w', newline='\n').write(text)
        srcs.append(p)
    lst = os.path.join(gen, 'demos.h')
    text = ''.join('DEMO(%s)\n' % d for d in DEMOS)
    if not os.path.exists(lst) or open(lst).read() != text:
        open(lst, 'w', newline='\n').write(text)
    srcs.append(os.path.join(pdcurses.PDC, 'demos', 'tui.c'))       # (tuidemo's menus)
    srcs.append(os.path.join(HERE, 'pdcdemo.c'))
    srcs += pdcurses.sources()
    listfile = os.path.join(HERE, 'out', 'pdcdemo_sources.txt')
    open(listfile, 'w').write('\n'.join(srcs) + '\n')
    cmd = [sys.executable, os.path.join(REPO, 'tools', 'sdk', 'build.py'), os.path.join(HERE, 'out', 'pdcdemo.app'),
           '--sources', listfile, '--api', '12', '-I', gen, '-I', os.path.join(pdcurses.PDC, 'demos'),
           '--cflag=-Wno-unused-function'] + pdcurses.flags()
    sys.exit(subprocess.run(cmd).returncode)


if __name__ == '__main__':
    main()
