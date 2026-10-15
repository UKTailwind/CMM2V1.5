' globals7.bas - item 7 part 2: the search of the global slots at the top level had no
' bound.  1023 globals is the most there can be (one of the 1024 slots stays empty; 511 of 512
' before the compact variable table).  ERASE of a variable whose
' next slot is in use leaves a marker; a new variable can then take the last empty slot, and a
' search for any missing name went round the table for ever, with no Ctrl-C.  Now it is an error.
' Before the fix this HANGS (by a simulation of the CMM2's hash, at k% = 3); run it after.
Option Explicit
Dim n%, k%, e
Do While MM.Info(VARCNT) < 1023
  n% = n% + 1
  Execute "Dim V" + Str$(n%)
Loop
On Error Ignore
For k% = 1 To 20
  Execute "Erase V" + Str$(k%)
  Execute "Dim W" + Str$(k%)
  e = NoSuchVar
Next
On Error Abort
If Instr(MM.ErrMsg$, "Too many global variables") Then
  Print "ok  globals7: a full global table is an error, not a hang"
Else
  Print "FAIL globals7 got "; MM.ErrMsg$; " want Too many global variables"
EndIf
