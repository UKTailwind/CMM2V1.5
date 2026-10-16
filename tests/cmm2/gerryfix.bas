' gerryfix.bas - two of the fixes taken from Gerry Allardice's 6.00.00b14 (docs/Gerry_b14_Review_and_Merge_Plan.md 3.1)
' that a program can see: an interrupt SUB starts with a clean error state and leaves the caller's as it was,
' and PIXEL(x, y) in a 32-bit mode returns the pixel (it returned a stale value).
Option Explicit
Dim integer fired = 0, bad = 0, inmsg = -1, inerr = -1
Dim string insub$ = "?"
Dim integer x

' 1. the error state across an interrupt: the main program has an error pending under ON ERROR SKIP
On Error Skip 1
x = 1 \ 0
Dim string m$ = MM.ErrMsg$, after$
Dim integer e = MM.ErrNo
Print "main error: "; e; " "; Mid$(m$, Instr(m$, ":") + 2)
SetTick 20, isr, 1
Do While fired = 0 : Loop
SetTick 0, 0, 1
Print "in the interrupt: errno"; inerr; " msg ["; insub$; "]"; Choice(inerr = 0 And insub$ = "", " clean", " NOT CLEAN")
If inerr <> 0 Or insub$ <> "" Then bad = bad + 1
after$ = MM.ErrMsg$
Print "after it: errno"; MM.ErrNo; Choice(MM.ErrNo = e And after$ = m$, " restored", " NOT RESTORED")
If MM.ErrNo <> e Or after$ <> m$ Then bad = bad + 1

' 2. PIXEL() in a 32-bit mode
Mode 8, 32
Pixel 300, 300, RGB(12, 34, 56)                ' (clear of the text at the top, which PRINT draws over)
Pixel 301, 300, RGB(200, 100, 50)
Print "pixel 32: "; Hex$(Pixel(300, 300)); " "; Hex$(Pixel(301, 300))
If Pixel(300, 300) <> (RGB(12, 34, 56) And &HFFFFFF) Or Pixel(301, 300) <> (RGB(200, 100, 50) And &HFFFFFF) Then bad = bad + 1   ' (RGB() carries the alpha bits)
Mode 8, 8
Print "bad"; bad
End

Sub isr
  inerr = MM.ErrNo
  insub$ = MM.ErrMsg$
  fired = 1
End Sub
