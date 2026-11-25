' stack1.bas - Stage 0 of docs/Threads_Plan.md: the C stack one nested FUNCTION call takes. A FUNCTION's body runs in
' an interpreter loop of its own, on top of the statement that called it; a SUB's does not. Each level notes the stack
' pointer (read inside MM.INFO, so every reading has the same frames above it), and the differences are printed: bytes
' a level. A number, a string and a SUB are tried. No .expected: the figures are the build's. Run it with OPTION
' COMPILE OFF and ON: a compiled call takes less.
Option Explicit
Const LEVELS = 10
Dim Integer sp(LEVELS), i, r
Dim s$

Function DeepI%(n%)
  sp(n%) = MM.Info(Stack)
  If n% < LEVELS Then DeepI% = DeepI%(n% + 1) + 1 Else DeepI% = 0
End Function

Function DeepS$(n%)
  sp(n%) = MM.Info(Stack)
  If n% < LEVELS Then DeepS$ = "x" + DeepS$(n% + 1) Else DeepS$ = ""
End Function

Sub DeepSub(n%)
  sp(n%) = MM.Info(Stack)
  If n% < LEVELS Then DeepSub n% + 1
End Sub

Sub Show(what$)
  Local Integer k, lo = 1000000, hi
  For k = 1 To LEVELS
    If sp(k - 1) - sp(k) < lo Then lo = sp(k - 1) - sp(k)
    If sp(k - 1) - sp(k) > hi Then hi = sp(k - 1) - sp(k)
  Next
  Print what$; (sp(0) - sp(LEVELS)) \ LEVELS; " bytes a level (from"; lo; " to"; hi; ")"
End Sub

r = DeepI%(0) : Show "FUNCTION returning an integer: "
s$ = DeepS$(0) : Show "FUNCTION returning a string:   "
DeepSub 0 : Show "SUB:                           "
Print "Deepest in this program:"; &H2001FFF0 - MM.Info(Stack Low); " bytes below the top of the stack"
