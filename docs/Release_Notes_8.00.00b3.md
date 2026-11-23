# Colour Maximite 2: MMBasic 8.00.00b3 (beta) release notes

*22 November 2026. Compared with 8.00.00b2, the last version released.*

This is the third beta of MMBasic 8.00 for the Colour Maximite 2. Please report problems on the Back Shed forum. 7.00.00RC6 remains the last release of version 7.

The same firmware file runs on the Generation 1, the Generation 2 and the Generation 3.

8.00.00b3 brings:

- **The Generation 3's WiFi and Bluetooth module on the board.** The Generation 3 carries the module on its board, on a bus of its own, so nothing is plugged into the rear I/O connector: with the WiFi and Bluetooth on, pins 32, 33, 38 and 40 and SPI2 stay free for other uses. The firmware finds the module by itself. On a Generation 1 or 2 the plug-in module works as before.
- **SQL(TYPE n)**, which says what SQLite holds in a column of the row: an integer, a floating point number, text, a BLOB or no value.
- **Fixes:** text arriving on the serial console was lost or garbled while a program switched OPTION CONSOLE; a key typed under OPTION CONSOLE NONE was stored in the wrong memory; the prompt's history recall drew and edited each recalled line a column to the left on a serial terminal exactly as wide as the screen; and under OPTION STATUS ON the file manager's prompts were hidden on the screen and the status line said "Check Disk" for a program that was on the card.

## Before you update

**From 8.00.00b2:** save the options first and load them again afterwards, as with every update. Writing the firmware erases them:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

The options are kept in the same form as in 8.00.00b2, so a file saved under 8.00.00b2 loads into 8.00.00b3, and one saved under 8.00.00b3 loads into 8.00.00b2. The console runs at 115200 baud until the options are loaded. Save the program in memory to the SD card first: version 8 does not keep it over a reset.

The flash library is kept and works as before, and .lib files and flash slots saved by 8.00.00b2 load, as no tokens have moved. The loadable programs' services are unchanged (version 22).

**From 7.00:** read "Before you update" in 8.00.00b1's notes first (they are in Release_Notes_8.00.00b3_full.md): the program held in the flash is lost, and the library and flash slots must be saved again. Everything there applies to 8.00.00b3 too.

**Going back to 8.00.00b2:** write it as you wrote this one, and OPTION DISK LOAD the same options file.

CMM2_Programs_8.00.00b3.zip holds the same programs as 8.00.00b2's zip, unchanged: only its README.txt is new. If your card has 8.00.00b2's programs, there is nothing to copy.

**A Generation 3 with the module on its board** needs 8.00.00b3 or later for the WiFi and Bluetooth. 8.00.00b2 and earlier run it as a Generation 3 without them: they look for the plug-in module on the rear connector, and with OPTION WIFI set they print "WiFi module not found".

## What is new

### The Generation 3's WiFi and Bluetooth module on the board

The Generation 3 has the WiFi and Bluetooth module, a Raspberry Pi RM2, on its board. It is joined to the processor by a bus of its own (the processor's SPI6), not by the rear I/O connector:

- **Nothing is plugged in**, and the module uses none of the connector's pins. With OPTION WIFI or OPTION RADIO set, pins 32, 33, 38 and 40 (and GP10 and GP11, their PicoMite names) stay free for SETPIN, and SPI2 can be opened alongside the WiFi.
- **Everything else is as with the plug-in module**: OPTION WIFI, OPTION RADIO, the WEB and BLUETOOTH commands, the 128KB the network keeps and the 64KB Bluetooth keeps, and MM.DEVICE$ ("Colour Maximite 2 G3").
- **The link to the chip** runs at up to 30MHz (the plug-in module's at 24MHz on a Generation 2 and 20MHz on a Generation 1).
- **The firmware tells the boards apart** at power up by a pin the Generation 3 connects to ground, so the same file serves every board: a Generation 1 or 2, with or without the plug-in module, works exactly as before.

The User Manual says so in Hardware Features, WiFi and Internet Functions (The WiFi Module), Bluetooth, Porting Programs, Appendix D (SPI) and Appendix M, and in the entries for OPTION WIFI and SPI2.

### SQL(TYPE n)

SQL(TYPE n), beside SQL(ISNULL n), returns what SQLite holds in column n of the current row: 1 an integer, 2 a floating point number, 3 text, 4 a BLOB, 5 no value (SQLite's own numbers, from sqlite3_column_type()). SQL(n) gives a BLOB as a string, as it gives text, and a column read into a float cannot tell 2.0 from 2, so a program that writes the data out again, as a dump does, can now keep each value's type (Matthias's wish, for mmsql). It uses what sql.mod already gives the firmware, so the zip's sql.mod is unchanged.

### Fixes

- **OPTION CONSOLE in a program.** OPTION CONSOLE SCREEN stopped the serial console's port, and BOTH or SERIAL set it up again, so text arriving while a program switched between them was lost or came in garbled (Matthias: mmsql switches while reading pasted text). The original Colour Maximite 2 firmware did this to free the port for OPEN "COM3:". OPTION CONSOLE now only chooses where the output goes, as on the PicoMite: the port keeps running, and nothing that arrives is lost however often a program switches. A program can still open the console's port as COM3 (or COM1 or COM2 under OPTION CONSOLE PORT) as before: the console gives the port up while it is open.
- **OPTION CONSOLE NONE:** a key typed on the serial console was stored through COM3's receive buffer, which does not exist unless COM3 is open, so it was written to the wrong memory. It now reaches the program, as under the other settings.
- **The prompt's history recall** (the Up and Down keys): on a serial terminal exactly as wide as the screen (Tera Term at 100 columns for MODE 1, for example) each recalled line, from the second on, was drawn over the space after the prompt (">option list") and edits landed one place to the left (Matthias). The VGA screen was right. In the Colour Maximite 2's firmware from the start: a terminal one column wider, which EDIT, FM and Shift-F6 ask for, hid it. Also, insert mode could push a line past the screen's right edge.
- **The file manager's prompts under OPTION STATUS ON:** while FM waited for a name (to rename, duplicate, go to a path, filter, or make a new file or directory), the screen's bottom row showed the prompt's status line (the program's name and the time) in place of the question, redrawn every second. The serial terminal was right. In the firmware since 7.00.00b4 (Peter).
- **The status line under OPTION STATUS ON** said "Wrong Disk" and then "Check Disk" after a program was loaded, although its file was on the card: the check looked for the name with a stray character in front of it. Since 7.00.00b4, when a program's name began to keep its drive.
- **MM.VER** is 8.000003.

## What behaves differently

- **OPTION CONSOLE SCREEN** no longer stops the serial console's input: keys typed on the serial console reach a program under OPTION CONSOLE SCREEN, CTRL-C included, as on the PicoMite. "Console port in use" is now the error of OPTION CONSOLE BOTH or SERIAL while the console's port is open as a COM port (COM1, COM2 or COM3).
- **On a Generation 3** the WiFi's and Bluetooth's pins and SPI2 are free (above).

## Structural changes

For those who build the firmware or study its source:

- **Which board:** getVersion (Src/main.c) reads PF6 as well as PF9 at power up. PF9 to ground is a Generation 2 or 3 (as before); PF6 to ground as well is a Generation 3 with the module on the board (RM2OnBoard). It now leaves both pins as analog inputs with no pull; it had set port E there by mistake, so PF9 kept its pull-up.
- **The module's bus on a Generation 3** (Src/net/cyw43_bus_cmm2.c): SPI6, with the clock on PG13 and the data, in both directions and with the chip's interrupt, on PG14 (both alternate function 5), chip select PF7 and power on PF8, all on the board. SPI6 is in the D3 domain, clocked from its bus clock (120MHz at 480MHz) divided by 4. It is a limited SPI (16-bit frames at most, an 8-byte FIFO), so each 32-bit word of the chip's protocol goes as two 16-bit frames, and a long transfer in parts of at most 511 words. The plug-in module's path (SPI2, 32-bit frames) is unchanged.
- **The pins:** cyw43_driver_cmm2.c has the board's pins beside the plug-in module's; Src/net/WebMite_cmm2.c reserves the connector's four pins and SPI2 only for the plug-in module.
- **The console's port** (Src/Serial.c): ConsoleUartUp says whether the console has its UART; a program's OPEN of that port hands it over (ConsoleHandOver) and ConsolePortOpen gives "Console port in use". The interrupt handlers of USART1, USART2 and UART4 (Src/stm32h7xx_it.c) give a byte to whoever has the port: the console, else the COM port if it is open, else it is read and dropped.
- **Unchanged:** the options record (layout 5), the tokens (image format 7) and the loadable programs' services (version 22).
- docs/G3_V1.2_Header_Plan.md records the board's connections, the antenna's keep-out and the firmware for the module on the board.

## The programs

CMM2_Programs_8.00.00b3.zip is 8.00.00b2's zip with README.txt for this version. When it was published no program in it had changed since 8.00.00b2 (tools/zip_apps_check.py: none to review; micropython.app is the same build), and each runs on 8.00.00b3 as on 8.00.00b2.

**quest/scummvm.app was replaced on 23 November.** In the games that speak, the first one read each line of speech from the SD card as it played, in the interrupt that makes the sound: Day of the Tentacle, Sam & Max Hit the Road, Indiana Jones and the Fate of Atlantis on CD, Full Throttle, and the CD versions of The Legend of Kyrandia 1 and 2. A line that played while the game loaded a room or saved could have read the wrong data, or damaged a file on the card; no one is known to have seen it happen. The new one reads each line into memory before it plays it: on a Generation 1 Day of the Tentacle played for two minutes with it, each line read in 111 ms at most. Copy the new A:/quest/scummvm.app from the zip if you downloaded it before then. 7.00.00RC6's zip has the same new file.

## Tested

**The release file on a Generation 1** (400MHz, with the plug-in WiFi module and OPTION WIFI set). The options file saved under 8.00.00b2 loaded unchanged.

- **The test suites, compared with 8.00.00b2's run on the same board.** These are:
  - tests/cmm2 (113 programs);
  - the PicoMite tests;
  - the crash tests: all passed;
  - the corpus (212 programs);
  - the test files (66 programs);
  - SP Tools: 686 of 727, as under 8.00.00b2 (the rest need a USB flash drive).

  Every program gave what it gave under 8.00.00b2, apart from four, each explained:
  - one test now shows pins 32, 33, 38 and 40 reserved, and two network tests now run: the WiFi module was plugged in after 8.00.00b2's run;
  - one test that switches OPTION CONSOLE now matches: under 8.00.00b2 it lost a character to the switching ("loop 1" came out as "loopð1"), the fault fixed here.
- **The WiFi:** it joined at start-up. 100 pings of 1400 bytes from a PC came back with 4 lost (WiFi). A 1MB file sent by TFTP and read back was identical. While the WiFi has them, pins 32, 33, 38 and 40 are reserved and SPI2 OPEN is refused. The network tests gave the same output, byte for byte, as on 22 November with the same code before the version number changed.
- **The fixes:**
  - **OPTION CONSOLE:** 200 characters pasted while a program switched between OPTION CONSOLE SCREEN and BOTH about 7,450 times arrived identical.
  - **OPTION CONSOLE NONE:** 200 characters typed arrived identical.
  - **History recall:** recalled lines were drawn in the right place with the cursor where MMBasic expects it, on a terminal 100 columns wide (the screen's width) and 101.
- **SQL(TYPE n):** it gave 3, 4, 1, 2, 3, 1 for text, a BLOB, 2, 2.0, '12' and 12, matching SQLite's typeof().
- **The programs:** each program in the zip that runs on a Generation 1 started from the prompt with no error, and the prompt came back when it was left. These are MicroPython, Doom, ScummVM, the 386 PC, the NES, Heretic, the Spectrum, the CPC, the C64, the Atari 800, the TRS-80, the BBC Micro, the VT100 terminal, zeptoforth for the G1, WordGrinder, sc-im and SQL's example. The Generation 2 programs (zeptoforth, Quake and the Amiga) refused with "Needs a G2", as they should. The Apple //e and the C compiler were not on that board's card.

**The same changes on a Generation 3** (V1.1, with the plug-in WiFi module), in builds of the same code before the version number changed:
- **21 November:** the OPTION CONSOLE paste test (200 characters over about 8,700 switches, twice), OPTION CONSOLE NONE, the history recall at 100 and 101 columns, and SQL(TYPE n).
- **22 November:**
  - The new 16-bit transfer code carried the plug-in module's traffic in short parts. It joined, 100 of 100 pings came back, a 1MB TFTP round trip was identical, and there were no bus errors.
  - A build that treated this board as one with the module on the board found no chip, said "WiFi module not found" within 2.2 seconds, and left every pin and SPI2 free.
  - The final code ran the plug-in module as before.

The Generation 2 and the Generation 3 have not run the release file itself: only one board could be connected.

**The release file was replaced on 22 November** with the two OPTION STATUS fixes above, an hour after it was first published. They change two lines, and no other code moved. The tests above ran on the first file. The new file runs on the same Generation 1 with its options loaded back unchanged. Under OPTION STATUS ON, FM's rename prompt there reads "Rename occom3.bas to: occom3.bas", and after RUN the status line shows "A:/T21/sqltype2.bas" with no "Check Disk".

## Known problems

- **No Generation 3 with the module on its board has been made yet.** Its path has run only in a test build that took a Generation 3 without the module for one with it: the firmware found no chip, said "WiFi module not found" and carried on with every pin free. The same 16-bit transfer code has carried the plug-in module's traffic without an error (see Tested). If the first boards misread at 30MHz, the bus can be slowed to 15MHz in the next beta.
- **Opening the console's own port as COM3** (or COM1 or COM2 under OPTION CONSOLE PORT) in a program has not been tried on a board since the OPTION CONSOLE change. COM3 still opens only when the saved option is OPTION CONSOLE SCREEN or NONE, as before.
- **A large loadable program on a Generation 1 after many BASIC programs.** After a long session the 386 PC and the NES emulator stopped with "Needs n KB, n KB free in one block". In this testing that came after about 400 test programs. Each started at once after a restart (CPU RESTART or the reset button). Something those programs used is held until a restart and keeps the loader from joining the display memory to the heap. 8.00.00b2 has the same code. Which statement does this is not yet known.
- **Bluetooth game controllers** have not been tried with a real controller yet.
- **Why the radio chip once gave a wrong Bluetooth buffer position** is not known; the firmware refuses it, so it costs at most a packet.
- **Bluetooth and the network wait while a loadable program runs:** a Bluetooth keyboard does not type into it and a speaker plays nothing (MicroPython's network is the exception: it services the network itself).
- **The Bluetooth serial device:** a Bluetooth LE module passes on only part of a long burst at 115200 baud (the module's limit): send in pieces or more slowly.
- **MicroPython on a Generation 1** has about 1.6MB for Python. In one long session two of the book's games (ch19/05 and ch25/02) still run out of memory after the examples before them; each runs after Ctrl-D (a soft reset).
- **The WebMite's own example programs** have not all been tried yet.
- **A Generation 3 can reset when its HDMI cable is unplugged** (the cable's shield meeting the monitor's ground on the reset line). The next board has a fix.
- The known problems of 7.00.00RC6 remain, apart from the three SQL faults fixed in 8.00.00b1.

## The manuals

- **The User Manual** is for 8.00.00b3: the Generation 3's module on the board (Hardware Features, WiFi and Internet Functions, Bluetooth, Porting Programs, Appendices D and M, OPTION WIFI and SPI2), SQL(TYPE n) (in SQL( and the chapter SQL Databases), and OPTION CONSOLE.
- **Colour Maximite 2 Loadable Programs:** its title page (firmware 8.00.00b3), and 8.00.00b3 in the list of services versions (still 22).
- **The App User Guide and the C Compiler guide** are unchanged apart from the zip's name. The 386 PC Emulator guide and the Stepper reference are unchanged.

## Licences

Unchanged since 8.00.00b2. The firmware's own licence (LICENSE) is unchanged. For the WiFi and Bluetooth it includes lwIP, Mbed TLS, the cyw43-driver, BTstack, puff and the CYW43439's firmware. **BTstack**'s licence allows redistribution and use "solely for personal benefit and not for any commercial purpose or for monetary gain" (Raspberry Pi's licence for BTstack covers only its own chips): free for the free firmware, but anyone who sells boards with it needs a commercial licence from BlueKitchen. cyw43-driver's copy carries the same condition. Each licence is beside its code in Middlewares/Third_Party, and in the licences folder of the zip.
