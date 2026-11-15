# The PicoMite's web support on the Colour Maximite 2

*8 November 2026. A plan for porting the whole of PicoMiteWEB's networking to the CMM2, for Peter's Wi-Fi module,
on the G1, the G2 and the G3.*

*15 November: the module and SPI2 tested from BASIC on the G3, with no firmware change (see "Tested from BASIC"
below). Work goes on in branch v8-wifi, made from v8-dev.*

*Revised the same day for V8's memory decision: BASIC programs always run from SDRAM and no longer from flash. That
gives bank 2's sectors 3 to 6 (and 7, if the library leaves flash too) to the firmware. The web support is a V8
feature, on branch v8-dev, and its step 2 needs that change first.*

## The module (Peter's schematic, 8 Nov)

The module plugs onto the CMM2's rear I/O header, and passes the header through to a second header on its back. It
carries a Raspberry Pi **RM2** (RMC20452T): the Infineon **CYW43439** Wi-Fi and Bluetooth chip, as on a Pico W or
Pico 2 W.

| RM2 | CMM2 | MMBasic pin | Notes |
|---|---|---|---|
| gSPI_SCLK | PB13, SPI2's clock | 40 | 10K pull-up (R3), so the start-up's factory-reset check reads it high |
| gSPI_Data_In | PB15, SPI2's MOSI | 38 | The one data line, used in both directions |
| gSPI_Data_out | PB15, through 470 ohms (R1) | 38 | Joined to the data line, as on a Pico W |
| hIRQ | PB15, through 10K (R2) | 38 | The interrupt also appears on the data line, as on a Pico W |
| gSPICS | PI3 | 33 | |
| Wi-Fi_on, Bluetooth_on | PI8 | 32 | Both on one pin, as on a Pico W |
| VDDIO, Vin | Its own 3.3 V, from +5V | | An AMS1117-3.3: the header's 3.3 V is rated for 100 mA, and the RM2 draws 271 mA transmitting |

Notes:
- The second header passes everything through except pins 1, 3, 8 and 9 (MMBasic pins 40, 38, 33 and 32).
- PB14, SPI2's MISO (pin 35), is not used, and passes through.
- With Wi-Fi on, those four pins and SPI2 belong to it:
  - The SPI2 command, and SETPIN on pins 32, 33, 38 and 40, must refuse.
  - The PicoMite's GP10 and GP11 (CMM2 pins 40 and 38) go with them.
- The same pins exist on the G1, the G2 and the G3, so one design serves all three.

## What the PicoMite has (d:\Dropbox\PicoMite\PicoMite\net)

All of it is in `net/`: 4,426 lines in nine C files, its lwIP settings, and an mbedTLS configuration. It is built
with pico-sdk's `pico_cyw43_arch_lwip_poll`: lwIP has no operating system (NO_SYS 1), and is pumped by
`ProcessWeb()`, which the PicoMite calls from its main loop, CheckAbort, console output, file transfers and XMODEM.

| File | Lines | What it gives |
|---|---|---|
| WiFi.c | 780 | `WEB CONNECT`, `WEB SCAN`, `WEB IOVAR`; OPTION WIFI (setwifi); ProcessWeb; the dispatch for the rest |
| MMtcpserver.c | 936 | `WEB TCP INTERRUPT/READ/SEND/CLOSE`, `WEB TRANSMIT PAGE/FILE/CODE`: a web server on OPTION TCP SERVER PORT |
| MMTCPclient.c | 925 | `WEB OPEN TCP CLIENT/STREAM`, `WEB TCP CLIENT REQUEST/READ/WRITE/STREAM`, `WEB CLOSE TCP CLIENT`; TLS clients and streams (`WEB OPEN TLS CLIENT/STREAM`, `WEB TLS CA`, `WEB TLS NOVERIFY`) |
| MMMqtt.c | 386 | `WEB MQTT CONNECT/PUBLISH/SUBSCRIBE/UNSUBSCRIBE/CLOSE` |
| MMudp.c | 191 | `WEB UDP SEND/INTERRUPT`, a UDP server on OPTION UDP SERVER PORT |
| MMntp.c | 249 | `WEB NTP` |
| MMping.c | 217 | `WEB PING` |
| MMtelnet.c | 205 | The telnet console |
| MMtftp.c | 185 | A TFTP server, for files to and from the card |

Elsewhere in the PicoMite:
- **Options:** OPTION WIFI ssid$, password$ [, hostname$] [, ip$, mask$, gateway$] [, country], TCP SERVER PORT,
  UDP SERVER PORT, TELNET CONSOLE, TFTP and WEB MESSAGES.
- **Information:**
  - MM.INFO(IP ADDRESS), MM.INFO(WIFI STATUS), MM.INFO(TCPIP STATUS), MM.INFO(TCP REQUEST n) and
    MM.INFO(SSID).
  - MM.MESSAGE$, MM.ADDRESS$ and MM.TOPIC$, for MQTT and UDP.
- **The telnet console:** TelnetPutC in the console's output; its input joins the console's.
- **Already on the CMM2:** JSON$ (fun_json) was ported earlier.

The port follows the rule for PicoMite code: copy it as it is, and change only what the CMM2 makes necessary.

## The radio's driver on an STM32

- **The driver:** pico-sdk drives the CYW43439 with George Robotics' **cyw43-driver**. It is now under the **MIT**
  licence, with no limit to Raspberry Pi chips.
  - MicroPython's STM32 port already uses it, on the Pyboard-D, so it runs on STM32s.
  - The firmware blob it loads into the chip (w43439A0_7_95_49_00_combined.h) is Infineon's. The PicoMite and pico-sdk
    distribute it, but its own terms are still to be read before release.
- **The bus:** pico-sdk's `cyw43_bus_pio_spi.c` does the gSPI framing with an RP2040 PIO:
  - a 32-bit command;
  - then data written, or the line turned round and data read;
  - then a status word.

  On the CMM2 that becomes SPI2 in **half-duplex** (CFG2.COMM = 3, one data line), switching direction (CR1.HDDIR)
  after the command. The framing functions (the register reads and writes, and the byte swaps) are plain C, and are copied as
  they are. Only the transfer itself is new.
- **The clock:** SPI2 takes PLL1Q, 48 MHz on a 480 MHz board, and runs at /2: **24 MHz** (Peter, 15 Nov). That is
  about 3 MB a second, more than Wi-Fi gives a microcontroller. A 400 MHz board's PLL1Q is 40 MHz, so a G1 runs at
  20 MHz; the divider is taken from the clock actually there.
  - Faster needs another kernel clock (32 MHz from the HSI worked), but SPI1, SPI2 and SPI3 share it, and SPI3 is
    the SD card at /2: not done.
- **The chip's mode:** high-speed mode (HIGH_SPEED_MODE, as cyw43_ll.c sets it), read in **SPI mode 0**: the chip
  changes the line on the rising edge, and SPI2 samples on that edge, the chip's output delay holding the bit, as
  pico-sdk's spi_gap01_sample0 does. Read in mode 1, every word comes one bit late. Normal mode works only to
  12 MHz.
- **The interrupt:** while CS is high, the chip pulls the data line (through R2) to ask for attention. That is read
  from PB15's input register: in poll mode the driver asks for it, and nothing needs an EXTI.
- **The port layer:**
  - `cyw43_configport.h` for the CMM2: delays from the firmware's timers, WL_ON on PI8, no RTOS.
  - A small `cyw43_arch` shim, with the calls the PicoMite's `net/*.c` makes: init and deinit, STA mode,
    wifi_connect_timeout_ms, poll, and lwip_begin and lwip_end, which are empty in poll mode. With it, those files
    compile unchanged.

## Tested from BASIC (15 November, on the G3)

Three programs in tests/wifi, run with the module on the G3's header and no firmware change:

- **wifitest.bas,** gSPI bit-banged with PIN():
  - the test register read 0xFEEDBEAD (on the second try after power-up: the driver allows ten);
  - the read/write test register kept two patterns;
  - the chip went to 32-bit words and still read 0xFEEDBEAD;
  - the backplane gave the ALP clock and ChipCommon's ID, 0x1545A9AF: a 43439, revision 5.
  - The old response-delay byte (0x0001) reads 0 on this chip. The F1 delay at 0x001D, which cyw43_ll.c sets, works.
- **wifispi.bas,** SPI2 in half-duplex, set up by SPI2 OPEN and POKEs (COMM = 3, HDDIR, TSIZE and CSTART):
  - each pass from a fresh power-up: 200 random patterns written and read back, 200 test-register reads and the chip
    ID, at 0.75 to 24 MHz, with the pins' drive speed low (as SPI2 OPEN sets it) and very high;
  - normal mode: no errors to 12 MHz; at 24 MHz 127 to 400 errors in 600;
  - high-speed mode read in SPI mode 0: no errors at any speed, and none in 15,000 more transactions at 24 MHz;
  - high-speed mode read in mode 1: every word one bit late (0xFDDB7D5A for 0xFEEDBEAD).
- **wificlk.bas,** above 24 MHz, with SPI1-3's kernel clock switched for each pass: no errors in 9,000 transactions
  at 30 MHz (PLL2P) or 32 MHz (the HSI's per_ck). The firmware stays at 24 MHz (Peter).

For comparison, the PicoMite sets the radio's PIO divider to CPU / 80 MHz, rounded up and at least 2, and takes two
PIO cycles a bit: 33.3 MHz at its default 200 MHz, at most 40 MHz.

## Flash

The sizes come from a PicoMiteWEB build's map (d:\Dropbox\PicoMite\PicoMite\build\PicoMite.elf.map): Cortex-M33
code, of much the same density as the M7's. The CMM2's own build may differ, so step 1 measures it.

| Part | Size |
|---|---|
| The CYW43439's firmware blob | 235 KB |
| mbedTLS, for the TLS client | 175 KB |
| lwIP | 50 KB |
| cyw43-driver | 25 KB |
| The PicoMite's `net/*.c` | 24 KB |
| The CMM2's bus and port layer | about 10 KB, estimated |
| **Total with TLS and the blob** | **about 520 KB** |
| **Code without TLS or the blob** | **about 110 KB** |

What the CMM2's flash will have free in V8 (today's figures from build/Release, 8 Nov):

| Part of the flash | Free |
|---|---|
| Bank 1, 1 MB | Under 1 KB |
| Bank 2 sectors 0 and 1, the firmware's (.bank2) | 85 KB |
| Bank 2 sector 2, after the options and mmbrun (.xip) | 32 KB |
| Bank 2 sectors 3 to 6, the program's until V7 | 512 KB |
| Bank 2 sector 7, the library | 128 KB, if the library leaves flash too |

**Everything goes in flash, TLS and the blob included,** as on the PicoMite:
- **The library stays in sector 7 for now (Peter, 8 Nov):** the 512 KB of sectors 3 to 6 is just short of the
  stack's 520 KB. The
  PicoMite's `net/*.c` and the bus layer (about 34 KB) therefore go into .bank2, and the rest into sectors 3 to 6.
  About 110 KB is left, in three pieces: 51 KB in .bank2, 32 KB in sector 2, and 27 KB in sectors 3 to 6.
- **If the library leaves flash later:** sectors 3 to 7 make 640 KB in one piece, which holds the whole stack with about
  120 KB to spare. The 117 KB in .bank2 and sector 2 stays free as well.
- **The linker:** FLASH2, the program's region now, becomes a code region with its own section. That section takes the
  network's objects by file name, as .bank2 takes its objects now.
- **Flash writes:** while sector 2 (the options) is erased or written, reads from bank 2 wait. The network code then
  waits too, as .bank2's code does now. None of it runs from an interrupt.

What V7's plan weighed, and V8 settles:
- **Layouts:** layouts A to D, which gave up OPTION FLASH sectors, go.
- **TLS:** TLS is no longer in question.
- **The blob:** it is in flash, so Wi-Fi does not need the card.
- **-Os:** compiling the network code with -Os is not needed.
- **Owners without the module:** they pay only in space that V8's other features could use. Those features are small:
  the G3's MS7210 driver and HDMI sound are a few KB each.

## RAM

- **lwIP's memory:** the PicoMite sets MEM_SIZE 40960, PBUF_POOL_SIZE 32, TCP_MSS 1460, and send and receive windows
  of 8 x MSS. That is roughly 150 KB in all, plus about 40 to 60 KB for each TLS session.
- **Not in internal RAM:** none of it fits in the chip's own RAM:
  - The DTCM has only about 14 KB spare.
  - The AXI SRAM is display page 0's frame buffer.
  - The D2 RAM holds the SDRAM heap's map, mmbrun's RAM, the GUI controls and the internal heap.
- **Not in MMBasic's heap:** the first draft of this plan put it there, which would not work.
  - The heap is wiped at every RUN and NEW (InitHeap and HeapWipe, Memory.c).
  - The Wi-Fi connection, the server's listening port and the telnet console must outlive both. On the PicoMite they
    do, because lwIP's memory there is static.
- **A network block in SDRAM:**
  - About 256 KB, taken at start-up only when OPTION WIFI is set, outside what HeapWipe clears. lwIP's heap and pools
    (MEM_LIBC_MALLOC and MEMP_MEM_MALLOC) and mbedTLS's allocations come from it.
  - Without OPTION WIFI nothing is taken.
  - It is still a new SDRAM area, if only on boards using Wi-Fi, so it is Peter's decision (the SDRAM is kept for the
    compiler).
- **What stays in the heap:** what the PicoMite gives each program stays in the heap, and is closed at RUN and NEW by
  the PicoMite's own clean-up. That is the TCP buffers it takes with GetMemory.
- **With V8's program block:**
  - **A G2** is unchanged: 512 KB at 0xD0700000 is reserved for OPTION RAM already.
  - **A G1** loses 512 KB of heap to the program block: SDRAMBASE moves from 0xD0300000 to 0xD0380000, as OPTION RAM
    does now. A G1's heap of about 5 MB becomes about 4.5 MB, and about 4.25 MB with Wi-Fi on.
- **Loadable programs on a G1:** on a G1 the loader lends the heap to a loadable program. The network block must lie
  outside what it lends, so that Wi-Fi comes back when the program ends.

## How it runs inside the firmware

- **Pumping:**
  - ProcessWeb() is called where the PicoMite calls it: console output, file loops, XMODEM and ZMODEM, waits and
    the prompt.
  - In the interpreter it runs from routinechecks, gated by a flag that the 1 ms tick sets, so it costs nothing
    between ticks. The touch screen's lesson applies: nothing per statement, as a check on every statement once made
    programs 2 to 2.7 times slower.
  - lwIP's timers come from the firmware's millisecond count (sys_now).
- **BASIC interrupts:** the TCP, UDP and MQTT interrupts go through the CMM2's existing interrupt dispatch, as the
  PicoMite's do.
- **The telnet console:** a third console path, beside the serial port and the screen and keyboard.
  - Output goes in at MMputchar's level, as TelnetPutC does on the PicoMite.
  - Input goes into the console's buffer.
  - OPTION TELNET CONSOLE ON, ONLY and OFF work as on the PicoMite.
- **Files:** TFTP and WEB TRANSMIT FILE use the CMM2's FatFs, on A: and the USB drive.
- **The time:** WEB NTP sets the CMM2's clock, and the DS3231 too if one is fitted.
- **Start-up:**
  - With OPTION WIFI set, Wi-Fi starts at power-up, as on the PicoMite, without holding up the prompt.
  - A missing module answers no gSPI test read (0xFEEDBEAD): the firmware says "WiFi module not found" and carries
    on.
  - The blob comes from flash, so Wi-Fi itself needs no card.
  - A program that starts at power-up, such as a web server, comes from the card in V8, as every program does. The
    PicoMite runs it from flash.
- **Loadable programs:** while one runs, ProcessWeb is not called, so connections may time out. A network service
  for programs could come later, for MicroPython and the C compiler.

## The options

- The PicoMite's fields are added to `option_s`: SSID, password, hostname, the IP address, mask and gateway, the TCP
  and UDP ports and their response times, telnet, TFTP and the country code. That is about 260 bytes.
- The options use 992 bytes of their 4 KB (SAVED_OPTIONS_SIZE), so there is room.
- OPTION LIST shows the PicoMite's lines, and the password is not printed.

## Steps

### Step 0: Peter's decisions

- **The library:** decided. It stays in sector 7 for now (Peter, 8 Nov), so the network code is split between
  sectors 3 to 6 and .bank2.
- **The network block:** still open. This is the SDRAM block of about 256 KB for the network, taken only when OPTION
  WIFI is set.

Before release, the blob's own terms (Infineon's) are also to be read. That is a check rather than a decision.

### Step 1: from a loadable test program, without reflashing

Before the firmware changes, a loadable program builds cyw43-driver and lwIP with the bus layer. A program has 8 MB
of SDRAM, so flash does not limit it. It:
1. Reads the gSPI test register (0xFEEDBEAD) at 24 MHz. (Done from BASIC on 15 Nov, below; the program does it
   again in C, as its first check.)
2. Loads the blob, built into the program.
3. Joins Peter's Wi-Fi, takes an address by DHCP, and answers a ping from the PC.
4. Times a transfer to the PC.
5. Gives, from its map, the stack's sizes with the CMM2's compiler flags, which fix where each part goes in flash.

**Done when:**
- Pings work both ways.
- The throughput is measured, on a G2 and on a G1.
- The sizes are known.

### Step 2: into the firmware

- **First, V8's memory change:** programs move to SDRAM, which frees sectors 3 to 6 (or 3 to 7).
- **The pieces:** the bus and port layer, lwIP, mbedTLS, cyw43-driver, the blob, and the PicoMite's `net/*.c`. They
  are copied as they are, and placed in sectors 3 to 6 (or 3 to 7) and .bank2, as the Flash section sets out.
- **The network block:** the SDRAM block, and the loader on a G1 kept out of it.
- **The firmware's own changes:** the options, the ProcessWeb calls, the telnet console's paths, the pins reserved,
  and MM.INFO and MM.MESSAGE$ and their kind.

### Step 3: each feature, tested from the PC (Python scripts as the other end)

1. The TCP server:
   - The PC fetches a page by HTTP.
   - WEB TRANSMIT PAGE, FILE and CODE.
   - Several clients at once.
2. The TCP client: a page from the PC's own web server, with JSON$ on the reply.
3. UDP, in both directions, with the interrupt.
4. WEB NTP and WEB PING.
5. MQTT, with Mosquitto on the PC: publish, subscribe, the interrupt, MM.TOPIC$ and MM.MESSAGE$.
6. TFTP: put and get from the PC's TFTP client, onto the card.
7. The telnet console: the prompt, LIST, EDIT and a running program, from the PC's telnet.
8. TLS: an HTTPS GET from a public site, with the certificate verified, and with NOVERIFY.
9. The PicoMite's own example web programs, unchanged.

### Step 4: what must not change

- Without the module and without OPTION WIFI, everything must be as before: the goldens on the G1 and the G2, and the
  start-up time.
- With the module plugged in but Wi-Fi off, the passed-through pins must work as before.

### Step 5: the manual

A networking chapter, from the PicoMite's WebMite documentation, with the module's pins and its limits on the CMM2.

## Later

- **Bluetooth:** the RM2 has it, on the same chip. A Bluetooth keyboard, mouse or gamepad would come from the
  PicoMite's own Bluetooth work (d:\Dropbox\PicoMite\Bluetooth_Classic_Keyboard_Plan.md).
- **Networking for loadable programs:** sockets for MicroPython and the C compiler, through new services.
- **DMA for SPI2,** if the polled transfers ever show.

## Risks

- **The gSPI at speed:** retired 15 Nov. No errors at 24 MHz (and up to 32 MHz) through the header and the 470-ohm
  resistor.
- **The H743's SPI in half-duplex:** retired 15 Nov. It turns round after the command, in high-speed mode read in
  SPI mode 0.
- **Flash:**
  - After the stack, about 110 KB is left, or about 240 KB if the library leaves flash.
  - The CMM2's build may come out larger than the PicoMite's figures. Step 1 measures it before anything is placed.
- **A web server at power-up:** in V8 it needs the card, as every program that starts at power-up does. Wi-Fi itself
  does not need the card.
- **The network block and the loader:** on a G1, a loadable program borrowing the heap must not overwrite the block.
  If it did, Wi-Fi would fail after the program ends rather than during it.
- **Radio range:** the module sits behind the CMM2. A metal case would cut its range.
