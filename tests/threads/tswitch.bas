' tswitch.bas - what a thread switch costs, in microseconds (the empty loop's time taken off). A timing program of
' tools/threadcheck.py (tests/threads/quick.txt): no .expected, its RESULT lines are compared with the build before.
' The second figure is the main program's THREAD YIELD, a switch to the thread, its LOOP and THREAD YIELD, and a
' switch back: two switches and three statements.
Option Default None
Option Explicit
Dim Integer i, n = 20000
Dim Float t0, tl

Sub T(s$, per As Float)
  Print "RESULT "; s$; Str$(per, 0, 2); " us"
End Sub

Sub Other
  Do
    Thread Yield
  Loop
End Sub

t0 = Timer : For i = 1 To n : Next : tl = Timer - t0
t0 = Timer : For i = 1 To n : Thread Yield : Next
T "Thread Yield with no thread   ", (Timer - t0 - tl) * 1000 / n
Thread Start 1, Other
t0 = Timer : For i = 1 To n : Thread Yield : Next
T "Yield to a thread and back    ", (Timer - t0 - tl) * 1000 / n
