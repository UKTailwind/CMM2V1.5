' csubint.bas - INTERRUPT sub (M9 of the b14 merge, the PicoMite's): the SUB runs when the CSUB-completion flag is set.
' A CSUB sets the flag through its call table; INTERRUPT with no argument sets it from BASIC, which is what is tested
' here, with INTERRUPT 0 forgetting the SUB.
Option Explicit
Dim integer fired = 0, i
Interrupt done
Print "set: fired"; fired
Interrupt
For i = 1 To 20 : If fired Then Exit For
Next
Print "raised: fired"; fired; " after"; i; " statements"
Interrupt
Pause 10
Print "again: fired"; fired
Interrupt 0
Interrupt
Pause 10
Print "forgotten: fired"; fired
Interrupt done
Interrupt
Pause 10
Print "set again: fired"; fired
Print "done"                               ' (the SUB is left set: csubint2.bas, run next, raises the flag)
End

Sub done
  fired = fired + 1
End Sub
