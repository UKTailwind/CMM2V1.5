# WordGrinder

WordGrinder is David Given's word processor for writing. It keeps to the words and their structure: paragraphs, headings, lists, quotations, and bold, italic and underlined text. How the page looks is left to whatever you export it to, so there are no fonts, margins or page breaks to set. Its documents can be exported as ODT files, which Microsoft Word and LibreOffice open, and as HTML, Markdown, LaTeX, Troff or plain text.

This is WordGrinder 0.8. On a Generation 2 it checks your spelling against a whole English dictionary. It needs firmware 7.00.00RC2 or later.

## Starting It

```
CHDIR "A:/wordgrinder"
RUN "wordgrinder.app"
```

It starts with a new, empty document. To open a document, name it after the comma:

```
RUN "wordgrinder.app", letter.wg
```

## Leaving It

Press **Ctrl+Q**, or press Esc and choose File, then Exit. If anything has not been saved, it asks first: Y leaves without saving, N or Ctrl+C goes back to the document.

**Ctrl+C does not leave WordGrinder.** It copies the selected text, as in other word processors.

## The Screen

WordGrinder uses the video mode MMBasic is in, if it is one of these, and otherwise MODE 1:

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

**Alt+=** changes to the next video mode and **Alt+Minus** to the next font, while you work. WordGrinder remembers your choice in pdcurses.cfg in its folder, and starts in it next time. You can also give them on RUN's line:

```
RUN "wordgrinder.app", mode=16 font=12x24 letter.wg
```

The first time it starts, WordGrinder shows the text in a column 80 characters wide in the middle of the screen, with each paragraph's first line indented and no blank line between paragraphs. To change this, press Esc and choose File, Global settings, Change look and feel.

## The Menus and Keys

**Esc** opens the menu. Choose with the arrow keys and Enter, or press an item's letter. **Alt** with a letter opens that menu at once: Alt+F is the File menu, Alt+E the Edit menu. Each menu shows the keys that do the same thing.

{widths: 20,32}
| Keys | What they do |
| Ctrl+S | Save |
| Ctrl+Q | Leave WordGrinder |
| Ctrl+X, Ctrl+C, Ctrl+V | Cut, copy and paste |
| Ctrl+Z, Ctrl+Y | Undo and redo |
| Ctrl+F | Find and replace. Ctrl+K finds the next, Ctrl+R replaces and finds the next |
| Ctrl+G | Go to a heading, from a list of the document's headings |
| Ctrl+B, Ctrl+I, Ctrl+U | Bold, italic and underline, for the selected text or for what you type next |
| Ctrl+O | Plain text, with no bold, italic or underline |
| Ctrl+P | The paragraph's style: P (plain), H1 to H4 (headings), Q (quotation), LB and LN (bulleted and numbered lists), L (a list item with no mark), V (verse), PRE (preformatted) and RAW (passed to the export as it is) |
| Shift with the arrows, Home, End, Page Up and Page Down | Select text |
| Ctrl with Left and Right | Move by a word. Ctrl with Up and Down moves by a paragraph, and Ctrl with Page Up and Page Down to the start and end of the document |
| Ctrl+Shift with those keys | Select by the word, the paragraph, or to the start or end |
| Ctrl+Space | Start selecting where the cursor is: then move the cursor |
| Ctrl+W | Select the word under the cursor |
| Ctrl+E | Delete a word |
| Ctrl+L | Find the next misspelt word |
| Ctrl+M | Add the word under the cursor to your own dictionary |

Use a USB keyboard. From a terminal on the serial console, Ctrl+I and Ctrl+M arrive as Tab and Enter, and Shift or Ctrl with the arrow keys may not arrive at all. Ctrl+Space and the arrows still select text there.

WordGrinder does not use the mouse.

## Documents and Files

A WordGrinder file (.wg) holds a *document set*: one or more documents, which you switch between with the Documents menu, together with their settings. File, Add new blank document adds another.

File, Load document set, Save document set as and the import and export commands show a list of the files and folders in the current folder. Move through it with the arrow keys and press Enter, or type a name. A name can be a whole one, such as c:/letters/june.wg. A drive letter on its own, such as c:, goes to that drive, where A: is the SD card and C: a USB flash drive.

{widths: 16,36}
| Command | What it does |
| File, Import new document | Reads an ODT, HTML, Markdown or text file into a new document |
| File, Export current document | Writes the document as ODT, HTML, Markdown, plain text, LaTeX or Troff |
| File, Document settings, Autosave | Saves a copy every few minutes, beside the document, as name.autosave.time.wg |
| File, Save as template, Create from template | A document to start others from, such as a letter with your address |

ODT is the format to use for Microsoft Word or LibreOffice. Copy the file to a PC on the SD card or a USB flash drive. Its headings, lists, bold, italic and underlining come through as Word's own styles, and you can then lay out and print the document there.

## Checking Spelling

On a Generation 2, WordGrinder checks spelling against a whole dictionary. The folder has two: british.dictionary, which WordGrinder uses from the start, and american-canadian.dictionary. To use the American one, press Esc and choose File, Global settings, Load new system dictionary, and pick it. Any list of words, one to a line, can be used the same way.

Spelling is turned on for each document set. Press Esc and choose File, Document settings, Spellchecker. Set Display misspelt words to YES with Space, and press Enter.

- The first time it is needed, the dictionary takes about five seconds to load, and the status line says so. It takes about 16MB of memory, which MMBasic lends WordGrinder while it runs.
- Misspelt words are shown in grey.
- **Ctrl+L** finds the next misspelt word.
- **Ctrl+M** adds the word under the cursor to your own dictionary. This is a document called User dictionary in the document set, which you can edit like any other.

A capitalised word must be in the dictionary just as it is written, except at the start of a sentence. So "Windows" in the middle of a sentence is shown as misspelt if the dictionary has only "windows".

On a Generation 1 there is not enough memory for a whole dictionary. WordGrinder says "Failed to load system dictionary: not enough memory" and carries on, and your own dictionary is still used. A shorter list of words of your own will load. The same message can come on a Generation 2 if WordGrinder is started from a BASIC program that is using much of MMBasic's memory: start it from the command prompt instead.

## Memory

WordGrinder itself takes about 1MB, and a document about 100 bytes for each word. A Generation 1 lends it about 4MB, room for some 20,000 words. A Generation 2 gives it about 7MB of its own, and lends it more from MMBasic's memory when it needs it. On a Generation 1, importing a very large Markdown file can run out of memory.

## What It Does Not Do

- It has no mouse, as WordGrinder 0.8 has none.
- It does not print. Export the document as ODT or HTML and print it from a PC.
- It has no pictures, tables, fonts or page layout. These are for a word processor on a PC, after an ODT export.

## If Something Goes Wrong

If WordGrinder stops with an error, MMBasic's screen comes back with the message it gave, and **[The program wrote this while its screen was up. Press a key for MMBasic.]** below it. Anything typed since the last save is lost, which is what Autosave is for.

If it says **WordGrinder: its memory is damaged**, it has found a fault in its own memory and stopped. Please report it as an issue of the firmware's repository, saying what you were doing.
