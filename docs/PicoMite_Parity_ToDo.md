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

## Awaiting a decision
- [ ] **GETSCANLINE in the 4-bit doubled modes**: found 26 Oct. In the CMM2's modes whose lines are drawn twice it
      counts the mode's lines (MODE 7 and 7,16 to 249, as the manual says), except in the 4-bit depth, which counts the
      screen's (MODE 7,4 and 3,4 to 499). Under OPTION PICOMITE that is what the PicoMite's HDMI versions do in their
      MODE 2. Either make the 4-bit depth count the mode's lines without OPTION PICOMITE, or say in the manual that it
      counts the screen's. (A loop also never sees the lines while the 4-bit merge runs, about 25 to 130 lines.)
- [ ] **OPTION AUTORUN's NORESET**: on the PicoMite a system error (a fault, its restart) turns AUTORUN off unless
      NORESET is given. Here a fault stops the board (HardFault_Handler loops) and the restarts after a command timeout
      or a watchdog timeout keep AUTORUN, so NORESET changes nothing yet. Either leave it so, or turn AUTORUN off after
      a command-timeout restart without NORESET (a change for programs that rely on it now).
- [ ] **OPTION RTC AUTO ENABLE | DISABLE**: recommended as its own option (G2), the PicoMite's meaning: the internal
      clock set from the DS3231 at start-up and every 24 hours, TIME$ and DATE$ read from the internal clock, TIME$ =
      and DATE$ = setting both; OPTION DS3231 ON kept as it is, the two excluding each other. Also: the DS3231 timer copy
      runs every 100 hours (`Src/Timers.c:168`), probably meant hourly.
- [ ] **RUN f$** with a variable or an expression (gap 2).
- [ ] **MM.INFO keys**: LINE, AUTORUN on its own, CALLTABLE, MAX VARS, SYSTEM HEAP, OPTION AUDIO | WIDTH | HEIGHT |
      KEYBOARD (gap 18).
- [ ] **Line length**: found on 26 Oct, the CMM2 takes program lines up to 251 characters (248 from line 1000 on),
      the PicoMite up to 254: the file reader counts the CMM2's longer line marker. Leave, or lengthen the buffer?
- [ ] **WATCHDOG HW**: decision deferred, as the STM32's watchdog cannot be stopped once started (OFF would have to be
      the firmware feeding it every millisecond).

## Deferred
- ONESHOT and ADC RUN (gaps 14, 24): nice to have.
- TRIANGLE SAVE / RESTORE (gap 11); BLIT MEMORY332 (gap 12).
- The PicoMite's FRAMEBUFFER forms in the 8, 16 and 32-bit modes (gap 1).

## Checks still wanting hardware (fixes made on 26 Oct)
- [ ] The Switch Pro Controller reports 135 in DEVICE(GAMEPAD n, T).
- [ ] An IR remote left running into EDIT or FM.
- [ ] EDIT: a held arrow stops when let go; a paste is kept whole; an import cut short by memory gives its count.
- [ ] `[USB EP_FULL ...]` with more USB devices than the host's endpoint table holds.
- [ ] OPTION RAM: a program too big for memory leaves no program.

## Before the release candidate
- [ ] Release notes for the next beta: the stepper, GP names in every mode, SAVE PNG and Print Screen, the 64-bit
      GPIO masks, the behaviour fixes (and the full notes from 5.07.02); no repository references.
- [ ] Stepper_Reference.pdf in the download with the manual and the binary.
- [ ] The repository made public.
