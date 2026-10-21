' A platform game in miniature: the PicoMite's static objects and SPRITE(B) together (Peter, 21 Oct: "A good test of
' statics is to use it combined with SPRITE(B) to create a simple platform type game demo").
'
' Everything solid is drawn. The ground and the platforms are static objects too: the player lands on one when it
' meets it from above (SPRITE(C) lists it as &H80 + its number). Anything drawn that the player's shape runs into
' stops it (SPRITE(B) = 2): a step across into the red wall, or a fall onto its top, is taken back.
' RUN "platform" plays a scripted run and prints what happens (tests/cmm2); RUN "platform", play takes the arrow keys
' and Space (Q to stop).
Option Explicit
Option Default Integer
Const PW = 16, PH = 24, GRAVITY = 1, JUMP = -15
Dim img%(PW * PH - 1), px = 40, py = 300, vx, vy, onground, frame, k, m, e, nx, ny, played, landed
Dim keys$, act$
Dim integer plat(3, 3) = (0, 440, 640, 40, 150, 360, 100, 12, 300, 300, 100, 12, 450, 240, 110, 12)
played = Instr(UCase$(MM.CmdLine$), "PLAY") > 0
MakePlayer
Mode 8, 16
Cls
For k = 0 To 3                                             ' the ground and the platforms, drawn and declared
  Box plat(0, k), plat(1, k), plat(2, k), plat(3, k), 0, RGB(GREEN), RGB(GREEN)
  Sprite Static #k + 1, plat(0, k), plat(1, k), plat(2, k), plat(3, k)
Next k
Box 270, 380, 16, 60, 0, RGB(RED), RGB(RED)               ' a wall, only drawn
Sprite LoadArray #1, PW, PH, img%()
Sprite Show #1, px, py, 1
If played Then Option Console Serial
Restore script
Read frame, act$
For k = 0 To Choice(played, 9999999, 400)                  ' (play: until Q)
  If played Then
    keys$ = ""
    If KeyDown(1) = 130 Or KeyDown(2) = 130 Then keys$ = keys$ + "L"
    If KeyDown(1) = 131 Or KeyDown(2) = 131 Then keys$ = keys$ + "R"
    If KeyDown(1) = 32 Or KeyDown(2) = 32 Then keys$ = keys$ + "J"
    If KeyDown(1) = Asc("q") Then Exit For
    Pause 20
  ElseIf k = frame Then
    keys$ = act$
    Read frame, act$
  EndIf
  Step1
Next k
If played Then Option Console Both
Print "end at"; px; py; " on the ground"; onground
Sprite Close All
Mode 8
End

' one frame: across, then down or up
Sub Step1
  vx = 0
  If Instr(keys$, "L") > 0 Then vx = -4
  If Instr(keys$, "R") > 0 Then vx = 4
  If Instr(keys$, "J") > 0 And onground Then vy = JUMP : onground = 0 : Say "jump"
  vy = Min(vy + GRAVITY, 10)
  nx = Max(0, Min(640 - PW, px + vx))                      ' across: into the scenery, the step is taken back
  If nx <> px Then
    Sprite Show #1, nx, py, 1
    If Sprite(B, 1) = 2 Then
      Say "wall, from the " + Choice(Sprite(B, 1, 5) > 0, "left", "right")
      nx = px
    EndIf
  EndIf
  ny = py + vy : landed = 0                                ' down or up: a platform met from above is landed on
  Sprite Show #1, nx, ny, 1
  For m = 1 To Sprite(C, 1)
    e = Sprite(C, 1, m)
    If e >= &H80 And e < &HF0 Then
      If vy >= 0 And py + PH <= Sprite(ST, e - &H80, Y) Then ny = Sprite(ST, e - &H80, Y) - PH : landed = e - &H80
    EndIf
  Next m
  If landed Then
    Sprite Show #1, nx, ny, 1
    vy = 0
    If Not onground Then Say "landed on" + Str$(landed)
    onground = 1
  ElseIf Sprite(B, 1) = 2 Then                             ' the scenery: back out, a pixel at a time
    Do While ny <> py And Sprite(B, 1) = 2
      ny = ny - Sgn(vy)
      Sprite Show #1, nx, ny, 1
    Loop
    If vy > 0 And Not onground Then Say "landed on the wall"
    If vy > 0 Then onground = 1
    vy = 0
  ElseIf vy > 1 Then
    onground = 0                                           ' in the air (walked off an edge)
  EndIf
  px = nx : py = ny
End Sub

Sub Say s$
  If Not played Then Print "frame"; k; ": "; s$; " at"; nx; ny
End Sub

' the player: a yellow block with eyes and black (transparent) corners
Sub MakePlayer
  Local x, y
  For y = 0 To PH - 1
    For x = 0 To PW - 1
      If Not ((x < 2 Or x > PW - 3) And (y < 2 Or y > PH - 3)) Then img%(y * PW + x) = RGB(YELLOW)
    Next x
  Next y
  img%(5 * PW + 4) = RGB(BLUE) : img%(5 * PW + 11) = RGB(BLUE)
End Sub

' frame, keys from then on: L left, R right, J jump
script:
Data 0, ""
Data 20, "R"
Data 80, "J"
Data 81, ""
Data 95, "R"
Data 101, ""
Data 115, "L"
Data 167, ""
Data 169, "RJ"
Data 170, "R"
Data 206, "RJ"
Data 207, "R"
Data 243, "RJ"
Data 244, "R"
Data 9999, ""
