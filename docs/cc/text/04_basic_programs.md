# MMBasic Programs

CC translates an MMBasic program to C and compiles the C. This is not OPTION COMPILE, which speeds up the interpreter as it runs: CC makes a separate compiled program, and needs no OPTION.

## What it does with your program

- game.bas is read, and any #INCLUDE files are read in as MMBasic would.
- game.mb.c is written: the translation, in C. CC removes it afterwards unless you give it the option -k.
- game.b.bc is written: the compiled program. Run this with RUN "game.b.bc".

If you change game.bas, run CC again. RUN "game.b.bc" does not check that it is up to date.

## What it translates

Almost all of MMBasic is translated into C: the arithmetic, strings, arrays, structures, SUBs and FUNCTIONs, files, the drawing commands, and DATA and SORT.

The Colour Maximite 2's larger graphics and sound commands are handed to the firmware to run: PAGE, IMAGE, MODE, BLIT, LOAD PNG, LOAD BMP, LOAD JPG, LOAD IMAGE, PLAY MODSAMPLE, PLAY EFFECT, PLAY MODFILE, FRAMEBUFFER and POKE VAR. Their arguments are compiled, and the command itself is prepared once and not again each time it runs. These take the same time as they do in BASIC, because it is the same code doing the work.

## What it does not translate

If CC cannot translate a statement it does not stop. The statement is left out, and CC reports the line and the reason. The translation has a comment in its place:

```
/* MMBASIC line 949 not translated: array rank mismatch */
```

Nothing runs in its place, so **read the report**, particularly if the line left out opened a block. A program with a line left out may run, and do the wrong thing.

Some things are refused with an error that names the line:

- LONGSTRING AES128 and the LONGSTRING BASE64 forms;
- STRUCT SORT, SAVE, LOAD, PRINT, EXTRACT and INSERT, STRUCT(FIND), a function that returns a structure, and arrays of structures as parameters;
- an array of structures whose size can change: one that is REDIMmed, or DIMmed with bounds that are not constants (the error is "an array of a TYPE needs constant bounds", at its DIM);
- a GOSUB inside a SUB to a label in the main program;
- RESTART, and the SAVE and RESTORE forms of a few drawing commands.

CC also gives warnings for things that work differently, such as an EXIT with no loop around it. Read them, but a warning does not stop the compile.

## Checking the result

A compiled program should behave as the original does. We have compared the output of a 3,200-line calculation byte for byte, and the pictures of two large games pixel for pixel, and they were the same. Your program may do something ours did not, so compare the two: run the BASIC program and the compiled one and check that they agree before you rely on it.

Programs that depend on the time they take can behave differently, because the compiled one is faster. A game that has no frame timer of its own will run too fast.

## Very large programs

A Generation 2 has 8MB for the compiler, and a Generation 1 has under 5MB. CC uses smaller tables on a Generation 1 when it has to, and a very large program can then stop with an error such as "jump out of range" or "code segment full". On a Generation 1, a 3,000-line program compiled in 7 seconds and a game of 72KB in about a minute. A game of 122KB, 4,300 lines, needs a Generation 2.
