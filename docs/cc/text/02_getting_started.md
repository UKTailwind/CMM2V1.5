# Getting Started

## Installing it

Copy the cc folder from CMM2_Programs_7.00.00RC3.zip to the SD card, so that the card has:

| File | What it is |
|---|---|
| A:/cc/cc.app | The compiler. |
| A:/cc/include | The header files that C programs include. |
| A:/cc/examples | Two example programs, shapes.c and blink.c. |

It has to be in A:/cc. CC looks for it there, whatever the current directory is.

## Your first C program

Type this into a file called hello.c, with EDIT or on a PC:

```
#include <stdio.h>

int main(void)
{
    printf("Hello from C, %d\n", 6 * 7);
    return 0;
}
```

Then, in the same directory:

```
CC "hello.c"
RUN "hello.bc"
```

CC makes hello.bc beside hello.c. It takes about a second, and prints a row of dots as it works. RUN then runs it, and MMBasic carries on when the program ends. Ctrl-C stops a program, as it does in BASIC.

RUN can do both steps. RUN "hello.c" compiles the file if hello.bc is missing or older than it, and then runs it, so that editing the program and trying it again is one step.

Words after a comma are the program's arguments, in argv[1] onward:

```
RUN "hello.bc", one two
```

## Your first compiled BASIC program

Any MMBasic program can be tried. CHDIR to its folder first, as a game finds its pictures and sounds from there:

```
CHDIR "A:/game"
CC "game.bas"
RUN "game.b.bc"
```

CC translates the program to C, in game.mb.c, and compiles that to game.b.bc. It prints "wrote game.mb.c" and then the dots. Any line it could not translate is reported as it goes (see the chapter MMBasic Programs).

A program of about 3,000 lines compiles in 7 seconds. The largest we have tried, 4,300 lines, takes about a minute and a half.

## CC's options

CC takes a second argument, a string of options for the compiler:

| Option | What it does |
|---|---|
| -v | Shows each pass of the compiler as it runs. |
| -k | Keeps the intermediate files. |
| -o name | Gives the object file another name. |

For example:

```
CC "hello.c", "-o greet.bc"
```
