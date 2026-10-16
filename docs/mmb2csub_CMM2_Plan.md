# mmb2csub for the CMM2: MMBasic SUBs and FUNCTIONs as M7 CSUBs

Step 6b of the P1 plan (`PicoMite_Parity_Audit_and_Plan.md`).

Peter, 16 Oct:

> Once symbols are done the next thing is to create a version of mmb2csub for the CMM2. This will need additional entries to the calltable, support for direct arithmetic rather than calling helper functions and the python script will need changes to build with H/W floating point for the M7 rather than m0+. In the Picomite this work then informed the creation of the compiler so it is a pre-cursor for that. The first test on the Picomite was solar_eclipse and changing the main calculation function and all its sub functions into a csub.

Written 16 Oct 2026 from:
- the PicoMite at `3ae5b9d`: `user-tools/mmb2csub.py`, `armcfgen.py`, `mmcsub.h`, `PicoCFunctions.h`, `docs/mmb2csub.md` and `docs/armcfgen.md`;
- the CMM2 at `aec2d8c`.

P marks a PicoMite file.

## How the PicoMite does it

- **mmb2csub.py:**
  1. It translates the whole program with mmb2c (`mmb2c.py`; the canonical copy is in the FUZIX tree, `Applications/mmb2c`).
  2. It slices out the chosen routine and its call-graph closure.
  3. It moves LOCALs onto the C stack and drops STATICs. Globals are reached through CFuncRam, as a `__gt()` table of `PEEK(VARADDR)` addresses when there are more than 10 arguments, and at most 16.
  4. It puts an `mm_poll()` (CheckAbort) in every loop and adds a `long long entry(void *...)` shim.
- **armcfgen.py** compiles and links at address 0, then writes the CSUB block and, if one is needed, a BASIC wrapper.
  - The code is position independent and text-relative (`-fpie -mpic-data-is-text-relative -msingle-pic-base`), with no GOT and no relocation. `.data` and `.bss` must be empty.
  - There is one profile, Cortex-M0+, used for both chips: `-mcpu=cortex-m0plus -mthumb -ffreestanding -fno-exceptions -ffunction-sections -fdata-sections -fno-jump-tables -Os` (P `armcfgen.py:40-53`).
- **The generated C uses plain operators.** The M0+ has no FPU and no 64-bit multiply, so GCC calls `__aeabi_dadd`, `__aeabi_lmul` and the rest. `mmcsub.h` defines those in the blob as one-line calls to the CallTable's `FAdd`/`FMul`/`LMul`/... slots (P `mmcsub.h:540-590`).
- **The CallTable** (`PicoCFunctions.h`) has 112 slots, 0x00-0x1BC.
  - It is found as `((unsigned *)VTOR)[7]`, which the firmware writes at the top of `main()`.
  - The slots are append-only.
- **First test:** `sefunc` from `solar_eclipse.bas`, 15 of its 20 profiled routines, 36.9 KB of code (P `Bas/bench/sefunc.bas`).
  - PC3 at 378 MHz: 13.2 s interpreted, 2.5 s with the CSUB.
  - PICORP2350 without a display: 12.1 s against 1.5 s.

## The CMM2 now

- **CallTable.**
  - 57 slots, 0x00-0xE0, at the fixed address 0x080002A0 (`.cspace`, first in `.text`; `CFunctions.c:35-93`, `ARMCFunctions.h`).
  - The slots match the PicoMite's up to 0x7C. After that they are the CMM2's own: ScrollBuffer, DrawBufferFast, IntToFloat at 0xA8, FloatToInt64 at 0xAC, sin at 0xD4, and so on.
- **Bug: slot 0x78 holds the function `CallCFuncmSec`.** The header reads the slot as the address of the variable `CFuncmSec`, as the PicoMite's does (P `PicoMite.c:508`: `&CFuncmSec`). A CSUB that sets `CFuncmSec` therefore writes into flash, and its millisecond routine never runs. To be fixed in 6b.1.
- **A call** has at most 10 pointer arguments and returns `long long` in r0:r1. The entry is the code start plus 4 × the first hex word (`CFunctions.c:104-200`).
- **VTOR is 0x08000000.** The vector table is in flash, and slot 7 is `.word 0` (`startup_stm32h743xx.s`). It cannot be written at run time the PicoMite's way.
- **The firmware is hard-float** (`-mfpu=fpv5-d16 -mfloat-abi=hard`). Every CallTable function that takes or returns a double expects it in d0-d7.
- **Where a CSUB runs.**
  - Normally it runs from program flash.
  - A program run from RAM is at 0xD0780000 (G2) or 0xD0300000 (G1). The MPU allows execution in both regions (`main.c:2916`, `:2960`).

## The port

### 6b.1 The firmware: a second CallTable in the PicoMite's layout

- **`CallTableP`** is a new constant table in `.text`, with every slot of `PicoCFunctions.h` at the same offset. Its address goes into vector slot 7 at link time (`.word CallTableP` in the startup file), which `BaseAddress` then finds unchanged.
- **The slots the CMM2 lacks** are written as the PicoMite writes them.
  - **Maths:** Cosine, Sqrt, Atan2, Power, Log, Tan.
  - **Single precision:** SAdd ... ItoS.
  - **64-bit integer:** LDiv, LMod.
  - **The M0+ helpers:** FMul, FAdd, FSub, FDiv, FCmp, LoadFloat, IDiv, IMod, LMul, LShl, LAsr, LLsr. They keep their one-line implementations, so the header stays shared.
  - **Library:** memcpy, memset, memmove.
  - **Strings:** MtoC, CtoM, Mstrcpy, Mstrcat, Mstrcmp. The Str* cores (StrLeft ... StrFormat, StrVal) are factored out of the CMM2's functions. 6a has just merged LEFT$, RIGHT$, UCASE$ and LCASE$ into `fun_schange`.
  - **Timers and pins:** TimerVal, RndVal, TimerSet, PinVal, PinPut.
  - **Variables:** gui_fcolour, gui_bcolour.
  - **CSubComplete, AudioOutput and CFuncInt3/4,** as the CMM2's interrupt code allows.
- **What the CMM2 doesn't have gets a stub that raises an error:** PIOExecute and AUDIO_WRAP.
  - The display slots (WriteBuf, FrameBuf, LayerBuf, DisplayBuf, Display_Refresh) point at the CMM2's page addresses, or a no-op where there is no equivalent.
  - DrawPixel is the CMM2's.
- **Arguments go to 16,** as the PicoMite's (`CallCFunction` and the type list).
- **The old table at 0x080002A0 stays** (question 1), with slot 0x78 corrected.

### 6b.2 The runtime header for the M7

- **`PicoCFunctions.h` can be used as it is,** since the offsets are the same. What differs is how the blob is compiled.
- **`mmcsub.h` gets an M7 branch** (`#if __ARM_FP`).
  - **Leave out:** the `__aeabi_d*` arithmetic and comparison shims and `__aeabi_lmul`. With the FPU, GCC emits VFP instructions and an inline 64-bit multiply, so these are never called.
  - **Leave out:** the 32-bit divide shims, since the M7 has SDIV/UDIV. This also avoids the M0+ branch's `__aeabi_uidiv` going through the signed IDiv.
  - **Keep:** the int64↔double conversions (`__aeabi_l2d`, `ul2d`, `d2lz`, `d2ulz`) and the 64-bit divide (`__aeabi_ldivmod`/`uldivmod`, if the generator lets one through).
- **The trap:** in a hard-float build, GCC calls the `__aeabi_*` helpers with the base calling convention (doubles in r0-r3). The shims must be declared `__attribute__((pcs("aapcs")))`, otherwise they read d0 and return garbage. A test in 6b.4 checks each conversion against the interpreter.
- **Variable 64-bit shifts** should be inline on ARMv7-M. If the link map shows `__aeabi_llsl`/`lasr`/`llsr`, their shims stay.

### 6b.3 The scripts

- **armcfgen.py gets a `--cpu cortex-m7` profile:**
  `-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb`, plus the same PIC, freestanding and section flags.
  - `-fno-jump-tables` is not needed, because Thumb-2 TBB/TBH are position independent.
  - The link script is unchanged.
- **mmb2csub.py gets `--target cmm2`.** It selects that profile and the M7 branch of mmcsub.h, and allows the CMM2's argument count.
- **Where the scripts live** is question 2.

### 6b.4 Tests

1. **A small CSUB first,** built by hand with armcfgen, against the interpreter. It covers:
   - double and int64 arithmetic;
   - each conversion in both directions, including negative values and values over 2^31;
   - LDiv/LMod with negative operands;
   - Sine/Cosine/Sqrt/Atan2/Power;
   - a string through StrLeft and StrFormat;
   - a loop that CheckAbort can break with Ctrl-C;
   - a program run from RAM (I-cache coherency after loading).
2. **solar_eclipse,** with `sefunc` and its closure as a CSUB.
   - **The gate:** the golden JD values (2451904.14541560 / .19690359 / .25420764) and the rest of the output, the same as interpreted.
   - **Recorded:** the time taken against 7.77 s interpreted with symbols, and 9.26 s as text.
3. **The regression,** unchanged, plus the crasher suite.

### 6b.5 Afterwards

- **Docs:** a CMM2 section in mmb2csub.md, covering the flags, the M7 runtime, the limits and the two tables.
- **What 6b shows feeds the compiler (Route B) on the M7,** as it did on the PicoMite.

## Progress, 16 Oct

- **6b.1 is done** (the firmware).
  - `CallTableP` has all 112 slots in the PicoMite's layout and is reached through vector slot 7. On the board, `BaseAddress` finds it at 0x080C8674.
  - The string cores (StrLeft ... StrFormat, StrVal), RndVal, TimerVal/TimerSet and PinVal/PinPut are factored out of the CMM2's functions.
  - A CSUB call takes 16 arguments, and slot 0x78 of the old table is corrected.
  - The full regression is unchanged from 6a.
- **6b.2 and 6b.3, as a prototype.** `tools/mmb2csub_cmm2.patch` holds the changes to the PicoMite's `user-tools` (armcfgen.py, mmb2csub.py, mmcsub.h), until question 2 settles where they go:
  - armcfgen gets `--cpu cortex-m7`, and mmb2csub gets `--target cmm2`;
  - mmcsub.h gets its M7 branch, with `MMCSUB_RTABI` on every run-time-ABI helper;
  - every CMM2 CSUB's first line ends with `' Cortex-M7 (CMM2)` (question 3).
- **The trap, met:** mm_int and mm_fix call `__aeabi_d2lz` directly from C. Its earlier declaration lacked `pcs("aapcs")`, so INT() and FIX() read the double from the wrong registers and came back 0. Every declaration of such a helper now carries `MMCSUB_RTABI`.
- **6b.4:**
  - **`tests/cmm2/csubm7.bas`:** two routines as CSUBs against the same routines interpreted, covering double and int64 arithmetic, values over 2^31, `\` and MOD of negatives, INT/FIX/CINT, the maths functions and a loop. They agree exactly (`bad 0`), as symbols and as text.
  - **solar_eclipse with `sefunc` as a CSUB:**
    - The CSUB is 26.2 KB, against 37 KB built for the M0+, and the program text goes from 70 KB to 130 KB.
    - The output is identical to the interpreted run, including the golden JD values.
    - **0.480 s**, against 7.77 s interpreted as symbols and 9.26 s as text: 16 to 19 times faster. The PicoMite PC3 takes 2.5 s with the same CSUB.
- **Found:** mmb2csub's default output comments out the original routine with `/* ... */`, which the CMM2 does not have. `--lean` avoids it. The fix is either block comments in the CMM2 (a PicoMite parity item), or `'` comments under `--target cmm2`.
- **Ctrl-C** breaks out of an endless M7 CSUB loop through `mm_poll()`. The board is back at the prompt and healthy.
- **OPTION RAM:** M7 CSUBs run correctly from SDRAM. solar_eclipse takes 0.514 s there, against 0.480 s from flash. The test found a bug, fixed in b71c5bd: in RAM mode any program saved as symbols looped on "Program damaged: symbol table", because three of the RAM-mode flash writers left `realflashpointer` behind. The area is write-back cached SDRAM, so a save to RAM now also cleans the D-cache and invalidates the I-cache, so the CSUB's code is certain to be fetched as written.
- **Left:**
  - the answers to the questions below;
  - the docs (6b.5).

## Questions for Peter

1. **Keep the old CallTable?** Keeping the old 57-slot table at 0x080002A0 next to the new one costs about 230 bytes of flash. Existing CMM2 CSUBs would keep running without a rebuild, apart from any that use slot 0x60, which has been `&slotrec` since step 4. The decision of 15 Oct was the PicoMite layout through slot 7, with existing CSUBs rebuilt. Both can be done. Recommended: keep it.
2. **Where should the scripts live?**
   - (a) In the PicoMite's `user-tools` with a `--target cmm2` option: one copy, sharing mmb2c and its fixes. Recommended.
   - (b) A copy in this repository's `tools/`.
3. **Should a CMM2 blob carry a mark** that the firmware checks, so that a CSUB built for the PicoMite's processors is refused instead of run? The CSUB format has no field for one. The entry-offset word's top half is free (offsets are under 2^16 words), so `--target cmm2` could put a signature there. The catch: today's CMM2 CSUBs have zero there too, so they could not be told apart from PicoMite ones.
