' The sprite engine in every colour depth (docs/Sprite_Plan.md, S0): the same steps in MODE 8 with depths 8, 12, 16, 32
' and 4, each step's screen checksummed (the top 240 rows of the write page, a CRC-32) with the collision results.
' The expected output is the engine's as it stood on 21 Oct 2026; lines marked BUG in the plan change as each is fixed.
Option Explicit
Option Default Integer
Dim a%(65535), ring%(255), cross%(255), solid%(63)
Dim depths(4) = (8, 12, 16, 32, 4)
Dim i, j, k, d, hits, sthits
MakeImages
Option Console Serial
For k = 0 To 4
  d = depths(k)
  On Error Skip
  Mode 8, d
  If MM.ErrNo Then
    Print "depth"; d; ": "; MM.ErrMsg$
  Else
    Print "depth"; d
    RunSteps
  EndIf
Next k
Kept
Statics
Overlap
Opaque
Mode 8
Option Console Both
End

' a .SPR file, TRANSPARENCY, and the images kept through MODE (colour words: the same in every depth)
Sub Kept
  Open "spritest.spr" For Output As #1
  Print #1, "' a test sprite"
  Print #1, "8,1,6"
  Print #1, " 1234567"
  Print #1, "89ABCDEF"
  Print #1, "0      0"
  Print #1, " 4    4 "
  Print #1, "  6  6  "
  Print #1, "   77   "
  Close #1
  Print "kept"
  Mode 8, 8
  Background
  Sprite Load "spritest.spr", 10
  Sprite LoadArray #11, 16, 16, ring%()
  Print "  loaded"; Sprite(W, 10); Sprite(H, 10); Sprite(W, 11)
  For k = 0 To 4
    On Error Skip
    Mode 8, depths(k)
    If MM.ErrNo = 0 Then
      Background
      Sprite Show #10, 30, 30, 1
      Sprite Show #11, 60, 30, 1
      Snap "depth" + Str$(depths(k)) + " show"
      If depths(k) = 12 Then
        Sprite Transparency #11, 7
        Sprite Show #11, 60, 50, 1
        Snap "  transparency 7"
      EndIf
    EndIf
  Next k
  Sprite Close All
  Kill "spritest.spr"
End Sub

' the PicoMite's static objects: rectangles the sprites collide with, reported while they overlap
Sub Statics
  sthits = 0
  Print "statics"
  Mode 8, 8
  Background
  Sprite LoadArray #1, 16, 16, ring%()
  Sprite Static #1, 100, 100, 50, 20
  Sprite Static #2, 300, 10, 10, 10
  Print "  ST 1"; Sprite(ST, 1, X); Sprite(ST, 1, Y); Sprite(ST, 1, W); Sprite(ST, 1, H); Sprite(ST, 1, A); "  ST 3"; Sprite(ST, 3, X); Sprite(ST, 3, A)
  Sprite StInterrupt StHit
  Sprite Show #1, 120, 90, 1                               ' over static 1
  Report 1
  Print "  ST COLLISION"; Sprite(ST, COLLISION); " OBJECT"; Sprite(ST, OBJECT)
  Sprite Show #1, 122, 90, 1                               ' still over it: reported again
  Report 1
  Sprite Show #1, 10, 10, 1                                ' clear of both
  Report 1
  Sprite Scroll 10, 0
  Print "  after scroll ST 1"; Sprite(ST, 1, X); Sprite(ST, 1, Y)
  Sprite Static #1, Off
  Print "  off"; Sprite(ST, 1, A); Sprite(ST, 2, A)
  Sprite Static Clear
  Print "  clear"; Sprite(ST, 2, A); Sprite(ST, COLLISION)
  Sprite NoStInterrupt
  Print "  st interrupts"; sthits
  Sprite Close All
End Sub

Sub StHit
  sthits = sthits + 1
End Sub

' the steps, in the current depth
Sub RunSteps
  hits = 0
  Background
  Snap "background"
  Sprite LoadArray #1, 16, 16, ring%()
  Sprite LoadArray #5, 16, 16, cross%()
  Sprite LoadArray #6, 8, 8, solid%()
  Box 600, 400, 24, 12, 0, RGB(YELLOW), RGB(YELLOW)       ' a shape on black for READ: its black stays transparent
  Box 604, 401, 6, 4, 0, RGB(MAGENTA), RGB(MAGENTA)       ' (not symmetric: every flip shows)
  Sprite Read #2, 598, 399, 28, 14
  Box 598, 399, 28, 14, 0, 0, 0
  Sprite Copy #1, #3, 2                                    ' 3 and 4 share 1's image
  Print "  sizes"; Sprite(W, 1); Sprite(H, 1); Sprite(W, 2); Sprite(H, 2); Sprite(W, 3); Sprite(W, 7)
  Sprite Interrupt Hit
  Sprite Show #1, 20, 20, 1
  Snap "show 1"
  Sprite Show #2, 28, 24, 1                                ' overlaps 1 on its layer
  Snap "show 2 over 1"
  Report 2
  Sprite Show #3, 100, 30, 0                               ' layer 0: under everything, collides with all
  Sprite Show #4, 630, 100, 2                              ' over the right edge
  Snap "show 3 and 4"
  Report 4
  Sprite Show #6, -4, 60, 1                                ' over the left edge
  Report 6
  Print "  N"; Sprite(N); Sprite(N, 0); Sprite(N, 1); Sprite(N, 2); "  L"; Sprite(L, 3); Sprite(L, 7)
  Print "  X Y"; Sprite(X, 2); Sprite(Y, 2); Sprite(X, 7); "  V D "; Str$(Sprite(V, 1, 2), 1, 4); " "; Str$(Sprite(D, 1, 2), 1, 4)
  Sprite Next #1, 40, 40
  Sprite Next #2, 44, 44
  Sprite Next #3, 34, 34                                   ' layer 0 under 1 and 2 now
  Sprite Move
  Snap "move"
  Report 0
  Report 1
  Report 2
  Sprite Move                                             ' the same again: no new collisions, the masks kept
  Report 0
  Report 1
  Sprite Hide Safe #3                                      ' the bottom of the layer-0 list
  Snap "hide safe 3"
  Sprite Show Safe #3, 36, 36, 0
  Snap "show safe 3"
  Sprite Hide Safe #1                                      ' the bottom of the main list
  Snap "hide safe 1"
  Sprite Show Safe #1, 40, 40, 1, 0, 1                     ' to the top
  Snap "show safe 1 on top"
  For i = 0 To 3
    Sprite Show #2, 150, 150, 1, i
    Snap "orientation " + Str$(i)
  Next i
  Sprite Swap #1, #5
  Snap "swap 1 for 5"
  Sprite Write #6, 200, 180                               ' a copy, not a sprite
  Sprite Write #2, 230, 180, 0
  Snap "write"
  Sprite ScrollR 0, 0, 160, 120, 5, 3
  Snap "scrollr"
  Sprite Scroll 4, 2
  Snap "scroll"
  Print "  after scroll X Y 3:"; Sprite(X, 3); Sprite(Y, 3)
  Sprite Hide All
  Snap "hide all"
  Sprite Restore
  Snap "restore"
  Sprite Hide #2
  Sprite Hide #4
  Snap "hide 2 4"
  Print "  interrupts"; hits
  Sprite NoInterrupt
  Sprite Close All
  Snap "close all"
End Sub

Sub Report n
  Local m, s$ = ""
  For m = 1 To Sprite(C, n)
    s$ = s$ + " " + Hex$(Sprite(C, n, m))
  Next m
  Print "  C"; n; ":"; Sprite(C, n); " ["; s$; " ] T "; Hex$(Sprite(T, n)); " E "; Sprite(E, n); " S "; Sprite(S)
End Sub

Sub Hit
  hits = hits + 1
End Sub

Sub Snap s$
  Print "  "; s$; ": "; Hex$(PageCRC%())
End Sub

Sub Background
  Cls
  Box 0, 0, 320, 240, 0, RGB(64, 64, 64), RGB(64, 64, 64)
  For i = 0 To 7
    Box i * 40, 0, 20, 240, 0, RGB(0, 0, 128 + i * 16), RGB(0, 0, 128 + i * 16)
  Next i
  For i = 0 To 5
    Line 0, i * 40, 319, i * 40 + 20, 1, RGB(0, 200, 0)
  Next i
End Sub

Sub MakeImages
  Local x, y, r
  For y = 0 To 15
    For x = 0 To 15
      r = (x - 7.5) ^ 2 + (y - 7.5) ^ 2
      If r >= 16 And r <= 56 Then ring%(y * 16 + x) = RGB(RED)
      If Abs(x - 7.5) < 2 And Abs(y - 7.5) < 2 Then ring%(y * 16 + x) = RGB(GREEN)
      If x = 7 Or y = 7 Then cross%(y * 16 + x) = RGB(BLUE)
    Next x
  Next y
  For x = 0 To 63 : solid%(x) = RGB(WHITE) : Next x
End Sub

' the top 240 rows of the write page (MODE 8 holds no doubled rows): a CRC-32 chained through its start mask
Function PageCRC%()
  Local string m$ = Str$(MM.Info(MODE))
  Local integer bits = Val(Mid$(m$, Instr(m$, ".") + 1)), addr = MM.Info(WRITEBUFF), left, n, crc = 0
  If bits = 12 Then bits = 16
  left = MM.HRes * 240 * bits \ 8
  Do While left > 0
    n = Min(left, 65535)
    Memory Unpack addr, a%(), n, 8
    crc = Math(CRC32 a%(), n, , crc)
    addr = addr + n : left = left - n
  Loop
  PageCRC% = crc
End Function

' SPRITE(B): the background under a sprite against its shape (the PicoMite's: row and column spans of its pixels)
Sub Overlap
  Local integer dd, m
  Print "overlap"
  For dd = 8 To 16 Step 8
    Mode 8, dd
    Cls
    Box 100, 100, 40, 40, 0, RGB(WHITE), RGB(WHITE)
    Sprite LoadArray #1, 16, 16, ring%()
    Sprite Show #1, 50, 50, 1
    BReport "  depth" + Str$(dd) + " far"
    Sprite Show #1, 85, 85, 1                                ' its corner on the box, outside the ring
    BReport "  corner"
    Sprite Show #1, 92, 95, 1                                ' the ring over the box's corner
    BReport "  over"
    Sprite Show #1, 92, 95, 1, 1                             ' mirrored
    BReport "  mirrored"
    Sprite Close All
  Next dd
End Sub

Sub BReport s$
  Local integer m
  Print s$; ":"; Sprite(B, 1); " [";
  For m = 0 To 7 : Print Sprite(B, 1, m); : Next m
  Print " ]"
End Sub

' orientations 4-7: the same flips, every pixel drawn (the PicoMite's), kept through MOVE
Sub Opaque
  Print "opaque"
  Mode 8, 8
  Background
  Sprite LoadArray #1, 16, 16, ring%()
  Sprite Show #1, 30, 30, 1
  Snap "orientation 0"
  Sprite Show #1, 30, 30, 1, 4
  Snap "orientation 4"
  Sprite Next #1, 40, 30
  Sprite Move
  Snap "moved"
  Sprite Show #1, 40, 30, 1, 5
  Snap "orientation 5"
  Sprite Show #1, 40, 30, 1, 0
  Snap "orientation 0 again"
  Sprite Set Transparent 6                                ' the PicoMite's green: the ring's centre, and black shows
  Sprite Show #1, 40, 30, 1
  Snap "transparent 6"
  Print "  B"; Sprite(B, 1)
  Sprite Set Transparent 0
  Sprite Show #1, 40, 30, 1
  Snap "transparent 0"
  Sprite Close All
End Sub
