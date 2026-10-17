' julia_m7.bas: made by tools/gen_csubtests.py from the PicoMite's mmb2csub/examples/julia.bas
' julia.bas - Julia set, with plotjulia taking everything it needs as
' parameters so it is a self-contained routine (the first mmb2csub target).
Mode 7,8
CLS
map maximite
'Specify initial values
RealOffset = -1.30
ImaginOffset = -1.22
'------------------------------------------------*
'Set the Julia set constant [eg C = -1.2 + 0.8i]
CRealVal = -0.78
CImagVal = -0.20
'------------------------------------------------*
MAXIT=80 'max iterations
PixelWidth = MM.HRes
PixelHeight = MM.VRes
GAP = PixelHeight / PixelWidth
SIZE = 2.50
XDelta = SIZE / PixelWidth
YDelta = (SIZE * GAP) / PixelHeight
' MAP() has no CSUB CallTable slot, and in a fixed mode its sixteen values are
' constants - so look them up once here and pass the table in.
Dim mp%(15)
For i% = 0 To 15 : mp%(i%) = map(i%) : Next i%
t = Timer
plotjulia PixelWidth, PixelHeight, XDelta, YDelta, RealOffset, ImaginOffset, CRealVal, CImagVal, MAXIT, mp%()
' the rendered image, for comparing this against the CSUB version byte for byte.
' SAVE IMAGE records the framebuffer and ignores the colour map, so what lands in
' the file is exactly what plotjulia computed.
t = Timer - t
' the picture's checksum, for comparing the versions pixel for pixel
s% = 0
For y% = 0 To PixelHeight - 1
  For x% = 0 To PixelWidth - 1
    s% = (s% * 31 + Pixel(x%, y%)) And &HFFFFFFFFFFFF
  Next x%
Next y%
Print "julia checksum"; s%
end
'
' w, h    picture size in pixels
' xd, yd  the step in the complex plane per pixel
' rOfs, iOfs   top-left corner of the view
' cRe, cIm     the Julia constant C
' mit          iteration limit
' mp%()        the sixteen colours, already resolved through MAP()
' --- mmb2csub: plotjulia is now the CSUB below (original in p.bas.bak)

' --- mmb2csub: generated code for plotjulia
CSUB plotjulia FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, INTEGER   ' Cortex-M7 (CMM2)
	00000000
	'plotjulia
	4DF0E92D 28E0F04F 4615461E F8D82200 46073D08 6A1B460C ED2D6FDB B0888B10 
	2200E9C3 3D08F8D8 F8D36A1B F8DBB07C F1BAA000 D1150F00 F44F6B1B 47985080 
	3D08F8D8 0000F8CB 6FDB6A1B A004F8C3 3D08F8D8 6FDB6A1B F5B2685A BF885F80 
	A004F8C3 23E0F04F 9B00EEB7 3D08F8D3 7B00ED97 6FDB6A1B 7B49EE37 8B62ED9F 
	ED8D685B 93037B00 21E0F04F 7B00ED9D 3D08F8D1 7BC8EEB4 6FDA6A1B FA10EEF1 
	9B03DA08 21002000 B0086053 8B10ECBD 8DF0E8BD 33016893 F3C36093 B9230309 
	3D08F8D1 6A9B6A1B 9B204798 7B00ED95 CB00ED93 BB00ED94 AB4BED9F CB08EEA7 
	BB49EE3B BBCAEEB4 21E0F04F 3D08F8D1 FA10EEF1 6FDA6A1B EE38DA02 E7C38B09 
	33016893 F3C36093 B9230309 3D08F8D1 6A9B6A1B 9B214798 DB4CEEB0 27E0F04F 
	7B00ED96 EB00ED93 FB37ED9F EB0AEEA7 F8D79A24 EE2D3D08 6A1B5B0D 6B0EEE2E 
	7B00ED92 FB47EEB4 FA10EEF1 EE35D808 EEB17B06 EEB44B00 EEF17BC4 D42AFA10 
	20ECF8D3 2BE0F04F 0B48EEB0 3088F8D3 A000F8D2 F8DB4798 EEB03D08 46070B4A 
	F8D36A1B 47983088 3D08F8DB 0B4FEEB0 6A1B4680 B148F8D3 3088F8D3 22104798 
	47D82300 46419B25 2030F853 47D04638 AB09EE3A 6FDAE796 33016893 F3C36093 
	B9630309 3D08F8D7 6A9B6A1B 6B06ED8D 5B04ED8D ED9D4798 ED9D6B06 9B225B04 
	6B46EE35 DB0DEE3D FB09EE3F 5B00ED93 ED939B23 EEAD7B00 EE367B0E EEB0DB05 
	E795EB47 8000F3AF 00000000 00000000 
End CSUB
