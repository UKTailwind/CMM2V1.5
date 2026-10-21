# RAY: the PicoMite's raycaster in every colour depth, in full colour

21 Oct 2026. Peter: "do the raycasting engine next and then it is time for OPTION PICOMITE", and, asked which depths:
"do the every depth, full colour design but no code changes until we understand the full impact". This is that design
and its impact. No code has been changed for it.

Sources read in full: the PicoMite's `graphics/Raycaster.c` (1,732 lines), `Raycaster.h` and `Testfiles/raycaster_demo.bas`
(268); the CMM2's `Src/Rgb121.c`, `ImageSlots.c` (ImageSlotBlit), `sprites.c` (the colour words and SpriteKey),
`OtherDisplays.c` (DrawPixel8/16/32), `ltdc.c` (pagegen), `Draw.c` (FRAMEBUFFER), `turtle.c`, `Memory.c` (HeapWipe), the
tokeniser in `MMBasic.c`, the compiler's function splice in `Stream.c`, the MPU set-up in `main.c`, the linker script
and the b2 map file. Line numbers are the CMM2's unless marked PM.

## 1. What the PicoMite's RAY is

A Wolfenstein-style raycaster (RP2350 only) that draws into WriteBuf, the 4-bit RGB121 framebuffer, two pixels a byte.

| Command | What it does |
|---|---|
| `RAY MAP w, h, map%()` or `map$()` | the grid, up to 256 x 256: 0 empty, 1-31 a wall type; a string array holds a row a string, `0`-`9` and `A`-`Z` |
| `RAY DEFINE type, fg, bg, pattern [, door]` | a wall type's two colours (0-15), Turtle pattern (0-31), and whether it is a sliding door |
| `RAY CAMERA x, y, angle [, fov]` | the viewer (fov 10-170, default 60) |
| `RAY COLOUR floor, ceiling [, floor_bg, ceiling_bg, floor_pat, ceiling_pat]` | solid, or patterned, floor and ceiling |
| `RAY MOVE speed [, strafe]`, `RAY TURN degrees` | movement with a 0.25 collision box and wall sliding |
| `RAY CELL x, y, value` | a map cell (a number or a one-character string) |
| `RAY CAST angle` | one ray, for the CAST functions |
| `RAY SPRITE id, n, x, y`, `REMOVE id`, `CLEAR` | up to 32 billboards, each a sprite buffer's image |
| `RAY DOOR x, y, offset`, `CLOSE x, y`, `CLEAR` | up to 8 sliding doors, 0 closed to 1 open |
| `RAY MINIMAP x, y, size` | a map from above, over the view |
| `RAY RENDER`, `RAY CLOSE` | draw a frame; free everything |

Functions: `RAY(MAPW | MAPH | CAMX | CAMY | CAMA | DIST col | WALL col | CELL x, y | CASTDIST | CASTWALL | CASTSIDE |
CASTX | CASTY | DOOR x, y | DEFINE type, property | SPRITES | SPRITEX id | SPRITEY id)`.

A frame, in RAY RENDER:
1. **Floor and ceiling.** For each row below the horizon its world coordinates are interpolated across the row; the
   8 x 8 pattern picks the foreground or background colour; the ceiling is the floor's mirror. The horizon row is the
   ceiling's foreground colour.
2. **Walls.** For each column a DDA walk finds the wall. A door lets the ray through its open fraction. The pattern is
   stretched twice across a cell and twice up the wall. A wall seen from the Y side is dimmed by taking the RGB121
   green down one level.
3. **Sprites.** Drawn furthest first, scaled by distance and hidden behind nearer walls column by column. A pixel whose
   4-bit value is `sprite_transparent` is skipped.

The minimap's colours are fixed indices:

| What | Index | Colour |
|---|---|---|
| empty, or a fully open door | 0 | black |
| a wall, or a closed door | the type's foreground | |
| a door that is opening | 14 | yellow |
| the border | 2 | myrtle |
| sprites | 9 | magenta (its comment says yellow) |
| the player and the heading | 15 | white |

Memory: the state is one heap block of about 0.9 KB, plus the map (a byte a cell) and 5 bytes a screen column. RAY
CLOSE frees it, and so does CloseAllFiles (RUN, NEW and the like).

The demo does MODE 2, FRAMEBUFFER CREATE and WRITE F, then a loop of RENDER, MINIMAP, two LINEs, FRAMEBUFFER COPY F, N
and PAUSE 50.

## 2. What the CMM2 has to build on

- **MODE n,4** (`Rgb121.c`): its pages are the PicoMite's framebuffers byte for byte (two pixels a byte, the left one in
  the low nibble), with the PicoMite's FRAMEBUFFER CREATE, WRITE, COPY, LAYER and MERGE. The PicoMite's renderer runs
  there as it is, on `PageTable[WritePage]`.
- **The patterns:** `turtle.c`'s `fill_patterns` are the PicoMite's, all 256 bytes the same (compared).
- **The PicoMite's 16 colours in every depth:**
  - `Rgb121Colours` (`ImageSlots.c:70`);
  - ImageSlotBlit (`ImageSlots.c:700`) already draws 4-bit values into every depth: a 16-entry table converted once
    a call, and both rows written where the page doubles them.
- **Sprite images are colour words in every depth** (Sprite plan S1):
  - `blitbuff[n].blitbuffptr`, w x h words;
  - a pixel is transparent when its colour, in the page's own format, is SpriteKey's (SpriteNative and SpriteMask,
    `sprites.c:197`), so SPRITE SET TRANSPARENT works as the PicoMite's `sprite_transparent` does.
- **Three copies of "colour word to page format":** SpriteNative (`sprites.c`), FillNative (`Draw.c:3389`) and
  ImageNative (`ImageSlots.c:691`, which makes 12-bit pixels opaque).
- **The page formats:**

  | depth | format | bytes a pixel | pages shown | doubled rows |
  |---|---|---|---|---|
  | 4 | RGB121, 2 a byte | ½ | 1 (+ the layers L and T) | never (the merge doubles them) |
  | 8 | RGB332 through the CLUT | 1 | 1 | pages 0 and 1 |
  | 12 | ARGB4444 | 2 | 0 and 1 over the background colour | pages 0, 1 and 2 |
  | 16 | RGB565 | 2 | 1 | pages 0 and 1 |
  | 32 (G2) | ARGB8888 | 4 | 1 | pages 0 and 1 |

  Rows are doubled only in modes 3, 5, 6, 7, 12, 13 and 17 (`ltdc.c:94`). MODE 7 is the PicoMite's MODE 2 size,
  320 x 240.
- **Caching:** the SDRAM that holds the displayed pages is marked shareable, which the M7 does not cache (`main.c`, the
  MPU regions up to ShareNeeded); the other pages are cached write-back. As for any CMM2 animation, the fast way is to
  draw off screen and PAGE COPY.
- **The heap:** HeapWipe forgets what lived in the heap: the tilemaps, the image slots, MATH(RAND)'s generator
  (`Memory.c:1069-1074`).
- **Flash** (b2's map):
  - bank 1 has about 26 KB left: `.data`'s load image ends at 0x080F9850, against 0x08100000;
  - bank 2's sector (FLASHB2, 128 KB, home of FM and the editor) has about 81 KB left (`_ebank2` 0x0810B63C).
- **Tokens:**
  - commands take two bytes, so there is room;
  - functions take one: the table has 125 (0x80-0xFC), so 0xFD and 0xFE are left (0xFF is best avoided: END TYPE's
    command token starts with it and line scans have tripped on it);
  - the tokeniser already folds families into one token to save them (`BIN$(` becomes `BASE$(2,`: FoldFrom and
    FoldTo, `MMBasic.c:1803`).
- **The compiler** splices only a list of functions (`Stream.c:1166`). A statement with RAY( in it, and every RAY
  command, runs through the interpreter as any other statement it does not handle. The compiler needs no change.

## 3. Design

- **D1. Where it draws: the write page.** `PageTable[WritePage]`, whatever selected it:
  - in the 4-bit depth, the PicoMite's framebuffers N, F, 2, L and T, as WriteBuf is there;
  - in the others, PAGE WRITE n, or the CMM2's FRAMEBUFFER WRITE (a page larger than the screen is drawn at its own
    size).

  RENDER and MINIMAP use the page's own width and height. The PicoMite's "No framebuffer" cannot happen.
- **D2. Colours** (R-D1). A value from 1 to 15 is the PicoMite's colour of that index; 0 is black; any other value is a
  CMM2 colour word, as RGB() gives.
  - **BLACK.** The CMM2's BLACK is RGB(0, 0, 0, 0), which is 0 (`Draw.h:90`), and the PicoMite's index 0 is black, so
    the value 0 means black in both.
    - It is drawn as every CMM2 command draws 0. In the 4, 8, 16 and 32-bit depths that is the PicoMite's black.
    - In the 12-bit depth it is transparent, as `BOX ..., RGB(BLACK)` is there. Opaque black is RGB(0, 0, 0) or
      NOTBLACK.
    - So the PicoMite's default floor (0) is see-through in the 12-bit depth: on page 0 it shows the MODE's background
      colour, and on page 1 it shows page 0.
  - **Telling an index from a colour word.**
    - Every other colour from RGB() has transparency bits, so it cannot be from 1 to 15. That covers the named colours
      and RGB(r, g, b) (0x0F000000), in the 32-bit depth too.
    - The exception is RGB(0, 0, b, 0) with b from 1 to 15: fully transparent dark blues, which would be taken as
      indices. Write a fully transparent colour as BLACK.
  - This applies to all four colours of RAY COLOUR, and to RAY DEFINE's fg and bg.
  - A value is kept as given, so RAY(DEFINE type, 0 or 1) returns it: a PicoMite program gets its index back.
  - A negative value is an error.

  How each depth draws them:
  - **4-bit:** an index or 0 is written as it is, so the output is the PicoMite's and MAP acts on it through the CLUT.
    A colour word is written as RGB121() of it, as DrawPixel4 draws any colour.
  - **Other depths:** an index is the PicoMite's colour, opaque. 0 and colour words are converted as a BOX would draw
    them.
  - **12-bit:** a colour word keeps its transparency nibble (`RGB(r, g, b, t)`). So a ceiling of BLACK, or of any
    transparency below 15, rendered on page 1 shows a sky picture on page 0 through it: a CMM2 extra that comes free.
- **D3. Colours converted once a frame.** RENDER builds a small table of page-format values:
  - each wall type's fg and bg, and their dimmed versions;
  - the floor's and ceiling's;
  - the minimap's fixed colours.

  That is about 140 conversions a frame. A change of mode or depth between frames needs nothing more, because the next
  RENDER converts for the new depth. The conversion is SpriteNative, made public as the one shared routine (ImageNative
  and FillNative can be made to call it later).
- **D4. Y-side dimming.**
  - An index is dimmed the PicoMite's way (green down a level) and then converted, so it gives the PicoMite's colours
    in every depth.
  - A colour word in a depth above 4 bits has its red, green and blue each cut to 3/4, its transparency kept (R-D2).
  - In the 4-bit depth a colour word becomes its index first, then the PicoMite's rule applies.
- **D5. Patterns and texture coordinates as the PicoMite's.** The same tables and arithmetic, so a scene looks the
  same in every depth.
- **D6. Sprites.** `RAY SPRITE id, n, x, y` takes any CMM2 sprite buffer (1-64), however it was loaded: SPRITE LOAD,
  LOADARRAY, LOADPNG, LOADBMP or READ.
  - Its colour words are sampled at render time. A pixel is skipped when it is SpriteKey's colour in the page's format
    (SHOW's rule: black, unless SET TRANSPARENT says otherwise); the others are converted as SHOW converts them.
  - Full colour in the 8, 12, 16 and 32-bit depths; RGB121 in the 4-bit one, which is the PicoMite's look.
  - A buffer closed after RAY SPRITE is skipped, as on the PicoMite.
  - Cost: a conversion for each sprite pixel drawn (three table lookups in the 16-bit depth). No page-format copy is
    kept, as decided for SHOW (Sprite plan D5), unless R2's timings call for one.
- **D7. Writing pixels.**
  - The 4-bit depth keeps the PicoMite's own loops unchanged, so its output is the PicoMite's.
  - The other depths share one set of loops, built for 1, 2 and 4 bytes a pixel (always_inline with a constant size,
    as the sprite draw routine is).
  - Floor and ceiling are written a row at a time; walls and sprites a column at a time.
  - On a page that doubles its rows, each row is drawn once at 2y and copied to 2y + 1 at the end: the whole page for
    RENDER, its own rows for MINIMAP.
  - Drawing straight onto the displayed page works, but it tears and is uncached; the manual will recommend an
    off-screen page.
- **D8. Double buffering, the recipe for each depth** (for the manual):
  - **4-bit:** the PicoMite's own: FRAMEBUFFER CREATE, WRITE F, RENDER, COPY F, N [, B], with LAYER for a HUD. A
    PicoMite program runs unchanged, its MODE 2 written MODE 7,4 until OPTION PICOMITE does that.
  - **8, 16 and 32-bit:** PAGE WRITE 1 (or any page), RENDER, MINIMAP, the HUD, PAGE COPY 1 TO 0, B.
  - **12-bit:** render on page 2 or above and copy to page 0. Or render on page 0 and keep the HUD on page 1, through
    its transparency. Or render on page 1 with a transparent ceiling over a picture on page 0.
- **D9. The mouse pointer.** When the write page is the displayed one and the pointer is showing, RENDER and MINIMAP
  hide it first and show it after. DrawPixel skips the pointer's pixels; a direct write would overwrite it.
- **D10. OPTION Y AXIS INVERT** is ignored: the view is drawn upright as a picture of the whole page, and MINIMAP's
  x, y are the page's. The PicoMite has no such option (R-D4).
- **D11. Long renders.** routinechecks every 16 rows or columns, as the 4-bit PAGE COPY does, so the audio and USB
  are seen to during an 800 x 600 or 1920 x 1080 frame.
- **D12. State and memory.** All of it is in the heap, with no fixed SDRAM area (Peter: SDRAM is for the compiler):
  - the state block, about 1.6 KB with 32-bit colours and the converted table;
  - the map, up to 64 KB;
  - 5 bytes a column (9.6 KB at 1,920 wide).

  RAY CLOSE frees it. HeapWipe forgets it, at RUN, NEW and CHAIN, as it forgets the tilemaps. It is kept through a
  MODE change; the columns are remade for the new width at the next RENDER, as on the PicoMite.
- **D13. Tokens** (R-D3). RAY is a command (two bytes, so there is room). RAY( is folded into SPRITE( rather than
  taking one of the last two one-byte slots.
  - The tokeniser writes `RAY(` as `SPRITE(RAY,`, and LIST and LIST FUNCTIONS show RAY( again, as with BIN$(.
  - fun_sprite hands it to fun_ray before its own getargs: one keyword compare at the start of each SPRITE( call.
  - SPRITE( has RAY('s types exactly (`T_FUN | T_INT | T_NBR`, `allcommands.h:677`, as the PicoMite's `Ray(`); DEVICE(
    can also give a string. It is graphics too, and RAY SPRITE draws from its buffers.
  - The compiler splices neither, so a statement with RAY( runs through the interpreter either way.
- **D14. The name.** RAY becomes a keyword, as on the PicoMite. A program with a variable or array called RAY stops
  with an error, because `ray = 1` is taken as the command.
  - None of the programs we have uses the name: the CMM2 repository (games and tests included), the goldens corpus
    and Testfiles.
  - The tokeniser's exception for TYPE and STRUCT could cover `ray =` and `ray(` at the start of a statement, but RAY(
    in an expression would still be the function, so it is not proposed.
- **D15. Where the code goes.**
  - `Src/Ray.c` and `Inc/Ray.h`, its code and constants in bank 2 (`.bank2` in the linker script, as FileManager and
    Editor are); nothing in ITCM.
  - getvalue's ITCM placement is checked unchanged in the map file (the -2^2 lesson).
  - Estimated 18-24 KB: the PicoMite's code at about 12 KB, plus the drawing loops built three more times.
  - Built (21 Oct): 12.1 KB in bank 2. tanf and floorf, which only RAY uses, brought 3.5 KB of libm (tanf's argument
    reduction): their objects are in `.bank2` too, so bank 1 grows by 288 bytes (the fold, the command, sprites.c).
- **D16. As the PicoMite's, so that its programs run unchanged:**
  - every syntax, range, message and limit;
  - the defaults: FOV 60, a black floor and cyan ceiling, wall types 1-15 green on mid-green, 16-31 yellow on brown
    and doors, the pattern type - 1;
  - the 0.25 collision box, the texture tiling and the minimap's colours.
- **D17. One PicoMite bug fixed.** `RAY MAP` with an integer array clamps a value to 32 (`NUM_PATTERNS`, PM 781), where
  the string form clamps to 31.
  - A cell of 32 or more then reads `walldefs[32]`, past the end of the state block: the next heap block's bytes are
    taken as its colours, pattern and door flag (PM 200, 224, 617, 1011, 1281).
  - The CMM2 clamps to 31. PicoMite issue #97 (21 Oct, by reading, cited at bb833ed).
  - Kept as the PicoMite's: RAY COLOUR with 3, 4 or 5 values is taken as the two-value form.

## 4. Speed

Measured on the G2 (21 Oct, `tests/bench/src/raybench.bas`, the scene of `raydepth.bas` turning a circle in 36 frames):
milliseconds a RAY RENDER on the highest page, on page 0, and a PAGE COPY of the first to page 0.

| MODE, depth | page n | page 0 | PAGE COPY |
|---|---|---|---|
| 7, 4 | 3.93 | 4.51 | 0.24 |
| 7, 8 | 3.86 | 3.67 | 0.66 |
| 7, 12 | 4.83 | 3.94 | 1.42 |
| 7, 16 | 4.21 | 3.94 | 1.28 |
| 7, 32 | 7.01 | 15.47 | 6.49 |
| 8, 4 | 34.78 | 20.69 | 1.32 |
| 8, 8 | 16.76 | 13.43 | 2.13 |
| 8, 12 | 33.30 | 33.51 | 14.04 |
| 8, 16 | 24.61 | 24.43 | 9.84 |
| 8, 32 | 40.25 | 40.84 | 28.05 |
| 1, 4 | 41.71 | 41.89 | 3.84 |
| 1, 8 | 27.36 | 20.78 | 3.33 |
| 1, 12 | 68.53 | 68.45 | 28.65 |
| 1, 16 | 41.27 | 41.41 | 16.71 |
| 1, 32 | 80.68 | 82.24 | 56.46 |

MODE 7, the PicoMite's MODE 2 size, takes 4 to 7 ms in every depth. In the bigger modes the time follows the memory
more than the estimate below did: the 4-bit depth's highest page in MODE 8 is far into the SDRAM (its pixel writes are
a read and a write of a byte each), and the 12-bit pages are slower than the 16-bit ones in the same place. Peter, on
the PicoMite's demo in MODE 7,4 and its full-colour copy in MODE 8,16: "looks good and very fast".

The estimate, before the code. What a frame writes, the page alone (PAGE COPY to a page with doubled rows writes twice that):

| mode | pixels | 4-bit | 8-bit | 12/16-bit | 32-bit |
|---|---|---|---|---|---|
| 7 (320 x 240) | 76,800 | 37.5 KB | 75 KB | 150 KB | 300 KB |
| 8 (640 x 480) | 307,200 | 150 KB | 300 KB | 600 KB | 1,200 KB |
| 1 (800 x 600) | 480,000 | 234 KB | 469 KB | 938 KB | 1,875 KB |

Every pixel is written once by the floor and ceiling pass, and the wall pixels again, about 1.5 to 2 writes a pixel. At
an estimated 20 to 30 cycles a pixel at 480 MHz, RENDER takes:
- MODE 7: 3-5 ms, inside one 75 Hz frame (13.3 ms) in every depth with room for PAGE COPY;
- MODE 8: about 13-19 ms;
- MODE 1: about 20-30 ms.

The G1, at 400 MHz with slower SDRAM, is perhaps a fifth slower.

A possible later gain: draw the floor and ceiling a column at a time, only where the wall does not cover them, which
saves the overdraw. It computes the same pattern points another way, so the floats could round differently and the
4-bit output could then differ from the PicoMite's by a pixel. Not in the first version (R-D6).

## 5. The impact

- **CMM2 users:**
  - new: the RAY command and the RAY( function, in every depth, with RGB colours;
  - broken: a program with a variable or array called RAY (none known);
  - nothing else changes.
- **PicoMite programs:**
  - a RAY program runs once its MODE 2 is MODE 7,4 (OPTION PICOMITE will do that);
  - in the 4-bit depth the output should be the PicoMite's byte for byte. Its float functions (cosf, sinf, tanf) and
    the compilers' fused multiply-adds could round a last bit differently and move a pixel; R1 compares.
- **The firmware outside RAY:**

  | File | Change |
  |---|---|
  | `sprites.c` | SpriteNative and the SpriteKey test made public (two small functions); fun_sprite hands RAY( to fun_ray (D13) |
  | `Memory.c` | one line in HeapWipe |
  | `allcommands.h` | one command entry |
  | `MMBasic.c` | one fold entry (D13) |
  | the linker script, the build list | one line each |

  Unchanged: the compiler, ITCM and the interpreter's hot paths, the sprite engine's drawing, FRAMEBUFFER and PAGE.
- **Memory:** heap only. **Flash:** about 20 KB of bank 2, leaving about 60 KB there; bank 1 is not touched.
- **Speed of everything else:** no change expected (the new code is in bank 2, and the tables only grow at their
  ends). This is checked with the micro benchmarks and the ITCM map.
- **Tests:**
  - new: `tests/cmm2/ray.bas` and `raydepth.bas`, and `tests/bench/src/raybench.bas`;
  - Testfiles' raycaster_demo goes from ERROR to running (TIMEOUT: it loops until Esc).
- **The manual:**
  - a new chapter, the RAY entries in `commands.txt` and `functions.txt`, Appendix N, `help.h`;
  - the release notes for the next beta.
- **PicoMite:** issues #97 (D17), #98 (the row above the horizon, which the floor and ceiling pass never drew: the
  CMM2's draws it in the ceiling's colour) and #99 (three messages with printf's %d and %c, which error() does not
  know).
- **Found on the way, the CMM2's own:** findvar kept a 1D string array of 12 bytes or less in its variable's record,
  its characters after the dimension table, and the counts that multiply every entry of that table took them for
  dimensions: RAY MAP of a three-row LENGTH 3 array said "String array too small" when the rows held a '-' or '#'.
  READ of such an array, GUI LISTBOX's items and the ARRAY commands were as wrong (b2 on the G1: `READ s$()` "Not
  enough internal memory"). Peter: "You can stop 1d arrays being stored like that if you want": no array is kept
  there now, as on the PicoMite (`tests/cmm2/shortarr.bas`). findvar's ITCM code is unchanged.

## 6. Decisions (Peter, 21 Oct)

- **R-D1. Colours:** the PicoMite's 16, and anything else a colour word (D2). Peter: "yes, but what about BLACK".
  Proposed in D2: 0 is BLACK in both, drawn as the CMM2 draws BLACK, so it is transparent in the 12-bit depth; the
  PicoMite's indices are then 1 to 15. Peter: "OK with black, carry on".
- **R-D2. Dimming a colour word:** red, green and blue to 3/4. Peter: "agree".
- **R-D3. RAY('s token:** folded, so the last two one-byte slots stay free for OPTION PICOMITE. Peter: "definitely
  fold but is device the best candidate? what about sprite?" Into SPRITE(, for the reasons in D13.
- **R-D4. OPTION Y AXIS INVERT:** ignored by RAY. Peter: "yes".
- **R-D5. The PicoMite issue for D17:** Peter: "yes". Filed as #97.
- **R-D6. Overdraw:** the first version keeps the PicoMite's whole floor first, then the walls, for the same 4-bit
  output. A column-at-a-time floor only if R2's timings call for it. Peter: "yes".

## 7. Stages

- **R0. Tests first.** `tests/cmm2/ray.bas`, before the code: the functions, CAST, MOVE and TURN in a known map. Its
  expected values are worked out from the code, and are exact at angles 0, 90, 180 and 270 (a cast from (2.5, 2.5)
  to a wall face at x = 5 gives 2.5). It also covers:
  - DOOR offsets and collision through an open door;
  - CELL with numbers and characters;
  - DEFINE's round trip;
  - SPRITES;
  - the errors, under ON ERROR SKIP with MM.ERRMSG$.

  It is the same in every depth.
- **R1. The 4-bit port.**
  - The code: `Ray.c`, the PicoMite's file with its renderer on `PageTable[WritePage]`; the commands and functions;
    HeapWipe; the token; bank 2.
  - Tests: ray.bas; the demo in MODE 7,4 on the G2 (Peter's look); the 4-bit page's CRC against a PicoMite's for the
    same scene, if one is run there; the frame time.
- **R2. Every depth.**
  - The code: the converted tables, the 1, 2 and 4-byte loops, doubled rows, sprites from colour words, the pointer,
    routinechecks.
  - `raydepth.bas` renders one scene in MODE 7 at 4, 8, 12, 16 and 32 bits, off screen and onto page 0 (doubled
    rows), on the G1 too without 32. The scene: walls of several types and patterns seen from both sides, a half-open
    door, sprites with transparent pixels, a patterned floor and ceiling, and the minimap.
  - Every pixel is compared with the 4-bit render's index through that depth's conversion (no differences expected),
    and a CRC is kept for each depth.
  - The speed table of section 4, measured.
- **R3. Full colour.**
  - The code: colour words in COLOUR and DEFINE, their dimming, 12-bit transparency, RAY(DEFINE)'s round trip.
  - Tests: a second scene in `raydepth.bas` in RGB, a CRC for each depth, looked at on the screen by Peter; the 12-bit
    sky through a transparent ceiling.
- **R4. The manual and release notes; full regression.** `regress_off`, the micro benchmarks and the ITCM map check.
  The manual chapter adapts the PicoMite's `Raycaster_User_Manual.md` and adds:
  - the colours (the 16 and RGB);
  - each depth's double buffering (D8);
  - the 12-bit layers;
  - the speed table.

Where it stands (21 Oct):
- R0: `tests/cmm2/ray.bas` and its expected output, written from the PicoMite's code before the port. The board's
  output matched it at the first run but for the short-array bug above.
- R1-R3, in one step: `Src/Ray.c` (the PicoMite's file, its names kept, with the page writer, the converted colours
  and the five sprite loops), `Inc/Ray.h`; the fold, the command, HeapWipe, sprites.c's NATIVE macros moved to
  sprites.h with SpriteNative, SpriteMask and SpriteKeyNative made public; bank 2. `tests/cmm2/raydepth.bas` on the G2:
  every pixel of the 8, 12, 16 and 32-bit renders is the 4-bit reference's colour, on page 5 and on page 0, every
  doubled row equals the one under it, and the RGB scene's 8, 12 and 16-bit renders match the 32-bit one on a grid of
  4,800 pixels. The 4-bit page's CRC (FD5968E2) is the number to compare with a PicoMite running the same scene.
- The PicoMite's demo (Testfiles/raycaster_demo.bas, MODE 2 as MODE 7,4) and a full-colour copy in MODE 8,16 with PAGE
  COPY run on the G2 (Peter: "looks good and very fast").
- R4: Appendix P (`docs/manual/text/45_appendix_p_raycaster.md`), the RAY and RAY( entries, a paragraph in Game
  Playing Features, Appendix N (RAY, RAY( and TURTLE now name their appendices), `help.h`. The release notes come with
  the next beta.

Then OPTION PICOMITE: its MODE 2 as MODE 7,4 makes the PicoMite's RAY programs run unchanged.
