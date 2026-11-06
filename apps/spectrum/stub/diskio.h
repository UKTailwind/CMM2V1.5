/* diskio.h: the firmware has the card */
#ifndef CMM2_DISKIO_H
#define CMM2_DISKIO_H
#include "ff.h"
#define STA_NOINIT 0x01
#define RES_OK 0
#define MMC_GET_TYPE 10
#define MMC_GET_CID 12
#ifdef __cplusplus
extern "C" {
#endif
static inline int disk_initialize(int drive) { (void)drive; return 0; }
static inline void disk_invalidate(void) { }
static inline int disk_ioctl(int drive, int cmd, void *buf) { (void)drive; (void)cmd; (void)buf; return 1; }
#ifdef __cplusplus
}
#endif
#endif
