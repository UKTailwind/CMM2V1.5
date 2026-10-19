' bitflag.bas - BIT( BYTE( and FLAG( as commands and functions, FLAGS and MM.FLAGS (M5 of the b14 merge, the PicoMite's).
' The three share one token through the tokeniser's folds (BITBYTEFLAG(1, 2 or 3), as LEFT$( and the rest do: written
' every way, with spaces before the bracket and inside expressions, and LIST must show them as typed.
Option Explicit
Dim integer x = 0, n, bad = 0
Dim string s$ = "abcdef", e$
Const c = 5
Bit(x, 0) = 1 : Bit(x, 63) = 1 : Bit(x, 5) = 1
Print "bit set: "; Hex$(x, 16)
Bit(x, 5) = 0
Print "bit clear: "; Hex$(x, 16); " "; Bit(x, 0); Bit(x, 5); Bit(x, 63)
Byte(s$, 1) = 65 : Byte(s$, 6) = Asc("Z")
Print "byte set: "; s$; " "; Byte(s$, 1); Byte(s$, 2); Byte(s$, 6)
Flags = 0
Flag(3) = 1 : Flag(62) = 1
Print "flags: "; Hex$(MM.Flags, 16); " "; Flag(3); Flag(4); Flag(62)
Flag(3) = 0
Flags = &H1234
Print "flags set: "; Hex$(MM.Flags); " "; Flag(2); Flag(4); Flag(12)
On Error Skip 1
Bit(s$, 1) = 1
e$ = MM.ErrMsg$ : Print "bit of a string: "; e$
On Error Skip 1
Byte(x, 1) = 1
e$ = MM.ErrMsg$ : Print "byte of an integer: "; e$
On Error Skip 1
Bit(c, 1) = 1
e$ = MM.ErrMsg$ : Print "bit of a constant: "; e$
On Error Skip 1
Bit(x, 64) = 1
e$ = MM.ErrMsg$ : Print "bit 64: "; e$
On Error Skip 1
n = Byte(s$, 0)
e$ = MM.ErrMsg$ : Print "byte 0: "; e$
On Error Skip 1
n = Flag(3, 4)
e$ = MM.ErrMsg$ : Print "flag two args: "; e$
Print "spaced: "; Bit (x, 0); Byte (s$, 1); Flag (12)
Print "in expr: "; Bit(x, 0) + Byte(s$, 1) * 2 + Flag(12)
Print "lower: "; bit(x, 63); byte(s$, 6); flag(4)
Print "done"
