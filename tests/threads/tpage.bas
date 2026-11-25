' tpage.bas - threads, Stage 4: each thread's write and read pages. A thread draws into the framebuffer while the main
' program draws on the display; the framebuffer cannot be closed while a thread writes to it; MODE gives every thread
' the new mode's pages, font and colours.
Option Explicit
Dim Integer tests, pass, phase, r1, r2, w0, wt, f1, c1, w1, fm, cm, wm

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub Drawer
  FrameBuffer Write F
  Pixel 10, 10, RGB(red)
  phase = 1
  Do While phase < 2 : Pause 2 : Loop
  r1 = Pixel(10, 10)                    ' its own read page: the framebuffer
  wt = MM.Info(Write Page)
  Font 3 : Colour RGB(cyan)
  phase = 3
  Do While phase < 4 : Pause 2 : Loop
  f1 = MM.Info(Font) : c1 = MM.Info(FColour) : w1 = MM.Info(Write Page)
  phase = 5
End Sub

CLS
FrameBuffer Create
Thread Start 1, Drawer
Do While phase < 1 : Pause 2 : Loop
w0 = MM.Info(Write Page)
Pixel 10, 10, RGB(green)                ' on the display, the main program's page
r2 = Pixel(10, 10)
phase = 2
Do While phase < 3 : Pause 2 : Loop
Check "the thread writes to another page than the main program", w0 <> wt, 1
Check "and its PIXEL drew there and read back from there", r1 <> r2 And r1 <> 0, 1
On Error Skip
FrameBuffer Close
Check "the framebuffer a thread writes to cannot be closed", MM.ErrMsg$ = "Framebuffer is set for write", 1
Mode 1, 8
fm = MM.Info(Font) : cm = MM.Info(FColour) : wm = MM.Info(Write Page)
phase = 4
Do While phase < 5 : Pause 2 : Loop
Check "after MODE the thread's font is the mode's", f1, fm
Check "and its colour", c1, cm
Check "and its write page", w1, wm
On Error Skip
FrameBuffer Close
Check "and the framebuffer can then be closed", MM.Errno, 0
CLS
Print "tpage:"; pass; " of"; tests; " passed"
