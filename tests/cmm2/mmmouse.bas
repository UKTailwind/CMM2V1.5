' mmmouse.bas - MM.MOUSE (1 Nov 2026): the channel of the mouse to use, -1 if none, so that a program need not fix one
' (BoxDot's CONST mouseport = 3, NOMIS's 2). The same output whether a mouse is connected or not; with one, it is
' opened on that channel and read. DEVICE(MOUSE n) and MOUSE INTERRUPT take 0, MM.MOUSE's PS2 channel, too. A mode
' change puts the mouse at the new screen's centre (1 Nov: Cuno's cursor began at the start-up mode's centre).
m = MM.Mouse
Print "MM.MOUSE in range:"; m >= -1 And m <= 3
r = 1
If m >= 0 Then
  Controller Mouse Open m
  r = Mouse(X, m) >= 0 And Mouse(Y, m) >= 0
  Controller Mouse Close m
EndIf
Print "open and read:"; r
Print "DEVICE(MOUSE 0, X):"; Device(Mouse 0, X) >= 0
Mouse Interrupt Disable 0
Print "MOUSE INTERRUPT DISABLE 0: ok"
Print "MM.MOUSE again:"; MM.Mouse = m
r = 1
If m >= 0 Then
  Controller Mouse Open m
  Mouse Set m, 10, 10
  Mode 9, 8
  r = Mouse(X, m) = MM.HRes \ 2 And Mouse(Y, m) = MM.VRes \ 2
  Controller Mouse Close m
EndIf
Print "centred by MODE:"; r
