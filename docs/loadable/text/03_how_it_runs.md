# How a Program Runs

This chapter follows a program from RUN to its end: how the firmware loads it, the memory it has, how it starts and stops, and what MMBasic puts back afterwards.

## RUN

`RUN "name.app" [, words]` runs a loadable program. MMBasic looks at the name: only a name that ends in .APP (in any case) is taken as a loadable program, so RUN of a BASIC program is unchanged. The name can be a string expression (`RUN f$`), and it is found as any file is: on the current drive and in the current directory unless it says otherwise. The BASIC program in program memory is not touched.

Before loading, MMBasic stops any sound that is playing. It then reads the file's header and checks it:

{widths: 38,62}
| Error | Why |
|---|---|
| Not a loadable program | The file does not start as a program file does. |
| Not a program this firmware can load | The header is of another version, or the file's size does not match it. |
| Needs a newer firmware | The program uses a later version of the services than this firmware has (its --api option). |
| Needs a G2 | A program built with --g2, on a Generation 1. |
| Needs n KB, n KB free | Generation 1: its code, data, stack and least heap do not fit in the memory that is free. |
| Needs n KB, more than the 8 MB | Generation 2: they do not fit in the 8MB. |
| Cannot lend the display memory | Generation 1, a program built with --vram: the screen could not be changed to free the display memory (MMBasic has a frame buffer of another colour depth). |
| The program file is damaged | The CRC of the code and the relocations does not match the header's. |

These are ordinary MMBasic errors, so a BASIC program can catch them with ON ERROR SKIP and MM.ERRNO.

The program's code and data are read straight into their place. If that is not the address the program was linked for, the program is moved: each address in it is adjusted by the difference, using the list of them in the file (the relocations: see Appendix B). Its .bss (its variables without initial values) is cleared to zero, the caches are made to agree with the memory, and the program is called.

## The Memory on a Generation 2

A Generation 2 sets aside the top 8MB of its SDRAM, 0xD1800000 to 0xD1FFFFFF, for loadable programs. A program is linked for 0xD1800000, so it is loaded there and nothing in it is moved:

{widths: 28,72}
| Address | What is there |
|---|---|
| 0xD1800000 | The program: its code, constants and initialised data (the image). |
| after the image | Its .bss, cleared to zero. |
| after the .bss | Its heap, for malloc() and new: everything up to the stack. |
| the top 64KB | Its stack (the default: --stack sets another size). |

The heap's address and size are in cmm2_start->heap and cmm2_start->heap_size. For a small program the heap is a little under 8MB.

The memory is cached and executable, and runs at the SDRAM's speed through the processor's 16KB instruction cache and 16KB data cache (see the chapter Performance).

## The Memory on a Generation 1

A Generation 1 has 8MB of SDRAM in all, most of it used by the display, so it cannot set any aside. Instead a program borrows the largest free part of MMBasic's heap while it runs: all of it but 256KB, which is left for the services and MMBasic. The program is put at the first 64KB boundary in it and moved there by its relocations, and that part of the heap is made executable while the program runs. When the program ends it is given back and the heap is as it was.

At the command prompt this gives a program about 4.2MB (MicroPython has a 2.3MB heap after its own code and buffers); run from a BASIC program it is what the BASIC program's variables leave free. So a program should never assume the size of its memory: it should size its buffers from cmm2_start->heap_size, as Doom sizes its zone and MicroPython its heap and its SQLite pool.

The program file says the least heap the program can run with (build.py's --heap option). If the memory there is cannot give it, RUN refuses the program before loading it ("Needs n KB, n KB free"), rather than the program failing later.

## The Display Memory Lent

A program built with --vram (version 8) is lent the display memory as well: the 3MB at 0xD0000000 to 0xD02FFFFF that holds MMBasic's video pages. As the program starts, the screen is changed to MODE 7 (320 x 240), whose page is in the processor's internal RAM, so nothing is drawn in the display memory while the program has it.

The display memory and the bottom of MMBasic's heap are normally next to each other, and the program then has the two as one area, laid out as on a Generation 2: its image at 0xD0000000, then its .bss, its heap and its stack. At the command prompt that gives a small program a heap of about 7.2MB. ScummVM, whose image is 3MB, has about 4.2MB of heap, and Doom a 6MB zone.

With OPTION RAM or OPTION COMPILE ON, MMBasic keeps memory between the two, and the program has two blocks instead. It is loaded into the display memory if its image, .bss and stack fit there, and into the borrowed part of MMBasic's heap if not. Each block's free memory is a heap: the larger is cmm2_start->heap, which malloc() uses, and the other is cmm2_start->heap2 (with its size in heap2_size, or NULL if there is none). A program that needs one large buffer can take it from heap2; one that knows nothing of heap2 just has less memory.

While the program runs, the screen can be set only to a mode whose page fits in the internal RAM's 512KB, and video_mode and video_scaled return -1 for the others. In 8-bit colour that is every mode but 9, 11, 12, 15, 16 and 18; in 16-bit colour, modes 2 to 7, 13 and 17. video_scaled at 2 halves a page, so in 8-bit colour MODE 9, 11 and 18 can be used at half their width and height. Doom (MODE 3) and ScummVM (MODE 7) are not limited by this.

The display memory is executable while the program runs. When the program ends it is cleared and made non-executable again (MMBasic keeps it that way, for the fastest drawing), and MMBasic's video mode is set again. On a Generation 2 --vram changes nothing: the program has its 8MB.

## The Start

The firmware calls the program's entry point, _start in crt0.c, on the program's own stack, with the start block:

```
typedef struct {
    const struct cmm2_services *svc;    /* the services */
    int      argc;
    char   **argv;                      /* the file's name, then RUN's words */
    void    *heap;                      /* .bss to stack, 8-byte aligned */
    uint32_t heap_size;
    uint32_t base;                      /* where the program was loaded */
    void    *heap2;                     /* a second heap, or NULL (version 8) */
    uint32_t heap2_size;
} cmm2img_start;
```

heap2 is only ever set for a program built with --vram, on a Generation 1 (see The Display Memory Lent above). Firmware before version 8 has a shorter start block, so a program reads heap2 only when it was built with --api 8 or later.

_start keeps the services in the variable cmm2 and the start block in cmm2_start, runs the C++ constructors (and C functions marked constructor), and calls main(argc, argv).

argv[0] is the program's name as RUN was given it ("first.app" or "A:/apps/first.app"). The words after the comma follow, split at spaces; a word in double quotes can hold spaces. The program's current directory is MMBasic's, so a program run with `CHDIR "A:/Doom" : RUN "doom.app"` finds its files in A:/Doom without a path. Doom, for one, looks for its game data beside its own file, using argv[0].

## The Stack

A program runs on a stack of its own at the top of its memory (the processor's process stack), 64KB unless build.py's --stack option says otherwise. The firmware's interrupts run on MMBasic's stack, in the processor's fast internal memory, so they do not use the program's. Nothing checks for the stack running over: a stack that grows past its size overwrites the top of the heap without any error. A program with deep recursion or large local arrays should ask for a larger stack.

## The End

A program ends by returning from main(), by calling exit(), or by calling the leave service from any depth:

```
cmm2->leave(code);
```

Returning from main() and exit() run the C++ destructors and the functions registered with atexit() and flush the C library's files. leave() goes straight back to MMBasic from wherever it is called (it is the program's longjmp to the firmware), without them. In each case the value is the program's exit code, and MMBasic prints "name.app ended with code n" if it is not zero.

MMBasic then tidies up after the program:

- a frame hook is removed and the DMA2D's copies stopped;
- the I/O pins the program set up are returned to unconfigured, its PWM outputs stopped and its I2C and SPI buses and COM ports closed;
- its sound and music are stopped;
- its files and directories are closed, and the blocks it took from MMBasic's heap with the alloc service are freed;
- on a Generation 1 the memory it borrowed is given back, and the display memory, if it was lent, is cleared and made non-executable again;
- the keys typed while it ran are thrown away, so they are not taken as commands;
- if it changed the video mode, MMBasic's mode, font and colours are set again, and the palette (the 256 colours of an 8-bit mode) is put back as MMBasic had it.

The program's memory is not cleared: a program run next starts with its own image and a cleared .bss.

## Ctrl-C

Ctrl-C, from the serial console or a USB keyboard, sets MMBasic's break flag. From then on the poll service returns 1, and the program should end. When it has ended, MMBasic stops the BASIC program that ran it too (as Ctrl-C stops a BASIC program), so a BASIC loop that runs programs one after another is stopped as well.

A program that uses Ctrl-C itself (MicroPython turns it into a KeyboardInterrupt) calls break_take instead, which returns 1 once for each Ctrl-C and clears the flag, so that the BASIC program carries on afterwards.

The serial console's Ctrl-C is seen at once. A USB keyboard's is seen only when the program calls poll, as are its other keys.

## The Rules

A loadable program runs with nothing between it and the hardware: it can read and write any memory and any peripheral. That is what makes it fast, and it means that a mistake is not caught as an error. A fault (a bad pointer, a jump to nowhere) restarts the Colour Maximite 2, as a fault in the firmware does. The BASIC program in program memory, the options and the files on the card are not affected. So that the program and MMBasic work together:

- **Use the services** for the hardware MMBasic uses: the console, the files, the screen, the keyboard and mouse, the sound and the I/O pins. They keep MMBasic's view of the hardware right, and MMBasic undoes what they did when the program ends.
- **Call poll() often**, at least every 10 to 20ms (each frame of a game is right). The USB devices, the keyboard and the mouse included, are looked after only when poll() is called, and a USB keyboard's Ctrl-C is seen then.
- **Leave MMBasic's memory alone.** The program's own memory is its image, its heap and its stack, and the blocks it takes with the alloc service. MMBasic's variables, its heap and the internal RAM belong to MMBasic.
- **Leave the processor's set-up alone**: the interrupt vectors, the interrupt priorities, the clocks, the MPU and the caches. The DMA2D is the exception: the firmware does not use it while a program runs, and the dma2d_copy service drives it.
- **Keep code that runs in an interrupt short**, and call no services from it but the DMA2D's (see Code Called From an Interrupt in the chapter Sound and Music).
