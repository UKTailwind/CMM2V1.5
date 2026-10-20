# Appendix G – Turtle Graphics

## Turtle Graphics

MMBasic includes a very comprehensive turtle graphics implementation (the same as the PicoMite's). The supported commands, each with "TURTLE " in front of it, are listed below. A short form is shown in brackets after a command's description.

Several commands may also be written in other ways: BACK as BACKWARD, LEFT and RIGHT as TURN LEFT and TURN RIGHT, SET XY as SETXY or MOVE, SET X as SETX, SET Y as SETY, SET HEADING as SETHEADING or HEADING, PEN UP as PENUP, PEN DOWN as PENDOWN, PEN COLOUR as PEN COLOR, PENCOLOUR or PENCOLOR, PEN WIDTH as PENWIDTH, ARCLEFT as ARC LEFT, ARCRIGHT as ARC RIGHT, FILL COLOUR as FILL COLOR, FILLCOLOUR or FILLCOLOR, FILL PATTERN as FILLPATTERN, NO FILL as NOFILL, BEGIN FILL as BEGIN_FILL, END FILL as END_FILL, SHOW TURTLE as SHOWTURTLE, HIDE TURTLE as HIDETURTLE, CURSOR SIZE as CURSORSIZE and CURSOR COLOUR as CURSOR COLOR, CURSORCOLOUR or CURSORCOLOR.

{widths: 40,60}
| Command | Description |
|---|---|
| **Movement Commands** |  |
| FORWARD distance | Move forward by distance pixels (FD) |
| BACK distance | Move backward by distance pixels (BK) |
| LEFT [angle] | Turn left by angle degrees, 90 degrees if not specified (LT) |
| RIGHT [angle] | Turn right by angle degrees, 90 degrees if not specified (RT) |
| **Position Commands** |  |
| SET XY x, y | Move to absolute position (x,y) |
| SET X x | Set X coordinate, keep Y |
| SET Y y | Set Y coordinate, keep X |
| SET HEADING angle | Set absolute heading (0=up, 90=right) (SETH) |
| HOME | Return to center (MM.HRES\2, MM.VRES\2) heading 0 |
| **Pen Control Commands** |  |
| PEN UP | Lift pen (stop drawing) (PU) |
| PEN DOWN | Lower pen (start drawing) (PD) |
| PEN COLOUR colour | Set pen colour (PC) |
| PEN WIDTH width | Set pen line width (PW) |
| **Arc and Curve Commands** |  |
| ARC radius, angle | Draw arc with given radius and angle |
| ARCLEFT radius, angle | Draw arc turning left (ARCL) |
| ARCRIGHT radius, angle | Draw arc turning right (ARCR) |
| BEZIER cp1, cp1angle, cp2, cp2angle, end, endangle | Draw Bezier curve with control points |
| **Basic Shape Commands** |  |
| CIRCLE radius | Draw circle at current position |
| DOT [size] | Draw filled dot (default size=5) |
| FCIRCLE radius | Draw filled circle |
| FRECTANGLE width, height | Draw filled rectangle (FRECT) |
| ARECTANGLE width, height | Draw rectangle outline (ARECT) |
| WEDGE radius, start, end | Draw filled wedge/pie slice |
| **Fill Commands** |  |
| FILL COLOUR colour | Set fill colour and enable filling (FC) |
| FILL PATTERN pattern | Set fill pattern (0-31) (FP) |
| NO FILL | Disable filling |
| BEGIN FILL | Start recording polygon for fill (BF) |
| END FILL | End recording and fill polygon (EF) |
| **Cursor Commands** |  |
| SHOW TURTLE | Show turtle cursor (ST) |
| HIDE TURTLE | Hide turtle cursor (HT) |
| CURSOR SIZE size | Set cursor size (CS) |
| CURSOR COLOUR colour | Set cursor colour (CC) |
| STAMP | Draw a turtle at the current x,y position |
| **State Management Commands** |  |
| RESET [show] | Clear screen and reset everything, show the turtle if show = 1 |
| PUSH | Save current position and heading to stack |
| POP | Restore position and heading from stack |

The code includes the ability to fill circles, rectangles and polygons with a textured fill, using these patterns:

{widths: 10,40,10,40}
| Nbr | Pattern | Nbr | Pattern |
|---|---|---|---|
| 0 | Solid fill | 16 | Grid |
| 1 | Checkerboard | 17 | Weave pattern |
| 2 | Vertical lines | 18 | Diamond |
| 3 | Horizontal lines | 19 | Gradient diagonal |
| 4 | Diagonal cross | 20 | Gradient diagonal reverse |
| 5 | Diagonal stripes | 21 | Border/frame |
| 6 | Crosshatch | 22 | Vertical split |
| 7 | Fine diagonal | 23 | Woven |
| 8 | Dense checkerboard | 24 | Sparse dots |
| 9 | Diagonal right medium | 25 | Diagonal very fine |
| 10 | Diagonal left medium | 26 | Arrow up |
| 11 | Vertical lines medium | 27 | Dense dots |
| 12 | Horizontal lines medium | 28 | Chevron |
| 13 | Large checkerboard | 29 | Diamond hollow |
| 14 | Dotted vertical | 30 | Circle |
| 15 | Horizontal stripes tight | 31 | Circle filled |
