# Installing MS-DOS

MS-DOS 6.2 and 6.22 come on three 1.44 MB Setup disks. MS-DOS is not free: use images of your own disks (Disk Images). This chapter calls them DISK1.IMG, DISK2.IMG and DISK3.IMG.

## Preparing

Copy DISK1.IMG, DISK2.IMG and DISK3.IMG to the emulator's folder on the SD card.

Make msdos.ini in the same folder, as in the first example of The Configuration File:

- Its list of floppies starts with Disk 1, so the PC starts from it.
- The list ends with none, the empty drive that Setup asks for at the end.
- Its new_disk line makes the hard disk, msdos.img, 64 MB.

## Running Setup

1. Start the PC:

```
CHDIR "A:/pc386"
RUN "pc386.app", msdos.ini
```

The first time, the emulator makes msdos.img, which takes a minute or two, and the serial console shows its progress. Then the PC starts from Disk 1 and Setup's first screen appears.

2. Press Enter to set up MS-DOS. Setup finds the new hard disk and offers to configure its space: choose the recommended choice. Setup then restarts the PC, which starts again from Disk 1, and formats drive C:.

3. Accept the settings Setup shows (the date, the time, the country and the keyboard) and the directory C:\DOS, with Enter. Setup copies the files from Disk 1.

4. When Setup asks for Disk 2, press Ctrl+Alt+Page Down. The top of the screen shows "A: DISK2.IMG". Then press Enter. Do the same when it asks for Disk 3.

5. When Setup asks you to take the disks out of the drives, press Ctrl+Alt+Page Down once more ("A: empty"), then Enter.

6. Setup ends and restarts the PC, which now starts MS-DOS from C:.

## Afterwards

From now on, start the PC with a configuration file whose list of floppies starts with none, such as msdosc.ini, the second example of The Configuration File. The PC then starts from C:.

Some lines are worth adding to C:\AUTOEXEC.BAT, with EDIT:

- KEYB with your keyboard's layout, for example KEYB UK.
- SET BLASTER=A220 I5 D1 H5 T6, for DOS programs that use the Sound Blaster.
- A mouse driver, for DOS programs that use the mouse: CuteMouse (free) or Microsoft's MOUSE.COM. Windows does not need one.
