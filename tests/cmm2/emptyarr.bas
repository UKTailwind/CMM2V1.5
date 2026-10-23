' emptyarr.bas: an array parameter the call gives no argument: "Incompatible type: " with nothing after it. The
' entries for it were never set, so the message printed the bytes at address 0 (PicoMite issue #131); MM.END(a()),
' called with none, is the mmb2c corpus's mmendarr.
Sub S(a())
  Print "not reached"
End Sub
On Error Skip
S
Print "["; MM.ErrMsg$; "]"
Dim x(2)
S x()
Print "with an array: no error"
