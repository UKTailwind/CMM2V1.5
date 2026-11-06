/* hardware/flash.h: pico-spec keeps a custom ROM in the RP2040's flash. Here its slots are in the program, in RAM, so the writes are copies, with XIP_BASE 0 making an
   offset an address: such a ROM lasts until the program ends */
#ifndef CMM2_FLASH_H
#define CMM2_FLASH_H
#include <string.h>
#include "pico.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#define FLASH_SECTOR_SIZE 4096u
#define FLASH_PAGE_SIZE 256u
#define XIP_BASE 0u
#define SRAM_BASE 0x20000000u
static inline void flash_range_erase(uint32_t offset, size_t count) { memset((void *)offset, 0xFF, count); }
static inline void flash_range_program(uint32_t offset, const uint8_t *data, size_t count) { memcpy((void *)offset, data, count); }
#endif
