' varsave.bas - VAR SAVE, VAR RESTORE and VAR CLEAR (kept in the backup SRAM).  VAR was missing from the command
' table (its entry was in Flash.h, which the table never included), so every VAR command was "Unknown command".
' This saves, changes and restores in one run, then runs varsave.inc, whose fresh run restores what was saved.
Option Explicit
Dim integer tests, pass
Dim float f = 1.5
Dim integer n = 42
Dim string s$ = "kept", e$ = ""
Dim integer ia(3), one(0)
Dim float fa(1, 2)
Type TPoint
  x As INTEGER
End Type
Dim p As TPoint

ia(3) = 7 : one(0) = 9 : fa(1, 2) = 2.25
Var Clear
Var Save f, n, s$, e$, ia(), one(), fa()
f = 0 : n = 0 : s$ = "lost" : e$ = "x" : ia(3) = 0 : one(0) = 0 : fa(1, 2) = 0
Var Restore
Check "a float", f, 1.5
Check "an integer", n, 42
CheckS "a string", s$, "kept"
CheckS "an empty string", e$, ""
Check "an integer array", ia(3), 7
Check "a single element array", one(0), 9
Check "a float array", fa(1, 2), 2.25

n = 43
Var Save n
n = 0 : f = 0
Var Restore
Check "VAR SAVE again replaces its entry", n, 43
Check "and keeps the others", f, 1.5

On Error Skip 1
Var Save p
Check "a structure is refused", Instr(MM.ErrMsg$, "structure") > 0, 1
On Error Skip 1
Var Save nosuch
Check "an unknown variable is an error", MM.ErrNo <> 0, 1

Print "varsave:"; pass; " of"; tests; " passed"
Run "varsave.inc"

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
