/* posix.c: two POSIX calls NetSurf makes that newlib-nano and tools/sdk/syscalls.c do not give (apps/browser).
     realpath   utils/filepath.c finds a resource by it: NULL (ENOENT) when the file is not there, else its whole name.
                On the current drive that is a name from its root with no drive, "/browser/res/default.css", as on
                a Unix: NetSurf turns a name into a file: URL and back, and "A:" came back as "/A:/...", which
                MMBasic does not take (27 Nov 2026: the style sheets failed to load). Another drive's name keeps
                its "B:".
     scandir    content/fetchers/file/file.c lists a folder for a file: URL with it.
   MIT License, as Inc/cmm2img.h. */
#include <sys/types.h>                          /* (first: newlib's dirent.h wants ssize_t) */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

char *realpath(const char *path, char *resolved) {
    char cwd[264], *out = resolved;
    struct stat st;
    size_t n;
    if(path == NULL || *path == 0) {
        errno = EINVAL;
        return NULL;
    }
    if(stat(path, &st) != 0) return NULL;       /* (stat set errno) */
    if(out == NULL && (out = malloc(PATH_MAX)) == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if(getcwd(cwd, sizeof(cwd)) == NULL || cwd[1] != ':') strcpy(cwd, "A:/");
    if(path[0] && path[1] == ':') {
        if((path[0] | 0x20) == (cwd[0] | 0x20)) path += 2;   /* (the current drive's: from its root, no drive) */
        if(path[0] && path[1] == ':') {                     /* (another drive's: kept as it is) */
            n = strlen(path);
            if(n >= PATH_MAX) goto toolong;
            memcpy(out, path, n + 1);
            return out;
        }
    }
    out[0] = 0;
    if(path[0] != '/' && path[0] != '\\') {                 /* (relative: the current directory's, less its drive) */
        if(strlen(cwd + 2) + 1 + strlen(path) >= PATH_MAX) goto toolong;
        strcpy(out, cwd + 2);
        n = strlen(out);
        if(n == 0 || out[n - 1] != '/') strcat(out, "/");
    } else if(strlen(path) >= PATH_MAX)
        goto toolong;
    strcat(out, path);
    for(n = 0; out[n]; n++) if(out[n] == '\\') out[n] = '/';
    return out;
toolong:
    if(out != resolved) free(out);
    errno = ENAMETOOLONG;
    return NULL;
}

int scandir(const char *dir, struct dirent ***namelist, int (*sel)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **)) {
    DIR *d = opendir(dir);
    struct dirent *e, **list = NULL, **grown;
    int n = 0, cap = 0;
    if(d == NULL) return -1;
    while((e = readdir(d)) != NULL) {
        if(sel && !sel(e)) continue;
        if(n == cap) {
            cap = cap ? cap * 2 : 32;
            if((grown = realloc(list, cap * sizeof(*list))) == NULL) goto nomem;
            list = grown;
        }
        if((list[n] = malloc(sizeof(struct dirent))) == NULL) goto nomem;
        memcpy(list[n++], e, sizeof(struct dirent));
    }
    closedir(d);
    if(compar && n > 1) qsort(list, n, sizeof(*list), (int (*)(const void *, const void *))compar);
    *namelist = list;
    return n;
nomem:
    while(n) free(list[--n]);
    free(list);
    closedir(d);
    errno = ENOMEM;
    return -1;
}
