# Doom and Heretic

Both are the original games, running at full speed on either generation.

## Doom

```
CHDIR "A:/Doom"
RUN "doom.app"
```

This plays Freedoom Phase 1, a free replacement for Doom's game data. The music is played on the firmware's FM synthesiser.

Always start it from its folder, so that its settings and saved games are kept there.

### Your Own Copy of Doom

If you have Doom's own game data, there are two ways to use it:

- Copy it into A:/Doom as doom.wad or doom1.wad, and remove freedoom1.wad.
- Name it on RUN's line:

```
RUN "doom.app", -iwad x.wad
```

## Heretic

```
CHDIR "A:/heretic"
RUN "heretic.app"
```

The folder holds the whole of Heretic's shareware version 1.2, the first episode, unchanged. HERETIC1.WAD is what the game plays, and the DOS programs beside it are not used here.

If you have the full game, put its heretic.wad in the folder and it is used in place of the shareware.

LICENSE.DOC, in the same folder, says how the shareware may be passed on: free of charge, and compressed if it is sent electronically.

## The Keys

{widths: 22,15,15}
| Action | Doom | Heretic |
| Move and turn | The arrows | The arrows |
| Fire | Ctrl | Ctrl |
| Open, use | Space | Space |
| Run | Shift | Shift |
| Strafe | Alt with the arrows, or , and . | Alt with the arrows, or , and . |
| The menu | Esc | Esc |
| Choose an artifact | | [ and ] |
| Use an artifact | | Enter |
| Leave the game | Ctrl-C | Ctrl-C |

Saving and loading are in the game's own menu, under Esc.
