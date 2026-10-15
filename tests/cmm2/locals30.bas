' locals30.bas - item 30: markers left by freed locals filled the local half of vartbl.
' 49 levels of a SUB with 10 locals hold 490 of the 511 locals there can be.  Unwinding left the
' freed slots as markers, so the second pass stopped with "Too many local variables" although
' every local of the first had gone.  Four passes must run.
Dim pass%, bad%
On Error Ignore
For pass% = 1 To 4
  R30 48
  If MM.ErrNo Then Exit For
Next
On Error Abort
If MM.ErrNo = 0 And bad% = 0 Then
  Print "ok  locals30: 4 passes of 490 live locals"
Else
  Print "FAIL locals30 got "; MM.ErrMsg$; " at pass"; pass%; ", bad"; bad%; " want no error"
EndIf
End

Sub R30(d)
  Local a, b, c, e, f, g, h, i, j
  a = d : j = d * 2
  If d > 0 Then R30 d - 1
  If a <> d Or j <> d * 2 Then bad% = bad% + 1
End Sub
