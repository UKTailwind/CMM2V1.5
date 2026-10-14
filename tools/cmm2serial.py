# cmm2serial.py PORT BAUD "cmd" ["cmd" ...]   or   cmm2serial.py list
# Sends Ctrl-C, then each command to the CMM2 console and prints the reply (pyserial). First seed of the P0 serial harness.
import sys, time, serial, serial.tools.list_ports
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
if sys.argv[1] == 'list':
    for p in serial.tools.list_ports.comports(): print(p.device, '|', p.description, '|', p.hwid)
    sys.exit(0)
port, baud = sys.argv[1], int(sys.argv[2])
cmds = sys.argv[3:]
def rd(s, t):
    end = time.time() + t; out = b''
    while time.time() < end:
        n = s.in_waiting
        if n: out += s.read(n); end = time.time() + 0.3 if t < 2 else end
        else: time.sleep(0.02)
    return out.decode('latin-1')
with serial.Serial(port, baud, timeout=0.1, rtscts=False, dsrdtr=False) as s:
    s.dtr = True; s.rts = False
    time.sleep(0.3); s.reset_input_buffer()
    s.write(b'\x03'); time.sleep(0.3); s.write(b'\r'); print('[wake]', repr(rd(s, 1.5)))
    for c in cmds:
        s.reset_input_buffer()
        s.write(c.encode('latin-1') + b'\r')
        t = 1.5
        if c.startswith('!'): pass
        print('>>', c); print(rd(s, 8 if 'TIMER' in c.upper() or 'RUN' in c.upper() else 2))
