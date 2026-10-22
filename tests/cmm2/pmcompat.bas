' pmcompat.bas - the small PicoMite items from the parity re-check (22 Oct 2026): OPTION NOCHECK and OPTION PROFILING
' in a program (accepted, nothing done: PROFILING restarted the board), MM.INFO$(LCDPANEL), the DEVICE prefix and the
' PicoMite's WII forms (nothing on the Wii connector: the errors), DEVICE( for the Wii, and a variable called DEVICE
' (an array cannot be: DEVICE( is a function).
Option Explicit
Dim integer n
Option NoCheck On
Option NoCheck Off
Print "OPTION NOCHECK: accepted"
Option Profiling On, Sample
Option Profiling Off
Print "OPTION PROFILING in a program: no restart"
On Error Skip 1
Option NoCheck Maybe
Print "OPTION NOCHECK MAYBE: "; MM.ErrMsg$
Print "MM.INFO$(LCDPANEL): "; MM.Info$(LCDPANEL); ", has MODE:"; Instr(MM.Info(LCDPANEL), "MODE") > 0
On Error Skip 1
Device Wii Open
Print "DEVICE WII OPEN: "; MM.ErrMsg$
On Error Skip 1
Device Wii Classic Open
Print "DEVICE WII CLASSIC OPEN: "; MM.ErrMsg$
On Error Skip 1
Wii Open
Print "WII OPEN: "; MM.ErrMsg$
On Error Skip 1
Device Nunchuck Open
Print "DEVICE NUNCHUCK OPEN: "; MM.ErrMsg$
On Error Skip 1
Wii Nunchuck Open
Print "WII NUNCHUCK OPEN: "; MM.ErrMsg$
On Error Skip 1
Device Wii Close
Print "DEVICE WII CLOSE: "; MM.ErrMsg$
On Error Skip 1
n = Device(Wii B)
Print "DEVICE(WII B): "; MM.ErrMsg$
On Error Skip 1
n = Device(Nunchuck Z)
Print "DEVICE(NUNCHUCK Z): "; MM.ErrMsg$
On Error Skip 1
Device Keypad k, 1, 2, 3, 4, 5, 6, 7, 8
Print "DEVICE KEYPAD: "; MM.ErrMsg$
On Error Skip 1
Device SerialTx 1, 9600, "x"
Print "DEVICE SERIALTX: "; MM.ErrMsg$
On Error Skip 1
Device Toaster
Print "DEVICE TOASTER: "; MM.ErrMsg$
arr
Print "done"

Sub arr
  Local integer device
  device = 4
  Print "a variable called device:"; device
End Sub
