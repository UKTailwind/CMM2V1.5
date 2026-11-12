# The BBC Micro Emulator

This is the BBC Microcomputer: the Model B and the BBC Master 128. It is frank-micro, rh1tech's port of B-em, Tom Walker's emulator, by way of Graham Sanderson's B-em for the Raspberry Pi Pico, made to run on the Colour Maximite 2. It has the BBC's disc drives, its sound, its keyboard and its sideways ROMs, and it runs at the BBC's own speed, 50 frames a second, with time to spare.

It is for the Generation 1 and the Generation 2. It needs firmware 7.00.00RC3 or later.

## Starting It

```
CHDIR "A:/bbc"
RUN "bbc.app"
```

It starts as a BBC Master 128, at BASIC's > prompt. **F12** changes it to a Model B, and it then starts as that until changed back. The BBC's ROMs are built into the program, so it needs nothing else on the card.

The program calls the folder it was started in /micro, and keeps its settings there in micro.ini.

Words after a comma:

{widths: 18,34}
| Words | What they do |
| disc | That disc image in drive 0, and the BBC started from it, as SHIFT-BREAK does: RUN "bbc.app", disk/elite.ssd |
| disc disc | The first in drive 0 and the second in drive 1 |
| -ro | The discs are write protected: nothing the BBC does changes them |
| -stats | The frames a second and the time to spare, on the serial console every five seconds |

## The Screen

On both generations the BBC is shown in 720 x 576 at 50 Hz, the standard 576p picture, with each of its lines drawn twice and a black border round it. Each BBC frame is one frame of the monitor, so movement is smooth. Most monitors take this mode. If yours shows it off to one side, try its auto-adjust button.

Every screen mode is shown at its full resolution, the 640 dots across of MODE 0 included.

## Discs

Put disc images in A:/bbc/disk, and press **F11**:

1. Choose drive 0 or drive 1 with the arrows, and press Enter.

2. Choose a disc image with the arrows, Page Up and Page Down, and press Enter. Folders are listed too: Enter opens one, and Backspace goes back up.

A disc put in drive 0 starts the BBC from it, as SHIFT-BREAK does. Esc goes back a step. When a drive has a disc in it, the first line of its list takes the disc out.

The images are .ssd (one side) and .dsd (two sides) for the BBC's DFS, and .adf and .adl for the Master's ADFS. Discs named drivea.ssd and driveb.ssd in A:/bbc/disk are put in drives 0 and 1 each time the program starts.

**Saving.** What the BBC writes to a disc goes into its image on the card at once, so SAVE, *SAVE, a word processor's files and *FORM all last. To keep a disc as it is, start the program with -ro, or make the image file read-only on a PC: the BBC then finds the disc write protected, and says Disc read only.

An MMB file, the one file of many discs that MMFS uses on a real BBC (BEEB.MMB), cannot be opened: take the discs out of it on a PC first. There is no cassette.

## ROMs

### The ROMs Built In

{widths: 14,38}
| Model | Its ROMs |
| BBC Master 128 | MOS 3.20, with BASIC 4, VIEW B3.0, ViewSheet B1.0, Edit, Terminal 1.20, ADFS 1.50 and DFS 2.24 |
| BBC Model B | OS 1.20, BASIC 2, and Acorn's 1770 DFS 2.26, which has the sideways RAM commands as well |

*HELP lists the ROMs of either model, and on a Master *ROMS shows each bank, with ? for an empty one.

### Adding ROMs: the roms Folder

Put ROM images in A:/bbc/roms, each a file ending .rom or .bin, of 16 KB or 8 KB. As the program starts, each is put into a free bank of sideways RAM, in the order of their names, and the BBC is then started again as if just switched on, so that it finds them. *2 ROMs from roms* shows for a moment when it has.

{widths: 16,36}
| Model | Banks |
| BBC Master 128 | 4, 5, 6 and 7: up to four ROMs |
| BBC Model B | 0 to 13: up to fourteen ROMs |

*HELP then lists them, and their commands work: *WORDWISE starts Wordwise-Plus, for example. A language ROM, such as Wordwise or VIEW, does not start by itself, as BASIC is still the language the BBC starts with: type its command.

The first name goes into the lowest bank. When two ROMs answer the same command, the one in the higher bank answers first, so start the names with 1, 2, 3 and so on to choose the order.

An 8 KB ROM is put in its bank twice, as a 16 KB socket of a real BBC shows it. A file that is not a ROM image still takes a bank, but the BBC does not see it. The serial console lists what went into each bank.

The ROMs are put in again whenever the model is changed in the F12 menu. Ctrl+Alt+Del keeps them.

### Loading a ROM from a Disc

A ROM image that is a file on a BBC disc can be loaded into a bank of sideways RAM with *SRLOAD, which the Master and the Model B (its DFS 2.26) both have:

```
*SRLOAD WWPLUS 8000 4 Q
```

loads the file WWPLUS from the disc in drive 0 into bank 4: Q makes it quicker. Then press **Ctrl+Pause**, CTRL-BREAK, so that the BBC finds it: BREAK alone does not. Choose a bank the roms folder has not filled: 4 to 7 on a Master, 0 to 13 on a Model B.

A ROM loaded this way is gone when the program ends or the model is changed. Copy it out of the disc image on a PC and put it in the roms folder to have it every time.

### ROMs That Need a Model B

Some early ROMs were made for the Model B and do not work on a Master: Wordwise-Plus 1.4A stops as soon as it starts on a Master, where 1.4F runs. If a ROM does nothing on the Master, try it on the Model B.

## The Settings

**F12** opens frank-micro's settings, over the BBC's screen. Up and down choose a line, left and right change it, Enter acts on the last three lines, and Esc goes back.

{widths: 18,34}
| Setting | What it does |
| Model | BBC B or Master 128 |
| Monitor | Colour, or a green or amber screen |
| Sound, Volume | The BBC's sound on or off, and how loud |
| Audio Out | Not used on the Colour Maximite 2: the sound always comes from its audio output |
| Limit Speed | On, the BBC's own speed; off, as fast as it will go |
| Startup Mode | The screen mode a Model B starts in, as its keyboard links set it. A Master's is set with *CONFIGURE MODE |
| CAPS/CTRL Keys | A/S makes A the BBC's CAPS LOCK and S its CTRL, for the games that use those two keys to move |
| Printer DAC | A sound device on the printer port, which a few programs use |
| Gamepad | Off, Arrows or Z X : / (see below) |
| Take a Screenshot | As Print Screen |
| Save changes and restart | Keeps the settings in micro.ini, and starts the BBC again |
| Back to BBC | Closes the menu |

The model and the startup mode change when the BBC starts again, the others at once. Settings that are not saved are forgotten when the program ends, and so are a Master's *CONFIGURE settings.

## The Keys

{widths: 18,34}
| Keys | What they do |
| F1 to F10 | The BBC's red keys, f0 to f9 |
| Pause | BREAK. Shift+Pause is SHIFT-BREAK, which starts the disc in drive 0, and Ctrl+Pause is CTRL-BREAK |
| End | COPY |
| Caps Lock | CAPS LOCK |
| Ctrl | CTRL |
| Backspace, Delete | DELETE |
| Tab, Esc, Enter, the arrows | TAB, ESCAPE, RETURN and the cursor keys |
| F11 | The discs |
| F12 | The settings |
| Print Screen | A screenshot, saved as a .bmp file in A:/bbc/screenshot |
| Ctrl+Alt+Del | Starts the BBC again, as if switched on. The ROMs in sideways RAM stay |
| Ctrl+Alt+End | Back to MMBasic |

The other keys give what is printed on them, following OPTION USBKEYBOARD, even where the BBC has the symbol on another key: Shift+8 gives *, which on a BBC is SHIFT and its : key. The BBC's £ is the ` key.

**Capitals.** A BBC starts with its CAPS LOCK on, as a real one does: letters come out as capitals, and Shift with a letter gives a small one. Caps Lock turns it off.

**MODE 7.** Teletext has other symbols in the places of some characters, so in MODE 7 a BBC shows [ as ←, ] as →, \ as ½, ^ as ↑, { as ¼, } as ¾, | as ‖ and ~ as ÷. The other modes show them as typed.

The BBC's SHIFT LOCK has no key here, and of the keypad only Enter is passed on.

Ctrl-C is a BBC key here, so it does not end the program.

## Gamepad

The first USB gamepad presses BBC keys, as the Gamepad setting chooses:

{widths: 16,18,18}
| Gamepad | Z X : / | Arrows |
| D-pad or left stick | Z left, X right, : up, / down | The cursor keys |
| A, Y | RETURN | RETURN |
| B, X | SPACE | SPACE |
| Start | RETURN | RETURN |
| Select | ESCAPE | ESCAPE |

Z X : / is the setting at first, as these are the keys most BBC games use. For a game that uses others, use the keyboard.

A DualShock 3 sends nothing until its PS button has been pressed.

## Sound

The BBC's sound chip comes out of the Colour Maximite 2's audio output. Its volume is in the F12 menu.

## The Serial Console

The serial console types into the BBC too, so that a program can be pasted: each character as the BBC's keys make it, Return as RETURN, and Backspace as DELETE. As well:

{widths: 18,34}
| Keys | What they do |
| Ctrl-F then 0 to 9 | f0 to f9 |
| Ctrl-F then u, d, l, r | The cursor keys |
| Ctrl-F then b | BREAK |
| Ctrl-F then B | CTRL-BREAK. Wait a few seconds before typing more |
| Ctrl-F then m, s | The discs, the settings |
| Ctrl-] | Back to MMBasic |

## What Has Been Tried

On a Generation 1, as a Master 128: BASIC; MODE 0, 2 and 7; the Master's Elite; Chuckie Egg, with the keyboard and with a gamepad; the sound; and the roms folder, with Wordwise-Plus 1.4F and Disc Doctor 1.09.

On a Generation 2: the same, and Wordwise-Plus editing, previewing and saving its text to a disc; saving and loading BASIC programs; *FORM; -ro; the Model B, with the roms folder and with *SRLOAD (EXMON II); changing the model in the menu; and the USB keyboard in the UK layout.

Not yet tried on the Colour Maximite 2: ADFS discs, the Printer DAC, Limit Speed off, and screenshots.

## Differences from B-em on a PC

- There are two models: the Model B with Acorn's 1770 disc interface, and the Master 128.
- The Model B's processor behaves as the Master's 65C12, so the few programs that use the original 6502's undocumented instructions may not run.
- There is no cassette, second processor, Econet, analogue joystick or mouse.
- MMB files, saved states and the debugger are not there.

## Leaving

Ctrl+Alt+End goes back to MMBasic, as does Ctrl-] on the serial console.
