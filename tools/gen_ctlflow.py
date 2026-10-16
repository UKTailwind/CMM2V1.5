"""gen_ctlflow.py DIR  (python tools/gen_ctlflow.py tests/cmm2)
Writes tests/cmm2/ctlflow.bas: IF/ELSEIF/ELSE/ENDIF and DO/LOOP in every form step 7 of the plan touches (the IF jump
table and the DO fast path), so the faster paths can be checked against the slower ones. Its expected output is the
firmware's before step 7, checked by hand. The IF nest deeper than the table's 64 levels is generated."""
import os, sys
out = sys.argv[1]
deep = 70
nest = []
for i in range(deep):
    nest.append('  ' * i + 'If d >= %d Then' % i)
nest.append('  ' * deep + 'Print "deep: reached "; d')
for i in reversed(range(deep)):
    nest.append('  ' * i + 'Else')
    nest.append('  ' * i + '  Print "deep: not at %d" : GoTo deepdone' % i)
    nest.append('  ' * i + 'EndIf')
bas = r'''' ctlflow.bas - IF/ELSEIF/ELSE/ENDIF and DO/LOOP in every form the plan's step 7 makes faster (the IF jump table,
' the DO fast path), with EXIT, CONTINUE and NEXT i, j. Made by tools/gen_ctlflow.py; the expected output is the
' firmware's before step 7.

Dim integer i, j, k, n, calls, d, zz
Dim float f
Dim string s$

Print "-- IF arms"
For i = 0 To 4
  calls = 0
  If t(i = 0) Then
    s$ = "IF"
  ElseIf t(i = 1) Then
    s$ = "ELSEIF 1"
  Else If t(i = 2) Then
    s$ = "ELSE IF 2"
  ElseIf t(i = 3) Then ' a comment after THEN
    s$ = "ELSEIF 3"
  Else
    s$ = "ELSE"
  End If
  Print i; " "; s$; ", conditions evaluated "; calls
Next

Print "-- nested, with single-line IFs inside"
For i = 0 To 3
  If i < 2 Then
    If i = 0 Then Print " a"; Else Print " b";
    If i = 0 Then
      Print " c";
    Else
      If i = 1 Then Print " d";
    EndIf
  ElseIf i = 2 Then
    For j = 1 To 2
      If j = 1 Then
        Print " e";
      Else
        Print " f";
      EndIf
    Next
  Else
    Select Case i
      Case 3
        If i = 3 Then Print " g";
    End Select
  EndIf
Next
Print

Print "-- GOTO out of and into IF blocks"
For i = 0 To 1
  If i = 0 Then
    GoTo out1
  Else
    Print " ELSE arm of"; i;
  EndIf
out1:
  Print " after"; i;
Next
Print
i = 0
GoTo into
If i = 5 Then
  Print " never"
Else
into:
  Print " jumped into the ELSE arm";
EndIf
Print ", then past ENDIF"

Print "-- deeper than 64"
For d = 68 To 71
''' + '\n'.join(nest) + r'''
deepdone:
Next

Print "-- in a SUB and a FUNCTION, recursion"
For i = 0 To 2 : Print classify$(i); " "; : Next
Print fact(10)

Print "-- errors"
zz = 0
On Error Skip
If 0 Then
  Print "no"
ElseIf 1 / zz Then
  Print "no"
EndIf
Print "ELSEIF 1/0: "; Mid$(MM.ErrMsg$, 7)
On Error Skip
If 0 Then
  Print "no"
ElseIf
  Print "no"
EndIf
Print "ELSEIF with no condition: "; Mid$(MM.ErrMsg$, 7)

Print "-- DO forms"
i = 0 : Do While i < 5 : i = i + 1 : Loop : Print "DO WHILE i < 5:"; i
i = 0 : Do Until i >= 5 : i = i + 1 : Loop : Print "DO UNTIL i >= 5:"; i
i = 0 : Do : i = i + 1 : Loop While i < 5 : Print "LOOP WHILE i < 5:"; i
i = 0 : Do : i = i + 1 : Loop Until i = 5 : Print "LOOP UNTIL i = 5:"; i
i = 0 : Do While 5 > i : i = i + 1 : Loop : Print "DO WHILE 5 > i:"; i
i = 0 : Do While i <= &H10 : i = i + 1 : Loop : Print "DO WHILE i <= &H10:"; i
i = 0 : Do While i < 2.5 : i = i + 1 : Loop : Print "DO WHILE i < 2.5 (an integer against a float):"; i
f = 0 : Do While f < 1 : f = f + 0.25 : Loop : Print "DO WHILE f < 1 (a float):"; f
f = 0 : Do Until f >= 1E1 : f = f + 2.5 : Loop : Print "DO UNTIL f >= 1E1:"; f
i = 10 : Do While i <> 0 : i = i - 3 : If i < 0 Then i = 0
Loop : Print "DO WHILE i <> 0:"; i
i = 3 : Do While i : i = i - 1 : Loop : Print "DO WHILE i (no comparison):"; i
i = 0 : Do While i < 2 * 3 : i = i + 1 : Loop : Print "DO WHILE i < 2 * 3 (an expression):"; i
s$ = "" : Do While s$ < "ccc" : s$ = s$ + "c" : Loop : Print "DO WHILE s$ < ""ccc"" (a string): "; s$
i = 0 : Do While i < 3 : bump i : Loop : Print "changed by a SUB:"; i
Print "in a recursive SUB:"; down(4)
i = 0 : Do While newv < 2 : newv = newv + 1 : i = i + 1 : Loop : Print "a variable made by the loop:"; i

Print "-- EXIT, CONTINUE, NEXT i, j"
n = 0
For i = 1 To 3
  For j = 1 To 3
    If j = 2 Then Continue For
    n = n + i * 10 + j
  Next j
Next i
Print "CONTINUE FOR:"; n
n = 0 : i = 0
Do While i < 6
  i = i + 1
  If i Mod 2 = 0 Then Continue Do
  n = n + i
Loop
Print "CONTINUE DO:"; n
i = 0
Do
  i = i + 1
  If i = 4 Then Exit Do
Loop
Print "EXIT DO:"; i
For i = 1 To 10
  If i = 3 Then Exit For
Next
Print "EXIT FOR:"; i
n = 0
For i = 1 To 2 : For j = 1 To 3 : n = n + 1 : Next j, i
Print "NEXT j, i:"; n
k = 0
Do While k < 3
  k = k + 1
  i = 0
  Do Until i = k
    i = i + 1
  Loop
  Print " inner"; i;
Loop
Print
Print "done"
End

Function t(c)
  calls = calls + 1
  t = c
End Function

Function zero()
  zero = 0
End Function

Function classify$(v)
  If v = 0 Then
    classify$ = "zero"
  ElseIf v = 1 Then
    classify$ = "one"
  Else
    classify$ = "many"
  EndIf
End Function

Function fact(v)
  If v <= 1 Then
    fact = 1
  Else
    fact = v * fact(v - 1)
  EndIf
End Function

Sub bump x As integer
  x = x + 1
End Sub

Function down(v) As integer
  Local integer c = v
  Do While c > 0
    c = c - 1
    If v > 1 Then down = down + down(v - 1)
  Loop
  down = down + 1
End Function
'''
open(os.path.join(out, 'ctlflow.bas'), 'w', newline='\n').write(bas)
print(len(bas.splitlines()), 'lines')
