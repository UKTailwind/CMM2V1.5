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

!NOTE PicoMite: the PicoMite's write page is its FRAMEBUFFER WRITE target (N, F, L, T or 2), each thread's own. FRAMEBUFFER CLOSE refuses a buffer another thread writes to ("Framebuffer in use by thread n"); a thread that closes the buffer it writes to itself goes back to N, as a program always has.

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

A lock is what other languages and operating systems call a *mutex*. The sections after *Deadlock* say how to program with one.

The rules:

- A lock belongs to the thread that took it. Taking it again in the same thread is an error ("Lock k is already held by this thread"), and so is unlocking a lock another thread holds ("Lock k is not held by this thread").
- A thread that ends, or is stopped, gives up its locks.
- THREAD LOCK k, timeout waits at most 'timeout' milliseconds; if the lock is still taken then, it is the error "Lock k timed out". A program that wants to go on without the lock catches the error with ON ERROR SKIP.
- MM.INFO(THREAD LOCK k) is the thread that holds lock k (0 the main program), or -1.
- Keep the statements between LOCK and UNLOCK few and quick: other threads that want the lock wait meanwhile.
- While a thread of a higher priority waits for a lock, the thread that holds it has that thread's longer turn until it unlocks (see *Priority* in *How Threads Run*), so a thread of a low priority does not keep it waiting long.

### Deadlock

Two threads that each hold one lock and wait for the other's wait for ever:

```
' thread 1                    ' thread 2
Thread Lock 1                 Thread Lock 2
Thread Lock 2                 Thread Lock 1
```

Avoid it by always taking locks in the same order (1 before 2, everywhere). If a program does stop like this, Ctrl-C lists each thread with the lock it waits for and the thread that holds it.

### A lock is a mutex

A *mutex*, short for mutual exclusion, is something that only one thread can have at a time. MMBasic has no command of that name because THREAD LOCK and THREAD UNLOCK are it: THREAD LOCK takes the mutex (other systems say acquire) and THREAD UNLOCK gives it back (release). Where another system has a program create a mutex, lock it, unlock it and at last destroy it, here the sixteen are there from RUN, all free. A program chooses a number for each thing it wants to protect. Nothing is created, and nothing needs tidying away.

Give each number a name with CONST. The program then says what each mutex guards, and two parts of it cannot choose the same number by accident:

```
Const mxTotal = 1, mxLog = 2

Thread Lock mxTotal
v = total
total = v + 1
Thread Unlock mxTotal
```

A mutex protects nothing by itself. It is an agreement: every part of the program that works on the shared thing takes the same mutex first, and a part that does not is not stopped. The sure way to keep the agreement is to put the lock, the work and the unlock in one SUB, and to have every thread do the work by calling it:

```
Sub Deposit(amount%)
  Local Integer v
  Thread Lock mxTotal
  v = total
  total = v + amount%
  Thread Unlock mxTotal
End Sub
```

Leave such a SUB only at its end. An EXIT SUB between the LOCK and the UNLOCK leaves the mutex taken: every other thread that wants it then waits for ever, and the same thread taking it again is the error "Lock k is already held by this thread".

A thread may wait while it has a mutex: PAUSE, INPUT, a file, the network. That is the difference from THREAD HOLD (*Way 4*, below), which stops every other thread and so allows no wait. With a mutex only the threads that want that mutex wait, and the rest run on. Use a mutex for longer work on one shared thing, such as a file that several threads write to or a display on an SPI bus, and a hold for two or three quick statements.

### Trying a mutex

THREAD LOCK k, 0 takes the mutex if it is free and gives up at once if it is not. Giving up is the error "Lock k timed out", so ON ERROR SKIP goes before it, and MM.ERRNO says which happened:

```
On Error Skip
Thread Lock mxLog, 0
If MM.Errno = 0 Then
  Print #2, msg$                    ' it was free, and is this thread's now
  Thread Unlock mxLog
Else
  Inc missed                        ' another thread has it: do something else
EndIf
```

A larger number waits that long first: THREAD LOCK mxLog, 100 gives the thread that has it a tenth of a second to finish. MM.INFO(THREAD LOCK k) is for showing which thread has a mutex, not for deciding whether to take it: by the next statement another thread may have taken it.

### A mutex the thread already has

A mutex is not counted: the thread that has it cannot take it a second time. That matters when a SUB that takes a mutex calls another SUB that takes the same one. The simplest cure is to decide which SUBs lock: the outer ones, which the threads call, take the mutex, and the inner ones they share expect to be called with it taken.

Where that is awkward, these two SUBs make a mutex that its owner may take again, which other systems call a recursive mutex. Each Enter is matched by a Leave, and the mutex is given back at the last Leave:

```
Dim Integer depth(16)

Sub Enter(k%)
  If MM.Info(Thread Lock k%) <> MM.Info(Thread) Then Thread Lock k%
  Inc depth(k%)
End Sub

Sub Leave(k%)
  Inc depth(k%), -1
  If depth(k%) = 0 Then Thread Unlock k%
End Sub
```

Only the thread that has the mutex changes its depth, so the count needs no protection of its own.

A mutex also belongs to its thread: only the thread that locked it can unlock it. So it cannot be locked in one thread and unlocked in another as a signal between them. The flags do that (*Passing messages with flags*, below).

### Busy threads and a mutex

A thread that waits for a mutex gets it when its turn comes round and the mutex is free. A thread that unlocks and locks again straight away, in a loop with no wait in it, has usually locked it again by then. The others get in only when the end of its time slice falls between the UNLOCK and the LOCK. They do get in, but unevenly: of two such threads tried on a Generation 3, one did about 40% of the work and the other 60%. THREAD YIELD after the UNLOCK makes them take turns exactly:

```
Do
  Thread Lock mxTotal
  ' ... the work ...
  Thread Unlock mxTotal
  Thread Yield                      ' a thread that waits for it has it next
Loop
```

A loop with a PAUSE or any other wait outside the mutex needs no THREAD YIELD.

### A mutex and interrupt routines

An interrupt routine runs in the thread that set it up, between two of that thread's statements. That can be between its THREAD LOCK and its THREAD UNLOCK, and between its THREAD HOLD and its THREAD RELEASE. So neither keeps a thread's own interrupt routine out: to the firmware the routine is that thread. In a routine:

- THREAD LOCK of a mutex its own thread has is the error "Lock k is already held by this thread";
- THREAD LOCK of one another thread has is the error "Cannot wait in an interrupt routine", since nothing else runs until the routine ends;
- THREAD LOCK of a free one takes it, and the routine must unlock it before it ends.

So keep mutexes out of interrupt routines. A routine may also come while another thread is half way through its protected work, and would see that work half done. Let the routine store what it has in a variable of its own and set a flag, and let its thread do the protected work when THREAD WAIT FLAG wakes it (see *Interrupts and Threads*).

## Way 4: THREAD HOLD

THREAD HOLD stops every other thread from running until THREAD RELEASE. It is the simplest way to make two or three quick statements into one:

```
Thread Hold
If free > 0 Then free = free - 1 : got = 1
Thread Release
```

Use it only for a few quick statements: everything else waits, the other threads' interrupts included. HOLD and RELEASE nest; RELEASE with no HOLD is an error.

Waits inside a hold:

- **THREAD WAIT** lets go of the hold while it waits, and takes it again when the wait ends: the other threads run meanwhile, and the statements after the wait are held again. Other systems call this a condition variable. *A queue between threads*, below, shows what it is for. If what it waits for is so already, it does not wait, and the hold is kept throughout.
- **THREAD LOCK** of a lock another thread has is an error inside a hold ("Cannot wait while the threads are held"), since that thread could not run to unlock it.
- **PAUSE, INPUT and the waits for a frame** do wait, and every other thread waits with them until they end. Keep them out of a hold.

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

## A queue between threads

The producer and consumer above pass one item at a time, and each waits for the other. A queue lets the producer run ahead: it puts messages in, as many as the queue holds, and a consumer takes them out in the same order when it is ready. Any number of threads may put and take.

The queue is an array used as a ring, with the place of the next message to take (`head`), the place for the next one put (`tail`) and the number in it (`count`). Putting a message is several statements, so it is done inside THREAD HOLD. Two flags say when a waiting thread should look again: FLAG(1) that the queue is no longer empty, FLAG(2) that it is no longer full.

```
Const QSIZE = 16
Dim q$(QSIZE - 1)
Dim Integer head, tail, count

Sub Put(m$)                         ' waits while the queue is full
  Thread Hold
  Do While count = QSIZE
    Flag(2) = 0                     ' full: wait for a Get to make room
    Thread Wait Flag 2              ' (lets go of the hold while it waits)
  Loop
  q$(tail) = m$
  tail = (tail + 1) Mod QSIZE
  Inc count
  Flag(1) = 1                       ' not empty: wakes a Get
  Thread Release
End Sub

Sub Get(m$)                         ' waits while the queue is empty
  Thread Hold
  Do While count = 0
    Flag(1) = 0                     ' empty: wait for a Put
    Thread Wait Flag 1
  Loop
  m$ = q$(head)
  head = (head + 1) Mod QSIZE
  Inc count, -1
  Flag(2) = 1                       ' not full: wakes a Put
  Thread Release
End Sub
```

A thread sends with `Put "hello"` and receives with `Get m$`, which waits until there is a message and puts it in `m$`:

```
Sub Producer
  Local Integer i
  For i = 1 To 100
    Put "Reading " + Str$(i)
  Next
  Put ""                            ' an empty message: the end
End Sub

Dim m$
Thread Start 1, Producer
Do
  Get m$
  If m$ = "" Then Exit Do
  Print m$
Loop
```

How it works:

- **The test, the flag and the wait are one step.** Get finds the queue empty, clears FLAG(1) and starts to wait, all inside the hold, so no Put can come in between. THREAD WAIT FLAG then lets go of the hold, and a Put can run.
- **Waking takes the hold again.** When the wait ends, Get holds the threads once more before it looks at the queue, so the statements after the wait are as safe as those before it.
- **A thread that wakes looks again.** With two consumers, both wake when a message is put, and the second finds the queue empty again and goes back to waiting. That is why the wait is in a loop: being woken means "look again", not "there is a message for you".
- **A waiting thread uses no processor time,** and the others run.

Use a different pair of flags for each queue, and keep its arrays and counts for it alone. An interrupt routine cannot wait, so it should not call Put or Get: let it set a flag, and let its thread do the Put (see *Interrupts and Threads*). THREAD WAIT lets go of a hold only: a lock (THREAD LOCK) stays taken while its thread waits, so keep these SUBs to THREAD HOLD.

!NOTE In the 8.00.00b5 firmware first posted on 26 November THREAD WAIT inside THREAD HOLD was an error. For that file, end the hold before the wait and hold again after it, and clear the flag inside the hold in the same step as the test: a flag stays 1 until a thread clears it, so a Put that comes between the release and the wait still wakes the Get.

!NOTE PicoMite: the PicoMite's threads have let go of the hold in THREAD WAIT from the start.

## Waiting for a thread

THREAD WAIT n waits until thread 'n' has ended, at once if it is not running. With a timeout it gives up after that many milliseconds, and MM.INFO(THREAD n) says whether the thread ended. A thread cannot wait for itself.

## The screen and the keyboard

Each thread has its own print position, so two threads printing with PRINT @ or TEXT at different places do not disturb each other, and a thread drawing a clock does not move the line a user is typing at an INPUT. But the screen itself is one: two threads that PRINT without saying where both carry on from where they were and write over each other, and CLS or a scroll moves what is under every thread's position. Give each thread its own part of the screen.

The keyboard is one too. INKEY$, INPUT and KEYDOWN in two threads at once race for the keys, each getting some. Let one thread read the keyboard, and pass what it reads to the others.

## The usual names

For readers who know threads from another language or a real-time operating system, these are the names used there and what does the job here.

{widths: 45,55}
| Elsewhere | In MMBasic |
|---|---|
| Mutex: create, lock (acquire, take), unlock (release, give) | THREAD LOCK k and THREAD UNLOCK k; the sixteen need no creating |
| Try-lock; a lock with a timeout | THREAD LOCK k, 0 and THREAD LOCK k, ms, each after ON ERROR SKIP |
| Recursive mutex | The Enter and Leave SUBs of *A mutex the thread already has* |
| Priority inheritance | Built in: a lock lends priority (*How Threads Run*) |
| Critical section; scheduler lock | THREAD HOLD and THREAD RELEASE |
| Atomic operation | Any single statement |
| Event or semaphore | FLAG(f) with THREAD WAIT FLAG f |
| Condition variable | THREAD WAIT FLAG f inside THREAD HOLD, which lets go of the hold while it waits |
| Message queue; bounded buffer | The Put and Get SUBs of *A queue between threads* |
| Join | THREAD WAIT n |
| Yield; sleep | THREAD YIELD; PAUSE |
| A thread's own storage | LOCAL variables and the SUB's parameters |
| The thread's identifier | MM.INFO(THREAD) |
