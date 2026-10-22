' redim.bas - REDIM [PRESERVE] (M6 of the b14 merge, the PicoMite's): an array made again with new bounds, keeping its
' type and a string's LENGTH; PRESERVE keeps the contents and allows only the last bound to change.
Option Explicit
Dim integer i, a(4), n
Dim float f(2, 3)
Dim string s$(3) Length 8, e$
For i = 0 To 4 : a(i) = i * 10 : Next
ReDim Preserve a(9)
Print "preserve grow: "; a(0); a(4); a(9); Bound(a(), 1)
a(9) = 99
ReDim Preserve a(2)
Print "preserve shrink: "; a(0); a(1); a(2); Bound(a(), 1)
ReDim a(5)
Print "redim clears: "; a(0); a(1); Bound(a(), 1)
a(5) = 7
Print "integer kept: "; a(5) \ 2; " "; a(5) / 2
For i = 0 To 3 : s$(i) = "item" + Str$(i) : Next
ReDim Preserve s$(6)
Print "string: "; s$(0); " "; s$(3); " ["; s$(6); "] "; Bound(s$(), 1)
On Error Skip 1
s$(6) = "123456789"
e$ = MM.ErrMsg$ : Print "length kept: "; e$
f(1, 2) = 1.5
ReDim Preserve f(2, 5)
Print "2d last bound: "; f(1, 2); Bound(f(), 1); Bound(f(), 2)
On Error Skip 1
ReDim Preserve f(5, 5)
e$ = MM.ErrMsg$ : Print "2d first bound: "; e$
ReDim f(5, 5)
Print "2d anew: "; f(1, 2); Bound(f(), 1); Bound(f(), 2)
ReDim Preserve a(n + 3)
Print "expression bound: "; Bound(a(), 1)
a(3) = 33
ReDim Preserve a(Bound(a()) * 2)
Print "bound from the array itself: "; Bound(a(), 1); a(3)
For i = 0 To 3 : s$(i) = "s" + Str$(i) : Next
ReDim Preserve s$(Bound(s$()) + 1)
Print "a string array's own bound: "; Bound(s$(), 1); " "; s$(3)
On Error Skip 1
ReDim i(3)
e$ = MM.ErrMsg$ : Print "not an array: "; e$
On Error Skip 1
ReDim nosuch(3)
e$ = MM.ErrMsg$ : Print "not declared: "; e$
Print "done"
