/* lwipopts.h for wifi.app (step 1 of docs/Web_Plan.md): the PicoMite's lwIP settings, net/lwipopts_examples_common.h
   copied as it is beside this file, with the lines of its net/lwipopts.h that do not need the firmware (MMBasic's
   error variables and FatFs). lwIP's heap is the C library's (MEM_LIBC_MALLOC, polled), as on the PicoMite: in a
   loadable program that is the program's own heap. */
#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

#define PICO_CYW43_ARCH_POLL 1
#include "lwipopts_examples_common.h"

#define LWIP_ALTCP 1
// TLS, as the PicoMite's lwipopts.h has it under PICOMITEWEB_TLS
#define LWIP_ALTCP_TLS 1
#define LWIP_ALTCP_TLS_MBEDTLS 1
#define MEM_SIZE 40960
// The receive window must hold a whole TLS record: altcp_tls_mbedtls.c opens the window again only for what mbedTLS
// has decrypted, and it decrypts only whole records, up to 16,384 bytes of data plus 29 of header and tag (16,413).
// The PicoMite's 8 x TCP_MSS (11,680) stalls a download for good at the server's first full-size record (15 Nov:
// speed.cloudflare.com stopped after 220 KB, its small opening records done). 16 x TCP_MSS = 23,360. PicoMite issue 153.
#undef TCP_WND
#define TCP_WND (16 * TCP_MSS)
#define DNS_TABLE_SIZE 1
#define MEMP_NUM_SYS_TIMEOUT (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 3)

#endif
