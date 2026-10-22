' datakw.bas - a keyword inside a DATA item's expression. A DATA statement keeps the case it was typed in, and READ
' evaluates its numbers: DATA rgb(red) gave "Invalid colour" (22 Oct 2026, a user's report), as the CMM2's checkstring
' matched keywords in capitals only. It ignores case now, as the PicoMite's does.
Dim integer a, b, c, d, e, f
Read a, b, c, d
Print Hex$(a); " "; Hex$(b); " "; Hex$(c); " "; Hex$(d)
Read e, f
Print e = MM.Info(FONTHEIGHT); f = MM.Info(OPTION BASE)
Print Hex$(RGB(red)); " "; Hex$(RGB(255, 0, 0))
Data rgb(red), RGB(Red), rgb(255, 0, 0), Rgb(blue)
Data mm.info(fontheight), Mm.Info(option base)
