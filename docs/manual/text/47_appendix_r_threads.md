# Appendix R – Threads

## What a thread is

A thread is an ordinary SUB that runs alongside the main program, for as long as it likes, with its own loops, its own LOCAL variables and its own PAUSEs. The firmware shares the processor between the main program and its threads, a millisecond at a time. A thread is started with THREAD START and ends when its SUB ends.

```
Dim Integer beats

Sub Heart
  Do
    Inc beats
    Pause 500                     ' the rest of the program runs meanwhile
  Loop
End Sub

Thread Start 1, Heart
Do
  Text 0, 0, "Beats: " + Str$(beats) + "  "
  Pause 100
Loop Until Inkey$ <> ""
End                               ' the threads stop with the program
```

Threads suit work that goes on by itself: a clock in the corner of the screen, a tune played note by note, a sensor read every second, a game's aliens each moving on its own, a long calculation while the screen stays alive, a network request while the user types. Interrupts remain the tool for a quick answer to an event; a thread is for something with a flow of its own, and the two work well together.

The *Colour Maximite 2 Threads User Manual* describes threads in detail, with worked programs. This appendix is an overview.

## The commands

{widths: 42,58}
| Command | What it does |
|---|---|
| THREAD START n [SIZE kb] [LOCALS m] [NEST d] [PRIORITY p], sub [, argument ...] | Starts thread n (1 to 16) running the SUB, with its arguments by value |
| THREAD EXIT | Ends the thread that runs it |
| THREAD YIELD | Gives up the rest of the time slice |
| THREAD STOP n | Ends thread n where it stands |
| THREAD PAUSE n, THREAD RESUME n | Holds thread n, and lets it go on |
| THREAD SLICE ms | The time slice, 1 to 100 ms (1 at RUN) |
| THREAD PRIORITY n, p | Thread n's priority (0 the main program): the slices in its turn, 1 to 10 (1 at RUN) |
| THREAD SIZE kb [, locals [, nest]] | The stack, local variables and depth of loops and calls of later threads (20 KB, 48 and 20 at RUN) |
| THREAD LOCK k [, timeout], THREAD UNLOCK k | One thread at a time between them (16 locks): a mutex |
| THREAD HOLD, THREAD RELEASE | No other thread runs between them |
| THREAD WAIT n [, timeout] | Waits until thread n has ended |
| THREAD WAIT FLAG f [, timeout] | Waits until FLAG(f) is 1 |
| THREAD LIST | Each thread's line, state and stack |
| MM.INFO(THREAD) | The number of the thread that runs it (0 the main program) |
| MM.INFO(THREAD n) | Thread n's state: 0 not running, 1 running, 2 in PAUSE, 3 paused, 4 waiting |
| MM.INFO(THREAD LOCK k) | The thread that holds lock k, or -1 |

Each is described in the commands and functions reference.

## How threads run

- **One thread runs at a time, and the switch comes only between statements.** A statement is never split: INC total in two threads never loses a count. A statement that takes a long time (a file copy, loading a picture, a PIXEL FILL of the screen) holds every thread until it ends.
- **The threads take turns, a time slice each** (1 ms, THREAD SLICE), in number order. A thread whose PAUSE has ended goes first, so things that are timed stay on time.
- **A thread's priority is the length of its turn.** Every thread, and the main program (thread 0), has a priority of 1 unless PRIORITY on THREAD START or THREAD PRIORITY n, p gives it another, up to 10: a thread at priority 5 runs five time slices to one for a thread at priority 1. No thread is left out, since the turns still go round in order. A thread that holds a lock which a thread of a higher priority is waiting for has that thread's turn until it unlocks. Long turns keep the other threads waiting longer, so keep THREAD SLICE at 1 ms with high priorities.
- **A thread that waits uses no processor time.** PAUSE, INPUT and LINE INPUT, the waits for a frame (PAGE COPY ...,B, FRAMEBUFFER WAIT and the others), SYNC, TEMPR(), the network's and Bluetooth's commands, THREAD LOCK and THREAD WAIT all let the other threads run meanwhile. A program can sit in INPUT for its user while its threads keep the screen moving.
- **A program without threads runs exactly as before**, and as fast. A switch between threads takes about 5 microseconds.
- OPTION COMPILE ON works with threads.

## What each thread has, and what is shared

Each thread has its own LOCAL variables, FOR and DO loops, SUB and FUNCTION calls, GOSUBs, ON ERROR setting with MM.ERRNO and MM.ERRMSG$, place in the DATA, EXECUTE line, print position, colours, font, PRINT @ mode and write page. A new thread starts with the print position, colours, font and write page of the thread that started it, and MODE gives every thread the new mode's.

Everything else belongs to the whole program: global variables and arrays, STATIC variables, the screen itself, files, the keyboard, sound, sprites, the turtle, the pins, the current drive and directory, the OPTIONs, TIMER and FLAG(). Two things follow:

- **Two statements that must go together need a lock.** A switch can come between any two statements, so `v = total` then `total = v + 1` in two threads can lose a count. Put them between THREAD LOCK 1 and THREAD UNLOCK 1 in every thread that does it, or between THREAD HOLD and THREAD RELEASE. A single statement (INC total) needs neither.
- **A lock is a mutex.** THREAD LOCK and THREAD UNLOCK are what other languages call taking and releasing a mutex. The sixteen exist from RUN and need no creating: choose a number for each shared thing, name it with CONST, and put the lock, the work and the unlock in one SUB that every thread calls. A thread may PAUSE or use a file while it has a lock (not inside a hold), and only the threads that want that lock wait. THREAD LOCK k, 0 after ON ERROR SKIP tries a lock without waiting: MM.ERRNO is 0 if it was taken. A lock cannot be taken twice by the thread that has it, and only that thread can unlock it. The Threads User Manual has more, with a lock its owner can take again and what to do in interrupt routines.
- **Give each shared thing one owner.** One thread reads the keyboard; one thread writes a file; each thread draws in its own part of the screen (two threads that PRINT without saying where write over each other). A variable that one thread sets and the others only read is always safe.

## Interrupts

An interrupt belongs to the thread that set it up (SETTICK, SETPIN, ON KEY, a COM port, the GUI and the rest). Its routine runs in that thread, with that thread's colours and print position, and to its end before any other thread runs. A thread can set up a SETTICK and wait in THREAD WAIT FLAG until its routine sets the flag:

```
Sub Sampler
  SetTick 100, Sample, 1
  Do
    Thread Wait Flag 1            ' asleep until the routine sets it
    Flag(1) = 0
    Print reading
  Loop
End Sub

Sub Sample
  reading = Pin(7)
  Flag(1) = 1
End Sub
```

The main program's interrupts run in the main program, as they always have. A routine whose thread has ended runs in the main program. A paused thread's interrupts wait until it is resumed, and those of a thread in INPUT (or another wait that takes no interrupt) until the wait ends.

## The network and Bluetooth

Each belongs to one thread. The main program may use it first, to set up (WEB CONNECT, WEB NTP); the first other thread that uses it then has it until that thread ends, and a WEB or BLUETOOTH command in any other thread is the error "The network belongs to thread n" (or "Bluetooth belongs to thread n"). The interrupt routines its thread set up (WEB TCP INTERRUPT and the others) run in that thread, so they may use it.

## Errors, END and Ctrl-C

An error in any thread ends the whole program, and says where it was ("Error in thread 3, line 40"). So do END in any thread and Ctrl-C. After an error or Ctrl-C each thread's line and state is listed, and CONTINUE is not possible. When the main program reaches its end, the threads stop with it: THREAD WAIT n lets the main program wait for a thread first.

These are refused in a thread other than the main program: RUN, CHAIN, FLASH RUN and CHAIN, RAM RUN and CHAIN, LIBRARY LOAD, LIST, DEBUG BREAK, XMODEM, ZMODEM, MsgBox() and GUI CLICK. ERASE, REDIM, CLEAR, SAVE CONTEXT and LOAD CONTEXT are refused while any thread runs.

## Memory and limits

A thread takes its stack (20 KB unless SIZE or THREAD SIZE says otherwise) and a record, whose size follows what the thread is given: about 6 KB by default. A stack of 20 KB lets a thread call about nine FUNCTIONs inside each other; SUB calls take none. Each thread may have 48 local variables unless LOCALS or THREAD SIZE says otherwise, and about ten threads can run with these sizes. Each thread's FOR loops, DO loops and calls may each be 20 deep unless NEST or THREAD SIZE says otherwise (the main program's 100, as always). So a default thread takes about 26 KB, and THREAD START n SIZE 12 LOCALS 8 NEST 4 makes a small thread of about 14 KB for a simple job.

THREAD LIST shows how much stack each thread has used, MEMORY has a line for the threads, TRACE ON shows a thread's lines as [#n:line], and OPTION PROFILING charges each line only with the time its own thread spent on it.
