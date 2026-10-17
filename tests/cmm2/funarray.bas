' funarray.bas - FUNCTIONs that return a whole array (plan step 8, the PicoMite's): AS INTEGER(...) or AS FLOAT(...)
' after the definition, dimensions evaluated on each call after the arguments, the result filled element by element
' or by a command, and taken only by a whole-array assignment; every other use refused before the body runs.
Dim integer b(6), c(9), d(3), i, ran
Dim float m(2, 2), mi(2, 2), e(6)
Dim integer two(1, 3)

Print "-- AS INTEGER(6)"
b() = fred(1, 2)
For i = 0 To 6 : Print b(i); : Next : Print

Print "-- dimensions from the arguments"
c() = sq(10)
For i = 0 To 9 : Print c(i); : Next : Print

Print "-- a 2x4 result into a 2x4, and into 8 elements of another shape"
two() = grid(2, 4)
Print two(0, 0); two(1, 0); two(0, 3); two(1, 3)
Dim integer flat(7)
flat() = grid(2, 4)
For i = 0 To 7 : Print flat(i); : Next : Print

Print "-- filled by a command: MATH M_INVERSE"
m(0, 0) = 2 : m(1, 1) = 4 : m(2, 2) = 8 : m(0, 2) = 1
mi() = Invr(m())
Print mi(0, 0); mi(1, 1); mi(2, 2); mi(0, 2)

Print "-- a FLOAT array function"
e() = halves(7)
For i = 0 To 6 : Print e(i); : Next : Print

Print "-- refused anywhere else, before the body runs"
ran = 0
On Error Skip
Print fred(1, 2)
Print em$(); "  body ran:"; ran
On Error Skip
i = fred(1, 2) + 1
Print em$(); "  body ran:"; ran

Print "-- the wrong size or type"
On Error Skip
d() = fred(1, 2)
Print em$()
On Error Skip
e() = fred(1, 2)
Print em$()
On Error Skip
b() = notarray(1)
Print em$()
Print "done"
End

Function fred(a1, a2) As Integer(6)
  Local integer k
  ran = ran + 1
  For k = 0 To 6 : fred(k) = a1 * 10 + a2 + k : Next
End Function

Function sq(n) As Integer(n - 1)
  Local integer k
  For k = 0 To n - 1 : sq(k) = k * k : Next
End Function

Function grid(r, w) As Integer(r - 1, w - 1)
  Local integer y, x
  For y = 0 To r - 1
    For x = 0 To w - 1
      grid(y, x) = y * 100 + x
    Next
  Next
End Function

Function Invr(n()) As Float(Bound(n(), 1), Bound(n(), 2))
  Math M_Inverse n(), Invr()
End Function

Function halves(n) As Float(n - 1)
  Local integer k
  For k = 0 To n - 1 : halves(k) = k / 2 : Next
End Function

Function notarray(n) As Integer
  notarray = n
End Function

' the error message without its "Error in line n: "
Function em$()
  em$ = Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ": ") + 2)
End Function
