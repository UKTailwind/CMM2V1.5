# Appendix A – The Commands in Brief

{widths: 45,55}
| Command or function | What it does |
|---|---|
| THREAD START n [SIZE kb] [LOCALS m] [NEST d], sub [, argument ...] | Starts thread n (1 to 16) running the SUB, with up to 16 arguments by value (a global array by name). SIZE 12 to 512 KB, LOCALS 8 to 256, NEST 4 to 100, for this thread only |
| THREAD EXIT | Ends the thread that runs it |
| THREAD STOP n | Ends thread n where it stands (nothing if it is not running) |
| THREAD YIELD | Gives up the rest of the time slice |
| THREAD PAUSE n | Holds thread n, and its interrupts |
| THREAD RESUME n | Lets thread n go on |
| THREAD SLICE ms | The time slice, 1 to 100 ms (1 at RUN) |
| THREAD SIZE kb [, locals [, nest]] | The stack, local variables and depth of loops and calls of threads started without SIZE, LOCALS and NEST (20, 48 and 20 at RUN) |
| THREAD LOCK k [, timeout] | Takes lock k (1 to 16), waiting for it if another thread has it; "Lock k timed out" after 'timeout' ms |
| THREAD UNLOCK k | Gives lock k up |
| THREAD HOLD | No other thread runs, and their interrupts wait, until THREAD RELEASE |
| THREAD RELEASE | Ends THREAD HOLD (they nest) |
| THREAD WAIT n [, timeout] | Waits until thread n has ended, or for 'timeout' ms |
| THREAD WAIT FLAG f [, timeout] | Waits until FLAG(f) is 1 (0 to 63), or for 'timeout' ms |
| THREAD LIST | Each thread's line, state and stack |
| MM.INFO(THREAD) | The running thread's number (0 the main program) |
| MM.INFO(THREAD n) | 0 not running, 1 running or ready, 2 in PAUSE, 3 paused, 4 waiting |
| MM.INFO(THREAD LOCK k) | The thread holding lock k (0 the main program), or -1 |

## The error messages

{widths: 45,55}
| Message | Meaning |
|---|---|
| Thread n is running | THREAD START of a number already in use |
| Thread n is not running | THREAD PAUSE or RESUME of a thread that is not running |
| x is not a SUB | THREAD START names something that is not a SUB |
| Only a global array can be passed to a thread | A SUB's own array given to THREAD START |
| Not enough memory for a thread | No room for the thread's stack and record |
| Not enough local variables for a thread | The local variable table is full (smaller LOCALS) |
| Not in a thread | THREAD EXIT in the main program |
| Not allowed in a thread | RUN, CHAIN, LIST and the others of *Memory and Limits* |
| Not allowed while a thread is running | ERASE, REDIM, CLEAR, SAVE or LOAD CONTEXT |
| Lock k timed out | THREAD LOCK k, timeout: the lock was not free in time |
| Lock k is already held by this thread | THREAD LOCK of a lock the thread holds |
| Lock k is not held by this thread | THREAD UNLOCK of a lock the thread does not hold |
| The threads are not held | THREAD RELEASE with no THREAD HOLD |
| Cannot wait while the threads are held | A wait inside THREAD HOLD |
| Cannot wait in an interrupt routine | THREAD LOCK or WAIT that cannot end at once, in a routine |
| A thread cannot wait for itself | THREAD WAIT of the running thread's number |
| The network belongs to thread n | A WEB command in a thread that does not own the network |
| Bluetooth belongs to thread n | A BLUETOOTH command in a thread that does not own Bluetooth |
| Thread n overflowed its stack | Thread n's stack was overrun (larger SIZE) |
| Stack overflow | A thread's FUNCTION calls went too deep for its stack |
| Not enough Local variable memory | A thread made more LOCALs than its LOCALS |
| Too many nested FOR loops (or DO or WHILE loops, SUB/FUN, GOSUB) | A thread's loops or calls went deeper than its NEST |
