# Errors and Debugging

## An error ends the program

An error in any thread that is not caught by ON ERROR ends the whole program, as an error always has. The message says which thread it was in:

```
Error in thread 3, line 40: Index out of bounds
```

ON ERROR is each thread's own: ON ERROR SKIP in one thread skips only that thread's next error, and MM.ERRNO and MM.ERRMSG$ are each thread's.

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

THREAD LIST prints the same list while the program runs, with each thread's stack: the most it has used, and its size.

```
Main program: line 22, running, stack used at most 3456 of 27720
Thread 3: line 8, in PAUSE, stack used at most 660 of 16384
```

This is the way to choose a thread's SIZE: run the program through its deepest work, then look at THREAD LIST (see *Memory and Limits*).

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

## Stack overflow

A thread's stack holds its SUB and FUNCTION calls. A FUNCTION that calls itself too deeply, or a thread given too small a SIZE, can run out:

```
Error in thread 2, line 12: Stack overflow
```

The cure is a bigger SIZE for that thread, or less recursion. THREAD LIST shows how close each thread has come.

## Common mistakes

- **A thread that never ends and a main program that does.** The main program reaching its end ends the program, and every thread with it. Put a loop, or THREAD WAIT, at the end of the main program.
- **Two threads reading the keyboard.** Each gets some of the keys. Let one thread read it.
- **Two statements on shared data without a lock.** See *Sharing Data Safely*.
- **A test of a value another thread is changing**, followed by an action on it: the value may have changed in between. Use THREAD HOLD or a lock around both.
- **A busy loop waiting for another thread** (`Do While done = 0 : Loop`). It works, but uses the processor the other thread needs. THREAD WAIT n, or THREAD WAIT FLAG, waits without.
- **Too little stack.** A thread of the default 20 KB can call about nine FUNCTIONs inside each other.
