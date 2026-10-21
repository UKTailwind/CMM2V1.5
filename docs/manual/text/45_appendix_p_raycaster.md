# Appendix P – The Raycaster

The RAY commands draw a first-person view of a map of walls, as in the game Wolfenstein 3D. They draw patterned walls, a patterned floor and ceiling, sliding doors, and sprites standing in the world. They also check movement against the walls, cast a ray to find what lies in a given direction, and draw a map of the world from above.

RAY is the PicoMite's raycaster, so a program written for it there runs unchanged in the 4-bit colour depth: MODE 7,4 is the PicoMite's MODE 2. On the Colour Maximite 2 it works in every colour depth, and takes RGB colours as well as the PicoMite's 16.

## Quick Start

This draws a small room in MODE 7 (320 x 240) with 16-bit colour. The W and S keys move, A and D turn, and Esc ends.

```
MODE 7, 16
DIM m$(4) LENGTH 6
m$(0) = "111111"
m$(1) = "100001"
m$(2) = "102001"
m$(3) = "100001"
m$(4) = "111111"
RAY MAP 6, 5, m$()
RAY CAMERA 1.5, 1.5, 30, 66
RAY COLOUR RGB(80, 60, 40), RGB(90, 130, 210)
RAY DEFINE 2, RGB(200, 60, 40), RGB(120, 40, 30), 4
PAGE WRITE 1
DO
  k$ = INKEY$
  IF k$ = "w" THEN RAY MOVE 0.1
  IF k$ = "s" THEN RAY MOVE -0.1
  IF k$ = "a" THEN RAY TURN -5
  IF k$ = "d" THEN RAY TURN 5
  RAY RENDER
  RAY MINIMAP 2, 2, 40
  PAGE COPY 1 TO 0, B
LOOP UNTIL k$ = CHR$(27)
PAGE WRITE 0
RAY CLOSE
```

The scene is drawn on page 1 and copied to the screen at the end of a frame, so there is no tearing (see Drawing a Frame below).

## The Map

The world is a grid of cells. Cell (0, 0) is at the top left; x increases to the right (east) and y downwards (south). The camera and sprites have positions in this grid, so (2.5, 3.5) is the centre of cell (2, 3). An angle of 0 degrees points along x (east), and angles increase clockwise: 90 is south, 180 west and 270 north.

{widths: 40,60}
| Command | Description |
|---|---|
| RAY MAP w, h, map%() | The grid, 'w' cells wide and 'h' high (each 1 to 256), from an integer array of at least w x h elements, row after row: element y * w + x is cell (x, y). 0 is an empty cell and 1 to 31 a wall type; a value below 0 is taken as 0 and one above 31 as 31. |
| RAY MAP w, h, map$() | The same from a string array of at least 'h' elements, each a row of at least 'w' characters: '0' to '9' are 0 to 9, and 'A' to 'Z' (or 'a' to 'z') 10 to 35, of which those above 31 are taken as 31. This takes a byte a cell, where the integer form takes eight. |
| RAY CELL x, y, value | Changes cell (x, y): 'value' is 0 to 31, or a one-character string as in the map. For doors, switches and walls that can be destroyed. |

## Walls, Floor and Ceiling

Each wall type has two colours and a pattern. The pattern is one of the 32 fill patterns of Turtle graphics (Appendix G), an 8 x 8 grid of bits: a 1 is drawn in the foreground colour and a 0 in the background colour. It is repeated twice across each cell and twice up the height of a wall.

{widths: 40,60}
| Command | Description |
|---|---|
| RAY DEFINE type, fg, bg, pattern [, door] | Wall type 'type' (1 to 31, or a character '1' to '9' or 'A' to 'V') is drawn with foreground colour 'fg', background colour 'bg' and pattern 'pattern' (0 to 31). 'door' is 1 if the type is a sliding door (see Doors), 0 (the default) if it is a wall. |
| RAY COLOUR floor, ceiling [, floor_bg, ceiling_bg, floor_pat, ceiling_pat] | The floor's and ceiling's colours: one each for a solid floor and ceiling, or a foreground and background colour each with a pattern (0 to 31). RAY COLOR is the same. |

A colour is one of these:
- 1 to 15: the PicoMite's colour of that number (the table below), solid in every colour depth.
- 0: BLACK. It is drawn as every drawing command draws BLACK, so in the 12-bit colour depth it is transparent. RGB(0, 0, 0) is a solid black there.
- Anything else: a colour as RGB() gives it. In the 12-bit colour depth its transparency is kept, so RGB(r, g, b, t) can be partly or fully transparent. Write a fully transparent colour as BLACK: RGB(0, 0, b, 0) with 'b' from 1 to 15 is read as one of the PicoMite's colours.

In the 4-bit colour depth an RGB colour is drawn as the nearest of the 16, as every drawing command draws it.

{widths: 10,20,20,10,20,20}
| Nbr | Colour | RGB | Nbr | Colour | RGB |
|---|---|---|---|---|---|
| 0 | BLACK | 000000 | 8 | RED | FF0000 |
| 1 | BLUE | 0000FF | 9 | MAGENTA | FF00FF |
| 2 | MYRTLE | 004000 | 10 | RUST | FF4000 |
| 3 | COBALT | 0040FF | 11 | FUCHSIA | FF40FF |
| 4 | MIDGREEN | 008000 | 12 | BROWN | FF8000 |
| 5 | CERULEAN | 0080FF | 13 | LILAC | FF80FF |
| 6 | GREEN | 00FF00 | 14 | YELLOW | FFFF00 |
| 7 | CYAN | 00FFFF | 15 | WHITE | FFFFFF |

A wall seen from the north or south (along y) is drawn darker than one seen from the east or west, which gives a sense of depth. One of the PicoMite's colours is darkened as the PicoMite does it, by taking its green down a level (GREEN 6 becomes MIDGREEN 4, and YELLOW 14 becomes BROWN 12). An RGB colour has its red, green and blue each taken down to three quarters (in the 4-bit colour depth it is first made the nearest of the 16).

Until RAY DEFINE and RAY COLOUR change them, wall types 1 to 15 are GREEN on MIDGREEN, types 16 to 31 YELLOW on BROWN and doors, each type's pattern is its number less one, and the floor is BLACK and the ceiling CYAN, both solid.

## The Camera and Movement

{widths: 40,60}
| Command | Description |
|---|---|
| RAY CAMERA x, y, angle [, fov] | Places the viewer at (x, y), looking at 'angle' degrees, with a field of view of 'fov' degrees (10 to 170, 60 if it has not been given). |
| RAY MOVE speed [, strafe] | Moves the viewer 'speed' cells forward (backward if it is negative) and 'strafe' cells to the right (left if negative). The viewer is a box 0.5 cells wide that cannot enter a wall: if the move is blocked, the viewer slides along the wall (the move along x alone, then along y alone), and stays where it is if both are blocked. |
| RAY TURN degrees | Turns the viewer 'degrees' to the right (left if negative). Its angle is kept from 0 to 360. |
| RAY CAST angle | Casts a single ray from the viewer at 'angle' degrees. RAY(CASTDIST), RAY(CASTWALL), RAY(CASTSIDE), RAY(CASTX) and RAY(CASTY) then give what it hit: for a use key, shooting, or what the player is looking at. |

## Doors

A wall type whose RAY DEFINE has 'door' set is a sliding door. It is drawn as a wall until it is opened with RAY DOOR.

{widths: 40,60}
| Command | Description |
|---|---|
| RAY DOOR x, y, offset | The door in cell (x, y) is open by 'offset', from 0 (closed) to 1 (fully open). Rays pass through the open part, so what is behind it shows. It blocks movement until it is fully open. Up to 8 doors can be open at once. |
| RAY DOOR CLOSE x, y | Forgets the door in cell (x, y), which is a closed door again. |
| RAY DOOR CLEAR | Forgets every door. |

To open a door smoothly, increase its offset a little each frame:

```
FOR o = 0.1 TO 1 STEP 0.1
  RAY DOOR dx, dy, o
  RAY RENDER
  PAGE COPY 1 TO 0, B
NEXT o
```

## Sprites

A sprite in the raycaster stands in the world. It always faces the viewer, is scaled by its distance and is hidden behind nearer walls. Its image is a sprite's (see Appendix F): one loaded by SPRITE LOAD, LOADARRAY, LOADPNG or LOADBMP, or read from the screen by SPRITE READ. It is not shown with SPRITE SHOW. Its pixels are drawn as SPRITE SHOW draws them, in full colour in every colour depth but the 4-bit one. A pixel that SPRITE SHOW would leave out is not drawn: a black one, unless SPRITE SET TRANSPARENT has chosen another colour.

{widths: 40,60}
| Command | Description |
|---|---|
| RAY SPRITE id, n, x, y | Raycaster sprite 'id' (0 to 31) is sprite 'n' (1 to 64) standing at (x, y). |
| RAY SPRITE REMOVE id | Removes raycaster sprite 'id'. |
| RAY SPRITE CLEAR | Removes every raycaster sprite. |

## Drawing a Frame

{widths: 40,60}
| Command | Description |
|---|---|
| RAY RENDER | Draws the view: the floor and ceiling, then the walls, then the sprites, furthest first. It covers the whole of the page being written (PAGE WRITE, or FRAMEBUFFER WRITE in the 4-bit colour depth), at that page's size. |
| RAY MINIMAP x, y, size | Draws the map from above at (x, y), its longer side 'size' pixels (4 to 256): each wall in its foreground colour, an empty cell BLACK, a door BROWN when closed, YELLOW while it opens and BLACK when open, each sprite a MAGENTA dot and the viewer a WHITE dot with a line for its direction, inside a MYRTLE border. Draw it after RAY RENDER. |
| RAY CLOSE | Frees the map and everything else the raycaster holds. RUN, NEW and CHAIN do this too. A change of MODE keeps it. |

RAY RENDER draws every pixel of the page, so draw a frame off the screen and copy it to the screen at the end of the frame, as for any animation:
- In the 8, 16 and 32-bit colour depths, PAGE WRITE 1 (or another page), then RAY RENDER, RAY MINIMAP and anything else on top of them, then PAGE COPY 1 TO 0, B.
- In the 4-bit colour depth, the PicoMite's way: FRAMEBUFFER CREATE and FRAMEBUFFER WRITE F, then RAY RENDER and the rest, then FRAMEBUFFER COPY F, N, B. FRAMEBUFFER LAYER gives a layer on top for a score or a map.
- In the 12-bit colour depth, the view can be drawn on page 1 with a transparent ceiling (BLACK, or RGB(r, g, b, t) with 't' below 15), so that a picture of the sky on page 0 shows through it. Or draw it on page 0, with a score on page 1 on top. Page 2 in MODEs 3, 5, 6, 7, 12, 13 and 17 holds each line twice, so draw a frame on page 3 or above and copy it to page 0.

Drawing straight onto page 0 works too, but the screen shows the frame while it is being drawn. RAY RENDER and RAY MINIMAP take care of the mouse pointer. OPTION Y AXIS INVERT does not change the view.

These are the times of RAY RENDER on the Generation 2, in milliseconds, with the view drawn on a page that is not shown:

{widths: 28,12,12,12,12,12}
| MODE | 4-bit | 8-bit | 12-bit | 16-bit | 32-bit |
|---|---|---|---|---|---|
| 7 (320 x 240) | 4 | 4 | 5 | 4 | 7 |
| 8 (640 x 480) | 21 | 17 | 33 | 25 | 40 |
| 1 (800 x 600) | 42 | 27 | 69 | 41 | 81 |

PAGE COPY to the screen adds about 1 ms in MODE 7, 10 ms in MODE 8 and 17 ms in MODE 1 at 16 bits.

## Functions

{widths: 40,60}
| Function | Returns |
|---|---|
| RAY(MAPW), RAY(MAPH) | The map's width and height (0 before RAY MAP). |
| RAY(CAMX), RAY(CAMY), RAY(CAMA) | The viewer's position and angle. |
| RAY(CELL x, y) | Cell (x, y) of the map. |
| RAY(DIST col) | The distance to the wall in column 'col' of the last RAY RENDER, measured along the direction of view (so a flat wall straight ahead is the same distance in every column). |
| RAY(WALL col) | The wall type in column 'col' of the last RAY RENDER. |
| RAY(CASTDIST) | The distance to the wall the last RAY CAST hit. |
| RAY(CASTWALL) | Its wall type. |
| RAY(CASTSIDE) | 0 if it hit the wall's west or east side, 1 if its north or south side. |
| RAY(CASTX), RAY(CASTY) | The cell it hit. |
| RAY(DOOR x, y) | The offset of the door in cell (x, y), or -1 if RAY DOOR has not set one there. |
| RAY(DEFINE type, property) | Wall type 'type' (a number or a character): 'property' 0 its foreground colour, 1 its background colour, 2 its pattern, 3 whether it is a door. A colour is returned as it was given. |
| RAY(SPRITES) | How many raycaster sprites there are. |
| RAY(SPRITEX id), RAY(SPRITEY id) | The position of raycaster sprite 'id' (0 if there is none). |

## Limits

- The map is up to 256 x 256 cells, with 31 wall types.
- Up to 32 sprites and 8 open doors at once.
- The floor and ceiling are flat, and every wall is one cell high.
- RAY is a keyword, so it cannot be the name of a variable or array.
