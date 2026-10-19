' jpgdec.bas - LOAD JPG through the PicoMite's decoder (MCU rows, source offsets, scaling, dithering), on images made
' by tools/gen_jpgs.py with flat colours. JPEG is lossy: each pixel read with PIXEL() is checked against the colour it
' was made with, within 16 in each component, and the line prints ok or the pixel found.
Option Explicit
Dim string s$
Dim integer i, x, y, c, d, n, bad, stripe(9)
Const TL = &HC83228, TR = &H28B43C, BL = &H323CC8, BR = &HDCC828, BG = 0
For i = 0 To 9 : Read stripe(i) : Next
Mode 8, 32
CLS
Load JPG "jd_q", 300, 300
Print "4:4:4: "; chk$(308, 308, TL); chk$(340, 308, TR); chk$(308, 340, BL); chk$(340, 340, BR); chk$(370, 370, BG)
CLS
Load JPG "jd_q420", 300, 300
Print "4:2:0: "; chk$(308, 308, TL); chk$(340, 308, TR); chk$(308, 340, BL); chk$(340, 340, BR); chk$(370, 370, BG)
CLS
Load JPG "jd_q", 300, 300, , 32, 32
Print "from 32,32: "; chk$(308, 308, BR); chk$(340, 308, BG); chk$(308, 340, BG)
CLS
Load JPG "jd_q", 300, 300, , 32
Print "from 32,0: "; chk$(308, 308, TR); chk$(308, 340, BR); chk$(340, 308, BG)
CLS
Load JPG "jd_q", 300, 300, , , 40
Print "from 0,40: "; chk$(308, 308, BL); chk$(340, 308, BR); chk$(308, 330, BG)
CLS
Load JPG "jd_q", 300, 300, , , , 2
Print "scale 2: "; chk$(308, 308, TL); chk$(324, 308, TR); chk$(308, 324, BL); chk$(324, 324, BR); chk$(340, 308, BG)
CLS
Load JPG "jd_q", 300, 300, , , , 4
Print "scale 4: "; chk$(304, 304, TL); chk$(312, 304, TR); chk$(304, 312, BL); chk$(312, 312, BR); chk$(320, 304, BG)
CLS
Load JPG "jd_q", 300, 300, , 32, 32, 2
Print "from 32,32 at scale 2: "; chk$(308, 308, BR); chk$(320, 308, BG)
CLS
Load JPG "jd_wide", 0, 300
s$ = ""
For i = 0 To 9 : s$ = s$ + chk$(i * 64 + 32, 310, stripe(i)) : Next
Print "wider than the screen: "; s$; chk$(639, 310, stripe(9)); chk$(10, 340, BG)
CLS
Load JPG "jd_ramp", 300, 300, 2
n = 0 : bad = 0 : d = -1
For x = 300 To 427
  c = Pixel(x, 308)
  If Not (lvl((c >> 16) And 255, 7) And lvl((c >> 8) And 255, 7) And lvl(c And 255, 3)) Then bad = bad + 1
  If c <> d Then n = n + 1 : d = c
Next
Print "dither 2 (Floyd-Steinberg, RGB332): "; Choice(bad = 0, "every pixel an RGB332 colour", Str$(bad) + " pixels not RGB332"); ", "; Choice(n > 40, "many changes along the row", "only " + Str$(n) + " changes")
CLS
Load JPG "jd_ramp", 300, 300, 6
bad = 0
For x = 300 To 427
  c = Pixel(x, 308)
  If Not (lvl((c >> 16) And 255, 7) And lvl((c >> 8) And 255, 7) And lvl(c And 255, 3)) Then bad = bad + 1
Next
Print "dither 6 (Atkinson, RGB332): "; Choice(bad = 0, "every pixel an RGB332 colour", Str$(bad) + " pixels not RGB332")
CLS
On Error Skip 1
Load JPG "jd_q", 0, 0, 3
Print "dither 3: "; MM.ErrMsg$
On Error Skip 1
Load JPG "jd_q", 0, 0, , , , 3
Print "scale 3: "; MM.ErrMsg$
On Error Skip 1
Load JPG "jd_q", 0, 0, , 64
Print "x offset 64: "; MM.ErrMsg$
On Error Skip 1
Load JPG "jd_prog"
Print "progressive: "; MM.ErrMsg$
On Error Skip 1
Load JPG "bd_bad.bmp"
Print "not a JPEG: "; MM.ErrMsg$
Open "jpgdec.tmp" For Output As #10 : Close #10 : Kill "jpgdec.tmp"
Print "file 10 free"
CLS
End

' ok if the pixel is within 16 of colour c in each component, otherwise the pixel
Function chk$(px, py, c)
  Local integer p = Pixel(px, py), k, e = 0
  For k = 0 To 16 Step 8
    If Abs(((p >> k) And 255) - ((c >> k) And 255)) > 16 Then e = 1
  Next
  chk$ = Choice(e, " " + Hex$(p, 6), " ok")
End Function

' true if v is one of the n+1 levels k*255/n
Function lvl(v, nn)
  Local integer k
  lvl = 0
  For k = 0 To nn
    If v = Int(k * 255 / nn) Then lvl = 1
  Next
End Function

Data &HC83228,&H28B43C,&H323CC8,&HDCC828,&HB428B4,&H28B4B4,&H787878,&HE6781E,&H1E78E6,&H78E61E
