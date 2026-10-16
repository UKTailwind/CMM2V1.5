' heapnames.bas - MM.INFO(HEAPI) and MM.INFO(HEAPS), Gerry Allardice's names in 6.00.00b14, are the same as
' MM.INFO(IHEAP) and MM.INFO(SHEAP) (M12 of the b14 merge, Peter's decision 6).
Option Explicit
Dim integer a, b, c, d
a = MM.Info(IHEAP) : b = MM.Info(HEAPI)
c = MM.Info(SHEAP) : d = MM.Info(HEAPS)
Print "internal: "; Choice(a > 0 And Abs(a - b) <= 4096, "same", "DIFFERENT " + Str$(a) + " " + Str$(b))
Print "sdram: "; Choice(c > 0 And Abs(c - d) <= 4096, "same", "DIFFERENT " + Str$(c) + " " + Str$(d))
Print "lower case: "; Choice(Abs(MM.Info(heapi) - a) <= 4096, "same", "DIFFERENT")
