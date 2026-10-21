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

## Picanoid

From `PicoMite/Bas/picanoid` (18 Sep 2026): `picanoid.bas` (2,211 lines) and `pic_art.bmp` (the bricks, every sprite and the title screen in one image). It needs MODE 2, an RP2350's TILEMAP, BLIT FLASH from an image slot, the PicoMite's FRAMEBUFFER, BBC sound, a USB mouse and firmware 6.03.02b11 or later.

| Line | PicoMite | CMM2 | Kind | Notes |
|---|---|---|---|---|
| 2179 | `If MM.VER > MINVER Then Exit Sub` (6.03.02b11 or later) | unchanged | A | The CMM2 is 7.00.00b1 from 18 Oct (Peter), so MM.VER is 7.000001; it was 6.000007 and the game refused to start. |
| 339 | `If MM.Info(PSRAM SIZE) > 0 Then artSlot = 4` | unchanged | A | The SDRAM's size, so the art goes into RAM slot 4 (a heap block) at every start, as on a PicoMite with PSRAM. |
| 354 | `Resolution 640, 315000` | removed (a comment) | E | The PicoMite's VGA scanout setting, for its 75 Hz. CMM2 MODE 7 is 75 Hz already. |
| 355 | `Mode 2` | `Mode 7,4` | C | As for any MODE 2 program. |
| 381 | `Mode 1` | `Mode 8,8` | C | |
| 456, 766 | `Device(MOUSE MOUSEDEV, L)`, `Device(MOUSE MOUSEDEV, X)` with `MOUSEDEV = 2` | unchanged | A | The USB mouse is channel 2, as on the PicoMite (`Src/USBMouse.c`). |

## Chuckie Egg

From `PicoMite/Bas/chuckie.bas` (16 Sep 2026, 1,770 lines; its sprites and floors are DATA in the program, made by `chuckie_tools`). It needs MODE 2, the SPRITE engine (LOADARRAY, SHOW SAFE, HIDE SAFE, WRITE, COPY, RESTORE), FRAMEBUFFER CREATE/WRITE/COPY, BBC sound and KEYDOWN. Plays as on the PicoMite (Peter, 18 Oct).

| Line | PicoMite | CMM2 | Kind | Notes |
|---|---|---|---|---|
| 177 | `MODE 2` | `MODE 7,4` | C | As for any MODE 2 program. The SPRITE commands are the CMM2's (the tokeniser makes SPRITE its BLIT), whose buffers in a 4-bit mode are a byte a pixel. |

## Exile

From `PicoMite/Bas/exile` and `exile_tools/out` (16-18 Sep 2026, version 0.9.1): `exile.bas` (825 lines), the kernel `exile_lib.bas` (one CSUB, `EXILETICK`, loaded as a RAM library), the two tilesets, the two world maps and five binary tables, as `install_game.py` lists them. It needs MODE 2, `LIBRARY LOAD ..., RAM`, RAM image slots, TILEMAP LOAD/DRAW, MAP, the PicoMite's FRAMEBUFFER, `MEMORY INPUT`, `SAVE DATA`/`LOAD DATA`, KEYDOWN with the modifiers in `KEYDOWN(7)`, and firmware 6.03.02b11 or later. Works as on the PicoMite (Peter, 18 Oct).

| Line | PicoMite | CMM2 | Kind | Notes |
|---|---|---|---|---|
| 264 | `If MM.VER > MINVER Then Exit Sub` (6.03.02b11 or later) | unchanged | A | 7.00.00b1 is 7.000001. |
| 274 | `MODE 2` | `MODE 7,4` | C | As for any MODE 2 program. |
| `exile_lib.bas` 1 | `CSUB EXILETICK` | `CSUB EXILETICK  ' runs on the CMM2 ...` | A (a mark only) | The kernel is built by `armcfgen.py` for the Cortex-M0+ at -Os and calls nothing in the firmware (its state is in its argument arrays), so the M7 runs it as it is, from the RAM library's executable window. The mark is our rule for PicoMite CSUBs (`tools/goldens.py` skips unmarked ones). An M7 build (`armcfgen.py --cpu cortex-m7`) would be faster. |

## Prince of Pico

From `PicoMite/Bas/sandglass_tools` (20 Sep 2026): the engine `player.bas` (6,190 lines, stripped to 4,488 by the game's `build.py`) and `title.jpg`. The artwork, levels, animation tables and sounds are the game's own and are not distributed: the game's `setup.py` fetches Jordan Mechner's published Apple II source release and converts it, and the converted files stay on the user's machine and board (`games/pop/.gitignore` keeps them out of the repository). It needs MODE 2, RAM image slots 4-8, BLIT FLASH, FRAMEBUFFER CREATE, CREATE 2, WRITE N/F/2 and MERGE, MAP, BLIT READ/WRITE, `LOAD JPG` with its scale, MEMORY INPUT and KEYDOWN. Plays as on the PicoMite (Peter, 18 Oct).

| Line | PicoMite | CMM2 | Kind | Notes |
|---|---|---|---|---|
| 393 | `Mode 2` | `Mode 7,4` | C | As for any MODE 2 program. |
| (README) | `LOAD "A:/player.bas", C` then `RUN` | `RUN "player.bas"` | C | The CMM2 has no LOAD of a program; RUN reads the file. |

## PETSCII Robots

From the TheBackShed release `2023-12-23_061811_PETROBOT.zip` (for PicoMiteVGA 5.08.00b4): `robots.bas` (2,335 lines), 14 levels, a tileset, BMP screens, a sprite library `lib/pet_lib23.bin` with its index, and seven MOD tunes. It needs MODE 2, FRAMEBUFFER LAYER, its own font (`DefineFont #9`), BLIT MEMORY and SPRITE MEMORY of the library's packed sprites, PLAY MODFILE/MODSAMPLE, and a map read with PEEK from inside a string array. Plays as on the PicoMite (Peter, 18 Oct).

| Line | PicoMite | CMM2 | Kind | Notes |
|---|---|---|---|---|
| 298 | `spn=Peek(byte(yp+yn)*129+xpn)` | unchanged | A | Fixed in the tokeniser (18 Oct): it folded `BYTE(` into the BIT/BYTE/FLAG function, so PEEK saw a syntax error. As on the PicoMite, `PEEK(BYTE` is written `PEEK(INT8` before the fold, PEEK takes either, and LIST turns it back. |
| 1854 | `flash disk load 3, path$("lib/pet_lib23.bin"),o` | `flash disk load 4, ...`, its address read after it | A, then C | FLASH DISK LOAD is implemented (18 Oct) for the RAM slots 4-8: the file's bytes in a heap block, "Already programmed" unless O. Slots 1-3 are flash on the PicoMite and are refused here, as for images. A PicoMite slot's address is fixed before it is loaded; a CMM2 RAM slot has one only once it holds something, so `MM.INFO(FLASH ADDRESS 4)` is read after the load. |
| 7-9 | `Const NES_A_DATA = Mm.Info(PinNo GP1)` (and GP2, GP3) | `Const NES_A_DATA = 0` | E (for now) | Run on every build, used only by the NES pad driver of the PicoGAME. `MM.INFO(PINNO GPn)` and the GPnn pin mapping are deferred (Peter, 18 Oct). |
| 12 | `Const LCD_DISPLAY = Mm.Device$ = "PicoMite"` | unchanged | - | The VGA path is chosen by itself, since the CMM2's name is not "PicoMite". |
| 23 | `... Else Mode 2` | `... Else Mode 7,4` | C | |
| 78, 477, 1899, 1985 | `Load image f$ [, x, y]` | `Load BMP f$ [, x, y]` | B | The CMM2's name for it. |

## Picofrog

From `2026-03-26_035852_picofrog6_b9_3.zip` (Martin Herhaus 2022, Volhout 2025): `picofrog6_b9_3.bas` (1,356 lines). It needs MODE 2, FRAMEBUFFER LAYER, SPRITE READ/WRITE, PLAY SOUND, SETTICK, its own font and KEYDOWN. Plays as on the PicoMite (Peter, 18 Oct).

| Line | PicoMite | CMM2 | Kind | Notes |
|---|---|---|---|---|
| 51 | `const lcd%=not(instr(mm.device$,"VGA")+instr(mm.device$,"HDMI"))` | `const lcd%=0` | D | The CMM2's MM.DEVICE$ ("Colour Maximite 2 G2") has neither, so the game took its LCD path and never set MODE 2. **OPTION PICOMITE ON must report MM.DEVICE$ as "PicoMiteVGA"** (Peter, 18 Oct): that satisfies this test and PETSCII Robots' `= "PicoMite"` alike. |
| 72, 93 | `mm.info(platform)`, `mm.info(system I2C)` | unchanged | A | Implemented (18 Oct): both "", as on a PicoMite with no OPTION PLATFORM and no system I2C, so it is not a Game*Mite and no Wii controller is looked for. |
| 68-70, 91 | `Mm.Info(PinNo GP1)` ... and `if nes_possible%() then nes_init` | `= 0`, the probe commented out | E (for now) | The NES probe runs on every non-Game*Mite board and reads `MM.INFO(PIN n)` of the three pins: it needs the GPnn mapping, deferred. |
| 103 | `mode 2` | `mode 7,4` | C | |

## What OPTION PICOMITE has to do, from these ports

- MODE 2 as MODE 7,4 and MODE 3 as MODE 8,4 (MODE 1 as 8,8).
- MM.DEVICE$ as "PicoMiteVGA".
- ~~LOAD IMAGE as LOAD BMP~~: accepted by the CMM2 as it stands (19 Oct), so no option is needed for it.
- RESOLUTION accepted and ignored where the CMM2 mode already has the timing.
- GPnn pin names and MM.INFO(PINNO GPn) through the mapping table (plan 6.4), deferred.
- FRAMEBUFFER as the PicoMite's in every mode, not only in a 4-bit one.
- SPRITE LOAD's palette 0 as the PicoMite's mode 0, whose digits 8-F are myrtle, cobalt, midgreen, cerulean, rust,
  fuchsia, brown and lilac; without the option they stay the CMM2's light grey to brown (Peter, 21 Oct: "The palette
  will depend on whether OPTION PICOMITE is set as part of the compatibility mode"). Palette 1 is the PicoMite's mode 1
  either way (docs/Sprite_Plan.md, D3).
