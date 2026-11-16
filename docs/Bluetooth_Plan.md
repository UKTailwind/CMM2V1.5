# Bluetooth on the Colour Maximite 2 (V8)

*16 November 2026. Peter: Bluetooth is the last part of the V8 firmware, apart from possibly more stand-alone apps.
Nothing is built yet. This plan says what there is to port, what it costs, and the decisions needed first.*

## What the PicoMite has

The PicoMite's PC3 build (the Pico Computer 3: WiFi and a Bluetooth host on the same CYW43439, the RM2's chip) has
the whole of it, on BTstack (pico-sdk's lib/btstack). The rule for PicoMite code holds: copy it as it is, and change
only what the Colour Maximite 2 makes necessary.

| File | Lines | What it does |
|---|---|---|
| bluetooth/BTKeyboard.c | 3,024 | The BLE HID host: keyboards, mice and game controllers, paired by themselves; reports go into the USB HID path (process_kbd_report). |
| bluetooth/BTClassicKeyboard.c | 816 | Bluetooth Classic keyboards (HID host), remembered and reconnected. |
| bluetooth/BTSerial.c | 1,046 | A serial device as a COM port: Classic (the Serial Port Profile, a GPS receiver, say) or BLE (HM-10 and Nordic UART). |
| bluetooth/BTAudio.c | 1,773 | Speakers and headphones (A2DP source, SBC): OPTION AUDIO BLUETOOTH. |
| bluetooth/btstack_config.h | 156 | BTstack's settings. |
| bluetooth/BTConsole.c | 664 | A BLE console (the PicoMite as a peripheral): in the BT build only, not the PC3's. Left out. |

What users see (the PicoMite's help):

- BLUETOOTH CONNECT [SPEAKER | SERIAL | KEYBOARD] name$ | addr$ [, pin$], BLUETOOTH DISCONNECT [SERIAL], BLUETOOTH
  FORGET [addr$], BLUETOOTH SCAN [seconds], BLUETOOTH STATUS [#n].
- OPTION AUDIO BLUETOOTH [idle]; OPTION RADIO BOTH | WEB | BLUETOOTH | NONE (the PC3's choice of the chip's halves).
- MM.INFO$(BLUETOOTH KEYBOARD | SPEAKER | SERIAL); a Bluetooth mouse for GUI CURSOR LINK MOUSE.
- Pairings kept over restarts; OPTION RESET forgets them.

Its hooks into the rest of the PicoMite (lines that name Bluetooth): MM_Misc.c 69, PicoMite.c 46, FileIO.c 23,
Serial.c 22, WiFi.c 18, SDCard.c 17, KeyboardMap.c 15, Pointer.c 6, Audio.c 4.

## The pieces on the Colour Maximite 2

- **The radio:** the same chip on the same bus. Bluetooth reaches the chip over the gSPI bus as well, through its
  shared memory (pico-sdk's cybt_shared_bus), so the module needs no more pins and the CMM2's SPI2 half-duplex bus
  (Src/net/cyw43_bus_cmm2.c) carries it.
- **The chip's firmware:** the WiFi firmware becomes the combined WiFi and Bluetooth one (wb43439A0_7_95_49_00, 232
  KB, 7 KB more than now), and the Bluetooth patch is added (cyw43_btfw_43439.h, 7 KB).
- **pico-sdk's glue** (BSD-3-Clause): cybt_shared_bus.c, cybt_shared_bus_driver.c, btstack_hci_transport_cyw43.c,
  btstack_cyw43.c and btstack_chipset_cyw43.c, about 1,400 lines; copied, with the RP2040 calls replaced as in the
  WiFi port.
- **BTstack** at the commit pico-sdk 2.3.1 has, unchanged, as lwIP and Mbed TLS are.
- **The run loop:** the PicoMite pumps BTstack from ProcessBT(), called where ProcessWeb() is. The CMM2's WebPoll()
  is the same place, so ProcessBT() goes beside it.
- **The pairings:** the PicoMite keeps them in its options (Option.bt_tlv, 2 KB), saved with them; the CMM2 does the
  same (step 1).
- **Memory:** the PicoMite's Bluetooth takes about 24 KB of static RAM. The CMM2's DTCM has about 8 KB left, so
  the radio's data and bss went to the D2 RAM (step 3), grouped by the linker so that a Generation 1 can start it
  again after a --vram program.
- **Keyboards:** the CMM2's KeyboardMap.c is the PicoMite's, process_kbd_report included, so BTKeyboard.c feeds it
  unchanged. Mice go to the CMM2's mouse and the GUI cursor, game controllers to DEVICE(GAMEPAD).
- **The serial port:** the PicoMite's Bluetooth serial device is COM7, but the CMM2's COM4 to COM7 are USB serial
  devices, so it needs another number (COM8).
- **Speakers:** the CMM2's sound is its own, not the PicoMite's Audio.c. BTAudio.c takes the 44.1 kHz stream where
  HdmiAudio.c takes it on a G3 (16-bit samples), and encodes it as SBC on the M7.
- **Loadable programs:** Bluetooth stops while a .app runs, as the network does, so a Bluetooth keyboard does not
  type into a program such as Doom. Pumping it from the services' keyboard poll could come later.

## Flash: about 170 to 200 KB needed, about 164 KB free

What the PicoMite PC3's build uses (buildRP2350L's map, 13 Nov, at -O2):

| Part | Flash | Static RAM |
|---|---|---|
| BTstack | 146,825 | 16,548 |
| bluetooth/*.c | 30,582 | 6,729 |
| pico-sdk's glue | 9,991 | 1,134 |
| The chip's firmware, more than now | about 14,000 | |
| **Total** | **about 201,000** | **about 24,400** |

The CMM2 builds C at -Os, which may take 15 to 25% off BTstack, so about 170 KB.

What the CMM2 has free (8.00.00b1): bank 2 sectors 3 to 6 (.net) about 80 KB; .bank2 about 37 KB; sector 2 after
mmbrun about 29 KB; bank 1 about 18 KB. About 164 KB in all, in four pieces. That is not enough, so room has to be
made. Three ways:

1. **The library out of sector 7** (128 KB): the library becomes a file on the SD card, read into RAM at start-up,
   as LIBRARY LOAD ... RAM does now. LIBRARY SAVE, LOAD, DELETE and the rest stay as they are. The firmware then
   writes no flash but its options. The cost: the library needs the card, as AUTORUN already does in V8.
2. **mmbrun out of flash** (95 KB, in sector 2): the C compiler's runtime is loaded from A:/cc when a translated
   program runs, into the executable SDRAM window that CSUBs and USER modules share (512 KB).
3. **No speakers:** BTAudio.c with A2DP, AVDTP and the SBC encoder is about 40 KB.

**Recommended: 1.** It leaves room for Bluetooth and for whatever comes after it, and finishes V8's move of
everything but the options out of the flash.

## Licences

- **BTstack** (BlueKitchen): redistribution and use "solely for personal benefit and not for any commercial purpose
  or for monetary gain". Raspberry Pi's licence for BTstack covers its own chips only, so on the CMM2's STM32 the
  plain licence applies: free for the free firmware, but not for anyone who sells boards with it. A commercial
  licence is BlueKitchen's to give.
- **cyw43-driver:** the copy the firmware has carries the same condition (upstream moved to MIT on 2 Sep 2026).
- **pico-sdk's glue:** BSD-3-Clause. **Infineon's firmware** for the chip: as for the WiFi.
- **The alternative,** Apache NimBLE (Apache-2.0), does BLE only: no speakers, no Classic keyboards, no Classic
  serial devices. The PicoMite's code is written for BTstack, so it would be a rewrite rather than a port. Not
  recommended.

## Steps

1. **A test program** (bt.app, as wifi.app was for the WiFi): the radio started with the combined firmware, the
   Bluetooth patch sent over the shared bus, BTstack's HCI running, and an inquiry that lists the devices nearby.
   It measures BTstack's size at -Os on the M7, and needs no firmware change.
2. **A keyboard from the test program:** a BLE keyboard and a Classic keyboard typing.
3. **Into the firmware:** the room made (the decision above); BTstack, the glue and the PicoMite's bluetooth/*.c by a
   tools/port_bt.py, as port_net.py did for the network; ProcessBT(); the pairings file; the radio's option; the
   BLUETOOTH commands and MM.INFO.
4. **The rest:** mice and game controllers, the serial port, speakers.
5. **The three generations:** the G1's memory and its --vram programs, the G2 and the G3, WiFi and Bluetooth
   together; the regression; the manual (the PicoMite's chapter Bluetooth Versions) and HELP.

## Peter's decisions (16 November)

1. **BTstack**, with its non-commercial condition, as for cyw43-driver.
2. **The room in flash:** Peter asked first for the optimisation level of the network code. It was already -Os
   throughout (lwIP, Mbed TLS, the driver, the PicoMite's network files and the CMM2's port), and -Oz gives a
   byte-for-byte identical .net, so the compiler has nothing more to give. The room comes instead from the chip's
   firmware, kept compressed: done (step 0 below), 82 KB freed, so neither the library nor mmbrun moves.
3. **What to include:** BLE keyboards, mice and game controllers; Classic keyboards; a serial device; and, decided again
   on 16 November, **speakers too**: the PicoMite's BLUETOOTH command and its Classic set-up live in BTAudio.c with the
   speaker code, so it is ported whole (about 40 KB, which the compressed firmware made room for), and OPTION AUDIO
   BLUETOOTH takes the CMM2's sound where HdmiAudio.c takes it on a G3.
4. **Turning it on:** the PC3's OPTION RADIO BOTH | WEB | BLUETOOTH | NONE, ported, with NONE the CMM2's default.
5. The serial device: COM8 (COM4 to COM7 are the USB serial devices).

## Step 0: room in the flash (16 November, done)

- The CYW43439's firmware is kept as raw deflate: tools/fwpack.py makes Inc/net/cyw43_fw_z.h from the driver's blob
  files, which stay as they are. The driver's own CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE is Inc/net/cyw43_fw_cmm2.h,
  whose fw_data inflates it (Src/net/cyw43_fw_cmm2.c, with zlib's puff, Middlewares/Third_Party/puff, zlib licence)
  into MMBasic's heap and checks its CRC-32; WebStart gives the memory back once the chip has its firmware and CLM.
  The driver's source is unchanged.
- WiFi firmware 225,240 bytes stored as 141,088. .net 432,432 -> 350,244 bytes; the firmware file 1,874,224 ->
  1,792,036.
- Checked on the G2 (COM10): the first start, OPTION DISK LOAD, joined, WEB NTP, WEB SCAN, WEB PING to the router and
  8.8.8.8; MM.INFO(HEAP) as before.
- With Bluetooth the combined blob (wb43439A0_7_95_49_00_combined.h, 232,408 bytes) packs to about 146 KB.

Free flash now: .net about 162 KB, .bank2 about 37 KB, bank 1 about 17 KB (sector 2's tail is left alone: it is
rewritten with the options). Bluetooth without speakers needs about 120 to 140 KB, so it fits in .net.

## Step 1: bt.app (16 November, done)

apps/bt/make.py builds bt.app: BTstack at pico-sdk 2.3.1's commit (eb0bb8b, fetched into apps/bt/btstack), pico-sdk's
Bluetooth glue copied as it is (port/: the CYW43 HCI transport, the chipset, cybt_shared_bus), the PicoMite's
btstack_config.h as it is (built as PICOMITEBTH), and apps/wifi's bus, pins and poll. `RUN "bt.app", scan [seconds]`
starts Bluetooth and runs a Classic inquiry and an LE scan together, then asks the Classic devices without a name for it.

- **On the G2 (COM10):** the combined firmware (7.95.61) and the Bluetooth patch (CYW4343A2_001.003.016.0065) loaded
  over SPI2, BTstack working in 0.97 s, 0 bus errors. Found Classic devices (two audio devices, Peter's "Bluetooth 3.0
  Keyboard", marked Keyboard by its class of device) with signal strength and names, and LE devices with the names
  their advertisements give.
- **The run loop:** BTstack's embedded run loop polls its data sources on every pass. The CYW43 transport's read wakes
  the chip's Bluetooth core and toggles its interrupt each time, and done as fast as the CMM2 loops, the core never
  answered: Bluetooth did not start. With the debug build's printing slowing the loop it did, which hid the cause.
  port/btstack_run_loop_cmm2.c is pico-sdk's async-context run loop made a polled one: BTstack's work is done only when
  a timer is due, a callback is queued, or the driver's poll has seen the chip's Bluetooth interrupt
  (cyw43_bluetooth_hci_process). The firmware will use the same.
- **Sizes** (the SDK's -O2, the scan's share only): BTstack 122 KB of code and 14 KB of RAM; the glue and run loop
  11 KB and 1 KB. The firmware builds at -Os, and the keyboards and the serial device add hid_host, hids_host, RFCOMM
  and the PicoMite's own code.
- **The pairings:** the PicoMite keeps them in the options, Option.bt_tlv (2 KB, two banks of 1 KB, its
  hal_flash_bank over the options record, saved with them). The CMM2's options are 1,212 bytes in sector 2's first
  4 KB, so the same fits: the CMM2 does as the PicoMite (not a file on the card, as first planned). bt.app keeps them in
  RAM.
- **Not for users:** with OPTION WIFI set, bt.app resets the chip under the firmware's own WiFi, which then has no
  network until a restart (the firmware does not know). Only a test program does that.

## Step 2: the keyboards from bt.app (16 November, done)

- **Classic** (`RUN "bt.app", kbd address`, as BTClassicKeyboard.c connects one: report protocol, Passkey Entry offered,
  hid_host): Peter's Bluetooth 3.0 Keyboard (54:46:6E:AD:87:50), once in pairing mode, paired by Just Works and
  connected 37 s into the run (the page answered only then), its 166-byte HID descriptor read; every key came through,
  16 reports, letters, Enter and Shift. Its reports start with the HIDP header (A1, input data), then report ID 1,
  then the boot layout. A page timeout is asked again until the time is up.
- **BLE** (`RUN "bt.app", ble`, as BTKeyboard.c connects one: active scan, the first advertiser of the HID service,
  LE connection, pairing requested by the board after 2 s if the keyboard does not ask, then hids_host in report
  protocol): Peter's "Bluetooth Keyboard" (10:12:00:B2:00:0A, an address that changes with each pairing), fully
  tested on the PC3, paired by Just Works 2.8 s and had its HID service 7.6 s after the link; "hello" and Enter came
  through, 27 notifications, 0 bus errors, and its battery level (100) on a handle of its own.
- **Learnt, as the PicoMite already knows:**
  - hids_host's own report events gave nothing: the keys are GATT notifications on the link, which BTKeyboard.c takes
    with a listener on all of them (bth_raw_notification_handler); bt.app does the same, 8 bytes the boot layout and
    7 the same without the reserved byte.
  - A speaker's BLE side advertising the HID service for its buttons (-88 dBm) was taken and failed again and again
    (0x3E): BTKeyboard.c's BTH_SCAN_RSSI_MIN (-70) leaves distant strangers alone, and bt.app skips a device that has
    just failed.
- In the firmware these are the PicoMite's own files, so all of this comes with them.

## Step 3: into the firmware (16 November, done)

- **The copies:** tools/port_btstack.py puts BTstack (pico-sdk 2.3.1's commit) in Middlewares/Third_Party/btstack,
  unchanged; tools/port_bt.py copies the PicoMite's bluetooth/*.c and .h (BTKeyboard, BTClassicKeyboard, BTSerial,
  BTAudio, btstack_config.h) into Src/net/bt/picomite with four mechanical rewrites (SaveOptions(1), SoftReset(),
  routinechecks(0), pico_flash_bank_instance) and COM7 -> COM8 in their words. Inc/net/BTMite_cmm2.h is their first
  include, as WebMite_cmm2.h is the network files'; Src/net/bt/BTMite_cmm2.c is what the PicoMite's core gives them.
  pico-sdk's glue (the HCI transport, the chipset, cybt_shared_bus, btstack_cyw43) and the polled run loop are bt.app's.
- **The pieces' homes:** BTstack's code in .net (the network's flash, sectors 3 to 6: 511,896 of 524,288 bytes now);
  the pools from newlib's malloc (HAVE_MALLOC, the PicoMite's MAX_NR_ limits undefined: Inc/net/btstack_config.h).
- **RAM:** the DTCM had no room for 17 KB more. The radio's data and bss (the driver, lwIP, Mbed TLS, BTstack, Src/net)
  moved to the top 32 KB of the D2 RAM's heap (RAMEND 0x30038000, the linker script's .radio and .radiobss: 17.8 KB
  used), their load image in .net. main() sets them up first thing, which first needs the D2 RAM's clocks on: they are
  off after a reset and the writes were lost, so the first build hard-faulted after the banner (closeMQTT on a
  pointer of power-up garbage). The heap is 32 KB smaller.
- **The options:** layout 5 adds Radio, audio_bluetooth and bt_tlv (2 KB of pairings); 3,260 bytes of sector 2's
  first 4 KB. A layout 4 file (8.00.00b1's) loads by OPTION DISK LOAD, with OPTION RADIO set as OPTION WIFI had it.
  A pairing's save rewrites sector 2 with mmbrun: 1.3 s (the PicoMite's is 100 ms), done after BTstack stops writing.
- **OPTION RADIO BOTH | WEB | BLUETOOTH | NONE**, the PC3's, NONE the default; OPTION WIFI turns the WiFi's half on
  (so nothing changes for 8.00.00b1's users). OPTION LIST shows it only when it is not what OPTION WIFI alone gives.
  WebStart brings up what is on: one cyw43_arch_init, then the join and the Bluetooth host (bt_keyboard_init, which
  starts the Classic keyboards, the serial device and the speakers). BtPoll is the PC3's ProcessBT, from CheckAbort
  and routinechecks' full checks, not while a loadable program runs.
- **BLUETOOTH SCAN | CONNECT | DISCONNECT | TEST | STATUS | FORGET** (a new command token at the table's end, so
  saved programs and libraries are unchanged), **MM.INFO$(BLUETOOTH KEYBOARD | SPEAKER | SERIAL)**, and HELP for them.
  BLUETOOTH STATUS #n prints to a file or COM port (ConsoleRedirect, as the PicoMite's).
- **Keyboards:** process_kbd_report with KBD_SOURCE_BT_LE / _CLASSIC, whose lock-key LEDs go back to the keyboard
  (KeyboardMap.c's kbd_send_leds, as the PicoMite's). A mouse, or a keyboard's touchpad, moves the CMM2's mouse on
  channel 2 (USBMouse.c's BtMouseInput), so MM.MOUSE, DEVICE(MOUSE) and the GUI cursor follow it. Peter's keyboard's
  touchpad works perfectly as a mouse (16 November).
- **On the G2 (COM10):** OPTION RADIO BOTH: the WiFi joined and Bluetooth came up together; BLUETOOTH STATUS "on";
  BLUETOOTH SCAN listed the speakers nearby. BLUETOOTH CONNECT KEYBOARD paged once (Bluetooth error 0x04 when the
  keyboard was not in pairing mode: the PicoMite's CONNECT does not try again). Then Peter, at the G2: "keyboard
  working an audio playing perfectly" (step 4's OPTION AUDIO BLUETOOTH with it).

## Step 4 so far (16 November)

- **COM8**: the PicoMite's COM7 (Serial.c) on the CMM2's USB serial devices' state (struct s_cdccom BtCom): OPEN, CLOSE,
  PRINT #, INPUT$, LOC, LOF, the COM interrupt and AS GPS. No TX interrupt, no OC/DE/INV. MAXCOMPORTS is 8.
  On the G2 with Peter's DSD TECH HM-10 (BLE, its UART on the PC's COM41 at 115200): BLUETOOTH CONNECT SERIAL by
  address, OPEN "COM8:", PRINT # and INPUT$ both ways, LOC; a program's COM interrupt; 20 lines (1,031 bytes) out
  whole; 2,000 bytes in whole when paced (50 bytes every 0.25 s). Sent as one burst at 115200, the HM-10 passed on
  only 460 of them (BLUETOOTH STATUS's count): the module's limit, not the CMM2's. The link stays up after CLOSE, as the
  PicoMite's, so the module no longer advertises and BLUETOOTH SCAN does not list it.
- **OPTION AUDIO BLUETOOTH [idle] | DAC:** the audio interrupt's pairs (stm32h7xx_it.c) also go to a ring of 4,096
  (HdmiAudio.h's BtPut), from which bt_audio_pull takes them at the speaker's 44.1 kHz with the two clocks' exact ratio,
  35 ms behind (Src/net/bt/BTMite_cmm2.c). Every PLAY source but a loadable program's stream (Loader.c's DMA). The DACs
  play too. MM.INFO(OPTION AUDIO) gives BLUETOOTH. Plays perfectly to Peter's speaker on the G2 (16 November).
- **Game controllers** (9cc48c1, not yet tried): BTKeyboard.c publishes the PicoMite's nunstruct[3], which
  DEVICE(GAMEPAD 3) now reads as the PicoMite's does (B the HID buttons 1-12 as they are, H the hat, six axes, T &H4254),
  unless a USB pad has channel 3; GAMEPAD INTERRUPT ENABLE 3 on any change. The PicoMite takes BLE controllers only
  (BTKeyboard.c's HID-over-GATT; BTClassicKeyboard.c is for keyboards), so a DualShock 4 (Classic HID) or a DualShock
  3 (Classic, paired over USB) is not one; an Xbox Series controller or an 8BitDo in a BLE mode would be.
- **The pairings' save** (9cc48c1): bt_tlv_save calls BtSaveOptions (a port_bt.py rewrite), which keeps the pairings in
  RAM and tries again every 2 s when there is no RAM for sector 2's 128 KB copy (a G1 running a program), instead of
  ending the program with "Not enough memory to save the options".

## The questions as asked

1. **The licence:** BTstack's non-commercial condition on the CMM2 (as now for cyw43-driver), or ask BlueKitchen
   first.
2. **The room in flash:** the library to the SD card (recommended), mmbrun out of flash, or no speakers.
3. **What to include:** all of the PC3's (BLE keyboards, mice and game controllers; Classic keyboards; a serial
   device; speakers), or a part of it.
4. **Turning it on:** the PC3's OPTION RADIO BOTH | WEB | BLUETOOTH | NONE, with NONE the CMM2's default, as the
   module is optional, or a separate OPTION BLUETOOTH ON | OFF beside OPTION WIFI.
5. **The serial port's number:** COM8.
