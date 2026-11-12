/* syscalls.c: newlib's system calls for a loadable program (tools/sdk), on the firmware's services. MIT License, as
   cmm2img.h.

   File descriptors 0-2 are the console (a newline goes out as CR LF while OPOST and ONLCR are on; a read waits for
   one key, echoes it, and gives Enter as a newline), or a file put in its place by dup2, as a shell's < and > would; 3
   up are the services' file handles 0-7. The console has a termios (sys/termios.h): ECHO, ICRNL, ONLCR, and with
   ICANON off VMIN and VTIME, so that a read can return at once with no key, as an INKEY$ does. The heap (malloc) is
   the start block's, between the program's .bss and its stack. time() and gettimeofday() read MMBasic's clock (DATE$
   and TIME$), with the microseconds since start-up for the fraction. opendir and the rest (sys/dirent.h), stat, chdir and getcwd are the
   services' (versions 5 and 10); fork, exec and wait refuse, as one program cannot start another. memcpy, memset and
   memmove are the firmware's (version 11). */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include "cmm2.h"

#undef errno
extern int errno;

#define FD_FILE 3                                       /* the first file's descriptor: handle 0 */
#define HANDLES 8                                       /* the services' file handles */

static int8_t stdh[FD_FILE] = {-1, -1, -1};             /* 0-2: the handle dup2 put in the console's place, or -1 */
static uint8_t refs[HANDLES];                           /* the descriptors open on each handle */

/* memcpy, memset and memmove (Peter, 4 Nov 2026): newlib-nano's are byte loops, and a program, newlib itself and GCC's
   structure copies all call them. From version 11 they are the firmware's (word at a time, from ITCM), bound by
   cmm2_mem_bind when _start has the services; before that, or from an older firmware, these byte loops. */
#define NOLIBCALL __attribute__((optimize("no-tree-loop-distribute-patterns")))   /* (a loop must not become a call) */
static void *NOLIBCALL byte_copy(void *d, const void *s, size_t n) {
    uint8_t *dp = d;
    const uint8_t *sp = s;
    while(n--) *dp++ = *sp++;
    return d;
}
static void *NOLIBCALL byte_fill(void *d, int c, size_t n) {
    uint8_t *dp = d;
    while(n--) *dp++ = (uint8_t)c;
    return d;
}
static void *NOLIBCALL byte_move(void *d, const void *s, size_t n) {
    uint8_t *dp = d;
    const uint8_t *sp = s;
    if(dp <= sp || dp >= sp + n) return byte_copy(d, s, n);
    dp += n;
    sp += n;
    while(n--) *--dp = *--sp;
    return d;
}
static void *(*mem_copy)(void *, const void *, size_t) = byte_copy;
static void *(*mem_fill)(void *, int, size_t) = byte_fill;
static void *(*mem_move)(void *, const void *, size_t) = byte_move;

void cmm2_mem_bind(void) {
    if(cmm2->version >= 11) {
        mem_copy = cmm2->mem_copy;
        mem_fill = cmm2->mem_fill;
        mem_move = cmm2->mem_move;
    }
}

void *memcpy(void *d, const void *s, size_t n) { return mem_copy(d, s, n); }
void *memset(void *d, int c, size_t n) { return mem_fill(d, c, n); }
void *memmove(void *d, const void *s, size_t n) { return mem_move(d, s, n); }

/* the services' handle a descriptor stands for, or -1: the console */
static int handle(int fd) {
    if(fd < 0) return -1;
    return fd < FD_FILE ? stdh[fd] : fd - FD_FILE;
}

/* the console's line discipline (tcgetattr, tcsetattr): at the start, a key at a time, echoed, Enter a newline */
static struct termios contio = {ICRNL, OPOST | ONLCR, CS8 | CREAD, ISIG | ICANON | ECHO, {[VMIN] = 1}};

int _write(int fd, const char *p, int n) {
    int i, h = handle(fd);
    int32_t r;
    if(h < 0 && (fd == 1 || fd == 2)) {
        int nl = (contio.c_oflag & (OPOST | ONLCR)) == (OPOST | ONLCR);
        for(i = 0; i < n; i++) {
            if(nl && p[i] == '\n') cmm2->con_putc('\r');
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
        if(!(contio.c_lflag & ICANON) && contio.c_cc[VMIN] == 0) {     // (a poll: VTIME tenths of a second at most)
            uint64_t end = cmm2->micros() + contio.c_cc[VTIME] * 100000ull;
            while((c = cmm2->con_getc()) < 0) {
                if(cmm2->micros() >= end) return 0;
                cmm2->poll();
            }
        } else
            while((c = cmm2->con_getc()) < 0) cmm2->poll();
        if(c == '\r' && (contio.c_iflag & ICRNL)) c = '\n';
        if(contio.c_lflag & ECHO) {
            if(c == '\n') cmm2->con_puts("\r\n");
            else cmm2->con_putc(c);
        }
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

int tcgetattr(int fd, struct termios *tp) {
    if(!_isatty(fd)) { errno = ENOTTY; return -1; }
    *tp = contio;
    return 0;
}

int tcsetattr(int fd, int when, const struct termios *tp) {
    (void)when;
    if(!_isatty(fd)) { errno = ENOTTY; return -1; }
    contio = *tp;
    return 0;
}

int tcflush(int fd, int which) {
    if(!_isatty(fd)) { errno = ENOTTY; return -1; }
    if(which != TCOFLUSH) while(cmm2->con_getc() >= 0) {}         // (the keys typed and not read)
    return 0;
}

void cfmakeraw(struct termios *tp) {
    tp->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tp->c_oflag &= ~OPOST;
    tp->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tp->c_cflag = (tp->c_cflag & ~(CSIZE | PARENB)) | CS8;
    tp->c_cc[VMIN] = 1;
    tp->c_cc[VTIME] = 0;
}

speed_t cfgetispeed(const struct termios *tp) { (void)tp; return B115200; }
speed_t cfgetospeed(const struct termios *tp) { (void)tp; return B115200; }
int cfsetispeed(struct termios *tp, speed_t s) { (void)tp; (void)s; return 0; }
int cfsetospeed(struct termios *tp, speed_t s) { (void)tp; (void)s; return 0; }

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
int _fork(void) { errno = ENOSYS; return -1; }                       // (one program cannot start another)
int _execve(const char *name, char *const argv[], char *const env[]) {
    (void)name; (void)argv; (void)env;
    errno = ENOSYS;
    return -1;
}
int _wait(int *status) { (void)status; errno = ECHILD; return -1; }
pid_t waitpid(pid_t pid, int *status, int options) { (void)pid; (void)status; (void)options; errno = ECHILD; return -1; }
int execv(const char *name, char *const argv[]) { return _execve(name, argv, NULL); }
int execvp(const char *name, char *const argv[]) { return _execve(name, argv, NULL); }
int pipe(int fd[2]) { (void)fd; errno = ENOSYS; return -1; }

/* us microseconds, with the USB devices looked after every 10ms meanwhile (and a Ctrl-C there seen), as MMBasic's
   PAUSE does */
int usleep(useconds_t us) {
    uint64_t end = cmm2->micros() + us, now;
    while((now = cmm2->micros()) < end) {
        cmm2->delay_us(end - now > 10000 ? 10000 : (uint32_t)(end - now));
        cmm2->poll();
    }
    return 0;
}

int fsync(int fd) {
    int h = handle(fd);
    if(h < 0) return 0;
    if(cmm2->file_sync(h) < 0) { errno = EIO; return -1; }
    return 0;
}

void sync(void) {}                                                  // (each file is written out as it is closed)
int rmdir(const char *name) { return cmm2->file_remove(name) < 0 ? (errno = ENOENT, -1) : 0; }   // (FatFs: an empty one)
int statvfs(const char *path, struct statvfs *buf) { (void)path; (void)buf; errno = ENOSYS; return -1; }
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

/* names with "." and ".." in them. FatFs takes them on a FAT card, but not on an exFAT one, which has no dot entries
   (f_stat calls "A:/GAMES/.." an origin directory and fails), and the programs that list a directory stat each name
   in it, readdir's ".." too. So stat and opendir make such a name whole first, as the services would take it
   (Loader.c's ld_path: "A:..." is from A:'s root, "/..." from the current drive's, anything else from the current
   directory), with "." left out and ".." taking the part before it off: "A:/GAMES/.." is "A:/" (12 Nov 2026) */
#define PATH_LEN 264                            /* (the services' longest name: FatFs's 255 with "A:/" and the end) */

static int dot_part(const char *name) {        /* 1 if a part of the name is "." or ".." */
    const char *p = name, *s;
    if(p[0] && p[1] == ':') p += 2;
    while(*p) {
        while(*p == '/' || *p == '\\') p++;
        for(s = p; *p && *p != '/' && *p != '\\'; p++) {}
        if((p - s == 1 && s[0] == '.') || (p - s == 2 && s[0] == '.' && s[1] == '.')) return 1;
    }
    return 0;
}

static int add_parts(char *out, size_t *n, const char *p) {   /* after out's "A:" and its parts: each "/part" */
    const char *s;
    size_t len;
    while(*p) {
        while(*p == '/' || *p == '\\') p++;
        for(s = p; *p && *p != '/' && *p != '\\'; p++) {}
        len = p - s;
        if(len == 0 || (len == 1 && s[0] == '.')) continue;
        if(len == 2 && s[0] == '.' && s[1] == '.') {
            while(*n > 2 && out[--*n] != '/') {}   /* (at the root it stays there) */
            continue;
        }
        if(*n + 1 + len >= PATH_LEN) return -1;
        out[(*n)++] = '/';
        memcpy(out + *n, s, len);
        *n += len;
    }
    return 0;
}

static int whole_path(const char *name, char *out) {    /* out: PATH_LEN. 0, or -1 if too long or no getcwd (version 10) */
    char cwd[PATH_LEN];
    size_t n = 2;
    if(name[0] && name[1] == ':') {
        out[0] = name[0];
        name += 2;
    } else {
        if(cmm2->version < 10 || cmm2->dir_current(cwd, sizeof(cwd)) < 0 || !cwd[0] || cwd[1] != ':') return -1;
        out[0] = cwd[0];
        if(*name != '/' && *name != '\\' && add_parts(out, &n, cwd + 2) < 0) return -1;
    }
    out[1] = ':';
    if(add_parts(out, &n, name) < 0) return -1;
    if(n == 2) out[n++] = '/';
    out[n] = 0;
    return 0;
}

int _stat(const char *name, struct stat *st) {
    cmm2_stat s;
    char full[PATH_LEN];
    int r;
    memset(st, 0, sizeof(*st));
    if(cmm2->version < 5) { errno = ENOSYS; return -1; }
    if(dot_part(name) && whole_path(name, full) == 0) name = full;
    if((r = cmm2->file_stat(name, &s)) < 0) { errno = (r == -4 || r == -5) ? ENOENT : EIO; return -1; }
    st->st_mode = s.is_dir ? S_IFDIR | 0755 : S_IFREG | 0644;
    st->st_size = s.size;
    st->st_mtime = (time_t)days(s.year, s.month, s.day) * 86400 + s.hour * 3600 + s.minute * 60 + s.second;
    return 0;
}

int chdir(const char *path) {
    if(cmm2->version < 10) { errno = ENOSYS; return -1; }
    if(cmm2->dir_change(path) < 0) { errno = ENOENT; return -1; }
    return 0;
}

char *getcwd(char *buf, size_t size) {
    if(cmm2->version < 10) { errno = ENOSYS; return NULL; }
    if(cmm2->dir_current(buf, size) < 0) { errno = ERANGE; return NULL; }
    return buf;
}

/* readdir gives ".." first in a directory below a drive's root, as Linux's does (FatFs gives neither "." nor ".."):
   the file browsers of programs ported from Linux go up by it, and had no way up without it (atari800's and sdltrs's,
   12 Nov 2026). "." is not given: they leave it out anyway */
DIR *opendir(const char *path) {
    DIR *d;
    char full[PATH_LEN];
    int whole = whole_path(path, full) == 0, h;
    h = cmm2->dir_open(whole && dot_part(path) ? full : path);
    if(h < 0) { errno = ENOENT; return NULL; }
    if((d = malloc(sizeof(DIR))) == NULL) { cmm2->dir_close(h); errno = ENOMEM; return NULL; }
    d->handle = h;
    d->dotdot = whole && full[3] != 0;          /* (full is "A:/" at a root) */
    return d;
}

struct dirent *readdir(DIR *d) {
    static cmm2_dirent e;
    if(d->dotdot) {
        d->dotdot = 0;
        strcpy(d->entry.d_name, "..");
        d->entry.d_type = DT_DIR;
        d->entry.d_ino = 0;
        return &d->entry;
    }
    if(cmm2->dir_read(d->handle, &e) != 1) return NULL;
    strncpy(d->entry.d_name, e.name, sizeof(d->entry.d_name) - 1);
    d->entry.d_name[sizeof(d->entry.d_name) - 1] = 0;
    d->entry.d_type = e.is_dir ? DT_DIR : DT_REG;
    d->entry.d_ino = 0;
    return &d->entry;
}

int closedir(DIR *d) {
    int r = cmm2->dir_close(d->handle);
    free(d);
    return r < 0 ? (errno = EBADF, -1) : 0;
}

int _gettimeofday(struct timeval *tv, void *tz) {
    int t[6];
    (void)tz;
    cmm2->datetime(t);
    tv->tv_sec = (time_t)days(t[0], t[1], t[2]) * 86400 + t[3] * 3600 + t[4] * 60 + t[5];
    tv->tv_usec = (suseconds_t)(cmm2->micros() % 1000000);
    return 0;
}
