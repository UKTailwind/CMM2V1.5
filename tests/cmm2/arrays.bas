' arrays.bas - ARRAY SET/ADD/INSERT/SLICE (the PicoMite's names for MATH's), whole-array assignment a() = b(),
' graphics commands given a structure member of every element (boxes().x), and a command that cannot take one.
Option Explicit
Option Base 0
Dim integer tests, pass, i
Dim float a(4), b(4), m(2,1), s(2)
Dim integer ia(5), ib(2,1)
Dim string sa$(2) Length 10, sb$(2) Length 10, sc$(2) Length 5
Type TBox
  x As INTEGER
  y As INTEGER
  w As INTEGER
  h As INTEGER
End Type
Dim boxes(1) As TBox = (10, 20, 30, 40, 100, 120, 20, 10)
Dim dup(1) As TBox

' ---- ARRAY SET, ADD, INSERT, SLICE --------------------------------------------------------------
Array Set 2.5, a()
Check "ARRAY SET", a(0) + a(4), 5
Array Add a(), 1, b()
Check "ARRAY ADD", b(3), 3.5
m(0,0) = 1 : m(1,0) = 2 : m(2,0) = 3 : m(0,1) = 4 : m(1,1) = 5 : m(2,1) = 6
Array Slice m(), , 1, s()
Check "ARRAY SLICE", s(0) * 100 + s(1) * 10 + s(2), 456
s(0) = 7 : s(1) = 8 : s(2) = 9
Array Insert m(), , 0, s()
Check "ARRAY INSERT", m(0,0) * 100 + m(1,0) * 10 + m(2,0), 789

' ---- a() = b() --------------------------------------------------------------------------------
For i = 0 To 4 : a(i) = i * 1.5 : Next
b() = a()
Check "b() = a()", b(4), 6
For i = 0 To 5 : ia(i) = i : Next
ib() = ia()
Check "a 6 into a 3x2 (only the count must match)", ib(2,1), 5
sa$(1) = "middle"
sb$() = sa$()
CheckS "strings", sb$(1), "middle"
dup() = boxes()
Check "an array of structures", dup(1).x + dup(0).h, 140
a() = a()
Check "a() = a()", a(4), 6
On Error Skip 1
sc$() = sa$()
Check "string LENGTHs must match", Instr(MM.ErrMsg$, "String length") > 0, 1
On Error Skip 1
a() = ia()
Check "types must match", Instr(MM.ErrMsg$, "Incompatible") > 0, 1
On Error Skip 1
s() = a()
Check "sizes must match", Instr(MM.ErrMsg$, "size mismatch") > 0, 1
On Error Skip 1
a() = 5
Check "a number is not an array", Instr(MM.ErrMsg$, "Expected an array") > 0, 1

' ---- graphics with every element's member --------------------------------------------------------
CLS
Box boxes().x, boxes().y, boxes().w, boxes().h, 1, RGB(white)
Check "BOX boxes().x ... draws the first", Pixel(10, 20) <> 0, 1
Check "and the second", Pixel(100, 120) <> 0, 1
Check "and nothing between", Pixel(60, 80), 0
CLS
Pixel boxes().x, boxes().y, RGB(red)
Check "PIXEL boxes().x, boxes().y", (Pixel(10, 20) <> 0) + (Pixel(100, 120) <> 0), 2
CLS

' ---- a command that cannot take one ----------------------------------------------------------------
On Error Skip 1
Print Json$(boxes().x, "a")
Check "JSON$(boxes().x, ...) is refused", Instr(MM.ErrMsg$, "Struct member arrays") > 0, 1

Print "arrays:"; pass; " of"; tests; " passed"
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
