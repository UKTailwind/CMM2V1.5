# Threads on the PicoMite: a plan

25 November 2026. Peter: "prepare a plan for implementing threads on the PicoMite".

This is a plan for bringing the Colour Maximite 2's BASIC threads to the PicoMite firmware (RP2040 and RP2350). It
assumes the reader knows the PicoMite's source and has `docs/Threads_Plan.md` (the CMM2's plan, with what was built
and measured at each stage) to hand. Nothing here has been built. The PicoMite facts come from a read of its source
at d68c9e6; file and line references are to that tree. Figures marked *to measure* are not known yet.

## What is being ported

On the CMM2 (branch `threads`, `Src/Thread.c`, about 1,400 lines, 12 KB of code):

- A thread is a SUB running on a C stack of its own, with a record that holds its interpreter state. The main
  program is thread 0; threads are numbered 1 to 16.
- A switch happens only between BASIC statements, at the statement tail, when the 1 ms tick has flagged the end of
  a time slice, or when the running thread waits or yields. A statement is never split.
- Each thread has its own FOR, DO, GOSUB and DATA stacks, temporary-memory list and local-variable list. The
  interpreter's names for them are pointers that a switch re-points; nothing is copied.
- Each thread has its own range of local-variable slots, its own print position, colours and font, and its own
  error state.
- PAUSE, INPUT, frame waits and the network's waits let the other threads run.
- An interrupt routine runs in the thread that set it up.
- A thread's priority is the number of time slices in its turn.

The BASIC is to be the same on both machines, so a program with threads runs on either:

`THREAD START n [SIZE kb] [LOCALS m] [NEST d] [PRIORITY p], sub [, args]`, `THREAD EXIT`, `YIELD`, `STOP n`,
`PAUSE n`, `RESUME n`, `SLICE ms`, `SIZE`, `PRIORITY n, p`, `LOCK k [, ms]`, `UNLOCK k`, `HOLD`, `RELEASE`,
`WAIT n | FLAG f [, ms]`, `LIST`; `MM.INFO(THREAD)`, `MM.INFO(THREAD n)`, `MM.INFO(THREAD LOCK k)`.

## What the PicoMite already has that helps

| Fact | Where | Why it helps |
|---|---|---|
| Locals are a stack in slots 0 to `maxlocalvars`-1, listed in `g_hashlist` with their level | `core/MMBasic.c` 329-335, 5341-5370 | The CMM2's local region is this design (it came from the PicoMite). A slot range for each thread works the same way |
| A FUNCTION body runs in a nested `ExecuteProgram`; SUB, GOSUB and interrupt routines do not nest | `MMBasic.c` 3681-3733 | As the CMM2: a thread needs a C stack of its own, and only FUNCTION depth uses it |
| `cmd_pause` already waits on `time_us_64()` | `core/MM_Misc.c` 703-760 | The CMM2 had to move PAUSE off a tick counter first. Here only its function-static end time has to become the thread's own |
| The interpreter's limits are already small: `MAXFORLOOPS` 20, `MAXDOLOOPS` 20, `MAXGOSUB` 50, `MAXTEMPSTRINGS` 64, `MAXRESTORE` 8 | `configuration.h` 632-636, 723 | A thread's own lists are about 1 KB at the full sizes; NEST can cut them further |
| `misc/Loader.c` already runs code on a process stack (PSP, with PSPLIM on the RP2350) in the MMBasic heap, and switches to the main stack for firmware services | `Loader.c` 1255-1338, 1940-2066 | The mechanism proposed below for threads' stacks exists and works on both chips |
| Bind stamps: `gen = SymBindGenG + SymLevelGen[g_LocalIndex]` | `core/Stream.c` 4930-4950 | Same as the CMM2, so the same fix applies (see *Traps*) |
| The network code is the code the CMM2 copied | `net/` | The CMM2's yields in the network's waits (`tools/net_yield.py`) and its one-owner rule go back unchanged |

## What is different, and what to do about each

### 1. Memory: the stack is the cost, not the record

| | CMM2 | PicoMite RP2350 | PicoMite RP2040 |
|---|---|---|---|
| MMBasic heap | 4 MB or more | 108 K (PC3) to 364 K | 108 K to 156 K |
| Main C stack | about 27 KB | 8 to 16 KB budget, 2 to 7 KB of margin | 8 KB budget, 0.5 to 4.5 KB of margin |
| A default thread | 26.6 KB (20 KB stack, 6 KB of it a guard) | too much | far too much |

On the CMM2 a thread's stack carries a 6 KB guard, because three things run on whatever stack is current: the
deepest command (LOAD PNG, about 5.6 KB), the housekeeping in `CheckAbort`, and the hardware interrupts. The
PicoMite cannot pay that for every thread. Three measures, in this order of value:

**a. Threads run on the process stack (PSP); interrupts and the housekeeping stay on the main stack.** With
`CONTROL.SPSEL` set while a thread other than the main program runs, every hardware interrupt uses the main stack
(MSP), not the thread's. `CheckAbort`'s housekeeping (`routinechecks`: the USB host, audio refill, `ProcessWeb` with
lwIP and TLS under it, `ProcessBT`) is called through a small veneer that runs it on the main stack too, as the
Loader runs firmware services. A thread's stack then holds only what the interpreter itself puts there: the
evaluator and the command in hand, and its FUNCTION nesting. Both chips have PSP and `CONTROL.SPSEL` (ARMv6-M and
ARMv8-M); the Loader uses them today.

**b. Commands that need a deep stack test for it themselves** (Peter's idea of 25 November: "have stack hogging
commands test the stack before starting processing ... A thread that calls LOAD PNG at the top level and doesn't
call any subroutines may only require 6K in total"). A helper, `StackNeed(bytes)`, called at the start of each such
command, gives the error "Not enough stack (thread n needs SIZE k)" where it would otherwise overrun. The list comes
from measurement (below); the candidates are the file system (littlefs and FatFs with long names), LOAD PNG and JPG,
JSON$, EVAL and EXECUTE, the TLS client, SQL and the FM. The general guard is then only what an ordinary command
needs.

**c. On the RP2350, PSPLIM is the thread's stack floor**, so an overrun is a fault at the instruction that causes
it, turned into the BASIC error "Thread n overflowed its stack", not silent damage to the heap. On the RP2040 there
is no limit register: the check word at the bottom of the stack and `TestStackOverflow` are the protection, as on
the CMM2.

Expected sizes, all *to measure*: a FUNCTION level costs about 770 bytes as text (`ExecuteProgram` 216, the
statement and evaluator 360, `DefinedSubFun` 192: `docs/Interpreter_RouteB_Design.html` 515). A thread that calls
SUBs only should run in about 2 KB; a default of 4 KB (three or four FUNCTIONs deep) with SIZE from 2 KB upward is
the target, in Kbytes as on the CMM2 but with fractions allowed (SIZE 2.5). A record with NEST 8 and LOCALS 16 is
about 1.5 KB. So a small thread is about 4 KB and a default one about 6 KB, against 26.6 KB on the CMM2: sixteen
small threads fit in an RP2040's heap.

### 2. The stack check assumes the main stack

`TestStackOverflow` reads MSP and compares it with `heapend` (`MMBasic.c` 1466-1486). The heap is below `heapend`,
so a stack in a heap block fails that test at once, and with PSP threads the test reads the wrong register. It
becomes: the running thread's own floor (`StackFloor`, `stackwarn`, set at each switch, as on the CMM2) compared
with the stack pointer in use. `MM.INFO(STACK)`, `MM.INFO(STACKPEAK)` and the PC sampler (`MMBasic.c` 273-282,
`MM_Misc.c` 9640-9671) read MSP by name and need the same care.

### 3. Thread stacks must be in SRAM

On the RP2350 the PSRAM cannot be read while the flash is written, and `GetMemory` may return PSRAM
(`Memory.c` 2373-2382). A thread's stack and record come from `GetSRAMMemoryNull`, as the Loader's stack does. A
whole program save runs with interrupts off (`FileIO.c` 8397-8469): no tick, so no switch, which is right.

### 4. The landing after an error or Ctrl-C

`error()` and `CheckAbort` end with `longjmp(mark)` into `main()`'s frame (`PicoMite.c` 4538), from whichever
stack is current. With PSP threads the jump leaves `CONTROL.SPSEL` set and MSP holding the place the main program
was switched out at: the next interrupt would then write below live frames. The first thing at the landing, before
anything else and in assembler, is: if SPSEL is set, clear it (the stack pointer the jump restored is then MSP).
Then, as on the CMM2: note where each thread was (`ThreadsStopped`), end them (`ThreadsEnd`, on the main stack
only), print the list after the error's message. The same applies to `jmprun` (EXECUTE "RUN ..."), which a thread
may not take: RUN, CHAIN, NEW and the other program-replacing commands are the main program's only.

### 5. The Loader and threads use the same register

`Loader.c` assumes BASIC is on MSP and takes PSP for the loaded program or the SQL module. Two rules keep them
apart, as on the CMM2: no thread switch while a loadable program runs, and in the first version only the main
program may start one (RUN "x.app", SQL, a USER module: "Not allowed in a thread"). Later the Loader's entry and
exit can save and restore the caller's PSP and SPSEL, which is a small change, if SQL from a thread is wanted.

### 6. The statement tail

The tail is `if (!OptionNoCheck) { CheckAbort(); check_interrupt(); }` (`MMBasic.c` 2826-2841), and
`check_interrupt` leaves at once if `IntReady.any` is 0. The slice flag goes into `IntReady` as a third field
beside `count` and `poll` (the CMM2's `IntReady.tick`), set by `timer_callback` (`PicoMite.c` 2428), so the quick
exit stays one test. The compiled route's `RBTail` (`Stream.c` 4013-4051) already looks when `IntReady.any` is set
or 100 us have passed, and chained statements look every eighth: a slice ends within a few statements either way.

**OPTION NOCHECK ON** skips the tail, so a thread that sets it would never give way. Proposed: with a thread
running, NOCHECK still skips `CheckAbort` and the interrupts but not the slice test (one test of `IntReady.any`).

### 7. Interrupts

Every vector is a single global and `checkdetailinterrupts` scans them in a fixed order (`MM_Misc.c` 10802-11183):
the same shape as the CMM2's `IntScan`. The CMM2's scheme carries over: the routine that takes an interrupt's
address records the thread that set it up, each source in the scan is taken only if its owner can take it now
(`IntMay`), and a source held back is found again. The saved ON ERROR state for an interrupt is one set of globals
(`MM_Misc.c` 220-224), which is enough because no switch is made while an interrupt routine runs.

### 8. Waits

| Wait | Now | With threads |
|---|---|---|
| PAUSE | loops on `time_us_64()` with `CheckAbort` and `check_interrupt` | the thread sleeps until its time; the main program's loop stays the idle loop |
| INPUT, LINE INPUT | `MMgetline` loops on `CheckAbort` and `MMfgetc` (`PicoMite.c` 1604-1631), no interrupts | one look, then the other threads (`ThreadPoll`) |
| FRAMEBUFFER WAIT | busy loop on the scan line, no `CheckAbort` (`FrameBuffer.c` 1945-1955, 1245-1252) | needs a signal from the scan-out on core 1 (a frame count the thread waits on); *to design per display type* |
| SYNC, ADC without interrupt, TEMPR | C loops (`External.c` 616-664, 5698-5701; `Onewire.c` 207-223) | sleep or poll, as the CMM2's |
| WEB waits | `ProcessWeb(0)` loops on `Timer4` (`MMTCPclient.c`, 13 places) | the CMM2's `NetYield` after each poll, and the network belongs to one thread |
| Bluetooth connects | loops in `BTAudio.c`, `BTSerial.c`, `BTClassicKeyboard.c` | as the network: one owner |
| I2C, serial TX full, XMODEM, flash writes | C loops | they hold every thread, and the manual says so |

### 9. Display state

`CurrentX`, `CurrentY`, `gui_fcolour`, `gui_bcolour`, `gui_font`, `PrintPixelMode` (`Draw.c` 113-119) become each
thread's own, as on the CMM2. The PicoMite has no pages; its equivalent is the FRAMEBUFFER WRITE target (`WriteBuf`
and, on the non-VGA builds, the five drawing function pointers that `setframebuffer` swaps: `FrameBuffer.c`
160-315). Proposed: each thread has its own target (N, F or L), applied at a switch only when it differs from the
running one's; FRAMEBUFFER CLOSE is refused while a thread writes to what it would free.

### 10. Core 1

Core 1 carries the video scan-out or the panel refresh on every build but the WEB ones, and stays as it is. Threads
are all on core 0: they are not parallel, exactly as on the CMM2, and the manual's promise that a statement is
never split depends on it. The core 1 stack canary in the statement tail is unaffected.

### 11. Code in RAM on the RP2040

The RP2040 builds have 0.5 to 4.5 KB of RAM to spare, and every byte of code in RAM moves the stack's floor up
(`docs/Interpreter_RouteB_Design.html` 515, 590). All of `Thread.c` goes in flash. What must be in RAM is only what
is added to functions already there: the slice test in `check_interrupt` (a few instructions) and the pointer
loads where the stacks were arrays. Each build's margin is checked before and after Stage P0.

## The work, in stages

Each stage ends with the regression on every build that changes, and with timings against the stage before, as on
the CMM2. The CMM2's thread tests (`tests/threads/t*.bas`, 32 programs with expected output) are the tests; a few
name CMM2 pins or pages and need a PicoMite version.

**P0. Groundwork, nothing visible.**
1. The interpreter's stacks become pointers with limits: `g_forstack`, `g_dostack`, `gosubstack`, `errorstack`,
   `substack`, `datastore`, `g_StrTmp`, `g_StrTmpLocalIndex`, `g_hashlist`, each with a `*_main` array of today's
   size. By script, as the CMM2's was. The places that copy whole arrays by `sizeof` must use the main arrays by
   hand: `SaveContext` and `RestoreContext` (`Commands.c` 4296-4574, with its `_Static_assert`), the `memset` at
   `MMBasic.c` 8037 and the loop at `Memory.c` 2140. `graphics/TileMap.c` saves `NextData` itself.
2. `StackFloor` and `stackwarn` as variables read by `TestStackOverflow`.
3. `IntReady` gains its tick field; `cmd_pause`'s end time moves out of the function.
4. `LocalBase` and `LocalLimit` around the local region.
Result: every build runs as before, within the usual scatter. Measure each build's RAM margin and the statement
times.

**P1. Threads that take turns.** `Thread.c` and `Thread.h` copied from the CMM2 by a port script (the house rule:
working code is copied, and only what the machine demands is changed). New for the PicoMite: the switch routine for
each chip (ARMv6-M has no `push {r8-r11}` and no FPU; the RP2350 has both), PSP and `CONTROL.SPSEL` handling,
PSPLIM, stacks from SRAM, the landing's fix, the veneer that runs `routinechecks` on the main stack. Commands:
THREAD START, EXIT, YIELD, STOP, LIST; `MM.INFO(THREAD)`. Errors, END and Ctrl-C end every thread.

**P2. Time slices, PAUSE, interrupts in their own thread.** The tick's slice count and wake time, `ThreadSchedule`,
sleeping threads, `IntMay` in the scan, THREAD SLICE, PAUSE and RESUME.

**P3. Locks, hold, waits, arguments, priority.** As built on the CMM2; no machine-specific part is expected.

**P4. Waits that let the others run, and each thread's display state.** The table in section 8, and section 9.

**P4b. The network and Bluetooth.** One owner each; yields in the waits.

**P5. The stack.** Measure, on each chip and for text and compiled: a FUNCTION level; the deepest ordinary command
between checks; each candidate for `StackNeed`. Set the default SIZE, the smallest SIZE and the guard from the
figures. THREAD LIST's stack column, MEMORY's line, TRACE's `[#n:line]`, profiling per thread.

**P6. Manual, release notes, the full regression on every build.** The CMM2's Threads User Manual is the base;
the chapters on memory and on the display differ.

## Traps the CMM2 work found, which apply here too

- **Bind stamps are a sum.** Moving on only the global number at a switch let one level's stamp equal another's,
  and a compiled statement used the wrong thread's variables about one run in four. Every level gets a new event at
  every switch (`BindsStale`). `tests/threads/tbind.bas` shows it. Check also the caches that go by stack position
  (`RBLoopRec`, `Stream.c` 3899-3913) now that each thread has its own stacks.
- **A thread test that passes a few times proves little.** Run the stress tests many times, compiled as well as
  text.
- **`makeargs` has a 256-byte buffer with no length check.** A thread's call line must stay under it: the CMM2
  keeps the argument values in the record and writes the call with `MM.INFO(THREAD ARG n)`.
- **Globals a command leaves in use across a FUNCTION call** are shared between threads: the CMM2's review found
  the print position, EVAL's and EXECUTE's use of `tknbuf`, and INPUT's line buffer. The PicoMite needs its own
  sweep; `DefinedSubFun`'s static argument buffers on the RP2350 (`MMBasic.c` 2934-2950) are one to check.
- **A thread in a wait that takes no interrupts** must let the other threads' interrupts be looked for once as it
  goes in, or a program can stall with every thread waiting.
- **C code that calls back into BASIC** (a CSUB's `RunBasicSub`, MM.USER_RECTANGLE and MM.USER_BITMAP from inside
  drawing, the GUI's `ServiceInterrupts`) must hold the threads until it returns.

## Decisions for Peter

1. **Which builds first.** Recommended: the RP2350 builds, where there is heap and PSPLIM, then the RP2040 builds
   once the RAM cost of Stage P0 is measured on the tightest (VGAUSB, 0.5 KB spare).
2. **Threads on PSP, with interrupts and the housekeeping on the main stack** (section 1a). Recommended. The other
   way is the CMM2's (every stack carries the interrupts and the housekeeping), which costs each thread several KB.
3. **Defaults.** Proposed: SIZE 4 KB, LOCALS 16, NEST 8, sixteen threads, to be confirmed by Stage P5's
   measurements. The CMM2's are 20 KB, 48 and 20; the BASIC is the same, only the defaults differ.
4. **Loadable programs, SQL and USER modules** from the main program only, at first (section 5).
5. **OPTION NOCHECK** keeps the slice test while a thread runs (section 6).
6. **The FRAMEBUFFER WRITE target** is each thread's own (section 9), or shared and documented.
7. **Where the work is done**: a `threads` branch of the PicoMite, by whichever session you choose. The CMM2's
   branch should be merged to v8-dev first, so that the port copies a settled `Thread.c`.

## Not known yet

- The real stack figures on either chip (Stage P5). The sizes above are estimates from the design documents.
- Whether the RP2040 builds have the RAM for Stage P0's pointer loads in their RAM-resident code.
- How FRAMEBUFFER WAIT is signalled from core 1 on each display type.
- The stack the network's commands need in the thread that owns the network (a TLS handshake runs in the caller).
