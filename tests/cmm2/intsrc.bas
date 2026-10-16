' intsrc.bas - interrupt sources that signal when they become ready (plan step 7, the PicoMite's S1c round 2): the
' end of PLAY TONE, ADC START and DAC START, the COUNT5 interrupt, sprite collisions, MODE's frame interrupt, ON KEY (keys typed, and keys
' already waiting when it is armed) and ON KEY k. intsrc.in types the keys at the prompts ending in "?". First, an
' armed source that is not due costs a statement nothing: the same loop is timed with several armed and none.
Dim integer nt, na, nd, nc, ns, nf, nk, ny, i, x, dv2
Dim float t, t0, t1, a(19)
Dim integer d(999), e(999)
Dim string k$, c$

Print "-- armed sources that are not due cost nothing (first, before any other is armed)"
t = Timer : For i = 1 To 100000 : x = x + 1 : Next : t0 = Timer - t
On Key keyh
On Key Asc("~"), yk
Sprite Interrupt coll
SetPin 18, CIN, cint
ADC Open 2000, 7, , , adone
t = Timer : For i = 1 To 100000 : x = x + 1 : Next : t1 = Timer - t
ADC Close
SetPin 18, Off
On Key 0
On Key Asc("~"), 0
Print "armed/none:"; rng$(t1 / t0 * 100, 85, 110); "%"

Print "-- PLAY TONE's end"
t = Timer
Play Tone 440, 440, 200, tdone
Do While nt = 0 And Timer - t < 2000 : Loop
t1 = Timer - t
Pause 100
Print "tone ends:"; nt; "  after"; rng$(t1, 150, 400)

Print "-- ADC START with an interrupt"
ADC Open 2000, 7, , , adone
ADC Start a()
t = Timer
Do While na = 0 And Timer - t < 2000 : Loop
Pause 50
ADC Close
Print "adc done:"; na

Print "-- DAC START with two arrays and an interrupt"
For i = 0 To 999 : d(i) = 1000 : e(i) = 3000 : Next
DAC Start 10000, d(), e(), ddone
Pause 50
dv2 = PEEK(WORD &H40007430)                                       ' DAC2's output during the playback
t = Timer
Do While nd = 0 And Timer - t < 2000 : Loop
Pause 20
Print "dac done:"; nd; "  second channel:"; dv2

Print "-- the COUNT5 interrupt (PIN(18) = n moves the high word)"
SetPin 18, CIN, cint
Pin(18) = &H100000000
Pause 20
Pin(18) = 0
Pause 20
SetPin 18, Off
Print "count interrupts:"; nc

Print "-- a sprite collision"
CLS
Box 0, 0, 16, 16, , RGB(red), RGB(red)
Sprite Read #1, 0, 0, 16, 16
Sprite Read #2, 0, 0, 16, 16
CLS
Sprite Interrupt coll
Sprite Show #1, 100, 100, 1
Sprite Show #2, 300, 100, 1
Pause 20
Print "apart:"; ns
Sprite Show #2, 108, 100, 1
Pause 20
Print "overlapping:"; ns
Sprite Close All
CLS

Print "-- MODE's frame interrupt (mode 8 is 75 Hz), and MODE without one"
Mode 8, 8, 0, fr
Pause 1000
x = nf
Mode 8, 8
Pause 200
Print "frames in 1 s:"; rng$(x, 70, 80); "  after:"; nf - x

Print "-- ON KEY"
On Key keyh
Print "keys?";
t = Timer
Do While nk < 4 And Timer - t < 3000 : Loop
Pause 50
On Key 0
Print "on key:"; nk; "  "; shown$(k$)

Print "-- ON KEY armed with keys already waiting"
nk = 0 : k$ = ""
Print "early?";
Pause 1000
On Key keyh
Pause 100
On Key 0
Print "waiting keys:"; nk; "  "; shown$(k$)

Print "-- ON KEY k"
On Key Asc("y"), yk
Print "more?";
Pause 1000
On Key Asc("y"), 0
k$ = ""
Do
  c$ = Inkey$
  If c$ = "" Then Exit Do
  k$ = k$ + c$
Loop
Print "on key y:"; ny; "  buffered: "; shown$(k$)

Print "done"
End

Sub tdone
  nt = nt + 1
End Sub
Sub adone
  na = na + 1
End Sub
Sub ddone
  nd = nd + 1
End Sub
Sub cint
  nc = nc + 1
End Sub
Sub coll
  ns = ns + 1
End Sub
Sub fr
  nf = nf + 1
End Sub
Sub keyh
  nk = nk + 1
  k$ = k$ + Inkey$
End Sub
Sub yk
  ny = ny + 1
End Sub

' the keys, with CR and LF shown
Function shown$(s1$)
  Local integer j
  Local string r$
  For j = 1 To Len(s1$)
    c$ = Mid$(s1$, j, 1)
    If c$ = Chr$(13) Then
      r$ = r$ + "<cr>"
    ElseIf c$ = Chr$(10) Then
      r$ = r$ + "<lf>"
    Else
      r$ = r$ + c$
    EndIf
  Next
  shown$ = r$
End Function

' ok when n is within lo to hi, otherwise the number
Function rng$(n, lo, hi)
  If n >= lo And n <= hi Then rng$ = " ok" Else rng$ = Str$(n) + " (expected" + Str$(lo) + " to" + Str$(hi) + ")"
End Function
