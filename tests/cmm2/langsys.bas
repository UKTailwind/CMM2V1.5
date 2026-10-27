' langsys.bas - the language and system items of 26 Oct 2026 (docs/PicoMite_Parity_ToDo.md) that a program can check:
' MM.DISPLAY, MM.USB, MM.INFO$(PLATFORM), CPU SLEEP, MM.INFO(BOOT), OPTION SCREEN KEYBOARD, OPTION AUTORUN n [, NORESET]
' (left OFF), OPTION LOGGING (A:/log.txt, deleted after), LIST VARIABLES, COLLISIONS and PINS, PEEK(BP/SP/WP var), and
' MM.INFO(PIN n) of a counting input. OPTION F1 and F5 to F9 act at the prompt, so they are checked there.
Option Explicit
Option Default Integer
Dim tests, pass, t, a, n
Dim float f(3)
Dim s$
Dim integer ls(200), w(1), tiny(1)

Check "MM.DISPLAY is 1", MM.Display, 1
Check "MM.USB is a key's code or -1", MM.USB >= -1 And MM.USB < 256, 1
CheckS "MM.INFO$(PLATFORM)", MM.Info$(Platform), "CMM2"

t = Timer
CPU Sleep 0.2
t = Timer - t
Check "CPU SLEEP 0.2: a pause of 200 ms (+-10)", Abs(t - 200) <= 10, 1
On Error Skip 1 : CPU Sleep 0
CheckS "  of 0: an error", MM.ErrMsg$, "Invalid period"

s$ = MM.Info(Boot)
Check "MM.INFO(BOOT) is one of the CMM2's words, or empty", Instr("|Switch|Power-On|Software|Brown-Out||", "|" + s$ + "|") > 0, 1
CheckS "  and empty once read, as MM.INFO(RESET) is", MM.Info(Reset), ""

On Error Skip 1 : Option Screen Keyboard 10, M
CheckS "OPTION SCREEN KEYBOARD 10, M accepted", MM.ErrMsg$, ""
On Error Skip 1 : Option Screen Keyboard 60
CheckS "  60 rows: an error", MM.ErrMsg$, "60 is invalid (valid is 0 to 50)"

Option Autorun 2
CheckS "OPTION AUTORUN 2: MM.INFO(OPTION AUTORUN)", MM.Info(Option Autorun), "2"
Option Autorun 3, NoReset
CheckS "  3, NORESET", MM.Info(Option Autorun), "3"
On Error Skip 1 : Option Autorun 4
CheckS "  4: an error", MM.ErrMsg$, "4 is invalid (valid is 0 to 3)"
On Error Skip 1 : Option Autorun 1, Later
CheckS "  1, LATER: an error", MM.ErrMsg$, "Syntax"
Option Autorun Off
CheckS "  OFF", MM.Info(Option Autorun), "Off"

On Error Skip 1 : Kill "A:/log.txt"
Option Logging On
On Error Skip 1 : Error "the first, logged"
On Error Skip 1 : a = 1 \ 0
Option Logging Off
On Error Skip 1 : Error "not logged"
Open "A:/log.txt" For Input As #1
Line Input #1, s$ : CheckS "OPTION LOGGING ON: the first error in A:/log.txt", s$, "the first, logged"
Line Input #1, s$ : CheckS "  the second", s$, "Divide by zero"
Check "  none after OPTION LOGGING OFF", Eof(#1) <> 0, 1
Close #1
Kill "A:/log.txt"

List Variables ls()
Check "LIST VARIABLES ls(): DIM INTEGER TESTS", LInstr(ls(), "DIM INTEGER TESTS" + Chr$(13)) > 0, 1
Check "  DIM FLOAT F(3)", LInstr(ls(), "DIM FLOAT F(3)" + Chr$(13)) > 0, 1
Check "  DIM STRING S", LInstr(ls(), "DIM STRING S" + Chr$(13)) > 0, 1
Check "  sorted: DIM FLOAT before DIM INTEGER", LInstr(ls(), "DIM FLOAT F(3)") < LInstr(ls(), "DIM INTEGER A" + Chr$(13)), 1
Locals
On Error Skip 1 : List Variables tiny()
CheckS "  an array too small for them", MM.ErrMsg$, "Destination array too small"
Print "-- LIST VARIABLES"
List Variables
Print "-- LIST COLLISIONS"
List Collisions

SetPin 8, DOut
SetPin 15, ARaw
SetPin 7, CIn
CheckS "MM.INFO(PIN 7) of a counting input: In Use (it said Reserved)", MM.Info(Pin 7), "In Use"
Print "-- LIST PINS"
List Pins
SetPin 7, Off : SetPin 15, Off : SetPin 8, Off

w(0) = &H0807060504030201
a = Peek(VarAddr w())
Check "PEEK(BP a): the byte at a", Peek(BP a), 1
Check "  and a steps on by 1", a - Peek(VarAddr w()), 1
On Error Skip 1 : n = Peek(SP a)
CheckS "PEEK(SP a) at an odd address: an error", MM.ErrMsg$, "Not on short boundary"
a = a + 1
Check "PEEK(SP a): the short", Peek(SP a), &H0403
Check "PEEK(WP a): the word after it", Peek(WP a), &H08070605
Check "  a stepped on by 2 then 4", a - Peek(VarAddr w()), 8
On Error Skip 1 : n = Peek(BP f(0))
CheckS "  a float variable: an error", MM.ErrMsg$, "Not integer variable"

Print "langsys:"; pass; " of"; tests; " passed"
End

' a SUB's LOCAL in LIST VARIABLES
Sub Locals
  Local float q
  List Variables ls()
  Check "  a SUB's LOCAL FLOAT Q", LInstr(ls(), "LOCAL FLOAT Q" + Chr$(13)) > 0, 1
End Sub

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub CheckS(n$, got$, want$)
  tests = tests + 1
  If got$ = want$ Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got "; got$; " want "; want$
  EndIf
End Sub
