' usbdrive.bas: MM.INFO$(USBDRIVE), the C: drive's state as MM.INFO$(SDCARD) gives the SD card's (a USB flash drive
' on the G2's hub: Ready). A file open on A: is still good after it, and the current drive is not changed.
Print "drive "; MM.Info$(DRIVE); ", SD "; MM.Info$(SDCARD); ", USB "; MM.Info$(USBDRIVE); ", drive "; MM.Info$(DRIVE)
Open "A:/usbd.txt" For Output As #1
Print #1, "one"
Print "USB "; MM.Info$(USBDRIVE)
Print #1, "two"
Close #1
Open "A:/usbd.txt" For Input As #1: Line Input #1, a$: Line Input #1, b$: Close #1
Print "A: file "; a$; " "; b$
Kill "A:/usbd.txt"
Drive "C:"
Print "on C: "; MM.Info$(DRIVE); " USB "; MM.Info$(USBDRIVE)
Drive "A:"
