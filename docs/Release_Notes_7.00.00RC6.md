# Colour Maximite 2: MMBasic 7.00.00RC6 (release candidate) release notes

*13 November 2026. Compared with 7.00.00RC5, the last version released. The full notes, from 5.07.02, are in Release_Notes_7.00.00.md.*

This is the seventh release candidate of MMBasic 7.00 for the Colour Maximite 2. Unless problems are found in it, it will become 7.00.00. Please report problems on the Back Shed forum.

It adds SQL databases to BASIC: SQLite, the database engine, through a new command SQL and a new function SQL(, with the rows of a table going into and out of variables, arrays and structures.

The same firmware file runs on both the Generation 1 and the Generation 2.

## Before you update

**Writing the firmware erases the options.** Save them first, and load them again afterwards:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

The console runs at 115200 baud until the options are loaded. The library, the variables saved with VAR SAVE and the files on the SD card are kept.

Copy the new CMM2_Programs_7.00.00RC6.zip to the card as well: the sql folder is new, and SQL needs it.

## What is new

- **SQL databases.** A database is a file on the SD card or USB flash drive, which a program opens with SQL OPEN and works on with statements in SQL, the standard language for databases:

  ```
  SQL OPEN "A:/data/club.db"
  SQL EXEC "CREATE TABLE IF NOT EXISTS member(id INTEGER PRIMARY KEY, name TEXT, born INTEGER)"
  SQL EXEC "INSERT INTO member(name, born) VALUES(?, ?)", n$, year%
  SQL QUERY "SELECT name, born FROM member WHERE born > ? ORDER BY born", 1950
  DO WHILE SQL(NEXT)
    PRINT SQL(1), SQL(2)
  LOOP
  SQL CLOSE
  ```

  - Values from the program go into a statement by its ? parameters, so a string needs no quoting.
  - SQL(n) gives a column as an integer, a float or a string, as the column holds it; SQL(SELECT puts a query's rows into an array; SQL(COLUMNS), SQL(NAME n), SQL(ISNULL n), SQL(ROWS) and SQL(ID) tell the rest.
  - Rows go into and out of MMBasic structures by matching the members' names with the columns': SQL CREATE makes a table from a TYPE, SQL INSERT adds a structure or a whole array of them (giving each its new key), SQL(FETCH reads a row into a structure, and a structure among a statement's values gives its :name parameters.
  - Errors are MMBasic errors, "SQL: " with the database's own message, so ON ERROR SKIP catches them; Ctrl-C stops a long statement.
  - The database engine, SQLite 3.47.0, is not in the firmware: it is the module A:/sql/sql.mod in the zip's sql folder, which SQL OPEN loads and SQL CLOSE unloads. On a **Generation 2** it runs in the memory kept for loadable programs, with about 7MB for it, and BASIC keeps all of its own memory; a BASIC program with a database open cannot run a loadable program (.app) until it closes it. On a **Generation 1** it has 1MB, taken from BASIC's memory while the database is open: fast with databases up to about 1MB, **but slow with large ones**, where whole-table work (unindexed sorts, GROUP BY, DISTINCT, big joins) can take minutes. The manual's new chapter, SQL Databases, explains.
  - The sql folder has a sample database of home computers, computers.db, and the chapter's example program for it, computers.bas, which reads its rows into a TYPE Computer. It also has sql.app, which runs SQL statements (or a file of them) at the command prompt: RUN "A:/sql/sql.app", A:/sql/computers.db "SELECT name, year FROM computer".

- **For programmers:**
  - Version 19 of the services: file_truncate, a file cut short at its position, and the tools' ftruncate() with it.
  - Modules: build.py --module makes an image the firmware loads beside a BASIC program and calls (the SQL module is the one there is: the firmware loads modules for its own commands only).
  - The tools (tools/sdk), since RC5: errno reaches programs, a steady clock and POSIX's clocks, --libc full (the whole C library, for its wide-character functions), getcwd(NULL, 0) allocates, and readdir gives "." as well as "..".

## What behaves differently

- A loadable program that sets a screen mode now leaves BASIC's screen clear when it ends, in BASIC's own mode too, as a MODE would. Before, its last picture could stay behind in BASIC's colours.
- In BASIC translated by the C compiler (CC), MODE with one number is the Colour Maximite 2's own, as in the interpreter: MODE 9 used to stop the program, and MODE 1 gave 640 x 480 in one bit rather than 800 x 600.
- The manual now says exactly what a single line IF with ELSE does: one statement after THEN, one after ELSE, and any statement after that on the line runs either way. MMBasic itself is unchanged.
- The new function SQL( takes the name from programs: an old program with an array named SQL() must rename it.
- MM.VER is 7.000056 for RC6.

## The programs

- **sql** is new: sql.mod (the SQL command's module), sql.app, computers.db and computers.bas.
- **bbc** and **wordgrinder** are as the RC5 zip has had them since 13 November: bbc.app no longer stops about 18 minutes after it starts, and WordGrinder's message when its memory is damaged stays on the screen until a key is pressed. The zip downloaded before then had the first versions.
- Every other program in the zip is unchanged.

## Tested

On a Generation 2 and a Generation 1, with this firmware and the zip's sql folder:
- SQL: a test of the commands and functions (rows into variables, string and integer arrays and arrays of structures, structures inserted with their new keys, FETCH, :name values, NULLs, errors caught by ON ERROR SKIP, CLOSE and OPEN again), all passed on both; a thousand inserts in one transaction 0.32 seconds on the Generation 2 and 0.38 on the Generation 1, one committed on its own 17 and 23 milliseconds.
- computers.bas on the sample database, and sql.app.
- A loadable program run from a BASIC program with a database open: on the Generation 2 the error "SQL CLOSE first"; on the Generation 1 it runs in the memory left, and sql.app, which needs more, did not start ("Needs 1608 KB"). After SQL CLOSE it ran on both.
- The single line IF with ELSE, as the manual now describes it.

On a Generation 2 (the same SQL in a build of the firmware before this one):
- The database left open by a program, at the prompt.
- SQL on a table of 50,000 rows (6.5MB, with a 6MB index): PRAGMA integrity_check 32 seconds, count(DISTINCT) 34 seconds, CREATE INDEX 21 seconds; a thousand inserts in one transaction from BASIC 0.33 seconds, one committed on its own 18 milliseconds. A rollback that made a database longer cuts it back. A crash in a transaction is undone at the next open (tried on a PC with the same engine and settings).
- SQL with a Generation 1's 1MB (on a Generation 2 given that memory): the same 50,000 rows work, slowly: integrity_check 160 seconds, count(DISTINCT) 460 seconds, CREATE INDEX 23 seconds.

## Known problems

- **SQL:** on a Generation 1 a large database is slow (above). Only one database is open at a time (ATTACH reaches others through it). A statement is a string, so at most 255 characters. Stopping a statement with Ctrl-C has not been tried by hand.
- The known problems of 7.00.00RC5 remain.

## The manual

The User Manual has a new chapter, SQL Databases, with SQL and SQL( in its Commands and Functions, and says exactly what a single line IF with ELSE does. Colour_Maximite_2_Loadable_Programs.pdf adds services version 19 and build.py's --module, and Colour_Maximite_2_C_Compiler.pdf lists an array of a TYPE whose size can change (REDIM, or bounds that are not constants) among what the compiler refuses.

## Licences

SQLite is in the public domain. The SQL module's settings follow usqlite's (Elvin Slavik), under the MIT licence; the module and the SQL command are the firmware's own. The licences folder of the zip has them, and its README lists every program's licence.
