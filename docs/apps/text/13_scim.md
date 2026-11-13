# sc-im

sc-im is Andrés Martinelli's spreadsheet. It grew from sc, the spreadsheet calculator that Unix systems have had since the 1980s, and it is worked from the keyboard in the manner of the vi editor: the arrow keys move from cell to cell, = starts a number or a formula, and : starts a command, such as :w to save. It keeps a sheet in its own text format (.sc), formulas and all, and reads and writes CSV files, which Microsoft Excel and LibreOffice open.

This is sc-im 0.8.6. It runs on a Generation 1 and a Generation 2, and needs firmware 7.00.00RC2 or later.

## Before You Start

sc-im does not work like Excel. Five things make the difference:

- **Letters are commands.** In NORMAL mode, the mode sc-im starts in, each key does something: d deletes, p pastes. To put something in a cell, start with **=** for a number or a formula, or **<** for text.
- **What you type appears on the second line of the screen**, not in the cell, until you press Enter.
- **Esc gives up what you are typing**, and the cell stays as it was. Backspace deletes what you have typed, but not the = or < at its start: only Esc takes that away.
- **While you type, the arrow keys and the mouse do not take you to another cell.** The arrow keys move along what you have typed, and a click types the name of the cell clicked. Press Enter or Esc first.
- **Rows are numbered from 0.** The first cell is A0, not A1.

## Starting It

```
CHDIR "A:/scim"
RUN "scim.app"
```

It starts with an empty sheet and a welcome message, which goes when you press a key. To open a sheet, name it after the comma:

```
RUN "scim.app", budget.sc
```

It opens its own sheets (.sc), CSV files (.csv, and .txt, which it reads as CSV), tab-separated files (.tsv and .tab) and Markdown tables (.md). If the file does not exist yet, sc-im says **New file** and starts an empty sheet, which it saves under that name.

## Leaving It

Type **:q** and press Enter. If you are typing into a cell, press Esc first.

If the sheet has changed since it was last saved, sc-im says **File not saved since last change. Add '!' to force** and stays. Then:

- **:x** saves the sheet and leaves (so does **:wq**);
- **:q!** leaves without saving.

**Ctrl+C does not leave sc-im**, and does nothing in it.

## A First Sheet

This makes a short shopping list with a total, and saves it. What you type is in bold.

1. At MMBasic's prompt, type **CHDIR "A:/scim"**, and then **RUN "scim.app", shopping.sc**. sc-im says New file. The cursor is on A0.

2. Type **<Bread** and press Enter. Bread appears in A0.

3. Press Right, to B0. Type **=1.20** and press Enter.

4. Press Down and then Left, to A1. Type **<Milk** and press Enter. Press Right, type **=0.95** and press Enter.

5. Press Down and then Left, to A2. Type **<Total** and press Enter. Press Right, type **=B0+B1** and press Enter. B2 shows 2.15.

6. Press Up twice, to B0. Type **=1.50** and press Enter. The total changes to 2.45 at once.

7. Type **:w** and press Enter. sc-im says File "shopping.sc" written. Type **:q** and press Enter to leave.

To work on it again: **RUN "scim.app", shopping.sc**.

## The Screen

The top line shows, from the left:

- the file's name (**[No Name]** until it has one), and the sheet's name in braces;
- the cell the cursor is on, such as **B4**;
- its column's format in round brackets: the width, the number of decimal places, and 0 for ordinary numbers (1 is scientific and 2 engineering). A new sheet's columns are **(10 2 0)**;
- what the cell holds, in square brackets. A formula is shown with its sheet's name before each cell: =B0+B1 shows as {"Sheet1"}!B0+{"Sheet1"}!B1. Text is shown in quotes.

**The second line is where you type**: a number, formula or text going into a cell, and a command after :. It also shows sc-im's messages, in red for an error.

The mode, such as **-- NORMAL --**, is at the top right. Below these two lines is the sheet.

Columns are lettered from A, and **rows are numbered from 0**.

Numbers you type are shown in cyan, the results of formulas in red, negative numbers in green and text in magenta. The **:color** command changes them (see Settings).

sc-im uses the video mode MMBasic is in, if it is one of these, and otherwise MODE 1:

{widths: 10,14,28}
| MODE | Resolution | Characters, with the font it picks |
| 2 | 640 x 400 | 80 x 25 (8 x 16) |
| 8 | 640 x 480 | 80 x 30 (8 x 16) |
| 1 | 800 x 600 | 100 x 37 (8 x 16) |
| 10 | 848 x 480 | 106 x 30 (8 x 16) |
| 14 | 960 x 540 | 120 x 33 (8 x 16) |
| 18 | 1024 x 600 | 128 x 37 (8 x 16) |
| 9 | 1024 x 768 | 128 x 48 (8 x 16) |
| 11 | 1280 x 720 | 160 x 45 (8 x 16) |
| 15 | 1280 x 1024 | 160 x 64 (8 x 16) |
| 16 | 1920 x 1080 (Generation 2) | 160 x 45 (12 x 24) |

It picks the font by the screen's width: 8 x 16 up to 1280 pixels and 12 x 24 above. There are four fonts: 6 x 12, 8 x 16, 12 x 24 and 16 x 32.

**Alt+=** changes to the next video mode and **Alt+Minus** to the next font, while you work, and sc-im redraws the sheet to fit. It remembers your choice in pdcurses.cfg in its folder, and starts in it next time. You can also give them on RUN's line:

```
RUN "scim.app", mode=16 font=12x24 budget.sc
```

## Modes

Like vi, sc-im has modes, and a key does different things in each. The mode is shown at the top right. Most of the time you use two: NORMAL, to move about and give commands, and INSERT, to type into a cell. **Esc takes you back to NORMAL mode.**

{widths: 14,38}
| Mode | What it is for |
| NORMAL | Moving between cells, and commands of a key or two, such as dd to delete a cell. sc-im starts in it |
| INSERT | Typing a number, formula or text into a cell, on the second line. =, <, > or the backslash starts it. Enter puts it in the cell, Esc gives it up |
| COMMAND | A command, typed on the second line after :, such as :w. Enter carries it out, Esc gives it up |
| EDIT | Changing what a cell already holds: e for a number or formula, E for text |
| VISUAL | Selecting a range of cells: v starts it |

In NORMAL mode, many commands are two or more keys, such as dd or yy. After the first key, a ? shows to the left of the mode, and sc-im waits up to three seconds for the rest. Esc gives up a command half typed.

A number before a command repeats it: 5j goes down five rows.

## Moving Around

In NORMAL mode:

{widths: 24,28}
| Keys | Where they go |
| The arrow keys, or h, j, k and l | A cell left, down, up or right |
| A click on a cell | That cell |
| Page Down and Page Up, or Ctrl+F and Ctrl+B | Half a screen down or up |
| The mouse's wheel | Half a screen up or down |
| Home, or 0 | Column A |
| End, or $ | The last column of the row with something in it |
| gg, or Ctrl+A | A0 |
| G | The last cell with something in it |
| go then a cell's name | That cell: gob12 goes to B12 after a moment, with no Enter |
| Ctrl+E and Ctrl+Y | Scroll the sheet a row down or up |
| / then a number, or text in quotes | The next cell holding it, such as /"Milk". n finds the next one and N the one before |
| gt and gT | The next and the previous sheet |

To make Page Down, Page Up and the wheel move a whole screen, type **:set half_page_scroll=0**.

## Putting Something in a Cell

Move to the cell, in NORMAL mode, and press one of these keys:

{widths: 10,42}
| Key | What it starts |
| = | A number or a formula, such as 1.25 or B1*B2 |
| < | Text, set to the left of the cell |
| > | Text, set to the right |
| the backslash | Text, centred |

Then type what goes in the cell. It appears on the second line of the screen, after the key you pressed. While you type, these keys work:

{widths: 16,36}
| Key | What it does |
| Enter | Puts it in the cell, and goes back to NORMAL mode |
| Esc | Gives it up: the cell stays as it was, and sc-im goes back to NORMAL mode |
| Backspace | Deletes the character before the cursor. It does not delete the =, <, > or backslash: Esc does that |
| Delete | Deletes the character under the cursor |
| Left and Right | Move along what you have typed |
| Home and End | Go to its start and its end |
| Up and Down | Bring back what you typed into cells before, the last first. If you have typed something, only what begins with it |
| A click on a cell | Types the cell's name: type =, click B0, type +, click B1, and the line reads =B0+B1 |
| Ctrl+K and two keys | A Greek letter or a maths sign (below) |

The arrow keys and the mouse do not take you to another cell while you type. Press Enter or Esc first.

After Enter, the cursor stays on the cell. To have it move down after each entry, as in other spreadsheets, type **:set newline_action=j** (or **=l** to move right).

What you type is kept from one time to the next, in the file sc-iminfo in sc-im's folder. After :, Up and Down bring back the commands in the same way.

Text can have accented letters, typed on the keyboard, such as é or ü, and sc-im keeps them in UTF-8.

**Ctrl+K** and then two keys gives a Greek letter or a maths sign. Press Ctrl+K, then a, then * (with no space between): α appears. Ctrl+K, then +, then - gives ±. **:help** lists them all: search it for Ctrl-K. The CMM2's fonts lack many of them, and a character the font lacks is shown as ?. The 8 x 16 font has the capital Greek letters but Ξ, the small letters α, δ, ε, κ, μ, ν, ο, π, σ, τ and φ, and ±, √, ∞, ≡, ≤, ∩ and ∪. The larger fonts have fewer, and the 6 x 12 font none of the Greek letters.

To change what a cell holds without typing it all again, press **e** (for a number or formula) or **E** (for text). What the cell holds appears on the second line, with the cursor at its end, in EDIT mode: Left and Right move along it, x deletes a character, and i or = starts typing (INSERT mode). Enter, or Esc, puts the change in the cell. To get the old contents back, press u (undo) afterwards.

These keys work on cells, in NORMAL mode:

{widths: 18,34}
| Keys | What they do |
| x, or dd | Delete the cell, keeping a copy to paste |
| yy | Copy the cell |
| p | Paste at the cursor. A formula keeps the cells it names |
| Pc | Paste, with the formula's cells moved as the copy was: B0's A0*2, pasted in B2, becomes A2*2 |
| Pv | Paste only the values |
| u | Undo. Ctrl+R redoes |
| ir and ic | Insert a row above the cursor, or a column to its left |
| or and oc | Insert a row below, or a column to the right |
| dr and dc | Delete the row or the column |
| yr and yc | Copy the row or the column. p pastes it as a new row or column |
| + and - | Add 1 to the number, or take 1 from it (5+ adds 5) |
| { and } | Set the text to the left or the right of the cell. The bar key centres it |
| f> and f< | Widen or narrow the column. f+ and f- give it more or fewer decimal places |
| aa | Fit the column to its widest cell |
| . | Do the last command again |
| Ctrl+L | Draw the screen again |

## Formulas

A formula is typed after =. It can use:

- numbers, and cells by name: A0, B12;
- + (add), - (subtract), * (multiply), / (divide), % (remainder) and ^ (power), with brackets;
- comparisons, <, <=, =, <> (not equal), >= and >, which give 1 for true and 0 for false, with & (and), | (or) and ~ (not): ~(B4>100);
- ranges of cells, from one corner to the other: B1:B3;
- functions, which start with @: @sum(B1:B3).

A $ in a cell's name fixes that part of it when the formula is pasted with Pc: $A$0 stays A0 wherever it goes.

{widths: 22,30}
| Function | What it gives |
| @sum, @avg, @count, @max, @min, @prod, @stddev | The sum, average, count and so on of a range: @avg(B1:B12) |
| @round(x, n) | x rounded to n decimal places. @floor and @ceil round down and up |
| @abs, @sqrt, @exp, @ln, @log, @pi | The usual maths |
| @sin, @cos, @tan, @asin, @acos, @atan | Trigonometry, in radians. @dtr and @rtd convert degrees and radians |
| @if(test, a, b) | a if the test is true, otherwise b: @if(B4>100, 1, 0) |
| @lookup, @hlookup, @vlookup, @index | Finding a value in a table |
| @now | The date and time now, from the CMM2's clock |
| @year, @month, @day, @hour, @minute, @second | Parts of a date: @year(@now) |

**:help** describes them all, and the functions for text.

Formulas are worked out again whenever a cell changes. A formula that cannot be worked out, such as 1/0, shows ERROR. **@** works them all out again by hand, if you have turned that off with **:set autocalc=0**.

## Selecting a Range

Press **v** and move the cursor: the cells from where you started to the cursor are selected. Then:

{widths: 14,38}
| Keys | What they do |
| y | Copy the range. Move to where it should go and press p (or Pc) |
| x | Delete the range |
| { and } | Set the range's text to the left or right. The bar key centres it |
| : | A command for the range, such as :format "#,##0.00" or :fcopy |
| Esc | Leave the range unselected |

**:fcopy** copies a formula down: select the formula's cell and the cells below it, and type **:fcopy**. **:fill A0:A9 1 1** fills A0 to A9 with 1, 2, 3 and so on (the start and the step).

## Formats and Dates

- **:format "#,##0.00"** sets the number format of the cell or the selected range: # and 0 are digits (0 is shown even when it is a leading or trailing zero), the comma separates thousands, and % shows a percentage.
- **:formatcol 12 2 0** sets the column's width, decimal places and kind of number.
- **:cellcolor "fg=RED bold=1"** colours the cell or the range. The colours are WHITE, BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA and CYAN.

A date is a number, the seconds since the start of 1970, shown with a date format. To put a date in a cell, type it as text with the backslash, then convert it:

```
\13/11/2026
:datefmt "%d/%m/%Y"
```

The cell now shows 13/11/2026 as a date, and can be used in sums. Ctrl+D also converts the text, but only month first and with two figures for the year, as 11/13/26: it takes 13/11/2026 for the 11th of the 13th month, and gives 01/11/27.

**=@now** gives the date and time from the CMM2's clock. Show it as a date with **:format "d%d %b %Y %H:%M"** (a format for dates starts with d), and widen the column with aa.

## Sheets

A file can hold several sheets, each with its own name. **:newsheet "Costs"** adds one and goes to it, **gt** and **gT** go to the next and the previous, **:renamesheet "Income"** renames this one and **:delsheet** deletes it. A formula can use a cell of another sheet: {"Costs"}!B4.

Only an .sc file keeps more than one sheet. A CSV file holds the sheet you are on.

## Files

{widths: 22,30}
| Command | What it does |
| :w | Save, under the file's name |
| :w budget | Save as budget.sc (.sc is added to a name without one) |
| :w budget.csv | Save as a CSV file |
| :w! budget.sc | Save, over a file that is already there |
| :load accounts.sc | Open another sheet in place of this one. :load! drops this one's changes |
| :e csv, :e tab, :e txt, :e mkd, :e tex | Export as CSV, tab-separated, plain text, a Markdown table or LaTeX, under the file's name with that ending: budget.sc exports to budget.csv. A name after it, such as :e csv c:/totals.csv, names the file. With a range selected, only the range. The sheet keeps its own name |
| :file | Show the file's name. :file new.sc changes it |

A name can have a drive and folders: c:/accounts/june.csv. A: is the SD card and C: a USB flash drive. A name with no drive is in the current folder.

**:w saves in the file's own format.** A sheet opened from a CSV file is saved as a CSV file, which keeps the values but not the formulas, the formats, the colours or the other sheets. To keep them, save it as an .sc file: **:w budget.sc**. Keep your sheets as .sc files, and use CSV only to take them to and from a PC.

### CSV Files from a PC

In Excel, use File, Save As, and choose **CSV UTF-8 (Comma delimited)**. Plain **CSV (Comma delimited)** works too, but it holds only the letters of Western European languages, and Excel writes ? for any others. In LibreOffice, choose Text CSV, with the character set Unicode (UTF-8). Copy the file to the SD card or a USB flash drive, and open it with **RUN "scim.app", sheet.csv**.

- Its first line goes into row 0. Plain numbers, such as 1.2 or -3, come in as numbers, with all their decimal places. Text comes in as it is, with any commas and quotes in it (He said "hi").
- **Numbers shown with a thousands separator (1,234.50), percentages (5.0%) and dates come in as text**, which sums leave out. Before saving the CSV, set those columns to a plain number format (Excel's General or Number with no 1000 separator). A date can be converted in sc-im with :datefmt (see Formats and Dates).
- A number with leading zeros, such as 00123, comes in as 123.
- Accented letters and signs such as é, £ and € come in from either kind of CSV file.
- Formulas come in as their values: a CSV file holds no formulas.

If a file has something sc-im cannot read, sc-im shows each error alone at the top of an empty screen, and waits: press Esc once for each, and the sheet appears.

### CSV Files to a PC

Type **:w sheet.csv** (or **:e csv**, which keeps the sheet's own name), and copy the file to the PC.

- **Each number is written as it is shown**: 3.14159265 in a column showing two decimal places is written as 3.14. To keep more, give the column more decimal places with f+ first.
- Numbers with a thousands separator, percentages and dates are written as they are shown (1,234,567.89, 12.5%, 13/11/2026). Excel reads them as numbers and dates again.
- Formulas are written as their values.
- The file is in UTF-8. **Opened with a double-click, Excel garbles accented letters** (Café shows as CafÃ©). Open it with Data, From Text/CSV instead, and choose 65001: Unicode (UTF-8) as the file origin. LibreOffice asks for the character set: choose UTF-8.
- Text with a comma or a quote in it is written in quotes, as CSV has it, and Excel and sc-im read it back as it was.
- A cell that shows ERROR is written as ERROR.

sc-im cannot read or write Excel's own files (.xlsx and .xls) or LibreOffice's (.ods): use CSV for them.

## The Mouse

With a USB mouse:

- in NORMAL mode, a click on a cell moves the cursor to it;
- while you type a formula or a command, a click on a cell types its name, such as B4, and the cursor stays where it was;
- the wheel scrolls the sheet up and down.

## Settings

**:set** shows the settings, and **:set name=value** changes one until sc-im ends.

To have your settings each time sc-im starts, put them in a settings file: a text file named scimrc, with no ending, in sc-im's folder (A:/scim/scimrc). There is none to begin with, and sc-im runs without one. Each line is a command as you would type it after the colon, without the colon. This one has the cursor move down after each entry, and shows the column letters on blue:

```
set newline_action=j
color "type=HEADINGS fg=WHITE bg=BLUE"
color "type=HEADINGS_ODD fg=WHITE bg=BLUE"
```

To write it with MMBasic's editor, make the file first, as EDIT adds .BAS to a new name with no ending. At MMBasic's prompt:

```
OPEN "A:/scim/scimrc" FOR OUTPUT AS #1 : CLOSE #1
EDIT "A:/scim/scimrc"
```

Type the lines, save the file, and start sc-im. To go back to sc-im's own settings, delete the file: KILL "A:/scim/scimrc".

Some settings:

{widths: 18,34}
| Setting | What it does |
| newline_action=j | Move down after each entry (l moves right, 0 stays) |
| numeric | A digit starts a number at once, without =. A number before a command then no longer repeats it |
| overlap | Text longer than its column runs on into the next, if that is empty |
| autowrap | Long text is wrapped and the row made taller (with overlap and truncate off) |
| half_page_scroll=0 | Page Down, Page Up and the wheel move a whole screen |
| ignorecase | / finds text whatever its case |
| underline_grid | Lines between the rows |
| input_bar_bottom | The line you type on at the bottom of the screen |
| history_file=0 | Do not keep what you type in sc-iminfo for next time (put it first in scimrc) |

## Help

**:help** shows sc-im's own manual. The Up and Down keys move through it, Space goes a page down, / finds text in it, and **:q** goes back to the sheet. It is written for sc-im on Linux, and some of what it describes is not here (see the next section). It is the file sc-im_help in sc-im's folder.

## What It Does Not Do

- It has no charts. **:plot** says **Gnuplot was not installed when building Sc-im**.
- It cannot run other programs. **:!** followed by a command says **Error creating pipe**, and waits for a key.
- It has no clipboard: **:ccopy** and **:cpaste** do nothing. Copy cells within sc-im with yy and p.
- It cannot read or write .xlsx, .xls or .ods files. Saving as .xlsx says **XLSX export support not compiled in**.
- It has no Lua scripts (@lua) and no external functions (@ext), and gf (open the file named in a cell) does nothing.
- It does not save a backup copy as you work. Save often with **:w**.
- It does not print. Save the sheet as CSV and print it from a PC, or export it with **:e txt** and print the text.

## If Something Goes Wrong

- **Keys seem to do nothing, or appear on the second line:** look at the mode at the top right. In INSERT or COMMAND mode, what you type goes on the second line: press Enter to finish it, or Esc to give it up.
- **A ? shows to the left of the mode:** sc-im is waiting for the rest of a command. Press Esc.
- **The screen is empty but for an error at the top, after opening a file:** sc-im found something it could not read in the file. Press Esc once for each error, and the sheet appears (see CSV Files from a PC).
- **sc-im stops with an error:** MMBasic's screen comes back with the message it gave, and **[The program wrote this while its screen was up. Press a key for MMBasic.]** below it. Anything since the last :w is lost.

This product includes software developed by Andrés G. Martinelli.
