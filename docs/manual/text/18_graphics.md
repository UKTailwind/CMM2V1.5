# Graphics Functions

These are the commands and functions that you can use within MMBasic to draw images on the VGA monitor (none of these apply to the serial console). There are also a number of advanced commands designed for programmers writing games (such as MODE, PAGE, BLIT, SPRITE and TILEMAP: see the chapters Video Modes and Pages and Game Playing Features) however this section will focus on the standard commands used by most programmers.

## Screen Coordinates

All screen coordinates and measurements on the screen are done in terms of pixels with the X coordinate being the horizontal position and Y the vertical position. The top left corner of the screen has the coordinates X = 0 and Y = 0 and the values increase as you move down and to the right of the screen.

By default on startup the VGA output will be set to 800x600 pixels with each pixel supporting 256 different colours. At this resolution the bottom right pixel will be at X = 799 and Y = 599.

## Read Only Variables

There are six read only variables which provide useful information about the VGA video output:

- MM.HRES - Returns the width of the display (the X axis) in pixels.
- MM.VRES - Returns the height of the display (the Y axis) in pixels.
- MM.INFO(FONTHEIGHT) - Returns the height of the current font (in pixels). All characters in a font have the same height.
- MM.INFO(FONTWIDTH) - Returns the width of a character in the current font (in pixels). All characters in a font have the same width.
- MM.INFO(HPOS) - Returns the X coordinate of the text cursor (ie, the horizontal location (in pixels) of where the next character will be printed on the VGA monitor).
- MM.INFO(VPOS) - Returns the Y coordinate of the text cursor (ie, the vertical location (in pixels) of where the next character will be printed on the VGA monitor).

## Colours

Colour is specified as a true colour 24 bit number where the top eight bits represent the intensity of the red colour, the middle eight bits the green intensity and the bottom eight bits the blue. For example the colour red is &HFF0000 and yellow is &HFFFF00.

An easier way to generate a colour value is to use the RGB() function which has the form:

```
RGB(red, green, blue)
```

A value of zero for a colour represents black and 255 represents full intensity.

The RGB() function also supports a shortcut where you can specify common colours by naming them. For example, RGB(red) or RGB(cyan). The colours that can be named using the shortcut form are white, black, blue, green, cyan, red, magenta, yellow, brown, grey (or gray), lightgrey (or lightgray), orange, pink, gold, salmon, beige, and the PicoMite's lilac, fuchsia, rust, cerulean, midgreen, cobalt and myrtle.

In addition there is a special colour NOTBLACK. For any mode this will be the darkest colour that can be displayed that will not act as transparent when manipulated by graphics commands that support transparency.

Because the Colour Maximite 2 uses double precision floating point it can store the 24 bit number representing colour (i.e. returned by the RGB() function) in either a floating point variable or an integer variable.

MMBasic will automatically convert colours to the format required for the current colour depth set by the MODE command. So, for example, at startup the Colour Maximite 2's VGA output defaults to 8-bit colour and in this case the 16777215 colours that can be represented by a 24-bit colour specification will be translated as best as possible to the 256 colours supported by the 8-bit colour mode. The 32-bit colour depth (on the Generation 2) displays the 24-bit colours without loss of information.

The default colour for commands that require a colour parameter can be set with the COLOUR command. This is handy if your program uses a consistent colour scheme, you can then set the defaults and use the short version of the drawing commands throughout your program (the USA spelling COLOR is also accepted). The COLOUR command takes the format:

```
COLOUR foreground-colour, background-colour
```

## Fonts

There are seven built in fonts. These are:

{widths: 12,16,26,46}
| Font Number | Size (width x height) | Character Set | Description |
|---|---|---|---|
| 1 | 8 x 12 | All 95 ASCII characters plus 7F to FF (hex) | Standard font (default on startup). Default font for the editor. |
| 2 | 12 x 20 | All 95 ASCII characters | Medium sized font |
| 3 | 16 x 24 | All 95 ASCII characters | A larger font useful for the 800 x 600 display mode |
| 4 | 10 x 16 | All 95 ASCII characters plus 7F to FF (hex) | A useful font for improved clarity in high resolution modes |
| 5 | 24 x 32 | All 95 ASCII characters | Large font, very clear |
| 6 | 32 x 50 | 0 to 9 plus some symbols | Numbers plus decimal point, positive, negative, equals, degree and colon symbols. Very clear. |
| 7 | 6 x 8 | All 95 ASCII characters | A small font useful when low resolutions are used. |

In all fonts (including font #6) the back quote character (60 hex or 96 decimal) has been replaced with the degree symbol (º).

Font #1 (the default font) and font #4 have an extended character set covering all characters from CHR$(32) to CHR$(255) or 20 to FF (hex) as illustrated below.

![Font 1](pdf:39:108){width=7}

## Embedded Fonts

If required, additional fonts can be embedded in a BASIC program. These fonts work exactly same as the built in fonts (ie, selected using the FONT command or specified in the TEXT command). The format of an embedded font is:

```
DefineFont #Nbr
    hex [[ hex[…]
    hex [[ hex[…]
END DefineFont
```

It must start with the keyword "DefineFont" followed by the font number (which may be preceded by an optional # character). Any font number in the range of 2 to 5 and 8 to 16 can be specified and if it is the same as a built in font it will replace that font. The body of the font is a sequence of 8-digit hex words with each word separated by one or more spaces or a new line. The font definition is terminated by an "End DefineFont" keyword. These can be placed anywhere in a program and MMBasic will skip over it. This format is the same as that used by the Micromite and the PicoMite.

Additional fonts and information can be found in the Embedded Fonts folder in the Colour Maximite 2 firmware download. These fonts cover a wide range of character sets including a symbol font (Dingbats) which is handy for creating on screen icons, etc.

In addition to using embedded fonts a program can dynamically load one font from the SD card using the LOAD FONT command. A program can load many fonts using this method during the course of its execution but each new font will overwrite the previously loaded font.

The format of fonts loaded using LOAD FONT have a similar format as the embedded fonts described above except that no comments or blank lines are allowed, the font number must always be #8, the first word must be on a line on its own and the following lines (except the last) must have exactly eight words per line.

As an example, the following is a tiny (6x4 pixel) font that is useful in the 320x200 display mode. This can be either loaded using LOAD FONT or embedded in the BASIC program:

```
DefineFont #8
60200604
44000000 00A04040 A0AEAE00 82406C6C EACC2048 00004460 84204424 E4A48044
00E404A0 00800400 040000E0 00480240 4CE0AAEA 48C24044 C062C2E0 E820E2AA
EA68E0E2 8048E2E0 EAE0EAEA 0404C0E2 80040400 0E208424 2484000E 4040E280
4A60E84A CACAA0EA 608868C0 E8C0AACA E8E8E0E8 60EA6880 E4A0EAAA 2A22E044
A0CAAA40 AEE08888 EEAEA0EA 40AA4AA0 4A80C8CA ECCA60AE C04268A0 AA4044E4
A4AA60AA A0EEAA40 AAA04AAA 48E24044 E088E8E0 E2004208 004AE022 F0000000
0C000084 AA8CE06A 608806C0 0660AA26 E42460AC 24AE0640 40A0CA88 22204044
A0CC8AA4 0EE044C4 AA0CA0EE 40AA04A0 06C8AA0C 880662AA C0C60680 0A60444E
AE0A60AA E0AE0A40 0AA0440A 6C0E24A6 608464E0 C4400444 006CC024 E0EEEE00
End DefineFont
```

You can convert the original Colour Maximite's font files to this format using the program FontTweak, which is included in the Colour Maximite 2 download zip.

## Drawing Commands

The drawing commands have optional parameters. You can completely leave these off the end of a command or you can use two commas in sequence to indicate a missing parameter. For example, the fifth parameter of the LINE command is optional so you can use this format:

```
LINE 0, 0, 100, 100, , rgb(red)
```

Optional parameters are indicated below by italics, for example: *font*.

In the following commands C is the drawing colour and defaults to the current foreground colour. FILL is the fill colour which defaults to -1 which indicates that no fill is to be used.

The drawing commands are:

- CLS *C* - Clears the screen to the colour C. If C is not specified the current default background colour will be used.
- PIXEL X, Y, *C* - Illuminates a pixel. If C is not specified the current default foreground colour will be used.
- LINE X1, Y1, X2, Y2, *LW*, *C* - Draws a line starting at X1 and Y1 and ending at X2 and Y2. LW is the line's width and is only valid for horizontal or vertical lines. It defaults to 1 if not specified or is changed to 1 if the line is a diagonal. LINE AA draws an anti-aliased line, and LINE GRAPH and LINE PLOT draw a series of lines from arrays (see the LINE command).
- BOX X, Y, W, H, *LW*, *C*, *FILL* - Draws a box starting at X and Y which is W pixels wide and H pixels high. LW is the width of the sides of the box and can be zero. It defaults to 1.
- RBOX X, Y, W, H, *R*, *C*, *FILL* - Draws a box with rounded corners starting at X and Y which is W pixels wide and H pixels high. R is the radius of the corners of the box. It defaults to 10.
- TRIANGLE X1, Y1, X2, Y2, X3, Y3, *C*, *FILL* - Draws a triangle with the corners at X1, Y1 and X2, Y2 and X3, Y3. C is the colour of the triangle and FILL is the fill colour. FILL can be omitted or be -1 for no fill.
- CIRCLE X, Y, R, *LW*, *A*, *C*, *FILL* - Draws a circle with X and Y as the centre and a radius R. LW is the width of the line used for the circumference and can be zero (defaults to 1). A is the aspect ratio which is a floating point number and defaults to 1. For example, an aspect of 0.5 will draw an oval where the width is half the height.
- ARC X, Y, R1, *R2*, A1, A2, *C* - Draws an arc with the centre at X and Y. R1 and R2 are the inner and outer radius defining the thickness of the arc (if R2 is omitted the arc will be one pixel thick), A1 and A2 are the start and end angles in degrees and C is the colour.
- POLYGON N, XARRAY%(), YARRAY%(), *C*, *FILL* - Draws an outline or filled polygon defined by the x, y coordinate pairs in xarray%() and yarray%(). 'n' is the number of points to use in drawing the polygon. If the last xy-coordinate pair is not the same as the first the firmware will automatically create an additional xy-coordinate pair to complete the polygon.
- BEZIER XARRAY%(), YARRAY%(), *N*, *C* - Draws a Bezier curve through the control points held in the arrays (the first N elements, or all of them).
- FILL X, Y, C, *BORDER* - Flood fills the area around X, Y with the colour C, up to the colour BORDER or, without it, over the area of the colour at X, Y.
- TEXT X, Y, STRING, *ALIGNMENT*, *FONT*, *SCALE*, *C*, *BC* - Displays a string starting at X and Y. ALIGNMENT is 0, 1 or 2 characters (a string expression or variable is also allowed) where the first letter is the horizontal alignment around X and can be L, C or R for LEFT, CENTER or RIGHT aligned text and the second letter is the vertical alignment around Y and can be T, M or B for TOP, MIDDLE or BOTTOM aligned text. The default alignment is left/top. FONT and SCALE are optional and default to that set by the FONT command. C is the drawing colour and BC is the background colour. They are optional and default to that set by the COLOUR command.

Most graphics commands allow the use of arrays as parameters so that you can draw multiple graphic objects with the one command. In this case the array is passed as the array name followed by empty brackets (eg arr()). Drawing multiple graphic elements this way is much faster than drawing them one by one using separate commands.

For example, the PIXEL command allows arrays to be specified for the x and y coordinates (in this case both must be arrays). The firmware will then plot the number of pixels as determined by the dimensions of the smallest array. For the PIXEL command 'c' can also be an array or a single variable/constant. This is demonstrated with the following example which will draw three pixels in different colours:

```
DIM xx(2) = (10, 20, 30)
DIM yy(2) = (100, 150, 200)
DIM cc(2) = (RGB(red), RGB(green), RGB(blue))
PIXEL xx(), yy(), cc()
```

## Example of Basic Graphics

As an example, the following program will draw a simple digital clock on the VGA monitor.

```
CLS
CONST DBlue = RGB(0, 0, 128)              ' A dark blue colour
COLOUR RGB(GREEN), RGB(BLACK)             ' Set the default colours
FONT 6                                    ' Set the default font
BOX 0, 0, MM.HRes-1, MM.VRes/2, 3, RGB(RED), DBlue
DO
  TEXT MM.HRes/2, MM.VRes/4, TIME$, "CM", 6, 1, RGB(CYAN), DBlue
  TEXT MM.HRes/2, MM.VRes*3/4, DATE$, "CM"
LOOP
```

The program starts by defining a constant with a value corresponding to a dark blue colour and then sets the defaults for the colours and the font. It then draws a box with red walls and a dark blue interior. Following this the program enters a continuous loop where it performs two functions:

- Displays the current time inside the previously drawn box. The string is drawn centred both horizontally and vertically in the middle of the box. Note that the TEXT command overrides both the default font and colours to set its own parameters.
- Draws the date centred in the lower half of the screen. In this case the TEXT command uses the default font and colours previously set.

The screenshot below shows the result.

![Clock](pdf:41:114){width=6}

## Rotated Text

As described above the alignment of the text in the TEXT command can be specified by using one or two characters. In addition a third character can be used to indicate the rotation of the text. This character can be one of:

>> N	for normal orientation
>> V	for vertical text with each character under the previous running from top to bottom.
>> I	the text will be inverted (ie, upside down)
>> U	the text will be rotated counter clockwise by 90º
>> D	the text will be rotated clockwise by 90º

As an example, the following will display the words "Vertical Text" vertically down the left hand margin of the monitor and centred vertically:

```
TEXT 0, 250, "Vertical Text", "LMV", 5
```

Positioning is relative to the top left corner of the character when viewed normally so inverted 100,100 will have the top left pixel of the first character at 100,100 and the text will then be above y=101 and to the left of x=101. Similarly "R" in the alignment string is viewed from the perspective of the character in whatever orientation it is in.

## Transparent Text

The TEXT command will allow the use of -1 for the background colour. This means that the text is drawn over the background with the background image showing through the gaps in the letters.

## Displaying Images

Using the LOAD command you can load an image from the SD card (or a USB flash drive) and display it on the VGA monitor. Supported formats are BMP, GIF, JPG and PNG and the image can be positioned anywhere on the screen. There are some limitations on the format of the images and these are detailed in the commands later in this manual (see also Load and Save Images in the chapter Program and Data Storage). The most flexible is the LOAD BMP command which supports all types of the BMP format including black and white and true colour 24-bit images. The image can be positioned anywhere on the screen and be of any size (pixels that end up being positioned off the screen will be ignored).

## Advanced Graphics Tutorial

The graphics subsystem in the Colour Maximite 2 has many advanced features such as multiple video pages, layered images, sprites and sophisticated methods of manipulating images. Peter Mather, who developed the graphics subsystem, has written a tutorial for games developers and advanced users. This is presented as a series of posts on the Back Shed Forum and is recommended reading for users who need to get into the details: https://www.thebackshed.com/forum/ViewTopic.php?FID=16&TID=12125.

## 3D Engine

The 3D Engine includes commands for manipulating 3D images including setting the camera, creating, hiding, rotating, etc (see the DRAW3D command). The document The 3D engine.pdf in the Colour Maximite 2 firmware download has a full description of these commands and how to use them. This thread on the Back Shed Forum gave the initial explanation of the 3D Engine and may have later updates or examples: https://www.thebackshed.com/forum/ViewTopic.php?TID=13139&P=1
