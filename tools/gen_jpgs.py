"""gen_jpgs.py DIR  (python tools/gen_jpgs.py tests/cmm2)
Writes the JPEG test images for tests/cmm2/jpgdec.bas, the program, and its expected output. JPEG is lossy, so the
program does not print pixels: it checks each against the colour the image was made with, within a tolerance, and
prints ok (or the pixel it found). The images: four flat quadrants in 4:4:4 and 4:2:0 (8 and 16-line MCU rows), a
strip wider than the screen, a grey ramp for dithering, a progressive file. Needs Pillow."""
import os, sys
from PIL import Image
out = sys.argv[1]

TL, TR, BL, BR = (200, 50, 40), (40, 180, 60), (50, 60, 200), (220, 200, 40)
def quads(w=64, h=64):
    im = Image.new('RGB', (w, h))
    for y in range(h):
        for x in range(w):
            im.putpixel((x, y), (TL if x < w // 2 else TR) if y < h // 2 else (BL if x < w // 2 else BR))
    return im
quads().save(os.path.join(out, 'jd_q.jpg'), quality=95, subsampling=0)
quads().save(os.path.join(out, 'jd_q420.jpg'), quality=95, subsampling=2)
quads().save(os.path.join(out, 'jd_prog.jpg'), quality=95, subsampling=0, progressive=True)
# 704 wide, eleven 64-pixel stripes: the screen (640) ends inside the tenth
STRIPES = [(200, 50, 40), (40, 180, 60), (50, 60, 200), (220, 200, 40), (180, 40, 180), (40, 180, 180),
           (120, 120, 120), (230, 120, 30), (30, 120, 230), (120, 230, 30), (200, 200, 200)]
im = Image.new('RGB', (704, 32))
for x in range(704):
    for y in range(32):
        im.putpixel((x, y), STRIPES[x // 64])
im.save(os.path.join(out, 'jd_wide.jpg'), quality=95, subsampling=0)
# a grey ramp, 128 x 16, for the dither
im = Image.new('RGB', (128, 16))
for x in range(128):
    for y in range(16):
        im.putpixel((x, y), (x * 2, x * 2, x * 2))
im.save(os.path.join(out, 'jd_ramp.jpg'), quality=95, subsampling=0)

hexc = lambda c: '&H%02X%02X%02X' % c
bas = '''' jpgdec.bas - LOAD JPG through the PicoMite's decoder (MCU rows, source offsets, scaling, dithering), on images made
' by tools/gen_jpgs.py with flat colours. JPEG is lossy: each pixel read with PIXEL() is checked against the colour it
' was made with, within 16 in each component, and the line prints ok or the pixel found.
Option Explicit
Dim string s$
Dim integer i, x, y, c, d, n, bad, stripe(9)
Const TL = %s, TR = %s, BL = %s, BR = %s, BG = 0
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
Print "dither 3: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
On Error Skip 1
Load JPG "jd_q", 0, 0, , , , 3
Print "scale 3: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
On Error Skip 1
Load JPG "jd_q", 0, 0, , 64
Print "x offset 64: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
On Error Skip 1
Load JPG "jd_prog"
Print "progressive: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
On Error Skip 1
Load JPG "bd_bad.bmp"
Print "not a JPEG: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
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

Data %s
''' % (hexc(TL), hexc(TR), hexc(BL), hexc(BR), ','.join(hexc(c) for c in STRIPES[:10]))
open(os.path.join(out, 'jpgdec.bas'), 'w', newline='\n').write(bas)
exp = '''4:4:4:  ok ok ok ok ok
4:2:0:  ok ok ok ok ok
from 32,32:  ok ok ok
from 32,0:  ok ok ok
from 0,40:  ok ok ok
scale 2:  ok ok ok ok ok
scale 4:  ok ok ok ok ok
from 32,32 at scale 2:  ok ok
wider than the screen:  ok ok ok ok ok ok ok ok ok ok ok ok
dither 2 (Floyd-Steinberg, RGB332): every pixel an RGB332 colour, many changes along the row
dither 6 (Atkinson, RGB332): every pixel an RGB332 colour
dither 3: RGB565 dithering not yet supported
scale 3: Scale must be 1, 2, 4 or 8
x offset 64: 64 is invalid (valid is 0 to 63)
progressive: Progressive JPEG files are not supported
not a JPEG: pjpeg_decode_init() failed with status 19
file 10 free
'''
open(os.path.join(out, 'jpgdec.expected'), 'w', newline='\n').write(exp)
for f in ('jd_q.jpg', 'jd_q420.jpg', 'jd_prog.jpg', 'jd_wide.jpg', 'jd_ramp.jpg'):
    print(f, os.path.getsize(os.path.join(out, f)))
