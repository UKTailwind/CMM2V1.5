' mode1.bas - MODE n,1, the PicoMite's MODE 1: its pages' bits as the PicoMite's framebuffer (the leftmost pixel in
' bit 0), the tiles' colours (TILE, text in line with them, CLS), the drawing set, the PicoMite's FRAMEBUFFER (L ORed
' into N, MERGE F OR 2), POKE, BLIT, SPRITE and PAGE COPY, and the merge onto the display, in MODE 8,1 (the PicoMite's
' MODE 1 at 640x480), MODE 7,1 (rows doubled) and MODE 11,1 (1280x720: the display in SDRAM)
Dim integer m, a, d, i, x, y, bad, v, n, w, h, sc, nt, tf, tb, s, gA, iA
Dim integer row(39)
Dim md$(2) = ("8,1", "7,1", "11,1")
Option Console Serial                                              ' (the text not drawn over what is checked)
For m = 0 To 2
  If m = 0 Then Mode 8,1
  If m = 1 Then Mode 7,1
  If m = 2 Then Mode 11,1
  w = MM.HRes : h = MM.VRes : a = MM.Info(WRITEBUFF)
  Print "mode "; md$(m); ":"; MM.Info(MODE); w; h; ", tile height"; MM.Info(TILE HEIGHT)
  Font 1 : Tile Height 12
  sc = 1 : If m = 1 Then sc = 2                                    ' (MODE 7 shows each row twice)
  nt = (w \ 8) * ((h + 7) \ 8)                                     ' the tiles' colours, then the display (Mono.c)
  tf = &H24000000 : tb = tf + nt
  d = tf + ((2 * nt + 8191) \ 8192) * 8192
  If d + w * h * sc > &H24080000 Then d = &HD0000000
  ' -------------------------------------------------- the bits: the leftmost pixel of each 8 in bit 0
  CLS
  Pixel 0, 0, RGB(255, 0, 0) : Pixel 9, 0, RGB(0, 0, 1) : Pixel 15, 1, RGB(255, 255, 255)
  Print "  bits:"; Peek(Byte a); Peek(Byte a + 1); Peek(Byte a + w \ 8 + 1);
  Pixel 9, 0, 0 : Print Peek(Byte a + 1)
  Print "  Pixel(): "; Hex$(Pixel(0, 0)); " "; Hex$(Pixel(1, 0))
  Poke Byte a + 2, &H81 : Print "  poked:"; px(16, 0); px(17, 0); px(23, 0)
  ' -------------------------------------------------- TILE
  Tile 0, 0, RGB(255, 0, 0), RGB(0, 0, 255)
  Tile 1, 0, , RGB(0, 255, 0), 3, 2
  Tile 4, 0, RGB(128, 128, 128)
  Pixel 32, 0, RGB(255, 255, 255)
  Print "  tiles: "; Hex$(Pixel(0, 0)); " "; Hex$(Pixel(1, 0)); " "; Hex$(Pixel(8, 0)); " "; Hex$(Pixel(31, 23)); " "; Hex$(Pixel(31, 24)); " "; Hex$(Pixel(32, 0))
  On Error Ignore
  Tile w \ 8, 0, RGB(255, 0, 0) : Print "  off the edge: "; em$()
  Tile 0, 0 : Print "  no colours: "; em$()
  Tile 0, 0, RGB(255, 0, 0), , w \ 8 + 1 : Print "  too wide: "; em$()
  On Error Abort
  Tile Reset : Print "  reset: "; Hex$(Pixel(0, 0)); " "; Hex$(Pixel(8, 0))
  ' -------------------------------------------------- text: in line with the tiles it sets them
  CLS
  Text 16, 12, "AB", "LT", 1, 1, RGB(255, 255, 0), RGB(0, 0, 255)
  bad = 0 : n = 0
  For y = 12 To 23 : For x = 16 To 31
    v = Pixel(x, y)
    If v = &HFFFF00 Then n = n + 1
    If v <> &HFFFF00 And v <> &HFF Then bad = bad + 1
  Next : Next
  Print "  text in line: yellow"; n; ", neither"; bad; ", the tile before it "; Hex$(Pixel(15, 12))
  Text 64, 36, "A", "LT", 1, 1, RGB(255, 255, 255), 0 : gA = cell(64, 36)
  Text 3, 40, "A", "LT", 1, 1, 0, RGB(255, 0, 0) : iA = cell(3, 40)
  Print "  black on red, not in line (inverted):"; gA; iA; gA + iA
  Tile 10, 3, RGB(0, 255, 0), RGB(0, 0, 255)
  Text 80, 36, "A", "LT", 1, 1, RGB(255, 0, 0), -1
  Print "  transparent background, in line: wrong"; cellcheck(80, 36, &HFF0000, &HFF); cell(80, 36)
  Text 96, 36, "A", "LT", 1, 1, 0, RGB(0, 255, 0)
  Print "  black on green, in line: wrong"; cellcheck(96, 36, 0, &HFF00); cell(96, 36)
  ' -------------------------------------------------- CLS: N cleared, every tile in COLOUR's foreground on the colour
  Colour RGB(255, 255, 0), 0
  CLS RGB(0, 0, 255)
  s = 0 : For i = 0 To 99 : s = s + Peek(Byte a + i) : Next
  Pixel 5, 5, RGB(255, 255, 255)
  Print "  CLS blue:"; s; " "; Hex$(Pixel(5, 5)); " "; Hex$(Pixel(6, 5))
  Colour RGB(255, 255, 255), 0
  ' -------------------------------------------------- the drawing set
  CLS
  Box 3, 2, 13, 4, 1, RGB(255, 0, 0), RGB(255, 0, 0)
  Line 0, 30, w - 1, 30, 1, RGB(255, 255, 255)
  Circle 60, 60, 9, 1, 1, RGB(255, 0, 255), RGB(255, 0, 255)
  Box 20, 2, 5, 5, 1, RGB(255, 255, 255), 0
  n = 0 : For i = 0 To (w \ 8) * 80 - 1 : n = n + pc(Peek(Byte a + i)) : Next
  Print "  set:"; n - w
  ' -------------------------------------------------- the PicoMite's framebuffers, and the merge onto the display
  CLS
  Framebuffer Layer 5
  Framebuffer Write L : Print "  layer:"; Peek(Byte MM.Info(WRITEBUFF)); (MM.Info(WRITEBUFF) <> a)
  Pixel 0, 0, RGB(255, 255, 255)
  Framebuffer Write N : Pixel 2, 0, RGB(255, 255, 255)
  Framebuffer Wait : Framebuffer Wait
  bad = 0
  For x = 0 To 15 : If shown(x, 0) <> want(x, 0, x = 0 Or x = 2) Then bad = bad + 1
  Next
  Print "  N"; Peek(Byte a); ", shown with L ORed in: wrong"; bad; ", rows doubled"; (Peek(Byte d + (sc - 1) * w) = Peek(Byte d))
  Framebuffer Layer Top : Framebuffer Write T : Pixel 1, 0, RGB(255, 255, 255)
  Framebuffer Wait : Framebuffer Wait
  Print "  T not shown:"; (shown(1, 0) = want(1, 0, 0))
  Framebuffer Write L : CLS : Framebuffer Write N
  Framebuffer Wait : Framebuffer Wait
  Print "  L cleared:"; (shown(0, 0) = want(0, 0, 0)); (shown(2, 0) = want(2, 0, 1))
  On Error Ignore
  Framebuffer Layer : Print "  a second layer: "; em$()
  Framebuffer Write F : Print "  write F: "; em$()
  Blit Framebuffer N, L, 0, 0, 0, 0, 8, 8 : Print "  BLIT FRAMEBUFFER: "; em$()
  On Error Abort
  Framebuffer Close L : Framebuffer Close T
  Framebuffer Create : Framebuffer Write F : CLS : Pixel 0, 0, RGB(255, 255, 255)
  Framebuffer Create 2 : Framebuffer Write 2 : CLS : Pixel 3, 0, RGB(255, 255, 255)
  Framebuffer Merge 7, B
  Framebuffer Write N : Print "  merge F and 2:"; Peek(Byte a);
  Framebuffer Copy F, N, B : Print ", copy F to N:"; Peek(Byte a)
  Framebuffer Close
  CLS : Poke Byte a + 3, &HFF : Framebuffer Wait : Framebuffer Wait
  Print "  POKE shown:"; (shown(24, 0) = want(24, 0, 1)); (shown(31, 0) = want(31, 0, 1)); (shown(32, 0) = want(32, 0, 0))
  ' -------------------------------------------------- BLIT, SPRITE, PAGE COPY
  CLS
  For x = 0 To 39 : If (x * 7) Mod 5 < 2 Then Pixel x, 100, RGB(255, 255, 255)
  Next
  For x = 0 To 39 : row(x) = px(x, 100) : Pixel x, 101, row(x) * &HFFFFFF : Pixel x, 102, row(x) * &HFFFFFF : Next
  Blit 0, 100, 5, 100, 30, 1                                         ' (overlapping, to the right)
  Blit 3, 102, 1, 102, 29, 1                                         ' (to the left)
  Blit 0, 101, 101, 151, 9, 1, , 1                                   ' (mirrored)
  bad = 0
  For x = 0 To 29 : If px(5 + x, 100) <> row(x) Then bad = bad + 1
  Next
  For x = 0 To 28 : If px(1 + x, 102) <> row(3 + x) Then bad = bad + 1
  Next
  For x = 0 To 8 : If px(101 + 8 - x, 151) <> row(x) Then bad = bad + 1
  Next
  Print "  BLIT wrong:"; bad;
  Tile 0, 10, RGB(255, 0, 0), RGB(0, 0, 255), 2, 1
  Blit 0, 120, 32, 144, 16, 12                                       ' (in line with the tiles: their colours go too)
  Print ", tiles with it: "; Hex$(Pixel(33, 145)); " "; Hex$(Pixel(48, 145))
  For x = 0 To 15 : row(x) = px(x, 100) : Next
  Sprite Read #1, 0, 100, 16, 1 : Sprite Show #1, 40, 110, 1
  bad = 0 : For x = 0 To 15 : If px(40 + x, 110) <> row(x) Then bad = bad + 1
  Next
  Sprite Close #1
  Print "  SPRITE wrong:"; bad; ", closed:"; px(40 + 5, 110) + px(40 + 7, 110) + px(40 + 12, 110)
  Page Copy 0 To 5
  bad = 0
  For i = 0 To (w \ 8) * 160 - 1 : If Peek(Byte MM.Info(PAGE ADDRESS 5) + i) <> Peek(Byte a + i) Then bad = bad + 1
  Next
  Page Write 5 : Print "  PAGE COPY wrong"; bad; ", page 5 written:"; (MM.Info(WRITEBUFF) = MM.Info(PAGE ADDRESS 5))
  Page Write 0
  ' -------------------------------------------------- what MODE 1 refuses
  On Error Ignore
  Map Reset : Print "  MAP: "; em$()
  v = Map(1) : Print "  MAP(): "; em$()
  Image Rotate 0, 0, 8, 8, 10, 10, 45 : Print "  IMAGE: "; em$()
  Page Display 1 : Print "  PAGE DISPLAY: "; em$()
  On Error Abort
Next
Mode 8
On Error Ignore : Tile 0, 0, RGB(255, 0, 0) : Print "TILE in MODE 8: "; em$() : On Error Abort
tile = 3 : Print "a variable called tile:"; tile
arr
Print "done"

Sub arr
  Local integer tile(3)
  tile(2) = 5 : Print "an array called tile:"; tile(2)
End Sub

Function em$()
  em$ = MM.ErrMsg$
End Function

Function px(x, y)                                                 ' pixel x, y of N: 1 set, 0 clear
  px = (Peek(Byte a + y * (w \ 8) + x \ 8) >> (x And 7)) And 1
End Function

Function shown(x, y)                                               ' the display's byte for pixel x, y: a CLUT entry
  shown = Peek(Byte d + y * sc * w + x)
End Function

Function want(x, y, set)                                           ' the entry pixel x, y shows, set or not: its tile's
  Local integer t = (y \ 12) * (w \ 8) + x \ 8
  If set Then want = Peek(Byte tf + t) Else want = Peek(Byte tb + t)
End Function

Function pc(b)                                                     ' the bits set in a byte
  Local integer k
  pc = 0
  For k = 0 To 7 : pc = pc + ((b >> k) And 1) : Next
End Function

Function cell(x0, y0)                                              ' the set pixels of an 8 x 12 cell
  Local integer xx, yy
  cell = 0
  For yy = y0 To y0 + 11 : For xx = x0 To x0 + 7 : cell = cell + px(xx, yy) : Next : Next
End Function

Function cellcheck(x0, y0, fg, bg)                                 ' the cell's pixels not fg where set, bg where clear
  Local integer xx, yy
  cellcheck = 0
  For yy = y0 To y0 + 11 : For xx = x0 To x0 + 7
    If px(xx, yy) Then
      If Pixel(xx, yy) <> fg Then cellcheck = cellcheck + 1
    Else
      If Pixel(xx, yy) <> bg Then cellcheck = cellcheck + 1
    EndIf
  Next : Next
End Function
