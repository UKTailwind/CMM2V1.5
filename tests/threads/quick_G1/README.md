# The quick check's baseline on a G1

What `tools/threadcheck.py` printed and kept for the Stage 4 build of the threads work, with the context review, (branch `threads`) on the G1
(400 MHz, COM51 at 115200) on 25 November 2026: each program of `../quick.txt` with OPTION COMPILE OFF, ON and
SHADOW, the timings, the Route B programs' results and each program's stack depth. The regression programs' outputs
are the same as the Stage 3 build's on the same board (and through the stages before as v8-dev 545908b's, but for
`locals`, which recurses 18 deep since v8-dev 9527b60). The twenty-five thread tests all pass, so this is the baseline a
later step is compared with:

```
python tools/threadcheck.py COM51 OUTDIR --baud 115200 --against tests/threads/quick_G1
```

Add `--rb` to compare the Route B programs' results too, and `--stack` for the stack's depth. When a step changes an
output on purpose, or a thread test joins the list, this directory is replaced by that step's run.
