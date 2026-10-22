' m4fixes.bas - the code issues the manual's reference pass found (docs/Code_Issues_from_the_Manual_Audit.md, 20 Oct
' 2026): a check for each that a program can see with nothing wired. It runs in MODE 8 (640 x 480, 8-bit colour); the
' CAN checks use the internal loop back (mode 1), which needs no transceiver.
Option Explicit
Dim integer tests, pass, i, k, n, sent, mlen, ref
Dim float x, t0
Dim callres$, s$

#COMMENT START
#DEFINE "M4ZZ", "123"
#COMMENT END
Dim integer M4ZZ = 5
Check "a #DEFINE inside #COMMENT START ... END is a comment (it took effect)", M4ZZ, 5
Dim integer MMDEBUGGER
MMDEBUGGER = 3
Check "a line that starts with MMDEBUGGER is kept (every MMDEBUG... line was dropped)", MMDEBUGGER, 3

' ---- sprites -------------------------------------------------------------------------------------------------------
CLS
Open "m4spr.spr" For Output As #1
Print #1, "4,1"                                                     ' one 4 x 4 sprite declared, and two sprites' lines
For i = 1 To 8 : Print #1, "4444" : Next
Close #1
Sprite Load "m4spr"
Check "SPRITE LOAD adds .SPR (it was appended to the statement)", Sprite(W, 1), 4
Check "it loads the number of sprites the file declares (it read one more)", Sprite(W, 2), -1
Open "m4spr2.spr" For Output As #1
Print #1, "8,2"
For i = 1 To 16 : Print #1, "11111111" : Next
Close #1
Sprite Load "m4spr2.spr", 1
Check "SPRITE LOAD over a sprite of another size takes the new size (the old buffer was kept)", Sprite(W, 1), 8
Sprite Copy #1, #33, 2
Sprite Close #33 : Sprite Close #34
On Error Skip 1
Sprite Close #1
CheckS "SPRITE COPY to 33 and 34, those closed, then the original (1<<33 overflowed)", ErrText$(), ""
On Error Skip 1
Sprite Copy #2, #64, 2
CheckS "SPRITE COPY past sprite 64", ErrText$(), "2 is invalid (valid is 1 to 1)"
Sprite Load "m4spr2.spr", 1
Sprite Show #1, 100, 100, 1
On Error Skip 1
Sprite Swap #1, #2
CheckS "SPRITE SWAP with # before the second number (it gave an error)", ErrText$(), ""
Check "  the second is showing", Sprite(X, 2), 100
Sprite Hide All
On Error Skip 1
Sprite Hide #2
CheckS "SPRITE HIDE during SPRITE HIDE ALL is refused", ErrText$(), "Sprites are hidden"
Sprite Restore
Sprite Hide #2
Sprite Close All
CLS
Sprite Scroll 10, 0, RGB(red)
Pixel 630, 470, RGB(red)
Check "SPRITE SCROLL x, y, colour fills with the colour (it took y's value)", Pixel(0, 0), Pixel(630, 470)
CLS

' ---- the cursor ------------------------------------------------------------------------------------------------------
Open "m4cur.cur" For Output As #1
Print #1, "3,3,1,1"
For i = 1 To 3 : Print #1, "777" : Next
Close #1
On Error Skip 1
GUI Cursor Load "m4cur"
CheckS "GUI CURSOR LOAD adds .CUR", ErrText$(), ""
GUI Cursor On
On Error Skip 1
GUI Cursor Color RGB(red)
CheckS "GUI CURSOR COLOR (only COLOUR was taken)", ErrText$(), ""
GUI Cursor Off

' ---- pages and the framebuffer ---------------------------------------------------------------------------------------
Framebuffer Create 800, 600
Framebuffer Backup
Framebuffer Create 1000, 700
On Error Skip 1
Framebuffer Restore
CheckS "FRAMEBUFFER CREATE again deletes the backup (RESTORE copied the new size through the old)", ErrText$(), "Framebuffer backup not created"
Page Write 1 : CLS RGB(white)
Page Write 2 : CLS RGB(red)
Framebuffer Write
Check "FRAMEBUFFER WRITE: MM.HRES is the framebuffer's", MM.HRes, 1000
On Error Skip 1
Framebuffer Create 900, 700
CheckS "FRAMEBUFFER CREATE is refused while the framebuffer is written to", ErrText$(), "Framebuffer is set for write"
Page And_Pixels 1, 2, 3                                             ' (the write page's width was used: an overrun)
Framebuffer Write N
Check "the PicoMite's FRAMEBUFFER WRITE N draws on the display again", MM.HRes, 640
Page Write 2 : ref = Pixel(639, 479)
Page Write 3
Check "PAGE AND_PIXELS with a bigger write page: the pages' own size", Pixel(639, 479), ref
Page Write 3 : CLS RGB(green) : ref = Pixel(5, 5)
Page Write 4 : CLS 0
Page Write 0
Page Copy 1 To 2, D
Page Copy 3 To 4, I
Page Write 4
Check "an immediate PAGE COPY while a D copy waits is made (it was skipped)", Pixel(5, 5), ref
Page Write 0
Framebuffer Close
On Error Skip 1
Pixel Fill MM.HRes, 0, RGB(red)
CheckS "PIXEL FILL off the page", ErrText$(), "640 is invalid (valid is 0 to 639)"
On Error Skip 1
Draw3D Light 5, 0, 0, 0, 50
CheckS "DRAW3D LIGHT on an object that does not exist (it faulted)", ErrText$(), "Object 5 does not exist"

' ---- strings and numbers ---------------------------------------------------------------------------------------------
CheckS "DATETIME$(0) (every number was refused)", Datetime$(0), "01-01-1970 00:00:00"
On Error Skip 1
x = Epoch("01-01-2020 10:61:00")
CheckS "EPOCH with 61 minutes (it tested the month)", ErrText$(), "Invalid time"
On Error Skip 1
s$ = Format$(1, "%300f")
CheckS "FORMAT$ longer than a string (it overran its buffer)", ErrText$(), "String too long"
Dim integer ls%(10), ld%(2)
LongString Load ls%(), 10, "abc"
Check "LONGSTRING LOAD with a count over the string's length (it copied the count)", LLen(ls%()), 3
On Error Skip 1
LongString Left ld%(), ls%(), -1
CheckS "LONGSTRING LEFT with a negative count (it ran away)", ErrText$(), "-1 is invalid (valid is 0 to 2147483647)"
Check "LINSTR's regular expression form with start past the string (it read past it)", LInStr(ls%(), "b", 10, mlen), 0
On Error Skip 1
x = Str2Bin(Int64, "123456789")
CheckS "STR2BIN with more than 8 bytes (it overwrote the stack)", ErrText$(), "String length"
Check "CHOICE(0.5, ...) is the first (the condition was truncated)", Choice(0.5, 1, 2), 1
CheckS "TRIM$ with a lower case where$", Trim$(" ab ", " ", "r"), " ab"
On Error Skip 1
Date$ = "01-01-1999"
CheckS "DATE$ = a year the clock cannot hold", ErrText$(), "Year is not valid"
Check "MM.INFO$(RESET) has no CR LF", Instr(MM.Info$(Reset), Chr$(13)), 0
Check "MM.INFO(OPTION CONSOLE PORT) (CONSOLE was matched first)", MM.Info(Option Console Port), 3

' ---- MATH ------------------------------------------------------------------------------------------------------------
Dim integer z%(1)
On Error Skip 1
x = Math(C_Add z%())
CheckS "MATH(C_ADD) with one argument (it read an unset one)", ErrText$(), "Invalid syntax"
Dim float fs(15)
On Error Skip 1
Math FFT fs()
CheckS "MATH FFT with one argument", ErrText$(), "Invalid syntax"
CheckT "MATH(C_ARG), b14's name for C_CARG", Math(C_Arg Math(C_Cplx 0, 1)), Pi / 2, 1e-6
Math Randomize 0
x = 0
For i = 1 To 10 : x = x + Math(Rand) : Next
Check "MATH RANDOMIZE 0 (every number after it was 0)", x > 0, 1
On Error Skip 1
Math Randomize 4294967296
CheckS "MATH RANDOMIZE's seed is 32 bits (2^32 seeded with 0)", ErrText$(), "Number out of bounds"
Dim integer sa%(2) = (1, 2, 3), sb%(2)
On Error Skip 1
Math Shift sa%(), 1, sb%(), X
CheckS "MATH SHIFT's fourth argument is U (another was ignored)", ErrText$(), "Invalid syntax"
Dim float w1(2) = (1.6, 2, 3.4), w2(2)
Dim integer wmin, wmax
Math Window w1(), 0, 1, w2(), wmin, wmax
Check "MATH WINDOW's minimum into an integer is rounded (1.6 was 1)", wmin, 2
Dim integer cd%(2) = (1, 2, 3), cn%(1) = (1, -1)
k = Math(CRC8 cd%(), , 7, 0, 1, 1, 1) Xor Math(CRC8 cd%(), , 7, 0, 0, 1, 1)
Check "CRC8's endmask after reverseOut, as CRC12's and CRC16's (it came out reflected)", k, 1
k = Math(CRC32 cd%(), , &H04C11DB7, 0, 1, 1, 1) Xor Math(CRC32 cd%(), , &H04C11DB7, 0, 0, 1, 1)
Check "CRC32's too", k, 1
Check "a trailing comma is an argument left out: argv[argc] is empty (makeargs, 9ad3be8)", Math(CRC8 cd%(),), Math(CRC8 cd%())
On Error Skip 1
k = Math(CRC8 cn%())
CheckS "a negative byte for MATH(CRC8)", ErrText$(), "Variable > 255"
Dim o$ Length 4
Dim kk$ = "0123456789ABCDEF", ii$ = "0123456789ABCDEF"
On Error Skip 1
Math AES128 Encrypt ECB kk$, ii$, o$
CheckS "MATH AES128 into a string of LENGTH 4 (it wrote 16 bytes)", ErrText$(), "Too many elements for string output"
s$ = "hello"
On Error Skip 1
n = Math(Base64 Encode s$, o$)
CheckS "MATH(BASE64) into a string of LENGTH 4 (it wrote 8 bytes)", ErrText$(), "Too many elements for string output"

' ---- CALL and EXECUTE ------------------------------------------------------------------------------------------------
Call Mid$("xM4T", 2), "a", 5
CheckS "CALL with a comma in the name's expression (the arguments were split inside MID$)", callres$, "a5"
Dim integer ex
Execute "M4Ex"
Check "EXECUTE inside an EXECUTE (the outer one's line was overwritten)", ex, 11

' ---- CAN, in the internal loop back ----------------------------------------------------------------------------------
Dim integer cid, ceid, crtr, cdlc, cmsg, cfmi, cret, fdm(1), fr(7)
Dim integer smsg = &HAABB000000000000
CAN Open 1, 500000, 1
CAN Start
CAN Send &H123, 0, 0, 2, smsg, sent
cret = 0 : t0 = Timer
Do : CAN Read 1, cid, ceid, crtr, cdlc, cmsg, cfmi, cret : Loop Until cret Or Timer - t0 > 200
Check "CAN: a Classical message comes back", cret > 0, 1
Check "  its identifier", cid, &H123
Check "  its length (CAN READ always read 0 bytes)", cdlc, 2
Check "  its data, from the most significant byte (SEND made the HAL read out of bounds)", cmsg = smsg, 1
Check "  not a remote frame (rtr was &H20000000 for one)", crtr, 0
CAN Close
fdm(0) = &H0102030405060708 : fdm(1) = &H090A0B0C0D0E0F10
CAN Open 1, 0, 1, 1, 63, 16, 16, 1, 64, 1, 15, 4, 4
CAN Start
CAN Send &H55, 0, 0, 12, fdm(), sent
cret = 0 : t0 = Timer
Do : CAN Read 1, cid, ceid, crtr, cdlc, fr(), cfmi, cret : Loop Until cret Or Timer - t0 > 200
Check "CAN FD: a 12-byte message comes back", cret > 0, 1
Check "  its length", cdlc, 12
Check "  its first 8 bytes, the last integer", fr(1) = fdm(1), 1
Check "  its last 4, the top of the integer before (SEND and READ disagreed)", fr(0) = (fdm(0) And &HFFFFFFFF00000000), 1
' (no CAN CLOSE: the end of the program closes it, so RUN can open it again)

' ---- MODE ------------------------------------------------------------------------------------------------------------
On Error Skip 1
Mode 15, 16
CheckS "MODE 15 in another depth: the message", ErrText$(), "Mode 15 has 8 and 1-bit colour only"
On Error Skip 1
Mode 8, 8, , M4Frame
CheckS "MODE r, bits, , interrupt (Syntax: argv[4] was tested, not *argv[4])", ErrText$(), ""
Mode 8, 8

Print "m4fixes:"; pass; " of"; tests; " passed"
Kill "m4spr.spr" : Kill "m4spr2.spr" : Kill "m4cur.cur"
End

Sub M4T(a$, b)
  callres$ = a$ + Str$(b)
End Sub

Sub M4Ex
  Execute "ex = ex + 10"
  ex = ex + 1
End Sub

Sub M4Frame
End Sub

' the error message without "Error in line n: "
Function ErrText$()
  ErrText$ = MM.ErrMsg$
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

Sub CheckT(n$, got, want, tol)
  tests = tests + 1
  If Abs(got - want) <= tol Then
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
