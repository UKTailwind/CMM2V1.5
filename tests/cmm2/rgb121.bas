' rgb121.bas - MODE n,4, the PicoMite's RGB121 display: its pages' bytes as the PicoMite's framebuffers, the drawing
' set, the PicoMite's FRAMEBUFFER on them, the merge of N, L and T onto the display, BLIT, BLIT FLASH, PAGE COPY and
' MAP, in MODE 7,4 (the PicoMite's MODE 2, rows doubled) and MODE 8,4 (its MODE 3)
Dim integer pal(15) = (&H00,&HFF,&H4000,&H40ff,&H8000,&H80ff,&Hff00,&Hffff,&Hff0000,&Hff00FF,&Hff4000,&Hff40ff,&Hff8000,&Hff80ff,&Hffff00,&Hffffff)
Dim integer m, a, d, i, x, y, bad, v, n, w, h, s, sw, sh, st, c
Dim integer row(39)
Dim md$(1) = ("7,4", "8,4")
Option Console Serial                                              ' (the text not drawn over what is checked)
Flash Load Image 4, "tilemap_tiles.bmp", O
s = MM.Info(FLASH ADDRESS 4) : sw = Peek(Word s) : sh = Peek(Word s + 4) : st = (sw + 1) \ 2
d = &H24000000                                                     ' the display the merge writes (LIST PAGES)
For m = 0 To 1
  If m = 0 Then Mode 7,4 Else Mode 8,4
  w = MM.HRes : h = MM.VRes
  Print "mode "; md$(m); ":"; MM.Info(MODE); w; h; ", N at &H"; Hex$(MM.Info(WRITEBUFF))
  a = MM.Info(WRITEBUFF)
  ' -------------------------------------------------- the bytes: two pixels each, the left one in the low nibble
  CLS RGB(255, 0, 0)
  Print "  CLS red: "; Hex$(Peek(Byte a)); " "; Hex$(Peek(Byte a + w \ 2 * h - 1))
  Pixel 1, 0, RGB(0, 0, 255)
  Print "  pixel 1,0 blue: "; Hex$(Peek(Byte a)); ", Pixel( "; Hex$(Pixel(0, 0)); " "; Hex$(Pixel(1, 0))
  Poke Byte a + w \ 2, &H5A
  Print "  poked &H5A: "; Hex$(Pixel(0, 1)); " "; Hex$(Pixel(1, 1))
  Print "  MAP(5) "; Hex$(Map(5)); ", MAP(13) "; Hex$(Map(13))
  ' -------------------------------------------------- the drawing set: odd and even ends, clipping
  CLS
  Box 11, 10, 7, 3, 1, RGB(0, 255, 0), RGB(0, 255, 0)
  Box w - 3, 20, 10, 2, 1, RGB(0, 255, 0), RGB(0, 255, 0)
  Line 0, 30, w - 1, 30, 1, RGB(255, 255, 255)
  Line 5, 40, 25, 50, 1, RGB(255, 255, 255)
  Circle 60, 60, 9, 1, 1, RGB(255, 0, 255), RGB(255, 0, 255)
  Text 100, 40, "Hi", , 1, 1, RGB(255, 255, 0), RGB(0, 0, 255)
  For i = 0 To 15 : row(i) = 0 : Next
  For y = 0 To 79 : For x = 0 To w - 1                               ' (the values, from the bytes)
    v = (Peek(Byte a + y * (w \ 2) + x \ 2) >> ((x And 1) * 4)) And 15 : row(v) = row(v) + 1
  Next : Next
  Print "  counts: green"; row(6); " white"; row(15); " magenta"; row(9); " yellow"; row(14); " blue"; row(1)
  ' -------------------------------------------------- the PicoMite's framebuffers, and the merge onto the display
  CLS RGB(0, 0, 255)
  Framebuffer Layer 9
  Framebuffer Write L
  Print "  layer cleared to 9: "; Hex$(Peek(Byte MM.Info(WRITEBUFF))); ", at &H"; Hex$(MM.Info(WRITEBUFF))
  Box 0, 0, 4, 1, 1, RGB(255, 255, 255), RGB(255, 255, 255)
  Framebuffer Wait : Framebuffer Wait
  Print "  display, L over N: ";
  For i = 0 To 5 : Print Hex$(Peek(Byte d + i), 2); " "; : Next
  Print "row 1 "; Hex$(Peek(Byte d + w), 2)
  Framebuffer Layer Top
  Framebuffer Write T : Pixel 0, 0, RGB(255, 255, 0)
  Framebuffer Wait : Framebuffer Wait
  Print "  display, T over L: "; Hex$(Peek(Byte d), 2); " "; Hex$(Peek(Byte d + 1), 2)
  Framebuffer Write N : Print "  N itself: "; Hex$(Pixel(0, 0))
  On Error Ignore
  Framebuffer Layer : Print "  a second layer: "; em$()
  Framebuffer Write F : Print "  write F: "; em$()
  On Error Abort
  Framebuffer Close L : Framebuffer Close T
  Framebuffer Wait : Framebuffer Wait
  Print "  closed: "; Hex$(Peek(Byte d), 2); " "; Hex$(Peek(Byte d + w * 2 - 1), 2)
  Framebuffer Create : Framebuffer Write F : CLS RGB(0, 255, 0)
  Framebuffer Create 2 : Framebuffer Write 2 : CLS : Pixel 2, 0, RGB(255, 0, 255)
  Framebuffer Merge 0, B
  Framebuffer Write N : Print "  merge F and 2: "; Hex$(Pixel(0, 0)); " "; Hex$(Pixel(2, 0))
  Framebuffer Copy F, N, B : Print "  copy F to N: "; Hex$(Pixel(2, 0))
  On Error Ignore
  Framebuffer Copy L, N : Print "  copy L, not created: "; em$()
  On Error Abort
  Framebuffer Close
  ' -------------------------------------------------- BLIT FLASH: the slot's values themselves, aligned or not
  CLS RGB(255, 255, 255)
  Blit Flash 4, N, 0, 0, 11, 5, 9, 3                                ' (even source, odd destination)
  Blit Flash 4, N, 1, 2, 31, 5, 9, 3                                ' (odd to odd)
  Blit Flash 4, N, 3, 4, 51, 5, 9, 3, 0                             ' (value 0 left alone)
  bad = 0
  For y = 0 To 2 : For x = 0 To 8
    v = (Peek(Byte s + 8 + y * st + x \ 2) >> ((x And 1) * 4)) And 15
    If Pixel(11 + x, 5 + y) <> pal(v) Then bad = bad + 1
    v = (Peek(Byte s + 8 + (y + 2) * st + (x + 1) \ 2) >> (((x + 1) And 1) * 4)) And 15
    If Pixel(31 + x, 5 + y) <> pal(v) Then bad = bad + 1
    v = (Peek(Byte s + 8 + (y + 4) * st + (x + 3) \ 2) >> (((x + 3) And 1) * 4)) And 15
    If v = 0 Then v = 15
    If Pixel(51 + x, 5 + y) <> pal(v) Then bad = bad + 1
  Next : Next
  Print "  BLIT FLASH wrong:"; bad
  ' -------------------------------------------------- BLIT on the page, along a row (overlapping), and BLIT READ/WRITE
  For x = 0 To 39 : row(x) = Pixel(11 + (x Mod 9), 5) : Pixel x, 100, row(x) : Next
  Blit 0, 100, 5, 100, 30, 1
  bad = 0
  For x = 0 To 29 : If Pixel(5 + x, 100) <> row(x) Then bad = bad + 1
  Next
  For x = 0 To 39 : Pixel x, 101, row(x) : Pixel x, 102, row(x) : Next
  Blit 1, 101, 5, 101, 29, 1                                         ' (odd to odd, to the right: bytes)
  Blit 3, 102, 1, 102, 29, 1                                         ' (to the left, an odd end)
  For x = 0 To 28
    If Pixel(5 + x, 101) <> row(1 + x) Then bad = bad + 1
    If Pixel(1 + x, 102) <> row(3 + x) Then bad = bad + 1
  Next
  Blit 11, 5, 101, 151, 9, 3, , 3                                   ' (turned both ways)
  For y = 0 To 2 : For x = 0 To 8 : If Pixel(101 + 8 - x, 151 + 2 - y) <> Pixel(11 + x, 5 + y) Then bad = bad + 1
  Next : Next
  Blit Read #1, 31, 5, 9, 3 : Blit Write #1, 200, 150, 0 : Blit Close #1
  For y = 0 To 2 : For x = 0 To 8 : If Pixel(200 + x, 150 + y) <> Pixel(31 + x, 5 + y) Then bad = bad + 1
  Next : Next
  Print "  BLIT wrong:"; bad
  ' -------------------------------------------------- PAGE COPY, PAGE WRITE of the other 4-bit pages
  Page Copy 0 To 5
  bad = 0
  For i = 0 To w \ 2 * 8 - 1 : If Peek(Byte MM.Info(PAGE ADDRESS 5) + i) <> Peek(Byte a + i) Then bad = bad + 1
  Next
  Page Write 5 : Print "  page 5 at &H"; Hex$(MM.Info(WRITEBUFF)); ", copy wrong"; bad
  Page Write 0
  On Error Ignore
  Page Display 1 : Print "  PAGE DISPLAY: "; em$()
  Image Rotate 0, 0, 8, 8, 10, 10, 45 : Print "  IMAGE: "; em$()
  On Error Abort
Next
Mode 8
Print "done"

Function em$()
  em$ = MM.ErrMsg$
End Function
