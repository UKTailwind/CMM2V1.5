' vartable.bas - P1 step 4: the compact variable table.  A global is a slot pointing at a record
' taken from a chunk of 32 when it is made; ERASE gives the record back.  Strings of LENGTH 16 or
' less, and short CONSTs, are kept inside their record and must never be freed into the heap.
Dim tests%, pass%, i%, j%, k%, x, s$, base%, hpa%, hpb%, ok%
Const short$ = "abc"
base% = MM.Info(VARCNT)

' ---- many globals across several chunks keep their values --------------------------------
For i% = 1 To 150
  Execute "Dim G" + Str$(i%) + "% = " + Str$(i% * 7)
Next
ok% = 1
For i% = 1 To 150
  If Eval("G" + Str$(i%) + "%") <> i% * 7 Then ok% = 0
Next
Check "150 globals across five chunks keep their values", ok%, 1
Check "and are counted", MM.Info(VARCNT) - base%, 150

' ---- ERASE gives the records back and new globals reuse them -------------------------------
For i% = 1 To 150 Step 2
  Execute "Erase G" + Str$(i%) + "%"
Next
Check "75 erased", MM.Info(VARCNT) - base%, 75
ok% = 1
For i% = 2 To 150 Step 2
  If Eval("G" + Str$(i%) + "%") <> i% * 7 Then ok% = 0
Next
Check "the others keep their values", ok%, 1
For i% = 1 To 150 Step 2
  Execute "Dim H" + Str$(i%) + "! = " + Str$(i%)
Next
ok% = 1
For i% = 1 To 150 Step 2
  If Eval("H" + Str$(i%) + "!") <> i% Then ok% = 0
Next
For i% = 2 To 150 Step 2
  If Eval("G" + Str$(i%) + "%") <> i% * 7 Then ok% = 0
Next
Check "new globals in the erased records, the old ones untouched", ok%, 1

' ---- strings kept inside their record ------------------------------------------------------
CheckS "a short CONST", short$, "abc"
For i% = 1 To 40
  Execute "Dim S" + Str$(i%) + "$ Length 10 = " + Chr$(34) + "s" + Str$(i%) + Chr$(34)
Next
ok% = 1
For i% = 1 To 40
  If Eval("S" + Str$(i%) + "$") <> "s" + Str$(i%) Then ok% = 0
Next
Check "40 strings of LENGTH 10 in their records", ok%, 1
For i% = 1 To 40
  Execute "Erase S" + Str$(i%) + "$"
Next
' create and erase them again: after the first round (which may take a new chunk of records, kept
' until the heap is wiped) the heap must come back to the same size each time
For k% = 1 To 5
  If k% = 2 Then hpa% = MM.Info(HEAP)
  For i% = 1 To 40
    Execute "Dim T" + Str$(i%) + "$ Length 12 = " + Chr$(34) + "t" + Str$(i%) + Chr$(34)
    Execute "Dim U" + Str$(i%) + "$ = " + Chr$(34) + "long one" + Chr$(34)
  Next
  For i% = 1 To 40
    Execute "Erase T" + Str$(i%) + "$"
    Execute "Erase U" + Str$(i%) + "$"
  Next
Next
hpb% = MM.Info(HEAP)
Check "erasing in-record and heap strings leaves the heap as it was", hpa% - hpb%, 0
Check "and the count", MM.Info(VARCNT) - base%, 150

' ---- the record is still there to PEEK at ----------------------------------------------------
Dim marker% = 12345
Check "PEEK(VARHEADER) is the record: the name", Peek(BYTE Peek(VARHEADER marker%)), Asc("M")
x = 0
On Error Skip 1
x = Peek(VARTBL, 0)
Check "PEEK(VARTBL) has gone", MM.ErrNo <> 0, 1

Print "vartable:"; pass%; " of"; tests%; " passed"
End

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
