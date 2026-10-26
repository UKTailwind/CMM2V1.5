' behfix.bas - the behaviour fixes of table 5.1 (docs/PicoMite_Parity_Recheck.md) that a program can check (26 Oct
' 2026): the PicoMite's behaviour in each. Uses files bf_*.* in the current directory and RAM slot 6 (left empty).
Option Explicit
Option Default Integer
Dim tests, pass, v0, here, n, b, g, r, ms
Dim s$, t$

' 7 and 17: a DIM that runs out of memory says how much (error 48), and is not counted as a variable
v0 = MM.Info(VARCNT)
On Error Skip 1
Dim big!(10000000)
CheckS "DIM out of memory gives the size", MM.ErrMsg$, "Not enough memory for 80000008 bytes"
Check "and MM.INFO(VARCNT) is as it was", MM.Info(VARCNT), v0

' 8: RESTORE of a name that is a label and a variable too goes to the label (the PicoMite's #103)
here = 99
Restore here
Read n
Check "RESTORE label that is also a variable: the label's DATA", n, 42

' 10: MATH's complex functions and FFT with one argument say "Argument count" (the PicoMite's #62)
Dim f!(15)
On Error Skip 1
Print Math(C_ADD n)                                                ' (a complex is packed in an integer)
CheckS "MATH(C_ADD) with one argument", MM.ErrMsg$, "Argument count"
On Error Skip 1
Math FFT f!()
CheckS "MATH FFT with one argument", MM.ErrMsg$, "Argument count"

' 16: #COMMENT END with two spaces ends the block (the PicoMite's #80)
#COMMENT START
s$ = "inside the block"
#COMMENT  END
s$ = "after the block"
CheckS "#COMMENT  END with two spaces ends the block", s$, "after the block"

' 22: LONGSTRING MID with the largest count is cut to the string (nbr + start overflowed)
Dim integer ls(10), ld(10)
LongString Append ls(), "abcdef"
LongString Mid ld(), ls(), 3, &H7FFFFFFF
CheckS "LONGSTRING MID with count &H7FFFFFFF", LGetStr$(ld(), 1, LLen(ld())), "cdef"

' 12: FLASH LOAD IMAGE into a RAM slot: a file that fails to decode leaves the slot as it was
Save Image "bf_ok.bmp", 0, 0, 8, 8
Open "bf_ok.bmp" For Input As #1
s$ = Input$(Lof(#1), #1)
Close #1
Open "bf_bad.bmp" For Output As #2
Print #2, Left$(s$, 28) + Chr$(7) + Mid$(s$, 30);                ' its bit depth 7
Close #2
Flash Load Image 6, "bf_ok.bmp", O
Check "FLASH LOAD IMAGE 6: an 8 pixel wide image", Peek(Word MM.Info(Flash Address 6)), 8
On Error Skip 1
Flash Load Image 6, "bf_bad.bmp", O
CheckS "a BMP of bit depth 7 is refused", MM.ErrMsg$, "Unsupported bit depth"
Check "and the slot still holds the 8 pixel image", Peek(Word MM.Info(Flash Address 6)), 8
On Error Skip 1
Flash Erase 6

' 9: SAVE IMAGE in MODE n,4 saves the colours shown: MAP's once MAP SET shows them
Mode 7, 4
Cls
Pixel 0, 0, RGB(0, 0, 255)
Map 1 = RGB(255, 128, 0)
Save Image "bf_map1.bmp", 0, 0, 1, 1
Pixel3 "bf_map1.bmp"
Check "n,4: MAP n = c not yet set: blue is saved (B, G, R)", b * 65536 + g * 256 + r, &HFF0000
Map Set
Save Image "bf_map2.bmp", 0, 0, 1, 1
Pixel3 "bf_map2.bmp"
Check "after MAP SET: the mapped colour is saved", b * 65536 + g * 256 + r, &H0080FF
Map Reset
Mode 8

' 24: OPTION MILLISECONDS: TIME$'s milliseconds start at 0 when the time is set
Option Milliseconds On
Time$ = Left$(Time$, 8)
t$ = Time$
ms = Val(Mid$(t$, 10))
Option Milliseconds Off
Check "TIME$ set: its milliseconds start again below 100", ms < 100, 1

Kill "bf_ok.bmp" : Kill "bf_bad.bmp" : Kill "bf_map1.bmp" : Kill "bf_map2.bmp"
Print "behfix:"; pass; " of"; tests; " passed"
End

here:
Data 42

Sub Pixel3(f$)                                                     ' the first pixel of a 24-bit BMP: b, g, r
  Local a$
  Open f$ For Input As #3
  a$ = Input$(Lof(#3), #3)
  Close #3
  b = Asc(Mid$(a$, 55, 1)) : g = Asc(Mid$(a$, 56, 1)) : r = Asc(Mid$(a$, 57, 1))
End Sub

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub CheckS(n$, got$, want$)
  tests = tests + 1
  If got$ = want$ Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got "; got$; " want "; want$
  EndIf
End Sub
