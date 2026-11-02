# tools/sdk: loadable programs for the CMM2 G2

A loadable program is C built for the M7 and linked to run in the G2's 8 MB at 0xD1800000. `RUN "name.app"` loads it there and runs it (`Src/Loader.c`). The design is in `docs/Loader_Plan.md`, and the file format and services are in `Inc/cmm2img.h`.

## Building

From `tools/sdk`:

```
python -I build.py out/hello.app examples/hello.c
python -I build.py out/oplbench.app examples/oplbench.c examples/nuked/opl3.c -I examples/nuked
python -I build.py out/oplplay.app examples/oplplay/oplplay.c examples/nuked/opl3.c examples/oplplay/emu8950/emu8950.c examples/oplplay/emu8950/emuadpcm.c ../../Src/OPLChip.cpp ../../Src/ymfm/ymfm_opl.cpp ../../Src/ymfm/ymfm_adpcm.cpp ../../Src/ymfm/ymfm_pcm.cpp -I examples/nuked -I examples/oplplay/emu8950 --cflag=-Wno-format
```

`oplplay.app` renders MIDI, IMF, DRO and VGM files to a WAV, through Nuked-OPL3, emu8950 or the firmware's ymfm (the same `OPLChip.cpp`), and reports each one's CPU share. It's the tool used to choose the firmware's synth.

`build.py` uses the firmware's own toolchain (from `CMakePresets.json`). It compiles the sources with `crt0.c` and `syscalls.c`, links them with `app.ld`, and calls `mkimage.py`. The `.elf` and `.map` are left beside the `.app` in `tools/sdk/out/`, which git ignores.

Options:

| Option | Meaning |
|---|---|
| `--stack BYTES` | The stack size (default 64 KB). |
| `--heap BYTES` | The least heap the program needs. The loader refuses the program if the 8 MB can't give it. |
| `--base 0x60000000` | Link elsewhere, so the loader has to relocate every address. This tests the relocations: a missed one faults, because that FMC bank is unused. |
| `--cflag FLAG` | Pass an extra compiler flag. |

## Running

Copy the `.app` to the SD card, for example with `ZMODEM RECEIVE "A:/apps/*"` on the board and `tools/zmodem_pc.py send COM4 115200 file.app` on the PC. Then:

```
RUN "A:/apps/hello.app", any words
```

The words become `argv[1]` onward. A non-zero exit code is printed. Run from a program, the program carries on with its next statement.

## What a program has

- **Standard C:** `printf` and the rest of stdio, `malloc`, `fopen` and friends, and `time()`. Names follow MMBasic's style: `A:/dir/name`, `C:/...`, or a name relative to the current directory.
- **The `cmm2` services** (`cmm2.h`): direct console, microsecond clock, files and directories, `poll()`, `leave()`, extra heap blocks, and the firmware's OPL3 synth.
- **Its own stack and heap** in the 8 MB.

Only the services touch MMBasic. They never raise an MMBasic error; each returns a status instead.

## Licences

`cmm2img.h`, `cmm2.h`, `crt0.c`, `syscalls.c`, `app.ld` and the examples are MIT, so a program under any licence can use them. Third-party code beside the examples keeps its own licence:
- `examples/nuked` (Nuked-OPL3) is LGPL-2.1-or-later, so a program built with it, such as `oplbench.app` or `oplplay.app`, is LGPL as a whole.
- emu8950 is MIT.
- ymfm, in `Src/ymfm`, is BSD-3-Clause.
