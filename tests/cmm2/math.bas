' math.bas - MATH after the port of the PicoMite's MATHS.c (9749c83), with the CMM2's fixes: one check for each
' item of docs/MATH_Update_Plan.md that a program can see.
Option Explicit
Dim integer tests, pass, h, h2, i
Dim float x

' ---- phase 1a: memory and crashes ---------------------------------------------------------------------------
Dim a(0,0), b(0,0)
a(0,0) = 4
Math M_Inverse a(), b()
CheckF "a 1x1 inverse (it was 0)", b(0,0), 0.25
Dim m3(2,2) = (2,0,0, 0,4,0, 0,0,8), r3(2,2)
Math M_Inverse m3(), r3()
CheckF "a 3x3 inverse", r3(1,1), 0.25
CheckF "its determinant", Math(M_Determinant m3()), 64
h = MM.Info(Heap)
For i = 1 To 300 : Math M_Inverse m3(), r3() : x = Math(M_Determinant m3()) : Next
h2 = MM.Info(Heap)
Check "the heap is the same after 300 of each (an inverse leaked 15.5 KB)", h2, h

Math Randomize 5
x = Math(Rand)
Math Randomize 5
CheckF "MATH RANDOMIZE n repeats MATH(RAND)'s sequence (it was the hardware RNG every time)", Math(Rand), x
Math Randomize 5
Check "MATH RANDOMIZE 5 gives the PicoMite's numbers (the seeding's multiplier 69069, not 6069)", Math(Rand) * 4294967296, 530851887
For i = 1 To 50 : Math Randomize : Next
Check "MATH RANDOMIZE with no seed, 50 times (about half were Number out of bounds)", 1, 1

Dim integer p%(2) = (6, 7, 8), q%(2) = (2, 0, 4), r%(2)
On Error Skip 1
Math C_Div p%(), q%(), r%()
CheckS "integer C_DIV by zero", ErrText$(), "Divide by zero"

Dim s2(2,3), v1(3)
On Error Skip 1
Math Slice s2(), 1, 2, v1()
CheckS "SLICE with every index given", ErrText$(), "One index must be omitted"
On Error Skip 1
Math Insert s2(), 1, 2, v1()
CheckS "INSERT with every index given (it returned silently)", ErrText$(), "One index must be omitted"

PidLocal

' ---- phase 1b: wrong results --------------------------------------------------------------------------------
Dim w!(3) = (1, 2, 3, 5), wo!(3)
Dim integer lo, hi
Math Window w!(), 0, 10, wo!(), lo, hi
Check "WINDOW's minimum into an integer variable (it got the float's bits)", lo, 1
Check "and its maximum", hi, 5
CheckF "and the scaling", wo!(3), 10
Dim wc!(2) = (3, 3, 3), wco!(2)
Math Window wc!(), 2, 9, wco!()
CheckF "WINDOW of a constant input gives the low end (it was 0/0)", wco!(1), 2

Dim crcdata$ = "123456789"
Check "CRC12 is 12 bits (it was &HFEFB)", Math(CRC12 crcdata$), &HEFB

Option Angle Degrees
Dim qc!(4)
Math Q_Create 90, 0, 0, 1, qc!()
CheckF "Q_CREATE under OPTION ANGLE DEGREES (the cosine took radians)", qc!(0), Cos(45)
Option Angle Radians
Dim qs!(3)
On Error Skip 1
Math Q_Create 90, 0, 0, 1, qs!()
CheckS "Q_CREATE names argument 5 (it said 4)", ErrText$(), "Argument 5 must be a 5 element floating point array"

Dim f1!(0), i1%(0)
f1!(0) = 2.5 : Math Scale f1!(), 0.5, i1%()
Check "SCALE into an integer array: 1.25 is 1 (the scale was rounded first: 3)", i1%(0), 1
f1!(0) = 0.3 : Math Add f1!(), 0.4, i1%()
Check "ADD into an integer array: 0.7 is 1 (0.4 was rounded first: 0)", i1%(0), 1
f1!(0) = 16 : Math Power f1!(), 0.5, i1%()
Check "POWER into an integer array: 16^0.5 is 4 (the exponent was rounded first: 16)", i1%(0), 4
Dim big%(0) = (9007199254740993), big2%(0)
Math Scale big%(), 1, big2%()
Check "SCALE of integers by an integer is exact above 2^53", big2%(0) = 9007199254740993, 1

Dim sf!(1,2) = (1.2, 2.2, 3.2, 4.2, 5.2, 6.2), si%(2)
Math Slice sf!(), 1, , si%()
Check "SLICE from floats into integers converts (it copied the bits)", si%(1), 4
Dim ia!(1) = (0, 10), ib!(1) = (10, 20), ic%(1)
Math Interpolate ia!(), ib!(), 0.5, ic%()
Check "INTERPOLATE into an integer array (it was always Arrays must be different)", ic%(1), 15
Dim mm!(1) = (-1e300, -5e300)
CheckF "MAX of values below -3e38 (it gave the -3e38 it started from)", Math(Max mm!()), -1e300
mm!(0) = 1e300 : mm!(1) = 5e300
CheckF "and MIN of values above 3e38", Math(Min mm!()), 1e300

Dim fi!(7), fc!(2,7)
On Error Skip 1
Math FFT Inverse fc!(), fi!()
CheckS "FFT INVERSE needs a 2 x n array", ErrText$(), "Argument 1 must be a 2 x n array"

' ---- phase 1c: reworked by the PicoMite ---------------------------------------------------------------------
Dim cr!(4) = (-1, -1, 1, 1, 1)
Check "CROSSING: the first sample past the level, with OPTION BASE (it was 1)", Math(Crossing cr!()), 2
Dim cn!(5) = (-1, 1, -1, -1, 1, 1)
Check "and confirmed for 2 samples (the spike at 1 is passed over)", Math(Crossing cn!(), 0, 1, 2), 4

' ---- phase 2 and 4: structure member arrays (stride) --------------------------------------------------------
Type Pt
  x As Float
  y As Float
  n As Integer
End Type
Dim pts(3) As Pt
For i = 0 To 3 : pts(i).x = i + 1 : pts(i).n = 10 * i : Next
Math Scale pts().x, 2, pts().y
CheckF "SCALE of a member of every element", pts(3).y, 8
CheckF "SUM of a member", Math(Sum pts().x), 10
Check "MAX of an integer member", Math(Max pts().n), 30
Math Clamp pts().x, 2, 3, pts().y
CheckF "CLAMP of a member: below", pts(0).y, 2
CheckF "and above", pts(3).y, 3
On Error Skip 1
Math Scale pts(2).x, 2, pts().y
CheckS "a member of one element is refused (it ran past the array's end)", ErrText$(), "Argument 1 must be a member of every element: a().x"
Dim one As Pt
On Error Skip 1
Math Scale one.x, 2, pts().y
CheckS "and a scalar structure's member", ErrText$(), "Argument 1 must be a member of every element: a().x"
Dim g2(1,1) As Pt
On Error Skip 1
Math V_Rotate 0, 0, 0, g2().x, g2().y, pts().x, pts().y
CheckS "the shape is checked (a 2D array of structures passed as 1D)", ErrText$(), "Argument 4 must be a 1D numerical array"
On Error Skip 1
Sort pts().x
CheckS "SORT refuses a member array", ErrText$(), "Struct member arrays not supported for this command"
On Error Skip 1
Math M_Print pts().x
CheckS "and so does a MATH command without the stride", ErrText$(), "Struct member arrays not supported for this command"

' ---- phase 3: string arrays ---------------------------------------------------------------------------------
Dim s$(2) Length 8, t$(2) Length 8
Array Set "ab", s$()
CheckS "ARRAY SET of a string array", s$(2), "ab"
Array Add s$(), "!", t$()
CheckS "ARRAY ADD to a string array appends", t$(1), "ab!"
Dim sm$(1,2) Length 8, sv$(2) Length 8
sm$(1,0) = "p" : sm$(1,1) = "q" : sm$(1,2) = "r"
Array Slice sm$(), 1, , sv$()
CheckS "ARRAY SLICE of a string array", sv$(1), "q"
sv$(1) = "z"
Array Insert sm$(), 0, , sv$()
CheckS "ARRAY INSERT into a string array", sm$(0,1), "z"
Math Set "cd", s$()
CheckS "MATH SET is the same command", s$(0), "cd"
Dim short$ Length 8 = "ABCDEFG"
On Error Skip 1
Array Set "", short$
CheckS "a scalar string is not a string array", ErrText$(), "Argument 2 must be a string array"

' ---- phase 5: CLAMP and SINC --------------------------------------------------------------------------------
Dim cl!(3) = (-5, 0.5, 2, 9), co!(3)
Math Clamp cl!(), 0, 1, co!()
CheckF "CLAMP: below the range", co!(0), 0
CheckF "inside it", co!(1), 0.5
CheckF "above it", co!(3), 1
Dim sx!(20), sy!(20), ox!(20), oy!(20)
For i = 0 To 20 : sx!(i) = i : sy!(i) = 5 : Next
Math Sinc sx!(), sy!(), 21, 7, 0.4, ox!(), oy!()
CheckT "SINC of a constant is the constant", oy!(10), 5, 0.001
CheckT "and the x values stay", ox!(10), 10, 0.001

Print "math:"; pass; " of"; tests; " passed"
End

Sub PidLocal
  Local p!(13)
  On Error Skip 1
  Math PID Init 1, p!(), PidTick
  CheckS "PID INIT with a LOCAL array (its SUB frees it)", ErrText$(), "Must be a global variable"
End Sub
Sub PidTick
End Sub

' the error message without "Error in line n: "
Function ErrText$()
  Local integer c
  ErrText$ = MM.ErrMsg$
End Function

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
  CheckT n$, got, want, 1e-9 * Max(1, Abs(want))
End Sub

Sub CheckT(n$, got, want, tol)
  tests = tests + 1
  If Abs(got - want) <= tol Then
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
