' pauseint.bas - a PAUSE that an interrupt cut short and that was never carried on leaves nothing behind (Gerry, 27 Oct
' 2026: after an error in a SETTICK interrupt, the next run's first PAUSE ended at once, so its interrupt was not
' called). Here the interrupt starts the program again (RUN), as the error's return to the prompt and a RUN did: the
' PAUSE it cut short is never finished.
Option Explicit
Option Default Integer
Dim called, t
If MM.CmdLine$ = "" Then
  SetTick 10, Restart
  Pause 1000
  Print "FAIL the PAUSE was carried on"
  End
EndIf
Print "run again: "; MM.CmdLine$
t = Timer
SetTick 10, Tick
Pause 100
t = Timer - t
Print "PAUSE 100 after the one cut short:"; Choice(Abs(t - 100) <= 5, " 100 ms", Str$(t) + " ms")
Print "its SETTICK interrupt called:"; called
End

Sub Restart
  SetTick 0, Restart
  Run "pauseint", second
End Sub

Sub Tick
  SetTick 0, Tick
  called = 1
End Sub
