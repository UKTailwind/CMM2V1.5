# TinyUSB (host subset)

The files the CMM2's USB keyboard host needs, copied from TinyUSB master
**efab607b8** (2026-10-11, "Merge pull request #3776"): the same commit the
PicoMite builds against (`PicoMite/tinyusb-patches/README.md`), with one patch
(below).

- `src/tusb.c`, `tusb.h`, `tusb_option.h`, `src/common/`, `src/osal/`
- `src/host/` (the host stack and the hub driver)
- `src/class/hid/hid.h`, `hid_host.c`, `hid_host.h`
- `src/portable/synopsys/dwc2/`: the dwc2 host driver and its STM32 header

None of the PicoMite's patches apply: they change the RP2 host driver
(`hcd_rp2040.c`, `rp2040_usb.c`) and the CDC host class, none of which the
CMM2 compiles. Master rather than release 0.21 because 0.21 needed a `usbh.c`
patch for devices behind a hub that fail their first enumeration (master's
#3862 retries them from a fresh port reset), and master also has the hub
fixes of #3776 and later dwc2 host fixes (channels retired on a root-port
disconnect, slave-mode periodic scheduling).

## The patch

`patches/hcd_dwc2-ls-behind-hub.patch` (`src/portable/synopsys/dwc2/hcd_dwc2.c`,
`patch -p1` against stock master): with `CFG_TUH_DWC2_REFUSE_LS_BEHIND_FS_HUB`
set, `hcd_edpt_open()` refuses a **low-speed device behind a hub** when the root
port is not high speed, and calls `tuh_dwc2_ls_hub_refused_cb()` (weak; the CMM2
prints a message from it).

The reason is a silicon erratum of the STM32 OTG cores on their full-speed PHY,
"Host packet transmission may hang when connecting the full speed interface
through a hub to a low-speed device" (STM32H743/753 ES0392 2.25.3, present in
revisions Y, X and V with no workaround; the same erratum is listed for the F4,
F7, H745/747 and MP1). On the CMM2 (USB2_OTG_FS, PA11/PA12, full speed only) a
low-speed keyboard or mouse behind a hub got through a few control transfers,
then a zero-length OUT never completed, and from then on every transfer to every
device hung, the hub's included, until the root port was reset. Refusing the
device before any packet is sent to it keeps the hub and the other devices on it
working. Full- and high-speed devices behind a hub are not affected (tested: a
Cherry keyboard, an HP flash drive and a DualShock 4 on one hub), and a
low-speed device plugged in directly is fine. The Raspberry Pi keyboard, whose
keyboard is low speed behind its own hub, cannot work on this port.

The configuration is `Inc/tusb_config.h`; the application side is
`Src/USBKeyboard.c` and `Src/KeyboardMap.c`. To update, copy the same files
from a newer tree and apply the patch.
