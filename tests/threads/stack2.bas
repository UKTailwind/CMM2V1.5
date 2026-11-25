' stack2.bas - Stage 0 of docs/Threads_Plan.md: how deep the main stack goes in the network's work, which runs from
' the housekeeping after any statement (CheckAbort) and so on whichever thread's stack is current. A plain request
' and one over TLS, whose handshake is the deepest part. Needs OPTION WIFI and the Internet (example.com). No
' .expected: the figures are bytes below the top of the stack (&H2001FFF0), each the deepest since RUN.
Option Explicit
Const TOP = &H2001FFF0
Dim buff%(4096 / 8)
Dim cr$ = Chr$(13) + Chr$(10)
Dim q$ = "GET / HTTP/1.0" + cr$ + "Host: example.com" + cr$ + cr$
Print "At the start             "; TOP - MM.Info(Stack Low)
On Error Skip 3
WEB OPEN TCP CLIENT "example.com", 80
WEB TCP CLIENT REQUEST q$, buff%(), 8000
WEB CLOSE TCP CLIENT
Print "After a plain request    "; TOP - MM.Info(Stack Low); "  ("; LLen(buff%()); " bytes back, error"; MM.Errno; ")"
LongString Clear buff%()
On Error Skip 3
WEB OPEN TLS CLIENT "example.com", 443
WEB TCP CLIENT REQUEST q$, buff%(), 8000
WEB CLOSE TCP CLIENT
Print "After a request over TLS "; TOP - MM.Info(Stack Low); "  ("; LLen(buff%()); " bytes back, error"; MM.Errno; ")"
