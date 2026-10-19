' context.bas - SAVE CONTEXT [CLEAR] and LOAD CONTEXT [KEEP] (plan step 8, the PicoMite's): the globals go to
' A:/.vars on the SD card and come back, in this program and across RUN into another (context_b.inc).
Type pt
  x As INTEGER
  y As FLOAT
  s As STRING LENGTH 8
End Type
Dim integer n = 5, a(4), big(19999)
Dim float f = 1.5
Dim string s$ = "abc", u$(2) Length 5
Dim p As pt, q(1) As pt
Const c = 9
Dim integer i
For i = 0 To 4 : a(i) = i + 10 : Next
big(19999) = 777 : u$(1) = "uu" : p.x = 1 : p.y = 2.5 : p.s = "member" : q(1).x = 11
Save Context
n = 99 : s$ = "changed" : a(4) = -1 : u$(1) = "zz" : p.s = "other" : big(19999) = 0 : q(1).x = 0
Load Context Keep
Print "restored:"; n; f; " "; s$; a(4); " "; u$(1); p.x; p.y; " "; p.s; q(1).x; big(19999); c
Save Context Clear
Print "after CLEAR: n ="; n; "  s$ = ["; s$; "]"
Load Context
Print "loaded again:"; n; " "; s$; a(4)
On Error Skip
Load Context
Print "a third LOAD: "; MM.ErrMsg$
Save Context
Run "context_b.inc"
