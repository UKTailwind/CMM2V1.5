' subst.bas - the substitutions made on a line before it is tokenised and as it is loaded (the PicoMite's
' STR_REPLACE, corrected: PicoMite issue #8).  Nothing in a string or a DATA statement is changed; a keyword or a
' #DEFINE inside a longer name is not, even after one at the start of the line; a line that starts REM... but is
' not a REM is substituted; a user FUNCTION may end in HEX$.
#DEFINE "COUNT", "TOTAL"
#DEFINE "LEVELS", "10"
Option Explicit
Dim integer tests, pass, y, total, account
Dim remaining$, a$, b$, c$, d$, e$, f$, g$
Dim z

' ---- strings are left alone ------------------------------------------------------------------
CheckS "ERASE in a string", "ERASE", "ER" + "ASE"
CheckS "a file name in a string", "SPI.BMP", "SP" + "I.BMP"
CheckS "keywords in a string", "I2C2 HUMID DHT22 ZMODEM MM.INFO$ =>", "I2" + "C2 HU" + "MID DH" + "T22 ZMO" + "DEM MM.IN" + "FO$ =" + ">"
CheckS "a #DEFINE name in a string", "LEVELS", "LEV" + "ELS"
Execute "a$ = " + Chr$(34) + "ERASE" + Chr$(34)
CheckS "and in a string given to EXECUTE", a$, "ER" + "ASE"

' ---- the substitutions still happen -----------------------------------------------------------
CheckS "HEX$(", Hex$(255), "FF"
CheckS "BIN$(", Bin$(5), "101"
CheckS "OCT$(", Oct$(15), "17"
Check "=>", 3 => 2, 1

' ---- but not inside a longer name -------------------------------------------------------------
CheckS "a FUNCTION whose name ends in HEX$", ToHex$(255), "ff"
Erase z : y = MyErase(2)
Check "ERASE starting the line, then MyErase( after it", y, 4
count = 1 : account = 5
Check "a #DEFINE starting the line, then inside a longer name", total * 10 + account, 15
remaining$ = Hex$(10)
CheckS "a line starting REMAINING is not a REM", remaining$, "A"

' ---- DATA keeps its words, but not its functions: READ evaluates a number (a #DEFINE applies too) --
Restore
Read a$, b$, c$, d$, e$, f$, g$, y
CheckS "DATA keeps its words", a$ + "," + b$ + "," + c$ + "," + d$, "spi,Erase,I2C2,SPI"
CheckS "and after a colon", e$ + "," + f$, "I2C,ZMODEM"
CheckS "a #DEFINE in DATA", g$, "10"
Check "DATA MM.HRES/2 read as a number", y, MM.HRES / 2

Print "subst:"; pass; " of"; tests; " passed"
End
Data spi, Erase, I2C2, SPI
y = 0 : Data I2C, ZMODEM
Data levels, MM.HRES/2

Function ToHex$(n)
  ToHex$ = LCase$(Hex$(n))
End Function

Function MyErase(n)
  MyErase = n * 2
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
