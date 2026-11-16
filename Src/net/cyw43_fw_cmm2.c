/*
 * cyw43_fw_cmm2.c: the CYW43439's firmware, kept compressed in the flash and inflated into MMBasic's heap for the
 * driver's download (16 Nov 2026, docs/Bluetooth_Plan.md: 84 KB of the flash freed for Bluetooth).
 *
 * The driver reads the blob through fw_data (Inc/net/cyw43_fw_cmm2.h, its CYW43_CHIPSET_FIRMWARE_INCLUDE_FILE): the
 * firmware itself when the chip starts (cyw43_ll_bus_init) and the CLM after it (cyw43_ll_wifi_on), both inside
 * WebStart's cyw43_arch_enable_sta_mode, after which WebStart calls cyw43_fw_release. The blob is raw deflate
 * (tools/fwpack.py), inflated by zlib's puff (Middlewares/Third_Party/puff) and checked against its CRC-32 before the
 * driver sees it. If there is no memory for it, or it does not check, the driver is given the compressed data instead,
 * which its own check refuses (no version trailer), and the start fails as with no module.
 */
#include <stdint.h>
#include <stddef.h>
#include "cyw43_config.h"
#define CYW43_FW_Z_DATA
#include "cyw43_fw_z.h"
#include "puff.h"

extern void *GetMemoryNull(size_t NbrBytes);
extern void FreeMemory(void *addr);
extern void MMPrintString(char *s);

void cyw43_fw_release(void);

static unsigned char *fwbuf;

static uint32_t crc32_of(const unsigned char *p, unsigned long n) {
    uint32_t c = 0xFFFFFFFFU;
    while (n--) {
        c ^= *p++;
        for (int k = 0; k < 8; k++)
            c = (c >> 1) ^ (0xEDB88320U & (0U - (c & 1U)));
    }
    return ~c;
}

uintptr_t cyw43_fw_data(void) {
    if (fwbuf == NULL) {
        unsigned long dlen = CYW43_FW_RAW_LEN, slen = CYW43_FW_Z_LEN;
        fwbuf = GetMemoryNull(CYW43_FW_RAW_LEN);
        if (fwbuf == NULL) {
            MMPrintString("WiFi: no memory for the chip's firmware\r\n");
            return (uintptr_t)cyw43_fw_z;
        }
        if (puff(fwbuf, &dlen, cyw43_fw_z, &slen) != 0 || dlen != CYW43_FW_RAW_LEN
            || crc32_of(fwbuf, dlen) != CYW43_FW_RAW_CRC) {
            MMPrintString("WiFi: the chip's firmware does not check\r\n");
            cyw43_fw_release();
            return (uintptr_t)cyw43_fw_z;
        }
    }
    return (uintptr_t)fwbuf;
}

void cyw43_fw_release(void) {
    if (fwbuf != NULL) {
        FreeMemory(fwbuf);
        fwbuf = NULL;
    }
}
