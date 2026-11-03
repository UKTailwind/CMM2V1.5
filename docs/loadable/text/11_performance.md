# Performance

The STM32H743 is fast, and a loadable program has all of it: no interpreter, no operating system, and the firmware's interrupts take only a little of its time when nothing else is going on. A few things about the way it is built decide how fast a program runs.

## Code and Data in the SDRAM

A program runs from the SDRAM, as its data is. The SDRAM is much slower than the processor, so the speed comes from the processor's two caches: 16KB for instructions and 16KB for data. A loop that fits in the instruction cache and works on data that fits in the data cache runs at the processor's full speed; code that jumps all over a large program, or data read once from a large area, runs at the SDRAM's.

Some consequences:

- **Measure, don't guess.** The same code can run at a different speed when a change elsewhere moves it in memory. When MicroPython was ported, adding a module that its benchmark never called made the benchmark 31% slower, by moving its interpreter's loop.
- **-O2 is usually best.** -O3 and loop unrolling make the code larger and often slower for the cache; -Os makes it smaller but slower in the loops that matter. Try --cflag=-O3 on a single hot file rather than on everything.
- **Keep hot data together.** Arrays of structures that are walked in order use the cache well; linked lists and tables of pointers scattered across a large heap do not.
- **The display memory is not cached**, so build a picture in the program's memory and copy it to the page (with memcpy() or the DMA2D) rather than drawing into the page a pixel at a time.

The processor's fast internal memories (ITCM and DTCM) are the firmware's and are not available to a program.

## Floating Point

The floating point unit does single and double precision in hardware: an addition or a multiplication takes a cycle or two, and a division or a square root about 14 cycles in single precision and 30 in double. So float and double cost about the same, except for division, square roots and the functions of math.h, where float is quicker (sinf() and the other float versions are quicker than sin()).

## The Interrupts

While a program runs the firmware still does its work in interrupts: the USB host, the console's serial port, the video's frame interrupt, and the sound while it plays. Without sound this takes very little. Sound takes more: the sample interrupt runs 44,100 times a second, and the program's fill and the FM synthesiser's music take what they take. Doom's timed demo runs at 97 frames a second silent and at 51 with its music and effects, the music at half its full rate.

## Some Figures

On a Generation 2 at 480MHz:

{widths: 55,45}
| What | Speed |
|---|---|
| Doom, timed demo 1, silent | 97 frames a second |
| Doom, with music at half rate and effects | 51 frames a second |
| MicroPython, the solar eclipse benchmark | 4.4 seconds (8.8 on a Pico Computer 3; MMBasic 6.1 interpreted, 1.7 with OPTION COMPILE ON) |
| The FM synthesiser, 18 voices sounding | 54% of the processor |

A Generation 1 at 400MHz runs a little slower than its clock alone would suggest: Doom's demo runs at 78 frames a second silent and 35 with sound.
