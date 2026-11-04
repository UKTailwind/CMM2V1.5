# A C compiler on the Colour Maximite 2

Peter, 4 November 2026: bring the Fuzix compiler of the Pico Computer 3 (PC3) to the Colour Maximite 2 "in a way that
gives maximum advantage for the user", using the H7's hardware double precision; then the MMBasic transpiler, so that
MMBasic programs can be converted, as a second phase; and prove each step with the conformance programs Fuzix uses.

## What is being brought across

Peter's Fuzix fork (github.com/UKTailwind/FUZIX, branch pc3), Applications/CC: the Fuzix Compiler Kit with a PC3
target. Read its REVIEW-2026-08-06.md, STATE.md and NOTES-cc-perf.md first.

- **The chain:** cpp -> cc0 (tokens) -> cc1 (trees) -> cc2 (bytecode, then each function translated to Thumb-2 by
  backend-thumb.c) -> prog.bc -> bcrun. No assembler, no linker.
- **The object (.bc, bytecode.h):** a header, code, data, fixups, symbols and their names. bcrun loads it, relocates it
  to absolute addresses, binds each library symbol to its own runtime by name, and runs it: native functions directly,
  the rest (too big to translate) as bytecode.
- **The native ABI:** r4 = the VM stack, r5 = the helper vector (bcrun's C side: calls, library calls, doubles, the
  string family), r6 = 0 (the memory base, now absolute). Doubles travel as bit patterns in r0:r1 / r2:r3 to the
  aeabi routines through helper slots; on the RP2350 those use its DCP coprocessor.
- **The dialect:** C89 plus declarations after statements; 165 of 175 C89 tests of c-testsuite pass, bitfields the
  only gap. About 142,000 Dhrystones/s on the RP2350 (37% of gcc -O2).
- **Licences:** the compiler passes (cc0, cc1, cc2) GPLv3, the runtime not (the kit's README); cpp from Fuzix.

## Decisions (Peter agreed, 4 Nov)

- **The compiler is a loadable program, never part of the firmware.** GPLv3 code cannot be linked into the firmware
  under the MMBasic licence; a separate program the firmware only starts is fine. It also keeps flash free (bank 1 has
  about 44KB left) and gives the compiler the G2's 8MB (about 7MB lent on a G1) instead of the PC3's 340KB, so its
  size limits (TMAX, THUMB_MAXFN) can rise. Its source must be published with any release that includes it.
- **The passes are ported intact.** Each keeps its main and its files, as on Fuzix; the CMM2 adds only what it must (a
  wrapper for the redirections Fuzix's shell made, the SDK's C library).
- **Doubles use the hardware.** First the helper slots become VFP shims (no compiler change); then backend-thumb.c
  emits VFP inline (vadd.f64 and friends, values in d registers) - the "small modification" of the compiler.
- **The user's view:** `CC "prog.c"` compiles, `RUN "prog.bc" [, args]` runs it as a loadable program does and comes
  back to BASIC, and `RUN "prog.c"` compiles first when the source is newer. Errors as prog.c:line.
- **C programs get the CMM2's own facilities** (graphics, sound, files, keyboard, mouse, I/O) through the firmware,
  not re-implemented copies (the PC3's bcrun carries a 27KB MMBasic runtime of its own).
- **The conformance programs prove every step:** c-testsuite's C89 set, the samples with gcc as the oracle
  (hosttest/optest.sh, all.sh), Dhrystone, and the eclipse benchmark, run on the board and compared.

## Where the runtime lives: decided with evidence, in phase 1c

The recommendation was the runtime in the firmware (bank 2 has about 92KB free in the firmware's sectors, so no
program-flash slot is lost). bcrun turned out to carry much that is the PC3's own (the mm BASIC runtime, a device
server, sockets, the PSRAM heap), so it is ported first as a loadable program, bcrun.app, where it can be proved
without reflashing. Phase 1c then chooses, measured:

- **in the firmware:** RUN "x.bc" with nothing else on the card, the firmware's graphics called directly, C functions
  as CSUBs later; costs flash and ties runtime updates to firmware releases;
- **bcrun.app plus services version 10** (MMBasic's graphics and sound as services, which every loadable program then
  gains, gcc-built ones too): no flash, updated on its own; RUN "x.bc" starts it.

## Phases

1. **Phase 1a - bcrun.app.** bcrun.c built with tools/sdk under a CMM2 platform section (in the fork, beside the
   PC3's): files, console and time through the SDK's C library and the services; memory from the program's heap;
   libm from the C library (hardware FP); the double slots as VFP shims; no sockets, no mm runtime yet; the native
   code's cache maintenance. Proved by running objects cross-compiled on the PC (host-armm0's passes, as on the PC3)
   on the board: the samples against gcc, the C89 conformance set, Dhrystone.
2. **Phase 1b - the compiler on the board.** cpp, cc0, cc1, cc2 and the driver (ccbc.c) built with tools/sdk; a
   compile on the board gives the same .bc as the host's, byte for byte, for every sample and test.
3. **Phase 1c - the user's commands.** CC and RUN of .bc and .c; where the runtime lives (above); the include files
   on the card; the release's zip and a chapter in the Loadable Programs guide.
4. **Phase 1d - speed.** VFP inline in backend-thumb.c; the size limits raised for the CMM2's memory; the review's
   remaining items. Measured with Dhrystone and the eclipse.
5. **Phase 2 - MMBasic programs.** The transpiler (mmb2c, and mmbc, the BASIC front end that compiles straight to
   .bc) with its mm runtime mapped onto the CMM2 firmware's own commands; compared with OPTION COMPILE.

## Progress

**4 November: phases 1a and 1b work on a Generation 2.**

- **bcrun.app** (74KB) runs objects cross-compiled on the PC:
  - All 165 C89 tests of c-testsuite that pass under the host's bcrun pass on the board, and the kit's 35 samples give the host's output.
  - Dhrystone: 306,822 per second, against 1,741,787 for the same source built by gcc -O2 as a loadable program (18%; the PC3 gets 142,000, 37% of its own gcc).
  - A loop of double multiplies, adds and compares runs at 26% of gcc's speed, with slots 4-12 as VFP shims.
- **cc.app** compiles on the board, about half a second per test:
  - It is the driver, ccbc.c, with cpp, cc0, cc1 and cc2 linked in: each pass is linked on its own, its main and exit renamed by objcopy, and the rest made local.
  - All 198 test sources compile to objects identical, byte for byte, to the host chain's (Fuzix's cpp, then cc0-cc2).
  - Compiled and run on the board alone, 160 of the 175 C89 tests pass. The 5 short of the host's 165 are Fuzix's cpp, which the PC3's own compiler shares: 00084, 00097 and 00202 are not compiled (cpp fails on the first two, and cc1 on the third's output), and 00206 and 00212 give the wrong output (00212 wants `__ILP32__` predefined).
- **A bcrun bug for every machine:** main() was entered without argc and argv, which it read from above the stack (zero on a PC, the last program's memory on the CMM2). bcrun now passes the object's name and the words after it.
- **The SDK gained:** creat() and dup2() in syscalls.c (the passes read and write descriptors 0 and 1); build.py links .o files given among the sources.
- **Not yet:**
  - cc.app has 3.3MB of tables (cc2's BIG_TABLES), so it is unproved on a G1.
  - `cc -r` cannot start bcrun (one loadable program cannot start another), which is phase 1c's CC/RUN.
- **Where it is:** the fork's branch cmm2 (github.com/UKTailwind/FUZIX): 7c44807 the argc/argv fix alone, so it can go to pc3 too, and 8038489 the CMM2 platform. apps/cc/make.py fetches 8038489.

## Where the work is

- The compiler, the runtime and their CMM2 platform code: the fork, branch cmm2 (from pc3 6d662a677).
- The CMM2 build: apps/cc/make.py, which fetches the fork at a pinned commit (as apps/pc386 fetches tiny386) and
  builds the programs with tools/sdk.
- Testing: the fork's hosttest/ harness, with a CMM2 runner beside the PC3's devtools/hwbuild.py.
