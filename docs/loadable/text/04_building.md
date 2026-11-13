# Building Programs

A program file is made in three steps: the sources are compiled for the Cortex-M7, linked with the tools' start-up code and library glue at the program's address, and the linked ELF file is turned into a program file. build.py does all three; this chapter describes it, its options, and how to do the same from another build system.

## build.py

```
python build.py OUT.app SOURCE.c|.cpp [...] [options]
```

From tools/sdk, or with a path to it. Each source is compiled to an object file in a directory named after OUT (out/first.obj for out/first.app), and the objects are linked together with crt0.c and syscalls.c. The files it leaves are:

{widths: 22,78}
| File | What it is |
|---|---|
| OUT.app | The program file, for the SD card. |
| OUT.elf | The linked program, with its symbols, for debugging (arm-none-eabi-addr2line, objdump, gdb). |
| OUT.map | The linker's map: where each function and variable is, and what took the space. |
| OUT.obj/ | The object files. |

A source is compiled again only when it, or a header it includes, has changed since its object was made (or the options have changed), and the sources are compiled in parallel, so building again after a small change is quick. ScummVM's 531 source files compile in about 23 seconds on a fast PC. An object file (.o) among the sources is linked as it is: a part compiled and linked on its own first, as each of the C compiler's passes is (apps/cc/make.py).

The options:

{widths: 26,74}
| Option | Meaning |
|---|---|
| --api N | The version of the services the program needs (default 1). Firmware with an older version refuses the program with "Needs a newer firmware". See below. |
| --heap BYTES | The least heap the program can run with (default 0). RUN refuses the program at once if it cannot have it. |
| --stack BYTES | The size of the program's stack (default 64KB). |
| --vram | A Generation 1 lends the program its display memory too, about 7MB in all, and limits its video modes while it runs (see The Display Memory Lent in the chapter How a Program Runs). Use it with --api 8: firmware before version 8 ignores it. A Generation 2 ignores it too. |
| --g2 | The program runs on a Generation 2 only: a Generation 1 refuses it with "Needs a G2". |
| --axi NAME | The code, constants and data of the source NAME (its file name without folder or extension) linked to run in the processor's 512KB of AXI RAM, which a Generation 2's display leaves free when its page is in the SDRAM. cmm2_axi_load() copies them there. Repeat it for more sources. Needs --g2. See Code in the Fast Memories in the chapter Performance. |
| --itcm NAME[:F1,F2,...] | The functions named of the source NAME, or all its code if none are named, linked to run in the ITCM from 0xA000 to 0xFFFF, the 24KB above the firmware's own code, also copied there by cmm2_axi_load(). Needs --g2, and --api 18: older firmware does not keep that memory free. |
| --libc full | The whole C library, newlib, in place of newlib-nano: its wide-character printf and file functions (swprintf, fwprintf, fgetws), which newlib-nano leaves out. The program is larger: hello.c is 27KB in place of 16KB. Neither library converts UTF-8 as STM32CubeIDE's tools build them: setlocale() takes only "C", and a byte is a character, as in Latin-1. |
| --cflag FLAG | A flag for the compiler, for C and C++ (eg, --cflag=-DDEBUG, --cflag=-O3). Repeat it for more. |
| --cxxflag FLAG | A flag for C++ sources only. |
| --ldflag FLAG | A flag for the link. |
| -I DIR | A directory of header files. |
| --sources FILE | A file listing sources, one a line (lines starting with # are left out), for projects too large for a command line. |
| -j JOBS | How many sources to compile at a time (default: the PC's processor threads). |
| --base ADDR | Link at another address (default 0xD1800000). Used to test the relocations: see below. |

## How It Compiles

The flags are those of the firmware, for the STM32H743:

```
-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb --specs=nano.specs
-O2 -Wall -ffunction-sections -fdata-sections
-fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables
```

C is compiled as gnu11 and C++ as gnu++14, without exceptions or run-time type information (-fno-rtti), and without the locks for local static variables (-fno-threadsafe-statics). The floating point unit does single and double precision in hardware, and float and double are both fast; long double is double.

--specs=nano.specs, which selects newlib-nano's headers and library, is left out with --libc full. The link uses the C library, with printf's floating point included, the maths library and libgcc:

```
-nostartfiles -T app.ld -Wl,--defsym=__app_base=0xD1800000
-Wl,--gc-sections -Wl,--emit-relocs -u _printf_float -lc -lm -lgcc
```

--gc-sections leaves out every function and variable that nothing uses, and --emit-relocs keeps the information that mkimage.py needs to make the relocations. A program with a C++ source is linked by g++, so that the C++ library is there.

## The Services' Version

The table of services only grows: each version of the firmware keeps the entries of the one before and may add more at the end. Its version is in cmm2->version:

{widths: 12,88}
| Version | Added |
|---|---|
| 1 | The console, the clock, files and directories, poll, leave, heap blocks, the FM synthesiser. |
| 2 | The screen (video_mode, video_palette, video_wait) and the keys held. |
| 3 | The sound stream and the music. |
| 4 | The mouse, and the console kept off the screen (con_screen). |
| 5 | Ctrl-C taken by the program (break_take), and a file's details (file_stat). |
| 6 | A display in the program's own memory (video_scaled, frame_hook), the DMA2D and 16-bit colour. |
| 7 | The I/O pins, the ADC, PWM, I2C, SPI and the COM ports. |
| 8 | No new services: the start block's second heap (heap2), and a Generation 1's display memory lent to a program built with --vram. |
| 9 | The USB keyboard's own codes (keys_raw), and fast seeking in a file that is read and written in place. |
| 10 | Drawing by MMBasic's routines (gfx_cls to gfx_save), the screen saved as a PNG, and the current directory (dir_change, dir_current). |
| 11 | The firmware's own memory routines and maths library (mem_copy, mem_fill, mem_move, libm), and MMBasic's statements and fonts (basic_cmd, gfx_target, font_size, font_define). |
| 12 | MMBasic's expressions (basic_eval), what is playing (sound_state), a statement tokenised once (basic_prep, basic_exec), and IMAGE RESIZE_FAST (image_resize). |
| 13 | USB gamepads (gamepad). |
| 14 | The com_ services reach COM4 to COM7, the USB serial devices. |
| 15 | Whether a port's device is there (com_connected). |
| 16 | All of a gamepad's axes: both sticks and the analogue triggers (gamepad_axes). |
| 17 | video_mode and video_scaled take depth 12: ARGB4444 on one layer. |
| 18 | A DMA2D copy without writing the source back from the cache first (dma2d_copy_nc). The ITCM from 0xA000 is kept for programs (--itcm), and the sound stream goes to the DACs by DMA. |
| 19 | A file cut short (file_truncate, and the C library's ftruncate()). |

Firmware 7.00.00RC3 has version 15, 7.00.00RC4 version 16, and 7.00.00RC5 version 18. A program that uses a service added after version 1 should be built with --api set to the highest version it uses, so that older firmware refuses it with a clear error rather than calling an entry it does not have. A program that can do without a service can instead check the version itself and work with less:

```
if(cmm2->version >= 4) cmm2->con_screen(0);
```

## C++

C++ works as it does on any small system without exceptions: classes, templates, virtual functions, new and delete (through malloc), global objects with constructors and destructors, and the parts of the C++ library that do not throw. As nothing can catch an exception, a new that fails ends the program: a program that must survive running out of memory uses new (std::nothrow), malloc(), or an operator new of its own, as ScummVM's port does. ScummVM, a large C++ program, is built this way.

## Larger Projects

Point build.py at the sources, the include directories and the flags the project needs, with a --sources file if the list is long. Two things are worth knowing:

- **File names.** On Windows file names are not case sensitive, so a project's header can be found in place of a system or SDK header of the same name (ScummVM's audio.h and the firmware's Audio.h met this way). The tools put only tools/sdk on the include path, and cmm2.h includes the firmware's header by its path.
- **Replacing files.** A port usually replaces a project's platform layer with its own. ScummVM's make.py keeps its replacements in an overlay directory, at the same paths as the files they replace, and builds the project's own source lists with those files swapped in, so the original source stays untouched (see the chapter Porting Existing Software).

## Using Another Build System

A project with its own make files can be built without build.py. It needs:

- the compiler flags above, with -I for tools/sdk;
- crt0.c and syscalls.c from tools/sdk, compiled with the same flags;
- the link flags above, with the linker script tools/sdk/app.ld;
- then mkimage.py to make the program file from the ELF.

MicroPython is built this way, by its own Makefile. In outline, its link and program file steps are:

```
LDFLAGS = $(CFLAGS_MCU) --specs=nano.specs -nostartfiles -T $(CMM2_SDK)/app.ld \
    -Wl,--defsym=__app_base=0xD1800000 -Wl,--gc-sections -Wl,--emit-relocs
$(BUILD)/micropython.elf: $(OBJ) $(SDK_O)
    $(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
$(BUILD)/micropython.app: $(BUILD)/micropython.elf
    $(PYTHON) $(CMM2_SDK)/mkimage.py $< $@ --name micropython --api 7 \
        --stack 262144 --heap 1572864
```

mkimage.py takes the options --name (up to 15 characters, the file's name by default), --api, --stack, --heap, --vram and --g2, with the same meanings as build.py's.

## mkimage.py and the Relocations

mkimage.py writes the program file: a 64-byte header, the image (every section with contents, from the link address, gaps filled with zeros), and the relocations, a list of the places in the image that hold an absolute address of the image (Appendix B gives the format).

A program linked at 0xD1800000 runs on a Generation 2 without them. A Generation 1 loads the program wherever its free memory is and adds the difference to each of those places. It always moves a program by a whole number of 64KB, so that of the two halves of an address loaded by a MOVW and MOVT instruction pair only the upper one (the MOVT) changes: mkimage.py keeps the 32-bit addresses (R_ARM_ABS32 and R_ARM_TARGET1) and the MOVTs (R_ARM_THM_MOVT_ABS), and leaves out the relative relocations, which a move does not change. Any other kind of relocation stops the build with an error, so a program that builds will load.

The compiler normally makes only these kinds. Hand-written assembly that loads an absolute address in another way (an LDR of a literal that is not marked as an address) would not be adjusted. To test that a program moves correctly, build it with --base 0x60000000: every address in it must then be relocated, and one that was missed points at memory that does not exist on the Colour Maximite 2 and faults at once.
