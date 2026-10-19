' ctlflow.bas - IF/ELSEIF/ELSE/ENDIF and DO/LOOP in every form the plan's step 7 makes faster (the IF jump table,
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
If d >= 0 Then
  If d >= 1 Then
    If d >= 2 Then
      If d >= 3 Then
        If d >= 4 Then
          If d >= 5 Then
            If d >= 6 Then
              If d >= 7 Then
                If d >= 8 Then
                  If d >= 9 Then
                    If d >= 10 Then
                      If d >= 11 Then
                        If d >= 12 Then
                          If d >= 13 Then
                            If d >= 14 Then
                              If d >= 15 Then
                                If d >= 16 Then
                                  If d >= 17 Then
                                    If d >= 18 Then
                                      If d >= 19 Then
                                        If d >= 20 Then
                                          If d >= 21 Then
                                            If d >= 22 Then
                                              If d >= 23 Then
                                                If d >= 24 Then
                                                  If d >= 25 Then
                                                    If d >= 26 Then
                                                      If d >= 27 Then
                                                        If d >= 28 Then
                                                          If d >= 29 Then
                                                            If d >= 30 Then
                                                              If d >= 31 Then
                                                                If d >= 32 Then
                                                                  If d >= 33 Then
                                                                    If d >= 34 Then
                                                                      If d >= 35 Then
                                                                        If d >= 36 Then
                                                                          If d >= 37 Then
                                                                            If d >= 38 Then
                                                                              If d >= 39 Then
                                                                                If d >= 40 Then
                                                                                  If d >= 41 Then
                                                                                    If d >= 42 Then
                                                                                      If d >= 43 Then
                                                                                        If d >= 44 Then
                                                                                          If d >= 45 Then
                                                                                            If d >= 46 Then
                                                                                              If d >= 47 Then
                                                                                                If d >= 48 Then
                                                                                                  If d >= 49 Then
                                                                                                    If d >= 50 Then
                                                                                                      If d >= 51 Then
                                                                                                        If d >= 52 Then
                                                                                                          If d >= 53 Then
                                                                                                            If d >= 54 Then
                                                                                                              If d >= 55 Then
                                                                                                                If d >= 56 Then
                                                                                                                  If d >= 57 Then
                                                                                                                    If d >= 58 Then
                                                                                                                      If d >= 59 Then
                                                                                                                        If d >= 60 Then
                                                                                                                          If d >= 61 Then
                                                                                                                            If d >= 62 Then
                                                                                                                              If d >= 63 Then
                                                                                                                                If d >= 64 Then
                                                                                                                                  If d >= 65 Then
                                                                                                                                    If d >= 66 Then
                                                                                                                                      If d >= 67 Then
                                                                                                                                        If d >= 68 Then
                                                                                                                                          If d >= 69 Then
                                                                                                                                            Print "deep: reached "; d
                                                                                                                                          Else
                                                                                                                                            Print "deep: not at 69" : GoTo deepdone
                                                                                                                                          EndIf
                                                                                                                                        Else
                                                                                                                                          Print "deep: not at 68" : GoTo deepdone
                                                                                                                                        EndIf
                                                                                                                                      Else
                                                                                                                                        Print "deep: not at 67" : GoTo deepdone
                                                                                                                                      EndIf
                                                                                                                                    Else
                                                                                                                                      Print "deep: not at 66" : GoTo deepdone
                                                                                                                                    EndIf
                                                                                                                                  Else
                                                                                                                                    Print "deep: not at 65" : GoTo deepdone
                                                                                                                                  EndIf
                                                                                                                                Else
                                                                                                                                  Print "deep: not at 64" : GoTo deepdone
                                                                                                                                EndIf
                                                                                                                              Else
                                                                                                                                Print "deep: not at 63" : GoTo deepdone
                                                                                                                              EndIf
                                                                                                                            Else
                                                                                                                              Print "deep: not at 62" : GoTo deepdone
                                                                                                                            EndIf
                                                                                                                          Else
                                                                                                                            Print "deep: not at 61" : GoTo deepdone
                                                                                                                          EndIf
                                                                                                                        Else
                                                                                                                          Print "deep: not at 60" : GoTo deepdone
                                                                                                                        EndIf
                                                                                                                      Else
                                                                                                                        Print "deep: not at 59" : GoTo deepdone
                                                                                                                      EndIf
                                                                                                                    Else
                                                                                                                      Print "deep: not at 58" : GoTo deepdone
                                                                                                                    EndIf
                                                                                                                  Else
                                                                                                                    Print "deep: not at 57" : GoTo deepdone
                                                                                                                  EndIf
                                                                                                                Else
                                                                                                                  Print "deep: not at 56" : GoTo deepdone
                                                                                                                EndIf
                                                                                                              Else
                                                                                                                Print "deep: not at 55" : GoTo deepdone
                                                                                                              EndIf
                                                                                                            Else
                                                                                                              Print "deep: not at 54" : GoTo deepdone
                                                                                                            EndIf
                                                                                                          Else
                                                                                                            Print "deep: not at 53" : GoTo deepdone
                                                                                                          EndIf
                                                                                                        Else
                                                                                                          Print "deep: not at 52" : GoTo deepdone
                                                                                                        EndIf
                                                                                                      Else
                                                                                                        Print "deep: not at 51" : GoTo deepdone
                                                                                                      EndIf
                                                                                                    Else
                                                                                                      Print "deep: not at 50" : GoTo deepdone
                                                                                                    EndIf
                                                                                                  Else
                                                                                                    Print "deep: not at 49" : GoTo deepdone
                                                                                                  EndIf
                                                                                                Else
                                                                                                  Print "deep: not at 48" : GoTo deepdone
                                                                                                EndIf
                                                                                              Else
                                                                                                Print "deep: not at 47" : GoTo deepdone
                                                                                              EndIf
                                                                                            Else
                                                                                              Print "deep: not at 46" : GoTo deepdone
                                                                                            EndIf
                                                                                          Else
                                                                                            Print "deep: not at 45" : GoTo deepdone
                                                                                          EndIf
                                                                                        Else
                                                                                          Print "deep: not at 44" : GoTo deepdone
                                                                                        EndIf
                                                                                      Else
                                                                                        Print "deep: not at 43" : GoTo deepdone
                                                                                      EndIf
                                                                                    Else
                                                                                      Print "deep: not at 42" : GoTo deepdone
                                                                                    EndIf
                                                                                  Else
                                                                                    Print "deep: not at 41" : GoTo deepdone
                                                                                  EndIf
                                                                                Else
                                                                                  Print "deep: not at 40" : GoTo deepdone
                                                                                EndIf
                                                                              Else
                                                                                Print "deep: not at 39" : GoTo deepdone
                                                                              EndIf
                                                                            Else
                                                                              Print "deep: not at 38" : GoTo deepdone
                                                                            EndIf
                                                                          Else
                                                                            Print "deep: not at 37" : GoTo deepdone
                                                                          EndIf
                                                                        Else
                                                                          Print "deep: not at 36" : GoTo deepdone
                                                                        EndIf
                                                                      Else
                                                                        Print "deep: not at 35" : GoTo deepdone
                                                                      EndIf
                                                                    Else
                                                                      Print "deep: not at 34" : GoTo deepdone
                                                                    EndIf
                                                                  Else
                                                                    Print "deep: not at 33" : GoTo deepdone
                                                                  EndIf
                                                                Else
                                                                  Print "deep: not at 32" : GoTo deepdone
                                                                EndIf
                                                              Else
                                                                Print "deep: not at 31" : GoTo deepdone
                                                              EndIf
                                                            Else
                                                              Print "deep: not at 30" : GoTo deepdone
                                                            EndIf
                                                          Else
                                                            Print "deep: not at 29" : GoTo deepdone
                                                          EndIf
                                                        Else
                                                          Print "deep: not at 28" : GoTo deepdone
                                                        EndIf
                                                      Else
                                                        Print "deep: not at 27" : GoTo deepdone
                                                      EndIf
                                                    Else
                                                      Print "deep: not at 26" : GoTo deepdone
                                                    EndIf
                                                  Else
                                                    Print "deep: not at 25" : GoTo deepdone
                                                  EndIf
                                                Else
                                                  Print "deep: not at 24" : GoTo deepdone
                                                EndIf
                                              Else
                                                Print "deep: not at 23" : GoTo deepdone
                                              EndIf
                                            Else
                                              Print "deep: not at 22" : GoTo deepdone
                                            EndIf
                                          Else
                                            Print "deep: not at 21" : GoTo deepdone
                                          EndIf
                                        Else
                                          Print "deep: not at 20" : GoTo deepdone
                                        EndIf
                                      Else
                                        Print "deep: not at 19" : GoTo deepdone
                                      EndIf
                                    Else
                                      Print "deep: not at 18" : GoTo deepdone
                                    EndIf
                                  Else
                                    Print "deep: not at 17" : GoTo deepdone
                                  EndIf
                                Else
                                  Print "deep: not at 16" : GoTo deepdone
                                EndIf
                              Else
                                Print "deep: not at 15" : GoTo deepdone
                              EndIf
                            Else
                              Print "deep: not at 14" : GoTo deepdone
                            EndIf
                          Else
                            Print "deep: not at 13" : GoTo deepdone
                          EndIf
                        Else
                          Print "deep: not at 12" : GoTo deepdone
                        EndIf
                      Else
                        Print "deep: not at 11" : GoTo deepdone
                      EndIf
                    Else
                      Print "deep: not at 10" : GoTo deepdone
                    EndIf
                  Else
                    Print "deep: not at 9" : GoTo deepdone
                  EndIf
                Else
                  Print "deep: not at 8" : GoTo deepdone
                EndIf
              Else
                Print "deep: not at 7" : GoTo deepdone
              EndIf
            Else
              Print "deep: not at 6" : GoTo deepdone
            EndIf
          Else
            Print "deep: not at 5" : GoTo deepdone
          EndIf
        Else
          Print "deep: not at 4" : GoTo deepdone
        EndIf
      Else
        Print "deep: not at 3" : GoTo deepdone
      EndIf
    Else
      Print "deep: not at 2" : GoTo deepdone
    EndIf
  Else
    Print "deep: not at 1" : GoTo deepdone
  EndIf
Else
  Print "deep: not at 0" : GoTo deepdone
EndIf
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
Print "ELSEIF 1/0: "; "in line " + Str$(MM.ErrLine) + ": " + MM.ErrMsg$
On Error Skip
If 0 Then
  Print "no"
ElseIf
  Print "no"
EndIf
Print "ELSEIF with no condition: "; "in line " + Str$(MM.ErrLine) + ": " + MM.ErrMsg$

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
