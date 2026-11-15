"""pcweb.py IP [N]: fetches http://IP/ N times (default 20) from this PC, as a browser would, and prints the first page,
the times and any failures (tests/wifi/webserver.bas on the CMM2)."""
import sys, time, urllib.request

ip = sys.argv[1]
n = int(sys.argv[2]) if len(sys.argv) > 2 else 20
times, bad = [], 0
for i in range(n):
    t0 = time.time()
    try:
        with urllib.request.urlopen('http://%s/' % ip, timeout=10) as r:
            body = r.read().decode('latin-1')
            if i == 0:
                print('HTTP %d, %d bytes:\n%s' % (r.status, len(body), body.strip()))
            if 'Colour Maximite 2' not in body:
                bad += 1
    except Exception as e:
        bad += 1
        print('request %d: %s' % (i + 1, e))
    times.append(time.time() - t0)
    time.sleep(0.2)
times.sort()
print('%d requests, %d failed; time min %.0f ms, median %.0f ms, max %.0f ms' %
      (n, bad, times[0] * 1000, times[len(times) // 2] * 1000, times[-1] * 1000))
