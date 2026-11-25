# Threads for MMBasic on the Colour Maximite 2

*25 November 2026. A review of what it would take to let a BASIC program start threads that run alongside it, and a
staged plan for doing it. The facts below were read from v8-dev at 545908b. Nothing was built or run for the plan
itself: every size and time marked "to measure" is an estimate, and Stage 0 measures them. Peter agreed all ten
decisions as recommended the same day; the work is on branch `threads`, from v8-dev at 545908b ("Progress", at the
end).*

## Summary

- **Threads are possible**, on all three generations, with no hardware dependence and no cost to a program that does
  not use them.
- **The unit of execution is one BASIC statement**, as assumed. A thread is only ever switched out between two of its
  statements (or inside a statement that is waiting, such as `PAUSE`). So the C code of the firmware never has to be
  re-entrant, and no command needs a lock inside it.
- **The switch is requested the way a software interrupt is**: the 1 ms tick sets a bit in `IntReady`, and the test
  that already follows every statement (`if(IntReady.any) check_interrupt()`) sees it. The statement loop itself does
  not change, text or compiled.
- **One thing differs from the software interrupt mechanism: each thread needs its own C stack.** An interrupt
  borrows the interrupted code's place and gives it back (last in, first out). Threads interleave, and a thread
  stopped inside a `FUNCTION` has half an expression on the C stack. With a stack of its own (about a dozen
  instructions of assembler to swap) a thread can be stopped at any statement, and `PAUSE` in one thread simply lets
  the others run.
- **Everything else a thread owns is copied out of the interpreter's globals when it stops and back when it runs**:
  the statement pointers, the GOSUB, FOR and DO stacks, the list of its local variables, its temporary memory, its
  error state. Only the parts in use are copied; a switch takes about 4.5 microseconds on a G1 (Stage 1's figure).
- **Locals are safe when two threads run the same SUB.** Each thread has its own range of local variable slots, its
  own call depth and its own name bindings. STATIC variables and globals are shared.
- **Interrupt routines stay with the main program.** A thread that finds an interrupt due hands it to the main thread
  at once, so interrupt latency stays at one statement.
- **Per thread: about 36 KB** from the heap (a 20 KB C stack, about 16 KB of save areas). Stage 0 measured the
  stack's needs on a G1: a nested FUNCTION call takes 1.4 to 1.6 KB of C stack, a SUB call none.
- **Six stages**, each leaving a working firmware: groundwork with no visible change, threads that take turns, time
  slices with `PAUSE` and interrupts, locks and flags, waits that let other threads run, tools and the manual.

## What the interpreter does today

These are the facts the design rests on.

| Fact | Where |
|---|---|
| The text loop runs one statement, then a tail: the ON ERROR SKIP count, `ClearTempMemory`, `CheckAbort` if the tick or Ctrl-C asks, `check_interrupt` if `IntReady.any` is not zero. | `ExecuteProgram`, `Src/MMBasic.c` 403-518 |
| The compiled stream runs the same tail (`RBTail`). Compiled statements chain into each other without it only while nothing is due (`RB_HOUSE_DUE`, which tests `IntReady.any`). | `Src/Stream.c` 4148-4164, 5646-5669 |
| A software interrupt does not nest the interpreter. `check_interrupt` saves `nextstmt` in `InterruptReturn`, points `nextstmt` at the routine and returns; `IRETURN` puts it back. | `Src/MM_Misc.c` 4196-4515, 2390 |
| A **SUB** call does not use the C stack: `DefinedSubFun` pushes the return address on `gosubstack` and points `nextstmt` at the body. | `Src/MMBasic.c` 1346-1349 |
| A **FUNCTION** call does: its body runs in a nested `ExecuteProgram` (and a nested `RunStream` when compiled), with the calling statement half evaluated on the C stack below it. | `Src/MMBasic.c` 1387, `Src/Stream.c` 4441 |
| Interrupts are taken inside FUNCTION bodies too, so the firmware's C code already copes with other statements running in the middle of a statement that called a FUNCTION. | the tail is in every `ExecuteProgram` |
| Local variables are a stack: slots 0 to 511 (`MAXLOCALVARS`), their records in D3 SRAM, listed in creation order in `hashlist` with each one's level; `LocalIndex` is the current level. | `Src/MMBasic.c` 119-130, 3443, 4862 |
| A program saved with symbols finds a local through `SymL[name]`, the newest live local of that name. The compiler's records cache what they bound, stamped with a generation number that every change to the bindings moves on. | `Inc/Symbols.h` 172-217, `Src/Stream.c` 4921, 5037-5053 |
| Temporary memory is a list (`StrTmp`) tagged with the level it was taken at; the tail frees the entries at the current level or deeper. | `Src/Memory.c` 936-970 |
| An error ends in `longjmp(mark)`, back to `main()`, or `longjmp(ErrNext)` under ON ERROR SKIP or IGNORE, back to the `ExecuteProgram` that ran the statement. | `Src/MMBasic.c` 4322-4456, `Src/main.c` 868 |
| The main stack is 28 KB at the top of the DTCM, with a 6 KB guard (4 KB until 25 November) above the static variables; the evaluator tests the stack pointer against `stackwarn` at every value. | `STM32H743II_FLASH.ld`, `Src/MMBasic.c` 134-144, `Inc/MMBasic.h` 415 |
| The firmware already runs code on other stacks in two places: loadable programs on the process stack, and PIXEL FILL on free heap. | `Src/Loader.c` 982, `Src/Draw.c` 3563 |

## Why each thread needs its own C stack

The proposal was a scheduler that works as software interrupts do. Most of it does, and the plan keeps that. The one
part that cannot is where a thread's state lives while it is not running.

An interrupt routine runs to its `IRETURN` before the interrupted code goes on, so it can use the same stacks and the
same C stack. Threads take turns in any order. Take two threads, A and B:

```basic
' thread A                         ' thread B
total = total + Reading(3)         Do : Inc n : Loop
```

When A's time is up it may be inside `Reading`, and `Reading`'s body is running in a nested `ExecuteProgram` with
`total = total + ...` half evaluated in the C frames below it. To run B, the interpreter must leave those frames where
they are and run B's statements somewhere else. There are two ways.

1. **Switch only when a thread is not inside a FUNCTION.** No second stack is needed: the loop just changes
   `nextstmt` and the stacks, as an interrupt does. But a thread that spends its time in a FUNCTION holds every other
   thread up, `PAUSE` in a thread stops them all unless it is rewritten to return and be called again, and so would
   every other command that waits. That is not pre-emptive in any useful sense.
2. **Give each thread a C stack.** The switch saves the registers a C function must keep (r4 to r11, lr, d8 to d15)
   on the old stack, changes the stack pointer and restores them from the new one. A thread can then be left at any
   statement boundary, however deep in FUNCTIONs, and a command that waits can let other threads run from inside its
   own loop.

**The plan takes the second.** The cost is a stack for each thread and the assembler stub, which `LoaderCall`
(`Src/Loader.c` 982) already has the shape of. Everything else is as proposed: the switch is asked for by the tick and
taken at the statement tail.

It stays safe for the same reason interrupts inside FUNCTIONs are safe today. To a statement suspended inside a
FUNCTION, a thread switch looks like the FUNCTION taking longer while other statements run. The C code already has to
survive that, because a FUNCTION's body may run any statement. What a FUNCTION call restores on return and a thread
switch would not (the stacks, the level, the temporary memory, the error state) is exactly the list the switch saves
and restores, below.

## The design

### What a thread is

A thread is a SUB running on its own C stack with its own copy of the interpreter's per-call state. The program as it
runs today is the **main thread** (number 0). Threads 1 to 16 are started by the program.

Each thread has one record in the heap:

- its state: ready, sleeping until a time, waiting for a lock, a flag or another thread, paused, or ended;
- its saved stack pointer and its C stack;
- the save area described next.

The main thread's record is made when the first thread is started and freed when the last one ends. While no thread
exists, nothing below is active.

### What is switched

The running thread always uses the interpreter's ordinary globals, at their ordinary addresses, so no existing code
changes how it reaches them and the hot paths in ITCM stay as they are. When a thread stops, its values are copied
from the globals into its record; when it runs again they are copied back. For the stacks, only the entries in use
are copied.

| State | Globals | Copied |
|---|---|---|
| Where it is | `nextstmt`, `cmdline`, `cmdtoken`, `CmdTokenPtr`, `CurrentLinePtr`, `ExecDepth` | whole |
| Calls | `gosubstack`, `errorstack`, `substack`, `gosubindex` | entries below `gosubindex` |
| Loops | `forstack`, `forindex`; `dostack`, `doindex` | entries in use |
| Locals | `LocalIndex`, `hashlist`, `hashlistpointer`, `localtop`, `Localvarcnt`, the thread's slot range | entries in use (4 bytes a local) |
| Name bindings | `SymL` (a pointer: each thread has its own array), `SymTextLocals` | the pointer |
| Temporary memory | `StrTmp`, `StrTmpLocalIndex`, `StrTmpIndex`, `TempMemoryIsChanged` | entries in use (none between top-level statements) |
| Errors | `OptionErrorSkip`, `ErrorSkipDepth`, `MMerrno`, `MMerrline`, the text of `MMErrMsg`, `ErrNext` (160 bytes) | whole |
| A call being set up | `DefinedSubFunMem`, `DefinedSubFunLocalIndex`, `FunReturnArrayCount` | whole |
| DATA | `NextData`, `NextDataLine`, `datastore`, `restorepointer` | entries in use |
| Stack check | `StackFloor`, `stackwarn` | whole |
| Profiling | the line being timed and when it started | whole |

The local variables' **records** are not copied and never move: a parameter passed by reference holds the address of
the caller's value, which may be inside another local's record.

**Not switched, because only the main thread uses them:** `InterruptReturn` and the error state `check_interrupt`
saves beside it, `PauseInterrupted` and PAUSE's saved count, `ContinuePoint`, `InMMEND`.

### What stays shared

Everything else belongs to the program, not to a thread: global variables, STATIC variables, open files, the screen
and its pages, the current font and colours, the text cursor, OPTION settings, timers, the console, sprites, sound,
`FLAG()`.

Because a thread is switched only between statements, **a statement is atomic** unless it calls one of the program's
own FUNCTIONs or waits. `Inc n`, `total = total + 1` and `PRINT a, b, c` cannot be interrupted by another thread.
`total = total + Reading(3)` can, between the statements of `Reading`. Anything that takes two statements (set the
colour, then draw; read a value, then write it back) needs a lock if another thread does the same.

### Local variables

The question was what happens when two threads call the same SUB at the same time. Four things keep them apart.

1. **Slots.** Each thread has its own range of local variable slots. The main thread has slots 0 up; a new thread is
   given a range from the top of the 512 (48 by default, so ten threads at most until `THREAD SIZE` lets a program
   choose). `localtop` starts at the range's base, and
   "Too many local variables" is tested against the range's end. A slot number is still unique across the program,
   so everything keyed by slot (`SymLShadow`, `SymLCanon`, DO's fast path) needs no change.
2. **The list.** `hashlist`, which `find_local_slot` and `ClearVars` walk, holds only the running thread's locals: it
   is part of what is copied.
3. **The level.** `LocalIndex` is per thread. A thread's SUB runs at level 1 whatever level its creator was at.
4. **The bindings.** `SymL` is a pointer, and each thread has its own array, so the symbol `X` finds this thread's
   `X`. `SymBindInit` must set the whole array to -1, not only the entries made so far (today an entry is set when its
   name is first read, which another thread's array would miss).

**The compiler's bind caches** are shared and stamped. Every thread switch takes a new value of `SymBindEvent` for
`SymBindGenG` (`SymBindGlobalsChanged()`), so no stamp written before the switch matches after it and each record
binds again the first time it runs in the new thread. `SymLevelGen` stays shared: the sum the stamp is compared with
then only ever moves forward. The cost is one rebind per record per time slice.

**STATIC** variables are globals under a hidden name (`StaticVarName`), one per SUB, so two threads in the same SUB
share them. The manual must say so.

**FOR loop variables** deserve a warning in the manual: a thread SUB that uses a global `i` shares it with every other
thread. `OPTION EXPLICIT` and `LOCAL` are the cure, as they are for interrupt routines.

### The switch

```c
void ThreadSwitchTo(struct s_thread *to) {      // from the statement tail, or from a wait
    struct s_thread *from = CurrentThread;
    ContextSave(from);                          // the globals in the table above, into its record
    ContextLoad(to);
    CurrentThread = to;
    SymBindGlobalsChanged();                    // the compiler's records bind again
    ThreadSwap(&from->sp, to->sp);              // returns when 'from' next runs
}
```

`ThreadSwap` is a naked function: push r4-r11 and lr, vpush d8-d15, store the stack pointer, load the other, vpop,
pop into pc. A new thread's stack is made to look as if it had called `ThreadSwap` from the start of `ThreadMain`:

```c
static void ThreadMain(void) {                  // a new thread's first run, on its own stack
    ThreadBindArguments();                      // Stage 3
    ExecuteProgram(CurrentThread->body);        // the SUB's body, text or compiled
    ThreadFinish();                             // ended: never returns
}
```

The thread's first `gosubstack` entry returns to an end marker (the `"\0\0\0"` that `cmd_endfun` uses), so `END SUB`
and `EXIT SUB` end `ExecuteProgram` and the thread with it. Its `errorstack` entry is the `THREAD START` line.

All threads run on the main stack pointer (MSP), as the interpreter does now. Hardware interrupt handlers therefore
run on whichever thread's stack is current, so each thread stack keeps a guard as the main stack does (`STACK_GUARD`,
6 KB on both: Stage 0 measured built-in commands going 5 KB below their last check, and the main stack's guard was
raised from 4 KB to match on 25 November), and
`TestStackOverflow` works unchanged against the thread's own `stackwarn`. A word at the bottom of each stack is
checked at every switch, to report an overflow the guard did not catch.

### The scheduler

- **Round robin**, no priorities at first. Each ready thread runs for a time slice (1 ms by default, `THREAD SLICE`).
- **The tick** (`Src/Timers.c`) counts the slice down only while more than one thread exists, and sets a second bit in
  the byte that is now `IntReady.gui`. `check_interrupt` takes and clears both bits with interrupts off, as
  `IntSignal` does for the count. Nothing else in the tick or the statement loop changes.
- **At the tail**, `check_interrupt` does its GUI work as now, then if threads exist and the slice bit was set it
  calls the scheduler, which picks the next ready thread and switches. The call returns when this thread is next
  chosen.
- **`THREAD YIELD`, and a command that must wait**, set the same bit, so the switch is still made at the tail, between
  statements.
- **Skipping.** The scheduler passes over a thread that is paused, sleeping before its time, or waiting for a lock, a
  flag or a thread that is not ready for it. Those tests are a compare each: no switch is made to find out.
- **A sleeper that is due goes first** (as built in Stage 2): the thread whose `PAUSE` ended earliest runs before the
  next in the round, and the tick ends the running thread's slice at that time, so a timed thread is on time among
  busy ones.
- **Nothing ready** (as built in Stage 2): the main program runs, asleep or not. Its `PAUSE` loop is the idle loop,
  as it always was: `CheckAbort()` (so Ctrl-C, USB and the network go on), the interrupt scan, the GUI's work. There
  is no second idle loop to keep in step with it.

With N ready threads and a slice of S ms, a thread waits at most (N - 1) x S plus the longest single statement. A
statement that takes a long time (a `SORT` of a large array, a file copy) holds every thread until it ends:
pre-emption is between statements.

### Interrupts

Interrupt routines (`SETTICK`, `SETPIN ... INTH`, `ON KEY`, COM ports, the GUI, the network and the rest) run in the
**main thread only**. The alternative, running one in whichever thread is current, would give it that thread's small
stack and slot range at random, and would need PAUSE's interrupt state for every thread.

`check_interrupt` is split at its existing label:

- **`IntScan()`**: everything down to `GotAnInterrupt`. It finds a source that is due and returns its address. It
  uses no thread state, so any thread, and the scheduler's idle loop, may call it.
- **`IntDispatch(addr)`**: from `GotAnInterrupt` on, unchanged. Main thread only.

A thread other than main that finds an interrupt due stores its address (one pending slot; no more scans until it is
taken) and switches to the main thread at once. The main thread comes back from its own call to the scheduler, sees
the pending address and dispatches it. Latency is one statement plus a switch, as now. If the main thread was
sleeping or waiting it is woken for it.

**No thread switch is made while an interrupt routine runs** (`InterruptReturn != NULL`). Interrupt routines are
meant to be short, this keeps the single saved state in `check_interrupt` valid, and it makes an interrupt routine
atomic with respect to the threads. A lock that is not free, or any other wait, inside an interrupt routine is an
error.

**The same hold applies in four other places**, each tested by the scheduler (`Held`, `Src/Thread.c`; the last three
built in Stage 2, the first for Stage 3). For a CSUB and a module the hold is the depth of the interpreter that BASIC
called from inside one runs at (`ThreadHoldDepth`), so an error that leaves the call by `longjmp` cannot leave the
hold on: the scheduler drops it the first time it is asked at a shallower depth.

- between `THREAD HOLD` and `THREAD RELEASE`;
- while a USER or SQL module call is under way (`usercall`, `Src/User.c` 187, puts its own `jmp_buf` in `mark` for
  the length of the call, and a module may call back into BASIC);
- while a CSUB runs (it may call a BASIC SUB through `CallExecuteProgram`, `Src/CFunctions.c` 365, and its own data
  is not per thread);
- while a loadable program runs (`LoaderActive`).

`MsgBox()` and `GUI CLICK` take interrupts while they wait, each with one saved state (`ServiceInterrupts`,
`Src/GUI.c` 3173; `GuiClickPhase`). They are for the main thread only at first: in another thread they are an error
("Not allowed in a thread", Stage 2). In the main program the other threads run while either waits.

### PAUSE and other waits

- **`PAUSE` is measured against the microsecond clock, in every thread**, not against `PauseTimer`. That counter is
  one for the whole firmware and is zeroed by each PAUSE, by `DISTANCE()` (`Src/External.c` 1958) and by XMODEM, so a
  pause that lets other threads run could be stretched by them.
- **`PAUSE` in a thread** keeps the time to wake at in a C local on the thread's own stack, marks the thread sleeping
  and calls the scheduler until the time has come.
- **`PAUSE` in the main thread** keeps its way of handling an interrupt (return, run the routine, run the statement
  again, `PauseInterrupted`), with the time to wake at in place of its saved count, and a call in its loop that lets
  the other threads run until the pause ends or an interrupt is due.
- **`THREAD LOCK`, `THREAD WAIT`** (as built in Stage 3) wait as PAUSE does. A thread marks itself waiting, with what
  it waits for and its time limit, and lets the others run from inside the command; the scheduler passes over it
  until the lock is free, the thread has ended, the flag is set or the time has come (a compare each), and its loop
  then looks again. The main program takes interrupts while it waits: the command returns, the routine runs and the
  statement runs again, going on to the same time limit (`InterruptReturn`, as PAUSE). The first draft of this plan had
  every wait run its statement again; one mechanism, PAUSE's, was simpler and already tested.
- **Other commands that wait** (`INPUT`, a serial or network read with a timeout, `PLAY` to the end) hold every
  thread until Stage 4 gives each one a call that lets the others run (as built: `ThreadPoll` and `ThreadFrameWait`,
  "Stage 4" under Progress; the network's waits not yet). Each needs a look at what it keeps in shared
  buffers while it waits: `INPUT` collects its line in `inpbuf`, which `error()` also uses as scratch for a line
  number.

### OPTION COMPILE

The compiled stream needs no change to its code.

- The executor's state is in C locals (`RBRun`'s frame), so it is on the thread's own stack.
- A slice ending sets `IntReady.any`, which ends a chain of compiled statements at the next statement boundary
  (`RB_HOUSE_DUE`) and reaches `RBTail`, which calls `check_interrupt`.
- `RBCallFun` compares the bind generation before and after a FUNCTION's body and binds the statement's variables
  again if it moved (`RBRebind`). A thread switch inside the body moves it, so the statement that called the FUNCTION
  is rebound when the body returns. This matters: while the thread was away, another thread running the same record
  may have written its own addresses into the record's cache.
- **One rule to keep:** a thread may be switched out from inside `RBRun` only through `RBCallFun`, which binds the
  statement again after the body, or through a command that ends its statement (as Stage 4 made them: INPUT, the
  frame waits, run as a CMD record or a spliced command). After such a command the next record's start compares its
  stamp again (`RBCALLOUT`), and `BindsStale` has made every stamp before the switch stale. What must never wait is an
  op in the middle of an expression: the spliced built-in functions (`RBFnSplice`'s list: LEN, MID$, PIXEL, KEYDOWN,
  PEEK and the rest). None does. `EVAL()` and `TEMPR()` are not on the list, so a compiled statement with either
  runs as text. A function that waits must not be added to the list.
- The stream's other caches (`RBCache`, `RBLoopRec`, a patched `RC_GOTO`) are keyed by program position and checked
  before use, so threads share them safely.
- **A switch makes every bind stamp stale, at every level** (`BindsStale`, found in Stage 2). A record's stamp is the
  sum of the last global event's number and the last local event's at its level. One new global event at a switch is
  not enough: with the global number moved on, the sum at one level can equal a sum made earlier at another level, by
  this thread or another, and the record then uses that level's addresses. So a switch gives every level an event of
  its own and the globals a newer one: every sum made after it is greater than any made before.

### Errors, END and Ctrl-C

- **An error in any thread ends the program**, as now. The message names the thread ("Error in thread 2, line 140:").
  `longjmp(mark)` goes from the thread's stack to `main()`'s frame on the main stack, which is still there because the
  main thread is only suspended.
- **`ThreadsEnd()`** is called first thing where `setjmp(mark)` returns (`Src/main.c` 868), in `ClearRuntime` (RUN,
  CHAIN) and at the prompt: each thread's locals and temporary memory are freed through `ClearVars` and
  `ClearTempMemory`, with that thread's state in the globals for the moment, then the blocks go. What is left is a
  program with no threads, no locals and empty stacks, the main stack's limits and the main program's `SymL`.
- **ON ERROR SKIP, IGNORE and ABORT are per thread**, and so are `MM.ERRNO` and `MM.ERRMSG$`. A new thread starts
  with ABORT.
- **`END`** in any thread ends the program: the other threads are stopped where they stand, `MM.END` runs in the
  thread that ended it, then `cleanend`. The main thread running off the end of the program does the same.
- **Ctrl-C** is seen by `CheckAbort` in whichever thread is running, or in the idle loop. Before the prompt, each
  thread's number, state and line are listed.
- **`CONTINUE`** is refused after a program that had threads running stops: the stopped thread's C stack is gone.

### Memory

| For each thread | Size | From |
|---|---|---|
| C stack | 20 KB by default: a 6 KB guard and 14 KB usable, about nine nested FUNCTION calls (Stage 0's figures, in "Progress"); `THREAD SIZE` | on-chip heap while 96 KB of it stays free, else SDRAM |
| Save areas: GOSUB 1.2 KB, FOR about 3.2 KB, DO about 4 KB, temporary memory 5 KB, locals, DATA, scalars | about 16 KB | SDRAM heap |
| `SymL` array | 2 bytes for each name in the program | with the save areas |

The save areas are full size so that a switch can never run out of memory. They are only touched for the entries in
use. On a G2 (about 15 MB of heap) and a G1 (about 4.5 MB, 3.5 MB with OPTION COMPILE ON) 36 KB a thread is small. The
on-chip heap is 192 KB and is wanted for strings and audio buffers, which is why only the first few stacks go there.

The main stack is in DTCM, the fastest RAM. A thread's stack in D2 SRAM or SDRAM will be a little slower, through the
data cache (to measure in Stage 1).

Firmware: `Src/Thread.c`, about 1,200 lines, in bank 2 by the link script (42 KB free there; bank 1 has about 7 KB
and the ITCM 1.2 KB, and nothing here needs either: the switch runs once a millisecond, not once a statement).

### What a thread may not do

| Refused | Why |
|---|---|
| `ERASE`, `REDIM`, `CLEAR` while another thread exists | a thread suspended in the middle of a statement may hold the address of the variable's data (DO's fast path does, so do the compiler's binds) |
| `RUN`, `CHAIN`, `FLASH RUN`, `FLASH CHAIN`, `LIBRARY LOAD`, a loadable program, `SAVE CONTEXT`, `LOAD CONTEXT`, in a thread other than main | they replace the program or its globals; in the main thread they end the threads first |
| `LIST`, `DEBUG BREAK`, `XMODEM`, `ZMODEM`, `MsgBox()`, `GUI CLICK`, in a thread other than main | they take the console or keep one saved state (EDIT, FM, FILES, NEW and AUTOSAVE are refused in any program already) |
| A FUNCTION or CSUB as a thread's entry | a thread is a SUB |
| A wait inside `THREAD HOLD` or an interrupt routine | nothing could end it |
| An argument by reference that is not a global (Stage 3) | the creator's local may be gone while the thread still uses it (`IsGlobalData` is the test) |

`EXECUTE` needs a change before a thread can use it: it builds its line in the global `inpbuf` and runs it from the
global `tknbuf`, saved and restored around the call (`Src/Commands.c` 310-332), which is right for nested calls and
wrong for interleaved ones. A thread's record will hold a token buffer of its own, and `EXECUTE` in a thread runs
its line from there; the main thread keeps `tknbuf`, so nothing changes for a program without threads (Stage 1; see
"Progress" for why not Stage 0). Its one-second `ScrewUpTimer`, which resets the processor if the line has not
finished, counts while other threads run and needs the same look.

## BASIC syntax (a draft)

One new command, `THREAD`, with keywords, and `MM.INFO(THREAD ...)`: command tokens are two bytes and plentiful, and
one function token (0xFE) is left, which this does not take.

| Statement | Does | Stage |
|---|---|---|
| `THREAD START n, sub` | starts thread n (1 to 16) running SUB `sub` | 1 |
| `THREAD START n, sub, arg [, arg ...]` | with up to 16 arguments, each evaluated now and passed by value (a global array by reference) | 3 |
| `THREAD EXIT` | ends the thread that runs it, as its SUB's `END SUB` does | 1 |
| `THREAD YIELD` | gives up the rest of the time slice | 1 |
| `THREAD STOP n` | ends thread n where it stands; its locals, temporary memory and locks are freed | 2 |
| `THREAD PAUSE n`, `THREAD RESUME n` | the scheduler skips thread n until it is resumed | 2 |
| `THREAD SLICE ms` | the time slice, 1 to 100 ms | 2 |
| `THREAD SIZE kb [, locals]` | the stack and local slots of threads started after it | 2 |
| `THREAD LOCK k [, timeout]`, `THREAD UNLOCK k` | lock k (1 to 16): one thread at a time between them. With a time limit in milliseconds, not getting the lock in that time is the error "Lock k timed out" | 3 |
| `THREAD HOLD`, `THREAD RELEASE` | no other thread runs between them (for a few statements) | 3 |
| `THREAD WAIT n [, timeout]` | waits until thread n has ended, or for the time limit: the program then looks for itself (`MM.INFO(THREAD n)`) | 3 |
| `THREAD WAIT FLAG f [, timeout]` | waits until `FLAG(f)` is 1 (the 64 flags the program already has, 0 to 63), or for the time limit | 3 |
| `THREAD LIST` | each thread's number, line and state, and what it waits for (3); its free stack (5) | 3, 5 |
| `MM.INFO(THREAD)` | the number of the thread that runs it, 0 in the main program | 1 |
| `MM.INFO(THREAD n)` | thread n's state: 0 not running, 1 running or ready, 2 in a PAUSE, 3 paused, 4 waiting for a lock, a thread or a flag | 2, 3 |
| `MM.INFO(THREAD LOCK k)` | the thread that has lock k (0 the main program), -1 if it is free | 3 |

```basic
Option Explicit
Dim Integer beats

Sub Heart
  Do
    Inc beats
    Pause 500                     ' the other threads run meanwhile
  Loop
End Sub

Sub Clock
  Do
    Thread Lock 1                 ' the colour and the text are two statements
    Colour RGB(yellow)
    Text 0, 0, Time$
    Thread Unlock 1
    Pause 1000
  Loop
End Sub

Thread Start 1, Heart
Thread Start 2, Clock
Do
  Thread Lock 1
  Colour RGB(white)
  Text 0, 20, Str$(beats)
  Thread Unlock 1
Loop Until Inkey$ <> ""
End                               ' the threads stop with the program
```

## Changes to existing code

The design keeps these small on purpose. `DefinedSubFun` and the statement loops are not touched.

| File | Change |
|---|---|
| `Src/Thread.c`, `Inc/Thread.h` (new) | the records, the stacks, `ContextSave` and `ContextLoad`, `ThreadSwap`, the scheduler, `cmd_thread`, `ThreadsEnd` |
| `Inc/allcommands.h` | the `Thread` command |
| `Src/MM_Misc.c` | `check_interrupt` split into `IntScan` and `IntDispatch` with the scheduler's call between them; `PauseFor` on the microsecond clock, its path for a thread and the call in its loop; `MM.INFO(THREAD)` |
| `Src/CFunctions.c`, `Src/User.c`, `Src/Loader.c` | the hold count raised around a CSUB, a module call and a loadable program |
| `Src/Timers.c`, `Inc/External.h` | the slice count; `IntReady.gui` becomes two bits |
| `Src/MMBasic.c` | the local stack's base and limit as variables (`LocalBase`, `LocalLimit`) at five places; `error()`: the thread's number, its own scratch buffer in place of `inpbuf`; the stack's high-water mark (`StackPaint`, `StackLowest`) |
| `Src/Symbols.c` | `SymBindInit` sets all of `SymL` to -1 |
| `Src/Commands.c` | `cmd_end` stops the other threads; ERASE, REDIM and CLEAR refused; `execute()` in a thread runs from the thread's token buffer; RUN starts the stack's high-water mark |
| `Src/main.c` | `ThreadsEnd()` where `setjmp(mark)` returns and at the prompt; no `ContinuePoint` after threads |
| `Src/Draw.c` | PIXEL FILL moves the stack to free heap, which may be below a thread's `stackwarn`: the check is switched off for its length |
| `Src/Memory.c` | MEMORY lists the threads' memory |
| `Src/Stream.c` | none (the debug assertion apart) |

## Risks

1. **Stack depth.** `CheckAbort` runs USB, the network (`WebPoll`) and Bluetooth from any thread's tail, on that
   thread's stack. Stage 0 measured it on a G1 ("Progress"): the housekeeping and the hardware interrupts' handlers
   stay within 1.2 KB of the statement loop with Wi-Fi joined and idle, and a TLS request within 2.6 KB, so they do
   not need a stack of their own. What goes deepest is a built-in command, about 5 KB, and nested FUNCTION calls.
   Not yet measured: the image decoders in 32-bit modes and SQL (a G2), Bluetooth, sound.
2. **State missed from the table.** A global that a statement sets and a later statement of the same thread relies
   on, and that is not in the table, would be trampled. "The inventory" below is the guard, and Stage 1's tests run
   every control structure in two threads at once with a yield after every statement.
3. **A wait reached from inside the compiled executor** other than through a FUNCTION call (see OPTION COMPILE). Held
   by the assertion and by keeping waits out of spliced handlers.
4. **Flash layout.** Adding code has moved unrelated statements by 5 to 8% before (the I-cache). `Thread.c` goes in
   bank 2, and each stage is timed against the build before it with the same programs.
5. **Programs.** Shared loop variables and unsynchronised screen or file use will be the common mistakes. The manual
   chapter needs the atomic-statement rule and the STATIC and global warnings up front.

## Decisions

| | Question | Recommended |
|---|---|---|
| Q1 | A C stack for each thread, or switching only outside FUNCTIONs? | A stack each |
| Q2 | Where do interrupt routines run? | The main thread only |
| Q3 | The main program ends while threads run? | The program ends and the threads stop; `THREAD WAIT` for a program that wants them to finish |
| Q4 | May a thread start, stop, pause and resume other threads? | Yes: nothing in the design depends on who asks |
| Q5 | Thread numbers chosen by the program (1 to 16), as file numbers are? | Yes |
| Q6 | Priorities? | Not at first: round robin and a 1 ms slice |
| Q7 | Is the DATA pointer per thread? | Yes: each thread reads from the start, or from its own RESTORE |
| Q8 | ERASE, REDIM and CLEAR while threads exist? | Refused |
| Q9 | Arguments to the thread's SUB? | Stage 3, by value; `MM.INFO(THREAD)` tells threads that share a SUB apart until then |
| Q10 | The names in the syntax table | As drafted |

Peter agreed all ten as recommended, 25 November.

## Stages

There are two levels of testing (Peter, 25 November: the full regression is the right baseline but takes too long to
run at every step).

- **The quick check**, `tools/threadcheck.py`, at every step. It runs the regression programs that bear on what
  threads touch (58 of tests/cmm2, listed in `tests/threads/quick.txt`: the statement loop and control structures,
  SUBs, FUNCTIONs, locals and bindings, interrupts and PAUSE, errors and END), each once with OPTION COMPILE OFF, ON
  and SHADOW, then a timing program and the crasher suite. Every output is compared with the build before, nothing
  masked. About five minutes on a G1. `--rb` adds the Route B programs on locals, calls and the bind caches (eight
  minutes more), for a step that touches what the compiler shares with the threads; `--stack` reads the stack's
  depth after each program. The thread tests of tests/threads, each a `.bas` with an `.expected`, join the list as
  each stage writes them.
- **The full regression** (`tools/goldens.py`, every program twice; over an hour with its other suites) for the
  baseline, which is the G1's compile-off run of 545908b on 25 November, and again before the branch goes back to
  v8-dev, on a G1 and a G2. A stage that changes something the quick check cannot see runs the part of it that can.

### Stage 0: groundwork, nothing visible

1. **Measure the stacks.** The free main stack is filled with a pattern at start-up and at every RUN
   (`StackPaint`), and `MM.INFO(STACK LOW)` gives the lowest address written since. Run the regression, the
   network, USB, SQL and sound tests and read it after each program (`tools/goldens.py --post
   "? Hex$(MM.Info(Stack Low))"` keeps it in each program's row). The same for an idle loop, which shows the depth
   of hardware interrupt handlers and of `CheckAbort` alone. This fixes the default thread stack and settles risk 1.
2. `check_interrupt` split into `IntScan` and `IntDispatch`, called in turn: no change in behaviour.
3. The local stack's base and limit as variables, set to 0 and `MAXLOCALVARS`.
4. `IntReady.gui` as a byte of bits, taken and cleared with interrupts off.
5. `SymBindInit` sets all of `SymL` to -1.
6. `error()` formats the line number in a buffer of its own.
7. The network's receive callback that sets `OptionErrorSkip` looked at (see "The inventory").

**Done when** the quick check and one full compile-off run print the same as the build before, and the timings are
within noise of it.

### Stage 1: threads that take turns

`THREAD START n, sub`, `THREAD EXIT`, `THREAD YIELD`, `MM.INFO(THREAD)`. A thread runs until it yields or ends.

1. `Thread.c`: the records, a stack from the heap with its guard and check word, the save areas, `ContextSave` and
   `ContextLoad`, `ThreadSwap`, `ThreadMain`, `ThreadFinish`, freeing an ended thread from another thread's stack.
2. The slot ranges and each thread's `SymL`.
3. `ThreadsEnd` at the prompt's landing, at RUN and at END; the thread's number in error messages; `END` in a
   thread; `EXECUTE` in a thread from the thread's token buffer.
4. The bind generation moved at every switch; the assertion for the compiled executor.

**Tests**
- Two threads passing a counter back and forth with `THREAD YIELD`.
- The same SUB in four threads, with LOCAL numbers, strings and arrays, recursion and parameters by reference: each
  thread's values stay its own.
- FOR, DO, GOSUB, SELECT CASE and multi-line IF in two threads, with a yield after every statement.
- A yield inside a FUNCTION called in the middle of a string expression, in both threads at once (temporary memory).
- An error in a thread (the line and thread reported), ON ERROR SKIP in one thread while another runs, `END` in a
  thread, the main program running off its end, Ctrl-C.
- Sixteen threads started and ended a thousand times: the heap comes back to where it was.
- All of these compiled, and the SUB test with SHADOW.

**Done when** those pass on a G1 and a G2 and a loop timed in the main thread runs as fast as before the stage.

### Stage 2: time slices, PAUSE and interrupts

Threads are now pre-empted. Adds `THREAD STOP`, `PAUSE`, `RESUME`, `SLICE`, `SIZE` and `MM.INFO(THREAD n)`.

1. The slice in the tick; the scheduler's round robin, sleeping threads and the idle loop.
2. `PAUSE` in a thread and in the main thread.
3. Interrupts handed to the main thread; none taken, and no switch made, where the rules above say.
4. `THREAD STOP`: the victim's context loaded for a moment so that `ClearVars(1)` and `ClearTempMemory` free what it
   held, then its blocks freed.
5. Where housekeeping runs, by Stage 0's numbers.

**Tests**
- Three busy threads counting for 10 seconds: the counts within a few percent of each other, and their sum within
  the switch cost of one thread alone.
- `PAUSE` in four threads at once, each timed with `TIMER`.
- A 1 ms `SETTICK` in the main program with two busy threads: no ticks lost, and the latency against the same
  program without threads.
- A pin interrupt through the loop-back wiring while the main thread is in `PAUSE` and the others are busy; while
  every thread is in `PAUSE`.
- `ON KEY`, a COM port interrupt and a GUI interrupt with threads running.
- A thread stopped inside a FUNCTION, inside a loop and inside `PAUSE`; memory afterwards.
- The speed of a program with no threads against the build before (text and compiled).

**Done when** those pass and the no-thread timings are within noise.

### Stage 3: locks, flags, waits and arguments

1. `THREAD LOCK` and `UNLOCK` (the owner recorded; a lock freed when its owner ends), `THREAD HOLD` and `RELEASE`,
   `THREAD WAIT n`, `THREAD WAIT FLAG f`, timeouts.
2. Arguments to `THREAD START`: evaluated in the creator, bound to the SUB's parameters by the new thread before its
   body runs (a routine of its own in `Thread.c`, on the pattern of `RBMakeParams`, so `DefinedSubFun` stays as it
   is).
3. The refusals in "What a thread may not do"; the DATA pointer per thread.
4. At Ctrl-C and at an error, the list of threads: state, line, what each waits for.

**Tests**: a two-statement read and write of a shared total from four threads, wrong without the lock and right with
it; a producer and a consumer through a flag; a lock's timeout; a thread ended while it owns a lock; every refusal;
arguments of each type, a global array, a local array refused.

### Stage 4: waits that let the other threads run

Each is a small change (a call in the wait loop) after a check of what the command keeps in shared buffers and
timers across the wait. In the order programs need them:

1. **The frame waits** (`FRAMEBUFFER WAIT`, `PAGE COPY ...,B`, `PAGE DISPLAY`, `FRAMEBUFFER WINDOW ...,B`): up to
   16 ms each, and a game makes one every frame. They wait on the one flag `pagesetdone`, which each waiter clears,
   so two threads waiting need a frame count in its place.
2. **`INPUT` and `LINE INPUT`**, console and COM port: a buffer of their own in place of `inpbuf`.
3. **The network's waits**: each needs a time of its own in place of `Timer4`.
4. `SYNC`, `ADC START` without an interrupt, `TEMPR()`, `PLAY BBC SOUND` with a full queue, output to a full serial
   buffer, the Bluetooth waits.

**Tests**: a thread animating the screen while the main program sits in `INPUT`; two threads each waiting for the
frame; a network request with a timeout while a thread counts.

### Stage 4b: the network's waits, Bluetooth's and TEMPR()

Added 25 November (Peter: "yes add it as stage 4b, and also tempr"). These hold every thread today for up to 100 s.

1. **The network's waits** (`WEB CONNECT`, `WEB OPEN TCP CLIENT`, `TCP CLIENT REQUEST`, `WEB NTP`, `WEB MQTT`,
   `WEB PING`, `WEB TRANSMIT`). Each is a loop in the PicoMite's files, ported unchanged (`Src/net/picomite`), of the
   form `Timer4 = timeout; while(!done && Timer4) cyw43_arch_poll();`. Each loop gets a call that lets the other
   threads run, and a deadline of its own in place of `Timer4`, which is one for the firmware: a thread's wait would
   set it again under another's. The change goes in as a rule of `tools/port_net.py`, so that a new port of the
   PicoMite's files keeps it, and the files stay the PicoMite's otherwise. What each client keeps (the TCP client's
   one `state`, MQTT's one client) is one for the program: a second thread that starts a request while one is under
   way waits for it (a lock inside the firmware) or is refused; to decide when the code is read.
2. **Bluetooth's waits** (connect, inquiry, query, up to 30 s): the same, in `Src/net/bt/picomite`.
3. **`TEMPR()`**: its conversion time (100 ms to 2 s, `MM_Delay`) and its wait for a `TEMPR START` conversion become
   a sleep that lets the other threads run and takes no interrupt, as SYNC's. It waits inside an expression, which is
   safe as text (a FUNCTION call does the same) and never happens in compiled code (above).

**Tests**: a thread counts while the main program makes a TCP request to an address that does not answer, with a
5 s timeout: the count goes on, the timeout comes at 5 s. A request in each of two threads. `WEB NTP` while a thread
animates. `TEMPR()` on a pin with no sensor (it waits its whole conversion time and gives 1000) while a thread counts.

**Done when** those pass on a G1 and a G2 with the Wi-Fi module, and the Wi-Fi tests (`tests/wifi`) are as before.

### Stage 5: tools, the manual and release

`THREAD LIST`; MEMORY's line for threads; TRACE with the thread's number; OPTION PROFILING's time not charged across
a switch; a manual chapter with the rules and three worked programs; the commands and functions reference; release
notes; the full regression and the thread tests on a G1, a G2 and a G3.

## The inventory

A sweep of every file in `Src/` and of the network's own code in `Src/net`, for commands that wait and for state one
statement leaves for a later one. It was made by a search of the source, and the lines given were not each read
again for this plan, so a stage that relies on one checks it first. The table of switched state above was built
separately, by reading `ExecuteProgram`, `DefinedSubFun`, `check_interrupt`, `ClearVars`, `ClearStack`, `error()`, the
temporary memory code, `Symbols.c` and the stream's executor; the sweep found nothing per call that the table lacks.

### Commands that wait

| Wait | Where | How long | Stage 4? |
|---|---|---|---|
| `PAUSE`, `CPU SLEEP` | `PauseFor`, `Src/MM_Misc.c` 776 | as asked | Stage 2 |
| `INPUT`, `LINE INPUT` from the console or a COM port | `MMgetline`, `Src/SerialFileIO.c` 291 | until a line comes | yes |
| Frame waits: `FRAMEBUFFER WAIT`, `PAGE COPY ...,B`, `PAGE DISPLAY`, `FRAMEBUFFER WINDOW ...,B`, `MAP SET` | `Src/Draw.c` 1017, 2674-2791, 2960; `Src/Warp.c` 586; `Src/Rgb121.c` 591 | up to a frame, 16 ms | yes |
| The network: `WEB CONNECT`, `WEB OPEN TCP CLIENT`, `TCP CLIENT REQUEST`, `WEB NTP`, `WEB MQTT`, `WEB PING`, `WEB TRANSMIT` | `Src/net/picomite`, all timed by `Timer4` | 5 s by default, up to 100 s | yes |
| `MsgBox()` | `Src/GUI.c` 3197 | until a button | main only |
| `GUI CLICK` | `Src/GUI.c` 944-963 | 50 to 500 ms a phase | main only |
| `SYNC` | `Src/External.c` 1627 | the period | yes |
| `ADC START` without an interrupt | `Src/MM_Custom.c` 487 | samples / frequency | yes |
| `TEMPR()` | `Src/Onewire.c` 193 | 100 ms to 2 s | yes |
| Output to a full console or COM buffer; `CLOSE` and `FLUSH` of a COM port | `Src/main.c` 3375, `Src/Serial.c` 821-853 | by baud rate | yes |
| Bluetooth connect, inquiry and query | `Src/net/bt/picomite` | up to 30 s | yes |
| `PLAY BBC SOUND` with a full queue; `PLAY MODFILE`'s read | `Src/Audio.c` 1732, 2230 | short | later |
| `PULSIN()`, `DEVICE SERIALRX`, `DISTANCE()`, `BITSTREAM`, DHT22, 1-Wire | `Src/External.c`, `Src/Onewire.c` | timed in the statement, some with interrupts off | no: they must not be broken into |
| SD card, USB drive, I2C, SPI | `Src/mmc_stm32.c`, `Src/USBDrive.c`, `Src/I2C.c`, `Src/SPI.c` | milliseconds | no |
| Long work in one statement: `SORT`, image loads, a file `COPY`, `PIXEL FILL` | | can be seconds | no: a statement is not broken into |

`INKEY$` and `INPUT$()` do not wait. PLAY returns at once for TTS, WAV, MP3 and FLAC.

### State that stays shared, and what a program must know

Reviewed again in Stage 4 (25 November), with Peter's decisions. **Each thread's own** since then: the print position
(`CurrentX`, `CurrentY`), the colours, the font, PRINT @'s pixel mode, and the write and read pages (`PAGE WRITE`,
`FRAMEBUFFER WRITE`), besides Stage 1's locals, loops, error state and DATA place. A new thread starts with its
starter's; `MODE` gives every thread the new mode's; a framebuffer another thread writes to cannot be closed (in a
4-bit mode that thread goes back to N, as the running one does).

**The program's, documented, not handled:**

| State | Where | For threads |
|---|---|---|
| The keyboard: `INKEY$`, `INPUT`, `LINE INPUT`, `KEYDOWN`, the key decoder's state, INPUT's flashing cursor | `Src/main.c` 3537-3552, 3721; `Src/SPI-LCD.c` 109 | one thread reads the keyboard at a time: two race for the keys |
| The console stream's column (`MMCharPos`): `TAB()` and PRINT's comma | `Src/main.c` 201 | one stream for all |
| The screen itself | | `CLS` and scrolling move what is under every thread's position; `CLS` resets the running thread's position only. Two threads that PRINT without saying where write over each other |
| The current drive and directory: `DRIVE`, `CHDIR` | `Src/FileIO.c` 124, 1470 | FatFs keeps one per drive |
| `DIR$()` with no arguments, which goes on from the last search | `fun_dir`'s statics, `Src/FileIO.c` | one thread lists at a time |
| The last command's result: `MM.I2C`, `MM.ONEWIRE`, sprite collisions | `mmI2Cvalue`, `mmOWvalue`, `Src/sprites.c` | one thread for each bus; a transaction over several statements needs one thread or a lock |
| `OPTION BASE`, `DEFAULT`, `EXPLICIT`, `ANGLE`, `Y_AXIS` and the rest | `Src/MM_Misc.c` | set them before the threads start |
| `TIMER`'s zero, `RANDOMIZE`'s sequence, `SYNC`'s period, the SENSORFUSION filter | `Src/MM_Misc.c`, `Src/External.c`, `Src/MATHS.c` | shared |
| `GUI SETUP`'s page for new controls | `SetupPage`, `Src/GUI.c` 138 | one thread makes the controls |
| The turtle, sound, sprites, BLIT and IMAGE buffers, the GUI's controls | `Src/turtle.c`, `Src/Audio.c`, `Src/sprites.c`, `Src/GUI.c` | shared: one thread should own each |
| TRACE's buffer | `Src/Commands.c` 82-84 | lines from every thread, in the order they ran |
| The multi-line IF table, the label cache, the SUBs' parameter layouts | built by `PrepareProgram` | read-only while a program runs |

### What the sweep found that needs action

- **The countdown timers are one each for the whole firmware**: `PauseTimer` (PAUSE, `DISTANCE()`, XMODEM),
  `Timer4` (every network wait), `InkeyTimer`, `ScrewUpTimer` (EXECUTE). A wait that lets other threads run cannot be
  timed by one of them. PAUSE is dealt with in Stage 2, the rest as Stage 4 reaches them.
- **A network callback sets `OptionErrorSkip = 1`** (`tcp_server_recv`, `Src/net/picomite/MMtcpserver.c` 219): each
  time the web server receives data while a program runs. It runs from `CheckAbort`, so with threads it would set it
  in whichever thread's statement had just ended. Read in Stage 0, it looks like a fault without threads too (from
  the code; not yet run on a board):
  - `CheckAbort` comes after the statement tail's count, so the statement **after** the receive runs with its
    errors skipped.
  - The tail only counts the skip down while `ExecDepth <= ErrorSkipDepth`, and `ErrorSkipDepth` is set by
    `ON ERROR SKIP` alone (it starts at 0, and RUN does not reset it). In a program that has not used
    `ON ERROR SKIP` the count never drops, and every later error in the program is skipped without a message.
  - If the callback's own `GetMemory` fails, `error()` jumps to `ErrNext`, which is only armed while a skip is in
    force: it may hold an earlier statement's landing, or none.

  The file is the PicoMite's, brought over by `tools/port_net.py`. What the line is for, and whether the PicoMite
  has the same fault, is Peter's to say; it is left as it is until then.
- **`mark` is replaced for the length of a USER module call** (`usercall`) and inside FM (`fm_try`). FM does not run
  in a program; the hold during a module call keeps another thread from raising an error while `mark` is not the
  prompt's.
- **`ErrNext` is replaced by C helpers** that catch an error and put it back (`FileTry`, `ld_try`, `UiTry`,
  `RBExecSkip`). It is in the switched state, so each thread's is its own.

## Not covered

- **The PicoMite.** This firmware's compiler and symbol code are ports of the PicoMite's, and the interpreter has the
  same shape, so the design should carry over with a switch stub for its processors. Its source was not read for this
  plan.
- **Loadable programs, the C compiler's runtime and MicroPython** have their own runtimes and are outside this plan:
  threads are for MMBasic programs.

## Progress

### Stage 0 (25 November, on branch `threads`)

In the source and built (Release); run on the G1 the same day (below).

| Item | What was done |
|---|---|
| 1 | `StackPaint` and `StackLowest` (`Src/MMBasic.c`, in bank 2), painted in `InitBasic` and in `RunStart`; `MM.INFO(STACK LOW)`; `tools/goldens.py --post CMD` keeps a command's reply in each program's row |
| 2 | `check_interrupt` is `IntScan` then `IntDispatch` (`Src/MM_Misc.c`): the 46 `goto GotAnInterrupt` are `return intaddr`, the consuming of a signal went into `IntScan`, nothing else moved. Neither is inlined |
| 3 | `LocalBase` (0) and `LocalLimit` (`MAXLOCALVARS`) in `findvar_local`, `findvar_body` (two places) and `ClearVars` (two). The tests of a slot number against `MAXLOCALVARS` that ask "local or global?" stay |
| 4 | `IntReady.gui` is `IntReady.tick`, with `INT_TICK_GUI` and `INT_TICK_SLICE`; the tick sets its bit, `check_interrupt` takes the byte with interrupts off (`IntTickTake`) |
| 5 | `SymBindInit` sets every `SymL` entry to -1 |
| 6 | `error()` formats the line number in its own buffer |
| 7 | read: see "What the sweep found that needs action" |

Against the build before (545908b): bank 1 88 bytes more, bank 2 112, ITCM 24, initialised data 8.

**`EXECUTE` moved to Stage 1.** Running its line from a buffer in `execute()`'s own frame would change what happens
after a break today: with Ctrl-C inside a SUB that `EXECUTE` called, `CONTINUE` goes on in the SUB and its `END SUB`
returns into the line's buffer. In `tknbuf` that finds the end of a line; in a dead stack frame it would find
whatever is there. So the main thread keeps `tknbuf`, and only a thread gets a buffer of its own, in its record.

### Stage 0 on the G1 (25 November)

The G1 (COM51, 400 MHz, Wi-Fi joined) was flashed with 545908b, tested, then with the Stage 0 build, tested, and left
on it with its options as they were. The image it had before is kept.

| Check | Result |
|---|---|
| Full regression, OPTION COMPILE OFF: tests/cmm2 (114 programs) and tests/picomite (4), every program twice | every first output the same as 545908b's, byte for byte |
| Quick check: 58 programs with OFF, ON and SHADOW | every output the same as 545908b's |
| Crasher suite | all passed, both builds |
| Route B programs on locals, calls and the bind caches (78, each OFF, ON and SHADOW) | the same result for each on both builds: 52 same three ways, 26 with the same error three ways, none differing |
| Timings, compile off (`tests/threads/tstmt.bas`) | within 2% of 545908b, but `Line` of one pixel +3.1% and `Colour 7` 0.34 to 0.28 us; a SUB call +0.4%, a FUNCTION call +0.9% |
| Timings, compile on | within 2.7% |

The two statements that moved do not run any changed code and moved in opposite directions: the flash layout, as
seen before. Not run: a full pass with ON or SHADOW (the quick check covers its 58 programs in both), the corpus,
the Testfiles, SP Tools, and anything on a G2.

**The stack**, as bytes below its top (24,568 are usable above the 4 KB guard):

| What | Bytes |
|---|---|
| The stack pointer inside a function called from a top-level statement | 2,400 |
| The deepest a RUN goes before the program's first line | 4,032 |
| Ten seconds of a loop and five of PAUSE, Wi-Fi joined: the statements, the housekeeping and the interrupts' handlers | 3,056 |
| A plain HTTP request | 3,464 |
| A request over TLS | 4,476 |
| The deepest programs of tests/cmm2 that do not recurse: `sprites` and `json` 7,408, `savepng` 7,268, `raydepth` 6,732, `mode1` 6,632 | up to 7,408 |
| One nested FUNCTION call returning a number, as text | 1,440 a level |
| The same, compiled | 664 a level |
| One nested FUNCTION call returning a string, text or compiled | 1,560 to 1,592 a level |
| One nested SUB call | 0 |
| `locals.bas`, which recurses a FUNCTION 20 deep | 21,564 |

What this says for a thread's stack:

- **The housekeeping is shallow**, so it can run on any thread's stack: no need to borrow the main stack for it.
- **A built-in goes about 5 KB below the statement that runs it** (7,408 less the 2,400 its arguments were checked
  at). That is more than the 4 KB guard, on the main stack too: a program at the very end of its stack could
  overrun on those commands today. A thread's stack takes a 6 KB guard.
- **FUNCTION nesting is what a program spends stack on**: 1.4 to 1.6 KB a level. The default of 20 KB gives a thread
  14 KB above its guard, about nine levels; the main program has about seventeen. `THREAD SIZE` for more.

**Tools made on the way** (in the repository): `tools/threadcheck.py` and `tests/threads/quick.txt` (the quick
check), `tests/threads/tstmt.bas` (timings), `stack0.bas`, `stack1.bas` and `stack2.bas` (the measurements above).
`tools/goldens.py` sent every file again on every run: its listing of the card could be read before the card had
answered. It now waits for the listing's own end (a minute saved on each run, for every user of it).

**Still open in Stage 0:** the stack depth of the image decoders in 32-bit modes, of SQL, sound and Bluetooth, and
the same checks on a G2.

### Stage 1 (25 November): threads that take turns

`THREAD START n, sub`, `THREAD YIELD`, `THREAD EXIT` and `MM.INFO(THREAD)`, as planned. A thread runs until it yields
or ends, and the next in number order runs.

| Part | How it was done |
|---|---|
| The record | One block from the heap for each thread (17 KB, its names' bindings after it), and one for the main program made with the first thread and freed when the last has ended: from then on nothing here is active |
| The stack | 20 KB with a 6 KB guard and a check word at its bottom, from the on-chip heap while 96 KB of that stays free, else from SDRAM |
| The switch | `ThreadSwap`, six instructions; `ContextSave` and `ContextLoad` copy the table of "What is switched", the stacks' entries in use only |
| A thread's first run | Its SUB is called as a statement of its own, the SUB's name in the thread's record, so that the call is made by `DefinedSubFun` as any other and `END SUB` returns to the end of that statement |
| The statement tail | `check_interrupt` calls `ThreadSchedule` when `INT_TICK_SLICE` was set, which only `THREAD YIELD` does so far. No switch in an interrupt routine, nor once the program is ending |
| Interrupts | Taken at the main program's own statement tail only: with a thread running they wait for the main program's turn (Stage 2 hands them over at once) |
| The end | `ThreadsEnd` at the prompt's landing, in `ClearRuntime` and at the prompt; `END` in a thread runs `MM.END` there; an error names its thread; no `CONTINUE` after threads |
| EXECUTE | A thread's line runs from a buffer in its record; `tknbuf`, which the main program may be part way through, is kept while the tokeniser uses it. EXECUTE's one-second timer runs only while its thread does |
| Refused | RUN, CHAIN, FLASH RUN, LIBRARY LOAD and EXECUTE "RUN" in a thread; ERASE, REDIM, CLEAR, SAVE CONTEXT and LOAD CONTEXT while a thread exists; THREAD START at the prompt |
| PIXEL FILL | The stack check is off for its length in a thread (its borrowed stack is below a thread's) |

Not done in Stage 1, against the plan: the assertion that the compiled executor is only left through a FUNCTION call.
Nothing can leave it any other way yet (a yield is made at the statement tail); it belongs with Stage 2's waits.

Firmware: 3.5 KB in bank 2, 280 bytes in bank 1, none in the ITCM.

**On the G1**, against the Stage 0 baseline:

| Check | Result |
|---|---|
| The ten thread tests (`tests/threads/t*.bas`), with OPTION COMPILE OFF, ON and SHADOW | all pass, the same output in the three modes |
| The 58 regression programs of the quick check, three modes | every output the same as Stage 0's |
| Crasher suite; the Route B programs on locals, calls and bind caches | all passed; the same result for every program |
| Ctrl-C with three threads running, three times (twice inside a thread) | the prompt back, CONTINUE refused, the heap as before the run, the next program runs |
| Timings | within 2% of Stage 0's on the final build (`Colour 7` 0.28 to 0.27 us) |
| `THREAD YIELD` with no thread | 3.5 us |
| A yield to another thread and back (two switches, three statements) | 17.7 us: about 4.5 us a switch |

The thread tests: turn-taking in a fixed order (`tyield`); the same SUBs and FUNCTIONs in four threads with their
own locals, strings, arrays, parameters by reference and recursion (`tlocals`); FOR, DO, SELECT CASE, IF and GOSUB in
three at once (`tloops`); strings built across yields in FUNCTIONs (`ttemp`); ON ERROR SKIP, MM.ERRNO and MM.ERRMSG$
per thread (`terror`); READ's place and EXECUTE's line per thread (`tdata`); 500 threads started and ended with the
heap unchanged, and what THREAD refuses (`tmany`); END in a thread (`tend`); the main program ending first
(`tmainend`); an error in a thread (`tfail`).

**Found on the way:** a GOSUB's routine inside a SUB does not see the SUB's locals (GOSUB makes a level of its own).
Not a thread matter, and as it was before; the loop test uses globals there.

**Merged from v8-dev the same day** (not thread work; the final Stage 1 build and its checks include them): the web
server's receive no longer leaves the program's errors skipped (the fault of "What the sweep found", fixed as the
PicoMite fixed it, with `GetMemoryNull`), and BLIT RESIZE steps exactly (`tests/picomite/resize.bas`).

**The quick check's tools:** `tools/goldens.py`'s card listing gives `NEW` first. After a program with OPTION
EXPLICIT the option is still on at the prompt, the listing's own variable was refused, and every file was sent
again. `tests/threads/quick_G1` is now this build's run, thread tests and all.

### Stage 2 (25 November): time slices, PAUSE and interrupts

Threads are now pre-empted: `THREAD STOP`, `PAUSE`, `RESUME`, `SLICE`, `SIZE` and `MM.INFO(THREAD n)`, as planned.

| Part | How it was done |
|---|---|
| The slice | The 1 ms tick counts the running thread's slice down and sets `INT_TICK_SLICE`; nothing is counted while a program has no thread. The switch is made at the statement tail, as a yield is |
| A sleeping thread | `PAUSE` gives the thread a time to wake at and lets the others run (`ThreadSleepUntil`). The tick ends the running thread's slice when the first sleeper is due, and a sleeper that is due runs before the next in the round |
| Nothing to run | The main program runs, asleep or not: its PAUSE loop is the idle loop (Ctrl-C, the interrupts, the GUI's work), as it always was |
| PAUSE | Measured against the microsecond clock in every thread, **and in a program with no threads**: `PauseTimer` and `IntPauseTimer` are no longer what a pause counts. An interrupted pause in the main program goes on to the same end time |
| Interrupts | A thread that finds one due (`IntScan`) stores its routine's address and switches to the main program, which takes it at once (`ThreadIntPending`). No scan is made twice for one interrupt |
| The hold | No switch in an interrupt routine, while a loadable program runs, once the program is ending, or in BASIC called from inside a CSUB or a USER or SQL module (`ThreadHoldDepth`) |
| `THREAD STOP n` | The thread's locals and temporary memory are freed with its state in the globals for the moment, as `ThreadsEnd` does, then its blocks go. A thread that stops itself ends as `THREAD EXIT` |
| `THREAD PAUSE n`, `RESUME n` | A flag the scheduler tests. A thread may pause itself: it stops at the end of that statement |
| `THREAD SIZE kb [, locals]` | 12 to 512 KB of stack and 8 to 256 local slots, for threads started after it; back to 20 KB and 48 at the next RUN |
| `MM.INFO(THREAD n)` | 0 not running, 1 running or ready, 2 in a PAUSE, 3 paused by `THREAD PAUSE` |
| Refused in a thread | `GUI CLICK`'s wait and `MsgBox()` ("Not allowed in a thread"): each takes interrupts while it waits, with one saved state. In the main program the threads run while either waits |

Not done in Stage 2: where the housekeeping runs was left as it is (Stage 0 found it shallow enough for any stack),
and the assertion on the compiled executor still waits for Stage 3's waits, the first that could trip it.

**On the G1**, against the Stage 1 build:

| Check | Result |
|---|---|
| The seventeen thread tests, with OPTION COMPILE OFF, ON and SHADOW | all pass, the same output in the three modes |
| The regression programs of the quick check, three modes | every output the same as Stage 1's |
| Crasher suite; the Route B programs on locals, calls and bind caches | all passed; the same result for every program |
| Ctrl-C with three threads running, three times | as Stage 1 |
| Timings with no thread, text and compiled | within 2.5% of Stage 1's |
| A yield to another thread and back | 21.0 us, was 17.7: at each switch the scheduler now looks for sleepers (1 us) and every level's bind stamp is made stale (0.7 us) |

The new tests:

| Test | What it shows |
|---|---|
| `tslice` | Four loops that never yield (three threads and the main program) for 400 ms: each has the same share within a tenth, and together they do what one does alone, within a tenth |
| `tpause` | PAUSE of 60 to 90 ms in three threads and the main program at once, four times each: every pause within 4 ms of its time, with and without a busy thread beside them, and the pauses run side by side |
| `ttick` | A 5 ms `SETTICK` for 300 ms with two busy threads, with the main program in a PAUSE, and with every thread in a PAUSE: 60 ticks, each in the main program, none more than 3 ms late |
| `tkey` | `ON KEY` with keys typed while two threads are busy, while the main program is in a PAUSE, and with a thread inside a FUNCTION: every key, in order, its routine in the main program |
| `tpin` | A pin interrupt (`SETPIN INTB`) twenty times in each of the same three states. No wire: the pin's level is moved through its pull-up |
| `tbind` | Four threads and the main program recursing the same FUNCTION to different depths for two seconds, switched by the clock and by yields, each checking every result and local: the compiler's bind stamps (below) |
| `tstop` | A thread stopped in a loop, inside a FUNCTION and in a PAUSE: the heap and the variable count as before. PAUSE and RESUME, `MM.INFO(THREAD n)`, `THREAD SIZE` (a FUNCTION twelve deep in a thread), `THREAD SLICE` |

`tyield` and `tmainend` now start with `THREAD SLICE 100`: they test the order threads take turns in, and with a
1 ms slice a PRINT can end one.

Not tested: a COM port's interrupt (the G1 has no loop-back wire) and a GUI interrupt with threads running. Both are
found by the same scan as the tick, the key and the pin.

**For every program, threads or not:** PAUSE now counts microseconds from the moment it starts. Before, it waited for
the next tick and then counted ticks, so the time was the same; what has changed is that DISTANCE() and XMODEM,
which zero the counter a pause used, can no longer stretch one that an interrupt routine calls them during.

**Found on the way:**

- **A compiled statement could use another call depth's variables after a thread switch.** `tlocals` failed two runs
  in eight with OPTION COMPILE ON: a recursing FUNCTION returned (n - 2)! for n!. The record's bind stamp, a sum of two
  event numbers, came out the same at one depth as a stamp made at another depth before the switch ("OPTION COMPILE",
  above). It could happen in Stage 1 too; there the switches came at the same statements every run, and that order
  happened to miss it. `BindsStale` fixes it, and `tbind` failed on nearly every run before the fix and passes since
  (six runs of six, and `tlocals` twelve of twelve). The same sum can in theory collide without threads, when a
  global is made or erased between two depths of a recursion: not seen, and not changed.
- `MM.INFO(HEAP)` read inside a SUB call's arguments is 3,072 bytes lower than at the statement before: the call's
  own argument block is in use. `tstop` reads the heap into variables first.
- The stack each built-in command takes below its last stack check was measured for the guard (Peter's question,
  25 November). Only three go beyond 4 KB: `LOAD PNG` (4.5 KB, 5.6 KB for an image 704 wide), `SPRITE LOADPNG`
  (4.6 KB) and `JSON$()` (96 + 40 bytes for each level of nesting: 5.2 KB at the 128 levels the build allowed). On
  v8-dev since (0d1c8be, f31df74 and 9527b60, merged here): the main stack's guard is 6 KB, as a thread's, and
  `JSON$()` reads 100 levels at most (4.1 KB; `tests/cmm2/jsondeep.bas`). With the larger guard a FUNCTION with locals
  20 deep, as text, comes within 848 bytes of the stack's warning line: `tests/cmm2/locals.bas` now goes 18 deep.
  Not measured: RAY, DRAW3D, FRAMEBUFFER WARP, the sound decoders, SQL, Bluetooth and
  image loads in 32-bit modes.

### Stage 3 (25 November): locks, flags, waits and arguments

`THREAD LOCK` and `UNLOCK`, `THREAD HOLD` and `RELEASE`, `THREAD WAIT n`, `THREAD WAIT FLAG f`, time limits, arguments
for a thread's SUB, `THREAD LIST`, `MM.INFO(THREAD LOCK k)`, and the list of threads at an error or Ctrl-C.

| Part | How it was done |
|---|---|
| Locks | Sixteen, each with its owner (a thread's number, or the main program). A thread that ends or is stopped gives up its locks; the program's end frees them all. Taking a lock twice, and unlocking one that is another's, are errors. A lock is not handed to the next in a queue: when it is free, the first waiter whose turn comes takes it |
| A lock's time limit | An error when it passes, "Lock k timed out", so that a program that does not look cannot run on without the lock; ON ERROR SKIP for one that wants to go on |
| `THREAD HOLD`, `RELEASE` | A count the scheduler tests: no other thread runs until it is back to 0. They nest. Interrupt routines wait too if a thread other than the main program holds. A thread that ends while it holds releases |
| `THREAD WAIT` | For a thread to end, or for `FLAG(f)` to be 1: an interrupt routine or any thread may set the flag. At a time limit the statement just ends |
| The waits | One mechanism for a lock, a thread and a flag, PAUSE's ("PAUSE and other waits"). Where no other thread can run (an interrupt routine, a hold, BASIC called from a CSUB) a wait that cannot be met at once is an error |
| Arguments | Up to 16. Each is evaluated by `THREAD START`, in the thread that runs it, and kept in the new thread's record; the new thread's call of its SUB is written with `MM.INFO(THREAD ARG n)` for each, which gives the value back exactly, of any type. So the call is made by `DefinedSubFun` as any other, by value, with the usual conversions and errors. A whole array is written as its name and must be a global |
| The list | `THREAD LIST`, and after the message when a program with threads stops at an error or Ctrl-C (not at END): each thread's line and what it is doing or waiting for, with a lock's owner |
| Refused in a thread | `LIST`, `DEBUG BREAK`, `XMODEM` and `ZMODEM`, with Stage 1's and 2's |

**Arguments were first written into the call as constants** (an integer as `&H` and sixteen digits, a float with
seventeen significant digits, a string in quotes). Every value came back exact, but a call is read with a program
line's room: `makeargs` copies the arguments into a 256-byte buffer with no check, because no program line is longer.
A call with a 255-character string overran it and hung the board. The values now stay out of the call.

**On the G1**, against the Stage 2 build:

| Check | Result |
|---|---|
| `tlock` (17 checks), `twait` (15), `targs` (12), with OPTION COMPILE OFF, ON and SHADOW | all pass, six more runs of each compiled and four as text |
| Ctrl-C with three threads running, three times | as before, and the list of threads is printed each time |
| The twenty thread tests and the quick check's regression programs, three modes | every output as the Stage 2 build's, but `tfail`'s, which has the list |
| Crasher suite; the Route B programs | all passed; the same result for every program |
| Timings | within 3% of Stage 2's; `THREAD YIELD` with no thread 3.5 us (YIELD is the first keyword THREAD tests) |

The new tests:

| Test | What it shows |
|---|---|
| `tlock` | Four threads add to a total with a read, a yield and a write: counts are lost with nothing, all 1,200 are there with a lock, and with a hold. A lock's time limit (the error, at 50 ms); a wait with no limit while a SETTICK routine runs; a lock whose owner ends, and whose owner is stopped; the refusals |
| `twait` | `THREAD WAIT n` with and without a limit; a producer and a consumer handing twenty items over through two flags; the main program waiting for a flag a SETTICK routine sets; a thread waiting for a flag |
| `targs` | Integers (the largest and smallest), 246 floats bit for bit, strings with quotes, zero bytes, every byte from 1 to 255 and none; conversions; by value; evaluated when THREAD START runs; a global array filled by a thread; a SUB's own array refused |

`tend` now starts with `THREAD SLICE 100`, as the other tests of order do: it differed once in a run of many.
`tfail`'s expected output has the list of threads after its error.

### Stage 4 (25 November): waits that let the other threads run

Peter: "The big one is stage 4 when a thread can wait for user input without blocking everything else."

| Wait | How it was done |
|---|---|
| `INPUT`, `LINE INPUT`, console or COM port | The key or byte loop (`MMgetchar`, `MMgetline`) calls `ThreadPoll` when nothing has come: the other threads run, and the loop looks again when this thread's turn comes round. Each command collects its line in a buffer of its own, not `inpbuf`, which the other threads use too. LINE INPUT takes the variable's size before the wait |
| The frame waits: `FRAMEBUFFER WAIT`, `PAGE COPY ...,B`, `PAGE DISPLAY`, `FRAMEBUFFER WINDOW ...,B`, `CLUT SET`, `MAP SET` | `ThreadFrameWait`, in place of each `pagesetdone = 0; while(!pagesetdone)` loop. With threads it waits on a count of frames the LTDC's interrupt keeps (`FrameCount`): each waiter cleared the one flag, so of two waiters one could miss its frame. The interrupt ends the running thread's slice when a thread waits, and the waiter runs first. With no thread it is the loop it was |
| `SYNC` | The other threads run until 2 ms before the period ends; the last 2 ms are timed as before |
| `ADC START` with no interrupt | `ThreadPoll` while the samples come in |

**No interrupt is taken in these waits, as none ever was.** While the main program is in one, the threads do not
hand it interrupts: the signals are kept (`ThreadBank`) and the statement tail takes them when the wait is over.
`tinput` checks it: a SETTICK interrupt that came during an INPUT runs once the line is in.

**Each thread's own print position, colours and font.** Peter tried a demo (four balls and a clock as threads, the
main program in INPUT) on the G1 with a USB keyboard: the typed characters were echoed a line down. The clock's TEXT
had moved the print position, which was the program's, while the line was being typed. Peter: "Each thread will need
to maintain some extra context". `CurrentX`, `CurrentY`, `gui_fcolour`, `gui_bcolour` and `gui_font` are now saved
and loaded at every switch (`ScreenSave`, `ScreenLoad`), and a new thread starts with its starter's. The demo then
worked ("That works perfectly"). `tests/threads/demo_input.bas` is the demo.

Not done in Stage 4: the network's waits (each loop is in the PicoMite's files, ported unchanged, and all time
themselves with the one `Timer4`), Bluetooth's, `TEMPR()` (a wait inside an expression), output to a full console or
COM buffer (a PRINT would be split), `PLAY BBC SOUND` with a full queue.

**On the G1:**

| Check | Result |
|---|---|
| `tinput` (9 checks), `tframe` (6), `tscreen` (9), OPTION COMPILE OFF, ON and SHADOW | all pass |
| The quick check's regression programs, the other thread tests, three modes | every output as the Stage 3 build's |
| Crasher suite; the Route B programs | all passed; the same result for every program |
| Timings | within 3% of Stage 3's |

The new tests:

| Test | What it shows |
|---|---|
| `tinput` | The main program in INPUT while two threads count and a SETTICK runs: the line, the threads' counts, the interrupt after the line. INPUT in a thread (a string and a number). LINE INPUT in a thread while another assigns to strings of five. A line typed while a thread draws text elsewhere: the print position after it is where the line was |
| `tframe` | Two threads that each wait for every frame both get all thirty while a third is busy; thirty PAGE COPY ...,B take thirty frames with a busy thread running |
| `tscreen` | A thread's COLOUR, FONT, PRINT @ and TEXT leave the main program's untouched; each keeps its own while the other changes; a new thread starts with its starter's |

**The context review** (Peter: "there may be other commands that rely on context, do a scan"; then "1-5 yes,
document the rest"). Handled:

| | What | How |
|---|---|---|
| 1 | A thread's font after `LOAD FONT` put another size in its slot | the size worked out from the font table at each switch (`FontMetrics`, SetFont's own); an empty slot gives font 1 |
| 2 | `MODE` while threads exist | every thread gets the new mode's position, colours, font and pages (`ThreadsScreenReset`, at both of setmode's ends) |
| 3 | PRINT @(x, y, mode)'s pixel mode, which lasts to the PRINT's end, across a FUNCTION in its list | each thread's |
| 4 | `EVAL()` borrowed the token buffer, where the main program's EXECUTE line is, while its expression ran | it gives the buffer back before the expression runs |
| 5 | The write and read pages (`PAGE WRITE`, `FRAMEBUFFER WRITE`) | each thread's, with `HRes`, `VRes` and the pages' addresses for CSUBs. `FRAMEBUFFER CLOSE` refuses a framebuffer another thread writes to, as it refuses the running thread's; in a 4-bit mode a thread on a closed framebuffer goes back to N, as the running one does; a page gone otherwise gives the display |

And `CLUT RESET` (`MAP RESET`), whose frame wait the first pass left, lets the other threads run. The rest is
documented under "State that stays shared". Tests: `tpage` (a thread drawing into the framebuffer while the main
program draws on the display, the close refused, MODE), `teval` (EVAL in a thread while the main program is in
EXECUTE, both in FUNCTIONs that yield).
