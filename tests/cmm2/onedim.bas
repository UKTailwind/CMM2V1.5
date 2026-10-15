' onedim.bas - P1 step 5a: single element dimensions (the PicoMite's DIM_ONE).  Under OPTION BASE 0
' an upper bound of 0 is a one element dimension; DIM a(0) was "Dimensions" before.  The bound is
' stored as a reserved value and decoded on every read.  onedim1.bas does the same under OPTION BASE 1.
Option Base 0
Dim tests%, pass%, x, i%, ok%
Dim a(0), b%(0), c$(0), d$(0) Length 5, e$(0,0) Length 11, f$(0) Length 12
Dim m(2,0), n(0,2), o(0,0), s(2), v(0), t(1,1)

' ---- every type, a heap string and the strings kept inside the record -----------------------
a(0) = 1.5 : b%(0) = 7 : c$(0) = "a heap string of more than sixteen characters"
d$(0) = "abcde" : e$(0,0) = "eleven char" : f$(0) = "twelve chars"
Check "a float a(0)", a(0), 1.5
Check "an integer b%(0)", b%(0), 7
CheckS "a string c$(0) in the heap", c$(0), "a heap string of more than sixteen characters"
CheckS "d$(0) LENGTH 5 in its record", d$(0), "abcde"
CheckS "e$(0,0) LENGTH 11 in its record", e$(0,0), "eleven char"
CheckS "f$(0) LENGTH 12 in the heap", f$(0), "twelve chars"
CheckS "and d$(0) is untouched", d$(0), "abcde"

' ---- BOUND reads the decoded bound, and is an error past the rank ---------------------------
Check "BOUND(a())", Bound(a()), 0
Check "BOUND(m(),1)", Bound(m(),1), 2
Check "BOUND(m(),2)", Bound(m(),2), 0
Check "BOUND(n(),1)", Bound(n(),1), 0
Check "BOUND(n(),2)", Bound(n(),2), 2
Check "BOUND(o(),2)", Bound(o(),2), 0
Check "BOUND(a(),0) is OPTION BASE", Bound(a(),0), 0
On Error Skip 1
x = Bound(a(),2)
Check "BOUND(a(),2) past the rank is an error", Instr(MM.ErrMsg$, "Dimensions") > 0, 1
On Error Skip 1
x = Bound(x)
Check "BOUND of a scalar is an error", Instr(MM.ErrMsg$, "Expected an array") > 0, 1

' ---- the bounds are checked as for any array ------------------------------------------------
On Error Skip 1
x = a(1)
Check "a(1) is out of bounds", MM.ErrNo <> 0, 1
On Error Skip 1
x = m(0,1)
Check "m(0,1) is out of bounds", MM.ErrNo <> 0, 1
On Error Skip 1
x = n(1,0)
Check "n(1,0) is out of bounds", MM.ErrNo <> 0, 1
On Error Skip 1
Dim w(-1)
Check "DIM w(-1) is still an error", Instr(MM.ErrMsg$, "Dimensions") > 0, 1

' ---- passed to a SUB, and as LOCAL ----------------------------------------------------------
P a()
Check "a() passed by reference", a(0), 2.5
L
L

' ---- ERASE and DIM again with a computed bound that lands on one element --------------------
ok% = 1
For i% = 3 To 1 Step -1
  Erase v
  Dim v(i% - 1)
  v(i% - 1) = i% * 100
  If Bound(v()) <> i% - 1 Or v(i% - 1) <> i% * 100 Then ok% = 0
Next
Check "DIM v(n-1) for n = 3, 2, 1", ok%, 1

' ---- SORT of one element --------------------------------------------------------------------
Sort b%()
Check "SORT b%()", b%(0), 7

' ---- MATH: a row and a column vector, a 1x1 result, SLICE and INSERT ------------------------
m(0,0) = 1 : m(1,0) = 2 : m(2,0) = 3
n(0,0) = 4 : n(0,1) = 5 : n(0,2) = 6
Check "MATH(SUM m())", Math(Sum m()), 6
Math M_Mult m(), n(), o()
Check "MATH M_MULT (1x3).(3x1) into o(0,0)", o(0,0), 32
Math Slice m(), , 0, s()
Check "MATH SLICE m(), , 0 counts both dimensions", s(0) * 100 + s(1) * 10 + s(2), 123
s(0) = 7 : s(1) = 8 : s(2) = 9
Math Insert m(), , 0, s()
Check "MATH INSERT m(), , 0", m(0,0) * 100 + m(1,0) * 10 + m(2,0), 789
Math Slice n(), 0, , s()
Check "MATH SLICE n(), 0, , s()", s(0) * 100 + s(1) * 10 + s(2), 456

' ---- MATH CHI: df is (columns-1)*(rows-1), and a single row has none -------------------------
t(0,0) = 20 : t(1,0) = 30 : t(0,1) = 30 : t(1,1) = 20
Check "MATH(CHI) of a 2x2", Math(Chi t()), 4
Check "MATH(CHI_P) of a 2x2 (df 1)", Abs(Math(Chi_p t()) - 5) < 1e-9, 1
On Error Skip 1
x = Math(Chi m())
Check "MATH(CHI) of a single row is an error", Instr(MM.ErrMsg$, "2 rows and 2 columns") > 0, 1

Print "onedim:"; pass%; " of"; tests%; " passed"
End

Sub P(z())
  Check "BOUND of the parameter", Bound(z()), 0
  z(0) = z(0) + 1
End Sub

Sub L
  Local q%(0), r$(0,0) Length 3
  Check "LOCAL q%(0) starts at zero", q%(0), 0
  q%(0) = 42 : r$(0,0) = "xyz"
  Check "LOCAL q%(0)", q%(0), 42
  CheckS "LOCAL r$(0,0) LENGTH 3", r$(0,0), "xyz"
End Sub

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
