' webserver.bas - the PicoMite's web server on the CMM2 (docs/Web_Plan.md, step 2): OPTION WIFI and OPTION TCP SERVER
' PORT 80 set, then RUN; a PC fetches http://<MM.INFO(IP ADDRESS)>/ (tests/wifi: pcweb.py). Each request: WEB TCP
' READ in the TCP interrupt, then WEB TRANSMIT PAGE with the page's {expressions} filled in, as on a WebMite.
Option Explicit
Option Default Integer
Dim buff%(512), count
Open "A:/wifi/index.html" For Output As #1
Print #1, "<html><body><h1>Colour Maximite 2 G3</h1>"
Print #1, "<p>{Date$} {Time$}, request {count}</p></body></html>"
Close #1
Print "Serving at "; MM.Info(IP ADDRESS); " on port"; MM.Info(TCP PORT); ": any key ends"
WEB TCP INTERRUPT webreq
Do : Loop Until Inkey$ <> ""
End

Sub webreq
  Local a
  For a = 1 To MM.Info(MAX CONNECTIONS)
    WEB TCP READ a, buff%()
    If LLen(buff%()) Then
      Inc count
      WEB TRANSMIT PAGE a, "A:/wifi/index.html"
    EndIf
  Next a
End Sub
