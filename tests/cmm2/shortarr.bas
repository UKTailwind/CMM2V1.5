' shortarr.bas - a 1D string array of 12 bytes or less. findvar kept its characters in the variable's record after the
' dimension table, and the counts that multiply every entry of that table (READ's whole array, parsestringarray's for
' RAY MAP, GUI LISTBOX and the ARRAY commands) took them for dimensions: a wrong count, an error or a crash, varying
' with the strings (21 Oct 2026). Such an array is now kept in the heap, as the PicoMite keeps every array.
Option Explicit
Dim s$(2) Length 3, t$(1) Length 5, u$(1) Length 2
s$(0) = "1-1" : s$(1) = "#a#" : s$(2) = "zzz"
Print s$(0); " "; s$(1); " "; s$(2); Bound(s$()); Len(s$(1))
Read s$(), t$(), u$()
Print s$(0); " "; s$(1); " "; s$(2); " "; t$(0); t$(1); " "; u$(0); u$(1)
On Error Skip : Ray Map 3, 3, s$() : Print MM.ErrMsg$
s$(0) = "101" : s$(1) = "1-1" : s$(2) = "111"
On Error Skip : Ray Map 3, 2, s$() : Print MM.ErrMsg$
s$(1) = "101"
Ray Map 3, 3, s$() : Print Ray(MapW); Ray(MapH); Ray(Cell 1, 1); Ray(Cell 0, 2)
Data "c-c", "b#b", "a-a", "hello", "world", "ab", "cd"
