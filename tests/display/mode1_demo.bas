' mode1_demo.bas - MODE n,1, the PicoMite's MODE 1, to look at. Each part waits for a key.
'  1. MODE 8,1 (640x480): coloured text (it sets the tiles it lines up with), bands of tiles set by TILE behind white
'     circles (drawn in each tile's foreground colour), and a box moving on FRAMEBUFFER LAYER, ORed over N.
'  2. MODE 11,1 (1280x720, its display in SDRAM): the same.
'  3. MODE 8,1: every tile's background a different colour, more than the 256 the CLUT holds (past 256 the nearest).
Dim integer c(7) = (RGB(red), RGB(green), RGB(blue), RGB(yellow), RGB(cyan), RGB(magenta), RGB(white), RGB(255,128,0))
Dim integer m, w, h, tx, ty, x, y, i, dx
Dim md$(1) = ("8,1", "11,1")
For m = 0 To 1
  If m = 0 Then Mode 8,1 Else Mode 11,1
  Font 1 : Tile Height 12
  w = MM.HRes : h = MM.VRes : tx = w \ 8 : ty = h \ 12
  CLS
  Colour RGB(yellow), RGB(blue) : Print "MODE "; md$(m); ": the PicoMite's MODE 1,"; w; " x"; h
  Colour RGB(white), RGB(black) : Print "Each 8 x 12 tile has its own colours. Drawing only sets or clears bits."
  Colour RGB(green), RGB(black) : Print "The box moves on FRAMEBUFFER LAYER, ORed over N.  Press a key."
  Colour RGB(white), RGB(black)
  For y = 3 To ty - 1 : For x = 0 To tx - 1 Step 4
    Tile x, y, c((x \ 4 + y) Mod 8), RGB(0, 0, (y * 4) Mod 128), 4, 1
  Next : Next
  For i = 1 To 40
    Circle Rnd * w, 48 + Rnd * (h - 60), 10 + Rnd * 40, 2, 1, RGB(white)
  Next
  Framebuffer Layer
  Framebuffer Write L
  x = 0 : dx = 4
  Do
    Box x, h \ 2 - 20, 40, 40, 1, RGB(white), RGB(white)
    Framebuffer Wait
    Box x, h \ 2 - 20, 40, 40, 1, 0, 0
    x = x + dx : If x < 0 Or x > w - 41 Then dx = -dx
  Loop Until Inkey$ <> ""
  Framebuffer Close
Next
Mode 8,1
Font 1 : Tile Height 12
tx = MM.HRes \ 8 : ty = MM.VRes \ 12
For y = 0 To ty - 1 : For x = 0 To tx - 1
  Tile x, y, RGB(255, 255, 255), RGB(x * 3, y * 6, 255 - x * 3)
Next : Next
Text 8, 12, "4800 tiles, each its own colour", "LT", 1, 1, RGB(white), -1
Do : Loop Until Inkey$ <> ""
Mode 8
