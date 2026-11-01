' forif.bas - a FOR, DO or WHILE right after a single-line IF's THEN or ELSE, inside another loop (1 Nov 2026:
' ForFindNext and DoFindLoop did not count it, so the outer loop took the inner NEXT or LOOP for its own, and its own
' stopped with "Cannot find a matching FOR" whenever the IF was false; Gauntlet's status panel, PicoMite #147). A NEXT
' right after THEN (IF c THEN NEXT i) is a second way to the loop's end and must not change which NEXT is its own.
Option Default Integer
Dim key(2), bomb(2)

Print "false IF:";
For a = 1 To 2
  If 0 Then For i = 1 To 2: Print " x";: Next i
  Print a;
Next a
Print

Print "true IF:";
For a = 1 To 2
  If 1 Then For i = 1 To 2: Print " y";: Next i
Next a
Print

Print "FOR after ELSE:";
For a = 1 To 2
  If a = 9 Then x = 1 Else For i = 1 To 2: Print " e";: Next i
Next a
Print

Print "IF THEN IF THEN FOR:";
For a = 1 To 2
  If 1 Then If 0 Then For i = 1 To 2: Print " w";: Next i
  Print a;
Next a
Print

Print "IF c THEN NEXT, not taken:";
For a = 1 To 3
  If a = 9 Then Next a
  Print a;
Next a
Print

Print "outer loop skipped:";
For a = 5 To 1
  If 1 Then For i = 1 To 2: Print " q";: Next i
Next a
Print " done"

Print "DO in IF in DO:";
n = 0
Do
  n = n + 1
  If 0 Then Do: Print " z";: Loop
  Print n;
Loop Until n >= 2
Print

Print "Gauntlet's panel:";
For ttt = 1 To 2
  If key(ttt) > 0 Then For i = 1 To key(ttt): Print " k";: Next i
  If bomb(ttt) > 0 Then For i = 1 To bomb(ttt): Print " b";: Next i
  Print ttt;
  key(ttt) = ttt : bomb(ttt) = 1
Next ttt
For ttt = 1 To 2
  If key(ttt) > 0 Then For i = 1 To key(ttt): Print " k";: Next i
  If bomb(ttt) > 0 Then For i = 1 To bomb(ttt): Print " b";: Next i
  Print ttt;
Next ttt
Print
