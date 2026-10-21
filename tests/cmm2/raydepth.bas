' raydepth.bas - RAY in every colour depth (docs/Ray_Plan.md, R2 and R3); the expected output is the G2's (the G1 has no
' 32-bit depth). A scene in the PicoMite's colours (walls of several types and patterns seen from both sides, a
' half-open door, sprites with transparent pixels, a patterned floor and ceiling, the minimap) is rendered in MODE 7
' at 4 bits, its pixels the reference, and its page's CRC the number to compare with a PicoMite's framebuffer. At 8,
' 12, 16 and 32 bits every pixel must be the reference's colour as that depth draws it (a PIXEL of it), on page 5 and
' on page 0, whose rows are doubled (each must equal the row under it). Then a scene in RGB colours is rendered at 32
' bits, and the 8, 12 and 16-bit renders are checked against it, every fourth pixel each way. Each frame is shown.
Option Explicit
Option Default Integer
Dim ref(76799), lut(15), pc(15), s32(4799), a%(1279), i, x, y, d, k, n
Dim depths(4) = (4, 8, 12, 16, 32)
Dim spr%(63)
Dim m$(9) Length 12
Restore pal : For i = 0 To 15 : Read pc(i) : pc(i) = pc(i) Or &H0F000000 : Next
Option Console Serial
Mode 7, 4
MakeSprites
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
Ray Camera 1.6, 8.3, -40, 66
Ray Door 6, 3, 0.5
Ray Sprite 0, 1, 4.5, 6.5
Ray Sprite 1, 2, 7.5, 5.5
Ray Sprite 2, 3, 3.5, 4.5
PicoScene

For d = 0 To 4
  On Error Skip
  Mode 7, depths(d)
  If MM.ErrNo Then
    Print "depth"; depths(d); ": "; MM.ErrMsg$
  Else
    Render 5
    If depths(d) = 4 Then
      Reference
      Print "depth 4: page CRC "; Hex$(PageCRC(5, 38400), 8)
      Render 0
    Else
      MakeLut
      Print "depth"; depths(d); ": page 5"; Diff(5); " different, page 0";
      Render 0
      Print Diff(0); " different,"; Doubled(); " rows not doubled"
    EndIf
    Pause 700
  EndIf
Next

' full colour: the 32-bit render the reference, the others checked against it through their own conversion
RgbScene
On Error Skip
Mode 7, 32
If MM.ErrNo Then
  Print "rgb: "; MM.ErrMsg$
Else
  Render 5
  k = 0
  For y = 0 To 239 Step 4 : For x = 0 To 319 Step 4 : s32(k) = Pixel(x, y, 5) : k = k + 1 : Next : Next
  Render 0 : Pause 700
  For d = 1 To 3
    Mode 7, depths(d)
    Render 5
    Page Write 6
    n = 0 : k = 0
    For y = 0 To 239 Step 4
      For x = 0 To 319 Step 4
        Pixel 0, 0, s32(k) Or &H0F000000
        If Pixel(x, y, 5) <> Pixel(0, 0, 6) Then n = n + 1
        k = k + 1
      Next
    Next
    Page Write 0
    Print "rgb depth"; depths(d); ":"; n; " of"; k; " different"
    Render 0 : Pause 700
  Next
  Mode 7, 4
  Render 5
  Print "rgb depth 4: page CRC "; Hex$(PageCRC(5, 38400), 8)
  Render 0 : Pause 700
EndIf
Ray Close
Mode 8
Option Console Both
End

' the scene and the minimap on page pg
Sub Render pg
  Page Write pg
  Ray Render
  Ray Minimap 2, 2, 48
  Page Write 0
End Sub

' the PicoMite's colours: no 0, which is BLACK (transparent in the 12-bit depth; the minimap's black is opaque)
Sub PicoScene
  Ray Colour 4, 1, 2, 3, 7, 12
  Ray Define 3, 8, 12, 5
  Ray Define 5, 1, 7, 9
  Ray Define 2, 14, 2, 13
  Ray Define 7, 13, 9, 20
End Sub

Sub RgbScene
  Ray Colour RGB(40, 90, 30), RGB(120, 160, 230), RGB(20, 60, 25), RGB(90, 120, 200), 7, 12
  Ray Define 1, RGB(160, 160, 170), RGB(110, 110, 120), 0
  Ray Define 3, RGB(200, 60, 40), RGB(90, 30, 20), 5
  Ray Define 5, RGB(30, 90, 200), RGB(230, 230, 250), 9
  Ray Define 2, RGB(250, 210, 60), RGB(60, 120, 60), 13
  Ray Define 7, RGB(180, 100, 220), RGB(70, 40, 90), 20
  Ray Define "A", RGB(255, 128, 0), RGB(0, 128, 128), 3
  Ray Define "G", RGB(140, 90, 40), RGB(220, 180, 120), 15, 1
  For i = 0 To 63 : spr%(i) = RGB(i * 4, 255 - i * 4, 100 + i) : Next
  Sprite LoadArray #4, 8, 8, spr%()
  Ray Sprite 1, 4, 7.5, 5.5
End Sub

' three 8 x 8 sprites in the PicoMite's colours, black transparent
Sub MakeSprites
  For i = 0 To 63 : spr%(i) = 0 : Next
  For i = 0 To 7
    spr%(i * 8 + 3) = RGB(RED) : spr%(i * 8 + 4) = RGB(RED)
    spr%(3 * 8 + i) = RGB(RED) : spr%(4 * 8 + i) = RGB(RED)
  Next
  Sprite LoadArray #1, 8, 8, spr%()
  For i = 0 To 63 : spr%(i) = 0 : Next
  For y = 0 To 7 : For x = 0 To 7
    If Abs(x - 3) + Abs(y - 3) <= 3 Then spr%(y * 8 + x) = RGB(YELLOW)
  Next : Next
  Sprite LoadArray #2, 8, 8, spr%()
  For i = 0 To 63
    If (i Mod 8) Mod 2 Then spr%(i) = RGB(CYAN) Else spr%(i) = RGB(255, 0, 255)
  Next
  Sprite LoadArray #3, 8, 8, spr%()
End Sub

' the 4-bit page 5's values, two pixels a byte
Sub Reference
  Local addr = MM.Info(PAGE ADDRESS 5), b
  For y = 0 To 239
    For x = 0 To 159
      b = Peek(Byte addr + y * 160 + x)
      k = y * 320 + x * 2
      ref(k) = b And 15 : ref(k + 1) = b >> 4
    Next
  Next
End Sub

' each of the PicoMite's colours as this depth draws it, read back
Sub MakeLut
  Page Write 6
  For i = 0 To 15 : Pixel 0, 0, pc(i) : lut(i) = Pixel(0, 0, 6) : Next
  Page Write 0
End Sub

' the pixels of page pg that are not the reference's colour
Function Diff(pg)
  Local j = 0, c = 0
  For y = 0 To 239
    For x = 0 To 319
      If Pixel(x, y, pg) <> lut(ref(j)) Then c = c + 1
      j = j + 1
    Next
  Next
  Diff = c
End Function

' page 0's rows (doubled in MODE 7) that differ from the row under them
Function Doubled()
  Local addr = MM.Info(PAGE ADDRESS 0), rb = 320 * Choice(depths(d) = 8, 1, Choice(depths(d) = 32, 4, 2)), c, e = 0
  For y = 0 To 239
    Memory Unpack addr + 2 * y * rb, a%(), rb, 8
    c = Math(CRC32 a%(), rb)
    Memory Unpack addr + (2 * y + 1) * rb, a%(), rb, 8
    If Math(CRC32 a%(), rb) <> c Then e = e + 1
  Next
  Doubled = e
End Function

' a CRC-32 of the first n bytes of page pg
Function PageCRC(pg, n)
  Local addr = MM.Info(PAGE ADDRESS pg), left = n, m, c = 0
  Do While left > 0
    m = Min(left, 1280)
    Memory Unpack addr, a%(), m, 8
    c = Math(CRC32 a%(), m, , c)
    addr = addr + m : left = left - m
  Loop
  PageCRC = c
End Function

pal:
Data &H000000, &H0000FF, &H004000, &H0040FF, &H008000, &H0080FF, &H00FF00, &H00FFFF
Data &HFF0000, &HFF00FF, &HFF4000, &HFF40FF, &HFF8000, &HFF80FF, &HFFFF00, &HFFFFFF
