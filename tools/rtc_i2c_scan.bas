' rtc_i2c_scan.bas: every device on the RTC's I2C bus, which a G3 shares with its MS7210 (HDMI).
'
' The bus is SCL on PD5 and SDA on PD4, which the firmware bit-bangs for the DS3231 (Src/Analog_I2C.c).
' BASIC's I2C commands drive the I/O connector's buses, not this one, so this program drives PD4 and PD5
' itself through the GPIO registers, as open-drain outputs at about 10 kHz, and puts them back as it
' found them. It only addresses each device (a write of no data), so nothing on the bus is changed,
' then reads the MS7210's ID and the DS3231's time if they are there.
'
'   RUN "rtc_i2c_scan.bas"
'
' Runs on a G2 (the DS3231 if one is fitted) and a G3. Not on a G1: its PD4 and PD5 are wired otherwise.
OPTION EXPLICIT
OPTION DEFAULT INTEGER

CONST GPIOD = &H58020C00                      ' STM32H743 port D
CONST MODER = GPIOD, OTYPER = GPIOD + &H04, PUPDR = GPIOD + &H0C
CONST IDR = GPIOD + &H10, ODR = GPIOD + &H14, BSRR = GPIOD + &H18
CONST SDA = &H10, SCL = &H20                  ' PD4, PD5
CONST HB = 0.005                              ' half a bit, ms (the statements add more)

IF INSTR(MM.DEVICE$, "Maximite 2 G") = 0 THEN
  PRINT "This is for a G2 or G3 (";MM.DEVICE$;"): a G1's PD4 and PD5 are not the RTC's bus"
  END
ENDIF

DIM moder0 = PEEK(WORD MODER), otyper0 = PEEK(WORD OTYPER)
DIM pupdr0 = PEEK(WORD PUPDR), odr0 = PEEK(WORD ODR)
DIM a, found, n, b(2)
DIM names$(127) LENGTH 60

PinsOpen
PRINT "The RTC's I2C bus (SCL PD5, SDA PD4) on the ";MM.DEVICE$
IF (PEEK(WORD IDR) AND SCL) = 0 THEN
  PRINT "SCL is held low: nothing can be done on this bus"
  PinsBack
  END
ENDIF
IF (PEEK(WORD IDR) AND SDA) = 0 THEN           ' a device left holding SDA (a transfer cut short):
  PRINT "SDA was held low: clocked out"        ' clock it until it lets go
  FOR n = 1 TO 9
    IF PEEK(WORD IDR) AND SDA THEN EXIT FOR
    SclLow : SclHigh
  NEXT
  BusStop
ENDIF

PRINT
PRINT "     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F"
FOR a = 0 TO &H7F
  IF a MOD 16 = 0 THEN PRINT HEX$(a, 2);":";
  IF a < &H08 OR a > &H77 THEN                 ' (reserved addresses: not probed, as i2cdetect)
    PRINT "   ";
  ELSE
    PinsOpen                                   ' (again each time: the firmware's hourly DS3231 read,
    BusStart                                   ' with OPTION DS3231 ON, sets the pins its own way)
    IF I2cPut(a << 1) THEN
      PRINT " ";HEX$(a, 2);
      names$(a) = Named$(a)
      INC found
    ELSE
      PRINT " --";
    ENDIF
    BusStop
  ENDIF
  IF a MOD 16 = 15 THEN PRINT
NEXT

PRINT
IF found = 0 THEN
  PRINT "No device answered"
ELSE
  PRINT found;" device";CHOICE(found > 1, "s", "");" answered (7-bit address, 8-bit form in brackets):"
  FOR a = &H08 TO &H77
    IF names$(a) <> "" THEN
      PRINT "  &H";HEX$(a, 2);" (&H";HEX$(a << 1, 2);"): ";names$(a)
      IF a = &H59 OR a = &H2B THEN             ' the MS7210: its ID registers 0x0000-0x0002
        FOR n = 0 TO 2 : b(n) = Ms7210Read(a, n) : NEXT
        PRINT "      ID ";HEX$(b(0), 2);" ";HEX$(b(1), 2);" ";HEX$(b(2), 2);
        IF b(0) = &HA2 AND b(1) = &H20 AND b(2) = &H0A THEN
          PRINT ": an MS7210"
        ELSE
          PRINT ": not the MS7210's A2 20 0A"
        ENDIF
      ENDIF
      IF a = &H68 THEN                         ' a DS3231: its seconds, minutes and hours (BCD)
        RegRead8 a, 0, 3
        PRINT "      its time ";HEX$(b(2) AND &H3F, 2);":";HEX$(b(1), 2);":";HEX$(b(0), 2);
        PRINT " (TIME$ ";TIME$;")"
      ENDIF
    ENDIF
  NEXT
ENDIF
PinsBack
END

' what is usually found at an address
FUNCTION Named$(a)
  SELECT CASE a
    CASE &H59 : Named$ = "MS7210 HDMI transmitter (SA floating or high)"
    CASE &H2B : Named$ = "MS7210 HDMI transmitter (SA grounded)"
    CASE &H68 : Named$ = "DS3231 real time clock (or another RTC at &H68)"
    CASE &H57 : Named$ = "EEPROM, as on DS3231 modules (AT24C32)"
    CASE &H50 TO &H56 : Named$ = "EEPROM?"
    CASE ELSE : Named$ = "unknown"
  END SELECT
END FUNCTION

' PD4 and PD5 as open-drain outputs, both released (the pull-ups take them high)
SUB PinsOpen
  POKE WORD BSRR, SDA OR SCL
  POKE WORD OTYPER, PEEK(WORD OTYPER) OR SDA OR SCL
  POKE WORD PUPDR, (PEEK(WORD PUPDR) AND &HFFFFF0FF) OR &H500   ' pull-ups on
  POKE WORD MODER, (PEEK(WORD MODER) AND &HFFFFF0FF) OR &H500   ' outputs
END SUB

' PD4 and PD5 as they were found
SUB PinsBack
  LOCAL s = 0
  IF odr0 AND SDA THEN s = s OR SDA ELSE s = s OR (SDA << 16)
  IF odr0 AND SCL THEN s = s OR SCL ELSE s = s OR (SCL << 16)
  POKE WORD BSRR, s
  POKE WORD MODER, (PEEK(WORD MODER) AND &HFFFFF0FF) OR (moder0 AND &HF00)
  POKE WORD PUPDR, (PEEK(WORD PUPDR) AND &HFFFFF0FF) OR (pupdr0 AND &HF00)
  POKE WORD OTYPER, (PEEK(WORD OTYPER) AND &HFFFFFFCF) OR (otyper0 AND &H30)
END SUB

' the lines: SCL released waits for it to rise (a device may hold it low: up to about 1000 reads)
SUB SclHigh
  LOCAL t = 0
  POKE WORD BSRR, SCL
  DO WHILE (PEEK(WORD IDR) AND SCL) = 0 AND t < 1000 : INC t : LOOP
  PAUSE HB
END SUB
SUB SclLow
  POKE WORD BSRR, SCL << 16
  PAUSE HB
END SUB
SUB SdaOut(v)                                  ' 1 releases SDA, 0 pulls it low
  IF v THEN POKE WORD BSRR, SDA ELSE POKE WORD BSRR, SDA << 16
  PAUSE HB
END SUB
SUB BusStart                                   ' SDA falls while SCL is high
  SdaOut 1 : SclHigh : SdaOut 0 : SclLow
END SUB
SUB BusStop                                    ' SDA rises while SCL is high
  SdaOut 0 : SclHigh : SdaOut 1
END SUB

' a byte out, most significant bit first; 1 if a device acknowledged it
FUNCTION I2cPut(v)
  LOCAL i
  FOR i = 7 TO 0 STEP -1
    SdaOut ((v >> i) AND 1)
    SclHigh : SclLow
  NEXT
  SdaOut 1                                     ' released for the acknowledge
  SclHigh
  I2cPut = ((PEEK(WORD IDR) AND SDA) = 0)
  SclLow
END FUNCTION

' a byte in; ack 1 asks for another, 0 ends the read
FUNCTION I2cGet(ack)
  LOCAL i, v = 0
  SdaOut 1
  FOR i = 1 TO 8
    SclHigh
    v = (v << 1) OR ((PEEK(WORD IDR) AND SDA) <> 0)
    SclLow
  NEXT
  SdaOut (ack = 0)
  SclHigh : SclLow
  SdaOut 1
  I2cGet = v
END FUNCTION

' one of the MS7210's registers: a 16-bit number, low byte first; -1 if it did not answer
FUNCTION Ms7210Read(a, r)
  LOCAL ok
  PinsOpen
  BusStart
  ok = I2cPut(a << 1)
  IF ok THEN ok = I2cPut(r AND &HFF)
  IF ok THEN ok = I2cPut(r >> 8)
  IF ok THEN BusStart : ok = I2cPut((a << 1) OR 1)   ' (a repeated start)
  IF ok THEN Ms7210Read = I2cGet(0) ELSE Ms7210Read = -1
  BusStop
END FUNCTION

' n bytes from an 8-bit register number on, into b()
SUB RegRead8(a, r, n)
  LOCAL i, ok
  PinsOpen
  BusStart
  ok = I2cPut(a << 1)
  IF ok THEN ok = I2cPut(r)
  IF ok THEN BusStart : ok = I2cPut((a << 1) OR 1)
  FOR i = 0 TO n - 1
    IF ok THEN b(i) = I2cGet(i < n - 1) ELSE b(i) = 0
  NEXT
  BusStop
END SUB
