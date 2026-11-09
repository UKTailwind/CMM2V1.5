# The Atari 8-bit Emulator

This is atari800, the Atari800 development team's emulator of the Atari 400 and 800, the XL and XE computers and the 5200 console. It starts as an 800XL with 64K, runs at the Atari's own 50 frames a second (PAL), in MODE 7, and has the emulator's own menus.

It has been tried on a Generation 2 only so far.

## Starting It

```
CHDIR "A:/atari800"
RUN "atari800.app"
```

Words after the name are atari800's own options, for example:

```
RUN "atari800.app", game.xex
RUN "atari800.app", -basic
RUN "atari800.app", -5200 -cart-type 4 -cart game.rom
```

The settings are kept in atari800.cfg in the same folder: the F1 menu's Save Configuration writes it.

## The ROMs

AltirraOS and Altirra BASIC, Avery Lee's free replacements for Atari's operating system and BASIC, are built into the program, so it needs nothing else to start, and nearly all programs run with them.

Atari's own ROMs are not included: Atari's copyright in them has not been given up. Copies are in the data folder of DnCraptor's atari800 repository, https://github.com/DnCraptor/atari800 (ATARIXL.ROM, ATARIOSB.ROM and ATARIBAS.ROM), which you download and use at your own risk. Put them in A:/atari800 before the first start: atari800 recognises them by their contents, whatever they are called, and then uses them in place of Altirra's. ROMs added later are found with F1, Emulator Configuration, System ROM settings, Find ROM images in a directory.

## BASIC

BASIC is off at first, as atari800 has it, because most programs load into the memory BASIC would use: on a real XL they are started with OPTION held down for the same reason.

**Alt+B** turns BASIC on or off and starts the Atari again. A BASIC program (.bas) turns it on by itself while it loads. RUN "atari800.app", -basic starts with it on.

## Loading Programs

**F1** opens the menu, and Alt with a letter goes straight to one of its pages:

{widths: 16,36}
| Keys | Page |
| Alt+R | Run Atari Program: a program (.xex, .com), a BASIC program, a disk, a cartridge or a tape |
| Alt+D | Disk Management: disk images (.atr, .xfd, .atx, .dcm, .pro) in drives D1: to D8: |
| Alt+C | Cartridge Management: insert or remove a cartridge (.car, .rom) |
| Alt+T | Tape Management: insert a tape (.cas), its position, recording, a blank tape |
| Alt+Y | System Settings: the machine, its memory, PAL or NTSC |
| Alt+J | Controller Configuration: what the mouse is |
| Alt+S, Alt+L | Save and load the machine's state |

Inserting a cartridge starts the Atari again in it, and removing it starts the Atari again without it.

Run Atari Program with a tape (.cas) boots it, as holding START while switching on does. A tape loads at once, as a disk does, because atari800's SIO patch (Emulator Configuration) passes each record straight to the operating system. With the SIO patch off a tape loads at its real speed, 600 baud, with the operating system's tone. RUN "atari800.app", -boottape name.cas boots a tape from MMBasic's prompt.

## The Keys

{widths: 16,36}
| Keys | What they do |
| F1 | The menu |
| F2, F3, F4 | OPTION, SELECT and START |
| F5 | RESET (Shift+F5: switch off and on again) |
| F6 | HELP |
| F7, Pause | BREAK |
| F9 | Back to MMBasic |
| F12 | Turbo: as fast as it can go |
| Alt+B | BASIC on or off, and the Atari started again |
| Ctrl+Alt+End | Back to MMBasic |
| Esc | ESC |
| Home | CLEAR |
| Insert, Delete | Insert and delete a character (with Shift, a line) |
| The arrows | The cursor keys (CTRL with - = + *) |
| Caps Lock | CAPS |
| ` and the left Windows key | The Atari key (inverse video) |

The other keys give what is printed on them, following OPTION USBKEYBOARD, so the symbols are where your keyboard has them. Shift with a letter is a capital, whatever Caps Lock says: Caps Lock is the Atari's own CAPS key, which keeps its own state. Ctrl with a key is the Atari's CTRL with it.

Ctrl-C is an Atari key here, so it does not end the program.

On the 5200 the digits, # (or =) and * are its keypad, P is PAUSE, R is RESET and F4 is START.

## Joysticks, Paddles and the Mouse

The first USB gamepad is joystick 1, the one nearly every game uses, and the second is joystick 2. The d-pad or the left stick steers, and any of the four face buttons fires. The pad's Start, Select and L buttons are the console's START, SELECT and OPTION. A DualShock 3 sends nothing until its PS button has been pressed.

With one pad, its right stick is joystick 2, and R1 or R2 its fire, for the twin-stick games such as Robotron: 2084 (this needs firmware with version 16 of the services).

The keypad is joystick 1 as well: 1 to 9 the directions, 0 or 5 the fire.

The mouse can be paddles, an ST or Amiga mouse, a trak-ball, a light pen or gun, or a Koala pad: choose it with Alt+J, Mouse device. With paddles, moving the mouse to the right turns the paddle clockwise, as a real one turns.

## Known Problems

- With the SIO patch off, a tape with a program file after its boot loader stops with a checksum error under AltirraOS; with Atari's own XL operating system it loads at real speed. With the SIO patch on, as it starts, both load.
- Programs that need two joysticks on one pad (twin-stick games) need firmware 7.00.00RC4 or later for the pad's right stick.
