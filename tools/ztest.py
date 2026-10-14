# ztest.py PORT BAUD INDIR OUTDIR: round-trip test of the CMM2's ZMODEM command.
#   1. ZMODEM RECEIVE (batch into the current directory) <- every file in INDIR
#   2. the sizes on the card are checked with MM.INFO(FILESIZE)
#   3. ZMODEM SEND name$ for each file -> OUTDIR, compared byte for byte with INDIR
#   4. ZMODEM RECEIVE "A:/zcopy.txt" (a named target), lower-case spelling
#   5. the test files are deleted from the card
import sys, os, time, re, serial
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import zmodem_pc
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
port, baud, indir, outdir = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4]
s = serial.Serial(port, baud, timeout=0.05)
def rd(t):
    end = time.time() + t; out = b''
    while time.time() < end:
        n = s.in_waiting
        if n: out += s.read(n)
        else: time.sleep(0.02)
    return re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '', out.decode('latin-1'))
def cmd(line, t=1.0):
    pending = rd(0.1)
    if pending.strip() and pending.strip() != '>': print('[board, unprompted]', pending.strip())
    s.write(line.encode('latin-1') + b'\r'); return rd(t)
def fail(msg): print('FAIL:', msg); sys.exit(1)
def prompt():
    """get the board to its prompt: cancel any ZMODEM session still running, then Ctrl-C and CR until '>' answers"""
    for _ in range(6):
        s.write(bytes([0x18] * 8 + [0x08] * 8)); time.sleep(1.5); rd(1.0)
        s.write(b'\x03'); time.sleep(0.2); s.reset_input_buffer(); s.write(b'\r')
        if '>' in rd(1.5): return
    fail('no prompt from the board')
time.sleep(0.3); prompt()
files = sorted(os.listdir(indir))
for f in files: cmd('KILL "A:/%s"' % f, 0.5)

print('--- 1. batch receive on the board, %d files ---' % len(files))
s.write(b'ZMODEM RECEIVE\r'); time.sleep(0.5)
zmodem_pc.send(s, baud, [os.path.join(indir, f) for f in files], 32768)
out = rd(3); print(out.strip().splitlines()[-1] if out.strip() else '(no reply)')
if 'Received %d files' % len(files) not in out: fail('board did not report the batch')

print('--- 2. sizes on the card ---')
for f in files:
    want = os.path.getsize(os.path.join(indir, f))
    got = cmd('? MM.INFO(FILESIZE "A:/%s")' % f)
    m = re.search(r'\n\s*(-?\d+)', got)
    print(f, want, 'on card:', m.group(1) if m else got.strip())
    if not m or int(m.group(1)) != want: fail('size mismatch for ' + f)

print('--- 3. send back from the board, compare ---')
for f in files:
    s.write(('ZMODEM SEND "A:/%s"\r' % f).encode()); time.sleep(0.3)
    zmodem_pc.recv(s, baud, outdir)
    out = rd(2.5)
    a = open(os.path.join(indir, f), 'rb').read(); b = open(os.path.join(outdir, f), 'rb').read()
    said = [l.strip() for l in out.splitlines() if l.strip() and l.strip() != '>']
    print(f, 'board said:', ' / '.join(said), '| identical' if a == b else '| DIFFERENT')
    if a != b: fail('round trip changed ' + f)

print('--- 4. named receive, lower-case command ---')
s.write(b'zmodem receive "A:/zcopy.txt"\r'); time.sleep(0.5)
zmodem_pc.send(s, baud, [os.path.join(indir, files[0])], 32768)
out = rd(3); print((out.strip().splitlines() or [''])[-1].strip())
got = cmd('? MM.INFO(FILESIZE "A:/zcopy.txt")'); m = re.search(r'\n\s*(-?\d+)', got)
if not m or int(m.group(1)) != os.path.getsize(os.path.join(indir, files[0])): fail('named receive size')
print('zcopy.txt size ok')

print('--- 5. clean up ---')
for f in files + ['zcopy.txt']: cmd('KILL "A:/%s"' % f, 0.5)
print(cmd('LIST FILES "z*.*"', 1.5).strip())
print('ALL PASSED')
