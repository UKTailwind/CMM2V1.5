# Worked Programs

## Balls and a clock while the user types

Four balls bounce, each in a thread running the same SUB with its own arguments; a clock in the corner is a fifth thread; the main program asks for lines of text. Everything goes on while the user types.

```
Option Explicit
Dim Integer i, go = 1
Dim a$
Dim Integer col(4) = (0, RGB(red), RGB(green), RGB(yellow), RGB(cyan))

Sub Ball(x As Float, y As Float, dx As Float, dy As Float, c As Integer)
  Do While go
    Circle x, y, 5, 1, 1, 0, 0                    ' rub the old one out
    x = x + dx : y = y + dy
    If x < 10 Or x > MM.HRes - 10 Then dx = -dx
    If y < 130 Or y > MM.VRes - 10 Then dy = -dy
    Circle x, y, 5, 1, 1, c, c
    Pause 20
  Loop
End Sub

Sub Clock
  Do While go
    Text MM.HRes - 1, 0, Time$, "RT", 1, 1, RGB(white), RGB(blue)
    Pause 250
  Loop
End Sub

CLS
For i = 1 To 4
  Thread Start i Size 12 Locals 8, Ball, 100 * i, 150 + 60 * i, 2 + i, 1 + i, col(i)
Next
Thread Start 5 Size 12 Locals 8, Clock
Do
  Print @(0, 40) Space$(70);
  Print @(0, 40);
  Input "Type something (Q to quit)"; a$
  Print @(0, 70) "You typed: "; a$; Space$(60)
Loop Until UCase$(a$) = "Q"
go = 0
End
```

Each ball has its own position and speed as the parameters of its SUB, so one SUB serves all four. Each CIRCLE and TEXT carries its own colour, so the threads cannot disturb each other's colours, and each thread's print position is its own, so the clock does not move the line being typed.

!NOTE PicoMite: in MODE 1 of a VGA or HDMI PicoMite each 8 by 8 block of the screen has one colour, so run it in MODE 2, or MODE 3 on an RP2350 (in place of CLS).

## A long job with a live screen

The picture is drawn by a thread. The main program reads the keys: P pauses the drawing, R lets it go on, Q stops it.

```
Option Explicit
Dim Integer done, row
Dim k$

Sub Picture
  Local Integer x, y
  For y = 0 To MM.VRes - 1
    For x = 0 To MM.HRes - 1
      Pixel x, y, RGB(x And 255, y And 255, (x Xor y) And 255)
    Next
    row = y
  Next
  done = 1
End Sub

CLS
Thread Start 1, Picture
Do While done = 0
  k$ = UCase$(Inkey$)
  If k$ = "P" Then Thread Pause 1
  If k$ = "R" Then Thread Resume 1
  If k$ = "Q" Then Exit Do
  Pause 50
Loop
Thread Stop 1                 ' (does nothing if the thread has ended)
Print @(0, 0) "Ended at row"; row
```

## Workers sharing a total

Four threads add up parts of an array into one total. The read and write of the total are two statements, so a lock keeps them together.

```
Option Explicit
Dim Integer i, total
Dim Integer d(3999)
For i = 0 To 3999 : d(i) = i : Next

Sub Adder(first%, last%)
  Local Integer k, part
  For k = first% To last%
    part = part + d(k)
  Next
  Thread Lock 1
  total = total + part        ' (one statement: the lock is for the
  Thread Unlock 1             '  pattern; a longer job would need it)
End Sub

For i = 0 To 3
  Thread Start i + 1, Adder, i * 1000, i * 1000 + 999
Next
For i = 1 To 4 : Thread Wait i : Next
Print "Total"; total          ' 7998000
```

## A sensor logger

One thread samples a pin every 100 ms with its own SETTICK; another writes the samples to the SD card every ten seconds; the main program shows the latest reading.

```
Option Explicit
Dim Integer n, latest
Dim Float buf(99)

Sub Sampler
  SetTick 100, Sample, 1
  Do : Pause 1000 : Loop          ' its interrupt runs in this thread
End Sub

Sub Sample
  latest = Pin(7)
  Thread Hold                     ' the index and the store together
  buf(n Mod 100) = latest
  Inc n
  Thread Release
End Sub

Sub Writer
  Local Integer k, done
  Open "log.csv" For Append As #2
  Do
    Pause 10000
    Do While done < n
      Print #2, buf(done Mod 100)
      Inc done
    Loop
  Loop
End Sub

SetPin 7, AIN
Thread Start 1 Size 12 Locals 8, Sampler
Thread Start 2 Size 12 Locals 8, Writer
Do
  Text 0, 0, "Latest:" + Str$(latest) + "   "
  Pause 200
Loop Until Inkey$ <> ""
End
```

The file belongs to the Writer thread alone; the Sampler only fills the array. THREAD HOLD in the routine is not strictly needed (an interrupt routine runs whole, with no other thread), but it shows the pattern for code that is not a routine.

!NOTE PicoMite: an analogue input is GP26, GP27 or GP28: SetPin GP26, AIN and Pin(GP26) in place of pin 7.

## A web request while a game runs

The network belongs to one thread, which fetches a value every minute and leaves it in a global. The rest of the program never waits for the network.

```
Option Explicit
Dim temperature$ = "?"
Dim Integer buff%(512)

Sub Weather
  Local req$ = "GET /temp.txt HTTP/1.1" + Chr$(13) + Chr$(10) + "Host: 192.168.1.10" + Chr$(13) + Chr$(10) + Chr$(13) + Chr$(10)
  Do
    On Error Skip 3
    WEB OPEN TCP CLIENT "192.168.1.10", 80, 3000
    WEB TCP CLIENT REQUEST req$, buff%(), 3000
    WEB CLOSE TCP CLIENT
    If MM.Errno = 0 Then temperature$ = LGetStr$(buff%(), LLen(buff%()) - 4, 5)
    Pause 60000
  Loop
End Sub

Thread Start 1, Weather
Do
  ' ... the game ...
  Text 0, 0, "Outside: " + temperature$
  Pause 20
Loop
```

The address, the request and the way the reply is read depend on your server; the shape of the program is the point.
