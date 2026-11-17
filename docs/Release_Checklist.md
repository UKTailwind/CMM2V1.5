# Release checklist

*16 November 2026, for 8.00.00b1. Built from the way the 7.00.00 betas and release candidates were made (b4 to RC6:
their notes, assets and scripts). No written list existed for them. Use it for every release, V7 or V8, and add to it
when a release finds something it missed.*

A release is the firmware and everything that goes with it: the programs zip, six PDFs and the notes. The GitHub release
is the only place users get them, and the Back Shed post points there. Nothing is published, pushed or posted without
Peter's word.

## 1. Before starting

- [ ] Peter has asked for the release, and said what it is: a beta (GitHub pre-release) or a release candidate or
      final (full release).
- [ ] `git fetch`: other sessions commit too. The release branch has everything meant for it:
  - V7 releases come from **main**.
  - V8 releases come from **v8-dev**. It must equal v8-wifi, and main must be merged in (`git log v8-dev..main` empty).
- [ ] Every worktree on the release branch is clean (`git worktree list`, then `git status` in each).
- [ ] The boards are free: ask before driving a board Peter may be using. Today: G1 COM47 (115200, ST-Link
      37FF71064E5734367D2B1643), G2 COM10 (460800, 0667FF575255878281071917), G3 COM46.
- [ ] Each board's console is plugged into the PC as well as its ST-Link: flashing erases the options, and only the
      console can load them again (for 8.00.00b2 the G3 had its ST-Link, 37FF71064E5734367D2B1643, but no console).

## 2. The version

- [ ] Inc/Version.h `VERSION` ("8.00.00b1"); MM.VER follows from it (b1 = 8.000001; a final release counts as 99:
      7.000099).
- [ ] The User Manual's note (docs/manual/text/01_introduction.md: "This manual is for version ...").
- [ ] The User Manual's title page, which is in the .docx itself: "MMBasic Ver", "Matches firmware" and the date
      under the author (RC6's manual still said 7.00.00RC5 and 7 November), and the document's subject.
- [ ] tools/manual/guide.py: `FIRMWARE` and `DATE` (Loadable Programs), and each other guide's `DATE` and
      `'Matches firmware'` when its text changes.
- [ ] The zip's name in the guides: docs/apps/text/01_introduction.md and 20_appendix_a_licences.md,
      docs/cc/text/01_introduction.md and 02_getting_started.md.
- [ ] docs/loadable/text/04_building.md: the services version of this firmware in its list.
- [ ] New services (Inc/cmm2img.h) in the Loadable Programs guide, in four places: 04_building.md's table of versions,
      the chapter that describes them, Appendix A (20_appendix_a_services.md) and the USER modules' table
      (14_user_modules.md). For 8.00.00b2 versions 21 and 22 were in the header only.
- [ ] `git grep -n "<previous version>"` (for example 7.00.00RC6), and check each hit. Minimum versions ("needs
      firmware 7.00.00RC3 or later") stay.

## 3. Compatibility with the last release

What a user brings from the previous release must work, or be refused with a message that says what to do.

- [ ] **The image format** (Inc/MMBasic.h `IMAGE_FORMAT`): if the token table changed, the flash library, .lib files
      and flash slots saved by the previous release are refused, each with its message (Library.c LibStamp,
      ImageSlots.c). Found for 8.00.00b1: the flash library was not checked and would have run with the wrong
      functions after OPTION DISK LOAD.
- [ ] **The options** (Inc/Flash.h `OPTION_LAYOUT`): OPTION DISK LOAD takes a file saved by the previous release
      (OPTION_SIZE_L3 for 7.00's 992 bytes); the new options start off.
- [ ] **The loadable programs' services** only grow: the zip's programs run without being rebuilt.
- [ ] **The update itself**, on a board, as a user does it (STM32CubeProgrammer without a full erase, so the library's
      sector 7 is kept): the previous release written fresh, a library (using EOF( LOF( INPUT$( ), a .lib file and
      the options saved under it, then the new firmware, OPTION DISK LOAD, and the library and .lib checked
      (scratchpad upg/upgrade_test.py, 16 Nov; rel_b2/upgrade_b1b2.py from 8.00.00b1, 17 Nov).
- [ ] **Going back:** say in the notes how (full chip erase, the old options file) when the new version cannot be
      undone by writing the old firmware alone.

## 4. The firmware

- [ ] A clean Release build of the release commit: `ninja -C build/Release`; no new warnings; the flash sections fit
      (`arm-none-eabi-size -A`: .text, .bank2, .net; bank 1's space).
- [ ] Copy build/Release/CMM2V1.5.bin to `CMM2_MMBasic_<version>.bin`; note its sha256.
- [ ] Write that exact file to each board: OPTION DISK SAVE first (stop if it fails), one ST-Link session (erase,
      write, verify, reset), 40 s untouched, OPTION DISK LOAD; MM.VER and MM.DEVICE$; OPTION LIST the same as before.

## 5. Testing

On the release's firmware, or on one whose later commits change only documents (say which in the notes).

- [ ] The regression on each generation: tools/goldens.py (tests/cmm2, tests/picomite, the crash tests, the corpus and
      Testfiles), SP Tools (686/727 is the baseline; tst_file needs the USB flash drive). Long runs in the
      background, with a log.
- [ ] Every video mode; the sound, short and quiet (PLAY VOLUME 30 to 50, seconds).
- [ ] What is new in this release, on each generation it runs on. For 8.00.00b1: WiFi (tests/wifi and the pc*.py
      scripts), the G3's HDMI and its sound, USER modules (tests/user), SQL (the zip's sql folder: computers.bas, a
      LONGSTRING).
- [ ] Each program in the zip starts on each generation it supports (one start each).

## 6. The PDFs

Built from their .docx by tools/manual/word.ps1 (no PDF may be open in a viewer; no dialog may appear on Peter's
screen). Check each title page's version and date, and the bookmarks.

| Asset | Source | How |
|---|---|---|
| Colour_Maximite_2_User_Manual.pdf | docs/manual (text/*.md, ref/*.txt, docx) | tools/manual/chapter.py and reftext.py; Appendix N by appendix_n.py from a fresh firmware.json (inventory.py); Inc/help.h by hand (CRLF) |
| Colour_Maximite_2_Loadable_Programs.pdf | docs/loadable/text | `python tools/manual/guide.py loadable` |
| Colour_Maximite_2_App_User_Guide.pdf | docs/apps/text | `python tools/manual/guide.py apps` |
| Colour_Maximite_2_C_Compiler.pdf | docs/cc/text | `python tools/manual/guide.py cc` |
| Colour_Maximite_2_386_PC_Emulator.pdf | docs/pc386/text | `python tools/manual/guide.py pc386` |
| Stepper_Reference.pdf | docs/Stepper_Reference.pdf (tracked) | docs/stepper/generate_stepper_pdf.py, only when STEPPER changes |

- [ ] Each PDF rebuilt when its text, the zip's name or the version on its title page changed; otherwise the
      previous release's copy is reused and the notes say it is unchanged.

## 7. The programs zip

- [ ] Start from the previous release's zip **as published**: download it from GitHub and check its sha256. Never
      rebuild it from a local folder (Peter's own games are in some of them).
- [ ] Replace or add only the programs that changed. Python's zipfile: `copy.copy` each ZipInfo, or `writestr` breaks
      the source zip.
- [ ] README.txt (CRLF) for this version: its name, what changed, and each program's licence.
- [ ] The licences folder: every program's licence, and the firmware's third-party code (for 8.00.00b1: lwIP, Mbed
      TLS, cyw43-driver and the CYW43439 firmware).
- [ ] Never in the zip: commercial ROMs other than those Peter has decided on, the Gobliiins demo, Amiga ROMs and
      .adf files, anything of Peter's own.
- [ ] Name it `CMM2_Programs_<version>.zip`; list it and test it (`unzip -t`); note its sha256.

## 8. The notes

- [ ] docs/Release_Notes_<version>.md, compared with the last release published: Before you update; What is new;
      What behaves differently; (V8: Structural changes); The programs; Tested; Known problems; The manuals;
      Licences. Written for forum readers. Every claim checked against the code, the manual or a test.
- [ ] The full notes: V7's docs/Release_Notes_7.00.00.md (from 5.07.02) is the `_full.md` asset. V8's start with
      8.00.00b1, whose notes are themselves the full notes; from b2, docs/Release_Notes_8.00.00.md collects them
      (`python tools/v8notes.py`: add the new beta to its BETAS), published as Release_Notes_<version>_full.md.
- [ ] The Back Shed copy: `python tools/md2bb.py docs/Release_Notes_<version>.md docs/Release_Notes_<version>.backshed.txt`.
- [ ] The release's known problems: the previous release's, less those fixed, plus the new ones.

## 9. Licences

- [ ] LICENSE unchanged (or Peter has agreed the change).
- [ ] Every new third-party component has its licence beside it (Middlewares/Third_Party/*/LICENSE or COPYING) and its
      source noted (CMM2_SOURCE.md), and its terms allow the binary to be given out as we do. For 8.00.00b1:
      cyw43-driver's copy (pico-sdk 2.3.1's commit) is under George Robotics' older licence, non-commercial use only;
      upstream moved to MIT on 2 Sep 2026. Infineon's firmware for the chip has no licence text in the driver.

## 10. Commit and push

- [ ] Commit the version, notes, manuals and PDFs (messages end with the Co-Authored-By line).
- [ ] Push the release branch, at Peter's word.

## 11. The GitHub release (UKTailwind/CMM2V1.5)

- [ ] Create it as a **draft**: `gh release create v<version> --draft [--prerelease] --target <FULL SHA>
      --title "Colour Maximite 2 MMBasic <version> (beta|release candidate)" --notes-file docs/Release_Notes_<version>.md`
      (with --draft, a short SHA gives HTTP 422).
  - A V8 beta is a **pre-release** and must not become Latest: 7.00's release candidate stays Latest
    (`--latest=false`).
- [ ] The assets (RC6 had nine; 8.00.00b1 has the same set):

| Asset | 7.00.00RC6 | 8.00.00b1 |
|---|---|---|
| CMM2_MMBasic_<version>.bin | yes | yes |
| CMM2_Programs_<version>.zip | yes | yes |
| Colour_Maximite_2_User_Manual.pdf | yes | rebuilt |
| Colour_Maximite_2_Loadable_Programs.pdf | yes | rebuilt (USER Modules) |
| Colour_Maximite_2_App_User_Guide.pdf | yes | rebuilt (the zip's name) |
| Colour_Maximite_2_C_Compiler.pdf | yes | rebuilt (the zip's name) |
| Colour_Maximite_2_386_PC_Emulator.pdf | yes | unchanged |
| Stepper_Reference.pdf | yes | unchanged |
| Release_Notes_<version>_full.md | yes | Release_Notes_8.00.00b1.md |

- [ ] Download every asset back from the draft and compare its sha256 with the local file.
- [ ] Publish at Peter's word: `gh release edit v<version> --draft=false` (the tag is made then, at the target).
- [ ] After publishing, check the release page: pre-release or not, Latest or not, the assets, the body.

## 12. The announcement

- [ ] A Back Shed post in BBCode, short, linking the release: what is new, how to update (the warnings of Before you
      update), where to report problems. Put on an artifact page for Peter to copy; he posts it.

## 13. Afterwards

- [ ] The boards' state noted (which firmware each runs).
- [ ] The branches in line again (v8-dev and v8-wifi; main merged into v8-dev).
- [ ] The memory notes: the release, its commit and tag, its assets.
