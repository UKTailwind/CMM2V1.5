# crashers_test.py PORT [BAUD]: regression test for the bounds on the source-line pipeline.
#
# Before the fix a line that the keyword substitutions grew past 288 bytes (DHT22 becomes BITBANG DHT22,
# ERASE becomes CLEAR VARS, I2C becomes I2C 1 ...) overran a stack buffer and then inpbuf, and one
# 251-character line of DHT22s typed at the prompt hung the board.  Now every such line is refused with
# "Line ... is too long", lines that worked before still work, and the board carries on.
#
#   1. an interrupt SUB and a label interrupt run and return (the SUB returns through the IRETURN stub)
#   2. a line that tokenises to about 300 bytes still loads and runs (tknbuf is now 512 bytes)
#   3. the same line typed at the prompt still runs
#   4. 42 DHT22s at the prompt, and 31 ERASEs: refused
#   5. 40 DHT22s on line 2 of a program: refused when the program loads, naming line 2
#   6. the same in an #INCLUDE file: refused, naming the line and the file
#   7. a #DEFINE that grows a line past 255 characters: refused, naming the line
#
# The test programs are sent to the SD card with ZMODEM and deleted afterwards.
import sys, os, time, re, tempfile, serial
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import zmodem_pc
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
port = sys.argv[1]; baud = int(sys.argv[2]) if len(sys.argv) > 2 else 460800

LONG = ':'.join(['a=1'] * 60)              # 239 characters, about 300 bytes once tokenised
DHT42 = ':'.join(['DHT22'] * 42)           # 251 characters, 587 after substitution
ERASE31 = ':'.join(['ERASE a'] * 31)       # 247 characters, 402 after substitution
PROGS = {
    'zint.bas': ['n=0 : m=0', 'SETTICK 20, TK, 1', 'SETTICK 25, LB, 2', 't0=TIMER',
                 'DO WHILE TIMER-t0 < 1000 : LOOP', 'SETTICK 0, TK, 1 : SETTICK 0, LB, 2',
                 'PRINT "ticks";n;" label ticks";m', 'END',
                 'SUB TK', '  n=n+1', 'END SUB', 'LB: m=m+1 : IRETURN'],
    'zlong.bas': ['PRINT "start"', LONG, 'PRINT "end"'],
    'zsubst.bas': ['PRINT "start"', ':'.join(['DHT22'] * 40), 'PRINT "end"'],
    'zinc.inc': ['PRINT "inc"', ':'.join(['DHT22'] * 30)],
    'zincmain.bas': ['#INCLUDE "zinc.inc"', 'PRINT "main"'],
    'zdefine.bas': ['#DEFINE "QQ", "PRINT 12345678901234567890"', ':'.join(['QQ'] * 80), 'PRINT "end"'],
}

s = serial.Serial(port, baud, timeout=0.05)
def rd(t):
    end = time.time() + t; out = b''
    while time.time() < end:
        n = s.in_waiting
        if n: out += s.read(n)
        else: time.sleep(0.02)
    return re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '', out.decode('latin-1'))
def cmd(line, t=1.5):
    rd(0.1); s.write(line.encode('latin-1') + b'\r'); return rd(t)
def alive():
    return '42' in cmd('? 6*7', 1.5)
def last(out, n=3):
    return ' | '.join([l.strip() for l in out.replace('\r', '').split('\n') if l.strip() and l.strip() != '>'][-n:])
results = []
def check(name, ok, detail):
    results.append(ok); print(('PASS ' if ok else 'FAIL ') + name + ': ' + detail)

time.sleep(0.3)
for _ in range(4):
    s.write(b'\x03'); time.sleep(0.2); s.write(b'\r')
    if '>' in rd(1.0): break
d = tempfile.mkdtemp()
for name, lines in PROGS.items():
    open(os.path.join(d, name), 'w', newline='\r\n').write('\n'.join(lines) + '\n')
for name in PROGS: cmd('KILL "A:/%s"' % name, 0.5)
s.write(b'ZMODEM RECEIVE\r'); time.sleep(0.5)
zmodem_pc.send(s, baud, [os.path.join(d, n) for n in PROGS], 32768)
rd(2)

out = cmd('RUN "A:/zint.bas"', 4)
m = re.search(r'ticks\s*(\d+)\s*label ticks\s*(\d+)', out)
check('1 interrupt SUB and label return', bool(m) and 40 <= int(m.group(1)) <= 60 and 30 <= int(m.group(2)) <= 50 and alive(), last(out))

out = cmd('RUN "A:/zlong.bas"', 5)
check('2 300-byte line in a program still runs', 'start' in out and 'end' in out and 'too long' not in out.lower() and alive(), last(out))

out = cmd(LONG, 2)
check('3 300-byte line at the prompt still runs', 'too long' not in out.lower() and 'rror' not in out and alive(), last(out, 1))

out = cmd(DHT42, 2)
ok = 'line is too long' in out.lower() and alive()
out2 = cmd(ERASE31, 2)
check('4 substitutions past 255 at the prompt refused', ok and 'line is too long' in out2.lower() and alive(), last(out, 1) + ' / ' + last(out2, 1))

out = cmd('RUN "A:/zsubst.bas"', 5)
check('5 in a program, refused at load', 'line 2 is too long' in out.lower() and 'start' not in out and alive(), last(out, 1))

out = cmd('RUN "A:/zincmain.bas"', 5)
check('6 in an include file, refused', 'line 2 of zinc.inc is too long' in out.lower() and alive(), last(out, 1))

out = cmd('RUN "A:/zdefine.bas"', 5)
check('7 #DEFINE growth refused', 'line 2 is too long' in out.lower() and alive(), last(out, 1))

for name in PROGS: cmd('KILL "A:/%s"' % name, 0.5)
print('ALL PASSED' if all(results) else 'FAILED')
sys.exit(0 if all(results) else 1)
