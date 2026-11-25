"""threadcheck.py PORT OUTDIR [--baud N] [--against DIR] [--modes OFF,ON,SHADOW] [--list FILE] [--stack] [--rb] [--nocrash]

The quick check for the threads work (docs/Threads_Plan.md): the regression programs that bear on what threads touch
(the statement loop, SUBs and FUNCTIONs, locals and their bindings, interrupts, PAUSE, errors, END), named in
tests/threads/quick.txt, each run ONCE with OPTION COMPILE OFF, ON and SHADOW; then the timing programs of that list,
and the crasher suite. Five minutes on a G1, where the full regression (goldens.py, every program twice, every suite)
takes over an hour: this is for each step of the work, the full regression for the end of it. --rb adds the Route B
programs on calls, locals and the bind caches (rbtest.py --only: each OFF, ON and SHADOW, eight minutes more), for a
step that touches what the compiler shares with the threads.

Each output is kept as OUTDIR/<MODE>/<corpus>_<name>.out. With --against DIR (an earlier OUTDIR: the build before)
every output is compared with that run's as it stands, nothing masked, and each timing with that run's; a program with
an .expected is also compared with it as goldens.py compares (its class). --stack reads MM.INFO(STACK LOW) after each
program: the deepest the main stack went in it.

The programs run from the full regression's directories on the card (A:/cmm2t, A:/pmt; A:/thr for tests/threads), with
the same MODE mapping goldens.py gives them; only files missing there or of another size are sent.

Exit 1 if an output differs from --against, a program of tests/threads does not match its .expected, or the crashers
or (--rb) the Route B programs fail. OPTION COMPILE is left as it was found.
"""
import json, os, re, subprocess, sys, tempfile, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import goldens
from goldens import Board, ANSI, run_program, norm, modemap, read_inputs, card_sizes

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
TOOLS = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(TOOLS)
CARD = {"cmm2": "A:/cmm2t", "picomite": "A:/pmt", "threads": "A:/thr"}
SUPPORT = (".dat", ".txt", ".csv", ".inc", ".bmp", ".jpg", ".png", ".gif", ".map")
RBFILES = "rb_cmm2,rblocal,rbcall,rbcallname,rbcache,rbdepth,rbnopchain,rbstamp"     # calls, locals, the bind caches
STACKTOP = 0x2001FFF0


def arg(name, default=None):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default


def read_list(path):
    """The list's programs and its timing programs, each (corpus, name)."""
    progs, timed = [], []
    for line in open(path, encoding="utf-8"):
        line = line.split("'")[0].strip()
        if not line:
            continue
        to = progs
        if line.lower().startswith("time "):
            to, line = timed, line[5:].strip()
        corpus, name = line.split("/")
        to.append((corpus, name))
    return progs, timed


def send(b, corpus, names):
    """The corpus's listed programs and its data files on the card, as goldens.py puts them there."""
    src, d = os.path.join(REPO, "tests", corpus), CARD[corpus]
    b.cmd('MKDIR "%s"' % d, 10)
    b.cmd('CHDIR "%s"' % d, 10)
    onboard = card_sizes(b, d)
    files = [n + ".bas" for n in names] + [f for f in os.listdir(src) if f.lower().endswith(SUPPORT) and f != "quick.txt"]
    stage, todo = tempfile.mkdtemp(prefix="threadcheck_"), []
    for f in sorted(set(files)):
        data = open(os.path.join(src, f), "rb").read()
        if f.lower().endswith(".bas"):
            data = modemap(data.decode("latin-1")).encode("latin-1")
        if onboard.get(f.lower()) != len(data):
            open(os.path.join(stage, f), "wb").write(data)
            todo.append(os.path.join(stage, f))
    for i in range(0, len(todo), 40):
        b.zmodem_put(todo[i:i + 40])
    return len(todo)


def classify(corpus, name, status, out, inputs):
    """The program's class against its .expected, as goldens.py's score gives it."""
    exp = os.path.join(REPO, "tests", corpus, name + ".expected")
    if status != "OK":
        return status
    if not os.path.exists(exp):
        return "NO-EXPECTED"
    return "MATCH" if norm(open(exp, encoding="latin-1").read()) == norm(out, inputs) else "DIFF"


def compile_mode(b, mode):
    b.cmd("OPTION COMPILE " + mode, 40)                 # (a G1 restarts the first time ON is given, to make its area)
    time.sleep(0.5)
    b.attention()


def first_diff(a, c):
    la, lc = a.splitlines(), c.splitlines()
    for x, y in zip(la, lc):
        if x != y:
            return "%r | %r" % (x[:60], y[:60])
    return "%d lines | %d lines" % (len(la), len(lc))


def main():
    port, outdir = sys.argv[1], sys.argv[2]
    baud = int(arg("--baud", "460800"))
    against = arg("--against")
    modes = arg("--modes", "OFF,ON,SHADOW").upper().split(",")
    progs, timed = read_list(arg("--list", os.path.join(REPO, "tests", "threads", "quick.txt")))
    stack = "--stack" in sys.argv
    t_start = time.time()
    os.makedirs(outdir, exist_ok=True)
    b = Board(port, baud)
    b.attention()
    print(ANSI.sub("", b.cmd('PRINT MM.DEVICE$; " "; MM.VER', 10)).strip(), flush=True)
    was = (ANSI.sub("", b.cmd("? MM.INFO$(COMPILE)", 10)).strip().split() or ["OFF"])[0].upper()
    was = was if was in ("OFF", "ON", "SHADOW") else "OFF"
    sent = 0
    for corpus in sorted(set(c for c, _ in progs + timed)):
        sent += send(b, corpus, [n for c, n in progs + timed if c == corpus])
    print("%d programs, %d timed; %d files sent" % (len(progs), len(timed), sent), flush=True)

    rows, bad = [], []
    for mode in modes:
        compile_mode(b, mode)
        os.makedirs(os.path.join(outdir, mode), exist_ok=True)
        t0, cur, counts, deepest = time.time(), None, {}, (0, "")
        for corpus, name in progs:
            if corpus != cur:
                b.cmd('CHDIR "%s"' % CARD[corpus], 10)
                cur = corpus
            Board.current = name
            inputs = read_inputs(os.path.join(REPO, "tests", corpus), name)
            status, out, secs = run_program(b, 'RUN "%s.bas"' % name, inputs)
            if status not in ("OK", "ERROR"):
                b.attention()
            key = "%s_%s" % (corpus, name)
            open(os.path.join(outdir, mode, key + ".out"), "w", encoding="utf-8").write(out)
            cls = classify(corpus, name, status, out, inputs)
            counts[cls] = counts.get(cls, 0) + 1
            row = {"mode": mode, "prog": corpus + "/" + name, "class": cls, "secs": secs}
            if stack:
                low = ANSI.sub("", b.cmd("? Hex$(MM.Info(Stack Low))", 10)).strip().splitlines()
                try:
                    row["stack"] = STACKTOP - int(low[-1].strip(), 16)
                    deepest = max(deepest, (row["stack"], name))
                except (ValueError, IndexError):
                    pass
            rows.append(row)
            if corpus == "threads" and cls != "MATCH":
                bad.append("%s %s/%s: %s" % (mode, corpus, name, cls))
            if against:
                old = os.path.join(against, mode, key + ".out")
                if not os.path.exists(old):
                    print("  %-7s %-22s not in %s" % (mode, corpus + "/" + name, against), flush=True)
                else:
                    was_out = open(old, encoding="utf-8").read()
                    if was_out != out:
                        bad.append("%s %s/%s differs" % (mode, corpus, name))
                        print("  %-7s %-22s DIFFERS: %s" % (mode, corpus + "/" + name, first_diff(was_out, out)), flush=True)
        print("%-7s %s in %.0f s%s" % (mode, " ".join("%s %d" % kv for kv in sorted(counts.items())), time.time() - t0,
              ("; deepest stack %d bytes (%s)" % deepest) if stack and deepest[0] else ""), flush=True)

    # the timing programs: each RESULT line, with OFF and with ON
    times = {}
    for mode in [m for m in modes if m in ("OFF", "ON")]:
        compile_mode(b, mode)
        for corpus, name in timed:
            b.cmd('CHDIR "%s"' % CARD[corpus], 10)
            status, out, secs = run_program(b, 'RUN "%s.bas"' % name, [])
            for m in re.finditer(r"RESULT (.*?)\s*(-?[\d.]+) us", out):
                times["%s %s: %s" % (mode, name, m.group(1).strip())] = float(m.group(2))
    old_times = {}
    if against and os.path.exists(os.path.join(against, "timing.json")):
        old_times = json.load(open(os.path.join(against, "timing.json")))
    for k, v in times.items():
        o = old_times.get(k)
        ch = (v - o) / o * 100 if o else None
        print("  %-44s %8.2f us%s" % (k, v, "" if ch is None else "  was %8.2f  %+5.1f%%%s" % (o, ch, "  <--" if abs(ch) >= 3 else "")), flush=True)
    json.dump(times, open(os.path.join(outdir, "timing.json"), "w"), indent=1)
    json.dump(rows, open(os.path.join(outdir, "summary.json"), "w"), indent=1)

    compile_mode(b, was)
    b.cmd('CHDIR "A:/"', 10)
    b.close()

    for name, run, cmd in (("crashers", "--nocrash" not in sys.argv, [os.path.join(TOOLS, "crashers_test.py"), port, str(baud)]),
                           ("rbtest", "--rb" in sys.argv, [os.path.join(TOOLS, "rbtest.py"), port, str(baud), "--only", RBFILES])):
        if not run:
            continue
        t0 = time.time()
        with open(os.path.join(outdir, name + ".log"), "w", encoding="utf-8") as log:
            r = subprocess.run([sys.executable, "-u"] + cmd, cwd=REPO, stdout=log, stderr=subprocess.STDOUT)
        tail = open(os.path.join(outdir, name + ".log"), encoding="utf-8", errors="replace").read().strip().splitlines()[-1:]
        print("%-8s exit %d in %.0f s: %s" % (name, r.returncode, time.time() - t0, " ".join(tail)[:150]), flush=True)
        if r.returncode:
            bad.append(name + " failed")
        if name == "rbtest" and against and os.path.exists(os.path.join(against, "rbtest.log")):
            # each program's class, file and name: the rest of its line holds printed times, and for a program with
            # interrupts a count of compiled statements that depends on when they came
            res = lambda p: [l[:43] for l in open(p, encoding="utf-8", errors="replace").read().splitlines()
                             if re.match(r"(SAME|ERRS|DIFF) ", l)]
            if res(os.path.join(against, "rbtest.log")) != res(os.path.join(outdir, "rbtest.log")):
                bad.append("rbtest's results differ from " + against)
                print("         its results differ from %s's" % against, flush=True)
    restore_after_rbtest(port, baud, was)
    print("%s in %.1f min%s" % ("ALL THE SAME" if against and not bad else ("DONE" if not bad else "FAILED"),
          (time.time() - t_start) / 60, "" if not bad else ": " + "; ".join(bad[:12])), flush=True)
    sys.exit(1 if bad else 0)


def restore_after_rbtest(port, baud, was):
    """rbtest.py leaves OPTION COMPILE OFF: back to what the board had."""
    if was == "OFF" or "--rb" not in sys.argv:
        return
    b = Board(port, baud)
    b.attention()
    compile_mode(b, was)
    b.close()


if __name__ == "__main__":
    main()
