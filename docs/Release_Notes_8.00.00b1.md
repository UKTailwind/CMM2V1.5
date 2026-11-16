# Colour Maximite 2: MMBasic 8.00.00b1 (beta) release notes

*16 November 2026. Compared with 7.00.00RC6, the last version released.*

This is the first beta of MMBasic 8.00 for the Colour Maximite 2. Please report problems on the Back Shed forum.

**7.00.00RC6 is the last release of version 7.** It is highly functional and well tested, and can be used as it is, but there will be no full 7.00.00 release: version 8 is needed for the new Generation 3 and for WiFi, and all further work goes into it.

The same firmware file runs on the Generation 1, the Generation 2 and the new Generation 3.

Version 8 brings:

- **WiFi and the Internet.** With a WiFi module on the rear I/O connector the Colour Maximite 2 runs the WebMite's networking: a web server, web and TLS clients, UDP, MQTT, time from the Internet, a Telnet console and TFTP file transfer, with the WebMite's WEB commands.
- **The Generation 3.** A Generation 2 with an HDMI output in place of VGA, with the sound sent to the HDMI monitor as well as to the audio socket.
- **Programs always run from RAM.** The BASIC program is never written to the flash, so loading is quicker and editing and running programs no longer wear it. The flash it used now holds the networking.
- **USER modules.** Commands and functions of your own, written in C, which a BASIC program calls like the built-in ones.
- **SQL fixes**, with text values of any length, and more of the PicoMite's commands for programs written for it.

## Before you update

Read this section first: going from 7.00 to 8.00 changes where the program and the library are kept.

**Save your program to the SD card.** Version 7 kept the program in the flash; version 8 puts its own code in that part of the flash, so the program held there is lost when the new firmware is written.

**Save the options first, and load them again afterwards**, as with every update. Writing the firmware erases them:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

A file saved by OPTION DISK SAVE under 7.00 loads into 8.00 (its WiFi options start off). The console runs at 115200 baud until the options are loaded. The variables saved with VAR SAVE and the files on the SD card are kept.

**The library has to be loaded again.** The flash library survives the update, but version 8 numbers some of its functions differently (below), so a library saved by 7.00 would call the wrong ones. When the options are loaded, MMBasic says "The library was saved by another firmware version and has been discarded: LIBRARY LOAD its files again". Load it from its source files with LIBRARY LOAD. A .lib file saved by LIBRARY DISK SAVE under 7.00 is refused for the same reason.

**The flash slots** (FLASH SAVE n) are files on the SD card and are kept, but a program saved in one by 7.00 is refused ("Flash slot n holds ... from another firmware version: load it and save it again"). Load the program from its .bas file and FLASH SAVE it again.

**A program that runs at power up** must now be the file AUTORUN.BAS in the root of the SD card, with OPTION AUTORUN ON (see Programs in RAM below).

**Going back to 7.00** is possible: write 7.00.00RC6 with "Full chip erase" ticked in STM32CubeProgrammer, then OPTION DISK LOAD an options file saved by 7.00 (7.00 cannot read one saved by 8.00), and LIBRARY LOAD the library's files again.

Copy CMM2_Programs_8.00.00b1.zip to the card as well if you use the programs in it. The programs are the same as in the RC6 zip; only its README and the licences of the firmware's network code are new.

## What is new

### WiFi and the Internet

The networking is the PicoMite's: the WebMite firmware's code, unchanged, with the same WEB commands, options and functions, so a WebMite program runs as it is, apart from the I/O pins it uses. The User Manual has a new chapter, WiFi and Internet Functions.

- **The module.** A Raspberry Pi RM2 (the WiFi chip of the Pico W, an Infineon CYW43439) on the rear I/O connector, using four pins: 40 (clock), 38 (data), 33 (chip select) and 32 (power). On a Generation 1 the connector is turned through 180 degrees, so the module plugs on the other way round: its pin 1 always goes to the connector's pin 1.
- **Turning it on.** OPTION WIFI ssid$, password$ [, name$] [, country$], or with a fixed IP address. Without OPTION WIFI the module is switched off and the four pins are free. If no module answers at start-up the Colour Maximite 2 says "WiFi module not found" and carries on without the network.
- **What works:** WEB CONNECT, SCAN, PING and NTP; the web server (OPTION TCP SERVER PORT, WEB TCP INTERRUPT, WEB TRANSMIT PAGE and FILE, several clients at once); TCP clients, encrypted with TLS too (WEB OPEN TLS CLIENT, with WEB TLS CA to check the server's certificate); UDP (OPTION UDP SERVER PORT, WEB UDP SEND, MM.ADDRESS$, MM.MESSAGE$); MQTT (WEB MQTT CONNECT, PUBLISH, SUBSCRIBE, MM.TOPIC$); the Telnet console (OPTION TELNET CONSOLE ON, or ONLY to leave the serial port out); TFTP file transfer to and from a PC (OPTION TFTP ON); MM.INFO(IP ADDRESS), (WIFI STATUS), (TCPIP STATUS), (TCP REQUEST n), (MAX CONNECTIONS), (TELNET) and the rest.
- **Memory.** The network keeps 128KB for itself while OPTION WIFI is set. On a Generation 2 or 3 it comes from BASIC's memory for variables (MM.INFO(HEAP) is 128KB less). On a Generation 1 it is the top of the video memory, so some modes and colour depths have one page fewer (the chapter lists them) and the memory for variables and loadable programs is unchanged.
- **Loadable programs.** The network waits while a loadable program (.app) runs. On a Generation 1 a program built to use the video memory (Doom, the emulators) is lent the network's memory as well, so afterwards the network starts again from the beginning and joins the WiFi network again, in about seven seconds.
- **Differences from the WebMite:** the pins; no heartbeat LED; no PLAY STREAM (it needs a VS1053); and WEB TCP INTERRUPT 0, which the WebMite's manual gives for turning the interrupt off, is an error, on the WebMite too.

### The Generation 3 (HDMI)

The Generation 3 is a Generation 2 whose video goes through a MacroSilicon MS7210, which turns it into HDMI. The firmware recognises it at start-up; MM.DEVICE$ returns "Colour Maximite 2 G3".

- **Every mode** shows, from power up, including OPTION PICOMITE's resolutions. Modes whose pixels are wider than a standard mode's are sent at a standard rate with each pixel repeated, so that monitors accept them: mode 7 reaches the monitor as 640 x 480 at 75Hz, 12 as 1920 x 1080 at 60Hz, 13 as 800 x 600, and 4, 5, 6 and 17 as 640 x 480. Mode 18 is sent as 1024 x 600, the resolution of many small HDMI displays.
- **Full colour levels:** white is full white. OPTION VGA OUTPUT and OPTION HORIZONTAL OFFSET are kept but not used on a Generation 3.
- **Sound through HDMI** as well as the audio socket: everything MMBasic and the loadable programs play reaches the HDMI monitor's speakers too, sixteen bits unchanged.
- **OPTION DVI ON** sends DVI instead, without sound, for a monitor (or an HDMI to DVI adapter) that does not accept the full HDMI signal. It gives "Needs a G3" on a Generation 1 or 2.
- **UPDATE FIRMWARE** works on a Generation 3: the HDMI chip is reset first, so that it stays quiet while the processor's bootloader runs.

### Programs always run from RAM

- The program is held in 512KB of the SDRAM, as OPTION RAM did in 7.00, on every generation. It is never written to the flash, so loading is quicker and editing and running programs do not wear the flash. LIBRARY SAVE and the options are the only writes to the flash that remain.
- OPTION FLASH and OPTION RAM are gone; typing either gives the ordinary error for an unknown option. An options file saved by 7.00 with either set loads without error.
- **The program is lost** when the power is removed, or on a reset (the reset button or the watchdog).
- **The program is kept over an option's restart:** when a command restarts MMBasic (an OPTION that needs a restart, OPTION RESET and the like), MMBasic first saves the program to a temporary file on the SD card and loads it back after the restart, so LIST, RUN and EDIT carry on. Without an SD card the program is lost, as with OPTION RAM.
- **OPTION AUTORUN ON** runs A:/AUTORUN.BAS at power up or after the watchdog. Without a card it goes to the prompt.
- **The library** has a 128KB sector of the flash to itself, so it no longer takes space from programs.

### USER modules

USER LOAD "file" loads a module of your own, written in C with the loadable programs' tools; then USER name [arguments] runs one of its commands and USER(name [arguments]) one of its functions, which read their keywords and arguments with the interpreter's own routines, as built-in commands do. USER CLOSE unloads it. RUN, NEW and CHAIN unload it with the rest of the program's memory, and the next USER or USER( loads it again from the same file; loading it again also picks up a rebuilt module without a restart.

The companion manual, Colour Maximite 2 Loadable Programs, has a new chapter, USER Modules, and the tools have a module to start from (tools/sdk/examples/user/proforma.c), built with build.py --user. The SQL command works the same way, with SQLite as its module, and a USER module can be loaded at the same time as the SQL module.

### SQL

The three faults homa found in RC6 are fixed:

- **Ctrl-C** in a long statement gives the error "SQL: stopped by Ctrl-C", which ON ERROR SKIP catches like any other error; before, it ended the program.
- **Long text:** SQL(LONGSTRING n, a%()) reads a column of any length into a LONGSTRING and returns its length. A LONGSTRING among a statement's values gives its text to a ? parameter, and a statement itself can be a LONGSTRING, for one longer than 255 characters. A string too long for SQL(n) is still an error, but the query stays on its row, so a program can read the column again with SQL(LONGSTRING.
- **SQL INSERT of an array of structures** that stops part way (an error or Ctrl-C) now undoes its rows and closes its transaction; before, every later change was lost at SQL CLOSE.
- UTF-16 databases made on a PC open (also in the RC6 zip's sql.mod since 16 November).

### For PicoMite programs

- **WATCHDOG HW timeout** and **WATCHDOG HW OFF**, under OPTION PICOMITE, work the ordinary watchdog (timeout 1 to 8331 ms, the PicoMite's limit), so a PicoMite program that uses the hardware watchdog runs unchanged.
- **MM.INFO(BOOT COUNT):** the number of starts, kept in the clock's battery-backed memory.
- **OPTION COUNT** under OPTION PICOMITE: accepted when it names the Colour Maximite 2's own four counting pins (7, 13, 15 and 24, which are GP6 to GP9), in any order; any other pin is an error that says which pins count here.
- **MODE 19** is now 720 x 400 at 70Hz, the PicoMite's 720x400, which OPTION PICOMITE's 720x400 now uses at full screen (it was shown in a corner of 848 x 480).
- **640x480 at 75Hz:** OPTION RESOLUTION 640x480, 315000 (and RESOLUTION or OPTION PICOMITE with that speed) gives 75Hz, as on the PicoMite; MM.INFO(MODE) then returns 20.
- Geoff Graham's Retic, the WebMite irrigation controller, runs under OPTION PICOMITE with only its pins changed.

### Other changes and fixes

- **OPTION COMPILE** compiles plain BLIT, BLIT WRITE (drawn directly: fire.bas 61.5 to 24.4 ms a frame), SPRITE SHOW, HIDE and NEXT, IMAGE RESIZE_FAST and RGB colour constants.
- **TURTLE ARC LEFT and ARC RIGHT** gave "Expression syntax"; they work now.
- **PIXEL(** in an 8-bit colour mode returns the colour the palette holds, on every board.
- **MODE 18** and OPTION PICOMITE's 1024x600 on VGA are sent as 1024 x 768 at 60Hz with the 600 lines in the middle, as many VGA monitors refuse 1024 x 600. A Generation 3 sends 1024 x 600 itself.
- **The first start's time:** the tens of hours accepted any key, so an Enter was taken as a digit.
- **The first start's font:** after a firmware update with the options erased, every program's END could fail with "Invalid font number #16".
- **TIMER = n** no longer resets the clock underneath: TIMER is now an offset from it, as on the PicoMite, so timeouts (and the network's clock) never go backwards.
- **MM.VER** is 8.000001.

## What behaves differently

- The program is lost at power off and reset, and OPTION FLASH and OPTION RAM are gone (above).
- **The memory for variables is smaller:** about 4MB on a Generation 1 (it was 5MB, 4.5MB with OPTION RAM) and 15MB on a Generation 2 or 3 (15.5MB), because the program and the CSUBs' code now each have their own 512KB. A loadable program on a Generation 1 borrows about 3.75MB at the command prompt (it was 4MB); all the programs in the zip still fit.
- **MODE 19** is 720 x 400; in 7.00.00RC5 and RC6 it was 640 x 512, an experiment.
- **EOF(, LOC( and LOF(** are now one token internally (LIST shows them as before). This moved the function tokens after them, which is why a library, a .lib file or a flash slot saved by 7.00 is refused.
- **New keywords:** WEB, USER, USER(, MM.MESSAGE$, MM.ADDRESS$ and MM.TOPIC$. A program that uses one of these names for its own variable or array must rename it.
- **With OPTION WIFI set** pins 32, 33, 38 and 40 and SPI2 belong to the network: SETPIN refuses them and the SPI2 command cannot open SPI2.

## Structural changes

For those who build the firmware or study its source:

- **Branches:** main stays 7.00. Version 8 is on the branch v8-dev (v8-wifi is the same commit at this release).
- **The flash:** bank 2's sectors 3 to 6 (512KB), the program's until 7.00, are now firmware: the network stack is there, in a .net section of about 430KB. Sector 7 is the library's alone and sector 2 still holds the options. The firmware file grew from 1.41MB to 1.87MB. TURTLE and DRAW3D moved from bank 1 to bank 2, leaving about 18KB of bank 1 free.
- **The SDRAM:** the program's 512KB is at the very top (Generation 1 0xD0780000, Generation 2 0xD1780000), with the 512KB executable window for CSUB code below it (together one 1MB executable MPU region), the heap below that, and the compiler's stream directly after the video memory. The Generation 2's 8MB for loadable programs is unchanged. The User Manual's memory map (Special Functions and Characteristics) has the details.
- **Third-party code** in Middlewares/Third_Party, each with its licence: lwIP 2.2.1, Mbed TLS 3.6.6 and George Robotics' cyw43-driver with Infineon's CYW43439 firmware, at the commits the PicoMite's pico-sdk 2.3.1 has, unchanged.
- **The PicoMite's network code** (MMtcpserver.c, MMTCPclient.c, MMudp.c, MMMqtt.c, MMntp.c, MMping.c, MMtelnet.c, MMtftp.c, WiFi.c, mqtt.c, tftp.c) is copied unchanged into Src/net/picomite by tools/port_net.py; the Colour Maximite 2's side is Src/net/WebMite_cmm2.c, with the radio's bus on SPI2 in half-duplex at 24MHz (20MHz on a Generation 1 at 400MHz) in Src/net/cyw43_bus_cmm2.c. The network's .data and .bss are grouped by the linker script, so that a Generation 1 can start the network again after lending its memory.
- **The Generation 3:** Src/MS7210.c (the HDMI chip: reset on PG3, set up over its own I2C, the clock on PG7 doubled for the wide-pixel modes) and Src/HdmiAudio.c (SAI2 as I2S at 44,117.6Hz to the MS7210).
- **USER modules:** Src/User.c and Inc/cmm2user.h; the loadable programs' services are version 20 (no new services: the start block gains a table of the interpreter's routines for a module).
- **The options record** is layout 4, 1,212 bytes (the WEB options and OPTION DVI added); OPTION DISK LOAD also takes 7.00's 992-byte files.
- **Tokens:** EOF( LOC( LOF( are one function token, EOFLOCLOF(, as BIT( BYTE( FLAG( are; SQL( is 0xFC, USER( 0xFD, and 0xFE is free. The image format is 7, and the flash library is now checked for it at start-up and by LIBRARY DISK LOAD.
- **TIMER** is an offset on the microsecond count (GetuSec), not a rewrite of the hardware timer.

## The programs

CMM2_Programs_8.00.00b1.zip has the same programs as the RC6 zip as it has been since 16 November (with the UTF-16 sql.mod), and they run on 8.00.00b1 unchanged, on the Generation 3 as well. README.txt is new, for this version, and the licences folder has the licences of the firmware's network code.

## Tested

- **Generation 3:** every mode on two HDMI monitors, OPTION PICOMITE's resolutions and modes, the emulators and Doom; the sound through HDMI checked bit for bit (65,536 frames identical to the file); OPTION DVI; the regression tests as on a Generation 2.
- **Generation 1 and Generation 2:** the full regression (the test programs, the PicoMite tests, the crash tests, the program corpus and SP Tools) with the same results as before, every video mode and the sound.
- **WiFi** on all three generations: joining, scanning, ping, the time, the web server (several clients at once), HTTP and HTTPS clients (with the certificates checked by WEB TLS CA), UDP, MQTT, TFTP and the Telnet console; the firmware with OPTION WIFI set and no module, and without the option; the regression run again with the network up, with no change but the network tests. Retic ran on a Generation 3 with its web pages in use from a PC.
- **USER modules** on a Generation 1 and a Generation 3.
- **The update from 7.00.00RC6** on a Generation 2, as a user does it: RC6 with a library using EOF(, LOF( and INPUT$(, its options saved; 8.00.00b1 written without a full erase; the options loaded, the library discarded with the message above, its .lib file refused, and the library loaded again from its source and working.

## Known problems

- **Bluetooth:** the RM2 module also has Bluetooth, which is not used yet.
- **The WebMite's own example programs** have not all been tried yet; the chapter's examples and Retic have.
- **A Generation 3 can reset when its HDMI cable is unplugged** (the cable's shield meeting the monitor's ground on the reset line). Switching the monitor off and on is fine. The next board has a fix.
- The network waits while a loadable program runs.
- The known problems of 7.00.00RC6 remain, apart from the three SQL faults, now fixed.

## The manuals

- **The User Manual** is for 8.00.00b1: the new chapter WiFi and Internet Functions, with every WEB command, option and function in its references; Running Programs (programs in RAM, kept over an option's restart, AUTORUN.BAS); the Generation 3's HDMI in Hardware Features; the new memory map; OPTION DVI, WATCHDOG HW, MM.INFO(BOOT COUNT), OPTION COUNT, USER and USER(, SQL(LONGSTRING, MODE 19; OPTION FLASH and OPTION RAM removed.
- **Colour Maximite 2 Loadable Programs** adds the chapter USER Modules, build.py --user and services version 20.
- The App User Guide, the C Compiler guide and the 386 PC Emulator guide are unchanged apart from the zip's name.

## Licences

The firmware's own licence (LICENSE) is unchanged. The network code brings lwIP (BSD-3-Clause), Mbed TLS (Apache-2.0) and George Robotics' cyw43-driver, each with its licence beside it in Middlewares/Third_Party, and Infineon's firmware for the CYW43439, which the driver loads into the chip. Their licence texts are also in the licences folder of the zip.
