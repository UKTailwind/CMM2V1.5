' cdc_msc.bas - the PicoMite's USB conflict (22 Oct 2026), tried on the CMM2: a flash drive (C:) read through DIR$
' while a USB serial device's bulk IN is busy. On the PicoMite the drive stopped answering until it was re-plugged:
' the RP2350's one bulk pipe, shared with the serial device, lost a packet of the drive's and its data toggle slipped.
' Needs a USB serial device as COM4 with its TX looped to RX (the G2's CH340), and a flash drive with a directory DIR1$.
' For each directory entry, a numbered line goes out through COM4 and must come back whole, and a file's first 1 KB is
' read from the drive; the entry is listed on the screen with its size and the line's result. Any line lost or wrong,
' or any error from the drive, fails the run.
Option Explicit
Const PASSES = 10
Const DIR1$ = "C:/sources"
Dim integer pass, n, total, lost, wrong, errs, reads, k, z
Dim string f$, s$, r$, a$
Dim float t
Open "COM4:115200" As #5
t = Timer
For pass = 1 To PASSES
  n = 0
  f$ = Dir$(DIR1$ + "/*", ALL)
  Do While f$ <> ""
    n = n + 1
    ' a line out through the loopback, and it back, while the drive's directory is being read
    s$ = "pass" + Str$(pass) + " entry" + Str$(n) + " " + f$
    Print #5, s$;
    r$ = ""
    k = Timer
    Do While Len(r$) < Len(s$) And Timer - k < 500
      r$ = r$ + Input$(255, #5)
    Loop
    If r$ <> s$ Then
      If Len(r$) < Len(s$) Then lost = lost + 1 Else wrong = wrong + 1
      Print "  COM4: sent ["; s$; "] got ["; r$; "]"
    EndIf
    ' the drive: the entry's size, and a file's first 1 KB
    On Error Skip
    z = MM.Info(FILESIZE DIR1$ + "/" + f$)
    If MM.ErrNo Then errs = errs + 1 : Print "  C: "; f$; ": "; MM.ErrMsg$
    If z > 0 Then
      On Error Skip
      Open DIR1$ + "/" + f$ For Input As #1
      If MM.ErrNo Then
        errs = errs + 1 : Print "  C: open "; f$; ": "; MM.ErrMsg$
      Else
        For k = 1 To 4
          If Not Eof(#1) Then a$ = Input$(255, #1)
        Next
        Close #1
        reads = reads + 1
      EndIf
    EndIf
    ' the listing: pass, entry, name, size (-2 for a directory), and how its line came back through COM4
    Print pass; n; "  "; f$; Tab(50); z; Tab(64); Choice(r$ = s$, "COM4 ok", "COM4 FAILED")
    f$ = Dir$()
  Loop
  total = total + n
  Print "pass"; pass; ":"; n; " entries,"; lost; " lines lost,"; wrong; " wrong,"; errs; " drive errors,";
  Print Int((Timer - t) / 1000); " s"
Next
Close #5
Print total; " entries,"; reads; " files read,"; total; " lines through COM4"
If lost + wrong + errs = 0 And total > 0 Then Print "PASS" Else Print "FAIL"
