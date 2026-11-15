' usertest.bas: USER and USER( with the pro-forma module (tools/sdk/examples/user, A:/proforma.mod)
' Setup (tools/sdk): python build.py out/proforma.mod examples/user/proforma.c --module --api 20 --stack 4096
' and python build.py out/filemod.mod ../../tests/user/filemod.c --module --api 20 --stack 4096; copy both .mod
' files and these programs to A:/, then RUN "A:/usertest.bas", RUN "A:/usertst2.bas" and RUN "A:/usersql.bas".
OPTION EXPLICIT
DIM a% = 1, b% = 2, x! = 1.5, y! = 2.5, s$ = "first", t$ LENGTH 10 = "second"
DIM ai%(4), af!(2,1), sa$(3) LENGTH 5, tot%, totf!
USER CLOSE
ON ERROR SKIP 1 : USER HELLO
PRINT "no module: "; MM.ERRMSG$
ON ERROR SKIP 1 : PRINT USER(CALLS)
PRINT "no module fn: "; MM.ERRMSG$
USER LOAD "A:/proforma"
USER HELLO
USER HELLO "Peter"
USER SWAP a%, b% : PRINT "swap ints"; a%; b%
USER SWAP x!, y! : PRINT "swap floats"; x!; y!
USER SWAP s$, t$ : PRINT "swap strings "; s$; " "; t$
ON ERROR SKIP 1 : USER SWAP a%, x!
PRINT "mixed: "; MM.ERRMSG$
USER FILL ai%(), 7 : PRINT "fill int"; ai%(0); ai%(4)
USER FILL af!(), 2.25 : PRINT "fill float 2d"; af!(0,0); af!(2,1)
USER FILL sa$(), "ab" : PRINT "fill str "; sa$(0); sa$(3)
ON ERROR SKIP 1 : USER FILL sa$(), "toolong"
PRINT "too long: "; MM.ERRMSG$
USER TOTAL ai%(), tot% : PRINT "total int"; tot%
USER TOTAL af!(), totf! : PRINT "total float"; totf!
PRINT "add"; USER(ADD 2, 3); USER(ADD 2, 3.5); USER(ADD a%, USER(ADD 1, 1))
PRINT "reverse "; USER(REVERSE$ "abcdef"); " "; USER(REVERSE$ s$ + "!")
PRINT "calls"; USER(CALLS)
ON ERROR SKIP 1 : USER JUMP
PRINT "unknown: "; MM.ERRMSG$
ON ERROR SKIP 1 : PRINT USER(NOPE)
PRINT "unknown fn: "; MM.ERRMSG$
ON ERROR SKIP 1 : PRINT USER(ADD "x", 1)
PRINT "string to add: "; MM.ERRMSG$
USER HELLO USER(REVERSE$ "retep")
ON ERROR SKIP 1 : USER HELLO Closer$()
PRINT "close inside: "; MM.ERRMSG$
USER CLOSE
ON ERROR SKIP 1 : USER HELLO
PRINT "closed: "; MM.ERRMSG$
USER LOAD "A:/proforma.mod"
PRINT "reloaded, calls"; USER(CALLS)
PRINT "done"
FUNCTION Closer$()
  USER CLOSE
  Closer$ = "x"
END FUNCTION
