# The C Library

A loadable program has the standard C library (newlib-nano, or the whole newlib with build.py's --libc full), with dlmalloc as its malloc, and the maths library. syscalls.c joins them to the firmware's services, so the usual functions work: printf and the rest of stdio, files through fopen, malloc and free, time(), and the functions of string.h, stdlib.h, ctype.h and math.h. This chapter says how each part behaves on the Colour Maximite 2.

## The Console

stdout and stderr go to the console: the serial port and the screen, as MMBasic's PRINT does, with each newline sent as a carriage return and line feed. stdout is buffered by the C library until a newline (or fflush(stdout)). A program with a display of its own calls `cmm2->con_screen(0)`, after which the console's output goes to the serial port only and is not drawn over the program's picture.

stdin reads the keyboard: the serial console and a USB keyboard (whose keys come with MMBasic's codes: see Appendix I of the user manual). A read waits for a key, looking after the USB devices while it waits, shows the key, and gives the Enter key as a newline. There is no line editing: a Backspace is read as the character 8. So scanf() and fgets() on stdin work for simple input, and a program that wants keys as they are pressed, without waiting, uses the services: con_getc for typed characters and keys_held for the keys held down (see the chapter The Keyboard and Mouse).

## Files

fopen(), fread(), fwrite(), fprintf(), fgets(), fseek(), ftell(), fclose(), remove(), rename() and mkdir() work on the SD card and the USB flash drive. File names are MMBasic's:

{widths: 30,70}
| Name | Is |
|---|---|
| A:/dir/name.txt | On the SD card. |
| C:/name.txt | On the USB flash drive. |
| /dir/name.txt | From the root of the current drive. |
| name.txt or dir/name.txt | In or below the current directory (MMBasic's, as CHDIR set it). |

The file system is FAT (FAT16, FAT32 or exFAT) with long names, and names are not case sensitive. Up to eight files can be open at once. rename() replaces a file that is there already, as POSIX's does. A file opened for reading, or for reading and writing in place (without creating or appending), is given a map of where its parts are on the card, so that seeking in it is fast: Doom reads its 28MB game data file by seeking to each part, and the 386 PC emulator seeks in its disk images. A write or seek past the end of the file drops the map, as the file then grows.

The POSIX calls under stdio work too: open(), creat(), read(), write(), lseek() and close(), with descriptors 0, 1 and 2 the console and 3 upwards the files. dup2(fd, 0), dup2(fd, 1) or dup2(fd, 2) puts a file in the console's place as standard input, output or error, as a shell's < and > do, so that a program written as a Unix filter reads and writes files; closing that descriptor gives the console back. The C compiler's passes run that way.

opendir(), readdir() and closedir() list a directory, with "." first and then, below a drive's root, "..", as Linux gives them. stat() gives a file's size, kind and date, rmdir() removes an empty directory, and chdir() and getcwd() are MMBasic's CHDIR and CWD$. The services' file functions (see the chapter The Services) can be used directly too, without stdio's buffering.

A call that fails sets errno: ENOENT for a name or path that is not there, EEXIST for one that is (mkdir() of a drive's root too, as POSIX has it), EACCES for a file that is read-only or a directory that is full or not empty, EROFS for a write-protected card, EMFILE when eight files are open, ENODEV for a drive with no card or no file system, and EIO for the card itself.

## Memory

malloc(), calloc(), realloc(), free(), and C++'s new and delete use the program's heap: all the memory between its .bss and its stack (cmm2_start->heap and cmm2_start->heap_size). When that is used up, malloc() goes on in blocks of MMBasic's own heap, taken with the alloc service and freed when the program ends. On a Generation 2 at the command prompt that is about 15MB more, besides the program's 7MB or so; a Generation 1 has already lent the program nearly all of its heap. The blocks are of 1MB, or of the size asked for if that is more, and 512KB of MMBasic's heap is always left free, for the services and MMBasic. malloc() returns NULL when there is no more.

The allocator is dlmalloc (Doug Lea's, version 2.8.6, in tools/sdk/dlmalloc.c and malloc.c), with either C library: newlib's own is left out. It keeps its free blocks in bins by size, so malloc() and free() take about the same time however many blocks a program has. (newlib-nano's free() searched a list of every free block: WordGrinder, freeing the 350,000 objects of a Lua state that held a dictionary, did not finish.) mallinfo() counts all of the memory: arena is what malloc() has been given, uordblks the part in use, and fordblks the part free for reuse.

A program that has its own memory manager, or needs one large block, takes it from the top of the heap with:

```
void *cmm2_heap_take(uint32_t size);
```

which returns the block (8-byte aligned) or NULL, and leaves malloc() the rest. Call it before malloc() has used much of the heap. MicroPython takes its garbage-collected heap this way, and ScummVM all of the heap but 512KB for its allocator to manage. A second heap that a Generation 1 may give a program built with --vram (cmm2_start->heap2: see The Display Memory Lent in the chapter How a Program Runs) is not used by malloc(): it is the program's to use as it likes.

The alloc and release services give blocks of MMBasic's heap instead, outside the program's memory, up to 64 of them. They are freed when the program ends. A program rarely needs them; the FM synthesiser's state (opl_new) is kept in one.

## Time

time() and gettimeofday() read MMBasic's clock (DATE$ and TIME$) the first time, and add the microseconds since, so that they never go back. localtime() and strftime() work, with no time zone: the clock's time is taken as local. clock_gettime(CLOCK_MONOTONIC), clock() and times() count the microseconds since the computer started, and nanosleep() waits as usleep() does. For timing, the micros service is simplest: a 64-bit count of microseconds since the computer started.

```
uint64_t t0 = cmm2->micros();
work();
printf("%lu us\n", (unsigned long)(cmm2->micros() - t0));
```

delay_us waits a number of microseconds. It does not look after the USB devices, so a long wait should call poll() as it goes:

```
uint64_t end = cmm2->micros() + 500000;
while(cmm2->micros() < end) cmm2->poll();
```

## Numbers

float and double are done in hardware, and the maths library (sin, sqrt, pow...) is there for both. printf() and scanf() handle floating point (%f, %g, %e). 64-bit integers (long long, int64_t) work, with division done in software.

## What Is Not There

There is no operating system, so there are no processes (system(), fork(), exec()), threads, signals or sockets. No environment variables are set at the start (getenv() returns NULL), but setenv() sets one for the program to read: a library that is configured by them, as PDCurses is, can be given them so. A program runs alone, with the firmware's interrupts under it, until it ends.
