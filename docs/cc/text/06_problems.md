# Problems

## CC says it cannot find the compiler

cc.app has to be in A:/cc. Copy the cc folder from the programs zip to the SD card, and check its name.

## "Cannot open include file stdint.h"

The header files in A:/cc/include are from an older version. Copy the whole cc folder from the zip that came with your firmware.

## "x.c did not compile"

The lines before it give the file, the line number and the error. If the error is on a line that looks right, look at the line before it: a missing semicolon is reported on the next line. Check the list of what is not supported in the chapter C Programs.

## A compiled BASIC program does the wrong thing

Look at what CC reported when it compiled. A line it could not translate is left out, and the program then runs without it. If nothing was reported, run the BASIC program beside it to find where they part, and please report it on the Back Shed forum with the program.

## A game runs too fast

The compiled program is faster than the interpreter. A game that paces itself by how long its own code takes needs a timer: wait for the TIMER, or for the next frame, in its main loop.

## "not enough memory for its tables", "jump out of range", "code segment full"

The program is too large for the compiler's tables. This is only likely on a Generation 1 (see Very large programs in the chapter MMBasic Programs). Compile it on a Generation 2.

## CC seems to be taking a long time

CC prints a row of dots as it compiles. A large program takes a minute or more, and the dots can stop for some time while it works on the largest functions.

## The compiled program cannot find its files

A program looks for its files from the current directory, as a BASIC program does. CHDIR to the program's folder before RUN.
