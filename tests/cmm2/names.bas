' names.bas - P1 step 4: one index of SUBs, FUNCTIONs and labels by name replaces funtbl.
' Labels and SUBs by name from every place that names them, a variable named like a SUB, and at
' the end a generated program with 600 labels and 300 SUBs (the old table held 511 names in all).
Dim tests%, pass%, r%, i%, s$, t%
Dim d%(3)

' ---- labels --------------------------------------------------------------------------------
r% = 0 : GoSub Lab1 : Check "GOSUB a label", r%, 1
GoTo Lab2
Print "FAIL GOTO did not jump"
Lab2:
Check "GOTO a label", 1, 1
Restore Data2 : Read i% : Check "RESTORE a label", i%, 22
For i% = 1 To 3
  On i% GoSub OnA, OnB, OnC
Next
CheckS "ON GOSUB to three labels", s$, "ABC"
s$ = "" : i% = 2 : On i% GoTo OnG1, OnG2
OnBack:
CheckS "ON GOTO to a label", s$, "G2"

' ---- SUBs and FUNCTIONs --------------------------------------------------------------------
r% = 0 : Bump 5 : Check "call a SUB", r%, 5
Check "call a FUNCTION", Twice(21), 42
Call "Bump", 3 : Check "CALL a SUB by name", r%, 8
Call "bump", 2 : Check "CALL a SUB by a name in lower case", r%, 10
Check "CALL( a FUNCTION", Call("Twice", 4), 8
Execute "Bump 1" : Check "EXECUTE a SUB call", r%, 11
On Error Skip 1
Dim Bump = 1
Check "a variable named like a SUB is an error", MM.ErrNo <> 0, 1
CheckS "and says so", Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2), "A sub/fun has the same name: BUMP"

' ---- SETTICK to a SUB and to a label -------------------------------------------------------
t% = 0 : SetTick 10, TickSub, 1
Timer = 0 : Do : Loop Until t% Or Timer > 500
SetTick 0, TickSub, 1
Check "SETTICK to a SUB", t%, 1
t% = 0 : SetTick 10, TickLab, 2
Timer = 0 : Do : Loop Until t% Or Timer > 500
SetTick 0, TickLab, 2
Check "SETTICK to a label", t%, 1

' ---- many names: write a program with 600 labels and 300 SUBs, and run it -------------------
Open "N600.BAS" For Output As #1
Print #1, "Dim n%, k%"
For i% = 1 To 600 : Print #1, "GoSub L" + Str$(i%) : Next
For i% = 1 To 300 : Print #1, "S" + Str$(i%) : Next
Print #1, "Print " + Chr$(34) + "names: 600 labels and 300 SUBs reached," + Chr$(34) + "; n%; k%"
Print #1, "End"
For i% = 1 To 600 : Print #1, "L" + Str$(i%) + ":" : Print #1, "n% = n% + 1 : Return" : Next
For i% = 1 To 300 : Print #1, "Sub S" + Str$(i%) : Print #1, "  k% = k% + 1" : Print #1, "End Sub" : Next
Close #1

Print "names:"; pass%; " of"; tests%; " passed"
Run "N600.BAS"

Lab1:
r% = 1
Return

Data2:
Data 22

OnA: s$ = s$ + "A" : Return
OnB: s$ = s$ + "B" : Return
OnC: s$ = s$ + "C" : Return
OnG1: s$ = "G1" : GoTo OnBack
OnG2: s$ = "G2" : GoTo OnBack

TickLab:
t% = 1
IReturn

Sub TickSub
  t% = 1
End Sub

Sub Bump n%
  r% = r% + n%
End Sub

Function Twice(v)
  Twice = v * 2
End Function

Sub Check(n$, got, want)
  tests% = tests% + 1
  If got = want Then
    pass% = pass% + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub CheckS(n$, got$, want$)
  tests% = tests% + 1
  If got$ = want$ Then
    pass% = pass% + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got "; got$; " want "; want$
  EndIf
End Sub
