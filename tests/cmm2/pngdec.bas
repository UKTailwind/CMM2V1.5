' pngdec.bas - LOAD PNG through the PicoMite's streaming decoder (with the CMM2's RGB, grey and grey-with-alpha), and
' BLIT LOADPNG through its whole-image wrapper: images made by tools/gen_pngs.py with known pixels, read back with PIXEL()
' in 32-bit colour (where DrawBuffer stores the alpha and draws every pixel), and the alpha cut in 16-bit colour, where
' pixels below it are not drawn. The expected output was written from the pixels the images were made with.
Option Explicit
Dim string n$, r$
Dim integer x, y, i, bg
Mode 8, 32
For i = 1 To 6
  Read n$
  CLS
  Load PNG n$, 300, 300
  Print n$; ":"
  For y = 0 To 2
    r$ = " "
    For x = 0 To 5 : r$ = r$ + " " + Hex$(Pixel(300 + x, 300 + y), 6) : Next
    Print r$
  Next
Next
CLS
Load PNG "pd_rgb", 300, 300, , , 2, 1
Print "from 2, 1:"
For y = 0 To 1
  r$ = " "
  For x = 0 To 3 : r$ = r$ + " " + Hex$(Pixel(300 + x, 300 + y), 6) : Next
  Print r$
Next
CLS
Load PNG "pd_rgb", -2, 300
r$ = " " : For x = 0 To 3 : r$ = r$ + " " + Hex$(Pixel(x, 300), 6) : Next
Print "at x -2:"; r$
CLS
Load PNG "pd_wide", 0, 300
r$ = "" : For i = 0 To 9 : r$ = r$ + " " + Hex$(Pixel(i * 64 + 32, 301), 6) : Next
Print "wider than the screen:"; r$; " "; Hex$(Pixel(639, 301), 6)
CLS
Load PNG "pd_rgb", 637, 300
Print "at x 637: "; Hex$(Pixel(637, 300), 6); " "; Hex$(Pixel(639, 300), 6)
CLS
Blit LoadPNG #1, "pd_rgb"
Blit Write #1, 300, 300
r$ = " " : For x = 0 To 5 : r$ = r$ + " " + Hex$(Pixel(300 + x, 301), 6) : Next
Print "BLIT LOADPNG:"; r$
Blit Close #1
Mode 8, 16
CLS RGB(0, 0, 255)
bg = Pixel(10, 10)
cut "", ""
cut "1", ""
cut "8", ""
cut "", "200"
CLS RGB(0, 0, 255)
Load PNG "pd_trns", 300, 300, 2
r$ = "" : For x = 0 To 3 : r$ = r$ + Choice(Pixel(300 + x, 300) <> bg, "1", "0") : Next
Print "indexed, transparent index 0, t 2: "; r$
Mode 8, 32
CLS
On Error Skip 1
Load PNG "pd_16"
Print "16 bits: "; MM.ErrMsg$
On Error Skip 1
Load PNG "bd_bad.bmp"
Print "not a PNG: "; MM.ErrMsg$
On Error Skip 1
Load PNG "pd_rgb", 0, 0, , , 6
Print "x offset 6: "; MM.ErrMsg$
Open "pngdec.tmp" For Output As #10 : Close #10 : Kill "pngdec.tmp"
Print "file 10 free"
CLS
End

' the seven alphas of pd_alpha drawn over blue in 16-bit colour: 1 where the red pixel was drawn
Sub cut t$, c$
  Local integer k
  Local string s$
  CLS RGB(0, 0, 255)
  If t$ = "" And c$ = "" Then Load PNG "pd_alpha", 300, 300
  If t$ <> "" Then Load PNG "pd_alpha", 300, 300, Val(t$)
  If c$ <> "" Then Load PNG "pd_alpha", 300, 300, , Val(c$)
  For k = 0 To 6 : s$ = s$ + Choice(Pixel(300 + k, 300) <> bg, "1", "0") : Next
  Print "alpha cut, t "; Choice(t$ = "", "none", t$); ", cutoff "; Choice(c$ = "", "none", c$); ": "; s$
End Sub

Data "pd_rgb", "pd_rgba", "pd_pal8", "pd_pal2", "pd_grey", "pd_greya"
