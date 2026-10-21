' SPRITE LOADBMP #n, f$ [, x, y, w, h] (and BLIT LOAD, its other name), SPRITE LOAD's palette, and where SPRITE and
' BLIT differ, the PicoMite's (docs/Sprite_Plan.md, S2): on the images tools/gen_bmps.py makes (bmpdec.bas's, with known
' pixels), each sprite shown over blue in MODE 8, 32 and read back.
Option Explicit
Option Default Integer
Dim a%(1) = (0, RGB(RED)), h, k, e$
Mode 8, 32
Option Console Serial
Sprite LoadBMP #1, "bd_24"
Rows 1, "24-bit"
Sprite LoadBMP #2, "bd_24td"
Rows 2, "24-bit top-down"
Sprite LoadBMP #3, "bd_p4", 2, 1, 3, 2
Rows 3, "4-bit, part 2, 1, 3, 2"
Sprite LoadBMP #4, "bd_r8", 5
Rows 4, "RLE8 from x 5"
Sprite LoadBMP #5, "bd_p1", , 1
Rows 5, "1-bit from y 1"
Blit Load #6, "bd_565"
Rows 6, "BLIT LOAD, 565"
Open "sp_pal.spr" For Output As #1
Print #1, "4,1,2"
Print #1, "0123"
Print #1, "89AF"
Close #1
Sprite Load "sp_pal", 7
Rows 7, "SPRITE LOAD"
Sprite Load "sp_pal", 8, 1
Rows 8, "SPRITE LOAD, palette 1"
Kill "sp_pal.spr"
Sprite LoadArray #9, 2, 1, a%()
Cls RGB(0, 0, 255)
Blit Write #9, 100, 100
Blit Write #9, 110, 100, 4
Sprite Write #9, 120, 100
Sprite Write #9, 130, 100, 0
Print "WRITE, black then red: BLIT "; Two$(100); ", BLIT 4 "; Two$(110); ", SPRITE "; Two$(120); ", SPRITE 0 "; Two$(130)
Sprite Close All
Try "Sprite LoadBMP #12, " + Chr$(34) + "bd_24" + Chr$(34) + ", 4, 0, 2"
Try "Sprite LoadBMP #12, " + Chr$(34) + "bd_24" + Chr$(34) + ", 0, 0, 0"
Try "Sprite LoadBMP #12, " + Chr$(34) + "bd_24" + Chr$(34) + ", 0, -1"
Try "Sprite LoadBMP #12, " + Chr$(34) + "bd_bad" + Chr$(34)
Try "Sprite LoadBMP #12, " + Chr$(34) + "bd_none" + Chr$(34)
Try "Sprite LoadBMP #12"
Open "bd_24.bmp" For Input As #1 : e$ = Input$(255, #1) : Close #1
Open "bd_hdr.bmp" For Output As #1 : Print #1, Left$(e$, 14) + Chr$(64) + Mid$(e$, 16); : Close #1
h = MM.Info(HEAP)
For k = 1 To 10
  On Error Skip
  Sprite LoadBMP #12, "bd_hdr"
Next k
h = h - MM.Info(HEAP)                                      ' (not within the PRINT: its strings take heap)
Print "the decoder's error, ten times: "; MM.ErrMsg$; ", the heap"; h; " bytes less"
Kill "bd_hdr.bmp"
Sprite LoadBMP #12, "bd_24"
Print "#12 free after the errors:"; Sprite(W, 12); Sprite(H, 12)
Try "Sprite LoadBMP #12, " + Chr$(34) + "bd_24" + Chr$(34)
Try "Sprite Load " + Chr$(34) + "sp_none" + Chr$(34) + ", 1, 2"
Sprite Close All
Mode 8
Option Console Both
End

' sprite n shown at 100, 100 over blue, its rows read back
Sub Rows n, t$
  Local x, y, r$
  Cls RGB(0, 0, 255)
  Sprite Show #n, 100, 100, 1
  Print t$; ":"; Sprite(W, n); Sprite(H, n)
  For y = 0 To Min(Sprite(H, n), 3) - 1
    r$ = " "
    For x = 0 To Min(Sprite(W, n), 6) - 1
      r$ = r$ + " " + Hex$(Pixel(100 + x, 100 + y) And &HFFFFFF, 6)
    Next x
    Print r$
  Next y
  Sprite Hide #n
End Sub

Function Two$(x)
  Two$ = Hex$(Pixel(x, 100) And &HFFFFFF, 6) + " " + Hex$(Pixel(x + 1, 100) And &HFFFFFF, 6)
End Function

Sub Try c$
  On Error Skip
  Execute c$
  Print Mid$(c$, Instr(c$, " ") + 1); ": "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
End Sub
