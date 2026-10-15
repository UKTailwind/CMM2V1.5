' static.bas - STATIC's hidden global: the SUB's name, 0x1E, then the variable (STATIC C in SUB AB and STATIC BC
' in SUB A shared the global ABC); a long declaration (it was copied into 65 bytes unchecked); STATIC with LENGTH
' and an initial value; an AS type in a STATIC list, on the first call and on later ones.
Option Explicit
Dim integer tests, pass, lastc, lastbc, n
Dim float lastb
Dim s$

AB : AB : A : A
Check "STATIC C in SUB AB counts its own calls", lastc, 2
Check "and STATIC BC in SUB A its own (they shared ABC)", lastbc, 2

LongDecl : LongDecl
Check "a long STATIC declaration keeps its value between calls", n, 2

For n = 1 To 3 : s$ = WithLength$() : Next
CheckS "STATIC s$ LENGTH 10 = an initial value, kept between calls", s$, "abc+++"

AsList : AsList
CheckF "an AS type in a STATIC list does not carry to the next name (second call)", lastb, 1.5

Print "static:"; pass; " of"; tests; " passed"
End

Sub AB
  Static C
  C = C + 1 : lastc = C
End Sub

Sub A
  Static BC
  BC = BC + 1 : lastbc = BC
End Sub

Sub LongDecl
  Static integer counts(1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1)
  counts(40) = counts(40) + 1 : n = counts(40)
End Sub

Function WithLength$()
  Static t$ Length 10 = "abc"
  t$ = t$ + "+"
  WithLength$ = t$
End Function

Sub AsList
  Static q As integer, b
  q = q + 1
  b = 1.5 : lastb = b
End Sub

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub CheckF(n$, got, want)
  tests = tests + 1
  If Abs(got - want) < 1e-9 Then
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
