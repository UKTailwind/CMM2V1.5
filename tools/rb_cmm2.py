"""rb_cmm2.py: the CMM2's own Route B checks, read by rbtest.py beside the PicoMite's rb*.py (20 Oct 2026).

Each program must print the same with OPTION COMPILE OFF, ON and SHADOW: the places where the review of the port found
the compiled code doing other than the CMM2's text handlers (docs/RouteB_CMM2_Port_Plan.md), and the CMM2's own forms.
An error a program raises must be the same error on the same line every way."""

PROGS = [
    ("int compare far apart", """a% = &H7FFFFFFFFFFFFFFF : b% = -1 : c% = -a% - 1
Print a% > b%; a% < b%; a% >= b%; b% <= a%; c% < 1; 1 > c%; c% < a%
If a% > b% Then Print "gt" Else Print "le"
Do While a% > b%
  Print "do"
  Exit Do
Loop
Select Case a%
  Case Is > b%
    Print "case gt"
  Case Else
    Print "case else"
End Select
For i% = 1 To 3
  If c% < i% Then n% = n% + 1
Next
Print n%
""", True),
    ("LOCAL LENGTH ok", """Sub U
  Local t$ Length 5 = "hello", n% = 3
  Print t$; n%
End Sub
U : U
""", True),
    ("LOCAL LENGTH too long", """Sub T
  Local s$ Length 3 = "hello"
  Print "not here"
End Sub
Print "before"
T
""", None),
    ("STATIC", """Sub UpdateFormation
  Static offs(10, 2), calls
  calls = calls + 1
  offs(1, 1) = calls * 10
  Print calls; offs(1, 1)
End Sub
For i = 1 To 3
  UpdateFormation
Next
""", True),
    ("STATIC name too long", """Sub UpdateAllEnemyFormationStatesX
  Static formationOffsetTable(10, 2)
End Sub
UpdateAllEnemyFormationStatesX
""", None),
    ("LET = in the index", """Dim a(2)
i = 1
a(i) = 4
Print a(1)
a(i = 1) = 5
Print a(1)
""", None),
    ("FOR = in the start", """For i = 1 To 2 : Print i; : Next : Print
For i = 2 = 2 To 3
  Print i
Next
""", None),
    ("LOCAL value of a name it makes", """count = 5
Sub S
  Local Count = count + 1
  Print Count
End Sub
S : S
""", None),
    ("CHOICE of strings", """a$ = "x"
b$ = Choice(1, a$, "y") : Print b$
c$ = Choice(0, a$, "y") : Print c$
d = Choice(0.5, 1, 2) : Print d
""", None),
    ("LOCAL declared twice", """Sub R
  Local q
  Local q
End Sub
R
""", None),
    ("STATIC over a LOCAL", """Sub R2
  Local z
  Static z
End Sub
R2
""", None),
    ("error in a body on the SUB line", """Sub E : Print 1 / 0 : End Sub
Print "before"
E
""", None),
    ("INC", """Dim integer i = 5
Dim float f = 1.5
Dim s$ = "ab"
For k = 1 To 3
  Inc i
  Inc i, 2
  Inc f, 0.25
  Inc s$, "c"
Next
Print i; f; " "; s$
""", True),
    ("TRACE and line numbers", """For i = 1 To 2
  x = x + i
Next
Trace On
y = x * 2
Trace Off
Print y
""", None),
    ("ON ERROR SKIP", """Dim a(3)
For i = 1 To 5
  On Error Skip 1
  a(i) = i
  If MM.ErrNo Then Print "e"; i;
Next
Print
Print a(1); a(3)
""", None),
    ("interrupt in a loop", """Dim integer n
SetTick 5, T, 1
t0 = Timer
Do While Timer - t0 < 200
  k = k + 1
Loop
SetTick 0, 0, 1
Print n > 20
Sub T
  n = n + 1
End Sub
""", None),
    ("RUN in a slot's program", """Print MM.Info(FLASH)
""", None),
]
