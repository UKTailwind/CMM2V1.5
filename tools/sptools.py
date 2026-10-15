"""sptools.py PORT OUTDIR [--src DIR] [--put] [--cap SECONDS] [--only GROUP,...] [--baseline DIR]
sptools.py --compare BASEDIR NEWDIR

Run the unit tests of Thomas Williams' SP Tools (https://github.com/thwill1000/mmbasic-sptools,
MIT licence) on the CMM2: some 600 tests of a large body of CMM2 BASIC (string, array, file,
list and map libraries, a transpiler spread over many #INCLUDE files, run under OPTION BASE 0
and 1).  They check themselves, so the CMM2 dialect is tested on real programs, where the
goldens corpus tests the PicoMite's.

The tree is fetched at the commit pinned below (or taken from --src) and kept in A:/sptools,
where its tools expect to be; only files missing there, or of a different size, are sent
(--put sends them all), directory by directory over ZMODEM.  Each group (src/splib, src/sptrans,
...) is run from its own directory with RUN "/sptools/sptest.bas", --catch-errors --verbose,
which chains through every tst_*.bas below it; the tests write their scratch files in A:/tmp.

Writes OUTDIR/<group>.out (the console output) and OUTDIR/results.json (file, options and test
name -> PASS / FAIL / ...).  With --baseline, or --compare, lists every test that passed in the
baseline and does not now, and the tests that appeared or vanished.
"""
import json, os, re, subprocess, sys, tempfile, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from goldens import ANSI, Board

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
REPO = "https://github.com/thwill1000/mmbasic-sptools.git"
COMMIT = "5e030f9"                                  # 6 Aug 2026; change it deliberately, then take a new baseline
ROOT = "A:/sptools"
GROUPS = ["src/splib", "src/sptrans", "src/spflow", "src/bas2js", "src/spgpconfig", "src/gonzo"]


def arg(name, default=None):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default


def fetch(commit):
    """A checkout of the pinned commit, cached in the temp directory.  Its files are only sent
    to the board; nothing in it is run on this machine."""
    d = os.path.join(tempfile.gettempdir(), "sptools_" + commit)
    if not os.path.isdir(os.path.join(d, ".git")):
        subprocess.run(["git", "clone", "-q", REPO, d], check=True)
    subprocess.run(["git", "-C", d, "checkout", "-q", commit], check=True)
    return d


def board_sizes(b, bdir):
    out = b.cmd('f$=Dir$("%s/*",FILE):Do While f$<>"":Print f$;"|";MM.INFO(FILESIZE "%s/"+f$):f$=Dir$():Loop' % (bdir, bdir), 60)
    sizes = {}
    for line in out.splitlines():
        if "|" in line:
            name, size = line.rsplit("|", 1)
            if size.strip().isdigit():
                sizes[name.strip().lower()] = int(size)
    return sizes


def upload(b, src, force):
    sent = 0
    for here, dirs, files in sorted(os.walk(src)):
        dirs[:] = sorted(d for d in dirs if d != ".git")
        rel = os.path.relpath(here, src).replace("\\", "/")
        bdir = ROOT if rel == "." else ROOT + "/" + rel
        b.cmd('MKDIR "%s"' % bdir, 10)                  # an error if it is there already, which is fine
        if not files:
            continue
        have = {} if force else board_sizes(b, bdir)
        todo = [os.path.join(here, f) for f in sorted(files)
                if have.get(f.lower()) != os.path.getsize(os.path.join(here, f))]
        if todo:
            b.cmd('CHDIR "%s"' % bdir, 10)
            b.zmodem_put(todo)
            sent += len(todo)
    return sent


def run_group(b, group, cap):
    """RUN sptest in the group's directory and collect the console until the prompt comes back."""
    b.attention()
    b.cmd('CHDIR "%s/%s"' % (ROOT, group), 10)
    b.drain(0.2)
    b.send_line('RUN "%s/sptest.bas", --catch-errors --verbose' % ROOT)
    buf, t0, last = "", time.time(), time.time()
    while time.time() - t0 < cap:
        t = b._read()
        if t:
            buf += t
            last = time.time()
        elif re.search(r"\n> ?$", ANSI.sub("", buf)) and time.time() - last > 3:
            return ANSI.sub("", buf), "DONE"
        else:
            time.sleep(0.02)
    b.s.write(b"\x03")
    time.sleep(0.5)
    b.s.write(b"\x03")
    return ANSI.sub("", buf + b.drain(1)), "TIMEOUT"


def parse(out, group):
    """{ "file [options]": { "test": "PASS" | "FAIL (n/m)" | ... } } from verbose sptest output."""
    res, key = {}, None
    for line in out.replace("\r", "").split("\n"):
        m = re.match(r"^(A:/sptools/\S+\.bas)\s*(.*)$", line, re.I)
        if m:
            opts = " ".join(o for o in m.group(2).split() if o.startswith("--base"))
            key = (m.group(1)[len(ROOT) + 1:] + " " + opts).strip()
            res.setdefault(key, {})
            continue
        m = re.match(r"^  (\w+):\s+(PASS|FAIL|NO ASSERTIONS|ERROR)(.*)$", line)
        if m and key:
            res[key][m.group(1)] = m.group(2) + m.group(3).rstrip()
    if not res:
        res[group + " (no test output)"] = {}
    return res


def compare(base, new):
    """Print what got worse; return the number of tests that passed in base and do not now."""
    worse = []
    for f in sorted(set(base) | set(new)):
        b, n = base.get(f, {}), new.get(f, {})
        for t in sorted(set(b) | set(n)):
            was, now = b.get(t, "-"), n.get(t, "-")
            if was.startswith("PASS") and not now.startswith("PASS"):
                worse.append("  WORSE   %s %s: %s -> %s" % (f, t, was, now))
            elif was != now and (was == "-" or now == "-"):
                print("  %s %s %s" % ("NEW    " if was == "-" else "MISSING", f, t))
            elif was != now and not was.startswith("PASS") and now.startswith("PASS"):
                print("  BETTER  %s %s: %s -> %s" % (f, t, was, now))
    for w in worse:
        print(w)
    total = sum(len(v) for v in new.values())
    passed = sum(1 for v in new.values() for s in v.values() if s.startswith("PASS"))
    print("%d of %d tests pass; %d passed in the baseline and do not now" % (passed, total, len(worse)))
    return len(worse)


def main():
    if sys.argv[1] == "--compare":
        base = json.load(open(os.path.join(sys.argv[2], "results.json")))
        new = json.load(open(os.path.join(sys.argv[3], "results.json")))
        sys.exit(1 if compare(base, new) else 0)
    port, outdir = sys.argv[1], sys.argv[2]
    cap = float(arg("--cap", "1800"))
    groups = arg("--only").split(",") if "--only" in sys.argv else GROUPS
    src = arg("--src") or fetch(COMMIT)
    os.makedirs(outdir, exist_ok=True)
    b = Board(port, 460800)
    b.attention()
    b.cmd('MKDIR "A:/tmp"', 10)
    print("sent %d files to %s" % (upload(b, src, "--put" in sys.argv), ROOT), flush=True)
    results = {}
    for g in groups:
        t0 = time.time()
        out, how = run_group(b, g, cap)
        open(os.path.join(outdir, g.replace("/", "_") + ".out"), "w", encoding="utf-8").write(out)
        r = parse(out, g)
        results.update(r)
        n = sum(len(v) for v in r.values())
        p = sum(1 for v in r.values() for s in v.values() if s.startswith("PASS"))
        print("%-16s %s %4.0f s  %d of %d tests pass" % (g, how, time.time() - t0, p, n), flush=True)
    json.dump(results, open(os.path.join(outdir, "results.json"), "w"), indent=1, sort_keys=True)
    b.cmd('CHDIR "A:/"', 10)
    b.close()
    if "--baseline" in sys.argv:
        compare(json.load(open(os.path.join(arg("--baseline"), "results.json"))), results)


if __name__ == "__main__":
    main()
