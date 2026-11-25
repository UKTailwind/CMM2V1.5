' tend.bas - threads, Stage 1: END in a thread ends the program, SUB MM.END running first, in that thread.
Option Explicit
Sub MM.End
  Print "MM.END runs in thread"; MM.Info(Thread)
End Sub
Sub Ender
  Print "thread"; MM.Info(Thread); " ends the program"
  End
  Print "not reached"
End Sub
Thread Start 2, Ender
Do
  Thread Yield
  Print "not reached: the main program"
Loop
