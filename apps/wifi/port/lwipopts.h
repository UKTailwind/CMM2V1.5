/* lwipopts.h for wifi.app (step 1 of docs/Web_Plan.md): the PicoMite's lwIP settings, net/lwipopts_examples_common.h
   copied as it is beside this file, with the lines of its net/lwipopts.h that do not need the firmware (MMBasic's
   error variables and FatFs). lwIP's heap is the C library's (MEM_LIBC_MALLOC, polled), as on the PicoMite: in a
   loadable program that is the program's own heap. */
#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

#define PICO_CYW43_ARCH_POLL 1
#include "lwipopts_examples_common.h"

#define LWIP_ALTCP 1
#define DNS_TABLE_SIZE 1
#define MEMP_NUM_SYS_TIMEOUT (LWIP_NUM_SYS_TIMEOUT_INTERNAL + 3)

#endif
