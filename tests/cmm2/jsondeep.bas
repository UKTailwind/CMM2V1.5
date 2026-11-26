' jsondeep.bas - JSON$() and nesting: 64 levels of objects or arrays are read, 65 are "Invalid JSON data"
' (CJSON_NESTING_LIMIT, CMakeLists.txt; 100 until 26 Nov 2026). The parser calls itself for each level, 40 bytes of
' stack a level: the limit keeps the deepest document inside the stack check's guard (STACK_GUARD, MMBasic.c).
Option Explicit
Dim Integer l%(200), k
Dim q$ = Chr$(34), path$

' n objects, one inside the other, the innermost {"x":5}; path$ is the way down to x
Sub Objects(n%)
  Local Integer j
  LongString Clear l%()
  path$ = ""
  For j = 2 To n%
    LongString Append l%(), "{" + q$ + "n" + q$ + ":"
    If Len(path$) < 250 Then path$ = path$ + "n."
  Next
  path$ = path$ + "x"
  LongString Append l%(), "{" + q$ + "x" + q$ + ":5}"
  For j = 2 To n% : LongString Append l%(), "}" : Next
End Sub

' {"x":5,"a":[[[...]]]}: n levels with the object
Sub Arrays(n%)
  Local Integer j
  LongString Clear l%()
  path$ = "x"
  LongString Append l%(), "{" + q$ + "x" + q$ + ":5," + q$ + "a" + q$ + ":"
  For j = 2 To n% : LongString Append l%(), "[" : Next
  LongString Append l%(), "7"
  For j = 2 To n% : LongString Append l%(), "]" : Next
  LongString Append l%(), "}"
End Sub

Sub Show(what$)
  Local r$
  On Error Skip
  r$ = JSON$(l%(), path$)
  If MM.Errno Then r$ = MM.ErrMsg$
  On Error Clear
  Print what$; ": "; r$
End Sub

Objects 10 : Show "10 objects"
Objects 64 : Show "64 objects"
Objects 65 : Show "65 objects"
Objects 150 : Show "150 objects"
Arrays 64 : Show "64 levels of arrays"
Arrays 65 : Show "65 levels of arrays"
Objects 3 : Show "3 objects, after them"
