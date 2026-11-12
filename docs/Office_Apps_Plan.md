# A word processor and a spreadsheet for the Colour Maximite 2

*12 November 2026. A plan for two loadable apps that share one curses library: **WordGrinder** (a word processor)
and **sc-im** (a spreadsheet), both on **PDCursesMod** with a CMM2 back end. Nothing has been built yet. The facts
below were checked on 12 Nov against the sources at the commits named.*

## What we port

| Program | Version | Licence | Size | Language |
|---|---|---|---|---|
| PDCursesMod | 4.5.4, `d494b49` (30 Sep 2026) | Public domain | Its Linux framebuffer port (`fb/`) is 1,744 lines | C |
| WordGrinder | 0.8, `ada51b4` (23 Oct 2020) | MIT | 4,726 lines of C, 15,539 lines (168 KB) of Lua | C and Lua 5.1 |
| sc-im | main, `8cd391b` (28 Oct 2026; last tag v0.8.5) | BSD 4-clause | 29,225 lines | C, a bison grammar |

WordGrinder 0.8 also bundles Lua 5.1.5, lpeg, LuaBitOp (all MIT), minizip (zlib-style) and uthash (BSD). It needs
zlib (zlib licence), which no app has yet.

### Why PDCursesMod, not PDCurses 3.9

- PDCursesMod's `fb/` port is most of the back end already:
  - It draws glyphs into an 8- or 32-bit framebuffer.
  - It fakes bold, italic, underline and strikeout from one font.
  - It reads PSF2 fonts with a Unicode table, and draws its own mouse pointer.
- PDCurses 3.9 has no such port, and has no dim attribute (`A_DIM` is `A_NORMAL`) and no strikeout.
- Both programs' curses calls were listed by grep. Every call is in PDCursesMod, except sc-im's `set_escdelay`.
  It needs a one-line stub, because PDCurses gets whole keys, not escape sequences.

### Why WordGrinder 0.8, not 0.9

- 0.9 (master) moved to **Luau**: 128,581 lines of C++, with a C++ host as well.
- 0.8 is plain C with Lua 5.1, and is the last plain-Lua release.
- 0.9 adds an orgmode export, type-checked scripts and an OpenGL front end. None of these is needed here.
- A later move to 0.9 stays possible: the front end is the same 12 functions.

## In the repository

```
apps/pdcurses/              the shared part
  pdcurses.py               fetches PDCursesMod at d494b49 into apps/pdcurses/PDCursesMod (git ignores it),
                            applies patches/, makes the font, and gives an app's make.py its sources and flags
  cmm2/                     the back end: pdcscrn.c, pdckbd.c, pdcmouse.c, pdcutil.c, pdcsetsc.c, pdcgetsc.c,
                            pdcclip.c, and include/ncurses.h (one line: #include <curses.h>)
  patches/                  only if fb/pdcdisp.c needs changing (it should not)
  make.py                   builds out/pdcdemo.app: PDCursesMod's own demos, for step 1
apps/wordgrinder/
  make.py, patches/, cmm2_main.c, out/wordgrinder.app
apps/scim/
  make.py, patches/, cmm2_main.c, out/scim.app
```

Each `make.py` works the way `apps/trs80/make.py` does:
- It fetches one commit and checks HEAD (a broken `.git` lets `git -C` fall through to this repository).
- It applies `patches/*.patch` behind a stamp.
- It builds with `tools/sdk/build.py --api N`, without `--vram`, so that a G1 keeps its higher-resolution modes
  (see "Screen sizes").

PDCurses is compiled into each app, not built as a separate library file.

## The back end (apps/pdcurses/cmm2)

`fb/pdcdisp.c` is used as it is. It holds `PDC_transform_line`, `PDC_gotoyx`, the text cursor, the synthesised
attributes and the mouse pointer. The new files feed it and take input:

| PDCurses entry | On the CMM2 |
|---|---|
| `PDC_scr_open` | `video_mode(mode, 8, &v)`, then fills the `video_info` that the fb code reads: framebuffer, width, height, 8 bpp, pitch. Modes whose lines are shown twice (`v.copies == 2`: MODE 3, 5, 6, 7) are not used: they are low resolutions only, too coarse for text, and MODE 1 is used instead. `LINES` and `COLS` come from the font's size. |
| `PDC_scr_close` | Nothing: the loader puts BASIC's mode and palette back when the program ends. |
| `PDC_init_color`, `PDC_color_content` | Palette entries, through `video_palette`. `COLORS` is 256, set up as xterm's 256 colours (as `apps/vt100` does). |
| `PDC_get_key`, `PDC_check_key` | `con_getc`, the modifiers from `keys_held()[6]`, and `break_take` for Ctrl-C (given as key 3, as vt100 does, because WordGrinder uses ^C for Copy). See the key table below. |
| mouse | The `mouse` service (x, y, buttons, wheel) gives PDCurses its press, release and click events, with the wheel as buttons 4 and 5. The pointer is drawn by fb's `PDC_update_mouse_cursor`. |
| `PDC_napms` | `delay_us` in steps of 1 ms or less, calling `poll()` so USB keeps working. |
| `PDC_beep` | A short `PLAY TONE` through `basic_cmd`, or nothing. |
| clipboard (`pdcclip.c`) | A buffer in the program: there is no system clipboard. |
| `set_escdelay` | A stub that returns OK. |

**Unicode:** the library is built with `PDC_WIDE` and `PDC_FORCE_UTF8`. PDCurses then does its own UTF-8 and never
depends on the C library's locale.

**Fonts:** Spleen (BSD-2), fetched as `apps/vt100` fetches it. `pdcurses.py` turns its BDF files into PSF2 with a
Unicode table, built in as C arrays, so PDCursesMod's `psf.c` reads them unchanged. Spleen 8x16 has 1,001 glyphs:

- all of ASCII, Latin-1, Latin Extended-A, box drawing and block elements;
- 74 of Cyrillic's 256 characters and 34 of Greek's 144.

A character Spleen lacks shows as PDCurses's replacement glyph. The sizes are 6x12, 8x16, 12x24 and 16x32.

### Screen sizes

Users will mostly want these apps at high resolutions, so nothing assumes a size.

**Depth: always 8 bits.**
- 8 bits is the only depth every mode offers. The loader's video service refuses 16 bits in MODE 15 (1280x1024) and
  MODE 16 (1920x1080), as MODE does.
- fb's drawing code knows 8 and 32 bits per pixel only.
- 256 colours are plenty for text.

**The mode, in this order:**
1. `mode=` after RUN's comma.
2. Else the mode last chosen in the program, saved in `pdcurses.cfg` in its folder.
3. Else the mode BASIC is in when RUN is given (`MM.INFO(MODE)`, through `basic_eval`). OPTION DEFAULT MODE has
   already made that one the monitor shows.

**The font, in this order:**
1. `font=` after RUN's comma.
2. Else the font last chosen in the program.
3. Else one chosen by the width: 8x16 up to 1280 pixels wide, 12x24 above. 8x16 at 1920x1080 would be 240 x 67
   characters, which is too small.

**While a program runs:**
- Alt-Minus cycles the fonts (the fb port's own key), and Alt-= cycles the modes (on the fb port, Alt-= cycles the
  displays).
- Each sends `KEY_RESIZE` to the program, and saves the choice. Check that both programs handle `KEY_RESIZE`.

`cmm2_main.c` passes `mode=` and `font=` to the back end in the environment variables `PDC_MODE` and `PDC_FONT` (as
the fb port reads `PDC_FONT`), and removes those words before the program sees its arguments.

| Mode | Size | 8x16 | 12x24 | G1 | G2 |
|---|---|---|---|---|---|
| 8 | 640x480 | 80 x 30 | 53 x 20 | Yes | Yes |
| 1 | 800x600 | 100 x 37 | 66 x 25 | Yes | Yes |
| 10 | 848x480 | 106 x 30 | 70 x 20 | Yes | Yes |
| 14 | 960x540 | 120 x 33 | 80 x 22 | Yes | Yes |
| 18 | 1024x600 | 128 x 37 | 85 x 25 | Without `--vram` | Yes |
| 9 | 1024x768 | 128 x 48 | 85 x 32 | Without `--vram` | Yes |
| 11 | 1280x720 | 160 x 45 | 106 x 30 | Without `--vram` | Yes |
| 15 | 1280x1024 | 160 x 64 | 106 x 42 | Without `--vram` | Yes |
| 16 | 1920x1080 | 240 x 67 | 160 x 45 | No | Yes, but not on a revision Y chip (the video service checks) |

**Why not `--vram`:** a G1 program built with `--vram` must keep the shown page within 512 KB of internal RAM (the
video service's check). At 8 bits that stops at MODE 14 (960x540). Without it, the G1 keeps its 3 MB of display pages
for the larger modes. In exchange, the app gets only the largest free part of BASIC's heap less 256 KB: about
4.5 MB of a G1's 5 MB, or 1 MB less with OPTION COMPILE ON. That is plenty for sc-im. WordGrinder's need is measured
in step 2.

**What a wide screen gives each program:**
- WordGrinder uses the full width by default. Its Look and Feel setting (File, Global settings) limits the text to
  80 columns (`maxwidth`, which the user can change), but it is off unless the user turns it on (corrected 12 Nov,
  from the code: `enabled = false`).
- sc-im uses every column: the higher the resolution, the more cells are visible.

**Drawing cost:** 1920x1080 at 8x16 is 16,080 cells (about 2 MB of pixels), against 3,700 at 800x600. PDCurses
redraws only the cells that change. A full redraw is timed once in step 1.

### Keys (MMBasic's codes to PDCurses)

| CMM2 key | PDCurses |
|---|---|
| `&H80`-`&H83` arrows | `KEY_UP` / `DOWN` / `LEFT` / `RIGHT`. With Shift: `KEY_SR`, `KEY_SF`, `KEY_SLEFT`, `KEY_SRIGHT`. With Ctrl: `CTL_*`. With Alt: `ALT_*`. |
| `&HA1`-`&HA4` (Shift with an arrow) | As above, with Shift. |
| `&H84` Insert, `&H7F` Delete | `KEY_IC`, `KEY_DC` (with Ctrl or Alt: `CTL_INS`, `CTL_DEL`, `ALT_INS`, `ALT_DEL`) |
| `&H86` Home, `&H87` End, `&H88` PgUp, `&H89` PgDn | `KEY_HOME`, `KEY_END`, `KEY_PPAGE`, `KEY_NPAGE`, and the `KEY_S*`, `CTL_*` and `ALT_*` forms |
| `&H91`-`&H9C` F1-F12 | `KEY_F(1)`-`KEY_F(12)` |
| `&HB1`-`&HBC` Shift with F1-F12 | `KEY_F(13)`-`KEY_F(24)`, as ncurses numbers them |
| `&H9F` Shift-Tab | `KEY_BTAB` |
| 8, 9, 13, 27 | Backspace (`KEY_BACKSPACE`), Tab, Enter, Esc |
| Ctrl-C (`break_take`) | 3 |
| `&HA0`-`&HFF` from a layout (é, £...) | That Latin-1 character as Unicode |

Every key also records its modifiers for `PDC_get_key_modifiers()`. That is how WordGrinder tells Ctrl+Shift with an
arrow apart (see below).

**Collision:** a layout's Latin-1 letters from `&HA0` to `&HBF` can share a code with Shift and Delete (`&HA0`),
Shift and an arrow (`&HA1`-`&HA4`), or Shift and an F key (`&HB1`-`&HBC`). `Src/KeyboardMap.c` (checked 12 Nov) has
three, all in the French layout:
- £ (163) is also Shift with Right;
- ² (178) is also Shift with F2;
- µ (181) is also Shift with F5.

¨ (168) and § (167) clash with nothing. BASIC's INKEY$ can't tell these apart either. The back end looks at
`keys_held`: if the arrow or F key is held, it is that key; otherwise it is the letter. See "System services" for
the case where that guess is wrong.

## System services

**No new service is needed.** The back end and the apps use these:

| Services | Version | For |
|---|---|---|
| `con_getc`, `poll`, `delay_us`, `micros`, `datetime`, the files and directories | 1 | Keys, timing, files (through `syscalls.c`) |
| `video_mode`, `video_palette`, `keys_held` | 2 | The screen, the palette, the modifiers |
| `mouse` | 4 | The mouse |
| `break_take`, `file_stat` | 5 | Ctrl-C, `stat` |
| `basic_cmd` | 11 | `PLAY TONE` for `beep()` |
| `basic_eval` | 12 | `MM.INFO(MODE)`, the mode BASIC is in |

So both apps are built with `--api 12`, and run on RC5 and on V8 (both at version 18). The text, the mouse pointer and
the clipboard are the back end's own, so they need no service.

### Worth adding: each key with its modifiers (version 19)

The firmware's key queue (`ConsoleRxBuf`, filled by `USR_KEYBRD_ProcessData` in `Src/KeyboardMap.c`) holds bytes only.
A program reads the modifiers from `keys_held`, which gives the keys held **when it reads the key**, not when the key
was pressed. That loses the modifier in two cases:
- **Ctrl or Alt with an arrow, Home, End, Page Up or Page Down, and Shift with Home, End, Page Up or Page Down.** The
  Shift forms of those four keys give the same code as without Shift.
  - If the user has let go of the modifier before the program reads the key (typing ahead while it loads a file or
    redraws a big screen), the key arrives plain.
  - In WordGrinder, Ctrl with Right then moves one character instead of one word, and Shift with End moves without
    selecting.
- **The French layout's £, ² and µ** (above). If Shift with Right is pressed and let go before it is read, it becomes
  a £ typed into the document.

These cases depend on timing and should be rare. The apps work without the new service, but it would make them
exact:

- **The service:** `int con_getkey(int *info)`. It gives the next key as `con_getc` does, and in `info` the modifier
  bits (`keys_held`'s layout) as they were when the key was queued. A flag in `info` also says whether the code came
  from the layout's characters or from a special key.
- **In the firmware:** a second ring of bytes beside `ConsoleRxBuf`, of the same size, written where the key is
  stored. The HID mapping sets the flag. Keys from the serial console get 0. About 30 lines, on the branch Peter
  chooses.
- **In the back end:** it uses `con_getkey` when the services' version is 19 or later, and `con_getc` with
  `keys_held` otherwise. The apps stay at `--api 12`.

## The C library

- `build.py` builds with newlib-nano (`--specs=nano.specs`). Its configuration has no UTF-8 (`_MB_CAPABLE` off,
  `_MB_LEN_MAX 1`) and no wide-character printf.
- A link test on 12 Nov found `swprintf`, `fwprintf`, `wprintf` and `fgetws` missing from it.
- sc-im calls `swprintf` 149 times, `fwprintf` 12, `fgetws` 3 and `wprintf` 2. It also converts between UTF-8 and wide
  characters (`wcstombs`, `mbsrtowcs`).
- The full newlib has the wide-character functions. The same test linked against it with nothing missing, apart from
  `wordexp`, which sc-im can be built without.

So:
- `build.py` gets **`--libc full`**, which drops `--specs=nano.specs`.
- sc-im is built with it.
- WordGrinder and PDCurses do not need it (WordGrinder does its own UTF-8, and PDCurses forces UTF-8), so they stay
  on nano.

**Corrected 12 Nov, after testing on the G1:**
- `--libc full` works on the G1. `swprintf`, `fwprintf` and `fgetws` work, errors are reported, and destructors run
  once.
- But it gives **no UTF-8**. The link test above used the Arm GNU Toolchain's newlib, which has `_MB_CAPABLE`.
  `build.py` uses STM32CubeIDE's toolchain (the folder `CMakePresets.json` names), and both of its newlibs are built
  without it.
  - `setlocale` takes only `"C"`, so no other locale name can be set.
  - `mbstowcs` makes each byte one character, which is Latin-1.
  - `wcwidth` gives -1 for every character above ASCII.
- This only matters to sc-im: see "sc-im's character set" in step 3.

**One SDK fix at the root:** `syscalls.c`'s `mkdir` turns every failure into `EIO`. WordGrinder's `Mkdirs` accepts
only `EEXIST`, `EACCES` or `EISDIR` for a folder that is already there, so it would stop on one. The fix is to map
FatFs's `FR_EXIST` (8) to `EEXIST` and `FR_DENIED` (7) to `EACCES`, in `mkdir` and in the other calls that turn
FRESULTs into errno values.

Everything else either is in the SDK already or fails cleanly:
- The SDK gives `open`, `read`, `write`, `lseek`, `stat`, `mkdir`, `rename`, `unlink`, `chdir`, `getcwd`, `readdir`
  (with `..` since 12 Nov) and `gettimeofday`.
- `fork`, `execve`, `pipe` and `waitpid` are stubs that fail, so the shell features fail rather than hang.

## WordGrinder 0.8 (apps/wordgrinder)

**Build.** `make.py` builds:
- `src/c/*.c` and `src/c/arch/unix/cursesw/dpy.c`;
- the bundled Lua 5.1.5 (without `lua.c` and `luac.c`), lpeg, LuaBitOp, minizip and uthash;
- zlib 1.3.1, fetched at a fixed commit.

`build.lua` turns `src/lua/**/*.lua` into `luascripts.c` (`script_table`) with `tools/multibin2c.lua`. `make.py` does
the same in Python, and checks once that its output is byte-identical to the Lua tool's, run in WSL.
- **Flags:** those `build.lua` gives a static Unix build, less ncurses's (`-DFILEFORMAT=8`, `-DNDEBUG`,
  `-DLUA_USE_EMU_TMPNAM`).
- **`main`:** renamed with `-Dmain=wg_main`, as WordGrinder's Windows build renames it `appMain`.

**`ARCH` = `"windows"` for the Lua code.** Only `browser.lua`, `main.lua` and `addons/spillchocker.lua` read it,
and the Windows branches are the right ones here:
- drive letters (`A:` typed in the file browser);
- filename matching without regard to case (FAT ignores case);
- the dictionary found beside the program.

`cmm2_main.c` sets:
- `WINDOWS_EXE` to the app's path (main.lua finds its folder from it);
- `HOME` to the folder it was started in, so the settings go in `A:/wordgrinder/.wordgrinder/`;
- `PDC_MODE` and `PDC_FONT`.

It then calls `wg_main`. Before relying on this, check that `ARCH == "windows"` switches nothing else on the C side:
the C code tests the `WIN32` macro, not `ARCH`.

**Patch 0001, key names (`dpy.c`).** ncurses names Ctrl and Alt with arrows by terminfo names (`kLFT5` and so on),
which `dpy_getkeyname` decodes. PDCurses has its own codes (`CTL_LEFT`, `ALT_LEFT`, `CTL_HOME`...). The patch adds,
under `#ifdef PDCURSES`, a table from those codes to the names WordGrinder's keymap uses (`KEY_^LEFT`, `KEY_ALEFT`,
...). PDCurses has no code for Ctrl+Shift with an arrow, so the patch reads `PDC_get_key_modifiers()` to make
`KEY_S^LEFT` from `CTL_LEFT` with Shift held. The rest of `dpy.c` uses ordinary curses: `get_wch`, `attrset`,
`mvaddnstr` and `timeout`.

**What it gives on the CMM2:**
- Paragraph styles: headings, lists, quotes, pre.
- Bold, italic and underline, drawn by the back end.
- Its own `.wg` files. Import and export of text, HTML, ODT and Markdown; export of LaTeX and troff. ODT opens in
  LibreOffice and Word on a PC.
- Spelling against a word list file (none is shipped at first: see step 0), with a user dictionary.
- Its own autosave (timed through `dpy_getchar`'s timeout).
- Cut, copy and paste within the program.
- No printing: export to ODT or HTML and print on the PC.

**Memory:** the whole document is Lua tables.
- A G2 has 8 MB. A G1 has about 4.5 MB, since the apps are built without `--vram` (see "Screen sizes").
- Measure the Lua heap once with a 50-page document, and note the result in the guide.

**Tests:**
- **Automatic:** the 66 scripts in `tests/`, each run as its own build runs them:
  `RUN "wordgrinder.app", --lua tests/x.lua`. Compare the console output, as raw bytes, with a WSL build of the
  same commit, masking only the time.
- **By hand:**
  - a document with every style, saved and loaded again;
  - an ODT export opened in LibreOffice, and an ODT and an HTML file from the PC imported;
  - every key in the keymap's table, including the Shift and Ctrl arrows;
  - the menus (Esc), the file browser on `A:` and `B:`, and autosave;
  - all of it on a G2 and a G1.

## sc-im (apps/scim)

**Build.**
- The sources as `src/Makefile` lists them, with `pvmtbl.c` built with `-DPSC` as it builds it.
- `gram.c` from `gram.y` with `bison -y`; `experres.h` and `statres.h` from `gram.y` with `eres.sed` and `sres.sed`.
  `make.py` runs both through WSL, as the MicroPython port builds in WSL, puts the outputs in `out/gen/`, and redoes
  them only when `gram.y` changes.
- `--libc full`.
- `cmm2_main.c` sets `HOME`, `PDC_MODE` and `PDC_FONT`, and calls `main`, renamed with `-Dmain=scim_main`.

**sc-im's character set (decide in step 3).** STM32CubeIDE's newlib has no UTF-8 ("The C library", corrected), so
there are three ways:
1. **Latin-1** (the least work).
   - Each byte of a file is one character, matching the CMM2's keyboard layouts and 8-bit files. PDCurses shows the
     characters correctly.
   - Files are read and written as Latin-1. A UTF-8 CSV from a PC shows é as two characters.
   - newlib's `wcwidth` gives -1 above ASCII, which would misalign sc-im's columns. A proper `wcwidth` and `wcswidth`
     must be supplied, as WordGrinder bundles one (`src/c/emu/wcwidth.c`).
2. **UTF-8 from the Arm GNU Toolchain's newlib**, which has `_MB_CAPABLE`.
   - sc-im is built with that toolchain (the same GCC, 13.3) by pointing `ARM_TOOLCHAIN_DIR` at it.
   - Anyone building it then needs that toolchain installed.
3. **UTF-8 conversion functions of our own** in the SDK.
   - newlib's internal paths (`fwprintf` to a file, `fgetws`) call its own conversion through the locale, so a few
     wrappers would not cover them. Not recommended.

**Flags.**

| Setting | Flags |
|---|---|
| **Kept** | `-DNCURSES -D_XOPEN_SOURCE_EXTENDED -D_GNU_SOURCE -DUSECOLORS -DUNDO -DMAXROWS=65536 -DUSELOCALE -DMOUSE` |
| **Added** | `-DNO_WORDEXP`; `HELP_PATH`, `CONFIG_DIR` and `HISTORY_DIR` set to the app's folder |
| **Left out at first** | `HAVE_PTHREAD`, and with it `AUTOBACKUP`, which needs a thread; `GNUPLOT`; `XLS`, `XLSX`, `ODS`, `XLSX_EXPORT`; `XLUA` |

The pager, editor and clipboard commands are set to empty strings: each one runs a shell command.

**No patches are expected.**
- `tui.c` includes `<ncurses.h>`, which the back end's `include/` provides.
- `set_escdelay` is the back end's stub.
- The shell features (`:!`, pipes, an external editor, plots, the clipboard commands) fail through the stubs. Check
  that each says so and carries on.

**What it gives:**
- Formulas with sc's `@` functions, ranges, sort, filter and subtotals.
- Undo and redo, several sheets, colours.
- vim-style keys, and choosing cells with the mouse.
- Its own `.sc` files; CSV and TSV read and written; Markdown, LaTeX and text exported.
- Its help file, copied to the card beside the app.

**Later, each Peter's choice:**
- XLSX and ODS import: libzip and libxml2. libxml2 is large, but it fits in 8 MB.
- XLSX export: libxlsxwriter, which bundles its own minizip and uses the same zlib as WordGrinder.
- Lua scripting (`XLUA`): the Lua 5.1 from WordGrinder's tree.
- Autosave without a thread: called from the key loop.

**Tests:**
- **Automatic:** sc-im's `tests/*.sh` run `sc-im file.sc --nocurses --nodebug --quit_afterload` with commands on
  standard input (for example `GETNUM C2`).
  - On the board: `RUN "scim.app", ...` with the commands typed through `cmm2serial.py`.
  - Compare the output, as raw bytes, with a WSL build of the same commit. The valgrind parts are left out.
- **By hand:**
  - a budget sheet with formulas, saved as `.sc`, exported as CSV and opened in Excel or LibreOffice;
  - a CSV from the PC imported;
  - the mouse, undo, sort and filter;
  - all of it on a G2 and a G1.

## Steps

### Step 0: Peter's decisions

**Decided 12 Nov (Peter):**
- **Branch:** main, merged into v8-dev, with the SDK changes. The work for main is done in a separate worktree.
- **The screen:** BASIC's own mode with the font chosen by width; no `--vram`.
- **`con_getkey`:** later, and only if testing shows the need. The back end starts with `keys_held`.
- **The boards:** the G2 and the G1, any time. Peter is told when the key and mouse checks need him.

The questions as they were put:

**Needed to start:**
1. **Branch:** main, merged into v8-dev as the TRS-80 app was, or v8-dev only?
   - The same choice covers the SDK changes (`--libc full`, the `mkdir` errors).
   - No firmware change is needed. The services used are all in RC5, so the apps run on V7 and V8.
   - Work for main is done in a separate worktree, so the uncommitted work on v8-dev (`apps/bbc`) is not touched.
2. **The screen:** start in BASIC's own mode with the font chosen by width ("Screen sizes"), or a fixed mode? And
   no `--vram`, so a G1 keeps its large modes and the app gets about 4.5 MB?
3. **`con_getkey` (services version 19, "System services"):** now, later if testing shows the need, or never? The
   apps work without it.
4. **The boards:** which ones step 1's tests may use, and when.
   - The key and mouse checks need Peter at the keyboard.
   - The rest (modes, fonts, timing) can be driven over the serial console.

**Needed later (proposed defaults in brackets):**
5. **Names on the card** (`A:/wordgrinder/wordgrinder.app` and `A:/scim/scim.app`), before step 2.
6. **Spelling** (none shipped, the user's own word list; or SCOWL's English list, which has a permissive licence),
   in step 2.
7. **sc-im's later items** (XLSX and ODS, Lua, autosave: after step 3, one at a time).
8. **The proportional add-on** (after step 2).
9. **Release:** into which zip, and when.

### Step 1: the SDK and the back end
1. `build.py --libc full`. `syscalls.c`: `mkdir` and its kind give `EEXIST` and `EACCES` (see above).
2. `apps/pdcurses`: fetch, the font conversion, and the back end.
3. `pdcdemo.app` runs PDCursesMod's `testcurs` (attributes, colours, ACS lines, Unicode, keys, mouse), `ptest` and
   `worm`. Run it on the G2, then the G1, with nano and with `--libc full`.
4. Check:
   - every attribute and colour pair;
   - every key in the table, with each modifier, and a French or German layout's letters;
   - the mouse, and `napms` timing (`timeout(500)` returns after 500 ms);
   - every 8-bit mode in the table: up to MODE 16 on the G2, MODE 15 on the G1;
   - each font, with `mode=` and `font=`, Alt-Minus and Alt-=, and the saved choice;
   - starting from BASIC in each mode.
5. Time a full redraw at 1920x1080, once.

**Step 1 results (12 Nov, G1 and G2 on V8 8.000001):**
- **SDK test** (errors, `mkdir` of a root, `setenv`, destructors): 13 of 13 on both boards, with newlib-nano and with
  `--libc full`.
  - The full library's wide-character `printf` and file functions work.
  - It has no UTF-8 (see "The C library").
- **All 16 existing apps** build with the changed SDK, with no new warnings.
- **testcurs:**
  - every ACS and WACS character draws;
  - all the attributes draw except dim, which the 8-bit path draws at normal brightness;
  - the 16 colours and `init_color` work;
  - `mode=9 font=12x24` works on the G1, and MODE 16 with 12x24 chosen by width on the G2.
- **speed** (whole-screen refreshes a second, the worst case: curses normally redraws only what changes):

  | Board | MODE 1, 8x16 | MODE 15, 8x16 | MODE 15, 12x24 | MODE 16, 12x24 | MODE 16, 8x16 |
  |---|---|---|---|---|---|
  | G1, 400 MHz | 44 | 25 | 21 | - | - |
  | G2, 480 MHz | 53 | 35 | - | 19 | 22 |

- **By hand (Peter, 12 Nov): all good.**
  - The keys with Shift, Ctrl and Alt, the mouse, Alt-= and Alt-Minus, and the settings file.
  - The mouse was plugged in during the test and was picked up: the back end looks for one again while the program
    asks for mouse events.
  - The French layout's £, ² and µ were not tried (optional).

Step 1 is done.

### Step 2: WordGrinder
1. `make.py`, zlib, `luascripts.c`, patch 0001 and `cmm2_main.c`.
2. The 66 tests compared with WSL, then the checks by hand.
3. Note the Lua heap for a 50-page document.

**Step 2 results so far (12 Nov, on the G1 only: Peter needed the G2):**
- **Build:** `wordgrinder.app` is 780 KB, from WordGrinder 0.8 and zlib 1.3.1.
- **Patch 0001** (`dpy.c`, all under `#if defined PDCURSES`):
  - the key names for PDCurses's codes and modifiers;
  - italics from the screen, not from terminfo;
  - `wtimeout()` in place of `timeout()`, which the argument of that name hides;
  - Latin-1 characters taken as text, not as keys: newlib's C locale prints only ASCII;
  - Alt with a letter or digit given as Esc and then the key, as a terminal sends it.
- **Patch 0002** (WordGrinder's own bug, in 0.8 and in master): the exporter closed an underline twice when an
  underlined word was followed by a plain one.
  - The HTML had `</u></u>`, and the ODT had a `</text:span>` too many, so its XML did not parse.
  - `tests/import-from-opendocument.lua` expected the bug's extra `}` in its LaTeX; the patch corrects that too.
  - Not yet reported upstream: Peter's say first.
- **`main` renamed in WordGrinder's `main.c` only** (`out/wg_main.c` includes it). A global `-Dmain=wg_main` reached
  the SDK's `crt0.c`, which build.py builds with the same flags, and `cmm2_main.c` never ran.
- **SDK fixes it needed:**
  - `getcwd(NULL, 0)` allocates, as glibc's does: the file browser failed without a folder's name.
  - `readdir` gives `.` as well as `..`, as Linux does.
- **Back end fixes it showed:**
  - Console output goes to the serial port only while curses has the screen (`con_screen(0)`). A Lua error's report
    on stderr had been drawn over the screen, and scrolled it.
  - The fonts gained ▲ ▼ ▷ ◇ ▔ where a size lacked them, and the 6x12 its dashes and quotes.
  - Ctrl with Space gives NUL, as a terminal sends it (WordGrinder's "toggle mark", `^@`).
- **WordGrinder's 65 tests, run on the G1 as its build runs them** (`--lua tests/NAME.lua`, self-checking): 63 pass.
  - `windows-installdir` wants `C:\` with a backslash. The CMM2's paths are `A:/`, so this is a difference of
    platform.
  - `import-from-markdown`: "not enough memory". The G1 lends 3.76 MB (with OPTION COMPILE ON), and WordGrinder's
    Lua already uses 1.29 MB once its scripts are loaded. The Markdown importer (lunamark on LPeg) needs more, and
    Lua 5.1, unlike 5.2 on, does not collect garbage before it gives up. To run on the G2 (about 7 MB).
- **By hand on the G1:**
  - typing, é £ ï from the keyboard's codes, the menus, the file browser (folder, `..`, the hidden `.wordgrinder`);
  - Save As, `^S`, bold, underline and italic;
  - export to ODT, HTML and Markdown, the files checked on the PC;
  - WordGrinder's 14-page manual (`README-v0.7.2.wg`) opened from the RUN line, paged, and shown in MODE 15
    (160 x 64).
- **From Peter's checks (12 Nov):**
  - **Shift-selections did not show.** The selection was made but not drawn. PDCursesMod's `fb/pdcdisp.c` swapped the
    colours for A_REVERSE before it turned the default colours (-1) into colours, so -1 swapped with -1 and reverse
    video did nothing. WordGrinder never calls `start_color()`, so its selections, menus and file browser showed no
    highlight; testcurs sets its colours, which hid the bug. Fixed in `apps/pdcurses/patches/0001` (an upstream
    candidate).
  - **Ctrl+I did not italicise.** The keyboard gives 9 for Ctrl+I and for Tab, as a terminal does, and the back end
    made 9 with Ctrl held CTL_TAB. Tab, Enter and Backspace are now told from Ctrl+I, M and H by the key itself
    (keys_raw's HID usage, version 9).
  - **WordGrinder ended with code 1 while editing `test.wg` (14 pages) on the G1.** Lua ran out of memory, and its
    panic message went to the serial console only.
    - Reproduced: the heap in use climbed from 1.44 to 2.98 MB in about 20 rounds of marking, styling and moving.
    - It was garbage, not a leak. Lua's GC pause of 200 lets the heap reach twice its use, and Lua 5.1 does not
      collect when an allocation fails.
    - Patch 0003 sets the pause to 110 (`-DWG_GC_PAUSE=110`). The same 60 rounds then finish with the heap under
      2 MB.
    - The Markdown import still needs more than a G1 has.
  - **Crash reports are now shown.** What a program writes while curses has the screen (to the serial console only)
    is kept, its last kilobyte. When the program ends having written some, the screen shows it with "Press a key for
    MMBasic", before the loader clears it.
- **Typing lost letters on the G1, fixed.** The USB keyboard is seen to only when the program calls `poll()`, and a
  key pressed and let go while WordGrinder's Lua was busy (laying out a long document) was never seen. Serial input
  arrives by interrupt, so the tests over the serial console could not show it. The back end's `PDC_cmm2_poll()`
  calls `poll()` every 4 ms at most, and patch 0003 calls it from a Lua count hook, every 5000 instructions. Peter:
  "Typing OK now".
- **Enter "steps two lines"**: as designed. WordGrinder puts a blank line between paragraphs. CR LF from the
  keyboard gives one new paragraph (checked: `P: 2/2`). Look and Feel's dense paragraphs drops the blank line.
- **The mouse: none in 0.8**, which never asks curses for mouse events. WordGrinder 0.9 (Luau) has it: a click puts
  the cursor there, a drag selects, and menus and dialogs take clicks (its `navigate.lua`'s `Cmd.GotoXYPosition`,
  which 0.8 lacks, and a few lines in `main.lua`, `menu.lua`, `forms.lua` and the curses front end). A backport to
  0.8 is possible later; Peter, 12 Nov: "That's fine - just asking".
- **Peter's checks (12 Nov):** Shift and Ctrl+Shift selection, Ctrl+I, Alt+F, Alt+= and Alt+Minus (redrawn at the new
  size, and back in it next time), cut, copy and paste, and saving to C: by typing `c:`: all OK.
  - A full name such as `c:/test.wg` was saved under the current folder (`A:/wordgrinder/c:/test.wg.new`).
    WordGrinder's browser took only `/` or `\` as a whole path, and on Windows a drive letter only when typed alone.
    Fixed in patch 0004, which also fixes the overwrite branch: an upstream bug.
  - Autosave (Document settings, not Global settings): a copy `NAME.autosave.DATE.TIME.wg` beside the document,
    checked on A: and on C:. It must be turned on (`> NO` to YES with Space), and the document saved under a name
    first.
- **The final build on the G1:** WordGrinder's tests 63 of 65, the same two as before.
- **Look and Feel on from the first start (Peter, 12 Nov: yes).** `cmm2_main.c` writes `.wordgrinder/settings.dat`
  when there is none, in WordGrinder's own form: `lookandfeel.enabled` and `denseparagraphs` true. The add-ons fill in
  the rest of their settings (`MergeTables`), and a user's saved settings are never touched. Checked in a new folder,
  `A:/wgtest`: the text is centred, 80 columns wide, with dense paragraphs.
- **Spelling: the system dictionary cannot be loaded on a CMM2.** WordGrinder reads the whole word list into a Lua
  table, keyed in lower case. The British list it ships (`extras/british.dictionary`, 171,840 words, 1.7 MB) would
  take about 14 MB of Lua memory (2^18 table slots and the strings), and a G2 lends 7 MB. The automated test passed
  only because it sets a two-word dictionary itself. The user dictionary (words added with `^M`) works. Open: Peter's
  choice of a short list, a lookup on the card, or none.
- **ODT in Word (Peter, 12 Nov): "perfect".** Both files were exported on the G1 and copied to the PC: a short
  document with underlined words followed by plain ones (patch 0002's case), bold, italic and café £5 naïve, and
  WordGrinder's 14-page manual with its headings and lists. Their XML parses.
- **Still to do:**
  - the G2;
  - the Markdown import's memory;
  - whether to turn Look and Feel's 80 columns on by default.

### Step 3: sc-im
1. `make.py`, with the grammar generated in WSL, and `cmm2_main.c`.
2. The tests compared with WSL, then the checks by hand.
3. Check that each shell feature fails cleanly.

### Step 4: documents and release
1. A chapter for each app in the App User Guide: starting it, `mode=` and `font=`, the keys, files to and from a PC,
   what it lacks (printing, shell commands, charts).
2. The licences in the zip:
   - PDCursesMod's public domain notice;
   - WordGrinder's MIT and its bundled components' licences;
   - Spleen's and zlib's;
   - sc-im's BSD-4. Its advertising clause needs the line "This product includes software developed by Andrés
     Martinelli" in the guide.
3. A line in the release notes. The apps go into a release zip when Peter says.
4. Upstream, asking Peter first:
   - the PDCurses key-name patch to WordGrinder, since it helps any PDCurses build;
   - fixes to PDCursesMod's `fb/` code found on the way. The CMM2 back end itself stays here.

## Possible add-on: proportional text in WordGrinder

*Agreed with Peter on 12 Nov as a possible add-on: not part of steps 1-4, and decided after step 2.*

**WordGrinder is fixed-width throughout:**
- curses is a grid of character cells.
- WordGrinder's C code measures text in cells (`emu_wcwidth` in `screen.c` and `word.c`).
- Its Lua code wraps, moves the cursor, sets margins and draws its menus and forms in columns (`document.lua`,
  `navigate.lua`, `redraw.lua`, `margin.lua`, `forms.lua`, `menu.lua`).
- Its own OpenGL front end (0.9) draws the same grid, with stb_truetype and a monospaced font (Fantasque Sans Mono).

Editing in proportional text would mean redoing all of that in pixels, without curses. That is a new word processor,
not a port.

WordGrinder's documents are styles, not layout: the proportional layout happens when a document is exported (ODT,
HTML) and opened on a PC.

Two additions keep the core as it is:
1. **Real bold and italic faces** in place of the synthesised ones. A monospaced font with all four faces (Fantasque
   Sans Mono, OFL, as WordGrinder 0.9 ships it) is rasterised by `pdcurses.py` at each size. The back end picks the
   face by attribute. Still a grid.
2. **A page preview** as a WordGrinder addon (it has 17 in `src/lua/addons`, loaded the same way). The preview shows
   the document as pages in proportional fonts, read-only, as WordPerfect 5.1's print preview did; the editing stays
   on the grid.
   - It needs a small C module bound into Lua: draw a run of text in a font at a pixel position, and give its width.
   - It needs a second display page to draw on.
   - The fonts are free TTFs (DejaVu or Liberation), rasterised at build time or with stb_truetype at run time.
   - Anti-aliased text would need a 16-bit mode (not MODE 15 or 16) or palette entries set aside for greys.
   - Rough effort: three to five days. Only if Peter wants it, after step 2.

## Risks

| Risk | Effect | What we do |
|---|---|---|
| The French layout's £, ² and µ share codes with Shift and Right, F2 and F5; modifiers are read when a key is taken, not when it is pressed | Typed ahead, a key loses its modifier, or Shift with Right becomes £ | `keys_held` at first; `con_getkey` (version 19) makes it exact (see "System services"); test with the French layout |
| The full newlib has not run on the loader before | A fault at start-up, or a larger `.app` | `hello.app` and `pdcdemo.app` with `--libc full` on both boards first (step 1) |
| Lua 5.1 is slow on a long document (WordGrinder rewraps a paragraph on each key) | Keys lag | Measure once (the speed rule); 480 MHz should be ample for ordinary documents |
| A G1 runs out of memory (about 4.5 MB without `--vram`) | A large document will not load | The guide states the size measured in step 2. If it is too small, a second G1 build with `--vram` (about 7 MB, but no modes above 960x540) |
| A program does not handle `KEY_RESIZE` | A font or mode change while it runs leaves a broken screen | Check both in step 1; if one fails, fix the change for that program at start-up only |
| A shell feature hangs instead of failing | sc-im stops | Check each one in step 3; `popen` returns NULL through the `fork` stub |
| Paths with a drive letter | The file browser goes wrong | `ARCH "windows"` for the Lua code; try `A:` and `B:` |
| Spleen lacks a character | A replacement glyph | Accepted: Latin-1 and Latin Extended-A are complete |
| sc-im's BSD-4 advertising clause | A credit line must appear | The line in the guide and the zip |

## Rough effort

- Step 1: two days.
- Steps 2 and 3: two days each.
- Step 4: one day.
- sc-im's later items: a day or two each.
