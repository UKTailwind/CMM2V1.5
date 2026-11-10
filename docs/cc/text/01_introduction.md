# Introduction

The Colour Maximite 2 can compile programs on the machine itself, with no PC. The CC command compiles a program written in C, and it also compiles an MMBasic program, so that a game or a calculation you already have can run faster with no change to it.

```
CC "hello.c"
RUN "hello.bc"
```

```
CC "game.bas"
RUN "game.b.bc"
```

Your BASIC program is not changed. RUN "game.bas" still runs it with the interpreter as it always did, and the compiled copy sits beside it in a file of its own.

## When it is worth compiling

| Your program | What to use |
|---|---|
| An MMBasic program that is fast enough | Nothing: leave it as it is. |
| An MMBasic program that spends its time calculating | CC. It is much the fastest way to run it. |
| An MMBasic game that spends its time drawing | Try OPTION COMPILE ON first. CC gains less here, as the drawing takes the same time however the program is run. |
| Something new that has to be as fast as it can be | Write it in C, with CC, or as a loadable program on a PC. |

The chapter Speed has the figures.

## What you need

- A Colour Maximite 2, Generation 1 or 2, with firmware 7.00.00RC2 or later.
- The cc folder of CMM2_Programs_7.00.00RC5.zip, copied to the SD card as A:/cc.

The firmware holds the part that runs a compiled program, so a compiled program needs nothing from the card to run. A:/cc is needed only to compile.

## Where it comes from

The compiler is the Fuzix Compiler Kit, with the bytecode and Arm (Thumb-2) code generator written for the Pico Computer 3. The translator from MMBasic to C was written for the same machine. Both were brought to the Colour Maximite 2 by Peter Mather. The Appendix gives their licences and where the source is.
