' arcbez.bas - ARC by the PicoMite's scanline method and BEZIER through control-point arrays (M7 of the b14 merge).
' Each shape is drawn small, at 400, 300 (clear of the text PRINT draws at the top), and read back with PIXEL() as a
' map, so the expected output shows the rendering itself.
Option Explicit
Dim integer x, y, n, bx(3), by(3), b5x(4), b5y(4)
Sub showmap(x0, y0, w, h)
  Local integer i, j, c
  Local string r$
  c = 0
  For j = y0 To y0 + h - 1
    r$ = ""
    For i = x0 To x0 + w - 1
      If Pixel(i, j) <> 0 Then
        r$ = r$ + "#" : c = c + 1
      Else
        r$ = r$ + "."
      EndIf
    Next
    Print r$
  Next
  Print "pixels"; c
End Sub
CLS
' a quarter ring, 0 (up) clockwise to 90 (right), radii 6 to 9
Arc 412, 312, 6, 9, 0, 90, RGB(White)
Print "arc 0-90:"
showmap(400, 300, 25, 25)
CLS
' one pixel wide, crossing 0: from 315 to 45
Arc 412, 312, 9, , 315, 45, RGB(White)
Print "arc 315-45 thin:"
showmap(400, 300, 25, 25)
CLS
' the long way round, 90 to 0
Arc 412, 312, 4, 9, 90, 0, RGB(White)
Print "arc 90-0:"
showmap(400, 300, 25, 25)
CLS
' a cubic bezier: start, two controls, end
bx(0) = 400 : by(0) = 315 : bx(1) = 410 : by(1) = 300 : bx(2) = 420 : by(2) = 300 : bx(3) = 430 : by(3) = 315
Bezier bx(), by(), , RGB(White)
Print "bezier 4 points:"
showmap(400, 300, 32, 17)
CLS
' five points, the first three only
b5x(0) = 402 : b5y(0) = 302 : b5x(1) = 415 : b5y(1) = 314 : b5x(2) = 428 : b5y(2) = 302 : b5x(3) = 499 : b5y(3) = 399 : b5x(4) = 499 : b5y(4) = 399
Bezier b5x(), b5y(), 3, RGB(White)
Print "bezier first 3 of 5:"
showmap(400, 300, 32, 17)
CLS
On Error Skip 1
Bezier bx(), b5y()
Print "mismatch: "; MM.ErrMsg$
On Error Skip 1
Arc 412, 312, 9, 6, 0, 90
Print "radii: "; MM.ErrMsg$
On Error Skip 1
Arc 412, 312, 6, 9, 45, 45
Print "radials: "; MM.ErrMsg$
Print "done"
