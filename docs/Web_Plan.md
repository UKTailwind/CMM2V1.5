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
- **Measured in step 1 (15 Nov, wifi.app on the G3):** read and write only; nothing runs from it (the code runs in place
  from flash), so the block stays execute-never, as the heap is. It is cacheable: the transfers are copied by the CPU.
  - **Static data, about 57 KB:** lwIP's packet pool 49 KB (32 x 1,532 bytes), the driver's cyw43_state 2.4 KB, and
    lwIP's smaller pools and variables about 6 KB. The DTCM, where the firmware's .bss goes, has about 13.5 KB free, so
    this has to move: into the block with MEMP_MEM_MALLOC (the pools from the heap), or a section placed there.
  - **Heap at its peak, about 31 KB,** with one TLS session (example.com, 2 MB from Cloudflare, google.com), lwIP's
    own allocations included.
  - **Stack, about 2.1 KB** deeper than the program's main loop during a TLS handshake (ECDSA certificates; an RSA
    chain with verification is still to be measured). The firmware's DTCM stack has room.
  - So about 90 KB in use: 128 KB would hold one TLS session and the server's connections with room to spare, and
    256 KB is generous.
  - **DMA later:** SPI2's DMA (DMA1 or 2) can reach the SDRAM but not the DTCM, so data in the block suits it, with the
    cache cleaned and invalidated around each transfer.
- **Not in internal RAM:** none of it fits in the chip's own RAM:
  - The DTCM has only about 14 KB spare.
  - The AXI SRAM is display page 0's frame buffer.
  - The D2 RAM holds the SDRAM heap's map, mmbrun's RAM, the GUI controls and the internal heap.
- **Not in MMBasic's heap:** the first draft of this plan put it there, which would not work.
  - The heap is wiped at every RUN and NEW (InitHeap and HeapWipe, Memory.c).
  - The Wi-Fi connection, the server's listening port and the telnet console must outlive both. On the PicoMite they
    do, because lwIP's memory there is static.
- **A network block in SDRAM (decided by Peter, 15 Nov):** taken at start-up only when OPTION WIFI is set, outside
  what HeapWipe clears. lwIP's heap and pools (MEM_LIBC_MALLOC and MEMP_MEM_MALLOC) and mbedTLS's allocations come from
  it. Without OPTION WIFI nothing is taken. Read and write, execute-never; nothing of the network runs from RAM, and it
  never uses the executable window.
  - **A G2 or G3: 256 KB of MMBasic's heap,** of its 15 MB.
  - **A G1: 128 KB at the top of the display memory,** 0xD02E0000 to 0xD02FFFFF, as the heap and everything above it
    were tuned for the apps. pagegen's limit comes down to 0xD02E0000, so 42 of the G1's 80 modes and depths lose a
    page (measured with LIST PAGES on the G3 for every mode, against the G1's limit). The ones that matter: MODE 9,16
    (1024x768) goes from 2 pages to 1; 14,12 and 14,16 (960x540) from 3 to 2; 8,12 and 8,16 from 5 to 4; 9,8 and 12,8
    from 4 to 3; 18,8 from 5 to 4. The PicoMite modes (x,4 and x,1) keep at least 11 pages of the 5 they need.
  - **A G1's loadable programs built --vram** (13 of them: apple2, atari800, bbc, c64, cpc, doom, heretic, nes, pc386,
    scummvm, spectrum, trs80, wordgrinder) are still lent the whole 3 MB, block included, so they lose nothing. Wi-Fi
    is not serviced while a program runs anyway; when such a program ends, the firmware starts Wi-Fi again (a rejoin of
    about 3 s), as it puts BASIC's screen back.
  - **To change with it on a G1:** pagegen's limit and Draw.c's page-address checks (0xD0300000), RUN's clearing of
    the display memory (Commands.c, which would wipe the block at every RUN), and the loader's give-back (Loader.c
    ld_giveback: the Wi-Fi restart).
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
- **The network block:** decided (Peter, 15 Nov): 256 KB of the heap on a G2 or G3; 128 KB at the top of the display
  memory on a G1, lent with the rest to --vram programs and Wi-Fi restarted after them (the RAM section).

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

**Results, 15 November, on the G3** (apps/wifi: `wifi.app` and `pctest.py`; a G3 is a G2 with HDMI, so this is the
G2's result too):
- **The program:** cyw43-driver and lwIP 2.2.1 at the commits the PicoMite's pico-sdk has, and pico-sdk's bus file
  with its framing as it is. Only the transfer is new (SPI2, 32-bit frames, each word's bytes reversed), with a
  polled stand-in for pico-sdk's async context, and the cyw43_arch calls that the PicoMite's net code makes.
- **The bus:** 30,000 register transfers at 24 MHz, no errors, 247,000 a second.
- **The radio:** firmware 7.95.49 loaded in 744 ms; the MAC address, 28:CD:C1:19:59:56, from the chip's OTP; the scan
  hears TOASTY2 in 0.7 s.
- **The network:** TOASTY2 joined in 3.1 s (6.5 s the first time), an address by DHCP, and the gateway pinged in
  1.5 to 1.8 ms. From the PC, pings take 1 to 6 ms with power saving off, as the PicoMite's WEB CONNECT sets it
  (7 to 309 ms with it on).
- **The throughput,** 10 s each way: **2.1 MB/s** from the CMM2 and **2.36 MB/s** to it, both ends agreeing, with no
  bus errors.
- **TLS** (`wifi.app https host path`: lwIP's altcp_tls, pico-sdk's copy of altcp_tls_mbedtls.c for mbedTLS 3 and
  mbedTLS 3.6.6 with the PicoMite's net/mbedtls_config.h, all as the PicoMite builds them; the RNG for entropy):
  - example.com: 200 OK over TLS 1.2, ECDHE-ECDSA with ChaCha20-Poly1305, the handshake in 1.1 to 1.6 s;
  - 2 MB from speed.cloudflare.com at **1.13 MB/s**, decrypted on the CMM2.
  - **The window:** with the PicoMite's TCP_WND (8 x TCP_MSS, 11,680 bytes) the download stopped for good after
    220 KB. altcp_tls_mbedtls.c opens the window again only for decrypted data, and mbedTLS decrypts only whole
    records, up to 16,413 bytes on the wire: once the server moved from its small opening records to full-size ones,
    no record could ever arrive. The CMM2's lwipopts.h sets 16 x TCP_MSS (23,360); the PicoMite's copy is unchanged.
    A PicoMite should stall the same way on a large HTTPS download (by reading, not yet tried on one): filed as
    PicoMite issue #153.
- **The sizes** with the CMM2's flags (code and constant data):

  | Part | Bytes |
  |---|---|
  | The blob (Wi-Fi only) | 225,240 |
  | mbedTLS | 165,949 |
  | lwIP | 48,306 |
  | cyw43-driver | 11,087 |
  | TLS glue (altcp_tls, and the CMM2's entropy, time and allocator) | 4,464 |
  | The bus and port layer | 4,307 |
  | **Total** | **459,353** |

  The PicoMite's net/*.c (about 24 KB on the PicoMite) and lwIP's MQTT and TFTP apps come on top in step 2. The
  stack is about 60 KB under the plan's estimate of 520 KB.
- **Still to do:** the G1's run (the module moved over; its SPI2 runs at 20 MHz), deferred (Peter, 15 Nov).

### Step 2: into the firmware

- **First, V8's memory change:** programs move to SDRAM, which frees sectors 3 to 6 (or 3 to 7). Done in v8-dev:
  sectors 3 to 6 are the firmware's and empty (flash.c).
- **The pieces:** the bus and port layer, lwIP, mbedTLS, cyw43-driver, the blob, and the PicoMite's `net/*.c`. They
  are copied as they are, and placed in sectors 3 to 6 (or 3 to 7) and .bank2, as the Flash section sets out.
- **The network block:** the SDRAM block, and the loader on a G1 kept out of it.
- **The firmware's own changes:** the options, the ProcessWeb calls, the telnet console's paths, the pins reserved,
  and MM.INFO and MM.MESSAGE$ and their kind.

In this order (15 Nov), each built and checked on the G3 before the next:
1. **The libraries into the tree:** cyw43-driver, lwIP and mbedTLS at step 1's commits, only the files built (every
   `.c` under Middlewares is compiled), with their licences, in Middlewares/Third_Party; the CMM2 port files from
   apps/wifi/port. Their defines (CYW43_LWIP, MBEDTLS_CONFIG_FILE, the TLS input length) for those files only.
2. **The linker:** FLASH2 split into sectors 3 to 6 (a `.net` code section taking the blob, mbedTLS, lwIP and the
   driver by file name, as .bank2 takes its files) and sector 7 (the library). The flash writer refuses sectors 3 to 6,
   as it refuses .bank2's.
3. **The network block and its allocator:** lwIP's own heap (mem.c, MEM_LIBC_MALLOC off) laid over the block
   (LWIP_RAM_HEAP_POINTER), with the pools from it (MEMP_MEM_MALLOC) and mbedTLS's allocations through it
   (CallocMemory and FreeMemory as mem_calloc and mem_free). The firmware's C library heap is not the block, so this
   one change from the PicoMite's MEM_LIBC_MALLOC is the CMM2's. G2 and G3 first; the G1's display-memory changes
   with the G1's run.
4. **The poll:** ProcessWeb from routinechecks on the 1 ms flag, and **never while bank 2 is being erased or written**:
   the options' erase loop calls routinechecks (flash.c), the network code is in bank 2, and a fetch from bank 2
   stalls until the erase is done, which the linker script's rule for bank 2 forbids. A flag tested in bank 1, before
   the call, skips it.
5. **First firmware milestone:** OPTION WIFI and WEB CONNECT, WEB SCAN, MM.INFO(IP ADDRESS) and WEB PING at the prompt,
   from the PicoMite's WiFi.c and MMping.c as they are.
6. **The rest of net/*.c** (TCP server and client, TLS, UDP, NTP, MQTT, telnet, TFTP), its interrupts, MM.MESSAGE$,
   MM.ADDRESS$ and MM.TOPIC$; the pins and SPI2 reserved; Wi-Fi at power-up.
7. **The G1:** the block at the top of the display memory, pagegen's limit, RUN's clearing, and Wi-Fi started again
   after a --vram program.

**Progress, 15 November (v8-wifi, f7e4dde and after), on the G3:**
- Stages 1 to 6 are in: the libraries in Middlewares/Third_Party, .net in sectors 3 to 6 (433 KB, 89 KB free), the
  PicoMite's net code copied by tools/port_net.py (three mechanical changes) with Inc/net/WebMite_cmm2.h, and the
  CMM2's side in Src/net/WebMite_cmm2.c. Bank 1 has 1.1 KB left; the network takes 5.2 KB of the DTCM.
- **The block is 128 KB on every board,** not 256 KB on a G2 or G3: mbedTLS takes MMBasic's heap as on the PicoMite
  (CallocMemory), so the block holds lwIP alone, and lwIP's MEM_SIZE has to be one number at compile time.
- **Working:** OPTION WIFI (joins at start-up in about 7 s), WEB SCAN, WEB PING (by name too), WEB NTP (sets the
  CMM2's clock and RTC), MM.INFO(IP ADDRESS) and the rest, OPTION TCP SERVER PORT with tests/wifi/webserver.bas
  (WEB TCP INTERRUPT, WEB TCP READ, WEB TRANSMIT PAGE: 40 of 40 pages over two RUNs), the telnet console
  (tests/wifi/pctelnet.py), and WEB OPEN TLS CLIENT with WEB TCP CLIENT REQUEST (example.com over HTTPS).
- **Fixed on the way:** SPI2Close took SPI2 from Wi-Fi at every RUN (the four pins are boot-reserved now and it leaves
  them); the PicoMite's checksent timed out ("LWIP send data timeout") until CheckAbort polled the network at every
  call, as the PicoMite's does.
- **Without the module (15 Nov, 697899c):**
  - No OPTION WIFI (every board without it): nothing starts; the restart is about 3 s, the heap is 128 KB larger than
    with Wi-Fi, pins 32, 33, 38 and 40 are Unused, and SETPIN and SPI2 OPEN work on them; WEB commands say "WiFi radio
    not running", MM.INFO(IP ADDRESS) is 0.0.0.0.
  - OPTION WIFI set, module unplugged (Peter unplugged it): "WiFi module not found" once at start-up and on to the
    prompt; the block back in the heap (the same size as without OPTION WIFI), the four pins Unused and SPI2 free,
    the WEB commands "WiFi radio not running"; the options are kept for when the module is back. The try costs well
    under a second, and drives the four pins briefly at start-up.
- **The host name:** OPTION WIFI's default is "PICO" and the board's ID (PICO0131697F on this G3), as on the PicoMite.
- **Still to do:** UDP, MQTT and TFTP from the PC; step 4's regression; the G1 (stage 7); the manual.

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
