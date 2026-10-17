' library.bas - LIBRARY LOAD ..., RAM (the PicoMite's, 17 Oct): the library's top level runs (its CONST, DIM and
' TYPE), its SUB and FUNCTIONs are called, its label and DATA are read, READ goes on from the program's DATA into the
' library's, RESTORE in library code starts at the library's DATA, its font is used, and an error in it names its file.
LIBRARY LOAD "library_lib.inc", RAM
Option Explicit
Dim integer i, a(2) = (1, 2, 3), r(1), d1, d2
Dim p AS LibPoint
Print "const"; LIB_K; " name "; libname$
For i = 1 To 3 : LibCount : Next
Print "count"; libcalls
Print LibSign$(5); " "; LibSign$(-2); " "; LibSign$(0)
Print "sum"; LibSum(a())
p.x = 3 : p.y = 4
Print "type"; p.x * p.y
Restore LibTable : Read d1, d2 : Print "label data"; d1; d2
Restore : Read d1, d2 : Print "own data, then the library's"; d1; d2
LibReadOwn r() : Print "library restore"; r(0); r(1)
Font 9 : d1 = MM.Info(FONTWIDTH) : Font 1 : Print "font width"; d1
On Error Ignore
LibDiv 0
Print "error: "; Mid$(MM.ErrMsg$, Instr(MM.ErrMsg$, "library_lib"))
On Error Abort
Print "done"
Data 1
