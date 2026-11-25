' tstmt.bas - the time of a statement of each kind, of a SUB call and of a FUNCTION call, in microseconds (the empty
' loop's time taken off). A timing program of tools/threadcheck.py (tests/threads/quick.txt): no .expected, its
' RESULT lines are compared with the build before.
Option Default None
Option Explicit
Dim Integer i,m,n=20000,a,b(10)
Dim Float t0,tl
MODE 7,4
CLS
t0=Timer:For i=1 To n:Inc a:Next:tl=Timer-t0
Sub T(s$)
 Print "RESULT ";s$;Str$((Timer-t0-tl)*1000/n,0,2);" us"
End Sub
Sub S0
End Sub
Function F0%()
 F0%=1
End Function
t0=Timer:For i=1 To n:a=100+100:Next:T("a=100+100        ")
t0=Timer:For i=1 To n:a=RGB(yellow):Next:T("a=RGB(yellow)    ")
t0=Timer:For i=1 To n:a=Max(100,100,7):Next:T("a=Max(100,100,7) ")
t0=Timer:For i=1 To n:b(3)=b(4)+b(5):Next:T("b(3)=b(4)+b(5)   ")
t0=Timer:For i=1 To n:Pixel 100,100:Next:T("Pixel 100,100    ")
t0=Timer:For i=1 To n:Pixel 100,100,7:Next:T("Pixel 100,100,7  ")
t0=Timer:For i=1 To n:Line 100,100,100,100:Next:T("Line 1 pixel     ")
t0=Timer:For i=1 To n:Colour 7:Next:T("Colour 7         ")
t0=Timer:For i=1 To n:a=Pixel(100,100):Next:T("a=Pixel(100,100) ")
t0=Timer:For i=1 To n:Blit 0,0,10,10,2,2:Next:T("Blit 0,0,10,10,2,2 ")
t0=Timer:For i=1 To n:S0:Next:T("S0 (a SUB call)  ")
t0=Timer:For i=1 To n:a=F0%():Next:T("a=F0%() (FUNCTION)")
MODE 1,8
End
