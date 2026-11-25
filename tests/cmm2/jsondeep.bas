' jsondeep.bas - JSON$() and nesting: 100 levels of objects or arrays are read, 101 are "Invalid JSON data"
' (CJSON_NESTING_LIMIT, Inc/cJSON.h). The parser calls itself for each level, 40 bytes of stack a level, and the stack
' check's guard is 6K (STACK_GUARD, MMBasic.c): at cJSON's own limit of 1000 a deep document ran through the guard.
Option Explicit
Dim Integer l%(200), k
Dim q$ = Chr$(34)

' n objects, one inside the other, the innermost {"x":5}
Sub Objects(n%)
  Local Integer j
  LongString Clear l%()
  For j = 2 To n% : LongString Append l%(), "{" + q$ + "n" + q$ + ":" : Next
  LongString Append l%(), "{" + q$ + "x" + q$ + ":5}"
  For j = 2 To n% : LongString Append l%(), "}" : Next
End Sub

' {"x":5,"a":[[[...]]]}: n levels with the object
Sub Arrays(n%)
  Local Integer j
  LongString Clear l%()
  LongString Append l%(), "{" + q$ + "x" + q$ + ":5," + q$ + "a" + q$ + ":"
  For j = 2 To n% : LongString Append l%(), "[" : Next
  LongString Append l%(), "7"
  For j = 2 To n% : LongString Append l%(), "]" : Next
  LongString Append l%(), "}"
End Sub

Sub Show(what$)
  Local r$
  On Error Skip
  r$ = JSON$(l%(), "x")
  If MM.Errno Then r$ = MM.ErrMsg$
  On Error Clear
  Print what$; ": "; r$
End Sub

Objects 10 : Show "10 objects"
Objects 100 : Show "100 objects"
Objects 101 : Show "101 objects"
Objects 150 : Show "150 objects"
Arrays 100 : Show "100 levels of arrays"
Arrays 101 : Show "101 levels of arrays"
Objects 3 : Show "3 objects, after them"
