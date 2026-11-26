# Colour Maximite 2: MMBasic 8.00 release notes

*The notes of every 8.00 beta, newest first. 8.00.00b1's are compared with 7.00.00RC6, the last release of version 7; each later beta's with the beta before it. Made by tools/v8notes.py from docs/Release_Notes_8.00.00bN.md.*

## 8.00.00b5 (26 November 2026)

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

### Before you update

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

### What is new

#### Threads

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

#### FRAMEBUFFER AFFINE and SPAN

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

#### The network

- **WEB TRANSMIT FILE is far quicker.** The web server's data was never pushed out after it was written, so each piece waited half a second for a timer: a 40,000-byte file took 13.75 seconds. Large files were also sent one piece for each acknowledgement, which a client holds back. A file of 1,000,000 bytes now takes 1.41 seconds, about 680 KB a second, and arrives identical. A client that takes it slowly gets it all, and one that hangs up is let go after 5 seconds without progress.
- **A large file no longer arrives cut short.** The wait for an acknowledgement gave up after a fraction of a second, so one that was delayed ended the transfer: a 40,000-byte file arrived as 1,556 to 4,380 bytes, and the program then stopped with "The file/directory object is invalid". The wait is now 5 seconds without progress, and a transfer that times out returns to the program.
- **A waiting packet is taken within 1 ms** while a program runs, where it waited for OPTION USB POLLING's tick (2 ms by default). Nothing is added to a statement's time.
- **WEB NTP sets the DS3231** as well as the processor's clock (see the clock, below).
- **WEB TCP INTERRUPT 0** switches the web server's interrupt off, as WEB UDP INTERRUPT 0 does. It was the error "Line number".
- **A Generation 3 V1.2 built without its WiFi module** says "No module on this board: OPTION RADIO NONE" once, sets that option, and starts without looking for the module again. OPTION WIFI or OPTION RADIO turn the search on again.

#### The clock

With OPTION DS3231 ON, a board whose processor's clock had lost the time asked for the time and date at every start-up, even when the DS3231 held them, and the answer did not reach the DS3231.

- **At the start-up** a sensible time in the DS3231 goes into the processor's clock, and if only the processor's clock kept the time it goes into the DS3231. The question comes only when neither has a sensible time, and the answer sets both.
- **The question comes before MM.STARTUP and AUTORUN.** It came after them, so not until an AUTORUN program had ended.
- **A DS3231 that lost its power or does not answer** is no longer copied into the processor's clock (it gave 01/01/00, or a time such as 45:85:85).
- **WEB NTP sets the DS3231 too.** Before, the DS3231's old time came back at the next prompt, RUN or hour.

#### Fixes

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

### What behaves differently

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

### Structural changes

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

### The programs

CMM2_Programs_8.00.00b5.zip is 8.00.00b4's zip with the SQL sample of this release and README.txt for this version. No program (.app or .mod) in it has changed since 8.00.00b4, and each runs on 8.00.00b5 as on 8.00.00b4.

- **sql/computers.bas and sql/computers.db:** the column and TYPE member cpu are named processor. A copy of the old computers.bas does not work with the new computers.db, or the other way round: copy both.
- **sql.mod and sql.app** are the files of 8.00.00b4's zip. The only change to their folder of the source since then is the sample.
- **micropython.app** is the same build: its source has had no change since 18 November.

### Tested

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

### Known problems

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

### The manuals

- **The User Manual** is for 8.00.00b5. New in it: THREAD and MM.INFO(THREAD) in the reference, and Appendix R, an overview of threads; FRAMEBUFFER and PAGE AFFINE and SPAN, each with an example, and in the chapters Video Modes and Game Features; MEMORY and TRACE with threads; WEB TCP INTERRUPT 0, WEB TLS CA and NOVERIFY with a connection open, and WEB NTP with the DS3231; how the two clocks are set at the start-up (DATE$, Appendix K and the Generation 2's clock); a Generation 3 V1.2 without its module; the SQL chapter's example with its column processor; and the Threads User Manual in Appendix O. It was built again from every chapter and reference section, and every difference from 8.00.00b4's is one of these.
- **Colour Maximite 2 Threads User Manual** is new: 28 pages, for firmware 8.00.00b5 and later.
- **Colour Maximite 2 Loadable Programs:** its title page (firmware 8.00.00b5), and 8.00.00b5 in the list of services versions (still 22).
- **The App User Guide and the C Compiler guide** are unchanged apart from the zip's name. The 386 PC Emulator guide and the Stepper reference are unchanged.

### Licences

Unchanged since 8.00.00b4. The firmware's own licence (LICENSE) is unchanged, and threads add no third-party code. For the WiFi and Bluetooth the firmware includes lwIP, Mbed TLS, the cyw43-driver, BTstack, puff and the CYW43439's firmware. **BTstack**'s licence allows redistribution and use "solely for personal benefit and not for any commercial purpose or for monetary gain" (Raspberry Pi's licence for BTstack covers only its own chips): free for the free firmware, but anyone who sells boards with it needs a commercial licence from BlueKitchen. cyw43-driver's copy carries the same condition. Each licence is beside its code in Middlewares/Third_Party, and in the licences folder of the zip.

## 8.00.00b4 (24 November 2026)

*24 November 2026. Compared with 8.00.00b3, the last version released.*

*Later on 24 November the release's .bin was replaced by one with PLAY MODSAMPLE's pan, and the User Manual and these notes with it. The new .bin is built from v8-dev 0e75330; the tag v8.00.00b4 stays at the first .bin's source (fbf8bb3), and the next release's tag will hold the pan's code.*

This is the fourth beta of MMBasic 8.00 for the Colour Maximite 2. Please report problems on the Back Shed forum. 7.00.00RC6 remains the last release of version 7.

The same firmware file runs on the Generation 1, the Generation 2 and the Generation 3.

8.00.00b4 brings:

- **EDIT opens a file where you left it.** After F2 runs the program, or after F1 or Esc, EDIT opens the program at the line and column the cursor was on, as the file manager comes back to the program it ran. If the program stops with an error, EDIT still opens at the error's line. This works for EDIT, EDIT "file" and E in the file manager.
- **PLAY MODSAMPLE LOOP, CHANGE, STOP and pan:** a MOD file's samples as continuous sound effects over its music (an engine whose note follows its revs, wind, a tyre's squeal), placed between the speakers, changed while they play (a car crossing from one speaker to the other as it goes by) and stopped when they should stop. Effects are heard sooner after the command.
- **Fixes:** a board switched on with OPTION SD TIMING FAST could say "SD Card not found" until the timing was set to NORMAL and back, and PLAY MODSAMPLE accepted a sample number of 32, which a MOD file does not have.

### Before you update

**From 8.00.00b3:** save the options first and load them again afterwards, as with every update. Writing the firmware erases them:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

The options are kept in the same form as in 8.00.00b3, so a file saved under 8.00.00b3 loads into 8.00.00b4, and one saved under 8.00.00b4 loads into 8.00.00b3. The console runs at 115200 baud until the options are loaded. Save the program in memory to the SD card first: version 8 does not keep it over a reset.

The flash library is kept and works as before, and .lib files and flash slots saved by 8.00.00b3 load, as no tokens have moved. The loadable programs' services are unchanged (version 22).

**From 7.00:** read "Before you update" in 8.00.00b1's notes first (they are in Release_Notes_8.00.00b4_full.md): the program held in the flash is lost, and the library and flash slots must be saved again. Everything there applies to 8.00.00b4 too.

**Going back to 8.00.00b3:** write it as you wrote this one, and OPTION DISK LOAD the same options file.

CMM2_Programs_8.00.00b4.zip holds the same programs as 8.00.00b3's zip, unchanged: only its README.txt is new. If your card has 8.00.00b3's programs (with the scummvm.app of 23 November), there is nothing to copy.

### What is new

#### EDIT opens a file where you left it

A Back Shed user's request: change a line, press F2 to run the program, and EDIT again should open at that line, not at the top, as the file manager comes back to the program it ran.

- **On leaving the editor,** by F1, F2 or Esc, it remembers the file, the line and the column. Until now only a save did, and only EDIT with no file name used it.
- **EDIT, EDIT "file" and E in the file manager** open that file at that place. A file named without its drive, or in other capitals, is the same file.
- **An error still wins:** if the program stops with an error after that, EDIT opens at the line of the error, as before. Whichever came last decides. The file manager's own return to an error's line is unchanged.
- **Only the last file is remembered:** any other file opens at its top. So after RUN "other", EDIT now opens that program at its top, where it used to open at the line remembered from the first one.

The PicoMite keeps the place in its own way, as its editor works on the program in its flash: this is the Colour Maximite 2's. The User Manual says so in EDIT and in the chapter Full Screen Editor.

#### PLAY MODSAMPLE LOOP, CHANGE, STOP and pan

```
PLAY MODSAMPLE sampleno, channel [, volume] [, samplerate] [, pan] [, LOOP]
PLAY MODSAMPLE CHANGE channel [, volume] [, samplerate] [, pan]
PLAY MODSAMPLE STOP [channel]
```

A Back Shed user's racing game needs sounds that run on over the MOD music (an engine whose revs change, a tyre's squeal, gravel) and effects heard close to the frame that asks for them. Until now an effect played once from its start each time, could not be stopped, and changing its rate started it again.

- **LOOP** plays the sample until it is stopped: to the end of the loop the sample has in the MOD file and back to the loop's start, as the MOD player plays it, or, if it has no loop, the whole sample over and over. LOOP can end the list after any argument from the channel on: PLAY MODSAMPLE 5, 1, LOOP plays sample 5 on channel 1 at the full volume and 16000.
- **CHANGE** changes the volume, the rate or the pan of the sample playing on a channel, or any of them, where it is, without starting it again, so that an engine's sound can follow its revs. An empty or missing argument is left as it is, and a channel that is not playing is left alone.
- **pan** places the sample between the speakers: -100 is the left speaker only, 100 the right only, and 0 (the default) both at the full volume. Between, the far speaker is turned down in proportion: at -50 the sample plays at its full volume on the left and at half on the right. With CHANGE a car can cross from one speaker to the other as it goes by.
- **STOP** stops one channel at once, or all four if no channel is given. It is not an error when no MOD file is playing.
- **Sooner:** a MOD file's sound is now made 512 samples at a time, not 2048, so an effect starts, changes or stops 12 to 23 ms after the command at 44100 samples a second (it was 46 to 93 ms). See What behaves differently for what this costs.
- **The sample's number** is 1 to 31, the samples a MOD file has (see Fixes).

The User Manual has them in PLAY MODSAMPLE, and PLAY MODFILE's entry says how the music is made. The PicoMite's 8.00.00b2 has the same, the pan included, so a program that uses them runs on both.

#### Fixes

- **OPTION SD TIMING FAST at power up.** A board switched on with OPTION SD TIMING FAST could say "SD Card not found" until the timing was set to NORMAL and back to FAST. It was seen with 8.00.00b3 on a Generation 3 and on a Generation 1. At FAST the card's data line from the card had no pull-up, and a card that has just been powered does not drive it until it is told to change to SPI, so the firmware waited for a card that looked busy and never sent it the first command. NORMAL pulls the line up, which is why it worked. At FAST the card's start is now made as NORMAL makes it, at the slow speed, and the SPI hardware takes over at the full speed once the card is ready. The first command is also tried again, up to 10 times in 2 seconds, with any transfer a reset left running stopped in between.
- **PLAY MODSAMPLE 32** was accepted, and played whatever lay after the MOD player's 31 samples: its length and data came from other memory. It is now refused ("32 is invalid (valid is 1 to 31)"), and the manual says 1 to 31.
- **MM.VER** is 8.000004.

### What behaves differently

- **EDIT "file" and E in the file manager** open the file where it was left, if it is the last file edited or stopped by an error, not always at the top. Any other file opens at its top, as before.
- **A MOD file's music** is made in pieces of 512 samples (12 ms at 44100), between the program's statements. A single statement that runs for longer than that without letting the MOD player in can make the music repeat a short piece: on a Generation 1, CLS of an 800 x 600 page in the 12-bit colour depth (37 ms) or a SORT of a large array. Most long commands (BLIT, PAGE COPY, PAUSE, file access) let it in as they run. The pieces were 2048 samples (46 ms), which hid this. Peter chose the shorter pieces for the effects' sake.
- **PLAY MODSAMPLE** with a sample number of 32 is an error.
- **PLAY MODSAMPLE's effects are mixed after the MOD music's stereo spread,** so that a pan is exact (mixed before it, a sample panned to one side was heard at half its level in the other speaker). A centred effect is now as loud in both speakers: 1.5 times its sample, as the left speaker had it, where the right speaker had 1.75 times (about 1.3 dB more).

### Structural changes

For those who build the firmware or study its source:

- **The editor's place** (Src/Editor.c): EdNoteStart records the file (its full path, with the drive: DrivePath), the line and the column in StartEditFile, StartEditLine and StartEditCharacter. The editor calls it on every way out, and error() (Src/MMBasic.c) calls it for an error in the program's own lines in place of setting the line itself. EdStartFor gives edit() the place for a file if it is that file (strcasecmp of the full paths), otherwise the top; the file manager's pending error seek is checked first.
- **PLAY MODSAMPLE** (Src/Audio.c, Src/hxcmod.c): ModSampleCommand and the MOD player's start, change and stop of an effect. The effect's step is worked out when it starts and at CHANGE, not divided out for each sample made. The pan gives each effect a volume for each speaker (lvol and rvol, set by hxcmod_seffectbalance); hxcmod_fillbuffer sums the effects in el and er, passes them through the MOD player's filter with their own history (last_el_sample and last_er_sample in the modcontext), and adds them after the music's stereo separation, made 1.5 times as loud there. MOD_FRAMES (512) is the size of each of a MOD file's two buffers.
- **The SD card's start at FAST** (Src/mmc_stm32.c): sd_start_bitbang and sd_end_bitbang make SPI3's pins plain I/O with MISO pulled up for the start and give them back to SPI3, as they were, at FCLK_FAST; sd_go_idle tries CMD0 up to 10 times, with sd_stop_transfer (CMD12, the Stop Tran token and 80 clocks) between.
- **The code's place:** the new code is in the .extra section, called from the commands, so the code run for every statement moved by a few bytes. Against 8.00.00b3 on a Generation 1 the statements' timings are within 3%, PIXEL within 0.5% and BOX within 1.5% in six modes. BLIT WRITE is 1 to 5.5% slower (0.1 to 0.4 us), though none of its code changed: the code after PLAY's moved, as PLAY's own code is now smaller.
- **Unchanged:** the options record (layout 5), the tokens (image format 7) and the loadable programs' services (version 22). LOOP, CHANGE and STOP are words of PLAY MODSAMPLE, not new tokens.

### The programs

CMM2_Programs_8.00.00b4.zip is 8.00.00b3's zip with README.txt for this version. When it was published no program in it had changed since 8.00.00b3 (tools/zip_apps_check.py: none to review; micropython.app is the same build), and each runs on 8.00.00b4 as on 8.00.00b3. It has the new quest/scummvm.app of 23 November (see 8.00.00b3's notes).

### Tested

**The release file on a Generation 1** (400MHz, with the plug-in WiFi module and OPTION WIFI set: it joined at start-up). The options file saved under the build before it loaded unchanged.

The first .bin (without the pan) ran the test suites, EDIT's checks and the programs. The .bin with the pan, which replaced it, ran PLAY MODSAMPLE's tests, the pan's own, tests/cmm2 and the timings; nothing else changed between them.

- **The test suites, compared with 8.00.00b3's run on the same board** (22 November, before WARPH and WARPV were added). These are:
  - tests/cmm2 (114 programs);
  - the PicoMite tests (4 programs);
  - the crash tests: all passed;
  - the corpus (212 programs);
  - the test files (66 programs);
  - SP Tools: 686 of 727, the same results as under 8.00.00b3 (the rest need a USB flash drive).

  Every program gave what it gave under 8.00.00b3, apart from five, each explained:
  - the two WARPH and WARPV tests, new since that run, matched;
  - two tests that differed under 8.00.00b3 now match: they expect the 640 x 480 default mode the board has now, and 8.00.00b3's run was made in MODE 1 (800 x 600);
  - one corpus program could not find the picture that a later program in the run writes: this card is not the one 8.00.00b3 was tested with, so the picture was not on it yet. Run again once the picture was there, it gave no error.

  With the pan, tests/cmm2 gave the same output as with the first .bin, byte for byte, for all 114 programs.
- **EDIT:** 10 checks by a script at the console, each reading where the editor's cursor opened:
  - a file opens at its top the first time;
  - after F1, after F2 (the program ran to its end) and after Esc, EDIT opens it where the cursor was;
  - EDIT "file" with the name written without its drive and in other capitals opens at the same place;
  - after an error, EDIT opens at the error's line, and so does EDIT "name" (relative, without .bas) after RUN stops at the error again;
  - after another program runs with no error, EDIT opens that program at its top, and the first file, opened again, opens at its top.
- **PLAY MODSAMPLE,** quietly, over a MOD file: a sample played once; with LOOP it was still playing after three times its length; STOP 1 stopped it at once and left channel 2 playing; CHANGE to twice the rate halfway through made the rest take half as long, without starting again; CHANGE of the volume alone kept the rate; each form's errors were as the manual says. Sample 32 is refused. With the pan, one form with too many arguments (PLAY MODSAMPLE 1, 1, 64, 16000, LOOP, 1) now says "Argument count" where it said "Syntax"; the rest is the same.
- **The pan:**
  - its forms and errors, 20 of them at the start and in CHANGE: -101 and 101 refused, an empty pan kept, LOOP only at the end;
  - the doppler demo's engine (its MOD's music blanked) measured in the MOD player's output at pans -100, -50, 0, 50, 100, -30 and 75: the far speaker exactly silent at -100 and 100, both the same at 0, the far side at 0.49 of the near at 50 and -50, and as the balance gives it at the others; a CHANGE of the volume or the rate alone kept the pan;
  - the effects go through the MOD player's filter, as before: at 16000, 31 to 32% of the output's samples equal the one before, where unfiltered steps give about 64%;
  - DAC1's two outputs read through the ST-Link while the engine played full left and full right, at the start and by CHANGE: the far channel stayed at silence for each whole 4 seconds;
  - the doppler demo (an engine and a horn passing at 20, 40 and 60 m/s) ran to its end.
- **Timings,** against 8.00.00b3's on the same board: see The code's place in Structural changes.
- **The programs:** that board's card held none of the zip's programs, so six were copied to it from the zip and each started once from the prompt, with no error: Doom (with Freedoom), the Apple //e, the CPC, the C64 and zeptoforth for the G1, which was left by `bye` back to the prompt. Quake refused with "Needs a G2", as it should. The other programs in the zip were not started on 8.00.00b4. They are the same files that 8.00.00b3 started on a Generation 1, and the loader and its services have not changed since.

**The SD card's start at FAST,** in builds of the same code before the version number changed:
- **A Generation 1** that said "SD Card not found" when switched on at FAST under 8.00.00b3 found its card when switched on with the card in. 10 resets during 2MB reads each recovered, and SPI3's pins were back as they were after the start.
- **A Generation 3** (V1.1) found its card when switched on with the card in, and when a card was put in while it was on. 10 resets during reads each recovered.

**PLAY MODSAMPLE on a Generation 3** (23 November, a build before the editor change and the SD fix): the same test gave the same results through the HDMI sound.

The editor change ran only on the Generation 1. The Generation 2 and the Generation 3 have not run the release file itself: only the Generation 1 was connected.

### Known problems

- **No Generation 3 with the module on its board has been made yet.** Its path has run only in a test build that took a Generation 3 without the module for one with it: the firmware found no chip, said "WiFi module not found" and carried on with every pin free. The same 16-bit transfer code has carried the plug-in module's traffic without an error. If the first boards misread at 30MHz, the bus can be slowed to 15MHz in the next beta.
- **Opening the console's own port as COM3** (or COM1 or COM2 under OPTION CONSOLE PORT) in a program has not been tried on a board since 8.00.00b3's OPTION CONSOLE change. COM3 still opens only when the saved option is OPTION CONSOLE SCREEN or NONE, as before.
- **A large loadable program on a Generation 1 after many BASIC programs.** After a long session the 386 PC and the NES emulator stopped with "Needs n KB, n KB free in one block". In 8.00.00b3's testing that came after about 400 test programs. Each started at once after a restart (CPU RESTART or the reset button). Something those programs used is held until a restart and keeps the loader from joining the display memory to the heap. Which statement does this is not yet known.
- **A MOD file's music can repeat a short piece** during a single statement that runs for longer than 12 ms without letting the MOD player in (see What behaves differently).
- **FRAMEBUFFER WARPV a column at a time over a whole 800 x 600 page** is slow. With offsets that differ from column to column it took 108 ms in the 8-bit colour depth, 253 ms in 16 bits and 594 ms in 12 bits on a Generation 1. Bands of four columns (`step` 4) took 20, 39 and 70 ms, and WARPH over the same page 4, 26 and 53 ms.
- **Bluetooth game controllers** have not been tried with a real controller yet.
- **Why the radio chip once gave a wrong Bluetooth buffer position** is not known; the firmware refuses it, so it costs at most a packet.
- **Bluetooth and the network wait while a loadable program runs:** a Bluetooth keyboard does not type into it and a speaker plays nothing (MicroPython's network is the exception: it services the network itself).
- **The Bluetooth serial device:** a Bluetooth LE module passes on only part of a long burst at 115200 baud (the module's limit): send in pieces or more slowly.
- **MicroPython on a Generation 1** has about 1.6MB for Python. In one long session two of the book's games (ch19/05 and ch25/02) still run out of memory after the examples before them; each runs after Ctrl-D (a soft reset).
- **The WebMite's own example programs** have not all been tried yet.
- **A Generation 3 can reset when its HDMI cable is unplugged** (the cable's shield meeting the monitor's ground on the reset line). The next board has a fix.
- The known problems of 7.00.00RC6 remain, apart from the three SQL faults fixed in 8.00.00b1.

### The manuals

- **The User Manual** is for 8.00.00b4: EDIT and the chapter Full Screen Editor (where the editor opens a file), PLAY MODSAMPLE (LOOP, CHANGE, STOP, the pan and the samples 1 to 31), PLAY MODFILE (the music made 512 samples at a time) and the chapter Sound (PLAY MODSAMPLE in its list). It was built again from every chapter and reference section, and nothing else in it changed.
- **Colour Maximite 2 Loadable Programs:** its title page (firmware 8.00.00b4), and 8.00.00b4 in the list of services versions (still 22).
- **The App User Guide and the C Compiler guide** are unchanged apart from the zip's name. The 386 PC Emulator guide and the Stepper reference are unchanged.

### Licences

Unchanged since 8.00.00b3. The firmware's own licence (LICENSE) is unchanged. For the WiFi and Bluetooth it includes lwIP, Mbed TLS, the cyw43-driver, BTstack, puff and the CYW43439's firmware. **BTstack**'s licence allows redistribution and use "solely for personal benefit and not for any commercial purpose or for monetary gain" (Raspberry Pi's licence for BTstack covers only its own chips): free for the free firmware, but anyone who sells boards with it needs a commercial licence from BlueKitchen. cyw43-driver's copy carries the same condition. Each licence is beside its code in Middlewares/Third_Party, and in the licences folder of the zip.

## 8.00.00b3 (22 November 2026)

*22 November 2026. Updated on 23 November with FRAMEBUFFER WARPH and WARPV. Compared with 8.00.00b2, the last version released.*

This is the third beta of MMBasic 8.00 for the Colour Maximite 2. Please report problems on the Back Shed forum. 7.00.00RC6 remains the last release of version 7.

The same firmware file runs on the Generation 1, the Generation 2 and the Generation 3.

8.00.00b3 brings:

- **The Generation 3's WiFi and Bluetooth module on the board.** The Generation 3 carries the module on its board, on a bus of its own, so nothing is plugged into the rear I/O connector: with the WiFi and Bluetooth on, pins 32, 33, 38 and 40 and SPI2 stay free for other uses. The firmware finds the module by itself. On a Generation 1 or 2 the plug-in module works as before.
- **SQL(TYPE n)**, which says what SQLite holds in a column of the row: an integer, a floating point number, text, a BLOB or no value.
- **FRAMEBUFFER WARPH and WARPV** (added on 23 November): each row or column of one buffer is copied to another, moved by its own offset from an integer array. One command makes racing roads, parallax, water that wobbles and screen melts, what the arcade hardware called row scroll and column scroll. PAGE WARPH and WARPV do the same with page numbers. FRAMEBUFFER WAIT now works in every colour depth. The PicoMite's 8.00.00b1 has the same commands.
- **Fixes:** text arriving on the serial console was lost or garbled while a program switched OPTION CONSOLE; a key typed under OPTION CONSOLE NONE was stored in the wrong memory; the prompt's history recall drew and edited each recalled line a column to the left on a serial terminal exactly as wide as the screen; and under OPTION STATUS ON the file manager's prompts were hidden on the screen and the status line said "Check Disk" for a program that was on the card.

### Before you update

**From 8.00.00b2:** save the options first and load them again afterwards, as with every update. Writing the firmware erases them:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

The options are kept in the same form as in 8.00.00b2, so a file saved under 8.00.00b2 loads into 8.00.00b3, and one saved under 8.00.00b3 loads into 8.00.00b2. The console runs at 115200 baud until the options are loaded. Save the program in memory to the SD card first: version 8 does not keep it over a reset.

The flash library is kept and works as before, and .lib files and flash slots saved by 8.00.00b2 load, as no tokens have moved. The loadable programs' services are unchanged (version 22).

**From 7.00:** read "Before you update" in 8.00.00b1's notes first (they are in Release_Notes_8.00.00b3_full.md): the program held in the flash is lost, and the library and flash slots must be saved again. Everything there applies to 8.00.00b3 too.

**Going back to 8.00.00b2:** write it as you wrote this one, and OPTION DISK LOAD the same options file.

CMM2_Programs_8.00.00b3.zip holds the same programs as 8.00.00b2's zip, unchanged: only its README.txt is new. If your card has 8.00.00b2's programs, there is nothing to copy.

**A Generation 3 with the module on its board** needs 8.00.00b3 or later for the WiFi and Bluetooth. 8.00.00b2 and earlier run it as a Generation 3 without them: they look for the plug-in module on the rear connector, and with OPTION WIFI set they print "WiFi module not found".

### What is new

#### The Generation 3's WiFi and Bluetooth module on the board

The Generation 3 has the WiFi and Bluetooth module, a Raspberry Pi RM2, on its board. It is joined to the processor by a bus of its own (the processor's SPI6), not by the rear I/O connector:

- **Nothing is plugged in**, and the module uses none of the connector's pins. With OPTION WIFI or OPTION RADIO set, pins 32, 33, 38 and 40 (and GP10 and GP11, their PicoMite names) stay free for SETPIN, and SPI2 can be opened alongside the WiFi.
- **Everything else is as with the plug-in module**: OPTION WIFI, OPTION RADIO, the WEB and BLUETOOTH commands, the 128KB the network keeps and the 64KB Bluetooth keeps, and MM.DEVICE$ ("Colour Maximite 2 G3").
- **The link to the chip** runs at up to 30MHz (the plug-in module's at 24MHz on a Generation 2 and 20MHz on a Generation 1).
- **The firmware tells the boards apart** at power up by a pin the Generation 3 connects to ground, so the same file serves every board: a Generation 1 or 2, with or without the plug-in module, works exactly as before.

The User Manual says so in Hardware Features, WiFi and Internet Functions (The WiFi Module), Bluetooth, Porting Programs, Appendix D (SPI) and Appendix M, and in the entries for OPTION WIFI and SPI2.

#### SQL(TYPE n)

SQL(TYPE n), beside SQL(ISNULL n), returns what SQLite holds in column n of the current row: 1 an integer, 2 a floating point number, 3 text, 4 a BLOB, 5 no value (SQLite's own numbers, from sqlite3_column_type()). SQL(n) gives a BLOB as a string, as it gives text, and a column read into a float cannot tell 2.0 from 2, so a program that writes the data out again, as a dump does, can now keep each value's type (Matthias's wish, for mmsql). It uses what sql.mod already gives the firmware, so the zip's sql.mod is unchanged.

#### FRAMEBUFFER WARPH and WARPV (added on 23 November)

```
FRAMEBUFFER WARPH from, to, xoff%() [, step] [, first] [, phase] [, edge] [, transparent] [, rows%()]
FRAMEBUFFER WARPV from, to, yoff%() [, step] [, first] [, phase] [, edge] [, transparent]
```

Each row (WARPH) or column (WARPV) of the buffer `from` is copied to the buffer `to`, moved by its own offset from an integer array: a positive offset moves a row to the right, or a column up, as PAGE SCROLL does. The old games made these effects by changing the scroll position on chosen lines (the arcade hardware's *row scroll*, or *line scroll*, and *column scroll*): the curves and hills of a racing road, parallax layers that move at different speeds, water that wobbles and a screen that melts away. An array operation such as MATH SCALE makes a frame's offsets, so there is no BASIC loop over the lines. They are Peter's design for the PicoMite, whose 8.00.00b1 has the same commands, and a program that uses them runs on both.

- **Bands:** each element of the array moves a band of `step` rows or columns (default 1), from the row or column `first` (default 0). Rows or columns outside the bands are left as they are, so a score line can stay still.
- **phase** rotates the element each band takes ((band + phase) MOD the number of elements), so a table holding a wave is animated by counting `phase` up.
- **edge:** if it is omitted the source wraps round, so any offset can be given (a camera position that keeps growing, for example). -1 leaves the destination's pixel as it is, and a colour fills it.
- **transparent** is a pixel that is not copied, so layers moved by different offsets can be stacked in one buffer. In the 12-bit colour depth the pixels with an alpha of 0 are not copied, whatever colour is given (give 0).
- **rows%()** (WARPH only) picks the source row shown on each band: a hill, or the colour of a road's kerbs, for each line.
- **The buffers:** in the 8, 12, 16 and 32-bit colour depths `from` and `to` are N (the display, page 0) and F (the framebuffer of FRAMEBUFFER CREATE). They can be of different sizes: the offsets wrap at the source's size, so WARPH from a framebuffer wider than the screen is a FRAMEBUFFER WINDOW for each line. In the 4-bit depth (the PicoMite's VGA and HDMI) they take the PicoMite's letters N, L, F, 2 and T.
- **PAGE WARPH and PAGE WARPV** do the same with page numbers (`PAGE WARPH 1 TO 0, off%()`), in every colour depth but 1 bit. The 1-bit depth does not have WARPH and WARPV ("Invalid for this screen mode").
- **FRAMEBUFFER WAIT,** which waits for the next frame blanking, now works in the 8, 12, 16 and 32-bit colour depths, as it did in the 4 and 1-bit ones. Use it before writing to the display, so that the picture is whole when it is shown.

The User Manual has them in the FRAMEBUFFER and PAGE entries, with an example, and in the chapters Video Modes and Pages and Game Playing Features. The PicoMite's warp_demo.bas runs unchanged under OPTION PICOMITE VGA at 59.5 to 59.9 frames a second on a Generation 1.

#### Fixes

- **OPTION CONSOLE in a program.** OPTION CONSOLE SCREEN stopped the serial console's port, and BOTH or SERIAL set it up again, so text arriving while a program switched between them was lost or came in garbled (Matthias: mmsql switches while reading pasted text). The original Colour Maximite 2 firmware did this to free the port for OPEN "COM3:". OPTION CONSOLE now only chooses where the output goes, as on the PicoMite: the port keeps running, and nothing that arrives is lost however often a program switches. A program can still open the console's port as COM3 (or COM1 or COM2 under OPTION CONSOLE PORT) as before: the console gives the port up while it is open.
- **OPTION CONSOLE NONE:** a key typed on the serial console was stored through COM3's receive buffer, which does not exist unless COM3 is open, so it was written to the wrong memory. It now reaches the program, as under the other settings.
- **The prompt's history recall** (the Up and Down keys): on a serial terminal exactly as wide as the screen (Tera Term at 100 columns for MODE 1, for example) each recalled line, from the second on, was drawn over the space after the prompt (">option list") and edits landed one place to the left (Matthias). The VGA screen was right. In the Colour Maximite 2's firmware from the start: a terminal one column wider, which EDIT, FM and Shift-F6 ask for, hid it. Also, insert mode could push a line past the screen's right edge.
- **The file manager's prompts under OPTION STATUS ON:** while FM waited for a name (to rename, duplicate, go to a path, filter, or make a new file or directory), the screen's bottom row showed the prompt's status line (the program's name and the time) in place of the question, redrawn every second. The serial terminal was right. In the firmware since 7.00.00b4 (Peter).
- **The status line under OPTION STATUS ON** said "Wrong Disk" and then "Check Disk" after a program was loaded, although its file was on the card: the check looked for the name with a stray character in front of it. Since 7.00.00b4, when a program's name began to keep its drive.
- **MM.VER** is 8.000003.

### What behaves differently

- **OPTION CONSOLE SCREEN** no longer stops the serial console's input: keys typed on the serial console reach a program under OPTION CONSOLE SCREEN, CTRL-C included, as on the PicoMite. "Console port in use" is now the error of OPTION CONSOLE BOTH or SERIAL while the console's port is open as a COM port (COM1, COM2 or COM3).
- **On a Generation 3** the WiFi's and Bluetooth's pins and SPI2 are free (above).

### Structural changes

For those who build the firmware or study its source:

- **Which board:** getVersion (Src/main.c) reads PF6 as well as PF9 at power up. PF9 to ground is a Generation 2 or 3 (as before); PF6 to ground as well is a Generation 3 with the module on the board (RM2OnBoard). It now leaves both pins as analog inputs with no pull; it had set port E there by mistake, so PF9 kept its pull-up.
- **The module's bus on a Generation 3** (Src/net/cyw43_bus_cmm2.c): SPI6, with the clock on PG13 and the data, in both directions and with the chip's interrupt, on PG14 (both alternate function 5), chip select PF7 and power on PF8, all on the board. SPI6 is in the D3 domain, clocked from its bus clock (120MHz at 480MHz) divided by 4. It is a limited SPI (16-bit frames at most, an 8-byte FIFO), so each 32-bit word of the chip's protocol goes as two 16-bit frames, and a long transfer in parts of at most 511 words. The plug-in module's path (SPI2, 32-bit frames) is unchanged.
- **The pins:** cyw43_driver_cmm2.c has the board's pins beside the plug-in module's; Src/net/WebMite_cmm2.c reserves the connector's four pins and SPI2 only for the plug-in module.
- **The console's port** (Src/Serial.c): ConsoleUartUp says whether the console has its UART; a program's OPEN of that port hands it over (ConsoleHandOver) and ConsolePortOpen gives "Console port in use". The interrupt handlers of USART1, USART2 and UART4 (Src/stm32h7xx_it.c) give a byte to whoever has the port: the console, else the COM port if it is open, else it is read and dropped.
- **WARPH and WARPV** (Src/Warp.c): the PicoMite's core (its graphics/Warp.c), with the Colour Maximite 2's additions: 32-bit pixels, the 12-bit depth's alpha, pages that hold each row twice, a source and destination of different sizes, and the routine checks every 16 rows. It is all in the .extra section, entered through one call, WarpCommand, from cmd_framebuffer (Src/Draw.c), Rgb121Framebuffer (Src/Rgb121.c) and cmd_page, so the code run for every statement moved by 64 bytes and the statements' timings are as before. The tests are tests/cmm2/warpn.bas and tests/picomite/warp.bas.
- **Unchanged:** the options record (layout 5), the tokens (image format 7) and the loadable programs' services (version 22). WARPH and WARPV are words of the FRAMEBUFFER and PAGE commands, not new tokens.
- docs/G3_V1.2_Header_Plan.md records the board's connections, the antenna's keep-out and the firmware for the module on the board.

### The programs

CMM2_Programs_8.00.00b3.zip is 8.00.00b2's zip with README.txt for this version. When it was published no program in it had changed since 8.00.00b2 (tools/zip_apps_check.py: none to review; micropython.app is the same build), and each runs on 8.00.00b3 as on 8.00.00b2.

**quest/scummvm.app was replaced on 23 November.** In the games that speak, the first one read each line of speech from the SD card as it played, in the interrupt that makes the sound: Day of the Tentacle, Sam & Max Hit the Road, Indiana Jones and the Fate of Atlantis on CD, Full Throttle, and the CD versions of The Legend of Kyrandia 1 and 2. A line that played while the game loaded a room or saved could have read the wrong data, or damaged a file on the card; no one is known to have seen it happen. The new one reads each line into memory before it plays it: on a Generation 1 Day of the Tentacle played for two minutes with it, each line read in 111 ms at most. Copy the new A:/quest/scummvm.app from the zip if you downloaded it before then. 7.00.00RC6's zip has the same new file.

### Tested

**The release file on a Generation 1** (400MHz, with the plug-in WiFi module and OPTION WIFI set). The options file saved under 8.00.00b2 loaded unchanged.

- **The test suites, compared with 8.00.00b2's run on the same board.** These are:
  - tests/cmm2 (113 programs);
  - the PicoMite tests;
  - the crash tests: all passed;
  - the corpus (212 programs);
  - the test files (66 programs);
  - SP Tools: 686 of 727, as under 8.00.00b2 (the rest need a USB flash drive).

  Every program gave what it gave under 8.00.00b2, apart from four, each explained:
  - one test now shows pins 32, 33, 38 and 40 reserved, and two network tests now run: the WiFi module was plugged in after 8.00.00b2's run;
  - one test that switches OPTION CONSOLE now matches: under 8.00.00b2 it lost a character to the switching ("loop 1" came out as "loopð1"), the fault fixed here.
- **The WiFi:** it joined at start-up. 100 pings of 1400 bytes from a PC came back with 4 lost (WiFi). A 1MB file sent by TFTP and read back was identical. While the WiFi has them, pins 32, 33, 38 and 40 are reserved and SPI2 OPEN is refused. The network tests gave the same output, byte for byte, as on 22 November with the same code before the version number changed.
- **The fixes:**
  - **OPTION CONSOLE:** 200 characters pasted while a program switched between OPTION CONSOLE SCREEN and BOTH about 7,450 times arrived identical.
  - **OPTION CONSOLE NONE:** 200 characters typed arrived identical.
  - **History recall:** recalled lines were drawn in the right place with the cursor where MMBasic expects it, on a terminal 100 columns wide (the screen's width) and 101.
- **SQL(TYPE n):** it gave 3, 4, 1, 2, 3, 1 for text, a BLOB, 2, 2.0, '12' and 12, matching SQLite's typeof().
- **The programs:** each program in the zip that runs on a Generation 1 started from the prompt with no error, and the prompt came back when it was left. These are MicroPython, Doom, ScummVM, the 386 PC, the NES, Heretic, the Spectrum, the CPC, the C64, the Atari 800, the TRS-80, the BBC Micro, the VT100 terminal, zeptoforth for the G1, WordGrinder, sc-im and SQL's example. The Generation 2 programs (zeptoforth, Quake and the Amiga) refused with "Needs a G2", as they should. The Apple //e and the C compiler were not on that board's card.

**The same changes on a Generation 3** (V1.1, with the plug-in WiFi module), in builds of the same code before the version number changed:
- **21 November:** the OPTION CONSOLE paste test (200 characters over about 8,700 switches, twice), OPTION CONSOLE NONE, the history recall at 100 and 101 columns, and SQL(TYPE n).
- **22 November:**
  - The new 16-bit transfer code carried the plug-in module's traffic in short parts. It joined, 100 of 100 pings came back, a 1MB TFTP round trip was identical, and there were no bus errors.
  - A build that treated this board as one with the module on the board found no chip, said "WiFi module not found" within 2.2 seconds, and left every pin and SPI2 free.
  - The final code ran the plug-in module as before.

The Generation 2 and the Generation 3 have not run the release file itself: only one board could be connected.

**The release file was replaced on 22 November** with the two OPTION STATUS fixes above, an hour after it was first published. They change two lines, and no other code moved. The tests above ran on the first file. The new file runs on the same Generation 1 with its options loaded back unchanged. Under OPTION STATUS ON, FM's rename prompt there reads "Rename occom3.bas to: occom3.bas", and after RUN the status line shows "A:/T21/sqltype2.bas" with no "Check Disk".

**The release file was replaced again on 23 November,** before 8.00.00b3 was announced, with FRAMEBUFFER WARPH and WARPV. Nothing else changed in the firmware. The release file runs on the same Generation 1, with its options loaded back unchanged:
- **tests/cmm2** (114 programs, now with warpn.bas: WARPH and WARPV in the Colour Maximite 2's own colour depths, in 13 configurations, and 18 error tests) and **the PicoMite tests** (now with warp.bas, the PicoMite's own WARP test under OPTION PICOMITE VGA in MODE 2 and 3): warpn.bas and warp.bas give their expected output. Every other program gave what it gave under the first 8.00.00b3 file, byte for byte, but for one line: a test that lists the options now also shows OPTION STATUS OFF, which was set on this board for the OPTION STATUS tests. The crash tests, the corpus, the test files and SP Tools were not run again. The statements' timings, measured with the same code before the release build, are as before.
- **The manual's example** for FRAMEBUFFER WARPH, as printed, ran in MODE 7 at 75 frames a second: every frame of that mode's 75Hz display.
- **The PicoMite's warp_demo.bas** ran unchanged under OPTION PICOMITE VGA at 59.5 to 59.9 frames a second, with the same code before the release build.
- **The programs:** after a restart, each program in the zip that runs on a Generation 1 started from the prompt with no error, the 386 PC and the NES included, and the prompt came back when it was left. The Generation 2 programs refused with "Needs a G2", as before.
- **The timings** over a whole 800 x 600 page are under Known problems.

### Known problems

- **No Generation 3 with the module on its board has been made yet.** Its path has run only in a test build that took a Generation 3 without the module for one with it: the firmware found no chip, said "WiFi module not found" and carried on with every pin free. The same 16-bit transfer code has carried the plug-in module's traffic without an error (see Tested). If the first boards misread at 30MHz, the bus can be slowed to 15MHz in the next beta.
- **Opening the console's own port as COM3** (or COM1 or COM2 under OPTION CONSOLE PORT) in a program has not been tried on a board since the OPTION CONSOLE change. COM3 still opens only when the saved option is OPTION CONSOLE SCREEN or NONE, as before.
- **A large loadable program on a Generation 1 after many BASIC programs.** After a long session the 386 PC and the NES emulator stopped with "Needs n KB, n KB free in one block". In this testing that came after about 400 test programs. Each started at once after a restart (CPU RESTART or the reset button). Something those programs used is held until a restart and keeps the loader from joining the display memory to the heap. 8.00.00b2 has the same code. Which statement does this is not yet known.
- **FRAMEBUFFER WARPV a column at a time over a whole 800 x 600 page** is slow. With offsets that differ from column to column it took 108 ms in the 8-bit colour depth, 253 ms in 16 bits and 594 ms in 12 bits on a Generation 1. Bands of four columns (`step` 4) took 20, 39 and 70 ms, and WARPH over the same page 4, 26 and 53 ms.
- **Bluetooth game controllers** have not been tried with a real controller yet.
- **Why the radio chip once gave a wrong Bluetooth buffer position** is not known; the firmware refuses it, so it costs at most a packet.
- **Bluetooth and the network wait while a loadable program runs:** a Bluetooth keyboard does not type into it and a speaker plays nothing (MicroPython's network is the exception: it services the network itself).
- **The Bluetooth serial device:** a Bluetooth LE module passes on only part of a long burst at 115200 baud (the module's limit): send in pieces or more slowly.
- **MicroPython on a Generation 1** has about 1.6MB for Python. In one long session two of the book's games (ch19/05 and ch25/02) still run out of memory after the examples before them; each runs after Ctrl-D (a soft reset).
- **The WebMite's own example programs** have not all been tried yet.
- **A Generation 3 can reset when its HDMI cable is unplugged** (the cable's shield meeting the monitor's ground on the reset line). The next board has a fix.
- The known problems of 7.00.00RC6 remain, apart from the three SQL faults fixed in 8.00.00b1.

### The manuals

- **The User Manual** is for 8.00.00b3: the Generation 3's module on the board (Hardware Features, WiFi and Internet Functions, Bluetooth, Porting Programs, Appendices D and M, OPTION WIFI and SPI2), SQL(TYPE n) (in SQL( and the chapter SQL Databases), and OPTION CONSOLE.
- **The User Manual was replaced on 23 November** (its title page says 23 November). It adds FRAMEBUFFER WARPH, WARPV and WAIT, and PAGE WARPH and WARPV, in the Commands and in the chapters Video Modes and Pages and Game Playing Features. It also carries changes made to the manual's text for 8.00.00b1 and later that the manuals of 8.00.00b1 to 8.00.00b3 had missed:
  - **Using MMBasic:** the program is held in RAM, not in the flash, and is lost at a reset or power off (MMBasic keeps it over the restart of an option change); AUTORUN.BAS for a program to run at power up; the library has 128KB of flash of its own; a loadable program on a Generation 1 borrows about 3.75MB.
  - **MMBasic Characteristics:** the memory map of version 8 (the flash and the SDRAM of each generation).
  - **Appendix K:** what a firmware upgrade keeps and clears in version 8.
  - **Video Modes and Pages:** MODE 19 is 720 x 400 (the PicoMite's), and its pages; the PicoMite's 640x480 at 75Hz (OPTION RESOLUTION 640x480, 315000); on VGA, MODE 18 and the PicoMite's 1024x600 are shown in a 1024 x 768 frame.
- **Colour Maximite 2 Loadable Programs:** its title page (firmware 8.00.00b3), and 8.00.00b3 in the list of services versions (still 22).
- **The App User Guide and the C Compiler guide** are unchanged apart from the zip's name. The 386 PC Emulator guide and the Stepper reference are unchanged.

### Licences

Unchanged since 8.00.00b2. The firmware's own licence (LICENSE) is unchanged. For the WiFi and Bluetooth it includes lwIP, Mbed TLS, the cyw43-driver, BTstack, puff and the CYW43439's firmware. **BTstack**'s licence allows redistribution and use "solely for personal benefit and not for any commercial purpose or for monetary gain" (Raspberry Pi's licence for BTstack covers only its own chips): free for the free firmware, but anyone who sells boards with it needs a commercial licence from BlueKitchen. cyw43-driver's copy carries the same condition. Each licence is beside its code in Middlewares/Third_Party, and in the licences folder of the zip.

## 8.00.00b2 (18 November 2026)

*18 November 2026. Compared with 8.00.00b1, the last version released.*

This is the second beta of MMBasic 8.00 for the Colour Maximite 2. Please report problems on the Back Shed forum. 7.00.00RC6 remains the last release of version 7.

The same firmware file runs on the Generation 1, the Generation 2 and the Generation 3.

8.00.00b2 brings:

- **Bluetooth.** The WiFi module's chip is a Bluetooth radio too. Bluetooth keyboards, mice (and a keyboard's touchpad) and game controllers; the sound through a Bluetooth speaker or headphones; and a Bluetooth serial device as COM8. It is the PicoMite's Bluetooth, with its BLUETOOTH command, and the WiFi and Bluetooth can be used at the same time.
- **MicroPython** runs every example program of the book *Programming the Pico Computer 3*, with the clock and its alarm, asyncio, native code, the I/O pins as on a Pico Computer 3 and, with the WiFi module, the network: sockets, HTTPS, MQTT.
- **Loadable programs** can use the network, the clock's alarm and the LED (services versions 21 and 22).
- **zeptoforth 1.17.2.1**, with fixes to code it writes as it runs (closures, CREATE and DOES>, DEFER) and to DEFER! on the Colour Maximite 2's memory; and zeptoforth_g1.app starts on a Generation 1 again.
- **Fixes:** a program that left GUI controls behind could, at the next RUN, damage the WiFi's and Bluetooth's memory (since 8.00.00b1); WEB TCP CLIENT with a 64KB receive array; a memory fault that could give a program memory already in use; LGETSTR$ of 255 characters, which could spoil an open file (an SQL COMMIT failed with "disk I/O error"); and WEB CONNECT, which did not join the WiFi again once it was down.

### Before you update

**From 8.00.00b1:** save the options first and load them again afterwards, as with every update. Writing the firmware erases them:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

A file saved under 8.00.00b1 loads into 8.00.00b2: OPTION RADIO is set from OPTION WIFI, so the WiFi carries on as before and Bluetooth stays off. The console runs at 115200 baud until the options are loaded. Save the program in memory to the SD card first: version 8 does not keep it over a reset.

The flash library is kept and works as before, and .lib files and flash slots saved by 8.00.00b1 load, as no tokens have moved.

**From 7.00:** read "Before you update" in 8.00.00b1's notes first (they are in Release_Notes_8.00.00b2_full.md): the program held in the flash is lost, and the library and flash slots must be saved again. Everything there applies to 8.00.00b2 too.

**Going back to 8.00.00b1:** write it with "Full chip erase" ticked in STM32CubeProgrammer, then OPTION DISK LOAD an options file saved under 8.00.00b1, and LIBRARY LOAD the library's files again. 8.00.00b1 refuses a file saved by 8.00.00b2 ("File size incorrect"), as that holds the Bluetooth options and pairings too.

Copy CMM2_Programs_8.00.00b2.zip to the card as well if you use the programs in it: its micropython.app is new, and needs 8.00.00b2.

### What is new

#### Bluetooth

The Bluetooth is the PicoMite's: the code of its Bluetooth versions (PicoMiteRP2350BTH, PicoMiteHDMIBTH and PicoMitePC3), unchanged, with the same BLUETOOTH command, options and functions. The User Manual has a new chapter, Bluetooth.

- **Turning it on.** It needs the WiFi module. OPTION RADIO BOTH (WiFi and Bluetooth), WEB, BLUETOOTH or NONE, the PicoMitePC3's option. NONE is the default, and OPTION WIFI turns the WiFi's half on, so nothing changes until OPTION RADIO is set.
- **Keyboards, mice and game controllers.** Bluetooth LE devices pair by themselves when put into pairing mode nearby; a Bluetooth Classic keyboard is found with BLUETOOTH SCAN and connected with BLUETOOTH CONNECT "name". A keyboard works as a USB keyboard does, at the prompt, in the editor and in programs, with its lock lights. A mouse, or a keyboard's touchpad, is the mouse on channel 2 (DEVICE(MOUSE 2, ...), MM.MOUSE, the GUI cursor). A Bluetooth LE game controller is DEVICE(GAMEPAD 3, ...), with GAMEPAD INTERRUPT 3.
- **Speakers and headphones.** OPTION AUDIO BLUETOOTH, then BLUETOOTH CONNECT "name": everything PLAY makes is played through the speaker, and through the audio socket as well. The speaker is remembered and connects again by itself. The stream is suspended after 60 seconds of silence (OPTION AUDIO BLUETOOTH n sets the time; 0 keeps it running).
- **Serial devices as COM8.** A Bluetooth Classic serial device (SPP: a GPS receiver, an HC-05) or a Bluetooth LE serial module (the Nordic UART Service, HM-10, Microchip's Transparent UART), connected with BLUETOOTH CONNECT and opened with OPEN "COM8:", with a receive buffer, an interrupt and AS GPS.
- **Pairings** are stored with the options, so paired devices reconnect after a power cycle. BLUETOOTH FORGET deletes them; OPTION DISK SAVE and LOAD keep them.
- **BLUETOOTH STATUS** lists the connections; MM.INFO$(BLUETOOTH KEYBOARD | SPEAKER | SERIAL) gives a device's address.
- **Memory:** Bluetooth keeps 64KB of the memory for variables while it is on (MM.INFO(HEAP) is 64KB less).
- **Differences from the PicoMite:** the serial device is COM8, not COM7; OPTION AUDIO DAC, not OPTION AUDIO DISABLE, ends the sound to Bluetooth; there is no console over Bluetooth; and Bluetooth is not serviced while a loadable program runs.

#### MicroPython

The CMM2's MicroPython (micropython.app in the zip) now runs every one of the 162 example programs of *Programming the Pico Computer 3*, Peter Mather's course in Python for the Pico Computer 3. The book's new Appendix K, Running this book on a Colour Maximite 2, gives the few differences: how to start it, where the files are, and which connector pin is which. The book is in github.com/UKTailwind/micropython, branch pico-computer-3.

New in the app:

- **The I/O pins as on a Pico Computer 3:** the same GP numbers, GP40 to GP46 and ADC(n) as on the RP2350B, Pin("LED") the SD card's activity light, I2C on any pins (a software I2C), and a pin's owner changes as on a PC3.
- **The clock:** gettime(), settime(), machine.RTC and a daily alarm, on the DS3231 with OPTION DS3231 ON or on the processor's own clock.
- **machine.WDT** (the watchdog), **asyncio**, and **@micropython.native, viper and asm_thumb**.
- **The network**, with the WiFi module and OPTION WIFI: wifi() and ntpsync() at the prompt, socket, network.WLAN, TLS (HTTPS), requests, MQTT (umqtt), ntptime and mip.
- **Fixes:** SQLite's memory is set aside at start-up (after a long session connect() could fail with memory free); `import ulab.numpy`; files that a program dropped without closing them are closed when the limit of open files is reached; pins are given back at a soft reset (Ctrl-D), and a pin another part has says "GPn is in use"; and a program's end now lets go of its off-screen buffer (hdmi.create()) and its sprites and collects the memory, so that the book's games run one after another on a Generation 1, which has less memory.

It needs 8.00.00b2, as it uses the new services below.

#### zeptoforth

The zip's zeptoforth is version 1.17.2.1: Bezier curves (bezier.fs, in the zeptoforth folder, with an example in WORDS.md under Drawing), 0x before a hexadecimal number, and zeptoed leaving the cursor where it belongs when it ends. Fixed:

- **Code written as it runs** reached the processor's instruction cache only by chance in three places: closures (bind and the words built on it), CREATE, <BUILDS and DOES>, and DEFER. A closure bound in a loop could run its first binding again, and a word made and used on one line could run what was there before. Each now makes the caches agree.
- **DEFER!** (and IS) on a word deferred at the prompt said "attempted to write past flash end" and left zeptoforth compiling to flash: it took the Colour Maximite 2's memory for flash.
- **zeptoforth_g1.app** did not start on a Generation 1 under version 8 ("the memory it is linked for is not free"): it is now linked where version 8 leaves room, so it needs version 8 (7.00's zip keeps the one for 7.00). zeptoforth.app is unchanged in that.

The two saved systems, zeptoforth.img and zeptoforth_g1.img, are made again for the new kernel. A zeptoforth.img you saved yourself with an earlier zeptoforth does not load into this one: keep your source files, include them again and save-image.

#### Loadable programs: services 21 and 22

- **Version 21:** rtc_alarm, the clock's daily alarm; and the network: net_poll, net_join, net_leave, net_ifconfig and net_dns, and eight TCP or UDP handles (sock_open to sock_close) over the firmware's own network. While a program runs the firmware does not service the network itself: net_poll does.
- **Version 22:** led (the SD card's activity LED as the program's own), gp_pin (a GP's connector pin) and pin_setup's pull for an open drain output.

The Loadable Programs guide describes them (The Services: The Clock and The Network; The I/O Pins: The LED).

#### GP40 to GP46

GP40 to GP46, the analog inputs of an RP2350B (and of the Pico Computer 3), are second names for the Colour Maximite 2's analog pins: GP40, GP41 and GP42 are pins 26, 29 and 37 (GP26 to GP28, so ADC channel n is GP40+n on both chips), and GP43 to GP46 are pins 7, 13, 15 and 24 (GP6 to GP9). A program that uses a second name must not use the pin's first name as well. MM.INFO(PINNO GP40) is 26. GP47 still has no pin. MM.INFO(MAX GP) is 46 (it was 37); under OPTION PICOMITE it is still the PicoMite's.

#### Fixes

- **RUN after a program that used GUI controls** (OPTION MAXCTRLS) could damage the network's memory, and with Bluetooth on Bluetooth's: RUN, NEW and loading a program freed the old program's controls (and its serial ports' buffers) after the memory they were in had already been cleared, and each such free went on to free the memory kept above it for the WiFi and Bluetooth. Programs then printed lwIP errors ("mem_free: illegal memory") or the Colour Maximite 2 stopped. In 8.00.00b1 too, on any board with OPTION MAXCTRLS and OPTION WIFI. The old program's I/O is now closed first, as on the PicoMite, and a second free of the same memory does nothing.
- **Bluetooth:** a wrong position read from the radio chip's Bluetooth buffer (once, on a Generation 2 with a speaker connected) made the firmware overwrite 56KB of its memory and stop. Such a position is now refused, and the read tried again at the next poll.
- **WEB TCP CLIENT REQUEST** into a 64KB array (DIM b%(8192)) received nothing ("No response from server"), and into a larger one lost part of each packet. The PicoMite had the same fault, now fixed there too.
- **Memory:** MMBasic's memory for variables could be given space that the C library's own allocator had already handed out (used by the audio decoders, JSON, the file system and now Bluetooth), when a program filled the memory. They are now kept apart.
- **LGETSTR$** of exactly 255 characters wrote a zero one byte past its result, into whatever came next in memory: an open file's record (a reader's SQL script then failed at COMMIT with "SQL: disk I/O error"), a string or an array. In 7.00 and the PicoMite too, where it shows less often.
- **WEB CONNECT** with no parameters joined OPTION WIFI's network again only after a failed connection, not when the WiFi was simply down (after a loadable program's join to another network, for example), so only CPU RESTART brought it back. It now joins whenever it is not connected, as the manual says. The PicoMite has the same fault.
- **MM.VER** is 8.000002.

### What behaves differently

- **New keyword:** BLUETOOTH. A program that uses it for its own variable must rename it.
- **With Bluetooth on** (OPTION RADIO BOTH or BLUETOOTH) the WiFi's four pins (32, 33, 38 and 40) belong to the radio, as with OPTION WIFI, and MM.INFO(HEAP) is 64KB less.
- **OPTION LIST** shows OPTION RADIO when it is not what OPTION WIFI alone gives, and OPTION AUDIO BLUETOOTH when it is set.

### Structural changes

For those who build the firmware or study its source:

- **BTstack** (BlueKitchen, at the commit of pico-sdk 2.3.1) is in Middlewares/Third_Party/btstack, unchanged (tools/port_btstack.py). The PicoMite's Bluetooth files are copied into Src/net/bt/picomite by tools/port_bt.py with four mechanical changes; the Colour Maximite 2's side is Src/net/bt/BTMite_cmm2.c, with pico-sdk's HCI transport for the CYW43439.
- **The flash:** the CYW43439's firmware is now kept compressed (141KB in place of 225KB), and unpacked by Mark Adler's puff (Middlewares/Third_Party/puff, zlib licence) when it is loaded into the chip. That made room for Bluetooth in the network's .net section (bank 2's sectors 3 to 6).
- **RAM:** the radio's data (the driver, lwIP, Mbed TLS, BTstack and Src/net) moved to the top 32KB of the D2 RAM. Bluetooth has its own 64KB heap, taken from MMBasic's SDRAM heap and kept across RUN and NEW, as the network's block is.
- **The options record** is layout 5: OPTION RADIO, OPTION AUDIO BLUETOOTH and 2KB of Bluetooth pairings (3,260 bytes). OPTION DISK LOAD also takes 8.00.00b1's layout 4 and 7.00's files.
- **Tokens:** BLUETOOTH is a new command token at the end of the command table, so no other token moved; the image format is still 7.
- **The loadable programs' services** are version 22 (Inc/cmm2img.h; the network's side is Src/net/LoaderNet.c).

### The programs

CMM2_Programs_8.00.00b2.zip is 8.00.00b1's zip with micropython.app replaced (it needs 8.00.00b2), the zeptoforth folder replaced (1.17.2.1, with bezier.fs) and README.txt for this version. The licences folder adds BTstack's licence (for the firmware's Bluetooth) and micropython-lib's (for MicroPython's network modules). The other programs are unchanged and run on 8.00.00b2 as on 8.00.00b1.

### Tested

On a Generation 2 with the WiFi module, Bluetooth on and a speaker connected, and a Generation 3 with the WiFi module, with the firmware of 17 November (this release but for the three fixes of 18 November: LGETSTR$, WEB CONNECT and MM.INFO(MAX GP)); and on a Generation 1 with the WiFi module:

- **The regression** on both: the test programs, the PicoMite tests, the crash tests, the program corpus, the PicoMite's Testfiles and SP Tools (686 of 727, as before). No change from the last runs but what the WiFi module's pins, its memory and the network bring, and the expected output of the GP pin test (GP40 to GP46).
- **The RUN and Bluetooth faults fixed above** were found by this regression on the Generation 2: with the fixes, the programs that failed ran clean six times over.
- **The update from 8.00.00b1** on the Generation 2, as a user does it: 8.00.00b1 written fresh, a library and its .lib file and the options saved; 8.00.00b2 written without a full erase; the options loaded, the library kept and working, the .lib file loaded.
- **MicroPython** (the zip's micropython.app) on both: its new modules, the pins and the LED, the clock, SQLite and an HTTPS fetch; on the Generation 3 also the book's Appendix K example and a short sound through HDMI. Every one of the book's 162 example programs ran on the Generation 2 with this release's services (on a build before the RUN and Bluetooth fixes, which do not touch loadable programs).
- **Bluetooth** on the Generation 3 with the WiFi: turned on, a scan finding a speaker, the 64KB set aside, COM8's message with no device. On 16 November, with this release's Bluetooth code: a Bluetooth Classic keyboard and its touchpad, a speaker and an HM-10 serial module on COM8 (both ways) on a Generation 2, and the regression on all three generations.
- **Generation 1**, on 18 November: the regression, with the LGETSTR$ fix, the same as on 15 November but for one test now right (SP Tools 686 of 727); then this release's file: MM.INFO(MAX GP), the program corpus's WiFi programs and TFTP as on 15 November, WEB CONNECT after a failed join, SQL, and MicroPython: 159 of the book's 162 example programs in one session, the network's included (two run out of memory there: below; the last restarts the board, as it is meant to, and was run on its own). The LGETSTR$ fix also on the Generation 2: the reporter's programs and the SQL tests.
- **zeptoforth** (the zip's), on the Generation 1 and a Generation 3, each with the saved system made on it: float64 108 of 108, the Black-Scholes and solar eclipse programs (the eclipse identical to MMBasic's output), the words tests for the documentation, the graphics and tasks under stress, and the fixes above (closures bound in a loop, DEFER and IS, CREATE and DOES> on one line). The stress test's count of a task's card reads now asks only that it ran: an old 4GB card reads at a fifth of the speed.

### Known problems

- **Bluetooth game controllers** have not been tried with a real controller yet.
- **Why the radio chip once gave a wrong Bluetooth buffer position** is not known; the firmware now refuses it (above), so it costs at most a packet.
- **Bluetooth and the network wait while a loadable program runs:** a Bluetooth keyboard does not type into it and a speaker plays nothing (MicroPython's network is the exception: it services the network itself).
- **The Bluetooth serial device:** a Bluetooth LE module passes on only part of a long burst at 115200 baud (the module's limit): send in pieces or more slowly.
- **MicroPython on a Generation 1** has about 1.6MB for Python. In one long session two of the book's games (ch19/05 and ch25/02) still run out of memory after the examples before them; each runs after Ctrl-D (a soft reset).
- **The WebMite's own example programs** have not all been tried yet.
- **A Generation 3 can reset when its HDMI cable is unplugged** (the cable's shield meeting the monitor's ground on the reset line). The next board has a fix.
- The known problems of 7.00.00RC6 remain, apart from the three SQL faults fixed in 8.00.00b1.

### The manuals

- **The User Manual** is for 8.00.00b2: the new chapter Bluetooth, with the BLUETOOTH command, OPTION RADIO, OPTION AUDIO BLUETOOTH, COM8 and MM.INFO$(BLUETOOTH ...) in the references; GP40 to GP46 in Using the I/O Pins; WiFi and Bluetooth in Porting Programs.
- **Colour Maximite 2 Loadable Programs** adds services 21 and 22: the clock's alarm, the network, the LED and gp_pin.
- **The App User Guide**: the MicroPython chapter (what it has now, and the book), and the zip's name.
- **The C Compiler guide** is unchanged apart from the zip's name. The 386 PC Emulator guide and the Stepper reference are unchanged.

### Licences

The firmware's own licence (LICENSE) is unchanged. Bluetooth brings **BTstack**, whose licence allows redistribution and use "solely for personal benefit and not for any commercial purpose or for monetary gain" (Raspberry Pi's licence for BTstack covers only its own chips): free for the free firmware, but anyone who sells boards with it needs a commercial licence from BlueKitchen. cyw43-driver's copy carries the same condition, as in 8.00.00b1. puff (zlib licence) unpacks the chip's firmware. Each licence is beside its code in Middlewares/Third_Party, and in the licences folder of the zip.

## 8.00.00b1 (16 November 2026)

*16 November 2026. Compared with 7.00.00RC6, the last version released.*

This is the first beta of MMBasic 8.00 for the Colour Maximite 2. Please report problems on the Back Shed forum.

**7.00.00RC6 is the last release of version 7.** It is highly functional and well tested, and can be used as it is, but there will be no full 7.00.00 release: version 8 is needed for the new Generation 3 and for WiFi, and all further work goes into it.

The same firmware file runs on the Generation 1, the Generation 2 and the new Generation 3.

Version 8 brings:

- **WiFi and the Internet.** With a WiFi module on the rear I/O connector the Colour Maximite 2 runs the WebMite's networking: a web server, web and TLS clients, UDP, MQTT, time from the Internet, a Telnet console and TFTP file transfer, with the WebMite's WEB commands.
- **The Generation 3.** A Generation 2 with an HDMI output in place of VGA, with the sound sent to the HDMI monitor as well as to the audio socket.
- **Programs always run from RAM.** The BASIC program is never written to the flash, so loading is quicker and editing and running programs no longer wear it. The flash it used now holds the networking.
- **USER modules.** Commands and functions of your own, written in C, which a BASIC program calls like the built-in ones.
- **SQL fixes**, with text values of any length, and more of the PicoMite's commands for programs written for it.

### Before you update

Read this section first: going from 7.00 to 8.00 changes where the program and the library are kept.

**Save your program to the SD card.** Version 7 kept the program in the flash; version 8 puts its own code in that part of the flash, so the program held there is lost when the new firmware is written.

**Save the options first, and load them again afterwards**, as with every update. Writing the firmware erases them:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

A file saved by OPTION DISK SAVE under 7.00 loads into 8.00 (its WiFi options start off). The console runs at 115200 baud until the options are loaded. The variables saved with VAR SAVE and the files on the SD card are kept.

**The library has to be loaded again.** The flash library survives the update, but version 8 numbers some of its functions differently (below), so a library saved by 7.00 would call the wrong ones. When the options are loaded, MMBasic says "The library was saved by another firmware version and has been discarded: LIBRARY LOAD its files again". Load it from its source files with LIBRARY LOAD. A .lib file saved by LIBRARY DISK SAVE under 7.00 is refused for the same reason.

**The flash slots** (FLASH SAVE n) are files on the SD card and are kept, but a program saved in one by 7.00 is refused ("Flash slot n holds ... from another firmware version: load it and save it again"). Load the program from its .bas file and FLASH SAVE it again.

**A program that runs at power up** must now be the file AUTORUN.BAS in the root of the SD card, with OPTION AUTORUN ON (see Programs in RAM below).

**Going back to 7.00** is possible: write 7.00.00RC6 with "Full chip erase" ticked in STM32CubeProgrammer, then OPTION DISK LOAD an options file saved by 7.00 (7.00 cannot read one saved by 8.00), and LIBRARY LOAD the library's files again.

Copy CMM2_Programs_8.00.00b1.zip to the card as well if you use the programs in it. The programs are the same as in the RC6 zip; only its README and the licences of the firmware's network code are new.

### What is new

#### WiFi and the Internet

The networking is the PicoMite's: the WebMite firmware's code, unchanged, with the same WEB commands, options and functions, so a WebMite program runs as it is, apart from the I/O pins it uses. The User Manual has a new chapter, WiFi and Internet Functions.

- **The module.** A Raspberry Pi RM2 (the WiFi chip of the Pico W, an Infineon CYW43439) on the rear I/O connector, using four pins: 40 (clock), 38 (data), 33 (chip select) and 32 (power). On a Generation 1 the connector is turned through 180 degrees, so the module plugs on the other way round: its pin 1 always goes to the connector's pin 1.
- **Turning it on.** OPTION WIFI ssid$, password$ [, name$] [, country$], or with a fixed IP address. Without OPTION WIFI the module is switched off and the four pins are free. If no module answers at start-up the Colour Maximite 2 says "WiFi module not found" and carries on without the network.
- **What works:** WEB CONNECT, SCAN, PING and NTP; the web server (OPTION TCP SERVER PORT, WEB TCP INTERRUPT, WEB TRANSMIT PAGE and FILE, several clients at once); TCP clients, encrypted with TLS too (WEB OPEN TLS CLIENT, with WEB TLS CA to check the server's certificate); UDP (OPTION UDP SERVER PORT, WEB UDP SEND, MM.ADDRESS$, MM.MESSAGE$); MQTT (WEB MQTT CONNECT, PUBLISH, SUBSCRIBE, MM.TOPIC$); the Telnet console (OPTION TELNET CONSOLE ON, or ONLY to leave the serial port out); TFTP file transfer to and from a PC (OPTION TFTP ON); MM.INFO(IP ADDRESS), (WIFI STATUS), (TCPIP STATUS), (TCP REQUEST n), (MAX CONNECTIONS), (TELNET) and the rest.
- **Memory.** The network keeps 128KB for itself while OPTION WIFI is set. On a Generation 2 or 3 it comes from BASIC's memory for variables (MM.INFO(HEAP) is 128KB less). On a Generation 1 it is the top of the video memory, so some modes and colour depths have one page fewer (the chapter lists them) and the memory for variables and loadable programs is unchanged.
- **Loadable programs.** The network waits while a loadable program (.app) runs. On a Generation 1 a program built to use the video memory (Doom, the emulators) is lent the network's memory as well, so afterwards the network starts again from the beginning and joins the WiFi network again, in about seven seconds.
- **Differences from the WebMite:** the pins; no heartbeat LED; no PLAY STREAM (it needs a VS1053); and WEB TCP INTERRUPT 0, which the WebMite's manual gives for turning the interrupt off, is an error, on the WebMite too.

#### The Generation 3 (HDMI)

The Generation 3 is a Generation 2 whose video goes through a MacroSilicon MS7210, which turns it into HDMI. The firmware recognises it at start-up; MM.DEVICE$ returns "Colour Maximite 2 G3".

- **Every mode** shows, from power up, including OPTION PICOMITE's resolutions. Modes whose pixels are wider than a standard mode's are sent at a standard rate with each pixel repeated, so that monitors accept them: mode 7 reaches the monitor as 640 x 480 at 75Hz, 12 as 1920 x 1080 at 60Hz, 13 as 800 x 600, and 4, 5, 6 and 17 as 640 x 480. Mode 18 is sent as 1024 x 600, the resolution of many small HDMI displays.
- **Full colour levels:** white is full white. OPTION VGA OUTPUT and OPTION HORIZONTAL OFFSET are kept but not used on a Generation 3.
- **Sound through HDMI** as well as the audio socket: everything MMBasic and the loadable programs play reaches the HDMI monitor's speakers too, sixteen bits unchanged.
- **OPTION DVI ON** sends DVI instead, without sound, for a monitor (or an HDMI to DVI adapter) that does not accept the full HDMI signal. It gives "Needs a G3" on a Generation 1 or 2.
- **UPDATE FIRMWARE** works on a Generation 3: the HDMI chip is reset first, so that it stays quiet while the processor's bootloader runs.

#### Programs always run from RAM

- The program is held in 512KB of the SDRAM, as OPTION RAM did in 7.00, on every generation. It is never written to the flash, so loading is quicker and editing and running programs do not wear the flash. LIBRARY SAVE and the options are the only writes to the flash that remain.
- OPTION FLASH and OPTION RAM are gone; typing either gives the ordinary error for an unknown option. An options file saved by 7.00 with either set loads without error.
- **The program is lost** when the power is removed, or on a reset (the reset button or the watchdog).
- **The program is kept over an option's restart:** when a command restarts MMBasic (an OPTION that needs a restart, OPTION RESET and the like), MMBasic first saves the program to a temporary file on the SD card and loads it back after the restart, so LIST, RUN and EDIT carry on. Without an SD card the program is lost, as with OPTION RAM.
- **OPTION AUTORUN ON** runs A:/AUTORUN.BAS at power up or after the watchdog. Without a card it goes to the prompt.
- **The library** has a 128KB sector of the flash to itself, so it no longer takes space from programs.

#### USER modules

USER LOAD "file" loads a module of your own, written in C with the loadable programs' tools; then USER name [arguments] runs one of its commands and USER(name [arguments]) one of its functions, which read their keywords and arguments with the interpreter's own routines, as built-in commands do. USER CLOSE unloads it. RUN, NEW and CHAIN unload it with the rest of the program's memory, and the next USER or USER( loads it again from the same file; loading it again also picks up a rebuilt module without a restart.

The companion manual, Colour Maximite 2 Loadable Programs, has a new chapter, USER Modules, and the tools have a module to start from (tools/sdk/examples/user/proforma.c), built with build.py --user. The SQL command works the same way, with SQLite as its module, and a USER module can be loaded at the same time as the SQL module.

#### SQL

The three faults homa found in RC6 are fixed:

- **Ctrl-C** in a long statement gives the error "SQL: stopped by Ctrl-C", which ON ERROR SKIP catches like any other error; before, it ended the program.
- **Long text:** SQL(LONGSTRING n, a%()) reads a column of any length into a LONGSTRING and returns its length. A LONGSTRING among a statement's values gives its text to a ? parameter, and a statement itself can be a LONGSTRING, for one longer than 255 characters. A string too long for SQL(n) is still an error, but the query stays on its row, so a program can read the column again with SQL(LONGSTRING.
- **SQL INSERT of an array of structures** that stops part way (an error or Ctrl-C) now undoes its rows and closes its transaction; before, every later change was lost at SQL CLOSE.
- UTF-16 databases made on a PC open (also in the RC6 zip's sql.mod since 16 November).

#### For PicoMite programs

- **WATCHDOG HW timeout** and **WATCHDOG HW OFF**, under OPTION PICOMITE, work the ordinary watchdog (timeout 1 to 8331 ms, the PicoMite's limit), so a PicoMite program that uses the hardware watchdog runs unchanged.
- **MM.INFO(BOOT COUNT):** the number of starts, kept in the clock's battery-backed memory.
- **OPTION COUNT** under OPTION PICOMITE: accepted when it names the Colour Maximite 2's own four counting pins (7, 13, 15 and 24, which are GP6 to GP9), in any order; any other pin is an error that says which pins count here.
- **MODE 19** is now 720 x 400 at 70Hz, the PicoMite's 720x400, which OPTION PICOMITE's 720x400 now uses at full screen (it was shown in a corner of 848 x 480).
- **640x480 at 75Hz:** OPTION RESOLUTION 640x480, 315000 (and RESOLUTION or OPTION PICOMITE with that speed) gives 75Hz, as on the PicoMite; MM.INFO(MODE) then returns 20.
- Geoff Graham's Retic, the WebMite irrigation controller, runs under OPTION PICOMITE with only its pins changed.

#### Other changes and fixes

- **OPTION COMPILE** compiles plain BLIT, BLIT WRITE (drawn directly: fire.bas 61.5 to 24.4 ms a frame), SPRITE SHOW, HIDE and NEXT, IMAGE RESIZE_FAST and RGB colour constants.
- **TURTLE ARC LEFT and ARC RIGHT** gave "Expression syntax"; they work now.
- **PIXEL(** in an 8-bit colour mode returns the colour the palette holds, on every board.
- **MODE 18** and OPTION PICOMITE's 1024x600 on VGA are sent as 1024 x 768 at 60Hz with the 600 lines in the middle, as many VGA monitors refuse 1024 x 600. A Generation 3 sends 1024 x 600 itself.
- **The first start's time:** the tens of hours accepted any key, so an Enter was taken as a digit.
- **The first start's font:** after a firmware update with the options erased, every program's END could fail with "Invalid font number #16".
- **TIMER = n** no longer resets the clock underneath: TIMER is now an offset from it, as on the PicoMite, so timeouts (and the network's clock) never go backwards.
- **MM.VER** is 8.000001.

### What behaves differently

- The program is lost at power off and reset, and OPTION FLASH and OPTION RAM are gone (above).
- **The memory for variables is smaller:** about 4MB on a Generation 1 (it was 5MB, 4.5MB with OPTION RAM) and 15MB on a Generation 2 or 3 (15.5MB), because the program and the CSUBs' code now each have their own 512KB. A loadable program on a Generation 1 borrows about 3.75MB at the command prompt (it was 4MB); all the programs in the zip still fit.
- **MODE 19** is 720 x 400; in 7.00.00RC5 and RC6 it was 640 x 512, an experiment.
- **EOF(, LOC( and LOF(** are now one token internally (LIST shows them as before). This moved the function tokens after them, which is why a library, a .lib file or a flash slot saved by 7.00 is refused.
- **New keywords:** WEB, USER, USER(, MM.MESSAGE$, MM.ADDRESS$ and MM.TOPIC$. A program that uses one of these names for its own variable or array must rename it.
- **With OPTION WIFI set** pins 32, 33, 38 and 40 and SPI2 belong to the network: SETPIN refuses them and the SPI2 command cannot open SPI2.

### Structural changes

For those who build the firmware or study its source:

- **Branches:** main stays 7.00. Version 8 is on the branch v8-dev (v8-wifi is the same commit at this release).
- **The flash:** bank 2's sectors 3 to 6 (512KB), the program's until 7.00, are now firmware: the network stack is there, in a .net section of about 430KB. Sector 7 is the library's alone and sector 2 still holds the options. The firmware file grew from 1.41MB to 1.87MB. TURTLE and DRAW3D moved from bank 1 to bank 2, leaving about 18KB of bank 1 free.
- **The SDRAM:** the program's 512KB is at the very top (Generation 1 0xD0780000, Generation 2 0xD1780000), with the 512KB executable window for CSUB code below it (together one 1MB executable MPU region), the heap below that, and the compiler's stream directly after the video memory. The Generation 2's 8MB for loadable programs is unchanged. The User Manual's memory map (Special Functions and Characteristics) has the details.
- **Third-party code** in Middlewares/Third_Party, each with its licence: lwIP 2.2.1, Mbed TLS 3.6.6 and George Robotics' cyw43-driver with Infineon's CYW43439 firmware, at the commits the PicoMite's pico-sdk 2.3.1 has, unchanged.
- **The PicoMite's network code** (MMtcpserver.c, MMTCPclient.c, MMudp.c, MMMqtt.c, MMntp.c, MMping.c, MMtelnet.c, MMtftp.c, WiFi.c, mqtt.c, tftp.c) is copied unchanged into Src/net/picomite by tools/port_net.py; the Colour Maximite 2's side is Src/net/WebMite_cmm2.c, with the radio's bus on SPI2 in half-duplex at 24MHz (20MHz on a Generation 1 at 400MHz) in Src/net/cyw43_bus_cmm2.c. The network's .data and .bss are grouped by the linker script, so that a Generation 1 can start the network again after lending its memory.
- **The Generation 3:** Src/MS7210.c (the HDMI chip: reset on PG3, set up over its own I2C, the clock on PG7 doubled for the wide-pixel modes) and Src/HdmiAudio.c (SAI2 as I2S at 44,117.6Hz to the MS7210).
- **USER modules:** Src/User.c and Inc/cmm2user.h; the loadable programs' services are version 20 (no new services: the start block gains a table of the interpreter's routines for a module).
- **The options record** is layout 4, 1,212 bytes (the WEB options and OPTION DVI added); OPTION DISK LOAD also takes 7.00's 992-byte files.
- **Tokens:** EOF( LOC( LOF( are one function token, EOFLOCLOF(, as BIT( BYTE( FLAG( are; SQL( is 0xFC, USER( 0xFD, and 0xFE is free. The image format is 7, and the flash library is now checked for it at start-up and by LIBRARY DISK LOAD.
- **TIMER** is an offset on the microsecond count (GetuSec), not a rewrite of the hardware timer.

### The programs

CMM2_Programs_8.00.00b1.zip has the same programs as the RC6 zip as it has been since 16 November (with the UTF-16 sql.mod), and they run on 8.00.00b1 unchanged, on the Generation 3 as well. README.txt is new, for this version, and the licences folder has the licences of the firmware's network code.

### Tested

- **Generation 3:** every mode on two HDMI monitors, OPTION PICOMITE's resolutions and modes, the emulators and Doom; the sound through HDMI checked bit for bit (65,536 frames identical to the file); OPTION DVI; the regression tests as on a Generation 2.
- **Generation 1 and Generation 2:** the full regression (the test programs, the PicoMite tests, the crash tests, the program corpus and SP Tools) with the same results as before, every video mode and the sound.
- **WiFi** on all three generations: joining, scanning, ping, the time, the web server (several clients at once), HTTP and HTTPS clients (with the certificates checked by WEB TLS CA), UDP, MQTT, TFTP and the Telnet console; the firmware with OPTION WIFI set and no module, and without the option; the regression run again with the network up, with no change but the network tests. Retic ran on a Generation 3 with its web pages in use from a PC.
- **USER modules** on a Generation 1 and a Generation 3.
- **The update from 7.00.00RC6** on a Generation 2, as a user does it: RC6 with a library using EOF(, LOF( and INPUT$(, its options saved; 8.00.00b1 written without a full erase; the options loaded, the library discarded with the message above, its .lib file refused, and the library loaded again from its source and working.

### Known problems

- **Bluetooth:** the RM2 module also has Bluetooth, which is not used yet.
- **The WebMite's own example programs** have not all been tried yet; the chapter's examples and Retic have.
- **A Generation 3 can reset when its HDMI cable is unplugged** (the cable's shield meeting the monitor's ground on the reset line). Switching the monitor off and on is fine. The next board has a fix.
- The network waits while a loadable program runs.
- The known problems of 7.00.00RC6 remain, apart from the three SQL faults, now fixed.

### The manuals

- **The User Manual** is for 8.00.00b1: the new chapter WiFi and Internet Functions, with every WEB command, option and function in its references; Running Programs (programs in RAM, kept over an option's restart, AUTORUN.BAS); the Generation 3's HDMI in Hardware Features; the new memory map; OPTION DVI, WATCHDOG HW, MM.INFO(BOOT COUNT), OPTION COUNT, USER and USER(, SQL(LONGSTRING, MODE 19; OPTION FLASH and OPTION RAM removed.
- **Colour Maximite 2 Loadable Programs** adds the chapter USER Modules, build.py --user and services version 20.
- The App User Guide, the C Compiler guide and the 386 PC Emulator guide are unchanged apart from the zip's name.

### Licences

The firmware's own licence (LICENSE) is unchanged. The network code brings lwIP (BSD-3-Clause), Mbed TLS (Apache-2.0) and George Robotics' cyw43-driver, each with its licence beside it in Middlewares/Third_Party, and Infineon's firmware for the CYW43439, which the driver loads into the chip. Their licence texts are also in the licences folder of the zip.
