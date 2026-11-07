# Tests

Each file here checks itself. Start zeptoforth, and at its prompt:

```
include float64_test.fs
```

`float64_test.fs` checks double precision against IEEE doubles by their bits, and is written by `make_float64_test.py`, which has the cases. It ends with how many failed.

The rest check the words of `cmm2.fs`. Each needs `words_h.fs` first, which has `check` and `ends`, and they print `CHECKS n FAILS n`. Reboot between them. `words_files.fs`, `words_gfx.fs`, `words_io.fs`, `words_stress.fs` and `words_doc.fs` make no sound. `words_snd.fs` and `words_misc.fs` do. `words_gfx.fs` tests its own `check`, so its first two failures, marked DELIBERATE, are expected and are not counted.

`words_io.fs` looks for the loop-back wires, GP0, 1 and 3-7 to GP8, 9 and 11-15, and leaves out what needs one that is not there.

`words_hold.fs`, then a reboot or Ctrl-C, then `words_free.fs`, shows that what Forth had of the firmware is given back.

`solar_eclipse.fs` is a 1,900-line program, converted from the BASIC at https://github.com/UKTailwind/PicoMite/blob/main/Bas/solar_eclipse.bas with every floating-point operation kept in the same order, so that it can be compared digit for digit. `include solar_eclipse.fs`, then `reference-run`. `solar_eclipse.out` is MMBasic's output for those inputs: from the line `searching for solar eclipse` up to the time line, the two should match byte for byte. It takes about 1.0 s on a G2, and 1.3 s on a G1. The reference records MMBasic taking 6.0 s on the same G2.
