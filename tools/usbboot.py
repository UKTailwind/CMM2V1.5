"""usbboot.py PORT BAUD N [st|sw|alt] [SECONDS] [LOGDIR]: restart a CMM2 N times and report, for each boot, the USB devices
it announced and whether the mouse, the keyboard and C: answer afterwards; then how many boots ended each way.

  st    a reset through the board's ST-Link (goldens.PROBES, by its console port)
  sw    CPU RESTART at the prompt
  alt   the two in turn (the default)

SECONDS is how long the boot is read (20: TinyUSB has enumerated a hub and four devices in 4 s). LOGDIR keeps each
boot's whole console output, for a firmware built with CFG_TUSB_DEBUG 2 (Inc/tusb_config.h: TinyUSB's log, each line
with the HAL tick, for the first 15 s). 21 Oct 2026: the G2 with a hub lost the devices behind it in 4 to 6 of 30
restarts until CFG_TUH_ENUM_RESET_RECOVERY_MS went from 10 to 100 ms."""
import os, re, subprocess, sys, time
import serial

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from goldens import ANSI, PROBES, STLINK

QUERY = ('On Error Skip : x% = Mouse(X, 2) : Print "MOUSE"; MM.ErrNo = 0; " KB "; MM.Info$(KEYBOARD); '
         '" C:"; MM.Info(EXISTS DIR "C:/")')


def ask(s, line, wait=3.0):
    s.reset_input_buffer()
    s.write(line.encode() + b'\r')
    out, end = b'', time.time() + wait
    while time.time() < end:
        out += s.read(4096)
        if out.rstrip().endswith(b'>') and out.count(b'\n') >= 2:
            break
    return ANSI.sub('', out.decode('latin-1'))


def main():
    port, baud, n = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    how = sys.argv[4] if len(sys.argv) > 4 else 'alt'
    secs = float(sys.argv[5]) if len(sys.argv) > 5 else 20
    logdir = sys.argv[6] if len(sys.argv) > 6 else None
    sn = PROBES.get(port.upper())
    if how != 'sw' and not sn:
        sys.exit("usbboot: no ST-Link known for %s (goldens.PROBES): use sw" % port)
    counts = {}
    with serial.Serial(port, baud, timeout=0.1) as s:
        for i in range(n):
            kind = how if how != 'alt' else ('st' if i % 2 == 0 else 'sw')
            s.reset_input_buffer()
            t0 = time.time()
            if kind == 'st':
                subprocess.run([STLINK, '-c', 'port=SWD', 'sn=' + sn, 'mode=UR', 'freq=480', '-rst'], capture_output=True)
            else:
                s.write(b'\x03\r')
                time.sleep(0.3)
                s.write(b'CPU RESTART\r')
            out, end = b'', t0 + secs
            while time.time() < end:
                out += s.read(4096)
            text = ANSI.sub('', out.decode('latin-1'))
            if logdir:
                os.makedirs(logdir, exist_ok=True)
                open(os.path.join(logdir, 'boot_%02d_%s.txt' % (i, kind)), 'w', encoding='utf-8').write(text)
            seen = [l.strip('> ').strip() for l in text.splitlines() if 'Connected' in l]
            r = re.search(r'MOUSE\s*(-?\d)\s*KB\s*([A-Za-z ]+?)\s*C:\s*(-?\d)', ask(s, QUERY))
            state = ('mouse=%s kb=%s C:=%s' % (r.group(1), 'y' if r.group(2).strip() == 'Connected' else 'n', r.group(3))
                     if r else '?')
            counts[state] = counts.get(state, 0) + 1
            print('%2d %-2s %s  %s' % (i, kind, state, ' | '.join(seen)), flush=True)
    for state, k in sorted(counts.items(), key=lambda x: -x[1]):
        print('%3d boots: %s' % (k, state))


if __name__ == '__main__':
    main()
