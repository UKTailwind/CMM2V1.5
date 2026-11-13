# SQL Databases


MMBasic can keep data in an SQL database: a file on the SD card or USB flash drive holding tables of rows, which a program adds to, changes, searches and sorts with statements in SQL, the standard language for databases. The database engine is SQLite, the same engine found in phones, browsers and aircraft, and a database file made on the Colour Maximite 2 can be read by SQLite on a PC and the other way round.

SQLite is not part of the firmware. It is a module, the file A:/sql/sql.mod, which MMBasic loads when a program opens a database and unloads when it closes it. It is in the cmm2 programs zip that comes with the firmware: copy its sql folder to the root of the SD card.

The firmware's commands for it are SQL and the function SQL(, described below and in the Commands and Functions sections. This chapter shows how to use them; the SQL language itself is described in many books and on the SQLite web site (sqlite.org/lang.html).

## Generation 1: Smaller and Slower

**On a Generation 1 the database engine has only 1MB of memory, so it is fast only with small databases.** A database of up to about 1MB (roughly 8,000 to 12,000 rows of 100 characters, with one index) works well. Above that, finding a row by its key or an index stays quick (a few milliseconds), but anything that has to look at the whole of a large table (an unindexed sort, GROUP BY, DISTINCT, a join of two big tables, PRAGMA integrity_check) reads the SD card all over again and can take minutes: with a Generation 1's memory, count(DISTINCT) over a 50,000 row table took over seven minutes, where a Generation 2 took 34 seconds. The engine and its memory also take about 1.7MB of the 3.7MB that a Generation 1 has for BASIC's variables and arrays, while the database is open.

A Generation 2 runs the engine in the memory kept for loadable programs, with about 7MB for it, and leaves BASIC all of its own memory.

## Opening and Closing

```
SQL OPEN "A:/data/club.db"
...
SQL CLOSE
```

SQL OPEN opens the database file, and makes it (empty) if it is not there. Only one database is open at a time: opening another closes the one before. A file name without a drive or folder is in the current directory, as with other files. SQL CLOSE closes it, and gives back the memory the engine used.

The database stays open until SQL CLOSE, or until the program is run again (RUN), cleared (NEW) or another one loaded (or CHAIN): each of these closes it, and any transaction not finished is undone. A program that ends without SQL CLOSE leaves it open at the command prompt.

On a Generation 2 the engine uses the memory where loadable programs (.app files) run. A BASIC program that has a database open cannot run one: RUN "name.app" gives the error "SQL CLOSE first". At the command prompt, running a loadable program closes a database a program left open. On a Generation 1 the engine is in BASIC's own memory, so a loadable program can run while a database is open, in the memory that is left; one that needs more does not start ("Needs 1608 KB, 1595 KB free in one block").

SQL OPEN can also be given the engine's memory, in kilobytes, and how much of it to use as the cache of the database's pages: SQL OPEN f$, poolKB, cacheKB. Neither is normally needed.

## Running Statements

SQL EXEC runs SQL statements: one, or several separated by semicolons.

```
SQL EXEC "CREATE TABLE IF NOT EXISTS member(id INTEGER PRIMARY KEY, name TEXT, born INTEGER)"
SQL EXEC "INSERT INTO member(name, born) VALUES('Ann', 1961)"
```

Values from the program go in as parameters: each ? in the statement is replaced by the next value after it, a number or a string. There is then no need to put a string's quotes into the statement, and no harm in a value that has quotes in it.

```
SQL EXEC "INSERT INTO member(name, born) VALUES(?, ?)", n$, year%
SQL EXEC "UPDATE member SET born = ? WHERE name = ?", 1962, "Ann"
```

SQL(ROWS) is the number of rows the last INSERT, UPDATE or DELETE changed, and SQL(ID) the key of the last row inserted.

Each statement on its own is a transaction: its changes are on the card, safe, when it ends. That takes about 20ms. Many changes together are much faster inside one transaction, which is written to the card once at its end:

```
SQL EXEC "BEGIN"
FOR i = 1 TO 1000
  SQL EXEC "INSERT INTO reading(t, value) VALUES(?, ?)", i, v(i)
NEXT i
SQL EXEC "COMMIT"
```

A thousand inserts take about a third of a second like this, against 20 seconds one by one. ROLLBACK in place of COMMIT undoes them all. If the power fails during a transaction, the next SQL OPEN finds it unfinished and undoes it, so the database is never left half changed.

## Queries

SQL QUERY starts a query, and SQL(NEXT) moves to its next row: 1 if there is one, 0 when there are no more. SQL(n) is the value of column n of the row (the first column is 1), as an integer, a floating point number or a string, as the column holds it.

```
SQL QUERY "SELECT name, born FROM member WHERE born > ? ORDER BY born", 1950
DO WHILE SQL(NEXT)
  PRINT SQL(1), SQL(2)
LOOP
```

SQL(COLUMNS) is the number of columns, SQL(NAME n) the name of column n. A column with no value (SQL's NULL) gives 0, or an empty string for a text column; SQL(ISNULL n) is 1 if column n has no value.

A query holds memory, and its tables, until it has given all its rows. A query read only in part (one row of a count, say) is ended by SQL CLOSE QUERY, by the next SQL QUERY, or by SQL CLOSE. A statement that needs a table such a query still holds (DROP or CREATE INDEX on it) ends the query first.

SQL(SELECT puts all the rows of a query into an array at once, and returns how many it put there. A one dimensional array takes the first column; a two dimensional array (row, column) takes as many columns as it has. A string array takes each value as text. It stops when the array is full.

```
DIM names$(100)
n = SQL(SELECT "SELECT name FROM member ORDER BY name", names$())
DIM t$(20, 2)
n = SQL(SELECT "SELECT name, born FROM member WHERE born < ?", t$(), 1970)
```

## Structures and Tables

A table's row and an MMBasic structure are much the same thing, and SQL can move rows into and out of structures by matching the names of the structure's members with the names of the table's columns.

```
TYPE Member
  id AS INTEGER
  name AS STRING LENGTH 40
  born AS INTEGER
END TYPE
DIM m AS Member, list(100) AS Member

SQL CREATE Member                   ' a table MEMBER with the type's members as its columns
m.name = "Ann" : m.born = 1961
SQL INSERT m                        ' a row from m; m.id is now its key
SQL INSERT list()                   ' a row for each element of the array
SQL QUERY "SELECT * FROM member ORDER BY name"
DO WHILE SQL(FETCH m)               ' as SQL(NEXT), the row put into m
  PRINT m.name, m.born
LOOP
n = SQL(SELECT "SELECT * FROM member WHERE born > ?", list(), 1950)
SQL EXEC "UPDATE member SET born = :born WHERE id = :id", m
```

- SQL CREATE type makes the table if it is not there, named after the type (or SQL CREATE type, table$). INTEGER members are INTEGER columns, FLOAT members REAL and STRING members TEXT; a member named ID that is an integer is the table's key.
- SQL INSERT var adds a row from a structure, or a row for each element of an array of structures (var()), in one transaction. When the ID member is 0, the database gives the row a new key and puts it into the member.
- SQL(FETCH var) and SQL(SELECT with an array of structures fill each member from the column of its name (in any case); members with no column of their name are cleared.
- A statement given a structure among its values takes :name parameters from the members of that name.
- A nested structure's members are columns named PARENT_CHILD, and an array member's elements columns NAME_0, NAME_1 and so on (NAME_1 first under OPTION BASE 1).

## An Example

The sql folder of the programs zip has a sample database, computers.db, with a table of home computers, and this program, computers.bas, which uses it. The table's columns have the names of the members of the type Computer, so each row goes straight into a structure.

```
TYPE Computer
  id AS INTEGER
  name AS STRING LENGTH 30
  maker AS STRING LENGTH 20
  year AS INTEGER
  cpu AS STRING LENGTH 10
  ramkb AS INTEGER
END TYPE
DIM c AS Computer, z80(20) AS Computer
DIM INTEGER i, n

SQL OPEN "A:/sql/computers.db"

PRINT "By year:"
SQL QUERY "SELECT * FROM computer ORDER BY year, name"
DO WHILE SQL(FETCH c)
  PRINT c.year, c.name, c.maker
LOOP

n = SQL(SELECT "SELECT * FROM computer WHERE cpu = ? ORDER BY ramkb DESC", z80(), "Z80")
PRINT : PRINT n; " with a Z80:"
FOR i = 0 TO n - 1
  PRINT z80(i).name, z80(i).ramkb; "K"
NEXT i

c.id = 0 : c.name = "ZX80" : c.maker = "Sinclair" : c.year = 1980 : c.cpu = "Z80" : c.ramkb = 1
SQL EXEC "DELETE FROM computer WHERE name = ?", c.name
SQL INSERT c
PRINT : PRINT "The ZX80 added as number"; c.id

PRINT : PRINT "Makers:"
SQL QUERY "SELECT maker, count(*) FROM computer GROUP BY maker ORDER BY 2 DESC, 1"
DO WHILE SQL(NEXT)
  PRINT SQL(1), SQL(2)
LOOP
SQL CLOSE
```

It lists the computers in the order they came out, finds those with a Z80 processor (filling an array of the type), adds a row from a structure (SQL INSERT gives it its key in c.id; the DELETE before it stops a second run adding it twice) and counts the computers of each maker.

## Errors

A statement the database cannot run is an MMBasic error, "SQL: " followed by SQLite's own message (SQL: no such table: member), which a program can catch with ON ERROR SKIP and MM.ERRMSG$. Ctrl-C stops a long statement with the error "SQL: stopped by Ctrl-C"; a transaction it was in is undone when the database is closed.

## Speed

The SD card is the limit for a large database: reading straight through a table runs at about 2.4MB a second, but each page fetched from a random place takes about 2ms. The engine keeps the pages it has used in memory (its cache), so a database that fits in the cache works at the speed of memory once it has been read. Measured with a table of 50,000 rows (6.5MB, with a 6MB index):

| | Generation 2 | Generation 1 (1MB) |
|---|---|---|
| A row found by an index, from a small database | well under 1ms | well under 1ms |
| 1,000 inserts in one transaction | 0.3s | 0.4s |
| One insert committed on its own | 16ms | 22ms |
| PRAGMA integrity_check | 32s | 160s |
| SELECT count(DISTINCT ...) | 34s | 460s |
| CREATE INDEX | 21s | 23s |

(The Generation 1's last three were measured on a Generation 2 given the Generation 1's 1MB: the SD card, not the processor, sets them.) Indexes make the difference: put one on the columns a program searches, sorts or joins by.

## Limits

- One database open at a time (ATTACH reaches more through it).
- A statement is a string, so at most 255 characters; up to 20 values after it.
- A structure used with SQL can have at most 64 columns, counting nested members and array elements; a string from the database longer than the variable or member it goes into is an error.
- During a large sort the engine writes temporary files to the root of the current drive (sqlite_....tmp), and removes them after.
