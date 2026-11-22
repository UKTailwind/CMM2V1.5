# Bluetooth


The WiFi module's chip is a Bluetooth radio as well. With it the Colour Maximite 2 can use Bluetooth keyboards, mice and game controllers, play its sound through a Bluetooth speaker or headphones, and use a Bluetooth serial device, such as a GPS receiver or a serial module, as the serial port COM8. The WiFi and Bluetooth can be used at the same time.

The Bluetooth is the PicoMite's, the code of its Bluetooth host versions (PicoMiteRP2350BTH, PicoMiteHDMIBTH and PicoMitePC3) unchanged, with the same BLUETOOTH command, options and functions. The differences are listed at the end of this chapter.

## Turning Bluetooth On

Bluetooth needs the WiFi module (see The WiFi Module in the chapter WiFi and Internet Functions). It is turned on with OPTION RADIO at the command prompt:

```
OPTION RADIO BLUETOOTH
```

OPTION RADIO says which halves of the chip are used: BOTH (the WiFi and Bluetooth), WEB (the WiFi only), BLUETOOTH (Bluetooth only) or NONE. NONE is the default, and OPTION WIFI turns the WiFi's half on, so a Colour Maximite 2 that uses the WiFi needs OPTION RADIO BOTH to add Bluetooth. The option is saved and the Colour Maximite 2 restarts. OPTION LIST shows it when it is not what OPTION WIFI alone gives.

With Bluetooth on, on a Generation 1 or 2, the module's four pins (32, 33, 38 and 40) belong to the radio as described in the chapter WiFi and Internet Functions; a Generation 3's module uses none of the connector's pins. If no module answers at start-up the Colour Maximite 2 prints "WiFi module not found" and carries on without the radio.

## Memory and Pairings

Bluetooth keeps 64KB of the memory for BASIC's variables for itself (MM.INFO(HEAP) is 64KB less), on every generation, from start-up until it is turned off. It holds the connections' data and the speaker's sound.

When a device is paired, the keys that let it reconnect are stored with the options, so a paired device reconnects by itself after a restart or a power cycle. Saving them rewrites the flash that holds the options, which takes about 1.3 seconds, during which everything else waits. OPTION RESET deletes all the pairings, as does BLUETOOTH FORGET (see below); OPTION DISK SAVE and OPTION DISK LOAD keep them.

Bluetooth is not serviced while a loadable program (a .app file) runs: a Bluetooth keyboard does not type into it, and a speaker plays nothing. On a Generation 1 with the WiFi on, a loadable program built to use the video memory is lent the network's memory, and the radio is started again from the beginning when it ends; the Bluetooth devices then connect again by themselves.

## Keyboards and Mice

Keyboards, mice and game controllers using Bluetooth LE (also called HID over GATT) pair by themselves: put the device into its pairing mode close to the Colour Maximite 2 and it is found, paired (normally without a PIN) and connected, and the message "Bluetooth Keyboard Connected" is shown on the console. Only devices close by (roughly within the same room) are taken, so a neighbour's devices do not connect by accident. Once paired, the device reconnects whenever it or the Colour Maximite 2 is switched on.

A few keyboards ask for a passkey while pairing. The Colour Maximite 2 then shows a six digit number on the console, for example "Bluetooth keyboard pairing: type 123456 on the keyboard being paired, then Enter", which is typed on that keyboard (not on one already connected) followed by Enter, within about 30 seconds.

A connected keyboard works as a USB keyboard does: at the command prompt, in the editor and the file manager, and in a program with INKEY$ and KEYDOWN(). OPTION KEYBOARD sets its layout, and its Caps, Num and Scroll Lock lights follow the lock keys. A Bluetooth mouse, or the touchpad of a Bluetooth keyboard, moves the mouse on channel 2: DEVICE(MOUSE 2, ...), MM.MOUSE and the GUI controls' cursor follow it.

Bluetooth Classic keyboards (the older kind, including many low cost keyboards with a built-in touchpad) do not pair by themselves. Put the keyboard into its pairing mode and search for it with BLUETOOTH SCAN, which lists it as a 'Keyboard':

```
> BLUETOOTH SCAN
Scanning...
54:46:6E:AD:87:50  -65 dBm  Keyboard    "Bluetooth 3.0 Keyboard"
1 device found
```

Then connect to it using the name the scan showed (or its address):

```
BLUETOOTH CONNECT "Bluetooth 3.0 Keyboard"
```

The keyboard is then remembered and reconnects whenever it wakes up or is switched on. Only the remembered Classic keyboard may connect, and connecting another replaces it. A Bluetooth LE keyboard, a Bluetooth Classic keyboard and a speaker can all be connected at the same time. The scan looks only once: a keyboard that is not in pairing mode when it runs is not listed, and BLUETOOTH CONNECT gives up if the keyboard does not answer (Bluetooth error 0x04).

Many low cost keyboards sold for tablets have a mode for each operating system, chosen with an Fn key combination or a switch. Set such a keyboard to its Windows or PC mode: in its iOS mode Home, End, Page Up and Page Down are sent as Cmd with an arrow key, the function keys send iPad shortcuts and the touchpad works differently.

BLUETOOTH STATUS shows whether a keyboard is connected (a separate line for a Bluetooth Classic keyboard), and MM.INFO$(BLUETOOTH KEYBOARD) returns its address, or an empty string if none is connected.

## Game Controllers

A Bluetooth LE game controller (for example an Xbox Series controller, or an 8BitDo controller in a Bluetooth LE mode) pairs by itself as a keyboard does, and is read as channel 3 with DEVICE(GAMEPAD 3, ...) unless a USB game controller has channel 3. Its values are those the PicoMite gives:

{widths: 15,85}
| Key | Value |
|---|---|
| B | Its buttons 1 to 12 as the controller numbers them, in bits 0 to 11 (not the button map of a USB game controller) |
| H | Its hat or d-pad: 0 to 7 from north clockwise, 255 when centred |
| LX, LY, RX, RY | The two sticks, as the controller reports them |
| L, R | Its analogue triggers (the fifth and sixth axes) |
| T | 16980 (&H4254, "BT") |

GAMEPAD INTERRUPT ENABLE 3, sub calls the subroutine whenever anything on the controller changes. Controllers that use Bluetooth Classic, such as the PlayStation 3 and 4 controllers and the Nintendo Switch Pro Controller, cannot be used over Bluetooth (they work over USB).

## Speakers and Headphones

The sound can be sent to a Bluetooth speaker or headphones (anything that supports the A2DP profile, which is almost every Bluetooth speaker). Everything the PLAY command makes is played through it: tones, sounds, WAV, FLAC, MP3 and MOD files, arrays, samples, speech, the OPL synthesiser and BBC sounds. The audio output of the Colour Maximite 2 plays it as well. The sound of a loadable program that sends its own stream (the emulators and games) is not played through the speaker.

First select Bluetooth for the sound. The command is entered at the command prompt and saved, so it is needed once (the Colour Maximite 2 then restarts):

```
OPTION AUDIO BLUETOOTH
```

Then put the speaker into its pairing mode and search for it with BLUETOOTH SCAN, which marks the audio devices:

```
> BLUETOOTH SCAN
Scanning...
DE:A5:EB:22:71:D4  -64 dBm  Audio       "SWL-BT"
1 device found
```

Connect to it using the name the scan showed (or its address):

```
BLUETOOTH CONNECT "SWL-BT"
```

The speaker is now paired and remembered. From then on it is connected again when the Colour Maximite 2 starts and when the speaker is switched off and on again or comes back into range (the Colour Maximite 2 keeps trying, at intervals of up to a minute), so BLUETOOTH CONNECT is normally needed once. Only one speaker is remembered: connecting another replaces it. When the speaker connects or disconnects by itself, "Bluetooth speaker connected" or "Bluetooth speaker disconnected" is shown at the command prompt.

A speaker that asks for a PIN when it pairs is given 0000. One that needs another PIN (its instructions say so; some car radios use 1234) is connected with the PIN added:

```
BLUETOOTH CONNECT "Car Radio", "1234"
```

The stream to the speaker starts as soon as it connects. It is suspended when nothing has played for 60 seconds, and started again by the next sound, which is then delayed slightly. The time is set by adding it to the option, for example OPTION AUDIO BLUETOOTH 10 (1 to 250 seconds, without a restart); OPTION AUDIO BLUETOOTH 0 keeps the stream running all the time, which avoids the delay but keeps the speaker awake. OPTION AUDIO DAC ends the sound to Bluetooth (with a restart). MM.INFO(OPTION AUDIO) returns "BLUETOOTH" while it is set.

Bluetooth speakers buffer the sound, so it is heard a fraction of a second after it is played (typically 0.2 seconds, depending on the speaker): unnoticed with music, but a game's sound effects lag slightly behind the action. Encoding the sound for Bluetooth takes about 6% of a Generation 2's processor while the stream runs, so a program runs a little slower then.

BLUETOOTH DISCONNECT disconnects the speaker and stops it being connected again until the next BLUETOOTH CONNECT or restart. MM.INFO$(BLUETOOTH SPEAKER) returns the address of the connected speaker, or an empty string, so that a program can check that it is there.

## Serial Devices (COM8)

A Bluetooth serial device becomes the serial port COM8 (COM4 to COM7 are the USB serial devices). It can be a Bluetooth Classic device with a serial port (the Serial Port Profile, often called SPP), for example a Bluetooth GPS receiver or an HC-05 module, or a Bluetooth LE device with a serial service: the Nordic UART Service (used by many modules and by the console of the PicoMiteRP2350BT), the service of HM-10 modules, or Microchip's Transparent UART.

Make the device discoverable (see its instructions) and search for it with BLUETOOTH SCAN. A Bluetooth LE serial device is listed as 'BLE serial', and a Bluetooth Classic serial device without a type:

```
> BLUETOOTH SCAN
Scanning...
B0:D2:78:1B:91:04  -55 dBm  BLE serial  "DSD TECH"
00:19:01:2D:DC:BF  -81 dBm              "XGPS150-2DDCBF"
2 devices found
```

Then connect to it using the name the scan showed (or its address):

```
BLUETOOTH CONNECT "DSD TECH"
```

A Bluetooth Classic device is first checked for a serial port (one without is taken as a speaker) and then paired: if it asks for a PIN, 0000 is given unless BLUETOOTH CONNECT gives another (many serial modules use 1234, for example BLUETOOTH CONNECT "HC-05", "1234"). A Bluetooth LE device is used without pairing unless it asks for it. The device is remembered. Only one serial device can be connected at a time, and connecting another replaces the remembered one, but a serial device, Bluetooth keyboards and a speaker can all be connected at the same time.

COM8 is then opened and used as COM4 to COM7 are, for example:

```
OPEN "COM8:" AS #1
```

or, for a GPS receiver:

```
OPEN "COM8:" AS GPS
```

A baud rate can be given but is ignored, as are 7BIT, EVEN, ODD and S2; OC, DE, INV and a transmit interrupt cannot be given. A receive buffer size, an interrupt subroutine and its trigger level are given as for COM1 and COM2 (see Appendix A). LOC(), LOF(), INPUT$ and EOF() work as for the other ports.

While COM8 is open the remembered device is connected again whenever it is not connected (when the port is opened, and when the device is switched off and on again or comes back into range), and the communication carries on. When the device connects or disconnects by itself, "Bluetooth serial device connected (COM8)" or "Bluetooth serial device disconnected (COM8)" is shown at the command prompt. CLOSE closes the port but leaves the device connected; while the port is closed the device is not connected again, and a connected Bluetooth LE device no longer advertises, so BLUETOOTH SCAN does not list it.

Bluetooth LE carries a little at a time. An HM-10 module fed a long burst at 115200 baud passes on only part of it: its other side should send in pieces, or at a lower speed.

BLUETOOTH DISCONNECT SERIAL disconnects the device and stops it being connected again until COM8 is next opened or the next BLUETOOTH CONNECT. BLUETOOTH STATUS shows the connection and the bytes received and sent, and MM.INFO$(BLUETOOTH SERIAL) returns the address of the connected device, or an empty string, so that a program can wait for it to connect.

Some Bluetooth GPS receivers have a mode switch for an iPhone or iPad which hides their serial port: set it for other devices (Android or PC). A Bluetooth LE device that is switched off may take some seconds to be shown as disconnected, as this is only noticed when the link times out.

## Status and Forgetting Devices

BLUETOOTH STATUS lists the connections: the Bluetooth LE keyboard (or mouse or game controller), the Bluetooth Classic keyboard, the serial device on COM8, and the speaker with its stream. The lines after them are diagnostic information, which may be useful when reporting a problem. BLUETOOTH STATUS #n sends the list to the file or serial port open as #n.

BLUETOOTH FORGET deletes the stored pairings of every device, and BLUETOOTH FORGET addr$ those of one device (its address as BLUETOOTH STATUS or MM.INFO$(BLUETOOTH ...) gives it). A connected device that is forgotten is disconnected. This clears a pairing that has stopped working, after which the device can be paired again.

## Differences from the PicoMite

- The Bluetooth serial device is COM8, not COM7.
- OPTION RADIO is the PicoMitePC3's, with NONE the default (the module is an option on a Generation 1 or 2), and OPTION WIFI turns the WiFi's half on.
- OPTION AUDIO DAC, not OPTION AUDIO DISABLE, ends the sound to Bluetooth, and the Colour Maximite 2's audio output plays the sound as well.
- A Bluetooth LE game controller is channel 3 only while no USB game controller has it.
- Bluetooth is not serviced while a loadable program runs.
- The console over Bluetooth (the PicoMiteRP2350BT version) is not available.
