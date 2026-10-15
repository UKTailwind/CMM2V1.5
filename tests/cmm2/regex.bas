' regex.bas - INSTR's regular expressions (tiny-regex-c, now the PicoMite's copy): {n} matches exactly n (it
' matched n or more, the PicoMite's 7c34e76), {n,} n or more, and the word boundaries \b and \B (new).
Option Explicit
Dim integer tests, pass, p
Dim float l

p = Instr(1, "BAAAB", "A{2}", l)
Check "A{2} in BAAAB: where", p, 2
Check "and exactly 2 (it matched 3)", l, 2
p = Instr(1, "AABBAAAABAAAAAAA", "[A]{3}", l)
Check "[A]{3}: where", p, 5
Check "and its length", l, 3
p = Instr(1, "BAAAB", "A{2,}", l)
Check "A{2,} is still 2 or more", l, 3
p = Instr(1, "concat cat", "\bcat\b", l)
Check "\bcat\b finds the word, not the end of concat", p, 8
p = Instr(1, "concat cat", "\Bcat", l)
Check "\Bcat finds it inside concat", p, 4
p = Instr(1, "abc123def", "[0-9]+", l)
Check "[0-9]+: where", p, 4
Check "and its length", l, 3
p = Instr(1, "abc", "x+", l)
Check "no match", p, 0

Print "regex:"; pass; " of"; tests; " passed"
End

Sub Check(n$, got, want)
  tests = tests + 1
  If got = want Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got"; got; " want"; want
  EndIf
End Sub
