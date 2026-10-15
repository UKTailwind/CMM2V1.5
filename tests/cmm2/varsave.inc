' varsave.inc - run by varsave.bas: RUN has cleared every variable, and VAR RESTORE brings back what was saved.
' Arrays must be declared first; a scalar is made by VAR RESTORE (without OPTION EXPLICIT).
Dim integer tests, pass
Dim integer ia(3), one(0)
Dim float fa(1, 2)
Var Restore
Check "after RUN: a float", f, 1.5
Check "an integer", n, 43
CheckS "a string", s$, "kept"
Check "an integer array", ia(3), 7
Check "a single element array", one(0), 9
Check "a float array", fa(1, 2), 2.25

n = 5
Var Clear
Var Restore
Check "VAR CLEAR leaves nothing to restore", n, 5

Print "varsave.inc:"; pass; " of"; tests; " passed"
End

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub CheckS(n$, got$, want$)
  tests = tests + 1
  If got$ = want$ Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got "; got$; " want "; want$
  EndIf
End Sub
