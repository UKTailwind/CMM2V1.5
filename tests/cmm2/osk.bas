' osk.bas: the on-screen keyboard with no touches. The test sets OPTION SCREEN KEYBOARD itself and clears it at the end
' (the other tests expect it off: Peter, 27 Oct 2026). A program has the whole screen with the option set, after LIST
' OPTIONS reloads the options too, so LIST PINS does not page; KEYBOARD ON and OFF; KEYBOARD ON with no keyboard set.
Option Explicit
Option Screen Keyboard 30
List Options
Print "-- LIST PINS after LIST OPTIONS: the whole screen, no paging"
List Pins
Keyboard On
Print "KEYBOARD ON: ok"
Keyboard Off
Print "KEYBOARD OFF: ok"
Option Screen Keyboard 0
On Error Skip
Keyboard On
Print "KEYBOARD ON with no keyboard set: "; MM.ErrMsg$
Print "done"
