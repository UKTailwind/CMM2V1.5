"""flashslots_test.py PORT [BAUD]: the flash slots on a CMM2 (ImageSlots.c), driven at the prompt.

Slots 1 to 3 are the PicoMite's flash slots, kept in A:/.flash on the SD card and run in place from the heap: FLASH
SAVE, OVERWRITE, LOAD, RUN, CHAIN, LIST, ERASE, LOAD IMAGE and DISK LOAD, MM.INFO(FLASH) and MM.INFO(FLASH ADDRESS n),
a program with CSUBs and one with a font, an error in a slot's program, RUN inside one, a program of another firmware's
format, and the slots kept through a restart. The programs go to A:/fstest by ZMODEM (csubm7.bas from tests/cmm2).
The slots are erased at the start and the end. Prints each check and "N of M checks passed"."""
import os, sys, tempfile, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from goldens import Board

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)

PROGRAMS = {
    "fsa.bas": """' fsa.bas: the flash slots test's first program
Print "A runs in slot"; MM.Info(FLASH)
Hello 1
Print "A ends"
Sub Hello(n)
  Print "hello"; n
End Sub
""",
    "fsb.bas": """' fsb.bas: a global, then slot 3's program chained to
Dim integer kept = 77
Print "B in slot"; MM.Info(FLASH)
Flash Chain 3
Print "not here"
""",
    "fsc.bas": """' fsc.bas: chained to with fsb's global; then, in a slot, fsa run from its file
Print "C in slot"; MM.Info(FLASH); " kept="; kept
Read a, b
Print "C data"; a; b
If MM.Info(FLASH) Then Run "fsa"
Data 5, 6
""",
    "fserr.bas": """' fserr.bas: an error in a slot's program
q = 5
Print "E"; q
x = 1 / 0
""",
    "fsrun.bas": """' fsrun.bas: RUN in a slot's program runs that program again
Open "fsrun.cnt" For Append As #1 : Print #1, "x" : Close #1
If MM.Info(FILESIZE "fsrun.cnt") < 6 Then Run
Print "ran twice in slot"; MM.Info(FLASH)
""",
    "fsfont.bas": """' fsfont.bas: a font in a slot's program
DefineFont #9
  01410404 F99F0000
End DefineFont
Font 9
w = MM.Info(FONTWIDTH) : h = MM.Info(FONTHEIGHT)
Font 1
Print "font"; w; h
""",
    "fsold.bas": """' fsold.bas: slot 1's file with another format's stamp, as old.slot
Open "A:/.flash/slot1" For Input As #1
Open "old.slot" For Output As #2
a$ = Input$(255, #1)
Print #2, Left$(a$, 7); "5"; Mid$(a$, 9);
Do While Not Eof(#1)
  a$ = Input$(255, #1)
  Print #2, a$;
Loop
Close #1, #2
Print "old.slot made"
""",
}

passed = failed = 0


def check(name, out, must=(), mustnot=()):
    global passed, failed
    low = out.lower()
    bad = [m for m in must if m.lower() not in low] + ["NOT " + m for m in mustnot if m.lower() in low]
    if bad:
        failed += 1
        print("FAIL  %s: wanted %s\n      got: %r" % (name, bad, out[-600:]))
    else:
        passed += 1
        print("ok    %s" % name)


def main():
    port = sys.argv[1]
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 460800
    b = Board(port, baud)
    b.attention()
    c = b.cmd
    c('Chdir "A:/"')
    c('Mkdir "fstest"')
    c('Chdir "A:/fstest"')
    tmp = tempfile.mkdtemp()
    paths = []
    for n, t in PROGRAMS.items():
        p = os.path.join(tmp, n)
        open(p, "w", newline="\r\n").write(t)
        paths.append(p)
    paths.append(os.path.join(REPO, "tests", "cmm2", "csubm7.bas"))
    c('Kill "fsrun.cnt"')
    for p in paths:
        c('Kill "%s"' % os.path.basename(p))
    b.zmodem_put(paths)

    c("Flash Erase All")
    check("FLASH LIST, all empty", c("Flash List"), ["Slot 1 available", "Slot 2 available", "Slot 3 available"])
    check("FLASH RUN of an empty slot", c("Flash Run 1"), ["Flash slot empty"])

    # SAVE, RUN, MM.INFO(FLASH), LIST, the prompt back at program memory
    check("RUN fsa", c('Run "fsa"'), ["A runs in slot 0", "hello 1", "A ends"])
    check("FLASH SAVE 1", c("Flash Save 1"), [], ["Error"])
    check("FLASH LIST names it", c("Flash List"), ['Slot 1 in use: "', 'FSA.BAS"', "Slot 2 available"])
    check("FLASH SAVE 1 again", c("Flash Save 1"), ["Already programmed"])
    check("FLASH OVERWRITE 1", c("Flash Overwrite 1"), [], ["Error"])
    check("FLASH RUN 1", c("Flash Run 1"), ["A runs in slot 1", "hello 1", "A ends"])
    check("MM.INFO(FLASH) at the prompt", c("Print MM.Info(FLASH)"), [" 0"])
    check("LIST FLASH is program memory's", c("List Flash"), ["Hello 1"])
    check("FLASH LIST 1", c("Flash List 1"), ["Sub Hello", "End Sub"])
    check("FLASH CHAIN at the prompt", c("Flash Chain 1"), ["Invalid at command prompt"])

    # CHAIN between slots with a global, DATA in the chained program, RUN "file" from a slot's program
    c('Run "fsb"', 15)
    c("Flash Save 2")
    c('Run "fsc"', 15)
    c("Flash Save 3")
    out = c("Flash Run 2", 15)
    check("FLASH CHAIN keeps the global", out, ["B in slot 2", "C in slot 3 kept= 77", "C data 5 6", "A runs in slot 0"], ["not here"])

    # an error in a slot's program: reported, the variables kept, no CONTINUE, the prompt's own program again
    c('Run "fserr"')
    c("Flash Overwrite 3")
    out = c("Flash Run 3")
    check("an error in a slot's program", out, ["E 5", "Divide by zero"])
    check("its variables are kept", c("Print q"), [" 5"])
    check("CONTINUE is refused", c("Continue"), ["Cannot continue"])
    check("then MM.INFO(FLASH) is 0", c("Print MM.Info(FLASH)"), [" 0"])
    check("and RUN runs program memory's", c("Run"), ["E 5"])

    # RUN inside a slot's program runs it again
    c('Run "fsrun"')
    c("Flash Overwrite 3")
    c('Kill "fsrun.cnt"')
    check("RUN in a slot's program", c("Flash Run 3", 15), ["ran twice in slot 3"])

    # CSUBs: their records made addresses in the block (the executable window), and in program memory by FLASH LOAD
    out = c('Run "csubm7"', 30)
    check("csubm7 from its file", out, ["bad 0"])
    c("Flash Overwrite 2")
    c('Run "fsa"')
    check("csubm7 from slot 2", c("Flash Run 2", 30), ["Mix 0 8642020.533565", "Loop1 281.385893485", "bad 0"])
    c("New")
    check("FLASH LOAD 2 then RUN", c("Flash Load 2") + c("Run", 30), ["bad 0"], ["Error"])

    # a font's record is its number, left alone
    font = c('Run "fsfont"')
    check("fsfont from its file", font, ["font 4 4"])
    c("Flash Overwrite 3")
    check("fsfont from slot 3", c("Flash Run 3"), ["font 4 4"])

    # FLASH LOAD: RUN runs what was loaded, not the file it came from, until a file is loaded
    c('Open "fsd.bas" For Output As #1 : Print #1, "Print " + Chr$(34) + "D1" + Chr$(34) : Close #1')
    c('Run "fsd"')
    c("Flash Overwrite 3")
    c('Open "fsd.bas" For Output As #1 : Print #1, "Print " + Chr$(34) + "D2" + Chr$(34) : Close #1')
    check("FLASH LOAD 3 then RUN", c("Flash Load 3") + c("Run"), ["D1"], ["D2"])
    check("RUN of the file", c('Run "fsd"'), ["D2"])
    check("RUN after that", c("Run"), ["D2"])

    # an image in a flash slot: in the file, read again after a heap wipe; an empty flash slot reads FF
    c('Save Image "fsimg.bmp", 0, 0, 16, 8')
    out = c('Flash Load Image 1, "fsimg", O')
    check("FLASH LOAD IMAGE 1", out, ["Saving fsimg.bmp to flash slot 1", "Image is 16 by 8"])
    peek = 'Print Peek(Word MM.Info(Flash Address 1)); Peek(Word MM.Info(Flash Address 1) + 4)'
    check("its header", c(peek), [" 16 8"])
    check("FLASH LIST shows the image", c("Flash List"), ["Slot 1 in use: image 16 x 8"])
    c("New")
    check("its header after NEW", c(peek), [" 16 8"])
    check("FLASH RUN of an image", c("Flash Run 1"), ["Flash slot empty"])
    c("Flash Erase 3")
    check("an empty flash slot reads FF", c("Print Hex$(Peek(Word MM.Info(Flash Address 3)))"), ["FFFFFFFF"])

    # DISK LOAD: a file's bytes
    c('Flash Disk Load 3, "fsa.bas"')
    check("FLASH DISK LOAD 3", c("Print Peek(Byte MM.Info(Flash Address 3))"), [" 39"])
    check("FLASH DISK LOAD 3 again", c('Flash Disk Load 3, "fsa.bas"'), ["Already programmed"])

    # a program saved by another firmware's format
    c('Run "fsa"')
    c("Flash Overwrite 1")
    check("fsold", c('Run "fsold"'), ["old.slot made"])
    c('Flash Disk Load 3, "old.slot", O')
    check("FLASH LIST: another version's", c("Flash List"), ["saved by another firmware version"])
    check("FLASH RUN refuses it", c("Flash Run 3"), ["from another firmware version"])

    # the folder is hidden; the slots are kept through a restart
    check("FILES does not show .flash", c('Files "A:/.fl*"', 15), [], [".flash"])
    b.send_line("Cpu Restart")
    time.sleep(4)
    b.drain(1.0)
    b.attention()
    check("the slots after a restart", c("Flash List"), ['Slot 1 in use: "', "Slot 2 in use", "saved by another firmware version"])
    check("FLASH RUN 1 after a restart", c("Flash Run 1"), ["A runs in slot 1"])

    c("Flash Erase All")
    check("FLASH ERASE ALL", c("Flash List"), ["Slot 1 available", "Slot 2 available", "Slot 3 available"])
    c('Chdir "A:/"')
    b.close()
    print("%d of %d checks passed" % (passed, passed + failed))
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
