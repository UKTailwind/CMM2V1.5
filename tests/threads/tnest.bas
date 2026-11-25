' tnest.bas - threads: each thread has its own FOR, DO, call and temporary-memory lists, as large as its NEST gives
' (THREAD START n NEST d, or THREAD SIZE kb, locals, nest; 20 otherwise). A thread of NEST 4 runs four FOR loops, four DO
' loops and four calls deep below its own SUB, and no deeper; the default goes twenty deep; the main program's limits are the ones it
' always had while threads run; a small NEST takes less memory; out of range is refused. Threads of different NEST
' run side by side, each looping and calling in its own lists.
Option Explicit
Dim Integer tests, pass, h0, h1, used1, used2, go(9)
Dim Integer reach(9), sum(9), dsum(9)
Dim msg$(9)

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub Idle
  Local Integer me = MM.Info(Thread)
  Do While go(me) : Pause 5 : Loop
End Sub

Sub Down(n%)                            ' calls itself until it is refused: how deep it went, and why it stopped
  Local Integer me = MM.Info(Thread)
  reach(me) = n%
  On Error Skip
  Down n% + 1
  If MM.Errno <> 0 And msg$(me) = "" Then msg$(me) = MM.ErrMsg$
  On Error Clear
End Sub

Sub Deep
  Down 1
End Sub

Sub Loops4                              ' four FOR loops and four DO loops deep, switching in the innermost
  Local Integer me = MM.Info(Thread), a, b, c, d, s
  For a = 1 To 3
    For b = 1 To 3
      For c = 1 To 3
        For d = 1 To 3
          s = s + a * b * c * d
          Thread Yield
        Next d
      Next c
    Next b
  Next a
  sum(me) = s
  a = 0 : s = 0
  Do
    a = a + 1 : b = 0
    Do
      b = b + 1 : c = 0
      Do
        c = c + 1 : d = 0
        Do
          d = d + 1
          s = s + 1
          Thread Yield
        Loop Until d = 2
      Loop Until c = 2
    Loop Until b = 2
  Loop Until a = 2
  dsum(me) = s
End Sub

Function Join$(n%)                      ' temporary memory at each level of a call
  If n% = 0 Then Join$ = "" Else Join$ = Str$(n%) + "," + Join$(n% - 1)
End Function

Sub Temps
  Local Integer me = MM.Info(Thread), i
  Local s$
  For i = 1 To 20
    s$ = Join$(3) + Left$("abcdef", 3) + Mid$(Str$(i * 1000), 1, 2) + UCase$("x")
    Thread Yield
  Next i
  msg$(me) = s$
End Sub

' the memory a thread takes: the stack is the same, its lists are not
go(9) = 1 : Thread Start 9, Idle
h0 = MM.Info(Heap)
go(1) = 1 : Thread Start 1 Size 12 Locals 8, Idle
used1 = h0 - MM.Info(Heap)
go(1) = 0 : Thread Wait 1
h1 = MM.Info(Heap)
go(1) = 1 : Thread Start 1 Size 12 Locals 8 Nest 4, Idle
used2 = h1 - MM.Info(Heap)
go(1) = 0 : Thread Wait 1 : go(9) = 0 : Thread Wait 9
Print "  SIZE 12 LOCALS 8:"; used1; " bytes; with NEST 4:"; used2
Check "NEST 4 takes at least 2 KB less than NEST 20", used2 <= used1 - 2048, 1

' how deep each may call
Thread Start 1 Nest 4, Deep
Thread Start 2, Deep
Thread Start 3 Nest 30 Size 64 Locals 80, Deep
Thread Wait 1 : Thread Wait 2 : Thread Wait 3
Print "  calls deep: NEST 4"; reach(1); ", the default"; reach(2); ", NEST 30"; reach(3)
Check "NEST 4: four calls deep", reach(1), 4
Check "the default: twenty", reach(2), 20
Check "NEST 30: thirty", reach(3), 30
Check "and the next is refused", msg$(1) = "Too many nested SUB/FUN" And msg$(2) = msg$(1) And msg$(3) = msg$(1), 1

' loops in their own lists, side by side with the main program's
Thread Start 1 Nest 4, Loops4
Thread Start 2, Loops4
Thread Start 3 Nest 8, Loops4
Loops4
Thread Wait 1 : Thread Wait 2 : Thread Wait 3
Check "four FOR loops deep in NEST 4, the default, NEST 8 and the main program", sum(1) = 1296 And sum(2) = 1296 And sum(3) = 1296 And sum(0) = 1296, 1
Check "and four DO loops", dsum(1) = 16 And dsum(2) = 16 And dsum(3) = 16 And dsum(0) = 16, 1

' temporary memory
msg$(1) = "" : msg$(2) = ""
Thread Start 1 Nest 4, Temps
Thread Start 2, Temps
Thread Wait 1 : Thread Wait 2
Check "strings in a NEST 4 thread", msg$(1) = "3,2,1,abc20X", 1
Check "and in a default one", msg$(2) = "3,2,1,abc20X", 1

' the main program's own limits are the ones it always had
msg$(0) = "" : reach(0) = 0
go(9) = 1 : Thread Start 9, Idle
Deep
go(9) = 0 : Thread Wait 9
Check "the main program still calls a hundred deep beside a thread (Deep and 99)", reach(0), 99

' THREAD SIZE's third value, and the limits
Thread Size 20, , 6
msg$(1) = "" : reach(1) = 0
Thread Start 1, Deep
Thread Wait 1
Check "THREAD SIZE 20, , 6: six calls deep", reach(1), 6
Thread Size 20, 48, 20
On Error Skip
Thread Start 4 Nest 3, Idle
Check "NEST 3 is refused", Instr(MM.ErrMsg$, "valid is 4 to 100") > 0, 1
On Error Skip
Thread Start 4 Nest 4 Nest 4, Idle
Check "NEST twice is refused", MM.Errno <> 0 And MM.Info(Thread 4) = 0, 1
Print "tnest:"; pass; " of"; tests; " passed"

' a fifth FOR loop in a thread of NEST 4 stops the program
Sub Five
  Local Integer a, b, c, d, e
  For a = 1 To 2
    For b = 1 To 2
      For c = 1 To 2
        For d = 1 To 2
          For e = 1 To 2
          Next e
        Next d
      Next c
    Next b
  Next a
End Sub
Thread Start 5 Nest 4, Five
Do : Thread Yield : Loop
