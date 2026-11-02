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
    # The value splice (RB_SPLICE 1, 1 Nov 2026): the graphics commands and functions handed values the compiled code
    # computed, each drawing counted by PIXEL(), the CMM2's own forms beside them (which stay text)
    ("splice TEXT justification", """CLS
j$ = "CM" : LT = 5
Text 50, 20, "Ab", LT
Text 150, 20, "Ab", CM, 1
Text 250, 20, "Ab", "RB", 1, 2
Text 350, 20, "Ab", j$, #7
Text 450, 20, "Ab", , 1, 1, RGB(red), RGB(blue)
Text 50, 60, "Ab", LMV
Text 150 + LT, 60, Str$(LT * 2), CT, 7
c = 0 : For y = 0 To 95 : For x = 0 To 520 Step 2 : If Pixel(x, y) Then c = c + 1
Next : Next
Print c
""", None),
    ("splice graphics", """CLS
For i = 1 To 5
  Line i * 10, 5, i * 10 + 30, 60, 1, RGB(green)
  Box 100 + i * 12, 5, 10, 20, 1, RGB(red), RGB(blue)
  Circle 200 + i * 15, 30, i + 3, 1, 1.5, RGB(yellow)
  Arc 300, 40, 10 + i, , i * 20, i * 20 + 90, RGB(cyan)
  RBox 350 + i * 14, 5, 12, 30, 3, RGB(white), RGB(magenta)
  Triangle 450, 5, 450 + i * 5, 60, 440, 60, RGB(green), RGB(red)
  Pixel 10 + i, 80, RGB(white)
Next
Colour RGB(green), RGB(black)
Pixel Fill 300, 40, RGB(blue)
Line Aa 5, 85, 95, 90, 1, RGB(white)
c = 0 : For y = 0 To 95 : For x = 0 To 520 Step 2 : If Pixel(x, y) Then c = c + 1
Next : Next
Print c; Pixel(305, 40)
Colour RGB(white), RGB(black)
""", None),
    ("splice functions", """a$ = "Hello World"
Print Left$(a$, 3); Right$(a$, 2); UCase$(a$); LCase$(a$); Mid$(a$, 2, 3); Len(a$); Asc(a$); Chr$(66)
Print Hex$(255); Bin$(5); Oct$(9); Hex$(255, 4); Max(3, 7, 2); Min(3, 7, 2); Instr(a$, "o"); Instr(6, a$, "o")
Print Str$(3.14159, 2, 3); Space$(3); "|"; Trim$("  x  "); "|"; Val("12.5") * 2
Print Asin(0.5); Acos(0.5); Atan2(1, 1); Cint(2.6)
Print RGB(red); RGB(10, 20, 30)
On Error Skip
x = RGB(10, 20, 30, 15)
Print MM.ErrNo <> 0
Print MM.HRes > 0; MM.Info(FONTHEIGHT) > 0; MM.Info(SOUND); MM.Info(FONT)
Pixel 5, 5, RGB(green) : Print Pixel(5, 5); Pixel(5, 5, 0)
Dim integer v = 77 : Print Peek(VAR v, 0); KeyDown(0)
""", None),
    ("splice under OPTION LEGACY", """Option Legacy On
CLS
Line (10, 10)-(60, 40), 2
Line (70, 10)-(90, 30), 3, BF
Circle (120, 30), 15, 4
Pixel(5, 5) = 7
Print Pixel(5, 5) <> 0
On Error Skip
Line 10, 50, 60, 55
Print MM.ErrNo <> 0
Option Legacy Off
c = 0 : For y = 0 To 60 : For x = 0 To 140 Step 2 : If Pixel(x, y) Then c = c + 1
Next : Next
Print c
""", None),
]
