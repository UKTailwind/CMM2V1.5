# Quake

This is id Software's Quake, with its shareware episode. It needs a Generation 2. It draws 320 by 240 in 256 colours, and its timed demo runs at about 53 frames a second with the sound on.

```
CHDIR "A:/quake"
RUN "quake.app"
```

Always start it from its folder. It finds its game data there, and keeps its settings and saved games in the id1 folder inside it.

The folder holds the whole of Quake's shareware version 1.06, unchanged. ID1/PAK0.PAK is what the game plays, and the DOS and Windows programs beside it are not used here. SLICNSE.TXT, in the same folder, says how the shareware may be passed on: free of charge, as a whole, and compressed if it is sent electronically.

Sound effects play, at PLAY VOLUME's level. Quake's music was on its CD, so there is none here, and there is no network play.

## The Keys

{widths: 24,28}
| Action | Key |
| Move forward and back | Up and Down |
| Turn | Left and Right |
| Sidestep | , and . or Alt with Left and Right |
| Run | Shift |
| Fire | Ctrl, or the left mouse button |
| Jump, or swim up | Space or Enter |
| Look up and down | A and Z |
| Look straight ahead | End |
| Choose a weapon | 1 to 8, or / for the next one |
| Show the scores | Tab |
| The menu | Esc |
| Help | F1 |
| Save and load | F2 and F3 |
| Quick save and quick load | F6 and F9 |
| The console | The key to the left of 1, called the backquote |
| Leave the game | F10 then Y, or Ctrl-C |

The keys can be changed under Options in the menu.

## The Mouse

Moving the mouse turns you. The left button fires, and the right button walks forward. Hold the middle button to look up and down with the mouse as well.

To keep looking with the mouse without holding a button, open the console and type:

```
+mlook
```

The sensitivity is under Options in the menu.

## The Mouse Wheel

Quake leaves the wheel unused, and turning it shows a message such as "MWHEELUP is unbound, hit F4 to set". To make it change weapons, open the console and type:

```
bind mwheelup "impulse 10"
bind mwheeldown "impulse 12"
```

## Settings

Settings and key changes are saved in id1/config.cfg when you leave the game with F10 or Ctrl-C. If the Colour Maximite 2 is restarted while the game is running, any changes since it started are lost.

Mouse look is not one of the settings saved. To have it on every time, make a text file called autoexec.cfg in A:/quake/id1, which the game runs each time it starts:

```
+mlook
bind mwheelup "impulse 10"
bind mwheeldown "impulse 12"
```

## Your Own Copy of Quake

If you have the full game, copy its PAK0.PAK and PAK1.PAK into A:/quake/id1 in place of the shareware's PAK0.PAK. This has not been tried: the program has only been tested with the shareware.

The game has 12MB of MMBasic's memory. If it runs out, ask for more on RUN's line, up to about 15:

```
RUN "quake.app", -mem 15
```
