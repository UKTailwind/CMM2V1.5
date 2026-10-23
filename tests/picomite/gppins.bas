' gppins.bas - OPTION PICOMITE's GP pin names (External.c, GPPin). Run with OPTION PICOMITE HDMI set:
'   python -I tools/goldens.py COM4 OUT --corpus tests/picomite --dir A:/p --nomodemap --pre "OPTION PICOMITE HDMI"
' and OPTION PICOMITE NONE after. Each GP is the pin that has its function here (MM.INFO(PINNO)); MM.INFO(MAX GP);
' GP names, unquoted and as strings, in SETPIN, PIN(), PIN() =, PORT, PULSE and MM.INFO(PIN); the GPs with no pin; the
' PicoMite's SETPINs for a port's pins and PWM, which are checked and change nothing; SETPIN pin, IR; FFIN. It drives
' only the pins that are free on Peter's board: 3, 5, 7, 19, 21 and 23 (GP16, GP17, GP6, GP19, GP20 and GP18).
Option Explicit
Dim integer i, n
Dim a$

Print "-- MM.INFO(PINNO GPn), 0 for none"
a$ = ""
For i = 0 To 47
  On Error Skip 1
  n = MM.Info(PINNO "GP" + Str$(i))
  If MM.ErrNo Then n = 0
  a$ = a$ + " " + Str$(n)
  If i Mod 12 = 11 Then Print "GP"; Str$(i - 11); "-"; Str$(i); ":"; a$: a$ = ""
Next
Print "gp13:"; MM.Info(PINNO gp13); ", 18:"; MM.Info(PINNO 18); ", MAX GP:"; MM.Info(MAX GP)
On Error Skip 1
n = MM.Info(PINNO GP23)
Print "GP23: "; MM.ErrMsg$
On Error Skip 1
n = MM.Info(PINNO GP48)
Print "GP48: "; MM.ErrMsg$

Print "-- SETPIN, PIN() and PIN() ="
SetPin GP16, DOut
Pin(GP16) = 1
Print "GP16:"; Pin(GP16); Pin(3); " "; MM.Info$(PIN GP16); ", GP17 "; MM.Info$(PIN GP17); ", GP23 "; MM.Info$(PIN GP23)
Pin(GP16) = 0
Print "GP16:"; Pin("GP16"); Pin(3)
a$ = "gp17"
SetPin a$, DOut
Pin(a$) = 1
Print a$; ":"; Pin(5); Pin(a$)

Print "-- PORT: from a GP name, consecutive GP numbers"
SetPin GP18, DOut
SetPin GP19, DOut
SetPin GP20, DOut
Port(GP18, 3) = &B101
Print "pins 23 19 21:"; Pin(23); Pin(19); Pin(21); ", PORT(GP18, 3) ="; Port(GP18, 3)
Port(GP16, 2, GP18, 3) = &B01010
Print "pins 3 5 23 19 21:"; Pin(3); Pin(5); Pin(23); Pin(19); Pin(21); ", PORT(GP16, 2, GP18, 3) ="; Port(GP16, 2, GP18, 3)
Print "PORT(19, 1, GP20, 1) ="; Port(19, 1, GP20, 1)
On Error Skip 1
n = Port(GP22, 2)
Print "PORT(GP22, 2): "; MM.ErrMsg$

Print "-- PULSE"
SetPin GP6, DOut
Pulse GP6, 0.5
Print "PULSE GP6: "; MM.Info$(PIN 7)
On Error Skip 1
Pulse 99, 1
Print "PULSE 99: "; MM.ErrMsg$
For i = 0 To 5
  SetPin MM.Info(PINNO "GP" + Str$(Choice(i = 0, 6, i + 15))), Off
Next

Print "-- names"
On Error Skip 1
SetPin GP23, DIn
Print "GP23: "; MM.ErrMsg$
On Error Skip 1
SetPin GP48, DIn
Print "GP48: "; MM.ErrMsg$
On Error Skip 1
SetPin "GPX", DIn
Print "GPX: "; MM.ErrMsg$
On Error Skip 1
n = Pin(GP29)
Print "PIN(GP29): "; MM.ErrMsg$
On Error Skip 1
n = Pin("GP12A")
Print "PIN(" + Chr$(34) + "GP12A" + Chr$(34) + "): "; MM.ErrMsg$

Print "-- a port's pins, checked and nothing more"
On Error Skip 1
SetPin GP0, GP1, COM1
r "GP0, GP1, COM1"
On Error Skip 1
SetPin GP1, GP0, COM1
r "GP1, GP0, COM1"
On Error Skip 1
SetPin GP12, GP13, COM1
r "GP12, GP13, COM1"
On Error Skip 1
SetPin 8, 10, COM1
r "8, 10, COM1"
On Error Skip 1
SetPin GP0, GP2, COM1
r "GP0, GP2, COM1"
On Error Skip 1
SetPin GP0, GP0, COM1
r "GP0, GP0, COM1"
On Error Skip 1
SetPin GP36, GP37, COM2
r "GP36, GP37, COM2"
On Error Skip 1
SetPin GP4, GP5, COM2
r "GP4, GP5, COM2"
On Error Skip 1
SetPin GP0, GP1, COM2
r "GP0, GP1, COM2"
On Error Skip 1
SetPin 16, 11, COM2
r "16, 11, COM2"
On Error Skip 1
SetPin GP16, GP17, I2C
r "GP16, GP17, I2C"
On Error Skip 1
SetPin GP14, GP15, I2C2
r "GP14, GP15, I2C2"
On Error Skip 1
SetPin GP16, GP17, I2C2
r "GP16, GP17, I2C2"
On Error Skip 1
SetPin 3, 5, I2C
r "3, 5, I2C"
On Error Skip 1
SetPin GP20, GP19, GP18, SPI
r "GP20, GP19, GP18, SPI"
On Error Skip 1
SetPin GP17, GP19, GP18, SPI
r "GP17, GP19, GP18, SPI"
On Error Skip 1
SetPin GP12, GP11, GP10, SPI2
r "GP12, GP11, GP10, SPI2"
On Error Skip 1
SetPin GP12, GP11, GP11, SPI2
r "GP12, GP11, GP11, SPI2"
On Error Skip 1
SetPin 35, 38, 40, SPI2
r "35, 38, 40, SPI2"
Print "pin 8 after them: "; MM.Info$(PIN 8)
Open "COM1:9600" As #5
Print "COM1 opened: "; MM.Info$(PIN 8)
Close #5

Print "-- PWM, IR and FFIN"
On Error Skip 1
SetPin GP2, PWM
r "GP2, PWM"
On Error Skip 1
SetPin GP3, PWM1B
r "GP3, PWM1B"
On Error Skip 1
SetPin GP5, pwm2b
r "GP5, pwm2b"
On Error Skip 1
SetPin 36, PWM2A
r "36, PWM2A"
On Error Skip 1
SetPin GP3, PWM1A
r "GP3, PWM1A"
On Error Skip 1
SetPin GP18, PWM
r "GP18, PWM"
On Error Skip 1
SetPin GP4, PWM0A
r "GP4, PWM0A"
Print "pin 12 after them: "; MM.Info$(PIN 12)
On Error Skip 1
SetPin GP5, IR
r "GP5, IR"
On Error Skip 1
SetPin GP1, FFIN
r "GP1, FFIN"
Print "end"

Sub r(s$)
  If MM.ErrNo Then Print s$; ": "; MM.ErrMsg$ Else Print s$; ": ok"
End Sub
