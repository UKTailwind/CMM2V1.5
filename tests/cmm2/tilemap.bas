' tilemap.bas - TILEMAP (the PicoMite's) on the CMM2: a 4-bit tileset in RAM slot 4, a map from a file, drawn into a
' page and checked pixel for pixel against the colours the tiles hold, drawn with PIXEL, in each colour depth
Dim integer pal(15) = (&H00,&HFF,&H4000,&H40ff,&H8000,&H80ff,&Hff00,&Hffff,&Hff0000,&Hff00FF,&Hff4000,&Hff40ff,&Hff8000,&Hff80ff,&Hffff00,&Hffffff)
Dim integer mp(11) = (1,2,0,1, 2,0,1,2, 0,1,2,1)
Dim integer m, a, x, y, bad, t, v, c, wx, wy, vx, vy, bk, i
Dim float tt
Dim md$(3) = ("7,8", "7,12", "7,16", "8,8")
Flash Load Image 4, "tilemap_tiles.bmp", O
a = MM.Info(FLASH ADDRESS 4)
Print "slot 4:"; Peek(Word a); " x"; Peek(Word a + 4); ", first bytes "; Hex$(Peek(Byte a + 8)); " "; Hex$(Peek(Byte a + 9))
Print "empty slot 5 header:"; Peek(Word MM.Info(FLASH ADDRESS 5))
For m = 0 To 3
  If m = 0 Then Mode 7,8
  If m = 1 Then Mode 7,12
  If m = 2 Then Mode 7,16
  If m = 3 Then Mode 8,8
  Tilemap Load "tilemap.map", 1, 4, 32, 32, 2
  For t = 0 To 1
    vx = t * 13 : vy = t * 7                                         ' a scrolled view: part tiles at the edges
    bk = RGB(128, 128, 128)
    Page Write 2 : CLS bk
    Tilemap Draw 1, F, vx, vy, 0, 0, 128, 96, 0                      ' value 0 transparent
    Page Write 3 : CLS bk
    For y = 0 To 95
      For x = 0 To 127
        wx = x + vx : wy = y + vy
        If wx < 128 And wy < 96 Then
          v = mp((wx \ 32) + (wy \ 32) * 4)
          If v > 0 Then
            If v = 1 Then c = ((wx Mod 32) + (wy Mod 32)) And 15 Else c = ((wx Mod 32) * 3 + (wy Mod 32) * 5) And 15
            If c > 0 Then Pixel x, y, pal(c) Or (m = 1) * &HF000000   ' (12-bit: opaque, as the tiles are)
          EndIf
        EndIf
      Next
    Next
    bad = 0
    For y = 0 To 95
      For x = 0 To 127
        Page Write 2 : v = Pixel(x, y)
        Page Write 3 : If Pixel(x, y) <> v Then bad = bad + 1
      Next
    Next
    Page Write 0
    Print "mode "; md$(m); " view"; vx; vy; ": wrong"; bad
  Next
  Tilemap Create big, 3, 4, 32, 32, 2, 10, 8
  Page Write 2
  Tilemap Draw 3, F, 5, 3, 0, 0, 320, 240, 0 : v = Pixel(0, 0)
  Print "  320x240 from 10x8 tiles, pixel(0,0) "; Hex$(v)
  Page Write 0 : CLS : Tilemap Draw 1, N, 0, 0, 0, 0, 128, 96          ' onto the display page, no transparency
  v = Pixel(33, 1) : Page Write 0
  Print "  display page, pixel(33,1) "; Hex$(v); " (tile 2, value 8: red)"
Next
Mode 8,8
Print "tile at 40,40:"; Tilemap(TILE 1, 40, 40); ", cols"; Tilemap(COLS 1); ", rows"; Tilemap(ROWS 1)
Tilemap Set 1, 1, 1, 7 : Print "after SET:"; Tilemap(TILE 1, 40, 40)
Print "collision at 0,0 32x32:"; Tilemap(COLLISION 1, 0, 0, 32, 32)
Tilemap Create mapdata, 2, 4, 32, 32, 2, 2, 2
Print "from DATA:"; Tilemap(TILE 2, 0, 0); Tilemap(TILE 2, 32, 0); Tilemap(TILE 2, 0, 32); Tilemap(TILE 2, 32, 32)
Tilemap Close
On Error Ignore
Print Tilemap(COLS 1)
Print "after CLOSE: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, "Tilemap"))
On Error Abort
End
mapdata:
Data 2, 1, 1, 2
big:
Data 1, 2, 1, 2, 1, 2, 1, 2, 1, 2
Data 2, 1, 2, 1, 2, 1, 2, 1, 2, 1
Data 1, 2, 1, 2, 1, 2, 1, 2, 1, 2
Data 2, 1, 2, 1, 2, 1, 2, 1, 2, 1
Data 1, 2, 1, 2, 1, 2, 1, 2, 1, 2
Data 2, 1, 2, 1, 2, 1, 2, 1, 2, 1
Data 1, 2, 1, 2, 1, 2, 1, 2, 1, 2
Data 2, 1, 2, 1, 2, 1, 2, 1, 2, 1
