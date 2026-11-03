# Porting Existing Software

Much good software is written in portable C or C++ with a thin layer for each machine it runs on. Porting it to the Colour Maximite 2 is mostly a matter of writing that layer on the services. The three programs included with the firmware were ported this way, and each shows a different approach.

## The Approach

1. **Find the platform layer.** Look for the files that open a window, read keys, play sound and read files: often one directory (backends, platform, port) or a few files with the platform's name in them. Everything else should compile unchanged.
2. **Build without it first.** Compile the portable part with build.py and stubs for the platform functions, and fix what the compiler finds: missing headers (POSIX ones such as unistd.h and dirent.h are not there), sizes of int and long (both 32 bits here), and functions the C library does not have.
3. **Write the layer** on the services: video_mode (or video_scaled and a frame hook) for the display, keys_held and con_getc for input, the sound stream for audio, and the C library's files.
4. **Fit the memory.** Size the program's buffers from cmm2_start->heap_size, and give a program with its own allocator one block with cmm2_heap_take().
5. **Keep the original untouched.** Fetch the project's source at a known commit and keep the port's own files separate, so that the port can be rebuilt, updated and checked against the original. A GPL program's source must be given with the program, and this makes that easy: the original's location and commit, and the port's files.

## Doom

Doom was written to be ported: its game code calls a small set of platform functions. doomgeneric reduces them to six (DG_Init, DG_DrawFrame, DG_SleepMs, DG_GetTicksMs, DG_GetKey and DG_SetWindowTitle), and the port is one file, apps/doom/doomgeneric_cmm2.c, of about 350 lines:

- **The picture.** DG_Init sets MODE 3,8 (320 x 200, 256 colours), Doom's own resolution and colour depth. DG_DrawFrame copies Doom's frame into the page, each line twice as the mode needs, and loads Doom's palette with video_palette when it changes.
- **The keys.** keys_held is read each frame and its changes are turned into Doom's key presses and releases, with the codes mapped to Doom's (Ctrl fires, Space opens, the arrows move).
- **The time.** DG_GetTicksMs is micros() divided by 1000, and DG_SleepMs waits while calling poll().
- **The sound effects.** Doom's sound functions are replaced by a mixer of sixteen voices in the stream's fill.
- **The music.** Doom's MUS music is turned into MIDI (by doomgeneric's mus2mid.c) and played by music_play with the game data's own instrument bank, as a Sound Blaster played it, at half the synthesiser's rate.
- **The memory.** Doom's zone, the memory it manages itself, is sized from the program's heap, up to doomgeneric's 6MB. Doom is built with --vram, so a Generation 1 lends it the display memory and it has the 6MB there too.

apps/doom/make.py fetches doomgeneric at a fixed commit, builds its files unchanged (with mus2mid.c, which its own make file leaves out) and the CMM2 layer, and links doom.app.

## ScummVM

ScummVM is a large C++ program (531 source files in this port), and frank-quest had already ported it to a small machine, the RP2350, with its display, sound and file drivers in a handful of files. The CMM2 port replaces those files and nothing else:

- apps/scummvm/make.py reads frank-quest's own CMakeLists.txt for its lists of source files, so the build follows the original's.
- The directory overlay/ holds the replacements, at the same paths as the files they replace: the start-up and display driver (main.c, MODE 7,8 at 320 x 240), the input (keys_held and the mouse), the sound mixer's connection to the stream, the files (the file services), the memory allocator (the heap but 512KB, taken with cmm2_heap_take and managed by dlmalloc), and the list of games, with one more game and a game started by the name given after RUN's comma.
- The directory stub/ holds the few Pico SDK headers that the engines include, with just the declarations they use.

The program is 3MB and needs about 4MB of heap besides, more than a Generation 1 has borrowing MMBasic's heap alone, so it is built with --vram: a Generation 1 lends it the display memory too, and its MODE 7 display is one that a lent program can use.

## MicroPython

MicroPython has its own build system, so it is built by its own make files with the tools' files (see Using Another Build System in the chapter Building Programs). It is a port in the usual MicroPython sense, a directory ports/cmm2 in MicroPython's tree, and much of it was taken from the Pico Computer 3's port so that the same Python programs run:

- **The REPL** reads con_getc, turning MMBasic's key codes back into the escape sequences that MicroPython's line editor expects, and writes with con_puts. Ctrl-C is taken with break_take and raised as KeyboardInterrupt.
- **The file system** is a MicroPython VFS on the file services: the SD card at / and the USB drive at /usb.
- **The display** keeps the Pico Computer 3's frame buffer in the program's memory and copies it to the page in a frame hook, by the DMA2D (or by the processor where a mode's pixels must be converted), so the hdmi module's modes and drawing work unchanged.
- **Sound** is the Pico Computer 3's I2S driver turned into a fill function, so its audio module plays WAV, MP3, FLAC and MOD files.
- **The machine module's Pin, ADC, PWM, I2C, SPI and UART** are written on the I/O services, with the PicoMite's GP numbers.
- **Memory**: the garbage-collected heap is taken with cmm2_heap_take, from what is left after the C library's share, and SQLite's memory pool is sized from the heap too (2MB on a Generation 2, 512KB on a Generation 1).

## Things That Catch Ports

- **Case-insensitive file names on Windows**: a project's header can be found in place of another of the same name (ScummVM's audio.h and the firmware's Audio.h).
- **Assertions**: some code asserts things that are only true on the systems it was tested on. ScummVM's port is built with NDEBUG, as frank-quest's release builds are.
- **Large local arrays** overflow the 64KB stack without warning: give the program more with --stack.
- **Busy loops** that wait for something without calling poll() leave the keyboard and mouse dead.
- **Exceptions**: C++ code that relies on them needs changing, as they are not supported.
