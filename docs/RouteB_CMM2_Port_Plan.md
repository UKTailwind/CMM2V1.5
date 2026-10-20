# Route B (the compiler) on the CMM2: port plan

20 Oct 2026. Peter: "Next I think it is time for the compiler. Implement that."

Route B is the PicoMite's compiler: `core/Stream.c` (5,754 lines) and `core/Stream.h`, designed in the PicoMite's
`docs/Interpreter_RouteB_Design.html` and reviewed in `Interpreter_RouteB_VM_Review.html` and `_Coverage.html`. The
tokenised program stays the program. At RUN, when the image's CRC differs from the stream's stamp, the program is
compiled into one record per statement: 16-bit wordcode for what the compiler handles, a fallback record calling the
statement's own handler for the rest. ExecuteProgram hands over to the executor (`RunStream`, `RBRun`, a computed-goto
VM) whenever the next statement is in the program image, and the executor keeps the interpreter's state (nextstmt,
CurrentLinePtr, the FOR/DO/GOSUB stacks, LocalIndex, temporary memory, ON ERROR SKIP) exactly as the text loop would
at every statement boundary. On the PC3 the games run 19-38% faster compiled, tight loops 8-10x.

## What the CMM2 already has

The V7 core the stream is built on is here: symbols and their bindings (SymCanonOf, SymG, SymL, SymTextLocals, library
symbols), variable records through VREC, the local stack and SUB layouts, findvar's symbol fast path, the IF table,
DoFindLoop and the DO fast-path state, the NEXT/DO/LOOP recursion fixes, ExecDepth and ErrorSkipDepth, CALL and CONST
as symbol-aware commands, commandtbl_at and 2-byte command tokens, the word of zeros after an image's end marker, and
ClearTempMemory's flag. Two read-only surveys (20 Oct) mapped every identifier Stream.c uses: 57 the same, 12 renamed
(the PicoMite's g_ prefixes), 21 different, 20 missing, 10 PicoMite-only.

## Core work before Stream.c (about 400 lines, mostly factoring code the CMM2 has)

| # | Item | Where |
|---|------|-------|
| 1 | Bind generations (SymBindEvent, SymBindGenG, SymLevelGen[SYM_LEVELS], SymBindGlobalsChanged, SymLocalEvent), bumped at every event that changes what a name binds to; SYM_LEVELS 112 (MAXGOSUB is 100 here) | Symbols.c/h, MMBasic.c, MM_Misc.c |
| 2 | SymIdAt and SymCanonById | Symbols.c/h |
| 3 | ForFindNext out of cmd_for | Commands.c |
| 4 | StaticVarName exported, StaticLink out of cmd_dim, the STATIC AS-type reset fix (PicoMite 74ed383), V_DIM_NEW in findvar | Commands.c, MMBasic.c |
| 5 | CheckByKeyword out of SubLayoutBuild | MMBasic.c |
| 6 | DefinedSubFunMem: 1 for DefinedSubFun's argument block (kept in its own pointer), 2 for a compiled call; error() frees only for 1 | MMBasic.c |
| 7 | An exported stack test for compiled recursion (the CMM2's __get_MSP test) | MMBasic.c |
| 8 | Fn* helpers (sin, cos, tan, atn, sqr, exp, log, deg, rad, int, fix, abs, sgn) out of the CMM2's own fun_ bodies | Functions.c |
| 9 | getvalue's T_VALUE case (the value splice); header prototypes for getvalue and ErrNext | MMBasic.c/h |
| 10 | The float overflow check of op_add, op_subtract, op_mul and op_div (the PicoMite's; the VM inlines it) | Operators.c |
| 11 | ExecuteProgram's hand-over to RunStream; PrepareProgram's RBLive = 0 and RBPrepare after IfTableBuild | MMBasic.c |
| 12 | OPTION COMPILE ON / OFF / SHADOW (a byte in the options, 0xFF read as OFF), OPTION LIST, MM.INFO(COMPILE) | flash.h, MM_Misc.c, main.c |
| 13 | The stream's SDRAM area and a CRC-32 of any length | Memory.h, main.c (MPU) |

## Stream.c on the CMM2

Kept as close to the PicoMite's as the CMM2 allows, so that later PicoMite changes can be brought over: the g_ names
through a port header, `-Wno-pointer-sign` for its unsigned char pointers. CMM2 changes:

- The statement tail and line head are the CMM2's: CheckAbort when USBtime, deferredcopy or MMAbort; check_interrupt
  when IntReady.any, autocursor or GUIactive; TRACE prints [file:line] from the line header; OPTION PROFILING's
  per-line counters (or no compiling while profiling). No core 1 stack check, no OPTION NOCHECK, no g_perf counters.
- The stream is in SDRAM, written directly: the flash and PSRAM writers go; the bind cache is always on.
- RBInImage and RBFind through InProgMemory and InLibrary (the program in program memory, a flash or RAM slot's
  program running in place, the RAM library in the heap).
- No "End If" / "Else If" command tokens, no /* */ tokens (the loader drops those lines), no BLIT MEMORY, fun_clut for
  MAP(, GetJustification, the CMM2's `~(` letters and MM.INFO keywords, `>>` unsigned (op_shiftright_unsigned).
- The splice: the CMM2's graphics commands are not symbol-aware, so spliced text has its symbols spelt out when it is
  compiled; each spliced handler's argument forms checked against the CMM2's (BLIT differs most).
- The executor (RBRun, RunStream, RBFind, RBTail, RBInImage) in ITCM (about 36 KB free), the rest of Stream.c in flash.
- CHOICE's compiled condition: any non-zero value is true, as the text path (a PicoMite bug: Stream.c missed issue
  #61's fix; filed).

## Where the stream lives

G2, Peter's layout of 17 Oct: the stream at 0xD0780000-0xD087FFFF (1 MB, about 3x a 340 KB program), the OPTION RAM
program area moved from 0xD0780000 to 0xD0700000 (MPU region 9 with it: its CSUBs run there), the heap from 0xD0880000
(23.5 MB). A program whose stream does not fit runs as text, and MM.INFO(COMPILE) says so.

G1 (Peter, 20 Oct: "G1 should do as proposed"): while OPTION COMPILE is ON or SHADOW the stream takes the bottom 1 MB
of the heap (0xD0300000, or 0xD0380000 above an OPTION RAM program), and the heap starts above it (4.7 MB to 3.7 MB).
The size, RBStreamSize, is fixed at boot from the saved option, so the first OPTION COMPILE ON or SHADOW after a restart
saves the option and restarts (as OPTION RAM does; "Invalid in a program" there). OPTION COMPILE OFF keeps the area
until the next restart, so turning the option off and on again costs nothing more.

## Where it stands (20 Oct, evening)

Done: every core item above but item 10 (see below), Stream.c built in with compiled code on, OPTION COMPILE and
MM.INFO$(COMPILE), the G2's SDRAM layout, the manual entries and the release notes.

- The value splice (P5b-P5e: built-in functions and graphics commands handed computed values) is off (RB_SPLICE 0):
  its handlers are to be checked against the CMM2's own argument reading first (its graphics commands are not
  symbol-aware, and several take keywords or names where the PicoMite's take numbers). Those statements run as text.
- Item 10, the float overflow check: the PicoMite's +, -, * and / raise "Overflow" on a result of +INFINITY; the CMM2's
  give the infinity (tests/cmm2/bugs.bas, items 25 and 26 of its bug list, prints INF). Peter, 20 Oct: "keep cmm2
  behaviour". The text path is unchanged and the compiled inline ops do not check either (RB_OVERFLOWCHECK 0).
- A review of every compiled statement against the CMM2's handlers found 15 differences; all fixed but the order of the
  gosub depth check for a call 100 levels deep (its error comes after the arguments, not before: the message is the
  same). Three were the CMM2's own text-path bugs, fixed at the root: compare() on integers far apart (undefined
  overflow; the DO fast path disagreed with the evaluator), and IF's THEN/ELSE found inside a quoted string (only for a
  line typed at the prompt or EXECUTE's: the loader drops bytes above 126). The PicoMite's issues #83 and #84.
- tools/rbtest.py runs the PicoMite's Route B tests (its Bas/bench/rb*.py, 392 programs) and the CMM2's own
  (tools/rb_cmm2.py, 17) with OPTION COMPILE OFF, ON and SHADOW: 409 programs, none differing (300 the same, 109 with
  the same deliberate error every way).
- Speed (tools/rbspeed.py, G2 at 480 MHz): K1 10.8x, K3 8.6x (the PicoMite's 10.7x and 8.6x), the P4/P5 speed programs
  2.9-7.7x but those through the splice (1.0-1.2x); solar_eclipse 5.99 s as text, 1.67 s compiled.
- The regression with OPTION COMPILE ON (G2, 20-21 Oct): tests/cmm2, the goldens corpus, Testfiles and SP Tools print
  what they print with it off (one SP Tools file differed when the USB flash drive dropped off the hub mid-run; run
  again, it matched). On the G1, tests/cmm2 the same both ways.
- The G1 (COM34, 400 MHz): the same 409 programs, none differing, and the same gains (K1 10.85x, K3 8.59x); its heap 3.7 MB
  while compiling, 4.7 MB after a restart with the option off.

## Steps and tests

1. The core items above, each checked with the regression (tests/cmm2, the goldens corpus, Testfiles, the editor, FM,
   SP Tools) with compiling off: nothing changes.
2. Stream.c built in, P1 behaviour first (fallback records only, by a development switch): the goldens and tests/cmm2
   identical with OPTION COMPILE ON.
3. Everything compiled: OPTION COMPILE SHADOW over tests/cmm2, the goldens, Testfiles and the games (exile, petrobot,
   picofrog, Elite); then ON. Interrupts, ON ERROR SKIP, TRACE, CHAIN, CONTEXT, flash and RAM slots, the RAM library.
4. Speed: solar_eclipse (7.3 s as text), the micro suite and perfguide, the games' frame times, compiled against text on
   the same firmware.
5. The manual (OPTION COMPILE, MM.INFO(COMPILE)) and the release notes.
