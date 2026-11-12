# Getting Started

This chapter builds a first program, puts it on the SD card and runs it.

## Setting Up the PC

Install Python 3 (3.8 or later) and the Arm GNU toolchain for arm-none-eabi, version 13.3: Arm's own (Arm GNU Toolchain 13.3.Rel1, from developer.arm.com) or the one that comes with ST's STM32CubeIDE. Then get the firmware's repository:

```
git clone https://github.com/UKTailwind/CMM2V1.5
```

The tools are in its tools/sdk directory. They find the compiler in the folder named by the environment variable ARM_TOOLCHAIN_DIR, if it is set, or else through the PATH. On Windows, for example (with the folder that holds arm-none-eabi-gcc.exe):

```
setx ARM_TOOLCHAIN_DIR "C:\Arm GNU Toolchain\13.3 rel1\bin"
```

## A First Program

Make a file called first.c in tools/sdk:

```
#include <stdio.h>
#include "cmm2.h"

int main(int argc, char **argv) {
    int i;
    printf("Hello from a loadable program at %lu MHz\n",
           (unsigned long)(cmm2->cpu_hz() / 1000000));
    for(i = 0; i < argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);
    printf("My heap: %lu KB\n", (unsigned long)(cmm2_start->heap_size / 1024));
    return 0;
}
```

It is ordinary C. printf() goes to the console: the serial port and the screen, as MMBasic's PRINT does. cmm2.h gives the program two variables: cmm2, the table of the firmware's services (here its cpu_hz service), and cmm2_start, what the firmware passed to the program as it started it (here the size of its heap).

## Building It

In tools/sdk:

```
python build.py out/first.app first.c
```

This prints:

```
compiling 3 of 3 sources, 24 at a time
out/first.app: 12540 bytes of image at 0xD1800000, 348 of .bss, entry +0x1, 152 relocations (1 absolute left alone), CRC DB38C447
```

build.py compiled first.c with two files of the tools, crt0.c (the program's start) and syscalls.c (what joins the C library to the services), linked them for the address where a Generation 2 loads a program, and wrote the program file out/first.app (the sources are compiled in parallel, as many at a time as the PC has processor threads). The ELF file and a map of the program are beside it (out/first.elf and out/first.map), for debugging.

## Copying It to the SD Card

Copy out/first.app to the SD card with a card reader, or send it over the serial console. To send it with ZMODEM, type this at the Colour Maximite 2's command prompt:

```
ZMODEM RECEIVE
```

and then send the file from your terminal emulator (in Tera Term: File, Transfer, ZMODEM, Send). The repository also has a small Python sender: `python tools/zmodem_pc.py send COM4 115200 tools/sdk/out/first.app` (with your serial port and baud rate). XMODEM RECEIVE "first.app" works too.

## Running It

At the command prompt:

```
RUN "first.app", hello world
```

The program prints:

```
Hello from a loadable program at 480 MHz
argv[0] = first.app
argv[1] = hello
argv[2] = world
My heap: 8115 KB
```

and MMBasic's prompt returns. On a Generation 1 the heap is about 4MB, at 400MHz. Change the return statement to return 3, build and run it again, and MMBasic prints "first.app ended with code 3": a program's exit code is shown when it is not zero.

## The Examples

tools/sdk/examples has programs to read and build on:

{widths: 22,78}
| Example | What it shows |
|---|---|
| hello.c | The console, the arguments, the heap, the clock, files (written, read back and removed), a directory listing, timing, and leaving from inside a function with leave(). |
| vidtest.c | The screen and the keyboard: a video mode, 256 colours from the palette, timing frames, turning the palette and reading the keys held. Built with --api 2. |
| nopoll.c | What happens to the USB devices when a program does not call poll(). |
| oplbench.c | The firmware's FM synthesiser against Nuked-OPL3 compiled into the program (LGPL: see Appendix C). |
| oplplay | Renders MIDI, IMF, DRO and VGM music to a WAV file through three different OPL emulators, timing each. |

They are built in the same way, for example:

```
python build.py out/hello.app examples/hello.c
python build.py out/vidtest.app examples/vidtest.c --api 2
```

The repository's apps directory holds the build scripts of the larger programs: apps/doom/make.py and apps/scummvm/make.py fetch the original source code and build doom.app and scummvm.app from it with the files in their directories.
