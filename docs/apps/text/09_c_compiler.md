# The C Compiler

The Colour Maximite 2 can compile programs on the machine itself, with no PC. The CC command compiles a program written in C. It also compiles an MMBasic program, so that a program that spends its time calculating runs much faster.

This chapter covers everyday use. The full details are in its own manual, *Colour Maximite 2 C Compiler*, posted with the firmware.

## Installing It

The cc folder has to be at A:/cc. CC looks for it there, whatever the current folder is. It needs firmware 7.00.00RC2 or later.

{widths: 16,36}
| File | What it is |
| A:/cc/cc.app | The compiler |
| A:/cc/include | The header files that C programs include |
| A:/cc/examples | Two example programs, shapes.c and blink.c |

A compiled program needs nothing from A:/cc to run, so you can pass it on by itself.

## When It Is Worth Using

{widths: 24,28}
| Your program | What to use |
| An MMBasic program that is fast enough | Nothing: leave it as it is |
| An MMBasic program that spends its time calculating | CC, which is much the fastest way to run it |
| An MMBasic game that spends its time drawing | Try OPTION COMPILE ON first. CC gains less here, as the drawing takes the same time however the program is run |
| Something new that has to be as fast as it can be | Write it in C |

## Compiling an MMBasic Program

Your BASIC program is not changed. RUN "game.bas" still runs it with the interpreter, and the compiled copy sits beside it in a file of its own. Change to the program's folder first, as a game finds its pictures and sounds from there:

```
CHDIR "A:/game"
CC "game.bas"
RUN "game.b.bc"
```

CC prints a row of dots as it works. A program of about 3,000 lines compiles in 7 seconds. The largest tried so far, 4,300 lines, takes about a minute and a half, and needs a Generation 2.

If you change game.bas, run CC again. RUN "game.b.bc" does not check that it is up to date.

### Read What CC Reports

If CC cannot translate a statement it does not stop. **The statement is left out**, and CC reports the line and the reason. Nothing runs in its place, so a program with a line left out may run and do the wrong thing. Read the report, particularly if the line left out opened a block.

### Check the Result

A compiled program should behave as the original does, but yours may do something that has not been tried. Run the BASIC program and the compiled one, and check that they agree before you rely on it.

A compiled program is faster, so a game that paces itself by how long its own code takes will run too fast. It needs a timer in its main loop.

## Compiling a C Program

Type this into a file called hello.c, with EDIT or on a PC:

```
#include <stdio.h>

int main(void)
{
    printf("Hello from C, %d\n", 6 * 7);
    return 0;
}
```

Then, in the same folder:

```
CC "hello.c"
RUN "hello.bc"
```

CC makes hello.bc beside hello.c, in about a second. MMBasic carries on when the program ends, and Ctrl-C stops a program as it does in BASIC.

RUN can do both steps. RUN "hello.c" compiles the file if hello.bc is missing or older than it, and then runs it, so editing a program and trying it again is one step.

Words after a comma are the program's arguments, in argv[1] onward:

```
RUN "hello.bc", one two
```

### The Language

The language is C89 (ANSI C) with long long, // comments and declarations after statements. A program is one source file, as there is no linker. Bit fields are not supported, nor are declarations inside a for statement.

The usual stdio.h, stdlib.h, string.h, math.h, ctype.h and time.h are there. There is no scanf. Files have MMBasic's names, such as A:/dir/name.txt.

cmm2.h gives a C program the Colour Maximite 2's own facilities: the video modes, drawing with MMBasic's own routines, the keyboard, the mouse, sound and the pins:

```
#include <cmm2.h>

int main(void)
{
    cmm2_video v;

    video_mode(1, 8, &v);
    gfx_cls(BLACK);
    gfx_circle(400, 300, 100, 2, 1.0, GREEN, RED);
    gfx_text(400, 300, "Hello", "CM", 1, 2, WHITE, NOFILL);
    return 0;
}
```

## CC's Options

CC takes a second argument, a string of options:

{widths: 12,40}
| Option | What it does |
| -v | Shows each pass of the compiler as it runs |
| -k | Keeps the intermediate files |
| -o name | Gives the object file another name |

For example:

```
CC "hello.c", "-o greet.bc"
```

## How Much Faster

A 3,200-line calculation, on a Generation 2 at 480MHz:

{widths: 26,13,13}
| How it ran | Time | Compared with the interpreter |
| The interpreter | 6.16 seconds | 1 |
| OPTION COMPILE ON | 1.71 seconds | 3.6 times faster |
| CC | 0.93 seconds | 6.6 times faster |

Games gain less, as they spend much of their time drawing. Try OPTION COMPILE ON first: it needs no extra step and works on any program.

## When Things Go Wrong

{widths: 22,30}
| What you see | What to do |
| CC cannot find the compiler | cc.app has to be in A:/cc. Copy the cc folder from the zip to the card, and check its name |
| Cannot open include file stdint.h | The files in A:/cc/include are from an older version. Copy the whole cc folder from the zip that came with your firmware |
| x.c did not compile | The lines before it give the file, line number and error. A missing semicolon is reported on the next line |
| A compiled BASIC program does the wrong thing | Look at what CC reported. A line it could not translate is left out |
| A game runs too fast | It needs a timer in its main loop, as the compiled program is faster |
| not enough memory for its tables, jump out of range, code segment full | The program is too large for the compiler on a Generation 1. Compile it on a Generation 2 |
| The compiled program cannot find its files | CHDIR to the program's folder before RUN |
