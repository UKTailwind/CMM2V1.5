# Tests

Each file here checks itself. Start zeptoforth, and at its prompt:

```
include float64_test.fs
```

`float64_test.fs` checks double precision against IEEE doubles by their bits, and is written by `make_float64_test.py`, which has the cases. It ends with how many failed.

The rest check the words of `cmm2.fs`. Each needs `words_h.fs` first, which has `check` and `ends`, and they print `CHECKS n FAILS n`. Reboot between them. `words_files.fs`, `words_gfx.fs`, `words_io.fs`, `words_stress.fs` and `words_doc.fs` make no sound. `words_snd.fs` and `words_misc.fs` do. `words_gfx.fs` tests its own `check`, so its first two failures, marked DELIBERATE, are expected and are not counted.

`words_io.fs` looks for the loop-back wires, GP0, 1 and 3-7 to GP8, 9 and 11-15, and leaves out what needs one that is not there.

`words_hold.fs`, then a reboot or Ctrl-C, then `words_free.fs`, shows that what Forth had of the firmware is given back.
