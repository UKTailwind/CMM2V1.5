# PicoMite parity: the audit checked again (22 Oct 2026)

Peter, 22 Oct 2026: "We should though revisit the original analysis to check properly". This is that check of
docs/PicoMite_Parity_Audit_and_Plan.md (14 Oct, against PicoMite 7.00.00RC1), done against both trees as they stood
on 22 Oct. The report itself, below the status, is as it was written.

## Status after the check (22 Oct 2026)

**Peter's decisions:**
- MANDELBROT was an Easter egg in the PicoMite: not needed.
- The order: the small confirmed items and the document corrections first, then OPTION PICOMITE.

**Peter's decisions on the remaining gaps (26 Oct 2026, going through them for the first release candidate;
PicoMite lines at 506500f):**
- To implement:
  - **SAVE IMAGE compresses, losslessly, wherever it can** (gap 13), and **SAVE COMPRESSED IMAGE is another name for
    SAVE IMAGE**. The PicoMite's SAVE COMPRESSED IMAGE writes a 4-bit BMP with RLE4 compression (P
    `misc/FileIO.c:3162`, biCompression 2, an even width) on its 16-colour displays. BMP can compress only its 4- and
    8-bit forms (RLE4 and RLE8), so here that means RLE4 in the 1- and 4-bit modes (n,1 and n,4) and RLE8 in the 8-bit
    modes (n,8), with the 16- and 32-bit modes saved as they are now. LOAD BMP already reads both (C
    `Src/BmpDecoder.c:101-102`).
  - **BLIT COMPRESSED uses BLIT MEMORY's code with bit 15 taken as set** (gap 12; Peter: "it isn't quite a synonym but
    needs to use the same code just assume the bit 15"). BLIT MEMORY decodes the compressed form only when bit 15 of the
    header's width or height is set (C `Src/sprites.c:1189`, as the PicoMite's own BLIT MEMORY, P
    `graphics/Blit.c:307`); the PicoMite's BLIT COMPRESSED always decodes it as compressed (P
    `graphics/Blit.c:625-645`).
  - **BLIT RESIZE** (gap 12, P `graphics/Blit.c:1400`).
  - **AUTOSAVE APPEND** (gap 27, P `misc/FileIO.c:7407`).
  - **BITSTREAM's 4th argument (mode) and its dual-pin form** (gap 4).
  - **DEVICE SERIALRX and DEVICE SERIALTX** (gap 3). SERIALTX can use the cycle-timed transmitter the stepper's
    TMC22xx added (`serialtx`, `Src/stepper_cmm2.c`).
  - **LIST VARIABLES [array], LIST OPTIONS, LIST PINS and LIST COLLISIONS** (gap 16), and **PEEK(BP var), PEEK(WP
    var) and PEEK(SP var)** (gap 17).
  - **MM.USB** (gap 19): the last USB key's code, which the CMM2 keeps in `USBcode` (`Src/KeyboardMap.c:74`).
  - **MM.DISPLAY returns 1, always** (gap 19).
  - **CPU SLEEP s becomes a PAUSE** of the same time when it is met (gap 20; the PicoMite's s is in seconds, P
    `core/MM_Misc.c`, `sleep_us(totalseconds * 1000000)`).
  - **SETPIN pin, ARAW** (gap 24). Peter: an immediate sample with no averaging, and PIN() returns the absolute count.
    The resolution (Peter: "In Picomite mode the adc should be locked to 12-bit and ARAW will then match. In normal
    mode use the maximum adc resolution"):
    - with OPTION PICOMITE, ARAW converts at 12 bits, so it counts 0-4095 as on the PicoMite;
    - without it, ARAW converts at 16 bits (0-65535), the ADC's maximum;
    - the lock is for ARAW only (Peter: "In AIN it returns the voltage so may as well use full resolution"): SETPIN
      pin, AIN [, bits] keeps its 8 to 16 bits, 16 by default, in both modes (`ADCbits[]`,
      `Src/External.c:147-172`).
  - **OPTION DEFAULT COLOURS fc [, bc]**, "yes if easy" (gap 22). It is: `Option.DefaultFC` and `DefaultBC` already
    exist, are saved, and are used by ResetDisplay (`Src/Draw.c:6230`), the console and MODE n,1 (`Src/Mono.c`);
    OPTION CONSOLE BOTH sets them (`Src/MiscSTM32.c:1715-1716`). What is missing is the option, with the PicoMite's 16
    colour names (P `core/MM_Misc.c:6320`), and every MODE (`Src/ltdc.c:284`) and leaving OPTION PICOMITE
    (`Src/PicoMode.c:142`) set them back to white on black.
  - With OPTION PICOMITE (the dialect switch, gap 2), each as the PicoMite has it, and the CMM2's own without it:
    (26 Oct, on doing them: KEYDOWN and GETSCANLINE already act as the PicoMite's, and LOAD PNG's arguments are the
    other way round, the PicoMite's t a colour: docs/PicoMite_Parity_ToDo.md)
    MM.INFO(CPUSPEED) a string; KEYDOWN's shifted F-keys adding &H40 (&H20 here); LOAD PNG's fourth argument the
    alpha cut-off (here the transparent colour's index).
  - **GETSCANLINE rescaled so that 0 is the first visible line, with OPTION PICOMITE** (gap 2; Peter: "getscanline
    should apply only under picomite mode"). Without it, 0 to 525 whatever the mode, above 479 the blanking, as now.
  - **MM.INFO(RESET) gives the brown-out reset too, and MM.INFO(BOOT) is another name for it** (gap 18, behaviour
    item 6), in the CMM2's words (Peter: "mm.info(boot) can follow cmm2"): "Switch", "Power-On", "Software" and now
    "Brown-Out" (`Src/main.c:792-797`), emptied once read as RESET is (`Src/MM_Misc.c:3392`). The PicoMite's BOOT words
    ("Reset Switch", "Power On", "Brown Out", "Restart", the watchdogs, "Firmware update", "Debug") are not used.
  - **OPTION KEYBOARD nn [, capslock] [, numlock] [, repeatstart] [, repeatrate]** as the PicoMite has it (Peter:
    "change to picomite standard and allow the same additional parameters"; the CMM2's is OPTION USBKEYBOARD) (gap
    22).
  - **MM.INFO$(PLATFORM) returns "CMM2"** (gap 22; on the PicoMite OPTION PLATFORM name$ sets it).
  - **OPTION F1 and OPTION F5 to F9** as the PicoMite's: a key's string, kept with the options (the CMM2 has F11,
    F12, F15, F16, F19 and F20, `Src/MiscSTM32.c:324`) (gap 22).
  - **OPTION LOGGING ON | OFF** as the PicoMite's: every error message appended to A:/log.txt; cleared when a program
    starts, as OPTION EXPLICIT is, and not saved (gap 22).
  - **OPTION AUTORUN n [, NORESET]** as the PicoMite's: slot n's program run at power-up; NORESET keeps AUTORUN after
    a system error (here ON and OFF only, `Src/MM_Misc.c:2635`) (gap 22).
  - **OPTION SCREEN KEYBOARD** accepted and ignored (gap 25).
- Deferred, nice to have: ONESHOT (gap 14); ADC RUN (gap 24).
- Decision deferred:
  - **WATCHDOG HW timeout | OFF** (gap 20), "a definite nice to have", but Peter: "defer decision on HW watchdog
    if it can't be cancelled", and the IWDG cannot be. The current status, reviewed:
    - The CMM2 has the software watchdog only: WATCHDOG n | OFF counts `WDTimer` down in the 1 ms timer and does a
      SoftReset with WATCHDOG_TIMEOUT (`Src/MM_Misc.c:3467`, `Src/Timers.c:302-308`); the next start prints "Watchdog
      timeout" and sets MM.WATCHDOG (`Src/main.c:836`). WATCHDOG HW gives an error, and the STM32's IWDG is not used.
    - On the PicoMite, WATCHDOG HW n (1 to 8331 ms) starts the RP2's watchdog (P `core/MM_Misc.c:9740-9752`,
      `watchdog_enable(n, 1)`, paused under a debugger). Nothing feeds it but the program giving the command again. It
      keeps running after the program ends, so a program that stops with an error restarts the board (and so the
      program, with OPTION AUTORUN). WATCHDOG HW OFF turns it off, and a restart it caused is reported as a watchdog
      timeout.
    - On the CMM2 it would be IWDG1 on the 32 kHz LSI: a prescaler of 128 gives 4 ms steps up to 16.4 s, so 1 to
      8331 ms fits with room to spare (LSI is only roughly 32 kHz, so the time is approximate). The restart is told
      from RCC->RSR's IWDG1RSTF. Once started the IWDG cannot be stopped, so HW OFF has to be emulated by feeding it
      from the 1 ms timer interrupt from then on; a reset stops it (to be checked on the board). DBGMCU can freeze
      it under the debugger, as the PicoMite does. Size: small.
- Deferred: TRIANGLE SAVE / TRIANGLE RESTORE (gap 11); BLIT MEMORY332 (gap 12); the PicoMite's FRAMEBUFFER forms in
  the 8, 16 and 32-bit modes, its MODE 4 and 5 (gap 1).
- Out of scope:
  - OPTION CONTINUATION LINES and the joining of lines that end " _" (gap 10). They were made because the PicoMite's
    editor could not handle long lines, and it now can. They stay in the PicoMite for users who are used to them.
    Programs stored on disk on the PicoMite do not include them.
  - SAVE PERSISTENT, MM.PERSISTENT and MM.INFO(PERSISTENT) (gap 13).
  - MANDELBROT (gap 8), as decided on 22 Oct.
  - COPY A2A, A2B, B2A and B2B (gap 21): they belong to the PicoMite's LittleFS drive.
  - I2C SLAVE (gap 15).
  - MM.INFO(BOOT COUNT) (gap 18). (MM.INFO(BOOT) is to be MM.INFO(RESET), above.)
  - OPTION DISPLAY rows, cols (gap 22): on the PicoMite's VGA and HDMI versions it cannot be changed either.
  - OPTION CDC POLL RATE (gap 22): the console here is a CH340, not USB CDC.
  - OPTION PROFILING ON, SAMPLE, the PicoMite's run-time profiler (gap 22): left accepted in a program and doing
    nothing, as since 22 Oct.
  - CALC and MM.ANSWER (gap 23).
  - OPTION GUI CONTROLS (gap 22): not needed, the CMM2 has a fixed allocation.
  - PWM SYNC (gap 24): not possible with the STM32's hardware.
  - KEYBOARD ON|OFF, the on-screen keyboard (gap 25).
  - MOUSE OPEN with its own pins (gap 26).
- Awaiting a decision:
  - **OPTION RTC AUTO ENABLE | DISABLE** (gap 22). Peter: it is not OPTION DS3231: "That says to read the RTC to
    answer time$ function. RTC AUTO turns on and off the automatic update of the internal clock from the DS3231.
    Review further existing code and recommend approach". The review:
    - On the PicoMite (help: "auto-load time$ & date$ from RTC on boot & every 24 hours") its software clock is set
      from the RTC at start-up and every 24 hours (P `PicoMite.c:1166`); TIME$ reads the software clock. If the RTC
      does not answer at start-up the option is turned off ("RTC not found, OPTION RTC AUTO disabled").
    - Here the G2's own clock is the STM32's RTC on its 32.768 kHz crystal and the battery, so it keeps the time
      through power-off itself; the DS3231 is the more accurate one. OPTION DS3231 ON (`Src/MiscSTM32.c:1565`, a
      restart) makes TIME$ and DATE$ read the DS3231 over I2C at every call (`Src/MM_Misc.c:2213`, `:2362`; with
      OPTION MILLISECONDS ON TIME$ reads the internal clock), TIME$ = and DATE$ = set both, and `copy_clock` sets the
      internal clock from the DS3231 at start-up (`Src/main.c:961`), at every PrepareProgram, which is every RUN and
      every line typed at the prompt (`Src/MMBasic.c:733`), and from the 1 ms timer when `mSecTimer % 360000000 ==
      2000` (`Src/Timers.c:168`): every 100 hours, perhaps meant as hourly (3600000).
    - Recommended: OPTION RTC AUTO ENABLE | DISABLE as its own saved option, G2 only, with the PicoMite's meaning:
      the internal clock set from the DS3231 at start-up and every 24 hours, and TIME$, DATE$ and the rest reading the
      internal clock (fast, and with its milliseconds). TIME$ = and DATE$ = set both clocks, as with DS3231 ON (on
      the PicoMite they set only its software clock until RTC SETTIME). A DS3231 that does not answer at start-up
      turns it off with the PicoMite's message. OPTION DS3231 ON stays as it is for the CMM2's programs; the two are
      alternatives, so setting one turns the other off. No restart needed for RTC AUTO. The 100-hour timer copy
      becomes the 24-hour one when RTC AUTO is on, and is looked at for DS3231 ON.
  - Not yet decided: RUN f$ with a variable or an expression (gap 2); the MM.INFO keys LINE, AUTORUN at the top level,
    CALLTABLE, MAX VARS, SYSTEM HEAP and OPTION AUDIO | WIDTH | HEIGHT | KEYBOARD (gap 18).

**Behaviour fixes done on 26 Oct 2026** (Peter: "Do the behavious fixes while I consider the rest"; table 5.1 and
behaviour items 5-6, numbered as in the list given to him that day). tests/cmm2/behfix.bas checks 7-10, 12, 16, 17, 22
and 24b; the others were checked at the prompt on the G2 where they can be:
- 6. The Switch Pro Controller is gamepad type 135 (the PicoMite's SWITCHPRO) in DEVICE(GAMEPAD n, T) as in
  MM.INFO(USB n). 133 and 134 are kept for the PicoMite's USB touch screens (USB_TOUCH, USB_TOUCHMOUSE in
  `Inc/USBGamepad.h`), which `IsGamepadType` leaves out of the gamepad paths; the CMM2's own X-input pad type is 200,
  never given out (reported as 131, XBOX).
- 7. Out of heap: "Not enough memory for n bytes" (the PicoMite's error 48) in place of "Not enough heap memory".
- 8. RESTORE of a bare name that is a label is the label, even when a plain variable has the name too (`LabelFind`,
  split from `findlabel`, as the PicoMite's).
- 9. SAVE IMAGE, SAVE PNG and Print Screen in MODE n,4 save the colours shown: MAP n = c is staged and MAP SET shows
  it (`SetMapRGB`, `CommitMapRGB`, `Rgb121Shown` in `Src/Rgb121.c`, the PicoMite's MapRGB). ReadBuffer4 still gives
  the colour before MAP, which BLIT READ and PIXEL( need.
- 10. MATH(C_ADD ...) to C_POLAR and the four FFTs with one argument say "Argument count" (the PicoMite's c1a5645).
- 11. A line that a #DEFINE makes too long: the file reader has said "Line n is too long" since 22 Oct (its check
  after the substitutions). The loaders' own check of a line now says "Line is too long", not "Not enough memory".
  Found while testing: the reader counts the CMM2's line marker ('|n), so the longest line it takes is 251 characters
  (248 from line 1000 on), where the PicoMite takes 254. Not changed.
- 12. FLASH LOAD IMAGE and FLASH DISK LOAD into a RAM slot let the slot's old contents go only once the new are read
  and decoded, so a file that fails leaves the slot as it was.
- 13. FLASH LOAD n or RAM LOAD n followed by LIST on the same line lists the new program's names
  (`ProgramImageToFlash` points the symbol table at it, as the PicoMite's FLASH LOAD does).
- 14. EDIT and FM turn IR off (`IrClose`) before they clear the variables it writes to.
- 15. A RAM library ends with the program: END, or running off the last line, after MM.END (`RamLibEnd`; the index
  of names is let go and made again at the prompt, `NameIndexFree`). The manual's LIBRARY LOAD says so.
- 16. "#COMMENT  END" with any number of spaces ends the block (`IsCommentEnd`, the PicoMite's).
- 17. A global is counted only once it is made (`VarCounted`, the PicoMite's), so a DIM whose memory ran out leaves
  MM.INFO(VARCNT) as it was. ERASE already lowered it.
- 18. EDIT: the keys queued during a redraw are dropped only when every one is the key that caused it, a held key
  repeating (`EdDropHeldRepeats`, the PicoMite's), so a held arrow stops when let go and typing and pastes are kept;
  an import cut short by memory says how many characters went in; an empty text is saved as an empty file.
- 19. A USB device whose endpoint finds no free entry is reported as "[USB EP_FULL n @a]" (`tuh_dwc2_ep_full_cb`,
  `Src/USBKeyboard.c`; the TinyUSB change is in its README).
- 20. A program that cannot be written whole (a line too long, out of memory) leaves no program, not part of one
  (`ProgramEmpty`): reached under OPTION RAM, as OPTION FLASH tokenises to the heap first.
- 21. MATH(C_DIV) divides in double and rounds, as the PicoMite does.
- 22. LONGSTRING MID with a count near 2^31 is cut to the string (`nbr > src[0] - start + 1`, the PicoMite's).
- 23. READ with no program in memory goes on to the library's DATA (the PicoMite's DataFallToLibrary).
- 24. A program's font left selected when the font table is made again: the default font (PrepareProgram). OPTION
  MILLISECONDS: TIME$'s milliseconds start at 0 when the time is set (checked: below 100 just after).
- A LIBRARY LOAD may come before OPTION SYMBOLS OFF at the top of a program (SymFindOff steps over it, as the
  PicoMite's; LIBRARY LOAD's own test already stepped over OPTION SYMBOLS OFF).
Already fixed before 26 Oct: mounting a drive closes file #10.

**Done on 22 Oct, the day of the check:**
- Gap 3 (DEVICE prefix) and gap 5 (the Wii spellings):
  - DEVICE WII [CLASSIC], DEVICE NUNCHUCK, MOUSE, GAMEPAD, HUMID, WS2812 and BITSTREAM;
  - WII OPEN/CLOSE and WII NUNCHUCK OPEN/CLOSE (the Wii connector, channel 3);
  - DEVICE(WII x), DEVICE(CLASSIC x) and DEVICE(NUNCHUCK x);
  - DEVICE KEYPAD, LCD, CAMERA, SERIALRX and SERIALTX say they are not available.
- Gap 7: MM.INFO$(LCDPANEL) returns "MODE" and the mode number.
- Gap 9: OPTION NOCHECK ON|OFF is accepted and does nothing. The checks it turns off on the PicoMite run here only
  when something is waiting, and Ctrl-C keeps working.
- Behaviour item 1: OPTION PROFILING inside a program is accepted and does nothing. It used to save the option and
  restart the board. At the prompt it is still the CMM2's line profiler.
- Behaviour item 2: REDIM PRESERVE with a bound that uses the array itself (the PicoMite's #105).
- Behaviour item 3: MATH RANDOMIZE seeds as the PicoMite does (69069), so a seed gives the PicoMite's numbers.
- Behaviour item 4: the BMP decoder calls routinechecks for each row, so sound and USB go on while an image decodes.
- The porting chapter's statement that the flash slots 1-3 are not provided, and the release notes' KEYPAD line.

**OPTION PICOMITE, its display (22 Oct, the next commit):** MODE, RESOLUTION, a program starting in MODE 1, the mode
kept at the prompt, HDMI's colours, MM.INFO$(LCDPANEL) and POKE DISPLAY VRES (gap 6, HRES refused): see
docs/PicoMite_CMM2_Mode_Map.md section 0.

**OPTION PICOMITE, its pin names (23 Oct, the commit after):** Peter chose a map by function ("much better would be
to try and match functionality"; then "gp1 - com1rx. Then put com2RX/TX on GP36,37 to use the extra pins so GP21
becomes pin 33"). This settles decision 6 of the audit's 7.2 and the GPn item of gap 2:
- all 28 connector pins have a GP name (the table: the audit's section 6.4 and the manual's PicoMite Pin Names);
- GP names, unquoted and as strings, wherever a pin is read (`Src/External.c`, GetPinArg), and PORT's consecutive
  GP numbers;
- MM.INFO(PINNO GPn) and MM.INFO(MAX GP);
- the PicoMite's SETPINs for COM1, COM2, I2C, I2C2, SPI and SPI2 are checked as the PicoMite checks them and change
  nothing (the ports keep their fixed pins); SETPIN pin, PWM on GP2-5; SETPIN pin, IR; FFIN refused (GP1 is COM1);
- plain numbers stay the CMM2's own pins.

**Still with OPTION PICOMITE:**
- Gap 1: the PicoMite's FRAMEBUFFER in the other depths. With the switch, a program with no MODE runs in MODE n,1,
  which has it already.
- Gap 2 and its dialect items.

**Open, medium:**
- BITSTREAM's mode argument and its two-pin form (gap 4).
- DEVICE SERIALRX and SERIALTX.
- ONESHOT (gap 14).
- I2C SLAVE (gap 15).

**Open, small:** gaps 16-20 and 22-26, and behaviour items 5-6 and the rest of table 5.1 (gaps 10-13, 21 and 27: see
Peter's decisions of 26 Oct above).

**Not changed:** the 7.00.00b2 release notes' "Not there yet" (RAY, the file manager's mouse). That was true of b2.
The next beta's notes cover them.

---

Read-only re-check. CMM2: `D:\Dropbox\VSCode-STM32\CMM2V1.5`, the working tree as it is on disk (HEAD a0ce2c2 plus the uncommitted changes git status lists). PicoMite: `D:\Dropbox\PicoMite\PicoMite` at **3c1cf0d** (RC4 release notes). The original audit is `docs/PicoMite_Parity_Audit_and_Plan.md` (14 Oct, against 7.00.00RC1).

Method. Both keyword tables were parsed with their `#if` context (`AllCommands.h`, `Inc/allcommands.h`). Every C function in both trees was scanned (PicoMite core/ graphics/ io/ input/ misc/ net/ bluetooth/ PicoMite.c: 2,614 functions; CMM2 Src/: 2,668). For every keyword in both tables, the `checkstring`/`checkword`/`checkname` literals were collected from the handler and from the helpers it calls, four levels down (other table handlers and the expression parser not followed). The literal sets were diffed, and each PicoMite-only item was then checked by hand in the CMM2 source. The CMM2 tokeniser's folds and aliases were applied first (`Src/MMBasic.c:1816-1861` SubstFold/FoldFrom; `:1938-1944` the `~(` MM.* folds, MM.INFO$ and TOUCH(; `:2059-2080` COLOR, WII, PIXEL FAST, ELSE IF, END IF, EXIT DO, CAT). Programs that use each gap were counted in a 315-file corpus: PicoMite `Bas/` and `Testfiles/`, and CMM2 `games/`. Nothing was built or run.

Abbreviations: **P** = PicoMite path, **C** = CMM2 path.

---

## The report: summary

**Counts.** These cover the PicoMite-only items: command and token table entries, MM.* variables, sub-keywords inside shared handlers (MM.INFO keys and OPTION names counted one by one), and argument forms found by reading the code.

| Class | Items | Notes |
|---|---|---|
| (a) PRESENT now | 41 | the aliases, folds and ports listed in step 3, each with a C file:line |
| (b) NOT APPLICABLE | about 180 | PIO and its 23 assembler words; about 60 WiFi/web/TLS/MQTT, Bluetooth and PicoCalc options and MM.INFO keys; LCD panels, touch and backlight (19 TOUCH() gestures alone); VS1053 PLAY forms (10); RP2 pins, clocks, PWM slices, PSRAM and boards; the stepper |
| (c) DOCUMENTED as not planned or not supported | 19 | KEYPAD, LCD, I2CLCD, RTC, EDIT FILE, FRAME and FRAME(, YMODEM, OPTION DISK SAVE/LOAD, RESOLUTION and OPTION RESOLUTION, GPn pin names and the pin-assigning SETPIN/OPTION forms, MM.INFO(PINNO)/(MAX GP), program LOAD/SAVE, RUN with a variable |
| (d) GAP | 71 | 27 ranked entries below (each MM.INFO key, OPTION name and sub-keyword counted once; the dialect switch counted once) |

The counts depend on how items are grouped. Each table in steps 1-2 names its items.

**Gaps, ranked by how likely a PicoMite BASIC program is to hit them.** The corpus evidence is given where it exists.

1. **The PicoMite's FRAMEBUFFER forms (LAYER, MERGE, COPY, CREATE 2, WAIT, WRITE L/T/2) outside MODE n,4 and n,1.** In 8, 12, 16 and 32-bit modes `cmd_framebuffer` is the CMM2's own (C `Src/Draw.c:926-930` hands only VideoColour<8 to `Rgb121Framebuffer`). The PicoMite has framebuffers and layers in all five modes (P `graphics/FrameBuffer.c:1541-1727`, SCREENMODE1-5). So PicoMite MODE 4 (RGB555, = CMM2 MODE 7,16) and MODE 5 (256 colours, = 7,8) programs fail with "Syntax". So does any program that uses them without setting a MODE, since it then runs in the CMM2's default depth. The manual documents the current behaviour (`docs/manual/text/12_video_modes.md:120`), and the audit's P3 lists it as "Not started: ... LAYER and MERGE" (audit line 377). Size: large. 92 corpus files use FRAMEBUFFER LAYER/MERGE/COPY/CREATE 2/WAIT:
   - 61 set MODE 2 or MODE 1,2 (which map to n,4 and work);
   - 6 are CMM2 game ports already converted;
   - 1 sets MODE 5 (`Bas/3ddemo.bas`);
   - **24 set no MODE at all** (e.g. `Bas/bubble.bas`, and parts of Elite's split source).
2. **The dialect switch, `OPTION PICOMITE` (planned, not built).** Nothing in the CMM2 sources mentions it. These items wait on it: MODE numbers, GPn pin names (16 corpus files), `MM.INFO(CPUSPEED)` (a number here, a string on the PicoMite), `RUN f$` with a variable (C `Src/Commands.c:1342` runs only a quoted name; documented in `docs/manual/ref/commands.txt:2404`), KEYDOWN's shifted F-keys, GETSCANLINE scaling, MM.DEVICE$, the PWM/SERVO/ADC dialects (below), and BLIT/LOAD PNG argument meanings. The manual's porting chapter (`22_porting.md:9-12`) documents the manual workarounds. Size: large.
3. **The `DEVICE` command prefix** (P `misc/External.c:5166-5310`). It covers `DEVICE WS2812|KEYPAD|LCD|CAMERA|WII [CLASSIC]|NUNCHUCK|MOUSE|GAMEPAD|HUMID|BITSTREAM` and two new commands, `DEVICE SERIALRX` and `DEVICE SERIALTX` (bit-banged serial, P `:5244`, `:5286`). Used by PETSCII Robots, which is in the CMM2 tree: `games/petrobot/robots.bas:2244 Device Wii Open`. Also by `Testfiles/Bitstream_Test.bas:31`, `IR_Carrier_Test.bas:63` and `PS2_ScanCode_Test.bas:100`. Size: small (the prefix), medium (SERIALRX/TX).
4. **BITSTREAM's 4th argument (mode) and its dual-pin form.** P `misc/External.c:4959-4976` takes up to 17 arguments; C `Src/External.c:1472-1473` refuses anything but 3 arguments ("Argument count"). Used by Testfiles NEC_IR_Test:128 ("Syntax" in the 21 Oct baseline), and through DEVICE BITSTREAM by IR_Carrier_Test:63 ("Unknown command") and PS2_ScanCode_Test:100 (that run stopped earlier, at a pin number). Size: small-medium.
5. **The WII spellings: `WII OPEN`, `WII NUNCHUCK OPEN`, `DEVICE(WII …)`, `DEVICE(NUNCHUCK …)`, `DEVICE(CLASSIC …)`.** C `Src/MiscSTM32.c:1201-1218` knows only NUNCHUK, MOUSE and CLASSIC after WII→CONTROLLER, and `fun_device_fn` has no WII. The P forms are at `io/I2C.c:1905`/`:1976` and `misc/External.c:4263-4299`. Used by `games/petrobot/robots.bas:2214-2244`. Size: small.
6. **`POKE DISPLAY HRES n` / `POKE DISPLAY VRES n`** (P `core/MM_Misc.c:9846-9856`). They set the drawing limits on every PicoMite build, VGA included. Used by Elite (`Bas/elite/elite.bas:389`, `:399`). C `cmd_poke` has no DISPLAY. Size: small.
7. **`MM.INFO$(LCDPANEL)` on a VGA/HDMI build**: the display's name, e.g. containing "MODE" (P `core/MM_Misc.c:8866-8872`). The chess programs test it (`Bas/chess/chessiter.bas:8`, `:1032`). Size: small.
8. **MANDELBROT** (P `graphics/DrawFill.c:1100-1174`, PICOMITEVGA or rp2350). Testfiles `MandelbrotTest` fails with "Unknown command". Nothing says it is not planned. Size: medium.
9. **`OPTION NOCHECK ON|OFF`** (P `core/MM_Misc.c:5326`; its effect is at `core/MMBasic.c:2739`). Performance programs use it (`Bas/bench/phase1/bench_micro2.bas`). The audit (6.6) wanted it at least accepted silently; it is "Syntax" today. Size: small.
10. **`OPTION CONTINUATION LINES ENABLE|DISABLE` and the joining of source lines that end " _"** (P `core/MM_Misc.c:7460`, `PicoMite.c:4951+`, `core/Symbols.c:909`; the editor and LIST write such lines). The CMM2 loader does not join them, and C `Src/Editor.c:980` has the test disabled with `&& 0`. No corpus file has them. Size: small-medium.
11. **TRIANGLE SAVE / TRIANGLE RESTORE** (P `graphics/Draw.c:3633-3645`, `:1789`, `:1804`). Size: small.
12. **BLIT RESIZE** (P `graphics/Blit.c:1363`), **BLIT COMPRESSED** (`:625`, the explicit form; C's BLIT MEMORY does handle compressed data, `Src/sprites.c:1177-1191`) and **BLIT MEMORY332** (`:648`, for an RGB332 display: CMM2 n,8). Size: small each.
13. **SAVE COMPRESSED IMAGE** (P `misc/FileIO.c:3110`, 16-colour displays: CMM2 n,4). **SAVE PERSISTENT n / MM.PERSISTENT / MM.INFO(PERSISTENT)** (P `misc/FileIO.c:3085`, `core/Functions.c:763`, `core/MM_Misc.c:9275`). Size: small.
14. **ONESHOT [R,] trig, edge, out, pre, width [, quiet] / ONESHOT DISABLE**: a hardware-timed monostable (P `misc/External.c:2522-2612`). Used by `Bas/bench/inttests/oneshot.bas`. Size: medium (EXTI and a timer).
15. **I2C / I2C2 SLAVE OPEN|CLOSE|READ|WRITE** (P `io/I2C.c:328-334`, `:367-373`). The audit calls it a "P2 decision (the H7 can do it)"; no decision is recorded. Size: medium.
16. **LIST VARIABLES [array], LIST OPTIONS, LIST PINS, LIST COLLISIONS** (P `core/Commands.c:2610-2909`). Size: small.
17. **PEEK(BP var) / PEEK(WP var) / PEEK(SP var)**: a byte, word or short read with the pointer variable stepped on (P `core/MM_Misc.c:10039-10075`). Size: small.
18. **MM.INFO keys** (each small): `USB n`, `USB VID n`, `USB PID n` (P `core/MM_Misc.c:8875-8897`; the CMM2 has TinyUSB HID slots); `BOOT` (`:8470`, the PicoMite's reset-reason strings; C has its own `RESET`); `BOOT COUNT` (`:8454`); `LINE` (`:8900`; C has it commented out at `Src/MM_Misc.c:3207`); `AUTORUN` at the top level (`:8386`); `CALLTABLE` (`:8513`); `MAX VARS` (`:8918`); `SYSTEM HEAP` (`:9520`); `OPTION AUDIO|WIDTH|HEIGHT|KEYBOARD` (`:9018`, `:9124`, `:9108`, `:9081`).
19. **MM.USB** (the last USB key's scan code, P `core/Functions.c:695`; C keeps it in `USBcode`, `Src/KeyboardMap.c:74`, unused) and **MM.DISPLAY** (P `core/Functions.c:748`). Size: tiny.
20. **CPU SLEEP s** (P `core/MM_Misc.c:9744`) and **WATCHDOG HW n|OFF** (`:9700-9711`). The STM32's IWDG cannot be stopped once started, so HW OFF can only be emulated. Size: small.
21. **COPY A2A|A2B|B2A|B2B f1$, f2$** (P `misc/FileIO.c:6327-6373`). On the CMM2 A: and B: are the same card, so each maps to a plain COPY. Size: small.
22. **OPTION names a program or a user may give** (each small): `DEFAULT COLOURS|COLORS` (P `core/MM_Misc.c:6302`), `KEYBOARD nn` (`:5938`, = USBKEYBOARD), `GUI CONTROLS n` (`:7297`, = MAXCTRLS), `PLATFORM` (`:5742`; MM.INFO$(PLATFORM) here returns "", C `Src/MM_Misc.c:3140`), `RTC AUTO` (`:7443`, = DS3231), `F1` and `F5`-`F9` (`:5622-5682`), `DISPLAY rows, cols` (`:7270`), `LOGGING` (`:5601`), `PRINTSCREEN` (`:5391`), `AUTORUN n [,NORESET]` (`:6236-6248`; C has ON/OFF only, `Src/MM_Misc.c:2605-2609`, though flash slots now exist), `PROFILING ON, SAMPLE` (`:5314`), `CDC POLL RATE` (`:5569`).
23. **CALC, and implied CALC at the prompt with MM.ANSWER** (P `core/Commands.c:1941`, `core/MMBasic.c:2063`, `:2151`; CALCPROMPT is defined for every build in `configuration.h:37`). Audit P4 lists "CALC mode" as open. Size: small.
24. **The PicoMite dialects of PWM, SERVO and ADC**: `PWM ch, OFF`, `PWM SYNC`, `SERVO ch, OFF` (P `misc/External.c:3262`, `:3170`, `:3115`), `ADC RUN` (`:5460`), MM.INFO(ADC) and (ADC DMA) (`core/MM_Misc.c:8413-8419`), `SETPIN p, ARAW` (`misc/External.c:1594`), `PIN(TEMP)` (`:2199`). They belong to the switch (item 2). Size: small-medium.
25. **KEYBOARD ON|OFF** (the on-screen keyboard, P `core/MM_Misc.c:8329`) and **OPTION SCREEN KEYBOARD** (`:6069`). The audit wanted the option accepted silently (6.6). Size: medium; unlikely to be hit.
26. **MOUSE OPEN ch, clk, data / MOUSE CLOSE** (a PS/2 mouse on pins, P `input/mouse.c:436`, `:471`). On the CMM2 the PS/2 mouse is fixed hardware (OPTION MOUSE 0). Size: small.
27. **AUTOSAVE APPEND** (P `misc/FileIO.c:7295`). Size: small.

**Behaviour gaps inside existing commands (step 5), most likely first.** About 28 items, all small but one; none shows in a keyword diff, and the 71 above leaves them out.
1. **`OPTION PROFILING ON` in a program saves the option and soft-resets the board** (C `Src/MiscSTM32.c:1523-1531`, `goto saveandreset`, with no in-program check). On the PicoMite it is a run-time profiler, and `, SAMPLE` is refused here. 17 corpus files use it, all of them benchmarks. A guard is small; a port is large.
2. **`REDIM PRESERVE a(BOUND(a()) * 2)` fails with "Cannot find"**, because the array is erased before its bounds are evaluated (C `Src/Commands.c:3746-3763`; P `core/Commands.c:7870-7910`, its #105).
3. **MATH RANDOMIZE / the Mersenne Twister seed with 6069, not 69069** (C `Src/MATHS.c:156`; P `core/MATHS.c:138`): a given seed gives a different sequence from the PicoMite's.
4. **Sound stalls while a BMP decodes** (LOAD BMP, SPRITE LOADBMP, BLIT LOAD, FLASH LOAD IMAGE: C `Src/BmpDecoder.c` never calls routinechecks).
5. **The Switch Pro gamepad reports type 133, not 135** (C `Inc/USBGamepad.h:12`; P `input/USBKeyboard.c:292`). MM.INFO(USB n) is missing as well.
6. **"Not enough heap memory" without the size** (C `Src/Memory.c:1183`); RESTORE of a label named like a scalar variable (C `Src/Commands.c:3236`); SAVE IMAGE and Print Screen in MODE n,4 ignore MAP (C `Src/Rgb121.c:318-333`); OPTION PRINTSCREEN and Ctrl-\ are missing; MM.INFO(BOOT) and the brown-out reset cause; then the rest of table 5.1.

Not gaps, but should be fixed:
- **Documents now out of date.**
  - The porting chapter says the flash slots 1-3 "are not provided" (`docs/manual/text/22_porting.md:9`), but they are, since 0f0d2cc (`Src/ImageSlots.c:9-29`, `:464`).
  - The release notes' "Not there yet" still lists RAY and the FM mouse; both are done (138de2e, 3fda0cc).
  - Thirteen of the audit's own statements are contradicted by the code; they are listed in 4.0. Among them: Route B "not started", FLASH LOAD IMAGE "refuses slots 1-3", the TURTLE spellings "missing", the ON ERROR SKIP count bug "open", the GNU regex "undecided" (it was removed in 10ceb73), P7 "not started".
- **Release notes vs code.** The release notes say "KEYPAD refuse[s] a LOCAL array" (`docs/Release_Notes_7.00.00.md`, the I/O fixes), but `cmd_keypad` is inside a comment block (`Src/External.c:1876-1960`), and so is `cmd_lcd` (`:1275-1382`). Neither is in the command table.
- **Corpus failures to re-run.** The 21 Oct goldens baseline (`tests/goldens/CMM2_G2_480/SUMMARY.md`, made at ffd0001) shows two programs that the PicoMite reference runs: `pathmap` (OPEN "a:pm_t.dat": "The logical drive number is invalid") and `demo` from Testfiles (PLAY MODFILE "b:mario.mod": the same error). Reading `PathDrive` (`Src/FileIO.c:1167`) and FatFs `get_ldnumber` (`Src/ff.c:3266-3268`, B: and lower case accepted), the code looks right today, so I can't tell statically whether they still fail. Re-run both. The MATH DIFFs in that baseline (matha, mathq, mathw) are the current PicoMite MATHS.c's behaviour: the PicoMite reference is 6.04.00b51, older (both trees round through FloatToInt64: P `core/MATHS.c` WINDOW, C `Src/MATHS.c` the same lines).

---

## Step 1. Appendix A regenerated: the keyword tables

PicoMite `AllCommands.h` has 228 command entries and 127 token entries, counting every `#if` variant; CMM2 `Inc/allcommands.h` has 168 and 125. Below, every PicoMite entry that has no CMM2 table entry of the same name, after the CMM2 tokeniser's aliases.

### 1.1 PicoMite command-table entries not in the CMM2 table

| PicoMite entry (AllCommands.h line, build) | Class | Evidence |
|---|---|---|
| `Else If` (441) | a | alias: C `Src/MMBasic.c:2070` (to ElseIf) |
| `End If` (442) | a | C `Src/MMBasic.c:2073` |
| `Exit Do` (443) | a | C `Src/MMBasic.c:2076` |
| `/*`, `*/` (489-490) | a | the loader drops block-comment lines: C `Src/FileIO.c:384-386` (main file), `:545-547` (includes) |
| `PIO` (505) | b | RP2 PIO |
| `Edit File` (520) | c | `docs/Editor_Port_Plan.md:248` "D4, EDIT FILE: not on the CMM2 for now"; C `Src/Editor.c:819-820` |
| `Blit Memory` (528); also P's `SPRITE MEMORY` substitution (P `core/MMBasic.c:2025`) | a | C `Src/sprites.c:1177` (BLIT MEMORY through cmd_blit; SPRITE is cmd_blit, `Inc/allcommands.h:551`) |
| `Device` (534) | **d** | P `misc/External.c:5166`; ranked gap 3 |
| `RTC` (541) GETTIME/SETTIME/GETREG/SETREG | c | `docs/manual/text/22_porting.md:13` ("the RTC and keypad commands"), `:118`. Corpus: `Testfiles/FishFeeder.bas`, goldens `rtcreg` |
| `Colour Map` (564) | a | C `Src/Draw.c:5145` (COLOUR MAP in cmd_colour) → `ColourMap` `:5115` |
| `Cat` (565) | a | C `Src/MMBasic.c:2079` (to Inc) |
| `Color` (566) | a | C `Src/MMBasic.c:2061` |
| `Keypad` (571) | c | `22_porting.md:13`, `:118`. The code is commented out: C `Src/External.c:1876-1960` |
| `LCD` (573) | c | `22_porting.md:118` ("LCD displays ... not supported"). Commented out: C `Src/External.c:1275-1382` |
| `Wii Classic` (574) | a | WII → CONTROLLER (C `Src/MMBasic.c:2064`), then CLASSIC (C `Src/MiscSTM32.c:1213`, `cmd_classic` `:689`) |
| `Wii Nunchuck` (575) | **d** | C knows only "NUNCHUK" (`Src/MiscSTM32.c:1203`); ranked gap 5 |
| `Wii` (576), bare (`WII OPEN` = classic) | **d** | C `cmd_Controller` falls to "Syntax" (`Src/MiscSTM32.c:1218`); ranked gap 5 |
| `_wrap target`, `_wrap`, `_line`, `_program`, `_end program`, `_side set`, `_label`, `Jmp`, `Wait`, `In`, `Out`, `Push`, `Pull`, `Mov`, `Nop`, `IRQ SET`, `IRQ WAIT`, `IRQ CLEAR`, `IRQ NOWAIT`, `IRQ PREV`, `IRQ NEXT`, `IRQ`, `Set` (580-602) | b | the PIO assembler: each calls `call_pio()` (P `misc/Custom.c:3036-3095`). SET and WAIT are PIO only |
| `Byte(`, `Flag(` (603-604), `Bit(` (659) as commands | a | fold to BITBYTEFLAG( (C `Src/MMBasic.c:1816-1820`), `cmd_bitbyteflag` C `Src/Commands.c:2702` |
| `RESOLUTION` (610, HDMI) | c | audit 3.2 (line 168): "`RESOLUTION` (HDMI only: no)". Programs use OPTION RESOLUTION (arkanoid, picanoid: 5 corpus files); `22_porting.md:9` says to change MODE statements |
| `Camera` (613 HDMI, 618 non-VGA) | b | camera hardware |
| `Refresh` (619, non-VGA) | b | buffered LCD. The audit (6.6) planned to accept it silently; today it is an unknown command |
| `Backlight` (625, 633) | b | LCD backlight |
| `WEB` (635) | b | WiFi |
| `Configure` (649) | b | board presets (PicoCalc etc.) |
| `CMM2 Load`, `CMM2 Run` (652-653, rp2350) | b | not applicable by nature: they convert CMM2 programs for the PicoMite |
| `YModem` (675, rp2350 without USB) | c | `docs/Release_Notes_7.00.00.md:320`, `Release_Notes_7.00.00b2.md:117`, audit line 376 |
| `Stepper` (688), `TMC22xx` (714) (rp2350) | b | the G-code motion controller (P `io/stepper.c`, 7,000 lines, RP2 timers and GPIO); audit 3.4 (line 178) lists both as not applicable |
| `Frame` (689, rp2350) | c | `Release_Notes_7.00.00.md:320`. Corpus: `Testfiles/FrameTest`, `VBufTest` |
| `I2CLCD` (700) | c | `22_porting.md:118` (LCD displays). Corpus: `Bas/AirMonitor.bas` |
| `OneShot` (702) | **d** | P `misc/External.c:2522`; ranked gap 14 |
| `Mandelbrot` (704, VGA or rp2350) | **d** | P `graphics/DrawFill.c:1100`; ranked gap 8 |
| `Calc` (710, CALCPROMPT: every build) | **d** | P `core/Commands.c:1941`; ranked gap 23 |
| `Keyboard` (717, VGA+USB+GUI) | **d** | the on-screen keyboard, P `core/MM_Misc.c:8329`; ranked gap 25 |
| `Bluetooth` (720) | b | Bluetooth |

### 1.2 PicoMite token-table entries not in the CMM2 table

| Entry | Class | Evidence |
|---|---|---|
| `Bit(` (763), `Byte(` (843), `Flag(` (856) | a | fold, C `Src/MMBasic.c:1816`; `fun_bitbyteflag` C `Src/Functions.c:1076` |
| `Pio(` (832) | b | PIO |
| `Touch(` (875) | a | rewritten to CLICK( (C `Src/MMBasic.c:1943`), `fun_touch` C `Src/GUI.c:268`. The gestures are class b (step 2) |
| `Frame(` (897, rp2350) | c | as FRAME |
| `Ray(` (900, RAYCASTER) | a | fold to SPRITE(RAY, (C `Src/MMBasic.c:1816-1823`), `fun_ray` C `Src/Ray.c:1582` |

### 1.3 MM.* read-only variables (the `~(` list: P `core/Functions.c:49-82`, C `Src/Functions.c:57-77`)

| Variable | Class | Evidence |
|---|---|---|
| MM.PS2 (non-USB builds) | b | PS/2 keyboard scan code |
| MM.USB (USB builds) | **d** | P `core/Functions.c:695-697`; C has `USBcode` (`Src/KeyboardMap.c:74`) but no variable |
| MM.MESSAGE$, MM.ADDRESS$, MM.TOPIC$ | b | MQTT |
| MM.DISPLAY | **d** | P `core/Functions.c:748` (1 when the display is a console); tiny |
| MM.PERSISTENT | **d** | P `core/Functions.c:763`; with SAVE PERSISTENT |
| MM.CODE (rp2350) | b | the stepper's G-code buffer |
| MM.SUPPLY | b | VSYS through GP29/ADC3 |

Everything else in P's list (HRES, VRES, VER, I2C, FONTHEIGHT, FONTWIDTH, HPOS, VPOS, ONEWIRE, ERRNO, ERRMSG$, WATCHDOG, DEVICE$, CMDLINE$, FLAGS, WIDTH, HEIGHT, ERRLINE) is in C's list. The SUB hooks MM.STARTUP and MM.PROMPT are present (C `Src/main.c:915`, `:1012`), and so is MM.END (`InMMEND`, C `Inc/MMBasic.h:456`; `END NOEND`, C `Src/Commands.c:1862`).

### 1.4 The reverse (CMM2 only), briefly

Commands: `MMDebug`, `Page`, `Image`, `Bitbang`, `DHT22` (also `Humid`), `I2C3`, `DAC`, `Controller` (`WII` is aliased to it), `CAN`, and `BitByteFlag(`, the internal fold token. Tokens: `>>>`, `Nunchuk(`, `Classic(`, `Mouse(`, `Baudrate(`, `BitByteFlag(`. `Servo` is `cmd_pwm` (`Inc/allcommands.h:517`). The audit's 6.1 list of CMM2 features to keep is all still in the table.

Handler names that differ but are the same feature (from a name-level diff; 103 P-only, 42 C-only): cmd_i2c2/cmd_i2c_2, cmd_spi2/cmd_spi_2, cmd_DHT22/cmd_humid, cmd_WS2812/cmd_ws2812, cmd_mouse/cmd_usbmouse, cmd_psram/cmd_ram, cmd_files/cmd_listfiles, cmd_edit/cmd_newedit, cmd_update/cmd_JumpToBootloader, cmd_timer/cmd_uSec, cmd_map/cmd_clut, fun_map/fun_clut, fun_dev/fun_device_fn, cmd_slice|insert|add/cmd_arrayslice|insert|add, fun_bit|byte|flag/fun_bitbyteflag, fun_left|right/fun_schange, fun_errno|errmsg|mm*/fun_tilde, cmd_Classic|Nunchuck/cmd_classic|nunchuck, cmd_let_arraycopy (C does whole-array copy in its LET path, audit P1 5b.6), cmd_keyscan (part of KEYPAD).

---

## Step 2. Appendix B regenerated: sub-keywords inside shared handlers

Handlers were paired through the keyword tables (with the aliases above). Among the pairs not listed, no PicoMite literal is missing on the CMM2: these include MATH and MATH(, TILEMAP and TILEMAP(, SPRITE (P cmd_sprite against C cmd_blit), TURTLE, TEXT, FONT, CIRCLE, LINE, POLYGON, ARC, RBOX, CLS, GUI (the GUICONTROLS version), DRAW3D, FILES, LIBRARY, TYPE, STRUCT, DIM/LOCAL/STATIC, GPS(, SETTICK, OPEN, SPI and SPI2, ONEWIRE, IR, PULSE, PORT, SORT, LONGSTRING, CHAIN, RUN, END, XMODEM and ZMODEM. `NOEND` showed up under EDIT, CHAIN, XMODEM, FM and ZMODEM only because the helper scan reached `cmd_end`; it is present (C `Src/Commands.c:1862`).

### 2.1 PicoMite-only sub-keywords (classified)

| Handler | PicoMite-only literal(s) (P file:line) | Class | Evidence / note |
|---|---|---|---|
| LIST | COLLISIONS (`core/Commands.c:2781`), OPTIONS (`:2610`), PINS (`:2904`), VARIABLES (`:2614`) | **d** | C `cmd_list` (`Src/Commands.c:977-1219`) has ALL, COMMANDS, FUNCTIONS, TYPE (`:1140`) and its own FILES, FLASH, PAGES, PROFILE; gap 16 |
| LIST | SYSTEM I2C (`:2909`) | b | the CMM2 has no SYSTEM I2C bus |
| ON | PS2 (`core/Commands.c:8519`) | b | PS/2 keyboard |
| SAVE | PERSISTENT (`misc/FileIO.c:3085`), COMPRESSED IMAGE (`:3110`) | **d** | C `cmd_save` (`Src/FileIO.c:200-287`) has CLEAR, CONTEXT, DATA, IMAGE; gap 13 |
| COPY | A2A, A2B, B2A, B2B (`misc/FileIO.c:6327-6373`) | **d** | gap 21. Wildcard COPY is present (C `Src/FileIO.c:1023`) |
| FLASH | MODBUFF LOAD (`misc/FileIO.c:1471`), UF2 (`:1688`, HAS_USB_MSC) | b | the MOD buffer in RP2 flash; RP2 firmware files. SAVE, OVERWRITE, LOAD, RUN, CHAIN, LIST, ERASE [ALL], DISK LOAD and LOAD IMAGE are present (C `Src/ImageSlots.c:464-600`) |
| PLAY | CMD, CONTINUE, HALT, MIDI, MIDIFILE, NOTE, OFF, ON, STREAM, TEST (`io/Audio.c:2525-2798`, all `!NOVS1053`) | b | VS1053 only ("Only available with VS1053 audio"). ARRAY, SAMPLE, LOAD SOUND, BBC SOUND/ENVELOPE, RELEASE, NEXT, PREVIOUS are present (C `Src/Audio.c:1643`, `:1713`, `:1721`, `:1859`) |
| TRIANGLE | SAVE, RESTORE (`graphics/Draw.c:3633-3645`) | **d** | gap 11 |
| FRAMEBUFFER | SYNC, MERGE ,R / ,A (`graphics/FrameBuffer.c:1147-1177`, `PICOMITE` = SPI-LCD builds only) | b | the core-1 merge to an LCD. CREATE, CREATE 2, LAYER [TOP], WRITE N/F/L/2/T, COPY [,B], MERGE [,B], WAIT and CLOSE are present in MODE n,4/n,1 (C `Src/Rgb121.c`, e.g. MERGE `:574`) and missing in other depths: gap 1 |
| BLIT | RESIZE (`graphics/Blit.c:1363`), COMPRESSED (`:625`), MEMORY332 (`:648`) | **d** | gap 12. FLASH, FRAMEBUFFER, MEMORY, LOADBMP, READ, WRITE, CLOSE are present (C `Src/sprites.c:1169-1177`, `:1602`) |
| BLIT | MERGE, A, B, R (`graphics/Blit.c:1282-1306`, `!PICOMITEVGA`) | b | LCD builds' BLIT MERGE |
| ADC | RUN (`misc/External.c:5460`) | **d** | the PicoMite's ADC dialect (gap 24); C has TRIGGER of its own |
| SETPIN | ARAW (`misc/External.c:1594`) | **d** | gap 24 |
| SETPIN | COM1, COM2, I2C, I2C2, SPI, SPI2, PWM, PWMnA/B, IR, INT, FFIN (`misc/External.c:1616-1861`) | c | `22_porting.md:12`: each port has fixed pins, and the PicoMite's pin-assigning SETPIN commands "are not needed here" |
| SETPIN | HEARTBEAT, PIO0-PIO2 | b | RP2 hardware |
| GUI (non-GUICONTROLS builds, cmd_guiMX170) | BEEP, CALIBRATE, LCDPANEL, RESET, TEST, TOUCH (`graphics/Draw.c:413-580`) | b | touch panels and LCDs |
| PWM | OFF (`misc/External.c:3262`), SYNC (`:3170`) | **d** | the PWM dialect (audit 6.1 "dual"); gap 24. C has STOP |
| SERVO | OFF (`misc/External.c:3115`) | **d** | as PWM |
| I2C, I2C2 | SLAVE OPEN/CLOSE/READ/WRITE (`io/I2C.c:328-334`, `:367-373`) | **d** | gap 15. CHECK, OPEN, CLOSE, READ, WRITE are present (C `Src/I2C.c:118`) |
| MEMORY | SHARE HOST/CLIENT/STOP [HOST/CLIENT] (`core/Memory.c:1005-1029`) | b | PIO and DMA (`core/Memory.c:995`); audit 6.6 lists it as an error |
| OPTION | see 2.2 | | |
| POKE | DISPLAY HRES, DISPLAY VRES (`core/MM_Misc.c:9846-9856`) | **d** | gap 6 (Elite) |
| POKE | DISPLAY cmd, data… (`:9858+`, non-VGA) | b | LCD controller commands; audit 6.6 |
| WATCHDOG | HW (`core/MM_Misc.c:9702`) | **d** | gap 20; C `cmd_watchdog` is software only (`Src/MM_Misc.c:3389`) |
| CPU | SLEEP (`core/MM_Misc.c:9744`) | **d** | gap 20; C `cmd_cpu` has RESTART only (`Src/MM_Misc.c:3407-3419`) |
| CPU | SPEED (`:9754`) | b | RP2 clocks ("Not available on this version" on VGA builds) |
| AUTOSAVE | APPEND (`misc/FileIO.c:7295`) | **d** | gap 27 |
| MOUSE | OPEN, CLOSE (`input/mouse.c:436`, `:471`) | **d** | gap 26. INTERRUPT ENABLE/DISABLE and SET are present (C `cmd_usbmouse`) |
| MODE | 400, 800 (`graphics/SSD1963.c:3115`, `:3127`) | b | SSD1963 panels |
| DEVICE( | WII, CLASSIC, NUNCHUCK (`misc/External.c:4263-4299`) | **d** | gap 5. MOUSE and GAMEPAD are present (C `fun_device_fn`) |
| PIN( | TEMP (`misc/External.c:2199`) | **d** | gap 24 (the H743 has an internal sensor on ADC3) |
| PIN( | BOOTSEL (`:2216`) | b | RP2 BOOTSEL button |
| PEEK( | BP, WP, SP (`core/MM_Misc.c:10039`, `:10052`, `:10067`) | **d** | gap 17 (C `fun_peek` has BYTE/INT8, VARADDR, VARHEADER, CFUNADDR, WORD, SHORT, INTEGER, FLOAT, PROGMEM, VAR) |
| PEEK( | A, X, Y, Z, ACTIVE, BUFFER, STATUS, STEPPER (`:10174-10208`, rp2350) | b | the stepper |
| TOUCH( (C: CLICK() | TAP, DTAP, TTAP, HOLD, SWIPE, SWL/SWR/SWU/SWD, PINCH, EXPAND, CONTRACT, ROTATE, CW, CCW, X2, Y2, XN, YN (`graphics/Touch.c:1055-1157`) | b | multi-touch panels; audit P3 (line 372-373) says the CMM2 has none |
| MM.INFO( | see 2.3 | | |
| (argument forms, not literals) BITSTREAM | mode, and the dual-pin form (`misc/External.c:4959-4976`) | **d** | gap 4 |
| (argument forms) the PicoMite FRAMEBUFFER outside 4/1-bit | | **d** | gap 1 |

A heuristic check of the largest `getargs`/`getcsargs` count per handler (helpers included) found only BLIT (RESIZE), ADC and PWM (the dialects), MEMORY (SHARE), OPTION (panel options) and BITSTREAM, all accounted for above. LOAD's `,R`/`,C` forms of program loading are class c (`docs/manual/text/07_using_mmbasic.md:98`: "the Colour Maximite 2 does not need LOAD or SAVE commands for programs").

### 2.2 OPTION: every PicoMite top-level option name the CMM2 lacks

The PicoMite's top-level option names were extracted from `cmd_option` (P `core/MM_Misc.c:5322-8240`); the CMM2's are in `cmd_option` (C `Src/MM_Misc.c:2446-2707`) and `OtherOptions` (C `Src/MiscSTM32.c:1341-1702`). Present on both: ANGLE, AUTORUN (ON/OFF), BASE, BAUDRATE, BREAK, CASE, COLOURCODE, COMPILE ON/OFF/SHADOW (C `Src/MiscSTM32.c:1429-1434`), CONSOLE, DEFAULT, DEFAULT MODE (`:1600`), ESCAPE, EXPLICIT, KEYBOARD REPEAT (`:1401`), LEGACY, LIST, LOCAL VARIABLES (a no-op, C `Src/MM_Misc.c:2450`), MILLISECONDS, MOUSE, MOUSE SENSITIVITY (`:1465`), PIN, PROFILING (`:1523`), SYMBOLS (C `Src/MM_Misc.c:2459`), TAB, VCC.

| P option (line) | Class | Note |
|---|---|---|
| CONTINUATION LINES (7460) | **d** | gap 10 |
| NOCHECK (5326) | **d** | gap 9 |
| DEFAULT COLOURS / COLORS (6302) | **d** | gap 22 |
| KEYBOARD nn (5938) | **d** | = USBKEYBOARD with the PicoMite's layout codes (GR = DE, audit 6.1) |
| GUI CONTROLS (7297) | **d** | = MAXCTRLS |
| PLATFORM (5742) | **d** | |
| RTC AUTO (7443) | **d** | = DS3231 |
| F1, F5, F6, F7, F8, F9 (5622-5682) | **d** | C has F11, F12 |
| DISPLAY (7270) | **d** | the terminal size |
| LOGGING (5601) | **d** | errors appended to log.txt (P `core/MMBasic.c:6890`) |
| PRINTSCREEN (5391) | **d** | |
| AUTORUN n [, NORESET] (6236-6248) | **d** | |
| PROFILING ON, SAMPLE (5314) | **d** | C has the sampler only as a diagnostic build (`Src/PcSampler.c`); 17 corpus files (benchmarks) use it |
| CDC POLL RATE (5569) | **d** | TinyUSB CDC tuning; audit 6.6 "silently accepted" |
| SCREEN KEYBOARD (6069) | **d** | gap 25 |
| DISK LOAD, DISK SAVE (8167-8187) | c | `Release_Notes_7.00.00.md:320` |
| RESOLUTION (6878 etc.) | c | audit 3.2 |
| SYSTEM I2C (7843), COUNT (7914), SERIAL CONSOLE (6086) | c | fixed pins (`22_porting.md:12`) |
| AUDIO (7569), FAST AUDIO (5466), MODBUFF (7492), AUTOREFRESH (7174), BACKLIGHT KBD/LCD (6712-6728), CPUSPEED (7156), CYW43 PINS, HDMI PINS, VGA PINS, KEYBOARD PINS, PS2 PINS, PSRAM PIN, SDCARD, SYSTEM SPI, LCD SPI, LCD320, LCDPANEL [CONSOLE/NOCONSOLE], TOUCH, HEARTBEAT, LOCAL KEYBOARD, POWER, PICO, PICO PLUS 2, RADIO, WEB MESSAGES, WIFI, TCP SERVER PORT, UDP SERVER PORT, TELNET CONSOLE, TFTP, and OPTION RESET's board presets | b | RP2 pins, panels, network. The audit (6.6) wanted AUDIO, CPUSPEED, HEARTBEAT, PICO*, POWER, PSRAM PIN, CDC POLL RATE, SERIAL CONSOLE, VGA/HDMI PINS, COUNT, FAST AUDIO, LCD320, AUTOREFRESH, BACKLIGHT, SCREEN KEYBOARD and LOGGING "silently accepted"; none is accepted today |

### 2.3 MM.INFO: the full key list

PicoMite `fun_info` (P `core/MM_Misc.c:8350-9695`) against CMM2 `fun_info` (C `Src/MM_Misc.c:2820-3387`). **Present on both**, top level: AUTORUN under OPTION only (see below), BBC BUFFER, BCOLOUR/BCOLOR, COMPILE, CPUSPEED (a number here, a string on P: dialect), CURRENT, DEVICE, DISK SIZE, DRIVE, ERRMSG, ERRNO, EXISTS DIR, EXISTS FILE, FCOLOUR/FCOLOR, FILESIZE, FLASH, FLASH ADDRESS, FONT, FONT ADDRESS, FONT POINTER, FONTHEIGHT, FONTWIDTH, FREE SPACE, HEAP, HEAPEND, HPOS, ID, KEYBOARD (different words: dialect), MODIFIED, ONEWIRE, PATH, PIN (different words: dialect), PLATFORM ("" here), PROGRAM, PSRAM SIZE, SDCARD, SOUND, STACK, SYSTEM I2C (""), TILE HEIGHT, TRACK, UPTIME, VARCNT, VERSION, VPOS, WRITEBUFF. Under OPTION: ANGLE, AUTORUN, BASE, BREAK, CONSOLE, DEFAULT, EXPLICIT.

| PicoMite key (P line) | Class | Note |
|---|---|---|
| AUTORUN, top level (8386) | **d** | C has it only under OPTION (`Src/MM_Misc.c:3080`) |
| BOOT (8470) | **d** | reset-reason words; C has its own RESET |
| BOOT COUNT (8454) | **d** | |
| CALLTABLE (8513) | **d** | the table is in vector slot 7 (C `Inc/CallTable.h`) |
| LINE (8900) | **d** | C has it commented out (`Src/MM_Misc.c:3207`) |
| MAX VARS (8918) | **d** | |
| SYSTEM HEAP (9520) | **d** | |
| USB n, USB VID n, USB PID n (8875-8897, USBKEYBOARD) | **d** | |
| LCDPANEL (8866 VGA builds; 8858 LCD builds) | **d** | gap 7 |
| PERSISTENT (9275) | **d** | with SAVE PERSISTENT |
| ADC, ADC DMA (8413-8419) | **d** | with the PicoMite's ADC |
| OPTION AUDIO (9018), OPTION WIDTH (9124), OPTION HEIGHT (9108), OPTION KEYBOARD (9081) | **d** | C has OPTION USBKEYBOARD and CONSOLE PORT instead |
| OPTION AUTOREFRESH (9047), OPTION FLASH SIZE (9102) | b | LCD; RP2 flash |
| PINNO (9145), MAX GP (9682) | c | GPn names not accepted (`22_porting.md:12`). Used by PETSCII Robots (`games/petrobot/robots.bas:2132`) and picofrog |
| DEBUG (8595), FLASHTOP (8719), INTERRUPTS (8844, RP2040), SYSTICK (9514), SPI SPEED (9492), VALID CPUSPEED (9656), STACKPEAK (9531, web+rp2350 only), MODBUFF ADDRESS (8924), PIO RX DMA, PIO TX DMA (9281-9287), PWM COUNT, PWM DUTY, PWM SLICE (9293-9323), PS2 (9407), LCD320 (8852), TOUCH (9567), SCROLL, SCREENBUFF (9549-9555, buffered LCD) | b | RP2 registers, slices, panels |
| BATTERY, BIOS, CHARGING (PicoCalc), BLUETOOTH, SPEAKER, SERIAL (BT), IP ADDRESS, TCP REQUEST, TCP PORT, UDP PORT, MAX CONNECTIONS, WIFI STATUS, TCPIP STATUS, TELNET, TLSHEAP, TLSHEAPERR, TLSHEAPMAX, CHEAPMAX, CHEAPTOTAL, OPTION SSID | b | PicoCalc, Bluetooth, network |

CMM2-only MM.INFO keys: CODE SIZE, DIRECTORY, FRAMEBUFFER, FRAMEH, FRAMEV, HARDWARE, HEAPI/IHEAP, HEAPS/SHEAP, I2C, ID48, LINES, MAX PAGES, MINHEAP, MODE, OVERRUN, PAGE ADDRESS, RESET, RTC, SAMPLE PLAYING, SEARCH PATH, WRITE PAGE, and OPTION CONSOLE PORT, LEGACY, MOUSE, PROFILING, USBKEYBOARD, Y_AXIS. The compiler's own MM.INFO table (C `Src/Stream.c:1061`) only lists types for the compiled path.

### 2.4 CMM2-only sub-keywords (the reverse, briefly)

CLEAR VARS; LIST COMMANDS V, FUNCTIONS V, FILES, FLASH [ALL], PAGES, PROFILE [CSV]; LOAD FONT, GIF; PLAY EFFECT, TTS (PHONETIC); PIXEL FILL; BOX AND_PIXELS/OR_PIXELS/XOR_PIXELS; FRAMEBUFFER BACKUP/RESTORE/WINDOW (8 to 32-bit modes); GUI CURSOR and its own sub-keywords; ADC TRIGGER; SETPIN OC/OOUT; PWM STOP; SETTICK FAST/PAUSE/RESUME; the OPTION list in the audit's 3.5 (VCC, Y_AXIS, SEARCH PATH, STATUS, COLOURCODE REVERSE, CR/LF/CRLF, CASE UPPER/LOWER/TITLE, F11/F12, DS3231, OVERCLOCK, BASELINE, SD TIMING, RTC CALIBRATE/DRIVE, USB POLLING, VGA OUTPUT, HORIZONTAL OFFSET, EDIT FONT, MAXCTRLS, FLASH, RAM, CONSOLE PORT, SERIAL PULLUP); PEEK(BYTE (INT8 inside); RGB(NOTBLACK); PIXEL(…, FRAMEBUFFER).

---

## Step 3. Every PicoMite-only item, by class

Steps 1 and 2 classify each item. Here they are by class, with the (a) evidence.

**(a) PRESENT now (41).** ELSE IF, END IF, EXIT DO, COLOR, CAT (C `Src/MMBasic.c:2061-2081`); WII CLASSIC (C `Src/MMBasic.c:2064`, `Src/MiscSTM32.c:1213`); `/*`, `*/` (C `Src/FileIO.c:384`, `:545`); BLIT MEMORY and SPRITE MEMORY (C `Src/sprites.c:1177`); COLOUR MAP (C `Src/Draw.c:5145`); BIT(, BYTE( and FLAG( as functions and commands (C `Src/Functions.c:1076`, `Src/Commands.c:2702`); TOUCH( (C `Src/MMBasic.c:1943`, `Src/GUI.c:268`); RAY( (C `Src/Ray.c:1582`); MM.STARTUP, MM.PROMPT, MM.END (C `Src/main.c:915`, `:1012`; `Src/Commands.c:1862`); PRINT #GPS (C `Src/Commands.c:465`); EXECUTE "RUN" (C `Src/Commands.c:303`); TURTLE PEN COLOR/PENCOLOUR/FILL COLOR/FILLCOLOUR/CURSOR COLOR/CURSORCOLOUR (C `Src/turtle.c:795`, `:926`, `:997`); FLASH SAVE/OVERWRITE/LOAD/RUN/CHAIN/LIST/ERASE (C `Src/ImageSlots.c:464-600`); RAM … (C `:620`); SAVE/LOAD CONTEXT (C `Src/FileIO.c:212`, `:797`); OPTION COMPILE/SHADOW (C `Src/MiscSTM32.c:1429`); OPTION SYMBOLS (C `Src/MM_Misc.c:2459`); OPTION LOCAL VARIABLES (accepted, C `:2450`); OPTION KEYBOARD REPEAT (C `Src/MiscSTM32.c:1401`); OPTION MOUSE SENSITIVITY (C `:1465`); PLAY ARRAY/SAMPLE/LOAD SOUND/BBC/RELEASE (C `Src/Audio.c:1643-1859`); MAP GREYSCALE (C `Src/Draw.c:2588`); I2C CHECK (C `Src/I2C.c:118`); SPRITE STATIC/SET TRANSPARENT/LOADBMP (C `Src/sprites.c:1602-1613`); LIST TYPE (C `Src/Commands.c:1140`); BLIT WRITE's default orientation, which now follows the command used (C `Src/sprites.c:1717`: `SpriteCommand() ? 4 : 0`); WS2812 in the PicoMite's form, type, pin, nbr, value (C `Src/External.c:1579-1580`); DEVICE(MOUSE …) and DEVICE(GAMEPAD …) (C `Src/USBMouse.c:340`); END NOEND (C `Src/Commands.c:1862`).

**(b) NOT APPLICABLE (about 180).** PIO and its 23 assembler words, and PIO(; WEB and every network option and MM.INFO key; MM.MESSAGE$/ADDRESS$/TOPIC$; BLUETOOTH; CAMERA; BACKLIGHT; REFRESH; CONFIGURE; CMM2 LOAD/RUN; STEPPER, TMC22XX, MM.CODE and the stepper PEEKs; MEMORY SHARE; VS1053 PLAY forms; FLASH MODBUFF LOAD and UF2; MODE 400/800; LCD-build FRAMEBUFFER SYNC/MERGE R,A and BLIT MERGE; the GUI MX170 touch forms; TOUCH() gestures; ON PS2; MM.PS2; MM.SUPPLY; PIN(BOOTSEL); CPU SPEED; SETPIN HEARTBEAT/PIO; the RP2/panel OPTIONs and MM.INFO keys listed in 2.2 and 2.3; LIST SYSTEM I2C.

**(c) DOCUMENTED (19).** KEYPAD, LCD, I2CLCD and RTC (`docs/manual/text/22_porting.md:13`, `:118`); EDIT FILE (`docs/Editor_Port_Plan.md:248`, "not on the CMM2 for now"); FRAME and FRAME(, YMODEM, OPTION DISK SAVE/LOAD (`docs/Release_Notes_7.00.00.md:320`); RESOLUTION and OPTION RESOLUTION (audit line 168); GPn names, MM.INFO(PINNO)/(MAX GP), and SETPIN's COM/I2C/SPI/PWM/IR/INT/FFIN assignments, OPTION SYSTEM I2C/COUNT/SERIAL CONSOLE (`22_porting.md:12`); program LOAD/SAVE including LOAD ,R/,C (`docs/manual/text/07_using_mmbasic.md:98`); RUN with a variable file name (`docs/manual/ref/commands.txt:2404`).

**(d) GAP (71).** The ranked list in the summary; each item there has its PicoMite file:line and a size.

---

## Step 4. The audit's sections 0, 2.1, 3, 4, 5 and 7.1 today

Anchors: HEAD a0ce2c2; 195 commits since the baseline 64b2746 (14 Oct); **no git tags**. Uncommitted: MODE n,1 and TILE (`Src/Mono.c`, `Inc/Mono.h` untracked), and edits to Rgb121, ltdc, Ray, ImageSlots, the manual and others. From the existing `build/Release/CMM2V1.5.map` (22 Oct 18:25, uncommitted work included):

| Area | Size |
|---|---|
| `.ITCMSection` | 0x87e0 = 34.8 KB (from 0x298) |
| Bank 1 | ends 0x080FAC20, about 21.5 KB free |
| `.bank2` | 84 KB of 128 KB |
| Image | 1,133,024 bytes |

Evidence is a C file:line or a CMM2 commit hash.

### 4.0 Statements in the audit that the code now contradicts

1. **Line 382, "Route B is not started":** wrong. ffd0001 added OPTION COMPILE ON/OFF/SHADOW (`Src/MiscSTM32.c:1429-1434`; `Src/Stream.c` is 5,733 lines; `RB_SPLICE 0` at `:65`).
2. **Line 353, "FLASH LOAD IMAGE refuses slots 1-3":** wrong. Slots 1-8 are accepted (`Src/ImageSlots.c:471`). The flash slots are files in A:/.flash (0f0d2cc). The manual's `22_porting.md:9` repeats the old claim.
3. **Lines 377 and 823, the TURTLE spellings:** present since d73cccd (`Src/turtle.c:795`, `:926`, `:997`).
4. **Line 124, the ON ERROR SKIP count bug "open":** fixed (`Src/MMBasic.c:495-500`, `ExecDepth <= ErrorSkipDepth`, 34c97cb).
5. **Bug 10, "CALL still to check":** fixed (`Src/Commands.c:1477`, 9ad3be8).
6. **Section 4, 7.2 item 9 and P6, the GNU regex:** removed in 10ceb73 (with AUTOSAVE2). INSTR and LINSTR use `re.c` (`Src/Functions.c:826`, `Src/MM_Misc.c:1389`). The `regcomp` calls left are dead code.
7. **Section 0 item 3, `.ramcode` in flash:** `assember.S` is `.section .itcm.asm` (fd1b0b0).
8. **P4, the FM mouse:** done (3fda0cc, `Src/FileManager.c:328`). The release notes still list it, and RAY, under "Not there yet" (`docs/Release_Notes_7.00.00.md`).
9. **"Waiting on Peter":** both items are settled. The compiler is on both boards (`Inc/Memory.h:116-118`), and the SP Tools baseline is committed (ffd0001).
10. **P7 "not started":** the manual (M0-M4), the release notes and the goldens references are committed.
11. **Sections 0.6 and 3.5 call the line profiler "OPTION PROFILE":** it has always been `OPTION PROFILING` (`Src/MiscSTM32.c:1523`), the PicoMite's name for a different profiler.
12. **P1 step 6, "OPTION SYMBOLS OFF kept as the escape hatch":** it is accepted and does nothing (`Src/MM_Misc.c:2459-2475`; 1736efd).
13. **7.2 item 2 (one flash slot in sector 5, program in sectors 2-4) and P0's "baseline tag":** superseded or not done. The program uses sectors 1-6 (`Inc/Flash.h:71-73`), and there are no tags.

### 4.1 Section 0, points 1-10

| # | Status | Evidence |
|---|---|---|
| 1 14-bit command tokens; function folds | DONE | `Inc/MMBasic.h:325`, `:329`; 9eb047f, b5bbff6; folds 9692ad1 (`Src/MMBasic.c:1816-1823`). The function table has 125 of about 127 one-byte slots, so it is nearly full again. MOUSE(, NUNCHUK( and CLASSIC( are not folded |
| 2 the H7 makes V7 pay | PARTLY | the executor is in ITCM (`Src/Stream.c:3845`), the stream in SDRAM (`Inc/Memory.h:116`). The locals' 512 records are still in D3 (`Src/Memory.c:905`) |
| 3 ITCM placement | PARTLY | b668f22; `*(.itcm .itcm.*)` rule (`STM32H743II_FLASH.ld:183`); `ITCM_CODE`. The object-name patterns remain, including the deleted hcd. There is no post-link check, and VTOR is still in flash (`Src/system_stm32h7xx.c:240`) |
| 4 flash budget | DONE (revised 19 Oct) | `.bank2` (`.ld:102-125`, 1718e3c); program in sectors 1-6; library in sector 6 (`Src/Library.c:34`); options in sector 7 |
| 5 thirty-plus gaps | MOSTLY DONE | see steps 1-3: 63 small items left, about 10 of them likely to matter |
| 6 CMM2-only features survive | DONE, apart from deliberate removals | PAGE, MODE, IMAGE, MOUSE(, NUNCHUK(, CLASSIC(, CONTROLLER, CAN, DAC/ADC, PLAY TTS/EFFECT (`Src/Audio.c:1995`, `:2027`), LOAD GIF/FONT, MMDEBUG, BITBANG, `>>>`, OPTION LEGACY, the line profiler, MAP/CLUT, BAUDRATE(, MM.CMDLINE$, MM.WATCHDOG. **Removed:** AUTOSAVE2 (10ceb73) and the hardware JPEG decoder (e325a5f) |
| 7 vendor libraries | DONE apart from the toolchain | 4.4 |
| 8 test harness | DONE | `tools/goldens.py`, `goldcmp.py`, `crashers_test.py`; `tests/{goldens,testfiles,cmm2,editor,sptools,audio,bench,usb}`. No pre-flight script |
| 9 known bugs | DONE | 4.2 |
| 10 the dialect switch | NOT DONE | no PICOMITE option and no dialect global in `Src/` or `Inc/`. Groundwork: `docs/PicoMite_CMM2_Mode_Map.md`, fonts 8 and 9 (a0ce2c2), B: (81f2433) |

### 4.2 Section 2.1, the known bugs (all fixed)

| Bug | Evidence |
|---|---|
| 5 STATIC after a nested call | `Src/Commands.c:3455-3467`, the 0x1E separator (d4477bc, 9ad3be8) |
| 7 unbounded probe loops | `Src/MMBasic.c:3436`; name index at most half full (`:540-622`, fb3016c) |
| 10 tokenise bound and CALL | `Src/MMBasic.c:1886`, `:2259` (905e59b); `Src/Commands.c:1477` (9ad3be8) |
| 11 `rti[4]` | `Src/MM_Misc.c:3906` (9eb047f) |
| 22 MAX/MIN at FLT_MAX | `Src/Functions.c:1499` |
| 25/26 INF/NaN | `Src/MMBasic.c:4704-4705`, `:5158-5163` |
| 27 integer `^` | `Src/Operators.c:81-101` (squaring) |
| 23 ABS/SGN | `Src/Functions.c:536` |
| 24 32-bit subscripts | `Src/MMBasic.c:3764` |
| ON ERROR SKIP count (PicoMite #34) | `Src/MMBasic.c:495-500` (34c97cb) |
| 16, 9, 14 (not checked in the audit) | `Src/MMBasic.c:4359-4366`; `:806-818`; `Src/MM_Misc.c:3275-3276` |

`tests/cmm2/bugs.bas` covers these (42 of 42 in `bugs.expected`).

### 4.3 Section 2 table ("worth on the CMM2") and section 3 (the functional gaps)

| Item | Status | Evidence |
|---|---|---|
| Line header and skip byte, T_LABEL 3 | DONE | `Inc/MMBasic.h:96`, `:113-119`; daa0537 |
| Symbols | DONE | `Src/Symbols.c`; 9eac831, aec2d8c |
| Compact variable table, name index, parameter-layout cache | DONE | `VREC`; `MAXGLOBALVARS` 1024; fb3016c. The locals are still in D3 |
| IntReady, routinechecks gate, IF jump table, DO fast path | DONE | `Src/MMBasic.c:503-504`; `Src/Commands.c:1489-1611` (b232cad), `:2324+` (d317170) |
| Route B | DONE, splice off | ffd0001 |
| Profiling | PARTLY | the line profiler is kept (heap counters, 9df6eb7). The PicoMite's OPTION PROFILING counters and `, SAMPLE` are absent (the sampler is only in the `Sampler` build preset) |
| Arrays and types | DONE | DIM_ONE, TYPE (`Src/Struct.c:342`), REDIM, ERASE, array FUNCTIONs (34c97cb). `MAXDIM` is still 5 |
| Libraries, flash/RAM/image slots, CSUB tables | DONE | f42724d, 0f0d2cc, 9c1c389, 87c41d0 |
| Error handling | DONE | MM.ERRLINE, ON ERROR RESTART (`Src/Commands.c:3339`), the stack check (29f6c2c) |
| Editor | DONE | `Src/Editor.c`; kilo removed (141b113) |
| Console line editing across rows | NOT DONE | `Src/main.c:4534-4537`: "Line is too long to edit" |
| HELP from help.txt | PARTLY | `Src/Commands.c:279-282` reads A:/help.txt if it is there. `help.h` is still compiled in; there is no generator and no help.txt |
| Section 3.1 language | DONE but for | SAVE PERSISTENT, ONESHOT, OPTION NOCHECK, DEFAULT COLOURS, F1/F5-F9, PLATFORM, CALC, LIST VARIABLES/OPTIONS/PINS/COLLISIONS, the DEVICE command, KEYBOARD. KEYPAD is documented as not supported |
| Section 3.2 graphics | DONE but for | the PicoMite FRAMEBUFFER outside n,4/n,1, TRIANGLE SAVE/RESTORE, BLIT RESIZE/COMPRESSED/MEMORY332, MANDELBROT, DRAW3D growth (MAX3D stays 128, as the audit wanted). FRAME is not planned; RESOLUTION, MODE 400/800 and REFRESH are documented as "no" |
| Section 3.3 sound | DONE | PLAY SOUND, SAMPLE, LOAD SOUND, ARRAY, BBC, RELEASE, NEXT/PREVIOUS, the track list. The PicoMite's NOTE, STREAM, HALT, CONTINUE, MIDI*, ON/OFF/TEST are VS1053-only (class b) |
| Section 3.4 I/O | PARTLY | Done: USB keyboard, mouse, gamepad, CDC COM4-7 and MSC C:; MOUSE SET/INTERRUPT; GAMEPAD; DEVICE(MOUSE/GAMEPAD); I2C CHECK; ZMODEM; FM; STAR/ASTRO. Gaps: I2C SLAVE, WATCHDOG HW, CPU SLEEP, the WII/DEVICE spellings, BITSTREAM's forms. By choice: YMODEM, EDIT FILE, LOAD ,C (no program LOAD) |

### 4.4 Section 4, vendor software

| Component | Status | Evidence |
|---|---|---|
| FatFs | DONE | R0.16, patches 1-2 (f48f1a1); B: in `get_ldnumber` (`Src/ff.c:3266`) |
| cJSON | DONE | 1.7.19 (63df823) |
| dr_flac / dr_mp3 / dr_wav | DONE | 0.13.4 / 0.7.4 / 0.14.6 (fd23567) |
| hxcmod | PARTLY | still differs from the PicoMite's copy by about 51 lines |
| picojpeg, uPNG, tiny-regex-c | DONE | `re.c` is the PicoMite's; uPNG 0e0cca6 |
| GNU regex | REMOVED | 10ceb73 |
| tiny-AES-c | not updated | formatting differences only |
| kilo | REMOVED | 141b113 |
| SAM, gifdec | KEPT | |
| STM32H7 HAL / CMSIS | DONE | HAL 1.11.6, device 1.10.7, core 5.6 (2072b14) |
| USB host | DONE | TinyUSB efab607b8; the ST library and the JPEG utilities removed (2cd87a8, e325a5f) |
| Toolchain | NOT DONE | GCC 13.3.rel1 (`CMakePresets.json:13`) |
| `.ioc` | NOT DONE | untouched (CubeMX 5.5.0) |

### 4.5 Section 5, tests and tooling

| Item | Status | Evidence |
|---|---|---|
| Version control | DONE (no tags, no pre-flight script) | git, 195 commits |
| Regression | DONE | goldens 112 programs (`tests/goldens/CMM2_G2_480/SUMMARY.md`: MATCH 36, DIFF 14, ERROR 51, made at ffd0001); Testfiles 66; `tests/cmm2` 86 programs with `.expected`; `tests/editor` 29 scripts; sptools, audio, usb |
| Benchmarks | PARTLY | `tests/bench/src`; the committed baseline is still 6.00.00b7's (4db1c0b) |
| Board driving | DONE | `tools/cmm2serial.py`, `ztest.py`, `usbboot.py`, `edit_test.py`, `flashslots_test.py`, `rbtest.py` and others |
| Help generator | NOT DONE | |
| Static checks | NOT DONE | only a D2 ASSERT in the linker script |
| Manuals | DONE | `docs/manual` (docx, pdf, 45 chapters, ref/) |

### 4.6 Section 7.1, phases P0-P7

| Phase | Done | Not done |
|---|---|---|
| P0 | git (64b2746), the placement fix (b668f22), `.ramcode` (fd1b0b0), harness, ZMODEM (449c54c), crashers (905e59b, d2482a5), budget, CSUB ABI (87c41d0) | the section-name rules only half done (the object patterns remain); the post-link ITCM check; goldens at 400 and 504 MHz; the baseline tag |
| P1 | steps 1-9 all done: tokens, bugs, line header, variables, structures, symbols, folds, mmb2csub (1858a28), the statement loop, `/* */` (c2b9ce0), array FUNCTIONs, MM.ERRLINE, ON ERROR RESTART, CHAIN, SAVE/LOAD CONTEXT (bfda1d7), MM.STARTUP/PROMPT/END, MATH (9f85a3d), LIBRARY (f42724d), FLASH and RAM slots, CSUB tables | gate not met (goldens ERROR 51, DIFF 14 at ffd0001); "a CSUB program is always reflashed" is probably still open (`Src/FileIO.c:687-698` unchanged; not run) |
| P2 | USB (2cd87a8 … 15094bf), storage (C:, B:), audio A1-A6, I2C step 1 (`i2c_wait_idle`, `Src/I2C.c:566`), I2C option 1 hold (`:598`, `:655-663`) | video: DMA2D initialised but unused for PAGE COPY, BLIT or MERGE; I2C steps 2, 3 and 5 (`mT4IntEnable(0)` at `Src/I2C.c:656`; `Src/Timers.c:168-213`); the polling does not skip a held bus; WATCHDOG HW; CPU SLEEP/SPEED; MM.INFO completeness (85 keys against 111) |
| P3 | image slots, MODE n,4 (5dd77e9), MODE n,1 + TILE (uncommitted), TILEMAP, SPRITE (feda722), GUI LISTBOX/SLIDER/CLICK (15571c4), RAY (138de2e), TURTLE spellings, LOAD PNG/JPG | LAYER/MERGE outside the 4-bit modes; DRAW3D growth; **OPTION PICOMITE and its mapping tables**; the P3 gate. FRAME is not planned |
| P4 | the editor E0-E6 (141b113), FM (81f2433) with mouse (3fda0cc), F11 help from help.txt (`Src/Editor.c:666`) | E7 (^R is Find, `Src/Editor.c:2225`; EDIT at an error line only for the main file), E8/E9 partly, LOAD ,C, AUTOSAVE APPEND, HELP generation, CALC, the LIST variants, MM.INFO completeness. YMODEM and OPTION DISK SAVE/LOAD are not planned |
| P5 | performance round 1 (fd1b0b0), hot paths in ITCM, Route B (ffd0001) | VTOR/vector table to ITCM, DMA2D, the library's string routines (nano.specs, `CMakeLists.txt:81`), the CMM2 column of the Performance Guide |
| P6 | every library row | the `.ioc`, the toolchain |
| P7 | manual, release notes, goldens references, upload instructions | help.txt, pre-flight script |

7.2: decisions 1, 3, 4, 7 (named), 8, 9 (in effect: regex removed), 10 and 11 (in effect: none) are settled. Decision 2 is revised. **5 (drive letters under the PicoMite dialect) and 6 (GPn pin naming) are still open.** 7.4: pin interrupts through EXTI, the performance remainder and the hardware tests are still open; the rest are done or settled.

---

## Step 5. V7.00.00 release notes: behaviour the keyword diff does not show

Source: P `docs/V7.00.00_release_notes.md` (3,349 lines, including "Everything in 6.04.00 RC2" from line 2718). Each BASIC-visible item (about 190) was checked in the CMM2 code. Left out: performance, build and flash layout, hardware the CMM2 lacks, and brand-new keywords (steps 1-2 cover those). I re-read nine of the claims below in the code myself: the 6069 seed, the REDIM path, OPTION PROFILING's reset, the out-of-memory text, the Switch Pro type, and others. "L" is a release-notes line.

### 5.1 Gaps and partial items, the most likely to be hit first

| Notes section, line | Behaviour on the PicoMite | CMM2 status | Evidence (C; P) | Size |
|---|---|---|---|---|
| Fixed since RC3, L499 (#105) | `REDIM PRESERVE buf(BOUND(buf())*2)`: each bound evaluated once, before the array goes | PARTLY | the array is erased (C `Src/Commands.c:3746`), then remade from the bound's text (`:3749-3763`). So `BOUND(buf())` fails with "Cannot find", and a FUNCTION in a bound runs twice. P `core/Commands.c:7870-7910`, `:8024` | small |
| Fixed since RC3, L468 | MATH RANDOMIZE and the Mersenne Twister use Knuth's 69069 | GAP | C `Src/MATHS.c:156` still has 6069, so a given seed gives a different sequence. P `core/MATHS.c:138` | small |
| Compiler section, L2237-2273, L2837 | `OPTION PROFILING ON [, SAMPLE]` in a program: counters and a [PERF] report at END | GAP | here OPTION PROFILING is the CMM2's line profiler, saved, and it soft-resets the board (`goto saveandreset`, C `Src/MiscSTM32.c:1523-1531`), even inside a program (no CurrentLinePtr check). `, SAMPLE` is refused. P `core/MM_Misc.c:5429-5449`, `core/Commands.c:4196-4355`. 17 corpus files (benchmarks, Elite's bench copy) use it | small (refusing it in a program); large (a port) |
| Fixed since RC2, L744-751 | Switch Pro is gamepad type 135; MM.INFO(USB n) | PARTLY | the pad works, but its type is 133 (C `Inc/USBGamepad.h:12`; on the PicoMite 133-134 are the touch screens, P `input/USBKeyboard.c:292`). MM.INFO(USB n) is missing (step 2.3) | small |
| Fixed since beta 7, L1433 | sound keeps playing while an image decodes | PARTLY | PNG and JPG are fine. BMP decoding (LOAD BMP, SPRITE LOADBMP, BLIT LOAD, FLASH LOAD IMAGE) never calls routinechecks (C `Src/BmpDecoder.c:180`, `:299`, `:864`, `:946`). P `graphics/BmpDecoder.c:167`, `:287`, `:827`, `:909` | small |
| 6.04 RC2 Changed, L2899 | "Not enough memory for N bytes" | GAP | C `Src/Memory.c:1183` (and `:1208`, `:1235`, `:1291`, `:1318`) says "Not enough heap memory", without a size. P `core/Memory.c:2188-2195` | small |
| Fixed since RC3, L493 (#103) | a name that is both a label and a variable is the label in RESTORE | PARTLY | it works for an array-like name, but a scalar variable of that name is taken as the variable (C `Src/Commands.c:3236-3237`). P `core/Commands.c:8432-8436` | small |
| 6.04, L3182 | SAVE IMAGE writes the colours shown, after MAP too | PARTLY | in MODE n,4, ReadBuffer4 uses the fixed palette ("MAP not applied", C `Src/Rgb121.c:318-333`), which Print Screen also uses. P `misc/FileIO.c:3142-3161` | small |
| New since RC3, L289; L285 | OPTION PRINTSCREEN ON/OFF in a running program; Ctrl-\ (&H1C) as Print Screen | GAP | no PRINTSCREEN in `Src`; only 0x9D is tested (C `Src/main.c:4550`, `Src/Editor.c:276`, `Src/FileManager.c:4047`). P `core/MM_Misc.c:5391`, `misc/FileIO.c:8599-8624` | medium / small |
| Fixed since beta 7, L1500; 6.04 L3137 | MM.INFO(BOOT) gives the reset cause, brown-out included | PARTLY | no MM.INFO(BOOT). MM.INFO(RESET) (C `Src/MM_Misc.c:3314`) has no brown-out case, and `_excep_code` is not cleared on a hardware reset (C `Src/main.c:793-840`). P `PicoMite.c:3723-3730`, `core/MM_Misc.c:8470-8495` | small |
| Fixed since RC3, L353 (#62) | MATH(C_ADD …) and the FFT forms with one argument say "Argument count" | PARTLY | guarded, but the message is "Invalid syntax" (C `Src/MATHS.c:2908-2940`, `:3038`, `:3046`, `:4018-4108`). P `core/MATHS.c:2852-2886` | small |
| Fixed since RC3, L407; RC1, L918 | a program line that is too long says "Line is too long" | PARTLY | over 255 characters is caught (C `Src/SerialFileIO.c:343`, `Src/FileIO.c:629`, `:642`). A line that a #DEFINE grows to exactly 255 gets "Not enough memory" (C `Src/main.c:3870-3875`, `:4188-4193`). P `PicoMite.c:5008`, `:5017` | small |
| Fixed since RC3, L466; 6.04 L2994 | FLASH LOAD IMAGE checks the file before the slot is emptied | PARTLY | the flash slots 1-3 are fine. The RAM slots 4-8 are dropped before the decode (C `Src/ImageSlots.c:487`, and `:537` for DISK LOAD). P `misc/FileIO.c:1294-1310` | small |
| Fixed since beta 8, L1211 | `FLASH LOAD n : LIST` (or RAM LOAD) on one line sees the new program's names | GAP | SlotLoadProgram (C `Src/ImageSlots.c:398-411`) does not repoint SymTabProg, so names list as "?". P `misc/FileIO.c:1013`, `:1634` | small |
| Fixed since RC2, L638 | mounting a drive closes that drive's files, #10 included | PARTLY | the loops run `i<MAXOPENFILES` from 0 (C `Src/FileIO.c:2390`, `:2407`), so file #10 keeps a stale FIL. P `misc/FileIO.c:5857`, `:5882` | small |
| beta 4, L1822 | EDIT and FM switch off an IR receiver left running | GAP | the editor and FM wipe the heap with IR still writing into it (C `Src/Editor.c:211-231`, `Src/FileManager.c:3746-3762`, `Src/Timers.c:380-388`). P `misc/External.c:5775`, `misc/Editor.c:606` | small |
| Libraries, L2957 | `LIBRARY LOAD …, RAM` lasts until END or the next RUN | PARTLY | released at RUN/NEW (C `Src/MMBasic.c:4897`, `:4906`), not at END (`Src/Commands.c:1860`). P `core/Commands.c:4198` | small |
| OPTION SYMBOLS OFF, L1988 | a LIBRARY LOAD may come before OPTION SYMBOLS OFF | PARTLY | SymFindOff stops at the LIBRARY LOAD line: "Must be at the top" (C `Src/Symbols.c:337-339`). P `core/Symbols.c:473-476` | small |
| Fixed since RC3, L395 (#80) | #COMMENT START/END blocks | PARTLY | present, but `#COMMENT  END` with two spaces does not end the block (C `Src/FileIO.c:389`, `:453`, `:550`, `:592`). P `misc/FileIO.c:3509` | small |
| Fixed since beta 2, L2314, L2311 | MM.INFO(VARCNT) right after a failed DIM; no false "Too many global variables" after ERASE | PARTLY | C `Src/MMBasic.c:3988`, `:4058-4072`, `:111`; `:3424-3444`. P `core/MMBasic.c:5262-5268`, `:5301-5341`, `:7517` | small |
| Fixed since RC2, L767, L785, L752 | EDIT: held-key filter; import count; an empty file saved empty | PARTLY | C `Src/Editor.c:3782`, `:4052`, `:4067`, `:2433`, `:610`. P `misc/Editor.c:543`, `:7668`, `:6469` | small |
| Fixed since RC3, L540 | `[USB EP_FULL …]` for a device that does not fit | PARTLY | only a TU_ASSERT (C `Middlewares/.../hcd_dwc2.c:634-635`); the device shows as "not supported" (C `Src/USBKeyboard.c:316`) | small |
| Fixed since RC2, L696-707 | a program too big leaves no program; a damaged program or library is erased at boot | PARTLY | OPTION FLASH keeps the old program (tokenised to SDRAM first); OPTION RAM leaves a partial image (C `Src/main.c:4132-4143`) | medium |
| 6.04 RC2, L2805 | a program's own font left selected does not break the next LOAD | PARTLY (needs a board test) | no PromptFont fallback when the font table is rebuilt (C `Src/MMBasic.c:699-704`) | small |
| Faster in RC3, L571 | complex arithmetic | PARTLY | MATH(C_DIV) divides in float (C `Src/MATHS.c:2933`; P `core/MATHS.c:2964`) | small |
| Fixed since RC3, L361 (#72) | LONGSTRING MID with a huge count clips | PARTLY | `nbr+start` can overflow near 2^31 (C `Src/MM_Misc.c:973`) | tiny |
| Libraries, L2966 | READ falls through to the library's DATA | PARTLY | not with no program in memory (C `Src/Commands.c:2908`) | tiny |
| Fixed since RC3, L547 | OPTION MILLISECONDS: TIME$'s ms start at 0 when the time is set | CAN'T TELL | depends on the RTC's SSR after `HAL_RTC_SetTime` (C `Src/MM_Misc.c:2103-2125`); needs a board | |

### 5.2 Deliberate differences (not gaps)

- A float subtraction that overflows gives INF, as + and * do (Peter, 20 Oct; C `Src/Stream.c:4150-4153`, `Src/Operators.c:105-147`).
- No-op options: OPTION SYMBOLS OFF (C `Src/Symbols.c:326-331`; so the PicoMite's ": put OPTION SYMBOLS OFF first" hint would mislead here) and OPTION LOCAL VARIABLES. OPTION TRACECACHE and CACHE SUB/DEBUG are accepted and ignored (C `Src/MM_Misc.c:2455`), where the PicoMite now says "Invalid Option".
- More permissive: 32-character names (MAXVARLEN 32); `tknbuf` is 512 bytes.
- No LOAD of a program (C `Src/FileIO.c:794-851`), so the LOAD ,C and ,R notes do not apply.
- Some messages differ: OPEN of a directory FOR OUTPUT, WS2812 "Array size", SPRITE LOAD's.

### 5.3 Present (the rest)

Most of the notes are already in the CMM2, each checked with file:line: the RC4 issues #58-#119 (structures, LONGSTRING, CHOICE, STR2BIN, FORMAT$, CRC, RANDOMIZE 0, the sprite and BLIT fixes, PORT() above 31 pins, the seven compiled-code differences of #84, ON ERROR in an interrupt, CONTINUE in a SUB, RAY, STAR/ASTRO, the USB serial fixes), -2^2 = -4 (C `Src/MMBasic.c:2529`, `:2595`; `Src/Stream.c:1451`), I2C/SPI/ONEWIRE READ into a member (C `Src/MMBasic.c:4538-4578`), DRAW3D's fixes, the Print Screen key, the PLAY, gamepad, editor and ZMODEM fixes, arrays (one-element dimensions, whole-array copy, FUNCTIONs returning arrays), libraries (LIBRARY LOAD with 8 files and a hash, CSUBs binary only), image slots and FRAMEBUFFER CREATE 2/MERGE in MODE n,4, OPTION COMPILE/SHADOW and MM.INFO(COMPILE), the stack warning (C `Src/main.c:976`, `Src/MMBasic.c:4252`), "Must be a global variable", and the 6.04 RC2 fixes (MAX/MIN, ABS/SGN, NAN/-INF, integer ^, 32-bit indices, M_INVERSE 1x1, DO with a fractional limit). The evidence, by release-notes line:

| Notes line(s) | Present at (C) |
|---|---|
| L279 READ into a member; L311/314 SPI READ | `Src/MMBasic.c:4538-4578`, `:4563`; `Src/SPI.c:328` |
| L340 LINE PLOT (#60); L343/398 CHOICE | `Src/Draw.c:4317`; `Src/Functions.c:1515`, `Src/Stream.c:4408` |
| L345-349 C_ARG, ARRAY messages, TRIM$ | `Src/MATHS.c:2899`; `Src/Commands.c:4168-4282`; `Src/Functions.c:632` |
| L351, L3241 DRAW3D | `Src/Draw.c:1793`, `:1844`, `:1107`, `:1621`, `:1648`, `:1714` |
| L357/359 STR2BIN, FORMAT$; L361 LONGSTRING | `Src/Functions.c:388`, `Src/MM_Misc.c:586`; `:904-970` |
| L364/367 AES128/BASE64, LINSTR | `Src/MATHS.c:1139`; `Src/MM_Misc.c:1377` |
| L370-461 BLIT and SPRITE fixes (#63-65, #88-92) | `Src/sprites.c:1081-1228`, `:980`, `:1485`, `:556-607`, `:1566-1665` |
| L380 PORT() > 31 pins (#81) | `Src/External.c:946`, `:976` |
| L383/385 trailing comma; MM.END on an implied END | `Src/MMBasic.c:4206`; `Src/Commands.c:1874` |
| L388-401 CRC, RANDOMIZE 0, V_MULT/SHIFT/SINC/WINDOW | `Src/MATHS.c:515`, `:623`, `:151`, `:2714`, `:1890`, `:1614`, `:1509`, `:2663` |
| L409-426 the compiled-code differences (#84) | `Src/Stream.c:5080`, `:4626`, `:1667`, `:3024-3047`, `:2361`, `:1348`, `:3399` |
| L429-472 ON ERROR in an interrupt; SUB at the prompt; CONTINUE in a SUB | `Src/MM_Misc.c:2401`, `:4195`; `Src/MMBasic.c:1258`; `Src/main.c:902`, `Src/Commands.c:1445` |
| L462 BMP checks (#93) | `Src/BmpDecoder.c:150`, `:1492-1540` |
| L480/526 RAY (#97-99) | `Src/Ray.c:906`, `:925`, `:689` |
| L484-531 (#101-117) | `Src/Commands.c:651`, `Src/MMBasic.c:5382`, `Src/Draw.c:3210`, `Src/Commands.c:3506`, `Src/Struct.c:430`, `Src/SerialFileIO.c:92`, `Src/Commands.c:303-344`, `Src/GUI.c:599`, `Src/MMBasic.c:336` |
| L529/732 STAR/ASTRO | `Src/Astro.c:822-867`, `:1148`, `:1177` |
| L533-546 USB serial (#119) | `Src/Serial.c:160-726`, `Src/SerialFileIO.c:156`, `:427`, `:255` |
| L554 Print Screen key | `Src/Console.c:90-107`, `Src/main.c:4550` |
| L577-618 WS2812, RIGHT$, LIST unfolding, LMID, REDIM buffer, CSUB vectors, BMP, PLAY (#17-32) | `Src/External.c:1608`, `Src/Functions.c:1110`, `Src/MMBasic.c:1851`, `Src/Commands.c:2750`, `:3726`, `Src/MMBasic.c:4917`, `Src/BmpDecoder.c:972`, `Src/Audio.c:1727`, `:1913`, `:2014` |
| L626 LOAD JPG checks; L644/672 keyboard, mouse | `Src/FileIO.c:1920-1961`; `Src/USBKeyboard.c:446`, `Src/USBMouse.c:156-245` |
| L659/666 PRINT comma stops (issues 37/38) | `Src/Commands.c:505-574` |
| L716-725 DO in recursion, ON ERROR SKIP level, ON ERROR at the prompt | `Src/Commands.c:2498`, `:2571`; `Src/MMBasic.c:498`; `Src/main.c:859` |
| L728/986/1247 GPS | `Src/Commands.c:477-486`; `Src/GPS.c:119`, `:414` |
| L736-743 gamepads (#43-46); L752-799 editor (#48-57) | `Src/USBGamepad.c:311`, `:206`, `:640`; `Src/Editor.c:886-916`, `:2174`, `:2520` |
| L806 tile maps closed at RUN; L838-860 FM track list | `Src/Memory.c:1075`; `Src/FileManager.c:3513-3620`, `Src/Audio.c:1503`, `:1957` |
| L913-995 (#7-#14) | `Src/MMBasic.c:3621`, `:1719-1723`, `Src/MM_Misc.c:4320-4382`, `Src/Commands.c:4174`, `:3419`, `Src/MATHS.c:676` |
| L1109-1157 ZMODEM | `Src/ZModem.c:1304`, `:466`, `:1344-1362`, `:1256` |
| L1354/1439 LOAD PNG crop, PNG messages | `Src/FileIO.c:1490-1547`; `Src/upng.c:968`, `:1113` |
| L1728-1822 NEXT in recursion, last-line IF, STATIC, duplicate labels | `Src/Commands.c:2312`, `Src/main.c:3909`, `Src/Commands.c:3589`, `Src/MMBasic.c:634-656` |
| L2029-2095 OPTION COMPILE, SHADOW, messages | `Src/MiscSTM32.c:1429`, `:308`; `Src/Stream.c:5653-5732`, `:4106-4146` |
| L2284 stack warning; L2302 "Must be a global variable" | `Src/main.c:976`, `Src/MMBasic.c:4252`; `Src/MMBasic.c:4467` |
| L2783-2826 the 6.04 RC2 interpreter fixes | `Src/MMBasic.c:451`, `Src/Commands.c:3456`, `Src/Functions.c:1491`, `:531`, `Src/MMBasic.c:4704`, `:5150`, `Src/Operators.c:81`, `Src/MATHS.c:2090`, `Src/Commands.c:2439` |
| L2913-2977 arrays and libraries | `Inc/MMBasic.h:174`, `Src/Commands.c:583-668`, `Src/MMBasic.c:958-1010`; `Src/Library.c:559`, `:586`, `:155` |
| L2981-3013 image slots, FRAMEBUFFER CREATE 2/MERGE | `Src/ImageSlots.c:30-35`; `Src/Rgb121.c:521-590` |
| L3109-3262 PLAY SOUND, TILEMAP LOAD, CLAMP, MM.ERRLINE, TURTLE COLOR, HELP file, CIRCLE aspect, REDIM type, JSON$, regex | `Src/Audio.c:1913`, `Src/TileMap.c:411`, `Src/MATHS.c:1657`, `Src/Functions.c:259`, `Src/turtle.c:795`, `Src/Commands.c:245-286`, `Src/Draw.c:5544`, `Src/Commands.c:3747`, `Src/MM_Misc.c:4311`, `Src/re.c:351` |

Skipped as not applicable: WebMite TCP/telnet/TFTP/WEB, PicoCalc (#114), OPTION RADIO/PicoMitePC3, every Bluetooth section, PSRAM, LCD panels, HDMI-only SAVE IMAGE/MAP MAXIMITE, PIO, RP2 clocks and errata, VS1053 (#29), the stepper, the RP2 USB fixes, the flash and build layout, the performance-only sections, documentation and tools.
