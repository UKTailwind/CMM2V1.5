# cmm2_se.py PORT RUNS [BAUD]   (the CMM2 console defaults to 460800 since 14 Oct 2026)
# Runs A:/solar_eclipse.bas on the CMM2 with the reference inputs (12,1,2000 / 39,40,36 / -104,57,12 / 1644 / 30),
# golden JD 2451904.14541560 / .19690359 / .25420764, and prints the Time taken line. 14 Oct 2026, G2 at 480 MHz: 7.454-7.471 s.
import sys, time, serial, re
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
port, runs = sys.argv[1], int(sys.argv[2])
baud = int(sys.argv[3]) if len(sys.argv) > 3 else 460800
inputs = ["12,1,2000", "39,40,36", "-104,57,12", "1644", "30"]
def rd(s, t):
    end = time.time() + t; out = b''
    while time.time() < end:
        n = s.in_waiting
        if n: out += s.read(n)
        else: time.sleep(0.02)
    return out.decode('latin-1')
with serial.Serial(port, baud, timeout=0.1) as s:
    time.sleep(0.3); s.reset_input_buffer()
    s.write(b'\x03'); time.sleep(0.3); s.write(b'\r'); rd(s, 1)
    for r in range(runs):
        s.reset_input_buffer()
        s.write(b'RUN "solar_eclipse.bas"\r')
        out = rd(s, 5.0)
        for inp in inputs:
            s.write(inp.encode() + b'\r'); out += rd(s, 1.5)
        end = time.time() + 90
        while time.time() < end:
            out += rd(s, 1.0)
            if re.search(r'\n> ?$', out) and 'Time' in out: break
        out = re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '', out)
        lines = [l for l in out.splitlines() if l.strip()]
        keep = [l for l in lines if re.search(r'(?i)time|taken|JD|2451904|19690359|25420764|error', l)]
        print(f"--- run {r+1}: {len(lines)} lines ---"); print('\n'.join(keep[-12:]))
