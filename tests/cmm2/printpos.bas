' printpos.bas - PRINT's column (PicoMite issues 37 and 38): a comma counts to the next 8-column stop, as a terminal
' and a file show its tab; a file or COM port keeps its own column, so TAB() in a file line counts from that line and
' a PRINT to a file leaves the console's column alone.
Dim integer p1, p2
Dim string a$
Print 1, 2; Tab(30); "e"
Print "pos after comma:"; : Print 1, : Print Pos
Print "a", "bcdefghi", "x"; : Print Pos
Open "pp.txt" For Output As #1
Print "abc"; : Print #1, "data" : Print Tab(10); "x"
Print "abc"; : Print #1, "d"; : p1 = Pos : Print p1
Print "abcdefghij"; : Print #1, Tab(5); "y"; : Print #1, Tab(9); "z", "w"
Print #1, "q"; : Close #1
Open "pp.txt" For Input As #1
Do While Not Eof(1) : Line Input #1, a$ : Print "["; a$; "]" : Loop
Close #1
Kill "pp.txt"
Open "pp.txt" For Output As #2                                     ' a new file starts at column 1
Print #2, Tab(3); "n"
Close #2
Open "pp.txt" For Input As #2 : Line Input #2, a$ : Close #2 : Kill "pp.txt"
Print "["; a$; "]"
