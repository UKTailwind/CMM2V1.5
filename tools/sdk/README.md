# tools/sdk: loadable programs for the Colour Maximite 2

A loadable program is C or C++ built for the M7 and run with `RUN "name.app"` (`Src/Loader.c`). A G2 loads it into the 8 MB at 0xD1800000 that it keeps for loadable programs; a G1 lends it the largest free part of BASIC's heap while it runs and moves it there by its relocations. The file format and the services are in `Inc/cmm2img.h`.

The companion manual **Colour Maximite 2 Loadable Programs** (`docs/loadable/Colour_Maximite_2_Loadable_Programs.pdf`) covers it all for a programmer: getting started, how a program runs, the tools, the C library, every service with examples, performance, debugging and porting. Its source is `docs/loadable/text`, built by `tools/manual/guide.py`.

## Building

From `tools/sdk`:

```
python build.py out/hello.app examples/hello.c
python build.py out/vidtest.app examples/vidtest.c --api 2
python build.py out/oplbench.app examples/oplbench.c examples/nuked/opl3.c -I examples/nuked
python build.py out/oplplay.app examples/oplplay/oplplay.c examples/nuked/opl3.c examples/oplplay/emu8950/emu8950.c examples/oplplay/emu8950/emuadpcm.c ../../Src/OPLChip.cpp ../../Src/ymfm/ymfm_opl.cpp ../../Src/ymfm/ymfm_adpcm.cpp ../../Src/ymfm/ymfm_pcm.cpp -I examples/nuked -I examples/oplplay/emu8950 --cflag=-Wno-format
```

`build.py` compiles the sources with `crt0.c` and `syscalls.c`, links them with `app.ld`, and calls `mkimage.py`. It finds `arm-none-eabi-gcc` in `ARM_TOOLCHAIN_DIR`, else in the folder `CMakePresets.json` names, else on the PATH; use GCC 13.3, as the firmware does. The `.elf` and `.map` are left beside the `.app` in `tools/sdk/out/`, which git ignores.

Options:

| Option | Meaning |
|---|---|
| `--api N` | The services' version the program needs (default 1); older firmware refuses it. |
| `--heap BYTES` | The least heap the program needs: the loader refuses it at once if it can't have it. |
| `--stack BYTES` | The stack size (default 64 KB). |
| `--g2` | A G1 refuses it ("Needs a G2"): for programs too large for a G1's memory (ScummVM). |
| `--base 0x60000000` | Link elsewhere, so the loader has to relocate every address. This tests the relocations: a missed one faults, because that FMC bank is unused. |
| `--cflag`, `--cxxflag`, `--ldflag` | Extra flags for the compiler (C and C++, or C++ only) and the link. |
| `-I DIR`, `--sources FILE`, `-j JOBS` | Include directories, a file listing the sources, parallel jobs. |

`oplplay.app` renders MIDI, IMF, DRO and VGM files to a WAV through Nuked-OPL3, emu8950 or the firmware's ymfm (the same `OPLChip.cpp`), and reports each one's CPU share. It's the tool used to choose the firmware's synth.

## Running

Copy the `.app` to the SD card, with a card reader or with `ZMODEM RECEIVE` on the board and `tools/zmodem_pc.py send COM4 115200 file.app` on the PC. Then:

```
RUN "A:/apps/hello.app", any words
```

The words become `argv[1]` onward. A non-zero exit code is printed. Run from a program, the program carries on with its next statement.

## Licences

`cmm2img.h`, `cmm2.h`, `crt0.c`, `syscalls.c`, `app.ld` and the examples are MIT, so a program under any licence can use them. Third-party code beside the examples keeps its own licence:
- `examples/nuked` (Nuked-OPL3) is LGPL-2.1-or-later, so a program built with it, such as `oplbench.app` or `oplplay.app`, is LGPL as a whole.
- emu8950 is MIT.
- ymfm, in `Src/ymfm`, is BSD-3-Clause.
