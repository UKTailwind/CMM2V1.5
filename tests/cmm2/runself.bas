' runself.bas - a line that is just RUN restarts the program (the loader builds that line
' by hand).  A file marks the second pass, so it runs exactly twice.
On Error Skip 1
Kill "runself.tmp"
If MM.ErrNo = 0 Then Print "second pass" : End
Open "runself.tmp" For Output As #1 : Close #1
Print "first pass"
RUN
Print "FAIL: RUN returned"
