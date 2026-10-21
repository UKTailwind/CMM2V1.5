' Sprite speed in every colour depth (docs/Sprite_Plan.md, D1: today's engine against the refactored one): in MODE 8,
' 32 sprites of 32 x 32 moved together with NEXT and MOVE, one of 64 x 64 and one of 32 x 32 moved with SHOW, and
' SCROLL of the page.
' Prints milliseconds a frame.
Option Explicit
Option Default Integer
Dim img%(1023), big%(4095), px(31), py(31), dx(31), dy(31)
Dim depths(4) = (8, 12, 16, 32, 4)
Dim i, j, k, d, f, x, y
Dim float t, tmove, tshow, tshow32, tscroll
For y = 0 To 31
  For x = 0 To 31
    If (x - 15.5) ^ 2 + (y - 15.5) ^ 2 < 240 Then img%(y * 32 + x) = RGB(255, x * 8, y * 8)
  Next x
Next y
For y = 0 To 63
  For x = 0 To 63
    If (x + y) Mod 7 Then big%(y * 64 + x) = RGB(x * 4, 255 - y * 4, 128)
  Next x
Next y
Option Console Serial
For k = 0 To 4
  d = depths(k)
  On Error Skip
  Mode 8, d
  If MM.ErrNo Then
    Print "depth"; d; ": "; MM.ErrMsg$
    Continue For
  EndIf
  Cls
  For i = 0 To 15 : Box i * 40, 0, 20, 480, 0, RGB(0, 0, 100 + i * 8), RGB(0, 0, 100 + i * 8) : Next i
  Sprite LoadArray #1, 32, 32, img%()
  Sprite Copy #1, #2, 31
  Sprite LoadArray #40, 64, 64, big%()
  For i = 0 To 31
    px(i) = (i Mod 8) * 70 + 20 : py(i) = (i \ 8) * 100 + 20 : dx(i) = 1 + i Mod 3 : dy(i) = 1 + i Mod 2
    Sprite Show #i + 1, px(i), py(i), 1 + i Mod 4
  Next i
  t = Timer
  For f = 1 To 100
    For i = 0 To 31
      px(i) = px(i) + dx(i) : If px(i) < 0 Or px(i) > 600 Then dx(i) = -dx(i)
      py(i) = py(i) + dy(i) : If py(i) < 0 Or py(i) > 440 Then dy(i) = -dy(i)
      Sprite Next #i + 1, px(i), py(i)
    Next i
    Sprite Move
  Next f
  tmove = (Timer - t) / 100
  For i = 32 To 1 Step -1 : Sprite Hide #i : Next i
  t = Timer
  For f = 0 To 299
    Sprite Show #40, f, 200, 1
  Next f
  tshow = (Timer - t) / 300
  Sprite Hide #40
  t = Timer
  For f = 0 To 299
    Sprite Show #1, f, 300, 1
  Next f
  tshow32 = (Timer - t) / 300
  Sprite Hide #1
  t = Timer
  For f = 1 To 50
    Sprite Scroll 3, 1
  Next f
  tscroll = (Timer - t) / 50
  Print "depth"; d; ": MOVE 32 x 32x32 "; Str$(tmove, 1, 2); " ms, SHOW 64x64 "; Str$(tshow, 1, 3); " 32x32 "; Str$(tshow32, 1, 3); " ms, SCROLL "; Str$(tscroll, 1, 2); " ms"
  Sprite Close All
Next k
Mode 8
Option Console Both
