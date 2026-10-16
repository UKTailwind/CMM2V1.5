' binding.bas - variable scoping on every path the symbols' bindings take (step 6.2): its output was made by the
' firmware without bindings, and must not change with them.  Locals hide a global at their own level only;
' recursion, a GOSUB inside a SUB, a FUNCTION's own variable, STATIC, BYREF, LOCAL arrays, DIM in a SUB, ERASE and
' the variable made again, and EXECUTE making a local from text (global bindings untrusted while it lives).
Option Explicit
Dim integer x = 1, y = 2, depth, r, total
Dim s$ = "global"
Dim integer arr(3) = (10, 20, 30, 40)

Print "global x"; x
ShowX
Print "after ShowX x"; x
Outer
Print "after Outer x"; x; " y"; y
Print "fact 6 ="; Fact(6)
Print "depth"; Recurse(4)
Print "func var"; Twice(21)
Print "static"; Counter(); Counter(); Counter()
r = 5 : AddTo r, 7 : Print "byref"; r
LocalArr : Print "global arr(2)"; arr(2)
MakeGlobal : Print "made in a SUB"; madeinsub
WithGosub
Erase madeinsub
Dim integer madeinsub = 99 : Print "made again"; madeinsub
ExecLocal
Print "after ExecLocal x"; x; " s$ "; s$
Print "done"
End

Sub ShowX
  Local integer x = 100
  Print "local x"; x; " global y"; y
  Inner
  Print "local x after Inner"; x
End Sub

Sub Inner
  Print "Inner sees the global x"; x
  x = x + 1
End Sub

Sub Outer
  Local integer y = 50
  ShowX
  Print "Outer's y"; y
End Sub

Function Fact(n)
  If n <= 1 Then Fact = 1 Else Fact = n * Fact(n - 1)
End Function

Function Recurse(n)
  Local integer level = n
  If n > 0 Then Recurse = Recurse(n - 1) + level Else Recurse = 0
End Function

Function Twice(v)
  Twice = v
  Twice = Twice * 2
End Function

Function Counter()
  Static integer n
  n = n + 1
  Counter = n
End Function

Sub AddTo(a, b)
  a = a + b
End Sub

Sub LocalArr
  Local integer arr(2)
  arr(2) = 7
  Print "local arr(2)"; arr(2)
End Sub

Sub MakeGlobal
  Dim integer madeinsub = 42
End Sub

Sub WithGosub
  Local integer x = 500
  GoSub InSub
  Print "back in WithGosub x"; x
  Exit Sub
InSub:
  Print "the GOSUB sees the global x"; x
  Return
End Sub

Sub ExecLocal
  Execute "LOCAL S$"
  s$ = "local"
  Print "ExecLocal s$ "; s$; " x"; x
End Sub
