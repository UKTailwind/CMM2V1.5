/*
 * cyw43_fw_cmm2.h: the driver's CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE (cyw43_configport.h), which cyw43_config.h says
 * defines CYW43_WIFI_FW_LEN, CYW43_CLM_LEN and fw_data. The Colour Maximite 2 keeps the chip's firmware compressed
 * (cyw43_fw_z.h, made by tools/fwpack.py: 141 KB of flash, not 225 KB), so fw_data is where Src/net/cyw43_fw_cmm2.c
 * has inflated it, in MMBasic's heap, for the download at start-up (WebStart frees it afterwards). 16 Nov 2026: room
 * in the flash for Bluetooth (docs/Bluetooth_Plan.md). The driver's own source is unchanged.
 */
#ifndef CYW43_FW_CMM2_H
#define CYW43_FW_CMM2_H
#include <stdint.h>
#include "cyw43_fw_z.h"

uintptr_t cyw43_fw_data(void);                  // the inflated blob (inflated at the first call)
void cyw43_fw_release(void);                    // its memory given back
#define fw_data (cyw43_fw_data())

#endif
