' udptest.bas - WEB UDP on the CMM2 (docs/Web_Plan.md, step 3): OPTION UDP SERVER PORT 5005 set, then RUN; the PC
' (pcudp.py) sends datagrams to port 5005, and each is printed with its sender (MM.ADDRESS$, MM.MESSAGE$) and echoed
' to the sender's port 5006 with WEB UDP SEND.
Option Explicit
Option Default Integer
Dim n
WEB UDP INTERRUPT got
Print "UDP on port"; MM.Info(UDP PORT); ": any key ends"
Do : Loop Until Inkey$ <> ""
End

Sub got
  Inc n
  Print "From "; MM.ADDRESS$; ": "; MM.MESSAGE$
  WEB UDP SEND MM.ADDRESS$, 5006, "echo" + Str$(n) + ": " + MM.MESSAGE$
End Sub
