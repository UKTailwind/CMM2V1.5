' onedim1.bas - P1 step 5a: single element dimensions under OPTION BASE 1.  DIM a(1) was "Dimensions"
' before; its bound needs no reserved value.  MATH SLICE/INSERT counted only the dimensions with two
' or more elements, and MATH CHI took df as the product of the upper bounds (4 for a 2x2 here).
Option Base 1
Dim tests%, pass%, x, i%
Dim a(1), b$(1) Length 5, m(3,1), s(3), t(2,2)

a(1) = 2.5 : b$(1) = "hello"
Check "a(1)", a(1), 2.5
CheckS "b$(1) LENGTH 5 in its record", b$(1), "hello"
Check "BOUND(a())", Bound(a()), 1
Check "BOUND(a(),0) is OPTION BASE", Bound(a(),0), 1
Check "BOUND(m(),2)", Bound(m(),2), 1
On Error Skip 1
x = a(2)
Check "a(2) is out of bounds", MM.ErrNo <> 0, 1
On Error Skip 1
x = a(0)
Check "a(0) is below OPTION BASE", MM.ErrNo <> 0, 1
On Error Skip 1
Dim w(0)
Check "DIM w(0) is still an error", Instr(MM.ErrMsg$, "Dimensions") > 0, 1

m(1,1) = 1 : m(2,1) = 2 : m(3,1) = 3
Math Slice m(), , 1, s()
Check "MATH SLICE m(), , 1 counts both dimensions", s(1) * 100 + s(2) * 10 + s(3), 123

t(1,1) = 20 : t(2,1) = 30 : t(1,2) = 30 : t(2,2) = 20
Check "MATH(CHI) of a 2x2", Math(Chi t()), 4
Check "MATH(CHI_P) of a 2x2 is df 1, as under OPTION BASE 0", Abs(Math(Chi_p t()) - 5) < 1e-9, 1

Print "onedim1:"; pass%; " of"; tests%; " passed"
End

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
