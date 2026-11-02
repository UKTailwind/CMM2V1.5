' flashtest.bas: a program saved in flash at the OPTION FLASH sector (dev firmware, 2 Nov 2026)
PRINT "flashtest ran: sector 2 starts &H"; HEX$(PEEK(WORD &H08140000)); ", sector 3 &H"; HEX$(PEEK(WORD &H08160000))
