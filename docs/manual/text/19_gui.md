# GUI Controls

The Colour Maximite 2 incorporates a suite of advanced graphic controls that respond to input from a mouse. These include on screen switches, buttons, indicator lights, number and text boxes, gauges, etc. MMBasic will draw the control and animate it (i.e. a switch will appear to depress when activated). All that the BASIC program needs to do is invoke a single line command to specify the basic details of the control. These functions make it easy to create a control panel to manage any control functions like a lathe, motor controller, heating system, small industrial process and so on.

![GUI controls](pdf:42:117){width=9}

The controls are created with the GUI command: GUI BUTTON, SWITCH, CHECKBOX, RADIO, LED, FRAME, NUMBERBOX, TEXTBOX, FORMATBOX, SPINBOX, DISPLAYBOX, CAPTION, GAUGE, BARGAUGE and AREA, and managed with GUI PAGE, SETUP, DELETE, DISABLE, ENABLE, HIDE, SHOW, RESTORE, REDRAW, FCOLOUR and BCOLOUR. The value of a control is read or set with the CTRLVAL() function and command, and MSGBOX() displays a message box and returns the button pressed. The GUI controls are described in detail in the document GUI Controls and Programming.pdf which is included in the firmware download file.

Before the controls can be used, memory must be reserved for them with the command OPTION MAXCTRLS n, where 'n' is the number of controls that the program will use (up to 2000). Without it a program that creates a control stops with the error "No memory allocated for GUI controls".

## Using a Mouse

The controls respond to a click of the left mouse button. The mouse must first be set up with OPTION MOUSE (a USB mouse is channel 2): without it, creating a control gives the error "Mouse not enabled". The first control a program creates opens the mouse and shows the cursor, which then tracks the mouse, and the left mouse button acts on the control under the cursor. GUI CURSOR COLOUR and GUI CURSOR LOAD set the cursor's colour and its shape.

The current state of the mouse button can be read at any time with the CLICK() function: CLICK(X) and CLICK(Y) return where it was clicked, CLICK(DOWN) and CLICK(UP) whether the button has been pressed or released, CLICK(REF) the control under the cursor, and CLICK(LASTX), CLICK(LASTY) and CLICK(LASTREF) where and on what the last click was.

The GUI INTERRUPT command arms interrupt routines that are called when the button is pressed or released:

```
GUI INTERRUPT Down [, Up]
```

'Down' is the subroutine called when the button is pressed and the optional 'Up' subroutine is called on release. GUI INTERRUPT 0 disables the interrupts.
