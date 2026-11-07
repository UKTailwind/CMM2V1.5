# The Amstrad CPC Emulator

This is frank-cpc, rh1tech's port of the Caprice32 emulator. It runs at the CPC's 50 frames a second on both generations, in MODE 8,8.

## Starting It

```
CHDIR "A:/cpc"
RUN "cpc.app"
```

It starts as a CPC 6128. Press a key to clear its welcome screen. **F12** chooses the 464, 664 or 6128, and 64K, 128K or 576K of memory. Its ROMs are in its rom folder.

The program calls the folder it was started in /cpc, and keeps its settings there in cpc.ini.

## Loading Games

Put disks (.dsk), tapes (.cdt, .cas) and cartridges (.cpr) in A:/cpc/disk, and open them with **F11**.

- For a tape on the 664 or 6128, type |TAPE first.
- Leave Fast Tape set to Off.

**Saving your work.** A SAVE to a disk is lost when the disk is ejected or the program ends. Use a snapshot, in the F12 menu.

## The Keys

{widths: 16,36}
| Keys | What they do |
| F11 | Disks, tapes and cartridges |
| F12 | The settings |
| Ctrl+Alt+Del | Reset the CPC |
| Ctrl+Alt+End | Back to MMBasic |
| Backspace | DEL |
| Delete | CLR |
| Ctrl | CONTROL |
| Alt | COPY |
| Caps Lock | CAPS LOCK |
| Keypad Enter | The small ENTER |

The other keys give what is printed on them, following OPTION USBKEYBOARD, so the symbols are where your keyboard has them.

Ctrl-C is a CPC key here, so it does not end the program. USB gamepads are the CPC's joystick.
