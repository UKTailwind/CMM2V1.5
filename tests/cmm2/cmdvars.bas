' cmdvars.bas - a command's name as a variable's (1 Nov 2026: NOMIS's "flash = 1" was the FLASH command, "Syntax").
' At the start of a statement NAME = x (and NAME$, NAME% or NAME! = x) is an assignment; tokenise leaves the name as
' text. Not a name that is a keyword or a function as well (FOR, TIMER), nor FLAGS, CASE, REM, RUN or DATA, where
' NAME = is the command's own. Not an array: IF (x) = y THEN and the like are the commands (issue #10); only TILE,
' DEVICE and RESOLUTION arrays, as before.
Option Default Integer
flash = 1 : fill = 2 : sync = 3 : mouse = 4 : var = 5 : chain = 6 : star = 7 : drive = 8
keyboard = 9 : library = 10 : randomize = 11 : location = 12 : fm = 13 : gamepad = 14 : interrupt = 15
Print flash; fill; sync; mouse; var; chain; star; drive; keyboard; library; randomize; location; fm; gamepad; interrupt
print = 16 : circle = 17 : pin = 18
Print print; circle; pin
ray% = 19 : slew$ = "s" : bezier! = 2.5
Print ray%; " "; slew$; bezier!
Dim tile(2) : tile(1) = 20 : Print "tile(1)"; tile(1)
x = 4
If (x And 1) = 0 Then Print "IF (x And 1) = 0: the IF"
If (x) = 5 Then
  Print "wrong"
ElseIf (x) = 4 Then
  Print "ELSEIF (x) = 4: the ELSEIF"
EndIf
Select Case x
  Case = 3
    Print "wrong"
  Case = 4
    Print "CASE = 4: the CASE"
End Select
PlayBeep 2, 1
PlayBeep 3, 0
rem = 3 : Print "not printed: REM's text"
REM = = =
Print "after REM"
On Error Skip
for = 1
Print "FOR = 1: error"; MM.ErrNo <> 0
Flags = 5
Print "MM.FLAGS"; MM.Flags
Print "DRIVE "; MM.Info$(DRIVE) : Drive "A:" : Print "DRIVE "; MM.Info$(DRIVE)

Sub PlayBeep button, toval
  Local note, flash
  flash = 1
  If toval Then flash = 0
  Print "button"; button; " flash"; flash
End Sub
