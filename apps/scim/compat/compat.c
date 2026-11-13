/* compat.c: what sc-im calls that a loadable program has not (apps/scim/compat). BSD 4-clause, as sc-im.

   A program cannot start another, so running a shell command fails and says so: popen gives NULL (an @ext function:
   "running ... failed"), and dup and execl, which sc-im calls only in the child after a fork (which always fails),
   fail too. There are no user accounts: getpwnam finds none (~user in a file name stays as it is). realpath makes a
   name whole from the current directory. select says the console can be read (sc-im's --nocurses reads its commands
   from it), and set_escdelay is ncurses's, which PDCurses has not: Esc is a key of its own here, with no delay.
   __collate_load_error is newlib's regex's (regex/: no collation table, so ranges are by the byte). */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <sys/select.h>

int __collate_load_error = 1;

FILE *popen(const char *command, const char *mode) {
    (void)command;
    (void)mode;
    errno = ENOSYS;
    return NULL;
}

int pclose(FILE *stream) {
    (void)stream;
    errno = ECHILD;
    return -1;
}

int dup(int fd) {
    (void)fd;
    errno = ENOSYS;
    return -1;
}

int execl(const char *path, const char *arg, ...) {
    (void)path;
    (void)arg;
    errno = ENOSYS;
    return -1;
}

struct passwd *getpwnam(const char *name) {
    (void)name;
    return NULL;
}

/* name made whole: from the current directory if it has no drive and does not start with /; . and .. are left to
   the firmware, which takes them */
char *realpath(const char *name, char *resolved) {
    char *out = resolved ? resolved : malloc(PATH_MAX);
    if(out == NULL) return NULL;
    if((name[0] && name[1] == ':') || name[0] == '/' || name[0] == '\\') {
        if(strlen(name) >= PATH_MAX) goto toolong;
        strcpy(out, name);
    } else {
        if(getcwd(out, PATH_MAX) == NULL) goto fail;
        if(strlen(out) + 1 + strlen(name) >= PATH_MAX) goto toolong;
        if(out[0] && out[strlen(out) - 1] != '/') strcat(out, "/");
        strcat(out, name);
    }
    return out;
toolong:
    errno = ENAMETOOLONG;
fail:
    if(resolved == NULL) free(out);
    return NULL;
}

/* the console (fd 0) is always ready: a read waits for its key */
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    int n = 0, fd;
    (void)timeout;
    for(fd = 0; fd < nfds; fd++) {
        if(readfds && FD_ISSET(fd, readfds)) {
            if(fd == 0) n++;
            else FD_CLR(fd, readfds);
        }
        if(writefds && FD_ISSET(fd, writefds)) n++;
    }
    if(exceptfds) FD_ZERO(exceptfds);
    return n;
}

int set_escdelay(int ms) {
    (void)ms;
    return 0;
}
