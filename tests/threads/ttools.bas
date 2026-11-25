' ttools.bas - threads, Stage 5: TRACE shows which thread ran a line ([#n:line]); THREAD LIST shows each thread's line,
' state and stack; MEMORY has a line for the threads. Not in the quick check: the stack's depths differ between the
' compile modes, so it is read, not compared.
Option Explicit
Dim Integer go = 1

Sub Waiter
  Do While go
    Pause 50
  Loop
End Sub

Sub Once
  Thread Yield
End Sub

Thread Start 3 Size 16 Locals 8, Waiter
Pause 20
Thread List
Memory
Thread Slice 100
Trace On
Thread Start 1, Once                    ' (after TRACE ON: its lines are traced, [#1:...])
Thread Yield
Trace Off
Print
go = 0
Thread Wait 3
Print "ttools: done"
