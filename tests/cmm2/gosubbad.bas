' GOSUB to a label that is not there (TBS TID=15486): the error must leave the GOSUB stack as it was
Dim integer n
On Error Skip
GoSub nowhere
Print "1: "; MM.ErrMsg$
On Error Skip
GoSub 9999
Print "2: "; MM.ErrMsg$
GoSub here
Print "back"; n
UsesLocal
On Error Skip
Return
Print "3: "; MM.ErrMsg$
End
here:
n = n + 1
Return
Sub UsesLocal
  Local x = 5
  Print "local"; x
End Sub
