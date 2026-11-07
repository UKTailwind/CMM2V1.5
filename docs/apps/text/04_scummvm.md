# ScummVM

ScummVM plays classic adventure games from their original data files. This version comes from FRANK Quest, rh1tech's port of ScummVM to the RP2350. It has the AGI, SCI, SCUMM (versions 1 to 7), Gobliiins and Kyrandia engines.

## Starting It

```
CHDIR "A:/quest"
RUN "scummvm.app"
```

It lists the games it finds in folders of A:/quest. Choose one with the arrow keys and press Enter.

To start a game at once, give its folder's name:

```
CHDIR "A:/quest"
RUN "scummvm.app", dott
```

On a Generation 1 the screen changes to MODE 7 as it starts, as ScummVM borrows the display memory.

## The Keys

{widths: 16,36}
| Keys | What they do |
| The mouse | Most of these games are played with it |
| F5 | Save and restore (see below) |
| Esc | Skips a cutscene, in most games |
| Ctrl+Alt+Del | Back to the list of games |
| Ctrl-C | Leave ScummVM |

## Saving Your Game

Saved games are kept in A:/quest/saves. How you save depends on who made the game.

### LucasArts Games

This means Monkey Island, Indiana Jones, Day of the Tentacle, Sam & Max, Loom and Full Throttle.

Press **F5** to open the save menu. It lists nine slots, with the date each one was saved.

{widths: 16,36}
| Keys | What they do |
| Up, Down or 1 to 9 | Choose a slot |
| S | Save in that slot. If the slot is in use it asks first |
| R | Restore that slot |
| Esc or F5 | Close the menu |

A message tells you when the game has been saved.

There are also shortcuts, which work without the menu and use the same slots:

{widths: 16,36}
| Keys | What they do |
| Alt-1 to Alt-9 | Save in that slot at once, replacing what was there |
| Ctrl-1 to Ctrl-9 | Restore that slot at once |

You cannot save during a cutscene, or before the game has properly started. The menu says so if that is why.

### Sierra Games

The AGI and SCI games use their own save and restore screens, as they did on a PC. Press **F5** to save and **F7** to restore, or use the game's menu bar, which Esc opens in most of them. You can choose a slot and give each save a name.

### Gobliiins and Kyrandia

These have save and restore in their own menus, on the game's screen.

## The Games That Come With It

{widths: 12,40}
| Folder | Game |
| freddy | Freddy Pharkas, Frontier Pharmacist: the interactive demo (Sierra, 1993) |
| agi | DG: The Adventure Game (Robin Gravel, 2001; public domain) |
| dgc64 | Another DG Game: I Want My C64 Back (Robin Gravel, 2002; public domain) |

## Adding Games You Own

Games you own go in folders of A:/quest. The folder's name tells ScummVM which game it is, so it must be one of those below, in upper or lower case. A folder with any other name is not listed.

Copy the game's data files only, not its .EXE and .DRV files.

### LucasArts (SCUMM)

{widths: 14,38}
| Folder | Game |
| mi1 | The Secret of Monkey Island, VGA floppy (needs 000.LFL) |
| mi1ega | The Secret of Monkey Island, EGA floppy (needs 000.LFL) |
| monkey, mi1cd | The Secret of Monkey Island, CD (needs monkey.000) |
| monkey2, mi2 | Monkey Island 2: LeChuck's Revenge (needs monkey2.000) |
| loom | Loom, VGA (needs 000.LFL) |
| indy4, atlantis | Indiana Jones and the Fate of Atlantis (needs atlantis.000) |
| dott, tentacle | Day of the Tentacle (needs tentacle.000) |
| samnmax, sam | Sam & Max Hit the Road |
| ft, fulltp | Full Throttle (needs ft.la0) |

### Sierra (SCI)

{widths: 24,28}
| Folder | Game |
| kq1 kq4 kq5 kq6 | King's Quest I, IV, V, VI |
| sq1 sq3 sq4 sq5 | Space Quest I, III, IV, V |
| lsl1 lsl2 lsl3 lsl5 lsl6 | Leisure Suit Larry 1, 2, 3, 5, 6 |
| pq1 pq2 pq3 | Police Quest I, II, III |
| qfg1 qfg1vga qfg2 qfg3 | Quest for Glory I (EGA, VGA), II, III |
| laurabow laurabow2 | Laura Bow 1 and 2 |
| ecoquest ecoquest2 | EcoQuest 1 and 2 |
| iceman longbow camelot | Codename: ICEMAN, Conquests of the Longbow, of Camelot |
| freddy jones pepper slater | Freddy Pharkas, Jones in the Fast Lane, Pepper's Adventures in Time, Slater & Charlie |
| castlebrain islandbrain mothergoose | Castle and Island of Dr. Brain, Mixed-Up Mother Goose |

### Others

{widths: 24,28}
| Folder | Game |
| kyra1, kyrandia | The Legend of Kyrandia (needs KYRA.DAT: copy it from A:/quest into the game's folder) |
| kyra2, handoffate | The Legend of Kyrandia 2: The Hand of Fate (needs KYRA.DAT too) |
| gob1 gob2 gob3 | Gobliiins, Gobliins 2, Goblins 3 |
| agi | One AGI game (version 2) of any kind, such as a fan game |

Other games, and demos other than those of Gobliiins and Kyrandia, are not supported. Nor are LucasArts' last two: The Dig (SCUMM version 7, not built into FRANK Quest) and The Curse of Monkey Island (SCUMM version 8, at 640 x 480).
