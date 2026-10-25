' savepng.bas - SAVE PNG (the PicoMite's, 25 Oct 2026): the screen saved and loaded back with LOAD PNG gives the same
' pixels, a window too, the name and the size of the file, and the forms of OPTION PRINTSCREEN. (The pixel for pixel
' comparison with SAVE IMAGE's BMP, in three colour depths, and the keys are tested from the PC, over the console.)
Option Explicit
Dim integer tests, pass, i, x, y, bad, sz
Dim integer px(39, 29), pw(9, 7)

CLS
For i = 0 To MM.HRes - 1 : Line i, 0, i, MM.VRes \ 2, 1, RGB(i Mod 256, (i * 3) Mod 256, 255 - (i Mod 256)) : Next
Circle MM.HRes \ 2, MM.VRes * 3 \ 4, 60, 3, 1, RGB(red), RGB(blue)
Text 10, MM.VRes \ 2 + 10, "SAVE PNG", , 4
For y = 0 To 29 : For x = 0 To 39 : px(x, y) = Pixel(x * MM.HRes \ 40 + 3, y * MM.VRes \ 30 + 5) : Next : Next
For y = 0 To 7 : For x = 0 To 9 : pw(x, y) = Pixel(101 + x * 11, 57 + y * 8) : Next : Next

Save Png "A:/sp_t"
CLS
Load Png "A:/sp_t"
bad = 0
For y = 0 To 29
  For x = 0 To 39
    If Pixel(x * MM.HRes \ 40 + 3, y * MM.VRes \ 30 + 5) <> px(x, y) Then bad = bad + 1
  Next
Next
Check "the screen saved and loaded back: a grid of 1200 pixels, the ones that differ", bad, 0
Check "the name gets .PNG", MM.Info(Exists File "A:/sp_t.png"), 1
sz = MM.Info(FileSize "A:/sp_t.png")
Check "and the file is compressed: under a tenth of a BMP's 3 bytes a pixel", sz < MM.HRes * MM.VRes * 3 / 10, 1

Save Png "A:/sp_w.png", 101, 57, 101, 57
CLS
Load Png "A:/sp_w.png", 101, 57
bad = 0
For y = 0 To 7 : For x = 0 To 9 : If Pixel(101 + x * 11, 57 + y * 8) <> pw(x, y) Then bad = bad + 1
Next : Next
Check "a 101 x 57 window saved and loaded back where it was: 80 pixels, the ones that differ", bad, 0
Check "nothing outside the window", Pixel(100, 56) + Pixel(202, 114) + Pixel(150, 114) + Pixel(202, 80), 0
On Error Skip 1
Save Png "A:/sp_x", 0, 0, 0, 10
CheckS "a width of 0", ErrText$(), "0 is invalid (valid is 1 to " + Str$(MM.HRes) + ")"
Kill "A:/sp_t.png" : Kill "A:/sp_w.png"

On Error Skip 1 : Option Printscreen On
CheckS "OPTION PRINTSCREEN ON", ErrText$(), ""
On Error Skip 1 : Option Printscreen Off
CheckS "OPTION PRINTSCREEN OFF", ErrText$(), ""
On Error Skip 1 : Option Printscreen Format JPG
CheckS "OPTION PRINTSCREEN FORMAT JPG", ErrText$(), "Syntax"
On Error Skip 1 : Option Printscreen
CheckS "OPTION PRINTSCREEN alone", ErrText$(), "Syntax"
CLS

Print "savepng:"; pass; " of"; tests; " passed"
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
