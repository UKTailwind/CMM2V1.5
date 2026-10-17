' endhook.bas - END and SUB MM.END (plan step 8, the PicoMite's): END runs MM.END first (an END inside it ends
' there), END NOEND does not; endcmd.bas has END cmd$.
Dim integer n
Print "running"
n = 1
End

Sub MM.END
  Print "MM.END ran, n ="; n
  End
  Print "not here"
End Sub
