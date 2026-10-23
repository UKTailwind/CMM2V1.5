' blockok.bas: /* ... */ block comments as the manual has them: /* and */ the first non-space characters of a line,
' a space or the end of the line after them, outside SUBs and FUNCTIONs, the rest of a */ line comment too. The forms
' the loader refuses are block*.bas, each failing the load.
Print "before"
/*
Print "inside: not run"
  /* a /* on a block's line is its text
Sub NotDefined
End Sub
*/ the rest of this line is comment too
Print "after"
   /*
   indented markers are fine
   */
Print "a /* and a */ in strings"
Print "in a comment"  ' /* not a block */
Rem /* nor here
Data "/*", "*/"
Read a$, b$
Print a$; " "; b$
/*
*/
S
Sub S
  Print "S runs"
End Sub
/* after the last SUB
*/
