' parity1.bas - the first PicoMite parity set taken from Gerry Allardice's 6.00.00b14 (M4 of the b14 merge,
' docs/Gerry_b14_Review_and_Merge_Plan.md 3.2): MID$ and LMID insert, LINPUT(, MEMORY INPUT's short read, INSTR's regex
' branch with an integer variable and OPTION ESCAPE, MM.VER, SYNC, SETPIN CIN's options, TEMPR's timeouts, MM.ONEWIRE,
' OPTION ESCAPE OFF, a FUNCTION's negative argument and a string literal without its closing quote.
Option Explicit
Dim string s$, e$
Dim integer n, bad = 0
Dim float fl, t
Dim ls%(32), raw%(1), arr(5)

' MID$(s$, pos, 0) = x$ inserts; a shorter or longer replacement moves the rest along
s$ = "abcdef"
Mid$(s$, 3, 0) = "XY"
Print "mid insert: "; s$
Mid$(s$, 1, 2) = "Q"
Print "mid shrink: "; s$
Mid$(s$, 2, 2) = "123"
Print "mid grow: "; s$
Mid$(s$, 1, 4) = "ab"
Print "mid replace: "; s$
On Error Skip 1
Mid$(s$, 5, 3) = "z"
e$ = MM.ErrMsg$ : Print "mid beyond: "; e$

' LMID does the same on a LONGSTRING
LongString Clear ls%()
LongString Append ls%(), "abcdef"
LMid(ls%(), 3, 0) = "XY"
Print "lmid insert: "; LGetStr$(ls%(), 1, LLen(ls%()))
LMid(ls%(), 1, 2) = "Q"
Print "lmid shrink: "; LGetStr$(ls%(), 1, LLen(ls%()))
LMid(ls%(), 2, 2) = "123"
Print "lmid grow: "; LGetStr$(ls%(), 1, LLen(ls%()))
LMid(ls%(), 1, 4) = "ab"
Print "lmid replace: "; LGetStr$(ls%(), 1, LLen(ls%()))
On Error Skip 1
LMid(ls%(), 5, 3) = "z"
e$ = MM.ErrMsg$ : Print "lmid beyond: "; e$

' LINPUT( reads n bytes of a file into a LONGSTRING; MEMORY INPUT reads fewer bytes than the file holds
Open "parity1.tmp" For Output As #1
Print #1, "0123456789ABCDEF";
Close #1
Open "parity1.tmp" For Input As #1
LongString Clear ls%()
n = LInput(ls%(), #1, 5)
Print "linput: "; n; " "; LGetStr$(ls%(), 1, LLen(ls%()))
n = LInput(ls%(), #1, 100)
Print "linput rest: "; n; " "; LGetStr$(ls%(), 1, LLen(ls%()))
Close #1
Open "parity1.tmp" For Input As #1
Memory Input #1, 8, raw%()
Print "memory input: "; Hex$(raw%(0), 16)
Memory Input #1, 8, raw%()
Print "memory input 2: "; Hex$(raw%(0), 16)
On Error Skip 1
Memory Input #1, 8, raw%()
e$ = MM.ErrMsg$ : Print "memory input at end: "; e$
Close #1
Kill "parity1.tmp"

' INSTR's regex branch: the length can go to an integer, and the pattern's backslashes are the regex's
Print "instr regex: "; Instr(1, "hello world", "w.r", n); n
Print "instr regex float: "; Instr("hello world", "l+o", fl); fl
Option Escape
Print "instr escape: "; Instr("a.b", "\.", n); n
Print "escape on: "; Len("a\nb")
Option Escape Off
Print "instr no escape: "; Instr("a.b", "\.", n); n
Print "escape off: "; Len("a\nb")

' MM.VER with a beta's fourth field
Print "ver: "; MM.Ver

' SYNC n, M then SYNC waits for the end of each period
SYNC 20, M
t = Timer
SYNC
SYNC
t = Timer - t
Print "sync: "; Choice(t >= 38 And t <= 44, "ok", "bad " + Str$(t))
SYNC 0
On Error Skip 1
SYNC
e$ = MM.ErrMsg$ : Print "sync off: "; e$
On Error Skip 1
SYNC 5, X
e$ = MM.ErrMsg$ : Print "sync unit: "; e$

' SETPIN pin, CIN, option: the edge and the pull
SetPin 7, CIN, 3
Print "cin: "; Pin(7)
SetPin 7, Off
On Error Skip 1
SetPin 7, CIN, 6
e$ = MM.ErrMsg$ : Print "cin option: "; e$

' TEMPR's conversion timeouts (no sensor on the pin: 1000)
On Error Skip 1
fl = Tempr(7, 50)
e$ = MM.ErrMsg$ : Print "tempr timeout: "; e$
Print "tempr no sensor: "; Tempr(7, 100)
Tempr Start 7, 1, 150
Print "tempr start: "; Tempr(7)
SetPin 7, Off
Print "onewire: "; MM.OneWire

' a FUNCTION's negative argument is not an array index; an array's still is
Print "function negative: "; sx(-24)
On Error Skip 1
n = arr(-1)
e$ = MM.ErrMsg$ : Print "array negative: "; e$

' a string literal without its closing quote is closed at the end of the line
Print "unterminated: "; "no closing quote
Print "after it: ok"
End

Function sx(x As Integer) As Integer
  sx = x * 2
End Function
