' astro.bas - LOCATION, ASTRO, STAR and SLEW, the PicoMite's astronomy (Src/Astro.c). The expected output is the
' PicoMite's own functions' (its io/GPS.c), compiled on a PC by tests/cmm2/astro_ref.py, for the same place, time,
' bodies and slews; the errors are its messages. Every way of naming a body is tried: a keyword (MOON, Vega, a name of
' two words), a string literal, s$, s$ before a space, MID$(), a DIM ... AS STRING variable and lower case.
Option Explicit
Dim float alt, az, ra, dec, lst, d1, d2, f1, f2, d3, d4
Dim integer i
Dim string s$, b$(13) = ("SUN", "MOON", "MERCURY", "VENUS", "MARS", "JUPITER", "SATURN", "URANUS", "NEPTUNE", "Vega", "Sirius", "Polaris", "Andromeda Galaxy", "Rigil Kent")
Dim t As String = "Deneb"

Location "21/10/2026 20:00:00", 51.4769, -0.0005, lst
Print "lst"; F$(lst)
For i = 0 To 13
  Astro b$(i), alt, az, ra, dec
  Print b$(i); F$(alt); F$(az); F$(ra); F$(dec)
Next
Astro MOON alt, az : Print "MOON"; F$(alt); F$(az)
Astro Vega alt, az : Print "Vega"; F$(alt); F$(az)
Astro Andromeda Galaxy alt, az, ra, dec : Print "Andromeda Galaxy"; F$(alt); F$(az); F$(ra); F$(dec)
s$ = "Capella"
Astro s$, alt, az : Print "s$"; F$(alt); F$(az)
Astro s$ alt, az : Print "s$ space"; F$(alt); F$(az)
Astro Mid$("xxPolarisxx", 3, 7), alt, az : Print "MID$"; F$(alt); F$(az)
Astro t, alt, az : Print "AS STRING"; F$(alt); F$(az)
Astro "vega", alt, az : Print "vega"; F$(alt); F$(az)
' the manual forms: J2000 RA (hours) and Dec (degrees), with proper motion (arcsec a year), with RA and Dec out
Astro alt, az, 18.6156, 38.7837 : Print "manual"; F$(alt); F$(az)
Astro alt, az, 18.6156, 38.7837, 0.2, 0.28 : Print "manual pm"; F$(alt); F$(az)
Astro alt, az, 18.6156, 38.7837, 0.2, 0.28, ra, dec : Print "manual out"; F$(alt); F$(az); F$(ra); F$(dec)
' the other date forms
Location "2026-10-21 06:30:15", -33.8688, 151.2093, lst : Print "lst"; F$(lst)
Astro SUN alt, az : Print "SUN"; F$(alt); F$(az)
Location "21/10/26 06:30:15", -33.8688, 151.2093, lst : Print "lst"; F$(lst)
Try "Astro fred alt, az"
Try "Astro " + Chr$(34) + "fred" + Chr$(34) + ", alt, az"
Try "Astro alt, az"
Try "Star SUN alt, az"
Try "Location " + Chr$(34) + "32/10/2026 20:00:00" + Chr$(34) + ", 0, 0"
Try "Location " + Chr$(34) + "21/10/2026 25:00:00" + Chr$(34) + ", 0, 0"
Try "Location " + Chr$(34) + "21/10/2026 20:00:00" + Chr$(34) + ", 95, 0"
Try "Location " + Chr$(34) + "21/10/2026 20:00:00" + Chr$(34) + ", 0, 190"
' SLEW: a target across the meridian (a flip), then one on the same side
Slew d1, d2, f1, f2, d3, d4, 2.0, 10.0, 4.0, 20.0, 3.0
Print "slew"; F$(d1); F$(d2); F$(f1); F$(f2); F$(d3); F$(d4)
Slew d1, d2, f1, f2, d3, d4, 2.0, 10.0, 2.5, 20.0, 5.0
Print "slew"; F$(d1); F$(d2); F$(f1); F$(f2); F$(d3); F$(d4)
Try "Slew d1, d2, f1, f2, d3, d4, 25, 10, 4, 20, 3"
Try "Slew d1, d2, f1, f2, d3, d4, 2, 10, 4, 95, 3"
Try "Slew d1, d2, f1, f2, d3, d4, 2, 10, 4, 20, 24"
End

Sub Try c$
  On Error Skip
  Execute c$
  Print c$; ": "; Choice(MM.ErrNo, MM.ErrMsg$, "no error")
End Sub

Function F$(x)
  F$ = Format$(x, " %.4f")
End Function
