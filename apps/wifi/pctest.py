"""pctest.py PORT [BAUD] [SECONDS]: step 1's checks from the PC (docs/Web_Plan.md). Runs RUN "wifi.app", join on the
CMM2 at PORT (A:/wifi/wifi.app, the network from A:/wifi/wifi.cfg), waits for its address, then from this PC:
  - pings it four times (Windows' ping);
  - TCP port 5001 for SECONDS (default 10): the CMM2 sends, this PC counts;
  - TCP port 5002 for SECONDS: this PC sends, the CMM2 counts;
then presses a key on the CMM2 to end it, and prints the CMM2's side (its own ping of the gateway, and each
connection's bytes and rate as it saw them).

    python apps/wifi/pctest.py COM10
"""
import os, re, socket, subprocess, sys, time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 'tools'))
from goldens import Board, ANSI

sys.stdout.reconfigure(encoding='utf-8', errors='replace')
port = sys.argv[1]
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
secs = float(sys.argv[3]) if len(sys.argv) > 3 else 10.0

b = Board(port, baud)
b.attention()
b.cmd('CHDIR "A:/wifi"', 10)
b.drain(0.2)
b.send_line('RUN "wifi.app", join')
out, t0, ip = '', time.time(), None
while time.time() - t0 < 60:
    out += b._read()
    m = re.search(r'Serving at (\d+\.\d+\.\d+\.\d+)', out)
    if m:
        ip = m.group(1)
        break
    if re.search(r'Not joined|did not start|No ssid|\n> ?$', ANSI.sub('', out)):
        break
print(ANSI.sub('', out).strip())
if not ip:
    b.s.write(b' ')
    print(ANSI.sub('', b.drain(2)))
    sys.exit('The CMM2 did not get on the network')

print('\n--- from the PC ---')
r = subprocess.run(['ping', '-n', '4', ip], capture_output=True, text=True)
print('\n'.join(l for l in r.stdout.splitlines() if l.strip()))


def rate(n, dt):
    return '%d bytes in %.2f s = %.0f KB/s' % (n, dt, n / dt / 1024 if dt else 0)


s = socket.create_connection((ip, 5001), timeout=10)
n, t = 0, time.time()
while time.time() - t < secs:
    d = s.recv(65536)
    if not d:
        break
    n += len(d)
dt = time.time() - t
s.close()
print('TCP 5001, the CMM2 sending: ' + rate(n, dt))

s = socket.create_connection((ip, 5002), timeout=10)
buf = bytes(range(256)) * 256
n, t = 0, time.time()
while time.time() - t < secs:
    s.sendall(buf)
    n += len(buf)
s.shutdown(socket.SHUT_WR)
try:
    while s.recv(4096):
        pass
except OSError:
    pass
dt = time.time() - t
s.close()
print('TCP 5002, the CMM2 receiving: ' + rate(n, dt))

time.sleep(1)
print('\n--- the CMM2 ---')
print(ANSI.sub('', b.drain(0.5)).strip())
b.s.write(b' ')
print(ANSI.sub('', b.wait_prompt(15)).strip())
b.close()
