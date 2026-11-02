' ctrlc.bas: Ctrl-C during a loadable program stops the BASIC program that ran it too (2 Nov 2026). Press Ctrl-C
' during the first run: the program must stop, not go on to run 2.
FOR i = 1 TO 5
  PRINT "run"; i
  RUN "A:/apps/oplbench.app", 4
NEXT
PRINT "all five ran: Ctrl-C did not stop the program"
