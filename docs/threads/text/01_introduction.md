# Introduction

## What threads are

A thread is a SUB of your program that runs alongside the main program. Once started, it goes on by itself: it can loop, wait, call other SUBs and FUNCTIONs, and keep its own LOCAL variables, while the main program carries on with its own work. A program can have up to sixteen threads at once.

The Colour Maximite 2 has one processor, so only one thread runs at any instant. The firmware shares the processor between them: each runs for a short time slice, a millisecond unless you choose otherwise, and the next one then has its turn. The switch from one thread to the next always comes between two statements, never in the middle of one. Because the slices are short, the threads appear to run at the same time.

## Why use them

MMBasic programs have long done several things at once with interrupts: SETTICK calls a routine every so often, SETPIN calls one when a pin changes, and the main program does the rest. That works well when each event needs a short, quick answer. It works less well when the job has a flow of its own: wait a second, do something, wait for a key, do something else. Written with interrupts, such a job becomes a routine that is called again and again and a variable that remembers where it had got to.

A thread lets you write that job as the straight sequence of steps it is:

```
Sub TrafficLights
  Do
    Lamp RGB(red)    : Pause 5000
    Lamp RGB(yellow) : Pause 1000
    Lamp RGB(green)  : Pause 4000
    Lamp RGB(yellow) : Pause 1000
  Loop
End Sub
```

While this thread is in a PAUSE it uses no processor time, and the rest of the program runs at full speed.

Threads suit:

- something with a rhythm of its own: a clock in the corner of the screen, a status line, a flashing LED, a tune played note by note;
- many of the same thing: each ball, alien or robot of a game as the same SUB in a thread of its own, with its own variables;
- a long calculation with a live screen: the work in a thread, the main program showing progress and reading the keys;
- waiting on the outside world: a thread waits on the network, a serial port or a sensor while the rest of the program goes on;
- a program that listens while it works: the main program sits in INPUT for the next command while threads run the machine or the display.

## A first program

```
Option Explicit
Dim Integer beats

Sub Heart
  Do
    Inc beats
    Pause 500
  Loop
End Sub

Thread Start 1, Heart
Do
  Text 0, 0, "Beats: " + Str$(beats) + "  "
  Pause 100
Loop Until Inkey$ <> ""
End
```

THREAD START 1, Heart starts thread 1 running the SUB Heart. From then on two things go on at once: Heart adds one to beats every half a second, and the main program shows beats ten times a second until a key is pressed. END stops the program, and its threads with it.

## How this manual is arranged

- **Starting and Ending Threads**: THREAD START, its arguments, and the ways a thread ends.
- **How Threads Run**: the time slice, which commands let the other threads run while they wait, and what threads cost.
- **Sharing Data Safely**: what each thread has of its own, what they share, and the locks, holds and flags that keep shared data right.
- **Interrupts and Threads**: interrupts belong to the thread that sets them up.
- **The Network and Bluetooth**: each belongs to one thread.
- **Errors and Debugging**: what happens at an error, Ctrl-C and END, and THREAD LIST, TRACE, MEMORY and profiling.
- **Memory and Limits**: what a thread takes, SIZE and LOCALS, and what cannot be done in a thread.
- **Worked Programs**: complete programs that use threads.
- **Appendix A**: the commands and functions in brief.

The firmware's user manual has the same commands in its reference sections, and an overview in its Appendix R.
