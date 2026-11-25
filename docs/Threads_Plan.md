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
  error state. Only the parts in use are copied, so a switch costs about a microsecond (to measure).
- **Locals are safe when two threads run the same SUB.** Each thread has its own range of local variable slots, its
  own call depth and its own name bindings. STATIC variables and globals are shared.
- **Interrupt routines stay with the main program.** A thread that finds an interrupt due hands it to the main thread
  at once, so interrupt latency stays at one statement.
- **Per thread: about 32 KB** from the heap (a 16 KB C stack, about 16 KB of save areas), sizes to confirm in Stage 0.
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
| The main stack is 28 KB at the top of the DTCM, with a 4 KB guard above the static variables; the evaluator tests the stack pointer against `stackwarn` at every value. | `STM32H743II_FLASH.ld`, `Src/MMBasic.c` 134-144, `Inc/MMBasic.h` 415 |
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
   given a range from the top of the 512 (64 by default, to settle). `localtop` starts at the range's base, and
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
run on whichever thread's stack is current, so each thread stack keeps the same 4 KB guard the main stack has
(`STACK_GUARD`), and `TestStackOverflow` works unchanged against the thread's own `stackwarn`. A word at the bottom of
each stack is checked at every switch, to report an overflow the guard did not catch.

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
- **Nothing ready.** The scheduler loops on the current stack: `CheckAbort()` (so Ctrl-C, USB and the network go on),
  the interrupt scan, the sleepers' times.

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

**The same hold applies in four other places**, through one count that the scheduler tests:

- between `THREAD HOLD` and `THREAD RELEASE`;
- while a USER or SQL module call is under way (`usercall`, `Src/User.c` 187, puts its own `jmp_buf` in `mark` for
  the length of the call, and a module may call back into BASIC);
- while a CSUB runs (it may call a BASIC SUB through `CallExecuteProgram`, `Src/CFunctions.c` 365, and its own data
  is not per thread);
- while a loadable program runs (`LoaderActive`).

`MsgBox()` and `GUI CLICK` take interrupts while they wait, each with one saved state (`ServiceInterrupts`,
`Src/GUI.c` 3173; `GuiClickPhase`). They are for the main thread only at first.

### PAUSE and other waits

- **`PAUSE` is measured against the microsecond clock, in every thread**, not against `PauseTimer`. That counter is
  one for the whole firmware and is zeroed by each PAUSE, by `DISTANCE()` (`Src/External.c` 1958) and by XMODEM, so a
  pause that lets other threads run could be stretched by them.
- **`PAUSE` in a thread** keeps the time to wake at in a C local on the thread's own stack, marks the thread sleeping
  and calls the scheduler until the time has come.
- **`PAUSE` in the main thread** keeps its way of handling an interrupt (return, run the routine, run the statement
  again, `PauseInterrupted`), with the time to wake at in place of its saved count, and a call in its loop that lets
  the other threads run until the pause ends or an interrupt is due.
- **`THREAD LOCK`, `THREAD WAIT`** do not wait inside the command. If they cannot go on they mark the thread waiting,
  point `nextstmt` back at their own token (as PAUSE does through `InterruptReturn`) and let the tail switch. When the
  scheduler finds the thing they wait for, the statement runs again. Every wait is then at a statement boundary, and
  the main thread can take an interrupt between tries.
- **Other commands that wait** (`INPUT`, a serial or network read with a timeout, `PLAY` to the end) hold every
  thread until Stage 4 gives each one a call that lets the others run. Each needs a look at what it keeps in shared
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
- **One rule to keep:** a thread may only be switched out from inside `RBRun` through `RBCallFun`. The other ops that
  call out of the executor (LOCAL, STATIC, string ops, the spliced handlers) must never reach a wait. None does
  today; a debug-build assertion in the scheduler will hold the rule.
- The stream's other caches (`RBCache`, `RBLoopRec`, a patched `RC_GOTO`) are keyed by program position and checked
  before use, so threads share them safely.

### Errors, END and Ctrl-C

- **An error in any thread ends the program**, as now. The message names the thread ("Error in thread 2, line 140:").
  `longjmp(mark)` goes from the thread's stack to `main()`'s frame on the main stack, which is still there because the
  main thread is only suspended.
- **`ThreadsReset()`** is called first thing where `setjmp(mark)` returns (`Src/main.c` 868) and before a RUN: the
  main stack's `StackFloor` and `stackwarn` again, the main thread's `SymL`, every thread marked ended, the blocks
  freed. The globals are left holding the state of the thread that stopped, so its locals can be printed at the
  prompt, as a SUB's can today.
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
| C stack | 16 KB by default (12 KB usable above the guard), `THREAD SIZE` | on-chip heap while 96 KB of it stays free, else SDRAM |
| Save areas: GOSUB 1.2 KB, FOR about 3.2 KB, DO about 4 KB, temporary memory 5 KB, locals, DATA, scalars | about 16 KB | SDRAM heap |
| `SymL` array | 2 bytes for each name in the program | with the save areas |

The save areas are full size so that a switch can never run out of memory. They are only touched for the entries in
use. On a G2 (about 15 MB of heap) and a G1 (about 4.5 MB, 3.5 MB with OPTION COMPILE ON) 32 KB a thread is small. The
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
| `THREAD START n, sub, arg [, arg ...]` | with arguments, each evaluated now and passed by value (a global array by reference) | 3 |
| `THREAD EXIT` | ends the thread that runs it, as its SUB's `END SUB` does | 1 |
| `THREAD YIELD` | gives up the rest of the time slice | 1 |
| `THREAD STOP n` | ends thread n where it stands; its locals, temporary memory and locks are freed | 2 |
| `THREAD PAUSE n`, `THREAD RESUME n` | the scheduler skips thread n until it is resumed | 2 |
| `THREAD SLICE ms` | the time slice, 1 to 100 ms | 2 |
| `THREAD SIZE kb [, locals]` | the stack and local slots of threads started after it | 2 |
| `THREAD LOCK k [, timeout]`, `THREAD UNLOCK k` | lock k (1 to 16): one thread at a time between them | 3 |
| `THREAD HOLD`, `THREAD RELEASE` | no other thread runs between them (for a few statements) | 3 |
| `THREAD WAIT n [, timeout]` | waits until thread n has ended | 3 |
| `THREAD WAIT FLAG f [, timeout]` | waits until `FLAG(f)` is 1 (the 64 flags the program already has) | 3 |
| `THREAD LIST` | each thread's number, state, line and free stack | 5 |
| `MM.INFO(THREAD)` | the number of the thread that runs it, 0 in the main program | 1 |
| `MM.INFO(THREAD n)` | thread n's state | 2 |

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
| `Src/Thread.c`, `Inc/Thread.h` (new) | the records, the stacks, `ContextSave` and `ContextLoad`, `ThreadSwap`, the scheduler, `cmd_thread`, `ThreadsReset` |
| `Inc/allcommands.h` | the `Thread` command |
| `Src/MM_Misc.c` | `check_interrupt` split into `IntScan` and `IntDispatch` with the scheduler's call between them; `PauseFor` on the microsecond clock, its path for a thread and the call in its loop; `MM.INFO(THREAD)` |
| `Src/CFunctions.c`, `Src/User.c`, `Src/Loader.c` | the hold count raised around a CSUB, a module call and a loadable program |
| `Src/Timers.c`, `Inc/External.h` | the slice count; `IntReady.gui` becomes two bits |
| `Src/MMBasic.c` | the local stack's base and limit as variables (`LocalBase`, `LocalLimit`) at five places; `error()`: the thread's number, its own scratch buffer in place of `inpbuf`; the stack's high-water mark (`StackPaint`, `StackLowest`) |
| `Src/Symbols.c` | `SymBindInit` sets all of `SymL` to -1 |
| `Src/Commands.c` | `cmd_end` stops the other threads; ERASE, REDIM and CLEAR refused; `execute()` in a thread runs from the thread's token buffer; RUN starts the stack's high-water mark |
| `Src/main.c` | `ThreadsReset()` where `setjmp(mark)` and `setjmp(run)` return; no `ContinuePoint` after threads |
| `Src/Draw.c` | PIXEL FILL moves the stack to free heap, which may be below a thread's `stackwarn`: the check is switched off for its length |
| `Src/Memory.c` | MEMORY lists the threads' memory |
| `Src/Stream.c` | none (the debug assertion apart) |

## Risks

1. **Stack depth.** Nobody knows how deep the firmware's commands and its housekeeping go. `CheckAbort` runs USB, the
   network (`WebPoll`) and Bluetooth from any thread's tail, and a TLS handshake may need several KB. The main stack's
   24 KB is known to be enough; a 12 KB thread stack is not known to be. Stage 0 measures it. If housekeeping is deep,
   it can be run on the main thread's stack from any thread (a call that borrows the suspended main stack below its
   saved pointer), which takes it out of every thread's budget.
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

Each stage ends with the regression (tests/cmm2 and tests/picomite with OPTION COMPILE OFF, ON and SHADOW) unchanged
on a G1 and a G2, and with the thread tests of every earlier stage passing. Thread tests go in tests/threads, each a
`.bas` with an `.expected`, run with OFF, ON and SHADOW.

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

**Done when** the regression output is identical and the timing programs (tests/bench) are within noise of the build
before, on both boards.

### Stage 1: threads that take turns

`THREAD START n, sub`, `THREAD EXIT`, `THREAD YIELD`, `MM.INFO(THREAD)`. A thread runs until it yields or ends.

1. `Thread.c`: the records, a stack from the heap with its guard and check word, the save areas, `ContextSave` and
   `ContextLoad`, `ThreadSwap`, `ThreadMain`, `ThreadFinish`, freeing an ended thread from another thread's stack.
2. The slot ranges and each thread's `SymL`.
3. `ThreadsReset` at the prompt's landing, at RUN and at END; the thread's number in error messages; `END` in a
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

| State | Where | For threads |
|---|---|---|
| The drawing target: `WritePage`, `ReadPage`, `HRes`, `VRes` | `Src/main.c` 177, `Src/Draw.c` 343 | `PAGE WRITE` and `FRAMEBUFFER WRITE` in one thread change where every thread draws: a lock, or one thread draws |
| Font and colours: `gui_font`, `gui_fcolour`, `gui_bcolour` | `Src/Draw.c` 262-264 | the same |
| The text cursor and print column: `CurrentX`, `CurrentY`, `MMCharPos`, `FileCharPos[]` | `Src/Draw.c` 266, `Src/main.c` 201, `Src/Commands.c` 452 | a `PRINT` is whole; two that should stay together need a lock |
| The console's key decoder: `InkeyUngot`, `MMInkey`'s and `MMgetchar`'s statics | `Src/main.c` 3537-3552, 3721 | one thread reads the keyboard |
| The current drive and directory: `filepath`, `DrivePaths` | `Src/FileIO.c` 124, 1470 | shared; `DRIVE` and `CHDIR` in one thread move every thread |
| `DIR$()` with no arguments, which goes on from the last search | `fun_dir`'s statics, `Src/FileIO.c` 1156 | one thread at a time |
| `OPTION BASE`, `EXPLICIT`, `DEFAULT`, `ANGLE`, `Y_AXIS`, `BREAK` and the rest | `Src/MM_Misc.c` 2488-2757 | shared: set them before the threads start |
| `TIMER`'s zero, `MATH RANDOMIZE`'s seed, the SENSORFUSION filter, `SYNC`'s period | `Src/MM_Misc.c` 671, `Src/MATHS.c` 127-138, `Src/External.c` 1629 | shared |
| Turtle, sprites and their collision results, sound, the GUI's controls and its last-control defaults | `Src/turtle.c` 90, `Src/sprites.c` 47-65, `Src/Audio.c` 124, `Src/GUI.c` 138-169 | shared: one thread should own each |
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

In the source and built (Release); not yet run on a board.

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

**Still to do for Stage 0, on the boards:** the regression with OPTION COMPILE OFF, ON and SHADOW against this
build and the build before; the timing programs against both; the stack measurements.
