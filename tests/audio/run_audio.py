"""run_audio.py DIR [--put] [--port COM4] [--baud 460800]: send DIR's files (from gen_audio.py) to A:/snd when --put is
given, then run audiotest.bas and a1test.bas on the board and print what they print."""
import sys, os, glob
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
import goldens

def arg(name, default):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default

d = sys.argv[1]
b = goldens.Board(arg('--port', 'COM4'), int(arg('--baud', '460800')))
b.attention()
if '--put' in sys.argv:
    b.cmd('On Error Skip : Mkdir "A:/snd"')
    b.cmd('Chdir "A:/snd"')
    print(b.zmodem_put(sorted(glob.glob(os.path.join(d, '*')))))
b.cmd('Chdir "A:/snd"')
print(b.cmd('RUN "audiotest.bas"', 120))
print(b.cmd('RUN "A:/snd/a1test.bas"', 600))
b.cmd('Chdir "A:/"')
b.close()
