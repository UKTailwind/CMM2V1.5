' context_b.inc - run by context.bas after SAVE CONTEXT: LOAD CONTEXT brings back its globals (the same TYPE first)
Type pt
  x As INTEGER
  y As FLOAT
  s As STRING LENGTH 8
End Type
Print "before LOAD: n ="; n
Load Context
Print "in context_b:"; n; " "; s$; a(4); big(19999); " "; p.s; q(1).x; c
End
