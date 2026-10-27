' pmdialect.bas - the OPTION PICOMITE dialect items of 26 Oct 2026 (docs/PicoMite_Parity_ToDo.md), run under OPTION
' PICOMITE: MM.INFO(CPUSPEED) a string; LOAD PNG's t and cutoff the PicoMite's (pd_alpha.png: four pixels, red alpha
' 255, green alpha 10, blue alpha 25, white alpha 0; colours 1 and 15, the same in the VGA and HDMI palettes);
' GETSCANLINE counting the screen's lines from the first visible one, as the PicoMite's HDMI versions do (in MODE 2
' too). Each check reads the pixels before it prints (its line is on the top row).
Option Explicit
Option Default Integer
Dim tests, pass, i, mx, g
Dim s$

s$ = MM.Info(CpuSpeed)
Check "MM.INFO(CPUSPEED) is a string, the clock in Hz", Val(s$) >= 400000000, 1

Mode 2
CLS RGB(cyan)
Load Png "pd_alpha", 0, 0
CheckS "LOAD PNG with no t: alpha 20 or less drawn in colour 0", Px$(), "FF0000 000000 0000FF 000000"
CLS RGB(cyan)
Load Png "pd_alpha", 0, 0, -1
CheckS "  t -1: the screen behind kept", Px$(), "FF0000 00FFFF 0000FF 00FFFF"
CLS RGB(cyan)
Load Png "pd_alpha", 0, 0, 15, 30
CheckS "  t 15 (white), cutoff 30: the blue's 25 cut too", Px$(), "FF0000 FFFFFF FFFFFF FFFFFF"
CLS RGB(cyan)
Load Png "pd_alpha", 0, 0, 1, 5
CheckS "  t 1 (&H0000FF), cutoff 5: only the white's 0 cut", Px$(), "FF0000 00FF00 0000FF 0000FF"
On Error Skip 1 : Load Png "pd_alpha", 0, 0, 16
CheckS "  t 16: an error", MM.ErrMsg$, "16 is invalid (valid is -1 to 15)"

mx = 0
For i = 1 To 20000 : g = GetScanLine : If g > mx Then mx = g
Next
Check "GETSCANLINE in MODE 2 (240 lines shown twice): the screen's lines, the most 480 to 499", mx >= 480 And mx <= 499, 1
Mode 1
mx = 0
For i = 1 To 20000 : g = GetScanLine : If g > mx Then mx = g
Next
Check "  in MODE 1 (480 lines): the most 480 to 499", mx >= 480 And mx <= 499, 1

Print "pmdialect:"; pass; " of"; tests; " passed"
End

' the first four pixels of the top row, as hex
Function Px$()
  Local k
  For k = 0 To 3
    Px$ = Px$ + Hex$(Pixel(k, 0), 6) + Choice(k < 3, " ", "")
  Next
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
