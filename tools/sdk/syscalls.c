/* syscalls.c: newlib's system calls for a loadable program (tools/sdk), on the firmware's services. MIT License, as
   cmm2img.h.

   File descriptors 0-2 are the console (a newline goes out as CR LF; a read waits for one key, echoes it, and gives
   Enter as a newline), or a file put in its place by dup2, as a shell's < and > would; 3 up are the services' file
   handles 0-7. The heap (malloc) is the start block's, between the
   program's .bss and its stack. time() and gettimeofday() read MMBasic's clock (DATE$ and TIME$), with the
   microseconds since start-up for the fraction. */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "cmm2.h"

#undef errno
extern int errno;

#define FD_FILE 3                                       /* the first file's descriptor: handle 0 */
#define HANDLES 8                                       /* the services' file handles */

static int8_t stdh[FD_FILE] = {-1, -1, -1};             /* 0-2: the handle dup2 put in the console's place, or -1 */
static uint8_t refs[HANDLES];                           /* the descriptors open on each handle */

/* the services' handle a descriptor stands for, or -1: the console */
static int handle(int fd) {
    if(fd < 0) return -1;
    return fd < FD_FILE ? stdh[fd] : fd - FD_FILE;
}

int _write(int fd, const char *p, int n) {
    int i, h = handle(fd);
    int32_t r;
    if(h < 0 && (fd == 1 || fd == 2)) {
        for(i = 0; i < n; i++) {
            if(p[i] == '\n') cmm2->con_putc('\r');
            cmm2->con_putc(p[i]);
        }
        return n;
    }
    if(h >= 0) {
        if((r = cmm2->file_write(h, p, n)) >= 0) return r;
        errno = EIO;
        return -1;
    }
    errno = EBADF;
    return -1;
}

int _read(int fd, char *p, int n) {
    int c, h = handle(fd);
    int32_t r;
    if(h < 0 && fd == 0) {
        if(n <= 0) return 0;
        while((c = cmm2->con_getc()) < 0) cmm2->poll();
        if(c == '\r') c = '\n';
        if(c == '\n') cmm2->con_puts("\r\n");
        else cmm2->con_putc(c);
        *p = c;
        return 1;
    }
    if(h >= 0) {
        if((r = cmm2->file_read(h, p, n)) >= 0) return r;
        errno = EIO;
        return -1;
    }
    errno = EBADF;
    return -1;
}

int _open(const char *name, int flags, int mode) {
    int acc = flags & O_ACCMODE, m = 0, h;
    (void)mode;
    if(acc != O_WRONLY) m |= CMM2_O_READ;
    if(acc != O_RDONLY) {
        m |= CMM2_O_WRITE;
        if(flags & O_APPEND) m |= CMM2_O_APPEND;
        else if(flags & (O_CREAT | O_TRUNC)) m |= CMM2_O_CREATE;
    }
    if((h = cmm2->file_open(name, m)) < 0) {
        errno = (h == -4 || h == -5) ? ENOENT : EIO;    /* (FatFs's FR_NO_FILE, FR_NO_PATH) */
        return -1;
    }
    if(h < HANDLES) refs[h] = 1;
    return h + FD_FILE;
}

int creat(const char *name, mode_t mode) { return _open(name, O_WRONLY | O_CREAT | O_TRUNC, mode); }   // (newlib declares
                                                                                                    // it, defines none)

int _close(int fd) {
    int h = handle(fd);
    if(fd >= 0 && fd < FD_FILE) stdh[fd] = -1;          /* (the console again) */
    if(h < 0) return 0;
    if(h < HANDLES && refs[h] > 1) { refs[h]--; return 0; }   /* still open on another descriptor */
    if(h < HANDLES) refs[h] = 0;
    if(cmm2->file_close(h) < 0) { errno = EBADF; return -1; }
    return 0;
}

/* fd2 (0-2) made another name for fd's file, in place of the console, or the console again if fd is the console. Only
   the console's descriptors can be made so: a file's are the services' handles. */
int dup2(int fd, int fd2) {
    int h = handle(fd);
    if(fd < 0 || fd2 < 0 || fd2 >= FD_FILE || h >= HANDLES) { errno = EBADF; return -1; }
    if(fd == fd2) return fd2;
    _close(fd2);
    if(h >= 0) {
        stdh[fd2] = h;
        refs[h] = (refs[h] ? refs[h] : 1) + 1;
    }
    return fd2;
}

off_t _lseek(int fd, off_t off, int whence) {
    int h = handle(fd);
    int32_t pos;
    if(h < 0) return 0;
    if(whence == SEEK_SET) pos = off;
    else if(whence == SEEK_CUR) pos = cmm2->file_tell(h) + off;
    else pos = cmm2->file_size(h) + off;
    if(pos < 0 || cmm2->file_seek(h, pos) < 0) { errno = EINVAL; return -1; }
    return pos;
}

int _fstat(int fd, struct stat *st) {
    int h = handle(fd);
    memset(st, 0, sizeof(*st));
    if(h < 0) { st->st_mode = S_IFCHR; return 0; }
    st->st_mode = S_IFREG;
    st->st_size = cmm2->file_size(h);
    return 0;
}

int _isatty(int fd) { return handle(fd) < 0 && fd >= 0 && fd < FD_FILE; }

static char *brk;                                                   // malloc's heap: from the start block's, upward

/* size bytes (rounded up to 8) taken from the top of the heap, for a program's own allocator; malloc keeps the rest.
   NULL if malloc has already taken too much. */
void *cmm2_heap_take(uint32_t size) {
    char *top = (char *)cmm2_start->heap + cmm2_start->heap_size;
    size = (size + 7) & ~7u;
    if(brk == NULL) brk = cmm2_start->heap;
    if(size > (uint32_t)(top - brk)) return NULL;
    cmm2_start->heap_size -= size;
    return top - size;
}

void *_sbrk(ptrdiff_t incr) {
    char *prev;
    if(brk == NULL) brk = cmm2_start->heap;
    if(brk + incr > (char *)cmm2_start->heap + cmm2_start->heap_size || brk + incr < (char *)cmm2_start->heap) {
        errno = ENOMEM;
        return (void *)-1;
    }
    prev = brk;
    brk += incr;
    return prev;
}

void _exit(int code) {
    cmm2->leave(code);
    for(;;) {}
}

int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
int _getpid(void) { return 1; }
int _unlink(const char *name) { return cmm2->file_remove(name) < 0 ? (errno = ENOENT, -1) : 0; }
int mkdir(const char *name, mode_t mode) { (void)mode; return cmm2->dir_make(name) < 0 ? (errno = EIO, -1) : 0; }
/* rename itself (newlib's would link and unlink): FatFs renames, and a file already called 'to' is replaced, as POSIX
   has it (FR_EXIST, 8, then the file removed and the rename tried again) */
int rename(const char *from, const char *to) {
    int r = cmm2->file_rename(from, to);
    if(r == -8 && cmm2->file_remove(to) == 0) r = cmm2->file_rename(from, to);
    return r < 0 ? (errno = r == -4 ? ENOENT : EIO, -1) : 0;
}

/* days from 1 Jan 1970 to the date (Howard Hinnant's days_from_civil) */
static int32_t days(int y, int m, int d) {
    int32_t era, yoe, doy;
    y -= m <= 2;
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}

int _gettimeofday(struct timeval *tv, void *tz) {
    int t[6];
    (void)tz;
    cmm2->datetime(t);
    tv->tv_sec = (time_t)days(t[0], t[1], t[2]) * 86400 + t[3] * 3600 + t[4] * 60 + t[5];
    tv->tv_usec = (suseconds_t)(cmm2->micros() % 1000000);
    return 0;
}
