' gfxsave.bas - the graphics items of 26 Oct 2026 (docs/PicoMite_Parity_ToDo.md): BLIT COMPRESSED (BLIT MEMORY with
' bit 15 taken as set), BLIT RESIZE (the PicoMite's, MODE n,4), and SAVE IMAGE without loss: 16 colours or fewer a
' 4-bit RLE4 BMP, up to 256 an 8-bit RLE8 one, more a 24-bit one, with SAVE COMPRESSED IMAGE the same command. Each
' saved picture is loaded back with LOAD BMP and compared pixel for pixel. Uses files gs_*.bmp in the current directory.
' The results are printed at the end: printed as they come, they would be drawn over the pixels being tested.
Option Explicit
Option Default Integer
Dim tests, pass, a, b, x, y, bad, k, n, r, g, bl, w
Dim integer buf(3), buf2(3), p(299, 9)
Dim res$(40) Length 120

' ---- BLIT COMPRESSED
Mode 8
CLS
a = Peek(VarAddr buf()) : b = Peek(VarAddr buf2())
Poke Short a, 4 : Poke Short a + 2, 2 : Poke Byte a + 4, &HF4 : Poke Byte a + 5, &H44   ' 4 x 2: 4 white, 4 midgreen
Poke Short b, 4 Or &H8000 : Poke Short b + 2, 2 : Poke Byte b + 4, &HF4 : Poke Byte b + 5, &H44
Blit Compressed a, 10, 10
Blit Memory b, 30, 10
bad = 0
For y = 0 To 1 : For x = 0 To 3 : If Pixel(10 + x, 10 + y) <> Pixel(30 + x, 10 + y) Then bad = bad + 1
Next : Next
Check "BLIT COMPRESSED draws as BLIT MEMORY with bit 15 set: the pixels that differ", bad, 0
Check "  two rows of different colours", Pixel(10, 10) <> Pixel(10, 11) And Pixel(10, 10) <> 0, 1
Blit Memory a, 50, 10
Check "BLIT MEMORY of the same data without the flag reads it uncompressed", Pixel(50, 10) <> Pixel(10, 10), 1

' ---- BLIT RESIZE
Mode 7, 4
CLS
Pixel 0, 0, RGB(red) : Pixel 1, 0, RGB(green) : Pixel 0, 1, RGB(blue) : Pixel 1, 1, RGB(white)
r = Pixel(0, 0) : g = Pixel(1, 0) : bl = Pixel(0, 1) : w = Pixel(1, 1)
Blit Resize N, N, 0, 0, 2, 2, 20, 20, 4, 4
n = (Pixel(20, 20) = r) + (Pixel(21, 21) = r) + (Pixel(22, 20) = g) + (Pixel(23, 21) = g)
n = n + (Pixel(20, 22) = bl) + (Pixel(21, 23) = bl) + (Pixel(22, 22) = w) + (Pixel(23, 23) = w)
Check "BLIT RESIZE 2 x 2 to 4 x 4: each pixel twice as wide and high (8 checked)", n, 8
Blit Resize N, N, 0, 0, 2, 2, 40, 20, 4, 4, 8
Check "  with 8 (red) transparent: the red left black, the green drawn", (Pixel(40, 20) = 0) + (Pixel(42, 20) = g), 2
Blit Resize N, N, 20, 20, 4, 4, 60, 20, 2, 2
n = (Pixel(60, 20) = r) + (Pixel(61, 20) = g) + (Pixel(60, 21) = bl) + (Pixel(61, 21) = w)
Check "  4 x 4 to 2 x 2: the four colours again", n, 4
Blit Resize N, N, 0, 0, 2, 2, 1, 0, 4, 4
n = (Pixel(1, 0) = r) + (Pixel(2, 1) = r) + (Pixel(3, 0) = g) + (Pixel(4, 1) = g) + (Pixel(1, 2) = bl) + (Pixel(4, 3) = w)
Check "  onto itself, overlapping: from the source as it was (6 checked)", n, 6
On Error Skip 1
Blit Resize N, N, 300, 0, 50, 2, 0, 0, 2, 2
CheckS "  a source past the buffer's edge", MM.ErrMsg$, "Number out of bounds"
Mode 8
On Error Skip 1
Blit Resize N, N, 0, 0, 2, 2, 20, 20, 4, 4
CheckS "  in an 8-bit mode", MM.ErrMsg$, "Invalid for this screen mode"

' ---- SAVE IMAGE: 12 colours, short and long runs (one of 300) and absolute runs
CLS
For y = 0 To 8 : For x = 0 To 299
  k = (x * 7 + y * 3 + x \ 5) Mod 12
  Pixel x, y, RGB(k * 20, 255 - k * 20, (k * 50) Mod 256)
Next : Next
Line 0, 9, 299, 9, 1, RGB(yellow)
Area "gs_4.bmp", 4, 2, "12 colours"

' ---- 200 colours, each an exact RGB332 one
CLS
For y = 0 To 9 : For x = 0 To 299
  k = (x + y * 13) Mod 200
  Pixel x, y, RGB(((k >> 5) And 7) * 36, ((k >> 2) And 7) * 36, (k And 3) * 85)
Next : Next
Save Compressed Image "gs_c.bmp", 0, 0, 300, 10
Area "gs_8.bmp", 8, 1, "200 colours"
Check "SAVE COMPRESSED IMAGE: the same file as SAVE IMAGE", MM.Info(FileSize "gs_c.bmp"), MM.Info(FileSize "gs_8.bmp")

' ---- more than 256 colours, in 16-bit colour
Mode 8, 16
CLS
For y = 0 To 9 : For x = 0 To 299
  Pixel x, y, RGB((x * 7) Mod 256, (x * 3 + y * 40) Mod 256, x Mod 256)
Next : Next
Area "gs_24.bmp", 24, 0, "more than 256 colours"

' ---- MODE n,1
Mode 7, 1
CLS
Box 10, 2, 40, 6, 1, RGB(white), RGB(white)
Save Image "gs_1.bmp", 0, 0, 100, 10
Check "MODE 7,1: a 4-bit BMP", Header("gs_1.bmp", 29), 4
Mode 8
Kill "gs_4.bmp" : Kill "gs_8.bmp" : Kill "gs_c.bmp" : Kill "gs_24.bmp" : Kill "gs_1.bmp"
CLS
For k = 1 To tests : Print res$(k) : Next
Print "gfxsave:"; pass; " of"; tests; " passed"
End

' the 300 x 10 area at 0, 0 saved as f$: its bits a pixel and compression, then loaded back and compared
Sub Area(f$, bits, comp, what$)
  Local x, y, bad
  For y = 0 To 9 : For x = 0 To 299 : p(x, y) = Pixel(x, y) : Next : Next
  Save Image f$, 0, 0, 300, 10
  Check "SAVE IMAGE, " + what$ + ": bits a pixel", Header(f$, 29), bits
  Check "  compression (2 RLE4, 1 RLE8, 0 none)", Header(f$, 31), comp
  If comp Then Check "  smaller than 24-bit", MM.Info(FileSize f$) < 54 + 900 * 10, 1
  CLS
  Load Bmp f$, 0, 0
  bad = 0
  For y = 0 To 9 : For x = 0 To 299 : If Pixel(x, y) <> p(x, y) Then bad = bad + 1
  Next : Next
  Check "  loaded back: the pixels of 3000 that differ", bad, 0
End Sub

' byte n (from 1) of a file's header
Function Header(f$, n)
  Local h$
  Open f$ For Input As #1
  h$ = Input$(54, #1)
  Close #1
  Header = Asc(Mid$(h$, n, 1))
End Function

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    res$(tests) = "ok  " + n$
  Else
    res$(tests) = "FAIL " + n$ + " got " + Str$(got) + " want " + Str$(want)
  EndIf
End Sub

Sub CheckS(n$, got$, want$)
  tests = tests + 1
  If got$ = want$ Then
    pass = pass + 1
    res$(tests) = "ok  " + n$
  Else
    res$(tests) = "FAIL " + n$ + " got " + got$ + " want " + want$
  EndIf
End Sub
