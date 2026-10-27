# Using MMBasic

## Commands and Program Input

At the command prompt you can enter a command and it will be immediately run. Most of the time you will do this to run a program or set an option. But this feature also allows you to test out commands at the command prompt.

To enter a program the easiest method is to use the EDIT command. This will invoke the full screen editor which is built into the Colour Maximite 2 and is described later in this manual. It includes advanced features such as search and replace, copy, cut and paste to and from a clipboard.

You could also compose the program on your desktop computer using something like Notepad and then transfer it via the XModem or ZModem protocols (see the XMODEM and ZMODEM commands), by streaming it up the console serial link (see the AUTOSAVE command) or by saving it to an SD card or a USB flash drive and transferring that to the Colour Maximite 2.

A fourth and convenient method of writing and debugging a program is to use MMEdit. This is a program running on your Windows computer which allows you to edit your program on your computer then transfer it via the serial console with a single click of the mouse. MMEdit was written by Jim Hiley and can be downloaded for free from https://geoffg.net/mmedit.html

One thing that you cannot do is use the old BASIC method of entering a program which was to prefix each line with a line number. Line numbers are optional in MMBasic so you can still use them if you wish but if you enter a line with a line number at the prompt MMBasic will simply execute it immediately.

## Editing the Command Line

When entering a line at the command prompt the line can be edited using the left and right arrow keys to move along the line, the Delete key to delete a character, the Backspace key to delete the character before the cursor and the Insert key to switch between insert and overwrite. Home and End move to the beginning and end of the line. At any point the Enter key will send the line to MMBasic which will execute it.

The up and down arrow keys will move through a history of previously entered commands which can be edited and reused.

## Console Keyboard/Display

Input can come from either the USB keyboard or from a computer using a terminal emulator via the USB serial interface (see the chapter Serial Console). Both the keyboard and the serial over USB can be used simultaneously and can be detached or attached at any time without affecting a running program.

The text output from MMBasic will be simultaneously sent to the VGA monitor and the serial over USB interface however the graphics commands operate on the video output only.

This behaviour can be changed with the OPTION CONSOLE command. Using this it is possible to turn either console off or on and save the setting so that it will be automatically applied on reboot.

## Shortcut Keys

The function keys on the keyboard or the serial console can be used at the command prompt to automatically enter common commands. The first four function keys (F1 to F4) will insert the text followed by the Enter key so that the command is immediately executed:

>> F1	FM (the file manager)
>> F2	RUN
>> F3	LIST
>> F4	EDIT

Function keys F5 to F10 will insert the text then position the cursor between the quote marks at the end so that the file name can be directly entered. Pressing Enter will then execute the command:

>> F5	AUTOSAVE ""
>> F6	XMODEM RECEIVE ""
>> F7	XMODEM SEND ""
>> F8	EDIT ""
>> F9	LIST ""
>> F10	RUN ""

Function keys F11 and F12, and Shift-F3, Shift-F4, Shift-F7 and Shift-F8, can be programmed with custom text: see the commands OPTION F11, OPTION F12, OPTION F15, OPTION F16, OPTION F19 and OPTION F20 (F15 is Shift-F3 and so on). So can F1 and F5 to F9, as on the PicoMite (OPTION F1, OPTION F5 to OPTION F9): the text set replaces the key's own above, and setting it to "" gives that back. Shift-F5 clears the console and the connected VT100 screen, and Shift-F6 sets the size of the terminal emulator's window to that of the screen.

## Line Numbers and Program Structure

A BASIC program starts at the first line and continues until it runs off the end of the program or hits an END command - at which point MMBasic will display the command prompt (>) on the console and wait for something to be entered.

The structure of a program line is:

```
[line-number] [label:] command arguments [: command arguments] …
```

A label or line number can be used to mark a line of code. A label has the same specifications (length, character set, etc) as a variable name but it cannot be the same as a command name. When used to label a line, the label must appear at the beginning of a line but after a line number (if used) and be terminated with a colon character (:).

Commands such as GOTO can use labels or line numbers to identify the destination (in that case the label does not need to be followed by the colon character). For example:

```
    GOTO xxxx
      - - -
    xxxx:  PRINT "We have jumped to here"
```

Multiple commands separated by a colon can be entered on the one line (as in A = 24.6 : PRINT A).

Appendix L (Programming in BASIC - A Tutorial) at the rear of this manual provides a comprehensive tutorial on the language which will take you through the fundamentals in an easy to read format with lots of examples.

## All Programs Are Run From the SD Card

On the Colour Maximite 2 all programs reside on the SD card (or a USB flash drive) which acts as the "disk drive" for the computer. As a result the SD card must be present for most operations. This is different from the original Maximite where the SD card was not necessarily required.

When you edit a program you are editing the program on the SD card, when you run a program you will run it from the SD card, etc. The reason for this arrangement is that when a program is loaded into memory for execution MMBasic will do a lot of pre-processing to speed up execution. This includes inserting any include files specified in the source, stripping out all comments, removing unnecessary spaces and so on. The resultant program is then saved in program memory but it cannot be edited or listed because after pre-processing the executable program is not easily human readable.

The benefit of this arrangement is a marked improvement in the speed of execution and that much larger programs can fit into program memory and be run.

The main commands used to manage a program are:

{widths: 25,75}
| Command | Description |
|---|---|
| RUN "prog" | Run the program called prog located on the SD card. |
| LIST "prog" | List the program called prog on the console screen. This will pause every screen full and any key press will continue the listing. |
| EDIT "prog" | Edit the program called prog located on the SD card. |

For example: RUN "hello.bas"

Note that the file name must be surrounded by double quotes as shown above. This is because the file name is a string and in MMBasic all string constants (ie, not a variable) must be quoted. In all cases the file extension ".BAS" will be automatically added if an extension was not specified in the command line. (See also the * (asterisk) shortcut which allows *hello to be used without quotes as an alias for RUN "hello".)

When RUN or EDIT are used they set what is known as the current program name. This is the file name that will be used if the commands RUN, EDIT and LIST are used without specifying a file name. For example, you could use the command EDIT "MyProg.bas" and that will set the current program name to "MyProg.bas". From then on you could use RUN, EDIT and LIST without a file name and they will refer to "MyProg.bas" on the SD card. (* can also be used to run the current program without supplying the program name.)

To clear the current program name and erase the processed program held in program memory you can use the command NEW. This also clears all variables, closes all files, etc (ie, resets MMBasic).

There are other commands that operate on program files: AUTOSAVE, LIST ALL, XMODEM and ZMODEM. These are used for sending/receiving programs via the serial console to or from the SD card. AUTOSAVE will also update the current file name so that after a file has been transferred the RUN command without a file name will run that program. Note that the Colour Maximite 2 does not need LOAD or SAVE commands for programs (LOAD and SAVE are used for images and other data).

Finally, all commands referred to above (with the exception of RUN) can be used with a different file extension to operate on files that are not programs. For example, EDIT "data.txt" will edit the text file on the SD card called "data.txt". In this case the current program name will not be changed.

## Status Line

When at the command prompt MMBasic will display a status line at the bottom of the VGA screen. On the left side this will show the current directory on the SD card while the text in the centre is the current program name (ie, the file that will be used if the commands RUN, EDIT and LIST are used without specifying a file name). On the right side it shows the current time and date.

The status line can be turned off with the OPTION STATUS OFF command.

## Running Programs

A program is set running by the RUN command. You can interrupt MMBasic and the running program at any time by typing CTRL-C on the console input and MMBasic will return to the command prompt. The key used to interrupt a program can be changed with the OPTION BREAK command.

The running program is normally held in non volatile flash memory. This means that it will not be lost if the power is removed and, if you have the AUTORUN feature turned on, the program will automatically run when power is restored (use the OPTION AUTORUN command to turn it on). Normally an SD card holding the original program must be present in order to run a program but this is one of the exceptions and allows you to change the SD card for recording data, etc.

## Expressions

In most cases where a number or string is required you can also use an expression. For example:

```
FNAME$ = "TEST"
LIST FNAME$ + ".BAS"
```

The RUN command is the only exception, in this case the filename argument must be a string constant surrounded by double quotes (ie, not an expression).

## Standards and Compatibility

MMBasic implements a large subset of Microsoft's GW-BASIC. There are numerous small differences due to physical and practical considerations but most ordinary BASIC commands and functions are essentially the same. An online manual for GW-BASIC is available at http://www.antonis.de/qbebooks/gwbasman/index.html and this provides a more detailed description of the commands and functions.

MMBasic also implements a number of modern programming structures documented in the ANSI Standard for Full BASIC (X3.113-1987) or ISO/IEC 10279:1991. These include SELECT CASE, SUB/END SUB, the DO WHILE … LOOP and structured IF .. THEN … ELSE … ENDIF statements.

The SELECT CASE commands allow the programmer to create a clear and structured decision tree that is more flexible and easier to understand when multiple decisions must be made. The DO WHILE … LOOP command makes it easy to build loops without using the GOTO statement. User defined subroutines and functions make it easy to add your own commands to MMBasic.

The IF… THEN command can span many lines with ELSEIF … THEN, ELSE and ENDIF statements as required and also spaced over many lines.

## Setting Options

Many options can be set by using commands that start with the keyword OPTION. They are listed in their own section of this manual. For example, you can set the video mode that the Colour Maximite 2 starts in with the command:

```
OPTION DEFAULT MODE 8
```

The options set at the command prompt are saved in the flash memory and are remembered when the power is removed.

## Saved Variables

Data is normally saved by the program to the SD card but sometimes there is a need to save a small amount of data which is independent of the SD card and will survive a power failure, reboot, etc. This data might include menu choices, calibration data and configuration information.

This can be done with the VAR SAVE command which will save the variables listed on its command line in non volatile memory. These variables can be restored with the VAR RESTORE command which will add all the saved variables to the variable table of the running program. Normally VAR RESTORE is placed near the start of a program so that the variables are ready for use by the program.

The space reserved for saved variables is 4KB. This is a RAM memory which is kept alive by the battery on the motherboard. Writing to this memory is near instantaneous and data can be written an unlimited number of times without degradation.

Saved variables are cleared by the NEW and VAR CLEAR commands, by a firmware upgrade and when MMBasic is reset to its original configuration (see below).

## Timing

You can get the current date and time using the DATE$ and TIME$ functions and you can set them by assigning the new date and time to them. The Colour Maximite 2 has a battery backed clock so it will not lose the time even when powered off. If you find that the time drifts while the power is off you can use the OPTION RTC CALIBRATE command to correct for any inaccuracies.

You can freeze program execution for a number of milliseconds using PAUSE. MMBasic also maintains an internal stopwatch function (the TIMER function) which counts up in milliseconds. You can reset this timer to zero or any other number by assigning a value to the TIMER.

Using SETTICK you can setup up to four "ticks" which will generate regular interrupts with a period from one millisecond to over a month, and SETTICK FAST sets up one more, from a hardware timer, at up to tens of thousands a second.

## Watchdog Timer

It is possible to use the Colour Maximite 2 without a VGA monitor and also have nothing connected to the serial console. With OPTION AUTORUN ON set the program will run automatically on power up without human intervention.

However there is the possibility that a fault in the program could cause MMBasic to generate an error and return to the command prompt. This would be of little use in this situation as there would be nothing connected to the console. Another possibility is that the BASIC program could get itself stuck in an endless loop for some reason. In both cases the visible effect would be the same, the program would stop running until the power was cycled.

To guard against this the watchdog timer can be used. This is a timer that counts down to zero and when it reaches zero the processor will be automatically restarted (the same as when power was first applied), this will occur even if MMBasic is sitting at the command prompt. Following the restart the automatic variable MM.WATCHDOG will be set to true to indicate that the restart was caused by a watchdog timeout.

The WATCHDOG command should be placed in strategic locations in the program to keep resetting the timer and therefore preventing it from counting down to zero. Then, if a fault occurs, the timer will not be reset, it will count down to zero and the program will be restarted (assuming the AUTORUN option is set).

## PIN Security

Sometimes it is important to keep the data and program confidential. In the Colour Maximite 2 this can be done by using the OPTION PIN command. This command will set a pin number (which is stored in non volatile memory) and whenever MMBasic returns to the command prompt (for whatever reason) the user at the console will be prompted to enter the PIN number. Without the correct PIN the user cannot get to the command prompt and their only option is to enter the correct PIN or reboot the computer. When it is rebooted the user will still need the correct PIN to access the command prompt.

Because an intruder cannot reach the command prompt they cannot list or copy the program held in the program memory, they cannot change the program or change any aspect of MMBasic. Once set the PIN can only be removed by providing the correct PIN as set in the first place. If the number is lost the only method of recovery is to reset MMBasic as described below (which will erase the program).

Note that this is not a complete protection as it is possible to connect a debugger to the ARM Cortex-M7 CPU and access all areas of memory using that facility but it does deter casual access.

## The Serial Console

The default settings for the serial over USB console are 115200 baud, 8 bits, no parity and one stop bit. Using the OPTION BAUDRATE command the baud rate of the serial console can be changed to any other speed. Changing the console baud rate to a higher speed makes the full screen editor faster in redrawing the screen: a Generation 2 runs well at 460800 baud, but the USB to serial converter of a Generation 1 cannot run at that speed.

Once changed the console baud rate will be permanently remembered unless another OPTION BAUDRATE command is used to change it. Using this command it is possible to accidentally set the baud rate to an invalid speed and in that case the only recovery is to connect a VGA monitor and USB keyboard to change the baudrate or reset MMBasic as described below.

If the serial console is not required it can be disabled with the command OPTION CONSOLE SCREEN. This is reset when the program ends. If you want to permanently disable the serial console then the above command should be followed with OPTION CONSOLE SAVE. Disabling the serial console has two advantages:

- The built in editor will operate faster as it does not need to echo the edited text on the slow serial console.
- The serial port used by the serial console can be opened as COM3:.

## Resetting MMBasic

MMBasic can be reset to its original configuration by placing a link between pins 39 and 40 on the rear I/O connector while power is applied. This will result in the program memory and saved variables being completely erased and all options (security PIN, console baud rate, etc) reset to their initial defaults. The OPTION RESET command resets the options to their defaults from the command prompt (it does not erase the program or the saved variables).

When using pin 40 for general I/O care must be taken so that it does not look like it is shorted to ground on power up. The symptom of this is unexplained sudden losses of all options that have been changed and the erasure of the program held in flash.

## The Library

Using the LIBRARY feature it is possible to create BASIC functions, subroutines and embedded fonts and add them to MMBasic to make them permanent and part of the language. For example, you might have written a series of subroutines and functions that perform sophisticated bit manipulation; these could be stored as a library and become part of MMBasic and perform the same as other built-in functions that are already part of the language. An embedded font can also be added the same way and used just like a normal font.

To install components into the library you need to write and test the routines as you would with any normal BASIC routines. When they are working correctly you can use the LIBRARY SAVE command. This will transfer the routines (as many as you like) to a non-visible part of flash memory where they will be available to any BASIC program but will not show when the LIST command is used and will not be deleted when a new program is loaded or NEW is used. However, the saved subroutines and functions can be called from within the main program and can even be run at the command prompt (just like a built-in command or function).

Some points to note:

- Library routines act exactly like normal BASIC code and can consist of any number of subroutines, functions, embedded C routines and fonts. The only difference is that they do not show when a program is listed and are not deleted when a new program is loaded.
- Library routines can create and access global variables and are subject to the same rules as the main program – for example, respecting OPTION EXPLICIT if it is set.
- When the routines are transferred to the library MMBasic will compress them by removing comments, extra spaces, blank lines and the hex codes in embedded C routines and fonts. This makes the library space efficient, especially when loading large fonts. Following the save the program area is cleared.
- You can use the LIBRARY SAVE command multiple times. With each save the new contents of the program space are appended to the already existing code in the library.
- You can use line numbers in the library but you cannot use a line number on an otherwise empty line as the target for a GOTO, etc. This is because the LIBRARY SAVE command will remove any blank lines.
- You can use READ commands in the library. DATA statements are searched as a single sequence: the main program first and then the library, so when the program's DATA statements are exhausted (or the program has none) reading continues with the DATA statements in the library. To read the library's DATA from the beginning use the RESTORE command before the first READ; used within library code this resets the pointer to the library space.
- Labels belong to the library and the program together: a label must not be defined in both, and the program must not have a subroutine or function with the name of one in the library. Either is reported when the program is run (Duplicate label or Duplicate name).
- The library is saved in the last 128KB of the program flash memory (flash slot 6), which cannot then be used for a program (see OPTION FLASH).
- You can see what is in the library by using the LIBRARY LIST command which will list the contents of the library space.
- The library can be saved to a file using LIBRARY DISK SAVE fname$ and restored using LIBRARY DISK LOAD fname$.

To delete the routines in the library space you use the LIBRARY DELETE command. This will clear the space and return the flash used by the library back to being available for normal programs. OPTION RESET does not delete the library.

A program can install its own library with LIBRARY LOAD fname$, which reads one or more BASIC source files (up to eight) and makes them the library. This is intended for programs that are distributed with a library of their own: the library becomes part of the program rather than something that must be installed first. The command must be the first statement in the program and the program is restarted once the library has been installed, so that the library's initialisation code runs before the program proper. Because the command remembers a hash of the source files it will do nothing at all when the library already matches, so it is safe to leave at the top of a program permanently.

With the RAM option (LIBRARY LOAD fname$, RAM) the library is kept in RAM instead of the flash memory. A RAM library lasts until the next RUN, NEW or program load, so the program loads it every time it starts, and the flash library is left as it is.

## Program Initialisation

The library can also include code that is not contained within a subroutine or function. This code (if it exists) will be run automatically before a program starts running (ie, via the RUN command). This feature can be used to initialise constants or setup MMBasic in some way. For example, if you wanted to set some constants you could include the following lines in the library code:

```
CONST TRUE = 1
CONST FALSE = 0
```

For all intents and purposes, the identifiers TRUE and FALSE have been added to the language and will be available to any program that is run.

## MM.STARTUP

There may be a need to execute some code on initial power up, perhaps to initialise some hardware, set some options or print a custom start-up banner. This can be accomplished by creating a subroutine with the name MM.STARTUP in the program. When the Colour Maximite 2 is first powered up or reset it will search the program held in program memory (the program that was last run) for this subroutine and, if found, it will be run once.

For example, the program could contain the following code:

```
SUB MM.STARTUP
  PRINT "Hello from " MM.DEVICE$
END SUB
```

This would print a greeting on every power up or reset.

After the code in MM.STARTUP has been run MMBasic will run the program if OPTION AUTORUN is on, otherwise it will return to the command prompt.

Note that you should not use MM.STARTUP for general setup of MMBasic (like dimensioning arrays, opening communication channels, etc) before running a program. The reason is that when you use the RUN command MMBasic will first clear the interpreter's state ready for a fresh start.

## MM.PROMPT

If a subroutine with this name exists it will be automatically executed by MMBasic instead of displaying the command prompt. This can be used to display a custom prompt, set colours, define variables, etc all of which will be active at the command prompt.

Note that MMBasic will clear all variables and I/O pin settings when a program is run so anything set in this subroutine will only be valid for commands typed at the command prompt (i.e. in immediate mode).

As an example the following will display a custom prompt:

```
SUB MM.PROMPT
  PRINT TIME$ "> ";
END SUB
```

Note that while constants can be defined, they will not be visible because a constant defined inside a subroutine is local to a subroutine. However, DIM will create variables that are global and that should be used instead.

## MM.END

If a subroutine named MM.END exists in the program it will be executed whenever the program ends with an END command or by running off its last line (an implied END). It is not executed if the program ends with an error or a Ctrl-C.

The optional parameter 'noend' to the END command can be used to block execution of the MM.END subroutine if needed (see the END command for more information).
