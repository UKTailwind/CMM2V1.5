# Quick Start Tutorial

The following assumes that you have built or purchased the Colour Maximite 2, loaded the firmware, attached a keyboard and VGA monitor, powered it up and tested that it runs. At that point you should have the command prompt (a greater than symbol ">") displayed on the monitor.

## Command Prompt

Most interaction with MMBasic is done via the console at the command prompt (ie, the greater than symbol (>) on the console). On startup MMBasic will issue the command prompt and wait for some command to be entered. It will also return to the command prompt if your program ends or if it generated an error message.

When the command prompt is displayed you have a range of commands that you can execute. Typically these would list a program (LIST) or edit it (EDIT) or set some options (the OPTION command). Most times the command is just RUN which instructs MMBasic to run the program.

When entering a line at the command prompt the line can be edited using the arrow keys to move along the line, the Delete key to delete a character and the Insert key to switch between insert and overwrite. The up and down arrow keys will move through a list of previously entered commands which you can edit and reuse. Finally the Enter key will cause MMBasic to execute whatever is showing at the command prompt.

Almost any command can be entered at the command prompt and this is often used to test a command to see how it works. A simple example is the PRINT command, which you can test by entering the following at the command prompt:

```
PRINT 2 + 2
```

and not surprisingly MMBasic will print out the number 4 before returning to the command prompt.

Here are a few more things that you can try out. What you type is shown in bold and the Colour Maximite's output is shown in normal text.

Try a simple calculation:

```
> **PRINT 1/7**
 0.1428571429
```

See how much memory you have:

```
> **MEMORY**
Program:
   0K ( 0%) Program (0 lines)
 516K (100%) Free

Data:
   0K ( 0%) 0 Variables
   0K ( 0%) General
24288K (100%) Free
```

(The free data memory depends on the Colour Maximite 2: this is a Generation 2.)

What is the current time?

```
> **PRINT TIME$**
10:04:01
```

What is the current date?

```
> **PRINT DATE$**
20-10-2026
```

Count to 20:

```
> **FOR a = 1 to 20 : PRINT a; : NEXT a**
 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
```

## Your First Program

To enter a program you can use the EDIT command which is described later in this manual. To get a quick feel for how it works, try this sequence:

- Make sure that you have a properly formatted SD card inserted into the SD card slot.
- At the command prompt type EDIT "hello.bas" followed by the Enter key.
- The editor should start up and you can enter this line: PRINT "Hello World"
- Press the F1 key on your keyboard. This tells the editor to save your program and exit to the command prompt.
- At the command prompt type RUN "hello.bas" followed by the Enter key.
- You should see the message: Hello World

Congratulations. You have just written and run your first program on the Colour Maximite 2. If you type EDIT again you will be back in the editor where you can change or add to your program.

## Something More Complicated

A more interesting program would be to fill the screen with coloured bubbles.

For this you need to know a little more about the editor. If you have used any full screen text editor in the past you will find the operation of this editor familiar. The arrow keys will move your cursor around in the text while the Home and End keys will take you to the beginning or end of the line. The Delete key will delete the character at the cursor and Backspace will delete the character before the cursor.

To enter the bubbles program you should use the command EDIT "bubbles.bas" at the command prompt. Then type in this short program:

```
DO
  r = RND * 255
  g = RND * 255
  b = RND * 255
  CIRCLE RND * 800, RND * 600, RND * 100,,, 0, RGB(r,g,b)
  PAUSE 5
LOOP
```

Press the F2 key which will save your program and automatically run it. You should see the screen continuously fill with hundreds of coloured bubbles as shown below.

![bubbles](pdf:9:22){width=7}

If there was an error you will get a message with the line number and a description of the error. If you then enter the command EDIT you will be taken back into the editor with the cursor positioned on the line that caused the error. Correct the error and then save and run the program by pressing F2 again.

In this program we first set three variables (r, g and b) to random numbers in the range of zero to 255. The random number generator is called RND and it returns a random number in the range of zero to 0.999999. We multiply it by 255 to give us a random number from 0 to 255. Then we draw a circle at a random position (again using the random number generator) with a random radius using the three colours previously calculated (ie, r, g and b). This code is contained within a DO...LOOP which instructs MMBasic to keep repeating this code (and drawing bubbles) forever.

You will notice that while this program is running you will not get the command prompt back. This is because MMBasic is now busy executing your program and drawing coloured bubbles. You can stop the program whenever you want by entering CTRL-C at the console and you should get the command prompt back again.

The purpose of the PAUSE 5 command in the program is to slow down the program so that you have time to see the bubbles. To see how fast the Colour Maximite 2 can really go you could go back into the editor and change that line to PAUSE 0 and then rerun the program.

The program is written for the default video mode, MODE 1, which is 800 x 600 pixels.

For a more in-depth tutorial and a description of programming in BASIC you should read Appendix L (Programming in BASIC - A Tutorial) at the end of this manual, and download Programming with the Colour Maximite 2 which can be found at http://geoffg.net/maximite.html (scroll to the bottom of the page).
