' stack0.bas - Stage 0 of docs/Threads_Plan.md: how deep the main stack goes with nothing running but the statement
' loop, the housekeeping after each statement (CheckAbort: USB, the network, Bluetooth) and the hardware interrupts'
' handlers. RUN fills the free stack with a pattern, and MM.INFO(STACK LOW) is the lowest address written since.
' The figures are bytes below the top of the stack (&H2001FFF0). No .expected: they differ with the board and with
' what is plugged in. Run it with the network joined and with USB devices in, and again without.
Option Explicit
Const TOP = &H2001FFF0
Dim Integer t, n, here, low0, low1, low2
here = TOP - MM.Info(Stack)           ' the stack pointer inside a function called from a statement
low0 = TOP - MM.Info(Stack Low)       ' the deepest so far: RUN's own work and these statements
t = Timer
Do While Timer - t < 10000            ' ten seconds of statements, each with its housekeeping
  Inc n
Loop
low1 = TOP - MM.Info(Stack Low)
Pause 5000                            ' and five in PAUSE's own loop
low2 = TOP - MM.Info(Stack Low)
Print "Stack pointer in a function  "; here
Print "Deepest at the start         "; low0
Print "Deepest after 10 s of a loop "; low1; "  ("; n; " passes)"
Print "Deepest after 5 s of PAUSE   "; low2
Print "Free above the 4 KB guard    "; MM.Info(Stack Low) - MM.Info(HeapEnd)
