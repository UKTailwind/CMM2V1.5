# Symbols: porting the PicoMite's S5/S6 to the CMM2

Step 6 of the P1 plan (`PicoMite_Parity_Audit_and_Plan.md`). Written 16 Oct 2026, from two read-throughs:
- the PicoMite at `3ae5b9d`: the design document, `core/Symbols.c/.h`, the introducing commits `e3298b9` (format) and `9cdcfe8` (bindings), and the 28 later fixes;
- every place in the CMM2 at `bcbf405` that reads a name from the program or walks the image.

Line numbers are the CMM2's unless marked P.

## What symbols are

A symbol is a variable, SUB, FUNCTION or label name, or a sub-keyword, in the stored program. It is replaced by an introducer byte plus one or two name characters:
- 0x04/0x05: a program symbol, long (3 bytes) or short (2 bytes);
- 0x06/0x07: a library symbol, long or short.

The 63 most used names get the short form. Up to 4032 names fit in an image. A table after the image holds the spellings.

At run time each name is bound lazily to what it means:
- a global's slot;
- the newest live local of that name, through a per-name shadow stack that a local's creation pushes and ClearVars pops;
- a SUB's index;
- a label's line.

So a variable access no longer hashes and compares the name. On the PicoMite this took 6-19% off run time, and it is the foundation of Route B. The design document is not the spec: where the code differs, the port follows the code.

## What the CMM2 brings to it

- **The image is crunched and upper-cased.** The loader upper-cases everything outside quotes (`FileIO.c:619-628`, except DATA-first and RUN-first lines). So every name has one spelling in the image, and the PicoMite's per-spelling ids collapse to one id per name. The PicoMite's code still works unchanged; it just never aliases.
- **LIST and EDIT use the source file.** Only LIST FLASH and LIST PROFILE detokenise (`llist`). The PicoMite's "LIST round trip" gate becomes LIST FLASH, with symbols against without.
- **T_LABEL is 0x04 here and 3 on the PicoMite.** The PicoMite's introducers are 0x04-0x07, so T_LABEL moves to 0x03, which is free. The byte walkers (`findline`, `TraceLines`, `cmd_cfunction`) and the element walkers all go through the `T_LABEL` macro. 0x05-0x07 never appear in a line body today: tokenise turns control characters into spaces (`MMBasic.c:1579-1582`).
- **There is no LIBRARY or CHAIN yet.** So the PicoMite's library symbols (`c8a1d56`) and the CHAIN binding fix (`8d3ee25`) wait for steps 8/9. Their introducers 0x06/0x07 are reserved now.
- **The variable table is already the PicoMite's (step 4):**
  - stable slots;
  - locals 0-511 and globals from 512;
  - a `level` per record;
  - `hashlist[]` in creation order;
  - ClearVars freeing newest first (`MMBasic.c:3982-4044`), which the shadow stack needs.
- **The name index is already in** (step 4, `MMBasic.c:429-571`): the text lookup the SUB and label bindings sit in front of.
- **The image writers:**
  - `SaveProgramToMemory` builds the image in SDRAM, and `FileLoadProgram` compares it with flash to decide whether to reflash (`FileIO.c:679-699`).
  - `SaveProgramToFlash` writes it. It already does a pre-pass over the whole text (`CollectIncludes`), and cleans the D-cache after programming (`main.c:3627`).
  - After the text come zeros to a 32-byte boundary and a `0xFFFFFFFF` word (`flash.c:136-149`), then the CSUB and font records. The table goes between the marker and the records, and `PrepareProgramExt` (`MMBasic.c:625-627`) must step over it.
- **The line profiler** keys its counts by each line's byte offset and needs lines at least 8 bytes apart (`FileIO.c:663-668`, `main.c:3602`). Symbols shorten lines, so profiling is part of the gate.
- **Memory:** 24 MB of SDRAM heap and on-chip D2 RAM. The PicoMite's hot/cold split maps across directly:
  - the hot block (canonical map, global and local slots, SUB indexes, local shadows) goes in on-chip RAM (`GetInternalMemoryNull`);
  - the cold block (spellings, labels, hash) goes in SDRAM;
  - the save-time collector (32 KB) comes from SDRAM.

## The port, in the PicoMite's order

### 6.0 Preparation (no format change yet)

One commit each. The full regression must be unchanged apart from the fixes.
1. **T_LABEL 0x04 to 0x03, with a new image format stamp.**
   - The stamp lives in `ImageIsCurrent` and the boot path `main.c:618-638`. An image in the old format is erased at boot and, with AUTORUN, reloaded from its file. The existing path does this for the format before step 3.
   - Every walker uses the macro; grep for literal 4s near label code.
2. **`commandtbl_at` in every scanner** that decodes a statement start (P `7aeafe8`).
   - A statement starting with a one-letter SUB name, or with a symbol, must never be read as a command token.
   - The CMM2 uses it in `ExecuteProgram`, `GetNextCommand` and `cmd_cfunction`; audit the rest.
3. **Names at most 31 characters on every path** (P `2c747af`). Today a 32-character name has no terminator in its record, and `skipvar` already refuses some 32-character names that `findvar` accepts (inventory 7.4). This is a behaviour change: 32-character names become "too long".
4. **STATIC's hidden name.**
   - Separate the SUB name and the variable with 0x1E, which `isnamechar` must accept: STATIC C in SUB AB and STATIC BC in SUB A both make ABC today.
   - Bound the `VarName[65]` copy in `cmd_dim` (`Commands.c:2774`, `2819`).
   - Take the PicoMite's `StaticVarName`/`StaticLink` (P `74ed383`, `b0f448e`).
5. **A word of zeros after the end marker** in both writers (P `bc0c157`). A false single-line IF on the last line can read the marker when the text ends exactly on a 32-byte boundary.
6. **Existing bugs found in the inventory** (none run on the board yet):
   - `TraceLines` stops at END TYPE's 0xFF byte, so TRACE LIST loses every later line number;
   - `NEXT JI` closes `FOR I` (FOR's NEXT search, `Commands.c:1771`);
   - the `SPI`/`SPI2` substitutions break `SPI(`/`SPI2(`;
   - a DATA-first line keeps lower-case names, which READ then looks up case-exactly;
   - `importfile` never drops MMDEBUG lines;
   - a program with a CSUB is reflashed on every RUN, because `SaveProgramToMemory` stores SDRAM addresses in the records.

   Each is checked on the board first, then fixed or noted.

### 6.1 The format (`e3298b9`, at today's speed)

- **`Src/Symbols.c` and `Inc/Symbols.h` from the PicoMite, without:**
  - the RAM/flash placement macros, the RP2040 out-of-line `SymSpelling`, the PSRAM branches;
  - Route B (`SymCanonById`, `SymIdAt`, the generation counters `SymBindEvent`/`SymBindGenG`/`SymLevelGen`/`SymLocalEvent`);
  - library saving (until LIBRARY exists).
  
  The same conversion as MATHS.c: a script for the mechanical changes (char types, `VarIndex`, `OptionBase`), and the platform edits by hand.
- **`tokenise`:**
  - a name at the name path goes to `SymName` (P `MMBasic.c:2377-2389`);
  - DATA statements stay text (`symrawstmt`); CSUB and DefineFont blocks stay text (`SymRawBlock`);
  - the letter after `~(` stays raw (`symrawnext`), and the letters after `&` stay raw (`&HFF`);
  - a `.` after `)` and before a name is copied alone (P `2112-2120`). This also fixes the CMM2's split of a member starting with E (`pts(1).E1`);
  - console lines (`tokenise(true)`: the prompt, EXECUTE, EVAL, CALL "x", MM.STARTUP/MM.PROMPT) never make symbols.
- **The savers.** Both `SaveProgramToMemory` and `SaveProgramToFlash` run the PicoMite's sequence: `SymBegin`, `SymCount` (pass 1), `SymRetry`, `SymRank`, then the emit pass, `SymTableWrite` and `SymEnd`.
  - `SymCount` must read exactly the lines the saver reads, so it reuses `LineSource` and the name-line pass rather than a copy of them.
  - Both builds must give the same bytes, or every RUN reflashes.
  - The table is written after the marker and before the CSUB records.
  - The table is skipped by `PrepareProgramExt` (with the strict-aliasing read of `e0515d3`), by MEMORY (which counts it as program) and by the boot image check.
- **Every reader of names.**
  - **Decoding:** `findvar`'s text path decodes a symbol into its name, upper case, at most 31 characters. `skipvar` steps 2 or 3 bytes, decoding only when the length test needs it (`b44dacc`). `getvalue` treats a symbol as a name start.
  - **Keywords:** `checkstring` matches a symbol's spelling (P `MMBasic.c:8429-8464`). That covers about 900 sub-keyword tests at once.
  - **SUBs and labels:** `FindSubFun` and `findlabel` read a symbol's spelling. So do `DefinedSubFun` (definition and caller names, and the suffix compare of 7.1), `SubLayoutBuild` (BYVAL/BYREF through `checkstring`, not `toupper(p[0])=='B'`), and `PrepareProgramExt` (SUB, FUNCTION and CSUB names, and TYPE names through `NameView`).
  - **Structures:** `StructDefine`, `ParseStructMember`, `FindStructType`, `ResolveStructMember`'s continuation and `StructMemberAt`; `CheckIfTypeSpecified` steps over a type name.
  - **Other parsers:**
    - `getargaddress`, the CSUB argument classifier and `SPI.c:286`;
    - `LetArrayCopy`;
    - FOR's NEXT search (`SymFindName`);
    - `PRINT #GPS` (`SymNameEqual`).
  - **Output:** `llist` spells symbols out (LIST FLASH, LIST PROFILE and its CSV). `error()` spells out its `$` arguments, and the "Invalid character" and argument-text messages show names, not bytes.
- **Commands and functions that are not symbol-aware get a spelt-out copy** of their statement or argument text (`SymExpandStatement`, `SymExpand` in `getvalue`). They see exactly what they see today.
  - **The aware list starts as the PicoMite's core:**
    - control flow;
    - LET, PRINT, INC, GOTO/GOSUB, CALL;
    - DIM/LOCAL/STATIC/CONST, SUB/FUNCTION/CSUB, DATA, REM;
    - TYPE/END TYPE.

    Those must be aware: they keep pointers into their statement or search the program from it.
  - **The CMM2's own graphics and other commands stay spelt out** until each handler is checked: it must not read a bare word's letters (TEXT's justification, as in P `9296e9c`), step back from `cmdline` (use `CmdTokenPtr`, as PAUSE and GUI CLICK do), or keep `cmdline` past the statement. That checking belongs to P5, since spelling out is correct, only slower.
  - A command that needs a big aligned or contiguous block while it runs must allow for the temporary copy at the heap base (P `d8efcc0`).
- **Fixes carried in from the PicoMite** (its checklist 7.1 and 7.2):
  - CASE IS through `checkstring` (`IS` may be 2 bytes);
  - `DefinedSubFun`'s suffix compare;
  - BYVAL/BYREF through `checkstring`;
  - MEMORY counting the table;
  - `SymRetry`;
  - fonts after the table (`e0515d3`);
  - the word of zeros (6.0.5);
  - names of at most 31 characters (6.0.3).
- **A test switch.** As P `SYMBOLS_TEST_SWITCH`, a build define that lets `OPTION SYMBOLS ON|OFF` at the prompt choose how the next load saves. The A/B gates need it, and it is off in release builds.

**Gate:**
- every goldens corpus program and Testfiles program saved both ways, with LIST FLASH and RUN output identical;
- the full regression unchanged;
- the PicoMite's `s5t1.bas` (names in every role) as a `tests/cmm2` program;
- a CMM2 `barewords` test: every unquoted-word form of an aware command or function, run at the prompt (text) and in a program (symbols), with the outputs matching;
- profiling (LIST PROFILE) on a program with short lines;
- a new `tests/cmm2/symbols.bas`: dotted names, members after an index, STATIC names, 31-character names, labels, CSUB and font after the table, `PEEK(PROGMEM)` reading a symbol.

### 6.2 The bindings (`9cdcfe8`, `b44dacc`)

- **Setup:** `SymBindInit` in `PrepareProgram`, which also runs before each prompt line. Everything it allocates must fail softly (`GetMemoryNull`, P `db140a5`). With no memory the program runs on the text path.
- **The `findvar` fast path:**
  - a bound symbol's local at the current level, else its global unless text locals are alive;
  - bind on find, and on create (`040c21c`);
  - `SymLocalMade` when a local is made, `SymLocalFreed` in ClearVars, `SymBindReset` in `ClearVars(0)`;
  - `SymBindForgetSlot` in ERASE, and in REDIM when it comes;
  - `SymBindForget` in `InitHeap`;
  - `SymBindFree` before anything takes most of the heap (`c10bc71`). On the CMM2 that is the editor, ZMODEM/XMODEM and the file manager.
- **SUBs and labels:** `FindSubFun` caches any kind and checks the kind on every call (`4fe3615`); `findlabel` caches the label line.
- **The aware list:** Const (`bb4540e`) and Call (`b494e10`).
- **`OPTION SYMBOLS OFF`** as the first statement runs without bindings, giving back the heap (`50ada44`). The CMM2 has heap to spare, so it is mostly a fallback, kept for parity.
- **Gate:** as 6.1, plus run-time figures before and after, which are recorded but don't decide anything: `bench_micro`, `perfguide` and `solar_eclipse`.

### Later

- **Library symbols (`c8a1d56`)** with LIBRARY (step 9).
- **The CHAIN binding rebuild (`8d3ee25`)** with CHAIN (step 8).
- **`SYMC_NOSUB` and the symbol half of F3a,** if they measure well.
- **Making more CMM2 commands symbol-aware,** in P5.

## Behaviour changes

| Change | Detail |
|---|---|
| Names are at most 31 characters | 32-character names were accepted unevenly before |
| STATIC hidden names change | The new names no longer collide (0x1E). VAR SAVE does not store STATICs |
| `PEEK(PROGMEM)` and MM.INFO(CODE SIZE) | see symbol bytes and a larger or smaller image. MEMORY's program figure includes the table |
| An old image at boot | erased and reloaded from its file |
| New error | "Program damaged: symbol table" |
| New option | `OPTION SYMBOLS OFF` |

## Size

- **Firmware:** Symbols.c without Route B and PSRAM is about 800 lines (8-10 KB of flash). The consumer edits are a few hundred lines across MMBasic.c, Commands.c, Struct.c, MM_Misc.c, Draw.c and main.c/FileIO.c.
- **Heap:** about 20 bytes a name at run time; 32 KB at save time only.
- **Images:** large programs shrink and small ones may grow by the table. On the PicoMite's figures, solar_eclipse grows by 132 bytes and Sandglass shrinks by 18 KB.

## Commits

One commit per mechanism, each gated: 6.0.1-6.0.6, then 6.1 in three or four commits (Symbols.c and tokenise behind the off switch; the savers; the readers; switching it on), then 6.2 in two (the bindings; OPTION SYMBOLS OFF). The full regression runs after 6.0, after 6.1 and after 6.2.
