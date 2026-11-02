' apprun.bas: RUN of a loadable program from a program goes on with the next statement, its variables kept
DIM keep$ = "kept"
PRINT "before the program"
t = TIMER
RUN "A:/apps/hello.app", from_basic
PRINT "after it, "; INT(TIMER - t); " ms later; keep$ is "; keep$
RUN "A:/apps/hello.app", code
PRINT "and after one ending with code 7"
