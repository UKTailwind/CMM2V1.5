' prep6.bas - fixes found while preparing symbols (step 6.0): TRACE LIST after END TYPE (its 0xFF ended the walk),
' NEXT JI closing FOR I, the SPI( function (the SPI substitution made it SPI 1(), a name in a lower-case DATA
' line (looked up in lower case), and an #INCLUDE file's MMDEBUG lines (never dropped).
#Include "prep6.inc"
Type T
  x As integer
End Type
Dim integer I, JI, y, x = 5
Dim s$
For I = 2 To 1
  For JI = 1 To 2
  Next JI
  s$ = "inside"
Next I
Print "a FOR that does not run skips to its own NEXT: ["; s$; "]"
SPI Open 1000000, 0, 8
y = SPI(&H55)
SPI Close
Print "SPI( is a function"
data x+1
Read y
Print "a name in a lower-case DATA line: y ="; y
IncSub
Trace On
For I = 1 To 2
Next
Trace Off
Print
Print "TRACE LIST after END TYPE:"
Trace List 3
Print
