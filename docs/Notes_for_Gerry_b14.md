# Notes on CMM2 6.00.00b14, from a comparison with Peter's current tree

Gerry,

Peter asked me to compare your 6.00.00b14 source (disco4now/CMM2, Beta14 of 2 October) with the tree he has been working on since September, and to plan bringing the two together. The full review is in that tree as docs/Gerry_b14_Review_and_Merge_Plan.md. These notes are the parts that concern your line directly: things I found by reading your code, and fixes in Peter's tree to the b7 code you started from that you may want. Nothing of yours was run on a board; everything below was read, so please check each against the hardware before acting on it.

## 1. Where the two lines stand

Both descend from 6.00.00b7. Peter's tree has since taken the PicoMite's interpreter mechanisms one by one: 16-bit command tokens (the same encoding as yours, so programs tokenise identically), a five-byte line header, the PicoMite's compact variable table, TYPE/STRUCT on that table, symbols, the whole PicoMite MATHS.c, its regex copy, a TinyUSB keyboard in place of the ST host stack, ZMODEM, and a CSUB CallTable in the PicoMite's layout. The plan is to keep that tree as the base and bring your work into it feature by feature.

What that means for your work: your 16-bit tokens, structures and MATHS changes are the same designs and need no merging. Your new commands (REDIM, LMID, LINPUT(, BIT/BYTE/FLAG/FLAGS, INTERRUPT, SYNC, FILL, BEZIER, the SETPIN CIN options, TEMPR with a timeout, MID$ insert, the MM. overlays), your OPTION F15-F20, help.txt and the BMP decoder are all going in. So are your real table entries for ERASE, I2C2, I2C3, SPI2, HUMID, WS2812 and BITSTREAM, which are better than the tokeniser substitutions Peter's tree still has. And about fifteen CMM2 bugs you fixed that his tree still has are going in as you wrote them: the CURSOR ON leak, PIXEL FILL and PIXEL() in 32-bit modes, the `%` glyph, AUTORUN without END, the F11/F12 key copy, the OPTION LIST and SEARCH PATH buffers, MOUSE(D), the ADC interrupt handler, the ADC OPEN error path, the error state across interrupts, the Option mirror in ARMCFunctions.h, the include-name cases. Thank you for those.

## 2. Things in b14 worth a look

Found by reading; line numbers are in your Src files.

1. **INSTR's regular-expression branch, Functions.c around 839-889.** The test that chooses where the match length goes is inverted against the PicoMite's: `if (!(regex_vtype & (T_NBR|T_INT))) tempi = temp; else tempf = temp;`. An integer length variable receives a double's bit pattern, and a string variable receives eight bytes of int64. The PicoMite's `fun_instr` errors when the variable is neither type and stores through `tempi` only for T_INT.
2. **The BMP decoder's seek, BmpDecoder.c 107-130.** `onBMPSeek` ignores its `origin` argument and always seeks to an absolute offset, but `decodeBMP` calls it with `SEEK_CUR` for the 108- and 124-byte (V4/V5) headers at 682-686. Those files then seek to the wrong place and the palette or bitfield masks are misread; 24-bit non-RLE files still display because rows are sought absolutely. `origin ? current + offset : offset` fixes it.
3. **FILL, Draw.c 3292-3884.** Three things, all in the CMM2 adaptation rather than the PicoMite code:
   - `set_color_in_buffer` writes 4 bytes a pixel when VideoColour is 32, but `ReadBuffer32` returns 3 bytes a pixel and `get_color_from_buffer` reads 3, so once a span is filled the cached line is corrupted at the 4-byte stride (and `(0xFF >> 24) & 0xFF` is 0, not 0xFF).
   - Colours are compared in the expanded RGB888 that ReadBuffer returns. `ReadBuffer16` expands through the RBIT tables and `ReadBuffer8` through the CLUT, so a boundary colour only matches if it survives the round trip, and boundary mode leaks in 8- and 16-bit modes for most colours. b7's PIXEL FILL compares native pixels through `ReadPixelFast`; FILL needs to do the same, or run the fill colour through the same tables.
   - In 12-bit mode the spans are written with `DrawBuffer16(skip=0)`, which sets the transparency nibble to 0.
4. **ARC, Draw.c 4078-4225.** The b7 `if(optiony){rad1+=180;rad2+=180;}` is commented out, so ARC under OPTION Y draws differently from before; and `routinechecks(1)` sits inside the per-pixel loop.
5. **BOX, Draw.c 5427-5798.** The PicoMite body replaces b7's generic body and loses the `colourmap[]` handling, so BOX no longer follows CMM1 colour emulation, and colours go through `getint(argv[10],0,WHITE)` rather than `getColour`.
6. **`~BBF(`.** LIST does not turn it back: a listed program shows `~BBF(1,x,3)` where the user typed `BIT(x,3)`. The BASE$ fold is unfolded at Commands.c 6546-6548; the same is needed for `~BBF(`. Also, `cmd_bitbyteflag` and `fun_bitbyteflag` leave `targ` unset when the selector is 2 or 3.
7. **REDIM, Commands.c 470-566.** This is an earlier PicoMite snapshot. The current one adds `T_IMPLIED` on both re-creations (without it `REDIM a(5)` of an INTEGER array declared without a suffix recreates it as float and reads the preserved bits back as doubles), frees `oldmemory` only when it was allocated, and terminates the 32-character name (`strncpy(old, argv[i], MAXVARLEN)` leaves it unterminated).
8. **`fun_version`, Functions.c 142-176.** The PicoMite's version has three `if (*p)` guards before each `strtol(p + 1, ...)`; the port has the comment about reading past the end of the literal but not the guards, so a version string with no fourth field still reads past it.
9. **`cmd_select`.** `SaveCurrentLinePtr = checkstring(p, "IS")` leaves `CurrentLinePtr` NULL after any `CASE IS`, so an error in that CASE body is reported without its line. The PicoMite fixed this in e3298b9.
10. **`cmd_for`'s NEXT scan, Commands.c 2784-2800.** `strncasecmp(xp, vname, vlen)` matches a prefix: `NEXT JI` closes `FOR I`. It needs a whole-name comparison.
11. **`execute()` for EXECUTE "RUN", Commands.c 986-1000.** `memmove(&p[2], &p[4], strlen(p) - 4)` moves `(size_t)-1` bytes for a bare `EXECUTE "RUN"`.
12. **`execute_one_command`, Commands.c 6561-6612, and the main loop in MMBasic.c.** The dispatch is `if (p[0] >= C_BASETOKEN && p[1] >= C_BASETOKEN) commandtbl[decode].fptr()` with no `< CommandTableSize` or `T_CMD` check. Function and operator tokens are single bytes above C_BASETOKEN too, so a statement that begins with two adjacent operator tokens (`IF 1 THEN ^^`, or a line that is just `**`) decodes to an index far past the table and calls whatever is there. The one-byte dispatch had the check; it went when the two bytes came in. The PicoMite has the same hole (issue #19 there).
13. **`cmd_read`, Commands.c 3598-3606.** `while(*p) *p++ = toupper(*p);` modifies and reads `p` in one expression (undefined behaviour), and it upper-cases a string expression that READ is evaluating into a numeric variable. Peter's tree fixed the underlying problem (a lower-case name in a DATA line not found) in `findvar`, by taking the name in capitals there.
14. **`cmd_dim` STATIC, Commands.c 4063-4250.** Three things b7 had that the PicoMite fixed: the hidden name is still built from `CurrentSubFunName`/`CurrentInterruptName`, so a STATIC in a SUB called from an interrupt, or after another SUB, is named after the wrong SUB; `VarName[65]` is filled by unchecked `strcpy`/`strcat`; and `*chPosit = chSave` is restored before the STATIC local is made, so `STATIC s$ LENGTH n = value` reads the value as part of LENGTH. `SetValue` also lets `DIM s$ LENGTH 3 = "abcdefgh"` write past the string.
15. **`cmd_gosub`'s "fixed as suggested by tom" version.** It drops `substack[gosubindex] = NULL`, which the STATIC naming above relies on.
16. **kilo.c's keyword list.** The comma after `"MM.VER"` is missing, so it is concatenated with `"MM.Watchdog"`.
17. **`GetCommandValue` under CMD16BIT** prints the name and reports "Invalid statement in Type definition" for any command name it cannot find, whichever caller asked.
18. **`ExecuteProgram`'s "Invalid character" path** still has a `PIntHC(*(p)); PRet();` debug print.
19. **`ExistsFile`** copies the PicoMite's `iret = 0` where `retval` was meant; no effect, since `retval` is already 0.

## 3. Fixes in Peter's tree to the b7 code, which your line still has

Each is in a commit of his tree with the test that covers it; the short names here are the commit ids.

- The interpreter bug list, 9ad3be8: MAX/MIN seeded from FLT_MAX (`MAX(-1e300,-2e300)` gave -3.4e38); integer `^` by repeated multiplication; `ABS(-1e19)` "Number too large"; a 64-bit subscript wrapping to 32 bits; INF and NaN printed as `0e+2147483647`; `STR$` of the most negative integer; FindSubFun ignoring the kind asked for (a FUNCTION called as a statement ran part way); CALL appending to a 256-byte temporary unchecked; `argv[]` past `argc` holding stack garbage (FRAMEBUFFER CREATE faulted on it); the STATIC naming above; the function table with no free-slot guarantee (512 names hung the first new variable); 32-character labels found one time in 256; MM.INFO(VARCNT) staying high after ERASE; 256 bytes leaked per string argument of a call that errored under ON ERROR; freed locals left as markers so deep recursion filled the table.
- Step 6.0's five, d4477bc: TRACE LIST printing nothing after an END TYPE; `NEXT JI` closing `FOR I`; the SPI substitution turning `SPI(` and `SPI2(` into `SPI 1(`; a lower-case name in a DATA line not found; an #INCLUDE file's MMDEBUG lines never dropped.
- Bounds, 661139b: `LGETSTR$` from past the end of a long string wrote about 4 GB; `LINSTR`'s regex ran on the unterminated array; LONGSTRING COPY's room excluded its length element; BLIT LOADARRAY, WS2812 and ADC START read (ADC START wrote) a whole count past the end of an array given by an element; I2C WRITE with a string source, SPI WRITE from an element, SPI READ with a count below 1; DAC START, ADC START with an interrupt, IR and KEYPAD accepting a LOCAL whose memory the interrupt kept using after the SUB returned.
- GPS, 303142e: the monitor-print recursion in `processgps` that overflowed the stack (a `busy` guard); `GPStime`'s length byte; the line assembler's end.
- STR_REPLACE, 50aac7c: the PicoMite's RC9 version you have, with the correction in PicoMite issue #8 (a result that would not fit the line now leaves the line as it was and reports it, instead of running past the buffer: one 251-character line of DHT22s, each made BITBANG DHT22, hung the board).
- JSON$, 92f02a9 and d588507: the PicoMite's parse arena and path walker, with its faults fixed.
- The CSUB CallTable's slot 0x78 held the function `CallCFuncmSec` where the header reads the variable's address (you fixed this one too).
- OPTION RAM: the RAM-mode flash writers left `realflashpointer` behind after an alignment (b71c5bd), and the program area is write-back cached SDRAM, so a save now cleans the D-cache and invalidates the I-cache (a0eb76c).
- FRAMEBUFFER CREATE with no size, and VRes after PAGE WRITE/FRAMEBUFFER WRITE (d982d4e).
- The three fold corrections of the function-token work (PicoMite issues #16, #17, #18): `RIGHT$(` with one argument reading an uninitialised `argv[4]`; LIST unfolding inside longer names (`TOBASE$(16, x)` listed as `TOHEX$(x)`); a space before the bracket defeating the folds (`HEX$ (255)` failed with "BIN is not declared" on b7 too).

## 3a. Found while merging b14's features into Peter's tree

Peter's tree now has your REDIM, LMID, LINPUT(, BIT/BYTE/FLAG/FLAGS, INTERRUPT, SYNC, FILL, BEZIER, ARC, SETPIN CIN options, TEMPR timeouts, ADC START scaling and TRIGGER timeout, the BMP decoder, OPTION F15-F20 with Shift-F5/F6, help.txt HELP and the HEAPI/HEAPS names. On the way these turned up in b14 (some come from the PicoMite code it carries, and are filed there as PicoMite issues 20-24):

- **BmpDecoder.c:** `onBMPSeek` ignores `origin`, so the `SEEK_CUR` skip of a V4/V5 header's extra bytes lands at the start of the file plus that offset, and the palette is read from there. Use `f_tell(fp) + offset` for a relative seek.
- **BmpDecoder.c:** in `decodeBMP(false)`, the path LOAD BMP uses, both row orders map file line `i` to screen row `height - 1 - i`, so a top-down file (negative height) is drawn upside down; the `else` branch wants `screenRow = i`.
- **BmpDecoder.c:** a 16-bit BI_BITFIELDS file with a V4/V5 header keeps its masks in the header; they are skipped with the extra bytes and looked for after the header, where there are none, so a GIMP 565 file decodes as 555.
- **BmpDecoder.c:** the errors raised before decoding starts (not a BMP, too large, no buffer) leave the file open; LOAD BMP takes the highest free number, so the program's next OPEN ... AS #10 fails.
- **HELP with help.txt:** when a topic's text ends at the next `~` line, that line has been read and is not looked at again (the PicoMite rewinds with `lfs_file_seek`, which b14 comments out), so of two matching topics in a row the second is skipped: HELP P* with PAUSE, PI, PIXEL, PRINT shows PAUSE and PIXEL.
- **LMID** (the PicoMite's code): the bytes after the section are moved with `totalsize - (start + num - 1)`, the whole array, so a growing replacement writes past it; and the bounds check is one too lenient.
- **REDIM** (the PicoMite's older code): each argument is copied with `strncpy` into a 33-byte buffer, unterminated at 32 characters, name and bounds together.
- **CSUB vectors:** `CFuncmSec`, `CFuncInt1`-`4` and `CSubInterrupt` are never cleared when a program ends, so after NEW or another RUN the tick, an interrupt pin or the completion flag calls into the old program's code. Clearing them in `ClearRuntime` fixes it.
- **ARC:** the OPTION Y compensation is commented out, and `routinechecks` runs for every pixel; Peter's keeps both as b7 had them (once a scanline).

## 4. The licence and the manual

Peter has agreed to adopt your licence header on every file and the sign-on line, and to make your 6.00.00 manual the base of the documentation for the merged tree; the things his tree adds (TYPE/STRUCT, ZMODEM, OPTION SYMBOLS, the M7 CSUBs) will be written up against it. The program area stays at sectors 1-6 for now, because his plan uses the top of bank 2 for a flash slot and the library in the PicoMite's shape. CFUNCTION stays out.

If you want to run the regression tests against your own builds, the drivers are in his tree's tools/ (goldens.py, sptools.py, crashers_test.py) and the expected outputs in tests/.
