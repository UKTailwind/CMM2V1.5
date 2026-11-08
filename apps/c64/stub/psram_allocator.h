/* psram_allocator.h: the CMM2 has no PSRAM, but a heap as big. frank-c64 is built as for a board with PSRAM (PSRAM_MAX_FREQ_MHZ), so that a cartridge goes to
   the heap and not to the RP2350's flash: these are malloc's (c64_cmm2.c) */
#ifndef CMM2_PSRAM_ALLOCATOR_H
#define CMM2_PSRAM_ALLOCATOR_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void *psram_malloc(size_t size);
void *psram_realloc(void *ptr, size_t size);
void psram_free(void *ptr);
#ifdef __cplusplus
}
#endif
#endif
