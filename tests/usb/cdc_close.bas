' cdc_close.bas - CLOSE of a USB serial port stops the device's bulk IN (22 Oct 2026: the IN is armed only while the
' port is open). Needs COM4 with TX looped to RX. Each round sends 200 characters, closes the port while they are
' still coming back (the IN is aborted with a packet on the way), opens it again and shows what arrived from before,
' then sends a check line, which must come back whole: a slipped data toggle loses the first packet after the abort.
' Nothing from before the CLOSE may arrive after the OPEN (the device holds it while its IN is stopped).
Option Explicit
Const ROUNDS = 20
Dim integer rd, k, stale, bad
Dim string s$, r$, c$
s$ = String$(200, "x")
For rd = 1 To ROUNDS
  Open "COM4:115200" As #5
  Print #5, s$;
  Pause 2
  Close #5
  Pause 50
  Open "COM4:115200" As #5
  Pause 50
  r$ = Input$(255, #5)
  If Len(r$) Then stale = stale + 1
  c$ = "check" + Str$(rd)
  Print #5, c$;
  r$ = ""
  k = Timer
  Do While Len(r$) < Len(c$) And Timer - k < 500
    r$ = r$ + Input$(255, #5)
  Loop
  If r$ <> c$ Then bad = bad + 1
  Print "round"; rd; ":"; Choice(r$ = c$, " check ok", " check FAILED ["+r$+"]")
  Close #5
Next
Print ROUNDS; " rounds,"; bad; " checks failed,"; stale; " opens that found bytes from before"
On Error Skip
Print "C: still there:"; MM.Info(EXISTS DIR "C:/sources")
If bad + stale = 0 Then Print "PASS" Else Print "FAIL"
