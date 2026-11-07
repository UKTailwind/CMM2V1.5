# The ZX Spectrum Emulator

This is pico-spec, DnCraptor's port of the ESPectrum emulator by Victor Iborra and David Crespo. It runs at the Spectrum's 50 frames a second on both generations, in MODE 7,8.

## Starting It

```
CHDIR "A:/spectrum"
RUN "spectrum.app"
```

It starts as a 48K Spectrum. The ROMs are built into the program, so it needs nothing else on the card. The F1 menu chooses another machine. The 48K and 128K have been tried here; the others in its menu are as pico-spec has them.

## Loading Games

Put tapes and snapshots in A:/spectrum, and open them with **F5**, which starts a tape by itself. You can also choose a tape and type LOAD "" as you would on a real Spectrum.

The program calls the folder it was started in SD:/, and keeps its settings there.

## The Keys

{widths: 16,36}
| Keys | What they do |
| F1 | The menu |
| F3, F4 | Load and save a snapshot, in numbered slots |
| F5 | Open a file |
| F6 | Start or stop the tape |
| F9, F10 | Volume down and up |
| F11 | Reset the Spectrum |
| F12 | Restart the emulator. It asks first, with No chosen |
| Shift | Caps Shift |
| Ctrl | Symbol Shift |
| Ctrl+Alt+End | Back to MMBasic |

Ctrl-C is a Spectrum key here, Symbol Shift and C, so it does not end the program.

Snapshots are how you save your place in a game: F4 saves one and F3 loads it.

USB gamepads step through the emulator's menus, and are the Spectrum's joysticks. They are Kempston joysticks unless the menu says otherwise.
