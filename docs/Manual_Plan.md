# The Colour Maximite 2 user manual for the alpha release: plan

*20 Oct 2026 (Peter: "a user manual up to date ... as I would like to release an alpha version to the users ... then we can keep it updated as we move forward").*

The goal is a Word manual, `docs/manual/Colour_Maximite_2_User_Manual.docx`, that describes this build exactly: the firmware 7.00.00b1 (`Inc/Version.h`) as it is at the release. After that, each change to the firmware updates the manual in the same commit, and a check run with the tests catches what was missed.

## The sources

| Source | Version | Form | What it is |
|---|---|---|---|
| `D:\Dropbox\VSCode-STM32\Colour_Maximite_2_User_Manual.doc` | MMBasic 5.05.04, July 2020 | Word 97, 121 pages | The last CMM2 manual in Word form that we have. Converted to `.docx` by Word, with its styles intact. |
| `D:\Dropbox\VSCode-STM32\Colour_Maximite_2_User_Manual.pdf` | 6.00.00 Beta 14, Revision 4, 2 Oct 2026 | PDF only, 180 pages | The newest CMM2 manual: Gerry's b14, which this tree merged on 16 Oct (`docs/Gerry_b14_Review_and_Merge_Plan.md`). |
| `D:\Dropbox\PicoMite\PicoMite\docs\PicoMite_User_Manual.docx` | 7.0.00, 19 Oct 2026 | Word, 4,512 paragraphs | The PicoMite manual, kept up to date by Peter. |
| this tree | 7.00.00b1 | source | What the manual must match: the command, function and operator tables in `Inc/allcommands.h`, each handler's sub-keywords and arguments, OPTION, MM.INFO, and the keys of the editor and FM. |

The PicoMite manual comes with tools and rules:
- 1,185 reference entries, which `tools/gen_help.py` harvests;
- `docs/MANUAL_STYLE.md`, its closed list of applicability labels, with Appendix K defining them;
- Appendix L, where each keyword is documented;
- companion manuals for FM, TILEMAP, structures, SPRITE, BLIT, PLAY SAMPLE and others;
- `docs/V7.00.00_release_notes.md`.

All three manuals are Geoff Graham's template: A4, 18 mm margins, the same heading and body styles, and the reference sections as two-column tables (syntax on the left, description on the right). So text moves between them without restyling.

What this means:
- The CMM2's Word text is two generations old. About 60 pages of changes (5.05.04 to 6.00.00 b14) exist only in the PDF.
- Since P1 the interpreter is mostly the PicoMite's: tokens, symbols, structures, LIBRARY, MATH, the statement loop, FM and the editor. So the PicoMite manual already describes most of the language as this build has it.
- The b14 PDF is the best text for what only the CMM2 has: the hardware, MODE and pages, the colour depths, IMAGE, the CMM2's BLIT and SPRITE, CAN, ADC/DAC, SAM speech, the Wii controllers and the connector.

## Decisions

**M-D1. The base document.** Recommended: **the PicoMite manual**, with the CMM2's chapters and CMM2-only entries brought in from the b14 PDF, and the 2020 Word file used for their figures and tables.
- **For it:** the language reference is current, and its tools and rules work on it. It is also what the parity plan's P7 says: "regenerated from the PicoMite manual sources with a CMM2 chapter".
- **The alternative,** the 2020 CMM2 file as the base, means two catch-ups: to b14 from the PDF, then to V7 from the PicoMite manual. The reference sections would be rewritten twice.

**Decided 20 Oct (Peter): the PicoMite manual.**

**M-D2. The b14 manual's Word source.** If Gerry (or Geoff) had it, the CMM2-only entries would come across with their formatting, instead of being re-entered from the PDF. **Decided 20 Oct (Peter): use the PDF.** The text extracts cleanly; tables and formatting are rebuilt in the PicoMite manual's styles.

**M-D3. The title page and credits.** The manuals were written by Geoff Graham. The new one combines his CMM2 and PicoMite manuals with Gerry's b14 changes and this work. **Decided 20 Oct (Peter):**
- Geoff Graham stays on the title page, with a line "updated for MMBasic 7.00 by Peter Mather".
- About credits Gerry's b14 work.
- The version line reads like "MMBasic Ver 7.00.00 alpha 1".

**M-D4. The alpha's scope.**
- **For the alpha:** the reference sections complete and checked against the build, every chapter present, and the new parts written.
- **Afterwards:** any chapter not yet revised line by line says so in a note at its head, and is revised in the releases that follow.

**Decided 20 Oct (Peter): as above.**

## The audit

Each step's output goes in `docs/manual/audit/`, committed.

**M-A1. What the firmware has** (`tools/manual/inventory.py`):
- the commands, functions and operators in `Inc/allcommands.h` (about 160, 100 and 20);
- for each handler, its sub-keywords (the `checkstring` literals) and the arguments it reads;
- the OPTION keys, the MM.INFO keys and the MM. variables;
- the editor's and FM's keys.

Output: `firmware.json`.

**M-A2. What the manuals say.** Each manual's reference entries are harvested as name, syntax, description and source page:
- the PicoMite manual through `gen_help.py --json`;
- the b14 PDF by position: syntax at x ≈ 51 pt, description at x ≈ 210 pt, with the bookmark labels marking each keyword group (about 320: 167 commands, 95 functions, 46 options, 15 read-only variables);
- the 2020 file by its tables.

Output: `pico.json`, `cmm2_b14.json`, `cmm2_2020.json`.

**M-A3. The cross-reference,** `keywords.md`. There is one row per firmware keyword and sub-keyword, with where each manual documents it and a class:

| Class | Meaning | What the manual gets |
|---|---|---|
| P | The PicoMite entry describes it as built. | The PicoMite entry. |
| PD | The PicoMite entry, but the CMM2 differs: pins, modes, drives, limits, arguments. | The PicoMite entry, corrected. |
| C | CMM2-only, and the b14 entry describes it as built. | The b14 entry. |
| CD | CMM2-only, and the b14 entry is out of date. | The b14 entry, corrected. |
| U | In the build, in neither manual. | A new entry. |
| X | In a manual, not in the build: the PicoMite's WiFi, PIO, displays and Bluetooth; or what the CMM2 has lost, such as the hardware JPEG decoder and the old FILES browser. | Nothing; dropped. |

The class is confirmed against the handler, not taken from the manuals' agreement. The bulk of the work is here, and anything the source leaves unclear is tried on the board. Each row is ticked when its entry is written, so the table shows the progress.

**M-A4. The chapters,** `chapters.md`: each of the PicoMite manual's 41 chapters and the b14's 36, against the build. Each is kept, replaced by the CMM2's, merged, or dropped.

**M1 as built (20 Oct).** The tools are in `tools/manual/`:
- `inventory.py` finds:
  - 161 commands, 94 functions and 21 operators, with 1,063 sub-keywords;
  - the 19 MM. names the tokeniser folds, and its 12 folded functions;
  - its 10 aliases: `?` is PRINT, COLOR is COLOUR, SPRITE is BLIT, WII is CONTROLLER, CAT is INC, PIXEL FAST is CLS, ELSE IF, END IF, EXIT DO, MM.INFO$;
  - the loader's 6 directives (`#DEFINE`, `#INCLUDE`, `#COMMENT START/END`, `#MMDEBUG ON/OFF`).
- `harvest_docx.py` harvests 739 rows of the PicoMite manual and 391 of the 2020 file.
- `harvest_pdf.py` harvests 634 rows of the b14 PDF.
- `crossref.py` writes `keywords.md`.

What the cross-reference found, before the audit confirms it:
- **Keywords:** 253 are in both the PicoMite manual and a CMM2 manual, 16 only in the PicoMite's, and 40 only in the CMM2's. The 40 are the operators, which the PicoMite manual has no table for, and the CMM2-only commands: PAGE, IMAGE, CAN, DAC, CONTROLLER, NUNCHUK(, CLASSIC(, BAUDRATE(.
- **In neither manual:**
  - WHILE;
  - I2C3;
  - CTRLVAL( and MSGBOX(;
  - PRINT's `@(`;
  - MM.END;
  - the PIXEL FAST alias.
- **b14 entries the build lacks:**
  - CFUNCTION, LS and MM.ESC;
  - SETTICK FAST.

  OPTION F15, F16, F19 and F20 were listed here at first. They are in the build: their keywords come from an array, which the inventory now follows.

  Its GUI entry spells CURSOR "CURSOUR".
- **PicoMite entries the build lacks:** 113 name no keyword of the build. They are:
  - WEB, PIO, CAMERA, BLUETOOTH, LCD, KEYPAD, ONESHOT, RAY, FRAME, STEPPER, MANDELBROT, STAR and ASTRO, TILE, RAM and RTC;
  - nine MM. variables.

  92 more name a sub-keyword the build lacks, mostly options for the PicoMite's hardware. Some of these are parity work still to come (RAY, FRAME, ONESHOT, KEYPAD, the RAM slots). Their entries come back when they do.

`chapters.md` is the chapter table: the proposal below, chapter by chapter, with each one's source and action.

**M2 as built (20 Oct).** `tools/manual/build_skeleton.py` made `docs/manual/Colour_Maximite_2_User_Manual.docx` from the PicoMite manual. It has:
- the title page of M-D3;
- the 44 chapters of `chapters.md` in its order, with 19 PicoMite chapters dropped;
- the footer reading "Colour Maximite 2 User Manual".

The chapters to keep or merge keep the PicoMite text. Those to replace or write are their heading only.

Under each chapter heading, and under About, is a highlighted note: `[MANUAL TODO] To be written / merged / checked / audited: ...`. It says what M3 or M4 does there, and the alpha has none left.

`tools/manual/word.ps1` has Word update the table of contents and fields, save, and export the PDF: 263 pages for now. The script was run once. From here the manual is edited in place.

**M3, the chapters to replace (20 Oct).** Each chapter's text is in `docs/manual/text/NN_name.md`, a small Markdown. `tools/manual/chapter.py` writes it into the manual under the chapter's heading, in the template's own formats:
- the pictures come from the b14 PDF, its embedded images or (for its vector drawings) an area of a page;
- running it again after an edit replaces what it wrote before.

Done 20 Oct: every chapter but the reference sections (24, 26-29) and Appendix N, which is made from them in M4.
- **Replaced by b14's text, checked against the build:** 1-5, 17, 20, 25, and appendices B to D and F to K.
- **Merged** (the PicoMite's or b14's text as the base, the other's added, both corrected): 6-16, 18, 19, 22 and 23, and appendix A.
- **New:** 12 (Video Modes and Pages) and appendix M (applicability).
- **Kept** after the check: 10, 11 and 21 (corrected where they differ), and appendices E, L and O.
- **The front matter:** `tools/manual/front.py` (About, Support, the acknowledgments, the licence, This Manual).

Each fact was checked in the source and, where it could be, on the board:
- the pages of every mode and depth;
- MEMORY, DATE$ and TIMER;
- the operators;
- the CRC check values;
- the longest name;
- the recursion depth.

The text chapters are spliced into the skeleton by `chapter.py`. A chapter kept as it is has a `!KEEP` file, which takes out only its notes.

The other tools:
- `extract.py` turns a chapter back into text.
- `photo.py` takes the editor and FM pictures on the board.

What the chapters found:
- **b14's text, wrong for this build or wrong outright:**
  - string comparison ("A" is less than "a", not greater);
  - CAN OPEN's default filter (FIFO1, not FIFO0);
  - the 29-bit CAN identifier range (0-1FFFFFFF);
  - WS2812 (now `type, pin, nbr, colours%()`, as the PicoMite's, with type W);
  - HUMID's syntax (typed "DHD22");
  - Alt giving 8B (it is a modifier);
  - Shift over VT100 (only Shift-F3 is translated);
  - a USB hub "not supported";
  - the console at 115200;
  - options "in the battery RAM" (they are in flash);
  - the first boot asking the screen type (it asks the keyboard, the time and the date);
  - TTS as a command (it is PLAY TTS);
  - TIMER "in microseconds" (it counts milliseconds);
  - SETTICK FAST (not in the build: `cmd_fasttick` is unreachable, as P1 step 7 found);
  - names "up to 31 characters" (32), and a 50-level recursion limit (100).
- **In the code**, to fix or decide (not manual work):
  - OPTION DEFAULT MODE tests "8" twice (`MiscSTM32.c:1573` and `:1579`), so its branch for mode 9 is dead.
  - WS2812 with one LED takes a value of at most &HFFFFFF, even for type W (RGBW).
  - CAN OPEN's error lists the speed "5000000" for 500000.
  - `KBrdList` names IT, but nothing can select it: OPTION USBKEYBOARD and the first boot take six layouts.
  - I2C WRITE/READ option 1 (keep the bus) is stored and never read, so every transfer ends with a stop. The manual says so.
  - TURTLE lacks the PicoMite's later spellings (PEN COLOR, PENCOLOUR and the others).
  - A program's OPTION EXPLICIT stays in force at the prompt after it ends: "FOR a = 1 TO 20" then fails with "A is not declared" until NEW. Is this as the PicoMite does?
  - The Xbox controllers (`USBXInput.c`) are named in the manual but untested with a genuine controller.
- **In this project's own documents:** the parity plan and `chapters.md` said the build has the CMM2's turtle; it has the PicoMite's (from b14). Both are corrected.

## The new manual's structure (proposed; M-A4 settles it)

**Front matter:** the title page and the About section (M-D3), Introduction, Generation 1 and Generation 2, Hardware Features. These are the CMM2's, from b14.

**Using it:**
- **Serial Console:** merged.
- **Using MMBasic:** the CMM2's, as programs run from the SD card, plus the PicoMite's prompt features.
- **Full Screen Editor:** new, written from `docs/Editor_Port_Plan.md` (editor E9).
- **File Manager:** the PicoMite's FM manual, with what differs on the CMM2 (P4's list).

**The language:** Variables and Expressions, Subroutines and Functions, Long Strings and MMBasic Characteristics come from the PicoMite manual. Structures and LIBRARY are included.

**The CMM2's devices, rewritten from b14 with the new parts:**
- **Video and Graphics:** MODE, pages, colour depths, and MODE n,4 with its RGB121 FRAMEBUFFER, TILEMAP and image slots.
- **Keyboard, Mouse and Gamepad:** USB devices, mouse channels and the Wii controllers.
- **Program and Data Storage:** A: and B:, the USB drive as C:, the flash slots, LIBRARY and ZMODEM.
- **Sound, Using the I/O Pins, Special Device Support and Game Playing Features.**
- **Porting Programs:** gains running PicoMite programs on a CMM2, with the mode map (parity plan 6.3).

**The reference:** Predefined Read Only Variables, Operators, Options, Commands, Functions, and Obsolete Commands and Functions.

**Appendices:**
- Serial, I2C, 1-Wire, SPI, CAN, Regular Expressions, Sprites, Turtle Graphics, CRC, Special Keyboard Keys and Loading the Firmware;
- Applicability: the CMM2's closed list of labels;
- where each keyword is documented.

**Applicability.** The PicoMite's labels name its sixteen images, and none of them applies here. The CMM2 needs its own closed list, kept by `MANUAL_STYLE.md`'s rules:
- `GENERATION 2 ONLY` (and `GENERATION 1 ONLY` where needed), for SDRAM size, the RTC and the executable window;
- later, `CMM2 DIALECT` and `PICOMITE DIALECT`, once `OPTION PICOMITE` exists (P3).

## Steps

| Step | What | Estimate |
|---|---|---|
| M0 | This plan; the sources converted; the folders. | done 20 Oct |
| M1 | The audit tools and their outputs: inventory, harvests, the cross-reference and the chapter table (M-A1 to M-A4). | 3-4 days |
| M2 | The skeleton: the new docx made from the PicoMite manual, with the title page, front matter and chapter list per M-A4. Chapters to be replaced are emptied to a heading and a note; styles, bookmarks and the table of contents are kept. | 1-2 days |
| M3 | The CMM2 chapters: text from b14 (or its Word source, M-D2), figures and tables from the 2020 file, corrected for this build. | 4-6 days |
| M4 | The reference sections, entry by entry from `keywords.md`. The most-used keywords come first: PRINT, the file commands, graphics, MODE and PAGE. | 2-3 weeks |
| M5 | The new parts: the editor, FM, USB devices, LIBRARY, structures, TILEMAP, MODE n,4, image slots, ZMODEM, the drives. | 1 week |
| M6 | The checks (`tools/manual/check.py`), a PDF made by Word, and Peter's review. | 3-4 days |
| M7 | The alpha: the docx and PDF, with `help.txt` for the CMM2 generated from the new manual (editor E8, P4's HELP item). | 1-2 days |

That is five to seven weeks of focused work. M4 is most of it.

**Keeping it current.** After the alpha:
- A commit that adds or changes a keyword changes the manual in the same commit, as the PicoMite's practice is.
- `check.py` runs with the tests. It fails on:
  - a firmware keyword with no entry;
  - an entry for a keyword the build lacks;
  - a label outside the closed list.
- The title page's version and date move with each release.

## The checks (M6)

`tools/manual/check.py` reads the new docx and the firmware inventory. It reports:
- each keyword and sub-keyword without an entry, and each entry without a keyword;
- labels outside the closed list;
- syntax lines whose keywords or argument counts differ from the handler's, where the inventory can tell;
- the examples that can be run: their code lifted from the manual into `tests/manual/` and run on the board through `tools/goldens.py`, with their stated output as `.expected`.

The PDF is made by Word through COM, as the `.doc` was converted. Pages are looked at as images to check the layout.

## Files

- **`docs/Manual_Plan.md`:** this plan.
- **`docs/manual/Colour_Maximite_2_User_Manual.docx`:** the manual (from M2 on).
- **`docs/manual/audit/`:** `firmware.json`, the harvests, `keywords.md` and `chapters.md`.
- **`docs/manual/sources/`:** the converted 2020 `.docx` and the harvests' raw text. These are made from Peter's files and not committed (`.gitignore`). The manuals themselves stay where Peter put them.
- **`tools/manual/`:** `inventory.py`, `harvest_pdf.py`, `harvest_docx.py`, `crossref.py`, `check.py`, and the scripts that build and edit the docx.

The docx is edited by scripts (python-docx and its XML), one change per commit, as the PicoMite manual is. That way each change can be seen and repeated, and Word's own formatting is never lost to a round trip through another format.
