' localfast.bas - the locals findvar_local makes without findvar's search (the PicoMite's a644dd8), a symbol's name
' checked against the SUBs once a run (SYMC_NOSUB), and every case they leave to findvar: each must behave as before.
' The expected output is the firmware's before the change.
Dim integer r
Print "-- parameters and LOCALs"
Print Add3%(1, 2, 3)
Print Mixed!(1.5, 2)
TwoLocals
TwoLocals
Print "-- recursion"
Print Fact%(10)
Print "-- a LOCAL twice"
On Error Ignore
Twice
Print em$()
On Error Abort
Print "-- a LOCAL named as a SUB, twice"
On Error Ignore
NamedAsSub
Print em$()
On Error Abort
On Error Ignore
NamedAsSub
Print em$()
On Error Abort
Print "-- a suffix that clashes"
On Error Ignore
Clash
Print em$()
On Error Abort
Print "-- an error in a SUB, then the SUB again"
On Error Ignore
Fails 1
Print "errno"; MM.Errno <> 0
On Error Abort
Fails 0
Fails 0
Print "-- a FUNCTION's result, a string, an array"
Print Res%(4)
Print StrLocal$("ab")
ArrLocal
ArrLocal
Print "-- the same name as a global"
r = 7
Shadow
Print "global r"; r
Print "-- a parameter's type from its declaration"
Typed 3, 2.5
Print "done"
End

Function Add3%(a%, b%, c%)
  Local integer t
  t = a% + b% + c%
  Add3% = t
End Function

Function Mixed!(x!, n%)
  Local float y
  y = x! * n%
  Mixed! = y
End Function

Sub TwoLocals
  Local integer i, j
  Local float f
  i = 3 : j = 4 : f = 0.5
  Print i * j + f
End Sub

Function Fact%(n%)
  Local integer m
  m = n%
  If m <= 1 Then
    Fact% = 1
  Else
    Fact% = m * Fact%(m - 1)
  EndIf
End Function

Sub Twice
  Local integer q
  Local integer q
End Sub

Sub NamedAsSub
  Local integer TwoLocals
End Sub

Sub Clash
  Local integer z!
End Sub

Sub Fails(e%)
  Local integer w
  w = 1
  If e% Then Error "deliberate"
  Print "fails 0: w ="; w
End Sub

Function Res%(n%)
  Res% = n% * n%
End Function

Function StrLocal$(s$)
  Local string t$
  t$ = s$ + s$
  StrLocal$ = t$
End Function

Sub ArrLocal
  Local integer a(3)
  a(2) = 5
  Print "array"; a(2)
End Sub

Sub Shadow
  Local integer r
  r = 99
  Print "local r"; r
End Sub

Sub Typed(a As integer, b As float)
  Local c As float
  c = a * b
  Print "typed"; c
End Sub

Function em$()
  em$ = MM.ErrMsg$
End Function
