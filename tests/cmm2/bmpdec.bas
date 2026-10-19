' bmpdec.bas - LOAD BMP through the PicoMite's decoder (M11 of the b14 merge): every kind of file it reads, made
' by gen_bmps.py with known pixels, and read back with PIXEL() in 32-bit colour; a sub-image, clipping at the left
' edge, and a file that is not a BMP.  The expected output was written from the pixels the images were made with.
Option Explicit
Dim string n$, r$
Dim integer w, h, x, y, i
Mode 8, 32
For i = 1 To 13
  Read n$, w, h
  CLS
  Load BMP n$, 300, 300
  Print n$; ":"
  For y = 0 To h - 1
    r$ = " "
    For x = 0 To w - 1 : r$ = r$ + " " + Hex$(Pixel(300 + x, 300 + y), 6) : Next
    Print r$
  Next
Next
CLS
Load BMP "bd_24", 300, 300, , 2, 1
Print "from 2, 1:"
For y = 0 To 2
  r$ = " "
  For x = 0 To 3 : r$ = r$ + " " + Hex$(Pixel(300 + x, 300 + y), 6) : Next
  Print r$
Next
CLS
Load BMP "bd_24", -2, 300
Print "at x -2:"
For y = 0 To 2
  r$ = " "
  For x = 0 To 3 : r$ = r$ + " " + Hex$(Pixel(x, 300 + y), 6) : Next
  Print r$
Next
CLS
On Error Skip 1
Load BMP "bd_bad"
Print "not a BMP: "; MM.ErrMsg$
Open "bmpdec.tmp" For Output As #10 : Close #10 : Kill "bmpdec.tmp"
Print "file 10 free"
Mode 8, 8
Print "done"
End
Data "bd_p1", 9, 3
Data "bd_p4", 7, 3
Data "bd_p8", 6, 3
Data "bd_p8v5", 6, 3
Data "bd_p8c4", 5, 2
Data "bd_grey", 5, 2
Data "bd_r8", 8, 3
Data "bd_r4", 8, 2
Data "bd_555", 5, 3
Data "bd_565", 5, 3
Data "bd_565v4", 5, 3
Data "bd_24", 5, 3
Data "bd_24td", 5, 3
