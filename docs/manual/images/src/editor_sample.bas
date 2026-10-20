' Bubbles: coloured circles drawn at random
' Press any key to stop
Const MAXR = 60                  ' the largest radius
Dim INTEGER count = 0

Cls
Do
  r = Rnd * 255 : g = Rnd * 255 : b = Rnd * 255
  x = Rnd * MM.HRes : y = Rnd * MM.VRes
  Circle x, y, Rnd * MAXR + 2,,, 0, RGB(r, g, b)
  count = count + 1
  If count Mod 100 = 0 Then Status count
Loop Until Inkey$ <> ""
End

' show how many bubbles have been drawn
Sub Status n As INTEGER
  Text 0, 0, "Bubbles: " + Str$(n), "LT", 1, 1, RGB(white), 0
End Sub
