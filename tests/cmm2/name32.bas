' name32.bas - an error's message naming a variable of 32 characters, the longest (1 Nov 2026: findvar left such a
' name unterminated, and "Different type already declared" and the others printed what followed it on the stack).
' Each message is printed between bars, so anything after the name shows.
Option Explicit
Dim integer abcdefghijklmnopqrstuvwxyz012345 = 1
On Error Skip
abcdefghijklmnopqrstuvwxyz012345$ = "x"
Print "|"; MM.ErrMsg$; "|"
On Error Skip
Dim integer abcdefghijklmnopqrstuvwxyz012345
Print "|"; MM.ErrMsg$; "|"
On Error Skip
ABCDEFGHIJKLMNOPQRSTUVWXYZ0123XY = 2
Print "|"; MM.ErrMsg$; "|"
S
Sub S
  Local float b234567890123456789012345678901Z
  On Error Skip
  Local float b234567890123456789012345678901Z
  Print "|"; MM.ErrMsg$; "|"
End Sub
