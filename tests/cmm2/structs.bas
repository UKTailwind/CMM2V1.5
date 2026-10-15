' structs.bas - P1 step 5b.2: structure variables and their members.  A structure, an array of them, members of
' every type, nested structures and member arrays, read and assigned; whole structures assigned; LOCAL, STATIC and
' ERASE; the errors.  Two PicoMite faults are checked as fixed: a member used as an index (a(p.x)) left its type
' behind, and a nested member assigned whole (a.inner = b.inner) copied the outer type's size.
Option Explicit
Dim integer tests, pass, i, j
Dim float x

Type TPoint
  x As INTEGER
  y As INTEGER
End Type

Type TRect
  topLeft As TPoint
  size As TPoint
  colour As INTEGER
  name As STRING LENGTH 12
  corners(3) As TPoint
  weights(1,2) As FLOAT
End Type

Dim p As TPoint, q As TPoint, q2 As TPoint
Dim r As TRect, s As TRect
Dim pts(4) As TPoint
Dim grid(2,3) As TPoint
Dim a(5), b$(3)

' ---- a structure and its members --------------------------------------------------------------
p.x = 3 : p.y = -7
Check "p.x", p.x, 3
Check "p.y", p.y, -7
q = p
Check "q = p copies the structure", q.x * 100 + q.y, 293
q.x = 99
Check "and q is a copy", p.x, 3

' ---- nested structures, strings, member arrays ------------------------------------------------
r.topLeft.x = 10 : r.topLeft.y = 20
r.size.x = 300 : r.size.y = 200
r.colour = 12345
r.name = "rectangle"
For i = 0 To 3
  r.corners(i).x = i * 10 : r.corners(i).y = i * 20
Next
For i = 0 To 1 : For j = 0 To 2 : r.weights(i, j) = i + j / 4 : Next : Next
Check "r.topLeft.x", r.topLeft.x, 10
Check "r.size.y", r.size.y, 200
Check "r.colour", r.colour, 12345
CheckS "r.name", r.name, "rectangle"
Check "r.corners(3).y", r.corners(3).y, 60
Check "r.weights(1,2)", r.weights(1, 2), 1.5
Check "r.corners(i).x in an expression", r.corners(1).x + r.corners(2).x * 2, 50
s = r
Check "s = r copies the nested parts", s.corners(2).y + s.topLeft.y + s.weights(0, 1), 60.25
CheckS "and the string", s.name, "rectangle"
s.topLeft = p
Check "s.topLeft = p, a nested member assigned whole", s.topLeft.x * 100 + s.topLeft.y, 293
Check "and only that member", s.size.x, 300
s.size = r.topLeft
Check "s.size = r.topLeft, nested to nested", s.size.x + s.size.y, 30
CheckS "and the members after it are untouched", s.name, "rectangle"
On Error Skip 1
r.name = "a name of more than twelve"
Check "a string member's LENGTH is kept", Instr(MM.ErrMsg$, "too long") > 0, 1

' ---- arrays of structures ---------------------------------------------------------------------
For i = 0 To 4 : pts(i).x = i : pts(i).y = i * i : Next
Check "pts(4).y", pts(4).y, 16
pts(0) = pts(3)
Check "pts(0) = pts(3)", pts(0).x * 10 + pts(0).y, 39
For i = 0 To 2 : For j = 0 To 3 : grid(i, j).x = i : grid(i, j).y = j : Next : Next
Check "grid(2,3)", grid(2, 3).x * 10 + grid(2, 3).y, 23
Check "grid(1,2)", grid(1, 2).x * 10 + grid(1, 2).y, 12
r.corners(1) = pts(4)
Check "a member array's element assigned whole", r.corners(1).y, 16

' ---- a member as an index (the PicoMite read a(p.x) as the member's type) ----------------------
a(3) = 1.5 : b$(3) = "three"
p.x = 3
Check "a(p.x) is the float", a(p.x), 1.5
CheckS "b$(p.x) is the string", b$(p.x), "three"
a(p.x) = 2.5
Check "a(p.x) = 2.5 stores the float", a(3), 2.5
Check "pts(p.x).y", pts(p.x).y, 9

' ---- LOCAL and STATIC structures --------------------------------------------------------------
Check "a LOCAL structure", LocalSum(4), 4 * 5
Check "and again (freed and made again)", LocalSum(2), 2 * 5
For i = 1 To 3 : x = Counter() : Next
Check "a STATIC structure keeps its value", x, 3

' ---- whole structures as parameters, and FUNCTIONs that return them ----------------------------
Check "a structure by reference", SumPoint(p), p.x + p.y
Check "a nested member as a structure (its own type, not the outer one)", SumPoint(r.topLeft), 30
MovePoint r.topLeft, 5
Check "and changed through it", r.topLeft.x, 15
q2 = MakePoint(7, 8)
Check "a FUNCTION returning a structure", q2.x * 10 + q2.y, 78
Check "an array of structures as a parameter", SumAll(pts()), 13
Check "an element of one", SumPoint(pts(4)), 20
On Error Skip 1
x = SumPoint(r)
Check "a structure of another type", Instr(MM.ErrMsg$, "mismatch") > 0, 1
On Error Skip 1
x = SumPoint(5)
Check "a number for a structure", Instr(MM.ErrMsg$, "structure variable") > 0, 1

' ---- statements that write a member: INC, FOR, MID$, READ ----------------------------------------
p.x = 1
Inc p.x
Inc p.x, 10
Check "INC p.x and INC p.x, 10", p.x, 12
r.name = "abc"
Inc r.name, "de"
CheckS "INC of a string member", r.name, "abcde"
On Error Skip 1
Inc r.name, "0123456789"
Check "INC keeps the member's LENGTH", Instr(MM.ErrMsg$, "too long") > 0, 1
j = 0
For p.y = 1 To 4 : j = j + p.y : Next p.y
Check "FOR with a member", j * 100 + p.y, 1005
Mid$(r.name, 2, 2) = "XY"
CheckS "MID$= on a string member", r.name, "aXYde"
Restore Pdata
Read pts(1).x, pts(1).y, r.name, r.weights(1, 1)
Check "READ into members", pts(1).x * 100 + pts(1).y, 4243
CheckS "a string member", r.name, "read"
Check "a member array's element", r.weights(1, 1), 0.5
On Error Skip 1
Read pts().x
Check "READ into a member array is refused", Instr(MM.ErrMsg$, "not supported") > 0, 1

' ---- DIM with initial values --------------------------------------------------------------------
Dim i1 As TPoint = (5, 6)
Check "DIM p AS TPoint = (5, 6)", i1.x * 10 + i1.y, 56
Dim i2(1) As TPoint = (1, 2, 3, 4)
Check "an array of structures", i2(0).x * 1000 + i2(0).y * 100 + i2(1).x * 10 + i2(1).y, 1234
Check "a LOCAL initialised", LocalInit(), 30
On Error Skip 1
Dim i3 As TPoint = (1)
Check "not enough values", Instr(MM.ErrMsg$, "Not enough") > 0, 1
On Error Skip 1
Dim i4 As TPoint = (1, 2, 3)
Check "too many values", Instr(MM.ErrMsg$, "Too many") > 0, 1
On Error Skip 1
Dim short$ Length 3 = "abcdefgh"
Check "DIM s$ LENGTH 3 = a longer string is an error (it ran past the string)", Instr(MM.ErrMsg$, "too long") > 0, 1

' ---- errors ------------------------------------------------------------------------------------
On Error Skip 1
p = r
Check "structure types must match", Instr(MM.ErrMsg$, "must match") > 0, 1
On Error Skip 1
p = 5
Check "a number is not a structure", Instr(MM.ErrMsg$, "structure") > 0, 1
On Error Skip 1
x = p.z
Check "an unknown member", Instr(MM.ErrMsg$, "Unknown structure member") > 0, 1
On Error Skip 1
x = r.corners(4).x
Check "a member array's bounds", Instr(MM.ErrMsg$, "out of bounds") > 0, 1
On Error Skip 1
x = r.corners.x
Check "a member array needs its index", Instr(MM.ErrMsg$, "index") > 0, 1
On Error Skip 1
x = pts(5).x
Check "an array of structures' bounds", Instr(MM.ErrMsg$, "out of bounds") > 0, 1

' ---- ERASE ----------------------------------------------------------------------------------
Erase q
Dim q As TRect
q.colour = 5
Check "ERASE, then DIM the name as another type", q.colour, 5

Print "structs:"; pass; " of"; tests; " passed"
End

Function LocalInit() As INTEGER
  Local t As TPoint = (10, 20)
  LocalInit = t.x + t.y
End Function

Function SumPoint(a As TPoint) As INTEGER
  SumPoint = a.x + a.y
End Function

Sub MovePoint(a As TPoint, d As INTEGER)
  a.x = a.x + d
End Sub

Function MakePoint(x As INTEGER, y As INTEGER) As TPoint
  MakePoint.x = x : MakePoint.y = y
End Function

Function SumAll(a() As TPoint) As INTEGER
  Local integer k
  For k = 0 To Bound(a())
    SumAll = SumAll + a(k).x
  Next
End Function

Function LocalSum(n As INTEGER) As INTEGER
  Local t As TPoint, u(2) As TPoint
  Check "the LOCAL starts at zero", t.x + u(2).y, 0
  t.x = n : u(2).y = 5
  LocalSum = t.x * u(2).y
End Function

Function Counter() As INTEGER
  Static c As TPoint
  c.x = c.x + 1
  Counter = c.x
End Function

Pdata:
Data 42, 43, "read", 0.5

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
