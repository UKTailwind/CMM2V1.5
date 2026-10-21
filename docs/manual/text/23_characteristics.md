# MMBasic Characteristics

## Naming Conventions

Command names, function names, labels, variable names, etc are not case sensitive, so that "Run" and "RUN" are equivalent and "dOO" and "Doo" refer to the same variable.

The type of a variable can be specified in the DIM command or by adding a suffix to the end of the variable's name. For example the suffix for an integer is '%' so if a variable called nbr% is automatically created it will be an integer. There are three types of variables:

- Floating point.  These can store a number with a decimal point and fraction (eg, 45.386) and also very large numbers.  However, they will lose accuracy when more than 14 significant digits are stored or manipulated.  The suffix is '!' and floating point is the default when a variable is created without a suffix

- 64-bit integer.  These can store numbers with up to 19 decimal digits without losing accuracy but they cannot store fractions (i.e. the part following the decimal point).  The suffix for an integer is '%'

- Strings.  These will store a string of characters (eg, "Tom").  The suffix for a string is the '$' symbol (eg, name$, s$, etc)   Strings can be up to 255 characters long.

Variable names and labels can start with an alphabetic character or underscore and can contain any alphabetic or numeric character, the period (.) and the underscore (_). They may be up to 32 characters long. A variable name or a label must not be the same as a command or a function or one of the following keywords: THEN, ELSE, TO, STEP, FOR, WHILE, UNTIL, MOD, NOT, AND, OR, XOR, AS. Eg, step = 5 is illegal.

It is important to note that the alphanumeric name of the variable must be unique so you cannot have both a variable s$ and s%. As a result, if a variable is declared without a type suffix, it can be used with the relevant suffix in a program and will still reference the same variable.

For file names see the chapter Program and Data Storage.

## Constants

Numeric constants may begin with a numeric digit (0-9) for a decimal constant, &H for a hexadecimal constant, &O for an octal constant or &B for a binary constant. For example &B1000 is the same as the decimal constant 8. Constants that start with &H, &O or &B are always treated as 64-bit integer constants.

Decimal constants may be preceded with a minus (-) or plus (+) and may be terminated with 'E' followed by an exponent number to denote exponential notation. For example 1.6E+4 is the same as 16000. If the decimal constant contains a decimal point or an exponent, it will be treated as a floating point constant; otherwise it will be treated as a 64-bit integer constant.

String constants are surrounded by double quote marks ("). Eg, "Hello World".

## Implementation Characteristics

- Maximum program size is 516KB of program memory. Note that MMBasic tokenises the program when it is loaded into program memory so the final size might vary from the plain text size.
- Maximum length of a command line, and of a line in a program, is 255 characters.
- Maximum length of a variable name or a label is 32 characters.
- Maximum number of variables is 1536: 1024 global and 512 local.
- Maximum number of dimensions to an array is 5.
- Maximum number of arguments to commands that accept a variable number of arguments, or of values in a DATA statement, is 75.
- Maximum number of nested FOR…NEXT loops is 100.
- Maximum number of nested DO…LOOP commands is 100.
- Maximum number of nested GOSUBs, subroutines and functions (combined) is 100.
- Maximum number of nested multiline IF…ELSE…ENDIF commands is 20.
- Maximum number of user defined subroutines and functions (combined) is 512. Labels are limited only by memory.
- Maximum number of structure types is 32, with up to 16 members each and nested up to 8 deep.
- Maximum number of interrupt pins that can be configured is 10.
- Numbers are stored and manipulated as double precision floating point numbers or 64-bit signed integers. The range of floating point numbers is 1.797693134862316e+308 to 2.225073858507201e-308.
- The range of 64-bit integers (whole numbers) that can be manipulated is ± 9223372036854775807.
- Maximum string length is 255 characters.
- Maximum line number is 65000.
- Maximum number of background pulses launched by the PULSE command is 5.
- Maximum number of sprites is 64 (#1 to #64) and of sprite collisions 8.
- Maximum number of files that can be open at once is 10, and the FILES command lists up to 2048.
- The maximum length of a filename/path is 127 characters.

## MMBasic Firmware Memory Map

Below is a summary of how the MMBasic firmware makes use of the memory of the STM32H743 chip. This detail is not needed to use MMBasic but may be of interest if you want to dig deeper.

{widths: 26,14,60}
| Address range (hex) | Size | Use |
|---|---|---|
| 0000 0000 - 0000 FFFF | 64K | ITCM RAM: the interrupt routines and some of the interpreter's code, run from here for speed. |
| 0800 0000 - 080F FFFF | 1024K | Flash bank 1: the MMBasic firmware and its constant data. |
| 0810 0000 - 0811 FFFF | 128K | Flash bank 2, sector 0: firmware too (the file manager and the editor). |
| 0812 0000 - 081D FFFF | 768K | Flash bank 2, sectors 1 to 6: the program in program memory (see OPTION FLASH) and the library (sector 6, 081C 0000 - 081D FFFF, when one is saved). |
| 081E 0000 - 081F FFFF | 128K | Flash bank 2, sector 7: the saved options. |
| 2000 0000 - 2001 FFFF | 128K | DTCM RAM: the firmware's own variables and, above them, the C stack (MM.INFO(HEAPEND) is the lowest it may go). |
| 2400 0000 - 2407 FFFF | 512K | AXI RAM: display page 0 (see the MODE command) and the display of the 4-bit modes. |
| 3000 0000 - 3003 FFFF | 256K | D2 RAM: the MMBasic heap in RAM, MM.INFO(HEAPI). |
| 3004 0000 - 3004 7FFF | 32K | D2 RAM: the firmware's buffers (sound's noise table, BLIT's buffers, the buffers for copies and lines). |
| 3800 0000 - 3800 FFFF | 64K | D3 RAM: MMBasic's variable records. |
| 3880 0000 - 3880 0FFF | 4K | Battery backed RAM: saved variables (VAR SAVE, VAR RESTORE and VAR CLEAR). |
| 4000 0000 - 5FFF FFFF | | The registers of the peripherals (GPIO, ADC, timers, SPI, DAC, USART, I2C...). MMBasic takes care of all this, however it is possible to PEEK and POKE these registers. |
| D000 0000 - D06F FFFF | 7M | SDRAM (Generation 2): the display pages (see the MODE command). |
| D070 0000 - D077 FFFF | 0.5M | SDRAM (Generation 2): program memory with OPTION RAM. |
| D078 0000 - D087 FFFF | 1M | SDRAM (Generation 2): the compiled program (OPTION COMPILE). |
| D088 0000 - D1FF FFFF | 23.5M | SDRAM (Generation 2): the MMBasic heap, MM.INFO(HEAPS): variables, arrays and buffers. Its top 512K (D1F8 0000 - D1FF FFFF) is where the code of a RAM library's CSUBs is kept. |
| D000 0000 - D02F FFFF | 3M | SDRAM (Generation 1): the display pages. |
| D030 0000 - D037 FFFF | 0.5M | SDRAM (Generation 1): program memory with OPTION RAM. |
| D030 0000 - D07F FFFF | 5M | SDRAM (Generation 1): the MMBasic heap (from D038 0000 with OPTION RAM). While OPTION COMPILE is ON or SHADOW its bottom 1M is the compiled program, and the heap starts above it. Its top 512K (D078 0000 - D07F FFFF) is where the code of a RAM library's CSUBs is kept. |

## Compatibility

MMBasic implements a large subset of Microsoft’s GW-BASIC. There are numerous differences due to physical and practical considerations but most standard BASIC commands and functions are essentially the same. An online manual for GWBASIC is available at http://www.antonis.de/qbebooks/gwbasman/index.html and this provides a more detailed description of the commands and functions.

MMBasic also implements a number of modern programming structures documented in the ANSI Standard for Full BASIC (X3.113-1987) or ISO/IEC 10279:1991. These include SUB/END SUB, the DO WHILE … LOOP, the SELECT…CASE statements and structured IF . THEN … ELSE … ENDIF statements.
