' wifispi.bas - Peter's RM2 (CYW43439) Wi-Fi module through SPI2 in half-duplex, as the firmware is to drive it
' (docs/Web_Plan.md, step 1), with no firmware change (15 Nov 2026). SPI2 OPEN gives the pins (PB13 clock, PB15 data:
' AF5), the 48 MHz kernel clock (PLL1Q) and AFCNTR (the pins kept while the SPI is off); POKEs then put it in
' half-duplex (CFG2.COMM = 3) and turn the line round with CR1.HDDIR. A transaction: CS (pin 33, PI3) low, the
' command and any data sent as 32-bit frames, then TSIZE frames read, CS high. WL_ON is pin 32 (PI8).
' For each of the chip's two modes, each pin drive speed and each clock from 0.75 to 24 MHz: the radio powered up
' afresh and set up at 0.75 MHz, then N random patterns written to the R/W test register and read back, the read-only
' test register read N times, and the chip ID read through the backplane. In normal mode the chip answers on the
' falling edge (read in SPI mode 0); in high-speed mode on the rising edge, read in SPI mode 0 (sampled on the edge
' that changes it, the chip's output delay holding the bit, as pico-sdk's gap01_sample0 does) and in mode 1.
Option Explicit
Option Default Integer
Const CLK = 40, DAT = 38, CS = 33, WLON = 32
Const S2 = &H40003800                   ' SPI2
Const rCR1 = S2, rCR2 = S2 + 4, rCFG1 = S2 + 8, rCFG2 = S2 + &H0C, rSR = S2 + &H14, rIFCR = S2 + &H18
Const rTXDR = S2 + &H20, rRXDR = S2 + &H30
Const rOSPEEDB = &H58020408             ' GPIOB OSPEEDR: PB13 bits 27-26, PB15 bits 31-30
Const N = 200
Dim cr1base, cfg2base, rxcpha, timeouts, w(3), v, i, m, hs, gs, bad, x, total
Dim string chipm(2) = ("normal, mode 0    ", "high-speed, mode 0", "high-speed, mode 1"), drive(1) = ("low      ", "very high")

Print "RM2 (CYW43439) through SPI2 in half-duplex"
If Init(0, 0) Then
  Print "SPI2 CR1 &H"; Hex$(Peek(Word rCR1), 8); "  CFG1 &H"; Hex$(Peek(Word rCFG1), 8); "  CFG2 &H"; Hex$(Peek(Word rCFG2), 8)
Else
  Print "The module does not answer through SPI2"
  Done
EndIf
Print "Chip mode, read in  Pin drive  Clock      Errors (of"; 3 * N; ")  Chip ID"
For hs = 0 To 2
  For gs = 0 To 1
    For m = 5 To 0 Step -1
      Print chipm(hs); "   "; drive(gs); "  ";
      Print Format$(48 / 2 ^ (m + 1), "%5.2f"); " MHz  ";
      If Init(hs, gs) Then
        SetMBR m
        bad = 0
        For i = 1 To N
          x = Int(Rnd * 4294967296)
          HWr Cmd(1, 0, &H18, 4), x
          If HRd(Cmd(0, 0, &H18, 4), 0) <> x Then Inc bad
          If HRd(Cmd(0, 0, &H14, 4), 0) <> &HFEEDBEAD Then Inc bad
        Next
        v = HRd(Cmd(0, 1, &H8000, 4), 1)  ' ChipCommon's ID: the window is 0x18000000, one word of response delay
        Print Format$(bad, "%5g"); "             &H"; Hex$(v, 8);
        If (v And &HFFFF) <> 43439 Then
          Print "  (not 43439)";
          Inc bad
        EndIf
        Print
        Inc total, bad
      Else
        Print "set-up failed"
        Inc total
      EndIf
    Next
  Next
Next
Print "Timeouts:"; timeouts
If total Then Print total; " error(s)" Else Print "No errors"
Done

Sub Done
  On Error Skip : SPI2 Close
  SetPin CS, OFF
  Pin(WLON) = 0
  Print "Radio off."
  End
End Sub

' Power the radio up afresh and set SPI2 up at 0.75 MHz; the chip goes to 32-bit words: hsm 0 normal mode, 1 high-speed
' read in SPI mode 0, 2 high-speed read in mode 1.
' Then the backplane gets the ALP clock and its window at ChipCommon. Returns 1 if it answers throughout.
Function Init(hsm, gsp)
  Local k, r
  On Error Skip : SPI2 Close
  SetPin WLON, DOUT : Pin(WLON) = 0
  SetPin CS, DOUT : Pin(CS) = 1
  SetPin CLK, DOUT : Pin(CLK) = 0
  SetPin DAT, DOUT : Pin(DAT) = 0      ' data low while the chip powers up, as the Pico W does
  Pause 20
  Pin(WLON) = 1
  Pause 250
  SetPin CLK, OFF : SetPin DAT, OFF
  SPI2 Open 750000, 0, 32
  cr1base = Peek(Word rCR1) And &HFFFFF5FE              ' without SPE, CSTART and HDDIR
  cfg2base = (Peek(Word rCFG2) And &HFEF9FFFF) Or &H60000 ' COMM = 3 (half-duplex), CPHA = 0
  Poke Word rCR1, cr1base
  Poke Word rCFG2, cfg2base
  If gsp Then
    Poke Word rOSPEEDB, Peek(Word rOSPEEDB) Or &HCC000000
  Else
    Poke Word rOSPEEDB, Peek(Word rOSPEEDB) And &H33FFFFFF
  EndIf
  SetMBR 5
  rxcpha = 0
  For k = 1 To 10
    r = Sw(HRd(Sw(Cmd(0, 0, &H14, 4)), 0))
    If r = &HFEEDBEAD Then Exit For
    Pause 1
  Next
  If r <> &HFEEDBEAD Then
    Print "no test pattern at power-up: &H"; Hex$(r, 8); "  ";
    Exit Function
  EndIf
  HWr Sw(Cmd(1, 0, 0, 4)), Sw(&HA1 Or ((hsm > 0) * &H10)) ' 32-bit, IRQ high, wake-up, [high-speed], no status word
  rxcpha = (hsm = 2)
  HWr Cmd(1, 0, &H1D, 1), 4                            ' F1 response delay: 4 bytes
  r = HRd(Cmd(0, 0, &H14, 4), 0)
  If r <> &HFEEDBEAD Then
    Print "no test pattern in 32-bit mode: &H"; Hex$(r, 8); "  ";
    Exit Function
  EndIf
  HWr Cmd(1, 1, &H1000E, 1), &H08                      ' ask for the ALP clock
  For k = 1 To 50
    If HRd(Cmd(0, 1, &H1000E, 1), 1) And &H40 Then Exit For
    Pause 1
  Next
  If k > 50 Then
    Print "no ALP clock  ";
    Exit Function
  EndIf
  HWr Cmd(1, 1, &H1000E, 1), 0
  HWr Cmd(1, 1, &H1000A, 1), 0                         ' the backplane window: 0x18000000
  HWr Cmd(1, 1, &H1000B, 1), 0
  HWr Cmd(1, 1, &H1000C, 1), &H18
  Init = 1
End Function

' The clock: the kernel clock (48 MHz) / 2^(m+1); CFG1.MBR, bits 30-28, written with the SPI off
Sub SetMBR(m)
  Poke Word rCFG1, (Peek(Word rCFG1) And &H8FFFFFFF) Or (m << 28)
End Sub

' A command word
Function Cmd(wr, fn, addr, sz)
  Cmd = (wr << 31) Or (1 << 30) Or (fn << 28) Or ((addr And &H1FFFF) << 11) Or sz
End Function

' Swap the 16-bit halves: words in the chip's start-up 16-bit mode
Function Sw(x)
  Sw = ((x And &HFFFF) << 16) Or ((x >> 16) And &HFFFF)
End Function

' Send n frames from w() (half-duplex transmit, SPI mode 0); CS is the caller's
Sub HTx(n)
  Local k
  Poke Word rCFG2, cfg2base
  Poke Word rCR1, cr1base Or &H800                     ' HDDIR: transmit
  Poke Word rCR2, n                                    ' TSIZE
  Poke Word rCR1, cr1base Or &H801                     ' SPE
  For k = 0 To n - 1
    Poke Word rTXDR, w(k)
  Next
  Poke Word rCR1, cr1base Or &HA01                     ' CSTART
  WaitEOT
  Poke Word rCR1, cr1base Or &H800
End Sub

' Read n frames into w() (half-duplex receive; SPI mode 1 when the chip is in high-speed mode)
Sub HRx(n)
  Local k, j
  Poke Word rCR1, cr1base                              ' HDDIR: receive
  Poke Word rCFG2, cfg2base Or (rxcpha << 24)          ' CPHA
  Poke Word rCR2, n
  Poke Word rCR1, cr1base Or 1
  Poke Word rCR1, cr1base Or &H201
  For k = 0 To n - 1
    For j = 1 To 1000
      If Peek(Word rSR) And 1 Then Exit For            ' RXP
    Next
    If j > 1000 Then
      Inc timeouts
      w(k) = -1
    Else
      w(k) = Peek(Word rRXDR)
    EndIf
  Next
  WaitEOT
  Poke Word rCR1, cr1base
End Sub

Sub WaitEOT
  Local k
  For k = 1 To 1000
    If Peek(Word rSR) And 8 Then Exit For              ' EOT
  Next
  If k > 1000 Then Inc timeouts
  Poke Word rIFCR, &H18                                ' EOTC, TXTFC
End Sub

' A write: command c then the 32-bit word d
Sub HWr(c, d)
  Pin(CS) = 0
  w(0) = c : w(1) = d
  HTx 2
  Pin(CS) = 1
End Sub

' A read: command c, pad words of response delay, then the 32-bit answer
Function HRd(c, pad)
  Pin(CS) = 0
  w(0) = c
  HTx 1
  HRx pad + 1
  Pin(CS) = 1
  HRd = w(pad)
End Function
