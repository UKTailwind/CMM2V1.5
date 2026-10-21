' SPRITE LOADPNG #n, f$ [, t] [, cutoff] [, x, y, w, h], the PicoMite's form (docs/Sprite_Plan.md, S2), on the images
' tools/gen_pngs.py makes (pngdec.bas's, with known pixels): each sprite shown over blue in MODE 8, 32 and read back.
' pd_rgba's alpha is 40x + 30y; pd_pal8 is black, red, green, blue along each row, one further on each row down.
Option Explicit
Option Default Integer
Dim e$
Mode 8, 32
Option Console Serial
Sprite LoadPNG #1, "pd_rgba"
Rows 1, "alpha above 30"
Sprite LoadPNG #2, "pd_rgba", , 100
Rows 2, "cutoff 100"
Sprite LoadPNG #3, "pd_rgba", 4
Rows 3, "t 4"
Sprite LoadPNG #4, "pd_pal8"
Rows 4, "pal8"
Sprite LoadPNG #5, "pd_pal8", -15
Rows 5, "pal8, t -15"
Sprite LoadPNG #6, "pd_rgb", , , 2, 1, 3, 2
Rows 6, "part 2, 1, 3, 2"
Sprite LoadPNG #7, "pd_rgb", , , 4
Rows 7, "from x 4"
Sprite LoadPNG #8, "pd_grey"
Rows 8, "grey"
Sprite LoadPNG #9, "pd_pal2"
Rows 9, "pal2"
Sprite LoadPNG #10, "pd_trns"
Rows 10, "tRNS index 0"
Sprite LoadPNG #11, "pd_wide", , , 0, 0, 640
Print "wide, 640 of it:"; Sprite(W, 11); Sprite(H, 11)
Sprite Close All
Try "Sprite LoadPNG #12, " + Chr$(34) + "pd_rgb" + Chr$(34) + ", , , 5, 0, 2"
Try "Sprite LoadPNG #12, " + Chr$(34) + "pd_rgb" + Chr$(34) + ", , , 0, 2, , 2"
Try "Sprite LoadPNG #12, " + Chr$(34) + "pd_rgb" + Chr$(34) + ", , , -1"
Try "Sprite LoadPNG #12, " + Chr$(34) + "pd_rgb" + Chr$(34) + ", 16"
Try "Sprite LoadPNG #12, " + Chr$(34) + "pd_rgb" + Chr$(34) + ", , 255"
Try "Sprite LoadPNG #12, " + Chr$(34) + "pd_wide" + Chr$(34)
Try "Sprite LoadPNG #12, " + Chr$(34) + "pd_16" + Chr$(34)
Try "Sprite LoadPNG #12"
Sprite LoadPNG #12, "pd_rgb"
Print "#12 free after the errors:"; Sprite(W, 12); Sprite(H, 12)
Try "Sprite LoadPNG #12, " + Chr$(34) + "pd_rgb" + Chr$(34)
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

Sub Try c$
  On Error Skip
  Execute c$
  Print Mid$(c$, 21); ": "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
End Sub
