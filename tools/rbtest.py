"""rbtest.py PORT [BAUD] [--only NAME,...] [--pico DIR]: the PicoMite's Route B tests on a CMM2.

The PicoMite's Bas/bench/rb*.py each hold a list PROGS of (name, program, want_code): every program must print the
same with OPTION COMPILE OFF, ON and SHADOW (docs/RouteB_CMM2_Port_Plan.md). Those files drive a PicoMite through its
own module (pc3); here they are only read for PROGS, with that module stubbed, and each program is sent to A:/rbt by
ZMODEM and run three ways. A program is

  SAME   the three outputs are the same, and none has an error
  ERRS   the three are the same but have an error (a PicoMite feature or form the CMM2 lacks: not the compiler's)
  DIFF   the outputs differ (the compiler's)

with the ON run's MM.INFO$(COMPILE) CODE count (statements run as compiled code). Leaves OPTION COMPILE OFF.
Prints each program and a summary; exits 1 if any differs."""
import os, re, sys, tempfile, time, types

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from goldens import Board, ANSI

PICO = r"D:\Dropbox\PicoMite\PicoMite\Bas\bench"
STAT = 'Print "STAT "; MM.Info$(COMPILE)\n'
TIMED = re.compile(r"TIME\s*-?[\d.]+")                  # (a printed time differs every run)


class _Stop(Exception):
    pass


def progs_of(path):
    """The PROGS list of one of the PicoMite's files, its driver module stubbed: the file runs until it makes its
    board, and stops there."""
    text = open(path, encoding="utf-8").read()
    if not re.search(r"^PROGS\s*=", text, re.M):
        return []                                   # (a tool, not a list of programs)
    stub = types.ModuleType("pc3")

    class PC3:
        def __init__(self, *a, **k):
            raise _Stop()
    stub.PC3 = PC3
    stub.ANSI = ANSI
    sys.modules["pc3"] = stub
    ns = {"__name__": "rbtest_load", "__file__": path}
    saved = sys.argv
    sys.argv = [path, "COM0"]
    try:
        exec(compile(text, path, "exec"), ns)
    except (_Stop, SystemExit):
        pass
    except Exception as e:
        print("(%s: %s, skipped)" % (os.path.basename(path), e))
        return []
    finally:
        sys.argv = saved
    return ns.get("PROGS", [])


def main():
    port = sys.argv[1]
    baud = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 460800
    only = sys.argv[sys.argv.index("--only") + 1].split(",") if "--only" in sys.argv else None
    pico = sys.argv[sys.argv.index("--pico") + 1] if "--pico" in sys.argv else PICO
    files = [os.path.join(pico, f) for f in sorted(os.listdir(pico)) if re.match(r"rb.*\.py$", f)]
    files.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "rb_cmm2.py"))   # (the CMM2's own)
    tests = []
    for path in files:
        f = os.path.basename(path)
        if only and f[:-3] not in only:
            continue
        for i, pr in enumerate(progs_of(path)):
            name, src = pr[0], pr[1]
            tests.append((f[:-3], i, name, src))
    b = Board(port, baud)
    b.attention()
    b.cmd("Option Compile On", 30)                  # (a G1 restarts here the first time, to make its area: Memory.h)
    time.sleep(1)
    b.attention()
    b.cmd('Chdir "A:/"')
    b.cmd('Mkdir "rbt"')
    b.cmd('Chdir "A:/rbt"')
    stage = tempfile.mkdtemp(prefix="rbtest_")
    paths = []
    for f, i, name, src in tests:
        p = os.path.join(stage, "%s_%d.bas" % (f, i))
        open(p, "w", newline="\r\n").write(src + STAT)
        paths.append(p)
    for k in range(0, len(paths), 40):
        b.zmodem_put(paths[k:k + 40])
    counts = {"SAME": 0, "ERRS": 0, "DIFF": 0}
    diffs = []
    for (f, i, name, src), p in zip(tests, paths):
        outs = {}
        for mode in ("OFF", "ON", "SHADOW"):
            b.cmd("Option Compile " + mode, 10)
            try:
                out = b.cmd('Run "%s"' % os.path.basename(p), 120)
            except TimeoutError as e:
                out = "TIMEOUT " + str(e)[-200:]
                b.attention()
            lines = [l.rstrip() for l in out.replace("\r", "").split("\n") if l.strip() and l.strip() != ">"]
            stat = [l for l in lines if l.startswith("STAT ")]
            body = [l for l in lines if not l.startswith("STAT ")]
            outs[mode] = (body, stat[0][5:] if stat else "")
        untimed = {m: [TIMED.sub("TIME *", l) for l in outs[m][0]] for m in outs}
        same = untimed["OFF"] == untimed["ON"] == untimed["SHADOW"]
        err = any("rror" in l or "TIMEOUT" in l or "Could not find" in l
                  for l in outs["OFF"][0] + outs["ON"][0] + outs["SHADOW"][0])
        cls = "DIFF" if not same else ("ERRS" if err else "SAME")
        counts[cls] += 1
        m = re.search(r"CODE (\d+)", outs["ON"][1])
        code = int(m.group(1)) if m else -1
        print("%-4s %-10s %-26s CODE %-7d %s" % (cls, f, name[:26], code, " | ".join(outs["ON"][0])[-70:]), flush=True)
        if cls != "SAME":
            for mode in ("OFF", "ON", "SHADOW"):
                print("       %-6s %s" % (mode, " | ".join(outs[mode][0])[-300:]))
        if cls == "DIFF":
            diffs.append("%s/%s" % (f, name))
    b.cmd("Option Compile Off", 10)
    b.cmd('Chdir "A:/"')
    b.close()
    print("%d programs: %d same, %d with the same errors, %d differ" % (len(tests), counts["SAME"], counts["ERRS"], counts["DIFF"]))
    if diffs:
        print("differ: " + ", ".join(diffs))
    sys.exit(1 if diffs else 0)


if __name__ == "__main__":
    main()
