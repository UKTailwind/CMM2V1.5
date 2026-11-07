# The NES Emulator

This is frank-nes, Mikhail Matveev's NES emulator, on the QuickNES core by Shay Green. It runs at full speed on both generations, in MODE 7,8.

## Starting It

```
CHDIR "A:/nes"
RUN "nes.app"
```

Its cartridges are the .nes files in A:/nes. It comes with two free games, Nova the Squirrel and Thwaite. To add more, copy the .nes files into the folder.

The welcome screen moves on by itself after 10 seconds, whether or not a button has worked.

## The Controls

It uses USB gamepads and the USB keyboard.

{widths: 20,16,16}
| Pad | Keyboard | NES |
| D-pad or stick | The arrows or WASD | D-pad |
| Circle, Triangle | X | A |
| Cross, Square | Z | B |
| Start | Enter | Start |
| Select | Space | Select |

These work at any time:

{widths: 26,26}
| Keys | What they do |
| Start and Select together, or F12 | The menu |
| F11 | Back to the cartridges |
| Ctrl+Alt+End, or Ctrl-C | Back to MMBasic |

## If a Gamepad Does Nothing

- A DualShock 3 sends nothing until its PS button has been pressed.
- A generic pad is usually a low speed device, and has to go straight into the Colour Maximite 2, not through a hub.
- If a generic pad gives no response after a restart, unplug it and plug it in again.
