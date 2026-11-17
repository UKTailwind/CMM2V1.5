# The Pico Computer 3 book on the CMM2's MicroPython: what is missing

*17 November 2026. Every example program of Peter's book (UKTailwind/micropython, branch `pico-computer-3`,
`ports/rp2/boards/PICO_COMPUTER_3/book/examples`, at 5dddc6868) was run in `micropython.app` (the port on branch `cmm2`,
the app on the card built 3 Nov) on the G2 on COM10 under 8.00.00b2. This lists the parts the CMM2 build lacks, ready
to add later, the faults the run found in the port and in the book, and what a rider to the book would have to say.
Peter wants the rider to say that a CMM2 can also be used.*

## Where it stands (17 November, after the fixes)

- **All 162 programs run** on the G2 with the fixed app, firmware and book: 156 in the table under "Results after
  the fixes", then the adventure and the mktime listing once the book was fixed, then the four that use the PC3's
  pins once the CMM2 took them (services version 22, "Pins as on a PC3" below): the LED (ch08/02), the analogue input
  on GP40 (ch32/03, 06) and I2C on GP20/21 (ch32/05).
- **The book's bugs B1-B7 were confirmed on a PC2 and fixed** in the book, its examples and two PC3 library files;
  retested on both boards (the section "The book's bugs" below).
- **Fixed:** the four port faults (F1-F4), the clock helpers (M1), the alarm (M2), asyncio (M3), the watchdog (M4),
  native code (M5) and the network (M7: Wi-Fi, sockets, HTTPS, MQTT, NTP, mip). Details in "Progress" below.
- **Not done:** touch (M6), XMODEM at the prompt (M8) and gamepads (M9); no example needs them.
- **Nothing is committed:** the port on branch `cmm2` of the MicroPython repository, the book's fixes in a worktree
  of `pico-computer-3` (`~/src/mp-pc3`, also copied into the main tree), the firmware on v8-dev.

## Summary of the first run

- **130 of the 162 programs run today** (161 examples, fragments left out, plus Northwind). Every graphics, sprite,
  tile-map, sound, GUI, turtle, plot and games example runs. All of chapters 5-24, 26 and 27 run except two, which
  need hardware the CMM2 lacks (the LED) or the clock helper.
- **The 32 that fail:**
  - 19 need a missing part. That's mostly the network (9) and the clock helpers `gettime`/`settime` (6, one of
    which also needs the DS3231 alarm). The rest, one each: `asyncio` (the other `asyncio` program also needs the
    network), `machine.WDT`, `@micropython.native`, and the DS3231 alarm on its own. Items M1-M7 below.
  - 8 hit a fault in the port. SQLite can't get its 2 MB once the heap has been used, `import ulab.numpy` fails, a
    pin can't change owner (UART after input), and dropped files keep their handles. Items F1-F4, each confirmed
    on the board in a second session.
  - 4 use PC3 pins: the LED, the analogue input on GP40, and I2C on GP20/21. These belong in the rider (R1-R3).
  - 1 is a bug in the book itself (the adventure, ch25/02) that would fail on a PC3 too. Two more book bugs, B3 and
    B4, are counted above under M1 and F2.
- **Most missing parts need no firmware change.** The clock, `asyncio`, the watchdog, `@native`, XMODEM and touch can
  all be done in the port through services the loader already has (`datetime`, `basic_cmd`, `basic_eval`). Two need
  new firmware services: the DS3231's alarm registers, and the network for apps.
- **After M1-M7 and F1-F4**, everything in the book runs on a CMM2 except four programs that need the rider's pin
  changes, the two book bugs, and the network examples on a board without the Wi-Fi module.

## How it was run

- The examples went to the card as `A:/book/chNN/` (ZMODEM from MMBasic).
- The book's `lib/` folder went to `A:/lib`, with `uQR.py` and `micropyGPS.py`, which the book installs with `mip`;
  both import and run.
- Northwind went to `A:/northwind`, and the PC3's `tests/` to `A:/tests` (with `test.jpg` and `test.png` from
  `A:/media`).
- The pictures and music the examples name were copied from `A:/media`:
  - `/sd/holiday.jpg` (tiger-640.jpg), `/sd/logo.bmp` (tiger320.bmp), `/sd/ghost.png`
  - `/sd/invaders.png`: a 4x2 sheet of 16x16 cells on magenta, made for the test
  - `/sd/music/song.mp3` (Bombora.mp3), `/sd/game.mod` (cartoon.mod), `/sd/asteroids.mod` (RISsound.mod)
- All of it is still on the card.
- A script typed `run("/book/chNN/file.py")` at the REPL over the serial console and answered each `input()` prompt.
  It pressed Ctrl-C after a time limit for the programs that loop until Esc (games, GUIs, clocks).
- A listing that continues an earlier one got the book's context first (`age = 20`, `t = Turtle()`, the `Hero`
  class...).
- The doubtful failures were run again in a fresh session.
- **Not checked:** what needs a person.
  - Playing the games: `keydown()` reads the USB keyboard, not the serial line.
  - Clicking in the GUIs.
  - How the pictures look and the sound sounds.
- "Runs" means the program started, drew or played, and stopped cleanly with no traceback.

## Results by chapter

The letters in brackets are the items below: M = a missing part, F = a fault in the port, R = a rider note, B = a bug
in the book.

| Chapter | Run | Run today | What fails, and why |
|---|---|---|---|
| 05 Writing real programs: the editor | 3 | 3 of 3 | all run |
| 06 Variables, numbers and text | 2 | 2 of 2 | all run |
| 07 Making decisions: if | 5 | 5 of 5 | all run |
| 08 Repeating yourself: loops | 7 | 6 of 7 | `02-blink.py`: `Pin("LED")`: no user LED (R1) |
| 09 Turtle graphics: loops you can see | 8 | 8 of 8 | all run |
| 10 Collections: lists, tuples and dictionaries | 5 | 5 of 5 | all run |
| 11 Functions: building your own commands | 11 | 11 of 11 | all run (06 shows the book's own NameError, as meant) |
| 12 Strings and files | 5 | 5 of 5 | all run |
| 13 When things go wrong: errors and debugging | 6 | 6 of 6 | all run |
| 14 Objects: a gentle introduction to classes | 7 | 7 of 7 | all run |
| 15 The screen up close | 8 | 7 of 8 | `08-clockface.py`: `gettime` missing (M1) |
| 16 Bitmap images and sprite sheets | 8 | 8 of 8 | all run |
| 17 Smooth motion: buffers and the overlay | 7 | 7 of 7 | all run |
| 18 Sprites | 4 | 4 of 4 | all run |
| 19 Tile maps: big worlds from small pieces | 5 | 5 of 5 | all run |
| 20 Sound and music | 7 | 7 of 7 | all run |
| 21 Reading the player: keyboard, mouse and touch | 6 | 6 of 6 | all run (touch reads nothing: M6) |
| 22 Anatomy of a game | 3 | 3 of 3 | all run |
| 23 Breakout | 1 | 1 of 1 | runs |
| 24 Asteroids: vectors, crowds and particles | 3 | 3 of 3 | all run |
| 25 The adventure: NPCs, dialogue and saved worlds | 3 | 2 of 3 | `02-adventure.py`: book bugs B1 and B2 (would fail on a PC3 too); runs with both fixed |
| 26 Polish: the last ten percent | 2 | 2 of 2 | all run |
| 27 Building apps: the GUI toolkit | 4 | 4 of 4 | all run |
| 28 Databases: keeping and querying real data | 1 | 0 of 1 | `01-scores.py`: SQLite's 2 MB not found after 120 programs (F1); runs in a fresh session |
| 29 Time and schedules: the real-time clock | 5 | 1 of 5 | `01`: `settime` (M1). `02`: `gettime` (M1), then book bug B3. `04-alarm.py`: `gettime` (M1). `05`: DS3231 alarm (M2) |
| 30 Going online: Wi-Fi and the internet | 7 | 0 of 7 | all need the network (M7): `wifi`, `tz`/`ntpsync`/`auto`, `requests`, `umqtt.simple` |
| 31 Numbers at speed: the maths lab | 5 | 2 of 5 | `03`, `04`, `05-mathlab.py`: `import ulab.numpy` (B4/F2); all run when the import works |
| 32 Talking to the outside world: GPIO | 6 | 3 of 6 | `03-knob.py`, `06-gpad.py`: ADC on GP40 (R2). `05-roll_call.py`: I2C on GP20/21 (R3) |
| 33 Standing on others' shoulders | 4 | 2 of 4 | `02-gpslook.py`, `03-where.py`: GP1 still held as an input from chapter 32 (F3); run when GP1 is free |
| 34 Doing several things at once | 5 | 1 of 5 | `02-wake.py`: `gettime` (M1), the DS3231 alarm and its pin (M2). `03`: `machine.WDT` (M4). `04-juggler.py`: `asyncio` (M3). `05-skywatch.py`: `asyncio` (M3), `requests` (M7) |
| 35 Performance and memory | 5 | 4 of 5 | `02-speedlab.py`: `@micropython.native` (M5) |
| 36 The system itself | 3 | 0 of 3 | `01-boot.py`: `tz` (M7). `02-backup.py`: `gettime` (M1). `03`: needs the PC3's `tests` folder on the card; with it, 150 tests pass and 3 crash because the file handles run out (F4) |
| Northwind (Appendix J) | 1 | 0 of 1 | SQLite's 2 MB (F1); in a fresh session it builds its database, the GUI comes up and the reports are right |
| **All** | **162** | **130 of 162** | |

## The missing parts

Each item gives what the book uses it for, what happens today, and how to add it. P means the change is in the port
(`~/src/micropython/ports/cmm2`) and F in the firmware. Sizes are rough: S under a day, M a few days, L a week or more.

### M1. The clock helpers `gettime()`, `settime()`, `synctime()` (P, S)

- **Book:** ch15/08 clock face, ch29/01, 02 and 04 (the alarm clock), ch34/02, ch36/02 (backup), and chapter 29's
  REPL lines.
- **Today:** NameError. `modules/_boot.py` doesn't put these names in the REPL. The frozen `ds3231.py` is the PC3's,
  which talks to I2C(0) on GP20/21. The CMM2 refuses those pins, and its DS3231 isn't on the I/O connector anyway.
- **Add:**
  - A CMM2 `ds3231.py`, frozen in place of the PC3's, with the same names.
  - `gettime()` returns `time.localtime()[:6]`, the firmware's clock (from `cmm2->datetime`, which it keeps from the
    onboard DS3231).
  - `settime(y, mo, d, h, mi, s)` goes through the `basic_cmd` service: `DATE$ = "dd-mm-yyyy"` and
    `TIME$ = "hh:mm:ss"`. Check that this writes the DS3231, as it does at MMBasic's prompt.
  - `synctime()` has nothing to do.
  - `_boot.py` puts `gettime`, `settime` and `synctime` into `__main__`, as the PC3's `_boot_board.py` does.
- **Needs:** a small C binding for `basic_cmd` and `basic_eval`, say a `cmm2` module with `basic(stmt)` and
  `evaluate(expr)`. M4, M6 and M7 use it too.

### M2. The DS3231 alarm: `ds3231.set_alarm`, `alarm_fired`, `clear_alarm`, `alarm_off`, `alarm_pin` (F + P, M)

- **Book:** ch29/05 (an alarm that survives resets), ch34/02 (wake on the alarm's interrupt).
- **Today:** `ValueError: bad SCL or SDA pin` (the PC3's I2C pins).
- **Add:**
  - The CMM2's DS3231 is on the firmware's own bit-banged I2C (`Src/Analog_I2C.c`, PD4/PD5), so a program can't
    reach its alarm registers.
  - A loader service (version 21) is needed: read and write DS3231 registers through `DS3231_RD_Reg`/`DS3231_WR_Reg`.
  - Then the CMM2 `ds3231.py` of M1 writes the alarm-1 registers, and reads and clears the A1F flag, as the PC3's
    version does over I2C.
- **Question for Peter:** is the DS3231's INT/SQW line wired to an H743 pin on the G2 and G3?
  - If it is, `alarm_pin()` returns that pin.
  - If not, `alarm_pin()` can return a stand-in whose `irq()` polls A1F from the VM hook. The port's `Pin.irq` is
    already polled, so the program can't tell.

### M3. `asyncio` (P, S)

- **Book:** ch34/04 (the juggler), ch34/05 (skywatch, which also needs M7).
- **Today:** `ImportError: no module named 'asyncio'`.
- **Add:**
  - In `manifest.py`, add `include("$(MPY_DIR)/extmod/asyncio")`, as rp2's `boards/manifest.py` does.
  - The C half (`_asyncio`) and `select` are already built in at the EXTRA_FEATURES level.
  - Check `asyncio.sleep_ms` and a `select.poll` on stdin under the port's console.

### M4. `machine.WDT` (P, S)

- **Book:** ch34/03 (feed me every 5 seconds).
- **Today:** `AttributeError: 'module' object has no attribute 'WDT'`.
- **Add:**
  - `WDT(timeout=ms)` sends `WATCHDOG ms` through `basic_cmd`, and `feed()` sends it again, as MMBasic programs do.
  - Check that the firmware's watchdog keeps counting while an app runs, and that a timeout restarts cleanly to
    MMBasic.
  - A timeout restarts the board, so test it on a board nobody is using.

### M5. `@micropython.native`, `@micropython.viper`, `@micropython.asm_thumb` (P, M)

- **Book:** ch35/02 (speedlab: bytecode against native).
- **Today:** `SyntaxError: invalid micropython decorator`.
- **Add:**
  - Turn on `MICROPY_EMIT_THUMB`, `MICROPY_EMIT_INLINE_THUMB` and `MICROPY_EMIT_THUMB_ARMV7M`.
  - The code goes into the GC heap in SDRAM. The app's own code already runs from SDRAM, so it is executable there.
  - After emitting, clean the D-cache and invalidate the I-cache over the new code (`MP_PLAT_COMMIT_EXEC`).

### M6. The touch panel (P, S to M; nothing fails without it)

- **Book:** ch21/03 (swipe and tap), ch21/04 (painting with a finger), and chapter 27's GUIs by finger.
- **Today:** these run, but the `touch` stand-in reports no panel, so only the mouse works.
- **Add:** report the firmware's USB touch screen, the PicoMite driver it already has.
  - The quick way is `basic_eval` of `TOUCH(X)`, `TOUCH(Y)` and the gestures for each call.
  - A touch service would be cheaper for a GUI that polls every frame.

### M7. The network: `wifi()`, `tz()`, `ntpsync()`, `auto()`, `requests`, `umqtt.simple`, `mip`, `socket` (F + P, L)

- **Book:** all of chapter 30, ch34/05 and ch36/01 (`tz` in `boot.py`). Also chapter 33's `mip.install` at the
  prompt; meanwhile the reader can copy `uQR.py` and `micropyGPS.py` into `A:/lib`, and both run.
- **Today:** NameError and ImportError.
- **Add:**
  - Only for V8 firmware on a board with the Wi-Fi module. The firmware owns the module and its lwIP (the PC3's own
    network is cyw43 + lwIP inside MicroPython).
  - New loader services give an app the firmware's TCP/UDP sockets, DNS and TLS.
  - The port gets `socket` and `ssl` modules over those services, and freezes micropython-lib's `requests`,
    `umqtt.simple`/`umqtt.robust` and `mip`, as the PC3 does with `bundle-networking`.
  - `wifi()`, `ntpsync()` and `auto()` call the firmware's own Wi-Fi join and NTP through `basic_cmd`.
  - `tz()` could come first and alone: it only keeps the offset, which lets ch36/01's `boot.py` run.

### M8. XMODEM: `xrecv()`, `xsend()` (P, S to M; at the prompt only)

- **Book:** chapter 5 (moving programs to and from the PC), chapter 16 (pictures).
- **Today:** NameError. No example file uses them.
- **Add:** port the PC3's C `xmodem` module (`MICROPY_HW_ENABLE_XMODEM`) to the CMM2's serial console: raw bytes, with
  Ctrl-C off during a transfer.
- **Until then:** the rider points to MMBasic's own XMODEM and ZMODEM at its prompt, before `RUN "micropython.app"`.

### M9. Gamepads (P, S; not used by the book)

- The `gamepad` stand-in reports no pad. Services 13 and 16 give the USB pads to an app.
- Not needed for the book, but the PC3's API has it.

## Faults found in the port

### F1. SQLite can't get its 2 MB after the heap has been used (P, S)

- usqlite reserves its pool with `m_malloc_maybe(2 MB)` at the first `connect()`.
- After the 120 earlier programs (frame buffers, sprite sheets, decoded images), the heap had no 2 MB run left, and
  ch28/01 and Northwind failed with `usqlite_Error: cannot reserve 2097152 bytes for the SQLite engine`.
- In a fresh session both work. Northwind builds `northwind.db` and its reports are right.
- **Fix:** take the pool when the app starts (`main.c` already sizes it as `cmm2_sqlite_pool`), outside the GC heap
  if the loader's memory allows. Otherwise take it from the GC heap at start-up, as the 3.4 MB "with a database open"
  figure already assumes.

### F2. `import ulab.numpy` fails (P, S; probably the book's too, B4)

- `from ulab import numpy` works on the CMM2, and `ulab.numpy` is a proper module, but every `import ulab.<sub>`
  fails with `ImportError: no module named 'ulab.numpy'`.
- MicroPython's `MICROPY_MODULE_BUILTIN_SUBPACKAGES` evaluates to 1 in this tree, so the cause isn't found yet.
- **The PC3 emulator** (`ports/unix/build-pc3`, the same 1.29 core with ulab) fails the same way. So the book's form
  probably fails on a PC3 as well. The PC3's own code (`tests/test_math.py`, `solar_eclipse_ulab.py`) uses
  `from ulab import numpy as np`.
- **Fix:** see B4 for the book. For existing code, `_boot.py` can add `sys.modules["ulab.numpy"] = ulab.numpy` (and
  the same for `scipy` and `utils`). With that, ch31/03, 04 and the whole maths lab ran and gave the right answers.

### F3. A pin can't change owner between `machine` objects (P, S)

- After `UART(0)` (GP0/1), `Pin(1, Pin.IN)` fails with a bare `OSError: 16`.
- After `Pin(1)` was an input, `UART(0)` fails with `OSError: Pin 10 is in use`.
- On the PC3 the newest user simply takes the pin. In the book, chapter 32's button on GP1 comes before chapter 33's
  GPS on GP0/1, so a reader who follows the book in one session meets this.
- **Fix:** when `Pin`, `UART`, `I2C`, `SPI`, `PWM` or `ADC` claims a GP that another `machine` object holds, release
  it first (`pin_setup` OFF, `com_close`...), and give EBUSY a message.

### F4. Files dropped without `close()` keep the loader's handles (P, S)

- The loader gives an app 8 file handles. A file that is opened and dropped keeps its handle until a garbage
  collection runs its finaliser.
- `exec(open(f).read())` is a common MicroPython idiom, and the PC3's `test_all.py` uses it for each test file.
- After seven such files every `open()` fails with `OSError: 24`, even `run()`'s. In the suite, test_cursor,
  test_math and test_misc crashed this way.
- `gc.collect()` frees the handles; that was checked by hand.
- On the PC3, VfsFat has no such limit.
- **Fix:** in `vfs_cmm2_file.c`'s open, on "too many files" run `gc_collect()` and try once more.

## Bugs in the book (would fail on a PC3 too)

- **B1. ch25/02-adventure never creates its back buffer.**
  - `screen(hdmi.RGB640)` frees F: hdmi.c's mode change drops every FRAMEBUFFER target.
  - The program then calls `hdmi.write("F")` and gets `ValueError: framebuffer not created`.
  - Fix: add `hdmi.close("F")` and `hdmi.create()` before `console("none")`, as the other games in chapters 17-26
    have.
- **B2. ch25/02-adventure passes a float viewport.**
  - The hero starts at `32 * 16.0` and moves by `120 * dt`, so `tm.view(vx, vy)` gets floats.
  - `TileMap.draw` then raises `TypeError: can't convert float to int` inside `hdmi.tilemap`, which is the PC3's own
    C code.
  - Fix: `tm.view(int(vx), int(vy))`, or `int()` inside `TileMap.view`.
  - With B1 and B2 fixed, the adventure runs until stopped.
- **B3. ch29/02: `time.mktime(gettime())`.**
  - `gettime()` returns 6 values, but MicroPython's `mktime` wants 8 or 9, so it raises
    `TypeError: mktime needs a tuple of length 8 or 9`.
  - This was checked on the CMM2 and in the PC3 emulator.
  - Fix: `time.mktime(gettime() + (0, 0))`.
- **B4. Chapter 31 (and `pcmath.py`'s comment): `import ulab.numpy as np`.**
  - This fails in the PC3 emulator too (F2).
  - Use `from ulab import numpy as np`, as the PC3's tests do, unless a real PC3 accepts the dotted form.

## Results after the fixes

The same examples on the same G2, with the port and the firmware as in "Progress" below (one session, as in
the first run; ch30/01 and 02 run last, as they leave the network for the book's "MyNetwork"). Four were
checked again on their own: ch12/05 (the test's own setup was wrong the first time), ch34/02 (the first try
armed the alarm for a minute that had just begun, a race in the example), ch34/03 (it restarts the board when it
is not fed, as meant) and ch36/03 (the PC3's whole suite: 256 pass, and 5 fail on B5).

| Chapter | Run | Run now | What still fails, and why |
|---|---|---|---|
| 05 Writing real programs: the editor | 3 | 3 of 3 | all run |
| 06 Variables, numbers and text | 2 | 2 of 2 | all run |
| 07 Making decisions: if | 5 | 5 of 5 | all run |
| 08 Repeating yourself: loops | 7 | 6 of 7 | `02-blink.py`: `Pin("LED")`: no user LED (rider) |
| 09 Turtle graphics: loops you can see | 8 | 8 of 8 | all run |
| 10 Collections: lists, tuples and dictionaries | 5 | 5 of 5 | all run |
| 11 Functions: building your own commands | 11 | 11 of 11 | all run |
| 12 Strings and files | 5 | 5 of 5 | all run |
| 13 When things go wrong: errors and debugging | 6 | 6 of 6 | all run |
| 14 Objects: a gentle introduction to classes | 7 | 7 of 7 | all run |
| 15 The screen up close | 8 | 8 of 8 | all run |
| 16 Bitmap images and sprite sheets | 8 | 8 of 8 | all run |
| 17 Smooth motion: buffers and the overlay | 7 | 7 of 7 | all run |
| 18 Sprites | 4 | 4 of 4 | all run |
| 19 Tile maps: big worlds from small pieces | 5 | 5 of 5 | all run |
| 20 Sound and music | 7 | 7 of 7 | all run |
| 21 Reading the player: keyboard, mouse and touch | 6 | 6 of 6 | all run |
| 22 Anatomy of a game | 3 | 3 of 3 | all run |
| 23 Breakout | 1 | 1 of 1 | all run |
| 24 Asteroids: vectors, crowds and particles | 3 | 3 of 3 | all run |
| 25 The adventure: NPCs, dialogue and saved worlds | 3 | 2 of 3 | `02-adventure.py`: book bugs B1 and B2 (runs with both fixed) |
| 26 Polish: the last ten percent | 2 | 2 of 2 | all run |
| 27 Building apps: the GUI toolkit | 4 | 4 of 4 | all run |
| 28 Databases: keeping and querying real data | 1 | 1 of 1 | all run |
| 29 Time and schedules: the real-time clock | 5 | 4 of 5 | `02-listing.py`: book bug B3 (`mktime` of a 6-tuple) |
| 30 Going online: Wi-Fi and the internet | 7 | 7 of 7 | all run |
| 31 Numbers at speed: the maths lab | 5 | 5 of 5 | all run |
| 32 Talking to the outside world: GPIO | 6 | 3 of 6 | `03-knob.py`: ADC on GP40: GP26 on a CMM2 (rider)<br>`05-roll_call.py`: I2C on GP20/21: GP16/17 on a CMM2 (rider)<br>`06-gpad.py`: ADC on GP40: GP26 on a CMM2 (rider) |
| 33 Standing on others' shoulders | 4 | 4 of 4 | all run |
| 34 Doing several things at once | 5 | 5 of 5 | all run |
| 35 Performance and memory | 5 | 5 of 5 | all run |
| 36 The system itself | 3 | 3 of 3 | all run |
| northwind Northwind Traders (Appendix J) | 1 | 1 of 1 | all run |
| **All** | **162** | **156 of 162** | |

## Progress (17 November, fixing by cause)

Peter, 17 Nov: "carry on and fix the 32 that fail by cause"; the book's bugs wait for his check on a PC3. The port's
changes are in `~/src/micropython/ports/cmm2` (branch `cmm2`), the firmware's on v8-dev; none committed yet.

| Item | Done | How | Checked on the G2 (COM10) |
|---|---|---|---|
| F1 SQLite's pool | yes | set aside in one piece at start-up (main.c), handed to usqlite by the Makefile's `-Dm_malloc_maybe(n)=cmm2_sqlite_pool_take(n)` for usqlite_mem.c only (lib/usqlite is a submodule: untouched). The prompt now shows 3.25 MB free, not 5.39 | ch28/01 and Northwind after chapters 15-27 (largest free piece 650 KB) |
| F2 `import ulab.numpy` | yes | `MICROPY_MODULE_BUILTIN_SUBPACKAGES 1` (MicroPython enables it only from FULL_FEATURES; rp2 and this port are EXTRA) | ch31/03, 04, 05 |
| F3 pins changing owner | yes | one owner per GP (machine_pin_cmm2.c): a claim releases the old owner (Pin unconfigured, port or bus closed, PWM stopped); UART, I2C, SPI and PWM one object per port, bus or output, as rp2; all released at a soft reset (the Pin table's stale pointers after Ctrl-D, found on the way, gone too); "OSError: 16" now "GPn is in use" | ch32/02 then ch33/02 then ch32/02 then ch33/03; two `I2C(0)` the same object; PWM after `Pin(2)` |
| F4 file handles | yes | on "too many open files" (files and directories) a collection, whose finalisers close what was dropped, then once more | ch36/03: the whole suite runs, 256 pass |
| M1 clock helpers | yes | `modules/ds3231.py` (the PC3's names: time through DATE$/TIME$, read from the firmware's clock), `gettime`/`settime`/`synctime` in the REPL; `machine.RTC` (machine_rtc_cmm2.c) | ch15/08, ch29/01, ch29/04, ch36/02 |
| M2 alarm | yes | services version 21 `rtc_alarm`: the DS3231's alarm 1 with OPTION DS3231 ON, else the H743 RTC's alarm A (this G2 has no DS3231: MM.INFO(RTC) 0); `alarm_pin()` polls the flag every 250 ms | ch29/05; ch34/02's alarm a minute ahead fired ("WAKE UP") |
| M3 asyncio | yes | extmod/asyncio frozen | ch34/04 |
| M4 machine.WDT | yes | MMBasic's WATCHDOG (machine_wdt.c); `machine.reset()` stops it | fed and stopped; ch34/03 unfed restarted the board ("Watchdog timeout") |
| M5 native, viper, asm_thumb | yes | Thumb emitters, `MP_PLAT_COMMIT_EXEC` (D-cache written back, I-cache emptied) and `MICROPY_MAKE_POINTER_CALLABLE` (the first try faulted: INVSTATE, no Thumb bit) | ch35/02 (native 2.4x plain), a viper and an asm_thumb function |
| M7 network | yes | firmware: services version 21 (Src/net/LoaderNet.c: net_poll/join/leave/ifconfig/dns, eight TCP/UDP handles, closed at the program's end); port: `socket` (modsocket_cmm2.c, the stream protocol), `network.WLAN` (modules/network.py), `tls` on mbedTLS (the H743's RNG for entropy), `machine.RTC`, pcnet unchanged, bundle-networking (mip, ntptime, requests) and umqtt frozen. Needs V8, the Wi-Fi module and OPTION WIFI (which starts the Wi-Fi at power-up) | WLAN, DNS, TCP; ch30/03 (NTP), 04 and 05 (HTTPS), 06 (the weather app), 07 (MQTT: heard its own hello back from test.mosquitto.org); ch34/05; `mip.install` of uQR from GitHub; ch36/01 |

New on the way:
- **B5 (the PC3's pccursor.py, not the book):** in RGB1024 it packs the 4-bit sprite with the left pixel in the low
  nibble, but hdmi.c's GS4_HMSB has it in the high one: the arrow is drawn one pixel off in each pair. The PC3's suite
  skips these checks when a mouse is plugged in, which is why it never showed; the G2 has none (5 cursor checks fail).
- **B6 (the PC3's _boot_board.py, not the book):** `auto` is never put in the REPL (only wifi, ntpsync, tz), so the
  book's `auto(True)` (ch30/03, ch36) is a NameError on a PC3. The CMM2's _boot.py has it.
- **B7 (the book, ch30/04):** `requests.get("https://api.github.com")` gets 403: GitHub refuses a request with no
  User-Agent header ("Request forbidden by administrative rules"), which micropython-lib's requests does not send. So
  the book's "200" will not come on a PC3 either; `requests.get(url, headers={"User-Agent": "pc3"})` gets 200.
- A new `cmm2` module: `basic(statement)` and `evaluate(expression)` run MMBasic's own commands and functions from
  Python (the loader's basic_cmd and basic_eval), with `alarm()` and the net calls for the modules above.
- **Firmware: 8.00.00b2 on v8-dev plus services version 21** (Inc/cmm2img.h, Loader.c, Src/net/LoaderNet.c,
  Inc/net/LoaderNet.h), flashed on the G2 on COM10 only. micropython.app now needs it (`--api 21`): on older firmware
  it says "Needs a newer firmware". The G2's flash: the verify reported a read error, so the whole flash was read back
  and compared: it matched the image but for what the first start writes itself (the options sector and main.c's
  `initoptions` word). Its options were put back from A:/g2options.opt, the same as before.
- The app is 1.40 MB (was 1.26), and 3.25 MB of heap is free at the prompt (SQLite's 2 MB set aside).

## The book's bugs: confirmed on a PC2, fixed, retested (17 November)

Peter put a Pico Computer 2 on COM39 (the PC3 firmware, MicroPython 1.29 built 3 Sep 2026, no Wi-Fi, no mouse).

**Confirmed on the PC2 before the fixes:** B1 `framebuffer not created`; B2 `can't convert float to int` once B1 is
fixed; B3 `mktime needs a tuple of length 8 or 9` (and `gettime()` gives 6 values); B4 `no module named
'ulab.numpy'`; B5 the PC3's own test_cursor.py: 55 pass, the same 5 RGB1024 checks fail as on the CMM2; B6 `auto` not
in the REPL; B7 from the PC: api.github.com answers 403 with no User-Agent and 200 with one.

**The example files matched the book before any change:** `extract_examples.py` run on a copy of the book folder gave
files identical to the committed ones (README, lib/ and northwind/ too); all 277 Python listings of the chapters are
in the built HTML and EPUB word for word; in the PDF (pdftotext -layout) 172 of the 173 files are there line for line,
with only page headers and figure captions between lines at page breaks; the 173rd, lib/handy.py, is two listings
joined by design (chapter 13's and chapter 18's held()), each found.

**Fixed** in a worktree of `pico-computer-3` (`~/src/mp-pc3`, uncommitted), copied into the main tree (branch `cmm2`):
- B1, B2: ch25's adventure: `hdmi.close("F")` and `hdmi.create()` before `console("none")`, as chapters 17-24 have
  them; `tm.view(int(vx), int(vy))`.
- B3: ch29: `time.mktime(gettime() + (0, 0))`, with a comment on the eight values.
- B4: ch31 (two listings): `from ulab import numpy as np`; the same in comments in pcmath.py and _boot_board.py.
- B5: pccursor.py: the 4-bit sprite's even (left) pixel in the high nibble (GS4_HMSB, as hdmi.c reads and blits).
- B6: _boot_board.py: `__main__.auto = pcnet.auto`.
- B7: ch30/04: `headers={"User-Agent": "pc3"}`, with a comment.
- The examples regenerated with `extract_examples.py` (5 programs and the README's line counts changed), and the
  book rebuilt with its Makefile (HTML, EPUB, PDF, 29 s; also copied over the 7 Sep builds in the main tree). The
  examples still match the rebuilt book in all four forms, by the same checks.

**Retested after the fixes:**

| | CMM2 G2 (COM10) | PC2 (COM39) |
|---|---|---|
| B1, B2 ch25/01 then 02 (the adventure) | runs until stopped | runs until stopped |
| B3 ch29/02 | 15/02/2027 | 06/04/2000 (its clock was never set) |
| B4 ch31/03, 04, 05 (the maths lab) | all run | all run |
| B5 the cursor | the PC3's whole suite: 261 pass, 0 fail (pccursor.py frozen in the rebuilt app) | test_cursor.py: 60 pass, 0 fail (the fixed pccursor.py loaded from /btest in place of the frozen one) |
| B6 `auto` | in the CMM2's own _boot.py | needs PC3 firmware built with the fixed _boot_board.py |
| B7 ch30/04 | 200 | no Wi-Fi on a PC2 |

On the PC2 the test files are in `/btest` (the adventure in three forms, the listings, test_cursor.py, testutil.py,
the fixed pccursor.py and tiles2.bmp).

## Pins as on a PC3 (17 November)

Peter agreed all three proposals (the SD LED is visible on the front), and the Wi-Fi fallback for GP21.

- **Firmware (v8-dev, uncommitted):** External.c's `GPPin[]` gives GP40-46 as second names (GP40-42 the pins of
  GP26-28, GP43-46 those of GP6-9); `PinGP` keeps a pin's first name; the per-port GP lists have room for both.
  Services version 22: `led` (the SD LED, put out at a program's end), `gp_pin`, and `pin_setup`'s pull for an
  open-drain output. The user manual (I/O chapter) and the loadable programs' guide say so. Flashed on the COM10 G2
  (verified at once this time; options the same after).
- **Port:** `Pin("LED")`; pins' owners kept by connector pin (GP40 and GP26 are one: taking one releases the other);
  `ADC(n)` is GP40+n as on the RP2350B; `I2C(n)` on other pins is a SoftI2C (lines pulled up), and falls back to the
  bus's own pins when MMBasic holds one of them (GP21 is the Wi-Fi module's). App `--api 22`.
- **Checked on the G2:** the LED reads back 1, 0, 1 for on, off, toggle, and ch08/02 runs; `ADC(0)` is GP40,
  `ADC(3)` GP43, both read; `Pin(26)` let go when `ADC(Pin(40))` took the pin, and took it back; ch32/03 and ch32/06
  run; I2C on GP20/21 falls back to pins 3/5 on this Wi-Fi G2 ("GP21 is in use" first), and ch32/05 prints
  "devices answering: []" (nothing wired); a SoftI2C on GP20 and GP13 (both free) runs with both lines reading 1 (the
  pull-ups); ch32/01, 02, 04, ch33/02 and ch34/01 still run; at MMBasic's prompt `SETPIN GP40, AIN` reads pin 26,
  `MM.INFO(PINNO GP40)` is 26, GP47 is still not available.

## For the rider: what stays different on a CMM2

- **Starting:**
  - Type `RUN "micropython.app"` at MMBasic's prompt, with the app on the SD card. `machine.reset()` goes back to
    MMBasic.
  - `boot.py` and `main.py` run at start-up as on a PC3.
  - It has been tested on the G1 and G2 (the G3 is a G2 with HDMI). A G1 has less memory: about 2.3 MB of heap and
    a 512 KB SQLite pool, against 5.4 MB and 2 MB.
- **One card for everything:**
  - The PC3's flash drive `/` and its SD card `/sd` are both the CMM2's `A:`, and `/usb` is `C:`.
  - Programs that say `/sd/...` work unchanged. ch36/02's backup copies `A:/*.py` into a folder on the same card.
- **The LED:** `Pin("LED")` is the SD card's activity light on the front (card access lights it too).
- **Analogue inputs:** GP40 to GP46 work as on a PC3. They are second names for the CMM2's analog pins: GP40 is pin
  26 (the same pin as GP26), GP41 pin 29, GP42 pin 37, GP43-46 pins 7, 13, 15 and 24. Chapter 32's knob on GP40 goes
  to pin 26.
- **I2C on GP20/21** (chapter 32, the QWIIC devices): a software I2C on pins 21 (GP20, SDA) and 33 (GP21, SCL), with
  the pins' pull-ups on. On a CMM2 with the Wi-Fi module, pin 33 is the module's: there `I2C(0)` uses the CMM2's own
  I2C, pins 3 (SDA) and 5 (SCL), so wire I2C devices there.
- **GP numbers are the PicoMite's.** Each GP is on the CMM2 connector pin that has that function:
  - UART(0) is GP0/1 (pins 8 and 10) and UART(1) is GP36/37.
  - PWM is on GP2-5.
  - SPI(0) is GP18-20 and SPI(1) is GP10-12.
  - The book's pins: GP0 is pin 8, GP1 pin 10, GP2 pin 12, GP20 pin 21, GP21 pin 33, GP26 and GP40 pin 26.
  - The PC3's header pins GP34, GP35, GP38 and GP39 have no CMM2 pin.
  - The rider needs the full table: the user manual's I/O chapter (GP names) and External.c's `GPPin[]`.
- **Warning: GP10 is connector pin 40, the factory-reset pin.** Held low at power-up, it wipes the saved program
  and every option. Don't wire a button or anything else that can hold it low.
- **Wi-Fi:** a CMM2 with the Wi-Fi module and V8 firmware, with `OPTION WIFI ssid$, password$` set once at MMBasic's
  prompt (it joins at power-up). Then chapter 30 works as written: `wifi()`, `ntpsync()`, `tz()`, `auto()`, requests
  over HTTPS, MQTT. `wifi("SSID", "password")` joins another network for the session without changing OPTION WIFI.
- **Add-on libraries:** `mip.install` works with the Wi-Fi. Without it, copy the `.py` files into `A:/lib`; `uQR` and
  `micropyGPS` both work.
- **File transfer:** until M8, use MMBasic's XMODEM or ZMODEM at its prompt before starting the app.
- **Screen modes:**
  - RGB640 and RGB640_4 are MODE 8,8; RGB320_8 is MODE 7,8; RGB320 is MODE 7,16. These run at 75 Hz on a CMM2.
  - RGB512 and RGB1024 (1024x600) are MODE 18.
  - The pictures are the same; a VGA monitor sees other frame rates.
- **Not there yet:** touch and gamepads (M6, M9) and XMODEM at the prompt (M8). The mouse works.
- **The clock:** `settime()` and `gettime()` work on the CMM2's own clock: the H743's battery-backed RTC, and a DS3231
  too where one is fitted (OPTION DS3231 ON). The daily alarm (`ds3231.set_alarm`) works on either; `alarm_pin()` is
  polled, as the INT line is not on the connector.
- **The watchdog:** `machine.WDT` restarts the board into MMBasic when it is not fed, as a PC3 restarts.
