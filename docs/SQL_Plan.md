# SQL for MMBasic on the Colour Maximite 2

*13 November 2026. A plan for giving BASIC programs an SQL database: SQLite, as the MicroPython port already has it,
loaded from the SD card and called from BASIC through an `SQL` command and one `SQL(` function. The facts below were
checked on 13 Nov against main 1468d08, v8-dev 21d5f8a and the MicroPython port at 096da75c5. Peter decided Q1-Q5 as
recommended and deferred Q6 the same day; the work is on branch `sql`, from v8-dev ("Progress", at the end).*

## Summary

- **What carries over from MicroPython:** SQLite 3.47.0 itself (public domain) and usqlite's build settings. Not its
  Python layer, and not its file layer, which goes through MicroPython's file objects.
- **How BASIC reaches it:** as a **module**, the kind of loadable program the loader plan designed beside applications
  (`docs/Loader_Plan.md`, Part 1 "Modules") and never built. A module stays loaded beside the BASIC program, is
  called with its arguments as a CSUB gets them, may raise BASIC errors, and is freed when the program ends.
- **Running SQLite as an application once per statement**, as `CC` runs cc.app, works with today's loader but is
  only good for scripts and reports (Route A below). It is the first step all the same: the same code becomes the
  module.
- **Structures map onto tables:** a `TYPE` is a fixed record with named members, so rows go into and out of
  structure variables and arrays by matching member names to column names.
- **One function token is left**, byte 0xFE. `SQL(` takes it, as one token for every value SQL gives back.
- **V8 work:** main's bank 1 has about 7.5 KB free (4a97126), not enough for the module loader.

## What exists: SQLite in the MicroPython port

In Peter's MicroPython repository (WSL `~/src/micropython`, branch `cmm2`, `lib/usqlite`, from 96519cb6e on 2 Nov):

| Part | Lines or size | Carries over? |
|---|---|---|
| `sqlite3.c`, SQLite 3.47.0 amalgamation | 522 KB of code compiled (`usqlite.o`), 6 KB data | Yes, unchanged |
| `usqlite_config.h`, the build settings | | Yes, with the changes below |
| `usqlite_vfs.c`, SQLite's OS layer | 387 lines, 465 bytes of code | Its shape: it calls `usqlite_file_*` |
| `usqlite_file.c`, the files | 386 lines, through MicroPython streams | No: rewritten over the SDK's `open`, `read`, `write`, `lseek`, `fsync`, `unlink`, `stat` |
| `usqlite_mem.c`, the memory pool | 151 lines | Yes (one MEMSYS5 pool) |
| connection, cursor, row, module | about 5 KB of code | No: replaced by the BASIC front end |

**Settings kept:** `SQLITE_OS_OTHER`, `SQLITE_THREADSAFE 0`, no WAL (rollback journals), `SQLITE_DEFAULT_SYNCHRONOUS 2`
(full sync at each commit), exclusive locking, MEMSYS5 pool: 2 MB on a G2, 512 KB on a G1 (`ports/cmm2/cmm2_sqlite.h`),
temporary files on the card.

**Settings to change:**
- **`SQLITE_OMIT_PROGRESS_CALLBACK` out**, so that Ctrl-C can stop a long query (`sqlite3_interrupt` from the
  progress callback when MMBasic's break flag is set).
- **Truncating.** usqlite truncates a file to zero only (`usqlite_file_truncate0` reopens it with `w+b`), and the SDK
  has no `ftruncate`: there is no file-truncate service. SQLite cuts a database back when a transaction that made it
  longer is rolled back. A small service over FatFs's `f_truncate` (`file_truncate(h)`, at the current position)
  makes that right; without it the file keeps the extra pages, which SQLite tolerates but never reclaims.

## Two routes

### Route A: an application run for each statement (no loader change)

`cmd_cc` (`Src/Loader.c`) runs `A:/cc/cc.app` through `ld_app` each time. An `SQL` command could run `A:/sql/sql.app`
the same way:

```
SQL "club.db", "INSERT INTO member VALUES('Ann', 1961)"
SQL "club.db", "SELECT name, born FROM member", rows$()
```

- **Results back to BASIC:** the program writes `rows$(i,j)="..."` statements through `basic_cmd` (about 85 us for a
  short statement, measured for PAGE WRITE; CHR$(34) for quotes inside values; numbers as text), or a CSV file.
- **Cost:** each statement loads a 550-600 KB application and opens the database again, and is its own transaction:
  a journal created, synced and deleted on FAT. Fine for a script of many statements in one call; unusable for a
  loop that inserts a row at a time. Not measured yet (step 1).
- **Sound:** `ld_app` calls `CloseAudio(1)` first, so BASIC's music stops at each call. A variant would have to keep
  it and have the program call `poll()` (whose `routinechecks` refills the players) often enough.
- **Structures:** not workable: the application cannot see the TYPE table or the variables except through
  `basic_eval` and `basic_cmd`, a member at a time.

### Route B: a module (recommended)

- **Loaded** by the first `SQL OPEN` from `A:/sql/` (as `CC` finds cc.app in `LD_CC_DIR`), and kept.
- **Called** with the arguments as a CSUB gets them: the firmware's `SQL` command parses the line, finds the variables
  (and for a structure its TYPE in `g_structtbl`), and hands the module pointers and a list of members (name, type,
  offset, size). The module never parses BASIC.
- **The database stays open** between statements, so `BEGIN` ... `COMMIT` around a loop gives bulk inserts at
  SQLite's speed, not one journal a row.
- **Values from BASIC by `?` and `:name` placeholders**, bound as values: no quoting, and no SQL injection from text a
  user typed.
- **Errors are BASIC errors** (`error()`, as a CSUB may raise them), so `ON ERROR SKIP` and `MM.ERRNO` work. Before
  raising one the module resets or finalises the statement, so that SQLite is never left half way.
- **Freed** when the program ends (Loader_Plan's D5), at NEW and at RUN: an end-of-program hook closes the database
  (an open transaction rolls back; a hot journal would roll it back at the next open anyway).
- **Where it lives:** the module's code (about 560 KB) and its pool (2 MB, 512 KB on a G1) in a block of MMBasic's
  heap, on both boards, as a G1 already places applications, with that block executable while BASIC runs. Not the
  G2's 8 MB application area: a BASIC program that uses SQL and also RUNs an application would collide. Loader_Plan's
  512 KB executable window is too small.
- **Firmware work:** the module kind in the loader (image kind 2, the export table, the hooks, placing and freeing),
  the `SQL` command's argument and structure handling, and `SQL(`.

## BASIC syntax (a first draft)

```
SQL OPEN "A:/data/club.db"               ' made if missing; one database open at a time to begin with
SQL EXEC "CREATE TABLE IF NOT EXISTS member(id INTEGER PRIMARY KEY, name TEXT, born INTEGER)"
SQL EXEC "INSERT INTO member(name, born) VALUES(?, ?)", n$, y%
SQL QUERY "SELECT name, born FROM member WHERE born > ? ORDER BY name", 1950
DO WHILE SQL(NEXT)
  PRINT SQL(1), SQL(2)                   ' a column's value, string or number as the column holds it
LOOP
n% = SQL(SELECT "SELECT name FROM member", names$())   ' a whole result into an array: the row count
SQL EXEC FILE "setup.sql"                ' a file of statements
SQL CLOSE
```

- A statement longer than 255 characters comes from a LONGSTRING or a file.
- `SQL(ROWS)` (rows changed by the last statement), `SQL(ID)` (the last inserted key), `SQL(ISNULL n)`,
  `SQL(COLUMNS)`, `SQL(NAME n)`.

## Structures and tables

An MMBasic structure (`TYPE ... END TYPE`, `Src/Struct.c`) is a fixed record: each member has a name (in capitals),
a type (`T_INT`, `T_NBR`, `T_STR` or a nested `T_STRUCT`), an offset, a size and optional dimensions
(`struct s_structmember`, `Inc/MMBasic.h`), and an array of structures is a contiguous block of records. That is a
table row. The firmware knows every layout, so the `SQL` command can resolve a structure and give the module its
members.

| MMBasic member | SQLite column | Notes |
|---|---|---|
| `INTEGER` | `INTEGER` | Both 64-bit. |
| `FLOAT` | `REAL` | Both doubles. |
| `STRING LENGTH n` | `TEXT` | A longer value read back is "String too long" (or cut, by an option). |
| A nested structure | Flattened columns, `ADDR_CITY` | One table, not a join. |
| A member array, `score(5)` | `SCORE_0` ... `SCORE_5`, or one BLOB or JSON value | Columns while it is small. |
| The type's name | The default table name | `TYPE Member` is table `MEMBER`. |
| A member `ID AS INTEGER` | `INTEGER PRIMARY KEY` | 0 on insert: SQLite chooses the key and it is written back. |

```
TYPE Member
  id AS INTEGER
  name AS STRING LENGTH 40
  born AS INTEGER
END TYPE
DIM m AS Member, list(100) AS Member

SQL CREATE Member                        ' CREATE TABLE IF NOT EXISTS from the TYPE
m.name = "Ann" : m.born = 1961
SQL INSERT m                             ' m.id now holds the new key
SQL INSERT list()                        ' a whole array, in one transaction
n% = SQL(SELECT "SELECT * FROM member WHERE born > ?", list(), 1950)
SQL EXEC "UPDATE member SET born = :born WHERE id = :id", m    ' named parameters from the members
SQL QUERY "SELECT * FROM member ORDER BY name"
DO WHILE SQL(FETCH m)                    ' a row at a time, any number of rows
  PRINT m.name, m.born
LOOP
```

- **By name:** a column with no member of its name is ignored; a member with no column is cleared, as STRUCT CLEAR
  clears it. A structure given to `SQL EXEC` binds its members to the `:name` parameters.
- **`SQL DESCRIBE "member"`** prints a `TYPE ... END TYPE` for an existing table, to paste into a program; a TYPE that
  has gained a member can add the missing column (`ALTER TABLE ... ADD COLUMN`), by an option.
- **Against STRUCT SAVE and LOAD:** they write raw bytes, so a file breaks when the TYPE changes; by name it keeps
  working when members are added or reordered, and a database adds indexes, queries over more than memory holds, and
  joins.

**What is awkward:**
- **NULL:** BASIC has none. NULL reads as 0 or "", and `SQL(ISNULL n)` tells them apart for the cursor's columns.
- **Limits:** 16 members a TYPE (`MAX_STRUCT_MEMBERS`, `Inc/Configuration.h`), 32 types, strings of 255 characters at
  most. Wider tables need nested types; long text a LONGSTRING.
- **Types:** SQLite does not enforce a column's type. A text value read into an INTEGER member takes SQLite's own
  conversion; a strict option could make it an error.

## The function token

Function tokens take one byte, 0x80 upward (`C_BASETOKEN`, `tokenfunction()` in `Inc/MMBasic.h`). On main and on
v8-dev the token table (`INCLUDE_TOKEN_TABLE` in `Inc/allcommands.h`) has 126 entries, 0x80-0xFD, and its end marker
at 0xFE:

- **0xFE is the one byte left.** An entry added just before the end marker takes it, and the marker moves to 0xFF,
  which is never written (its name is empty). Added at the end, no existing token changes number, so tokenised
  programs in the flash slots and the library stay valid.
- **0xFF itself stays unused:** END TYPE's two-byte command token starts with it, and line scans have tripped on it
  (`docs/Ray_Plan.md`, "Tokens"); erased flash is 0xFF too.
- **It is the last one.** After it, a new function has to be a sub-function of an existing token (MATH(, DEVICE(,
  MM.INFO(), folded by the tokeniser as `BIN$(` becomes `BASE$(2,` (FoldFrom and FoldTo in `Src/MMBasic.c`), or
  function tokens go to two bytes as commands did: a large change to every image walker, the compiler and the
  translator.
- **So `SQL(` is one token for everything:** `SQL(NEXT)`, `SQL(FETCH m)`, `SQL(SELECT ...)`, `SQL(ROWS)`, `SQL(n)`,
  returning an integer, a float or a string by sub-function and column, as DEVICE( already returns all three
  (`T_FUN | T_INT | T_NBR | T_STR`). No `SQL$(`.
- **Without the token:** the values could come through `MM.INFO(SQL ...)` and the commands alone, but a fetch with
  side effects inside MM.INFO reads badly in a loop. Decision Q2.
- **Name clash:** a function name wins over an array of the same name, so an old program with an array `SQL()` would
  stop working. `SQL` as a variable or a command name is fine.

## Decisions

Peter, 13 Nov 2026: "q1-q5 as recommended, q6 defer".

| | Decision | Decided |
|---|---|---|
| Q1 | Route A only, or B | B, with A's application as step 1 |
| Q2 | The last function token for `SQL(` | Yes, one token for every value |
| Q3 | The file-truncate service | Yes: a few lines over `f_truncate` |
| Q4 | Structures in the first version, or after | In it: it is what makes SQL feel like part of BASIC |
| Q5 | One database open at a time, or several (ATTACH works within one) | One, with ATTACH |
| Q6 | The SQLite shell as an application too (`RUN "sqlite3.app", club.db`) | Deferred |

## Steps

1. **SQLite as an application** (no firmware change): `apps/sql/make.py` builds `sqlite3.c` with the settings above
   and a file layer over the SDK, and a small front end that runs a statement or a file of them and prints the
   results. Measure on both boards: the time to load and open, a commit on the card, a thousand inserts in one
   transaction and in a thousand. Optionally SQLite's own shell (`shell.c`) the same way.
2. **The file-truncate service** (if Q3), and SQLite's tests that roll back a grown file.
3. **Modules in the V8 loader:** image kind 2, the export table, placing in the heap and executable, the hooks, freeing
   at the end of the program, NEW and RUN; a tiny test module first.
4. **The `SQL` command and `SQL(`:** OPEN, EXEC, QUERY, NEXT, SELECT into arrays, CLOSE, errors, Ctrl-C.
5. **Structures:** CREATE, INSERT, SELECT and FETCH into structures, `:name` parameters, DESCRIBE.
6. **Documentation:** a chapter in the user manual, the help entries, the release notes.

## Progress

**Step 1, 13 Nov 2026: done: built, tried on the PC and on both boards.** `apps/sql`:
- `make.py` fetches the 3.47.0 amalgamation from sqlite.org (the zip's SHA-256 checked; its `sqlite3.c` and `sqlite3.h`
  are byte for byte the MicroPython port's) and builds `out/sql.app`: 545,004 bytes (532 KB of image), services
  version 10, so it runs on RC5 and V8.
- `sqlite_cmm2.h` (usqlite's settings, the progress callback kept), `sqlite_unit.c` (the amalgamation compiled as
  usqlite compiles it: an OMIT option leaves the parser calling `sqlite3Analyze`, so ANALYZE is now an error, "ANALYZE
  is not available", where usqlite's stub did nothing), `sqlcore.c` (the VFS on the services, the pool, Ctrl-C) and
  `sql_app.c` (statements, `@file`, `-h`, `-t`, `-log`, `-bench`).
- **Tried on the PC** (scratchpad `host_sql`: the same files built in WSL over a stand-in for the services that keeps
  FatFs's ways): 17 of 17. Rows in and out, a rollback of a transaction that made the file longer, a crash in a
  transaction (the process killed) and the journal rolling it back at the next open, a 200,000-row sort and an index
  over 26 MB with temporary files (all removed after), ATTACH, a file of statements, names after the current
  directory and on the current drive, errors, `-bench`; never more than 2 files open.

**Found on the way:**
- **The sorter's least memory is about 1 MB**, whatever the cache: it keeps rows until it has the larger of the cache
  and 250 pages (`SQLITE_SORTER_PMASZ`). In a 2 MB buddy pool a free 1 MB block is seldom there, and a 512 KB pool
  never has one: a sort of 200,000 rows failed "out of memory" on the PC. `sqlcore_init` sets `SQLITE_CONFIG_PMASZ` to a
  sixteenth of the pool. The MicroPython port has the same 2 MB pool and the same floor, so its big sorts are likely to
  fail the same way (not tried there).
- **A sort merges with a 4 KB buffer for each piece it wrote**, so the memory a big sort needs grows with the data:
  an index over 26 MB in 64 KB pieces wanted 1.6 MB. MicroPython's 512 KB pool for a G1 is too small for that. The pool
  is 2 MB on both boards (Peter, 13 Nov: on the MicroPython port giving SQLite too much locked it up, so 2 MB to start);
  with it the pieces are 256 KB and the index's buffers 400 KB.
- **Without the truncate service** (step 2) a rolled-back database stays longer: 12 KB became 2.2 MB on the PC after
  20,000 rows were rolled back. SQLite reads its size from the header, so nothing is wrong, but the space stays used.

**On the boards (13 Nov), sql.app (Route A):**

| | G1 (V8) | G2 (V7 RC5) |
|---|---|---|
| A statement from a BASIC program, RUN "sql.app" each time | 529 ms (SELECT), 588 ms (INSERT) | 235 ms, 278 ms |
| An insert committed alone, inside the app | 22 ms | 25 ms |
| 1000 inserts in one transaction, inside the app | 96 ms | 126 ms |
| 1000 rows found by an index | 79 ms | 65 ms |

- A G2's RUN of the 545 KB app costs about 0.23 s before SQLite does anything; a G1's twice that (it relocates the
  program into borrowed heap). Route A is for scripts, as expected.
- **The card's random reads are the limit for big databases**: a 6.5 MB table of 50,000 rows with a 6 MB index on it:
  `PRAGMA integrity_check` 137 s (2 ms a 4 KB page from the SPI card, once the index is bigger than the 256 KB cache),
  `count(DISTINCT v)` 314 s (a temporary b-tree in random order), `CREATE INDEX` 21 s (the sorter: sequential),
  `count(*)` 2.7 s (2.4 MB/s). Databases that fit the cache are fast.
- **A file's cluster map is dropped when it grows** (the loader's file services: FatFs's fast seek cannot make a file
  longer), and without it each seek back follows the FAT from the file's start: the same integrity check took 182 s
  in the run whose CREATE INDEX had made the file longer. Making the map again at the next seek back after a file
  has grown would gain that 25%, for every program that writes and seeks (left for later: an offer).

**Step 2, 13 Nov: done.** `file_truncate` (services version 19: `Inc/cmm2img.h`, `ld_truncate` over FatFs's
`f_truncate`; a file's cluster map stays right for what is left), the SDK's `ftruncate()`, the Loadable Programs
guide (chapters 4, 5 and 6, Appendix A), and `sqlcore.c` cutting a file short with it when the firmware has it. On the
G1: a new database, a transaction of 550 KB of rows rolled back, the file back to 8 KB (2 pages), its integrity
check ok. (A database already longer than its pages stays so: SQLite takes the file's size as the database's when
a transaction starts.)

**Step 3, 13 Nov: built; tried on the G1.**
- **Modules** (`CMM2IMG_KIND_MODULE`, Loader.c): `build.py --module` (crt0.c's `_start` returns `module_main()`'s
  table, a `cmm2mod_head` first); the image relocated into a block of the heap from a 64 KB boundary, its stack (64
  KB) after its .bss, its C library's heap a second block; called through `LoaderModFn`, a ten-instruction trampoline
  onto the module's stack; the heap executable while it is there (`LoaderModExec`: the G1's regions 8 and 9 as for a
  program, the G2's 0, 13 and 14); a module's files kept apart from a program's (an application's end leaves them);
  gone with the heap (`HeapWipe` calls `LoaderModuleForget`: the module's `forget`, its files closed, its blocks freed).
- **The SQL module** `apps/sql/out/mod/sql.mod` (527 KB of image; `Inc/sqlmod.h` its table: SQLite's own functions,
  thin) and **`Src/SQL.c`** (in bank 2's sector: bank 1 had 2 KB left): `SQL OPEN/CLOSE/EXEC/QUERY/CREATE/INSERT`,
  `SQL(NEXT/FETCH/SELECT/n/COLUMNS/NAME/ISNULL/ROWS/ID)`; `SQL(` is the 0xFE token; the command at the command
  table's end. Values bound by place (?), and :name from a structure among them; a NULL read as 0, 0.0 or "" by the
  column's declared type; an SQLite error is "SQL: message", an MMBasic error.
- **On the G1 (V8, 13 Nov): 17 of 17** (`sqlmod.bas`): rows into variables, a 2-D string array and an integer array,
  values bound; a structure inserted (its new ID written back), an array of them in one savepoint, FETCH into a
  structure, SELECT into an array of structures, :name values from a structure; NULL; an error caught by ON ERROR
  SKIP with MM.ERRMSG$; CLOSE and OPEN again. The first SQL OPEN (the module loaded) 504 ms; **1000 inserts in a
  transaction from BASIC 379 ms** (0.38 ms each, against 530 ms a statement by Route A); an insert committed alone
  22 ms.
- **The G1's heap**: 3,285 KB before the first SQL OPEN, 526 KB after (the module 600 KB, its 64 KB boundary, the 2
  MB pool and 64 KB for its C library). A BASIC program with big arrays wants `SQL OPEN f$, 1024` (a 1 MB pool) on
  a G1: to decide.
- **Not a bug** (Peter, 13 Nov): a one-line `IF c THEN a ELSE b : d` runs d either way: a one-line IF takes one
  statement after THEN and one after ELSE, and d is a statement of its own. (The test's check SUB uses a multi-line
  IF.)

**The pool and the cache, on the G2 (13 Nov, Peter: "try setting the pool to 3Mb with 1.5Mb allocated to the cache";
then "carry on with the testing and then we can make decisions on g1/g2 memory usage").** 50,000 rows (6.5 MB) with a
6 MB index, `A:/sql/s.db`; each configuration a program of its own (`SQL OPEN f$, poolKB, cacheKB`):

| Pool, cache | BASIC's heap left | integrity_check | count(DISTINCT k \|\| v) | CREATE INDEX |
|---|---|---|---|---|
| 1 MB, 120 KB | 13.8 MB | 160 s | 461 s | 22.6 s |
| 2 MB, 244 KB (the default) | 12.8 MB | 142 s | 451 s | about 21 s |
| 3 MB, 1.5 MB | 11.8 MB | 79 s | 222 to 275 s | 21 s |
| 8 MB, 6 MB | 6.7 MB | 26 s | 26 s | 21 s |
| 12 MB, 8 MB, `temp_store = MEMORY` | 2.6 MB | 10 s | out of memory | |

- What made a big cache work: the cache in slots of its own (`SQLITE_CONFIG_PAGECACHE`: in the buddy allocator a
  page took 8 KB, and a 1.5 MB cache in a 3 MB pool ran out at once), and the sorter capped at 256 KB (make.py's one
  patch to sqlite3.c: `SQLITE_MAX_PMASZ` guarded; the sorter follows the cache's size, and with 1.5 MB of cache
  CREATE INDEX ran out of memory). Also: a query is ended at its end (and by `SQL CLOSE QUERY`): one read to its
  first row only held its temporary table's memory and its tables (the defaults' CREATE INDEX ran out of memory, and
  a DROP INDEX met "database table is locked").
- No size locked up: what failed failed with "out of memory". The Pico Computer 3's lock-up (a 4 MB pool with half
  of it cache) was the sorter following the cache to the pool's edge: filed with the rest as
  UKTailwind/micropython#2.
- The cache's own slots are shared by the database and a temporary b-tree: with a small cache the DISTINCT took 451 s
  where it took 282 s before them. Nothing with a big cache.
- A 1 MB pool (a G1's) does it all, slowly on big tables: CREATE INDEX 22.6 s.
- `temp_store = MEMORY` is not worth it: the 8 MB cache's DISTINCT spilled to the card in 26 s.

Decisions for Peter: the G2 (the module and a 7 MB pool in the applications' 8 MB, BASIC keeping its 15.5 MB; or a
bigger default in the heap) and the G1 (1 MB in the heap, or no SQL).

**Decided (Peter, 13 Nov 2026): "1Mb on the G1 and g2 in the applications area. The manual will need to include a clear
warning on G1 slowdown."** Built and tried on the G2 the same day:
- **G2**: the module at 0xD1800000, where it is linked (no relocation, the area executable already: no change to the
  heap's MPU regions); its heap all the area leaves (7,248 KB of pool), three quarters of it the cache. BASIC's heap
  untouched: 15,571 KB with a database open as without. The 50,000-row test: integrity_check 31.6 s, count(DISTINCT)
  34.0 s, CREATE INDEX 21.4 s. A BASIC program with a database open cannot run an application ("SQL CLOSE first: the
  database is in the programs' memory", which ON ERROR SKIP catches; the database goes on working); at the prompt a
  program run there closes a database a program left open. 17 of 17 again.
- **G1**: the heap, a 1 MB pool, an eighth of it the cache (not yet tried on a G1 with this build: Peter has the G1).
- **SQL CLOSE unloads the module** on both (its memory back: the area, or the G1's heap); the next OPEN loads it
  again (0.22 s on the G2). An OPEN closes the last database and module first, so its pool and cache take effect.
- **Step 6, the manual** (V8, wt_sql): the chapter SQL Databases (`docs/manual/text/21a_sql.md`, a new Heading 1 before
  Porting Programs; "Generation 1: Smaller and Slower" its second section), SQL and SQL( in the Commands and Functions
  references, a line in the Introduction, the firmware's help (help.h); the one-line IF made explicit, as on main.

