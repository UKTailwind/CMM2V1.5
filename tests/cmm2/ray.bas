' ray.bas - RAY, the PicoMite's raycaster (docs/Ray_Plan.md, R0): its commands and functions, CAST, MOVE, TURN, DOOR,
' CELL, DEFINE and their errors, which are the same in every colour depth. The expected output was worked out from the
' PicoMite's code (graphics/Raycaster.c at bb833ed) before the port was written, with the CMM2's changes: a colour
' word in DEFINE (D2), a cell clamped to 31 (D17, PicoMite issue #97) and the messages in MMBasic's own format.
Option Explicit
Option Default Integer
Dim i, mi%(8), spr%(3), xray(2), spray(1)
Dim m$(6) Length 8, b$(2) Length 3
Dim q$ = Chr$(34)

Print "before map"; Ray(MapW); Ray(MapH); Ray(CamX); Ray(Sprites); Ray(Door 1, 1); Ray(Cell 0, 0); Ray(Dist 0)
Try "Ray Move 1"
Try "Ray Turn 10"
Try "Ray Cast 0"
Try "Ray Render"
Try "Ray Minimap 0, 0, 10"
Try "Ray Cell 1, 1, 1"
Try "Ray Door 1, 1, 0.5"
Try "Ray Fly"
Try "Print Ray(Fly)"

' the map as strings: '0' to '9', then 'A' or 'a' for 10 on (G is 16, a door by default)
m$(0) = "11111111"
m$(1) = "10000001"
m$(2) = "10000001"
m$(3) = "10002001"
m$(4) = "1000G001"
m$(5) = "10000001"
m$(6) = "11111111"
Ray Map 8, 7, m$()
Print "map"; Ray(MapW); Ray(MapH); Ray(Cell 4, 3); Ray(Cell 4, 4); Ray(Cell 0, 0); Ray(Cell 7, 6)
Try "Print Ray(Cell 8, 0)"
Try "Ray Map 257, 1, m$()"
b$(0) = "111" : b$(1) = "1#1" : b$(2) = "11"
Try "Ray Map 3, 4, b$()"
Try "Ray Map 3, 3, b$()"
b$(1) = "1a1"
Try "Ray Map 3, 3, b$()"
' as integers: below 0 is 0, and above 31 is 31 (the PicoMite's took 32, past its last wall type)
For i = 0 To 8 : mi%(i) = 1 : Next
mi%(3) = -5 : mi%(4) = 40 : mi%(5) = 0
Ray Map 3, 3, mi%()
Print "integer map"; Ray(MapW); Ray(MapH); Ray(Cell 0, 1); Ray(Cell 1, 1); Ray(Cell 2, 1)
Try "Ray Map 3, 4, mi%()"
Ray Map 8, 7, m$()

' CAST along the axes, onto a wall, a door and through it
Ray Camera 1.5, 1.5, 0
Print "camera"; F$(Ray(CamX)); F$(Ray(CamY)); F$(Ray(CamA))
Try "Ray Camera 1, 1"
Shoot 0 : Shoot 90 : Shoot 180 : Shoot 270
Ray Camera 1.5, 3.5, 0 : Shoot 0
Ray Camera 1.5, 4.5, 0 : Shoot 0
Ray Door 4, 4, 1 : Shoot 0
Ray Door 4, 4, 0.5 : Shoot 0
Ray Door 4, 4, 0.6 : Shoot 0
Print "door"; F$(Ray(Door 4, 4)); Ray(Door 1, 1)
Ray Door Close 4, 4
Print "door"; Ray(Door 4, 4)
' a door blocks until it is fully open
Ray Door 4, 4, 0.9
Ray Camera 3.5, 4.5, 0 : Ray Move 0.5 : Print "door move"; F$(Ray(CamX));
Ray Door 4, 4, 1 : Ray Move 0.5 : Print F$(Ray(CamX))
Ray Door Clear
For i = 0 To 7 : Ray Door i, 0, 0.5 : Next
Ray Door 3, 0, 0.7
Try "Ray Door 0, 1, 0.5"
Print "doors"; F$(Ray(Door 3, 0)); F$(Ray(Door 7, 0))
Ray Door Clear
Print "doors"; Ray(Door 3, 0)

' MOVE, with its strafe, its collision box and its sliding along a wall; TURN
Ray Camera 1.5, 1.5, 0
Ray Move 0.5 : Print "move"; F$(Ray(CamX)); F$(Ray(CamY));
Ray Move 0, 0.5 : Print F$(Ray(CamX)); F$(Ray(CamY));
Ray Move -10 : Print F$(Ray(CamX)); F$(Ray(CamY))
Ray Camera 2.5, 1.3, 225 : Ray Move 0.2 : Print "slide"; F$(Ray(CamX)); F$(Ray(CamY))
Ray Camera 1.5, 1.5, 225 : Ray Move 0.5 : Print "blocked"; F$(Ray(CamX)); F$(Ray(CamY))
Ray Camera 1.5, 1.5, 350 : Ray Turn 20 : Print "turn"; F$(Ray(CamA));
Ray Turn -30 : Print F$(Ray(CamA)); : Ray Turn 720 : Print F$(Ray(CamA));
Ray Camera 1.5, 1.5, 400 : Print F$(Ray(CamA))

' CELL, with a number or one character
Ray Cell 3, 2, "A" : Print "cell"; Ray(Cell 3, 2);
Ray Cell 3, 2, "v" : Print Ray(Cell 3, 2);
Ray Cell 3, 2, 0 : Print Ray(Cell 3, 2)
Try "Ray Cell 3, 2, " + q$ + "z" + q$
Try "Ray Cell 3, 2, " + q$ + "AB" + q$
Try "Ray Cell 3, 2, " + q$ + "#" + q$
Try "Ray Cell 3, 2, 32"
Try "Ray Cell 8, 0, 1"
Try "Ray Cell 3, 2"

' DEFINE: the defaults, the PicoMite's colours, and a colour word (D2: 1 to 15 an index, 0 BLACK, others a colour)
Print "define"; Ray(Define 1, 0); Ray(Define 1, 1); Ray(Define 1, 2); Ray(Define 1, 3)
Print "define"; Ray(Define 16, 0); Ray(Define 16, 1); Ray(Define 16, 2); Ray(Define 16, 3); Ray(Define "V", 2); Ray(Define "g", 3)
Ray Define 3, 9, 2, 5, 1
Print "define"; Ray(Define 3, 0); Ray(Define 3, 1); Ray(Define 3, 2); Ray(Define 3, 3)
Ray Define "C", RGB(255, 128, 0), RGB(BLACK), 7
Print "define "; Hex$(Ray(Define 12, 0)); Ray(Define 12, 1); Ray(Define 12, 2); Ray(Define 12, 3)
Try "Ray Define 0, 1, 1, 1"
Try "Ray Define 3, -1, 1, 1"
Try "Ray Define 3, 1, 1, 32"
Try "Ray Define 3, 1, 1, 1, 2"
Try "Ray Define 3, 1, 1"
Try "Print Ray(Define 3, 4)"
Try "Print Ray(Define " + q$ + "W" + q$ + ", 0)"

' COLOUR
Try "Ray Colour 1"
Try "Ray Colour 1, 2, 3, 4, 5, 32"
Try "Ray Color 1, -2"
Try "Ray Colour RGB(BLUE), RGB(WHITE), 0, 7, 3, 4"

' SPRITE: a sprite buffer's image, placed in the world
Try "Ray Sprite 0, 1, 2.5, 2.5"
Try "Ray Sprite 32, 1, 2.5, 2.5"
Try "Ray Sprite 0, 65, 2.5, 2.5"
Try "Ray Sprite 0, 1, 2.5"
For i = 0 To 3 : spr%(i) = RGB(RED) : Next
Sprite LoadArray #1, 2, 2, spr%()
Ray Sprite 0, 1, 2.5, 2.5 : Ray Sprite 5, 1, 3.5, 4.25
Print "sprites"; Ray(Sprites); F$(Ray(SpriteX 5)); F$(Ray(SpriteY 5)); F$(Ray(SpriteX 1));
Ray Sprite Remove 0 : Print Ray(Sprites);
Ray Sprite Clear : Print Ray(Sprites)

' RENDER: each column's distance and wall (the middle one looks straight along the row, at wall 2)
Ray Camera 1.5, 3.5, 0, 60
Ray Render
Ray Minimap 0, 0, 40
Print "render"; F$(Ray(Dist MM.HRes \ 2)); Ray(Wall MM.HRes \ 2); F$(Ray(Dist 0)); Ray(Wall 0)
Try "Ray Minimap 0, 0, 3"

' RAY( is folded into SPRITE(: names that end in RAY are not, and SPRITE( itself is as it was
xray(1) = 5 : spray(1) = 6
Print "names"; xray(1); spray(1); Ray (MapW); Sprite(W, #1)

Ray Close
Print "closed"; Ray(MapW); Ray(Sprites); Ray(Door 4, 4)
Ray Close
Print "done"
End

Sub Try c$
  On Error Skip
  Execute c$
  Print c$; ": "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
End Sub

Sub Shoot a!
  Ray Cast a!
  Print "cast"; a!; F$(Ray(CastDist)); Ray(CastWall); Ray(CastSide); Ray(CastX); Ray(CastY)
End Sub

Function F$(x!)
  F$ = Format$(x!, " %.4f")
End Function
