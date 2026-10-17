' library_lib.inc - the library tests/cmm2/library.bas loads into RAM
CONST LIB_K = 6011
DIM INTEGER libcalls = 0
DIM libname$ LENGTH 20
libname$ = "the library"
TYPE LibPoint
  x AS INTEGER
  y AS INTEGER
END TYPE

SUB LibCount
  libcalls = libcalls + 1
END SUB

FUNCTION LibSign$(n)
  IF n > 0 THEN
    LibSign$ = "plus"
  ELSEIF n < 0 THEN
    LibSign$ = "minus"
  ELSE
    LibSign$ = "zero"
  ENDIF
END FUNCTION

FUNCTION LibSum(a%())
  LOCAL i, s
  FOR i = 0 TO 2 : s = s + a%(i) : NEXT
  LibSum = s
END FUNCTION

SUB LibReadOwn(r%())
  RESTORE
  READ r%(0), r%(1)
END SUB

SUB LibDiv(d)
  LOCAL q = 10 / d
END SUB

LibTable:
DATA 101, 102, 103

DefineFont #9
  01410404 F99F0000
End DefineFont
