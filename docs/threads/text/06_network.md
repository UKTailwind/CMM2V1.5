# The Network and Bluetooth

These apply only to a Colour Maximite 2 with the WiFi module.

## The network belongs to one thread

The network's commands (WEB OPEN TCP CLIENT, TCP CLIENT REQUEST, WEB NTP, WEB MQTT, WEB PING, WEB TRANSMIT and the rest) share one set of state inside the firmware: one TCP client connection, one MQTT client, one timer for their waits. So the network belongs to one thread at a time:

- The main program may use the network first, to set up: WEB CONNECT, WEB NTP and the like at the start of the program do not tie it to the main program.
- The first thread other than the main program that uses a WEB command then has the network, until that thread ends.
- A WEB command in any other thread, the main program included, is then the error "The network belongs to thread n".
- When the owner ends, the network is free again, and the next thread to use it has it.

The interrupt routines the owning thread set up (WEB TCP INTERRUPT, MQTT's and UDP's) run in that thread, so they may use the network too.

If a thread's first WEB command comes while the main program's setup command is still under way, the thread waits for it to finish.

## Waiting on the network

A WEB command that waits for an answer lets the other threads run meanwhile. A request with a five second timeout to a server that does not answer costs the rest of the program nothing:

```
Sub Fetcher
  Do
    On Error Skip
    WEB OPEN TCP CLIENT "example.com", 80, 5000
    If MM.Errno = 0 Then
      WEB TCP CLIENT REQUEST req$, buff%(), 5000
      WEB CLOSE TCP CLIENT
      Flag(5) = 1                   ' new data
    EndIf
    Pause 60000
  Loop
End Sub
```

The thread that waits for the network takes no interrupt while it waits, as a WEB command never did; the other threads' interrupts carry on.

## Bluetooth

Bluetooth is the same. Its scan, its serial connection (COM8) and its keyboard each keep one state, so BLUETOOTH commands belong to one thread, by the same rule as the network: the main program may use them to set up, the first other thread to use one has Bluetooth until it ends, and a BLUETOOTH command in any other thread is the error "Bluetooth belongs to thread n". BLUETOOTH CONNECT, SCAN and the other commands that wait let the other threads run.

Using COM8 once it is open (PRINT #, INPUT #) is like using any COM port: it is shared, and best given to one thread.
