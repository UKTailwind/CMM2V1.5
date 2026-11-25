' tmainend.bas - threads, Stage 1: the main program runs off its end while a thread is still running: the program
' ends and the thread with it.
Option Explicit
Dim Integer n
Sub Forever
  Do
    Inc n
    Thread Yield
  Loop
End Sub
Thread Start 1, Forever
Thread Yield
Thread Yield
Print "the main program ends with thread 1 still running; it ran"; n; " times"
