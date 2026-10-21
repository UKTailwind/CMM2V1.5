# Sprites: the CMM2 engine refactored, with the PicoMite's additions

21 Oct 2026. Peter: "start the work on sprites ... The sprite code on the CMM2 is pretty convoluted and may need to be
refactored. The additional functionality of the Picomite with static sprites and the overlap function work well and are
useful additions."

Sources read in full: the CMM2's `Src/sprites.c` (2,127 lines) and what it calls (`OtherDisplays.c`, `Draw.c`, `ltdc.c`,
`Rgb121.c`, `ImageSlots.c`, `TileMap.c`); the PicoMite's `graphics/Sprite.c` (2,703), `graphics/Blit.c` (2,177) and
`docs/SPRITE_User_Manual.md`. Line numbers below are the CMM2's unless marked PM. The bugs listed were found by reading;
none has been run yet (stage S0 runs them).

## 1. What the CMM2 has

**Storage.** A sprite is not held as RGB8888. Its image and its saved background are in the *native pixel format of the
colour depth current when it was loaded*, and drawing is a raw copy:

| depth | page format | bytes a pixel | transparent |
|---|---|---|---|
| 8 | RGB332 through the CLUT | 1 | 0 |
| 4 (MODE n,4) | RGB121, packed 2 a byte in the page | 1 (the low nibble) | 0 |
| 12 | ARGB4444, two LTDC layers | 2 | 0x0000 |
| 16 | RGB565 | 2 | 0 |
| 32 | ARGB8888 | 4 | 0x00000000 |

Colours are converted once, by the loader (LOAD, LOADARRAY, LOADPNG); READ copies the page's own pixels. A change of
colour depth closes every sprite (`ltdc.c:257`), because their pixels are in the old format. Black in an image is stored
as NOTBLACK, the darkest colour that is not 0.

**Commands.** `SPRITE` is not a command of its own: the tokeniser turns it into BLIT at the start of a statement
(`MMBasic.c:2029-2044`), so SPRITE and BLIT are one command with one pool of 64 buffers, and every sub-command works
under either name (S2 gives SPRITE a token of its own for the same handler: D2). `SPRITE(` is a separate function. Sub-commands: SHOW, SHOW SAFE, HIDE, HIDE SAFE, HIDE ALL, RESTORE,
SWAP, READ, COPY, LOADARRAY, LOADPNG, LOAD (.SPR), INTERRUPT, NOINTERRUPT, CLOSE, CLOSE ALL, TRANSPARENCY (12-bit), NEXT,
MOVE, SCROLL, SCROLLR, WRITE, the bare `BLIT x1,y1,x2,y2,w,h`, and the PicoMite's FLASH, FRAMEBUFFER and MEMORY.

**Model.** Two draw lists: `zeroLIFO` (layer 0: always underneath, moves with SCROLL) and `LIFO` (layers 1-10, in show
order). The layer number is a collision group, not a z-order. Collisions are bounding boxes (touching counts), sprite to
sprite and sprite to screen edge, reported once when they start, through SPRITE(C ...), SPRITE(S) and SPRITE INTERRUPT.

**Where it is convoluted** (about a third of `sprites.c` is repetition):
- three near-identical BlitShowBuff32/16/8 (`419-586`), and per-depth if-chains in each loader (`629-657`, `715-807`,
  `965-1052`); the .SPR colour ladder again in the cursor loader (`Draw.c:330-457`);
- the colour-to-native conversion in at least five places, which disagree about 12-bit alpha;
- "hide everything / redraw everything with NEXT applied" written out in RESTORE, MOVE, SCROLL, SCROLLR and HIDE ALL;
  new-buffer set-up 4 times, link/unlink 5 times, the cursor wrapper about 13 times, three copies of the overlap loop;
- BLIT MEMORY's compressed and plain paths line for line the same but for the nibble reader (`1118-1315`);
- a GetMemory and FreeMemory for a scratch buffer on every draw (`444`, `482`).

## 2. What the PicoMite adds

The PicoMite's engine descends from the CMM2's (the same two lists, LIFO helpers and collision model). Its sprites are
always RGB121 nibbles, and since 6.00.00 SPRITE and BLIT are separate commands with separate pools.

| addition | what it is |
|---|---|
| `SPRITE STATIC #n, x, y, w, h` / `STATIC #n, OFF` / `STATIC CLEAR` | up to 64 invisible rectangles (no pixels): the program draws the scenery and declares where it is solid. Every collision check tests shown sprites against them; SPRITE SCROLL moves them with the background. |
| `SPRITE STINTERRUPT sub` / `NOSTINTERRUPT` | an interrupt for sprite-static hits (level-triggered: on every SHOW or MOVE while overlapping) |
| `SPRITE(ST, #n, X\|Y\|W\|H\|A)`, `SPRITE(ST, COLLISION)`, `SPRITE(ST, OBJECT)` | a static object's rectangle; the sprite and object of the last static hit |
| static hits in `SPRITE(C, #n, i)` as `&H80 + k` | so the ordinary collision list shows them too |
| `SPRITE(B, #n [, side 0-7])` | the overlap with the background: 0 none, 1 some background inside the rectangle but outside the sprite's opaque shape, 2 touching the shape (row and column spans of the sprite's opaque pixels, built once); with `side`, how far in from each side the hit is |
| SHOW, SHOW SAFE, SWAP orientation 4-7 | the same flips, drawn opaque |
| `SPRITE SET TRANSPARENT n` | the transparent colour (an RGB121 index there) |
| `SPRITE LOADBMP #n, f$ [, x, y, w, h]` | a sprite from a BMP, cropped |
| `SPRITE LOADPNG #n, f$ [, t] [, cutoff] [, x, y, w, h]` | cropped, decoded a row at a time, with a transparent index |
| `SPRITE LOAD f$ [, start] [, mode]` | a choice of two palettes |

Its BLIT is full colour (READ, WRITE, LOAD/LOADBMP, the bare copy), with WRITE's default copying every pixel; FLASH,
FRAMEBUFFER, MEMORY, COMPRESSED, RESIZE and MERGE work on the RGB121 buffers.

## 3. Design

**Images in ARGB8888, backgrounds native** (Peter, 21 Oct: "Should we change the way they are stored to RGB8888 in all
cases. This would mean the mode could be changed and the sprites would still be usable. It would also simplify loading
sprites").

- A sprite's *image* is ARGB8888 in every depth: the alpha byte is its transparency (0 transparent, else opaque, and in
  the 12-bit and 32-bit depths its level). It survives a change of mode or depth, every loader writes the one format,
  and opaque black is black (no NOTBLACK, in the 4-bit depth too).
- Its *saved background* stays in the screen's native format, read and restored raw as now. Through RGB888 it would not
  survive: the 8-bit depth reads back the CLUT's colour, not the index, so after MAP a background written back is a
  different colour, and the 12-bit depth's per-pixel alpha would be lost; and SHOW would convert every pixel twice.
- The compositing loop, which already visits every pixel to test transparency, converts each opaque image pixel to the
  native format as it goes (a few shifts; the conversions the loaders do today, gathered into one place).
- The conversions, per depth: ARGB to native is what every drawing command does (RGB332 or RGB121 by truncation; in
  the 12-bit and 16-bit depths the BIT_4/5/6 tables of OPTION VGA OUTPUT, so a sprite's colour is a BOX's; the 12-bit
  alpha nibble from the alpha byte; the 32-bit unchanged). Native to ARGB, for READ, is its exact inverse on every
  value those can give: bit replication for RGB332, RGB121 and the HIGH tables, and with LOW or MEDIUM a table built at
  the mode change from the BIT tables (each level's first input), as their RBIT tables are not inverses (a read sprite
  would change colour). A native 0 read is transparent, as now.
- A change of mode clears the screen, so it hides the sprites (their backgrounds are gone) instead of closing them.
- To keep what CMM2 programs rely on: READ from the screen makes black transparent (sprites drawn on black), and 0 in
  LOADARRAY is transparent. SPRITE(A) gives the address of ARGB data now.
- Cost: 4 bytes a pixel for images (a 32 x 32 sprite is 4 KB, against 23.5 MB of heap), and a conversion per drawn
  pixel, to be measured against today's in S0 and S1.

The engine is then built so:

1. **A pixel format per depth**, set by setmode beside the Read/DrawBufferFast pointers: bytes a pixel and the
   ARGB-to-native conversion, the one function every user of a colour calls (the sprites, the cursor, image slots, FILL),
   so that 12-bit alpha and near-black come out the same everywhere.
2. **One draw routine** for all depths, its inner loop specialised by bytes a pixel (1, 2, 4), with a persistent scratch
   buffer instead of a GetMemory on every draw.
3. **The bookkeeping once:** buffer set-up, link, unlink, hide-all and redraw-all, the cursor wrapper, one overlap test.
4. **The PicoMite's additions on top** (section 2), in the CMM2's colours: SET TRANSPARENT takes a colour in the depths
   that have no index, LOADBMP and the cropping LOADPNG convert through the pixel format, SPRITE(B) works on native
   pixels (exactly per pixel, as the CMM2 can afford, or the PicoMite's spans for identical results: see D4).
5. The CMM2's own extras stay: 10 layers, 8 collisions, SCROLLR, TRANSPARENCY, READ from a page.

## 4. Bugs to fix on the way (each checked on the board in S0 first)

Both engines:
- the whole-scene check (after MOVE, SCROLL, RESTORE) sets and clears the sprite's own bit in `lastcollisions` instead of
  the other sprite's (`367`, `384-388`; PM `567`, `595-601`), so later collisions are suppressed and SPRITE(T) is wrong;
- a sprite with exactly MAXCOLLISIONS entries is left out of the `#0` list (`393`; PM `608`);
- WRITE of a shown sprite moves its x, y and overwrites its saved background (`440-442`; PM `719-722`);
- SHOW SAFE of a shown sprite, without the ontop argument, ignores the new layer (`1362-1364`);
- in the whole-scene check a layer-0 sprite meets only layer-0 sprites (the single-sprite check has it meet all);
- CollisionFound is cleared by the next check before the interrupt has run.

Done in S1 (21 Oct): all of these, the CMM2's hidesafe/showsafe below, and its colours (the colour words). PicoMite
issue #88 has the collision ones.

The CMM2's:
- hidesafe and showsafe take index 0 as "not found" (`818`, `865`): a sprite at the bottom of a list leaves others hidden
  until the next MOVE (the PicoMite has fixed this with a sentinel);
- the colours: 12-bit LOADARRAY images get alpha 0 (invisible), a 12-bit RGB PNG makes black transparent, black in the
  4-bit mode becomes transparent (no near-black RGB121 value), 32-bit LOADPNG ignores its cut-off;
- a new resolution in the same depth keeps shown sprites with backgrounds from the cleared screen; CLS keeps them too;
- MoveBufferNormal's overlapping orientation-3 path overwrites the row it has just mirrored (`OtherDisplays.c:785-804`);
- plain BLIT and READ leave ReadPage changed after an error.

The PicoMite's own (checked against its code at b3c8155, to file): SWAP leaves the old image's pixels where the new one
is transparent (it draws the image alone, skipping those; the CMM2's draws the whole rectangle over the stored
background, so not there), and exchanges the copy bookkeeping and the bounds; edge entries written past `collisions[]`;
COPY's count unbounded and its copy mask's shifts; LOAD's width unchecked and its nibble toggle carried between sprites;
SHOW SAFE dropping the opaque flag; SPRITE(B) with odd widths and its side form's NULL; MODE 1 SCROLL's 1 bpp strip (by
reading); the manual's SCROLL direction.

## 5. Decisions for Peter

- **D1. Storage:** ARGB8888 images (the CMM2's colour words) with native backgrounds (section 3), from Peter's
  proposal of 21 Oct; measured in S1 (below).
- **D2. SPRITE and BLIT** (Peter, 21 Oct: "stay as one command unless there is a reason not to"): one command, one
  pool of 64. D3 gives the one reason for a change: SPRITE keeps a token of its own for the same handler, so that
  where the PicoMite's two commands differ (BLIT WRITE's default) the handler can tell which was written.
- **D3. The argument conflicts** (Peter: "as per picomite"): LOADPNG's third argument is the PicoMite's transparent
  index -15..15 (its colours the PicoMite's 16, in every depth), then its cut-off 1-254 (default 30: the old default
  cut was 128, so a soft edge's pixels of alpha 31-127 now show) and crop; `'0'` in a .SPR file is transparent black,
  with LOAD's palette argument; BLIT WRITE copies every pixel by default, SPRITE WRITE skips the transparent ones; BLIT
  LOAD is the PicoMite's other name for LOADBMP, SPRITE LOAD reads a .SPR file. The release notes say so: a CMM2 .SPR
  file whose `'0'` meant black needs another digit, and a program using LOADPNG's old cut-off 1-15 now gets a colour.
  LOAD's palettes: 0 (the default) the CMM2's own, which its manual documents (0-7 the PicoMite's mode 0 too; 8-F light
  grey, grey, orange, pink, gold, salmon, beige, brown, where the PicoMite's mode 0 has myrtle, cobalt, midgreen,
  cerulean, rust, fuchsia, brown and lilac); 1 the PicoMite's mode 1, its 16 colours in index order. Palette 0 becomes
  the PicoMite's mode 0 under OPTION PICOMITE, when that exists (Peter, 21 Oct: "The palette will depend on whether
  OPTION PICOMITE is set as part of the compatibility mode"; in its list in `Game_Porting_Log.md`).
- **D4. SPRITE(B)** (Peter: "This is new functionality so nothing to break - match picomite"): the PicoMite's row and
  column spans, with its odd-width bug fixed.
- **D5. Speed** (Peter, 21 Oct: "The performance difference is acceptable, no further work needed on that"): on the G1
  at S2, against S0, MOVE of 32 sprites of 32 x 32 is 7-19% faster in every depth, SHOW of a 64 x 64 4-5% faster in
  the 12 and 16-bit depths but 17% slower in the 8-bit and 9% in the 4-bit (the image's 4 bytes a pixel read from
  SDRAM: 16 KB against 4 KB), SCROLL the same. No native copy of the images is kept to win that back.

## 6. Stages

- **S0. Tests first.** A sprite suite in tests/cmm2 run in every depth (8, 12, 16, 32 and 4): images from LOADARRAY and
  READ, SHOW/HIDE/SAFE/MOVE/SWAP/COPY/SCROLL/SCROLLR with overlaps, orientations and the screen edges, each step's screen
  checksummed and its collisions printed. Its expected output is today's, with the bugs of section 4 marked; the games
  (Chuckie, PETSCII Robots, Picofrog) as the integration test.
  Where it stands (21 Oct): `tests/cmm2/sprites.bas` (the five depths in MODE 8; each step's screen a CRC-32 of the
  top 240 rows through MEMORY UNPACK and MATH(CRC32), with SPRITE(C, T, E, S, N, L, X, Y, V, D) and the interrupt
  count) runs on the G1 the same twice; it shows the whole-scene collision bug (after MOVE a layer-0 overlap is missed,
  and a second MOVE reports the same collisions again). CLOSE ALL now clears the collision state, which outlived a RUN
  and made the first depth's SPRITE(T) depend on the program before. `tests/bench/src/spritebench.bas` times MOVE of 32
  sprites of 32 x 32, SHOW of a 64 x 64 and SCROLL: on the G1 (400 MHz) 6.7, 21.8, 15.1 and 14.5 ms a MOVE in the 8,
  12, 16 and 4-bit depths.
- **S1. The refactor (section 3, 1-3)**, with no change of output but the bugs fixed one commit each, and the test's
  expected output corrected with each.
  Where it stands (21 Oct): one draw routine for every depth; images as colour words (READ's round trip exact, the
  suite's checksums unchanged in the 8, 16, 4 and 32-bit depths, the 12-bit ones changed by the LOADARRAY alpha fix);
  a change of mode keeps the images; the output buffer on chip (6 KB in .d2free, then a block of the on-chip heap kept
  while the heap lasts, HeapGen); hidesafe/showsafe, the collision checks, WRITE and SHOW SAFE's layer fixed (each
  change of the suite's output checked to be that fix's). The suite's expected output is the G2's, with a last part for
  LOAD of a .SPR file, TRANSPARENCY and images kept through MODE. Speed on the G2 (spritebench, ms, before and after):

  | depth | MOVE of 32 (32 x 32) | SHOW 32 x 32 | SHOW 64 x 64 | SCROLL |
  |---|---|---|---|---|
  | 8 | 5.34 / 4.51 | 0.110 / 0.092 | 0.353 / 0.389 | 2.32 / 2.35 |
  | 12 | 15.59 / 13.22 | 0.312 / 0.248 | 1.425 / 1.339 | 26.93 / 26.93 |
  | 16 | 11.66 / 9.72 | 0.243 / 0.186 | 1.023 / 0.944 | 19.08 / 19.10 |
  | 32 | 25.01 / 18.32 | 0.492 / 0.324 | 2.656 / 1.961 | 61.32 / 61.30 |
  | 4 | 11.22 / 10.13 | 0.281 / 0.252 | 0.983 / 1.021 | 30.96 / 30.98 |

  Left for S1: the colour conversion shared with the cursor, image slots and FILL (section 3, 1), the bookkeeping once
  (3), BLIT MEMORY's two paths, MoveBufferNormal's orientation 3, ReadPage after an error.
- **S2. The PicoMite's additions** (section 2), as decided in D2-D4, with tests.
  Where it stands (21 Oct): static objects (STATIC, STATIC CLEAR, STINTERRUPT, SPRITE(ST ...), the &H80 + k entries),
  SPRITE(B), orientations 4-7, SET TRANSPARENT, LOADPNG (any PNG the decoder reads, a row at a time), LOADBMP (and
  BLIT LOAD), LOAD's palette, and SPRITE a token of its own for cmd_blit (`allcommands.h`; the tokeniser no longer
  turns it into BLIT, so LIST shows what was written; a program tokenised before keeps BLIT's token). Tests:
  `tests/cmm2/sprites.bas` (its parts Statics, Overlap and Opaque), `sprpng.bas` and `sprbmp.bas` (their expected
  output from the generators' pixels, not from a run; checked on the G1 at 16-bit through its readback of each colour),
  and `platform.bas`, a platform game in miniature of statics and SPRITE(B) (`RUN "platform", play` on the board).
  The new BLIT WRITE default fixes Prince of Persia's FlipBuffer (`games/pop`), whose black pixels were skipped.
  On the way: the BMP decoder raises its errors inside decodeBMP (cleanupAndError), so LOAD BMP leaked its line and
  dither buffers on a file it could not read (256 bytes a time), as FLASH LOAD IMAGE its image; both, and LOADBMP,
  now decode into temporary memory (`sprbmp.bas` measures it). PicoMite issue #93 has it, with LOADBMP's and BLIT
  LOAD's buffer left in use there.
- **S3. The manual** (SPRITE, BLIT and SPRITE() entries, the chapter) and the release notes; the PicoMite issues.
  Where it stands (21 Oct): done with S2, in the same commit: the BLIT and SPRITE entries of `commands.txt`, SPRITE()
  in `functions.txt` (A, B, C and ST), Appendix F (static objects and the background, with a platform step checked on
  the G1), the release notes and their forum copy; the docx rebuilt. PicoMite issues #88-#93.
