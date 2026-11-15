' usersql.bas: a USER module beside SQL's; the file a USER module keeps open survives SQL CLOSE (filemod.mod)
' Setup (tools/sdk): python build.py out/proforma.mod examples/user/proforma.c --user
' and python build.py out/filemod.mod ../../tests/user/filemod.c --user; copy both .mod
' files and these programs to A:/, then RUN "A:/usertest.bas", RUN "A:/usertst2.bas" and RUN "A:/usersql.bas".
DIM a$
USER LOAD "A:/filemod.mod"
USER FOPEN "A:/userfile.txt"
USER FWRITE "before SQL"
SQL OPEN "A:/usersql.db"
SQL EXEC "CREATE TABLE IF NOT EXISTS t(n INTEGER)"
SQL EXEC "DELETE FROM t"
SQL EXEC "INSERT INTO t VALUES(?)", 42
USER FWRITE "with SQL open"
SQL QUERY "SELECT n FROM t"
DO WHILE SQL(NEXT) : PRINT "row"; SQL(1) : LOOP
SQL CLOSE
PRINT "after SQL CLOSE, file open:"; USER(0)
USER FWRITE "after SQL CLOSE"
USER FCLOSE
OPEN "A:/userfile.txt" FOR INPUT AS #1
DO WHILE NOT EOF(#1) : LINE INPUT #1, a$ : PRINT "file: "; a$ : LOOP
CLOSE #1
' the module closed with a file open: the loader closes it (USER CLOSE); then SQL's still works
USER FOPEN "A:/userfil2.txt"
USER FWRITE "left open"
SQL OPEN "A:/usersql.db"
USER CLOSE
SQL QUERY "SELECT n * 2 FROM t"
DO WHILE SQL(NEXT) : PRINT "row after USER CLOSE"; SQL(1) : LOOP
SQL CLOSE
OPEN "A:/userfil2.txt" FOR INPUT AS #1
LINE INPUT #1, a$ : PRINT "file 2: "; a$
CLOSE #1
' the pro-forma and SQL together
USER LOAD "A:/proforma"
SQL OPEN "A:/usersql.db"
SQL EXEC "INSERT INTO t VALUES(?)", USER(ADD 40, 2)
SQL QUERY "SELECT COUNT(*), SUM(n) FROM t"
DO WHILE SQL(NEXT) : PRINT "count"; SQL(1); "  sum"; SQL(2) : LOOP
SQL CLOSE
USER HELLO "beside SQL"
PRINT "done"
