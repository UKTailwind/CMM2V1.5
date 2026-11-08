# V8: BASIC programs always run from SDRAM

*8 November 2026. Peter's decision for V8 (branch v8-dev): the program no longer lives in flash. Nothing here is
built yet.*

## What Peter decided

- **Programs in SDRAM:** a BASIC program always runs from a 512 KB area of SDRAM, on the G1, the G2 and the G3, as
  OPTION RAM does now.
- **Flash for the firmware:** bank 2's sectors 3 to 6 (512 KB), the program's until V7, go to the firmware. The first
  users are the Wi-Fi stack ([Web_Plan.md](Web_Plan.md)), the G3's MS7210 and HDMI sound.
- **The library:** it stays in sector 7 for now.
- **Gains:** no flash wear from editing and running programs, and faster loading.
- **The cost Peter accepts:** a program that runs at power-up needs the SD card (AUTORUN.BAS), as with OPTION RAM now.
- **The layout:**
  - In SDRAM: display memory, then the heap, then the program area next to the executable window. That way the space
    from the start of the SDRAM is cleaner.
  - **As built (8 Nov):** the program area is at the very top, with the executable window just below it, the reverse
    of the order first drawn. The heap and its window then stay one range for the allocator, as now (Memory.h), and the
    program is outside it, as OPTION RAM's was on a G2. Everything else is as decided: display memory then heap from
    the start, one executable megabyte, and the same room for a G1's apps.
  - The compiler's stream goes directly after the display memory (agreed 8 Nov).

## What is there already

- **OPTION RAM:** the whole path to run from SDRAM exists and is in use: main.c's ProgMemory, and AUTORUN.BAS at
  start-up.
- **The G2:** it already keeps its 512 KB at 0xD0700000 for the program, whatever the option.
- **Flash slots:** FLASH SAVE and FLASH LOAD's slots are already files on the card (ImageSlots.c), so they do not change.
- **Saved variables:** VAR SAVE's variables are in the backup SRAM (0x38800000), not in flash.

## The SDRAM, today and in V8

### G1: 8 MB

| Address | Today | V8 |
|---|---|---|
| D0000000 | Display memory, 3 MB | Display memory, 3 MB |
| D0300000 | OPTION RAM's program (512 KB), if set, then the stream (1 MB) with OPTION COMPILE ON, then the heap | The stream (1 MB) with OPTION COMPILE ON, then the heap |
| D0700000 | Heap | Executable window, the heap's top 512 KB |
| D0780000 | Executable window, the heap's top 512 KB | **Program area, 512 KB** |

A G1's heap for variables:

| Options | Today | V8 |
|---|---|---|
| Default | 4.5 MB | 4 MB |
| OPTION RAM | 4 MB | (the default) |
| OPTION COMPILE ON | 3.5 MB | 3 MB |

### G2: 32 MB

| Address | Today | V8 |
|---|---|---|
| D0000000 | Display memory, 7 MB | Display memory, 7 MB |
| D0700000 | Program area, 512 KB | Compiler's stream, 1 MB |
| D0780000 | Compiler's stream, 1 MB | (stream) |
| D0800000 | (stream) | Heap |
| D0880000 | Heap | Heap |
| D1700000 | Heap | Executable window, the heap's top 512 KB |
| D1780000 | Executable window, 512 KB | **Program area, 512 KB** |
| D1800000 | Loadable programs, 8 MB | Loadable programs, 8 MB |

The G2's heap for variables stays at 15 MB.

### The MPU

- **One executable block:** the program area and the executable window together are one megabyte on a megabyte
  boundary, executable as one MPU region: D0700000 on a G1, D1700000 on a G2. It is region 15, as the highest number
  wins where regions overlap (on a G2 region 14 covers the heap to D17FFFFF; region 9, OPTION RAM's, is now off). A program's CSUBs run from the program
  area, and the CSUBs of the flash slots and the library run from the executable window.
- **The heap:** it stays execute-never. The loader's LoaderExec still makes, on a G1, the run it lends executable
  while a program runs.

## Loadable programs on a G1

- **The joined block:** a G1 lends a program built --vram its display memory and the bottom of its heap as one block.
  That happens only when the heap, or the compiler's stream when nothing is running from it, starts right after the
  display memory (Loader.c).
- **Why not D0300000:** keeping the program area there, where OPTION RAM puts it now, would end that. NES (6.6 MB) and
  pc386 (5.5 MB) would not load.
- **In V8:** the block is about 6.75 MB at best. That is 3 MB of display memory and 4 MB of heap, less the loader's
  256 KB reserve, and with COMPILE ON the stream's 1 MB more.

From the image headers through Loader.c's rules, with the whole heap free:

| App | Needs | V8 |
|---|---|---|
| nes | 6.6 MB in one block | Loads, with only about 165 KB to spare (today about 1.2 MB) |
| pc386 | 5.5 MB | Loads |
| cpc, doom, heretic, spectrum, scummvm | 3 to 3.8 MB | Load |
| cc, micropython, mmbrun, bcrun, vt100, zeptoforth_g1 | Under 3.3 MB | Load in the heap, 512 KB smaller |

- **NES's margin** is the tightest. Most of what it asks for is the 5 MB its Pico board had as PSRAM. That holds the
  ROM, a tile cache at 2 MB and the selector's lists at 4 MB, and can be trimmed if the margin bites.
- **Something already at the bottom of the heap** cuts the margin further, such as NES started from inside a BASIC
  program.

## The flash

- **The region:** FLASH2 (0x08160000, 640 KB) becomes a code region for sectors 3 to 6, with the library still in
  sector 7.
- **Free afterwards:** the firmware then has 512 KB more. The web plan's stack takes most of it, and leaves about 110
  KB across the three free pieces.
- **Writes:** program loading writes no flash. The only writes left are the options (sector 2) and LIBRARY SAVE
  (sector 7).

## The commands (Peter, 8 Nov)

- **OPTION FLASH and OPTION RAM:** both are removed completely. They can only be run at the command prompt, so typing
  either just gives the ordinary error for an unknown option.
- **Options code:** OPTION LIST no longer prints "OPTION RAM ON". Option.ProgramStartCode and its uses (Memory.h's
  G1 SDRAMBASE, main.c's choice of ProgMemory) go.
- **Old options:** options saved by V7 with OPTION RAM or OPTION FLASH n set are read without error, and the field is
  ignored.
- **AUTORUN:** OPTION AUTORUN ON runs A:/AUTORUN.BAS at power-up, as OPTION RAM's start-up does now. Without a card it
  goes to the prompt.
- **NEW, LOAD, RUN, EDIT, CHAIN, the file manager's run:** unchanged, except that they never write flash.

## What users lose, and keeping the program over a restart

- **Lost:** the program is gone after a power cycle or a reset button press, as with OPTION RAM now.
- **Kept over an option restart** (Peter, 8 Nov: "yes as proposed"):
  - **Saving:** when a command restarts MMBasic (an OPTION that needs a restart, OPTION RESET, and the like), the
    firmware first writes the program area, tokenised as it is up to the program's end, to a card file such as
    A:/.restart.prg.
  - **Loading back:** at start-up, if that file exists, it is read back into the program area and deleted, before
    AUTORUN is looked at.
  - **The result:** the program and its CSUBs come back exactly, with no tokenising, and LIST, RUN and EDIT carry
    on.
  - **Without a card,** or if the write fails, the restart goes ahead and the program is lost, as with OPTION RAM
    now. The command does not refuse.
  - **Not covered:** a power cycle, the reset button and the watchdog, which give no warning.
- **To measure, not rely on:** whether the SDRAM keeps its contents through a soft reset, given the 64 ms refresh
  period.

## Steps

1. **The BASIC side, on V7 firmware:** Mauro's programs, the largest first (the demoscene, demox_cmm2), with OPTION
   RAM on a G1, against the same run from flash. This shows that a program run from SDRAM behaves the same, and how
   long it takes. It is the same code that V8 makes the only path.

   **Done 8 Nov, on the G1 with RC3:**
   - **demox_cmm2** (MODE 2,16, seven pages, MOD music, PLY models): it ran to the end with no error from flash
     (306.6 s) and from RAM (304.9 s). The demo holds its own frame rate, so the times show only that both keep up.
   - **bench_micro.bas:** every statement was as fast from RAM as from flash, within 0.5% (RAM/flash 0.995 to
     1.000). The whole run took 5.5 s from RAM against 6.6 s from flash, as the run from flash first wrote the
     program to flash.
   - **CSUBs:** Mauro's repositories have none, so a program's CSUBs running from the program area are still to be
     tested in step 5.
   **Steps 2, 3 and most of 5 done 8 Nov on v8-dev, flashed on the G1 and the G2:**
   - **What works:**
     - Program memory is at D0780000 (G1) and D1780000 (G2).
     - The heap for variables is 4 MB on a G1 (3 MB with COMPILE) and 15 MB on a G2.
     - OPTION RAM and OPTION FLASH give "Unrecognised option".
   - **The tests:**
     - CSUBs run from program memory (csubm7 "bad 0", csubabi 68 of 68, on both boards, also compiled on a G1).
     - The program is kept over an option restart, and FLASH RUN 0 runs the restored image.
     - AUTORUN.BAS runs at a restart.
     - The flash-slot test passed 65 of 65 on both boards.
     - NES and pc386 load on a G1, also with COMPILE ON.
   - **tests/cmm2 on the G2 against RC2 (5 Nov):** 110 of 113 programs give byte-identical output. Each of the three
     others is explained:
     - parity1 prints MM.VER (RC2 to RC3).
     - osk prints OPTION LIST (the version and the keyboard option).
     - guictl found no mouse plugged in.
   - **The G1:** its run differs from the G2's only where a G1 has no 32-bit colour.
   - **Not tested:** the update from a firmware before RC2 (OldLayoutMove) needs an old firmware on a board.
   - **Step 4 (the linker):** waits for the first code to go into sectors 3 to 6.

2. **The layout:**
   - Memory.h's SDRAMBASE, SDRAMEND, SDRAMEXEC, RB_STREAM_BASE and the program area, for both boards.
   - main.c's MPU regions.
   - Loader.c's G1 branch, with the join and the stream.
3. **The flash path removed:**
   - ProgMemory always in SDRAM.
   - The flash program writes taken out (FlashWriteInit and the erase at start-up).
   - OPTION FLASH and OPTION RAM removed, as described under The commands.
   - AUTORUN made to use the card.
   - The program kept over an option restart, through the card file.
4. **The linker:** sectors 3 to 6 made a code region.
5. **Tests:**
   - The goldens and SP Tools on the G1 and the G2.
   - Every app on a G1, NES and pc386 above all.
   - OPTION COMPILE ON and OFF.
   - A program with CSUBs, the library, CHAIN, the flash slots, AUTORUN with and without a card.
   - An option restart with a program loaded, with and without a card.
6. **The manual:**
   - Every place that says the program is kept in flash.
   - OPTION RAM and OPTION FLASH, removed.
   - AUTORUN.
   - The program kept over an option restart.
   - The G1's memory figures.
