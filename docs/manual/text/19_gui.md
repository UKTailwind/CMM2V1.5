# GUI Controls

The Colour Maximite 2 incorporates a suite of advanced graphic controls that respond to input from a mouse or a USB touch screen. These include on screen switches, buttons, indicator lights, number and text boxes, gauges, etc. MMBasic will draw the control and animate it (i.e. a switch will appear to depress when activated). All that the BASIC program needs to do is invoke a single line command to specify the basic details of the control. These functions make it easy to create a control panel to manage any control functions like a lathe, motor controller, heating system, small industrial process and so on.

![GUI controls](pdf:42:117){width=9}

The controls are created with the GUI command: GUI BUTTON, SWITCH, CHECKBOX, RADIO, LED, FRAME, NUMBERBOX, TEXTBOX, FORMATBOX, SPINBOX, DISPLAYBOX, CAPTION, GAUGE, BARGAUGE, AREA, LISTBOX and SLIDER, and managed with GUI PAGE, SETUP, DELETE, DISABLE, ENABLE, HIDE, SHOW, RESTORE, REDRAW, FCOLOUR and BCOLOUR. The value of a control is read or set with the CTRLVAL() function and command, and MSGBOX() displays a message box and returns the button pressed. The GUI controls are described in detail in the document GUI Controls and Programming.pdf which is included in the firmware download file; GUI LISTBOX (a list to choose from), GUI SLIDER (a value set with a thumb on a track) and GUI CLICK, which came later from the PicoMite, are described with the GUI command.

Before the controls can be used, memory must be reserved for them with the command OPTION MAXCTRLS n, where 'n' is the number of controls that the program will use (up to 2000). Without it a program that creates a control stops with the error "No memory allocated for GUI controls".

## Using a Mouse or a Touch Screen

The controls respond to a finger on a USB touch screen, where it touches, and to a click of the left mouse button. A USB touch screen and a USB mouse are used as soon as they are plugged in, with no option to set; a PS2 or I^^2^^C mouse is set with OPTION MOUSE. The first GUI command a program runs opens the mouse and shows a pointer at it, linked to the mouse as GUI CURSOR ON : GUI CURSOR LINK MOUSE would link it, and the left mouse button acts on the control under the pointer. The GUI CURSOR commands work on the pointer: GUI CURSOR COLOUR and GUI CURSOR LOAD set its colour and its shape, and after GUI CURSOR UNLINK MOUSE the program moves it with GUI CURSOR x, y (from a joystick, say). A program can create its controls before a mouse is there: a USB mouse plugged in later is used at once. The pointer goes at the end of the program.

GUI CLICK clicks the controls at the pointer without a mouse: from the program (at a position, or held down and released), or with a button on a pin.

A press belongs to what made it, a finger or the mouse (or GUI CLICK), until it is released. The CLICK() function reads the mouse button and GUI CLICK, and TOUCH() a touch screen as well: X and Y return where the press is, DOWN and UP whether there is one, REF the control under it, and LASTX, LASTY and LASTREF where and on what the last press was. TOUCH() also reports a second finger, and the gestures: a swipe, a tap, a double tap, a long press, a pinch, a twist and a two-finger tap.

The GUI INTERRUPT command arms interrupt routines that are called when the screen is touched or the button is pressed, and when it is released:

```
GUI INTERRUPT Down [, Up]
```

'Down' is the subroutine called when the button is pressed and the optional 'Up' subroutine is called on release. GUI INTERRUPT 0 disables the interrupts.
