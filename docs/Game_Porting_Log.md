# PicoMite games on the CMM2: the porting log

Each game is ported by implementing what the CMM2 lacks altogether, and by changing a local copy where the CMM2 already does the same thing under another name or syntax (Peter, 17 Oct). Every change to a game is listed here. Together they are the specification for `OPTION PICOMITE`.

The originals are kept untouched in `games/<name>/original/`; the CMM2 copies are beside them, in `games/<name>/`.

Each entry has a kind, which says what `OPTION PICOMITE` would need for it:

| Kind | Meaning | What `OPTION PICOMITE` needs |
|---|---|---|
| A | Missing on the CMM2: implemented in the firmware | nothing |
| B | The same thing under another name | a token alias in `tokenise` |
| C | The same thing with other syntax or numbering | a rewrite in `tokenise` |
| D | Different behaviour | a run-time switch |
| E | Hardware the CMM2 does not have | nothing it can do: noted only |

## Elite

From `PicoMite/Bas/elite` (built 14 Sep 2026): `elite.bas` (4,731 lines), `elite_lib.bas` (1,159 lines, loaded as a library) and `title.jpg`. It needs MODE 2 (320x240), a framebuffer, `DRAW3D`, BBC sound and a 6502-faithful flight model.

| Line | PicoMite | CMM2 | Kind | Notes |
|---|---|---|---|---|
| 3 | `LIBRARY LOAD MM.INFO(PATH) + "elite_lib.bas", RAM` | implemented (17 Oct): the PicoMite's LIBRARY, `Src/Library.c` | A | The RAM library is a heap block, read again at every RUN (the PicoMite keeps its PSRAM slot and only re-attaches). The flash library is bank 2 sector 6. Elite runs unchanged on this line; 16.92 ms a frame, as with `#INCLUDE`. |
| 4 | `MM.INFO(PSRAM SIZE)` | implemented: the SDRAM, 8 MB on a G1 and 32 MB on a G2, in bytes | A | |
| 4 | `OPTION TRACECACHE ON n` | accepted and ignored | A | The PicoMite removed the trace cache on 26 Sep (c50b6bc) and the option is now an error there too, so this Elite build no longer runs on current PicoMite firmware either. |
| 5 | `OPTION CACHE SUB name, ...` | accepted and ignored (and `OPTION CACHE DEBUG`) | A | As line 4. |
| 391 | `MODE 1` | `MODE 8,8` | C | PicoMite MODE 1 is 640x480. `tools/goldens.py` already maps MODE 1-5 (`MODEMAP`). |
| 396 | `MODE 2` | `MODE 7,12` | C | PicoMite MODE 2 is 320x240; CMM2 MODE 2 is 640x400 and was taken without an error, so the game drew at the wrong size. |
| 389, 399 | `POKE DISPLAY VRES n` | not reached once the mode is right | C | The CMM2's `POKE DISPLAY` has other arguments ("Argument count"). Only used on a screen taller than 240 rows. |
| 400 | `FRAMEBUFFER CREATE` | `PAGE WRITE 0` | C/D | The CMM2 has a `FRAMEBUFFER` of its own with another meaning (a "world" larger than the screen, with WINDOW, BACKUP and RESTORE), so the PicoMite's took no error and did the wrong thing. On the CMM2 the pages already exist. |
| 401 | `FRAMEBUFFER WRITE F` | `PAGE WRITE 2` | C/D | Page 2: in a 12-bit mode page 1 is the top layer, shown over page 0 (the PicoMite's layer buffer). MODE 7,12 has pages 0-45. |
| 23 lines | `FRAMEBUFFER COPY F, N` | `PAGE COPY 2 TO 0` | C/D | |
| 8 lines | `FRAMEBUFFER CLOSE` | `PAGE WRITE 0` | C/D | |
| 2840 | `BOX x, y, w, h, 1, cWhite, -1` | implemented: a fill of -1 is no fill in BOX and RBOX, and in the array forms of BOX, RBOX and CIRCLE | A | CIRCLE and TRIANGLE already took -1 for one fill; BOX and RBOX gave "-1 is invalid". Not in CMM1 mode, whose 8 colours are looked up in a table. |
| 1013, 1025, 1027 | `fadd = MM.INFO(WRITEBUFF) + DASHY * SCRW / 2` and `MEMORY COPY INTEGER` of the dashboard's rows to and from `dashStore()` | `fadd = 1` (a flag only); `PAGE WRITE 3 : BLIT 0, DASHY, 0, DASHY, SCRW, SCRH - DASHY, 2 : PAGE WRITE 2` to keep it, `BLIT ..., 3` to put it back | A, then D | `MM.INFO(WRITEBUFF)` is implemented (the write page's address). But the arithmetic assumes the PicoMite's 4-bit pixels, and a MODE 7 page stores each row twice (320x480, 2 bytes a pixel), so the copy is done by BLIT between pages, which is independent of the layout. `dashStore()` is no longer used. |
| 3957, 3960 | `tStage = TIMER` and `ResetTick`, around `InfoScreen kChart` | `tStage = TIMER : IF ecmActive > 0 THEN Sfx SFX_ECMOFF` and `ResetTick : IF ecmActive > 0 THEN Sfx SFX_ECM` | Elite bug | Not a CMM2 difference: a bug in Elite (PicoMite issue 39). The E.C.M. hum (duration 255) is stopped only by `ECMService`'s countdown, which runs in the flight loop, and an information screen pauses that loop. The demo opens one 120 frames (2 s) after its E.C.M., inside the 2.6 s burst, so the hum ran 16.8 s, through the screens. It now pauses with the game and resumes with it. |
