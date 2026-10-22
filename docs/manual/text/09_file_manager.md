# File Manager

The FM command opens a full screen, two panel file manager in the style of the classic Norton Commander. It can also be started by pressing F1 at the command prompt. It works on the VGA screen and on the serial console using a VT100 terminal emulator. On the VGA screen it sets up the display as the editor does, in the default video mode with the editor's font.

![](docs/manual/images/fm.png){width=14}

Each panel shows the contents of one directory. Pressing A, B or C switches the active panel between the drives: A: (or B:) is the SD card and C: is a USB flash drive. The panel header shows the drive, the current path and the active filter, and the highlighted panel is the active one - all file operations apply to it. Files can be marked with the space bar (shown with a leading *) so that copy and delete operations work on several files at once, and each panel can have a wildcard filter (eg, *.BAS) and its own sort order. The status line at the bottom shows the result of the last operation.

Pressing ENTER on a directory opens it; on a .BAS file it runs the program, and the file manager returns with its state when the program ends (if the program stops with an error, the error is shown on the file manager's status line); on an audio file (WAV, FLAC, MP3 or MOD) it plays the file in the background while the file manager remains usable; and on an image file (BMP, JPG or PNG) it displays a full screen preview on a graphical display.

Marked audio files play as a track list. Mark them with the space bar and press ENTER: when every marked item in the panel is an audio file, the file manager plays the marked files in panel order, as the PLAY WAV, FLAC and MP3 commands play a directory, with each track starting when the one before it ends. A list holds up to 20 tracks and may mix WAV, FLAC and MP3 files; MOD files cannot be in a list. The status line shows the track that is playing (for example "Playing 2/5 name") and reports when the list has finished. The . or > key skips to the next track and , or < returns to the previous one, as PLAY NEXT and PLAY PREVIOUS do, and F6 or T stops the list. With any other marks (a directory, a program, an image) ENTER acts on the selected item as usual.

Pressing F4 opens the selected file in the full screen editor and exiting the editor returns to the file manager. If a program run from the file manager stops with an error, pressing F4 (or E) on that file opens the editor with the cursor on the line that caused the error. Pressing F2 within the editor saves and runs the program as normal.

The file manager remembers its state (paths, selection, sort order and filters) between invocations, until the Colour Maximite 2 is restarted. The full set of keys is (press F1 inside the file manager for a quick reminder):

{widths: 50,50}
| **Action** | **Key(s)** |
|---|---|
| Exit the file manager | ESC |
| Help screen | F1, H or ? |
| Switch the active panel | TAB, LEFT or RIGHT (Ctrl-S = left, Ctrl-D = right) |
| Move the selection up / down | UP, DOWN (Ctrl-E, Ctrl-X) |
| Page up / page down | PgUp, PgDn (Ctrl-P, Ctrl-L) |
| Top of the drive / end of the list | HOME, END (Ctrl-U, Ctrl-K) |
| Go to the parent directory | Backspace |
| Open / run the selected item | ENTER or F2 |
| Change the drive | A, B or C |
| Go to a path | G |
| Cycle the sort mode (name / datetime / type) | S |
| Type-select by filename prefix | / then type (ENTER opens, ESC cancels) |
| Set / clear the file filter | F9 (F) / F10 (W) |
| Mark or unmark the current item | SPACE |
| Mark all / clear all marks | * / \ |
| Play the marked audio files as a track list | ENTER or F2 (when every marked item is an audio file) |
| Copy the selected or marked items to the other panel | F5 or Y |
| Move the selected item to the other panel | M |
| Delete the selected or marked items | DEL |
| Recursive delete (including directories) | X |
| Rename the selected item | R |
| Duplicate the selected item | D |
| New file (create and edit) | N |
| Make a directory | F11 or K |
| Edit the selected file | F4 or E |
| List the selected file (paged) | F3 or L |
| Stop audio playback | F6 or T |
| Volume down / volume up | F7 or - / F8 or + |
| Previous / next track of a track list | , or < / . or > |

Notes: a file copy overwrites the destination without asking and a directory copy is recursive and works across drives. DEL only deletes empty directories - use X (which asks for two confirmations) to remove a directory and its contents. M moves within the same drive only; to move between drives use copy (F5) then delete (DEL). CTRL-C is ignored while the file manager is active, except in the LIST viewer where it exits the listing.

FM starts by clearing the variables and the other memory left by the last program to run, as EDIT does, so the file manager and its editor always have the memory they need.

## Using a Mouse

The file manager can be used with a mouse: a USB mouse, or a PS2 mouse set with OPTION MOUSE 0 (see Mouse Support in the chapter Hardware Features). A USB mouse plugged in while the file manager is running is used at once. The mouse pointer is shown on the screen while the file manager waits for a key.

{widths: 55,45}
| Action | Mouse |
|---|---|
| Select a file or directory, and make its panel the active one | Click it |
| Open a directory, run a program, play a sound or show an image (as Enter) | Double click it |
| Mark or unmark a file or directory (the selection does not move) | Right click it |
| Move the selection up or down | Scroll wheel, over a panel |
| Make a panel the active one | Click its heading |
| A key named on the top line (F1:Help, A/C:Drive, S:Sort, F3:List, SPACE:Mark, /:Type-select) | Click it; on A/C:Drive click A or C, or Drive for the other drive |
| Return from an image, the help screen or the end of a listing | Click anywhere |

The mouse is ignored while the file manager asks a question (a name, a path or Y/N) and during type-select. On the serial console a terminal emulator that reports mouse clicks (such as Tera Term) can be used in the same way.
