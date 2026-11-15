' wifitest.bas - Peter's RM2 (CYW43439) Wi-Fi module on the rear I/O header, tested from BASIC with no firmware
' support (15 Nov 2026). The gSPI bus is bit-banged with PIN(); pins from docs/Web_Plan.md:
'   40 = PB13  gSPI clock (10K pull-up on the module, for the factory-reset check at power-up)
'   38 = PB15  gSPI data, both ways (the chip's data out through 470R, its IRQ through 10K)
'   33 = PI3   gSPI chip select, low selects
'   32 = PI8   WL_ON and BT_ON: the radio's power-up
' Framing as pico-sdk's cyw43 driver (cyw43_spi.c, cyw43_ll.c): a 32-bit command (bit 31 write, 30 increment,
' 29-28 function, 27-11 address, 10-0 length) then the data, each bit sent MSB first on the rising edge; the chip
' answers on the falling edge. In the chip's start-up 16-bit mode each 32-bit word goes as its low half, then its
' high half (Sw below), until the bus control register is set to 32 bits.
Option Explicit
Option Default Integer
Const CLK = 40, DAT = 38, CS = 33, WLON = 32
Const F0 = 0, F1 = 1                    ' function 0 = the gSPI bus's own registers, 1 = the backplane
Dim v, i, fails, w(3)

Print "RM2 (CYW43439) gSPI test, bit-banged on pins 40, 38, 33 and 32"
SetPin WLON, DOUT : Pin(WLON) = 0      ' radio off
SetPin CS, DOUT : Pin(CS) = 1
SetPin CLK, DOUT : Pin(CLK) = 0
SetPin DAT, DOUT : Pin(DAT) = 0        ' data low while the chip powers up, as the Pico W does
Pause 20
Pin(WLON) = 1
Pause 250
SetPin DAT, DIN
Print "Data line with CS high (the IRQ):"; Pin(DAT)

' 1. The read-only test register, 16-bit mode (the driver tries ten times, 1 ms apart)
For i = 1 To 10
  v = Sw(GRd(Sw(Cmd(0, F0, &H14, 4)), 0))
  If v = &HFEEDBEAD Then Exit For
  Pause 1
Next
Check "Test register 0x14, 16-bit mode (try" + Str$(i) + ")", v, &HFEEDBEAD
If v <> &HFEEDBEAD Then
  Print "No answer from the module: stopping with the radio off"
  Pin(WLON) = 0
  End
EndIf

' 2. The read/write test register 0x18, two patterns
GWr Sw(Cmd(1, F0, &H18, 4)), Sw(&H12345678)
Check "R/W test register 0x18 = &H12345678", Sw(GRd(Sw(Cmd(0, F0, &H18, 4)), 0)), &H12345678
GWr Sw(Cmd(1, F0, &H18, 4)), Sw(&HA5C3F00F)
Check "R/W test register 0x18 = &HA5C3F00F", Sw(GRd(Sw(Cmd(0, F0, &H18, 4)), 0)), &HA5C3F00F

' 3. To 32-bit little-endian words at normal speed (the answer still on the falling edge), the IRQ active high,
'    WAKE_UP, 4 bytes of response delay for backplane reads, no status word. Byte 0 = &HA1, byte 1 = 4, byte 2 = 0.
GWr Sw(Cmd(1, F0, 0, 4)), Sw(&H04A1)
Check "Test register 0x14, 32-bit mode", GRd(Cmd(0, F0, &H14, 4), 0), &HFEEDBEAD
v = GRd(Cmd(0, F0, 0, 4), 0)
Check "Bus control 0x0000, byte 0", v And &HFF, &HA1
Print "  bytes 0-3: &H"; Hex$(v, 8); " (byte 1, the old response delay, reads 0 on this core: 0x001D below serves)"
GWr Cmd(1, F0, &H1D, 1), 4               ' F1 response delay (corerev >= 3), 4 bytes as well
Check "F1 response delay 0x001D", GRd(Cmd(0, F0, &H1D, 1), 0) And &HFF, 4
v = GRd(Cmd(0, F0, &H4, 2), 0) And &HFFFF
Print "Interrupt register 0x0004: &H"; Hex$(v, 4)

' 4. The backplane: ask for the ALP clock, then read the chip ID from ChipCommon at 0x18000000
GRdN Cmd(0, F1, &H1000E, 1), 3
Print "Chip clock CSR before, raw words: &H"; Hex$(w(0), 8); " &H"; Hex$(w(1), 8); " &H"; Hex$(w(2), 8)
GWr Cmd(1, F1, &H1000E, 1), &H08         ' SBSDIO_ALP_AVAIL_REQ
For i = 1 To 50
  v = GRd(Cmd(0, F1, &H1000E, 1), 1) And &HFF
  If v And &H40 Then Exit For           ' SBSDIO_ALP_AVAIL
  Pause 1
Next
Check "Chip clock CSR: ALP available (&H40 set)", v And &H40, &H40
Print "  CSR = &H"; Hex$(v, 2); " after"; i; " reads"
GWr Cmd(1, F1, &H1000E, 1), 0
GWr Cmd(1, F1, &H1000A, 1), 0            ' the backplane window: 0x18000000
GWr Cmd(1, F1, &H1000B, 1), 0
GWr Cmd(1, F1, &H1000C, 1), &H18
GRdN Cmd(0, F1, &H8000, 4), 3            ' (0x18000000 And 0x7FFF) Or the 32-bit access flag
Print "ChipCommon ID, raw words: &H"; Hex$(w(0), 8); " &H"; Hex$(w(1), 8); " &H"; Hex$(w(2), 8)
v = w(1)
Check "Chip ID (43439 = &HA9AF)", v And &HFFFF, 43439
Print "  chip revision"; (v >> 16) And 15; ", package"; (v >> 20) And 15; ", cores"; (v >> 24) And 15

' 5. The IRQ on the data line: CS high, polarity high
SetPin DAT, DIN
Print "Data line with CS high (the IRQ):"; Pin(DAT)

Pin(WLON) = 0
Print "Radio off again."
If fails Then Print fails; " check(s) FAILED" Else Print "All checks passed"
End

' A command word
Function Cmd(wr, fn, addr, sz)
  Cmd = (wr << 31) Or (1 << 30) Or (fn << 28) Or ((addr And &H1FFFF) << 11) Or sz
End Function

' Swap the 16-bit halves: words in the chip's start-up 16-bit mode
Function Sw(x)
  Sw = ((x And &HFFFF) << 16) Or ((x >> 16) And &HFFFF)
End Function

' Clock out 32 bits, MSB first; the data pin is an output and the clock low. If keep is set, the clock is left
' high after the last bit, so a read can let go of the line before the chip's first falling edge.
Sub Out32(x, keep)
  Local j
  For j = 31 To 0 Step -1
    Pin(DAT) = (x >> j) And 1
    Pin(CLK) = 1
    If j <> 0 Or keep = 0 Then Pin(CLK) = 0
  Next
End Sub

' Clock in 32 bits: each read before the rising edge, the chip having changed the line on the falling edge
Function In32()
  Local j, r
  For j = 1 To 32
    r = (r << 1) Or Pin(DAT)
    Pin(CLK) = 1
    Pin(CLK) = 0
  Next
  In32 = r
End Function

' A write: command c, then the 32-bit word d
Sub GWr(c, d)
  SetPin DAT, DOUT
  Pin(CS) = 0
  Out32 c, 0
  Out32 d, 0
  Pin(CS) = 1
  SetPin DAT, DIN
End Sub

' A read: command c, pad words skipped (the response delay), then a 32-bit word
Function GRd(c, pad)
  Local j, x
  SetPin DAT, DOUT
  Pin(CS) = 0
  Out32 c, 1
  SetPin DAT, DIN                       ' let go while the clock is high
  Pin(CLK) = 0                          ' the chip puts out its first bit on this edge
  For j = 1 To pad
    x = In32()
  Next
  GRd = In32()
  Pin(CS) = 1
End Function

' A read of n raw words into w(), to show where the answer lands after the response delay
Sub GRdN(c, n)
  Local j
  SetPin DAT, DOUT
  Pin(CS) = 0
  Out32 c, 1
  SetPin DAT, DIN
  Pin(CLK) = 0
  For j = 0 To n - 1
    w(j) = In32()
  Next
  Pin(CS) = 1
End Sub

Sub Check(t$, got, want)
  If got = want Then
    Print "PASS  "; t$; ": &H"; Hex$(got, 8)
  Else
    Print "FAIL  "; t$; ": &H"; Hex$(got, 8); ", wanted &H"; Hex$(want, 8)
    Inc fails
  EndIf
End Sub
