# The manual audit: the chapters (docs/Manual_Plan.md, M-A4)

Each chapter of the new manual, where its text comes from, and what is done to it. This is a proposal for Peter to review (20 Oct).

The sources:
- **P:** the PicoMite manual 7.0, which is the base (M-D1). Its 41 chapters are in `pico_chapters.json`.
- **C:** the CMM2 manual 6.00.00 b14, the PDF. Its 36 chapters are listed with their pages.

The actions:
- **keep:** P's text, checked against the build.
- **replace:** C's text instead of P's, brought up to date.
- **merge:** both, as the notes say.
- **new:** written for this build.
- **drop:** the CMM2 lacks it.

| # | Chapter of the new manual | From | Action | Notes |
|---|---|---|---|---|
| 1 | Introduction | C p7 | replace | What the CMM2 is, and what 7.00 brings: the PicoMite's interpreter, USB devices, FM, the editor. |
| 2 | Quick Start Tutorial | C p8 | replace | |
| 3 | Colour Maximite 2 Generation 1 | C p10 | replace | Mouse interface: PS/2 and now USB. |
| 4 | Colour Maximite 2 Generation 2 | C p12 | replace | |
| 5 | Hardware Features | C p14 | replace | The USB socket now takes a hub, mice, gamepads, USB serial and a flash drive. A low-speed device cannot work behind a hub (ES0392). |
| 6 | Serial Console | P, C p14 | merge | P's terminal-emulator sections; C's connector. The default speed is the build's (460800). |
| 7 | Using MMBasic | C p19, P | merge | C: programs run from the SD card, the status line, timing. P: the library, MM.STARTUP, MM.PROMPT, MM.END, shortcut keys. |
| - | Calculator Immediate Mode | P | drop | Not in the build (parity plan P4: CALC). It comes back when CALC does. |
| 8 | Full Screen Editor | P | new | P's chapter as the base, with the CMM2's additions: Shift-Tab, Shift-Del, Shift-arrows, two-stage PgUp/PgDn, F11 help, the mouse, Print Screen, saving that cannot lose the edits (`docs/Editor_Port_Plan.md`, editor E9). |
| 9 | File Manager | P, FM_User_Manual.md | keep | With the CMM2's differences (parity plan P4): drives A:/B:/C:, RUN and back, E at an error, no mouse yet. C's chapter described the old fm.c, which has gone. |
| 10 | Variables and Expressions | P | keep | The same sections in both. Structures and 64-bit unsigned integers are already in. |
| 11 | Subroutines and Functions | P | keep | |
| 12 | Video Modes and Pages | C p56 (VGA Resolution, Colour Depth and Pages), C p12 | new | In place of P's Video Output (VGA/HDMI). MODE 1-18 and the colour depths; pages; MODE n,4 (the PicoMite's RGB121) with FRAMEBUFFER; image slots; the PicoMite mode map. |
| 13 | Keyboard, Mouse and Gamepad | P, C p14/p56 | merge | P's USB sections (interface, hub, keyboard, mouse on channel 2, gamepads on 3-4, USB serial as COM4-COM7, flash drive). C's mouse channels and Wii controllers. Without P's PS/2 keyboard and on-screen keyboard. |
| 14 | Program and Data Storage | C p43, P | merge | C: the SD card as A: (and B:). P: the USB drive as C:, XMODEM and ZMODEM, LOAD and SAVE IMAGE, sequential and random I/O. The program's flash and LIBRARY replace P's flash slots and flash filesystem (decision 11: no A: in flash). |
| 15 | Sound Output | C p47, P | merge | The CMM2's DAC output. P's PLAY sections, merged audio (WAV/FLAC/MP3/MOD, BBC sound, SAMPLE, ARRAY, PAUSE, volume). Without PWM, VS1053, MCP48n2 and Bluetooth. |
| 16 | Using the I/O Pins | C p49, P | merge | C's connector and pin numbers. P's communications section. |
| 17 | Special Device Support | C p52 | replace | IR, temperature, humidity, distance, WS2812, the Hobbytronic mouse, CAN. Without P's LCD, keypad and camera, which the build lacks. |
| - | Display Panels | P | drop | The build has no OPTION LCDPANEL. |
| 18 | Graphics Functions | C p38, P | merge | C's coordinates, colours, fonts, images, advanced graphics, 3D engine. P's BLIT and sprites where the build has the PicoMite's forms. |
| 19 | GUI Controls | P, C p38 (GUI Controls) | merge | The build has CTRLVAL( and MSGBOX(, which neither manual's reference lists: to check. |
| 20 | Game Playing Features | C p56 | replace | Brought up to date: USB gamepads, TILEMAP, image slots, the game ports. |
| - | WiFi and Internet Functions | P | drop | |
| 21 | Long Strings | P | keep | |
| - | Bluetooth Versions | P | drop | |
| 22 | Porting Programs | C p58 | merge | C's text, plus a new section: running PicoMite programs on the CMM2. It covers the mode map (parity plan 6.3), B:, pin names, PicoMite CSUBs (CallTablePS), and what cannot run. |
| 23 | MMBasic Characteristics | P, C p63 | merge | P's text. C's memory map, brought up to date: SDRAM, the executable window, bank 2 and the library sector. |
| 24 | Predefined Read Only Variables | P, C p66 | reference | `keywords.md` |
| 25 | Operators | C p70 | replace | P has no Operators chapter. |
| 26 | Options | P, C p71 | reference | `keywords.md` |
| 27 | Commands | P, C p77 | reference | `keywords.md` |
| 28 | Functions | P, C p135 | reference | `keywords.md` |
| 29 | Obsolete Commands and Functions | P, C p154 | reference | |
| A | Serial Communications | C p155, P | merge | COM1-COM3 on the connector; COM4-COM7 on USB. |
| B | I2C Communications | C p158 | replace | Three buses, and no system I2C. |
| C | 1-Wire Communications | C p160 | replace | |
| D | SPI Communications | C p161 | replace | |
| E | Regular Expressions | C p170, P | merge | The PicoMite's regex is in the build. Leave out P's PIO sections, which the harvest found under this heading. |
| F | Sprites | C p163 | replace | The CMM2's BLIT/SPRITE engine. |
| G | Turtle Graphics | C p165 | replace | The build still has the CMM2's `turtle.c` (the PicoMite's turtle is P3). |
| H | Cyclic Redundancy Check (CRC) | C p167 | replace | |
| I | Special Keyboard Keys | C p172 | replace | The codes are now KeyboardMap.c's, the PicoMite's: Shift-Up 164, Shift-Left 162. |
| J | CAN Support | C p173 | replace | |
| K | Loading the Firmware | C p177 | replace | |
| L | Programming in BASIC: a Tutorial | P | keep | |
| M | Firmware Applicability | P (form) | new | The CMM2's closed list of labels (Manual_Plan.md). |
| N | Where Each Keyword Is Documented | P (form) | new | Generated from the reference sections. |
| O | Companion Manuals | P | keep | Only those that apply to the build: structures, FM, TILEMAP, PLAY SAMPLE, PLAY BBC, and others to check. |

These PicoMite chapters are also dropped, each replaced by the CMM2 chapter named:
- Introduction, by 1;
- Firmware Versions and Files, which lists the PicoMite's images, by K (Loading the Firmware);
- First Steps, by 2 (Quick Start Tutorial);
- Hardware Details, by 5 (Hardware Features);
- Appendix F, the PIO Programming Package: the CMM2 has no PIO.
