"""gen_bmps.py DIR  (python tools/gen_bmps.py tests/cmm2)
Writes the BMP test images for tests/cmm2/bmpdec.bas, the program, and its expected output, which comes from the
pixels each image was made with (not from a run): 1, 4 and 8-bit palettes, a V5 header, a short palette, a grey file
with no palette, RLE8 and RLE4, 16-bit 555 and 565 (masks after a V3 header and inside a V4 one), 24-bit bottom-up and
top-down, odd widths for the row padding, a sub-image, clipping, and a file that is not a BMP."""
import struct, sys, os
out = sys.argv[1]

def info(w, h, bpp, comp, hdr, clrused=0, masks=None, sizeimage=0):
    b = struct.pack('<IiiHHIIiiII', hdr, w, h, 1, bpp, comp, sizeimage, 2835, 2835, clrused, 0)
    if hdr >= 108:
        m = masks or (0, 0, 0)
        b += struct.pack('<IIII', m[0], m[1], m[2], 0) + struct.pack('<I', 0x73524742) + bytes(36) + bytes(12)
    if hdr == 124:
        b += struct.pack('<IIII', 4, 0, 0, 0)
    assert len(b) == hdr
    return b

def write(name, w, h, bpp, comp, hdr, data, palette=(), clrused=0, masks=None, v3masks=False, offbits=None):
    pal = b''.join(struct.pack('<BBBB', bl, g, r, 0) for (r, g, bl) in palette)
    extra = struct.pack('<III', *masks) if v3masks else b''
    off = offbits if offbits is not None else 14 + hdr + len(extra) + len(pal)
    body = info(w, h, bpp, comp, hdr, clrused, masks, len(data)) + extra + pal
    body += bytes(off - 14 - len(body)) if off - 14 > len(body) else b''
    f = b'BM' + struct.pack('<IHHI', 14 + len(body) + len(data), 0, 0, off) + body + data
    open(os.path.join(out, name + '.bmp'), 'wb').write(f)

def rows(w, h, rowbytes, topdown=False):
    """the file's rows, each padded to four bytes: bottom-up unless topdown"""
    order = range(h) if topdown else range(h - 1, -1, -1)
    d = b''
    for y in order:
        r = rowbytes(y)
        d += r + bytes((-len(r)) % 4)
    return d

def pack_bits(vals, bits):
    out, acc, n = bytearray(), 0, 0
    for v in vals:
        acc = (acc << bits) | v; n += bits
        if n == 8: out.append(acc); acc = n = 0
    if n: out.append(acc << (8 - n))
    return bytes(out)

images = []                                                         # name, w, h, pixel(x, y) -> (r, g, b)
def rgb(c): return '%02X%02X%02X' % c

# 1-bit, 9 wide
p1 = [(10, 20, 30), (200, 150, 100)]
i1 = lambda x, y: (x + x // 3 + y) % 2
write('bd_p1', 9, 3, 1, 0, 40, rows(9, 3, lambda y: pack_bits([i1(x, y) for x in range(9)], 1)), p1)
images.append(('bd_p1', 9, 3, lambda x, y: p1[i1(x, y)]))
# 4-bit, 7 wide
p4 = [(i * 16, 255 - i * 16, (i * 37) & 255) for i in range(16)]
i4 = lambda x, y: (x * 3 + y * 5) % 16
write('bd_p4', 7, 3, 4, 0, 40, rows(7, 3, lambda y: pack_bits([i4(x, y) for x in range(7)], 4)), p4)
images.append(('bd_p4', 7, 3, lambda x, y: p4[i4(x, y)]))
# 8-bit, 6 wide; the same with a V5 header (its extra bytes are skipped from where the file is)
p8 = [(i, (i * 7) & 255, 255 - i) for i in range(256)]
i8 = lambda x, y: (x * 40 + y * 7) & 255
write('bd_p8', 6, 3, 8, 0, 40, rows(6, 3, lambda y: bytes(i8(x, y) for x in range(6))), p8)
images.append(('bd_p8', 6, 3, lambda x, y: p8[i8(x, y)]))
write('bd_p8v5', 6, 3, 8, 0, 124, rows(6, 3, lambda y: bytes(i8(x, y) for x in range(6))), p8)
images.append(('bd_p8v5', 6, 3, lambda x, y: p8[i8(x, y)]))
# 8-bit with a palette of four (biClrUsed)
pc = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0)]
ic = lambda x, y: (x + y) % 4
write('bd_p8c4', 5, 2, 8, 0, 40, rows(5, 2, lambda y: bytes(ic(x, y) for x in range(5))), pc, clrused=4)
images.append(('bd_p8c4', 5, 2, lambda x, y: pc[ic(x, y)]))
# 8-bit with no palette at all: grey levels (as the b7 decoder read it)
gv = lambda x, y: x * 50 + y * 10
write('bd_grey', 5, 2, 8, 0, 40, rows(5, 2, lambda y: bytes(gv(x, y) for x in range(5))), (), offbits=54)
images.append(('bd_grey', 5, 2, lambda x, y: (gv(x, y),) * 3))
# RLE8, 8 by 3: the file's first line is the bottom one
r8 = [[10, 10, 10, 20, 30, 40, 50, 50], [60] * 8, [1, 2, 3, 4, 5, 5, 5, 5]]   # file lines, bottom first
d = bytes([3, 10, 0, 3, 20, 30, 40, 0, 2, 50, 0, 0])               # a run, three absolute (padded to a word), a run, EOL
d += bytes([8, 60, 0, 0])
d += bytes([0, 4, 1, 2, 3, 4, 4, 5, 0, 1])                          # four absolute, a run, end of bitmap
write('bd_r8', 8, 3, 8, 1, 40, d, p8)
images.append(('bd_r8', 8, 3, lambda x, y: p8[r8[2 - y][x]]))
# RLE4, 8 by 2
r4 = [[1, 2, 1, 2, 1, 3, 4, 5], [7] * 8]
d = bytes([5, 0x12, 0, 3, 0x34, 0x50, 0, 0])                        # a run of 1 2 1 2 1, three absolute, EOL
d += bytes([8, 0x77, 0, 1])
write('bd_r4', 8, 2, 4, 2, 40, d, p4)
images.append(('bd_r4', 8, 2, lambda x, y: p4[r4[1 - y][x]]))
# 16-bit 555 (BI_RGB), 565 with the masks after a V3 header, and 565 with them inside a V4 header
c5 = lambda x, y: ((x * 7 + y) % 32, (x * 3 + y * 11) % 32, (y * 13 + x) % 32)
w555 = lambda y: b''.join(struct.pack('<H', (r << 10) | (g << 5) | b) for (r, g, b) in (c5(x, y) for x in range(5)))
write('bd_555', 5, 3, 16, 0, 40, rows(5, 3, w555))
images.append(('bd_555', 5, 3, lambda x, y: tuple(v * 255 // 31 for v in c5(x, y))))
c6 = lambda x, y: ((x * 7 + y) % 32, (x * 9 + y * 11) % 64, (y * 13 + x) % 32)
w565 = lambda y: b''.join(struct.pack('<H', (r << 11) | (g << 5) | b) for (r, g, b) in (c6(x, y) for x in range(5)))
e565 = lambda x, y: (c6(x, y)[0] * 255 // 31, c6(x, y)[1] * 255 // 63, c6(x, y)[2] * 255 // 31)
write('bd_565', 5, 3, 16, 3, 40, rows(5, 3, w565), masks=(0xF800, 0x07E0, 0x001F), v3masks=True)
images.append(('bd_565', 5, 3, e565))
write('bd_565v4', 5, 3, 16, 3, 108, rows(5, 3, w565), masks=(0xF800, 0x07E0, 0x001F))
images.append(('bd_565v4', 5, 3, e565))
# 24-bit, bottom-up and top-down
c24 = lambda x, y: (x * 50 + 5, y * 80 + 3, (x * y * 31 + 7) & 255)
w24 = lambda y: b''.join(struct.pack('<BBB', b, g, r) for (r, g, b) in (c24(x, y) for x in range(5)))
write('bd_24', 5, 3, 24, 0, 40, rows(5, 3, w24))
images.append(('bd_24', 5, 3, c24))
f = os.path.join(out, 'bd_24td.bmp')
write('bd_24td', 5, 3, 24, 0, 40, rows(5, 3, w24, topdown=True))
b = bytearray(open(f, 'rb').read()); struct.pack_into('<i', b, 22, -3); open(f, 'wb').write(b)   # biHeight negative
images.append(('bd_24td', 5, 3, c24))
open(os.path.join(out, 'bd_bad.bmp'), 'wb').write(b'This is not a BMP file.\n' + bytes(40))

# the program and its expected output
bas = ["' bmpdec.bas - LOAD BMP through the PicoMite's decoder (M11 of the b14 merge): every kind of file it reads, made",
       "' by gen_bmps.py with known pixels, and read back with PIXEL() in 32-bit colour; a sub-image, clipping at the left",
       "' edge, and a file that is not a BMP.  The expected output was written from the pixels the images were made with.",
       "Option Explicit",
       "Dim string n$, r$",
       "Dim integer w, h, x, y, i",
       "Mode 8, 32",
       "For i = 1 To %d" % len(images),
       "  Read n$, w, h",
       "  CLS",
       "  Load BMP n$, 300, 300",                    # (clear of the text PRINT draws at the top)
       "  Print n$; \":\"",
       "  For y = 0 To h - 1",
       "    r$ = \" \"",
       "    For x = 0 To w - 1 : r$ = r$ + \" \" + Hex$(Pixel(300 + x, 300 + y), 6) : Next",
       "    Print r$",
       "  Next",
       "Next",
       "CLS",
       "Load BMP \"bd_24\", 300, 300, , 2, 1",
       "Print \"from 2, 1:\"",
       "For y = 0 To 2",
       "  r$ = \" \"",
       "  For x = 0 To 3 : r$ = r$ + \" \" + Hex$(Pixel(300 + x, 300 + y), 6) : Next",
       "  Print r$",
       "Next",
       "CLS",
       "Load BMP \"bd_24\", -2, 300",
       "Print \"at x -2:\"",
       "For y = 0 To 2",
       "  r$ = \" \"",
       "  For x = 0 To 3 : r$ = r$ + \" \" + Hex$(Pixel(x, 300 + y), 6) : Next",
       "  Print r$",
       "Next",
       "CLS",
       "On Error Skip 1",
       "Load BMP \"bd_bad\"",
       "Print \"not a BMP: \"; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, \":\") + 2)",
       "Open \"bmpdec.tmp\" For Output As #10 : Close #10 : Kill \"bmpdec.tmp\"",
       "Print \"file 10 free\"",
       "Mode 8, 8",
       "Print \"done\"",
       "End"]
bas += ['Data "%s", %d, %d' % (n, w, h) for (n, w, h, _) in images]
exp = []
for (n, w, h, px) in images:
    exp.append(n + ':')
    for y in range(h):
        exp.append(' ' + ''.join(' ' + rgb(px(x, y)) for x in range(w)))
exp.append('from 2, 1:')
for y in range(3):
    exp.append(' ' + ''.join(' ' + (rgb(c24(2 + x, 1 + y)) if x < 3 and y < 2 else '000000') for x in range(4)))
exp.append('at x -2:')
for y in range(3):
    exp.append(' ' + ''.join(' ' + (rgb(c24(2 + x, y)) if x < 3 else '000000') for x in range(4)))
exp.append('not a BMP: BMP: Invalid file signature (not a BMP file)')
exp.append('file 10 free')
exp.append('done')
open(os.path.join(out, 'bmpdec.bas'), 'w', newline='\n').write('\n'.join(bas) + '\n')
open(os.path.join(out, 'bmpdec.expected'), 'w', newline='\n').write('\n'.join(exp) + '\n')
print(len(images), 'images')
