/* sys/ioctl.h for tiny386's misc.c (see termios.h): FIONREAD on the host's terminal, never asked on the CMM2 */
#ifndef PC386_IOCTL_H
#define PC386_IOCTL_H
#define FIONREAD 0x541B
int ioctl(int fd, unsigned long request, ...);
#endif
