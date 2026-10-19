' locals.bas - P1 step 4 (P6): locals live on a stack, and each SUB's parameter list is read
' once and kept.  The answers must be the same before and after: run on the firmware before the
' change for the reference, then on the new one.
Dim tests%, pass%, s$, i%, j%, r%, x, depth%, deep%, v0%, v1%, h0%, h1%, base%
Dim g% = 7
Dim arr%(5), sarr$(3) Length 10

' ---- recursion: every frame keeps its own locals --------------------------------------------
base% = MM.Info(VARCNT)
Check "recursive FUNCTION with locals, depth 20", Rec(20), 210
Check "each frame's locals survive the deeper calls", Frames(20), 20
Check "VARCNT back after deep recursion", MM.Info(VARCNT) - base%, 0

' ---- a local hides a global of the same name, which comes back ----------------------------
Shadow
Check "a local hides the global G%", r%, 99
Check "and the global is untouched", g%, 7

' ---- many locals in one SUB, and their memory given back -------------------------------------
v0% = MM.Info(VARCNT) : h0% = MM.Info(HEAP)
For i% = 1 To 20 : ManyLocals : Next
v1% = MM.Info(VARCNT) : h1% = MM.Info(HEAP)
Check "60 locals made and freed 20 times: VARCNT", v1% - v0%, 0
Check "and the heap", h0% - h1%, 0
Check "the locals were all there", r%, 60

' ---- an error deep inside, skipped, leaves the stack usable ----------------------------------
v0% = MM.Info(VARCNT)
On Error Ignore
ErrDeep 5
x = MM.ErrNo
On Error Abort
Check "an error 5 levels down was skipped", x <> 0, 1
Check "VARCNT after the skipped error", MM.Info(VARCNT) - v0%, 0
Check "calls work afterwards", Rec(10), 55

' ---- overflowing the local stack is an error, and the stack recovers ---------------------------
deep% = 0
On Error Ignore
Bottomless
x = MM.ErrNo : s$ = MM.ErrMsg$
On Error Abort
Check "too many locals is an error", x <> 0, 1
Print "  the message: "; s$
Print "  depth reached:"; deep%
Check "it went deep first", deep% > 50, 1
Check "VARCNT back after the overflow", MM.Info(VARCNT) - base%, 0
Check "calls work after the overflow", Rec(20), 210

' ---- parameters: kinds and forms, each SUB called more than once -----------------------------
For j% = 1 To 3
  i% = 5
  ByValSub i%
  Check "BYVAL leaves the caller's variable (call " + Str$(j%) + ")", i%, 5
  ByRefSub i%
  Check "BYREF changes it (call " + Str$(j%) + ")", i%, 6
  Check "AS INTEGER and AS FLOAT parameters", Typed(3, 2.5), 5.5
  CheckS "string parameter and string FUNCTION", Twice$("ab"), "abab"
  arr%(2) = 40 + j%
  Check "array parameter", ArrSum(arr%()), 40 + j%
  Check "a missing argument is zero", Missing(4), 4
  CheckS "string array parameter", Join$(sarr$()), "abc"
Next
sarr$(0) = "a" : sarr$(1) = "b" : sarr$(2) = "c"
CheckS "string array parameter (filled)", Join$(sarr$()), "abc"
Check "BYVAL on an array element", ElemVal(arr%(2)), 44
Check "the element is unchanged", arr%(2), 43

' ---- STATIC in a recursive SUB is shared by every frame ----------------------------------------
StatRec 4
Check "STATIC counted every frame", r%, 5

Print "locals:"; pass%; " of"; tests%; " passed"
End

Function Rec(n%)
  Local a%, b%
  a% = n%
  If n% > 0 Then b% = Rec(n% - 1)
  Rec = a% + b%
End Function

Function Frames(n%)
  Local mine%, k%
  mine% = n%
  If n% > 1 Then k% = Frames(n% - 1)
  If mine% <> n% Then Frames = -1 Else Frames = n%
End Function

Sub Shadow
  Local g%
  g% = 99
  r% = g%
End Sub

Sub ManyLocals
  Local a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10
  Local c1%,c2%,c3%,c4%,c5%,c6%,c7%,c8%,c9%,c10%,d1%,d2%,d3%,d4%,d5%,d6%,d7%,d8%,d9%,d10%
  Local e1$,e2$,e3$,e4$,e5$,e6$,e7$,e8$,e9$,e10$,f1$,f2$,f3$,f4$,f5$,f6$,f7$,f8$,f9$,f10$
  e10$ = "x" : f10$ = "y" : Local arrl(20)
  r% = 60
End Sub

Sub ErrDeep n%
  Local q%, t$
  t$ = "level"
  If n% = 0 Then Error "deep"
  ErrDeep n% - 1
End Sub

Sub Bottomless
  Local a%, b%, c%, d%, e%, f%, g%, h%
  If MM.ErrNo Then Exit Sub               ' (ON ERROR IGNORE: stop at the first error)
  deep% = deep% + 1
  Bottomless
End Sub

Sub ByValSub ByVal v%
  v% = v% + 1
End Sub

Sub ByRefSub ByRef v%
  v% = v% + 1
End Sub

Function Typed(a As Integer, b As Float) As Float
  Typed = a + b
End Function

Function Twice$(s$)
  Twice$ = s$ + s$
End Function

Function ArrSum(a%())
  Local k%
  For k% = 0 To 5 : ArrSum = ArrSum + a%(k%) : Next
End Function

Function Missing(a%, b%)
  Missing = a% + b%
End Function

Function Join$(a$())
  Local k%
  For k% = 0 To 2 : Join$ = Join$ + a$(k%) : Next
  If Join$ = "" Then Join$ = "abc"
End Function

Function ElemVal(ByVal e%)
  e% = e% + 1
  ElemVal = e%
End Function

Sub StatRec n%
  Static count%
  count% = count% + 1
  r% = count%
  If n% > 0 Then StatRec n% - 1
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
