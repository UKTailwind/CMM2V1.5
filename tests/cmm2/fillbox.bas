' fillbox.bas - FILL x, y, colour [, boundary] in each colour depth, and BOX with arrays of x and y and one width and
' height (M8 of the b14 merge, the PicoMite's).  The pixels are counted with PIXEL(), against a pixel drawn in the same
' colour, so the counts do not depend on how a depth rounds a colour.  The box is drawn in a colour that 8- and 16-bit
' pixels cannot hold exactly: a boundary compared after expansion to RGB888 would be missed there and the fill leak.
' The shapes are at 300, 300, clear of the text PRINT draws at the top.
Option Explicit
Dim integer depths(3) = (8, 12, 16, 32), bx(2) = (100, 150, 200), by(2) = (100, 100, 100), bw(2) = (20, 30, 10)
Dim integer i, d, ref, edge, n

Function count(c As Integer, x0 As Integer, y0 As Integer, w As Integer, h As Integer) As Integer
  Local integer x, y
  For y = y0 To y0 + h - 1
    For x = x0 To x0 + w - 1
      If Pixel(x, y) = c Then count = count + 1
    Next
  Next
End Function

For i = 0 To 3
  d = depths(i)
  On Error Clear
  On Error Skip 1
  Mode 8, d
  If MM.ErrNo Then Print "depth"; d; ": not available" : Continue For
  CLS RGB(Black)
  Pixel 500, 400, RGB(Green) : ref = Pixel(500, 400)
  Pixel 501, 400, RGB(200, 100, 50) : edge = Pixel(501, 400)
  ' replace: the black inside the box and not the black outside it, around two blue pixels
  Box 310, 310, 30, 20, 1, RGB(200, 100, 50)
  Pixel 320, 315, RGB(Blue) : Pixel 321, 315, RGB(Blue)
  Fill 315, 315, RGB(Green)
  Print "depth"; d; " replace:"; count(ref, 300, 300, 60, 40); " (502) box:"; count(edge, 300, 300, 60, 40); " (96)";
  Fill 315, 315, RGB(Green)
  Print " again:"; count(ref, 300, 300, 60, 40);
  ' boundary: everything inside the box, across a wall of the fill colour and over a blue pixel
  CLS RGB(Black)
  Box 310, 310, 30, 20, 1, RGB(200, 100, 50)
  Line 325, 311, 325, 328, 1, RGB(Green)
  Pixel 315, 315, RGB(Blue)
  Fill 330, 320, RGB(Green), RGB(200, 100, 50)
  Print " boundary:"; count(ref, 300, 300, 60, 40); " (504)";
  Fill 310, 310, RGB(Red), RGB(200, 100, 50)
  Print " on the edge:"; count(edge, 300, 300, 60, 40)
  ' the whole screen, from a corner
  CLS RGB(Black)
  Fill MM.HRes - 1, MM.VRes - 1, RGB(Green)
  Print "  whole screen: "; Choice(Pixel(0, MM.VRes - 1) = ref And Pixel(MM.HRes - 1, 200) = ref And Pixel(MM.HRes \ 2, MM.VRes \ 2) = ref, "filled", "NOT FILLED")
Next
Mode 8, 8
CLS RGB(Black)
Pixel 300, 300, RGB(Green) : ref = Pixel(300, 300)
Box bx(), by(), 20, 10, 1, RGB(Green)
Print "box arrays, one size:"; count(ref, 90, 90, 140, 30); " (168)"
CLS RGB(Black)
Box bx(), by(), bw(), 10, 1, RGB(Green)
Print "box arrays, widths:"; count(ref, 90, 90, 140, 30); " (" ; 2 * (20 + 30 + 10) + 3 * 16; ")"
CLS RGB(Black)
On Error Skip 1
Fill 15, 15
Print "fill arguments: "; MM.ErrMsg$
On Error Skip 1
Fill MM.HRes, 15, RGB(Green)
Print "fill x: "; MM.ErrMsg$
CLS
Print "done"
