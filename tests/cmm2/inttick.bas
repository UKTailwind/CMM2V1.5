' inttick.bas - SETTICK through the interrupt-ready word (plan step 7, the PicoMite's S1c): the 1 ms tick signals a
' tick on the millisecond it becomes due, so an armed SETTICK no longer costs every statement a scan. Counts are
' checked against the time they ran, within a tolerance, so the output is fixed.
Dim integer a, b, c, s, k
Dim float t

Print "-- one tick of 10 ms, 1 s of busy loop"
SetTick 10, ta, 1
t = Timer : Do While Timer - t < 1000 : k = k + 1 : Loop
SetTick 0, ta, 1
Print "ticks:"; rng$(a, 98, 101)

Print "-- two ticks, 10 and 25 ms"
a = 0 : b = 0
SetTick 10, ta, 1 : SetTick 25, tb, 2
t = Timer : Do While Timer - t < 1000 : k = k + 1 : Loop
SetTick 0, ta, 1 : SetTick 0, tb, 2
Print "10 ms:"; rng$(a, 98, 101); "  25 ms:"; rng$(b, 38, 41)

Print "-- PAUSE and RESUME of a tick"
a = 0
SetTick 10, ta, 1
t = Timer : Do While Timer - t < 300 : Loop
SetTick Pause, ta, 1
s = a
t = Timer : Do While Timer - t < 300 : Loop
Print "while paused:"; a - s; " more"
SetTick Resume, ta, 1
t = Timer : Do While Timer - t < 300 : Loop
SetTick 0, ta, 1
Print "after resume:"; rng$(a - s, 28, 31)

Print "-- ticks during a PAUSE statement"
a = 0
SetTick 10, ta, 1
Pause 500
SetTick 0, ta, 1
Print "in PAUSE 500:"; rng$(a, 48, 51)

Print "-- SETTICK 0 stops it"
a = 0
SetTick 10, ta, 1
Pause 100
SetTick 0, ta, 1
s = a
Pause 200
Print "after SETTICK 0:"; a - s; " more"

Print "-- a slow interrupt SUB: ticks due during it are served after it"
a = 0 : c = 0
SetTick 10, ta, 1 : SetTick 50, slow, 2
t = Timer : Do While Timer - t < 1000 : k = k + 1 : Loop
SetTick 0, ta, 1 : SetTick 0, slow, 2
Print "slow SUB ran:"; rng$(c, 18, 21); "  10 ms ticks still:"; rng$(a, 60, 101)

Print "-- a tick due during a long statement fires after it"
a = 0
SetTick 5, ta, 1
q$ = String$(200, "x") : t = Timer
For k = 1 To 2000 : q$ = Mid$(q$, 2) + "y" : Next
SetTick 0, ta, 1
Print "ticks in the loop:"; rng$(a, 1, 100000)
Print "done"
End

Sub ta
  a = a + 1
End Sub
Sub tb
  b = b + 1
End Sub
Sub slow
  Local float u = Timer
  c = c + 1
  Do While Timer - u < 30 : Loop
End Sub

' ok when n is within lo to hi, otherwise the count
Function rng$(n, lo, hi)
  If n >= lo And n <= hi Then rng$ = " ok" Else rng$ = Str$(n) + " (expected" + Str$(lo) + " to" + Str$(hi) + ")"
End Function
