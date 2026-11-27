/* cmm2_stdio.c: fopen and fdopen with a 16 KB buffer (the link's --wrap=fopen and --wrap=fdopen, make.py), for this app
   alone. newlib-nano gives each file BUFSIZ, 1 KB, so a download reached the card 1 KB at a time, each a FatFs write of
   its own: 164 KB/s (27 Nov 2026). curl's output files and mbedTLS's read of the CA bundle both go through these.
   MIT License, as cmm2img.h. */
#include <stdio.h>

#define FILE_BUF 16384

FILE *__real_fopen(const char *name, const char *mode);
FILE *__real_fdopen(int fd, const char *mode);

static FILE *buffered(FILE *f) {
    if(f) setvbuf(f, NULL, _IOFBF, FILE_BUF);   /* (before any read or write, as setvbuf must be) */
    return f;
}

FILE *__wrap_fopen(const char *name, const char *mode) {
    return buffered(__real_fopen(name, mode));
}

FILE *__wrap_fdopen(int fd, const char *mode) {
    FILE *f = __real_fdopen(fd, mode);
    if(f && fd > 2) buffered(f);                /* (not the console's descriptors) */
    return f;
}
