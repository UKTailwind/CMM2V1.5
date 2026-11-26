# Errors and Debugging

## An error ends the program

An error in any thread that is not caught by ON ERROR ends the whole program, as an error always has. The message says which thread it was in:

```
Error in thread 3, line 40: Index out of bounds
```

ON ERROR is each thread's own: ON ERROR SKIP in one thread skips only that thread's next error, and MM.ERRNO and MM.ERRMSG$ are each thread's.

!NOTE PicoMite: the PicoMite shows the thread and line before the statement, as TRACE does, and the message after it: "[#3:40] x = a(i)" and then "Error : Index out of bounds".

## The list of threads

After an error or Ctrl-C, the firmware lists each thread with the line it was at and what it was doing:

```
Error in thread 3, line 40: Index out of bounds

Main program: line 18, in PAUSE
Thread 1: line 7, waiting for lock 2 (thread 3 has it)
Thread 3: line 40, running
```

This shows where every part of the program was, and, for a program that has stopped making progress, which thread waits for what. Ctrl-C on a program that seems stuck is the quickest way to find a deadlock.

CONTINUE is not possible after a program with threads has stopped: their stacks have gone.

## THREAD LIST

THREAD LIST prints the same list while the program runs, with each thread's stack: the most it has used, and its size. Both lists show a thread's priority when it is not 1, and "priority 1 (7 while its lock is waited for)" for a thread that has a longer turn for the moment because a thread at priority 7 waits for a lock it holds.

```
Main program: line 22, running, stack used at most 3456 of 27720
Thread 3: line 8, in PAUSE, stack used at most 660 of 16384
```

This is the way to choose a thread's SIZE: run the program through its deepest work, then look at THREAD LIST (see *Memory and Limits*).

!NOTE PicoMite: the main program's line has no stack figure: its stack is the processor's own, not one of the threads'.

## TRACE

TRACE ON prints each line's number as it runs, in square brackets. A line run by a thread other than the main program is shown with the thread's number:

```
[23][#1:13][#1:14][24]
```

Here the main program ran line 23, thread 1 ran lines 13 and 14, and the main program went on at line 24. TRACE LIST shows the last lines run, by every thread, in the order they ran.

## MEMORY

In a program with threads running, MEMORY has a last line with the memory of their stacks and records:

```
  48K      1 Thread (its stack and the records)
```

## OPTION PROFILING

With OPTION PROFILING ON, LIST PROFILE gives each line's count and average time. With threads, a line's time is only the time its own thread spent on it: the time other threads ran while it waited is theirs. A main program in PAUSE 300 while a thread counts shows a few microseconds for the PAUSE line, and the thread's lines show where the 300 ms went.

!NOTE PicoMite: the PicoMite's OPTION PROFILING reports by SUB and FUNCTION when the program ends. With threads, a SUB's time is only its own thread's: the time other threads ran while its thread waited is not charged to it. OPTION PROFILING is refused while the program has a thread, so set it before the first THREAD START.

## Stack overflow

A thread's stack holds its SUB and FUNCTION calls. A FUNCTION that calls itself too deeply, or a thread given too small a SIZE, can run out:

```
Error in thread 2, line 12: Stack overflow
```

The cure is a bigger SIZE for that thread, or less recursion. THREAD LIST shows how close each thread has come.

!NOTE PicoMite: the message is "Stack overflow, at depth n, stack s, heap h". On the RP2350 the processor also guards the bottom of each thread's stack, so an overrun restarts the PicoMite rather than overwriting memory; the RP2040 has no such guard, which is one more reason to give a thread room to spare.

## Common mistakes

- **A thread that never ends and a main program that does.** The main program reaching its end ends the program, and every thread with it. Put a loop, or THREAD WAIT, at the end of the main program.
- **Two threads reading the keyboard.** Each gets some of the keys. Let one thread read it.
- **Two statements on shared data without a lock.** See *Sharing Data Safely*.
- **A global variable as a thread's loop counter.** FOR i = 1 TO 10 in a SUB that two threads run, with a global i, is one counter for both. Make it LOCAL, as in an interrupt routine; OPTION EXPLICIT finds the ones that were never declared.
- **A lock left taken.** EXIT SUB between THREAD LOCK and THREAD UNLOCK: the next thread to want the lock waits for ever. Ctrl-C shows which thread has it.
- **A test of a value another thread is changing**, followed by an action on it: the value may have changed in between. Use THREAD HOLD or a lock around both.
- **A busy loop waiting for another thread** (`Do While done = 0 : Loop`). It works, but uses the processor the other thread needs. THREAD WAIT n, or THREAD WAIT FLAG, waits without.
- **Too little stack.** A thread of the default 20 KB can call about nine FUNCTIONs inside each other.

!NOTE PicoMite: a thread of the default 8 KB can call about three FUNCTIONs inside each other as text on a PC3, two on the RP2040, and about six with OPTION COMPILE ON.
