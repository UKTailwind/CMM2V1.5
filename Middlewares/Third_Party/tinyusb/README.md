# TinyUSB (host subset)

The files the CMM2's USB host needs, copied from TinyUSB master
**efab607b8** (2026-10-11, "Merge pull request #3776"): the same commit the
PicoMite builds against (`PicoMite/tinyusb-patches/README.md`), with the local
changes listed below.

- `src/tusb.c`, `tusb.h`, `tusb_option.h`, `src/common/`, `src/osal/`
- `src/host/` (the host stack and the hub driver)
- `src/class/hid/hid.h`, `hid_host.c`, `hid_host.h`
- `src/portable/synopsys/dwc2/`: the dwc2 host driver and its STM32 header
- `src/class/cdc/` (USB serial, COM4-COM7) and `src/class/msc/` (the flash
  drive, C:): from the PicoMite's patched tree (18 and 17 Oct 2026)

The PicoMite's RP2 host driver patches (`hcd_rp2040.c`, `rp2040_usb.c`) do not
apply. Master rather than release 0.21 because 0.21 needed a `usbh.c`
patch for devices behind a hub that fail their first enumeration (master's
#3862 retries them from a fresh port reset), and master also has the hub
fixes of #3776 and later dwc2 host fixes (channels retired on a root-port
disconnect, slave-mode periodic scheduling).

## Local changes

- `src/class/cdc/cdc_host.c`, `cdc_host.h`: the PicoMite's re-arm of a failed
  bulk IN, and `tuh_cdc_rx_enable()` (b8cc9c3, 22 Oct 2026). A USB serial
  device's bulk IN is armed only while BASIC has its port open, and every
  arming goes through `cdch_rx_arm()`. The PicoMite carries its own version of
  the same function (its 1dc9572); CMM2V1.5 issue #1 compares the two, and
  this one has the PicoMite's no-op for an unchanged state and its reset when
  a device goes.
- `src/portable/synopsys/dwc2/hcd_dwc2.c`: the low-speed-behind-a-hub refusal
  (below), and in slave mode an abort that retires its channel on the halt,
  keeping the PID it reached (b8cc9c3): the halt handler re-armed a NAKed IN, so
  an abort never ended a transfer to a device with nothing to send.
- `src/host/usbh.c`: the reset recovery is `CFG_TUH_ENUM_RESET_RECOVERY_MS`
  (100 ms in `Inc/tusb_config.h`, 29f6c2c).

## The low-speed patch

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
`Src/USBKeyboard.c` and `Src/KeyboardMap.c` (and `Src/Serial.c` and
`Src/USBDrive.c` for the CDC and MSC classes). To update, copy the same files
from a newer tree and apply the patch and the local changes.
