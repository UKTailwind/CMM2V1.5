# Colour Maximite 2: MMBasic 8.00.00b5 (beta) release notes

*26 November 2026. Compared with 8.00.00b4, the last version released.*

This is the fifth beta of MMBasic 8.00 for the Colour Maximite 2. Please report problems on the Back Shed forum. 7.00.00RC6 remains the last release of version 7.

The same firmware file runs on the Generation 1, the Generation 2 and the Generation 3.

8.00.00b5 brings:

- **Threads.** A SUB can run alongside the main program, for as long as it likes, with its own loops, LOCAL variables and PAUSEs. THREAD START starts one; up to sixteen run at once, sharing the processor a millisecond at a time. A program can sit in INPUT while threads keep the screen moving, play a tune, read a sensor or wait on the network. There is a new manual for them, the Colour Maximite 2 Threads User Manual.
- **FRAMEBUFFER AFFINE and SPAN** (and PAGE AFFINE and SPAN): a picture turned and zoomed, a floor drawn in perspective, and each row copied between two columns, in one command each. They are the PicoMite's two new commands, so a program that uses them runs on both.
- **A faster web server.** WEB TRANSMIT FILE sends a large file at about 680 KB a second, where it took half a second for each 1.4 KB.
- **The DS3231 clock and the processor's clock set each other** at the start-up, and WEB NTP sets the DS3231.
- **The sound's left and right are the right way round.** Since the first boards the left channel has come out of the right speaker.
- **Fixes:** a board that stopped at RUN after a TLS connection, a band of wrong pixels after MAP SET in the 4-bit modes, errors that were skipped after a web request, BLIT RESIZE a pixel out, the time going to the year 2074 after TIME$ =, and others below.

## Before you update

**From 8.00.00b4:** save the options first and load them again afterwards, as with every update. Writing the firmware erases them:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

The options are kept in the same form as in 8.00.00b4, so a file saved under 8.00.00b4 loads into 8.00.00b5, and one saved under 8.00.00b5 loads into 8.00.00b4. The console runs at 115200 baud until the options are loaded. Save the program in memory to the SD card first: version 8 does not keep it over a reset.

The flash library is kept and works as before, and .lib files and flash slots saved by 8.00.00b4 load: THREAD is a new command, but it was added at the end of the table, so no other token has moved. The loadable programs' services are unchanged (version 22).

**A program with a SUB or a label named `thread`** must give it another name: THREAD is now a command. Variables, arrays and FUNCTIONs of that name are not affected.

**If your speakers are wired to suit the old firmware,** with left and right swapped at the plug or in a cable to make them come out right, undo that: 8.00.00b5 puts the left channel on the socket's tip, as a standard stereo plug expects.

**From 7.00:** read "Before you update" in 8.00.00b1's notes first (they are in Release_Notes_8.00.00b5_full.md): the program held in the flash is lost, and the library and flash slots must be saved again. Everything there applies to 8.00.00b5 too.

**Going back to 8.00.00b4:** write it as you wrote this one, and OPTION DISK LOAD the same options file. A library or flash slot saved under 8.00.00b5 from a program that uses THREAD does not run under 8.00.00b4, which has no such command.

CMM2_Programs_8.00.00b5.zip holds the same programs as 8.00.00b4's zip. Only the SQL sample is new (sql/computers.bas and computers.db: see The programs) and README.txt.

## What is new

### Threads

A thread is an ordinary SUB that runs alongside the main program. The firmware shares the processor between the main program and its threads, a millisecond at a time, and the switch from one to the next always comes between two statements.

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

Threads suit work that goes on by itself: a clock in the corner of the screen, a tune played note by note, a sensor read every second, a game's aliens each moving on its own, a long calculation while the screen stays alive, a network request while the user types. Interrupts remain the tool for a quick answer to an event.

| Command | What it does |
|---|---|
| THREAD START n [SIZE kb] [LOCALS m] [NEST d] [PRIORITY p], sub [, argument ...] | Starts thread n (1 to 16) running the SUB, with its arguments by value |
| THREAD EXIT, THREAD STOP n | Ends the thread that runs it, or thread n |
| THREAD YIELD | Gives up the rest of the time slice |
| THREAD PAUSE n, THREAD RESUME n | Holds thread n, and lets it go on |
| THREAD SLICE ms | The time slice, 1 to 100 ms (1 at RUN) |
| THREAD PRIORITY n, p | Thread n's priority: the time slices in its turn, 1 to 10 |
| THREAD SIZE kb [, locals [, nest]] | The stack, local variables and depth of loops and calls of later threads |
| THREAD LOCK k [, timeout], THREAD UNLOCK k | One thread at a time between them (16 locks): the mutex of other systems |
| THREAD HOLD, THREAD RELEASE | No other thread runs between them |
| THREAD WAIT n [, timeout], THREAD WAIT FLAG f [, timeout] | Waits until thread n has ended, or until FLAG(f) is 1 |
| THREAD LIST | Each thread's line, state and stack |
| MM.INFO(THREAD), MM.INFO(THREAD n), MM.INFO(THREAD LOCK k) | The running thread's number, thread n's state, and the thread that holds lock k |

How they behave:

- **A statement is never split.** INC total in two threads never loses a count, a PRINT prints all its items together, and a LINE or BOX is drawn whole. Two statements that must go together need a lock (THREAD LOCK) or a hold (THREAD HOLD).
- **A thread that waits uses no processor time.** PAUSE, INPUT and LINE INPUT, the waits for a frame (FRAMEBUFFER WAIT, PAGE COPY ...,B, MAP SET and the others), SYNC, TEMPR(), the WEB and BLUETOOTH commands, THREAD LOCK and THREAD WAIT all let the other threads run meanwhile.
- **Each thread has its own** LOCAL variables, FOR and DO loops, SUB and FUNCTION calls, ON ERROR setting, place in the DATA, print position, colours, font and write page. Global variables, the screen, files, the keyboard and everything else belong to the whole program.
- **An interrupt routine runs in the thread that set it up** (SETTICK, SETPIN, ON KEY and the rest), with that thread's colours and print position.
- **The network and Bluetooth each belong to one thread.** The main program may use them first to set up; the first other thread that uses one has it until that thread ends.
- **An error in any thread ends the program** and says where it was ("Error in thread 3, line 40"). After an error or Ctrl-C each thread's line and state is listed.
- **A program without threads runs as before,** and as fast. A switch between threads takes about 5 microseconds.
- **OPTION COMPILE ON and SHADOW** work with threads.
- **Memory:** a thread takes about 26 KB at the default sizes (a 20 KB stack, 48 local variables, loops and calls 20 deep), and THREAD START n SIZE 12 LOCALS 8 NEST 4 makes one of about 14 KB for a simple job.

The **Colour Maximite 2 Threads User Manual** (28 pages, posted with this release) describes them in detail: starting and ending threads, how they take turns, sharing data safely (with the lock as a mutex, how to try one without waiting, a queue of messages between threads, and a table of the names other systems use), interrupts, the network, errors and debugging, memory and limits, and five complete programs. The User Manual has the commands in its reference, an overview in the new Appendix R, and THREAD in HELP.

Threads are new and touch the centre of the interpreter. Reports of programs that behave differently, with or without threads, are especially welcome for this beta.

### FRAMEBUFFER AFFINE and SPAN

```
FRAMEBUFFER AFFINE from, to, u%(), v%(), du%(), dv%() [, step] [, first] [, left] [, width] [, edge] [, transparent]
FRAMEBUFFER SPAN from, to, left%(), right%() [, step] [, first] [, fill] [, transparent] [, outside]
PAGE AFFINE from TO to, ...
PAGE SPAN from TO to, ...
```

The PicoMite's two new raster commands, beside FRAMEBUFFER WARPH and WARPV.

- **AFFINE** reads each row of the destination along a straight line of the source, in 16.16 fixed point. One set of values turns and zooms a whole picture; a set for each row draws a floor in perspective (the "Mode 7" of the games consoles); `left` and `width` make a window that turns a sprite. `edge` and `transparent` are as WARPH's.
- **SPAN** copies, on each row, the columns from `left` to `right` (or, with `outside`, the others), and leaves or fills the rest: a picture seen through a circle, a wipe from one screen to another, a shape cut out.
- **The buffers** are WARPH's: the PicoMite's letters N, L, F, 2 and T in the 4-bit colour depth, N (page 0) and F (the framebuffer) in 8, 12, 16 and 32 bits, and page numbers in the PAGE form. They are refused in the 1-bit depth.
- **More than the PicoMite's:** the source and the destination may be of different sizes, so a framebuffer larger than the screen is a larger map; and the 12, 16 and 32-bit depths.
- **Speed** on a Generation 1: a 320 x 240 screen through AFFINE takes 4.4 ms in 8 bits and 4.9 ms in 4 bits at any angle, a floor 3.0 to 3.3 ms, and SPAN 0.2 to 1.9 ms. An 800 x 600 page takes 17 ms in 8 bits read in order, 100 ms when it is shrunk, and far longer in 12 and 16 bits.

The User Manual has them under FRAMEBUFFER and PAGE, each with an example, and in the chapters Video Modes and Game Features. The PicoMite's warp_demo.bas, with eight scenes now, runs unchanged under OPTION PICOMITE VGA at 59.6 to 59.9 frames a second on a Generation 1.

### The network

- **WEB TRANSMIT FILE is far quicker.** The web server's data was never pushed out after it was written, so each piece waited half a second for a timer: a 40,000-byte file took 13.75 seconds. Large files were also sent one piece for each acknowledgement, which a client holds back. A file of 1,000,000 bytes now takes 1.41 seconds, about 680 KB a second, and arrives identical. A client that takes it slowly gets it all, and one that hangs up is let go after 5 seconds without progress.
- **A large file no longer arrives cut short.** The wait for an acknowledgement gave up after a fraction of a second, so one that was delayed ended the transfer: a 40,000-byte file arrived as 1,556 to 4,380 bytes, and the program then stopped with "The file/directory object is invalid". The wait is now 5 seconds without progress, and a transfer that times out returns to the program.
- **A waiting packet is taken within 1 ms** while a program runs, where it waited for OPTION USB POLLING's tick (2 ms by default). Nothing is added to a statement's time.
- **WEB NTP sets the DS3231** as well as the processor's clock (see the clock, below).
- **WEB TCP INTERRUPT 0** switches the web server's interrupt off, as WEB UDP INTERRUPT 0 does. It was the error "Line number".
- **A Generation 3 V1.2 built without its WiFi module** says "No module on this board: OPTION RADIO NONE" once, sets that option, and starts without looking for the module again. OPTION WIFI or OPTION RADIO turn the search on again.

### The clock

With OPTION DS3231 ON, a board whose processor's clock had lost the time asked for the time and date at every start-up, even when the DS3231 held them, and the answer did not reach the DS3231.

- **At the start-up** a sensible time in the DS3231 goes into the processor's clock, and if only the processor's clock kept the time it goes into the DS3231. The question comes only when neither has a sensible time, and the answer sets both.
- **The question comes before MM.STARTUP and AUTORUN.** It came after them, so not until an AUTORUN program had ended.
- **A DS3231 that lost its power or does not answer** is no longer copied into the processor's clock (it gave 01/01/00, or a time such as 45:85:85).
- **WEB NTP sets the DS3231 too.** Before, the DS3231's old time came back at the next prompt, RUN or hour.

### Fixes

- **A board that stopped at RUN after a TLS connection.** When a server closed a TLS connection before it had sent any data, the firmware kept a pointer to a connection that had been freed, and still showed it as connected. The next WEB CLOSE TCP CLIENT, WEB OPEN or RUN closed freed memory, and if that memory had been used again the board stopped until it was reset. Many servers close an idle connection after about 10 seconds. A WEB TCP CLIENT REQUEST on such a connection now says "No connection".
- **MAP SET in the 4-bit modes** (and the 8-bit modes whose rows are shown four times) showed a band of wrong pixels a few lines high near the top of the screen each time, visible in a program that sets its colours for every frame. The colours are now loaded while the frame is blanked.
- **Errors skipped after a web request.** A request to the web server while a program ran left the program's errors skipped without a message, for the rest of the run, unless the program had used ON ERROR SKIP itself.
- **BLIT RESIZE** in the 4-bit modes showed the edges of blocks a pixel late at scales that are not a power of two.
- **TIME$ = "hh:mm:ss"** after DATE$ = (with OPTION DS3231 ON) set the year to 2074 until the clocks were next copied, and DATE$ = put the time of day back to when the clock was last read (20 seconds back after a PAUSE 20000).
- **WEB TLS CA and WEB TLS NOVERIFY with a TLS connection open** changed the certificates under a connection that was using them. They are now refused ("Close the TLS client connection first", or "Close the MQTT connection first").
- **WEB UDP followed by a word that is neither INTERRUPT nor SEND** did nothing, without a message, so a typing mistake sent nothing. It is now "Invalid syntax".
- **SQL( with a string + in an argument** returned a string, or failed with "Expected a number".
- **LIBRARY LOAD in a program started by CHAIN, FLASH CHAIN or RAM CHAIN** wiped the variables that CHAIN had kept. It is now the error "Invalid in a chained program".
- **MKDIR of a directory that exists** said "Access denied due to prohibited access". It now says "The file or directory already exists", and so does RENAME to a name in use.
- **JSON$() with very deep nesting** could take more stack than the firmware keeps free. It reads 100 levels at most (it was 128); deeper data is "Invalid JSON data".
- **MM.VER** is 8.000005.

## What behaves differently

- **Sound: left and right have changed places,** on every generation and for every kind of sound (PLAY TONE, SOUND, WAV, FLAC, MP3, MOD files and their effects, speech, DAC 1 and DAC 2, DAC START, and the loadable programs' sound). The left channel now comes out of the left speaker. "DAC 1 is the left channel", as the manual has always said, is now true. A Generation 3's HDMI sound was already right and is unchanged.
- **THREAD is a command,** so a SUB or a label named `thread` no longer works, as with any command's name: give it another name. A variable, an array or a FUNCTION named `thread` still works.
- **A recursive FUNCTION can go one or two levels less deep** before "Stack overflow" (of about forty with OPTION COMPILE OFF), and a program that comes within 6 KB of the stack's end, where it was 4 KB, is told so at the prompt. LOAD PNG was found to use more than the 4 KB that was kept free.
- **JSON$()** reads at most 100 levels of nesting (it was 128).
- **LIBRARY LOAD** in a chained program is an error.
- **WEB TCP INTERRUPT 0, WEB TLS CA and NOVERIFY with a connection open, and WEB UDP with an unknown word** each behave as Fixes says. A program with a line such as `WEB UDP SERVER PORT 47999`, which is not a command (the port is set by OPTION UDP SERVER PORT) and did nothing, now stops there with "Invalid syntax".
- **MKDIR and RENAME** onto a name that exists give a different message.
- **With OPTION DS3231 ON** the start-up asks for the time only when both clocks have lost it, and before MM.STARTUP and AUTORUN.
- **A Generation 3 V1.2 without its module** sets OPTION RADIO NONE by itself.
- **The SQL sample's column cpu is named processor** (sql/computers.bas and computers.db), as on the PicoMite, where a line in a TYPE block that begins with a command's name is read as that command.

## Structural changes

For those who build the firmware or study its source:

- **Threads** are Src/Thread.c (in flash bank 2) with hooks in the interpreter: 14.6 KB of flash and 1.4 KB of RAM in all. docs/Threads_Plan.md has the design, what was built at each stage and what was measured.
  - A thread runs on a C stack of its own in the heap. ThreadSwap moves the main stack pointer and the registers a C function must keep. The switch is made only at the statement's tail, when the 1 ms tick has flagged the end of a slice (IntReady.tick) or the thread waits.
  - The interpreter's FOR, DO, GOSUB, SUB, error and DATA stacks, the temporary-string list and the local variables' list are now pointers (forstack, dostack, gosubstack and the rest, each with its `_main` array), re-pointed at a switch. Each thread has its own range of local-variable slots (LocalBase, LocalLimit).
  - The compiler's bind caches get a new event at every switch (BindsStale), so that a compiled statement never uses another thread's variables.
  - PAUSE waits on the microsecond clock; the frame waits are one function (ThreadFrameWait); the network's and Bluetooth's waits yield (NetYield, made by tools/net_yield.py from the PicoMite's files).
  - An interrupt's routine is recorded with the thread that set it up (ThreadIntOwn), and the scan takes a source only if its owner can take it now (IntMay).
  - The stack check reads the running thread's own floor. STACK_GUARD is 6 KB (it was 4 KB).
  - tests/threads has 34 programs with their expected output, and tools/threadcheck.py runs the quick check.
- **AFFINE and SPAN** are in Src/Warp.c beside WARPH and WARPV, entered through the same call (WarpCommand), and share their handling of the buffers, the edge, the transparent value and the GUI cursor. tests/host/affine_host.c compiles the core on a PC against a pixel model.
- **MAP SET** asks the merge's interrupt for the load (Rgb121ClutPending, Rgb121ClutLoad in Src/Rgb121.c). Without a merge MAP SET loads the table itself, as before.
- **The sound's channels** have one definition in Inc/Audio.h (DAC_LEFT is DAC channel 2, DAC_RIGHT channel 1), used by every place that writes the DACs.
- **The web server** (Src/net/picomite/MMtcpserver.c): tcp_output after each write, and tcp_server_stream for large files. The TCP client's tcp_client_err clears its reference for every error, ERR_ABRT included. These two are the Colour Maximite 2's own; the PicoMite has been told of both (its issues #165 and #166).
- **The clock's start-up** is ClockStart, ClockSensible and DS3231_PutClock in Src/MM_Misc.c.
- **The flash:** bank 1 was full. PLAY MODSAMPLE's parser and the table of HELP's topics moved to bank 2, which leaves 5.7 KB free in bank 1, 23 KB in bank 2 and 7 KB in the network's area.
- **Unchanged:** the options record (layout 5), the tokens (image format 7: THREAD is the last command of the table) and the loadable programs' services (version 22).

## The programs

CMM2_Programs_8.00.00b5.zip is 8.00.00b4's zip with the SQL sample of this release and README.txt for this version. No program (.app or .mod) in it has changed since 8.00.00b4, and each runs on 8.00.00b5 as on 8.00.00b4.

- **sql/computers.bas and sql/computers.db:** the column and TYPE member cpu are named processor. A copy of the old computers.bas does not work with the new computers.db, or the other way round: copy both.
- **sql.mod and sql.app** are the files of 8.00.00b4's zip. The only change to their folder of the source since then is the sample.
- **micropython.app** is the same build: its source has had no change since 18 November.

## Tested

**The release file on a Generation 1** (400MHz, with the plug-in WiFi module and OPTION WIFI set: it joined at start-up). The options file saved under the build before it loaded unchanged.

The test suites, the timings and the update from 8.00.00b4 ran on the build just before the release file. The release file adds one fix to threads (an interrupt routine handed to a thread could wait a whole round of time slices, found by the PicoMite's threads port), whose code is in flash bank 2 and runs only while a program has threads. In bank 1 nothing moved; only the offsets of its calls into bank 2 changed. The release file then ran the thread tests and the programs again.

- **The test suites, compared with 8.00.00b4's run on the same board** (24 November):
  - tests/cmm2 (116 programs, two of them new: the AFFINE and SPAN test and a JSON$() nesting test, both passed). 110 gave exactly what they gave under 8.00.00b4. The other four differ as intended: a DAC test now reads the left channel's value from DAC channel 2 (the sound's channels changed places); the recursion test goes 18 levels deep where it went 20 (the larger stack guard); and two print the version or the board's keyboard setting.
  - the PicoMite tests (6 programs, two new: AFFINE and SPAN under OPTION PICOMITE VGA with no wrong pixel, and BLIT RESIZE's steps, which passed). The other four gave the same as before.
  - the crash tests: all passed;
  - the corpus (212 programs): 201 the same. Nine differ only in the times or speeds they print; one found its picture this time (8.00.00b4's run was on a new card); and one has a line `WEB UDP SERVER PORT 47999`, which is not a command and is now "Invalid syntax" (see What behaves differently).
  - the test files (66 programs): 59 the same, seven differing only in the version or times printed;
  - SP Tools: 686 of 727, the same results as under 8.00.00b4 (the rest need a USB flash drive).
- **Threads:** the 35 thread tests (tests/threads, each with its expected output) match with OPTION COMPILE OFF, ON and SHADOW. ttick, which times SETTICK's routine while threads are busy, matched in 12 runs in a row. They cover each command and function, time slices and priorities, PAUSE, INPUT and the frame waits, MAP SET with threads, locks used as the manual's mutex, the queue of the Threads User Manual, interrupts in their own thread, each thread's screen state, the network's owner, errors, END, Ctrl-C and the limits. The queue example of the Threads User Manual, taken from its text, prints its 100 messages in order. Programs that use `thread` as a variable, an array and a FUNCTION ran; a SUB of that name is refused (see What behaves differently).
- **The full regression before threads were merged:** on the threads branch (25 and 26 November), tests/cmm2 and the PicoMite tests with OPTION COMPILE OFF gave the same output as the build before threads were begun, byte for byte, apart from the recursion test and the keyboard setting; ON and SHADOW gave the same as OFF; and the 413 Route B programs gave the same results.
- **Timings,** against 8.00.00b4's on the same board, a statement of each kind with OPTION COMPILE OFF: within 2% (an array assignment 1.8% slower, a FUNCTION call 1.4%, BLIT 1.1%, the rest within 0.7%).
- **The update from 8.00.00b4,** as a user does it: 8.00.00b4 written fresh, a library (using EOF( LOF( INPUT$() loaded and saved to a .lib file, two options set and saved with OPTION DISK SAVE; then 8.00.00b5 written without a full erase, and the options loaded. The library was kept and worked, the .lib file loaded and worked, and the options came back.
- **HELP** finds the THREAD topics and MM.INFO(THREAD LOCK.
- **The programs:** six of the zip's programs, copied to that board's card for 8.00.00b4, each started once from the prompt with no error: Doom (with Freedoom), the Apple //e, the CPC, the C64 and zeptoforth for the G1, which was left by `bye` back to the prompt. Quake refused with "Needs a G2", as it should. The other programs in the zip were not started on 8.00.00b5; they are the files 8.00.00b4's zip had, and the loader and its services have not changed apart from the sound's channels.

**FRAMEBUFFER AFFINE and SPAN** were tested on a PC against a pixel model as well (300,000 random cases, and faults planted in a copy of the code all caught), and the PicoMite's warp_demo.bas ran unchanged at 59.6 to 59.9 frames a second.

**Threads on a Generation 3** (25 and 26 November, builds before the version number changed): the thread tests matched with OPTION COMPILE OFF, and the network's fixes were tested there with WiFi: WEB TCP INTERRUPT 0, WEB TLS CA and NOVERIFY with a TLS client and with MQTT over TLS, WEB UDP, and the TLS connection closed by its server, read through the debugger before and after the fix.

**The release file on a Generation 3** (V1.1, 480MHz, with WiFi), the same file as the Generation 1's: the 35 thread tests match with OPTION COMPILE OFF, ON and SHADOW; tests/cmm2 (116 programs) gave the Generation 3's results, with the 32-bit colour depth's tests passing too; the PicoMite tests and the crash tests passed. Three programs print what this board shows differently, as under 8.00.00b2 and 8.00.00b3 (its full colour levels: a picture's checksum and a red of FF0000 where the others show E00000). The options file saved under 8.00.00b4 loaded unchanged.

The Generation 2 has not run 8.00.00b5: it was not connected.

## Known problems

- **Threads are new.** They have run their 35 tests on a Generation 1 and a Generation 3, and the full regression on a Generation 1, but few programs have been written for them yet. A Generation 2 has not run them.
- **Loadable programs, the C compiler's programs and MicroPython do not use threads,** and a CSUB runs whole, with no switch inside it.
- **A statement that takes a long time holds every thread** until it ends: a SORT of a large array, a COPY of a big file, LOAD PNG, the SD card's and USB drive's transfers.
- **No Generation 3 with the module on its board has been made yet.** Its path has run only in a test build that took a Generation 3 without the module for one with it: the firmware found no chip, said "WiFi module not found" and carried on with every pin free. The same 16-bit transfer code has carried the plug-in module's traffic without an error. If the first boards misread at 30MHz, the bus can be slowed to 15MHz in the next beta.
- **Opening the console's own port as COM3** (or COM1 or COM2 under OPTION CONSOLE PORT) in a program has not been tried on a board since 8.00.00b3's OPTION CONSOLE change. COM3 still opens only when the saved option is OPTION CONSOLE SCREEN or NONE, as before.
- **A large loadable program on a Generation 1 after many BASIC programs.** After a long session the 386 PC and the NES emulator stopped with "Needs n KB, n KB free in one block". In 8.00.00b3's testing that came after about 400 test programs. Each started at once after a restart (CPU RESTART or the reset button). Something those programs used is held until a restart and keeps the loader from joining the display memory to the heap. Which statement does this is not yet known.
- **A MOD file's music can repeat a short piece** during a single statement that runs for longer than 12 ms without letting the MOD player in.
- **FRAMEBUFFER WARPV a column at a time over a whole 800 x 600 page** is slow, and so is AFFINE of a large page in the 12 and 16-bit depths on a Generation 1 (see What is new).
- **Bluetooth game controllers** have not been tried with a real controller yet.
- **Why the radio chip once gave a wrong Bluetooth buffer position** is not known; the firmware refuses it, so it costs at most a packet.
- **Bluetooth and the network wait while a loadable program runs:** a Bluetooth keyboard does not type into it and a speaker plays nothing (MicroPython's network is the exception: it services the network itself).
- **The Bluetooth serial device:** a Bluetooth LE module passes on only part of a long burst at 115200 baud (the module's limit): send in pieces or more slowly.
- **MicroPython on a Generation 1** has about 1.6MB for Python. In one long session two of the book's games (ch19/05 and ch25/02) still run out of memory after the examples before them; each runs after Ctrl-D (a soft reset).
- **The WebMite's own example programs** have not all been tried yet.
- **A Generation 3 can reset when its HDMI cable is unplugged** (the cable's shield meeting the monitor's ground on the reset line). The next board has a fix.
- The known problems of 7.00.00RC6 remain, apart from the three SQL faults fixed in 8.00.00b1.

## The manuals

- **The User Manual** is for 8.00.00b5. New in it: THREAD and MM.INFO(THREAD) in the reference, and Appendix R, an overview of threads; FRAMEBUFFER and PAGE AFFINE and SPAN, each with an example, and in the chapters Video Modes and Game Features; MEMORY and TRACE with threads; WEB TCP INTERRUPT 0, WEB TLS CA and NOVERIFY with a connection open, and WEB NTP with the DS3231; how the two clocks are set at the start-up (DATE$, Appendix K and the Generation 2's clock); a Generation 3 V1.2 without its module; the SQL chapter's example with its column processor; and the Threads User Manual in Appendix O. It was built again from every chapter and reference section, and every difference from 8.00.00b4's is one of these.
- **Colour Maximite 2 Threads User Manual** is new: 28 pages, for firmware 8.00.00b5 and later.
- **Colour Maximite 2 Loadable Programs:** its title page (firmware 8.00.00b5), and 8.00.00b5 in the list of services versions (still 22).
- **The App User Guide and the C Compiler guide** are unchanged apart from the zip's name. The 386 PC Emulator guide and the Stepper reference are unchanged.

## Licences

Unchanged since 8.00.00b4. The firmware's own licence (LICENSE) is unchanged, and threads add no third-party code. For the WiFi and Bluetooth the firmware includes lwIP, Mbed TLS, the cyw43-driver, BTstack, puff and the CYW43439's firmware. **BTstack**'s licence allows redistribution and use "solely for personal benefit and not for any commercial purpose or for monetary gain" (Raspberry Pi's licence for BTstack covers only its own chips): free for the free firmware, but anyone who sells boards with it needs a commercial licence from BlueKitchen. cyw43-driver's copy carries the same condition. Each licence is beside its code in Middlewares/Third_Party, and in the licences folder of the zip.
