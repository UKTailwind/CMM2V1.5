# Full Screen Editor

An important productivity feature is the built-in full screen editor. When running it looks like this:

![](docs/manual/images/editor.png){width=14}

The editor operates with both consoles: the VGA screen with the USB keyboard, and the serial console using a VT100 terminal emulator (such as Tera Term).

The editor is a very productive method of writing a program. With the command EDIT "file" you can enter and edit your program then, by pressing the F2 key, you can save and run the program. If your program stops with an error, pressing the function key F4 (EDIT) at the command prompt will load the editor and position the cursor at the line that caused the error. This edit/run/edit cycle is very fast.

The editor works on a file on the SD card (A: or B:) or a USB flash drive (C:). Files of up to about 1MB can be edited.

If you have previously used an editor like Windows Notepad you will find that the operation of this editor is familiar. The arrow keys will move the cursor around in the text, Home and End will take you to the beginning or end of the line. Page Up and Page Down will do what their titles suggest. The Delete key will delete the character at the cursor and Backspace will delete the character before the cursor. The Insert key will toggle between insert and overtype modes. About the only unusual key combination is that two Home key presses will take you to the start of the program and two End key presses will take you to the end.

The best way to learn how to use the editor is to simply fire it up and experiment.

## Edit Functions

When the editor starts the cursor will be positioned at the last editing position or, if your program had been stopped by an error, the line that caused the error. At the bottom of the screen the status line will list details such as the current cursor position and the common functions supported by the editor.

In more detail these functions are:

>> ESC	This will cause the editor to abandon all changes and return to the command prompt with the file unchanged. If you have changed the text, you will be asked if you really want to abandon your changes.
>> F1: SAVE	This will save the file and return to the command prompt.
>> F2: RUN	This will save the file and immediately run it.
>> F3: FIND	This will prompt for the text that you want to search for, offering the text last searched for (Enter accepts it, typing replaces it). When you press Enter the cursor will be placed at the start of the first match found, which is brought into view. The search ignores case and wraps round at the end.
>> F4: MARK	This is described in detail below.
>> F5: PASTE	This will insert (at the current cursor position) the text that had been previously cut or copied (see below).
>> F6: FIND AGAIN	Once you have used the search function you can repeat the search by pressing F6 (Shift-F3 is also valid).
>> F7: REPLACE	If the cursor is positioned after F3 or F6, this will open a dialog allowing you to directly enter a string into the paste buffer and on pressing Enter, replace the search target string.
>> F8: REPLACE AGAIN	If the cursor is positioned after F3 or F6 the system will replace the search target string with the contents of the paste buffer.
>> F9: INSERT	You will be prompted for a file name and when Enter is pressed that file will be inserted at the current cursor position.
>> F10: SAVE CLIPBOARD	You will be prompted for a file name and when Enter is pressed the contents of the clipboard (if any) will be saved to that file. See the mark mode below.
>> F11: HELP	This will show the help for the keyword at the cursor (or just before it). The help is taken from the file A:/help.txt if the SD card has one, otherwise from the help built into MMBasic. Use the arrow keys and Page Up/Page Down to scroll through it, and Esc or F11 to go back to the text.
>> F12: BEAUTIFY	This will re-indent the program using a consistent two space indent for the body of multiline structures such as IF…END IF, FOR…NEXT, DO…LOOP, SELECT CASE…END SELECT, SUB…END SUB and FUNCTION…END FUNCTION. Single line IF statements are not changed, comment lines are indented at the current level and in programs that use line numbers the statements are aligned in a column after the longest line number. A single blank line is always placed after each END SUB and END FUNCTION. If the program contains any other blank lines you will be asked "KEEP BLANK LINES? (Y/N)" - answer Y (or press Enter) to keep them, N to remove them or press ESC to cancel. When finished the cursor will be placed at the start of the program. Ctrl-A can also be used to start the beautify.

## Additional Keys

These keys are particular to the Colour Maximite 2's editor:

>> Shift-Tab	Delete up to a tab stop's worth of spaces at the cursor (at the start of a line, the indent goes back by one tab stop).
>> Shift-Del	At the start of a line, delete its leading spaces; elsewhere the same as Delete.
>> Shift-arrow	Start mark mode, with the arrow as its first move (Shift and the arrows also work in mark mode).
>> Page Up/Down	The first press moves the cursor to the top (or bottom) row of the screen, the next moves a page.
>> Print Screen	Save a picture of the screen to the SD card as A:/SCREEN_yyyymmdd_hhmmss.PNG (Ctrl-\ does the same: see Appendix I).

The Tab key inserts spaces up to the next tab stop (see OPTION TAB).

## Saving

When the editor saves a file it first writes the text to a temporary file (name.$$$), then renames the old file to name.bak (replacing any older .bak) and finally renames the temporary file to name. Whatever goes wrong, the file or its .bak is whole. If the save fails (for example because the SD card has been removed) the editor says " SAVE FAILED: " with the reason and stays open with your text, so that you can correct the problem and save again.

In a program (a .bas file) a line can be at most 255 characters long, and F1 and F2 refuse to save a longer line with a message. Other files (for example .txt files) can have longer lines.

## Mark Mode

Pressing the mark key (F4) will put the editor into mark mode which will allow you to cut or copy text to the clipboard or delete or save text.

When in the mark mode the arrow keys are used to mark the text which will be highlighted in reverse video. If you reach the beginning or end of the screen the arrow keys will continue to mark text as the screen scrolls. This also works with Page Up and Page Down.

In the mark mode the status line will change to show the functions available in this mode. These are:

>> ESC	Exit mark mode without changing anything.
>> F4: CUT	Copy the marked text to the clipboard and remove it from the program.
>> F5: COPY	Just copy the marked text to the clipboard.
>> F10: SAVE	Prompt for a filename and when Enter is pressed, save the marked text to the file.
>> DELETE	Delete the marked text leaving the clipboard unchanged.

The clipboard can hold as much text as there is memory for, and the status line shows how many characters it holds. A paste inserts the clipboard's text exactly as it is, tabs included.

## Alternate Keys

You can also use control keys instead of the function keys listed above. These control keystrokes are:

{widths: 13,12,13,12,13,12,13,12}
| Key | Control | Key | Control | Key | Control | Key | Control |
|---|---|---|---|---|---|---|---|
| LEFT | Ctrl-S | RIGHT | Ctrl-D | UP | Ctrl-E | DOWN | Ctrl-X |
| HOME | Ctrl-U | END | Ctrl-K | PageUp | Ctrl-P | PageDn | Ctrl-L |
| DEL | Ctrl-] | INSERT | Ctrl-N | F1 | Ctrl-Q | F2 | Ctrl-W |
| F3 | Ctrl-R | F4 | Ctrl-T | F5 | Ctrl-Y or Ctrl-V | F6 | Ctrl-G |
| F7 | Ctrl-F | F8 | Shift-F5 | F9 | Ctrl-O | F10 | Ctrl-B |
| F12 | Ctrl-A |  |  |  |  |  |  |

## Long Lines

Long lines are handled by horizontal scrolling. A line that is longer than the display width will initially show just its first part, however, when the cursor is moved beyond the right hand margin, the line that the cursor is on will scroll horizontally so that any part of it can be seen and edited. The other lines on the screen are not affected and continue to show their first part.

## Using a Mouse

The editor can be used with a mouse: a USB mouse (which needs no configuration), or a PS2 mouse or Hobbytronic mouse interface configured with OPTION MOUSE (see Mouse Support in the chapter Hardware Features). The mouse pointer is shown on the screen while the editor is running.

- Clicking the left button moves the edit cursor to the pointer (ie, the same as using the cursor keys). Double clicking at the left or right edge of the screen is the same as Home or End.
- Holding down the left button (for half a second, or while moving the mouse) starts mark mode at the pointer, and the text is marked as the mouse is moved.
- Clicking the right button moves the cursor to the pointer and starts mark mode (the same as F4). In mark mode it cuts the marked text (F4).
- Clicking the scroll wheel (the middle button) pastes the clipboard at the pointer (the same as F5). In mark mode it copies the marked text (F5).
- Turning the scroll wheel moves up and down through the text.

The buttons work only inside the editing area: on the status line they do nothing.

## Using a Touch Screen

With a USB touch screen (see the chapter Keyboard, Mouse and Gamepad), as on the PicoMite:

- A tap in the text moves the edit cursor there.
- A swipe up or down is Page Up or Page Down, and a swipe left or right is Home or End.

With OPTION SCREEN KEYBOARD set, the on-screen keyboard is shown below the text and types into the editor; taps on it are its keys, not moves of the cursor.

## Colour Coded Editor Display

The editor can colour code the edited program with keywords, numbers, strings and comments displayed in different colours. This feature can be turned on or off at the command prompt with the commands:

```
OPTION COLOURCODE ON
```

or

```
OPTION COLOURCODE OFF
```

This option is saved in non-volatile memory and will be automatically applied on start-up. Only BASIC programs (.bas files) are colour coded.
