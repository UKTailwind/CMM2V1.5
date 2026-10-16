# Gerry Allardice's CMM2 6.00.00b14 against this tree: review and merge plan

Written 16 October 2026, for Peter (and for Gerry, if he wants it). Compares Gerry's source at https://github.com/disco4now/CMM2 (commit 0bb9911 "Beta14", 2 Oct 2026; manual "6.00.00 Beta 12") with this tree at 372fb03. Nothing of Gerry's was built or run on a board; every statement about his code comes from reading it. The few checks on the board were of our own build.

## 0. Summary

1. **Same ancestor.** Both lines descend from Peter's 6.00.00b7, which is our baseline commit 64b2746. Gerry's repository is upload snapshots, not history, but his file set is b7's apart from the GNU regex and BmpDecoder.h (both sides dropped them), and his vendored libraries (HAL, FatFs, Utilities) are byte-identical to b7. The whole comparison is Src, Inc, startup and the linker script: 33 files changed on both sides, 18 only by him, 16 only by us.
2. **His two big mechanisms are ones we also did, and ours is the later form of each.** His 16-bit command tokens use exactly the PicoMite's encoding, so the stored form agrees with ours; his structures are the PicoMite's 6.02-era design on the old variable table, ours the current design on the compact table. Neither needs merging. His MATHS.c is a hand-port of the same PicoMite code our whole-port took; ours has CLAMP and the later fixes. His regex is an older copy of the same tiny-regex.
3. **He has done a good part of our step 8** (language parity): REDIM, LMID, LINPUT(, BIT/BYTE/FLAG/FLAGS, INTERRUPT (CSUB), SYNC, FILL, BEZIER, the SETPIN CIN options, TEMPR with a timeout, MID$ insert, and the MM.ONEWIRE/MM.FLAGS reads. Most are PicoMite ports; a few carry porting slips (an inverted type test in INSTR's regex branch, a missing SEEK_CUR in the BMP decoder, a mis-ported fun_version). The plan is to take each from the PicoMite's current code, guided by his adaptation, rather than copy his.
4. **He fixed about fifteen CMM2-original bugs we still have.** All small, all worth taking as they are: a CURSOR ON leak, PIXEL FILL and PIXEL() in 32-bit modes, the `%` glyph in font 7, AUTORUN needing END, an F11/F12 key-string overflow, the OPTION LIST and SEARCH PATH buffers, MOUSE(D) repeating, two errors in the ADC interrupt handler, the ADC OPEN error path, error state across interrupts, and a stale `Option` mirror in ARMCFunctions.h.
5. **One structural choice of his is better than ours:** real table entries for ERASE, I2C2, I2C3, SPI2, HUMID, WS2812 and BITSTREAM. Ours are tokeniser substitutions, and LIST shows the substituted text: `Erase a` lists as `Clear VARS A`, `I2C2 Close` as `I2C 2 CLOSE`, `Humid` as `Bitbang DHT22` (checked on the board). With 14-bit command tokens the slots cost nothing.
6. **Things of his that would undo fixes of ours**, if taken verbatim: his cmd_if, cmd_select (CASE IS line pointer), cmd_for (NEXT prefix match), execute (bare RUN), cmd_dim (STATIC naming, LENGTH value, overruns), cmd_gosub (drops `substack`), GPS.c (three fixes), ADC START (drops RequireGlobal), and the bounds checks of 661139b. Section 5 lists them against our commits. Nothing in a function both sides edited needs merging from his side.
7. **His build** is the CubeIDE project with per-file optimisation levels, a workaround for the ST USB host stack's keyboard enumeration problem. We replaced that stack with TinyUSB and build with CMake, so none of it applies.
8. **Two decisions are his that change the user's view:** the licence (a BSD-style header naming Geoff Graham, Peter Mather and Gerry Allardice, with Gerry on the sign-on line) and the program area moved to bank-2 sector 0 (896 KB instead of 768 KB). Both are questions for Peter (section 7).
9. **The review found one PicoMite bug** on the way: both statement dispatchers index `commandtbl[]` past its end for a statement that starts with two adjacent operator tokens. Filed as PicoMite issue #19; our dispatch has the check.
10. **Effort:** about 35-40 hours in twelve commits, each gated by the full regression, listed in section 6.

## 1. Method

- Gerry's tree was cloned read-only into a scratch directory. His only source uploads are 23 Feb 2026 (b12) and 28 Sep 2026 (b14); b14 minus b12 is mainly BmpDecoder.c, so b12 already had the tokens, structures and new commands.
- Every file was compared three ways with the licence header stripped and line endings normalised: b7 to Gerry, b7 to ours, Gerry to ours. The table is in the appendix.
- The diffs were read in full: the interpreter core (MMBasic.c, MMBasic.h, allcommands.h) by me, the rest by five reviewers in parallel, each cross-checking against the PicoMite tree for ports and against our commit messages for fixes. Their reports are summarised here; where a reviewer could not determine something, it is marked.
- His switches `CMD16BIT` and `STRUCTENABLED` are compiler defines in the CubeIDE project, on in his Release build, so everything behind them is live.

## 2. The two lines side by side

| Mechanism | Gerry b14 | Ours (372fb03) | Merge |
|---|---|---|---|
| Command tokens | 14-bit, two bytes of 0x80+7 bits, `commandtbl_decode`; `GetCommandValue` returns the index | the same encoding, through `commandtbl_at/decode/put` with `CMD_NOTOKEN` (b5bbff6, 9eb047f) | nothing; his `decode` lacks the token-byte check (section 4) |
| Line format | b7's | five-byte header, image format stamp (daa0537, d4477bc) | ours |
| Variable table | b7's, `namelen` reused as flag bits | the PicoMite's compact table, VREC, locals on a stack (fb3016c) | ours |
| Structures | the PicoMite's 6.02.01 design in MMBasic.c/Commands.c, `size` holds the type index, `FindStructBase`/`ResolveStructMember` | the PicoMite's current design in Struct.c, on the compact table, with StructTest passing (step 5b) | ours; his has a few checks ours lacks only where ours is deliberately looser (`a(0)` under OPTION BASE 0) |
| Symbols | none | S5/S6 with bindings (step 6) | ours |
| MATHS | hand-port of the PicoMite's strides, SINC, array_* | the whole PicoMite file, plus CLAMP and the 9749c83 fixes (9f85a3d) | ours; nothing in his that ours lacks |
| Regex | tiny-regex, older than the PicoMite's first commit | the PicoMite's current copy (7e10bc6) | ours |
| Function folds | BIN$/OCT$/HEX$ as b7; `~BBF(` for BIT/BYTE/FLAG (no un-fold in LIST) | BASE$, SCHANGE$, TOPBOTTOM with `ListUnfold` (9692ad1) | ours; BIT/BYTE/FLAG through our fold |
| ERASE, I2C2/3, SPI2, HUMID, WS2812, BITSTREAM | real table entries | tokeniser substitutions, which LIST shows | his shape |
| USB keyboard | ST host stack, per-file optimisation workaround | TinyUSB with the PicoMite's keyboard code (2cd87a8) | ours |
| CSUB CallTable | b7's table plus three slots (CFuncInt3/4, CSubComplete), slot 0x78 corrected | b7's table with 0x78 corrected, plus `CallTableP` in the PicoMite's layout via vector slot 7 (1858a28) | ours; append his three to the old table if wanted |
| Program area | bank-2 sectors 0-6, 896 KB (`PROGRAMSTARTCODE 0`) | sectors 1-6, 768 KB | question 2 |
| Linker | `.extra` into `.text`, one 1 MB region | the same (ce0b696) | nothing |
| ZMODEM | none | yes (449c54c) | ours |
| GPS | the PicoMite's parser fixes | the same, plus three more (303142e) | ours |
| Tests | none in the repository | tests/cmm2, goldens, Testfiles, SP Tools, crashers | ours |
| Build | STM32CubeIDE project | CMake + build.cmd | ours |
| Manual | 6.00.00 Beta 12 PDF, by Gerry | none since 5.07 | his, as the base (section 7) |

## 3. What to take from Gerry's tree

Dispositions: **take** means as he wrote it, adapted only to our accessors; **adapt** means take the PicoMite's current code with his CMM2 adaptations as the guide; **decide** means Peter's call.

### 3.1 CMM2-original bug fixes (take; about 3 hours in all)

| Fix | Where in his tree | Ours today |
|---|---|---|
| CURSOR ON allocated `cursorsave` twice on one path; the first leaked each time | Draw.c:550-620 | both allocations still there (Draw.c:518, 559) |
| PIXEL FILL in a 32-bit mode filled with the previous colour (`if(VideoColour==32){}` was empty) | Draw.c:3888-3966 | unfixed (Draw.c:3254) |
| PIXEL(x,y) in a 32-bit mode returned a stale value | Draw.c:6019-6046 | unfixed |
| The `%` glyph of font 7 (F_6x8_LE) had its diagonal the wrong way | Font_8x6.h:8-9 | unfixed |
| AUTORUN of a program without END did not clean up (`cleanend()` after the autorun RUN) | main.c | unfixed: main.c:831-838 has no `cleanend()` on either autorun branch |
| OPTION F11/F12 allow 63 characters but are copied into `FunKey[][25]` at boot; "Maximum 23" message wrong | main.c, MM_Misc.c; he reads `Option.F11Key` directly | latent overflow: main.c:531-532, MM_Misc.c:2383-2393. Take his, with MAXKEYLEN 64 (the PicoMite's) |
| OPTION LIST copies a 63-character key string plus `<crlf>` into `cc[40]` | MiscSTM32.c | unfixed (MiscSTM32.c:273, 283); make it 72 |
| OPTION SEARCH PATH checked against `sizeof(Option.F12Key)` ("Maximum 23") instead of `sizeof(Option.path)` (127) | MM_Misc.c | unfixed (MM_Misc.c:2406) |
| MOUSE(D) reported the same double-click repeatedly (the flag is never cleared) | MM_Misc.c:501-504 | unfixed (MM_Misc.c:508) |
| TIM7 ADC handler divides channels B and C by channel A's bit depth, and in two pre-trigger branches writes the integer array `a3point` instead of `a3float` | stm32h7xx_it.c | unfixed (stm32h7xx_it.c:692-694, 719-721, 729-731). Take the two one-line corrections now; the scaling feature separately (3.3) |
| ADC OPEN: an error after channel A was configured unconfigured a COM/SPI pin through ADCclose | MM_Custom.c:318-345 | unfixed (MM_Custom.c:316-329) |
| An interrupt SUB inherits and leaves the caller's error state: save and clear OptionErrorSkip, MMErrMsg and MMerrno at entry, restore at IRETURN (the PicoMite does the same) | MM_Misc.c | ours saves OptionErrorSkip only, and does not zero it (MM_Misc.c:4203); size the message save to our `MMErrMsg` |
| `struct option_s` mirror in ARMCFunctions.h was b7's stale copy (RunInRam, 24-byte keys): a CSUB reading `Option` past offset 4 reads garbage | ARMCFunctions.h:183-240 | still stale (ARMCFunctions.h:181-227); regenerate from our Flash.h |
| `#include "Turtle.h"` for a file named turtle.h, and nine other include-name case mismatches | turtle.c:3, Hardware_Includes.h, main.h, syscalls.c, it.c, DEV_Config.h, MM_Misc.c | ours still has them; harmless on Windows, wrong on a case-sensitive build |
| Inc/BmpDecoder.h was never included anywhere | deleted | still tracked |
| GetSector fallthrough reports "Sector not located" | flash.c | diagnostic; optional |

### 3.2 PicoMite-parity features he has and we lack (adapt; our step 8)

| Feature | His version | Notes for the port | Effort |
|---|---|---|---|
| REDIM [PRESERVE] | Commands.c:470-566, an older PicoMite snapshot | take the PicoMite's current `cmd_redim` (his lacks `T_IMPLIED` on re-creation, frees `oldmemory` unconditionally, `strncpy` unterminated at 32 characters); onto VREC/`RAW_DIM`/`DimIsRealArray`/`VAR_INLINE_STR`; our `cmd_erasevar` split into an `erase(p, nofree)` that returns the old data pointer, as his; `g_StructArg` for structure arrays | 3-5 h |
| BIT(/BYTE(/FLAG( as functions and commands, FLAGS, `g_flag`, MM.FLAGS | the PicoMite's bodies, folded into one `~BBF(` command and function at tokenise time; his LIST does not un-fold it | commands as real table entries; the three functions through our `SubstFold`/`ListUnfold` if one-byte function tokens are short (they are: 121 used); `FoundType()` for the member type; add the names to LIST COMMANDS/FUNCTIONS | 2-3 h |
| LMID | MM_Misc.c:796, verbatim PicoMite | take | 0.5 h |
| LINPUT(fnbr, n) into a LONGSTRING, with FileGetData/FilePutData/ExistsFile | MM_Misc.c:1334, FileIO.c:1816-1915, PicoMite ports | take; FileGetData also fixes the LONGSTRING file read (3.3) | 1 h |
| INTERRUPT (a CSUB's interrupt), CSubInterrupt/CSubComplete, the dispatch in CheckInterrupt | MM_Misc.c:3399 and the hook | take the 12-line command and the hook; point `CallTableP` 0xc0 at the real `CSubComplete`; CFuncInt3/4 through EXTI3/4 with it, into our table not his | 2-3 h |
| SYNC n[,U/M/S] and bare SYNC | External.c:134-162, PicoMite with GetuSec | take | 0.5 h |
| SETPIN pin, CIN, option 1-5 (edge and pull on INT1-4) | External.c:955-1043, ExtCfg | take; the PicoMite allows 1..10, adapt the range | 1 h |
| TEMPR START pin [,prec] [,timeout] and TEMPR(pin [,timeout]) | Onewire.c:122-152, PicoMite | take | 0.5 h |
| MID$(s$, pos, 0) = x$ inserts | Commands.c:3080-3123, PicoMite | take with FoundType/FoundSize; a tests/cmm2 line | 0.5 h |
| INSTR's regex branch: OPTION ESCAPE off while the pattern is read; the length variable may be integer | Functions.c:839-889 | port from the PicoMite, not his: his type test is inverted (an integer variable receives a double's bits) | 0.5 h |
| fun_version RC handling (`RCn` adds 50) | Functions.c:142-176 | port from the PicoMite, not his: he copied the comment but not the three `if (*p)` guards | 0.2 h |
| MM.ONEWIRE | Functions.c overlay | take (trivial); MM.ESC and POS-as-overlay are his own, optional | 0.2 h |
| ADC START with per-channel scale and offset, ADC TRIGGER ch, volts [,timeout] | MM_Custom.c:403-589 and the TIM7 handler | add to ours; keep our `RequireGlobal` and the `volatile` pointers, which his drops | 2-3 h |
| LONGSTRING read from a file by FileGetData ("fix from PicoMite 6.02.01RC9") | Memory.c | with FileGetData above | 0.3 h |
| The deferred "Dimensions" error (a FUNCTION such as `SX(-24)` probed as an array under V_NOFIND_NULL; PicoMite 6.02.01B4) | MMBasic.c findvar | take into our findvar | 0.5 h |
| An unterminated string literal is accepted and closed | MMBasic.c tokenise, PicoMite | take; ours errors | 0.2 h |
| HELP without quotes | tokenise, `helpquotes` | quote the argument in tokenise (his MMBasic.c:1990 does); do not take the re-entrant tokenise in his cmd_help | 0.3 h |
| help.txt on the SD card with pattern-matched, wrapped lookup | Commands.c:239-397 | optional; the CMM2 has built-in help | 2-3 h |
| OPTION ESCAPE OFF | MM_Misc.c | take (ours can only turn it on) | 0.1 h |
| OPTION DISPLAY resends the terminal size | MM_Misc.c | optional | 0.1 h |
| PEEK(BP var), PEEK(WP var), PEEK(SP var) with post-increment | MM_Misc.c | origin not the PicoMite (Micromite?); optional | 0.3 h |

### 3.3 Graphics (adapt; about 10 hours)

| Item | His version | Notes | Effort |
|---|---|---|---|
| BEZIER | Draw.c:4389-4620, byte-for-byte PicoMite 6.01 | take, add the table entry, delete the old commented code | 1 h |
| FILL x,y,colour[,boundary] | Draw.c:3292-3884, PicoMite DrawFill port | three defects to repair: the 32-bit line cache writes 4 bytes a pixel where ReadBuffer gives 3; colours are compared in the expanded RGB888 that ReadBuffer returns, so boundary mode leaks in 8/16-bit modes (compare native pixels, as PIXEL FILL does); 12-bit span writes drop the transparency nibble. The visited bitmap is 259 KB at 1920x1080 | 3-4 h with tests in 8/12/16/32-bit modes |
| ARC | Draw.c:4078-4225, PicoMite scanline port | restore the b7 OPTION Y compensation he commented out; move `routinechecks` out of the per-pixel loop; compare with the arc goldens | 1-2 h |
| BOX with scalar width/height and array x/y | Draw.c:5427-5798, PicoMite body | keep our body, add only the mixed case; his drops the CMM1 colourmap handling and `getColour` | 1 h |
| BMP decoder (1/4/8/16-bitfield/24-bit, RLE4/8, V4/V5 headers, top-down, clipping) | BmpDecoder.c, PicoMite port, 1579 lines | `onBMPSeek` ignores `origin`, so V4/V5 headers seek wrong; put `ReadAndDisplayBMP` in a header; test palette, RLE, bitfield and V4/V5 files; LOAD IMAGE's dither/sub-image arguments are not exposed | 4-6 h |

### 3.4 Structural choices to adopt

- **Real commands instead of substitutions** for ERASE, I2C2, I2C3, SPI2, HUMID, DHT22, WS2812 and BITSTREAM, as his table has them. LIST then shows what the user typed. His bodies are b7's (the WS2812 bit-bang driver, BITSTREAM); ours keep the 661139b bounds checks, so only the dispatch changes. ~2 h.
- **Dead code:** he deleted the GNU regex files (Src/regex.c, Inc/xregex.h are still tracked and compiled here, with their callers commented out) and the `cmd_autosave2`/`cmd_autosaveXXX` bodies. Delete them here too.
- **MATHS.h banner:** ours still carries b7's mis-pasted "Editor.h" banner; his names the file. Fix with the licence change.
- **kilo.c keyword list:** he adds Bit, Byte, Flag, MM.FLAG and MM.VER, with a missing comma after "MM.VER" that concatenates it with MM.Watchdog. Add the names with the comma.

### 3.5 Things of his to leave (already present or superseded)

16-bit token scaffolding (every `#ifdef CMD16BIT` block, the three copies of `commandtbl_decode`, the RUN-line byte rewrite, `GetNextCommand`); `STRUCTENABLED` (all of it); MATHS.c; re.c; STR_REPLACE (his is the PicoMite's RC9 version, ours has the issue #8 correction); the array_* commands (ours have two fixes his lack: int/float conversion in INSERT/SLICE and "One index must be omitted"); cmd_struct, fun_struct, ParseStructMember; CheckIfTypeSpecified; the `getcsargs` macro (we have the PicoMite's getargs form; add the macro only if the ported code wants it); the linker changes; MX_USB_HOST_Init moved; MM.INFO(IHEAP/SHEAP) renamed HEAPI/HEAPS (would break programs; accept both at most); PWM negative frequency as open-drain (undocumented, marked "testing"; a documented option if wanted); his version and copyright strings; the debug print left in ExecuteProgram's "Invalid character" path; the CubeIDE build settings.

## 4. His code that would reintroduce bugs we fixed

Never take these functions verbatim. Our commit and the bug are given so the regression can confirm each stays fixed.

| His function | What it would undo | Our commit |
|---|---|---|
| `execute()` (EXECUTE "RUN") | a bare EXECUTE "RUN" moved (size_t)-1 bytes | 9eb047f |
| `cmd_if` | the THEN clause step without the token check: a THEN clause that is a SUB call | 82f5c92 |
| `cmd_select` | `SaveCurrentLinePtr = checkstring(p,"IS")`: after CASE IS every error in a CASE body loses its line | 9eac831 |
| `cmd_for` NEXT scan | `strncasecmp` prefix: NEXT JI closed FOR I | d4477bc |
| `cmd_dim` | STATIC named after the last SUB entered or the interrupt SUB; `VarName[65]` strcpy; STATIC s$ LENGTH n = value reads the value as LENGTH; DIM s$ LENGTH 3 = "abcdefgh" overruns | 9ad3be8, d4477bc, 493053f |
| `cmd_gosub` | drops `substack[gosubindex] = NULL`, breaking STATIC after a GOSUB; our fix for the mis-blamed line is in the name index and error() | 9ad3be8 |
| `cmd_read` | upper-cases DATA values with an undefined-behaviour loop; ours fixed the lower-case DATA name in findvar | d4477bc |
| GPS.c whole | no `busy` guard in processgps (the monitor-print recursion overflowed the stack), GPStime length byte, line assembler to 128 | 303142e |
| ADC START | drops `RequireGlobal` with an interrupt and the `volatile` pointers | 661139b |
| External.c, SPI.c, I2C.c whole | the bounds checks of WS2812, ADC START, I2C WRITE, SPI READ/WRITE | 661139b |
| `fun_instr` regex branch | the inverted type test is worse than b7 | (new in his) |
| `execute_one_command`, the main loop | no `< CommandTableSize` and T_CMD check: two adjacent operator tokens dispatch past the table (PicoMite issue #19) | 9eb047f |
| main.c RUN-line handling | the "RUN line stored by overwriting bytes" hack | f9d336d |
| CallTable (old) | his table layout for CFuncInt3/4 and CSubComplete differs from `CallTableP`'s | 1858a28 |

And a PicoMite slip he inherited: `ExistsFile` returns `iret = 0` where `retval` was meant (harmless).

## 5. Fixes of ours that must survive the merge

Every one is covered by a test that the regression runs (tests/cmm2, the goldens corpus, Testfiles, SP Tools, the crasher suite), so a merge commit that undoes one shows up. The areas Gerry's code touches: the interpreter bug list (9ad3be8: MAX/MIN, integer ^, ABS, 64-bit subscripts, INF/NaN, STR$, FindSubFun kind, CALL, argv past argc, STATIC, funtbl guard, labels, VARCNT, string-argument leaks, ClearVars markers); step 6.0's five (d4477bc: TRACE LIST after END TYPE, NEXT whole names, SPI( substitution, DATA names, #INCLUDE MMDEBUG); 661139b's bounds; the GPS fixes (303142e); STR_REPLACE (50aac7c); JSON$ (92f02a9, d588507); the MATH fixes (9f85a3d); the OPTION RAM writers (b71c5bd, a0eb76c); the CallTable's slot 0x78 (1858a28, which he also fixed). Nothing in sections 3.1-3.4 touches these paths except through the accessors.

## 6. Merge order

As separate commits on our tree, each built with 0 warnings and gated by the full regression, in this order so that the cheap, certain ones land first:

| # | Commit | Content | Hours |
|---|---|---|---|
| M1 | Licence and banners | the new header on every file, the sign-on copyright line, MATHS.h's banner, the include-name cases (decision 1) | 1 |
| M2 | Small fixes | the whole of 3.1, with a tests/cmm2 case for each that can be tested from a program (PIXEL in 32-bit modes, MOUSE(D) cannot; the TIM7 fixes need the ADC) | 3 |
| M3 | Real commands | ERASE, I2C2, I2C3, SPI2, HUMID, DHT22, WS2812, BITSTREAM as table entries; LIST shows them | 2 |
| M4 | Parity set 1 | MID$ insert, LMID, LINPUT( with FileGetData/FilePutData, the LONGSTRING read, TEMPR timeout, SETPIN CIN, SYNC, MM.ONEWIRE, OPTION ESCAPE OFF, the deferred Dimensions error, the unterminated string, HELP quoting, INSTR's regex branch, fun_version | 5 |
| M5 | BIT/BYTE/FLAG/FLAGS | with MM.FLAGS, through our folds; kilo.c names | 3 |
| M6 | REDIM [PRESERVE] | from the PicoMite's current code, with erase() split | 4 |
| M7 | BEZIER and ARC | | 2 |
| M8 | FILL and BOX | with the three FILL repairs; tests in four colour depths | 4 |
| M9 | CSUB INTERRUPT | with CFuncInt3/4 into CallTableP | 3 |
| M10 | ADC | START scaling, TRIGGER, the OPEN error path | 3 |
| M11 | BMP decoder | with the SEEK_CUR fix and test files | 5 |
| M12 | OPTION F15-F20 and Shift-F5/F6 | with the `Option` mirror in ARMCFunctions.h and the saved-options reset (decision 3); help.txt HELP (decision 5); MM.INFO accepting HEAPI/HEAPS as well as IHEAP/SHEAP (decision 6). The program area stays as planned (decision 2) and CFUNCTION stays out (decision 4) | 3 |
| | Cleanup | the dead regex files, autosave2 | 0.5 |

Total about 38 hours. The step 8 items this clears from the parity plan: REDIM, ERASE as a command, BIT/BYTE/FLAG, and the graphics commands; INTERRUPT from the CSUB list.

## 7. Questions for Peter (answered 16 Oct)

Peter's answers: (1) adopt the licence; (2) the program area stays as planned, 384 KB plus the slot and the library; (3) take OPTION F15-F20 and Shift-F5/F6; (4) leave CFUNCTION out; (5) take help.txt; (6) accept both MM.INFO spellings; (7) his manual becomes the base of ours; (8) Gerry sees this document and the tree, and only he does (`docs/Notes_for_Gerry_b14.md` is written for him).

1. **The licence.** Gerry's files carry a BSD-style header: "Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice", with the four conditions (MMBasic named, the copyright on the console, the acknowledgement), and the sign-on prints a line for Gerry. Ours carry b7's restricted header. Adopt his on every file (M1)?
2. **The program area.** He moved it to bank-2 sector 0: 896 KB for programs, 128 KB more. Our plan of 15 Oct set aside 384 KB for programs, one flash slot and the library, in the PicoMite's shape. The two are not compatible as they stand. Which?
3. **OPTION F15/F16/F19/F20 and Shift-F5/F6.** His own addition, 256 bytes more in `struct option_s`, so saved options are reset on upgrade and the CSUB mirror changes. Take?
4. **CFUNCTION.** He restored the CFUNCTION keyword and its save path. Our mmb2csub work is CSUB-only, and I found no working call path for a CFunction in his tree. Leave it out?
5. **help.txt.** His HELP reads /help.txt from the SD card when present. Worth having beside the built-in help?
6. **MM.INFO(HEAPI/HEAPS).** He renamed IHEAP/SHEAP. I propose keeping ours and accepting both spellings.
7. **The manual.** His 6.00.00 Beta 12 manual is the only one since 5.07. It documents everything in sections 3.2-3.4 but not TYPE/STRUCT, ZMODEM, OPTION SYMBOLS or the M7 CSUBs. Should it become the base of ours, with our additions written up against it?
8. **Gerry.** Should he see this document, and our tree? Several of his fixes are worth having in his line too (the INSTR type test, the BMP seek, the CASE IS line pointer, the NEXT prefix match, the dispatch check), and the PicoMite-side fixes we took would save him re-porting them.

## Appendix: the three-way table

Diff lines after the licence header is stripped. "BOTH" is the conflict set.

| File | Gerry vs b7 | Ours vs b7 | Gerry vs ours | Class |
|---|---|---|---|---|
| Src/MATHS.c | 1013 | 6211 | 6066 | BOTH |
| Src/Commands.c | 3787 | 952 | 4617 | BOTH |
| Src/MMBasic.c | 1623 | 2367 | 3870 | BOTH |
| Src/re.c | 1610 | 1626 | 22 | BOTH |
| Src/Draw.c | 1789 | 127 | 1920 | BOTH |
| Src/BmpDecoder.c | 1903 | 0 | 1903 | G-only |
| Src/MM_Misc.c | 753 | 743 | 1242 | BOTH |
| Src/main.c | 299 | 417 | 670 | BOTH |
| Src/GPS.c | 196 | 485 | 473 | BOTH |
| Src/Functions.c | 176 | 365 | 539 | BOTH |
| Inc/MMBasic.h | 127 | 221 | 334 | BOTH |
| Src/External.c | 152 | 109 | 259 | BOTH |
| Src/MM_Custom.c | 157 | 71 | 218 | BOTH |
| Src/CFunctions.c | 41 | 186 | 223 | BOTH |
| Src/FileIO.c | 122 | 43 | 165 | BOTH |
| Inc/allcommands.h | 127 | 35 | 154 | BOTH |
| Src/I2C.c | 56 | 105 | 161 | BOTH |
| Src/Memory.c | 21 | 93 | 102 | BOTH |
| Src/stm32h7xx_it.c | 58 | 21 | 79 | BOTH |
| STM32H743II_FLASH.ld | 37 | 33 | 70 | BOTH |
| Inc/ARMCFunctions.h | 58 | 7 | 65 | BOTH |
| Src/SPI.c | 35 | 26 | 61 | BOTH |
| Src/flash.c | 7 | 50 | 57 | BOTH |
| Src/PWM.c | 53 | 0 | 53 | G-only |
| Src/MiscSTM32.c | 48 | 0 | 48 | G-only |
| Src/keyboard.c | 43 | deleted | | O-deleted |
| Inc/Flash.h | 37 | 0 | 37 | G-only |
| Inc/Configuration.h | 23 | 13 | 36 | BOTH |
| Inc/CAN.h, fileIO.h, turtle.h, main.h, MATHS.h, GPS.h, Audio.h, Version.h, Font_8x6.h, DEV_Config.h, Draw.h, Onewire.h | small | | | G-only or BOTH |
| Src/Onewire.c, GUI.c, kilo.c, fm.c, syscalls.c, turtle.c, Custom.c | small | | | |
| Inc/keyboard.h, Src/CAN.c, sprites.c, Operators.c, XModem.c, Timers.c, startup, MMBasic_Includes.h | 0 | small | | O-only |
| Src/Struct.c, Symbols.c, ZModem.c, USBKeyboard.c, KeyboardMap.c, Inc/Symbols.h, KeyboardMap.h, tusb_config.h | | new | | O-new |
| Src/usb_host.c, usbh_conf.c, usbh_platform.c, keyboard.c, the three headers | | deleted | | O-deleted |
| Src/regex.c, Inc/xregex.h, xregex2.h, BmpDecoder.h | deleted | | | G-deleted |
| 89 files | 0 | 0 | 0 | same |
