# PicoMite parity: the to-do list (26 Oct 2026)

Made from Peter's decisions of 26 Oct 2026, which are recorded with their details, file references and reasons in
docs/PicoMite_Parity_Recheck.md (the decisions block at its top). Gap numbers are that document's. Sizes: tiny (an
hour or less), small (a few hours), medium (a day or so).

## To implement

Suggested order: the tiny and small items first, then those that need wiring or a board test.

### Graphics and files
- [x] **BLIT COMPRESSED** with BLIT MEMORY's code, bit 15 taken as set (gap 12). Tiny.
- [x] **SAVE COMPRESSED IMAGE** as another name for SAVE IMAGE, and **SAVE IMAGE compressing losslessly where BMP
      can**: RLE4 in the n,1 and n,4 modes, RLE8 in n,8; 16 and 32-bit as now. LOAD BMP reads both already (gap 13).
      Small to medium.
- [x] **BLIT RESIZE** (gap 12). Small.
- [x] **AUTOSAVE APPEND** (gap 27). Small.
- [x] **OPTION DEFAULT COLOURS fc [, bc]** with the PicoMite's 16 colour names; MODE and leaving OPTION PICOMITE must
      stop setting the defaults back to white on black (gap 22). Small.

Done 26 Oct 2026 (tests/cmm2/gfxsave 22 of 22; OPTION DEFAULT COLOURS and AUTOSAVE APPEND checked at the prompt):
SAVE IMAGE chooses by the area's colours in every mode, not by the depth (16 or fewer RLE4, up to 256 RLE8, more
24-bit), and LOAD BMP gives the same pixels back; BLIT RESIZE in MODE n,4, as BLIT FRAMEBUFFER; AUTOSAVE APPEND adds to
the file AUTOSAVE writes. Found on the way: the 8-bit modes' colour tables (Inc/CLUT.h) had 13 wrong entries (OPTION
VGA OUTPUT HIGH's RGB(0, 36, 0) showed nearly black), corrected. Flash: bank 1 was full; the BMP decoder and the PNG
encoder moved to bank 2, leaving about 4.5 KB in each bank, which the rest of this list will need more than.

### I/O
- [x] **BITSTREAM's 4th argument (mode) and its dual-pin form** (gap 4). Small to medium; the loop-back wiring tests it.
- [x] **DEVICE SERIALRX and DEVICE SERIALTX** (gap 3). Medium; SERIALTX can use the stepper's cycle-timed `serialtx`.
- [x] **SETPIN pin, ARAW**: one sample, no averaging, PIN() the count; 12 bits (0-4095) with OPTION PICOMITE, 16 bits
      without; AIN unchanged (gap 24). Small.
- [x] **OPTION KEYBOARD nn [, capslock] [, numlock] [, repeatstart] [, repeatrate]**, the PicoMite's form of OPTION
      USBKEYBOARD (gap 22). Small to medium.

Done 26 Oct 2026 (tests/loopback/ioserial 23 of 23; OPTION KEYBOARD checked on the G2, the locks by KEYDOWN(8)):
BITSTREAM's times go to 65535 us (TIM17's period was 50000, so a longer time never ended); mode 1 and the two-pin form
wait before each change, as the PicoMite's do; SERIALTX is the stepper's cycle-timed transmitter, SERIALRX is timed by
the cycle counter and keeps to the string's LENGTH; OPTION KEYBOARD's locks left out are Caps Lock off and Num Lock on,
as the PicoMite's, OPTION USBKEYBOARD sets both off again, and its GR is DE here. Flash: bank 1 was down to 524 bytes;
help.h's topic names lost their padding to 25 characters (the HELP command's matching takes a name's end as spaces),
which left 12 KB in bank 1 and 4.5 KB in bank 2.

### Language and system
- [x] **LIST VARIABLES [array], LIST OPTIONS, LIST PINS, LIST COLLISIONS** (gap 16). Small each.
- [x] **PEEK(BP var), PEEK(WP var), PEEK(SP var)** (gap 17). Small.
- [x] **MM.USB**: the last USB key's code (gap 19). Tiny.
- [x] **MM.DISPLAY** returning 1 (gap 19). Tiny.
- [x] **CPU SLEEP s** done as PAUSE s * 1000 (gap 20). Tiny.
- [x] **MM.INFO(RESET) with the brown-out cause, and MM.INFO(BOOT) as another name for it**, in the CMM2's words
      ("Switch", "Power-On", "Software", and "Brown-Out" added) (gap 18). Small.
- [x] **MM.INFO$(PLATFORM)** returning "CMM2" (gap 22). Tiny.
- [x] **OPTION F1 and OPTION F5 to F9**: the PicoMite's function-key strings, beside the CMM2's F11 to F20 (gap 22).
      Small.
- [x] **OPTION LOGGING ON | OFF**: each error message appended to A:/log.txt; cleared when a program starts; not saved
      (gap 22). Small.
- [x] **OPTION AUTORUN n [, NORESET]**: slot n's program at power-up; NORESET keeps AUTORUN after a system error (gap
      22). Small.
- [x] **OPTION SCREEN KEYBOARD** (gap 25): accepted and ignored on 26 Oct, before the CMM2 had a touch screen; since
      27 Oct the PicoMite's on-screen keyboard (below, From users' reports).

Done 26 Oct 2026 (tests/cmm2/langsys 31 of 31; OPTION F1 and F5 to F9, LIST OPTIONS and AUTORUN 2 run from slot 2 at
a CPU RESTART checked at the G2's prompt): LIST VARIABLES gives the type as a word always (the records here do not keep
whether a name had its suffix); LIST PINS lists the connector's pins with their GP names; MM.USB is the HID code of
the last key; OPTION F1 and F5 to F9, set, replace the keys' own strings (FM, AUTOSAVE "" ...), and "" gives them back
(Option.layout 2: six strings appended); OPTION AUTORUN n is kept in Option.Autorun's bits, so a saved ON or OFF keeps
its meaning; OPTION LOGGING writes the message alone (the PicoMite writes its whole buffer: its #139). NORESET is kept
and listed only: what it should change is for Peter (below). Found on the way: sortStrings swapped the strings through
a 16-byte buffer (the pointers now); MM.INFO(PIN n) gave a period, counting or interrupt input as "Reserved".
Flash: 7,972 bytes left in bank 1, 4,632 in bank 2.

### With OPTION PICOMITE (the CMM2's own behaviour without it)
- [x] **MM.INFO(CPUSPEED)** as a string (gap 2). Tiny.
- [x] **KEYDOWN**'s shifted F-keys adding &H40, not &H20 (gap 2). Tiny.
- [x] **LOAD PNG**'s fourth argument as the alpha cut-off, not the transparent colour's index (gap 2). Small.
- [x] **GETSCANLINE** rescaled so that 0 is the first visible line (gap 2). Small.

Done 26 Oct 2026 (tests/picomite/pmdialect, under OPTION PICOMITE HDMI): MM.INFO(CPUSPEED) is the clock as a string;
LOAD PNG's t and cutoff are the PicoMite's. Two of these items had the machines the wrong way round, and the code
showed it: the PicoMite's LOAD PNG t is the colour of the cut pixels (-1 to 15, 0 if left out, cutoff 20), and the
CMM2's the alpha cut; KEYDOWN adds &H20 for Shift with a function key on both (the CMM2's keyboard map is the
PicoMite's; it was the old CMM2 that added &H40), so nothing changed; GETSCANLINE already counts as the PicoMite's
HDMI versions do under OPTION PICOMITE HDMI: the screen's lines from the first visible one, in every mode (their
MODE 2, the CMM2's MODE 7,4, to 499 here), so nothing changed either (the 0 to 525 came from the old manual; the
PicoMite's VGA versions count from the vertical sync). Flash: 7,684 bytes left in bank 1, 4,632 in bank 2.

## From users' reports
- [x] **OPTION PICOMITE's resolution** (26 Oct: with DEFAULT MODE 9 the programs ran at 640x480, as OPTION PICOMITE HDMI
      1024x768 without its comma had been taken as HDMI at 640x480): with no resolution given it is OPTION DEFAULT MODE's,
      so the monitor keeps one timing; a resolution without the comma is refused; the PicoMite's OPTION RESOLUTION sets
      it; OPTION LIST shows CURRENT PICOMITE resolution and mode, and names the 1 and 4-bit depths. Checked on the G2.

- [x] **SETTICK after an error in an interrupt** (27 Oct, Gerry, on both machines): an interrupt that cut a PAUSE short
      and ended in an error left PAUSE's "carry on" flag set, so the next run's first PAUSE ended at once and its
      SETTICK interrupt was not called. The flag is cleared at RUN with InterruptReturn (ClearStack); GUI CLICK's
      phase, the same pattern, already was (ResetGUI, since 21 Oct). tests/cmm2/pauseint; Gerry's program right on
      every run on the G2. PicoMite: its #140.
- [x] **Interrupt events from before their interrupt was armed** (27 Oct, the review after Gerry's report): the
      Nunchuk/Classic Z and C, the mice's buttons (CONTROLLER MOUSE OPEN's and MOUSE INTERRUPT ENABLE's) and the
      frame were recorded whether or not their interrupt was armed and cleared only when it was dispatched, so a
      program that armed one later was called at once for a press, or a frame, long before. Each is now recorded
      only while its interrupt is armed and cleared at RUN; ON KEY ascii's and MODE's are also cleared when armed.
- [x] **USB touch screens and the GUI's touch, mouse and programmed cursor** (27 Oct, Peter: "The GUI support allows
      mouse, touchscreen or programmed cursor movement. We should do the same"):
  - **The touch code:** the PicoMite's, copied intact by tools/port_usbtouch.py (Src/USBTouch.c): TOUCH() with X2/Y2,
    XN/YN and the gestures, its own token now.
  - **The WCH controller (27C0:0859):** gave multi-touch only once TinyUSB read each string descriptor whole, not two
    bytes first (usbh.c; PicoMite #141, TinyUSB issue 4346).
  - **Fixed in the copy:**
    - contacts ordered by contact ID (PicoMite #144: that panel puts a new finger first);
    - a touch screen's IDs on its own channel (PicoMite d34fd21 and #142).
  - **The GUI review's changes:**
    - GUI CURSOR as the PicoMite's on the CMM2's drawing;
    - the GUI's pointer as GUI CURSOR ON : LINK MOUSE while a mouse is present;
    - GUI CLICK at the cursor's point;
    - a press owned by what made it;
    - CLICK() the PicoMite's;
    - GUI TEST LCDPANEL/TOUCH;
    - MsgBox's refusal after a programmed click;
    - GUI CLICK PIN let go at RUN;
    - the touch clock unaffected by TIMER.
  - **The on-screen keyboard:** the PicoMite's, by tools/port_osk.py (Src/OSK.c, bank 2): OPTION SCREEN KEYBOARD n [,M],
    KEYBOARD ON/OFF, and the editor's swipes and taps. KEYBOARD ON's message is fixed in the copy (PicoMite #143).
  - **Kept the CMM2's (Peter):** the slider drag; no keyboard in MODE n,1; OPTION STATUS's line above it at the prompt;
    the console above it in whole rows.
  - **Other changes:** DrawRBox is the PicoMite's, and MiscSTM32.c, flash.c and SerialFileIO.c are compiled at -Os for
    flash room.
  - **Checked on the G2** with Peter's three touch screens and a mouse.

## After the beta (Peter's decisions, 27 Oct, for 7.00.00b4)
- [x] **GETSCANLINE in the 4-bit doubled modes** (decided: report as the PicoMite does, after the beta): found 26 Oct. In the CMM2's modes whose lines are drawn twice it
      counts the mode's lines (MODE 7 and 7,16 to 249, as the manual says), except in the 4-bit depth, which counts the
      screen's (MODE 7,4 and 3,4 to 499). Under OPTION PICOMITE that is what the PicoMite's HDMI versions do in their
      MODE 2. Either make the 4-bit depth count the mode's lines without OPTION PICOMITE, or say in the manual that it
      counts the screen's. (A loop also never sees the lines while the 4-bit merge runs, about 25 to 130 lines.)
      Done 1 Nov: Peter's "getscanline should report the same as Picomite in the doubled modes" and "should apply
      only under picomite mode" together: under OPTION PICOMITE the doubled modes count the screen's lines, as the
      PicoMite's do (MODE 2 and 5 to 499); without it every depth counts the mode's (ModeRowScale, not page 0's
      expand, which the merged 4 and 1-bit modes leave 0), so MODE 7,4 and 3,4 to 249 as 7,8. Measured on the G2:
      7,8 / 7,16 / 3,4 249, 7,4 240 (the merge), 8,8 499; under OPTION PICOMITE HDMI MODE 1 499, 2 480, 5 499.
- [ ] **OPTION AUTORUN's NORESET** (decided: the CMM2 to behave as the PicoMite, after the beta): on the PicoMite a system error (a fault, its restart) turns AUTORUN off unless
      NORESET is given. Here a fault stops the board (HardFault_Handler loops) and the restarts after a command timeout
      or a watchdog timeout keep AUTORUN, so NORESET changes nothing yet. Either leave it so, or turn AUTORUN off after
      a command-timeout restart without NORESET (a change for programs that rely on it now).
- [ ] **OPTION RTC AUTO ENABLE | DISABLE** (after the beta; the DS3231 copy made hourly for b4, from the routinechecks): recommended as its own option (G2), the PicoMite's meaning: the internal
      clock set from the DS3231 at start-up and every 24 hours, TIME$ and DATE$ read from the internal clock, TIME$ =
      and DATE$ = setting both; OPTION DS3231 ON kept as it is, the two excluding each other. (The DS3231 timer copy
      has been hourly since b4, `Src/Timers.c:169`; it was every 100 hours.)
- [x] **RUN f$** with a variable or an expression (gap 2). After the beta. Done 1 Nov: the name is an expression when
      it has a string name in it (RUN f$, RUN "GAME" + STR$(n), RUN MID$(a$, 3), text) or a quoted name with more
      than a comma after it; anything else unquoted is still MM.CMDLINE$'s text alone (RUN -f), where the PicoMite
      reads every argument as an expression. The text after the comma is still taken as it is, not evaluated.
      Tested on the G2 with six programs and at the prompt.
- [x] **MM.INFO keys** (after the beta): LINE, AUTORUN on its own, CALLTABLE, MAX VARS, SYSTEM HEAP, OPTION AUDIO | WIDTH | HEIGHT |
      KEYBOARD (gap 18). Done 1 Nov: LINE gives the number an error there would give ("LIBRARY", "UNKNOWN" at the
      prompt); CALLTABLE the table in the PicoMite's layout (CallTablePS under OPTION PICOMITE, else CallTableP);
      MAX VARS; SYSTEM HEAP what malloc can still have (syscalls.c); OPTION WIDTH and HEIGHT the console's; OPTION
      AUDIO "DAC" ("PWM" under OPTION PICOMITE); OPTION KEYBOARD the PicoMite's layout name ("GR" for DE, "" for
      none). OPTION AUTORUN and KEYBOARD already matched. Checked on the G2.
- [ ] **Line length** (to be reviewed in more detail after the beta): found on 26 Oct, the CMM2 takes program lines up to 251 characters (248 from line 1000 on),
      the PicoMite up to 254: the file reader counts the CMM2's longer line marker. Leave, or lengthen the buffer?
- [x] **WATCHDOG HW**: not needed (Peter, 27 Oct), as the STM32's watchdog cannot be stopped once started (OFF would have to be
      the firmware feeding it every millisecond).

## Deferred
- ONESHOT and ADC RUN (gaps 14, 24): nice to have.
- TRIANGLE SAVE / RESTORE (gap 11); BLIT MEMORY332 (gap 12).
- The PicoMite's FRAMEBUFFER forms in the 8, 16 and 32-bit modes (gap 1).

## Checks still wanting hardware (fixes made on 26 Oct; deferred to after the beta)
- [ ] The Switch Pro Controller reports 135 in DEVICE(GAMEPAD n, T).
- [ ] An IR remote left running into EDIT or FM.
- [ ] EDIT: a held arrow stops when let go; a paste is kept whole; an import cut short by memory gives its count.
- [ ] `[USB EP_FULL ...]` with more USB devices than the host's endpoint table holds.
- [ ] OPTION RAM: a program too big for memory leaves no program.

## Before the release candidate
- [x] Release notes for the next beta (7.00.00b4, 27 Oct): the stepper, GP names in every mode, SAVE PNG and Print Screen, the 64-bit
      GPIO masks, the behaviour fixes (and the full notes from 5.07.02); no repository references.
- [x] Stepper_Reference.pdf in the download with the manual and the binary.
- [x] The repository made public (27 Oct, with b4).

## Found on 1 Nov 2026 (the cmm2-welcome, Wolf3D and Gauntlet reviews)
- [x] **A FOR or DO right after a single-line IF's THEN or ELSE**, inside another loop: the outer loop took the inner
      NEXT or LOOP for its own, and stopped with "Cannot find a matching FOR" when the IF was false (Gauntlet's status
      panel). ForFindNext and DoFindLoop now count it. 9a168da; tests/cmm2/forif; PicoMite #147.
- [x] **FM's auto-repeat for Up and Down** with a USB keyboard (a user's report): the redraw's drain reset the repeat
      at every step. PgUp and PgDn still do not repeat. 9a168da; PicoMite #148.
- [x] **MM.ERRNO and MM.ERRMSG$ at the prompt**: the status line, drawn each second, called InitSDCard, which cleared
      them, so a program's error was gone a second later. PrintStatus keeps them.
- [x] **Print Screen saves the screen as shown**: the page each LTDC layer shows (PAGE DISPLAY, double buffering),
      not the read page, and in the 12-bit depth both layers blended over the background as the LTDC blends them.
- [x] **USB connect and disconnect notes printed between programs**: a program's RUN loads the next one with no line
      running, so a touch monitor's reconnect after the MODE change was printed into the program's output.
      With RunStarting, set from the RUN until the program starts, they are left out, as while a program runs.
- [x] **tools/goldens.py** ignores those notes in a capture.
- [ ] **FM: sound left running after a program crashes** (the same user's report): not reproduced; waiting for the
      details.
- [x] **A file command on C: with no USB flash drive says "SD Card not found"** (MKDIR "C:", found by SP Tools'
      tst_file): FR_NOT_READY's message is the SD card's. It should say "USB drive not present", as DRIVE "C:" does.
      Done 1 Nov: disk_initialize notes the drive that was not ready (mmc_stm32.c) and FileErrorNo (FileIO.c) gives
      error 21 for C:, for ErrorCheck, InitACard, InitUSBDrive and FM's copy. MKDIR, OPEN and DRIVE on C: checked.
- [x] **Flash room** (Peter: "change some functions to Os"): bank 1 had 16 bytes left. OPTSIZE (Configuration.h) makes
      11 large functions whose speed does not matter -Os in their -O2 files: main, SaveProgramToFlash and
      SaveProgramToMemory, EditInputLine, cmd_option, fun_info, cmd_list, do_listfiles, cmd_save, SerialOpen and
      setmode. Bank 1 now has about 6 KB; bank 2 has 216 bytes.
- [x] **MM.INFO(FONTHEIGHT) and MM.FONTHEIGHT for font 7** (found with Z-MIM, whose README lists "*FONT SMALL on the
      Colour Maximite 2 causes the prompt to glitch"): the CMM2 spaces font 7's lines 10 apart (SetFont, as 6.00.00b7
      did) but reported the font's 8, so a program placing lines by it (Print @(x * w, row * h)) drew them off its rows.
      Both now give gui_font_height, the spacing (8 under OPTION PICOMITE, as on the PicoMite, which has no such case).
      Z-MIM's small font checked on the G2; fonts89, datakw, libcsub and picomode still match.
- [x] **A variable named after a command** (NOMIS, Bill Leue 2023: "flash = 1" was the FLASH command, added in 7.00,
      "Syntax"). Peter's rule: at the start of a statement NAME = x (and NAME$, NAME%, NAME! = x) is an assignment,
      for any command, except where the name is a keyword, operator or function too (FOR, GOTO, TIMER, DATE$) or NAME =
      is the command's own (FLAGS, CASE; REM, RUN, DATA). A tokeniser change only (CmdNameVar, MMBasic.c): the line runs
      as any assignment. The array form (NAME(bounds) = x) was dropped: it made IF (x) = y THEN, ELSEIF, WHILE and PRINT
      assignments, and CASE = 5 was missed (another session's review, CMM2V1.5 issue #10); arrays are TILE, DEVICE and
      RESOLUTION's as before. tests/cmm2/cmdvars has those lines; the manual's naming rule updated. NOMIS then runs; it
      needs MODE 1 (800x600), the CMM2's default, which the G2 now has (OPTION DEFAULT MODE 1,8, Peter, 1 Nov).
      Raised for the PicoMite as #149.
- [x] **MM.MOUSE** (Peter, found with BoxDot, whose CONST mouseport = 3 named a Hobbytronic channel the USB mouse is not
      on, "Sorry, this program needs a mouse"): the channel of the mouse to use, so a program fixes none. 2 for a USB
      mouse; else the open one's (0-3); else OPTION MOUSE's (0 if the PS/2 mouse answers, MouseOptionStart; 1-3
      unprobed, for OPEN to check: Peter); else -1. It opens nothing. A fun_tilde code (V) appended before MMEND; the
      compiler leaves it to the interpreter. DEVICE(MOUSE n), MOUSE INTERRUPT and MOUSE SET take n = 0 to 4 (they act on
      the mouse in use), so MM.MOUSE's 0 works with them. tests/cmm2/mmmouse; manual entries. NOMIS (A:/nomis) and
      BoxDot (A:/boxdot) changed on the card to use it, and both start on the G2's USB mouse.
- [x] **The G2 hung after OPTION DEFAULT MODE's restart** (1 Nov): no console and no display until an ST-Link reset.
      Proven: reproduced with a Ctrl-C (a serial call's first byte) about two seconds into the restart; the ST-Link probe
      found the CPU in HardFault_Handler with SP 0xFFFFFF98 and CFSR INVSTATE + IMPRECISERR, the marks of a longjmp
      through main's still-empty jmp_buf (SP 0, PC 0). The path: start_console() starts the console's reception, then
      InitDisplayOther's setmode (a new mode) waits for a frame in reset_CLUT with while(!pagesetdone) CheckAbort(); the
      Ctrl-C's MMAbort made CheckAbort call cleanend(), whose longjmp(mark) ran before main's setjmp(mark). Fixed:
      MarkSet (main.c), set on setjmp's first pass; CheckAbort acts on MMAbort only then, and a Ctrl-C typed during
      start-up is dropped with the rest of the input. A USB keyboard's Ctrl-C (KeyboardMap.c) is covered too, which
      disabling the console interrupt would not have been. The failing sequence then passed 4 times of 4. (error() before
      setjmp would still jump through the empty jmp_buf; nothing calls it there in practice.)
- [x] **MM.INFO(SOUND)'s strings follow OPTION PICOMITE** (Peter, 1 Nov, reversing 16 Oct's "the PicoMite's
      throughout"): Into the Darkness stopped with "Samples play over MOD file" (SOUND.INC tests ="MODFILE", else PLAY
      STOP and PLAY WAV, which ended the module before PLAY MODSAMPLE); Wolf3D and Gauntlet make the same test and lost
      their music at the first effect. By default "MODFILE" and a bare "PAUSED" for any pause, as 6.00; under OPTION
      PICOMITE the PicoMite's "MOD" and "PAUSED WAV" and so on. The states 6.00 had not (ARRAY, SAMPLE, BBC) keep their
      names. Done after the switch (MM_Misc.c), so the PicoMite's table stays as it was. tests/cmm2/sndinfo and
      tests/picomite/pmdialect check a paused tone; checked on the G2 with a module, an EFFECT over it and a WAV in both
      dialects. Into the Darkness then plays its first stage to the end without an error, its effects by PLAY EFFECT over
      the module, as on 6.00.
- [x] **The mouse and the GUI cursor centred by a mode change** (Peter, 1 Nov: "the cursor should re-center on any mode
      change"). Found with Cuno (Bill Leue): its GUI CURSOR ON 1, MM.HRES\2, MM.VRES\2 then GUI CURSOR MOUSE(X), MOUSE(Y)
      put the cross at (400, 300), the USB mouse's place since it was attached in MODE 1 (USBMouseAttached centres it;
      CONTROLLER MOUSE OPEN 2 sets only interrupts). MouseModeChange (USBMouse.c), at the end of setmode's change path
      (ltdc.c): every channel's ax, ay and the cursor's point at HRes/2, VRes/2; an open I2C mouse's adapter is sent
      registers 20 to 28 again (MouseI2CScreen, MiscSTM32.c, with OPEN's sensitivity kept), as its OPEN does, since it
      holds the position and the screen's size and kept to the old screen. The I2C part is untested: no I2C mouse here.
      tests/cmm2/mmmouse checks MODE 9's centre; checked on the G2 with the USB mouse in MODEs 9, 16, 7 and back to 1,
      and Cuno's cross starts at (960, 540). Cuno also needs MODE 16 (its MODE 15 is 1280x1024, its layout 1920x1080):
      changed on the card, with MM.MOUSE.
- [x] **The compiler's value splice on (RB_SPLICE 1)**, in 7.00.00b5 (Peter, 1 Nov, after a user's lifepx ran twice as fast on a
      PicoMite: its OPTION COMPILE ON compiles PIXEL and PIXEL(, the CMM2's left them as text). lifepx's row, 318 cells
      of five PIXEL() reads and a PIXEL: PicoMite PC3 378 MHz 29.0 ms as text, 9.6 compiled; CMM2 G2 20.2 as text,
      18.8 compiled with the splice off, 7.8 with it on (PIXEL() 6.1 to 3.5 us, PIXEL 7.4 to 4.4, LINE 14.0 to 8.4).
      Not frame blanking: MODE 7,8 is no slower than 8,8 or 1,8. The review of the spliced handlers against the CMM2's:
      every one reads its values through getargs and getint, getinteger, getnumber, getColour, getCstring or
      getargaddress's literal path (a T_VALUE is no name); their `*argv[n]` tests are for an empty argument, which stays
      empty; OPTION LEGACY's syntax is only PIXEL's, CIRCLE's and LINE's, which RC_GUARD sends to the text; the CMM2's own
      keyword forms (BOX AND_PIXELS, COLOUR MAP, PIXEL FILL) have arguments after the keyword, so are no one expression
      and stay text; PIXEL( with a page and RGB( with four stay text; the fold selectors are the CMM2 tokeniser's. One
      fault: TEXT's justification as a bare word went in as its symbol, which GetJustification (not symbol-aware) cannot
      read, so it compiled as a variable's value; now its spelling (RBBareJustify). tools/rb_cmm2.py has four splice
      programs (TEXT's justification, the graphics commands, the functions, OPTION LEGACY); tools/rbtest.py drops USB
      notes and reads on to a program's STAT line (a touch monitor reconnecting after a mode change ended a run early).
      rbtest: 409 programs, no difference (the seven first seen were the harness). It costs 4 KB of bank 1 (1288 left).
      The regression with OPTION COMPILE OFF, ON and SHADOW (G2, 1-2 Nov): tests/cmm2, tests/picomite, the goldens and
      Testfiles print the same every way, SP Tools ON as OFF; the goldens' two differences were solar_eclipse's printed
      time and longname's error message, which printed the stack after a 32-character name in OFF too: findvar left such
      a name unterminated (MMBasic.c, since the 6.00.00b7 baseline; the PicoMite's core/MMBasic.c has the same guards).
      Fixed, with tests/cmm2/name32. solar_eclipse compiled: 2.21 s with the splice off, 2.19 with it on (an ASIN loop
      5.6 to 3.7 us).
- [x] **Every statement calls check_interrupt() while a USB touch screen is present and OPTION MAXCTRLS is above 0**
      (found 2 Nov by bisecting solar_eclipse's compiled time: 1.71 s up to 340e1c0, 2.20 s from bc781eb, the touch
      port). GUI_INPUT_ON (GUI.h) is GUIactive || (usb_touch_present && Option.MaxCtrls > 0) || osk_state, and both
      MMBasic.c's per-statement check and Stream.c's RBTail called check_interrupt() on it; RB_HOUSE_DUE was then
      always true, so the compiled chain never ran one statement into the next. Fixed 2 Nov: the 1 ms tick sets
      IntReady.gui while GUI_INPUT_ON or a linked cursor, check_interrupt clears it, and the statement tails test
      IntReady.any alone: the GUI is seen to once a millisecond. G2, MAXCTRLS 20 and the GUI started, ns a pass of
      a = a + 1: 4096 as text and 2032 compiled before, 3508 and 680 after (b5 with MAXCTRLS 0: 3581 and 745);
      solar_eclipse 2.19 s compiled before, 1.70 after. tests/cmm2 and tests/picomite OFF, ON and SHADOW: no
      differences. Still to do: Peter's GUI checks by hand on the G2 (finger, mouse, the OSK, a spinner's repeat, a
      slider's drag). The PicoMite's text loop calls ProcessTouch (in flash) after every statement while Ctrl or a USB
      touch screen is there: likely the same cost, not measured.
- [x] **RUN f$ against SP Tools and the Welcome Tape** (Peter's check): every RUN either uses is a quoted name and a comma,
      the old path, or RUN prog$ (sys.run_shell), which now runs prog$. Checked on the G2: SP Tools matches its baseline
      but for tst_file, which needs a USB flash drive on C:; sptest's own chain in spflow, sptrans and splib; the menu's
      launch and return with its label.
