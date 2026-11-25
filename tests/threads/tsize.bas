' tsize.bas - threads, Stage 4b: THREAD START n SIZE kb LOCALS m, for one thread; THREAD SIZE's otherwise. A small
' thread takes less memory than one of THREAD SIZE's (MM.INFO(HEAP) counts the on-chip heap too); LOCALS m allows m
' locals; SIZE gives a deep recursion room; the two come in either order; out of range is refused.
Option Explicit
Dim Integer tests, pass, h0, h1, h2, used1, used2, deep, made, go(9)
Dim msg$

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub Idle                                ' (go(n) is set before thread n starts: set here, it could come after the
  Local Integer me = MM.Info(Thread)     '  main program had cleared it, and the thread would never end)
  Do While go(me) : Pause 5 : Loop
End Sub

Sub Many                                ' nine locals in a thread with eight
  Local Integer a1, a2, a3, a4, a5, a6, a7, a8
  made = 8
  On Error Skip
  Local Integer a9
  msg$ = MM.ErrMsg$
End Sub

Function Down%(n%)
  Local Integer k = n%
  If n% > 0 Then Down% = Down%(n% - 1) + 1 Else Down% = 0
End Function

Sub GoDeep
  deep = Down%(14)
End Sub

go(9) = 1 : Thread Start 9, Idle                    ' (the main program's record made first, and kept while a thread exists)
h0 = MM.Info(Heap)
go(1) = 1 : Thread Start 1, Idle
h1 = MM.Info(Heap)
used1 = h0 - h1
go(1) = 0 : Thread Wait 1
h1 = MM.Info(Heap)
go(1) = 1 : Thread Start 1 Size 12 Locals 8, Idle
h2 = MM.Info(Heap)
used2 = h1 - h2
go(1) = 0 : Thread Wait 1 : go(9) = 0 : Thread Wait 9
Print "  a thread of THREAD SIZE's (20 KB, 48 locals):"; used1; " bytes; SIZE 12 LOCALS 8:"; used2
Check "SIZE 12 LOCALS 8 takes at least 8 KB less", used2 <= used1 - 8192, 1

Thread Start 2 Locals 8, Many
Thread Wait 2
Check "LOCALS 8: eight locals made", made, 8
Check "and a ninth refused", msg$ = "Not enough Local variable memory", 1

Thread Start 3 Locals 64 Size 48, GoDeep          ' (three locals a level: the parameter, k and the result)
Thread Wait 3
Check "SIZE 48 (after LOCALS): a FUNCTION fourteen deep", deep, 14

On Error Skip
Thread Start 4 Size 8, Idle
Check "SIZE 8 is refused", Instr(MM.ErrMsg$, "valid is 12 to 512") > 0, 1
On Error Skip
Thread Start 4 Size 12 Size 12, Idle
Check "SIZE twice is refused", MM.Errno <> 0 And MM.Info(Thread 4) = 0, 1
Print "tsize:"; pass; " of"; tests; " passed"
