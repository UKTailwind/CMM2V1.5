"""gen_pngs.py DIR  (python tools/gen_pngs.py tests/cmm2)
Writes the PNG test images for tests/cmm2/pngdec.bas, the program, and its expected output, which comes from the pixels
each image was made with (PNG is lossless): RGB, RGBA, indexed at 8 and 2 bits, indexed with transparency, grey, grey
with alpha, a strip wider than the screen, a 16-bit file the decoder refuses; then offsets, clipping at both edges, the
alpha cut in 16-bit colour (where pixels below it are not drawn), BLIT LOADPNG, and files it cannot read. Needs Pillow."""
import os, sys
from PIL import Image
out = sys.argv[1]
W, H = 6, 3
RGB = [[(10 + 40 * x, 20 + 70 * y, 200 - 30 * x) for x in range(W)] for y in range(H)]
images = {}

def save(name, im, **kw):
    im.save(os.path.join(out, name + '.png'), **kw)

im = Image.new('RGB', (W, H))
for y in range(H):
    for x in range(W): im.putpixel((x, y), RGB[y][x])
save('pd_rgb', im); images['pd_rgb'] = RGB
im = Image.new('RGBA', (W, H))
for y in range(H):
    for x in range(W): im.putpixel((x, y), RGB[y][x] + (40 * x + 30 * y,))
save('pd_rgba', im); images['pd_rgba'] = RGB
PAL = [(0, 0, 0), (250, 30, 30), (30, 250, 30), (30, 30, 250)]
IDX = [[(x + y) % 4 for x in range(W)] for y in range(H)]
for name, bits in (('pd_pal8', 8), ('pd_pal2', 2)):
    im = Image.new('P', (W, H))
    im.putpalette([v for c in PAL for v in c] + [0] * (768 - 12))
    for y in range(H):
        for x in range(W): im.putpixel((x, y), IDX[y][x])
    save(name, im, bits=bits)
    images[name] = [[PAL[IDX[y][x]] for x in range(W)] for y in range(H)]
GREY = [[(15 * x + 60 * y + 5) for x in range(W)] for y in range(H)]
im = Image.new('L', (W, H))
for y in range(H):
    for x in range(W): im.putpixel((x, y), GREY[y][x])
save('pd_grey', im); images['pd_grey'] = [[(g, g, g) for g in row] for row in GREY]
im = Image.new('LA', (W, H))
for y in range(H):
    for x in range(W): im.putpixel((x, y), (GREY[y][x], 255 - 20 * x))
save('pd_greya', im); images['pd_greya'] = images['pd_grey']
# 704 wide, eleven 64-pixel stripes: the screen (640) ends inside the tenth
STRIPES = [(200, 50, 40), (40, 180, 60), (50, 60, 200), (220, 200, 40), (180, 40, 180), (40, 180, 180),
           (120, 120, 120), (230, 120, 30), (30, 120, 230), (120, 230, 30), (200, 200, 200)]
im = Image.new('RGB', (704, 4))
for x in range(704):
    for y in range(4): im.putpixel((x, y), STRIPES[x // 64])
save('pd_wide', im)
# a row of red at seven alphas, for the cut in 16-bit colour
ALPHAS = [0, 64, 127, 128, 191, 192, 255]
im = Image.new('RGBA', (len(ALPHAS), 1))
for x, a in enumerate(ALPHAS): im.putpixel((x, 0), (255, 0, 0, a))
save('pd_alpha', im)
# indexed with transparency: index 0 fully transparent, the rest opaque
im = Image.new('P', (4, 1))
im.putpalette([v for c in PAL for v in c] + [0] * (768 - 12))
for x in range(4): im.putpixel((x, 0), x)
save('pd_trns', im, transparency=0)
# 16 bits a sample: refused
im = Image.new('I;16', (4, 2))
for x in range(4): im.putpixel((x, 0), 1000 * x)
save('pd_16', im)

order = ['pd_rgb', 'pd_rgba', 'pd_pal8', 'pd_pal2', 'pd_grey', 'pd_greya']
data = ', '.join('"%s"' % n for n in order)
bas = '''' pngdec.bas - LOAD PNG through the PicoMite's streaming decoder (with the CMM2's RGB, grey and grey-with-alpha), and
' BLIT LOADPNG through its whole-image wrapper: images made by tools/gen_pngs.py with known pixels, read back with PIXEL()
' in 32-bit colour (where DrawBuffer stores the alpha and draws every pixel), and the alpha cut in 16-bit colour, where
' pixels below it are not drawn. The expected output was written from the pixels the images were made with.
Option Explicit
Dim string n$, r$
Dim integer x, y, i, bg
Mode 8, 32
For i = 1 To %d
  Read n$
  CLS
  Load PNG n$, 300, 300
  Print n$; ":"
  For y = 0 To %d
    r$ = " "
    For x = 0 To %d : r$ = r$ + " " + Hex$(Pixel(300 + x, 300 + y), 6) : Next
    Print r$
  Next
Next
CLS
Load PNG "pd_rgb", 300, 300, , , 2, 1
Print "from 2, 1:"
For y = 0 To 1
  r$ = " "
  For x = 0 To 3 : r$ = r$ + " " + Hex$(Pixel(300 + x, 300 + y), 6) : Next
  Print r$
Next
CLS
Load PNG "pd_rgb", -2, 300
r$ = " " : For x = 0 To 3 : r$ = r$ + " " + Hex$(Pixel(x, 300), 6) : Next
Print "at x -2:"; r$
CLS
Load PNG "pd_wide", 0, 300
r$ = "" : For i = 0 To 9 : r$ = r$ + " " + Hex$(Pixel(i * 64 + 32, 301), 6) : Next
Print "wider than the screen:"; r$; " "; Hex$(Pixel(639, 301), 6)
CLS
Load PNG "pd_rgb", 637, 300
Print "at x 637: "; Hex$(Pixel(637, 300), 6); " "; Hex$(Pixel(639, 300), 6)
CLS
Blit LoadPNG #1, "pd_rgb"
Blit Write #1, 300, 300
r$ = " " : For x = 0 To 5 : r$ = r$ + " " + Hex$(Pixel(300 + x, 301), 6) : Next
Print "BLIT LOADPNG:"; r$
Blit Close #1
Mode 8, 16
CLS RGB(0, 0, 255)
bg = Pixel(10, 10)
cut "", ""
cut "1", ""
cut "8", ""
cut "", "200"
CLS RGB(0, 0, 255)
Load PNG "pd_trns", 300, 300, 2
r$ = "" : For x = 0 To 3 : r$ = r$ + Choice(Pixel(300 + x, 300) <> bg, "1", "0") : Next
Print "indexed, transparent index 0, t 2: "; r$
Mode 8, 32
CLS
On Error Skip 1
Load PNG "pd_16"
Print "16 bits: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
On Error Skip 1
Load PNG "bd_bad.bmp"
Print "not a PNG: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
On Error Skip 1
Load PNG "pd_rgb", 0, 0, , , 6
Print "x offset 6: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
Open "pngdec.tmp" For Output As #10 : Close #10 : Kill "pngdec.tmp"
Print "file 10 free"
CLS
End

' the seven alphas of pd_alpha drawn over blue in 16-bit colour: 1 where the red pixel was drawn
Sub cut t$, c$
  Local integer k
  Local string s$
  CLS RGB(0, 0, 255)
  If t$ = "" And c$ = "" Then Load PNG "pd_alpha", 300, 300
  If t$ <> "" Then Load PNG "pd_alpha", 300, 300, Val(t$)
  If c$ <> "" Then Load PNG "pd_alpha", 300, 300, , Val(c$)
  For k = 0 To 6 : s$ = s$ + Choice(Pixel(300 + k, 300) <> bg, "1", "0") : Next
  Print "alpha cut, t "; Choice(t$ = "", "none", t$); ", cutoff "; Choice(c$ = "", "none", c$); ": "; s$
End Sub

Data %s
''' % (len(order), H - 1, W - 1, data)
open(os.path.join(out, 'pngdec.bas'), 'w', newline='\n').write(bas)

hx = lambda c: '%02X%02X%02X' % c
exp = []
for n in order:
    exp.append(n + ':')
    for y in range(H): exp.append('  ' + ' '.join(hx(images[n][y][x]) for x in range(W)))
exp.append('from 2, 1:')
for y in (1, 2): exp.append('  ' + ' '.join(hx(RGB[y][x]) for x in range(2, 6)))
exp.append('at x -2:  ' + ' '.join(hx(RGB[0][x]) for x in range(2, 6)))
exp.append('wider than the screen: ' + ' '.join(hx(STRIPES[i]) for i in range(10)) + ' ' + hx(STRIPES[9]))
exp.append('at x 637: %s %s' % (hx(RGB[0][0]), hx(RGB[0][2])))
exp.append('BLIT LOADPNG:  ' + ' '.join(hx(RGB[1][x]) for x in range(W)))
# DrawBuffer in 16-bit colour: a pixel is drawn when its alpha is at least the cut (t * 16, or cutoff's top four bits);
# with no t, or t 1 (a cut of 0), every pixel is drawn
def drawn(cutv): return ''.join('1' if a >= cutv else '0' for a in ALPHAS)
exp.append('alpha cut, t none, cutoff none: ' + drawn(0))
exp.append('alpha cut, t 1, cutoff none: ' + drawn(0))
exp.append('alpha cut, t 8, cutoff none: ' + drawn(128))
exp.append('alpha cut, t none, cutoff 200: ' + drawn(200 & 0xF0))
exp.append('indexed, transparent index 0, t 2: 0111')
exp.append('16 bits: Invalid format, must be 8-bit RGB, RGBA, grey or indexed PNG')
exp.append('not a PNG: Invalid or damaged PNG file')
exp.append('x offset 6: Offset outside the image')
exp.append('file 10 free')
open(os.path.join(out, 'pngdec.expected'), 'w', newline='\n').write('\n'.join(exp) + '\n')
for f in sorted(os.listdir(out)):
    if f.startswith('pd_'): print(f, os.path.getsize(os.path.join(out, f)))
