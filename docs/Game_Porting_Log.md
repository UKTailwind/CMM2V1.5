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

## Any PicoMite MODE 2 or MODE 3 program

The PicoMite's RGB121 display is a CMM2 mode of its own since 18 Oct (`MODE n,4`, `Src/Rgb121.c`): its pages are the PicoMite's framebuffers byte for byte, and its layers are merged onto the display at each frame as the PicoMite's VGA output composites them. So the `FRAMEBUFFER` lines, `BLIT FLASH`, `TILEMAP` and the PEEKs and POKEs of the buffers stay as they are.

| Line | PicoMite | CMM2 | Kind | Why |
|---|---|---|---|---|
| any | `MODE 2` | `MODE 7,4` | C | PicoMite MODE 2 is 320x240 in 16 colours; CMM2 MODE 2 is 640x400. |
| any | `MODE 3` | `MODE 8,4` | C | 640x480 in 16 colours. |
| any | `FRAMEBUFFER CREATE`, `LAYER`, `WRITE`, `COPY`, `MERGE`, `CLOSE` | unchanged | A/D | The PicoMite's, in a 4-bit mode; in the other modes `FRAMEBUFFER` is the CMM2's own (a world larger than the screen), so a program must set the 4-bit mode first. |

## Elite

From `PicoMite/Bas/elite` (built 14 Sep 2026): `elite.bas` (4,731 lines), `elite_lib.bas` (1,159 lines, loaded as a library) and `title.jpg`. It needs MODE 2 (320x240), a framebuffer, `DRAW3D`, BBC sound and a 6502-faithful flight model.

| Line | PicoMite | CMM2 | Kind | Notes |
|---|---|---|---|---|
| 3 | `LIBRARY LOAD MM.INFO(PATH) + "elite_lib.bas", RAM` | implemented (17 Oct): the PicoMite's LIBRARY, `Src/Library.c` | A | The RAM library is a heap block, read again at every RUN (the PicoMite keeps its PSRAM slot and only re-attaches). The flash library is bank 2 sector 6. Elite runs unchanged on this line; 16.92 ms a frame, as with `#INCLUDE`. |
| 4 | `MM.INFO(PSRAM SIZE)` | implemented: the SDRAM, 8 MB on a G1 and 32 MB on a G2, in bytes | A | |
| 4 | `OPTION TRACECACHE ON n` | accepted and ignored | A | The PicoMite removed the trace cache on 26 Sep (c50b6bc) and the option is now an error there too, so this Elite build no longer runs on current PicoMite firmware either. |
| 5 | `OPTION CACHE SUB name, ...` | accepted and ignored (and `OPTION CACHE DEBUG`) | A | As line 4. |
| 391 | `MODE 1` | `MODE 8,8` | C | PicoMite MODE 1 is 640x480. `tools/goldens.py` already maps MODE 1-5 (`MODEMAP`). |
| 396 | `MODE 2` | `MODE 7,4` | C | PicoMite MODE 2 is 320x240 in RGB121; CMM2 MODE 2 is 640x400 and was taken without an error, so the game drew at the wrong size. Until 18 Oct this was `MODE 7,12`, with the FRAMEBUFFER lines turned into PAGE commands and the dashboard copied by BLIT; in MODE 7,4 they are all the PicoMite's again (below). |
| 389, 399 | `POKE DISPLAY VRES n` | not reached once the mode is right | C | The CMM2's `POKE DISPLAY` has other arguments ("Argument count"). Only used on a screen taller than 240 rows. |
| 400, 401, 25 lines, 8 lines | `FRAMEBUFFER CREATE`, `FRAMEBUFFER WRITE F`, `FRAMEBUFFER COPY F, N`, `FRAMEBUFFER CLOSE` | unchanged | A | The PicoMite's FRAMEBUFFER, in a 4-bit mode (`Src/Rgb121.c`). |
| 2840 | `BOX x, y, w, h, 1, cWhite, -1` | implemented: a fill of -1 is no fill in BOX and RBOX, and in the array forms of BOX, RBOX and CIRCLE | A | CIRCLE and TRIANGLE already took -1 for one fill; BOX and RBOX gave "-1 is invalid". Not in CMM1 mode, whose 8 colours are looked up in a table. |
| 1024 | `CIRCLE CPX, CPY, CPR + 2, 1, 1.25, cDim, -1` (the scanner) | implemented: the PicoMite's fix to a 1-pixel outline with an aspect over 1 (its 79168bf, 4 Aug) | A | The outline had gaps: the points the circle algorithm gives are more than a pixel apart once x is stretched, and the two octants stop short of each other at 45 degrees. They are now joined with short rows. `tests/cmm2/ellipse.bas` fills outlines of aspect 1 to 6 from the centre and checks that nothing leaks out. |
| 1013, 1025, 1027 | `fadd = MM.INFO(WRITEBUFF) + DASHY * SCRW / 2` and `MEMORY COPY INTEGER` of the dashboard's rows to and from `dashStore()` | unchanged | A | In MODE 7,4 the write page holds the PicoMite's 4-bit pixels, so the address arithmetic is right as it stands. |
| 3957, 3960 | `tStage = TIMER` and `ResetTick`, around `InfoScreen kChart` | `tStage = TIMER : IF ecmActive > 0 THEN Sfx SFX_ECMOFF` and `ResetTick : IF ecmActive > 0 THEN Sfx SFX_ECM` | Elite bug | Not a CMM2 difference: a bug in Elite (PicoMite issue 39). The E.C.M. hum (duration 255) is stopped only by `ECMService`'s countdown, which runs in the flight loop, and an information screen pauses that loop. The demo opens one 120 frames (2 s) after its E.C.M., inside the 2.6 s burst, so the hum ran 16.8 s, through the screens. It now pauses with the game and resumes with it. |

