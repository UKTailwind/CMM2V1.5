# Installing Windows 3.1

Windows 3.1 comes on six 1.44 MB disks, and runs on MS-DOS. Use images of your own disks: this chapter calls them WIN1.IMG to WIN6.IMG. Windows runs in its 386 enhanced mode, which needs 2 MB of memory, on both the G1 and the G2.

## Preparing

Install MS-DOS first (the previous chapter). Then copy WIN1.IMG to WIN6.IMG to the emulator's folder, and add them to the list of floppies in msdosc.ini, after none:

```
floppies = none WIN1.IMG WIN2.IMG WIN3.IMG WIN4.IMG WIN5.IMG WIN6.IMG
```

## Running Setup

1. Start MS-DOS with msdosc.ini.

2. Press Ctrl+Alt+Page Down to put WIN1.IMG in drive A:, then type:

```
A:SETUP
```

3. Choose Express Setup with Enter. Setup copies its first files, then changes to its windows, where the mouse works.

4. Each time Setup asks for the next disk, press Ctrl+Alt+Page Down, check the top of the screen for the right disk, then press Enter or click OK.

5. Setup finds the PC's hardware itself: a VGA display and a "Microsoft, or IBM PS/2" mouse. If it asks, choose these.

6. Set up printers and programs as you wish. At the end, choose to restart (reboot) the PC.

Setup puts C:\WINDOWS on the PATH but does not start Windows when the PC starts. Type WIN at the C:\> prompt to start it, or add WIN as the last line of C:\AUTOEXEC.BAT.

Leave Windows (File, Exit Windows in the Program Manager) before you leave the emulator.

## Sound

Windows 3.1 has drivers for the Sound Blaster 1.5 and the Adlib, which Setup does not install. Add them in the Control Panel:

1. Open Control Panel, then Drivers, then Add.

2. Choose "Creative Labs Sound Blaster 1.5". When Windows asks for one of its disks, put that disk in drive A: with Ctrl+Alt+Page Down (or Page Up), then click OK.

3. Set the port to 220 and the interrupt to 5.

4. Add "Ad Lib" the same way, for MIDI music.

5. Restart Windows when it asks.

The Sound Blaster driver plays Windows' sounds and WAV files. MIDI music goes through the Adlib's FM synthesiser. To use it, open Control Panel, then MIDI Mapper, and choose the setup "Ad Lib General", which plays the General MIDI instruments on the FM synthesiser. Then, in a program that plays MIDI, choose the MIDI Mapper as its MIDI output.
