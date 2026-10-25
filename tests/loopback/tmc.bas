' tmc.bas - TMC22xx's bit-banged UART (Src/stepper_cmm2.c's serialtx, timed by the cycle counter; 25 Oct 2026): its
' datagrams sent on pin 24 (GP9), which is wired to pin 10 (GP1), COM1's receive pin, and read back at 115200: the
' five set-up writes (GCONF, IHOLD_IRUN, TPOWERDOWN, CHOPCONF, PWMCONF) and SGTHRS, each byte and each CRC.
Option Explicit
Option Default Integer
Dim tests, pass, i, k, n, bad, crcbad, regs$, b(63)
Dim s$

Function Crc8(f)                                                    ' the TMC22xx's CRC of a datagram's first 7 bytes
  Local c, j, m, v
  c = 0
  For j = 0 To 6
    v = b(f + j)
    For m = 0 To 7
      If ((c >> 7) Xor (v And 1)) And 1 Then c = ((c << 1) Xor 7) And 255 Else c = (c << 1) And 255
      v = v >> 1
    Next
  Next
  Crc8 = c
End Function

Sub Take                                                            ' what COM1 has, into b()
  Local c$
  n = 0
  Do While Loc(#1) > 0 And n < 64
    c$ = Input$(1, #1)
    b(n) = Asc(c$) : n = n + 1
  Loop
End Sub

Open "COM1:115200" As #1
Pause 100 : s$ = Input$(255, #1)                                    ' (anything already there)
TMC22xx 24, 2209, 0, 800, 50, 16
Pause 100
Take
Check "TMC22xx: five datagrams, 40 bytes received", n, 40
bad = 0 : crcbad = 0 : regs$ = ""
For k = 0 To 4
  If b(k * 8) <> 5 Or b(k * 8 + 1) <> 0 Then bad = bad + 1
  regs$ = regs$ + " " + Hex$(b(k * 8 + 2), 2)
  If Crc8(k * 8) <> b(k * 8 + 7) Then crcbad = crcbad + 1
Next
Check "  each starts with the sync byte 5 and slave 0", bad, 0
CheckS "  the registers written (bit 7 set: a write)", regs$, " 80 90 91 EC F0"
Check "  every CRC right", crcbad, 0
TMC22xx SGTHRS 0, 100
Pause 50
Take
Check "TMC22xx SGTHRS 0, 100: one datagram", n, 8
Check "  register &H40 written, value 100, CRC right", (b(2) = &HC0) + (b(6) = 100) + (Crc8(0) = b(7)), 3
TMC22xx CLOSE
Close #1

Print "tmc:"; pass; " of"; tests; " passed"
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

Sub CheckS(n$, got$, want$)
  tests = tests + 1
  If got$ = want$ Then
    pass = pass + 1
    Print "ok  "; n$
  Else
    Print "FAIL "; n$; " got "; got$; " want "; want$
  EndIf
End Sub
