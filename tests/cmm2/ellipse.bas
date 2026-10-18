' ellipse.bas - a 1-pixel CIRCLE outline with an aspect over 1 has no gaps (the PicoMite's fix 79168bf): each outline is
' filled from its centre, and a gap would let the fill out to the corner of the page; in MODE 8,8 and MODE 7,4
Dim integer m, r, k, leaks, n
Dim float asp(4) = (1.0, 1.25, 2.0, 3.5, 6.0)
Dim md$(1) = ("8,8", "7,4")
Option Console Serial
For m = 0 To 1
  If m = 0 Then Mode 8,8 Else Mode 7,4
  leaks = 0 : n = 0
  For k = 0 To 4
    For r = 3 To 30 Step 3
      If r * asp(k) < MM.HRes \ 2 - 2 And r < MM.VRes \ 2 - 2 Then
        CLS
        Circle MM.HRes \ 2, MM.VRes \ 2, r, 1, asp(k), RGB(white)
        Fill MM.HRes \ 2, MM.VRes \ 2, RGB(red)
        If Pixel(0, 0) <> 0 Then leaks = leaks + 1 : Print "  leak: radius"; r; " aspect"; asp(k)
        n = n + 1
      EndIf
    Next
  Next
  Print "mode "; md$(m); ":"; n; " outlines,"; leaks; " leaking"
Next
Mode 8
