' bugs.bas - the interpreter bugs fixed in P1 step 2 (the PicoMite's September bug report and
' what the CMM2's own tests found), each with the answer it must give.  Before the fixes most of
' these print FAIL; some stopped the program, which the ON ERROR SKIPs below now turn into a FAIL.
Dim tests%, pass%, x, y, z, i%, j%, r%, s$, flag%, q_ran%, after%
Dim a(3)

' ---- MAX and MIN started from FLT_MAX (item 22) ----------------------------------------
Check "MAX of two below -3.4e38", Max(-1e300, -2e300), -1e300
Check "MIN of two above 3.4e38", Min(1e300, 2e300), 1e300
Check "MAX of one value", Max(7), 7

' ---- an integer power ran one multiplication per unit of the exponent (item 27) ---------
Timer = 0
i% = 1 ^ 4000000000
Check "1^4000000000 is quick", Timer < 1000, 1
Check "1^4000000000", i%, 1
CheckS "3^40 wraps as before", Str$(3 ^ 40), "-6289078614652622815"
CheckS "(-2)^63", Str$((-2) ^ 63), "-9223372036854775808"
Check "0^0", 0 ^ 0, 1
Check "2^10", 2 ^ 10, 1024

' ---- ABS asked for an integer, so a float of 2^63 or more was "Number too large" (item 23) --
r% = 0 : On Error Skip 1
x = Abs(-1e19)
Check "ABS(-1e19)", x, 1e19
Check "ABS(-5)", Abs(-5), 5

' ---- an integer subscript beyond 32 bits wrapped to a small index (item 24) ---------------
a(1) = 11
On Error Skip 1
x = a(4294967297)
Check "a(4294967297) is out of bounds, not a(1)", MM.ErrNo <> 0 And x <> 11, 1

' ---- INF, -INF and NaN (items 25 and 26) ---------------------------------------------------
x = 1e308 : y = x * 10 : z = y - y
CheckS "STR$ of +infinity", Str$(y), "INF"
CheckS "STR$ of -infinity", Str$(-y), "-INF"
CheckS "STR$ of NaN", Str$(z), "NAN"
On Error Skip 1
i% = Int(z)
Check "INT(NaN) is an error", MM.ErrNo <> 0, 1
On Error Skip 1
i% = Int(1e30)
Check "INT(1e30) is an error", MM.ErrNo <> 0, 1
On Error Skip 1
i% = Fix(-1e30)
Check "FIX(-1e30) is an error", MM.ErrNo <> 0, 1
On Error Skip 1
i% = 9223372036854775808.0
Check "2^63 into an integer is an error", MM.ErrNo <> 0, 1
Check "INT(-7.5)", Int(-7.5), -8
Check "FIX(-7.5)", Fix(-7.5), -7

' ---- &h in a DATA item kept its case and was "Type prefix"; a-f were 42 and up -------------
Restore hexdata
Read i%, j%
Check "DATA &h10", i%, 16
Check "DATA &hff", j%, 255

' ---- FindSubFun ignored the kind asked for (item 9) -------------------------------------------
flag% = 0
On Error Skip 1
KindF
Check "a FUNCTION called as a statement does not run", flag%, 0
Check "and is an error", MM.ErrNo <> 0, 1
flag% = 0
On Error Skip 1
x = KindS()
Check "a SUB in an expression does not run", flag%, 0
Check "CALL() runs a FUNCTION", Call("Sq", 4), 16

' ---- CALL appended to a 256-byte temporary unchecked (item 10) -------------------------------
On Error Skip 1
Call "Q" + Space$(250), 1, 2, 3
Check "CALL with a 251-character name is refused", MM.ErrNo <> 0, 1

' ---- a one-letter SUB called alone on a line (item 29) ----------------------------------------
q_ran% = 0 : after% = 0
Q
after% = 1
Check "one-letter SUB alone on a line ran", q_ran%, 1
Check "and the next line ran", after%, 1

' ---- STATIC named its variable after the last SUB entered, not its own (item 5) -------------
s$ = "" : Stat5A : Stat5A : Stat5A
CheckS "STATIC after a call to another SUB", s$, "1,2,3,"
s$ = "" : Stat5C : Stat5C
CheckS "STATIC after a FUNCTION call", s$, "10,20,"
s$ = "" : flag% = 0
SetTick 5, Stat5T, 1
Timer = 0
Do : Loop Until flag% Or Timer > 1000
CheckS "STATIC in a SUB that an interrupt SUB calls", s$, "100"

' ---- a label that shares a SUB's name (item 8: the CMM2's findlabel already skips SUBs) -----
s$ = ""
GoSub Same8
Same8
Restore Same8
Read j%
CheckS "GOSUB a label that shares a SUB's name, then the SUB", s$, "LS"
Check "RESTORE to a label that shares a SUB's name", j%, 88
On Error Skip 1
GoTo Only8
Check "GOTO a name that is only a SUB is an error", MM.ErrNo <> 0, 1
On Error Skip 1
GoSub ABCDEFGHIJKLMNOPQRSTUVWXYZ012345
Check "GOSUB a 32-character label", MM.ErrNo, 0

' ---- MM.INFO(VARCNT) stayed high after ERASE and after a SUB returned (item 14) -------------
Dim era14(10)
i% = MM.Info(VARCNT)
Erase era14
j% = MM.Info(VARCNT)
Var14
x = MM.Info(VARCNT)
Check "VARCNT after ERASE of one array", j% - i%, -1
Check "VARCNT in a SUB with two LOCALs", r% - j%, 2
Check "VARCNT after the SUB returned", x - j%, 0

' ---- an error in a call's arguments under ON ERROR leaked each string copy (item 16) --------
z = 0
y = Leak16(1, 2) : y = Leak16S("x", 1)
i% = MM.Info(HEAP)
On Error Ignore
For j% = 1 To 50
  y = Leak16(1 / z, 1)
  y = Leak16S("abc", 1 / z)
Next
On Error Abort
r% = MM.Info(HEAP)
Check "50 skipped errors in arguments leave the heap as it was", i% - r%, 0
Check "and the FUNCTIONs still work", Leak16(1, 2) + Leak16S("abc", 1), 7

Print "bugs:"; pass%; " of"; tests%; " passed"
End

hexdata:
Data &h10, &hff


Same8:
Data 88
s$ = s$ + "L"
Return

ABCDEFGHIJKLMNOPQRSTUVWXYZ012345:
Return

Sub Same8
  s$ = s$ + "S"
End Sub

Sub Only8
End Sub

Sub Stat5A
  Stat5B
  Static n
  n = n + 1
  s$ = s$ + Str$(n) + ","
End Sub

Sub Stat5B
  Static n
  n = n + 100
End Sub

Function Stat5F()
  Static n
  n = n + 1
  Stat5F = n
End Function

Sub Stat5C
  x = Stat5F()
  Static n
  n = n + 10
  s$ = s$ + Str$(n) + ","
End Sub

Sub Stat5T
  SetTick 0, Stat5T, 1
  Static n
  n = n + 1
  Stat5U
  flag% = 1
End Sub

Sub Stat5U
  Static n
  n = n + 100
  s$ = Str$(n)
End Sub

Sub Var14
  Local u, w
  r% = MM.Info(VARCNT)
End Sub

Function Leak16(a, b)
  Leak16 = a + b
End Function

Function Leak16S(a$, b)
  Leak16S = Len(a$) + b
End Function

Sub Q
  q_ran% = q_ran% + 1
End Sub

Function KindF()
  flag% = 1
  KindF = 1
End Function

Sub KindS
  flag% = 1
End Sub

Function Sq(v)
  Sq = v * v
End Function

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
