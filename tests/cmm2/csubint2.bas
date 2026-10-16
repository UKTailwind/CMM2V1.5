' csubint2.bas - runs after csubint.bas, which ends with its INTERRUPT SUB still set.  RUN clears it (M9), so raising
' the flag here runs nothing.  Before, the SUB's address in the last program was kept, and the flag sent execution
' into whatever this program has there.
Interrupt
Pause 20
Print "nothing ran"
