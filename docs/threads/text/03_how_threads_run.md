# How Threads Run

## Taking turns

Only one thread runs at a time. Each runs until one of these happens, and then the next thread that can run has its turn:

- its time slice is up: 1 ms unless THREAD SLICE says otherwise;
- it waits: PAUSE, INPUT, THREAD LOCK, THREAD WAIT and the other waits of the next section;
- it runs THREAD YIELD, which gives up the rest of its slice;
- an interrupt that belongs to another thread is due (see *Interrupts and Threads*).

The threads take turns in number order, the main program among them as thread 0. A thread whose PAUSE has just ended goes first, ahead of the others, so that a thread that does something every 50 ms does it on time even while others are busy.

THREAD SLICE ms sets the slice, from 1 to 100 ms. A longer slice means fewer switches, so slightly less time lost to them, but a busy thread then keeps the others waiting longer. One millisecond suits almost every program.

## Priority

Every thread, the main program included, has a priority from 1 to 10. It is 1 unless you say otherwise. A thread's priority is the number of time slices in its turn: a thread at priority 5 beside one at priority 1 runs for five slices, then the other runs for one, and round again. So two busy threads at 5 and 1 share the processor five to one.

```
Thread Start 1 Priority 5, Compute        ' five slices a turn
Thread Start 2, Logger                    ' one
Thread Priority 0, 3                      ' the main program: three
Thread Priority 2, 4                      ' thread 2 from now on: four
```

THREAD PRIORITY n, p changes the priority of a running thread (n = 0 is the main program) from its next turn. All priorities are 1 again at the next RUN, and THREAD LIST shows any that is not 1.

- **No thread is left out.** The threads still take turns in number order: a higher priority only makes a turn longer. A thread waits for its turn while each of the others has its own, whatever its own priority is.
- **Priority shares the time of busy threads.** A thread that waits (PAUSE, INPUT, THREAD LOCK and the rest) gives up what is left of its turn, as it always did, and a thread whose PAUSE has just ended still goes first. So a thread that mostly waits needs no priority to be on time.
- **Long turns make the others wait.** With one busy thread at priority 10, the others run once every 10 ms instead of every 1 ms. A key typed into INPUT is then echoed about 5 ms later than usual, and beside three such threads about 15 ms later. THREAD SLICE multiplies this, so keep the slice at 1 ms when you use high priorities.
- **A lock lends priority.** While a thread waits at THREAD LOCK, the thread that holds the lock has the waiting thread's turn, if that is longer than its own, until it unlocks. So a thread at priority 1 that holds a lock does not keep a thread at priority 9 waiting at its own slow pace. THREAD WAIT and the flags lend nothing.

## Statements are never split

The switch to another thread is only ever made between two statements. A statement is always whole: no other thread can see it half done. This is what makes threads in MMBasic straightforward to use:

- INC total, or total = total + 1, in two threads never loses a count;
- a PRINT always prints all its items together;
- a LINE, BOX or TEXT is always drawn whole.

The other side is that a long statement holds every thread until it ends. A SORT of a large array, a COPY of a big file, a LOAD PNG or a PIXEL FILL of the whole screen are each one statement: the other threads wait for them.

One exception to "whole": a FUNCTION of your own, called from inside a statement, is made of statements, and the switch can come between them. The statement that called it continues afterwards, so the statement still happens, but other threads may run before it finishes.

## Waits that let the other threads run

A thread that waits uses no processor time: the others run until what it waits for has happened. These commands wait in this way:

{widths: 35,65}
| Command | Waits for |
|---|---|
| PAUSE, CPU SLEEP | the time |
| INPUT, LINE INPUT | a line typed at the keyboard or the console, or from a COM port |
| FRAMEBUFFER WAIT, PAGE COPY ...,B, PAGE DISPLAY, FRAMEBUFFER WINDOW ...,B, MAP SET, RESET, GREYSCALE and MAXIMITE | the start of the next frame |
| SYNC | the end of SYNC's period |
| ADC START (with no interrupt) | the samples |
| TEMPR() | the sensor's conversion |
| WEB commands (requests, NTP, MQTT, PING, UDP) | the network's answer |
| BLUETOOTH commands | the device |
| THREAD LOCK | the lock |
| THREAD WAIT | a thread to end, or a flag |

So a program can sit in INPUT for its user while threads keep the screen moving, or wait two seconds for a web server while a game goes on.

When two threads wait for the same frame (FRAMEBUFFER WAIT, PAGE COPY ...,B), both get it: each wakes at the start of the next frame.

## Waits that hold every thread

Other commands that take time hold every thread until they end, as any long statement does. Among them:

- the timed pin commands: PULSIN(), DISTANCE(), BITSTREAM, DEVICE SERIALRX, the DHT22 and 1-Wire timing, which must not be interrupted;
- the SD card, USB drive, I2C and SPI transfers, which take milliseconds;
- PRINT to the console or a COM port whose output buffer is full, until there is room;
- PLAY BBC SOUND with its queue full.

## The cost

A program that starts no thread runs exactly as fast as it did before threads existed. With threads, a switch from one thread to another takes about 5 microseconds, so 1 ms slices cost about half a percent of the processor. A thread in a wait costs nothing.

Starting a thread takes about a third of a millisecond, mostly for preparing its stack.

## OPTION COMPILE

Threads work with OPTION COMPILE ON and SHADOW as they do without: the compiled statements of several threads, even of the same SUB at the same time, each work on their own thread's variables.
