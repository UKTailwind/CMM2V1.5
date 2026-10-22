# Porting Programs

This chapter covers some of the considerations involved in porting programs to the Colour Maximite 2: first from the PicoMite, whose version of MMBasic the Colour Maximite 2 now shares, and then from the original Colour Maximite.

## Running PicoMite Programs

The language of MMBasic version 7 is the same on the PicoMite and the Colour Maximite 2, so most PicoMite programs run with few or no changes. The differences come from the hardware:

- **The display.** The PicoMite's MODE numbers mean other resolutions on the Colour Maximite 2 (MODE 1 here is 800 x 600). Change a program's MODE statements as the table in the chapter Video Modes and Pages shows: the PicoMite's MODE 2 is MODE 7,4 here and its MODE 3 is MODE 8,4, with the same 16 colours, framebuffers and layers (FRAMEBUFFER) and the same screen memory for PEEK and POKE. Its MODE 1 is MODE 8,1, with the same tiles (TILE), framebuffers and screen memory. TILEMAP, SPRITE and BLIT work on these modes as on the PicoMite.
- **Image slots.** FLASH LOAD IMAGE can load images into the slots 4 to 8 only, which are held in RAM; slots 1 to 3 (the PicoMite's flash slots) are not provided. A program loads its images every time it starts.
- **Drives.** On the PicoMite A: is its internal flash filesystem and B: its SD card. On the Colour Maximite 2 both A: and B: are the SD card, and a USB flash drive is C: on both. A program that uses B: works unchanged; one that uses A: uses the SD card.
- **I/O pins.** The PicoMite's pin names (GP0, GP1 and so on) are not accepted: use the pin numbers of the Colour Maximite 2's rear connector (see the chapter Hardware Features). The I/O devices are on different pins too, and the PicoMite's SETPIN commands that assign pins to COM ports, I2C and SPI are not needed here (each port has fixed pins).
- **Hardware the Colour Maximite 2 does not have:** WiFi and the WEB commands, Bluetooth, PIO, LCD display panels and touch, cameras, the RTC and keypad commands, the PicoMite's flash slots and filesystem. Programs that use these cannot run without changes.
- **CSUBs.** A CSUB built for the PicoMite runs unaltered if it uses only MMBasic's call table and none of the PicoMite's hardware (the Colour Maximite 2 provides the PicoMite's call table as well as its own). A CSUB can also be built for the Colour Maximite 2 with mmb2csub using --target cmm2.
- **MM.DEVICE$** returns "Colour Maximite 2" (or "Colour Maximite 2 G2") so a program can tell which computer it is running on.

## Porting from the Original Colour Maximite

There is a high degree of backwards compatibility in the Colour Maximite 2 and most programs written for the original Colour Maximite will run with little effort, however, as can be expected, there are some differences that need to be addressed.

Most of these differences involve the more specialised functions such as graphics, input/output and some functions like the random number generator. Note that not all differences are listed here, just the more important ones that are likely to cause problems when porting programs.

## Variables

In the original Colour Maximite it was possible to define variables with the same name but using a different type. For example, it was possible to use the following: v=3.4512 and v$="abcdefg" in the same program (ie, the variable was defined as both a float and a string). This is not allowed in the Colour Maximite 2, variables must be unique regardless of their type.

## Floating Point

In the Colour Maximite 2 floating point is double precision. This means that if a number is printed without formatting it will contain more significant digits than the same number on the original Colour Maximite. Generally, this will not cause an issue but it might mess up numbers that need to be printed in neat columns.

## Graphic Commands

The syntax of the graphic commands PIXEL, LINE, BOX and CIRCLE have completely changed. However, using the command OPTION LEGACY ON you can change these commands back to the same syntax as used in the original Colour Maximite. This option is not saved so it needs to be placed in the program before any graphic commands are used.

This option will also cause the drawing commands to accept colours in the range of 0 to 7. However, the colour shortcuts (red, blue, etc) are not defined as they were on the original Colour Maximite so, if these are required, they will need to be defined in the program as follows:

```
CONST Black = 0
CONST Blue = 1
CONST Green = 2
CONST Cyan = 3
CONST Red = 4
CONST Purple = 5
CONST Yellow = 6
CONST White = 7
```

You can also use OPTION LEGACY OFF to switch back to the Colour Maximite 2 syntax, so it is possible to mix both forms of the graphics commands in the one program.

The VGA display mode is not changed with this option so it should be specifically set to MODE 4, 8 for emulating the 480x432 resolution of the original Colour Maximite or MODE 5, 8 for emulating its 240x216 resolution.

The syntax of the MODE command has changed. The Scan Line Colour Override function is not supported in the Colour Maximite 2.

## Fonts

The FONT command has a completely different purpose and syntax in the Colour Maximite 2. The Colour Maximite 2 has the ability to load fonts using the LOAD FONT command and you can convert the original Colour Maximite's font files to this format using the program FontTweak included in the Colour Maximite 2 download zip.

The BLIT command is compatible with the exception that it does not implement the optional parameter specifying which colour planes to copy.

## Sprites

As with the original Colour Maximite sprites can be loaded from a Maximite sprite file using the command:

```
SPRITE LOAD filename$ [,start_sprite_number]
```

This is compatible with the original Colour Maximite however the Colour Maximite 2 has a few improvements:

- An optional third parameter is available in the first line of the sprite file in which case the first parameter becomes the width and the third parameter the height. Both width and height can be set to any size.
- If only two parameters are specified the first parameter sets both the width and height and can be a number other than 16.
- Multiple sprite files can be loaded by specifying the optional 'start_sprite_number' to ensure that the sprites from different files do not overlap. This parameter defaults to 1 if not specified.

The following example shows a red mouse pointer. The first line of the file holds the parameters for the sprite (ie, the width, the number of sprites in the file, and the height in that order).

```
13,1,19
4
44
4 4
4  4
4   4
4    4
4     4
4      4
4       4
4        4
4         4
4          4
4      444444
4   4  4
4  44  4
4 4  4  4
44   4  4
      4  4
      4444
```

The above sprite can be saved as "mouse.spr". Then the following commands will load and show it:

```
SPRITE LOAD "mouse.spr", 5
SPRITE SHOW 5, 100, 100, 1
```

Note that a space character is interpreted as BLACK in 8 and 16-bit colour modes and transparent BLACK in 12-bit colour. A "0" is interpreted as NOTBLACK in 8 and 16-bit colour modes and solid BLACK in 12-bit colour.

## SOUND and TONE

The SOUND command has a different purpose and syntax in the Colour Maximite 2 and should be replaced with the PWM command. The TONE command has been replaced by PLAY TONE.

## SD Card and File Related Commands

The SD card is drive A: (also called B:). The commands PLAYMOD, LOADBMP and SAVEBMP have changed names: see PLAY MODFILE, LOAD BMP and SAVE IMAGE. CHAIN, the LIBRARY commands and DRIVE are available, as described in this manual.

## Special Devices

LCD displays, Real Time Clocks (RTC) and keypads are not supported in the Colour Maximite 2. These can all be implemented in BASIC.

## CONFIG Commands

Relevant CONFIG commands in the original Colour Maximite have been converted to OPTION commands.

## Error Handling

Error handling using the OPTION ERROR commands has changed (see ON ERROR).

## Random Number Generator

The Colour Maximite 2 has an advanced random number generator based on an analog circuit that generates a sequence of true random numbers which will never repeat. For this reason RND cannot be seeded to give a repeatable sequence (RANDOMIZE is accepted, for programs written for the PicoMite, but it does not change the numbers that RND returns).

The original Colour Maximite generated a pseudo random number sequence that always repeated with the same seed and in some rare cases this is what the programmer requires. If you need this behaviour you can use the following to generate a repeatable set of random numbers:

```
FUNCTION pseudo() AS FLOAT
  STATIC seed% = 7
  STATIC a% = 1103515245, c% = 12345, m% = 2^31
  seed% = (a% * seed% + c%) MOD m%
  pseudo = seed% / m%
END FUNCTION
```

Change the assignment to seed% to change the seeding number.
