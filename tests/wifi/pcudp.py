"""pcudp.py IP [N]: UDP with the CMM2 running udptest.bas (OPTION UDP SERVER PORT 5005): N datagrams (default 5) sent
to IP:5005, and the echoes it sends back to this PC's port 5006 counted."""
import socket, sys, time

ip = sys.argv[1]
n = int(sys.argv[2]) if len(sys.argv) > 2 else 5
rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
rx.bind(('', 5006))
rx.settimeout(0.2)
tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
got = []
for i in range(n):
    msg = 'hello %d from the PC' % (i + 1)
    t0 = time.time()
    tx.sendto(msg.encode(), (ip, 5005))
    while time.time() - t0 < 2:
        try:
            d, a = rx.recvfrom(1500)
            got.append((d.decode('latin-1'), a[0], (time.time() - t0) * 1000))
            break
        except socket.timeout:
            pass
    time.sleep(0.2)
for d, a, ms in got:
    print('%-40s from %s in %.0f ms' % (d, a, ms))
print('%d sent, %d echoes back' % (n, len(got)))
