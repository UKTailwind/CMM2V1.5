' json.bas - JSON$ (the PicoMite's, with its faults fixed): values of every type; a second index read
' (it was always 0); the long string read by its own length (after LONGSTRING CLEAR the old bytes were parsed);
' the result's own buffer (true/false went into the last string function's); the errors.
Option Explicit
Dim integer tests, pass, i, n
Dim integer j%(200), k%(100)
Dim s$

LongString Append j%(), "{" + Chr$(34) + "a" + Chr$(34) + ":1," + Chr$(34) + "b" + Chr$(34) + ":" + Chr$(34) + "two" + Chr$(34) + ","
LongString Append j%(), Q$("c") + ":true," + Q$("d") + ":false," + Q$("e") + ":null," + Q$("f") + ":1.5," + Q$("Name") + ":" + Q$("N") + ","
LongString Append j%(), Q$("obj") + ":{" + Q$("in") + ":3},"
LongString Append j%(), Q$("rows") + ":[{" + Q$("cells") + ":[{" + Q$("v") + ":1},{" + Q$("v") + ":2},{" + Q$("v") + ":3}]},"
LongString Append j%(), "{" + Q$("cells") + ":[{" + Q$("v") + ":4},{" + Q$("v") + ":5},{" + Q$("v") + ":6}]}]}"

' ---- values -----------------------------------------------------------------------------------
CheckS "an integer", Json$(j%(), "a"), "1"
CheckS "a string", Json$(j%(), "b"), "two"
CheckS "true", Json$(j%(), "c"), "true"
CheckS "false", Json$(j%(), "d"), "false"
CheckS "null", Json$(j%(), "e"), ""
CheckS "a float", Json$(j%(), "f"), "1.5"
CheckS "the last key in any case", Json$(j%(), "name"), "N"
CheckS "a nested object's item", Json$(j%(), "obj.in"), "3"
CheckS "no such item", Json$(j%(), "nosuch"), ""
CheckS "an index", Json$(j%(), "rows[0].cells[1].v"), "2"
CheckS "a second index (it read cells[0])", Json$(j%(), "rows[1].cells[2].v"), "6"
CheckS "true after a string function (it was written into that one's buffer)", LCase$("AB") + Json$(j%(), "c"), "abtrue"
s$ = Json$(j%(), "a")
i = MM.Info(Heap)
For n = 1 To 200 : s$ = Json$(j%(), "rows[1].cells[2].v") : Next
n = MM.Info(Heap)                       ' (in a statement of its own: a SUB call's arguments take memory)
Check "the heap is the same after 200 calls (the arena is freed each time)", n, i

' ---- errors ------------------------------------------------------------------------------------
On Error Skip 1
s$ = Json$(j%(), "obj")
CheckS "an object is not an item", ErrText$(), "Not an item"
On Error Skip 1
s$ = Json$(j%())
CheckS "the argument count", ErrText$(), "Argument count"
On Error Skip 1
s$ = Json$(j%(), 5)
Check "a path that is not a string", MM.ErrNo <> 0, 1
CheckS "and then JSON$ works again", Json$(j%(), "a"), "1"
On Error Skip 1
s$ = Json$(j%(), String$(40, "k"))
CheckS "an item name over 31", ErrText$(), "Item name too long"

' a string value over 255 characters
LongString Append k%(), "{" + Q$("s") + ":" + Chr$(34) + String$(200, "x")
LongString Append k%(), String$(100, "y") + Chr$(34) + "}"
On Error Skip 1
s$ = Json$(k%(), "s")
CheckS "a string over 255", ErrText$(), "String too long"

' nesting beyond 128 levels
LongString Clear k%()
LongString Append k%(), "{" + Q$("a") + ":" + String$(150, "[")
LongString Append k%(), String$(150, "]") + "}"
On Error Skip 1
s$ = Json$(k%(), "a")
CheckS "nesting deeper than 128", ErrText$(), "Invalid JSON data"

' after LONGSTRING CLEAR the old bytes are still in the array
LongString Clear j%()
On Error Skip 1
s$ = Json$(j%(), "a")
CheckS "a cleared long string is empty (its old bytes were parsed)", ErrText$(), "Invalid JSON data"
n = MM.Info(Heap)
Check "and the heap is the same after the errors", n, i

Print "json:"; pass; " of"; tests; " passed"
End

Function Q$(t$)
  Q$ = Chr$(34) + t$ + Chr$(34)
End Function

' the error message without "Error in line n: "
Function ErrText$()
  Local integer c
  c = Instr(MM.ErrMsg$, ": ")
  If c Then ErrText$ = Mid$(MM.ErrMsg$, c + 2) Else ErrText$ = MM.ErrMsg$
End Function

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub

Sub CheckS(n$, got$, want$)
  tests = tests + 1
  If got$ = want$ Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got "; got$; " want "; want$
  EndIf
End Sub
