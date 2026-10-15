' gen7.bas - item 7 part 1: writes T7A.BAS (400 SUBs + 111 labels = 511 names) and T7B.BAS
' (400 + 112 = 512, a full funtbl).  After the fix: RUN "T7A" prints "ok funtbl7 511", and
' RUN "T7B" stops with "Too many subroutines, functions and labels".
' Before the fix T7B hangs at its first new variable (findvar's SUB-name check never ends), and
' if the program is kept in flash it hangs again at every boot (FindSubFun("MM.STARTUP")):
' do not RUN T7B on unfixed firmware with a flash program slot.
Dim i%, f%
For f% = 0 To 1
  Open "T7" + Chr$(65 + f%) + ".BAS" For Output As #1
  Print #1, "Dim r%"
  Print #1, "On Error Skip 1"
  Print #1, "NoSuchSub7"
  Print #1, "On Error Skip 1"
  Print #1, "GoTo NoSuchLabel7"
  Print #1, "S1"
  Print #1, "GoSub L1"
  Print #1, "Print " + Chr$(34) + "ok funtbl7 " + Str$(511 + f%) + Chr$(34)
  Print #1, "End"
  For i% = 1 To 400
    Print #1, "Sub S" + Str$(i%)
    Print #1, "End Sub"
  Next
  For i% = 1 To 111 + f%
    Print #1, "L" + Str$(i%) + ":"
  Next
  Print #1, "Return"
  Close #1
Next
Print "written T7A.BAS (511 names) and T7B.BAS (512 names)"
