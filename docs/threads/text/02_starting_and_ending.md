# Starting and Ending Threads

## THREAD START

```
THREAD START n, sub [, argument ...]
THREAD START n SIZE kb LOCALS m NEST d PRIORITY p, sub [, argument ...]
```

THREAD START starts thread 'n', a number from 1 to 16 that you choose, running the SUB 'sub'. The main program is thread 0. THREAD START returns at once: the new thread runs when its turn comes, a moment later.

The SUB must be a SUB of the program (not a FUNCTION, and not a CSUB), and thread 'n' must not already be running ("Thread n is running"). The number becomes free again when the thread ends. THREAD START is for programs: at the command prompt it is refused.

A thread starts with the print position, colours, font and write page of the thread that started it.

## Arguments

Up to sixteen arguments may follow the SUB's name, as in an ordinary call:

```
Sub Blink(pin%, ms%)
  SetPin pin%, DOUT
  Do
    Pin(pin%) = 1 - Pin(pin%)
    Pause ms%
  Loop
End Sub

Thread Start 1, Blink, 7, 250
Thread Start 2, Blink, 8, 400
```

Each argument is worked out by THREAD START, in the thread that runs it, and handed to the new thread's SUB by value: the new thread has its own copy, and changing its parameter changes nothing outside it. Integers, floats and strings are all passed exactly, and the usual conversions apply (an integer for a float parameter, and so on).

A whole array, written as its name with empty brackets, is passed as itself, so that the thread can fill it or read it:

```
Dim Float samples(99)
Thread Start 3, Recorder, samples(), 10
```

It must be a global array. A SUB's own array would vanish when that SUB returned while the thread still used it, so it is refused ("Only a global array can be passed to a thread").

Several threads may run the same SUB at the same time, each with its own parameters and LOCAL variables. MM.INFO(THREAD) tells a SUB which thread is running it.

## SIZE, LOCALS and NEST

Each thread has a stack of its own, for the SUB and FUNCTION calls it makes, room for a number of LOCAL variables, and its own lists of the FOR loops, DO loops and calls it is inside. Unless you say otherwise a thread gets a 20 KB stack, 48 local variables, and loops and calls 20 deep. SIZE, LOCALS and NEST, after the thread's number and in any order, set them for that thread:

```
Thread Start 4 Size 12 Locals 8 Nest 4, Clock   ' a small thread
Thread Start 5 Size 48 Locals 100, Solver       ' a deep recursion
```

SIZE is 12 to 512 KB, LOCALS 8 to 256 and NEST 4 to 100. THREAD SIZE sets the defaults for threads started later without their own:

```
Thread Size 12, 16                               ' stack 12 KB, 16 locals
Thread Size 12, 16, 8                            ' and loops and calls 8 deep
```

They go back to 20 KB, 48 and 20 at the next RUN. PRIORITY p, in the same place, gives the thread more of the processor than the others: see *Priority* in the next chapter. The chapter *Memory and Limits* says how to choose them.

## How a thread ends

A thread ends:

- when its SUB ends (END SUB, or EXIT SUB at its top level);
- at THREAD EXIT, wherever it is, even deep inside SUBs and FUNCTIONs;
- when another thread stops it with THREAD STOP n;
- when the program ends.

When a thread ends, its LOCAL variables and the memory it used are freed, its locks are given up, and if the network or Bluetooth belonged to it they are free again. Its number may then be used for a new thread.

THREAD STOP n ends thread 'n' wherever it is: in a loop, in a PAUSE, inside a FUNCTION. Nothing happens if thread 'n' is not running. A thread may stop itself, which is the same as THREAD EXIT.

THREAD EXIT is an error in the main program ("Not in a thread"): the main program ends with END.

## The end of the program

The program ends when any thread, the main program included, runs END, when an error is not caught by ON ERROR, or at Ctrl-C. Every thread stops with it, wherever it is. SUB MM.END, if the program has one, runs in the thread that ran END.

The program also ends when the main program runs off its end. If the main program should wait for its threads to finish first, THREAD WAIT does it:

```
Thread Start 1, Worker
Thread Start 2, Worker
Thread Wait 1
Thread Wait 2
Print "Both done"
```

## PAUSE and RESUME

THREAD PAUSE n holds thread 'n': it stays where it is and does not run until THREAD RESUME n. Its interrupts wait for it too. A thread may pause itself, and stops at the end of that statement; another thread then has to resume it. Both are errors if thread 'n' is not running ("Thread n is not running").

```
If k$ = "P" Then Thread Pause 1
If k$ = "R" Then Thread Resume 1
```

## Is it running?

MM.INFO(THREAD n) gives the state of thread 'n':

{widths: 15,85}
| Value | Thread n is |
|---|---|
| 0 | not running: never started, or ended |
| 1 | running, or ready to run when its turn comes |
| 2 | in a PAUSE |
| 3 | paused by THREAD PAUSE |
| 4 | waiting: for a lock, another thread, a flag, the next frame or the network |

MM.INFO(THREAD 0), the main program, is always 1. MM.INFO(THREAD), with no number, is the number of the thread that runs it.
