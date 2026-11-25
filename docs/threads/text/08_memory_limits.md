# Memory and Limits

## What a thread takes

A thread takes two blocks of memory while it runs, both freed when it ends:

- **its stack**: 20 KB unless SIZE or THREAD SIZE says otherwise. It comes from the fast memory inside the processor while plenty of that is free, and from the SDRAM otherwise;
- **its record**: the thread's own lists of its FOR loops, DO loops, calls, temporary strings and local variables, with its other state. Its size follows LOCALS and NEST: about 6 KB by default, about 2 KB at the smallest.

{widths: 50,25,25}
| Thread | Stack | In all |
|---|---|---|
| The default (SIZE 20 LOCALS 48 NEST 20) | 20 KB | about 26 KB |
| SIZE 12 LOCALS 8 | 12 KB | about 17 KB |
| SIZE 12 LOCALS 8 NEST 4 | 12 KB | about 14 KB |

On a Generation 1, with about 4 MB of memory for programs, and on a Generation 2 or 3, with about 15 MB, sixteen threads fit easily. A thread that runs EXECUTE takes another 512 bytes from its first EXECUTE to its end.

## The stack

A thread's stack holds the SUBs and FUNCTIONs it is inside. SUB calls take almost none of it. FUNCTION calls take about 1.4 KB each (less with OPTION COMPILE ON, more for a string FUNCTION). The commands themselves need room below that, up to about 6 KB for the largest (LOAD PNG), which every stack keeps free.

With the default 20 KB, a thread can be about nine FUNCTIONs deep. To choose a size:

1. Run the program through the deepest work each thread does.
2. Look at THREAD LIST: "stack used at most n of m".
3. Give each thread SIZE enough for its most, with a few KB to spare.

A thread that only loops and pauses, and calls SUBs, needs very little: SIZE 12, the smallest, is plenty.

## Local variables

Each thread has room for a number of LOCAL variables at once: 48 unless LOCALS or THREAD SIZE says otherwise. Every LOCAL, every parameter of every SUB and FUNCTION it is inside, and a FUNCTION's own result each take one. A FUNCTION that calls itself 14 deep with one parameter and one LOCAL needs 3 x 15 = 45. Running out is the error "Not enough Local variable memory".

All the threads' local variables come from one table of 512, which the main program shares, so the more each thread has, the fewer threads can run: with 48 each, about ten. LOCALS 8 for small threads lets all sixteen run.

## Loops and calls (NEST)

Each thread keeps its own lists of the loops and calls it is inside, and NEST says how long they are: 20 unless NEST or THREAD SIZE says otherwise. With NEST d a thread may be:

- d FOR loops inside each other;
- d DO loops inside each other;
- d SUB, FUNCTION and GOSUB calls deep, counted from its own SUB. An interrupt routine that runs in the thread counts as one of them.

Going deeper is the usual error: "Too many nested FOR loops", "Too many nested DO or WHILE loops", "Too many nested SUB/FUN" or "Too many nested GOSUB". NEST also sets how many temporary strings the thread's expressions may hold at once (64 and 16 more for each level), which no ordinary statement comes near; running out is "Not enough temporary memory".

The main program's lists are the ones it always had, 100 deep, whatever the threads are given. A thread that loops, pauses and calls a SUB or two needs no more than NEST 4, the smallest.

## Limits

{widths: 50,50}
| What | Limit |
|---|---|
| Threads at once | 16 (numbered 1 to 16), and the main program |
| Arguments to THREAD START | 16 |
| Stack (SIZE) | 12 to 512 KB, 20 by default |
| Local variables (LOCALS) | 8 to 256, 48 by default |
| Loops and calls deep (NEST) | 4 to 100, 20 by default |
| Time slice (THREAD SLICE) | 1 to 100 ms, 1 by default |
| Priority (PRIORITY, THREAD PRIORITY) | 1 to 10, 1 by default |
| Locks | 16 |
| Flags | 64 (FLAG(0) to FLAG(63)) |

## What cannot be done in a thread

These are refused in a thread other than the main program ("Not allowed in a thread"):

- RUN, CHAIN, FLASH RUN, FLASH CHAIN, RAM RUN, RAM CHAIN and LIBRARY LOAD, which replace the program;
- LIST, DEBUG BREAK, XMODEM and ZMODEM, which take over the console;
- MsgBox() and GUI CLICK, which take interrupts while they wait in a way only the main program can.

These are refused while any thread is running ("Not allowed while a thread is running"), as another thread may be using the variable or memory they change:

- ERASE, REDIM and CLEAR;
- SAVE CONTEXT and LOAD CONTEXT.

And these are not possible:

- THREAD START at the command prompt;
- CONTINUE after a program with threads has stopped.

## Things to know

- **FRAMEBUFFER CLOSE** is refused while any thread writes to the framebuffer ("Framebuffer is set for write").
- **MODE** gives every thread the new mode's print position, colours, font and write page.
- **CLS** puts only the running thread's print position back to the top.
- **TIMER = 0** in one thread sets the timer for all.
- **OPTION settings** (BASE, DEFAULT, EXPLICIT, ANGLE and the rest) are the program's: set them before the threads start.
- **Loadable programs** (RUN "name.app"), the C compiler's programs and MicroPython do not use threads; a CSUB runs whole, with no switch inside it.
