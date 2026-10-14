# stress_reset.py PORT ROUNDS [BAUD]: hunt for unexpected resets.
#
# Each round: a ZMODEM batch of three files onto the SD card, a file-size query, a program that runs,
# a program refused at load, a short computation; any sign-on banner in the replies is an unexpected
# reset. Prints one line per round and a total. Leaves nothing on the card.
import sys, os, time, re, tempfile, serial
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import zmodem_pc
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
port, rounds = sys.argv[1], int(sys.argv[2]); baud = int(sys.argv[3]) if len(sys.argv) > 3 else 460800
s = serial.Serial(port, baud, timeout=0.05)
def rd(t):
    end = time.time() + t; out = b''
    while time.time() < end:
        n = s.in_waiting
        if n: out += s.read(n)
        else: time.sleep(0.02)
    return re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '', out.decode('latin-1'))
def cmd(line, t=1.5):
    pending = rd(0.1); s.write(line.encode() + b'\r'); return pending + rd(t)
d = tempfile.mkdtemp()
files = {'zs1.bas': 'FOR i=1 TO 2000 : a=a+SIN(i) : NEXT : PRINT "done"\n',
         'zs2.bas': 'PRINT "start"\n' + ':'.join(['DHT22'] * 40) + '\nPRINT "end"\n',
         'zs3.dat': ''.join(chr(32 + (i * 7) % 90) for i in range(20000)) + '\n'}
for n, c in files.items(): open(os.path.join(d, n), 'w', newline='\r\n').write(c)
s.write(b'\x03'); time.sleep(0.2); s.write(b'\r'); rd(1)
resets = probe = 0; t0 = time.time()
for r in range(rounds):
    log = ''
    s.write(b'ZMODEM RECEIVE\r'); time.sleep(0.5)
    try:
        zmodem_pc.send(s, baud, [os.path.join(d, n) for n in files], 32768)
    except SystemExit as e:                          # the sender gives up with sys.exit: note it and go on
        log += '[transfer failed: %s] ' % e
        s.write(b'\x18' * 8 + b'\x08' * 8); time.sleep(0.5); s.write(b'\x03\r')
    log += rd(2)
    log += cmd('? MM.INFO(FILESIZE "A:/zs3.dat")')
    log += cmd('RUN "A:/zs1.bas"', 3)
    log += cmd('RUN "A:/zs2.bas"', 3)
    log += cmd('? 6*7')
    hit = 'Colour Maximite' in log
    resets += hit
    fromprobe = 'outside the firmware' in log          # the boot says so when the reset did not come from SoftReset()
    probe += fromprobe
    tags = ('RESET (probe) ' if fromprobe else 'RESET ' if hit else '') + ('ok' if ('done' in log and 'too long' in log and '42' in log) else 'unexpected')
    print('round %2d  %5.0f s  %s' % (r + 1, time.time() - t0, tags), flush=True)
    if hit or 'unexpected' in tags: print('   ', ' | '.join(l.strip() for l in log.replace('\r', '').split('\n') if l.strip())[-400:], flush=True)
for n in files: cmd('KILL "A:/%s"' % n, 0.6)
print('%d rounds, %d unexpected resets (%d requested from outside the firmware), %.0f s' % (rounds, resets, probe, time.time() - t0))
