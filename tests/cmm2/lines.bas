' lines.bas - P1 step 3: every line of the image starts T_NEWLINE, skip, file, line_hi, line_lo.
' Walks the image with PEEK(PROGMEM) by the skip bytes and by the elements, and checks what
' the header feeds: error messages, TRACE, TRACE LIST, READ/DATA, labels and MM.INFO$(CURRENT),
' in the program and in an #INCLUDE file (lines.inc).  The long lines push the skip byte past
' 0xFD, where a line is walked by its elements instead.
Dim tests%, pass%, s$, i%, j%, e$, names$
Dim nlines%, nlong%, nnames%, bad%, asc1%, asc2%
s$ = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
s$ = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
s$ = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
s$ = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
s$ = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
Check "the long lines kept their text", Len(s$), 243

' ---- the image itself ------------------------------------------------------------------
WalkImage
Check "the image starts with the program's name line", bad% And 1, 0
Check "each skip byte lands where the elements end", bad% And 2, 0
Check "every line starts with T_NEWLINE and ends the image with a zero", bad% And 4, 0
Check "the program's lines ascend", asc1%, 1
Check "the #INCLUDE file's lines ascend", asc2%, 1
Check "two name lines", nnames%, 2
Check "some lines are longer than 0xFD bytes", nlong% >= 1, 1
CheckS "the name lines", names$, "LINES.BAS,lines.inc"

' ---- errors name the line and the file --------------------------------------------------
On Error Skip 1
Error "main"
e$ = MM.ErrMsg$
CheckS "an error in the program", e$, "Error in line 28: main"
On Error Ignore
IncError
e$ = MM.ErrMsg$
On Error Abort
CheckS "an error in the #INCLUDE file", e$, "Error in lines.inc line 13: inc"

' ---- TRACE -------------------------------------------------------------------------------
Trace On
i% = 1
IncTrace
Trace Off
Print
Trace List 4
Print

' ---- READ, RESTORE and labels across the files -------------------------------------------
Restore MainData
Read i%, j%
Check "READ after RESTORE to a label in the program", i% * 10 + j%, 12
Restore IncData
Read i%, j%
Check "READ after RESTORE to a label in the #INCLUDE file", i% * 10 + j%, 34
Restore
Read i%
Check "READ from the top finds the program's first DATA", i%, 1
GoSub IncLabel
Check "GOSUB a label in the #INCLUDE file", j%, 77
Check "a FUNCTION in the #INCLUDE file", IncTwice(21), 42
CheckS "MM.INFO$(CURRENT) names the program", UCase$(Right$(MM.Info$(CURRENT), 9)), "LINES.BAS"
CheckS "MM.CMDLINE$ is empty", MM.CmdLine$, ""

Print "lines:"; pass%; " of"; tests%; " passed"
End

MainData:
Data 1, 2

#Include "lines.inc"

' walk the image: by each line's skip byte, and by its elements as NextLine() does, which must agree
Sub WalkImage
  Local a%, k%, sk%, f%, l%, last1%, last2%, nxt%, nm$
  If Peek(PROGMEM, 0) <> 1 Or Peek(PROGMEM, 2) <> 5 Or Peek(PROGMEM, 3) <> 5 Or Peek(PROGMEM, 4) <> 5 Or Peek(PROGMEM, 5) <> 39 Then bad% = bad% Or 1
  If Peek(PROGMEM, 6) <> Asc("*") Or Peek(PROGMEM, 7) <> Asc("6") Then bad% = bad% Or 1   ' the format's stamp (IMAGE_FORMAT)
  asc1% = 1 : asc2% = 1 : last1% = 0 : last2% = 0
  Do While Peek(PROGMEM, a%) = 1
    sk% = Peek(PROGMEM, a% + 1)
    f% = Peek(PROGMEM, a% + 2) - 4
    l% = (Peek(PROGMEM, a% + 3) - 5) * 250 + Peek(PROGMEM, a% + 4) - 5
    nlines% = nlines% + 1
    If l% = 0 Then
      nnames% = nnames% + 1
      nm$ = "" : k% = a% + 8
      Do While Peek(PROGMEM, k%) <> 0
        nm$ = nm$ + Chr$(Peek(PROGMEM, k%)) : k% = k% + 1
      Loop
      If f% = 1 Then nm$ = UCase$(Mid$(nm$, Len(nm$) - 8))
      If names$ <> "" Then names$ = names$ + ","
      names$ = names$ + nm$
    ElseIf f% = 1 Then
      If l% <= last1% Then asc1% = 0
      last1% = l%
    Else
      If l% <= last2% Then asc2% = 0
      last2% = l%
    EndIf
    k% = a% + 5
    Do
      Do While Peek(PROGMEM, k%) <> 0
        k% = k% + 1
      Loop
      If Peek(PROGMEM, k% + 1) = 1 Or Peek(PROGMEM, k% + 1) = 0 Then Exit Do
      k% = k% + 1
    Loop
    nxt% = k% + 1
    If sk% = 254 Then
      nlong% = nlong% + 1
    ElseIf sk% > 253 Or a% + sk% <> nxt% Then
      bad% = bad% Or 2
    EndIf
    a% = nxt%
  Loop
  If Peek(PROGMEM, a%) <> 0 Then bad% = bad% Or 4
End Sub

Sub Check(n$, got, want)
  tests% = tests% + 1
  If got = want Then
    pass% = pass% + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub CheckS(n$, got$, want$)
  tests% = tests% + 1
  If got$ = want$ Then
    pass% = pass% + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got "; got$; " want "; want$
  EndIf
End Sub
