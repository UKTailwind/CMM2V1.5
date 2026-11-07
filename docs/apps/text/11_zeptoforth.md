# zeptoforth

zeptoforth is Travis Bemann's Forth for ARM microcontrollers. On the Colour Maximite 2 it is a complete Forth system with preemptive tasks, double-precision arithmetic, a screen text editor, and words for the screen, sound, files and I/O pins.

## Why Forth

Forth is a very small language that you extend as you go. A program is a set of words. You build each word from the words you already have, and from then on it is part of the language, no different from the words that came with it. Numbers pass between words on a stack, so a word needs no declarations and no argument lists:

```
: square  dup * ;
7 square .
```

The first line defines a word that squares a number. The second puts 7 on the stack, squares it, and prints 49.

Forth is compiled and interactive at once. Each word becomes ARM machine code the moment you finish typing it, on the Colour Maximite 2 itself, with no PC and no build step, and you can try it at the prompt straight away. You build a program from the bottom up, testing each small word as you go, so a fault is usually in the last thing you typed.

The language is yours to change. The compiler is made of words too, so you can add control structures of your own or change how numbers are read. And nothing stands between you and the machine: words read and write any address, and there is an assembler when you need one.

## Starting It

Copy the zeptoforth folder to the card, then on a Generation 2:

```
CHDIR "A:/zeptoforth"
RUN "zeptoforth.app"
```

On a Generation 1:

```
CHDIR "A:/zeptoforth"
RUN "zeptoforth_g1.app"
```

Each has its own saved Forth system, zeptoforth.img or zeptoforth_g1.img, in the same folder. Keep them together.

## Leaving It

Type **bye** to go back to MMBasic.

**Ctrl-C** stops a word that is running and starts Forth again. What you compiled to RAM is lost, and what you saved is kept. Ctrl-C does not leave zeptoforth.

## The Console

The console is a terminal on the screen, the same one as the VT100 terminal program, so Forth's colours and editors work on the Colour Maximite 2 with no PC. It is also sent to the serial port, and you can type at either.

Shift with Page Up and Page Down, or the mouse wheel, looks back through what has scrolled off the screen.

The font suits the video mode. To change it, or the size of the console:

```
cmm2 import
4 console-font
80 24 console-size
```

console-font takes 0 to 5, from 5 x 8 to 16 x 32, or -1 to go back to the one that suits the mode. console-size takes the columns and lines, and 0 0 means all there is.

To recall and edit what you have typed with the arrow keys, type **enable-line**. Up and Down go through the lines you typed before. disable-line turns it off, as you should before sending a file to the console from a PC.

Programs that ask how big the terminal is get the size of the screen, so a terminal program on a PC should be set to the same size, or use console-size.

## Editing Files

zeptoed is a full-screen editor for files on the SD card or USB drive:

```
s" A:/zeptoforth/mycode.fs" zed
```

The file is made if it is not there. Names are MMBasic's, with the drive and folders, or relative to the current folder.

{widths: 15,37}
| Key | What it does |
| Ctrl-W | Saves the file |
| Ctrl-V | Leaves the editor |
| Ctrl-Z | Undoes the last change |
| Ctrl-R | Searches forward |
| Ctrl-Space | Starts or clears a selection |
| Ctrl-K | Cuts the selection |
| Ctrl-Y | Pastes |
| Ctrl-O | Opens another file |
| Ctrl-N, Ctrl-P | Moves between the files open, and the help |
| Ctrl-? or Ctrl-/ | Shows all the keys. Ctrl-P goes back to your file, and Esc then Ctrl-X closes the help |

Files in the editor are held in 512 KB of memory, with what it needs to undo your changes. To change it, type for example 1000000 to zeptoed-heap-size.

Ctrl-C restarts Forth from the editor as it does anywhere, so save first.

## Loading a Program

```
include mycode.fs
```

A file may include another, up to four deep. fat32-tools import gives zeptoforth's own file words, such as list-dir, list-file and copy-file. The module cmm2 has MMBasic's, such as files and chdir.

## Keeping Your Work

Words go to RAM to begin with, and are lost at a restart. To keep them, compile them to the flash dictionary and save it:

```
compile-to-flash
include mycode.fs
compile-to-ram
save-image
```

save-image writes the whole Forth system back to zeptoforth.img or zeptoforth_g1.img, and it is there at the next start. What zeptoforth calls flash is memory here, so nothing is written to the Colour Maximite 2's own flash.

Keep a copy of the image file before you save over it. If a saved image stops zeptoforth from starting, put the copy back.

## Blocks

Forth's blocks are kept in the file blocks.dat in the current folder, up to 4096 blocks of 1024 bytes. **n edit** opens block n in the block editor, Ctrl-V leaves it, and **n list** and **n load** work as they do in any Forth.

## Graphics, Sound and I/O

The words for the screen, sound, the keyboard and mouse, files and the I/O pins are in the module cmm2:

```
cmm2 import
1 8 mode
300 300 80 2 1de0 cyan orange circle
```

WORDS.md in the folder lists them. Their arguments are in MMBasic's order. The console goes on working in any video mode, on top of what you draw. A graphics program should usually turn it off the screen with 0 console-screen, so that nothing it prints lands on the picture.

## Double Precision

Decimal numbers have a d before the exponent, and the words have dv in front:

```
float64 import
2de0 dvsqrt dv.
```

The words are those of zeptoforth's single-precision float32, with v changed to dv: dv+, dvsin, dv. and so on. A value takes two cells. A program written with the names v+ and numbers like 1.5e0 can type include float64_v.fs and float64-v import.

## Errors

A bad address gives the message **bad address**, and you go back to the prompt with what is in RAM kept. Inside a firmware service it restarts Forth instead.

If a Generation 1 says **the memory it is linked for is not free**, MMBasic is using too much of its memory. Run zeptoforth from the prompt after a restart, with no program running.

## Memory

{widths: 18,17,17}
| | Generation 2 | Generation 1 |
| RAM dictionary and stacks | 3 MB | 1 MB |
| Flash dictionary | 2 MB | 768 KB |

The Forth system itself takes about 510 KB of the flash dictionary.
