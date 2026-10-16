' folds.bas - LEFT$( RIGHT$( UCASE$( LCASE$( MIN( MAX( BIN$( OCT$( HEX$( through the tokeniser's folds (step 6a,
' the PicoMite's SCHANGE$( and TOPBOTTOM(): its output was made by the firmware with a token for each function, and
' must not change with the folds.  Variables named as the selector letters must not be read in their place.
Option Explicit
Dim s$ = "Hello World", t$
Dim integer n = 3, E = 99, R = 98, U = 97, L = 2, A = 7, I = 5
Dim arr(3) = (1, 5, 2, 4)
Const CAPS$ = UCase$("const")
Print Left$(s$, 5); "|"; Right$(s$, 5); "|"; UCase$(s$); "|"; LCase$(s$)
Print left$(s$,n); "|"; RIGHT$(s$, n); "|"; ucase$( "mixed Case" ); "|"; LCase$("ABC")
Print Left$ (s$, 2); "|"; Right$  (s$, 2); "|"; UCase$ ("x"); "|"; LCase$ ("Y"); "|"; Max (3, 4); Min  ( 9, 8 )
Print Left$(s$, 0); "|"; Left$(s$, 99); "|"; Right$(s$, 0); "|"; Right$(s$, 99); "|"
Print Left$(UCase$(Right$(s$, 5)), 3); "|"; LCase$(Left$(s$, 1) + Right$(s$, 1))
Print Left$(s$, L); "|"; Right$(s$, L); Max(A, I); Min(A, I); Max(E, R, U)
Print Max(1, 5, 3); Min(4, -2, 7); Max(2.5); Min(-1e300, -2e300); Max(-1e300, -2e300)
Print Max(Min(10, 20), Min(30, 5)); Max(1, 2) + Min(3, 4); Len(Left$(s$, 4)) + Max(1, 2)
Print Hex$(255); " "; Bin$(5); " "; Oct$(8); " "; Hex$(255, 4); " "; Hex$(Max(15, 16))
Print CAPS$; " "; Math(Max arr()); Math(Min arr())
If Left$(s$, 1) = "H" Then Print "if "; LCase$(Left$(s$, 1)) Else Print "else"
t$ = "LEFT$(s$, 1) MAX(1,2) in a string": Print t$     ' and LEFT$( MAX( in a comment
Print "MIN(1,2) stays"; ' MIN(
Restore d1: Read t$: Print t$
Print MyLeft$("abc"); MyMax(1); Tobase$(2, 5); " "; Show$(UCase$("sub"))
On Error Skip: Print Left$(s$): Print "err "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
On Error Skip: Print Right$(s$): Print "err "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
On Error Skip: Print Max(): Print "err "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, ":") + 2)
Print "done"
End
d1: Data "LEFT$(quoted)"
Function MyLeft$(a$)
  MyLeft$ = Left$(a$, 1)
End Function
Function MyMax(a)
  MyMax = Max(a, 10)
End Function
Function Tobase$(b, v)
  Tobase$ = "base" + Str$(b) + ":" + Str$(v)
End Function
Function Show$(x$)
  Show$ = "[" + LCase$(x$) + "]"
End Function
