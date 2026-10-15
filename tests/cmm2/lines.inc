' lines.inc - the #INCLUDE file for lines.bas: its lines carry file 2 in their headers

IncFirst:
Data 9

IncData:
Data 3, 4

' line 13 below raises the error lines.bas expects

Sub IncError
  Local u%
  Error "inc"
End Sub

Sub IncTrace
  Local v%
  v% = 2
End Sub

IncLabel:
j% = 77
Return

Function IncTwice(n)
  IncTwice = n * 2
End Function
