# Loadable programs for the CMM2, and an OPL synth in the firmware

Peter, 2 Nov 2026:

> I'd like to consider porting a scumm vm to the CMM2. There is a port on https://github.com/rh1tech/frank-quest. We already have most of the drivers needed in place with the possible exception of midi support. Review if this could be incorporated into MMbasic as a service that can be called to launch scumm compatible games

> Is this concept of loading a program potentially generic? What other sorts of s/w could we consider loading in this way

> Prepare a plan for a generic mechanism. Perhaps we should build a opl decoder direct into the firmware to simplify things in the same way as our mod decoder

> nukeykt/Nuked-OPL3 on github is an option for opl

> The CMM2 doesn't support the vs1053 and won't. The device is not convenient as different firmware has to be uploaded for different purposes

Written 2 Nov 2026 from:
- the CMM2 at `686ca06`, with sizes measured from its Release build;
- frank-quest v1.00 (rh1tech/frank-quest `949301b`);
- Nuked-OPL3 1.8 (nukeykt/Nuked-OPL3), compiled here with the firmware's flags;
- the PicoMite's `PicoMite/io/Audio.c`.

P marks a PicoMite file.

## Status, 2 Nov 2026 (the development repository)

Work is in `D:\Dropbox\VSCode-STM32\CMM2V1.5-loader`, a clone of this repository at `268eda3`, on branch `loader`.
It was tested on the G2 on COM4.

Peter's decisions since the plan was written:

> take an extra flash bank and add the loader and opl support ready for testing opl performance

> You can permanently allocate 8Mb on the G2 for loadable programs. Gate all the code as G2 only.

> Check that OPTION FLASH respects the new flash layout

**Flash (D1 decided: A).**
- Bank 2 sector 1 is now firmware (FLASHB2 is 256 KB), and the program starts at sector 2.
- `PROGRAMSTARTCODE` is 2 and OPTION FLASH takes 2 to 6. OPTION FLASH with no number now gives 2 rather than 0, which only the boot remap had put right. The manual's OPTION FLASH entry and its memory map are updated.
- This is the layout the parity plan agreed on 15 Oct: a 384 KB program in sectors 2-4, the library in 6, options in 7.
- Tested on the board:
  - 1 and 7 are refused ("valid is 2 to 6");
  - 3 moves the program to sector 3;
  - the bare form moves it back to 2.
- Flash left: 1,096 bytes in bank 1 (the hooks cost 160) and 120,072 bytes in bank 2 sectors 0-1.

**Memory (Peter: a permanent 8 MB, G2 only).**
- `LOADER_BASE` 0xD1800000, `LOADER_SIZE` 8 MB, executable and write-back cached through MPU region 8.
- MPU region 14 is now 8 MB, so the two don't overlap.
- The heap ends at 0xD1800000: MM.INFO(HEAP) went from 24,341,504 to 15,955,968. The library's executable window moved to 0xD1780000.
- This replaces Part 1's "an image in the heap": an application is linked at 0xD1800000, so it loads without relocation. The relocation code stays for images linked elsewhere, and was tested with a copy linked at 0x60000000.

**The loader (`Src/Loader.c`, `Inc/cmm2img.h`).**
- `RUN "name.app" [, words]` (D2, provisional). Only names ending `.app` are looked at, and G1 gives "Only available on a G2".
- The program runs on the process stack at the top of the 8 MB, and comes back by returning or through `leave()` from any depth.
- What it leaves open is closed and freed when it ends.
- From a program, RUN of a `.app` goes on with the next statement, the program's variables kept. At the prompt it returns to the prompt.
- Service table version 1: the console, time, files and directories (FatFs), poll, leave, heap blocks, and the firmware's OPL3 synth.
- The SDK is `tools/sdk`: `build.py`, `mkimage.py`, `app.ld`, `crt0.c`, `syscalls.c` (stdio, malloc, time) and two examples.
- Two existing bugs were fixed on the way:
  - The prompt's RUN path left the token buffer ended with one zero, not tokenise's three. RUN never returned before, so it never showed; with a returning RUN it gave a stray "Unknown command" after a shorter line.
  - `CloseAudio` left `WAV_fnbr` naming the file it had closed. After PLAY WAV and PLAY STOP, the next `FindFreeFileNbr` could hand that number out, and the next `CloseAudio` closed that file: here the loader's program, mid-read ("Cannot read the program"). It now sets `WAV_fnbr` to 0, as PLAY MODFILE's load already did.
- Ctrl-C: `poll()` reports it and leaves it set, so the program ends and the BASIC program that ran it stops too (`tests/opl/ctrlc.bas`). Clearing it had let a loop of `RUN "x.app"` go on to the next run.

**OPL (`Src/OPL.c`, `Src/OPLChip.cpp`, ymfm in `Src/ymfm`).**
- **D3 decided, 2 Nov: ymfm.** Peter heard the same music through Nuked-OPL3, ymfm and emu8950. Of the ones heard so far, he found "ymfm sounds very good"; on WONDERIN.WLF, "no obvious sound differences".
- Nuked-OPL3 has left the firmware; it is kept in `tools/sdk/examples/nuked` for comparison programs. With it goes the firmware's only LGPL code: ymfm is BSD-3-Clause.
- `OPLChip.cpp` puts ymfm's YMF262 behind C:
  - placement new into heap memory, so no C++ heap is used;
  - resampling from 49,716 Hz to the output rate;
  - the level halved to Nuked's;
  - register writes queued and applied one per chip sample, as a real OPL2 (about 23 µs a write) takes them.
- The write queue is needed. ymfm acts on a key change only at its next sample, so a drum struck again (key off and on written together, as YsBattle.vgm's rhythm section does) was lost. Peter heard the difference in the opening seconds. With the queue: "no problems sounds good".
- C++ in the build: `CMakeLists.txt` enables CXX for `Src/ymfm` and `OPLChip.cpp` only, with no exceptions, RTTI or unwind tables. Flash: 1.3 KB more than Nuked in bank 2, and 32 bytes in bank 1.
- Commands, all provisional (D2): PLAY OPL, PLAY OPL WRITE reg, value [, ...] (up to 19 pairs, the argument limit), PLAY OPL RESET and PLAY OPL TEST [seconds [, voices]]. PAUSE, RESUME and STOP work as for any sound, and MM.INFO(SOUND) gives "OPL". PLAY OPL plays through MOD's buffers and interrupt branch.

**PLAY MIDI and OPL files (2 Nov).** Peter's answers:
- **The instrument bank:** "flash".
- **The names:** "PLAY OPL".
- **PLAY MIDI TEST:** "store a midi sample in flash and play it from flash".

Built, tested on the G2 (`tests/opl/miditest.bas`), and heard: "sounds very good".

```
PLAY OPL file$ [, interrupt]             ' IMF (.imf 560 Hz, .wlf 700 Hz), DRO 0.1 and 2.0, VGM of OPL/OPL2 (one or two)/OPL3
PLAY MIDI                                ' real-time MIDI on (the PicoMite's syntax; no VS1053 needed)
PLAY NOTE ON channel, note, velocity     ' channel 0-15, 9 the drums
PLAY NOTE OFF channel, note [, velocity]
PLAY MIDI CMD cmd, data1 [, data2]       ' &H80-&HEF
PLAY MIDI TEST [n]                       ' the tune in flash: Freedoom's FreeDM title (d_fdmttl.mid, 11 s); n 1-3 as on the PicoMite
PLAY MIDIFILE file$ [, interrupt]        ' Standard MIDI File, types 0 and 1
```

- **How it plays:**
  - MIDI is driven as Doom's DMX driver drives an OPL: GENMIDI patches, 18 OPL3 voices with the oldest taken when all are busy, and velocity, volume, expression, pan, sustain and pitch bend followed.
  - A file's events fall at their sample in each buffer. PLAY NOTE and PLAY MIDI CMD are heard at the next buffer, 46-93 ms later.
  - A two-OPL2 file gets a second chip, mixed in.
  - The whole file is read into the heap first.
  - A file ends after two seconds of releases, the way a module played once ends: the buffers drain, `audio_checks` closes it and the interrupt runs.
- **MM.INFO:** SOUND gives "MIDI" or "OPL"; the CMM2 dialect shortens a pause to "PAUSED". TRACK gives the file's name, or "MIDI TEST".
- **Flash:**
  - `Src/OPLData.c`, made by `tools/mkopldata.py`: the GENMIDI bank (175 x 36 bytes, names left out) and the test tune, 9.9 KB together.
  - The new code is about 6 KB.
  - Bank 2 has 102 KB left. **Bank 1 is down to 712 bytes.**
- **Licence:** Freedoom's data is BSD-3. Its notice has to go in the documentation of a binary release; it is in `OPLData.c`'s header, ready for the manual and the release notes when this is merged.

**Measurements, on the G2 at 480 MHz.**

PLAY OPL TEST (all 18 voices held with tremolo and vibrato; and the synth on but silent):

| Synth | 18 voices | 0 voices |
|---|---|---|
| Nuked-OPL3, `-O2`, bank 2 flash | 61.2% | 51.4% |
| ymfm, `-O2`, bank 2 flash (now) | 53.6% | 19.9% |

Real music, through `oplplay.app` (`tools/sdk/examples/oplplay`, which renders MIDI, IMF, DRO and VGM to a WAV with any of the three, timed):

| Music | Nuked-OPL3 | ymfm | emu8950 (OPL2 only) |
|---|---|---|---|
| WONDERIN.WLF (IMF) | 59% | 28% | 22% |
| Freedoom E1M1 (MIDI, 18 voices) | 68% | 52% | |
| YsBattle.vgm (OPL2, rhythm), ymfm with the write queue | | 36% | |
| BeyondSN.vgm (OPL3) | | 41% | |
| Two OPL2 chips (dual DRO / VGM) | 114%: it can't keep up | 65% | |

- The chip's state in internal RAM and in SDRAM time the same, and code in SDRAM matches code in flash, so ITCM would gain little.
- A BASIC loop keeps 78% of its speed while PLAY OPL plays with ymfm (107.2 ms against 83.5 ms). With Nuked it kept 44%.

**The test material** (on the G2's card in `A:/opl`; `tests/opl` has the programs):
- Freedoom's MIDI music and GENMIDI instrument bank (BSD-3);
- AdPlug's test files for IMF/WLF, DRO and VGM.

`render.bas` writes the comparison WAVs and `jukebox.bas` plays them with titles. `make_opl_bas.py` (also in `tests/opl`) generated both, and lists the set.

**The services, version 2: the screen and the keys held** (`tools/sdk/examples/vidtest.c`, built with `--api 2`).
- `video_mode(mode, 8, &v)`: MODE's checks made first, so the program never meets an MMBasic error. A bad mode or depth gives -1, and another depth while a FRAMEBUFFER is open gives -2. The page is cleared.
- The doubled modes (3, 5, 6, 7...) store each line twice: `v.copies` is 2 and `v.pitch` 2 x width. MODE 3,8 gives 320x200 at 0x24000000 (internal RAM).
- `video_palette` loads R,G,B bytes into the CLUT at the next frame, and `video_wait` waits for one (50 ms at most).
- `keys_held`: KEYDOWN's six codes and the modifier bits. With Ctrl held a letter comes as its control code (Ctrl-D is &H04).
- When the program ends, BASIC's mode (OPTION PICOMITE's through its MODE), font, colours and MAP palette are put back, and the keys typed meanwhile are dropped.
- **On the G2:**
  - a frame in MODE 3 takes 13,377 us (75 Hz);
  - 225 palette loads took 3,009 ms, one per frame;
  - the bad modes were refused;
  - keys, Shift, Alt, Ctrl and F5-F12 were reported;
  - Esc and Ctrl-C both ended it at BASIC's prompt;
  - Peter saw the bars and the colours turning.
- **The USB touch screen:** it disconnects about 0.1 s after any mode change and reconnects 1.6 s later. BASIC's MODE does it too, with USB serviced every millisecond throughout, while 3 s and 10 s without any polling (`tools/sdk/examples/nopoll.c`) left it connected. So the monitor resets its touch controller as it relocks, and the firmware can't prevent it. The keyboard and mouse are not affected. Peter unplugged the touch screen for the Doom work.

**The services, version 3: sound.**
- **The stream:** `audio_start(fill)` returns the rate (44.1 kHz as the timer makes it).
  - A new sound mode, `P_STREAM`, plays through MOD's branch of the sample interrupt, in 1024-frame buffers (23 ms).
  - The interrupt pends PendSV at each change of buffer. PendSV, now set to the lowest priority (CubeMX had left it at 0, unused), calls the program's fill for the free buffer, adds the music, then applies PLAY VOLUME and the DACs' scale as MOD's refill does.
  - `audio_lock` and `audio_unlock` set BASEPRI at PendSV's level.
  - `video_mode` holds the refill off too. setmode turns the MPU off for a moment, and the SDRAM is then execute-never, so a fill at that moment would fault fetching the program's code.
- **The music:** `music_play(midi, size, loop, bank)` plays a MIDI file in the program's memory through OPL.c's sequencer and DMX driver (`OPLStreamPlay`/`OPLStreamMix`). It uses the program's GENMIDI bank, or the flash's. The track parse is shared with PLAY MIDIFILE (`song_tracks`).
- **Files read through a cluster map** (FatFs fast seek, `ld_map`): a seek no longer walks the FAT from the file's start over the card's SPI. Without it Doom's start-up crawled: each lump read began with a backward seek in the 28.8 MB WAD.
- The SDK's `rename` is its own, since newlib's links and unlinks.
- Bank 2: 161,872 bytes used of 256 KB.

**Doom, the first loadable game** (`apps/doom`: `make.py` fetches doomgeneric at a pinned commit and builds `doom.app`, 342 KB).
- The CMM2 layer, `doomgeneric_cmm2.c`:
  - **picture:** MODE 3,8, with Doom's 8-bit frame and palette;
  - **keys:** KEYDOWN's changes become Doom's events;
  - **effects:** 16 voices of DMX lumps, mixed in the fill;
  - **music:** MUS lumps turned to MIDI by mus2mid and played by the firmware's synth with the WAD's GENMIDI.
- Freedoom Phase 1 (`A:/Doom/freedoom1.wad`) starts in a few seconds, from `CHDIR "A:/Doom": RUN "doom.app"`.
- **Speed:** 56.9 frames a second without sound (135 s of play) and 32.9 with sound and music (112 s). Doom's game runs at 35 tics a second.
- **Peter:**
  - "graphics smooth and fast";
  - with sound, "sounded good - framerate still acceptable".

**Not done yet.**
- Modules.
- Doom: the frame rate with music (the synth at half rate, or its loop in faster memory); a `-timedemo` measurement; saves into the game's directory without CHDIR; the mouse.
- A G1 check of the gating (no G1 on the bench).
- A full regression.

The G2's whole flash as it was before the first dev flash (7.00.00b5, its program and options) is in
`build/g2backup/g2_flash_20261102.bin`, with its OPTION LIST. git ignores it. To put it back:
`STM32_Programmer_CLI -c port=SWD mode=UR freq=480 -w build/g2backup/g2_flash_20261102.bin 0x08000000 -v -rst`.

## Summary

- **One mechanism, two kinds of program.** It loads programs from the SD card:
  - *applications* take over the machine (ScummVM, emulators, games);
  - *modules* load beside a BASIC program and add things it calls.
- **How they're built and run:**
  - A program is built with arm-none-eabi-gcc against a small SDK.
  - A post-link tool turns its ELF file into a relocatable image.
  - The loader places the image in the SDRAM heap, marks it executable, and calls it with a versioned table of firmware services.
- **GPL code stays out of the firmware.** Each program is a separate work on the card. That is what lets ScummVM, which is GPL, run on a firmware with a BSD-style licence.
- **An OPL synth goes into the firmware**, as the MOD player did: Nuked-OPL3, which is LGPL.
  - It gives BASIC the PicoMite's MIDI commands without a VS1053, plus the OPL music file formats.
  - Programs reach it through the service table.
- **Flash comes first.** The firmware's flash is full, so decision D1 has to be made before anything else.

## Decisions needed

| | Decision | Recommendation |
|---|---|---|
| D1 | **Flash room** for the loader and the OPL synth, about 26-34 KB in all (Part 3) | **Decided 2 Nov: bank 2 sector 1 (A)** |
| D2 | Names: the image's file extension, the command that starts an application, the module commands | `RUN f$` recognising an image; `MODULE LOAD` / `MODULE CALL` / `MODULE()` |
| D3 | The OPL emulator, once its CPU cost is measured (S2) | **Decided 2 Nov: ymfm, after Peter heard all three (status above)** |
| D4 | What PLAY MIDI TEST does on the CMM2 | **Decided 2 Nov: plays a MIDI tune kept in flash (Freedoom's FreeDM title)** |
| D5 | When a module is freed | When the program ends, as the RAM library is |
| D6 | Where the SDK lives | `tools/sdk/` in this repository |
| D7 | G1 support | **Decided 2 Nov: G2 only, with a permanent 8 MB at 0xD1800000** |

## The CMM2 now

**Flash** (`STM32H743II_FLASH.ld:43-56`)
- The firmware fills bank 1 to within 1,256 bytes and bank 2 sector 0 to within 216 bytes.
- Bank 2 sectors 1-7 are all in use: the program, the flash slots, the library and the options (`flash.h:72-76`, `main.c:241-245`, `Library.c:34`).

**SDRAM** (`Memory.h:113-127`)
- The G2's heap runs from 0xD0880000 to 0xD2000000, about 23.5 MB. The G1's is about 5 MB, and 1 MB less while COMPILE is on.
- The OPTION RAM program and the compiler's stream sit below the heap.

**Executable SDRAM**
- MPU regions 13 and 14 cover the heap and are not executable.
- Region 15 makes 512 KB at `SDRAMEXEC` executable. `GetExecMemory` allocates in it (`main.c:2996-3007`).
- Region 8 is spare (`main.c:3009`).

**CSUBs**
- The firmware has three call tables (`CFunctions.c:71-234`): `CallTable`, plus `CallTableP` and `CallTablePS` in the PicoMite's layout. These tables are append-only.
- `CallCFunction` passes BASIC variables as pointer arguments.
- `CFuncmSec` runs in the 1 ms SysTick at `IRQ_PRI(0)` (`Timers.c:150`).

**Audio**
- One double buffer (`sbuff1`/`sbuff2`, `swingbuf`) is played by the sample interrupt.
- `checkWAVinput()` refills it between statements. The MOD player refills through `hxcmod_fillbuffer` (`Audio.c:2312-2333`), and PLAY ARRAY through a reader function (`Audio.c:191`).

**Input**
- USB devices are serviced by `routinechecks()` between statements (`main.c:3239`).
- `getConsole` calls `CheckAbort` (`main.c:3382`), so it can longjmp.

**Interrupts**
- Almost everything runs at `IRQ_PRI(0)`, which is 1.
- PendSV is unused (`stm32h7xx_it.c:311`).

**RAM that survives a reset**
- `.my_section` in DTCM holds the exception code and address, and `_reset_magic` (`main.c:197-203`).

**Video**
- MODE 3,8 is 320x200 with a 256-colour palette, set through `HAL_LTDC_ConfigCLUT` (`Draw.c:2643`). That is the frame ScummVM draws.

**MIDI**
- There is none. The File Manager reports "MIDI files cannot be played" (`FileManager.c:3469-3471`).
- The PicoMite's PLAY NOTE, PLAY MIDI and PLAY MIDIFILE all need a VS1053 (P `Audio.c:2525-2680`). The CMM2 will not support one.

## Part 1: the loader

### Two kinds of program

| | Application | Module |
|---|---|---|
| Runs | instead of BASIC | inside a BASIC program, when called |
| Memory | the whole SDRAM heap: image, stack, its own heap | a block in the 512 KB executable window |
| Video and audio | its own (sets a mode, streams audio) | as BASIC leaves them; may play through the audio stream |
| Errors | must never longjmp; every service returns a status | may raise a BASIC error, as a CSUB does |
| Ends | returns, or calls the exit service | freed when the program ends (D5) |
| Examples | ScummVM, emulators, DOOM, a C compiler | a SID or VGM player, features moved out of flash |

### The image

- **Build.**
  - Flags: `-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb`, as the firmware.
  - Linked at 0x60000000 with `-Wl,--emit-relocs`. 0x60000000 is an FMC bank the CMM2 leaves unused, so any pointer that missed relocation faults at once.
- **Post-link tool** (`tools/sdk/mkimage.py`; name provisional).
  - It writes the header, then the loadable bytes (`.text`, `.rodata`, `.data`, the init/fini arrays), then the relocation list.
  - Relocations it keeps:
    - `R_ARM_ABS32` and `R_ARM_TARGET1`: add the offset to a word.
    - `R_ARM_THM_MOVW_ABS_NC` and `R_ARM_THM_MOVT_ABS`: patch the 16-bit immediates.
  - PC-relative relocations need nothing, because the image moves as one piece.
  - Any other absolute type fails the build. It must not fail at load time.
- **Why not position-independent code.**
  - CSUBs are text-relative PIC with no `.data` (`-fpie -msingle-pic-base`, `mmb2csub_CMM2_Plan.md`). That cannot carry C++ vtables or initialised pointers.
  - A global offset table (GOT) costs a load for every global access.
  - Relocating once at load gives ordinary code at full speed.
  - The TinyCC fork's YAFF format (PIC with an SB-relative GOT) was considered and is not adopted, for the same reason.
- **Placement.** Relocation lets the loader put an image at any free, suitably aligned address, so nothing in SDRAM is reserved permanently.

**Header** (little-endian, version 1)

| Field | Use |
|---|---|
| magic `CMM2IMG\0`, header version | recognise the file |
| service-table version needed | refuse an image built for a newer firmware |
| kind (application or module), flags (G2 only, needs audio, needs video) | refuse a program the board cannot run |
| image size, `.bss` size, stack size (default 64 KB), smallest heap | memory the loader must find |
| entry offset; for a module, export-table offset | start and call points |
| relocation offset and count | fix-ups, discarded after loading |
| CRC-32 of everything after the header | catch a damaged file |
| name and version strings | messages, MM.INFO |

Each refusal is an ordinary MMBasic error, raised before anything changes.

### Starting an application (G2)

1. End the program as RUN does: close its files, stop audio, clear interrupts.
2. Read and check the header.
3. Clear the heap.
   1. Take an aligned block at the top for the image. ScummVM's 3.7 MB goes at 0xD1C00000, in 4 MB.
   2. Take the stack below it, with region 8 as a no-access guard under the stack.
   3. Hand the rest to the program as its heap: about 19 MB for ScummVM.
   4. The OPTION RAM program and the compiler's stream are not touched.
4. Load the image, zero `.bss`, apply the relocations, and check the CRC.
5. Clean the D-cache over the image and invalidate the I-cache.
6. Resize MPU region 15 over the image: power-of-two size, sub-regions trimmed.
7. Disable Ctrl-C's break. The program decides its own quit key; frank-quest uses Ctrl+Alt+Del.
8. Switch to the process stack (PSP) on the program's stack and call the entry with a start block. The block holds the service table, argc/argv, the heap's base and size, and the return-program name. Interrupts keep using the main stack in DTCM.

**When the program returns** (or calls the exit service), the loader:
- closes any file still open through the table;
- stops the audio stream;
- restores the MPU and the main stack;
- re-initialises the heap and resets the display;
- RUNs the return program through the `RUN f$` path, or returns to the prompt.

A game menu written in BASIC therefore gets control back.

**When the program faults:**
- The existing path records the exception in `.my_section` and restarts.
- The loader also records the image's name and base address there, so the message after restart reads "in quest.app at offset 0x...".
- A file the program had open for writing may be incomplete. This must be documented.

### Modules

- **Loading:** `MODULE LOAD f$` (D2). The loader relocates the module into a block from `GetExecMemory`.
- **Calling:** `MODULE CALL name$, entry$ [, arg ...]` and the function `MODULE(name$, entry$ [, arg ...])`.
  - Arguments are passed as for a CSUB (`CallCFunction`).
  - Entries are looked up in the module's export table (name and offset).
- **Context:**
  - A module runs inside the program, so `error()` is allowed, as in a CSUB.
  - A module written in C++ must not rely on destructors running across one.
- **Hooks:** it may register a 1 ms hook and an end-of-program hook through the table.
- **Freeing:** when the program ends (D5).

### The SDK (`tools/sdk/`, D6)

- **`cmm2img.h`:** the service table and the start block, under the MIT licence, so GPL and BSD programs alike can include it.
- **`crt0.c`:**
  - takes the start block and sets up newlib's heap;
  - runs the constructors and calls `main(argc, argv)`;
  - runs the destructors and returns to the loader.
- **`syscalls.c`:** newlib's `_open`, `_close`, `_read`, `_write`, `_lseek`, `_fstat`, `_isatty`, `_sbrk`, `_gettimeofday` and `_exit`, mapped onto the table. Portable C using `fopen`, `printf`, `malloc` and `time` then builds unchanged.
- **Build files:** a linker script, a CMake toolchain file, and `mkimage.py`.
- **Example programs:** hello, a file copy, a benchmark, and a 320x200 demo with sound.

### The service table, version 1

Rules:
- It is a struct of function pointers with a version and a count, append-only like the CSUB tables.
- For applications, no entry may call `error()`, call `CheckAbort()` or longjmp. Each one returns a status.

| Group | Entries | Built on |
|---|---|---|
| Console | putc, puts, getc (never blocks, no `CheckAbort`) | `putConsole`, `MMPrintString`; a new getc |
| Time | micros, delay_us, millis, date and time | `timer()`, `uSec`, the RTC |
| Files | open, close, read, write, lseek, tell, size, sync, opendir, readdir, closedir, stat, mkdir, unlink, rename, getfree | FatFs (`ff.c`); handles come from a table the loader can close |
| Video | set_mode, page address / size / pitch, set_palette, wait_vblank, show_page | MODE's code, `GetPageAddress`, the CLUT, the vblank flag (`pagesetdone`) |
| Input | key events (code, ASCII, modifiers, down or up), mouse (x, y, buttons, wheel), gamepad | a new queue fed by the keyboard drivers; the mouse state; `USBGamepad.c` and the Nunchuk |
| Audio | stream_start(rate, channels, callback), stream_stop, lock, unlock, volume | a new `P_STREAM` mode |
| OPL | reset, write, generate | Part 2 |
| System | poll (USB and keyboard only), exit, board, clocks | a safe subset of `routinechecks()`: no SD checks, no `CheckAbort` |
| Version 2 | pins, COM ports, I2C, SPI | `ExtCfg`/`ExtSet`/`ExtInp`, `Serial.c`, `I2C.c` and `SPI.c`, wrapped because they can call `error()` |

### The audio stream

- **New mode.** `P_STREAM` in `Audio.c` uses the existing double buffer. The program's callback fills it with 16-bit stereo.
- **Refill for an application:**
  - The refill runs in PendSV at the lowest priority. The sample interrupt pends PendSV when it swaps buffers.
  - frank-quest refilled from its main loop at first, and the music broke whenever a SCUMM frame took more than about 23 ms. It moved its mixing into the DMA interrupt (frank-quest `drivers/audio/audio.c:470-485`).
  - lock and unlock raise and restore BASEPRI to PendSV's level, which holds off only the refill.
- **Refill for a module:** a module's stream refills in `checkWAVinput()`, as MOD does, because BASIC is running.

## Part 2: an OPL synth in the firmware

### Why in the firmware

- **The PicoMite's MIDI commands** (P `Audio.c:2525-2680`), without a VS1053. A VS1053 would need different firmware loaded for different jobs, and the CMM2 will not support one.
- **The File Manager can play `.mid` files.**
- **OPL music files:**
  - IMF (id Software and Apogee games);
  - DRO (DOSBox captures);
  - VGM logs of the YM3812 and YMF262.
  Once the chip exists, each is a small parser.
- **Programs reach the chip through the service table** (reset, write, generate). A DOOM or Wolfenstein port then needn't carry its own synth, and the synth runs from flash rather than SDRAM.
- **ScummVM gains least.**
  - It mixes OPL output with speech and effects in its own mixer, so it would call generate into its own buffer.
  - The port starts with ScummVM's own OPL code, as the port-intact rule says.
  - It moves to the firmware's synth only if a measurement shows a gain.

### The emulator (D3)

| | Nuked-OPL3 | emu8950 | ymfm |
|---|---|---|---|
| Licence | LGPL-2.1-or-later | MIT | BSD-3-Clause |
| Language | C, two files | C | C++ |
| Chips | OPL3, which plays OPL2 music too | OPL, OPL2, Y8950 | OPL to OPL4 |
| Accuracy | cycle-accurate | good | good |
| CPU | the heaviest | light | light |

GPL emulators can't go in the firmware. That rules out DOSBox's DBOPL and the emulators ScummVM carries.

**Nuked-OPL3 measured here** with the firmware's flags:
- 4,334 bytes of code and 1,158 of tables at `-Os` (5.5 KB); 6.3 KB at `-O2`.
- Its state is 19,520 bytes of RAM. 16 KB of that is the 1,024-entry register write buffer (`OPL_WRITEBUF_SIZE`, `opl3.h:46`).

**CPU:**
- Nuked emulates at the chip's 49,716 Hz and resamples.
- It is reported to run in real time on an overclocked Pico 2 only in a fork 1.4-2.2x faster.
- PicoGUS on the RP2040 used emu8950, and later ymfm, for speed.
- A 480 MHz M7 should run it in real time, but its share of the CPU has to be measured (S2) before D3.

**Licence:**
- Unlike the GPL, the LGPL allows linking into the firmware. The condition is that users can rebuild the firmware with a changed `opl3.c`. The firmware's source is public, which meets that.
- `opl3.c` and `opl3.h` stay unaltered, with their notices, and are named in the licence notes and the release notes.
- That is a reading of the licence, not legal advice; it should be checked before a release.

### Structure, after the MOD player

- **`Src/opl3.c`, `Inc/opl3.h`:** Nuked-OPL3 as published.
- **`Src/OPL.c`,** under the firmware's own licence. Its code must be new: GPL drivers can't be copied (Chocolate Doom's `i_oplmusic.c`, ScummVM's `adlib.cpp`, libADLMIDI).
  - Sequencers: Standard MIDI Files (types 0 and 1), IMF, DRO and VGM.
  - A MIDI-to-OPL driver:
    - General MIDI patch lookup;
    - voice allocation (9 two-operator voices on OPL2, 18 on OPL3);
    - percussion on channel 10;
    - pitch bend, volume and expression.
  - The BASIC commands.
- **Instrument bank:** Freedoom's GENMIDI.
  - Licence: BSD-3-Clause.
  - Contents: 175 instruments (128 General MIDI and 47 percussion), which Freedoom took from OpenBSD's kernel OPL driver.
  - Size: about 6.3 KB without the names, at 36 bytes each.
- **`Audio.c`:** `P_MIDI` and `P_OPL` modes, with paused versions, refilled in `checkWAVinput()` as `P_MOD` is.
  - For each buffer, the sequencer runs to its next event, then `OPL3_GenerateStream` makes the samples up to it, then `i2convert`.
  - Events are therefore timed to the sample within each buffer.
- **Real-time mode:** PLAY MIDI keeps the synth running, silent, until PLAY STOP. PLAY MIDI CMD and PLAY NOTE feed the driver directly.
- **MM.INFO(SOUND)** reports "MIDI" as the PicoMite does.

### BASIC commands

| Command | PicoMite (VS1053) | CMM2 (OPL) |
|---|---|---|
| PLAY MIDI | real-time mode on | the synth in real-time mode |
| PLAY MIDI CMD cmd, data1 [, data2] | a raw MIDI message | the same, to the driver |
| PLAY MIDI TEST n | the VS1053's test | D4 |
| PLAY NOTE ON / OFF | as the PicoMite's | as the PicoMite's |
| PLAY MIDIFILE f$ [, interrupt] | plays a MIDI file | plays a MIDI file |
| PLAY OPL f$ [, interrupt] (new) | - | IMF, DRO or VGM, by extension (D2) |

### Cost

| Part | Flash |
|---|---|
| Nuked-OPL3 | 5.5 KB (measured, `-Os`) |
| `OPL.c`: sequencers, driver, commands | 6-10 KB (estimated) |
| GENMIDI bank | 6.3 KB |
| **Total** | **about 18-22 KB** |

RAM: about 20 KB of chip and driver state, taken from the heap while playing and freed afterwards, as MOD's buffers are.

## Part 3: flash room (D1)

| Need | Flash |
|---|---|
| Loader, image checks, relocation, the service table and its wrappers, `P_STREAM`, the key queue, the MODULE commands | 8-12 KB (estimated) |
| The OPL synth (Part 2) | 18-22 KB |
| **Total** | **about 26-34 KB, against about 1.5 KB free** |

**Options:**
- **A.** Give the firmware bank 2 sector 1: 128 KB.
  - The program area moves up one sector and one flash slot goes. That changes the manual and OPTION/FLASH behaviour.
  - It is the simplest option, with room for years.
- **B.** Once the loader works, move self-contained features into modules:
  - Astro (28 KB);
  - SAM speech (`sam.c`, `reciter.c`, `render.c`: 15.5 KB);
  - Ray (11 KB);
  - turtle (9.4 KB).
  That is about 64 KB in all. A program using one of them would need the module file on the card. A stub could load the module on first use and keep the syntax unchanged.
  - Leave STEPPER out of this list: its 200 kHz tick runs from ITCM.
  - B needs the loader to fit first.
- **C.** Find a few KB, enough for the loader alone, by trimming. The `-O2` file list was measured for speed, so there is little to gain there.

**Recommendation:** A. If A is not acceptable, C first, then B.

## Phases

| Phase | Work | Done when |
|---|---|---|
| S1 (spike) | A minimal loader behind a temporary command, `mkimage.py`, and a test application (console and timing). Measure the load time for 4 MB from the SD card, and a benchmark run from SDRAM against the same code in flash. | It loads, runs and returns on a G2; the figures are recorded here |
| S2 (spike) | A test build timing `OPL3_GenerateStream` for one second at 44.1 kHz, with 9 and 18 voices playing; `-Os` against `-O2`, flash against ITCM | D3 decided |
| 1 | After D1: the image format, the loader, the SDK, and the console, time, files and system services | Test applications pass on the G2 (COM4) and the G1 (COM34) |
| 2 | Video, input and the audio stream; exit and fault handling; a first real application, a small emulator (a Game Boy core such as Peanut-GB, MIT) | It plays with sound; exit returns to the BASIC program; a deliberate fault is reported with its offset |
| 3 | The OPL synth, independent of the loader: Nuked-OPL3, GENMIDI, the MIDI file player and driver, PLAY MIDI / NOTE / MIDIFILE, `.mid` in the File Manager; then PLAY OPL; then the OPL services | The PicoMite's MIDI examples run unchanged; tests in `tests/` with goldens |
| 4 | ScummVM: frank-quest's OSystem and file layer (about 2,300 lines) replaced, its engines unchanged; Monkey Island 1 first, then sound | Monkey Island 1 plays from start to the first save |
| 5 | Modules: MODULE LOAD / CALL / MODULE(), the export table, the hooks; a first module (a SID or VGM player, or Astro if D1 is B) | A BASIC program calls the module and it is freed at the end |
| 6 (later) | Version-2 hardware services; MIDI out on a COM port at 31250 baud to an external synth (Monkey Island 1 and 2, Indy 4 and Loom were written for the MT-32); applications on the G1 where they fit | - |

Phase 3 can start as soon as D1 and S2 are settled, in parallel with phases 1 and 2.

## Licences

- **The firmware keeps its licence.** Nuked-OPL3 inside it stays LGPL, in its own files.
- **Programs on the card keep theirs.** Most of those named here are GPL: frank-quest and ScummVM, frank-nes, frank-micro and frank-video (GPL-3.0), frank-wolf3d (GPL-2.0). Each is a separate work loaded at run time. Their source must be published with any binary.
- **The SDK header is MIT,** so programs under any licence can include it.
- **Each port needs its own licence check.** For example, Duke Nukem 3D's Build engine has its own licence.
