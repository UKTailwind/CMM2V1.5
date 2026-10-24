' draw3dfix.bas - DRAW3D's issues 2 to 6 (UKTailwind/CMM2V1.5, 24 Oct 2026; the PicoMite's 132 to 136): HIDE goes on
' past an object already hidden, each lit face is lit from the light to that face, CREATE refuses a face of more than
' 128 vertices and a vertex number outside the object, and ROTATE, RESET, HIDE, RESTORE and CLOSE count their
' arguments. Two triangles side by side in the plane z = 0 are drawn at scale 1 (z = the view plane, 1000).
Option Explicit
Dim integer tests, pass, i, xa, xb, yc, ref
Dim float v(2, 5), q(4)
Dim integer fc(1), fv(5), col(0), lc(1), fl(1), big(0), bigv(128), bad(2)
Dim float v3(2, 2)

For i = 0 To 5 : Read v(0, i), v(1, i), v(2, i) : Next
Data -200, -50, 0, -100, -50, 0, -150, 50, 0
Data 100, -50, 0, 200, -50, 0, 150, 50, 0
fc(0) = 3 : fc(1) = 3
For i = 0 To 5 : fv(i) = i : Next
col(0) = RGB(white)
xa = MM.HRes \ 2 - 150 : xb = MM.HRes \ 2 + 150 : yc = MM.VRes \ 2 + 15   ' the triangles' centroids on the screen

CLS
Pixel xa, yc, RGB(white) : ref = Pixel(xa, yc) : CLS             ' full white in this mode's colour depth
Draw3D Camera 1, 1000

' ---- 3: the light's direction for each face ---------------------------------------------------------------------------
' The light is 100 in front of face 1's second vertex, so face 1 is lit square on (full white at ambient 50); face 2's
' second vertex is 300 to the side of it (a third as bright). Every face was lit as the last face, face 2.
Draw3D Create 1, 6, 2, 1, v(), fc(), fv(), col(), lc(), fl()
Draw3D Set Flags 1, 8, 0, 2
Draw3D Light 1, -100, -50, 900, 50
Draw3D Show 1, 0, 0, 1000, 1
Check "a lit face square on to the light is at full brightness", Pixel(xa, yc), ref
Check "a lit face at an angle to it is darker (it had the other face's brightness)", Pixel(xb, yc) < Pixel(xa, yc), 1
Draw3D Close 1

' ---- 2: HIDE past an object already hidden ------------------------------------------------------------------------------
Draw3D Create 1, 6, 2, 1, v(), fc(), fv(), col(), lc(), fl()
Draw3D Create 2, 6, 2, 1, v(), fc(), fv(), col(), lc(), fl()
Draw3D Show 1, 0, 0, 1000, 1
Draw3D Show 2, 0, -120, 1000, 1
Draw3D Hide 1
Draw3D Hide 1, 2
Check "HIDE 1, 2 with 1 hidden hides 2 (it returned at 1)", Pixel(xa, yc + 120), 0
On Error Skip 1
Draw3D Restore 2
CheckS "and RESTORE 2 then draws it again", ErrText$(), ""
Draw3D Close 1, 2

' ---- 4 and 5: CREATE's checks of the faces --------------------------------------------------------------------------------
For i = 0 To 2 : v3(0, i) = v(0, i) : v3(1, i) = v(1, i) : v3(2, i) = v(2, i) : Next
big(0) = 129
On Error Skip 1
Draw3D Create 3, 3, 1, 1, v3(), big(), bigv(), col(), lc()
CheckS "a face of 129 vertices (it overran display3d's arrays)", ErrText$(), "Vertex count more than 128 for face 0"
bad(0) = 0 : bad(1) = 1 : bad(2) = 3
On Error Skip 1
Draw3D Create 3, 3, 1, 1, v3(), fc(), bad(), col(), lc()
CheckS "vertex 3 of an object of three, 0 to 2 (it read outside the object)", ErrText$(), "Vertex 3 does not exist (face/vertex array element 2)"
bad(2) = -1
On Error Skip 1
Draw3D Create 3, 3, 1, 1, v3(), fc(), bad(), col(), lc()
CheckS "vertex -1", ErrText$(), "Vertex -1 does not exist (face/vertex array element 2)"
bad(2) = 2
On Error Skip 1
Draw3D Create 3, 3, 1, 1, v3(), fc(), bad(), col(), lc()
CheckS "the object refused is not left behind: the same number is created", ErrText$(), ""
Draw3D Close 3

' ---- 6: the argument counts ---------------------------------------------------------------------------------------------
q(0) = 1 : q(4) = 1
Draw3D Create 1, 6, 2, 1, v(), fc(), fv(), col(), lc()
On Error Skip 1 : Draw3D Rotate q()
CheckS "ROTATE with no object (it did nothing)", ErrText$(), "Argument count"
On Error Skip 1 : Draw3D Reset
CheckS "RESET with no object", ErrText$(), "Argument count"
On Error Skip 1 : Draw3D Hide
CheckS "HIDE with no object", ErrText$(), "Argument count"
On Error Skip 1 : Draw3D Restore
CheckS "RESTORE with no object", ErrText$(), "Argument count"
On Error Skip 1 : Draw3D Close
CheckS "CLOSE with no object", ErrText$(), "Argument count"
On Error Skip 1 : Draw3D Rotate q(), 1
CheckS "ROTATE with one object", ErrText$(), ""
On Error Skip 1 : Draw3D Reset 1
CheckS "RESET with one object", ErrText$(), ""
On Error Skip 1 : Draw3D Close 1
CheckS "CLOSE with one object", ErrText$(), ""
Draw3D Close All
CLS

Print "draw3dfix:"; pass; " of"; tests; " passed"
End

' the error message without "Error in line n: "
Function ErrText$()
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

Sub CheckS(n$, got$, want$)
  tests = tests + 1
  If got$ = want$ Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got "; got$; " want "; want$
  EndIf
End Sub
