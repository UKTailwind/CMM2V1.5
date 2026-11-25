# Sharing Data Safely

## What each thread has of its own

Each thread has its own:

- LOCAL variables and SUB and FUNCTION parameters;
- FOR, DO and WHILE loops, and the SUBs, FUNCTIONs and GOSUBs it is inside;
- ON ERROR setting, MM.ERRNO and MM.ERRMSG$;
- place in the DATA statements for READ (each thread reads from the start, or from its own RESTORE);
- EXECUTE line;
- print position, colours, font, PRINT @ mode and write page (PAGE WRITE, FRAMEBUFFER WRITE).

The last group means that one thread's COLOUR, FONT, PRINT @ or PAGE WRITE does not change another thread's. A new thread starts with those of the thread that started it, and MODE gives every thread the new mode's.

## What they share

Everything else belongs to the whole program:

- global variables and arrays, and STATIC variables;
- the screen itself, its pages and the framebuffers;
- open files, the current drive and directory, and DIR$() going on from its last search;
- the keyboard, the mouse and the gamepads;
- sound, sprites, BLIT buffers, the turtle and the GUI's controls;
- the pins, I2C, SPI, 1-Wire and the COM ports, and MM.I2C and MM.ONEWIRE after them;
- the OPTIONs, TIMER, RANDOMIZE's sequence, SYNC's period and FLAG().

## The danger: two statements that belong together

Because a switch can come between any two statements, a job that takes two statements can be cut in the middle by another thread doing the same job. Suppose two threads each add to a shared total like this:

```
v = total
total = v + 1
```

If a switch comes between the two lines, both threads read the same total, both add one, and one count is lost. The same thing happens with:

- `If free > 0 Then free = free - 1` (the test and the change);
- `Colour RGB(red)` then `Print "Alarm!"` in one thread, `Colour RGB(green)` and `Print` in another, *if* they share the print position;
- reading a record from a file in one thread while another moves the file's position.

These faults are timing faults: a program may run correctly a thousand times and fail the next. It is worth arranging a program so that they cannot happen. There are four ways.

## Way 1: one statement

A job done in one statement cannot be cut. INC total is one statement; so is total = total + 1. TEXT with its colour (TEXT x, y, s$, , , , RGB(red)) carries the colour with it.

## Way 2: one writer, one owner

A variable that only one thread sets, and the others only read, is always safe. More generally, give each shared thing an owner: one thread reads the keyboard, one thread writes the log file, one thread draws each part of the screen. The other threads leave values in global variables for the owner, and the owner acts on them.

## Way 3: a lock

When several threads must do the same multi-statement job on shared data, a lock lets only one of them do it at a time:

```
Thread Lock 1
v = total
total = v + 1
Thread Unlock 1
```

Every thread that does the job takes the same lock first. A thread that reaches THREAD LOCK 1 while another holds it waits there, using no processor time, until the holder reaches THREAD UNLOCK 1. There are sixteen locks, numbered 1 to 16: use a different number for each separate thing that needs protecting.

The rules:

- A lock belongs to the thread that took it. Taking it again in the same thread is an error ("Lock k is already held by this thread"), and so is unlocking a lock another thread holds ("Lock k is not held by this thread").
- A thread that ends, or is stopped, gives up its locks.
- THREAD LOCK k, timeout waits at most 'timeout' milliseconds; if the lock is still taken then, it is the error "Lock k timed out". A program that wants to go on without the lock catches the error with ON ERROR SKIP.
- MM.INFO(THREAD LOCK k) is the thread that holds lock k (0 the main program), or -1.
- Keep the statements between LOCK and UNLOCK few and quick: other threads that want the lock wait meanwhile.

### Deadlock

Two threads that each hold one lock and wait for the other's wait for ever:

```
' thread 1                    ' thread 2
Thread Lock 1                 Thread Lock 2
Thread Lock 2                 Thread Lock 1
```

Avoid it by always taking locks in the same order (1 before 2, everywhere). If a program does stop like this, Ctrl-C lists each thread with the lock it waits for and the thread that holds it.

## Way 4: THREAD HOLD

THREAD HOLD stops every other thread from running until THREAD RELEASE. It is the simplest way to make two or three quick statements into one:

```
Thread Hold
If free > 0 Then free = free - 1 : got = 1
Thread Release
```

Use it only for a few quick statements: everything else waits, the other threads' interrupts included. A wait that cannot end at once (PAUSE, THREAD LOCK, INPUT and the rest) is an error inside a hold ("Cannot wait while the threads are held"), since no other thread could make it end. HOLD and RELEASE nest; RELEASE with no HOLD is an error.

## Passing messages with flags

FLAG(0) to FLAG(63) are bits the whole program shares. THREAD WAIT FLAG f waits, using no processor time, until FLAG(f) is 1. This lets one thread wake another:

```
Sub Producer
  Do
    item = NewReading()
    Flag(1) = 1                     ' an item is ready
    Thread Wait Flag 2              ' until it has been taken
    Flag(2) = 0
  Loop
End Sub

Sub Consumer
  Do
    Thread Wait Flag 1
    Flag(1) = 0
    Process item
    Flag(2) = 1                     ' taken
  Loop
End Sub
```

THREAD WAIT FLAG does not clear the flag: the thread that waited clears it when it has dealt with it, as above. THREAD WAIT FLAG f, timeout gives up after 'timeout' milliseconds; the program then looks at FLAG(f) to see which happened.

An interrupt routine can set a flag too, which is how a thread waits for an event (see *Interrupts and Threads*).

## Waiting for a thread

THREAD WAIT n waits until thread 'n' has ended, at once if it is not running. With a timeout it gives up after that many milliseconds, and MM.INFO(THREAD n) says whether the thread ended. A thread cannot wait for itself.

## The screen and the keyboard

Each thread has its own print position, so two threads printing with PRINT @ or TEXT at different places do not disturb each other, and a thread drawing a clock does not move the line a user is typing at an INPUT. But the screen itself is one: two threads that PRINT without saying where both carry on from where they were and write over each other, and CLS or a scroll moves what is under every thread's position. Give each thread its own part of the screen.

The keyboard is one too. INKEY$, INPUT and KEYDOWN in two threads at once race for the keys, each getting some. Let one thread read the keyboard, and pass what it reads to the others.
