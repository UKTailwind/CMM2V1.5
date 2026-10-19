' onerror.bas - ON ERROR (plan step 8): MM.ERRLINE; ON ERROR SKIP not used up by the statements of a FUNCTION the
' protected statement calls (PicoMite issue 34), while a SUB's count as on the PicoMite (the corpus's onerror.bas
' stops in one); SKIP n over n statements; ON ERROR IGNORE and the error state kept across an interrupt. Only errors that are skipped are tested: one that is not stops
' the program; that an interrupt between ON ERROR SKIP and its statement gives no skip more was checked by hand.
Dim integer z, n, ln
Dim float r

Print "-- MM.ERRLINE"
On Error Skip
r = 1 / z
Print MM.Errno; "  line"; MM.ErrLine; "  "; em$()
On Error Clear
Print MM.Errno; "  line"; MM.ErrLine; "  ["; em$(); "]"

Print "-- a FUNCTION called by the protected statement does not use the skip up"
' (the error is in the ELSEIF's condition, so the program goes on at the IF's next statement, the first arm's body)
On Error Skip
If 0 Then
  Print "no"
ElseIf 1 / f() Then
  Print "no"
EndIf
Print "skipped: "; em$(); "  line"; MM.ErrLine

Print "-- a SUB's statements count, as on the PicoMite: the call, then each of its body"
On Error Skip 3
s1
Print "after s1: n ="; n; "  "; em$(); "  line"; MM.ErrLine

Print "-- a FUNCTION with an error in its body, called by the protected statement"
n = 0
On Error Skip
r = g() + 1
Print "r ="; r; "  n ="; n; "  "; em$()

Print "-- ON ERROR SKIP 3: three statements"
On Error Skip 3
r = 1 / z
r = Sqr(-1)
r = Log(0)
Print "all three skipped: "; em$()

Print "-- statements on one line count one each"
On Error Skip 2
r = 1 / z : r = Sqr(-1)
Print "both skipped: "; em$()

Print "-- the count goes on in the caller when the SUB that set it returns"
s2
r = 1 / z
Print "skipped in the caller: "; em$()

Print "-- ON ERROR IGNORE and the error state across an interrupt"
On Error Skip
r = 1 / z
ln = MM.ErrLine
n = -1
SetTick 5, tk, 1
Pause 50
SetTick 0, tk, 1
Print "after the ticks: line"; MM.ErrLine = ln; "  "; em$(); "  in the tick:"; n
On Error Ignore
SetTick 5, tk, 1
Pause 50
SetTick 0, tk, 1
r = 1 / z
r = Sqr(-1)
Print "still ignored: "; em$()
On Error Abort
Print "done"
End

Function f()
  f = z
End Function

Function g()
  n = n + 1
  g = 1 / z
  n = n + 1
  g = 5
End Function

Sub s1
  n = 1
  r = 1 / z                                      ' the third: skipped
  n = 2
End Sub

Sub s2
  On Error Skip 3                                ' n = 0 and END SUB use two of the three
  n = 0
End Sub

Sub tk
  n = MM.ErrLine + MM.Errno
End Sub

' the error message without its "Error in line n: " (the line is MM.ERRLINE)
Function em$()
  em$ = MM.ErrMsg$
End Function
