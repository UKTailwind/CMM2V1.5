/* port.h: included first in every file of the PC (make.py's -include). tiny386 times its PIT, CMOS clock and RDTSC
   with POSIX's clock_gettime(CLOCK_MONOTONIC), which newlib for a bare Arm declares only where _POSIX_TIMERS is set:
   here it is declared, and pc386_cmm2.c makes it from the CMM2's microsecond clock. */
#ifndef PC386_PORT_H
#define PC386_PORT_H
#include <sys/types.h>
#include <time.h>
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 4
#endif
int clock_gettime(clockid_t clock_id, struct timespec *tp);
#endif
