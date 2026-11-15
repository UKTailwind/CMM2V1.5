' usertst2.bas: run after usertest.bas: RUN took the module with the heap; the first USER loads it again, afresh
' Setup (tools/sdk): python build.py out/proforma.mod examples/user/proforma.c --user
' and python build.py out/filemod.mod ../../tests/user/filemod.c --user; copy both .mod
' files and these programs to A:/, then RUN "A:/usertest.bas", RUN "A:/usertst2.bas" and RUN "A:/usersql.bas".
PRINT "after RUN, calls"; USER(CALLS)
USER HELLO "again"
PRINT "calls"; USER(CALLS)
