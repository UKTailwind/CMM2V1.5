"""goldens.py PORT OUTDIR [--baud N] [--corpus DIR] [--only NAME,...] [--from NAME] [--put] [--dir A:/g] [--pre CMD] [--listflash]
                         [--cap SECONDS] [--nomodemap] [--inputs DIR]
goldens.py --recompare OUTDIR [--corpus DIR]     re-diff saved outputs against .expected
goldens.py --reclassify OUTDIR [--corpus DIR]    score saved outputs again from scratch

Run the mmb2c test corpus (or any directory of .bas programs) on a CMM2 and keep what
this firmware prints as the golden for later builds, and compare it with each program's
.expected where there is one.  The CMM2 port of the PicoMite's Bas/bench/goldens.py: the
run, the classes and the files written are the same, so the two boards can be compared.

The corpus lives in A:/g/ on the SD card between runs (--dir elsewhere): only files missing
there, or of a different size, are sent (--put sends them all), in ZMODEM batches.  For
every program: CHDIR to A:/g, then RUN "name.bas" twice (the CMM2 has no LOAD: programs run
from the card).  The driver watches every byte and ends a
run as soon as it is over: back at the prompt, an Error, or an INPUT it has no answer for
(Ctrl-C).  It answers INPUT from <name>.in (in the corpus, or in --inputs DIR for a corpus that is not ours to add
files to), at a prompt ending in "?" or, while the file has answers left, in ":"; and PRESS ANY KEY (any case) with a
space.  A 60 s cap only catches a program that really does run away.

The corpus is written for the PicoMite, whose MODE numbers mean other resolutions on the
CMM2 (MODE 1 here is 800x600).  The copies sent to the board have their MODE statements
mapped as in section 6.3 of docs/PicoMite_Parity_Audit_and_Plan.md (MODE 1 -> MODE 8,8,
MODE 2 -> MODE 7,4, ...), so every program stays on a 640x480 signal; --nomodemap sends
them unchanged.

A board that no longer answers Ctrl-C after a run is reset through the ST-Link (connect
under reset) and the program is classed HUNG.

A program holding a CSUB or CFUNCTION, or loading a library or include from the corpus
that does, is SKIPPED: that code is built for the PicoMite's processors and must not run here.
A CSUB whose first line ends with the comment ' Cortex-M7 (CMM2) is the CMM2's own (mmb2csub
--target cmm2) and does run. So does one built for the PicoMite whose first line's comment says
' runs on the CMM2: such a CSUB uses only the CallTable, which the CMM2 also has in the PicoMite's
soft-float convention (CallTablePS, vector slot 7), so its code runs here unaltered.

Writes OUTDIR/<name>.out and .out2, <name>.diff where it differs from .expected, and
OUTDIR/SUMMARY.md and summary.json.
"""
import difflib, json, os, re, sys, tempfile, time
import serial
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import zmodem_pc

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
CORPUS = r"\\wsl.localhost\Ubuntu\home\peter\src\FUZIX\Applications\mmb2c\tests"
ANSI = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]|\x1b[=>78]|\r")     # ESC 7 / ESC 8 (save, restore cursor) come before CMM2 error messages
CAP = 60.0
INPUTS = None                                           # --inputs: more <name>.in files, for a corpus outside this repo


def read_inputs(corpus, n):
    """The answers for program n's INPUTs: <n>.in in the corpus, else in the --inputs directory."""
    for d in (corpus, INPUTS):
        if d and os.path.exists(os.path.join(d, n + ".in")):
            return open(os.path.join(d, n + ".in"), encoding="latin-1").read().splitlines()
    return []
MODEMAP = {"1": "8,8", "2": "7,4", "3": "8,4", "4": "7,16", "5": "7,8"}     # PicoMite 640x480 modes -> CMM2 (2 and 3: its RGB121, Rgb121.c)


FOREIGN = re.compile(r"(?im)^[ \t]*(?:CSUB|CFUNCTION)\b(?![^\n]*'[^\n]*(?:Cortex-M7 \(CMM2\)|runs on the CMM2))")   # (not one marked as running here)
LOADS = re.compile(r"""(?im)^[ \t]*(?:LIBRARY[ \t]+LOAD|#INCLUDE)\b([^\n']*)""")


def foreign_code(corpus, name, seen=()):
    """The file holding a CSUB or CFUNCTION, in this program or in a library or include
    it loads from the corpus: that code is built for the PicoMite (Cortex-M0+/M33) and its
    calls into the PicoMite's CallTable land on unrelated CMM2 functions, so it must not run."""
    path = os.path.join(corpus, name)
    if name.lower() in seen or not os.path.exists(path):
        return None
    text = open(path, encoding="latin-1").read()
    if FOREIGN.search(text):
        return name
    for args in LOADS.findall(text):
        for ref in re.findall(r'"([^"]+)"', args):
            hit = foreign_code(corpus, os.path.basename(ref.replace("\\", "/")), tuple(seen) + (name.lower(),))
            if hit:
                return hit
    return None


def modemap(text):
    """MODE n as a statement (start of line or after a colon), n a PicoMite mode with no
    colour depth given, becomes the CMM2 mode of the same geometry."""
    return re.sub(r"(?im)((?:^|:)[ \t]*MODE[ \t]+)([1-5])\b(?![ \t]*,)",
                  lambda m: m.group(1) + MODEMAP[m.group(2)], text)


class Board:
    def __init__(self, port, baud):
        self.baud = baud
        self.s = serial.Serial(port, baud, timeout=0.05, write_timeout=5)

    def close(self):
        self.s.close()

    def _read(self):
        b = self.s.read(4096)
        return b.decode("latin-1") if b else ""

    def drain(self, quiet=0.2):
        out, last = "", time.time()
        while time.time() - last < quiet:
            t = self._read()
            if t:
                out += t
                last = time.time()
        return out

    def wait_prompt(self, timeout=10.0):
        """Collect output until the console shows its '> ' prompt and goes quiet."""
        buf, t0 = "", time.time()
        while time.time() - t0 < timeout:
            t = self._read()
            if t:
                buf += t
                clean = ANSI.sub("", buf)
                if clean.endswith("\n> ") or clean.endswith("\n>") or clean in ("> ", ">"):
                    time.sleep(0.05)
                    tail = self._read()
                    if not tail:
                        return ANSI.sub("", buf)
                    buf += tail
        raise TimeoutError("no prompt after %.0fs; last output:\n%s" % (timeout, ANSI.sub("", buf)[-800:]))

    def send_line(self, line):
        self.s.write(line.encode("latin-1") + b"\r")

    def attention(self):
        """Ctrl-C twice, then wait for a prompt."""
        self.s.write(b"\x03")
        time.sleep(0.3)
        self.s.write(b"\x03")
        time.sleep(0.3)
        self.drain(0.3)
        self.s.write(b"\r")
        return self.wait_prompt(5)

    def cmd(self, line, timeout=10.0):
        """Run one command at the prompt; return its output without the echo and the prompt."""
        self.drain(0.05)
        self.send_line(line)
        out = self.wait_prompt(timeout)
        return "\n".join(l for l in out.split("\n") if l.strip() not in ("", ">", "> ", line.strip()))

    def zmodem_put(self, paths):
        """Send files into the current directory in one ZMODEM batch."""
        self.drain(0.05)
        self.send_line("ZMODEM RECEIVE")
        time.sleep(0.5)
        zmodem_pc.send(self.s, self.baud, paths, 32768)
        return self.wait_prompt(30)


def norm(text, inputs=()):
    """What the corpus Makefile compares: no CR, no 'Time taken' lines.  A board also echoes
    what is typed at an INPUT, which the translator's stdin does not, so the echoed answers
    (and the blank line after each) come out too; so do leading and trailing blank lines."""
    lines = [l.rstrip() for l in text.replace("\r", "").split("\n")]
    if inputs:
        out, want = [], list(inputs)
        i = 0
        while i < len(lines):
            if want and lines[i].strip() == want[0].strip():
                want.pop(0)
                i += 1
                if i < len(lines) and lines[i] == "":
                    i += 1
                continue
            out.append(lines[i])
            i += 1
        lines = out
    lines = [l for l in lines if "Time taken" not in l]
    while lines and not lines[-1]:
        lines.pop()
    while lines and not lines[0]:
        lines.pop(0)
    return "\n".join(lines)


def recompare(outdir, corpus):
    """Re-score saved outputs against .expected without running anything."""
    rows = json.load(open(os.path.join(outdir, "summary.json")))
    for r in rows:
        n = r["name"]
        outf = os.path.join(outdir, n + ".out")
        exp = os.path.join(corpus, n + ".expected")
        if r["class"] not in ("MATCH", "DIFF") or not os.path.exists(outf) or not os.path.exists(exp):
            continue
        inputs = read_inputs(corpus, n)
        got = norm(open(outf, encoding="utf-8").read(), inputs)
        want = norm(open(exp, encoding="latin-1").read())
        dpath = os.path.join(outdir, n + ".diff")
        if got == want:
            r["class"] = "MATCH"
            if os.path.exists(dpath):
                os.remove(dpath)
        else:
            r["class"] = "DIFF"
            d = difflib.unified_diff(want.split("\n"), got.split("\n"), "expected", "board", lineterm="")
            open(dpath, "w", encoding="utf-8").write("\n".join(d))
    json.dump(rows, open(os.path.join(outdir, "summary.json"), "w"), indent=1)
    return rows


def score(n, s1, o1, o2, t1, inputs, corpus, outdir):
    """The result row for one program from its two outputs and how the first run ended."""
    det = norm(o1, inputs) == norm(o2, inputs)
    exp = os.path.join(corpus, n + ".expected")
    dpath = os.path.join(outdir, n + ".diff")
    if os.path.exists(dpath):
        os.remove(dpath)
    cls = s1
    note = ""
    if s1 == "OK" and os.path.exists(exp):
        want = norm(open(exp, encoding="latin-1").read())
        got = norm(o1, inputs)
        if want == got:
            cls = "MATCH"
        else:
            cls = "DIFF"
            d = difflib.unified_diff(want.split("\n"), got.split("\n"), "expected", "board", lineterm="")
            open(dpath, "w", encoding="utf-8").write("\n".join(d))
    elif s1 == "OK":
        cls = "NO-EXPECTED"
    if s1 in ("ERROR", "NEEDS-INPUT", "TIMEOUT"):
        errs = re.findall(r"[^\n]*\bError\b[^\n]*", o1)        # the last one is what stopped it
        note = (errs[-1] if errs else o1.strip().split("\n")[-1])[-160:]
    if "MMBasic Version" in o1 + o2:
        note = "RESET " + note                          # the sign-on banner: the board restarted (MM.DEVICE$ alone is not one)
    return {"name": n, "class": cls, "deterministic": det, "secs": t1, "note": note.replace("\n", " | ")}


def reclassify(outdir, corpus):
    """Score the saved outputs again, without the board: for when the scoring itself changes
    (e.g. an escape sequence that hid an "Error").  Runs that timed out, hung, needed input
    or were skipped keep their class."""
    rows = json.load(open(os.path.join(outdir, "summary.json")))
    for i, r in enumerate(rows):
        n = r["name"]
        if r["class"] in ("TIMEOUT", "NEEDS-INPUT", "SKIPPED", "HUNG"):
            continue
        o = []
        for ext in (".out", ".out2"):
            p = os.path.join(outdir, n + ext)
            t = ANSI.sub("", open(p, encoding="utf-8").read()) if os.path.exists(p) else ""
            open(p, "w", encoding="utf-8").write(t)
            o.append(t)
        inputs = read_inputs(corpus, n)
        s1 = "ERROR" if re.search(r"\bError\b", o[0]) else "OK"
        rows[i] = score(n, s1, o[0], o[1], r.get("secs", ""), inputs, corpus, outdir)
    json.dump(rows, open(os.path.join(outdir, "summary.json"), "w"), indent=1)
    return rows


def run_program(b, run, inputs):
    """RUN a program; return (class, output, seconds)."""
    b.drain(0.2)
    b.send_line(run)
    buf, t0, fed, last = "", time.time(), 0, time.time()
    anykey = 0                                          # "Press any key" (not upper case) prompts answered
    status = None
    while True:
        t = b._read()
        now = time.time()
        if t:
            buf += t
            last = now
        clean = ANSI.sub("", buf)
        tail = clean[-200:]
        if re.search(r"PRESS ANY KEY", tail):
            time.sleep(0.1)
            b.s.write(b" ")
            buf = buf.replace("PRESS ANY KEY", "PRESS-ANY-KEY")
            continue
        k = len(re.findall(r"[Pp]ress any key", clean))    # (counted, so the output is left as printed)
        if k > anykey:
            time.sleep(0.1)
            b.s.write(b" ")
            anykey = k
            continue
        quiet = now - last > 0.3
        if quiet and re.search(r"\n> ?$", clean):
            status = "ERROR" if re.search(r"\bError\b", clean) else "OK"
            break
        if quiet and (clean.rstrip(" ").endswith("?") or (clean.rstrip(" ").endswith(":") and fed < len(inputs))):
            if fed < len(inputs):
                b.send_line(inputs[fed])
                fed += 1
                buf += "\n"
                last = time.time()
                continue
            b.s.write(b"\x03")
            status = "NEEDS-INPUT"
            time.sleep(0.5)
            buf += b.drain(0.5)
            break
        if now - t0 > CAP:
            b.s.write(b"\x03")
            time.sleep(0.3)
            b.s.write(b"\x03")
            status = "TIMEOUT"
            time.sleep(0.5)
            buf += b.drain(0.5)
            break
        if not t:
            time.sleep(0.01)
    clean = ANSI.sub("", buf)
    clean = re.sub(r"^\s*RUN[^\n]*\n", "", clean)       # drop the echoed RUN and the final prompt
    clean = re.sub(r"\n> ?$", "\n", clean)
    return status, clean, round(time.time() - t0, 1)


STLINK = r"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"


def alive(b):
    """True if the board answers Ctrl-C with a prompt.  If it does not, reset it through the
    ST-Link and return False.  Connect under reset only: a hot-plug connect cannot reach a
    running CMM2 and leaves the probe wedged."""
    try:
        b.attention()
        return True
    except TimeoutError:
        pass
    print("board not answering: resetting it through the ST-Link", flush=True)
    import subprocess
    subprocess.run([STLINK, "-c", "port=SWD", "mode=UR", "freq=1000", "-rst"], capture_output=True, timeout=120)
    time.sleep(10)
    b.drain(0.5)
    b.attention()                                       # still nothing: stop the run here
    return False


def arg(name, default=None):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default


def main():
    global CAP, INPUTS
    INPUTS = arg("--inputs")
    if sys.argv[1] in ("--recompare", "--reclassify"):
        outdir = sys.argv[2]
        corpus = arg("--corpus", CORPUS)
        device = open(os.path.join(outdir, "SUMMARY.md"), encoding="utf-8").readline().replace("# Goldens on", "").strip()
        rows = (recompare if sys.argv[1] == "--recompare" else reclassify)(outdir, corpus)
        write_summary(outdir, rows, device)
        counts = {}
        for r in rows:
            counts[r["class"]] = counts.get(r["class"], 0) + 1
        print(counts)
        return
    port, outdir = sys.argv[1], sys.argv[2]
    baud = int(arg("--baud", "460800"))
    CAP = float(arg("--cap", CAP))                      # seconds a run may take before Ctrl-C (perfguide needs more)
    corpus = arg("--corpus", CORPUS)
    only = arg("--only").split(",") if "--only" in sys.argv else None
    gdir = arg("--dir", "A:/g").rstrip("/")
    mapped = "--nomodemap" not in sys.argv
    pre = arg("--pre")                                  # a command sent before each program
    listflash = "--listflash" in sys.argv               # also keep LIST FLASH ALL of each program (NAME.list)
    os.makedirs(outdir, exist_ok=True)
    names = sorted(f[:-4] for f in os.listdir(corpus) if f.lower().endswith(".bas"))
    if only:
        names = [n for n in names if n in only]
    # Every result goes to rows.jsonl as it comes, so a run that stops can go on with --from NAME,
    # keeping the results before NAME.
    saved = os.path.join(outdir, "rows.jsonl")
    start = arg("--from")
    rows = []
    if start:
        if os.path.exists(saved):
            rows = [r for r in (json.loads(l) for l in open(saved, encoding="utf-8") if l.strip()) if r["name"] < start]
        names = [n for n in names if n >= start]
    open(saved, "w", encoding="utf-8").write("".join(json.dumps(r) + "\n" for r in rows))

    def keep(r):
        print(json.dumps(r), flush=True)
        open(saved, "a", encoding="utf-8").write(json.dumps(r) + "\n")

    b = Board(port, baud)
    b.attention()
    device = b.cmd('PRINT MM.DEVICE$; " "; MM.VER', 10).strip()
    speed = b.cmd('PRINT MM.INFO(CPUSPEED)', 10).strip()
    if speed.isdigit():
        device += " " + speed
    if mapped:
        device += " (PicoMite MODEs mapped)"
    b.cmd('MKDIR "%s"' % gdir, 10)                     # an error if it is there already, which is fine
    b.cmd('CHDIR "%s"' % gdir, 10)
    # Send a file only if the card lacks it or has a different size, or everything with --put.
    onboard = {}
    if "--put" not in sys.argv:
        listing = b.cmd('f$=Dir$("%s/*",FILE):Do While f$<>"":Print f$;"|";MM.INFO(FILESIZE "%s/"+f$):f$=Dir$():Loop' % (gdir, gdir), 60)
        for line in listing.splitlines():
            if "|" in line:
                name, size = line.rsplit("|", 1)
                if size.strip().isdigit():
                    onboard[name.strip().lower()] = int(size)
    files = [f for f in os.listdir(corpus) if f.lower().endswith(".bas") and f[:-4] in names]   # every program: some run or read others
    files += [f for f in os.listdir(corpus) if f.lower().endswith((".dat", ".txt", ".csv", ".inc", ".bmp", ".jpg", ".png", ".gif", ".map"))]
    stage = tempfile.mkdtemp(prefix="goldens_")
    todo = []
    for f in sorted(files):
        data = open(os.path.join(corpus, f), "rb").read()
        if mapped and f.lower().endswith(".bas"):
            data = modemap(data.decode("latin-1")).encode("latin-1")
        if onboard.get(f.lower()) != len(data):
            open(os.path.join(stage, f), "wb").write(data)
            todo.append(os.path.join(stage, f))
    for i in range(0, len(todo), 40):                   # a few batches, so one bad file does not cost the lot
        b.zmodem_put(todo[i:i + 40])
    print("sent %d of %d files to %s" % (len(todo), len(files), gdir), flush=True)
    for n in names:
        inputs = read_inputs(corpus, n)
        b.attention()
        b.cmd('CHDIR "%s"' % gdir, 10)
        if pre:
            b.cmd(pre, 10)                              # eg OPTION SYMBOLS OFF (the symbols' test switch)
        prog = next(f for f in files if f[:-4] == n)
        foreign = foreign_code(corpus, prog)
        if foreign:
            rows.append({"name": n, "class": "SKIPPED", "note": "PicoMite CSUB code in " + foreign})
            keep(rows[-1])
            continue
        run = 'RUN "%s"' % prog
        s1, o1, t1 = run_program(b, run, inputs)
        open(os.path.join(outdir, n + ".out"), "w", encoding="utf-8").write(o1)
        if not alive(b):
            rows.append({"name": n, "class": "HUNG", "secs": t1, "note": "no prompt after Ctrl-C, reset through the ST-Link: " + o1.strip()[-120:]})
            keep(rows[-1])
            continue
        if listflash:                                   # the image as LIST FLASH spells it out (text against symbols)
            open(os.path.join(outdir, n + ".list"), "w", encoding="utf-8").write(b.cmd("LIST FLASH ALL", 60))
        s2, o2, t2 = run_program(b, run, inputs)
        open(os.path.join(outdir, n + ".out2"), "w", encoding="utf-8").write(o2)
        if not alive(b):
            s2 = "HUNG"
            o2 += "\n[no prompt after Ctrl-C, reset through the ST-Link]\n"
        rows.append(score(n, s1, o1, o2, t1, inputs, corpus, outdir))
        keep(rows[-1])
    write_summary(outdir, rows, device)
    json.dump(rows, open(os.path.join(outdir, "summary.json"), "w"), indent=1)
    b.cmd("MODE 8", 10)
    b.close()


def write_summary(outdir, rows, device):
    with open(os.path.join(outdir, "SUMMARY.md"), "w", encoding="utf-8") as f:
        f.write("# Goldens on %s\n\n" % device)
        counts = {}
        for r in rows:
            counts[r["class"]] = counts.get(r["class"], 0) + 1
        f.write(" ".join("%s %d" % kv for kv in sorted(counts.items())) + "\n\n")
        f.write("| program | class | deterministic | secs | note |\n|---|---|---|---|---|\n")
        for r in rows:
            f.write("| %s | %s | %s | %s | %s |\n" % (r["name"], r["class"], r.get("deterministic", ""), r.get("secs", ""), r.get("note", "").replace("|", "/")))


if __name__ == "__main__":
    main()
