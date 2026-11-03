/* termios.h for tiny386's misc.c: its host console capture (a PC's serial port on the host's terminal), which the CMM2
   never turns on (enable_serial). Just what it names; pc386_cmm2.c's tcgetattr and tcsetattr do nothing. */
#ifndef PC386_TERMIOS_H
#define PC386_TERMIOS_H
struct termios {
    unsigned int c_iflag, c_oflag, c_cflag, c_lflag;
};
#define ISIG    0x01
#define ICANON  0x02
#define ECHO    0x08
#define TCSANOW 0
int tcgetattr(int fd, struct termios *t);
int tcsetattr(int fd, int action, const struct termios *t);
#endif
