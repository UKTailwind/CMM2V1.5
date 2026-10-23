' p1fixes.bas - P1's open items fixed on 17 Oct: /* */ block comments taken by the loader; a PRINT that fails part
' way prints nothing (the PicoMite's buffered PRINT); OPTION LOCAL VARIABLES accepted; an expression passed BYVAL
' from a SUB whose first local is an array; an out-of-memory error under ON ERROR SKIP in a SUB keeps its locals;
' SETTICK refuses -1.
Dim integer r
/*
Print "this is inside a block comment"
Sub NotReal
End Sub
*/
Print "after the block comment"
/* the rest of a /* line is comment
*/ and so is the rest of a */ line
Print "after the second block comment"
Option Local Variables 300
Print "OPTION LOCAL VARIABLES accepted"
On Error Ignore
Print "partial "; 1 / 0; " never"
Print "errno"; MM.Errno <> 0
On Error Abort
Outer
MemTest
On Error Ignore
SetTick -1, Tick
Print "settick -1: "; em$()
On Error Abort
Print "done"
End

Sub Outer
  Local integer arr(3)
  arr(1) = 9
  Print "byval with a local array first:"; Twice(3 + 4); arr(1)
End Sub

Function Twice(BYVAL x%) As integer
  Twice = 2 * x%
End Function

Sub MemTest
  Local integer keep = 5
  On Error Skip
  Local integer big(100000000)
  Print "after the memory error: keep ="; keep; ", errno"; MM.Errno <> 0
End Sub

Sub Tick
End Sub

Function em$()
  em$ = MM.ErrMsg$
End Function
