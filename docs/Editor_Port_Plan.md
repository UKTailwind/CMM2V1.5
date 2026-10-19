# Editor: the PicoMite's Editor.c in place of kilo.c

The editor item of P4 in `PicoMite_Parity_Audit_and_Plan.md` (decision 8). Written 17 Oct 2026, from two read-throughs:
- `Src/kilo.c` at `f382824`;
- the PicoMite's `misc/Editor.c`. Only the editor parts were read: P 1-835 and P 5682-9303. P 837-5680 is the file manager.

Line numbers are the CMM2's unless marked P. Nothing has been built.

**Status, 17 Oct:**
- D1-D5 decided by Peter (below).
- Next: E0.
- E8 (help from help.txt) waits for the HELP command from help.txt, which is a separate P4 item.

## Review, 19 Oct

Checked against the CMM2 tree after the FM port (`Src/FileManager.c`), and against the PicoMite's `misc/Editor.c`.
That file has not changed since 13 Oct (`c271835`), so every P line number below still holds.

**1. Flash: the engine does not fit beside help.h.**

The 1 MB code region (bank 1) ends at 0x080FA1E8, which leaves 23.5 KB free. The measured sizes:

| What | Flash |
|---|---|
| kilo.c, at -Oz | 15.7 KB |
| FileManager.c, at -Oz (4,500 lines of the same file's style) | 24.8 KB |
| help.h: 1,171 strings and their pointers | about 35 KB |

By FileManager.c's measure, the engine's 4,500 lines come to 20-25 KB at -Oz. Without kilo, that is 5-10 KB more than now; E2-E7 add more. Bank 2 is already used: the program, the library, the image slots and the options.

Retiring help.h (E8) frees about 35 KB, more than all the space now free.
- E8 no longer waits for anything: HELP already reads A:/help.txt (`Commands.c:242-286`).
- help.h is now only HELP's fallback when the card has no help.txt, and kilo's F12/F11 (`kilo.c:1703`).
- Proposed: E8 before E1. F12/F11 then use help.txt from the start (E5's help part becomes E8's), and HELP without help.txt says so. This needs Peter's agreement, because a card without help.txt then has no help at all.

**Done, 19 Oct (Peter): bank 2 sector 0 is firmware.** This is the first step of the parity plan's 15 Oct budget, which allows firmware up to bank 2 sectors 0-1.

The program has always started at sector 1 (`FLASH_PROGRAM_ADDR`, and OPTION FLASH 1-6), so nothing moves and OPTION FLASH keeps its range.

`STM32H743II_FLASH.ld` has a `FLASHB2` region with a `.bank2` section, filled file by file:
- FileManager.c and kilo.c now;
- the engine's Editor.c, and help text if it stays, later.

It is not an overflow of `.text`. Writing the bank (a program, the library, the options) stalls any fetch from it, so interrupt code, routinechecks() code and their tables must stay in bank 1.

The image is still one block: the .bin ends in sector 0 and never reaches the program's sector.

The guards:
- `FlashWriteInit` refuses addresses below `FLASH_FIRMWARE_END` (bank 2 sector 1).
- At boot, an `Option.ProgramStartCode` outside 1-6 (saved by another firmware) becomes 1.

After the move:

| Region | Used | Free |
|---|---|---|
| Bank 1 | to 0x080F0648 | 62.4 KB |
| Bank 2 sector 0 | FM and kilo, 39.3 KB | 88.7 KB |

So E1 fits without E8, and E8 can be done on its own merits.

**2. The file manager now drives the editor.**

FM (`Src/FileManager.c`, the PicoMite's) opens EDIT for its E/F4 and N keys. It does this through `cmd_newedit` with the file name in `cmdline` (FileManager.c, its `edit_from_fm` block), using two hooks (`Inc/FileManager.h`):
- `fm_pending_edit_seek_valid/_line/_char`: the editor opens at an error's line. In kilo, this is in `cmd_newedit`, after it clears `StartEditLine`.
- `fm_editing`: 1 while FM's EDIT is open. On F2, kilo saves the file, sets it to 2 and returns, instead of running the program. FM then runs the program through the prompt and comes back after it.

The engine has the same two hooks under `MMBASIC_FM`:
- the seek, at P 764-788;
- `fm_edit_wants_run`, at P 7219.

The engine's return to the caller is its EDIT FILE branch (`cmdfile == false`, P 7216-7223), so D4 is refined as follows:
- The branch's `SaveContext` and `RestoreContext` stay out. FM has cleared the heap itself.
- Its return to the caller stays. On the CMM2 the trigger is `fm_editing != 0`, not `cmdfile`.
- F2 sets `fm_editing = 2`. The PicoMite's ^W also runs the file; that is not kept, because ^W is kilo's "Backup as" (D1).

E1's gate gains FM: E then Esc, E then F2 (the program runs and FM comes back), N (a new file), and E after an error (the editor opens at the line).

**3. Who uses kilo.c's exports now** (the table under Source layout, updated; fm.c has gone):

| Symbol (kilo.c) | Used by |
|---|---|
| `fstrstr` | `MMBasic.c` |
| `strcasechr`, `bstrstr` | kilo.c only |
| `setterminal` | `FileManager.c`, `MM_Misc.c`, `main.c` |
| `MX470Display`, `MX470Cursor`, `MX470PutS` | `FileManager.c`, `MMBasic.c`, `MM_Misc.c`, `main.c`, `Hardware_Includes.h` |
| `parseanddisplay`, `abAppend` | kilo.c only |
| `StartEditLine`, `StartEditCharacter` | `MMBasic.c`, `MM_Misc.c` |
| `BreakKeySave` | `XModem.c`, `ZModem.c` |
| `cmd_newedit` | `allcommands.h:504` (EDIT), `FileManager.c` |

FM's `fm_display_setup` (FileManager.c) is a copy of kilo's entry sequence (1913-1951), and so is E1's. Under E1, one function in the new `Console.c` serves both.

**4. The mouse (D2).** A USB mouse is `mousestruct[2]` (`USBMouse.c`, 17 Oct), so `mousestruct[Option.Mouse]` already covers it. The open question is still whether the editor should prefer a USB mouse. FM has no mouse support: the PicoMite's has none, and the old fm.c read `Option.Mouse`. Both can be settled together.

**5. The test driver (E0).** `cmm2serial.py` is a minimal reader. FM's test driver (fmdrive.py, scratchpad, 19 Oct) already has the pieces E0 needs:
- a small VT100 screen;
- the size from the terminal-resize sequence;
- highlight and colour attributes per cell;
- keys as a terminal sends them.

It becomes `tools/vt100drive.py`, and `tools/edit_test.py` is built on it. Script 17 (colour coding) reads the colours from that screen.

**6. Smaller corrections.**
- EDIT's table entry is `allcommands.h:504`, not 495.
- `error()` records the main file's line in `MMBasic.c`'s `error()`, which also hands it to `FmNoteError`.
- The kilo line numbers are still those of `f382824`. kilo.c has gained the FM hooks since (about 12 lines, in F2 and in `cmd_newedit`).

**Proposed order:** E0 → E1 (with FM's gate; Editor.c in `.bank2`) → E2-E6 → S → E7 → E8 → E9. E8 can come earlier if Peter wants help.h's 35 KB back, or F12 on help.txt sooner.

## The approach

The PicoMite editor becomes the engine. The CMM2 keeps kilo's keys and behaviour (D1).

**What the engine brings that kilo lacks:**
- lines of any length, with the 255-character check on saving a .bas file;
- colour coding taken from the interpreter's own tables, including the tokeniser's folds and `/* */` block comments, which closes the 7.4 deferred item;
- Beautify;
- auto-indent;
- selection in both directions;
- the Tera Term mouse report.

**What kilo has that the engine lacks**, which E2-E6 add:
- the CMM2 key map;
- an unlimited clipboard;
- interactive search, both ways;
- a save that checks the card first, and "Backup as";
- help on F12 and F11.

The upgrades go into the CMM2 copy first. The PicoMite takes them later, gated per build where flash or RAM is short (D3).

**Until the switch-over gate** (S, below):
- `kilo.c` stays the EDIT command;
- the new editor is built behind a CMake switch (`EDITOR_ENGINE`, off by default), so a development build never loses a kilo feature.

## Decisions (Peter, 17 Oct)

**D1, the key map: keep the CMM2's.**
- Every key kilo uses keeps its kilo meaning.
- The engine's features that kilo has no key for go on keys kilo leaves free. Beautify goes on ^A, with no confirmation.
- The PicoMite's WordStar keys are not used (^E ^X ^S ^D, ^U ^K, ^P ^L, ^N, ^]). Several of them clash with kilo, and the CMM2 always has arrow keys.

The full map is below.

**D2, the mouse.** The PicoMite's buttons, with kilo's wheel and drag-select added:

| | New editor | kilo (`editorReadKey`, 356-483) |
|---|---|---|
| Left click | Place the cursor; in select mode, extend the selection | Place the cursor |
| Left, held for 0.5 s and dragged | Select (kilo) | Select |
| Right | Start selecting; in select mode, cut | Home or End, at the left or right edge only |
| Middle | Paste; in select mode, copy | Page up or down, by screen half; in select mode, cancel |
| Wheel | Line up and down (kilo) | Line up and down |

The CMM2 reads `mousestruct[Option.Mouse]` until the parity plan settles whether the editor prefers a USB mouse.

**D3, where the upgrades land: the CMM2 first.**
- The PicoMite takes them later, behind build conditions where flash or RAM is short (see Later: the PicoMite).
- The CMM2 copy is written so that this is easy:
  - the platform differences sit behind macros at the top of the file;
  - the keys are `KEY_*` names defined per platform in `Editor.h`;
  - each new action is its own function.

**D4, EDIT FILE: not on the CMM2 for now.** Its point is saving and restoring the variable space when EDIT is used inside a program. The PicoMite's `EDIT FILE` branch (`cmdfile == false`: `SaveContext`/`RestoreContext`, P 498-524, 7216-7223) is left out.

**D5, the edit buffer: 1 MB** from the SDRAM heap (estimate).
- That is larger than any source whose program fits the 512 KB image, and leaves the G1's 5 MB heap room for the clipboard.
- A larger file is refused on entry, as now (P 736).

## The CMM2 key map (D1)

**Edit mode:**

| Key | Action | From |
|---|---|---|
| Esc | Quit; with unsaved changes, press it three times (1844-1860) | kilo |
| F1 | Save, load the program, exit | kilo |
| F2 | Save, load, run | kilo |
| F3, ^F | Search: as you type; Down/Right next, Up/Left previous; Enter stays on the match, Esc goes back; F5/^V replaces the match with the paste buffer (1166-1289) | kilo |
| F4, ^S | Select (1648-1659); a second start with an unpasted buffer warns once (879-883) | kilo |
| Shift-Down, Shift-Right | Select and move (1828-1836) | kilo |
| F5, ^V | Paste | kilo |
| F6 | Save without loading, exit (1622) | kilo |
| F7 | Insert a file (900-944) | kilo |
| F8, ^B | Save the paste buffer to a file (884-899) | kilo |
| F11 | Insert the syntax line of the last help topic (1663) | kilo |
| F12 | Help on the word under the cursor (1670-1707) | kilo |
| ^W | Backup as: write the buffer under another name and keep editing; typing the file's own name saves it in place (863-878) | kilo |
| ^K | Delete to the end of the line and go to the next line; at column 1, delete the line (1713-1720) | kilo |
| Shift-Tab | Delete up to one tab stop of spaces at the cursor (1767-1776) | kilo |
| Shift-Del | At column 1, delete the leading spaces; otherwise delete the next character (1777-1790) | kilo |
| Home, End | Start or end of line; twice, top or end of file (1721-1740) | kilo |
| PgUp, PgDn | First to the top or bottom row of the screen, then a page (1801-1827) | kilo |
| Ins, Tab, Del, Backspace, arrows | As kilo | kilo |
| Enter | New line, indented to match the line above | engine |
| Backspace inside the indent | Back to the previous tab stop (P 6763-6785) | engine |
| ^A | Beautify the whole file; no confirmation | engine |
| ^G | Find the last search text again, forwards (E7) | new |
| F9, F10 | Free | |

Enter and Backspace keep the engine's behaviour. These are the only two kilo keys whose behaviour changes, and they can go back to kilo's if Peter prefers.

**Select mode:**

| Key | Action | From |
|---|---|---|
| Arrows, Home, End | Extend, now in either direction from the start | kilo keys, engine range |
| PgUp, PgDn | Extend by a page | engine |
| ^X, F4 | Cut | kilo |
| ^C, F5 | Copy | kilo |
| Del | Delete | kilo |
| F8, ^B | Save the selected text to a file (P 8082-8131) | engine, on kilo's key |
| Esc | Cancel | kilo |

**Inside search:** ^R replaces the match with typed text. It asks the first time and reuses the text after that (E7).

**The screen.** Kilo's layout is kept:
- the status bar: file name, line count, "(modified)", paste-buffer size, INS/OVR, file type, column/row/total (1397-1426);
- the message line, with the prompt sized to the width (149-151, 166, 1428-1436);
- control characters shown in reverse as ^letter (1362-1371);
- sideways scrolling that moves the whole screen, with Up and Down keeping the column (1323-1331).

The engine scrolls only the cursor's line and snaps back to column 1 on Up/Down (P 6642-6647, 6678-6683), so that changes (E2).

**What a CMM2 user will see that is new:**
- no 240-character limit while typing. On saving a .bas file, a line over 255 characters is reported and the editor stays open.
- auto-indent and tabbed backspace;
- Beautify on ^A;
- colour on every keyword and on `/* */` blocks;
- selection in both directions and by page;
- the right and middle mouse buttons (D2);
- clicking in Tera Term moves the cursor;
- a save that never loses the edits (E3);
- later, typed-text replace and ^G.

## E1: the engine port

### Source layout

- **The new files.** `Src/Editor.c` and `Inc/Editor.h`, taken from P `misc/Editor.c` and `misc/Editor.h`. The file manager stays out behind `MMBASIC_FM`, as the PicoMite already gates it, until the FM port (P4).
- **Behind `EDITOR_ENGINE`.** The new `cmd_edit` replaces `cmd_newedit` at `allcommands.h:495`.
- **What kilo.c provides to other files.** Move these first, to `Src/MiscSTM32.c` or a small `Src/Console.c`, so that kilo.c can leave the build at S without touching anything else:

| Symbol (kilo.c) | Used by |
|---|---|
| `fstrstr`, `strcasechr`, `bstrstr` (109, 1143, 1155) | `MMBasic.c` (the folds), `fm.c` |
| `setterminal` (1876) | `MM_Misc.c`, `fm.c`, `main.c` |
| `MX470Display`, `MX470Cursor`, `MX470PutS` (61-107) | `MMBasic.c`, `MM_Misc.c`, `fm.c`, `main.c`, `Hardware_Includes.h` |
| `parseanddisplay` (293), `abAppend` (1294) | `fm.c` |
| `StartEditLine`, `StartEditCharacter` (138) | `MMBasic.c:4193`, `MM_Misc.c:3765` |
| `BreakKeySave` (140) | `XModem.c`, `ZModem.c`, `fm.c` |

### The platform layer

**Output.**
- On the CMM2, `Option.DISPLAY_CONSOLE` becomes `OptionConsole & 2`, and the serial writes are gated by `OptionConsole & 1`. They go through `SerUSBPutS`/`SerUSBPutC`, as kilo does (1457-1464).
- `HRes`/`VRes`/`VResEdit` become `PageTable[WritePage].xmax/.ymax` (85-86). The on-screen-keyboard reserve goes.
- Scrolling uses `ScrollLCD(n, 1)` (62).
- `tud_cdc_write_flush` goes.

**Dropped from the engine.**
- All the tile code: `DisplayPutClever`, the tile branches of `MX470Display`, and the tile mouse cursor.
- The VGA and HDMI mode and font rules (P 525-605).
- `PICOMITEWEB`, touch, the on-screen keyboard, `Option.Refresh`, `mapreset`.
- The program-memory path (`llist`, `SaveToProgMemory`, `StartEditPoint`).
- The `EDIT FILE` branch (D4).
- `Option.continuation`, which the CMM2 does not have; define it as 0.
- The `SHIFT_F3/F4/F5/F8` cases, which have no CMM2 key codes.
- The WordStar control keys (D1).

**Input.** `MMgetchar` (`Hardware_Includes.h:89`) and `routinechecks` (`:165`) take an argument on the CMM2. Adapt the four calls.

**Entry.** This is kilo's sequence (1913-1951, 1968-2039):
- `setmode(Option.mode, ...)` and the edit font `((Option.editfont-1) << 4) | 1`.
- `reset_CLUT`, and the REVERSE CLUT (1922-1928).
- Close the serial ports, I2C and the mouse.
- `ClearVars`, `closeallsprites`, `closeall3d`, `hidecursor`, `SymBindFree`.
- `CloseAllFiles` and `InitHeap`, unless audio is playing (1948). With audio playing, the 1 MB buffer must fit in what is free, or EDIT says so.
- `EDIT` with no argument opens `ProgName()` at `StartEditLine`/`StartEditCharacter`. These are set by `error()` for the main file (`MMBasic.c:4192-4195`) and by the last F1. `EDIT f$` opens at the top.
- A missing file is created and killed, to prove its path (1971-1993).
- `GUI CURSOR ON` and `CONTROLLER MOUSE OPEN` when `Option.Mouse >= 0`.
- `setterminal()` without arguments.

**Exit.**
- **F1, F2 and F6** do what kilo does (829-862, 1581-1625). F1 and F2 call `FileLoadProgram(fname, 0)` for a .bas file, and F2 then runs kilo's run sequence (1595-1617).
- **Every exit** runs one cleanup:
  - `CONTROLLER MOUSE CLOSE`;
  - `GUI CURSOR OFF`;
  - `SetFont(Option.DefaultFont)`;
  - `setterminal()`;
  - `setmode`;
  - `reset_CLUT`;
  - the autowrap and mouse-report resets (P 7107-7108).

  The error path can call the same function.
- In E1 the saves use the engine's writer. E3 replaces it.

**Colour coding.**
- `SetColour` (P 8530-8832) needs `commandtbl`/`tokentbl`/`CommandTableSize`/`TokenTableSize` (`MMBasic.h:358`), `overlaid_functions`/`MMEND` (`Functions.c:57`, `allcommands.h:65`) and `isnamechar` (`MMBasic.h:309`). All are present.
- `hiddenfunctions` (P 8566-8568) is replaced by the tokeniser's own `FoldFrom[]` (`MMBasic.c:1759-1774`, exported). That way BIN$, LEFT$, MAX, BIT and the rest stay coloured as the folds change.
- `twokeywordtbl` takes kilo's OPTION words (264-266), in place of the PicoMite's GUI-control words.
- `Option.ColourCode` is `Option.colourmode == 1`.
- REVERSE (`colourmode == -1`) stays as kilo has it: black on white through the CLUT, with no colour coding (624).

**The mouse (D2).** The PicoMite blocks (P 6369-6517, 7654-7781) are replaced by one reader of `mousestruct[Option.Mouse]`, from kilo 356-483:
- pixels to character cells;
- the GUI cursor;
- the wheel;
- the 0.5 s drag-select;
- the PicoMite's right and middle buttons.

**The stack.** `FullScreenEditor`'s 2 KB of arrays (P 6331) move to the heap, because the CMM2's DTCM holds heap and stack in 18 KB.

**Gate E1:** with `EDITOR_ENGINE` on, and keys as far as E1 has them:
- scripts 1-3, 7, 15-18;
- consoles: serial only, screen only, both;
- the four edit fonts;
- COLOURCODE ON, OFF and REVERSE;
- `MM.INFO(HEAP)` the same before and after an edit, with and without audio playing;
- the goldens run unchanged.

## E2-E6: kilo parity

**E2, the key map and kilo's smaller functions.** The `KEY_*` map above, plus the kilo functions the engine lacks:
- ^K;
- Shift-Tab and Shift-Del;
- Shift-Down and Shift-Right;
- ^X and ^C in select mode;
- Esc three times;
- two-stage PgUp/PgDn;
- the unpasted-buffer warning;
- kilo's status bar and message line, with timed messages (`showdefault`);
- control characters as ^letter;
- whole-screen sideways scrolling.

The engine already draws the selection with `edx` on every line (P 8231-8431). Edit mode gains the same: keep `edx` and the absolute column across Up/Down, and redraw when `edx` changes.

**Gate:** scripts 4, 5, 8, 12, 14, 19.

**E3, saving.** The edits are written while the editor is still running, through a writer that returns an error code instead of throwing (`f_open`/`f_write`/`f_close`). The engine's `BasicFileOpen` throws on a FatFs error, and its return value is not checked (P 7192).
1. Check the drive: `MDD_SDSPI_CardDetectState` for the SD card (as kilo 1583), and C: when it holds a USB drive.
2. Write `name.$$$` in the same directory.
3. Rename the old file to `.bak`, deleting any previous `.bak`, with `f_rename` as kilo does (841-845).
4. Rename `name.$$$` to `name`.

On any failure, the editor shows "Save failed: reason" in the message line and stays open with the buffer intact. The screen is torn down only after a successful write.
- F1, F2, F6, ^W (Backup as), F8/^B (save the paste buffer) and select-mode F8 all go through it.
- **Gate:**
  - script 11;
  - card removed, bad drive, path removed: each leaves the buffer intact;
  - then the card back in and F1 succeeds.

**E4, the clipboard.**
- The clipboard becomes a heap buffer that grows as needed (`ReAllocMemory`, `Memory.h:76`), as kilo's does (964-987). The engine's 1 KB `MAXCLIP` and its check go (P 478, 8056).
- Paste becomes one bulk insert:
  - one `memmove` of the tail;
  - a copy;
  - newlines counted into `nbrlines`;
  - one redraw.
- Today paste feeds the type-ahead buffer one character at a time (P 7365-7376), which drops TAB characters (P 7582).
- The status bar shows the size, as kilo's does.
- **Gate:** script 9, with a 100 KB block.

**E5, search and help.**
- **Search** is kilo's interactive search, on the engine's buffer:
  - It ignores case (`mem_equal`) and wraps.
  - The match is shown in reverse.
  - A match past the right edge scrolls the screen. The engine's F3 sets `edy` but not `edx` (P 7297-7301).
  - F5/^V replaces the match with the paste buffer.
- **Help.** F12 and F11 call the CMM2's existing `do_help` (`Commands.c:120`) as kilo does: the top rows are saved with `PAGE COPY 0 TO 1` and restored, and the serial terminal is saved and restored with `ESC[s`/`ESC[u` (1670-1707). E8 moves it to help.txt.
- **Gate:** scripts 10 and 13.

**E6, speed on large files.** The engine finds a line by counting newlines from the start of the buffer:
- `printLine` does this twice, for the screen and the terminal (P 8878, 8905);
- `printScreen` calls it for every row (P 8982-8997);
- Enter redraws the screen (P 6634);
- `PositionCursor` (P 7617) and `editInsertChar` (P 9018) each scan the buffer on every keystroke.

kilo keeps an array of rows, so it does none of this. Near the end of a 300 KB file, the engine's full redraw reads about 2 x 50 x 300 KB: roughly 0.1-0.3 s at 480 MHz (estimate).

**The fix:**
- Keep a pointer to the top line on screen, with its comment state (`inmulti`), and walk forward from it.
- Keep the cursor's line number and line start beside `txtp`.
- Keep a pointer to the end of the buffer.

**Gate:** times before and after on a 300 KB file, at the end of the file:
- a keystroke;
- Enter;
- PgDn;
- a search.

The targets are a full redraw under 50 ms and a keystroke under 5 ms (estimates). At the least, no worse than kilo on the same file.

## S: the switch-over

**Gate:**
- every kilo baseline script (1-14) passes on the new editor;
- the engine scripts (15-19) pass;
- E6's timings are met;
- a day of normal use on the board.

**Then:**
- `EDITOR_ENGINE` becomes the default and the switch goes;
- `kilo.c` is deleted;
- the parity plan's P4 entry and 7.4 (block comments) are updated.

## After the switch-over

**E7, additions:**
- ^R in search replaces with typed text, asking the first time. It has its own buffer, so it never touches the paste buffer.
- ^G finds the last search text again.
- Tab and Shift-Tab in select mode indent or unindent every selected line.
- After an error in an `#INCLUDE` file, EDIT opens that file at the line. `error()` already has the file (`LineFileName`, `MMBasic.c:4184-4186`), but it records only the main file (4192).

**E8, help from help.txt.** This comes with the HELP command (P4).
- F12 and F11 use the same lookup over the same file (P `core/Commands.c:3111`, whose topics are lines starting with `~`).
- `help.h` and `do_help` retire.

**E9, documentation:**
- the manual's editor section, with the key map above;
- help.txt entries;
- release notes listing what a CMM2 user will see that is new.

| Step | What | Estimate |
|---|---|---|
| E0 | The edit-script driver; scripts 1-14 against kilo | 2-3 days |
| E1 | The engine port | 1 week |
| E2 | The key map and kilo's smaller functions | 1 week |
| E3 | Saving | 2-3 days |
| E4 | The clipboard | 2 days |
| E5 | Search and help | 2-3 days |
| E6 | Speed | 2-3 days |
| S | The switch-over | 1 day of use |
| E7 | Additions | 2-3 days |
| E8 | Help from help.txt | 1-2 days |
| E9 | Documentation | 1-2 days |

That is about five to six weeks. Keeping kilo's key map adds about a week to the parity plan's 2-4 weeks for P4.

## Tests

**The driver.** `tools/edit_test.py` is built on `cmm2serial.py`. For each script it:
1. sends the starting file with `goldens.py`'s transfer;
2. types `EDIT "t.bas"`;
3. sends the keys as the VT100 sequences a terminal sends;
4. exits;
5. reads the file back with `LIST "t.bas"`, or with ZMODEM for exact bytes;
6. compares it with the script's expected file.

Timing comes from the serial traffic.

**kilo baseline**, run against kilo in E0 and against the new editor before S. Where kilo's behaviour is a bug, the script says so.
1. Open, then F1 an unchanged file: identical bytes, CRLF endings, a `.bak`.
2. Typing, deleting and joining lines, then F1: the expected file. `RUN` afterwards runs the new program.
3. F2: the program's output appears.
4. F6: the file is saved, and `RUN` still runs the old program.
5. Esc on a changed file: one or two presses stay in the editor; three leave with the file unchanged.
6. A program with an error at line N, then `EDIT`, one character typed, F1: the character is on line N.
7. `EDIT "new.bas"` on a missing file creates it on F1. `EDIT "Z:/x/new.bas"` is refused on entry.
8. ^K at column 1 and mid-line; Shift-Tab; Shift-Del; Tab stops 2, 3, 4 and 8.
9. Select with F4 and with Shift-Down; ^X, ^C, F5, Del; a 100 KB block cut and pasted.
10. Search: as you type, forwards, backwards, wrap, case; Esc goes back, Enter stays; ^V replaces with the paste buffer.
11. ^W: a copy is written and editing goes on; the file's own name saves in place.
12. F7 inserts a file; F8 and ^B save the paste buffer.
13. F12 shows the topic for the word under the cursor; F11 inserts its syntax.
14. PgUp and PgDn in two stages; Home and End twice.

**The engine:**

15. A 300-character line saves in a `.txt`. In a `.bas` it is refused with a message, and the editor stays open.
16. ^A Beautify on a sample with IF, FOR, DO, SELECT and SUB, against its expected file.
17. Colour coding, from the captured VT100 colour codes: keywords, a folded function (LEFT$), strings, numbers, a `'` comment, REM, a `/* */` block.
18. Auto-indent and tabbed backspace.
19. Selecting backwards and by page; whole-screen sideways scrolling, with Up and Down keeping the column.

**By hand:**
- the local screen in several modes and edit fonts;
- the USB keyboard's F-keys, Shift-Tab, Shift-Del, Shift-arrows;
- the mouse, under D2;
- REVERSE;
- the save failures of E3.

## Later: the PicoMite

The upgrades go back into `misc/Editor.c` when Peter chooses, with build conditions where they cost flash or RAM (D3).

| Upgrade | Cost | Suggested condition |
|---|---|---|
| E6 line and cursor caches | Small code, a few words of RAM | All builds |
| E3 safe save | Small code | All builds (it fixes data loss) |
| E4 growing clipboard, bulk paste | Heap at run time | All builds, capped at a share of the free heap on the RP2040 |
| E5 interactive search, both ways | Moderate code | Not `PICOMITEMIN` |
| E5/E8 help in the editor | Code, and help.txt on A: | Builds that have HELP |
| E2 kilo-only actions (^K, Shift-Del...) | Small code each | All builds; the keys stay the PicoMite's own |
| E7 additions | Small code | Not `PICOMITEMIN` |

The PicoMite keeps its own key map. Its copy takes the actions, not the CMM2's `KEY_*` definitions.

## Risks

- **Flash.** The editor half of `Editor.c` is about 4,500 lines, against kilo's 2,071, and E2-E7 add to it: perhaps 20-35 KB more code (estimate). Measure at E1 against the current region use. Retiring `help.h` (E8) gives some back.
- **An error inside the editor.** `error()` jumps to the prompt with the edit mode, font and mouse still set, as kilo does today. E3 removes the file-I/O cases, and the cleanup function can be called from the error path.
- **Two copies drifting apart.** The CMM2 copy leads (D3). Platform macros, `KEY_*` names and action functions keep the back-port mechanical. Diff the two copies at each release.
- **Regressions before S.** kilo stays the EDIT command until every baseline script passes.

## Found while planning

These are on the PicoMite side. All but the last are fixed by the steps above:
- F3 does not scroll sideways to a match past the right edge (P 7297-7301; E5).
- Replace overwrites the clipboard (P 7345; E7's separate buffer).
- Paste drops TAB characters (P 7582; E4).
- The save path ignores `BasicFileOpen`'s return value (P 7192; E3).
- `EDIT FILE` at the PicoMite prompt, outside the file manager: F2 saves and exits but does not run, though the legend says "F2:Run" (P 7216-7223). This is for the PicoMite.
