' pmfix.bas - the PicoMite's issues 101 to 116 (21 Oct 2026) that apply to the CMM2, each as its repro. The ones a
' program cannot check here (EXECUTE "RUN", a comment after FILES, RUN or HELP, GUI CLICK PIN's INV, PLAY SOUND) were
' run at the prompt.
Type tpt
  x As INTEGER
  y As INTEGER
End Type
k = 1
' #101: the '=' of an assignment is the one after the variable, not a comparison inside its brackets
Dim a(3)
a(Choice(k = 1, 1, 2)) = 5 : Print "101 let"; a(1); a(2)
s$ = "abcdef" : Mid$(s$, Choice(k = 1, 2, 3), 2) = "XY" : Print "101 mid$ "; s$
b% = 0 : Bit(b%, Choice(k = 1, 2, 3)) = 1 : Print "101 bit"; b%
t$ = "abc" : Byte(t$, Choice(k = 1, 2, 3)) = 65 : Print "101 byte "; t$
On Error Skip : a(1) 5 : Print "101 err "; MM.ErrMsg$
' #102: a FUNCTION's call as a graphics command's coordinate (findvar evaluated its arguments as indices)
Dim xs(1) = (10, 20)
On Error Skip : Pixel mean(xs()), 10, RGB(white) : Print "102 pixel "; MM.ErrMsg$; Pixel(15, 10) <> 0
On Error Skip : Pixel big(3E9), 10 : Print "102 big "; MM.ErrMsg$
' #103: RESTORE with an expression; a label that is also an array's name
n = 2
Restore Choice(n = 1, "one", "two") : Read d : Print "103 choice"; d
Restore Mid$("xxtwoxx", 3, 3) : Read d : Print "103 mid$"; d
Restore pick$(2) : Read d : Print "103 fun"; d
Restore picks$("x") : Read d : Print "103 fun str"; d
Dim scores(3) : Restore scores : Read d : Print "103 label"; d
Dim lab$(1) = ("one", "two") : Restore lab$(1) : Read d : Print "103 element"; d
w$ = "one" : Restore w$ : Read d : Print "103 var"; d
Restore two : Read d : Print "103 two"; d
Restore "one" : Read d : Print "103 quoted"; d
On Error Skip : Restore nolabel : Print "103 err "; MM.ErrMsg$
' #104: DIM's LENGTH with an '=' or an apostrophe; a quoted bracket in the bounds
wide = 1
Dim msg$ Length Choice(wide = 1, 80, 40) : msg$ = String$(80, "x") : Print "104 choice"; Len(msg$)
Dim s2$ Length Len("it's") : On Error Skip : s2$ = "abcde" : Print "104 len "; MM.ErrMsg$
q$ = "ab)c" : On Error Skip : Dim a2$(Instr(q$, ")")) : Print "104 bound "; MM.ErrMsg$; Bound(a2$())
q$ = "ab(c" : Dim b2$(Instr(q$, "(")) Length 5
On Error Skip : b2$(1) = "123456789" : Print "104 length "; MM.ErrMsg$
' #105: REDIM's bounds with a function or a bracket
Dim r(2)
ReDim r(Max(4, 8)) : Print "105 max"; Bound(r())
ReDim Preserve r((3)) : Print "105 bracket"; Bound(r())
' #106: STRUCT SAVE and LOAD with a function in the file number
Dim pt1 As tpt, pt2 As tpt
pt1.x = 3 : pt1.y = 4
Open "pmfix.dat" For Output As #1 : Struct Save #Max(1, 0), pt1 : Close #1
Open "pmfix.dat" For Input As #1 : Struct Load #Max(1, 0), pt2 : Close #1
Print "106"; pt2.x; pt2.y
' #107: a file number in a variable starting with G is not GPS
Open "pmfix.dat" For Input As #2
g = 2 : On Error Skip : Close g : Print "107 g "; MM.ErrMsg$
Open "pmfix.dat" For Input As #2
gpsf = 2 : On Error Skip : Close gpsf : Print "107 gpsf "; MM.ErrMsg$
On Error Skip : Close #2 : Print "107 closed "; MM.ErrMsg$
Kill "pmfix.dat"
' #108: EXECUTE of a statement starting with the letters RUN
runs = 1 : Execute "runs = 5" : Print "108"; runs
running = 0 : Execute "running = 1" : Print "108"; running
' #110: ERROR's text is the message, not error()'s format; a long one is cut, not run past errstring
On Error Skip : Error "50% off" : Print "110 "; MM.ErrMsg$
On Error Skip : Error "mail me @ home" : Print "110 "; MM.ErrMsg$
On Error Skip : Error "$5 & up | ~1" : Print "110 "; MM.ErrMsg$
On Error Skip : Error String$(250, "x") + "end" : Print "110 long"; Len(MM.ErrMsg$); " "; Right$(MM.ErrMsg$, 3)
' #111: messages with error()'s codes as text
On Error Skip : Print Format$(1, "%g %g") : Print "111 "; MM.ErrMsg$
On Error Skip : z = Struct(Sizeof) : Print "111 "; MM.ErrMsg$
On Error Skip : z = Struct(Offset "tpt") : Print "111 "; MM.ErrMsg$
On Error Skip : Ray Map 1 : Print "111 "; MM.ErrMsg$
' #112: TRIM$'s letter only as the whole argument (L(0) is an expression)
Dim L(1) As String = ("R", "L")
Print "112 ["; Trim$("  ab  ", " ", L(0)); "]["; Trim$("  ab  ", " ", L); "]["; Trim$("  ab  ", " ", L(1)); "]"
' #116: MEMORY PACK with addresses (CheckEmpty stepped a NULL pointer)
Dim src%(1) = (&H0102030405060708, 0), dst%(1)
sa% = Peek(VarAddr src%()) : da% = Peek(VarAddr dst%())
Memory Pack sa%, da%, 2, 16 : Print "116 "; Hex$(dst%(0))
Memory Pack src%(), dst%(), 2, 32 : Print "116 "; Hex$(dst%(0))
End
one:
Data 1
two:
Data 2
scores:
Data 42
Function mean(v())
  mean = (v(0) + v(1)) / 2
End Function
Function big(v)
  big = 10
End Function
Function pick$(k)
  pick$ = "two"
End Function
Function picks$(k$)
  picks$ = "two"
End Function
