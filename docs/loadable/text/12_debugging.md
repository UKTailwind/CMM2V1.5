# Debugging

There is no debugger on the Colour Maximite 2 itself, but a program has a console, and the files that build.py leaves beside the program file describe it exactly.

## printf

The console is the first tool: printf() to the serial port (with con_screen(0), so that the messages stay off the program's display), read in a terminal emulator on the PC. stdout is buffered until a newline; fflush(stdout) or a message ending in a newline shows it at once. For timing, micros() measures any stretch of code to the microsecond.

A program that might crash can write its progress to a file instead, with fflush() and the file closed and opened again (or file_sync), so that what was written survives the restart.

## When a Program Faults

A fault (a bad pointer, a stack overflow into nothing, a jump to a wrong address) restarts the Colour Maximite 2. To find out where:

- Narrow it down with messages: print as the program goes, and see which was the last.
- Look the addresses up. On a Generation 2 the program runs at the address it was linked for, so an address the program prints (say, a function pointer, or the return address from `__builtin_return_address(0)`) can be looked up in out/name.map, or with addr2line:

```
arm-none-eabi-addr2line -f -e out/name.elf 0xD1801234
```

On a Generation 1 the program is moved: subtract cmm2_start->base and add 0xD1800000 first.

- With an ST-Link connected to the processor's SWD pins, STM32CubeProgrammer can read the processor's registers from a program that has stopped or is stuck in a loop, without resetting it:

```
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -coreReg
```

The PC and LR registers it shows are looked up with addr2line as above.

## Memory

Running out of memory shows as malloc() returning NULL (check it), or as a stack overflow, which corrupts the top of the heap without any message. Print cmm2_start->heap_size at the start to see what the program has (malloc() can also go on in MMBasic's heap: see Memory in the chapter The C Library), or mallinfo() as it runs, and give a program with deep recursion or large local arrays more stack with --stack.

The map file shows what takes the space in the image: the sizes of the sections of each object file, and of each function with -ffunction-sections, which build.py uses.

## Testing the Moves

A program that runs on a Generation 2 but faults on a Generation 1 may hold an address that was not relocated. Build it with --base 0x60000000 and run it on a Generation 2: it is then moved too, and a missed address points at memory that is not there and faults at once, near the code that used it.

## Running Again Quickly

Building, copying and running can be one step. On the PC the program is sent with ZMODEM (tools/zmodem_pc.py), and RUN typed over the serial port: the repository's tools/cmm2serial.py sends a line to the Colour Maximite 2 and prints the reply. Note that it sends Ctrl-C first, to get to the command prompt, which stops a program that is running.
