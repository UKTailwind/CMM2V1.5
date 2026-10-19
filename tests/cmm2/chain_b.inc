' chain_b.inc - the program chain.bas chains to (it defines the same TYPE first, as CHAIN needs)
Type pt
  x As INTEGER
  y As INTEGER
End Type
pass = pass + 1
If pass = 2 Then heap2 = MM.Info(HEAP)
If pass = 1 Then
  Print "MM.CMDLINE$: ["; MM.CmdLine$; "]"
  Print "kept:"; n; f; " "; s$; " "; t$(2); a(9); big(99999); k; p.x; p.y
  w = Timer
  Do While Timer - w < 1200 : Loop
  Print "ticks after CHAIN:"; tickc
  On Error Skip
  Print #1, "x"
  Print "file #1: "; MM.ErrMsg$
EndIf
If pass < 20 Then Chain "chain_b.inc"
h = MM.Info(HEAP)                                               ' (in a statement of its own: a PRINT's strings take some)
Print "passes:"; pass; "  the free heap at the 20th: "; Choice(h = heap2, "as at the 2nd", "DIFFERENT by" + Str$(h - heap2))
Print "still kept:"; n; " "; s$; " "; t$(2); big(99999); p.y
End
