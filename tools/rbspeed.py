"""rbspeed.py PORT [BAUD] [FILE.bas ...]: compiled against text on a CMM2 (Route B, Stream.c).

Runs each program with OPTION COMPILE OFF and then ON, twice each, and prints the times: the G3 kernels K1 and K3 as
the PicoMite's Bas/bench/rbp3a.py has them (in SUBs), the "speed" programs of its rb*.py files, and any .bas files
given (on the PC: sent to A:/rbt). A program's time is what it prints after "TIME" or as the last number of a line
starting "K", else the wall time between RUN and the prompt. Leaves OPTION COMPILE OFF."""
import os, re, sys, tempfile, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from goldens import Board
import rbtest


def programs(extra):
    out = []
    ns = {}
    for f in sorted(os.listdir(rbtest.PICO)):
        if not re.match(r"rb.*\.py$", f):
            continue
        for name, src, *_ in rbtest.progs_of(os.path.join(rbtest.PICO, f)):
            if name == "speed" or "loop" in name and "speed" in f:
                out.append(("%s/%s" % (f[:-3], name), src))
    # K1 and K3 from rbp3a.py: module globals the loader leaves in its namespace
    path = os.path.join(rbtest.PICO, "rbp3a.py")
    text = open(path, encoding="utf-8").read()
    for k in ("K1", "K3"):
        m = re.search(r'^%s = """(.*?)"""' % k, text, re.M | re.S)
        if m:
            out.append(("rbp3a/" + k, m.group(1)))
    for p in extra:
        out.append((os.path.basename(p), open(p, encoding="latin-1").read()))
    return out


def timed(b, name):
    t0 = time.time()
    out = b.cmd('Run "%s"' % name, 600)
    wall = time.time() - t0
    m = re.findall(r"TIME\s*(-?[\d.]+)", out)
    if m:
        return float(m[-1]), out
    m = re.findall(r"^K.*?([\d.]+)\s*$", out, re.M)
    if m:
        return float(m[-1]), out
    return wall * 1000, out


def main():
    port = sys.argv[1]
    args = sys.argv[2:]
    baud = int(args.pop(0)) if args and args[0].isdigit() else 460800
    progs = programs(args)
    b = Board(port, baud)
    b.attention()
    b.cmd("Option Compile On", 30)                  # (a G1 restarts here the first time, to make its area: Memory.h)
    time.sleep(1)
    b.attention()
    b.cmd('Chdir "A:/"')
    b.cmd('Mkdir "rbt"')
    b.cmd('Chdir "A:/rbt"')
    stage = tempfile.mkdtemp(prefix="rbspeed_")
    names = []
    for i, (name, src) in enumerate(progs):
        fn = "spd_%d.bas" % i
        open(os.path.join(stage, fn), "w", newline="\r\n").write(src)
        names.append(fn)
    b.zmodem_put([os.path.join(stage, n) for n in names])
    print("%-28s %10s %10s %7s" % ("program", "text ms", "compiled", "ratio"))
    for (name, src), fn in zip(progs, names):
        res = {}
        for mode in ("OFF", "ON"):
            b.cmd("Option Compile " + mode, 10)
            ts = [timed(b, fn)[0] for _ in range(2)]
            res[mode] = min(ts)
        r = res["OFF"] / res["ON"] if res["ON"] else 0
        print("%-28s %10.1f %10.1f %6.2fx" % (name[:28], res["OFF"], res["ON"], r), flush=True)
    b.cmd("Option Compile Off", 10)
    b.cmd('Chdir "A:/"')
    b.close()


if __name__ == "__main__":
    main()
