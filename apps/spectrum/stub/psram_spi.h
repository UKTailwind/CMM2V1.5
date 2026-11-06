/* psram_spi.h: no PSRAM here (psram_size() is 0, so none of this is reached): the Spectrum's memory is on the heap */
#ifndef CMM2_PSRAM_SPI_H
#define CMM2_PSRAM_SPI_H
#include "pico.h"
#ifdef __cplusplus
extern "C" {
#endif
uint32_t psram_size(void);
void init_psram(void);
#define psram_id(rx) ((void)(rx))
static inline uint8_t read8psram(uint32_t a) { (void)a; return 0; }
static inline uint16_t read16psram(uint32_t a) { (void)a; return 0; }
static inline uint32_t read32psram(uint32_t a) { (void)a; return 0; }
static inline void write8psram(uint32_t a, uint8_t v) { (void)a; (void)v; }
static inline void write16psram(uint32_t a, uint16_t v) { (void)a; (void)v; }
static inline void write32psram(uint32_t a, uint32_t v) { (void)a; (void)v; }
static inline void psram_read_range(uint32_t a, void *p, size_t n) { (void)a; (void)p; (void)n; }
static inline void psram_write_range(uint32_t a, const void *p, size_t n) { (void)a; (void)p; (void)n; }
static inline void readpsram(void *p, uint32_t a, size_t n) { (void)a; (void)p; (void)n; }
static inline void writepsram(uint32_t a, const void *p, size_t n) { (void)a; (void)p; (void)n; }
static inline void psram_read_page(uint32_t a, void *p) { (void)a; (void)p; }
static inline void psram_write_page(uint32_t a, const void *p) { (void)a; (void)p; }
static inline void psram_write_page_async(uint32_t a, const void *p) { (void)a; (void)p; }
#ifdef __cplusplus
}
#endif
#endif
