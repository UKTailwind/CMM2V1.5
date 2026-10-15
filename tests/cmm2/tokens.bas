' tokens.bas - every statement whose command token the interpreter reads, steps over or
' compares, with the answer it must give.  Written for the move to two-byte command
' tokens: a token step that is one byte short makes the next byte (0x80, the NOT
' operator) part of the statement, so most mistakes show here as a wrong value, not a
' crash.  Prints "ok name" or "FAIL name got .. want ..", then a total.
Dim tests%, pass%, a%, b%, c%, i%, j%, n%, r%, s$, t$, ticks%, lticks%
Dim d1%, d2%, d3$, cnt%, noargs_ran%, x, y, ez

' ---- single-line IF ---------------------------------------------------------
a% = 1
If a% = 1 Then b% = 10 Else b% = 20
Check "IF THEN ELSE, true", b%, 10
If a% = 2 Then b% = 10 Else b% = 20
Check "IF THEN ELSE, false", b%, 20
b% = 0 : If a% = 1 Then b% = 1 : c% = 2
Check "IF THEN two statements", b% + c%, 3
c% = 0 : If a% = 2 Then c% = 5 : c% = 6
Check "IF THEN false skips the rest", c%, 0
If a% = 1 Then Print "ok  IF THEN PRINT" Else Print "FAIL IF THEN PRINT"
tests% = tests% + 1 : pass% = pass% + 1
r% = 1
If a% = 1 Then GoTo lbl1
r% = 0
lbl1:
Check "IF THEN GOTO", r%, 1
r% = 1
If a% = 1 GoTo lbl2
r% = 0
lbl2:
Check "IF GOTO", r%, 1
r% = 0
If a% = 1 Then If b% = 1 Then r% = 7
Check "IF THEN IF", r%, 7

' ---- multi-line IF, ELSEIF, ELSE, ENDIF and the two-word spellings -------------
For i% = 1 To 4
  If i% = 1 Then
    r% = 100
  ElseIf i% = 2 Then
    r% = 200
  Else If i% = 3 Then
    r% = 300
  Else
    r% = 400
  End If
  Check "multi-line IF arm " + Str$(i%), r%, i% * 100
Next i%
r% = 0
If a% = 1 Then
  If b% = 2 Then
    r% = 1
  Else
    r% = 2
  EndIf
EndIf
Check "nested IF", r%, 2

' ---- SELECT CASE ---------------------------------------------------------------
For i% = 1 To 6
  Select Case i% * 3
    Case 3
      r% = 1
    Case 6, 7
      r% = 2
    Case 8 To 10
      r% = 3
    Case Is >= 15
      r% = 5
    Case Else
      r% = 4
  End Select
  Check "SELECT " + Str$(i%), r%, Pick(i% < 4, i%, Pick(i% = 4, 4, 5))
Next i%
r% = 0
Select Case 2
  Case 1
    r% = 1
  Case 2
    Select Case 3
      Case 3
        r% = 23
    End Select
End Select
Check "nested SELECT", r%, 23

' ---- DO / LOOP -------------------------------------------------------------------
n% = 0
Do While n% < 5
  n% = n% + 1
Loop
Check "DO WHILE", n%, 5
n% = 0
Do
  n% = n% + 1
Loop Until n% >= 7
Check "LOOP UNTIL", n%, 7
n% = 0
Do
  n% = n% + 1
Loop While n% < 3
Check "LOOP WHILE", n%, 3
n% = 0
Do
  n% = n% + 1
  If n% = 4 Then Exit Do
Loop
Check "EXIT DO", n%, 4
n% = 0 : r% = 0
Do While n% < 6
  n% = n% + 1
  If n% Mod 2 Then Continue Do
  r% = r% + n%
Loop
Check "CONTINUE DO", r%, 12
r% = 0
For i% = 1 To 3
  j% = 0
  Do
    j% = j% + 1
    r% = r% + 1
  Loop Until j% = i%
Next i%
Check "DO inside FOR", r%, 6

' ---- FOR / NEXT ------------------------------------------------------------------
r% = 0
For i% = 10 To 1 Step -3
  r% = r% + i%
Next i%
Check "FOR STEP -3", r%, 22
r% = 0
For i% = 1 To 10
  If i% = 5 Then Exit For
  r% = r% + 1
Next
Check "EXIT FOR, NEXT without a name", r%, 4
r% = 0
For i% = 1 To 6
  If i% Mod 3 Then Continue For
  r% = r% + i%
Next i%
Check "CONTINUE FOR", r%, 9
r% = 0
For i% = 1 To 3 : For j% = 1 To 3 : r% = r% + 1 : Next j% : Next i%
Check "nested FOR on one line", r%, 9

' ---- GOSUB, ON GOSUB, GOTO, labels ------------------------------------------------
r% = 0
GoSub addten
GoSub addten
Check "GOSUB", r%, 20
r% = 0
On 2 GoSub add1, add2, add3
Check "ON GOSUB", r%, 2
r% = 0
On 3 GoTo og1, og2, og3
og1:
og2:
r% = -1
og3:
Check "ON GOTO", r%, 0

' ---- SUB, FUNCTION, LOCAL, STATIC, EXIT SUB/FUNCTION, recursion -------------------
a% = 5
Shadow
Check "LOCAL hides the global", a%, 5
Counter : Counter : Counter
Check "STATIC", Counter2%(), 4
Check "recursive FUNCTION", Fact%(10), 3628800
Check "FUNCTION in an expression", Sq%(3) + Sq%(4), 25
CheckS "string FUNCTION", Twice$("ab"), "abab"
r% = 0 : EarlyOut r%
Check "EXIT SUB", r%, 1
Check "EXIT FUNCTION", Early%(), 2
Check "FUNCTION with empty brackets", HowMany%(), 0
Noargs
Check "SUB with no arguments", Noargs2%(), 1

' ---- DATA, READ, RESTORE ---------------------------------------------------------
Restore data2
Read d1%, d2%, d3$
Check "READ after RESTORE label", d1% + d2%, 42
CheckS "READ string", d3$, "token"
Restore
Read d1%
Check "RESTORE to the first DATA", d1%, 7

' ---- the aliases the tokeniser rewrites ---------------------------------------------
? "ok  ? for PRINT"
tests% = tests% + 1 : pass% = pass% + 1
s$ = "to"
Cat s$, "ken"
CheckS "CAT", s$, "token"
n% = 3 : Inc n% : Inc n%, 5
Check "INC", n%, 9
Colour RGB(white), RGB(black)
Color RGB(white), RGB(black)
Check "COLOUR and COLOR", 1, 1
ez = 1
Erase ez
Dim ez
ez = ez + 2
Check "ERASE (CLEAR VARS)", ez, 2

' ---- EVAL, implied LET, functions that ask which command is running ----------------
Check "EVAL", Eval("2*3+1"), 7
x = 1.5 : y = x * 2
Check "implied LET", y, 3
On Error Skip 1
t$ = Str$(@(1, 1))
Check "@( outside PRINT is an error", MM.ErrNo <> 0, 1

' ---- a DefineFont block in the middle of a program is skipped -----------------------
r% = 1
DefineFont #9
  01410404 F99F0000
End DefineFont
r% = r% + 1
Check "DefineFont block skipped", r%, 2

' ---- interrupts: a SUB and a label with IRETURN, during PAUSE --------------------------
ticks% = 0 : lticks% = 0
SetTick 5, TickSub, 1
SetTick 7, TickLabel, 2
Pause 200
SetTick 0, TickSub, 1
SetTick 0, TickLabel, 2
Check "SETTICK SUB ran", ticks% >= 10, 1
Check "SETTICK label with IRETURN ran", lticks% >= 10, 1
n% = ticks% : Pause 30
Check "SETTICK stopped", ticks%, n%

' ---- last, because before the fix it stops the program: the ELSE scan stopped at the
' ERROR command, whose one-byte token has the same value as the ELSE token
r% = 0
On Error Skip 1
If a% = 2 Then Error "boom" Else r% = 5
Check "IF THEN ERROR ELSE, false", r%, 5

Print "tokens:"; pass%; " of"; tests%; " passed"
End

' ---- the helpers and the code the tests call ------------------------------------------
addten:
  r% = r% + 10
Return
add1: r% = 1 : Return
add2: r% = 2 : Return
add3: r% = 3 : Return

TickLabel:
  lticks% = lticks% + 1
IReturn

Sub TickSub
  ticks% = ticks% + 1
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

Function Pick(c, x, y)
  If c Then Pick = x Else Pick = y
End Function

Sub Shadow
  Local a%
  a% = 99
End Sub

Sub Counter
  Static k%
  k% = k% + 1
  cnt% = k%
End Sub

Function Counter2%()
  Counter
  Counter2% = cnt%
End Function

Function Fact%(n%)
  If n% <= 1 Then Fact% = 1 Else Fact% = n% * Fact%(n% - 1)
End Function

Function Sq%(v%)
  Sq% = v% * v%
End Function

Function Twice$(x$)
  Twice$ = x$ + x$
End Function

Sub EarlyOut(v%)
  v% = 1
  Exit Sub
  v% = 2
End Sub

Function Early%()
  Early% = 2
  Exit Function
  Early% = 3
End Function

Function HowMany%()
  HowMany% = 0
End Function

Sub Noargs
  noargs_ran% = 1
End Sub

Function Noargs2%()
  Noargs2% = noargs_ran%
End Function

Data 7, 8
data2:
Data 40, 2, "token"
