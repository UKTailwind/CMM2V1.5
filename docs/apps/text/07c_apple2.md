# The Apple //e Emulator

This is frank-apple, rh1tech's port of Michel Pollet's MII emulator. It is an enhanced Apple //e with 128K, an 80-column card, two Disk II drives and two hard disks, and runs at the Apple's 60 frames a second with most of the processor to spare, in MODE 8.

The screen is shown at the Apple's full resolution: 80 columns and double hi-res keep all 560 of their dots across.

It is for the Generation 1 and the Generation 2, but has been tried on a Generation 1 only so far.

## Starting It

```
CHDIR "A:/apple2"
RUN "apple2.app"
```

A start screen shows for three seconds, and then the Apple starts. With no disk in a drive it goes to Applesoft BASIC's ] prompt. The Apple //e's ROMs are built into the program.

The program calls the folder it was started in /apple.

## Disks

Put disk images (.dsk, .do, .po, .nib, .woz) in A:/apple2, and press **F11**:

1. Choose drive 1 or 2 with the arrows (or press 1 or 2), and press Enter.

2. Choose a disk image with the arrows, Page Up and Page Down, and press Enter. Folders are listed too, and Enter opens one.

3. Choose **Boot** to put the disk in and start the Apple from it, or **Insert** to change the disk without starting again, for a program that asks for its next disk. **Cancel** goes back.

Esc goes back a step. In the first step, D takes the disk out of the drive chosen.

The first time a disk image is chosen, a copy of it in the drive's own form is made beside it, with .bdsk added to its name (DISK.DSK.BDSK), and that copy is what the drive uses from then on: the image itself is never changed. A disk is read-only at first, so that a program's changes to it are not kept: Space, in the third step, makes it writable, and the changes then go to its .bdsk file. Tick Recreate .bdsk with D, in the third step, to start again from the image.

Take care in the list of disks: D deletes the file chosen from the card, without asking.

Hard disk images in ProDOS order, up to 32MB, can be put in A:/apple2 as drive0.img and drive1.img. They are the two drives of a SmartPort card in slot 5, which the Apple should start from when there is no disk in the Disk II drives.

Saving to a disk, and the hard disks, have not been tried on the Colour Maximite 2 yet.

## The Keys

{widths: 16,36}
| Keys | What they do |
| F11 | Disks |
| F12 | Runs as fast as it can while held |
| Scroll Lock | Runs as fast as it can, until pressed again |
| F9 | Shows the speed, at the top of the screen |
| F10 | Changes the colours: NTSC, a second NTSC set, the Apple IIGS's, and a green or amber monitor |
| Ctrl+Pause | Ctrl+Reset. With the left Alt (Open Apple) as well, the //e starts again from cold |
| Ctrl+Alt+Del | Starts the Apple again from cold |
| Ctrl+Alt+End | Back to MMBasic |
| Left Alt, left Windows key | Open Apple, the first joystick button |
| Right Alt, right Windows key | Closed Apple, the second joystick button |
| Backspace, left arrow | The left arrow |
| Delete | DELETE |
| Caps Lock | The //e's Caps Lock |

The other keys give what is printed on them, following OPTION USBKEYBOARD. The //e's Caps Lock is down at the start, as most programs of the time want capitals: Applesoft and DOS 3.3 do not understand their commands in lower case. Caps Lock lets it up, and then Shift gives the capitals. A key held down repeats, as on the //e.

Ctrl-C is an Apple key here, so it does not end the program: in Applesoft it stops a program.

## Joysticks

The first USB gamepad is the Apple's joystick. Its left stick moves it, and the d-pad moves it too, gradually, for the paddle games. Its a and x buttons (circle and triangle on a PlayStation pad) are the first joystick button, b and y (cross and square) the second, and Start the third.

Select opens the disks, where the d-pad moves about, a is Enter and b is Esc. Start, a and b together start the Apple again from cold.

The keypad's 8, 2 (or 5), 4, 6, and 7, 9, 1 and 3 for the diagonals, are the joystick as well, and its 0 the third button.

A DualShock 3 sends nothing until its PS button has been pressed.

## The Serial Console

The serial console types into the Apple too, so that a program can be pasted. Return is Return, Backspace is the left arrow, and the Apple's own control keys are its arrows: Ctrl-H left, Ctrl-U right, Ctrl-K up, Ctrl-J down. Ctrl-F then 1 to 9, a, b or c is F1 to F12, so Ctrl-F b gives the disks. Ctrl-F then r is Ctrl+Reset, and Ctrl-] goes back to MMBasic.

RUN "apple2.app", -stats prints, every 5 seconds, the frames a second and how much of the time the emulator has to spare.
