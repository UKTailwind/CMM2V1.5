' raybench.bas - RAY RENDER's time (docs/Ray_Plan.md, section 4): MODE 7, 8 and 1 in each colour depth, the scene of
' tests/cmm2/raydepth.bas turning a full circle in 36 frames. Rendered on the highest page (not shown, rows not
' doubled), then on page 0 (shown: uncached, and in MODE 7 its rows doubled); and PAGE COPY of the first to page 0.
Option Explicit
Option Default Integer
Dim modes(2) = (7, 8, 1), depths(4) = (4, 8, 12, 16, 32), i, j, k, pg
Dim float t1, t2, t3
Dim m$(9) Length 12
m$(0) = "111111111111"
m$(1) = "1000000000A1"
m$(2) = "103300000001"
m$(3) = "103300G00001"
m$(4) = "100000100001"
m$(5) = "1000000002B1"
m$(6) = "150000000001"
m$(7) = "100000700001"
m$(8) = "1000000000C1"
m$(9) = "111111111111"
Ray Map 12, 10, m$()
Ray Colour 4, 1, 2, 3, 7, 12
Ray Define 3, 8, 12, 5
Ray Door 6, 3, 0.5
Option Console Serial
Print "mode  depth  page n (ms)  page 0 (ms)  PAGE COPY (ms)"
For i = 0 To 2
  For j = 0 To 4
    On Error Skip
    Mode modes(i), depths(j)
    If MM.ErrNo Then
      Print modes(i), depths(j), "-"
    Else
      pg = MM.Info(MAX PAGES)
      Ray Camera 5.5, 5.5, 0, 66
      Page Write pg
      t1 = Timer
      For k = 1 To 36 : Ray Turn 10 : Ray Render : Next
      t1 = (Timer - t1) / 36
      Page Write 0
      t2 = Timer
      For k = 1 To 36 : Ray Turn 10 : Ray Render : Next
      t2 = (Timer - t2) / 36
      t3 = Timer
      For k = 1 To 10 : Page Copy pg To 0 : Next
      t3 = (Timer - t3) / 10
      Print modes(i), depths(j), Str$(t1, 4, 2), Str$(t2, 4, 2), Str$(t3, 4, 2)
    EndIf
  Next
Next
Ray Close
Mode 8
Option Console Both
End
