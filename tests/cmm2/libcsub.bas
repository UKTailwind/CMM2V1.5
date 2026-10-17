' libcsub.bas - a RAM library's CSUBs run from the executable window at the top of the heap (GetExecMemory, MPU region 15),
' and its font is used: csubm7's CSUBs and their interpreted twins must agree (bad 0)
LIBRARY LOAD "libcsub_lib.inc", RAM
Option Explicit
Dim integer k, bad = 0
Dim float x, r1, r2
Dim integer n, w, h
Dim float xs(5) = (0.5, -2.25, 3.75, 100.125, -0.001, 12345.678)
Dim integer ns(5) = (7, -1000003, 3000000000, 0, 42, -9876543210)
For k = 0 To 5
  x = xs(k) : n = ns(k)
  r1 = Mix(x, n) : r2 = MixB(x, n)
  Print "Mix"; k; " "; Str$(r1, 0, 6); " "; Str$(r2, 0, 6); " "; Choice(r1 = r2, "OK", "DIFFERENT")
  If r1 <> r2 Then bad = bad + 1
Next
Loop1 20000, r1 : Loop1B 20000, r2
Print "Loop1 "; Str$(r1, 0, 9); " "; Str$(r2, 0, 9); " "; Choice(r1 = r2, "OK", "DIFFERENT")
If r1 <> r2 Then bad = bad + 1
Print "bad"; bad
Font 9 : w = MM.Info(FONTWIDTH) : h = MM.Info(FONTHEIGHT) : Font 1
Print "font 9:"; w; h
Print "bad"; bad
