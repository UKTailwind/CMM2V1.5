Sub Show(n)
Print n
End Sub
For i = 1 To 3
If i = 2 Then
Print "two"
Else
Print i
End If
Next i
Do
x = x + 1
Loop Until x > 3
Select Case x
Case 4
Print "four"
Case Else
Print "other"
End Select
