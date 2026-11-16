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
- **The pairings:** the PicoMite keeps them in flash, through pico-sdk's flash bank. The CMM2 has no flash to spare
  and writes none by design (V8), so they go in a file on the SD card (A:/.bluetooth), kept in RAM and written when
  they change. The battery-backed SRAM is the other place, but it is VAR SAVE's.
- **Memory:** the PicoMite's Bluetooth takes about 24 KB of static RAM. The CMM2's DTCM has about 8 KB left, so
  BTstack's data goes in the SDRAM: in the network's block (grown) or a block of its own. Its .data and .bss are
  grouped by the linker as the network's are, so that a Generation 1 can start it again after a --vram program.
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

## Decisions for Peter

1. **The licence:** BTstack's non-commercial condition on the CMM2 (as now for cyw43-driver), or ask BlueKitchen
   first.
2. **The room in flash:** the library to the SD card (recommended), mmbrun out of flash, or no speakers.
3. **What to include:** all of the PC3's (BLE keyboards, mice and game controllers; Classic keyboards; a serial
   device; speakers), or a part of it.
4. **Turning it on:** the PC3's OPTION RADIO BOTH | WEB | BLUETOOTH | NONE, with NONE the CMM2's default, as the
   module is optional, or a separate OPTION BLUETOOTH ON | OFF beside OPTION WIFI.
5. **The serial port's number:** COM8.
