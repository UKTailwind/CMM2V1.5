' chain.bas - CHAIN (plan step 8, the PicoMite's): the globals are kept (numbers, a string and a string array with a
' LENGTH, arrays, one in the SDRAM heap, a CONST and a structure), and MM.CMDLINE$ is CHAIN's second argument;
' everything else goes as on RUN: an armed SETTICK, an open file. chain_b.inc is the program chained to: it chains
' to itself until its 20th pass, and the free heap is the same at its 20th as at its 2nd.
Type pt
  x As INTEGER
  y As INTEGER
End Type
Dim integer n = 42, a(9), big(99999), pass, tickc, heap2
Dim float f = 2.5
Dim string s$ = "kept", t$(3) Length 10
Dim p As pt
Const k = 7
Dim integer i
For i = 0 To 9 : a(i) = i * i : Next
big(99999) = 123 : t$(2) = "two" : p.x = 3 : p.y = 4
SetTick 1000, tk                                                ' (it would first fire in 1 s: chain_b.inc waits 1.2 s)
Open "chain.tmp" For Output As #1
Print "chaining"
Chain "chain_b.inc", "hello world"
Print "not here"

Sub tk
  tickc = tickc + 1
End Sub
