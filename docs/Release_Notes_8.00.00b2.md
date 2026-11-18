# Colour Maximite 2: MMBasic 8.00.00b2 (beta) release notes

*18 November 2026. Compared with 8.00.00b1, the last version released.*

This is the second beta of MMBasic 8.00 for the Colour Maximite 2. Please report problems on the Back Shed forum. 7.00.00RC6 remains the last release of version 7.

The same firmware file runs on the Generation 1, the Generation 2 and the Generation 3.

8.00.00b2 brings:

- **Bluetooth.** The WiFi module's chip is a Bluetooth radio too. Bluetooth keyboards, mice (and a keyboard's touchpad) and game controllers; the sound through a Bluetooth speaker or headphones; and a Bluetooth serial device as COM8. It is the PicoMite's Bluetooth, with its BLUETOOTH command, and the WiFi and Bluetooth can be used at the same time.
- **MicroPython** runs every example program of the book *Programming the Pico Computer 3*, with the clock and its alarm, asyncio, native code, the I/O pins as on a Pico Computer 3 and, with the WiFi module, the network: sockets, HTTPS, MQTT.
- **Loadable programs** can use the network, the clock's alarm and the LED (services versions 21 and 22).
- **Fixes:** a program that left GUI controls behind could, at the next RUN, damage the WiFi's and Bluetooth's memory (since 8.00.00b1); WEB TCP CLIENT with a 64KB receive array; a memory fault that could give a program memory already in use; LGETSTR$ of 255 characters, which could spoil an open file (an SQL COMMIT failed with "disk I/O error"); and WEB CONNECT, which did not join the WiFi again once it was down.

## Before you update

**From 8.00.00b1:** save the options first and load them again afterwards, as with every update. Writing the firmware erases them:

```
OPTION DISK SAVE "A:/myoptions"
```

Then write the firmware, and:

```
OPTION DISK LOAD "A:/myoptions"
```

A file saved under 8.00.00b1 loads into 8.00.00b2: OPTION RADIO is set from OPTION WIFI, so the WiFi carries on as before and Bluetooth stays off. The console runs at 115200 baud until the options are loaded. Save the program in memory to the SD card first: version 8 does not keep it over a reset.

The flash library is kept and works as before, and .lib files and flash slots saved by 8.00.00b1 load, as no tokens have moved.

**From 7.00:** read "Before you update" in 8.00.00b1's notes first (they are in Release_Notes_8.00.00b2_full.md): the program held in the flash is lost, and the library and flash slots must be saved again. Everything there applies to 8.00.00b2 too.

**Going back to 8.00.00b1:** write it with "Full chip erase" ticked in STM32CubeProgrammer, then OPTION DISK LOAD an options file saved under 8.00.00b1, and LIBRARY LOAD the library's files again. 8.00.00b1 refuses a file saved by 8.00.00b2 ("File size incorrect"), as that holds the Bluetooth options and pairings too.

Copy CMM2_Programs_8.00.00b2.zip to the card as well if you use the programs in it: its micropython.app is new, and needs 8.00.00b2.

## What is new

### Bluetooth

The Bluetooth is the PicoMite's: the code of its Bluetooth versions (PicoMiteRP2350BTH, PicoMiteHDMIBTH and PicoMitePC3), unchanged, with the same BLUETOOTH command, options and functions. The User Manual has a new chapter, Bluetooth.

- **Turning it on.** It needs the WiFi module. OPTION RADIO BOTH (WiFi and Bluetooth), WEB, BLUETOOTH or NONE, the PicoMitePC3's option. NONE is the default, and OPTION WIFI turns the WiFi's half on, so nothing changes until OPTION RADIO is set.
- **Keyboards, mice and game controllers.** Bluetooth LE devices pair by themselves when put into pairing mode nearby; a Bluetooth Classic keyboard is found with BLUETOOTH SCAN and connected with BLUETOOTH CONNECT "name". A keyboard works as a USB keyboard does, at the prompt, in the editor and in programs, with its lock lights. A mouse, or a keyboard's touchpad, is the mouse on channel 2 (DEVICE(MOUSE 2, ...), MM.MOUSE, the GUI cursor). A Bluetooth LE game controller is DEVICE(GAMEPAD 3, ...), with GAMEPAD INTERRUPT 3.
- **Speakers and headphones.** OPTION AUDIO BLUETOOTH, then BLUETOOTH CONNECT "name": everything PLAY makes is played through the speaker, and through the audio socket as well. The speaker is remembered and connects again by itself. The stream is suspended after 60 seconds of silence (OPTION AUDIO BLUETOOTH n sets the time; 0 keeps it running).
- **Serial devices as COM8.** A Bluetooth Classic serial device (SPP: a GPS receiver, an HC-05) or a Bluetooth LE serial module (the Nordic UART Service, HM-10, Microchip's Transparent UART), connected with BLUETOOTH CONNECT and opened with OPEN "COM8:", with a receive buffer, an interrupt and AS GPS.
- **Pairings** are stored with the options, so paired devices reconnect after a power cycle. BLUETOOTH FORGET deletes them; OPTION DISK SAVE and LOAD keep them.
- **BLUETOOTH STATUS** lists the connections; MM.INFO$(BLUETOOTH KEYBOARD | SPEAKER | SERIAL) gives a device's address.
- **Memory:** Bluetooth keeps 64KB of the memory for variables while it is on (MM.INFO(HEAP) is 64KB less).
- **Differences from the PicoMite:** the serial device is COM8, not COM7; OPTION AUDIO DAC, not OPTION AUDIO DISABLE, ends the sound to Bluetooth; there is no console over Bluetooth; and Bluetooth is not serviced while a loadable program runs.

### MicroPython

The CMM2's MicroPython (micropython.app in the zip) now runs every one of the 162 example programs of *Programming the Pico Computer 3*, Peter Mather's course in Python for the Pico Computer 3. The book's new Appendix K, Running this book on a Colour Maximite 2, gives the few differences: how to start it, where the files are, and which connector pin is which. The book is in github.com/UKTailwind/micropython, branch pico-computer-3.

New in the app:

- **The I/O pins as on a Pico Computer 3:** the same GP numbers, GP40 to GP46 and ADC(n) as on the RP2350B, Pin("LED") the SD card's activity light, I2C on any pins (a software I2C), and a pin's owner changes as on a PC3.
- **The clock:** gettime(), settime(), machine.RTC and a daily alarm, on the DS3231 with OPTION DS3231 ON or on the processor's own clock.
- **machine.WDT** (the watchdog), **asyncio**, and **@micropython.native, viper and asm_thumb**.
- **The network**, with the WiFi module and OPTION WIFI: wifi() and ntpsync() at the prompt, socket, network.WLAN, TLS (HTTPS), requests, MQTT (umqtt), ntptime and mip.
- **Fixes:** SQLite's memory is set aside at start-up (after a long session connect() could fail with memory free); `import ulab.numpy`; files that a program dropped without closing them are closed when the limit of open files is reached; and a program's end now lets go of its off-screen buffer (hdmi.create()) and its sprites and collects the memory, so that the book's games run one after another on a Generation 1, which has less memory.

It needs 8.00.00b2, as it uses the new services below.

### Loadable programs: services 21 and 22

- **Version 21:** rtc_alarm, the clock's daily alarm; and the network: net_poll, net_join, net_leave, net_ifconfig and net_dns, and eight TCP or UDP handles (sock_open to sock_close) over the firmware's own network. While a program runs the firmware does not service the network itself: net_poll does.
- **Version 22:** led (the SD card's activity LED as the program's own), gp_pin (a GP's connector pin) and pin_setup's pull for an open drain output.

The Loadable Programs guide describes them (The Services: The Clock and The Network; The I/O Pins: The LED).

### GP40 to GP46

GP40 to GP46, the analog inputs of an RP2350B (and of the Pico Computer 3), are second names for the Colour Maximite 2's analog pins: GP40, GP41 and GP42 are pins 26, 29 and 37 (GP26 to GP28, so ADC channel n is GP40+n on both chips), and GP43 to GP46 are pins 7, 13, 15 and 24 (GP6 to GP9). A program that uses a second name must not use the pin's first name as well. MM.INFO(PINNO GP40) is 26. GP47 still has no pin. MM.INFO(MAX GP) is 46 (it was 37); under OPTION PICOMITE it is still the PicoMite's.

### Fixes

- **RUN after a program that used GUI controls** (OPTION MAXCTRLS) could damage the network's memory, and with Bluetooth on Bluetooth's: RUN, NEW and loading a program freed the old program's controls (and its serial ports' buffers) after the memory they were in had already been cleared, and each such free went on to free the memory kept above it for the WiFi and Bluetooth. Programs then printed lwIP errors ("mem_free: illegal memory") or the Colour Maximite 2 stopped. In 8.00.00b1 too, on any board with OPTION MAXCTRLS and OPTION WIFI. The old program's I/O is now closed first, as on the PicoMite, and a second free of the same memory does nothing.
- **Bluetooth:** a wrong position read from the radio chip's Bluetooth buffer (once, on a Generation 2 with a speaker connected) made the firmware overwrite 56KB of its memory and stop. Such a position is now refused, and the read tried again at the next poll.
- **WEB TCP CLIENT REQUEST** into a 64KB array (DIM b%(8192)) received nothing ("No response from server"), and into a larger one lost part of each packet. The PicoMite had the same fault, now fixed there too.
- **Memory:** MMBasic's memory for variables could be given space that the C library's own allocator had already handed out (used by the audio decoders, JSON, the file system and now Bluetooth), when a program filled the memory. They are now kept apart.
- **LGETSTR$** of exactly 255 characters wrote a zero one byte past its result, into whatever came next in memory: an open file's record (a reader's SQL script then failed at COMMIT with "SQL: disk I/O error"), a string or an array. In 7.00 and the PicoMite too, where it shows less often.
- **WEB CONNECT** with no parameters joined OPTION WIFI's network again only after a failed connection, not when the WiFi was simply down (after a loadable program's join to another network, for example), so only CPU RESTART brought it back. It now joins whenever it is not connected, as the manual says. The PicoMite has the same fault.
- **MM.VER** is 8.000002.

## What behaves differently

- **New keyword:** BLUETOOTH. A program that uses it for its own variable must rename it.
- **With Bluetooth on** (OPTION RADIO BOTH or BLUETOOTH) the WiFi's four pins (32, 33, 38 and 40) belong to the radio, as with OPTION WIFI, and MM.INFO(HEAP) is 64KB less.
- **OPTION LIST** shows OPTION RADIO when it is not what OPTION WIFI alone gives, and OPTION AUDIO BLUETOOTH when it is set.

## Structural changes

For those who build the firmware or study its source:

- **BTstack** (BlueKitchen, at the commit of pico-sdk 2.3.1) is in Middlewares/Third_Party/btstack, unchanged (tools/port_btstack.py). The PicoMite's Bluetooth files are copied into Src/net/bt/picomite by tools/port_bt.py with four mechanical changes; the Colour Maximite 2's side is Src/net/bt/BTMite_cmm2.c, with pico-sdk's HCI transport for the CYW43439.
- **The flash:** the CYW43439's firmware is now kept compressed (141KB in place of 225KB), and unpacked by Mark Adler's puff (Middlewares/Third_Party/puff, zlib licence) when it is loaded into the chip. That made room for Bluetooth in the network's .net section (bank 2's sectors 3 to 6).
- **RAM:** the radio's data (the driver, lwIP, Mbed TLS, BTstack and Src/net) moved to the top 32KB of the D2 RAM. Bluetooth has its own 64KB heap, taken from MMBasic's SDRAM heap and kept across RUN and NEW, as the network's block is.
- **The options record** is layout 5: OPTION RADIO, OPTION AUDIO BLUETOOTH and 2KB of Bluetooth pairings (3,260 bytes). OPTION DISK LOAD also takes 8.00.00b1's layout 4 and 7.00's files.
- **Tokens:** BLUETOOTH is a new command token at the end of the command table, so no other token moved; the image format is still 7.
- **The loadable programs' services** are version 22 (Inc/cmm2img.h; the network's side is Src/net/LoaderNet.c).

## The programs

CMM2_Programs_8.00.00b2.zip is 8.00.00b1's zip with micropython.app replaced (it needs 8.00.00b2) and README.txt for this version. The licences folder adds BTstack's licence (for the firmware's Bluetooth) and micropython-lib's (for MicroPython's network modules). The other programs are unchanged and run on 8.00.00b2 as on 8.00.00b1.

## Tested

On a Generation 2 with the WiFi module, Bluetooth on and a speaker connected, and a Generation 3 with the WiFi module, with the firmware of 17 November (this release but for the three fixes of 18 November: LGETSTR$, WEB CONNECT and MM.INFO(MAX GP)); and on a Generation 1 with the WiFi module:

- **The regression** on both: the test programs, the PicoMite tests, the crash tests, the program corpus, the PicoMite's Testfiles and SP Tools (686 of 727, as before). No change from the last runs but what the WiFi module's pins, its memory and the network bring, and the expected output of the GP pin test (GP40 to GP46).
- **The RUN and Bluetooth faults fixed above** were found by this regression on the Generation 2: with the fixes, the programs that failed ran clean six times over.
- **The update from 8.00.00b1** on the Generation 2, as a user does it: 8.00.00b1 written fresh, a library and its .lib file and the options saved; 8.00.00b2 written without a full erase; the options loaded, the library kept and working, the .lib file loaded.
- **MicroPython** (the zip's micropython.app) on both: its new modules, the pins and the LED, the clock, SQLite and an HTTPS fetch; on the Generation 3 also the book's Appendix K example and a short sound through HDMI. Every one of the book's 162 example programs ran on the Generation 2 with this release's services (on a build before the RUN and Bluetooth fixes, which do not touch loadable programs).
- **Bluetooth** on the Generation 3 with the WiFi: turned on, a scan finding a speaker, the 64KB set aside, COM8's message with no device. On 16 November, with this release's Bluetooth code: a Bluetooth Classic keyboard and its touchpad, a speaker and an HM-10 serial module on COM8 (both ways) on a Generation 2, and the regression on all three generations.
- **Generation 1**, on 18 November: the regression, with the LGETSTR$ fix, the same as on 15 November but for one test now right (SP Tools 686 of 727); then this release's file: MM.INFO(MAX GP), the program corpus's WiFi programs and TFTP as on 15 November, WEB CONNECT after a failed join, SQL, and MicroPython: 159 of the book's 162 example programs in one session, the network's included (two run out of memory there: below; the last restarts the board, as it is meant to, and was run on its own). The LGETSTR$ fix also on the Generation 2: the reporter's programs and the SQL tests.

## Known problems

- **Bluetooth game controllers** have not been tried with a real controller yet.
- **Why the radio chip once gave a wrong Bluetooth buffer position** is not known; the firmware now refuses it (above), so it costs at most a packet.
- **Bluetooth and the network wait while a loadable program runs:** a Bluetooth keyboard does not type into it and a speaker plays nothing (MicroPython's network is the exception: it services the network itself).
- **The Bluetooth serial device:** a Bluetooth LE module passes on only part of a long burst at 115200 baud (the module's limit): send in pieces or more slowly.
- **MicroPython on a Generation 1** has about 1.6MB for Python. In one long session two of the book's games (ch19/05 and ch25/02) still run out of memory after the examples before them; each runs after Ctrl-D (a soft reset).
- **The WebMite's own example programs** have not all been tried yet.
- **A Generation 3 can reset when its HDMI cable is unplugged** (the cable's shield meeting the monitor's ground on the reset line). The next board has a fix.
- The known problems of 7.00.00RC6 remain, apart from the three SQL faults fixed in 8.00.00b1.

## The manuals

- **The User Manual** is for 8.00.00b2: the new chapter Bluetooth, with the BLUETOOTH command, OPTION RADIO, OPTION AUDIO BLUETOOTH, COM8 and MM.INFO$(BLUETOOTH ...) in the references; GP40 to GP46 in Using the I/O Pins; WiFi and Bluetooth in Porting Programs.
- **Colour Maximite 2 Loadable Programs** adds services 21 and 22: the clock's alarm, the network, the LED and gp_pin.
- **The App User Guide**: the MicroPython chapter (what it has now, and the book), and the zip's name.
- **The C Compiler guide** is unchanged apart from the zip's name. The 386 PC Emulator guide and the Stepper reference are unchanged.

## Licences

The firmware's own licence (LICENSE) is unchanged. Bluetooth brings **BTstack**, whose licence allows redistribution and use "solely for personal benefit and not for any commercial purpose or for monetary gain" (Raspberry Pi's licence for BTstack covers only its own chips): free for the free firmware, but anyone who sells boards with it needs a commercial licence from BlueKitchen. cyw43-driver's copy carries the same condition, as in 8.00.00b1. puff (zlib licence) unpacks the chip's firmware. Each licence is beside its code in Middlewares/Third_Party, and in the licences folder of the zip.
