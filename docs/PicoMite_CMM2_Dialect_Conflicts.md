# PicoMite (V7.00.00) vs Colour Maximite 2 (V5.07.xx) MMBasic dialect conflicts

Prepared as input to the design of a "PicoMite compatibility mode" for the CMM2 firmware.
Sources: the CMM2 User Manual text extraction (page numbers given as `p.nn`) and the PicoMite
User Manual V7.00.00 text extraction (the appended detailed-listing tables; no page numbers are
available in that extraction, so PicoMite entries are cited by listing/chapter name). Every syntax
line is quoted verbatim from the manual concerned. Where a manual does not give a syntax, this is
stated rather than guessed.

Severity key: **BREAKS** = an unchanged PicoMite program using the feature will fail to tokenise,
raise an error, or behave visibly differently on the CMM2; **minor** = difference in an edge case,
default or returned value that a typical program will survive.

---

## 0. Reading notes and caveats

1. **SPRITE is not documented as a synonym of BLIT in the CMM2 manual.** The CMM2 manual documents
   BLIT (p.68-69: `BLIT READ`, `BLIT WRITE`, `BLIT CLOSE`, `BLIT x1, y1, x2, y2, w, h [, page] [,orientation]`)
   and SPRITE (p.107-109: CLOSE, COPY, HIDE, INTERRUPT, LOAD, LOADARRAY, LOADPNG, MOVE, NEXT,
   NOINTERRUPT, RESTORE, READ, SCROLL, SCROLLR, SHOW, SHOW SAFE, SWAP, TRANSPARENCY, WRITE) as two
   separate command families. The only "synonym"/"alias" wording in the whole CMM2 manual is for `*`
   (synonym for RUN, p.66) and `LS` (alias for LIST FILES, p.87). The absence of SPRITE from the CMM2
   firmware keyword table therefore means the CMM2 tokeniser maps SPRITE onto the BLIT handler
   internally (undocumented). The PicoMite manual states that the same aliasing existed on its side
   and was removed: "In previous versions of the firmware the blit and sprite commands were synonyms
   for the same functionality. In release 6.00.00 onwards they are separate commands" (chapter "BLIT
   and Sprite Commands").
2. **The PicoMite manual does not contain the SPRITE command/function syntax.** Its SPRITE and
   SPRITE() listing entries read only "See Appendix G and the separate SPRITE_User_Manual pdf".
   Appendix G reproduces the CMM2 collision-model text (layers 0-10, LIFO, `SPRITE INTERRUPT`,
   `SPRITE(S)`, `SPRITE(C,...)`, edge codes &HF1/&HF2/&HF4/&HF8) and mentions `SPRITE SHOW` with a
   4th "layer" parameter and `SPRITE SHOW SAFE`; the interrupt-priority list also names
   `SPRITE STINTERRUPT` ("Static object collision"). A sub-command level diff of SPRITE must be
   taken from SPRITE_User_Manual.pdf, which was not available here.
3. **Keyword-table anomalies.** Several keywords reported as "in PicoMite's table but not CMM2's"
   are nevertheless documented in the CMM2 manual, so the CMM2 tokeniser must resolve them another
   way (sub-keyword of another handler, or alias): `CAT S$, N$` (p.70), `ERASE variable [,variable]...`
   (p.76), `HEX$( number [, chars])` (p.120), `BIN$( number [, chars])` (p.115: "Internally the
   functions BIN$, OCT$, and HEX$ use" BASE$), `ELSE IF expression THEN` (p.76), `END IF` (p.76),
   `EXIT DO` (p.76), `WII CLASSIC OPEN` / `WII NUNCHUK OPEN` (p.114, "See the CONTROLLER COMMAND"),
   `HELP [text]` (p.80), `VAR SAVE` (p.113). Conversely `MM.CMDLINE$` and `MM.WATCHDOG` are documented
   in the PicoMite read-only variable listing but reported absent from its keyword table. `RANDOMIZE`
   appears in the CMM2 manual only as "not supported" (p.127). These need checking in the sources
   before assuming a keyword is really missing.
4. CMM2 features marked "G2 only" (32-bit colour, MODE 16, OPTION DS3231) are treated as CMM2
   features to preserve.

---

## 1. CONFLICTS (keywords present in both dialects with differing syntax, numbering, units or behaviour)

### 1.1 Platform model: program storage, drives, paths, console

| Keyword | CMM2 syntax (page) | PicoMite syntax | Difference | Severity |
|---|---|---|---|---|
| Program storage model | "all programs reside on the SD card ... when you run a program you will run it from the SD card" (p.17); "the Colour Maximite 2 does not have the commands LOAD or SAVE as they are not required" (p.17). Program is pre-processed into flash or RAM (`OPTION FLASH [n]` / `OPTION RAM`, p.62-63). | Program lives in program memory (flash): `LOAD file$ [,C] [,R]`, `SAVE file$`, `NEW`, `FLASH SAVE n` etc. "The BASIC program is held in flash memory and is run from there" (Program and Data Storage). | PicoMite programs that `LOAD`, `SAVE`, `CHAIN`, `FLASH RUN/CHAIN`, `RAM RUN/CHAIN`, `LIBRARY LOAD`, `EDIT FILE`, `SAVE/LOAD CONTEXT` rely on a program-memory concept the CMM2 does not have. | BREAKS |
| Drive letters | "There is no drive B: in the Colour Maximite 2. The SD card is drive A:." (p.53); `CWD$` returns "A:/dir1/dir2" (p.117); `OPEN "../dir1/dir2/filename.txt" FOR INPUT AS #1` (p.95). | `DRIVE drive$` with "A:", "B:" or "C:" where "A is the flash drive, B is the SD Card and C is a USB flash drive"; prompt shortcuts `A: , B: or C:`; "Any file path that uses the drive letter must be a full path from the root (ie, "A:/mypath/myfile.txt")"; "On startup the active drive (ie, the default) is A: (the Flash Filesystem)". | A PicoMite program's `"B:/..."` (SD card) must map to the CMM2 `A:`; its `"A:/..."` (internal flash FS) has no CMM2 equivalent and must be redirected (e.g. to a directory on the SD card). `DRIVE`, `A:`/`B:`/`C:`, `COPY A2A/A2B/B2A/B2B` are unknown on the CMM2. `MM.INFO$(DRIVE)` / `MM.INFO(drive)` are PicoMite-only; CMM2 has `MM.INFO$(DIRECTORY)` (p.56). | BREAKS |
| Path length / case | "The maximum file/path length is 127 characters" (p.38); "the file system is not case sensitive" (p.38). | "63 characters (127 characters for RP2350 versions)"; storage chapter says neither filesystem is case sensitive, WiFi chapter says "the flash filesystem is case sensitive". | CMM2 is the superset. | minor |
| `RUN` | `RUN file$` / `RUN file$, cmdline` - "'file$' must be a string constant (ie, "MYPROG.BAS") ... It cannot be a variable or expression" (p.103). `*myprog any test string` (p.66). | `RUN [file$] [, cmdline$]` - "Both 'file$' and 'cmdline$' may be supplied as string expressions"; `RUN` with no file runs program memory; `*file` shortcut with argument-quoting rules. | CMM2 must accept string expressions in RUN; `RUN` with no argument must run the "current program". | BREAKS (expressions) |
| `EDIT` | `EDIT` / `EDIT "filename"` edits a file on the SD card (p.22, p.76). | `EDIT` (program memory), `EDIT fname$` (file), `EDIT FILE fname$` ("can be used in a program ... automatically save the entire variable space to the A: drive and restores it on exit"). | `EDIT FILE` form unknown on CMM2. | minor (BREAKS only if a program issues EDIT FILE) |
| `FILES` / `LIST FILES` / `LS` | `FILES` "Invoke the File Manager. To simply list the files on the SD card ... use the command LIST FILES or LS" (p.77); `LIST FILES [fspec$] [, sort]` (p.84, sort = size/time/name/type); `LS` alias (p.87). | `FILES [fspec$] [,sort]` lists files ("Can only be used at the command prompt"); `FM` opens the two-panel file manager; no `LIST FILES` or `LS` entries. | Same keyword, opposite behaviour: PicoMite `FILES "*.bas"` would open the CMM2 file manager and reject the argument. | BREAKS (prompt use) |
| `AUTOSAVE` | `AUTOSAVE file$` saves the streamed text "to a file on the SD card" (p.67), terminated by F1/F2/F6. | `AUTOSAVE` / `AUTOSAVE CRUNCH` / `AUTOSAVE APPEND` / `AUTOSAVE N` saves to program memory, terminated by Ctrl-Z/F1/F2. | File name mandatory on CMM2, absent on PicoMite. | minor (console) |
| `XMODEM` | `XMODEM SEND file$ [,comportno]` / `XMODEM RECEIVE file$ [,comportno]` (p.114). | `XMODEM SEND` / `XMODEM SEND file$` / `XMODEM RECEIVE` / `XMODEM RECEIVE file$` / `XMODEM CRUNCH`; "SEND, RECEIVE and CRUNCH can be abbreviated to S, R and C". | File-less and CRUNCH forms missing on CMM2; COM-port form missing on PicoMite. | minor |
| Console routing | `OPTION CONSOLE SCREEN` / `OPTION CONSOLE SERIAL` / `OPTION CONSOLE BOTH`, `OPTION CONSOLE SAVE`, `OPTION CONSOLE PORT n` (p.61). | `OPTION CONSOLE output` - "Valid settings are BOTH (i.e. SCREEN and SERIAL), SERIAL, SCREEN, NONE. This is a temporary option that is reset when a program exits." | `NONE` missing on CMM2; `SAVE`/`PORT` missing on PicoMite; CMM2 setting persistent via SAVE. | minor |
| Function keys at prompt | F1 FILES, F2 RUN, F3 LIST, F4 EDIT, F5 AUTOSAVE "", F6 XMODEM RECEIVE "", F7 XMODEM SEND "", F8 EDIT "", F9 LIST FILE "", F10 RUN "", F11/F12 user (p.16); `OPTION F11 string$` / `OPTION F12 string$` (p.62). | F2 RUN, F3 LIST, F4 EDIT, F5 clear screen, F10 AUTOSAVE, F11 XMODEM RECEIVE, F12 XMODEM SEND; `OPTION FNKey string$` - "'FNKey' can be F1, and F5 thru to F9". | Different programmable keys (F11/F12 vs F1,F5-F9). | minor |
| Implementation limits | Variable/label name 32 chars; 5 array dimensions; 100 nested FOR, 100 nested DO; 32 arguments to variadic commands; 1024 variables; line 240 chars (p.55). | 31 chars; 6 dimensions on RP2040 / 5 on RP2350; 20 nested FOR / 20 nested DO; 50 arguments; 240-576 variables (table); line 255 chars. | CMM2 is the superset except for the 6th dimension and 50 arguments. | minor |
| Multi-line comments | `#COMMENT START` / `#COMMENT END` (p.66). | `/*` ... `*/` "must be the first non-space characters at the start of a line". | PicoMite form is a syntax error on the CMM2. | BREAKS |
| `#INCLUDE` / `#DEFINE` | `#INCLUDE file$` (p.66, ".INC" extension required), `#DEFINE "before", "after"` (p.66). | Only via `CMM2 LOAD` / `CMM2 RUN` ("supports #INCLUDE files and #DEFINE text replacement", RP2350 only). | Same semantics; no conflict on the CMM2 side. | - |

### 1.2 Video: MODE, resolutions, colour depth, pages vs framebuffers

| Keyword | CMM2 syntax (page) | PicoMite syntax | Difference | Severity |
|---|---|---|---|---|
| `MODE` | `MODE r, bits [, bg [, int]]` (p.91). `r` = 1 (800x600), 2 (640x400), 3 (320x200), 4 (480x432), 5 (240x216), 6 (256x240), 7 (320x240), 8 (640x480), 9 (1024x768), 10 (848x480), 11 (1280x720), 12 (960x540, lines duplicated), 13 (400x300), 14 (960x540), 15 (1280x1024), 16 (1920x1080, G2), 17 (384x240). `bits` = 8 (RGB332), 12 (ARGB4444, 2 layers + background), 16 (RGB565), 32 (ARGB8888, G2). `bg` = background colour for 12-bit; `int` = subroutine called at frame blanking (p.92). Power-up default 800x600 8-bit (p.33). | VGA: `MODE 1` / `MODE 2` / `MODE 3 (RP2350 only)`; HDMI: `MODE n` with n = 1..5; the resolution is a separate persistent `OPTION RESOLUTION nn [,cpuspeedinKhz]` (640x480, 720x400, 800x600, 848x480, 1280x720, 1024x768, 1024x600, 800x480) or run-time `RESOLUTION nn`. n = 1: full-res 2-colour with colour tiles (`TILE`); 2: quarter-res 16 colours (RGB121 on VGA, palette on HDMI), one or two optional layers; 3: full-res 16 colours; 4: quarter-res RGB555 (HDMI only); 5: quarter-res 256 colours (HDMI only). Buffered LCD: `MODE 400` / `MODE 800`. Default 640x480 `MODE 1`. | Totally different numbering and semantics: PicoMite `MODE 2` means 320x240x16 whereas CMM2 `MODE 2` means 640x400 at the depth given by the mandatory second argument. A mapping table (PicoMite resolution+mode -> CMM2 r,bits) is required; several PicoMite geometries (720x400, 360x200, 424x240, 320x180, 640x360, 256x192, 512x384, 256x150, 512x300, 1024x600, 800x480, 400x240) have no exact CMM2 mode. | BREAKS |
| `OPTION RESOLUTION` / `RESOLUTION` | Not present; CMM2 uses `OPTION DEFAULT MODE n` with n = 1, 8, 9, 10, 11, 12, 14, 15, 16 (p.61-62). | `OPTION RESOLUTION nn [,cpuspeedinKhz]` ("VGA AND HDMI VERSIONS ONLY"); `RESOLUTION nn [,cpuspeedinKHz]` ("HDMI VERSIONS ONLY ... the display restarts in MODE 1"). | PicoMite-only keywords that must be accepted and mapped (same table as MODE). | BREAKS |
| `OPTION DEFAULT MODE n` | n is a CMM2 mode number (p.61). | `OPTION DEFAULT MODE n` "sets the default display mode on boot" with a PicoMite mode 1..5. | Same option, different number space. | minor (prompt-level) |
| Colour values / depth | 24-bit RGB888 everywhere, "automatically convert colours to the format required for the current colour depth" (p.33); special colour `NOTBLACK` (p.33); `RGB(red, green, blue [, trans])` / `RGB(shortcut [, trans])` with "'trans' is the level of transparency for colour depths 4 and 12 ... defaults to 15" (p.126-127). 8-bit mode uses a 256-entry CLUT set by `MAP` (p.87). | 24-bit RGB888 input "automatically translate[d] ... to the format required by the individual display controller"; VGA = RGB121 (16 fixed colours), HDMI 640x480/720x400 = RGB555, other HDMI resolutions and HDMIWEB = RGB332, MODE 5 = 256-entry palette. Many commands take a **colour index 0-15** instead of an RGB value: `BLIT MERGE colour, x, y, w, h`, `BLIT FRAMEBUFFER ... [,colour]`, `BLIT MEMORY address, x, y [,col]`, `BLIT RESIZE ... [,transparent]`, `FRAMEBUFFER LAYER [colour]`, `LOAD PNG ... [,transparent]`, `TILE x, y [,foreground] [,background]` (named colours), with the fixed order "Black, Blue, Myrtle, Cobalt, Midgreen, Cerulean, green, cyan, red, magenta, rust, fuchsia, brown, lilac, yellow and white" (RGB values in the VGA `MAP RESET` table). `RGB()` has no `trans` argument and no NOTBLACK. | CMM2 has no concept of a 0-15 colour index; the 16-colour palette and the extra names MYRTLE, COBALT, MIDGREEN, CERULEAN, RUST, FUCHSIA, LILAC must be provided as a mapping table. PicoMite sprites are "always stored as RGB121 nibbles with 2 pixels to a byte", so colour fidelity differs. | BREAKS (index-colour arguments) |
| `MAP` command | "allow the programmer to set the colours used in 8-bit colour modes" (p.87): `MAP( n ) = rgb%`, `MAP MAXIMITE`, `MAP SET`, `MAP RESET`. | Three variants: HDMI "4 or 8-bit colour modes ... RGB555"; VGA "4-bit colour modes ... one of the 16 available colours"; RP2350 buffered LCD "8-bit RGB332". `MAP( n ) = rgb%`, `MAP MAXIMITE` (VGA/HDMI), `MAP GREYSCALE` / `MAP GRAYSCALE` (HDMI), `MAP SET`, `MAP RESET` (VGA form lists the 16 defaults: 15 WHITE ... 0 BLACK). | Index range 0-15 in 16-colour modes vs 0-255; `MAP GREYSCALE` missing on CMM2; CMM2 MAP only in 8-bit modes. | minor |
| `MAP()` function | `MAP(number)` "returns the RGB332 colour that is used internally to map to the colour lookup table (CLUT) ... For example MAP(255) will always return &HE0E0C0 ... It is not a window into the CLUT" (p.122). | `MAP( n )` "Returns the 24-bit RGB value for the index 'n' in the colour map table ... MAP(8) = RGB(100,100,100) : MAP SET : Pixel x,y,map(8)". | Opposite semantics: PicoMite returns the CLUT entry, CMM2 returns the canonical colour that selects slot n. | BREAKS |
| `COLOUR` / `COLOR` | `COLOUR fore [, back]` "The background is optional and if not specified will default to black" (p.70). | `COLOUR fore [, back]` "if not specified will default to any previously set background colour or black if not previously changed". Also `COLOUR MAP inarray%(), outarray%() [,colourmap%()]` ("generates RGB888 colours in outarray% from colour codes (0-15) in inarray%"). | Default-background rule differs; `COLOUR MAP` sub-command unknown on CMM2. | minor / BREAKS (COLOUR MAP) |
| `PAGE` vs `FRAMEBUFFER` (layer model) | Numbered video pages per mode (table p.92-93, up to page 60); `PAGE WRITE n`, `PAGE COPY n TO m [,when] [,t]` (when = I/B/D; t = non-black only), `PAGE DISPLAY n [,page]`, `PAGE DISPLAY -1`, `PAGE RESIZE pageno, w, h`, `PAGE SCROLL pageno, x, y [,fillcolour]`, `PAGE STITCH frompage1, from_page_2, topage, offset`, `PAGE AND_PIXELS/OR_PIXELS/XOR_PIXELS sourcepage1, sourcepage2, destinationpage` (p.96-97). 12-bit mode: "page 0 is the lower level and page 1 the upper so the stack is: background, page 0, page 1" (p.93). Several commands take a page argument: `BLIT READ ... [,pagenumber]`, `BLIT ... [, page]`, `SPRITE READ ... [,pagenumber]`, `PIXEL(x, y [,page_number])`, `IMAGE ... [,page_number]`, `BOX AND_PIXELS ... [,pageno]`; the word `FRAMEBUFFER` is accepted as a page number (p.68, p.69, p.81, p.97, p.108, p.126). | Fixed set of named buffers selected by a one-letter code: N (physical display), F (framebuffer), 2 (second framebuffer, RP2350), L (layer buffer), T (second layer, RP2350). `FRAMEBUFFER WRITE where/where$`; the L/T layers are "automatically applied on top of the main display image as it is output to the screen" in VGA modes 2/3 and HDMI modes 2-5 with a transparent colour index. | CMM2 pages are numbered and freely copyable; PicoMite buffers are named and the L layer is composited by the video engine. A PicoMite program that draws into L and expects hardware overlay needs emulation on the CMM2 (12-bit mode page 1, or `PAGE COPY ... ,B,t` at frame blanking). | BREAKS |
| `FRAMEBUFFER` sub-commands | `FRAMEBUFFER CREATE HorizontalSize%, VerticalSize%` ("HorizontalSize>=MM.HRES and <=1600: VerticalSize>=MM.VRES and <=1200"), `FRAMEBUFFER WRITE` (no argument), `FRAMEBUFFER BACKUP`, `FRAMEBUFFER RESTORE [x, y, w, h]`, `FRAMEBUFFER WINDOW x, y, page [,I or B]`, `FRAMEBUFFER CLOSE` (p.77); "JPG files cannot be loaded to the framebuffer". | `FRAMEBUFFER CREATE` (no args), `FRAMEBUFFER CREATE 2`, `FRAMEBUFFER LAYER [colour]`, `FRAMEBUFFER LAYER TOP [colour]`, `FRAMEBUFFER WRITE where/where$` (N/F/L/2/T), `FRAMEBUFFER CLOSE [which]`, `FRAMEBUFFER COPY from, to [,b]`, `FRAMEBUFFER WAIT`, `FRAMEBUFFER MERGE [colour] [,mode] [,updaterate]` (LCD) / `FRAMEBUFFER MERGE [colour] [,b]` (VGA/HDMI RP2350), `FRAMEBUFFER SYNC` (LCD). | Same command, incompatible argument lists: `FRAMEBUFFER CREATE` without arguments and `FRAMEBUFFER WRITE N` are errors on the CMM2; LAYER/COPY/WAIT/MERGE/SYNC do not exist on CMM2; BACKUP/RESTORE/WINDOW do not exist on PicoMite. | BREAKS |
| `MM.HRES` / `MM.VRES` | Resolution of the current mode (p.56); `PAGE RESIZE` changes them (p.97). | Resolution of the current MODE at the current OPTION RESOLUTION (e.g. 320x240 in MODE 2 at 640x480). | Values follow whatever mode mapping is chosen. | minor |
| `GETSCANLINE` | "The first visible line will return a value of 0. Any line number above MM.VRES is in the frame blanking period" (p.120). | "in the range of 0 to 525. This is irrespective of the current MODE ... Any line number above 479 is in the frame blanking period." | Range depends on the physical mode; a program comparing against 479/525 will misbehave on an 800x600 CMM2 mode. | minor |
| `MM.INFO(MODE)` vs `MM.INFO$(LCDPANEL)` | `MM.INFO(MODE)` "Returns the video mode as a floating point number e.g. 1.8, 2.16" (p.57). | No MM.INFO(MODE); `MM.INFO$(LCDPANEL)` "In the case of VGA or HDMI versions returns the current mode". | Different keyword and type. | minor |
| `TILE` / `TILE HEIGHT` | Not present. | `TILE x, y [,foreground] [,background] [,nbr_tiles_wide] [,nbr_tiles_high]`, `TILE HEIGHT n` ("VGA AND HDMI VERSIONS ONLY, IN MODE 1 ONLY"); `MM.INFO(TILE HEIGHT)`. | PicoMite-only colour-attribute mechanism of the 2-colour mode; needs emulation (text-cell colouring) or a no-op. | BREAKS if used |

### 1.3 Graphics commands: BLIT, SPRITE, images, fonts, drawing

| Keyword | CMM2 syntax (page) | PicoMite syntax | Difference | Severity |
|---|---|---|---|---|
| `BLIT READ` | `BLIT READ [#]b, x, y, w, h [,pagenumber]` (p.68); "Sixty four buffers are available ranging from #1 to #64" (p.69). | `BLIT READ [#]b, x, y, w, h`; "32 buffers are available ranging from #1 to #32"; buffers "stored as RGB888 values". | Page argument CMM2-only; buffer count 64 vs 32 (CMM2 superset). | minor |
| `BLIT WRITE` | `BLIT WRITE [#]b, x, y [,orientation]` - "The optional 'orientation' parameter defaults to 4 ... &B001 = mirrored left to right, &B010 = mirrored top to bottom, &B100 = don't copy transparent pixels" (p.68). | `BLIT WRITE [#]b, x, y [,mode]` - "The optional 'mode' parameter defaults to 0" with the same three bits. | **Default differs: 4 (skip transparent) on CMM2 vs 0 (copy everything) on PicoMite.** | BREAKS (visual) |
| `BLIT CLOSE` | `BLIT CLOSE [#]b` (p.69). | `BLIT CLOSE [#]b`. | Same. | - |
| `BLIT x1,y1,...` | `BLIT x1, y1, x2, y2, w, h [, page] [,orientation]` (p.69). | `BLIT x1, y1, x2, y2, w, h` (no page/orientation). | CMM2 superset. | - |
| `BLIT LOAD` | Not present (CMM2 loads sprites with `SPRITE LOADPNG` / `SPRITE LOADARRAY`). | `BLIT LOAD[BMP] [#]b, fname$ [,x] [,y] [,w] [,h]` "load a blit buffer from a 24-bit bmp image file". | PicoMite-only sub-command. | BREAKS if used |
| `BLIT MERGE`, `BLIT FRAMEBUFFER`, `BLIT MEMORY`, `BLIT MEMORY332`, `BLIT COMPRESSED`, `BLIT FLASH`, `BLIT RESIZE` | Not present. | `BLIT MERGE colour, x, y, w, h` (LCD only); `BLIT FRAMEBUFFER from, to, xin, yin, xout, yout, width, height [,colour]` (buffers N/F/L/T/2); `BLIT MEMORY address, x, y [,col]` (packed RGB121 nibbles, header word = width/height, optional RLE); `BLIT MEMORY332 address, x, y [,col]`; `BLIT COMPRESSED address%, x, y [,col]`; `BLIT FLASH from, to, xin, yin, xout, yout, width, height [,colour]` (image slots 1-8); `BLIT RESIZE src, dst, sx, sy, sw, sh, dx, dy, dw, dh [,transparent]`. | PicoMite-only; BLIT FRAMEBUFFER/RESIZE correspond loosely to CMM2 `PAGE COPY`/`IMAGE RESIZE_FAST` but with the named-buffer model and 0-15 transparent index. | BREAKS if used |
| `SPRITE` (command) | p.107-109: `SPRITE CLOSE [#]n`, `SPRITE CLOSE ALL`, `SPRITE COPY [#]n, [#]m, nbr`, `SPRITE HIDE [#]n`, `SPRITE HIDE ALL`, `SPRITE HIDE SAFE [#]n`, `SPRITE INTERRUPT sub`, `SPRITE LOAD fname$ [,start_sprite_number]`, `SPRITE LOADARRAY [#]n, w, h, array%()`, `SPRITE LOADPNG [#]n, fname$ [, transparency_cut_off]`, `SPRITE MOVE`, `SPRITE NEXT [#]n, x, y`, `SPRITE NOINTERRUPT`, `SPRITE RESTORE`, `SPRITE READ [#]n, x , y, w, h [,pagenumber]`, `SPRITE SCROLL x, y [,col]`, `SPRITE SCROLLR x, y, w, h, delta_x, delta_y [,col]`, `SPRITE SHOW [#]n, x,y, layer, [orientation]`, `SPRITE SHOW SAFE [#]n, x,y, layer [,orientation] [,ontop]`, `SPRITE SWAP [#]n1, [#]n2 [,orientation]`, `SPRITE TRANSPARENCY [#]n, transparency`, `SPRITE WRITE [#]n, x y [,orientation]`. 64 sprites, 8 collisions (p.55). Sprites are full colour; transparency = black (8/16-bit) or alpha (12-bit) (p.50, p.53). | Syntax not in the manual (see note 0.2). Known from text: `SPRITE SHOW` with layer as 4th parameter, `SPRITE SHOW SAFE`, `SPRITE INTERRUPT`, `SPRITE STINTERRUPT`, sprites "always stored as RGB121 nibbles", enabled on in-memory framebuffers and VGA/HDMI screens. | Cannot be diffed at sub-command level from the manuals. Known differences: storage format (RGB121 vs full colour), the page argument of `SPRITE READ`, and PicoMite's static-object collision interrupt. The CMM2 implements SPRITE inside its BLIT handler (note 0.1). | unknown / likely BREAKS at edges |
| `SPRITE()` (function) | p.127-128: `SPRITE(C, [#]n )`, `SPRITE(C, [#]n, m)`, `SPRITE(D ,[#]s1, [#]s2)`, `SPRITE(E, [#]n)`, `SPRITE(H,[#]n)`, `SPRITE(L, [#]n)`, `SPRITE(N)`, `SPRITE(N,n)`, `SPRITE(S)`, `SPRITE(T, [#]n)`, `SPRITE(V,[#]s1, [#]s2)`, `SPRITE(W, [#]n)`, `SPRITE(X, [#]n)`, `SPRITE(Y, [#]n)`; edge codes &HF1/&HF2/&HF4/&HF8. | Appendix G uses `SPRITE(S)`, `SPRITE(C, 0)`, `SPRITE(C, 0, i)`, `SPRITE(C, S)`, `SPRITE(C, S, i)` and the same edge codes; the full list is in SPRITE_User_Manual.pdf. | Collision model text is identical; remaining function letters unverified. | unknown |
| `LOAD BMP` / `LOAD IMAGE` | `LOAD BMP file$ [, x, y]` (p.85). No `LOAD IMAGE`. | `LOAD IMAGE/BMP fname$ [,x] [,y] [,mode] [,ximage] [,yimage]` - `mode` = dither selector (-1 none; 0/1/2 Floyd-Steinberg RGB121/222/332; 4/5/6 Atkinson), `ximage`/`yimage` = offset into the image. | Keyword `LOAD IMAGE` unknown on CMM2; extra dither/offset arguments. | BREAKS |
| `LOAD JPG` | `LOAD JPG file$ [, x, y]` - "cannot use progressive encoding and are limited to being completely within the screen resolution ... MODE 2,16 is the optimum" (p.85). | `LOAD JPG file$ [, x] [, y] [,mode] [,ximage] [,yimage] [,scale]` (`scale` 1/2/4 pixel averaging; image larger than display is cropped). | Extra arguments on PicoMite; CMM2 errors on off-screen pixels. | minor (BREAKS with 4+ args) |
| `LOAD PNG` | `LOAD PNG file$ [, x, y] [, transparency_cut_off]` - "Valid values are 1 to 15, no default. MMBasic compares the 4 most significant bits of the transparency data in the file with the cut off value" (p.85); RGB888/ARGB8888 only. | `LOAD PNG fname$ [, x] [, y] [,transparent] [,alphacut] [,ximage] [,yimage]` - "'transparent' (defaults to 0) specifies one of the colour codes (0-15) which will be allocated to pixels in the png file with an alpha value less than 'alphacut' (defaults to 20). If 'transparent' is set to -1 ... missed completely"; RGBA8888 or indexed, no interlace. | **Third optional argument means a 4-bit alpha cut-off on CMM2 but a colour index 0-15 on PicoMite**; alphacut (0-255) is a separate 4th argument on PicoMite. | BREAKS |
| `LOAD GIF` | `LOAD GIF [file$ [, x, y]]` incl. animated GIF in background; no-argument form stops animation (p.85). | Not present. | CMM2-only. | - |
| `LOAD FONT` | `LOAD FONT file$` "install it as font #8" (p.85); font file format p.34-35. | Not present (fonts only via `DefineFont`). | CMM2-only; note that PicoMite's built-in font #8 collides with the CMM2 LOAD FONT slot. | minor |
| `LOAD DATA` | `LOAD DATA fname$, address` (p.85). | `LOAD DATA fname$, address`. | Same. | - |
| `LOAD file$` / `LOAD CONTEXT` | Not present ("does not have the commands LOAD or SAVE", p.17). | `LOAD file$ [,C] [,R]` ("LOAD with the ,R suffix is also the only form that may be used inside a running program, which is how one program can chain to another"); `LOAD CONTEXT [KEEP]`. | PicoMite-only; `LOAD "x",R` must become `RUN`. | BREAKS |
| `SAVE IMAGE` | `SAVE IMAGE file$ [,x, y, w, h]` 24-bit BMP (p.104). | `SAVE IMAGE file$ [,x, y, w, h]` / `SAVE COMPRESSED IMAGE file$ [,x, y, w, h]` ("'width', if used, must be a multiple of 2"). | COMPRESSED form PicoMite-only. | minor |
| `SAVE file$`, `SAVE CONTEXT`, `SAVE PERSISTENT` | Not present (`SAVE DATA fname$, address, size` only, p.104). | `SAVE file$`, `SAVE CONTEXT [CLEAR]`, `SAVE PERSISTENT n%`, `SAVE DATA fname$, address, nBytes`. | PicoMite-only. | BREAKS if used |
| `FONT` | `FONT [#]font-number, scaling` (p.77). Built-in fonts (p.34): 1 = 8x12 (ASCII + 7F-FF, default), 2 = 12x20, 3 = 16x24, 4 = 10x16 (ASCII + 7F-FF), 5 = 24x32, 6 = 32x50 (digits/symbols), 7 = 6x8; font #8 = LOAD FONT slot; `DefineFont` numbers "2 to 5 and 8 to 16" (p.34) / "1 to 16 ... except fonts 1, 6 and 7" (p.74). | `FONT [#]font-number, scaling`. Built-in fonts (Fonts table): 1 = 8x12 (ASCII + 7F-FF, default), 2 = 12x20, 3 = **16x16 on VGA versions / 16x24 on HDMI and LCD**, 4 = 10x16 (ASCII + 7F-FF), 5 = 24x32, 6 = 32x50, 7 = 6x8, 8 = **4x6 built-in**; `DEFINEFONT #Nbr` 1 to 16 "can be the same as a built in font and in that case it will replace the built in font". | Font 8 (CMM2 user slot vs PicoMite 4x6 built-in) and font 3 height on VGA builds (16 vs 24). Text layout that assumes PicoMite font 3 = 16 px tall or uses font 8 will differ. | BREAKS (font 8), minor (font 3) |
| `LINE` | `LINE x1, y1, x2, y2 [, LW [, C]]` - "'LW' is the line's width and is only valid for horizontal or vertical lines" (p.83). | `LINE x1, y1, x2, y2 [,[-] LW [, C]]` - "If width is given as a -ve value then width will apply to lines in all directions and they are centred"; plus `LINE AA x1, y1, x2, y2 [, LW [, C]]`, `LINE GRAPH x(), y(), colour`, `LINE PLOT ydata() [,nbr] [,xstart] [,xinc] [,ystart] [,yinc] [,colour]`. | Negative width and AA/GRAPH/PLOT forms unknown on CMM2. | BREAKS if used |
| `PIXEL FILL` vs `FILL` | `PIXEL FILL x, y, c` flood fill (p.98). | `FILL x, y, fillcolour [,bordercolour]`. | Different keyword for the same operation; PicoMite adds border-colour mode. | BREAKS |
| `BOX` | `BOX x, y, w, h [, lw] [,c] [,fill]`; "'x', 'y', 'w', and 'h' must all be arrays or all be single" (p.69); `BOX AND_PIXELS/OR_PIXELS/XOR_PIXELS x, y, w, h, colour [,pageno]` (p.69). | `BOX x, y, w, h [, lw] [,c] [,fill]`; "'x' and 'y', must both be arrays or be single ... 'w', 'h', 'lw', 'c', and fill can be either arrays or single". | Array-argument rule relaxed on PicoMite; logical-pixel forms CMM2-only. | minor |
| `ARC` | `ARC x, y, r1, [r2], rad1, rad2, colour` (p.67). | `ARC x, y, r1, [r2], a1, a2 [, c]` "Zero degrees is at the 12 o'clock position". | Same. | - |
| `CIRCLE`, `RBOX`, `TRIANGLE`, `POLYGON`, `PIXEL`, `CLS`, `TEXT`, `GUI BITMAP` | p.70, p.103, p.111, p.100, p.97, p.70, p.111, p.79 - identical argument lists. | Identical; PicoMite adds `TRIANGLE SAVE [#]n, x1,y1,x2,y2,x3,y3` / `TRIANGLE RESTORE [#]n`; TEXT alignment string "can also be unquoted (eg, CM)". | TRIANGLE SAVE/RESTORE PicoMite-only; unquoted alignment undocumented on CMM2. | minor |
| `BEZIER` | Not present. | `BEZIER x%(),y%() [,n] [,colour]`. | PicoMite-only drawing primitive. | BREAKS if used |
| `IMAGE` | `IMAGE RESIZE x, y, width, height, new_x, new_y, new_width, new_height [,page_number]`, `IMAGE RESIZE_FAST ... [,flag]`, `IMAGE ROTATE x, y, width, height, new_x, new_y, angle! [,page_number]`, `IMAGE ROTATE_FAST ... [,flag]`, `IMAGE WARP_H ...`, `IMAGE WARP_V ...` (p.81-82). | Not present (nearest: `BLIT RESIZE`). | CMM2-only. | - |
| `DEFINEFONT` | `DEFINEFONT #n` ... `END DEFINEFONT`, "'#n' ... (1 to 16). It can be the same as an existing font (except fonts 1, 6 and 7)" (p.74). | `DEFINEFONT #Nbr` ... `END DEFINEFONT`, 1 to 16, may replace any built-in font. | CMM2 forbids replacing 1, 6, 7. | minor |
| `TURTLE` | p.112: `TURTLE RESET`, `TURTLE DRAW TURTLE`, `TURTLE DRAW PIXEL x, y`, `TURTLE FILL PIXEL x, y`, `TURTLE DRAW LINE x1, y1, x2, y2`, `TURTLE DRAW CIRCLE x, y, r`, `TURTLE PEN UP`, `TURTLE PEN DOWN`, `TURTLE FORWARD n`, `TURTLE BACKWARD n`, `TURTLE DOT`, `TURTLE TURN LEFT deg`, `TURTLE TURN RIGHT deg`, `TURTLE BEGIN FILL`, `TURTLE END FILL`, `TURTLE HEADING deg`, `TURTLE PEN COLOUR col`, `TURTLE FILL COLOUR col`, `TURTLE MOVE x, y`. | Appendix H: `FORWARD distance (FD)`, `BACK distance (BK)` (BACKWARD accepted), `LEFT [angle] (LT)` / `RIGHT [angle] (RT)` (TURN LEFT/TURN RIGHT accepted, default 90), `SET XY x, y` (SETXY, MOVE), `SET X x`, `SET Y y`, `SET HEADING angle (SETH)` (HEADING accepted), `HOME`, `PEN UP (PU)`, `PEN DOWN (PD)`, `PEN COLOUR color (PC)`, `PEN WIDTH width (PW)`, `ARC radius angle`, `ARCLEFT radius,angle`, `ARCRIGHT radius,angle`, `BEZIER cp1, cp1angle, cp2, cp2angle, end, endangle`, `CIRCLE radius`, `DOT size`, `FCIRCLE radius`, `FRECTANGLE width,height`, `ARECTANGLE width,height`, `WEDGE radius start end`, `FILL COLOUR color (FC)`, `FILL PATTERN pattern (FP)`, `NO FILL`, `BEGIN FILL (BF)`, `END FILL (EF)`, `SHOW TURTLE (ST)`, `HIDE TURTLE (HT)`, `CURSOR SIZE size (CS)`, `CURSOR COLOUR color`, `STAMP`, `RESET [show]`, `PUSH`, `POP`. | Common core (RESET, PEN UP/DOWN, FORWARD, BACKWARD, TURN LEFT/RIGHT, HEADING, MOVE, BEGIN/END FILL, PEN/FILL COLOUR, DOT) is compatible. CMM2-only: `DRAW TURTLE`, `DRAW PIXEL`, `FILL PIXEL`, `DRAW LINE`, `DRAW CIRCLE x,y,r`. PicoMite-only: all abbreviations (FD, BK, LT, RT, PU, PD, PC, FC...), LEFT/RIGHT without TURN and default 90, SET X/Y/XY/HEADING, HOME, PEN WIDTH, ARC*, BEZIER, `CIRCLE radius` (at turtle position - **not** the CMM2 `DRAW CIRCLE x, y, r`), FCIRCLE, FRECTANGLE, ARECTANGLE, WEDGE, FILL PATTERN, NO FILL, SHOW/HIDE TURTLE, CURSOR SIZE/COLOUR, STAMP, PUSH, POP, `DOT size`, `RESET show`. | BREAKS (anything outside the core) |
| `GUI` controls | "full implementation of the GUI controls as popularised on the Micromite Plus ... mouse as the user interface rather than touch. GUI BEEP not implemented ... The TOUCH function is renamed CLICK ... OPTION MAXCTRLS n" (p.78-79). `GUI CURSOR ON [cursorno [, x, y [,cursorcolour] ] ]`, `GUI CURSOR x, y`, `OFF`, `HIDE`, `SHOW`, `COLOUR cursorcolour`, `LOAD "fname"`, `LINK MOUSE`, `UNLINK MOUSE` (p.79). | GUI controls documented in Advanced Graphics Functions.pdf; `OPTION GUI CONTROLS n` (0-200); `GUI CURSOR ON [n [,x, y [,colour]]]` and the same cursor sub-commands (max 24x24 loaded cursor); `GUI CLICK`, `GUI CLICK x, y`, `GUI CLICK DOWN`/`UP`, `GUI CLICK PIN pin [,INV]` / `GUI CLICK PIN OFF`; `GUI INTERRUPT TouchDown [, TouchUp]`; `GUI CALIBRATE`, `GUI TEST LCDPANEL`, `GUI TEST TOUCH`, `GUI RESET LCDPANEL`; input state via `CLICK()` or `TOUCH()`. | Option name (`OPTION MAXCTRLS` vs `OPTION GUI CONTROLS`), `GUI CLICK*`, `GUI INTERRUPT` (CMM2 manual silent; may exist via the GUI manual), touch-related tests. | BREAKS (GUI CLICK, option name) |

### 1.4 Sound: PLAY

| Keyword | CMM2 syntax (page) | PicoMite syntax | Difference | Severity |
|---|---|---|---|---|
| Audio output | Built-in stereo DAC, no configuration (p.42). | `OPTION AUDIO PWMnApin, PWMnBpin` / `OPTION AUDIO SPI ...` / `OPTION AUDIO VS1053 ...` / `OPTION AUDIO I2S BCLKpin, DINpin` / `OPTION AUDIO BLUETOOTH [idle]` / `OPTION AUDIO DISABLE` must be set first. | PicoMite programs do not need this; the OPTIONs must be accepted as no-ops. | minor |
| `PLAY TONE` | `PLAY TONE left , right [, dur [, interrupt]]` (p.98). | `PLAY TONE left, right [,dur] [,interrupt]`. | Same. | - |
| `PLAY WAV` / `FLAC` / `MP3` | `PLAY WAV file$ [, interrupt]` / `PLAY FLAC file$ [, interrupt]` / `PLAY MP3 file$ [, interrupt]`; directory = play all; max 96 kHz 24-bit (p.98). | `PLAY WAV file$ [, interrupt]` (PCM 8/16-bit, up to 48 kHz), `PLAY FLAC file$ [, interrupt]`, `PLAY MP3 file$ [, interrupt]` ("RP2350 ONLY"); "If file$ is a directory on the B: drive the Pico will play all of the files". | Same syntax; CMM2 capabilities are a superset. | - |
| `PLAY MODFILE` | `PLAY MODFILE file$ [,samplerate]` - "samplerate ... Valid values are 8000, 16000, 22050, 44100, 48000"; "will run continuously until PLAY STOP is called" (p.99). | `PLAY MODFILE file$ [,interrupt]` - "will play continuously in a loop. If the optional 'interrupt' is specified This will be called when the file has played once through the sequence and playback will then be terminated". | **Second argument is a sample rate (number) on CMM2 but an interrupt subroutine name on PicoMite**, and PicoMite stops after one pass when it is given. | BREAKS |
| `PLAY MODSAMPLE` | `PLAY MODSAMPLE sampleno, channelno [,volume] [,samplerate]` - channel 1-4, volume 0-64 default 64, samplerate default 16000 (p.99). | `PLAY MODSAMPLE Samplenum, channel [,volume]` - "volume ... between 0 and 64". | CMM2 superset (extra samplerate). | - |
| `PLAY SOUND` | `PLAY SOUND soundno, channelno, type [,frequency] [,volume]` - type "S (sine wave), Q (square wave), T (triangle wave), W (rising sawtooth), N (noise), P (periodic noise) or O (turn off sound)"; volume 1-25 (p.99). | `PLAY SOUND soundno, channelno, type [,frequency] [,volume]` - types S, Q, T, W, O, P, N "or U (user defined using PLAY LOAD SOUND)"; "In Type U mode, this parameter can also accept decimal values"; `PLAY LOAD SOUND array%()`; `OPTION FAST AUDIO ON\|OFF` controls click suppression. | Type U and PLAY LOAD SOUND PicoMite-only. | BREAKS if U used |
| `PLAY VOLUME` | `PLAY VOLUME left, right` 0-100, "linear relationship" (p.100). | `PLAY VOLUME left, right` 0-100; listing says "linear", Sound chapter says "A logarithmic scale is used so that PLAY VOLUME 50,50 should sound half as loud as 100,100". | Loudness curve may differ (manual is self-contradictory). | minor |
| `PLAY PAUSE/RESUME/STOP/NEXT/PREVIOUS` | p.99-100. | Same. | Same. | - |
| `PLAY EFFECT` | `PLAY EFFECT file$ [,interrupt]` "play the WAV file ... at the same time as a MOD file is playing" (p.98). | Not present. | CMM2-only. | - |
| `PLAY TTS` | `PLAY TTS [PHONETIC] "text" [,speed] [,pitch] [,mouth] [,throat] [, interrupt]` (p.100). | Not present. | CMM2-only. | - |
| `PLAY ARRAY`, `PLAY SAMPLE`/`RELEASE`, `PLAY BBC SOUND`/`ENVELOPE`, `PLAY MIDIFILE`/`MIDI`/`MIDI CMD`/`MIDI TEST`/`NOTE ON`/`NOTE OFF`/`HALT`/`CONTINUE`/`STREAM` | Not present. | `PLAY ARRAY l%(), r%(), freq [,start] [,end] [,terminationinterrupt]`; `PLAY SAMPLE left%(), right%(), freq, attack, decay, sustain, release [, freqR, attackR, decayR, sustainR, releaseR] [, interrupt]` / `PLAY RELEASE`; `PLAY BBC SOUND channel, amplitude, pitch, duration`; `PLAY BBC ENVELOPE n, T, PI1, PI2, PI3, PN1, PN2, PN3, AA, AD, AS, AR, ALA, ALD`; VS1053 only: `PLAY MIDIFILE file$ [, interrupt]`, `PLAY MIDI`, `PLAY MIDI CMD cmd%, data1%, data2%`, `PLAY MIDI TEST n`, `PLAY NOTE ON channel%, note%, velocity%`, `PLAY NOTE OFF channel%, note% [,velocity%]`, `PLAY HALT`, `PLAY CONTINUE track$`, `PLAY STREAM buffer%(), readpointer%, writepointer%`. | PicoMite-only. ARRAY/SAMPLE/BBC are implementable on the CMM2 DAC; MIDI/STREAM are VS1053-specific. | BREAKS if used |
| `MM.INFO$(SOUND)` | "OFF, PAUSED, TONE, WAV, MP3, MODFILE, TTS, FLAC, DAC, SOUND" (p.58). | "OFF, TONE, SOUND, BBC, WAV, FLAC, MP3, MOD, MIDI, STREAM, ARRAY or SAMPLE, or PAUSED followed by the name of the paused activity (e.g. PAUSED BBC)". | "MODFILE" vs "MOD"; "PAUSED" vs "PAUSED xxx". | minor |
| `DAC` | `DAC n, voltage`, `DAC START frequency, DAC1array%() [,DAC2array%()] [,interrupt]`, `DAC STOP` (p.73). | Not present (nearest: `PLAY ARRAY`). | CMM2-only. | - |

### 1.5 I/O pins: SETPIN, PIN, PORT, PWM, SERVO, ADC, IR, COM/SPI/I2C

| Keyword | CMM2 syntax (page) | PicoMite syntax | Difference | Severity |
|---|---|---|---|---|
| Pin naming | "An I/O pin is referred to by its pin number" 1-40 on the rear connector with fixed functions (p.14 table: 3 I2C SDA, 5 I2C SCK, 7 COUNT1/AIN, 8 COM1 TX/AIN, 10 COM1 RX/AIN, 11 COM2 RX, 12 PWM1A/AIN, 13 COUNT2/AIN, 15 COUNT3/AIN, 16 COM2 TX/AIN, 18 FAST COUNT, 19 SPI MOSI, 21 SPI MISO, 22 PWM1B/AIN, 23 SPI CLOCK, 24 COUNT4/AIN, 26 AIN, 27 I2C2 SDA, 28 I2C2 SCK, 29 PWM1C/AIN, 31 PWM2B, 32/33 general, 35 SPI2 MISO, 36 PWM2A, 37 COM1 DE/AIN, 38 SPI2 MOSI, 40 SPI2 CLOCK); pin 41 = IR, pin 42 = DS18B20 (p.15); pins 1,2,4,6,9,14,17,20,25,30,34,39 are power/ground. | "I/O pins can be referred by using the physical pin number (i.e. 1 to 40) or the GP number (i.e. GP0 to GP28) ... it is strongly recommended to use only the GP number"; string literals/variables of the form "GPn" also accepted (except PORT); RP2350B adds GP30-GP47; `MM.INFO(PINNO GPnn)`, `MM.INFO(MAX GP)`. | `GPnn` tokens are unknown on the CMM2 and the physical numbering 1-40 refers to completely different signals (e.g. PicoMite pin 1 = GP0 = COM1 TX/I2C SDA; CMM2 pin 1 = 3.3V). A pin-mapping table is mandatory and string-form pin names must be parsed. | BREAKS |
| `SETPIN pin, cfg [, option]` | cfg = OFF, AIN ("'option' ... number of bits ... 8, 10, 12, 14, and 16"), DIN (PULLUP/PULLDOWN), FIN (gate 10-100000 ms), PIN (cycles 1-10000), CIN, DOUT ("'option' can be "OC""); numeric cfg and OOUT accepted for compatibility (p.105-106). | cfg = OFF, AIN, **ARAW** ("Fast analog input returning a value between 0 and 4095"), DIN (PULLUP/PULLDOWN), FIN, PIN, CIN ("'option' ... 2 specifies a falling edge with pullup, 3 ... both ... 5 ... both edges but with a pullup"), DOUT ("'option' is not used"); counting pins restricted to GP6-GP9 (`OPTION COUNT`). | ARAW and the CIN edge option are PicoMite-only; AIN bit-count option and DOUT OC are CMM2-only. | BREAKS (ARAW) / minor |
| `SETPIN pin, cfg, target [, option]` | INTH / INTL / INTB, max 10 (p.106). | Same. | Same. | - |
| `SETPIN` device-allocation forms | Not present - peripherals have fixed pins (p.14). | `SETPIN rx, tx, COM1` (RX GP1/GP13/GP17, TX GP0/GP12/GP16/GP28), `SETPIN rx, tx, COM2`, `SETPIN rx, tx, clk, SPI`, `SETPIN rx, tx, clk, SPI2`, `SETPIN sda, scl, I2C`, `SETPIN sda, scl, I2C2`, `SETPIN pin, PWM[nx]`, `SETPIN pin, IR`, `SETPIN pin, PIOn`, `SETPIN GP1, FFIN [,gate]`, `SETPIN GP25, DOUT \| HEARTBEAT`. | Every PicoMite program that opens COM/SPI/I2C/PWM/IR issues these first; the CMM2 must accept them (validate against its fixed assignment or ignore). | BREAKS |
| `PIN()` function | `PIN( pin )`; `PIN( function )` with a **string**: "BAT", "TEMP", "DAC1", "DAC2", "SREF", "IREF" (p.125-126). | `PIN( pin )`; `PIN( BOOTSEL )`, `PIN( TEMP )` (unquoted keywords). | `PIN(TEMP)` unquoted vs `PIN("TEMP")`; BOOTSEL has no CMM2 equivalent. | minor |
| `PIN()=` command | `PIN ( pin ) = value` (p.97). | `PIN( pin ) = value`. | Same (subject to pin mapping). | - |
| `PORT` | `PORT(start, nbr [,start, nbr]...) = value` / `PORT(start, nbr [,start, nbr]...)` - pins "must be numbered consecutively" (p.101, p.126). | Same, plus "If the pins are defined using the GPn syntax then the firmware will ignore invalid pins so PORT(GP0, 8) = &B10000011 Will set eight I/O pins". | GP-sequence semantics (consecutive GP numbers, skipping power pins) cannot be reproduced with CMM2 connector numbering. | BREAKS (GPn form) |
| `PWM` | `PWM 1, freq, 1A` / `PWM 1, freq, 1A, 1B` / `PWM 1, freq, 1A, 1B, 1C` / `PWM 2, freq, 2A` / `PWM 2, freq, 2A, 2B` / `PWM channel, STOP` - two controllers (3 + 2 outputs) on fixed pins, 1 Hz - 24 MHz, duty % (p.102). | `PWM channel, frequency, [dutyA] [,dutyB][,phase][,defer]` - channels 0-7 (0-11 on RP2350B), two outputs each, pins chosen with `SETPIN pin, PWMnx`; negative duty inverts; `PWM SYNC s0 [,s1]...`; `PWM channel, OFF`. | Channel numbering (0-based, up to 12) vs 1-2; `OFF` vs `STOP`; third output 1C CMM2-only; phase/defer/SYNC PicoMite-only. | BREAKS |
| `SERVO` | `SERVO 1 [, freq], 1A` ... `SERVO 2 [, freq], 2A, 2B` / `SERVO channel, STOP` - pulse widths in **milliseconds** (0.01-18.9), freq 20-1000 Hz default 50 (p.105). | `SERVO channel [positionA] [,positionB]` - "'positionA' and 'positionB' can be between -20 and 120 and will generate a 50Hz signal between 800uSec and 2.2mSec"; pins via `SETPIN n,PWM`. | Different units (ms vs 0-100 position), no STOP/freq on PicoMite, channel numbering. | BREAKS |
| `ADC` | `ADC OPEN frequency, channel1-pin [,channel2-pin] [,channel3-pin] [, interrupt]` (pins 7,10,16,22,24,37 / 8,12,26,29 / 13,15; bit depth derived from frequency), `ADC FREQUENCY frequency`, `ADC TRIGGER channel, level`, `ADC START channel1array!() [,channel2array!()] [,channel3array!()]`, `ADC CLOSE` (p.67). | `ADC OPEN freq, n_channels [,interrupt]` (always GP26.. sequentially), `ADC FREQUENCY freq`, `ADC CLOSE`, `ADC START array1!() [,array2!()] [,array3!()] [,array4!()] [,C1min] [,C1max] ...`, `ADC RUN array1%(),array2%()`; `MM.INFO(ADC)`, `MM.INFO(ADC DMA)`. | Second OPEN argument is a pin on CMM2 but a channel count on PicoMite; 4 channels, min/max scaling and RUN mode PicoMite-only; TRIGGER CMM2-only. | BREAKS |
| `IR` | `IR dev, key , int` / `IR CLOSE` "It should be connected to the IR pin" (fixed, pin 41) (p.83). `IR SEND pin, dev, key` (p.83). | `IR dev, key , int` / `IR CLOSE` - "this pin must be configured in advanced using the command: SETPIN n, IR". `IR SEND pin, dev, key`. | PicoMite needs the SETPIN first (CMM2 must accept and ignore / remap to pin 41). | minor |
| `HUMID`, `TEMPR()`, `TEMPR START`, `DISTANCE()`, `PULSIN()`, `PULSE`, `ONEWIRE` | `HUMID pin, tvar, hvar [,DHT11]` (p.80); `TEMPR( pin )` (p.129); `TEMPR START pin [, precision]` (p.110); `DISTANCE( trigger, echo )` (p.118); `PULSIN( pin, polarity [, t1 [, t2]] )` (p.126); `PULSE pin, width` (p.102); `ONEWIRE RESET/WRITE/READ` (p.94). | Same; PicoMite adds `TEMPR( pin [,timeout])`, `TEMPR START pin [, precision] [,timeout]`, ONEWIRE/SPI/I2C data as lists, arrays or strings. | Optional timeout PicoMite-only. | minor |
| `OPEN "COMn:..."` | `OPEN comspec$ AS [#]fnbr` with `"COMn: baud, buf, int, int-trigger, (DEN or DEP), 7BIT, (ODD or EVEN), INV, OC, S2"`; COM1:, COM2:, COM3: (console port when OPTION CONSOLE SCREEN); baud "1200 (the minimum) and 1000000" (p.95-96, p.132). | `"COMn: baud, buf, int, int-trigger, EVEN, ODD, S2, 7BIT"` plus INV; COM1/COM2 need `SETPIN rx, tx, COMn` first; COM3-COM6 = USB serial devices, COM7 = Bluetooth; baud "1200 to 921600". | DEN/DEP/OC CMM2-only; COM3+ meaning differs (console port vs USB CDC); SETPIN prerequisite. | minor (BREAKS only for COM3-7 semantics) |
| `BAUDRATE()` | `BAUDRATE( comm [, timeout] )` (p.115). | Not present. | CMM2-only. | - |
| `SPI OPEN` | `SPI OPEN speed, mode, bits` - speed one of 25000000 ... 195315, "'bits' ... can be 8, 16 or 32" (p.138); fixed pins; `SPI2` identical. | `SPI OPEN speed, mode, bits` - any speed (CPUspeed/2 divided by 1-256), "'bits' ... any number in the range of 4 to 16 bits"; `SETPIN rx, tx, clk, SPI` first; `SPI READ nbr, data [, data...]` accepts variable lists/strings. | 32-bit transfers CMM2-only; 4-7 bit transfers PicoMite-only; pin allocation. | minor |
| `I2C` | `I2C OPEN speed, timeout`, `I2C WRITE addr, option, sendlen, senddata [,sendata ....]`, `I2C READ addr, option, rcvlen, rcvbuf`, `I2C CLOSE`; "I2C (no suffix) refers to channel 1 while commands I2C2 and I2C3 refer to channels 2 and 3" (p.80-81, p.135); I2C3 = Wii connector; master only. | Same four commands for I2C and I2C2 (+ `I2C CHECK addr`), plus slave mode `I2C SLAVE OPEN addr, send_int, rcv_int`, `I2C SLAVE WRITE sendlen, senddata`, `I2C SLAVE READ rcvlen, rcvbuf, rcvd`, `I2C SLAVE CLOSE`; `SETPIN sda, scl, I2C` first; `OPTION SYSTEM I2C` makes a permanently open bus ("I2C(2) OPEN and I2C(2) CLOSE are not available in this case"). | I2C CHECK and slave mode PicoMite-only; I2C3 CMM2-only; SYSTEM I2C concept (RTC, Wii, LCD) PicoMite-only. | BREAKS (slave/CHECK), minor otherwise |
| `SETTICK` | `SETTICK period, target [, nbr]` and `SETTICK FAST frequency, target` (p.106). | `SETTICK period, target [, nbr]` and `SETTICK PAUSE, target [, nbr]` / `SETTICK RESUME, target [, nbr]`. | PAUSE/RESUME PicoMite-only; FAST CMM2-only. | BREAKS if PAUSE/RESUME used |
| `WATCHDOG` | `WATCHDOG timeout` / `WATCHDOG OFF` (p.113). | Adds `WATCHDOG HW timeout` / `WATCHDOG HW OFF` (max 8.3 s). | HW form PicoMite-only (can alias to the software watchdog). | minor |
| `CPU` | `CPU RESTART` only (p.73). | `CPU RESTART`, `CPU SLEEP n` ("sleep for 'n' seconds"), `CPU SPEED n` (kHz, "NOT VGA OR HDMI VERSIONS"). | SLEEP/SPEED PicoMite-only. | BREAKS if used |
| `BITBANG` / `WS2812` / `BITSTREAM` | `BITBANG BITSTREAM pinno, n_transitions, array%()`; `BITBANG WS2812 type, pin, colours%()` (p.68). | `BITBANG` is obsolete ("will be automatically converted to DEVICE"); `BITSTREAM pin1, count1, array1() [, mode1] [,pin2, count2, array2()] [,mode2] [,logic]`; `WS2812 type, pin, nbr, value%[()]` with types O, B, S, W. | **WS2812 has an extra `nbr` argument on PicoMite and may take a scalar**; BITSTREAM is a top-level command with extra modes; type W (RGBW) PicoMite-only. | BREAKS |

### 1.6 Input devices: mouse, Wii controllers, keyboard, touch/click

| Keyword | CMM2 syntax (page) | PicoMite syntax | Difference | Severity |
|---|---|---|---|---|
| Mouse open/close | `CONTROLLER MOUSE OPEN [n] [,LBinterrupt ][,RBinterrupt] [,sensitivity][,LBUpinterrupt]` / `CONTROLLER MOUSE CLOSE [n]` (Hobbytronic I2C, default channel 2 = pins 27/28); `CONTROLLER MOUSE OPEN 0 [,LBinterrupt [,RBinterrupt] ]` / `CONTROLLER MOUSE CLOSE 0` (PS2 mouse) (p.71-72); `OPTION MOUSE n [,sensitivity]` / `OPTION MOUSE OFF` (p.63). | `MOUSE OPEN channel, CLKpin, DATApin` / `MOUSE CLOSE channel` (PS2, "Channel ... must be set to 2"), `MOUSE INTERRUPT ENABLE channel, int`, `MOUSE INTERRUPT DISABLE channel`, `MOUSE SET channel, x-coord, y-coord [, wheel-count]`; USB mice need nothing; `OPTION MOUSE CLKpin, DATApin` / `OPTION MOUSE DISABLE` / `OPTION MOUSE SENSITIVITY f!`. | Command name (`MOUSE` vs `CONTROLLER MOUSE`), argument meaning (channel = USB port 1-4 / fixed 2 vs I2C channel 0-3), INTERRUPT ENABLE/DISABLE and SET sub-commands, OPTION MOUSE argument types. | BREAKS |
| Mouse read | `MOUSE(funct [, channel])` funct = X, Y, L, R, W (wheel button), D (double click), T (type; 0 or 3) (p.124). | `DEVICE(MOUSE channel, funct)` funct = X, Y, L, R, M (middle), D, W ("delta of the wheel position"), B (button bitmap). | Function name, channel is **mandatory and first** on PicoMite; W means wheel delta (PicoMite) vs wheel-button state (CMM2); M/B PicoMite-only; T CMM2-only. | BREAKS |
| Wii Classic | `CONTROLLER CLASSIC OPEN [n] [,interrupt [,bitmask] ]` / `CONTROLLER CLASSIC CLOSE [n]` (p.71); alias `WII CLASSIC OPEN [n] [,interrupt [,bitmask] ]` / `WII CLASSIC CLOSE [n]` (p.114); function `CLASSIC(funct [, channel])` funct = LX, LY, RX, RY, L, R, B, T (T "should be hex &H4200101") (p.116-117). | `WII [CLASSIC] OPEN [,interrupt]` / `WII [CLASSIC] CLOSE` (needs `OPTION SYSTEM I2C`, polled at 50 Hz, interrupt "if any of the buttons changes (both on and off)"); function `DEVICE(WII [CLASSIC] funct)` same funct codes, T "should be hex &HA4200101"; same 15-bit button map. | `WII CLASSIC OPEN` syntax is compatible except that CMM2 has a channel and bitmask; the **function** differs: `CLASSIC(B)` vs `DEVICE(WII CLASSIC B)`; interrupt fires on press only (CMM2) vs any change (PicoMite). | BREAKS (function) |
| Wii Nunchuk | `CONTROLLER NUNCHUK OPEN [n] [,Zinterrupt [,Cinterrupt] ]` / `CONTROLLER NUNCHUK CLOSE [n]` (p.72); alias `WII NUNCHUK OPEN [n] [,Zinterrupt [,Cinterrupt] ]` (p.114); function `NUNCHUK(channel, funct)` ("'channel' is optional") funct = JX, JY, AX, AY, AZ, Z, C, T, JXL, JXC, JXR, JYT, JYC, JYB, AX0, AX1, AY0, AY1, AZ0, AZ1 (p.124-125). | `WII NUNCHUCK OPEN [,interrupt]` / `WII NUNCHUCK CLOSE`; function `DEVICE(NUNCHUCK funct)` funct = AX, AY, AZ, JX, JY, C, Z, T. | **Spelling: NUNCHUK (CMM2) vs NUNCHUCK (PicoMite)**; one interrupt vs two; calibration codes CMM2-only; function name differs. | BREAKS |
| USB gamepad | Not present. | `GAMEPAD COLOUR channel, colour`, `GAMEPAD HAPTIC channel left, right`, `GAMEPAD INTERRUPT ENABLE channel, int [,mask]`, `GAMEPAD INTERRUPT DISABLE channel`, `GAMEPAD MONITOR [SILENT]`, `GAMEPAD CONFIGURE vid,pid,i0,c0,...,i15,c15`; `DEVICE(GAMEPAD channel, funct)` funct = LX, LY, RX, RY, GX, GY, GZ, AX, AY, AZ, L, R, B, T. | PicoMite-only (USB host). The CMM2 keyboard port cannot host a gamepad; could be mapped onto Wii Classic. | stub/error or Wii mapping |
| `KEYDOWN(n)` | `KEYDOWN(n)`: n = 0 count, 1-6 keys in press order, 7 modifier bitmask (lalt 1, lctrl 2, lgui 4, lshift 8, ralt 16, rctrl 32, rgui 64, rshift 128), 8 lock bitmask (caps 1, num 2, scroll 4); codes in Appendix F (p.121, p.142). | Identical definition and bitmasks; key codes in Appendix I identical for the listed keys (DEL 7F ... F12 9C, PrtScr 9D, PAUSE 9E, SHIFT_TAB 9F, SHIFT_DEL A0, SHIFT_DOWN A1, SHIFT_RIGHT A3) plus SHIFT_LEFT_ARROW A2 and SHIFT_UP_ARROW A4. | **Shift-function-key offset differs: CMM2 adds &H40 (Shift-F10 = &HDA, p.142) whereas PicoMite adds &H20 (Shift-F10 = &HBA)**. | minor (BREAKS for shifted F-keys) |
| `INKEY$`, `INPUT$`, `INPUT`, `LINE INPUT` | p.120, p.121, p.82-84: identical. | Identical (INPUT$ "#0 can be used which refers to the console's input buffer"). | None. | - |
| `ON KEY` | `ON KEY target` / `ON KEY ASCIIcode, target` "for both the USB keyboard and a serial console" (p.94). | Same. | None. | - |
| `ON PS2` | Not present. | `ON PS2 target` + `MM.PS2` / `MM.INFO(PS2)` raw scan codes. | PicoMite-only. | stub |
| Keyboard layout | `OPTION USBKEYBOARD nn [,noLED]` (US, UK, DE, FR, ES) (p.64); `OPTION KEYBOARD REPEAT firstchar, nextchar` (p.62); `OPTION CRLF mode` (p.61). | `OPTION KEYBOARD nn [,capslock] [,numlock] [,repeatstart] [,repeatrate]` (US, UK, FR, GR, IT, ES, BE, BR), `OPTION KEYBOARD DISABLE`, `OPTION KEYBOARD I2C`, `OPTION KEYBOARD PINS clockpin, datapin` / `OPTION PS2 PINS`, `OPTION KEYBOARD REPEAT repeatstart , repeatrate` (same ms ranges as CMM2), `OPTION LOCAL KEYBOARD ON\|OFF`, `OPTION SCREEN KEYBOARD nn [, M]`, `KEYBOARD ON` / `KEYBOARD OFF`. | `OPTION KEYBOARD nn` (layout) is the PicoMite spelling of CMM2 `OPTION USBKEYBOARD nn`; Germany is "GR" on PicoMite but "DE" on CMM2; `KEYBOARD ON/OFF` is PicoMite-only. | minor / BREAKS (KEYBOARD command) |
| Touch / click | "The TOUCH function is renamed CLICK" (p.79); no touch hardware. | `TOUCH(X)`, `TOUCH(Y)`, `TOUCH(DOWN)`, `TOUCH(UP)`, `TOUCH(X2)`, `TOUCH(Y2)`, `TOUCH(XN n)`, `TOUCH(YN n)`, gestures `TOUCH(TAP)`, `DTAP`, `HOLD`, `SWL/SWR/SWU/SWD`, `SWIPE`, `EXPAND`, `CONTRACT`, `PINCH`, `CW`, `CCW`, `ROTATE`, `TTAP`; `CLICK(DOWN)`, `CLICK(UP)`, `CLICK(X)`, `CLICK(Y)`, `CLICK(REF)`, `CLICK(LASTREF)`, `CLICK(LASTX)`, `CLICK(LASTY)` ("this function is compatible with the Colour Maximite 2"). | `TOUCH()` family unknown on CMM2 - alias the X/Y/DOWN/UP subset to CLICK(); gestures and multi-touch have no equivalent. | BREAKS (TOUCH) |

### 1.7 Options present in both dialects with different arguments or defaults

| Option | CMM2 (page) | PicoMite | Difference | Severity |
|---|---|---|---|---|
| `OPTION AUTORUN` | `OPTION AUTORUN OFF \| ON`; with OPTION RAM looks for AUTORUN.BAS (p.60). | `OPTION AUTORUN ON [,NORESET]` / `OPTION AUTORUN n [,NORESET]` / `OPTION AUTORUN OFF` (n = flash slot 1-3). | Slot form and NORESET PicoMite-only. | minor |
| `OPTION COLOURCODE` | `ON` / `OFF` / `REVERSE`, "The default is ON" (p.61). | `ON` / `OFF`, "The default is OFF". | REVERSE CMM2-only; default differs. | minor |
| `OPTION CONSOLE` | see 1.1. | see 1.1. | NONE vs SAVE/PORT. | minor |
| `OPTION DEFAULT` | `FLOAT \| INTEGER \| STRING \| NONE` (p.61); `OPTION DEFAULT MODE n` (CMM2 modes). | `FLOAT \| INTEGER \| STRING \| NONE`; `OPTION DEFAULT COLOURS foreground [,background]` (named colours only); `OPTION DEFAULT MODE n` (PicoMite modes). | COLOURS sub-option PicoMite-only; MODE numbers differ. | minor |
| `OPTION KEYBOARD` | Only `OPTION KEYBOARD REPEAT firstchar, nextchar` (p.62). | `OPTION KEYBOARD nn ...` (layout) and the forms listed in 1.6. | Layout form is CMM2 `OPTION USBKEYBOARD`. | minor |
| `OPTION LEGACY` | `OPTION LEGACY ON \| OFF` - "Commands such as LINE, CIRCLE and PIXEL work as they originally did and all drawing commands will accept colours in the range of 0 to 7" (p.62-63), also BOX (p.52). | `OPTION LEGACY ON \| OFF` - "The commands COLOUR, LINE, CIRCLE and PIXEL use the legacy syntax". | Same intent; COLOUR listed only on PicoMite. | minor |
| `OPTION MILLISECONDS` | `ON \| OFF` (p.63). | `ON\|OFF`; "The milliseconds counter is set to zero whenever the time is updated". | Same. | - |
| `OPTION MOUSE` | `OPTION MOUSE n [,sensitivity]` / `OPTION MOUSE OFF` (p.63). | `OPTION MOUSE CLKpin, DATApin` / `OPTION MOUSE DISABLE` / `OPTION MOUSE SENSITIVITY f!`. | Argument meaning (channel vs pins), OFF vs DISABLE, SENSITIVITY sub-option. | minor (prompt-level) |
| `OPTION PROFILING` | `OPTION PROFILING ON\|OFF` + `LIST PROFILE` / `LIST PROFILE CSV` (p.63, p.84). | `OPTION PROFILING ON` / `OFF` / `ON, SAMPLE [, entries]`; report printed automatically at END; no LIST PROFILE. | Output mechanism differs. | minor |
| `OPTION RESET` | `OPTION RESET` (p.64). | `OPTION RESET`, `OPTION RESET cfg`, `OPTION RESET LIST`. | cfg forms PicoMite-only. | minor |
| `OPTION BAUDRATE` | `OPTION BAUDRATE nbr` 1200-1000000, permanent (p.60). | `OPTION BAUDRATE nn` "of the serial console (if it is configured)". | Range not stated on PicoMite. | minor |
| `OPTION ANGLE`, `OPTION BASE`, `OPTION BREAK`, `OPTION EXPLICIT`, `OPTION LIST`, `OPTION PIN`, `OPTION TAB`, `OPTION VCC` | p.60-65. | Identical wording. | None. | - |
| `OPTION ESCAPE` | Not present (no backslash escapes in strings). | `OPTION ESCAPE` enables `\a \b \e \f \n \r \q \t \v \\ \nnn \&hh` in string constants. | **PicoMite programs very commonly use `OPTION ESCAPE` and `"...\r\n"`.** | BREAKS |
| `OPTION DISPLAY` | Not present. | `OPTION DISPLAY lines [,chars]` (24x80 default, max 100x240). | Needed for LIST/EDIT paging on serial console. | minor (accept) |
| `OPTION CPUSPEED` | Not present (`OPTION OVERCLOCK ON/OFF`, `OPTION BASELINE ON/OFF`, p.60, p.63). | `OPTION CPUSPEED speed` (kHz). | PicoMite-only; accept as no-op. | minor |
| `OPTION FNKey` | `OPTION F11 string$` / `OPTION F12 string$` (p.62). | `OPTION FNKey string$` for F1, F5-F9. | Different key set. | minor |

### 1.8 Read-only variables and MM.INFO sub-keywords with different results

| Keyword | CMM2 (page) | PicoMite | Difference | Severity |
|---|---|---|---|---|
| `MM.DEVICE$` | "Colour Maximite 2" (p.56; other listed values are Micromite family strings). | "A string representing the device or platform that MMBasic is running on" - the actual strings are **not listed in the PicoMite manual**. | Programs branch on this string; the PicoMite value(s) must be taken from the source (e.g. "PicoMite", "PicoMiteVGA", "WebMite" - unverified here). | BREAKS (for branching programs) |
| `MM.VER` | Not present; `MM.INFO(VERSION)` "aa.bbcc" (p.58). | `MM.VER` "aa.bbcc ... version 6.03.01 will return 6.0301"; `MM.INFO(VERSION)` "(MM.VER converts to this)". | PicoMite programs test `IF MM.VER < 6.0002 THEN ERROR ...`; CMM2 must provide MM.VER and a believable value (CMM2 is 5.07.xx, lower than any PicoMite 6.x check). | BREAKS |
| `MM.CMDLINE$` | "arguments on the command line when the program was run" (p.56). | Same; "will return an empty string for programs run from the editor or using OPTION AUTORUN". | None. | - |
| `MM.ERRNO`, `MM.ERRMSG$` | p.56. | Same + `MM.ERRLINE` ("number of the program line that caused the error"). | ERRLINE PicoMite-only. | minor |
| `MM.HRES`, `MM.VRES`, `MM.I2C`, `MM.WATCHDOG`, `MM.ONEWIRE` | p.56, p.58 (MM.ONEWIRE 0/1). | Same; `MM.ONEWIRE` adds "2=Device timeout". | None. | - |
| `MM.FONTHEIGHT`, `MM.FONTWIDTH`, `MM.HPOS`, `MM.VPOS`, `MM.HEIGHT`, `MM.WIDTH`, `MM.DISPLAY`, `MM.FLAGS`, `MM.SUPPLY`, `MM.PERSISTENT`, `MM.PS2`, `MM.USB`, `MM.CODE`, `MM.ADDRESS$`, `MM.MESSAGE$`, `MM.TOPIC$`, `MM.ANSWER` | Not present (CMM2 has only the `MM.INFO(FONTHEIGHT)` etc. forms, p.33). | Present as bare read-only variables. | PicoMite-only short forms; FONTHEIGHT/FONTWIDTH/HPOS/VPOS are trivially aliasable. | BREAKS (short forms) |
| `MM.INFO$(CPUSPEED)` | "400000000 ... or 480000000" (p.56). | CPU speed string (e.g. 200000000). | Value only. | minor |
| `MM.INFO$(CURRENT)` | "name of the current program or NONE if called after a NEW" (p.56). | "NONE if called after a NEW, AUTOSAVE, XMODEM or EDIT Command". | Minor. | minor |
| `MM.INFO$(DIRECTORY)` | "current working directory. This will always end with a '/'" (p.56). | Not present (`CWD$`, `MM.INFO$(DRIVE)`). | CMM2-only. | - |
| `MM.INFO(DISK SIZE)`, `MM.INFO(FREE SPACE)` | SD card (p.56-57). | "whichever is the active drive". | Same (drive mapping applies). | - |
| `MM.INFO(EXISTS FILE fname$)` | "Returns 1 if the file filename$ exists otherwise returns 0" (p.56). | "Returns 1 if the file specified exists, returns -1 if fname$ is a directory, otherwise returns 0". | -1 for directories PicoMite-only. | minor |
| `MM.INFO(FILESIZE file$)` | "Returns -1 if the file is not found. Returns -2 if file$ is the name of a valid directory" (p.57). | "Returns the size of file$ in bytes or 0 if not found". | Not-found value -1 vs 0. | minor |
| `MM.INFO$(KEYBOARD)` | "CONNECTED" / "NOT CONNECTED" for the USB keyboard (p.57). | "On" / "Off" for the on-screen keyboard. | Same keyword, unrelated meaning. | minor |
| `MM.INFO(OPTION option)` | option = ANGLE, AUTORUN, BASE, BREAK, CONSOLE, CONSOLE PORT, DEFAULT, EXPLICIT, LEGACY, MOUSE, PROFILING, USBKEYBOARD, Y_AXIS (p.57). | option = AUTORUN, AUDIO, BASE, BREAK, CONSOLE, DEFAULT, EXPLICIT, KEYBOARD, ANGLE, HEIGHT, WIDTH, FLASH SIZE, AUTOREFRESH, SSID. | Different option vocabularies. | minor |
| `MM.INFO$(PIN pinno)` | "INVALID, RESERVED, IN USE, and UNUSED" (p.57). | "OFF, DIN, DOUT, AIN, etc". | Return vocabulary differs (status vs configuration). | BREAKS (if compared) |
| `MM.INFO$(RESET)` vs `MM.INFO(BOOT)` | `MM.INFO$(RESET)`: "Switch", "Power-On", "Software", "Watchdog" (p.57). | `MM.INFO(BOOT)`: "Restart", "S/W Watchdog", "H/W Watchdog", "Firmware update", "Power On", "Brown Out", "Reset Switch", "Unknown code &Hn"; `MM.INFO(BOOT COUNT)`. | Different keyword and strings. | minor |
| `MM.INFO$(SOUND)` | see 1.4. | see 1.4. | MODFILE vs MOD. | minor |
| `MM.INFO(FONT ADDRESS n)`, `(FONT POINTER n)`, `(FONTHEIGHT)`, `(FONTWIDTH)`, `(HPOS)`, `(VPOS)`, `(PATH)`, `(PROGRAM)`, `(UPTIME)`, `(VERSION)`, `MM.INFO$(SDCARD)`, `MM.INFO$(MODIFIED file$)`, `MM.INFO$(TRACK)`, `MM.INFO(BCOLOUR)`, `(FCOLOUR)`, `MM.INFO$(PIN...)` | p.56-58. | Same spellings (PicoMite also accepts FCOLOR/BCOLOR; `MM.INFO(TRACK)` without $). | None (except as noted). | - |
| `MM.INFO(FRAMEBUFFER)`, `(FRAMEH)`, `(FRAMEV)`, `(MAX PAGES)`, `(PAGE ADDRESS n)`, `(WRITE PAGE)`, `(MODE)`, `MM.INFO$(SEARCH PATH)` | CMM2-only page/framebuffer introspection (p.57-58). | Not present; nearest `MM.INFO(WRITEBUFF)` "address in memory of the current buffer used for drawing commands", `MM.INFO(SCREENBUFF)`. | Keep on CMM2; WRITEBUFF could alias to MM.INFO(WRITE PAGE). | minor |
| PicoMite-only MM.INFO | - | ADC, ADC DMA, BATTERY, BBC BUFFER, BIOS, BLUETOOTH device, BOOT, BOOT COUNT, CHARGING, CALLTABLE, LCDPANEL, LCD320, COMPILE, DRIVE, SYSTEM I2C, FONT, FLASH, FLASHTOP, FLASH ADDRESS n, HEAP, ID, IP ADDRESS, MAX GP, MAX VARS, MODBUFF ADDRESS, HEAPEND/STACKPEAK/CHEAPMAX/CHEAPTOTAL/TLSHEAP*, PINNO GPnn, PIO RX DMA, PIO TX DMA, PLATFORM, PSRAM SIZE, PS2, PWM COUNT, PWM DUTY C%, n%, PWM SLICE pinno, SPI SPEED, STACK, SYSTEM HEAP, SYSTICK, TILE HEIGHT, TOUCH, USB n, USB VID n, USB PID n, VARCNT, LINE, VALID CPUSPEED speed%, WRITEBUFF, SCROLL, SCREENBUFF, TCP PORT, UDP PORT, MAX CONNECTIONS, TCP REQUEST n, TCPIP STATUS, WIFI STATUS, TELNET, PERSISTENT. | Must at least not error for the commonly used ones (ID, HEAP, PLATFORM, PINNO, FONT, LINE, BOOT, FLASH, DRIVE, LCDPANEL, WRITEBUFF, PSRAM SIZE, MAX GP). | minor/BREAKS |

### 1.9 Language-level commands and functions

| Keyword | CMM2 syntax (page) | PicoMite syntax | Difference | Severity |
|---|---|---|---|---|
| `DO ... LOOP` | `DO ... LOOP`, `DO WHILE expression ... LOOP`, `DO ... LOOP UNTIL expression` (p.75); `LOOP [UNTIL expression]` (p.87). | Also `DO UNTIL expression ... LOOP` and `DO ... LOOP WHILE expression`; `LOOP [WHILE expression \| UNTIL expression]`. | `DO UNTIL` and `LOOP WHILE` are not documented on the CMM2 (verify in source; may already work). | BREAKS if unsupported |
| `SUB` / `FUNCTION` arguments | By reference when a variable is passed (p.31, p.78, p.110). | Same, plus `BYVAL` ("prefix its definition with the keyword BYVAL") and `BYREF`; whole-array return: `FUNCTION fred (arg1, arg2) AS INTEGER(6)` usable only as `b%() = fred(1, 2)`. | BYVAL/BYREF keywords and array-returning functions are PicoMite-only. | BREAKS |
| Whole-array assignment | Not present. | `b%() = a%()` (same type and total element count; "a 2 x 3 array can be copied into a six element array"). | PicoMite-only (`LET` entry). | BREAKS |
| `DIM ... LENGTH` | "This keyword can also be used with non array string variables but it will not save any memory unless the length is less than 16" (p.75). | "For the RP2040, if length is greater than 9 it will be ignored ... For the RP2350, if the length is greater than 15 it will be ignored". | Memory behaviour only. | minor |
| `DIM a(0)` single-element arrays | Not documented. | "A dimension can hold a single element. With the default OPTION BASE 0 this is written as DIM a(0)". | Verify CMM2 accepts `DIM a(0)`. | minor |
| `TYPE` / `END TYPE` / `STRUCT` / `STRUCT()` / `DIM x AS typename` | Not present. | `TYPE struct-name ... END TYPE`, `DIM alarm AS StructAlarm`, member access `alarm.time`, `STRUCT`, `STRUCT( arg, ...)` (MMBasic_Structures_Manual.pdf). | PicoMite-only language feature; note `.` is legal in CMM2 variable names (p.25) so `alarm.time` would silently become a plain variable. | BREAKS |
| `REDIM` | Not present. | `REDIM [PRESERVE] array1(dimensions) [, array2(dimensions]...`. | PicoMite-only. | BREAKS |
| `INC` | `INC var [,increment]` (p.82). | Same. | None. | - |
| `END` | `END` (p.76). | `END [noend]` / `END cmd$`; hook subroutines `MM.STARTUP`, `MM.PROMPT`, `MM.END`. | `END noend`/`END cmd$` and the three hook subs are PicoMite-only (CMM2 manual never mentions MM.STARTUP). | BREAKS (END cmd$), minor (hooks silently ignored) |
| `ON ERROR` | `ABORT \| IGNORE \| SKIP [nn] \| CLEAR` (p.94). | Adds `ON ERROR RESTART` ("execute a Hardware Reset"). | RESTART PicoMite-only (alias to CPU RESTART). | minor |
| `READ` / `RESTORE` | `READ variable[, variable]...`, `RESTORE [line]` (p.103). | Adds `READ SAVE` / `READ RESTORE`; DATA in the library; RESTORE inside library code. | READ SAVE/RESTORE PicoMite-only. | BREAKS if used |
| `MID$()=` | `MID$( str$, start [, num]) = str2$` "the replacement of characters never goes beyond the original length of 'str$'" (p.91). | `MID$( str$, start, num) = str2$` "'num' can be set be 0 i.e. insert the new string at the position specified". | Insert mode (num = 0) PicoMite-only; CMM2 never lengthens the string. | minor |
| `INSTR()` / `LINSTR()` | `INSTR( [start-position,] string-searched$, string-pattern$ )` (p.121); `LINSTR(array%(), search$ [,start])` (p.122). | `INSTR( [start-position,] string-searched$, string-pattern$ [,size] )` and `LINSTR(array%(), search$ [,start] [,size])` - "If the optional parameter "size" is specified the "string-pattern" is treated as a regular expression" (Appendix E). | Regex support PicoMite-only. | BREAKS if used |
| `STRING$()` | `STRING$( nbr, ascii )` "decimal number in the range of 32 to 126" (p.129). | "integer or float number in the range of 0 to 255". | Range check differs. | minor |
| `FIELD$()` | `FIELD$( string1, nbr, string2 [, string3] )` (p.119). | Same, "a space character cannot be used as a delimiter". | Minor. | minor |
| `FORMAT$()` | `FORMAT$( nbr [, fmt$] )` (p.119). | Same; precision "defaults to 4 digits". | Minor. | minor |
| `TRIM$()`, `BIT()`/`BIT()=`, `BYTE()`/`BYTE()=`, `FLAG()`/`FLAG()=`/`FLAGS=`, `LMID()=`, `CHOICE()`, `BOUND()`, `CALL`/`CALL()`, `BIN2STR$`/`STR2BIN`, `BASE$`, `EVAL`, `EXECUTE`, `JSON$`, `DATETIME$`, `DAY$`, `EPOCH`, `LONGSTRING ...`, `LGETBYTE`, `LGETSTR$`, `LLEN`, `LCOMPARE`, `SORT`, `MATH ...`, `MEMORY ...`, `POKE`/`PEEK` | All present on CMM2 except TRIM$, BIT, BYTE, FLAG/FLAGS, LMID (p.115-130). | TRIM$(source$ [,mask$] [,where/where$]), BIT(var%, bitno), BYTE(var$, byteno), FLAG(n%), FLAGS=value%, LMID(array%(),start [,num])=string$, LINPUT(array%(),fnbr,nbr), LONGSTRING AES128/BASE64, MATH additions, MEMORY PRINT/INPUT/PACK/UNPACK, PEEK(CFUNADDR), PEEK(BP/SP/WP), POKE DISPLAY. | PicoMite-only items need adding; `PEEK(VARTBL,...)` / `POKE VARTBL` "removed in V7.00.00" on PicoMite but exist on CMM2 (p.100, p.125). | BREAKS (TRIM$, BIT, BYTE, FLAG, LMID, LINPUT are commonly used) |
| `MATH` command | SET, SCALE, ADD, INTERPOLATE, SLICE, INSERT, M_INVERSE, M_PRINT, M_TRANSPOSE, M_MULT, V_PRINT, V_NORMALISE, V_MULT, V_CROSS, Q_INVERT, Q_VECTOR, Q_CREATE, Q_EULER, Q_MULT, Q_ROTATE, FFT, FFT INVERSE, FFT MAGNITUDE, FFT PHASE, SENSORFUSION (p.87-90). | Same set (SET/ADD/SLICE/INSERT "See ARRAY ..."), plus `MATH RANDOMIZE [n]`, `MATH WINDOW in(), minout, maxout, out() [,minin!, maxin!]`, `MATH CLAMP in(), lo, hi, out()`, `MATH POWER inarray(), power, outarray()`, `MATH SHIFT inarray%(), nbr, outarray%() [,U]`, `MATH V_PRINT array() [,hex]`, `MATH V_ROTATE x, y, a, xin(), yin(), xout(), yout()`, `MATH C_ADD/C_SUB/C_MUL/C_DIV/C_AND/C_OR/C_XOR array1(), array2(), array3()`, `MATH SINC ...`, `MATH PID INIT channel, pid_params!(), callback` / `START` / `STOP`, `MATH AES128 ...`; `MATH SENSORFUSION type ax, ay, az, gx, gy, gz, [mx, my, mz, ] pitch, roll, yaw [,p1] [,p2]` (magnetometer optional); `MATH Q_CREATE` "Theta is specified in radians" (CMM2: "respects the setting of OPTION ANGLE", p.89). | Additions PicoMite-only; SENSORFUSION optional magnetometer; Q_CREATE angle unit wording. | BREAKS if used |
| `MATH()` function | ATAN3, COSH, LOG10, SINH, TANH, CHI, CHI_p, CORREL, MAX, MEAN, MEDIAN, MIN, SD, SUM, MAGNITUDE, DOTPRODUCT, M_DETERMINANT (p.123). | Same plus `MATH(CRCn data [,length] [,polynome] [,startmask] [,endmask] [,reverseIn] [,reverseOut])`, `MATH(RAND)`, `MATH(CROSSING array() [,level] [,direction] [,confirm])`, `MATH(MAX a() [,index%])`, `MATH(MIN a(), [index%])`, complex `MATH(C_CPLX ...)` family, `MATH(PID channel, setpoint!, measurement)`, `MATH(BASE64 ENCODE/DECODE in$/in(), out$/out())`. | Additions PicoMite-only. | BREAKS if used |
| `MEMORY SET/COPY` | `MEMORY SET INTEGER` "INTEGER = Four bytes per memory address. FLOAT = Four bytes per memory address"; "COPY INTEGER and FLOAT will copy four bytes per operation" (p.90-91). | "FLOAT = Eight bytes per memory address"; "COPY INTEGER and FLOAT will copy eight bytes per operation". | Documented element size differs (CMM2 text may be a documentation error - verify in source). | minor |
| `ARRAY ADD/INSERT/SET/SLICE` | Not present (same operations exist as `MATH ADD/INSERT/SET/SLICE`, p.87-88, numeric only). | `ARRAY ADD in(), value ,out()`, `ARRAY INSERT targetarray(), [d1] [,d2] [,d3] [,d4] [,d5] , sourcearray()`, `ARRAY SET value, array()` (same argument order as `MATH SET nbr, array()`), `ARRAY SLICE sourcearray(), [d1] ... destinationarray()`; all work on string arrays too. | ARRAY keyword and string-array support PicoMite-only. | BREAKS if used |
| `COPY` | `COPY fname1$ TO fname2$` (p.71). | `COPY fname1$ TO fname2$`, `COPY fname$ TO dirname$` (wildcards), `COPY A2A/A2B/B2A/B2B fname1$, fname2$`. | Wildcard and drive forms PicoMite-only. | minor |
| `KILL` | `KILL file$` (p.83). | `KILL file$ [,all]` with wildcards. | Wildcards PicoMite-only. | minor |
| `DATE$ =` | `DATE$ = "DD-MM-YY"` / `DATE$ = "DD/MM/YY"` (p.74). | Also `DATE$ ="YYYY-MM-DD"` / `DATE$="YYYY/MM/DD"`. | Year-first forms PicoMite-only. | minor |
| `DATE$`, `TIME$`, `TIMER`, `TIME$ =`, `TIMER =` | "DD-MM-YYYY", "HH:MM:SS" (+ .mmm with OPTION MILLISECONDS), TIMER in ms "fractional floating point number with a resolution of 1µs" (p.117, p.130, p.111). | Same formats; TIMER "64 bit number". | None. | - |
| `TIME$ = ±sec` | "Adds or subtracts 'sec' seconds from the current time" (p.111). | Not documented. | CMM2-only. | - |
| `CSUB` | `CSUB name [type [, type] ...]` ... `END CSUB` - ARM Cortex-M7 code, "CSUBs must call routinechecks() every millisecond" (p.72-73). | Same syntax; code is RP2040 (Cortex-M0+) / RP2350 (Cortex-M33) binary; "Up to ten arguments ... on the RP2040 and up to sixteen on the RP2350". | **Binary blobs are not portable**; a PicoMite program embedding a CSUB cannot run on the CMM2 regardless of syntax compatibility. | BREAKS |
| `RANDOMIZE` | "the RANDOMIZE command is no longer needed and is not supported" (p.127). | `RANDOMIZE nbr` (RP2040; "does nothing on the RP2350"). | Must be accepted as a no-op. | BREAKS (syntax error today) |
| `RND()` | Hardware true random (p.127). | Random (RP2350) / pseudo-random (RP2040). | None in practice. | - |
| `PRINT` | "A semicolon (;) at the end of the expression list will suppress the automatic output of a carriage return" (p.101). | "A semicolon (;) or a comma (,) at the end of the expression list will suppress". | Trailing comma behaviour. | minor |
| `LIST` | `LIST [file$]`, `LIST ALL [file$]`, `LIST FILES`, `LIST COMMANDS`, `LIST FUNCTIONS`, `LIST PAGES`, `LIST PROFILE [CSV]` (p.84). | `LIST [fname$]`, `LIST ALL [fname$]`, `LIST PINS`, `LIST SYSTEM I2C`, `LIST COMMANDS`, `LIST FUNCTIONS`, `LIST VARIABLES [s%()]`. | PINS / SYSTEM I2C / VARIABLES PicoMite-only; FILES/PAGES/PROFILE CMM2-only. | minor |
| `HELP` | `HELP [text]` built-in command syntax (p.80). | `HELP searchtext` reads "help.txt" on the A: drive. | Data source differs. | minor |
| `CONTINUE` | `CONTINUE`, `CONTINUE DO`, `CONTINUE FOR`; also exits `MMDEBUG BREAK` (p.70-71). | `CONTINUE`, `CONTINUE DO`, `CONTINUE FOR`. | None. | - |
| `MMDEBUG` / `#MMDEBUG` | `#MMDEBUG ON\|OFF`, `MMDEBUG` (= PRINT), `MMDEBUG BREAK` (p.66, p.91). | Not present. | CMM2-only. | - |
| `INTERRUPT` | Not present. | `INTERRUPT [myint]` software interrupt (also from a CSUB). | PicoMite-only. | BREAKS if used |
| `SYNC` | Not present. | `SYNC time% [,period]` / `SYNC`. | PicoMite-only precise loop timer. | BREAKS if used |
| `ONESHOT` | Not present. | `ONESHOT triggerPin, sense, outputPin, prePulseDelay, pulseWidth [, quiescentPeriod]`, `ONESHOT R ...`, `ONESHOT DISABLE`. | PicoMite-only. | BREAKS if used |
| `FLUSH` | Not present. | `FLUSH [#]fnbr`. | PicoMite-only; trivial no-op/alias on CMM2. | BREAKS (syntax error today) |
| `CAT` | `CAT S$, N$` (p.70). | `CAT S$, N$`. | None. | - |
| `VAR SAVE/RESTORE/CLEAR` | 4 KB battery-backed RAM (p.113). | 16 KB flash ("should not be used for high-speed saves"). | Capacity only. | minor |
| `UPDATE FIRMWARE` | p.113. | Same. | None. | - |

---

## 2. PICOMITE-ONLY features (to be added for compatibility)

"stub/error" marks features that cannot make sense on the CMM2 hardware; everything else is a
candidate for real implementation or a semantic mapping. PicoMite syntax quoted from the V7.00.00
listings.

### 2.1 Language / interpreter
- `OPTION ESCAPE` - backslash escapes in string constants (`\n \r \t \q \e \\ \nnn \&hh`). Implementable.
- `/*` ... `*/` multi-line comments (CMM2 has `#COMMENT START/END`). Implementable in the pre-processor.
- `TYPE ... END TYPE`, `DIM x AS typename`, `x.member`, `STRUCT`, `STRUCT()` (structures manual). Large.
- `BYVAL` / `BYREF` argument prefixes in `SUB`/`FUNCTION`.
- `FUNCTION name(...) AS INTEGER(n)` / `AS FLOAT(d1, d2)` array-returning functions, whole-array assignment `b%() = a%()`.
- `REDIM [PRESERVE] array(dims) [, ...]`.
- `DO UNTIL expr ... LOOP`, `LOOP WHILE expr` (verify on CMM2).
- `END [noend]`, `END cmd$`, hook subroutines `MM.STARTUP`, `MM.PROMPT`, `MM.END`, `MM.ANSWER` / calculator prompt (prompt-level; MM.END is the only one a program would notice).
- `ON ERROR RESTART`.
- `READ SAVE` / `READ RESTORE`; DATA continues into the library.
- `CHAIN fname$ [cmdline$]` ("run another program with the variable space preserved"), `LOAD file$ [,C] [,R]`, `SAVE file$`, `SAVE CONTEXT [CLEAR]`, `LOAD CONTEXT [KEEP]`, `NEW` clearing program memory, `EDIT FILE fname$`.
- `LIBRARY SAVE`, `LIBRARY DELETE`, `LIBRARY LIST [ALL]`, `LIBRARY DISK SAVE fname$`, `LIBRARY DISK LOAD fname$`, `LIBRARY LOAD fname$ [, fname$ ...] [, O] [, RAM]` (library initialisation code runs before the program; labels/DATA shared) - CMM2 says "The library and the LIBRARY commands are not implemented" (p.53); `LIBRARY LOAD` is the form a distributed program uses.
- `FLASH SAVE n / LOAD n / RUN n / CHAIN n / LIST [n [,ALL]] / ERASE n|ALL / OVERWRITE n / DISK LOAD n, fname$ [,O] / LOAD IMAGE n, filename$ [,O] / MODBUFF LOAD fname$`, `FLASH UF2 fname$` (stub/error), `RAM ...` (PSRAM slots, stub/error unless mapped to files), `MM.INFO(FLASH)`, `MM.INFO(FLASHTOP)`, `MM.INFO(FLASH ADDRESS n)`.
- `CMM2 LOAD` / `CMM2 RUN` - PicoMite's own CMM2-style loader; on the CMM2 these are simply `RUN`.
- `INTERRUPT [myint]` software interrupt; `SYNC time% [,period]` / `SYNC`; `ONESHOT ...`.
- `CPU SLEEP n` (map to PAUSE), `CPU SPEED n` (stub/no-op).
- `RANDOMIZE nbr` (no-op), `MATH RANDOMIZE [n]`, `MATH(RAND)`.
- `TRIM$(source$ [,mask$] [,where/where$])`, `BIT(var%, bitno)` / `BIT(var%, bitno) = value`, `BYTE(var$, byteno)` / `BYTE(var$, byteno)=value`, `FLAG(n%)` / `FLAG(n%)=value` / `FLAGS=value%` / `MM.FLAGS`, `LMID(array%(),start [,num])=string$`, `LINPUT(array%(),fnbr,nbr)`, `INSTR(... [,size])` / `LINSTR(... [,size])` regex (Appendix E), `MID$(str$, start, 0) = str2$` insert mode.
- `ARRAY ADD / INSERT / SET / SLICE` (string-capable versions of MATH ADD/INSERT/SET/SLICE).
- `MATH WINDOW`, `MATH CLAMP`, `MATH POWER`, `MATH SHIFT`, `MATH V_ROTATE`, `MATH C_ADD..C_XOR` (cell-wise), `MATH SINC`, `MATH PID INIT/START/STOP` + `MATH(PID ...)`, `MATH AES128 ...`, `MATH(CRCn ...)`, `MATH(CROSSING ...)`, `MATH(MAX a() [,index%])` / `MATH(MIN a(), [index%])`, complex-number `MATH(C_* ...)` family, `MATH(BASE64 ENCODE/DECODE ...)`, `MATH V_PRINT array() [,hex]`, optional magnetometer in `MATH SENSORFUSION`.
- `LONGSTRING AES128 ENCRYPT/DECRYPT ...`, `LONGSTRING BASE64 ENCODE/DECODE in%(), out%()`, `LONGSTRING PRINT [#n,] src%() [;]` (trailing `;`).
- `MEMORY PRINT [#]fnbr , nbr, address%/array()`, `MEMORY INPUT ...`, `MEMORY PACK ...`, `MEMORY UNPACK ...`, `MEMORY SHARE` (separate manual; stub/error - Pico-to-Pico link).
- `PEEK(CFUNADDR cfun)`, `PEEK(BP n%)`, `PEEK(SP n%)`, `PEEK(WP n%)`, `POKE DISPLAY command [,data...]` (stub/error), `POKE DISPLAY HRES n` / `VRES n`.
- `SAVE PERSISTENT n%` / `MM.PERSISTENT` (could use battery RAM), `MM.ERRLINE`, `MM.VER`, `MM.INFO$(LINE)`, `MM.INFO(ID)`, `MM.INFO(HEAP)`, `MM.INFO(VARCNT)`, `MM.INFO(MAX VARS)`, `MM.INFO$(PLATFORM)` / `OPTION PLATFORM name$`, `MM.INFO(BOOT)` / `MM.INFO(BOOT COUNT)`, `MM.INFO$(CALLTABLE)`, `MM.INFO(STACK)`, `MM.INFO(SYSTICK)`, `MM.INFO(SYSTEM HEAP)`, `MM.INFO$(COMPILE)` (stub), `MM.INFO(VALID CPUSPEED ...)` (stub).
- `OPTION DISPLAY lines [,chars]`, `OPTION CASE LOWER|UPPER|TITLE`, `OPTION CONTINUATION LINES ON/OFF`, `OPTION LOCAL VARIABLES n`, `OPTION LOGGING ON|OFF`, `OPTION NOCHECK ON|OFF`, `OPTION SYMBOLS OFF`, `OPTION COMPILE ON|SHADOW|OFF` (stub), `OPTION DISK SAVE/LOAD fname$`, `OPTION FNKey string$`, `OPTION DEFAULT COLOURS fg [,bg]`, `OPTION HEARTBEAT ON/OFF [pin]` (stub), `OPTION PICO ON/OFF` (stub), `OPTION PICO PLUS 2` (stub), `OPTION POWER PFM|PWM` (stub), `OPTION PSRAM PIN n` (stub), `OPTION CPUSPEED speed` (stub), `OPTION CDC POLL RATE n` (stub), `OPTION RESET cfg` / `OPTION RESET LIST` / `CONFIGURE cfg` / `CONFIGURE LIST` (stub).
- `LIST PINS`, `LIST SYSTEM I2C`, `LIST VARIABLES [s%()]`.
- `XMODEM CRUNCH`, `YMODEM ...`, `ZMODEM SEND/RECEIVE [file$] / RECEIVE "*" / CRUNCH` (console transfer; ZMODEM worth having).
- `AUTOSAVE CRUNCH/APPEND/N`, `*file` argument-passing forms, `A:`/`B:`/`C:` prompt shortcuts, `DRIVE drive$`, `DRIVE "A:/FORMAT"` (stub/error), `FM` file manager (CMM2 has `FILES`), `HELP searchtext` from help.txt.
- `OPTION PROFILING ON, SAMPLE [, entries]` (stub to existing profiling).

### 2.2 Graphics
- `MODE 1..5` numbering, `OPTION RESOLUTION nn [,cpuspeedinKhz]`, `RESOLUTION nn [,cpuspeedinKHz]` - need the mode-mapping table (section 4).
- Named buffers N/F/L/2/T: `FRAMEBUFFER CREATE [2]`, `FRAMEBUFFER LAYER [colour]`, `FRAMEBUFFER LAYER TOP [colour]`, `FRAMEBUFFER WRITE where`, `FRAMEBUFFER CLOSE [which]`, `FRAMEBUFFER COPY from, to [,b]`, `FRAMEBUFFER WAIT`, `FRAMEBUFFER MERGE [colour] [,b]` (and the LCD forms with mode/updaterate), `FRAMEBUFFER SYNC`; automatic layer compositing with a transparent colour index.
- 16-colour index arguments and the named palette (Black, Blue, Myrtle, Cobalt, Midgreen, Cerulean, Green, Cyan, Red, Magenta, Rust, Fuchsia, Brown, Lilac, Yellow, White) + `COLOUR MAP inarray%(), outarray%() [,colourmap%()]`.
- `TILE x, y [,fg] [,bg] [,w] [,h]`, `TILE HEIGHT n`, `MM.INFO(TILE HEIGHT)` (MODE 1 text-cell colouring).
- `BLIT LOAD[BMP] [#]b, fname$ [,x] [,y] [,w] [,h]`, `BLIT MERGE colour, x, y, w, h`, `BLIT FRAMEBUFFER from, to, xin, yin, xout, yout, width, height [,colour]`, `BLIT MEMORY address, x, y [,col]`, `BLIT MEMORY332 address, x, y [,col]`, `BLIT COMPRESSED address%, x, y [,col]`, `BLIT FLASH from, to, ...`, `BLIT RESIZE src, dst, sx, sy, sw, sh, dx, dy, dw, dh [,transparent]`; `BLIT WRITE` default mode 0.
- `SPRITE STINTERRUPT` (static-object collision) and any other SPRITE sub-commands in SPRITE_User_Manual.pdf not present on the CMM2 (unverifiable here).
- `LOAD IMAGE fname$ [,x] [,y] [,mode] [,ximage] [,yimage]` (keyword + dither modes + offsets), `LOAD JPG ... [,mode] [,ximage] [,yimage] [,scale]`, `LOAD PNG ... [,transparent] [,alphacut] [,ximage] [,yimage]`, `SAVE COMPRESSED IMAGE`.
- Built-in font 8 (4x6) and font 3 at 16x16 on VGA builds.
- `FILL x, y, fillcolour [,bordercolour]` (CMM2 `PIXEL FILL`), `LINE` negative width, `LINE AA`, `LINE GRAPH x(), y(), colour`, `LINE PLOT ydata() [...]`, `BEZIER x%(),y%() [,n] [,colour]`, `TRIANGLE SAVE/RESTORE [#]n`, `MANDELBROT [DRAW|PAN|ZOOM|CENTRE|RESET]`.
- `TURTLE` extensions (abbreviations, SET X/Y/XY/HEADING, HOME, PEN WIDTH, ARC/ARCLEFT/ARCRIGHT, BEZIER, CIRCLE radius, FCIRCLE, FRECTANGLE, ARECTANGLE, WEDGE, FILL PATTERN 0-31, NO FILL, SHOW/HIDE TURTLE, CURSOR SIZE/COLOUR, STAMP, PUSH, POP, `DOT size`, `RESET [show]`).
- `MAP GREYSCALE`, `MAP()` returning the CLUT entry.
- `TILEMAP` / `TILEMAP()`, `RAY` / `RAY()`, `FRAME` / `FRAME()` (separate manuals; large).
- `GUI CLICK [x, y] / DOWN / UP / PIN pin [,INV] / PIN OFF`, `GUI INTERRUPT TouchDown [, TouchUp]`, `OPTION GUI CONTROLS n` (CMM2 `OPTION MAXCTRLS n`), `CLICK(REF)`, `CLICK(LASTREF)`, `CLICK(LASTX)`, `CLICK(LASTY)`; `TOUCH(...)` family (alias X/Y/DOWN/UP to CLICK; gestures/multi-touch stub); `GUI CALIBRATE`, `GUI TEST LCDPANEL`, `GUI TEST TOUCH`, `GUI RESET LCDPANEL` (stub/error).
- `MM.INFO(WRITEBUFF)`, `MM.INFO(SCREENBUFF)`, `MM.INFO(SCROLL)`, `MM.INFO$(LCDPANEL)`, `MM.INFO(LCD320)`, `MM.DISPLAY`, `MM.FONTHEIGHT`, `MM.FONTWIDTH`, `MM.HPOS`, `MM.VPOS`, `MM.HEIGHT`, `MM.WIDTH`, `MM.INFO(FONT)`.
- LCD panel ecosystem (stub/error on CMM2): `OPTION LCDPANEL ...` (all controllers, VIRTUAL_C/VIRTUAL_M, USER, CONSOLE/NOCONSOLE, DISABLE), `OPTION LCD SPI`, `OPTION LCD320`, `OPTION BACKLIGHT KBD|LCD n`, `BACKLIGHT n [,DEFAULT|FreqInHz]`, `REFRESH`, `OPTION AUTOREFRESH`, `MODE 400|800`, `OPTION TOUCH ...`, `OPTION SYSTEM SPI/I2C`, `OPTION SDCARD ...`, `OPTION VGA PINS`, `OPTION HDMI PINS`.
- `CAMERA OPEN/CAPTURE/CAPTURE JPEG/CLOSE/CHANGE/TEST/REGISTER` (OV7670/OV2640/OV5640) - stub/error.
- `DRAW3D` / `DRAW3D()` exist on both (3D manuals) - compare separately.

### 2.3 Sound
- `OPTION AUDIO PWM/SPI/VS1053/I2S/BLUETOOTH/DISABLE` (accept as no-op; the CMM2 DAC is always present).
- `PLAY ARRAY l%(), r%(), freq [,start] [,end] [,terminationinterrupt]` (CMM2 has `DAC START` with 12-bit unpacked arrays - mapping possible).
- `PLAY SAMPLE ...` / `PLAY RELEASE` (ADSR looped waveform), `PLAY LOAD SOUND array%()` and `PLAY SOUND` type `U`, `OPTION FAST AUDIO ON|OFF`.
- `PLAY BBC SOUND channel, amplitude, pitch, duration`, `PLAY BBC ENVELOPE n, T, PI1, PI2, PI3, PN1, PN2, PN3, AA, AD, AS, AR, ALA, ALD`, `MM.INFO(BBC BUFFER)`.
- `PLAY MODFILE file$ [,interrupt]` interrupt semantics (one pass then stop), `OPTION MODBUFF ENABLE/DISABLE [sizeinK]` (no-op), `FLASH MODBUFF LOAD fname$`.
- VS1053-only: `PLAY MIDIFILE`, `PLAY MIDI`, `PLAY MIDI CMD`, `PLAY MIDI TEST`, `PLAY NOTE ON/OFF`, `PLAY HALT`, `PLAY CONTINUE track$`, `PLAY STREAM buffer%(), readpointer%, writepointer%` - stub/error (HALT/CONTINUE could be emulated for MP3).
- `MM.INFO$(SOUND)` vocabulary ("MOD", "BBC", "ARRAY", "SAMPLE", "MIDI", "STREAM", "PAUSED xxx").
- Bluetooth audio (`BLUETOOTH SCAN/CONNECT/DISCONNECT/STATUS/FORGET`, `MM.INFO$(BLUETOOTH ...)`) - stub/error.

### 2.4 I/O
- GPnn pin names (including string forms "GPnn"), `MM.INFO(PINNO GPnn)`, `MM.INFO(MAX GP)` - mapping table.
- `SETPIN rx, tx, COM1|COM2`, `SETPIN rx, tx, clk, SPI|SPI2`, `SETPIN sda, scl, I2C|I2C2`, `SETPIN pin, PWM[nx]`, `SETPIN pin, IR`, `SETPIN pin, PIOn` (stub), `SETPIN GP1, FFIN [,gate]` (map to CMM2 FAST COUNT pin 18), `SETPIN GP25, DOUT|HEARTBEAT` (stub), `SETPIN pin, ARAW`, `SETPIN pin, CIN, edge-option`, `OPTION COUNT pin1..pin4` (map to CMM2 COUNT pins 7/13/15/24).
- `PWM channel, frequency, [dutyA] [,dutyB] [,phase] [,defer]`, `PWM SYNC ...`, `PWM channel, OFF`; `SERVO channel [positionA] [,positionB]` (0-100 position scale); `MM.INFO(PWM COUNT)`, `MM.INFO(PWM DUTY C%, n%)`, `MM.INFO(PWM SLICE pinno)`.
- `ADC OPEN freq, n_channels [,interrupt]`, `ADC START ... [,Cmin] [,Cmax]`, `ADC RUN array1%(),array2%()`, `MM.INFO(ADC)`, `MM.INFO(ADC DMA)`.
- `I2C CHECK addr`, `I2C SLAVE OPEN/WRITE/READ/CLOSE` (I2C2 too) - CMM2 is master-only ("slave mode is not available", p.135): stub/error unless the STM32 I2C slave is implemented.
- `OPTION SYSTEM I2C sdapin, sclpin [,SLOW/FAST]` (map to the fixed CMM2 I2C channel; needed by WII, RTC, I2CLCD, LCD, CAMERA), `OPTION SYSTEM SPI` (no-op), `OPTION SDCARD` (no-op), `OPTION RTC AUTO ENABLE|DISABLE` (map to OPTION DS3231), `RTC GETTIME`, `RTC SETTIME year, month, day, hour, minute, second`, `RTC SETREG reg, value`, `RTC GETREG reg, var` - CMM2 says "Real Time Clocks (RTC) ... are not supported" (p.53) but G2 has OPTION DS3231; implementable in firmware I2C.
- `KEYPAD var, int, r1, r2, r3, r4, c1, c2, c3 [, c4]` / `KEYPAD CLOSE` / `KEYPAD keymap!(), var!, int, startcolpin, nocols, startrowpin, norows` - implementable with GPIO.
- `LCD INIT d4, d5, d6, d7, rs, en`, `LCD line, pos, text$`, `LCD CLEAR`, `LCD CLOSE`, `LCD CMD/DATA ...`; `I2CLCD INIT/CLOSE/CLEAR/BACKLIGHT/CURSOR/CREATECHAR/CMD/DATA/line,pos,string$` - implementable.
- `WS2812 type, pin, nbr, value%[()]` (type W), `BITSTREAM pin1, count1, array1() [, mode1] [,pin2, ...] [,logic]` (dual-pin form), `DEVICE SERIALTX pinno, baudrate, ostring$`, `DEVICE SERIALRX pinno, baudrate, istring$, timeout_in_ms, status% [,nbr] [,terminators$]`, `DEVICE xxx` aliases (BITSTREAM, CAMERA, GAMEPAD, HUMID, KEYPAD, MOUSE, LCD, WII, WS2812).
- `WATCHDOG HW timeout|OFF`, `SETTICK PAUSE|RESUME, target [, nbr]`, `TEMPR(pin [,timeout])`, `TEMPR START pin [, precision] [,timeout]`, `PIN(BOOTSEL)` (stub), `PIN(TEMP)` unquoted.
- `MOUSE OPEN channel, CLKpin, DATApin` / `MOUSE CLOSE channel` / `MOUSE INTERRUPT ENABLE channel, int` / `MOUSE INTERRUPT DISABLE channel` / `MOUSE SET channel, x, y [, wheel]`; `DEVICE(MOUSE channel, funct)`; `OPTION MOUSE CLKpin, DATApin|DISABLE|SENSITIVITY f!`.
- `WII [CLASSIC] OPEN [,interrupt]` / `CLOSE`, `WII NUNCHUCK OPEN [,interrupt]` / `CLOSE`, `DEVICE(WII [CLASSIC] funct)`, `DEVICE(NUNCHUCK funct)`.
- `GAMEPAD ...`, `DEVICE(GAMEPAD channel, funct)`, `MM.INFO(USB n)`, `MM.INFO(USB VID n)`, `MM.INFO(USB PID n)`, `MM.USB` - stub/error (no USB host) or map onto Wii Classic.
- `KEYBOARD ON|OFF`, `OPTION SCREEN KEYBOARD nn [, M]`, `OPTION KEYBOARD nn [...]`, `OPTION KEYBOARD DISABLE|I2C|PINS`, `OPTION PS2 PINS`, `OPTION LOCAL KEYBOARD`, `ON PS2 target`, `MM.PS2`, `MM.INFO(PS2)` - layout form maps to OPTION USBKEYBOARD; rest stub.
- `PIO ...` commands, `PIO(...)` functions, PIO assembler mnemonics (JMP, WAIT, IN, OUT, PUSH, PULL, MOV, IRQ, SET as commands), `PIO DMA`, `PIO INTERRUPT`, `SETPIN pin, PIOn`, `MM.INFO(PIO RX|TX DMA)` - stub/error (no PIO hardware).
- `STEPPER`, `TMC22xx ...`, `MM.CODE` - stub/error (or GPIO implementation).
- `ASTRO`, `STAR`, `LOCATION`, `SLEW`, `OPTION GPS` - pure computation; could be ported, otherwise stub.
- `OPTION SERIAL CONSOLE uartapin, uartbpin [,B]|DISABLE`, `OPTION BAUDRATE` semantics, COM3-COM6 (USB CDC host) and COM7 (Bluetooth) - stub/error; CMM2 COM3 is the console UART.
- `MM.SUPPLY`, `MM.INFO(BATTERY)`, `MM.INFO(CHARGING)`, `MM.INFO(BIOS)` (PicoCalc) - stub.

### 2.5 Files
- Drive model A:/B:/C:, `DRIVE`, drive-qualified paths, `COPY A2A/A2B/B2A/B2B`, wildcard `COPY fname$ TO dirname$`, `KILL file$ [,all]` wildcards, `FLUSH [#]fnbr`, `FILES [fspec$] [,sort]` as a listing command, `MM.INFO$(DRIVE)`, `MM.INFO(drive)`, `MM.INFO(EXISTS FILE)` = -1 for directories, `MM.INFO(FILESIZE)` = 0 for not found, `OPEN ... FOR RANDOM` creating the file if absent, `LINPUT()`, `MEMORY PRINT/INPUT`.
- `FLASH DISK LOAD`, `FLASH LOAD IMAGE`, `LIBRARY DISK SAVE/LOAD`, `OPTION DISK SAVE/LOAD`.
- `FLASH UF2 fname$`, USB flash drive C: - stub/error.

### 2.6 Networking (all stub/error on CMM2 - no WiFi/Bluetooth hardware)
- `WEB CONNECT`, `WEB NTP`, `WEB OPEN TCP|TLS CLIENT|STREAM`, `WEB TCP CLIENT READ|REQUEST|STREAM|WRITE`, `WEB CLOSE TCP CLIENT`, `WEB TCP INTERRUPT`, `WEB TCP READ|SEND|CLOSE`, `WEB TRANSMIT CODE|FILE|PAGE`, `WEB UDP INTERRUPT|SEND`, `WEB MQTT CONNECT|PUBLISH|SUBSCRIBE|UNSUBSCRIBE|CLOSE`, `WEB TLS CA|NOVERIFY`, `WEB PING`, `WEB SCAN`.
- `OPTION WIFI`, `OPTION TCP SERVER PORT`, `OPTION UDP SERVER PORT`, `OPTION TELNET CONSOLE`, `OPTION TFTP`, `OPTION WEB MESSAGES`, `OPTION CYW43 PINS`, `OPTION RADIO`.
- `MM.ADDRESS$`, `MM.MESSAGE$`, `MM.TOPIC$`, `MM.INFO$(IP ADDRESS)`, `MM.INFO(TCP PORT|UDP PORT|MAX CONNECTIONS|TCP REQUEST n|TCPIP STATUS|WIFI STATUS|TELNET)`.
- `JSON$(array%(), string$)` is marked "WIFI VERSIONS ONLY" on PicoMite but exists on the CMM2 (p.121) - no action.
- `BLUETOOTH SCAN|CONNECT|DISCONNECT|STATUS|FORGET`, `OPTION AUDIO BLUETOOTH`, COM7.

### 2.7 Misc
- `OPTION PLATFORM name$` / `MM.INFO$(PLATFORM)` (cheap and useful for compatibility detection).
- `MM.INFO(ID)` (unique chip ID - STM32 has one), `MM.INFO(BOOT)`.
- `UPDATE FIRMWARE` exists on both.
- `HELP` from help.txt, `FM`, calculator prompt, `OPTION CASE`, editor F12 beautify, `OPTION CONTINUATION LINES` - console conveniences.

---

## 3. CMM2-ONLY features that must be preserved

Graphics and video
- `MODE r, bits [, bg [, int]]` with the 17 resolutions, 8/12/16/32-bit depths, 12-bit alpha layering, frame-blanking interrupt (p.91-93); `OPTION DEFAULT MODE n` (p.61).
- Video pages: `PAGE WRITE n`, `PAGE COPY n TO m [,when] [,t]`, `PAGE DISPLAY n [,page]` / `PAGE DISPLAY -1`, `PAGE RESIZE pageno, w, h`, `PAGE SCROLL pageno, x, y [,fillcolour]`, `PAGE STITCH ...`, `PAGE AND_PIXELS/OR_PIXELS/XOR_PIXELS ...` (p.96-97); `LIST PAGES` (p.84); `MM.INFO(MAX PAGES)`, `MM.INFO(PAGE ADDRESS n)`, `MM.INFO(WRITE PAGE)`, `MM.INFO(MODE)` (p.57-58); the word `FRAMEBUFFER` as a page selector.
- `FRAMEBUFFER CREATE HorizontalSize%, VerticalSize%` / `WRITE` / `BACKUP` / `RESTORE [x, y, w, h]` / `WINDOW x, y, page [,I or B]` / `CLOSE` (p.77); `MM.INFO(FRAMEBUFFER)`, `(FRAMEH)`, `(FRAMEV)`.
- `BLIT READ ... [,pagenumber]`, `BLIT ... [, page] [,orientation]`, 64 BLIT buffers, `BLIT WRITE` default orientation 4 (p.68-69).
- The whole `SPRITE` family (p.107-109) and `SPRITE()` functions (p.127-128), 64 sprites, SPRITE LOAD of Maximite .spr files (p.53), `SPRITE LOADPNG` with 4-bit alpha cut-off, `SPRITE SCROLL`/`SCROLLR`, `SPRITE NEXT`/`MOVE`, `SPRITE SWAP`, `SPRITE TRANSPARENCY`, `SPRITE WRITE`, `SPRITE READ ... [,pagenumber]`.
- `IMAGE RESIZE / RESIZE_FAST / ROTATE / ROTATE_FAST / WARP_H / WARP_V` (p.81-82).
- `BOX AND_PIXELS/OR_PIXELS/XOR_PIXELS` (p.69), `PIXEL FILL x, y, c` (p.98), `PIXEL(x, y [,page_number])` (p.126).
- `LOAD BMP|GIF|JPG|PNG file$ [, x, y] [, transparency_cut_off]` incl. animated GIF (p.85), `LOAD FONT file$` into font #8 (p.85), `SAVE IMAGE file$ [,x, y, w, h]` (p.104).
- `RGB(... [, trans])`, `NOTBLACK`, `MAP(number)` semantics, `MAP MAXIMITE` (p.87, p.122, p.126-127).
- Font table 1-7 with font 3 = 16x24 (p.34); `DEFINEFONT` protection of fonts 1, 6, 7 (p.74).
- `GUI CURSOR ...` (p.79) and the mouse-driven GUI controls, `OPTION MAXCTRLS n` (p.78), `CLICK()`.
- `TURTLE DRAW TURTLE / DRAW PIXEL / FILL PIXEL / DRAW LINE / DRAW CIRCLE x, y, r` (p.112).
- `GETSCANLINE` with MM.VRES-relative semantics (p.120), `OPTION Y_AXIS DOWN|UP` (p.60), `OPTION EDIT FONT ...` (p.62), `OPTION STATUS ON|OFF` (p.64), `OPTION SLEEP n` (p.64), `OPTION COLOURCODE REVERSE` (p.61).
- `DRAW3D` / `DRAW3D()` 3D engine (p.75, p.118).

Sound
- Built-in DAC audio with no OPTION AUDIO; `PLAY EFFECT file$ [,interrupt]` (p.98); `PLAY TTS [PHONETIC] "text" [,speed] [,pitch] [,mouth] [,throat] [, interrupt]` (p.100); `PLAY MODFILE file$ [,samplerate]` and `PLAY MODSAMPLE ... [,samplerate]` (p.99); FLAC/WAV up to 96 kHz 24-bit; `DAC n, voltage`, `DAC START frequency, DAC1array%() [,DAC2array%()] [,interrupt]`, `DAC STOP` (p.73); `MM.INFO$(SOUND)` values TTS/DAC/MODFILE (p.58).

I/O and devices
- Connector pin numbering 1-40 with fixed peripheral pins, pins 41 (IR) and 42 (DS18B20) (p.14-15); `SETPIN pin, AIN, bits` (8-16 bit ADC), `SETPIN pin, DOUT, OC` (p.105); the fast-count pin 18 (p.45); `PIN("BAT"|"TEMP"|"DAC1"|"DAC2"|"SREF"|"IREF")` (p.125-126).
- `ADC OPEN frequency, channel1-pin [...] [, interrupt]`, `ADC TRIGGER channel, level`, auto bit-depth from frequency (p.67).
- `PWM 1, freq, 1A [, 1B [, 1C]]` / `PWM 2, ...` / `PWM channel, STOP` (p.102); `SERVO 1 [, freq], 1A ...` in ms (p.105).
- `CONTROLLER MOUSE|CLASSIC|NUNCHUK OPEN/CLOSE` with I2C channel, Hobbytronic USB mouse, PS2 mouse on channel 0 (p.71-72); `MOUSE(funct [, channel])`, `CLASSIC(funct [, channel])`, `NUNCHUK(channel, funct)` incl. calibration codes (p.124-125); `WII CLASSIC|NUNCHUK OPEN [n] ...` aliases (p.114); `OPTION MOUSE n [,sensitivity]` (p.63).
- I2C3 (Wii connector), COM1 DE pin / RS-485 `DEN`/`DEP`, `OC` and `INV` COM options, 1 Mbaud, `COM3:` = console UART with `OPTION CONSOLE SCREEN` (p.95-96, p.132); `BAUDRATE(comm [, timeout])` (p.115); 25 MHz SPI with 32-bit transfers (p.138).
- `SETTICK FAST frequency, target` (p.106); `BITBANG BITSTREAM pinno, n_transitions, array%()` and `BITBANG WS2812 type, pin, colours%()` (p.68).
- `OPTION SERIAL PULLUP ENABLE|DISABLE`, `OPTION SD TIMING NORMAL|FAST`, `OPTION RTC CALIBRATE ±n`, `OPTION DS3231 ON/OFF`, `OPTION VCC`, `OPTION OVERCLOCK ON/OFF`, `OPTION BASELINE ON/OFF`, `OPTION USBKEYBOARD nn [,noLED]`, `OPTION CRLF mode`, `OPTION KEYBOARD REPEAT firstchar, nextchar` (p.60-65).
- Battery-backed 4 KB `VAR SAVE` area (p.113), `MM.INFO$(RESET)` (p.57), `CPU RESTART` (p.73), `UPDATE FIRMWARE` (p.113).

Program model / language
- SD-card-resident programs with `#INCLUDE file$` (".INC"), `#DEFINE "before", "after"`, `#COMMENT START/END`, `#MMDEBUG ON/OFF`, `MMDEBUG`, `MMDEBUG BREAK` + `CONTINUE` (p.66, p.91); `OPTION FLASH [n]` / `OPTION RAM` (p.62-63); `OPTION SEARCH PATH pathname$` and `MM.INFO$(SEARCH PATH)` (p.64, p.58); `AUTOSAVE file$`, `LIST FILES`, `LS`, `FILES` file manager, `LIST PROFILE [CSV]`, `OPTION PROFILING`, `OPTION CONSOLE SAVE|PORT n`, `OPTION F11|F12 string$`.
- `RUN file$, cmdline` with string-constant file names (p.103), `MM.CMDLINE$`, `*prog args` (p.66).
- `MM.DEVICE$ = "Colour Maximite 2"` (p.56), `MM.INFO(VERSION)` (p.58), `MM.INFO$(DIRECTORY)` (p.56), `MM.INFO$(KEYBOARD)` = CONNECTED/NOT CONNECTED (p.57), `MM.INFO$(PIN pinno)` vocabulary (p.57), `MM.INFO(OPTION ...)` CMM2 list (p.57).
- `PEEK(VARTBL, ±offset)`, `POKE VARTBL, offset, byte`, `PEEK(PROGMEM, ±offset)` (p.100, p.125); `MEMORY SET/COPY` current semantics (p.90-91).
- `TIME$ = ±sec` (p.111); `STRING$(nbr, ascii)` 32-126 range check (p.129); `MID$()=` never lengthening (p.91); `MATH Q_CREATE` honouring OPTION ANGLE (p.89); 32-character names, 100-deep loop nesting, 512+512 variables (p.55).
- CSUBs compiled for the Cortex-M7 with the `routinechecks()` contract (p.72-73).

---

## 4. Recommended design for the compatibility switch

### 4.1 The switch itself
- Add `OPTION PICOMITE ON|OFF` (temporary, reset at program end like OPTION LEGACY, p.62-63) plus a
  permanent `OPTION PICOMITE DEFAULT ON|OFF` so a PicoMite-style auto-run machine can boot in that
  mode. Also auto-enable it when the program's first non-comment line is one of the PicoMite-only
  prologue statements (`OPTION ESCAPE`, `LIBRARY LOAD`, `OPTION SYMBOLS OFF`, `TYPE`), and offer
  `RUN file$, PICO` / `*prog --pico`. Keep a single global `PicoMode` flag read by the handlers below.
- In PicoMode `MM.DEVICE$` should return the PicoMite string (take the exact text from the PicoMite
  source; the manual does not list it), `MM.VER` must exist (return a value >= the PicoMite version
  the programs expect, e.g. 7.0000, while `MM.INFO(VERSION)` keeps the CMM2 number), and
  `MM.INFO$(PLATFORM)` should return "Colour Maximite 2" so programs can still detect the host.

### 4.2 Keywords needing dual handlers selected by the mode flag
(same keyword, incompatible argument lists)
- `MODE` (CMM2 `MODE r, bits [, bg [, int]]` vs PicoMite `MODE n`), `OPTION DEFAULT MODE n`.
- `FRAMEBUFFER` (CREATE with/without size, WRITE with/without buffer code; LAYER/COPY/WAIT/MERGE/SYNC
  added in PicoMode).
- `BLIT WRITE` default orientation (4 vs 0); `BLIT READ` (page arg vs none); new BLIT sub-commands
  only in PicoMode.
- `LOAD PNG` 3rd/4th argument (alpha cut-off 1-15 vs transparent index + alphacut); `LOAD JPG`
  extra args; `LOAD IMAGE` keyword.
- `PLAY MODFILE` 2nd argument (samplerate vs interrupt + stop-after-one-pass).
- `PLAY SOUND` type U / `PLAY LOAD SOUND` (PicoMode only).
- `MAP()` function (CLUT entry vs canonical RGB332) and `MAP(n)=` index range.
- `FONT`/`TEXT` font 8 (built-in 4x6 in PicoMode vs LOAD FONT slot) and optionally font 3
  (16x16 "VGA" variant when emulating a VGA build; keep 16x24 when emulating HDMI).
- `SETPIN` (GP names, device-allocation forms, ARAW, CIN edge option, PWM[nx], IR, HEARTBEAT),
  `PIN()`/`PIN()=`/`PORT` pin-number translation, `PIN(TEMP)`/`PIN(BOOTSEL)` keyword forms.
- `PWM` (`channel, freq, [dutyA] [,dutyB] [,phase] [,defer]`, `OFF`, `SYNC` vs `1|2, freq, 1A, 1B, 1C` / `STOP`).
- `SERVO` (position -20..120 vs ms pulse width).
- `ADC OPEN` (channel count vs pin list), `ADC START` scaling args, `ADC RUN`.
- `WS2812 type, pin, nbr, value%[()]` (top-level, extra `nbr`) vs `BITBANG WS2812 type, pin, colours%()`.
- `FILES` (listing vs file manager), `EDIT FILE`, `AUTOSAVE` forms, `XMODEM` file-less forms.
- `COPY`/`KILL` wildcard and drive-qualified forms, `OPEN` path drive-letter translation.
- `MM.INFO$(KEYBOARD)`, `MM.INFO$(PIN n)`, `MM.INFO(FILESIZE)`, `MM.INFO(EXISTS FILE)`,
  `MM.INFO$(SOUND)`, `MM.INFO(OPTION x)` (return the PicoMite vocabularies in PicoMode).
- `OPTION KEYBOARD nn ...` (layout) vs `OPTION KEYBOARD REPEAT`, `OPTION MOUSE pins|DISABLE|SENSITIVITY`,
  `OPTION CONSOLE NONE`, `OPTION AUTORUN n [,NORESET]`.
- `GETSCANLINE` (0-525 scaled to the emulated 640x480 timing in PicoMode).
- `KEYDOWN()` shifted function keys (+&H20 in PicoMode vs +&H40).
- `STRING$()` range check (0-255 in PicoMode), `MID$()=` insert mode (num = 0), `PRINT` trailing comma.
- `CSUB` must raise a clear "CSUB not portable" error in PicoMode rather than executing foreign code.

### 4.3 Keywords that can be plain aliases (no mode flag needed)
- `MM.VER` -> `MM.INFO(VERSION)` (value policy above); `MM.FONTHEIGHT`/`MM.FONTWIDTH`/`MM.HPOS`/`MM.VPOS`
  -> the `MM.INFO(...)` forms; `MM.INFO(FONT)` -> current font number; `MM.INFO(WRITEBUFF)` ->
  `MM.INFO(WRITE PAGE)`; `MM.INFO(BOOT)` -> `MM.INFO$(RESET)` with string translation; `MM.ERRLINE`
  -> existing error-line tracking.
- `FILL x, y, c [,border]` -> `PIXEL FILL x, y, c` (border form needs a small extension).
- `FLUSH [#]fnbr` -> f_sync on the file; `RANDOMIZE n`, `MATH RANDOMIZE [n]` -> no-op; `MATH(RAND)` -> RND.
- `CPU SLEEP n` -> `PAUSE n*1000`; `CPU SPEED n` -> no-op; `ON ERROR RESTART` -> `CPU RESTART` on error.
- `WATCHDOG HW t` -> `WATCHDOG t`; `SETTICK PAUSE|RESUME` -> implement on the existing 4 tick timers.
- `TOUCH(X)|(Y)|(DOWN)|(UP)` -> `CLICK(...)`; `CLICK(REF)|(LASTREF)|(LASTX)|(LASTY)` -> GUI state.
- `OPTION GUI CONTROLS n` -> `OPTION MAXCTRLS n`; `OPTION KEYBOARD nn` -> `OPTION USBKEYBOARD nn`
  ("GR" -> "DE"); `OPTION RTC AUTO ENABLE|DISABLE` -> `OPTION DS3231 ON|OFF`; `RTC GETTIME` -> read DS3231.
- `LOAD "file",R` and `CHAIN fname$ [cmdline$]` -> `RUN fname$ [, cmdline$]` (variables are lost -
  document it); `CMM2 LOAD`/`CMM2 RUN` -> `RUN`; `FLASH RUN n` / `FLASH CHAIN n` -> `RUN` of the
  slot file (4.4); `FLASH SAVE n` / `SAVE file$` -> copy the current program file.
- `COLOUR MAP in%(), out%() [,map%()]`, `ARRAY ADD|SET|SLICE|INSERT` -> extend `MATH ADD|SET|SLICE|INSERT`
  with string arrays and register the ARRAY keyword.
- `DEVICE xxx ...` -> dispatch to `HUMID`, `KEYPAD`, `LCD`, `MOUSE`, `WII`, `WS2812`, `BITSTREAM`, `GAMEPAD`, `CAMERA`.
- `WII NUNCHUCK ...` -> `WII NUNCHUK ...`; `DEVICE(NUNCHUCK f)` -> `NUNCHUK(f)`; `DEVICE(WII [CLASSIC] f)` -> `CLASSIC(f)`;
  `DEVICE(MOUSE ch, f)` -> `MOUSE(f, 0)` for the PS2 mouse (W = wheel delta, M = middle button need new code);
  `MOUSE OPEN 2, clk, data` / `MOUSE CLOSE 2` -> `CONTROLLER MOUSE OPEN 0` / `CLOSE 0`;
  `MOUSE INTERRUPT ENABLE 2, sub` -> the LBinterrupt of CONTROLLER MOUSE OPEN.
- `OPTION DISPLAY lines [,chars]`, `OPTION CASE`, `OPTION CONTINUATION LINES`, `OPTION PLATFORM`,
  `OPTION LOGGING`, `OPTION NOCHECK`, `OPTION SYMBOLS OFF`, `OPTION LOCAL VARIABLES` -> accept
  (implement cheaply or ignore).
- `OPTION AUDIO ...`, `OPTION MODBUFF ...`, `OPTION FAST AUDIO`, `OPTION SYSTEM SPI|I2C`, `OPTION SDCARD`,
  `OPTION COUNT`, `OPTION CPUSPEED`, `OPTION HEARTBEAT`, `OPTION PICO`, `OPTION POWER`, `OPTION PSRAM PIN`,
  `OPTION RESOLUTION` (feeds the mode table), `OPTION VGA PINS`, `OPTION HDMI PINS`, `OPTION CDC POLL RATE`,
  `OPTION SERIAL CONSOLE` -> accept and ignore (warn once on the console in verbose mode).
- `SETPIN rx, tx, COM1|COM2`, `SETPIN rx, tx, clk, SPI|SPI2`, `SETPIN sda, scl, I2C|I2C2`, `SETPIN pin, PWM[nx]`,
  `SETPIN pin, IR` -> validate that the GP pins map onto the CMM2's fixed pins for that peripheral
  (or just ignore); `SETPIN GP1, FFIN` -> `SETPIN 18, FIN`; `SETPIN GP25, HEARTBEAT|DOUT` -> no-op.
- `HELP`, `FM` -> existing HELP / FILES; `ZMODEM`/`YMODEM` -> XMODEM (or implement ZMODEM).

### 4.4 Mapping tables
**Pins (PicoMite GPnn -> CMM2 connector pin).** Map by *function* because the CMM2 peripherals are
fixed: COM1 TX (GP0/GP12/GP16/GP28) -> 8, COM1 RX (GP1/GP13/GP17) -> 10, COM2 TX (GP4/GP8/GP20) -> 16,
COM2 RX (GP5/GP9/GP21) -> 11, I2C SDA (GP0/4/8/12/16/20/28) -> 3, I2C SCL (GP1/5/9/13/17/21) -> 5,
I2C2 SDA (GP2/6/10/14/18/22/26) -> 27, I2C2 SCL (GP3/7/11/15/19/27) -> 28, SPI RX/TX/CLK -> 21/19/23,
SPI2 RX/TX/CLK -> 35/38/40, counting inputs GP6..GP9 -> 7/13/15/24 (and GP1 FFIN -> 18), ADC0..ADC3
(GP26-GP29) -> 26/22/24/12 (or any four of the 12 AIN pins), PWM outputs -> 12, 22, 29, 36, 31 in order
of first use, IR -> 41, DS18B20 default -> 42. For plain digital GPnn use a fixed table of the general
pins (32, 33, 26, 18, 37, 7, 13, 15, 24, 8, 10, 11, 16, 12, 22, 29, 31, 36, 19, 21, 23, 35, 38, 40,
3, 5, 27, 28, 41, 42 = 30 pins for GP0-GP29) and reject GP30-GP47. Also accept the physical Pico
numbers 1-40 by converting them to GPnn first (the PicoMite manual's pin diagram order), and parse
string pin names ("GP5") as the PicoMite does ("except PORT command and function").

**Drives / paths.** `B:` -> `A:` (SD card root); `A:` -> `A:/FLASH/` (a configurable directory on the
SD card, so the two PicoMite drives do not collide); `C:` -> error "USB drive not available";
bare `A:`/`B:`/`C:` prompt commands and `DRIVE` set the prefix; `MM.INFO$(DRIVE)` reports the virtual
drive. FLASH slots 1-3 -> `A:/FLASH/SLOT1.BAS` etc.; `LIBRARY` -> `A:/FLASH/LIBRARY.BAS` treated as an
implicit `#INCLUDE` at the top of every program run in PicoMode; saved variables already exist.

**Fonts.** PicoMite 1,2,4,5,6,7 = CMM2 1,2,4,5,6,7 (identical sizes); PicoMite 8 (4x6) must be added
as a built-in on the CMM2 (the CMM2 font-8 LOAD FONT slot moves to a higher number in PicoMode or
LOAD FONT is simply unavailable in PicoMode); PicoMite 3 = 16x24 (HDMI/LCD) = CMM2 3, or a new 16x16
font when the emulated target is a VGA build (make this selectable, e.g. `OPTION PICOMITE VGA|HDMI`).

**Modes (PicoMite resolution + MODE -> CMM2 `MODE r, bits`).**
| PicoMite | Logical size / colours | CMM2 mode | Notes |
|---|---|---|---|
| 640x480 MODE 1 | 640x480, 2 colours + tiles | `MODE 8, 8` | emulate `TILE` by colouring text cells; default PicoMode mode |
| 640x480 MODE 2 | 320x240, 16 col, 2 layers | `MODE 7, 12` (ARGB4444, 2 h/w layers) or `MODE 7, 8` | 12-bit gives the L layer for free (page 1) |
| 640x480 MODE 3 | 640x480, 16 col, 1 layer | `MODE 8, 12` or `MODE 8, 8` | |
| 640x480 MODE 4 | 320x240, RGB555 | `MODE 7, 16` | |
| 640x480 MODE 5 | 320x240, 256 col palette | `MODE 7, 8` + `MAP` | PicoMite MAP(0-255) maps directly to the CMM2 CLUT |
| 720x400 MODE 1/2/3/4/5 | 720x400, 360x200 | `MODE 2, x` (640x400) / `MODE 3, x` (320x200) | width differs (720/360 vs 640/320); MM.HRES tells the program |
| 800x600 MODE 1/3 | 800x600 | `MODE 1, x` | |
| 800x600 MODE 2/5 | 400x300 | `MODE 13, x` | |
| 848x480 MODE 1/3 | 848x480 | `MODE 10, x` | |
| 848x480 MODE 2/5 | 424x240 | none | nearest `MODE 7` (320x240) or `MODE 17` (384x240) |
| 1280x720 MODE 1 | 1280x720 | `MODE 11, 8` | |
| 1280x720 MODE 2/5 (320x180), MODE 3 (640x360) | | none | nearest `MODE 7` / `MODE 2`, or add new CMM2 modes |
| 1024x768 MODE 1 | 1024x768 | `MODE 9, 8` | |
| 1024x768 MODE 2/5 (256x192), MODE 3 (512x384) | | none | nearest `MODE 6` (256x240) / `MODE 13` |
| 1024x600, 800x480 (all modes) | | none | nearest 1024x768 / 848x480 family |
| LCD `MODE 400` / `MODE 800` | 400x240 / 800x480 | none | error in PicoMode |

The 16-colour index palette (0 Black, 1 Blue, 2 Myrtle, 3 Cobalt, 4 Midgreen, 5 Cerulean, 6 Green,
7 Cyan, 8 Red, 9 Magenta, 10 Rust, 11 Fuchsia, 12 Brown, 13 Lilac, 14 Yellow, 15 White, RGB values
from the PicoMite VGA `MAP RESET` table) must be a static table used by LOAD PNG transparent,
FRAMEBUFFER LAYER colour, BLIT MERGE/FRAMEBUFFER/MEMORY/RESIZE colour, COLOUR MAP, TILE names and by
`RGB()` for the seven extra colour names.

**Device strings.** `MM.DEVICE$` -> PicoMite string in PicoMode (from source); `MM.INFO$(SOUND)`
MODFILE -> "MOD", "PAUSED" -> "PAUSED xxx"; `MM.INFO$(KEYBOARD)` -> "On"/"Off"; `MM.INFO$(PIN n)` ->
"OFF"/"DIN"/"DOUT"/"AIN"...; `MM.INFO(BOOT)` -> "Power On"/"Restart"/"S/W Watchdog"/"Reset Switch";
`MM.INFO$(DRIVE)` -> "A:"/"B:" virtual letters; `MM.INFO$(LCDPANEL)` -> the emulated mode number.

### 4.5 PicoMite features that would be no-ops or errors on the CMM2
- Silent no-ops: `RANDOMIZE`, `MATH RANDOMIZE`, `OPTION AUDIO *`, `OPTION MODBUFF`, `OPTION SYSTEM SPI/I2C`,
  `OPTION SDCARD`, `OPTION CPUSPEED`, `CPU SPEED`, `OPTION HEARTBEAT`, `OPTION PICO*`, `OPTION POWER`,
  `OPTION PSRAM PIN`, `OPTION CDC POLL RATE`, `OPTION SERIAL CONSOLE`, `OPTION VGA/HDMI PINS`,
  `OPTION COMPILE`, `OPTION SYMBOLS OFF`, `OPTION NOCHECK`, `OPTION LOCAL VARIABLES`, `OPTION COUNT`,
  `SETPIN GP25, HEARTBEAT`, `SETPIN ..., PIOn`, `FLUSH`, `OPTION FAST AUDIO`, `OPTION LCD320`,
  `OPTION AUTOREFRESH`, `REFRESH`, `BACKLIGHT`, `KEYBOARD ON|OFF`, `FRAMEBUFFER WAIT` (wait for blanking),
  `FRAMEBUFFER SYNC`, `OPTION SCREEN KEYBOARD`, `OPTION LOGGING`, `MM.END`/`MM.STARTUP`/`MM.PROMPT` hooks (ignore).
- Errors ("not available on the Colour Maximite 2"): `PIO *` and `PIO()`, the PIO mnemonics (JMP, WAIT,
  IN, OUT, PUSH, PULL, MOV, IRQ, SET) when PicoMode is on, `WEB *`, `OPTION WIFI/TCP/UDP/TELNET/TFTP/
  WEB MESSAGES/CYW43/RADIO`, `MM.ADDRESS$`/`MM.MESSAGE$`/`MM.TOPIC$`, `BLUETOOTH *`, `CAMERA *`,
  `STEPPER`, `TMC22xx`, `MM.CODE`, `GAMEPAD *` (unless mapped to Wii), `MM.INFO(USB ...)`, `MM.USB`,
  `FLASH UF2`, `RAM *`, `OPTION LCDPANEL *` (all LCD controllers), `OPTION TOUCH`, `GUI CALIBRATE`,
  `GUI TEST TOUCH`, `GUI RESET LCDPANEL`, `POKE DISPLAY`, `MEMORY SHARE`, `I2C SLAVE *` (unless implemented),
  `I2CLCD *`/`LCD *`/`KEYPAD` (unless implemented on GPIO), `PLAY MIDI*`/`PLAY NOTE*`/`PLAY STREAM`,
  `OPTION AUDIO BLUETOOTH`, `COM3:`-`COM7:` in the PicoMite sense, `DRIVE "C:"`, `ONESHOT`, `SYNC`
  (could be implemented with a hardware timer), `CSUB` blobs, `MODE 400|800`, `TILEMAP`/`RAY`/`FRAME`
  until ported, `MM.INFO(PSRAM SIZE)` (return 0), `MM.INFO(MAX GP)` (return 29).
- Behavioural notes to document for users: variables are not preserved across `CHAIN`/`FLASH CHAIN`
  emulation; PicoMite sprites/BLITs being RGB121 means programs may rely on exactly 16 colours
  (use the 12-bit CMM2 mode so that transparent colour index 0 maps to alpha 0); `GETSCANLINE` is
  rescaled; `KEYDOWN` shifted F-keys differ unless remapped; `MM.VER` policy; `PLAY VOLUME` curve.

### 4.6 Implementation order suggestion
1. Mode flag, `MM.VER`, `MM.DEVICE$`, `OPTION ESCAPE`, `/* */`, `RANDOMIZE`, `FLUSH`, `TRIM$`, `BIT`, `BYTE`,
   `FLAG/FLAGS`, `LMID`, `LINPUT`, `DO UNTIL`/`LOOP WHILE`, `BYVAL/BYREF`, whole-array assignment,
   `MM.FONT*`/`MM.HPOS/VPOS`, `MM.ERRLINE` - cheap and unblock most "pure BASIC" PicoMite programs.
2. Drive/path translation, `LOAD ,R`/`CHAIN`/`FLASH`/`LIBRARY LOAD` -> file mapping, `SAVE`, `FILES`.
3. Pin mapping + `SETPIN` device forms + `PWM`/`SERVO`/`ADC` dual handlers + `WS2812`/`BITSTREAM`.
4. Video: mode table, `OPTION RESOLUTION`/`RESOLUTION`, `FRAMEBUFFER` N/F/L model on top of pages,
   16-colour index palette, `LOAD IMAGE/PNG/JPG` argument forms, `FILL`, `LINE` extensions, `TILE`,
   font 8, `MAP()` semantics, `BLIT` sub-commands, `GUI CLICK`/`TOUCH()` aliases.
5. Sound: `PLAY MODFILE` interrupt form, `PLAY ARRAY`, `PLAY SOUND U`, `PLAY SAMPLE`, BBC sound.
6. Devices: `MOUSE`/`DEVICE()`/`WII NUNCHUCK` aliases, `RTC`, `KEYPAD`, `LCD`/`I2CLCD`, `I2C CHECK`.
7. Structures (`TYPE`/`STRUCT`), array-returning functions, `REDIM`, MATH/ARRAY additions, ZMODEM.
