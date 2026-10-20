# Code issues found by the manual audit (20 Oct 2026)

*The SP Tools regression of 20 Oct (70 tests lost in tst_map, tst_set, tst_list and tst_history) is not a firmware fault: SP Tools' error reporter needs the CMM2 6.00 form of MM.ERRMSG$, "Error in line N: ...", and since cd87a88 (19 Oct) MM.ERRMSG$ is the bare message, as on the PicoMite. tools/sptools.py now patches the reporter.*

Found while the reference sections were checked against the code (docs/Manual_Plan.md, M4), and fixed on 20 Oct 2026
(Peter: "Fix all of the other issues found"), except OPTION EXPLICIT at the prompt (by design) and the Xbox controller
(waiting on hardware), which are not in this list. Each item was confirmed in the source before it was fixed;
tests/cmm2/m4fixes.bas checks those that a program can see with nothing wired, CAN in the internal loop back.

The PicoMite shares 23 of them (checked against its code at 85a90a6): GitHub issues #60 to #82 on UKTailwind/PicoMite.
Not shared there: CALL's split, PIXEL FILL (the PicoMite has FILL, checked), PRINT @'s column and the 3D cameras (its
MAXCAM is 3).

Where the manual already describes the intended behaviour (marked **manual: intended**), the manual is right only once the code is fixed. The other entries follow the code as it is.

## Serious: data broken, or memory overwritten

| # | Where | What goes wrong | Status |
|---|---|---|---|
| 1 | Src/CAN.c:274, 367, 397 | Since the HAL 1.11.6 update (2072b14) the HAL takes `DataLength` as a plain DLC code (`FDCAN_DLC_BYTES_8` is 8). CAN.c still passes `dlc<<16`, so CAN SEND makes the HAL read `DLCtoBytes[]` far out of bounds while copying the frame, which can overwrite memory. CAN READ shifts `DataLength` again, so it always reports 0 bytes. Our regression. | Fixed |
| 2 | Src/sprites.c:932 | SPRITE LOAD appends ".SPR" to `p` (the statement in program memory) instead of `fname`: a name without an extension is not found, and the append writes past the statement. **manual: intended** | Fixed |
| 2b | Src/Draw.c:343 | GUI CURSOR LOAD has the same slip as SPRITE LOAD: `strcat(p, ".CUR")` appends to the statement, not to the file name (and each load leaks the previous cursor's data). | Fixed |
| 2c | Src/MM_Misc.c:501 | DATETIME$(n) refuses every number: `getint(ep, 0x80000000, 0x7FFFFFFF)` makes the minimum +2147483648 ("0 is invalid (valid is 2147483648 to 2147483647)"). | Fixed |
| 2d | Src/Functions.c:389 | STR2BIN copies the whole string (up to 255 bytes) into an 8-byte union before checking its length: a long string overwrites the stack. The PicoMite has the same. | Fixed |
| 2e | Src/MM_Misc.c:585 | FORMAT$ sprintf's into a 256-byte buffer with no limit on width or precision: FORMAT$(1, "%300f") overruns it. | Fixed |
| 3 | Src/OtherDisplays.c:261, 297, 334, 369, 437 | PAGE AND_PIXELS, OR_PIXELS, XOR_PIXELS, MERGE and STITCH take the width (MERGE and STITCH the height too) from the write page, not from the pages they combine: with a larger write page (PAGE RESIZE, FRAMEBUFFER WRITE) they overrun memory. | Fixed |
| 4 | Src/Draw.c:994-1008 | FRAMEBUFFER CREATE over an existing framebuffer keeps the old backup: a later BACKUP or RESTORE copies the new size through the old allocation. | Fixed |
| 5 | Src/Serial.c:606 | Opening COM3 sets `com2Tx_tail = 0` instead of `com3Tx_tail`: it disturbs a running COM2 and leaves COM3's tail stale. | Fixed |
| 6 | Src/PWM.c:274, 305, 370 | Changing the duty of a running output 1B, 1C or 2B writes CCR1 (output A's register) instead of CCR2/CCR3. | Fixed |
| 7 | Src/CAN.c:674 | `speed==10000000`: CAN OPEN at 1000000 bit/s sets no timing. | Fixed |
| 8 | Src/sprites.c:279 (checklimits) | The screen-edge collisions are added to `collisions[]` (MAXCOLLISIONS+1 bytes) without the limit the sprite collisions have: up to four more past the seven. | Fixed |
| 9 | MM_Misc.c:906 | LONGSTRING LOAD with `nbr` longer than the string copies `nbr` and can write past the end of the array; a negative `nbr` in LOAD, LEFT, RIGHT or MID runs away. | Fixed |
| 10 | Src/MM_Custom.c (ADC) | With a trigger set, the ADC interrupt writes to the arrays of channels that ADC START was not given (NULL pointers). | Fixed |
| 11 | Src/sprites.c (SPRITE COPY) | `1<<cpy` overflows for sprite numbers 32 and up, and m+nbr-1 is not checked against 64. | Fixed |
| 12 | BLIT MEMORY | With an odd width it writes one byte past a stack buffer. | Fixed |
| 13 | MODE | A change of colour depth does not close the sprites, so showing old ones overruns memory (the manual tells users to reload them). | Fixed |
| 14 | SPRITE LOAD | It can read one sprite too many, and reuses an existing buffer without checking its size. | Fixed |
| 46 | Src/MATHS.c:2895-2919, 3015, 3021, 3991-4075 | MATH(C_ADD, C_MUL, C_SUB, C_DIV, C_POW, C_CPLX, C_POLAR) and MATH FFT, FFT MAGNITUDE, PHASE and INVERSE read argv[2] without checking argc: `MATH(C_ADD z%)` or `MATH FFT s!()` follows an uninitialised pointer. The CRC functions (3051 on) read an unset entry when the list ends with a comma. | Fixed (the argument counts). The trailing comma was already safe: since 9ad3be8 makeargs points argv[argc] onwards at an empty string |
| 47 | Src/MATHS.c:1365-1381, 3882, 3896 | MATH AES128 and BASE64 write a string result of up to 255 bytes without checking the variable's LENGTH: they overflow `DIM o$ LENGTH n` (into the variable table when the string is held in its record). As PicoMite issue #59. | Fixed |

## One-character slips

| # | Where | What goes wrong | Status |
|---|---|---|---|
| 15 | Src/sprites.c:1515 | SPRITE SWAP: a `#` before the second number gives an error (`argv[0]++` for `argv[2]++`). **manual: intended** | Fixed |
| 16 | Src/sprites.c:1964 | SPRITE SCROLL reads the fill colour from `argv[2]` (y) instead of `argv[4]`. **manual: intended** | Fixed |
| 17 | Src/MiscSTM32.c:1318, 1321 | MODE tests `argv[4]` and `argv[6]` (always set) instead of `*argv[4]`: `MODE r, bits, , int` gives "Syntax" (the manual says to give 0 for bg). | Fixed |
| 18 | Src/MiscSTM32.c:1314-1315 | MODE 15,4 and 16,4: the message "12, 16, and 32-bit colour not available" leaves out 4. | Fixed |
| 19 | Src/MM_Custom.c:335, 524 | ADC's 480KHz limit on a 400 MHz processor tests `SystemCoreClock==400000`, which is never true. | Fixed |
| 20 | Src/Draw.c:2817-2823 | PAGE COPY: a D copy with a 7th argument (even 0) sets `deferredtransparent`, which nothing clears, so every later D copy is transparent; and an immediate copy made while a D copy is pending is skipped. | Fixed |
| 21 | Src/Commands.c:983, 1198 | LIST "prog" and LIST ALL "prog" add ".BAS" to `buff` after the path was made in `qq`: "File not found". | Fixed |
| 22 | Src/main.c:773-775 | MM.INFO$(RESET) ends with CR LF, so `= "Power-On"` is false. | Fixed |
| 23 | Src/CAN.c:472 | CAN FILTER takes index 0-20; there are 32 filters and Appendix J says 0-31. | Fixed |
| 24 | LOAD PNG | The cut-off `t`=1 cuts nothing (`>1` for `>=1`). **manual: intended** | Fixed |
| 25 | SPRITE HIDE | It is not refused during HIDE ALL, as b14 says it is. **manual: intended** | Fixed |
| 26 | Src/CAN.c:613 | `brs` is inverted; `fdcan` is never reset, so a Classical open after an FD open runs as FD. CAN READ returns rtr as &H20000000, not 1. | Fixed |
| 27 | CAN | Not closed by ClearExternalIO: RUN again gives "Already open". In FD mode, SEND and READ disagree on byte placement for 12 and 20-byte messages and for messages up to 8 bytes. | Fixed |
| 28 | Src/PWM.c:182 | `else if(f!=f2)` re-initialises controller 2's timer when controller 1 is re-issued at the same frequency. | Fixed |
| 29 | Src/Draw.c (IMAGE) | In the 32-bit depth, ROTATE_FAST, the scaling path of RESIZE_FAST, WARP_H and WARP_V read the buffer through `short *` (Draw.c:1888, 2066, 2206, 2403); with the Y axis inverted WARP assigns `yy` for `yp` (2016, 2043, 2070, 2156, 2183, 2210). | Fixed |
| 30 | Src/Draw.c:2628 | PAGE DISPLAY's limit for the line-doubled modes leaves out mode 17. | Fixed |
| 48 | Src/MATHS.c:145-153, 2701-2708 | MATH RANDOMIZE 0 (or a hardware seed of 0) fills the Mersenne Twister with zeros: MATH(RAND) then returns 0 for ever. n goes into an int, so 2^32 seeds with 0. | Fixed |
| 49 | Src/MATHS.c:329 | MATH(RAND) divides by 0xFFFFFFFF, so it can return exactly 1.0 (documented as less than 1). | Fixed |
| 50 | Src/MATHS.c:2888 | MATH(C_CARG) is read, but the PicoMite and b14 manuals say C_ARG (the manual now says C_CARG; the PicoMite has the same). | Fixed |
| 52 | Src/Draw.c:463 | GUI CURSOR OFF does not clear `autocursor`: the mouse link survives OFF, and LINK MOUSE then says "Already linked". | Fixed |
| 53 | Src/MM_Misc.c:3858 | CursorFollowMouse does not subtract the cursor's offset: a cross hair or loaded cursor puts its top left, not its point, at the mouse. | Fixed |
| 54 | Src/Draw.c:599 | GUI CURSOR COLOR is refused (COLOR is made COLOUR only at the start of a statement); GAMEPAD takes both. | Fixed |
| 55 | Src/USBGamepad.c:625-628 | GAMEPAD MONITOR OFF turns monitoring on (OFF is never read), and monitoring is never cleared, not even at the end of the program. | Fixed |
| 56 | Src/Commands.c:254 | HELP opens "/help.txt" on the current drive while cmd_help checks "0:/help.txt" on the SD card: with C: current it fails or reads C:'s file. | Fixed |
| 57 | Src/Draw.c:4260-4261 | LINE PLOT compares the data index with the number to plot, not the array size: with nbr given, ystart > 0 or yinc > 1 cuts the plot short. | Fixed |
| 58 | Src/Draw.c:1020, 1036, 5871 | PRINT @: the CR/LF is always printed (AutoLineWrap = false is commented out), so the bottom line can scroll; the VT100 column lacks the +1 the row has; m=5 draws in the background colour, not inverted. | Fixed: the VT100 column. The CR/LF and m = 5 are left as the PicoMite has them (the manual describes them) |
| 59 | Src/MM_Misc.c:2312-2318 | TIME$ = sec sets only the hour, minute and second: an adjustment across midnight leaves the date wrong. | Fixed |
| 60 | Src/External.c:946 | PORT: `value` is a signed int, so beyond 32 pins the shift repeats bit 31. | Fixed |
| 61 | Src/main.c:1003-1023 | `*"foo bar"` fails: the * conversion splits at the first space even inside quotes (RUN ""foo",bar"); the PicoMite's transform_star_command was not ported. | Fixed |
| 62 | Src/FileIO.c:390, 557 | Inside a #COMMENT START/END block, lines starting with # are still acted on: an #INCLUDE or #DEFINE in a commented-out block takes effect. | Fixed |
| 63 | Src/MM_Misc.c:2114-2193 | DATE$= accepts years 1900-9999, but the RTC and the DS3231 hold year-2000 in 0-99, so years outside 2000-2099 are stored wrongly; the clock is reset at power-up for a year before 2018 (main.c:1755). | Fixed |
| 64 | Src/Commands.c:1415-1419 | CALL splits its arguments at the first comma or apostrophe anywhere in the line: `CALL MID$(n$,2), x` passes the wrong arguments. | Fixed |
| 65 | Inc/Draw.h:87 | The RGB() macro keeps 4 bits of transparency, so RGB(r,g,b,t) in the 32-bit depth keeps only t AND 15 (RGB(colour, t) keeps 8). | Fixed |
| 66 | Src/Commands.c:290-322 | A nested EXECUTE is not refused, and the inner one's return clears the outer one's watchdog. | Fixed |
| 67 | Src/MM_Misc.c:746 | EPOCH checks `m > 59` (the month) for the minutes, so minutes over 59 are accepted. | Fixed |
| 68 | Src/FileIO.c:867 | DIR$ tests the SD card's status even for C:, so with no SD card DIR$ on the USB drive says "SD card not found". | Fixed |
| 69 | Src/GUI.c:215-216 | GetTouch indexes `mousestruct[Option.Mouse]`: with OPTION MOUSE unset (-1) and a USB mouse button down, CLICK(X) and CLICK(Y) read mousestruct[-1]. | Fixed |
| 70 | Src/External.c:1868 | DISTANCE divides by 11600, which assumes 200 timer ticks a microsecond; at 480 MHz there are 240, so distances read 20% long. | Fixed |
| 71 | Src/Functions.c:1471 | CHOICE truncates its condition to an integer: CHOICE(0.5, a, b) gives b. | Fixed |
| 72 | Src/External.c:993 | PORT() refuses output pins ("Invalid input pin"); b14 and the PicoMite read an output's state. The manual follows the code. | Fixed |
| 73 | | Smaller: TRIM$'s where$ must start with a capital L, R or B; LINSTR's regex form does not check start; ds18b20Timers has NBRPINS entries but is indexed up to NBRPINS (Onewire.c:173, 188). | Fixed |
| 51 | Src/MATHS.c | Smaller: V_MULT's errors name argument 1 for 2 and 3; SHIFT ignores a 4th argument other than U; SINC with n=1 and m≠1 silently does nothing; CRC8 and CRC32 apply endmask before reverseOut (CRC12 and CRC16 after; the PicoMite the same); WINDOW truncates min and max; CRC takes negative values; ARRAY INSERT and SLICE name the wrong argument in errors (Commands.c:4068, 4178). | Fixed |

## Behaviour to decide

| # | What | Status |
|---|---|---|
| 31 | The editor opens no mouse unless OPTION MOUSE is set (Editor.c:330: `Option.Mouse < 0` returns before the USB mouse test on the next line); the editor chapter says a USB mouse needs no configuration. One of the two is to change. | Fixed: the editor takes a USB mouse without OPTION MOUSE |
| 32 | GUI controls need OPTION MOUSE ("Mouse not enabled", GUI.c:249). The GUI chapter now says so. | Fixed: the controls take a USB mouse without OPTION MOUSE, as the editor does |
| 33 | MM.WATCHDOG is always 0 in an autorun program: RUN clears it (Commands.c:1324), and AUTORUN starts with RUN. The PicoMite clears it at END. | Fixed |
| 34 | RUN does not reset MM.ERRNO (it resets MM.ERRLINE and MM.ERRMSG$); the PicoMite resets all three. | Fixed |
| 35 | MM.INFO(OPTION CONSOLE PORT) cannot be reached ("CONSOLE" is tested first), and OPTION CONSOLE NONE reads back as "Serial". | Fixed |
| 36 | NEW clears OPTION AUTORUN in memory without saving it, so it stays on (b14 says NEW turns it off). | Fixed |
| 37 | ResetAllOptions sets KEYBOARD REPEAT 500/75, but OPTION LIST tests against 600/150 (b14's default), so it always lists 500,75. | Fixed |
| 38 | OPTION SLEEP's timer counts while a program runs, and INKEY$ does not restart it: a game's monitor goes off and no key brings it back. | Fixed |
| 39 | A firmware upgrade keeps OPTION MOUSE's channel but resets its sensitivity; OPTION RESET does not reset the sensitivity. | Fixed (seen on the board: the sensitivity survived the flash) |
| 40 | OPTION LIST prints names the handler refuses (SERIAL PULLUPS OFF, RTC DRIVE LOW/MEDIUMLOW/HIGH, CONSOLE OFF), prints CONSOLE INVERT/AUTO that no command sets, and never shows STATUS, VGA OUTPUT, HORIZONTAL OFFSET or USB POLLING. | Fixed |
| 41 | OPTION USBKEYBOARD DISABLE falls back to the UK layout rather than disabling the keyboard. | Fixed |
| 42 | MM.INFO(CPUSPEED) returns an integer; the PicoMite (and b14's text) a string, so `a$ = MM.INFO$(CPUSPEED)` from a PicoMite program fails. | Left as a number: CMM2 programs compare it with 480000000 (the manual now says how a PicoMite program reads it) |
| 43 | PAGE DISPLAY -1: a program that ends with the display off leaves the sync pins off. | Fixed |
| 44 | SAVE IMAGE with OPTION Y_AXIS UP mirrors x and saves the area below y. | Fixed |
| 44b | A program that runs off its last line does not run MM.END (main.c:1087 calls cleanend, not END); the PicoMite is the same, though its manual says "actual or implied END". This manual now says an END command. | Fixed |
| 45 | Any line whose first seven letters are "MMDEBUG" is dropped at load time. | Fixed |

## Found while fixing (20 Oct 2026), and fixed

| Where | What went wrong |
|---|---|
| Src/PWM.c | A duty change rewrote the polarity bit one place too far (TIM_OCPOLARITY_LOW is bit 1 already): an inverted output lost its inversion. |
| Src/Draw.c (FRAMEBUFFER WRITE) | Anything after WRITE was ignored, so the PicoMite's FRAMEBUFFER WRITE N selected the framebuffer, not the display. Now F or N (or a string). |
| Src/Draw.c (FRAMEBUFFER BACKUP) | The memory check used the backup's bytes a pixel, 0 before it exists: it never failed. |
| Src/Draw.c, sprites.c | IMAGE, BLIT, SPRITE READ and the others read page FRAMEBUFFER without checking it exists. |
| Src/Draw.c (DRAW3D) | CLOSE ALL left object 128 and reset cameras 1-3 of 6; LIGHT, SET FLAGS and RESET followed a NULL object (the PicoMite's fixes, ce4ebfe); SET FLAGS took a negative face; CAMERA checked x for y. |
| Src/Draw.c | PIXEL FILL took x = MM.HRES and y = MM.VRES; POLYGON, CIRCLE's array form and LINE PLOT read a colour with getint(0, WHITE), refusing transparency and MODE 1 CMM1's colours. |
| Src/Draw.c (showcursor) | The 16 and 32-bit loops stopped one pixel short. |
| Src/Rgb121.c | The 4-bit FRAMEBUFFER CREATE ignored a size given to it. |
| Src/Serial.c, stm32h7xx_it.c | BAUDRATE(3) read pin 0 (COM3's receive pin is not on a connector); the COM1-COM3 transmit interrupts never fired. |
| Src/sprites.c, Draw.c | The 32-bit sprite and cursor loaders stored grey ('9') as a 16-bit value. |
| Src/MM_Misc.c (EPOCH) | It set the clock's day, month and year variables (the PicoMite had commented them out). |
| Src/OtherDisplays.c | PAGE AND_PIXELS, OR_PIXELS and XOR_PIXELS combined only whole 8-byte words of each row: a width not a multiple of 8 (PAGE RESIZE) left the end of each row. |
| Src/Draw.c (PAGE SCROLL) | The limits and the buffer came from the write page, not the page scrolled. |
| Src/MM_Misc.c (MM.INFO) | MM.INFO$(OPTION CONSOLE) said "Serial" for NONE; OPTION USBKEYBOARD DISABLE read back as "". |
| Src/sprites.c (SPRITE SWAP) | The sprite swapped in took the old one's background store by copying the pointer, so both held it (and the new one's own store leaked): SPRITE CLOSE ALL after a SWAP freed it twice, and the second free cleared the pages of the next block on the heap ("Variable name" errors in m4fixes.bas). The pointers are exchanged. PicoMite #65. |
| Src/Editor.c (EDIT) | With OPTION MOUSE set and that mouse not plugged in (OPTION MOUSE 2 and the USB dongle out: an I2C mouse is tried), EDIT stopped with "Mouse not connected" before it opened, so the editor could not be used at all (kilo was the same). The mouse is now opened with its errors caught (OptionErrorSkip and ErrNext borrowed for the call), and the editor goes on without one. Seen in the regression of 20 Oct with Peter's dongle out; the editor suite passes 24 of 24 with and without the mouse. |
| Src/Draw.c (FRAMEBUFFER CREATE) | The new "set for write" check came after the sizes, whose minimum is the write page's, the framebuffer itself: "900 is invalid (valid is 1000 ...)". It comes first. |

