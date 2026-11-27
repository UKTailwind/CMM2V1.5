/* iconv.c: iconv for apps/browser. newlib declares it (iconv.h) but newlib-nano, as tools/sdk builds programs, has none.
   NetSurf converts with it from UTF-8 for forms (in the page's own character set, else ISO-8859-1//TRANSLIT), for text
   saved as HTML (an escape for each character the set lacks: EILSEQ, character by character) and for the internal
   font's CP1252. Modern pages are UTF-8 throughout, so the sets here are the older pages' ones:
       UTF-8, US-ASCII, ISO-8859-1 (Latin-1), ISO-8859-15 (Latin-9), Windows-1252 (CP1252)
   with //TRANSLIT ('?' for a character the target lacks) and //IGNORE (it is left out). Any other set: iconv_open
   fails with EINVAL, and NetSurf falls back as it does when a system's iconv lacks a set. MIT License, as
   Inc/cmm2img.h. */
#include <errno.h>
#include <iconv.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

enum { E_UTF8, E_ASCII, E_LATIN1, E_LATIN9, E_CP1252 };

typedef struct {
    int from, to, translit, ignore;
} conv_t;

static const struct { const char *name; int enc; } names[] = {
    {"UTF-8", E_UTF8}, {"UTF8", E_UTF8},
    {"US-ASCII", E_ASCII}, {"ASCII", E_ASCII}, {"ANSI_X3.4-1968", E_ASCII}, {"US", E_ASCII},
    {"ISO-8859-1", E_LATIN1}, {"ISO8859-1", E_LATIN1}, {"ISO_8859-1", E_LATIN1}, {"LATIN1", E_LATIN1}, {"L1", E_LATIN1},
    {"ISO-8859-15", E_LATIN9}, {"ISO8859-15", E_LATIN9}, {"ISO_8859-15", E_LATIN9}, {"LATIN9", E_LATIN9},
    {"WINDOWS-1252", E_CP1252}, {"WINDOWS1252", E_CP1252}, {"CP1252", E_CP1252},
};

/* Windows-1252's 0x80-0x9F (0: not used) */
static const uint16_t cp1252_hi[32] = {
    0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
    0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178,
};

/* ISO-8859-15's eight changes from Latin-1 */
static const struct { uint8_t byte; uint16_t uc; } latin9[8] = {
    {0xA4, 0x20AC}, {0xA6, 0x0160}, {0xA8, 0x0161}, {0xB4, 0x017D}, {0xB8, 0x017E}, {0xBC, 0x0152}, {0xBD, 0x0153},
    {0xBE, 0x0178},
};

/* the name's set and its flags, or -1 */
static int lookup(const char *name, int *translit, int *ignore) {
    char buf[40];
    size_t i, n;
    const char *suffix = strstr(name, "//");
    n = suffix ? (size_t)(suffix - name) : strlen(name);
    if(n >= sizeof(buf)) return -1;
    for(i = 0; i < n; i++) buf[i] = (char)toupper((unsigned char)name[i]);
    buf[n] = 0;
    if(suffix) {
        if(strstr(suffix, "TRANSLIT") || strstr(suffix, "translit")) *translit = 1;
        if(strstr(suffix, "IGNORE") || strstr(suffix, "ignore")) *ignore = 1;
    }
    for(i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        if(!strcmp(buf, names[i].name)) return names[i].enc;
    return -1;
}

iconv_t iconv_open(const char *to, const char *from) {
    conv_t *c;
    int t = 0, ig = 0, f, d;
    f = lookup(from, &t, &ig);
    d = lookup(to, &t, &ig);
    if(f < 0 || d < 0) {
        errno = EINVAL;
        return (iconv_t)-1;
    }
    if((c = malloc(sizeof(*c))) == NULL) {
        errno = ENOMEM;
        return (iconv_t)-1;
    }
    c->from = f;
    c->to = d;
    c->translit = t;
    c->ignore = ig;
    return (iconv_t)c;
}

int iconv_close(iconv_t cd) {
    free(cd);
    return 0;
}

/* one character from p (n bytes there): its length, 0 if incomplete, -1 if not valid */
static int decode(int enc, const uint8_t *p, size_t n, uint32_t *uc) {
    int len, i;
    uint32_t c;
    if(n == 0) return 0;
    c = p[0];
    switch(enc) {
    case E_UTF8:
        if(c < 0x80) { *uc = c; return 1; }
        if(c < 0xC2) return -1;
        else if(c < 0xE0) { len = 2; c &= 0x1F; }
        else if(c < 0xF0) { len = 3; c &= 0x0F; }
        else if(c < 0xF5) { len = 4; c &= 0x07; }
        else return -1;
        for(i = 1; i < len; i++) {
            if((size_t)i >= n) return 0;
            if((p[i] & 0xC0) != 0x80) return -1;
            c = c << 6 | (p[i] & 0x3F);
        }
        if((len == 3 && c < 0x800) || (len == 4 && (c < 0x10000 || c > 0x10FFFF)) || (c >= 0xD800 && c <= 0xDFFF))
            return -1;
        *uc = c;
        return len;
    case E_ASCII:
        if(c >= 0x80) return -1;
        *uc = c;
        return 1;
    case E_LATIN9:
        for(i = 0; i < 8; i++) if(latin9[i].byte == c) { *uc = latin9[i].uc; return 1; }
        *uc = c;
        return 1;
    case E_CP1252:
        if(c >= 0x80 && c < 0xA0) {
            if(!cp1252_hi[c - 0x80]) return -1;
            c = cp1252_hi[c - 0x80];
        }
        *uc = c;
        return 1;
    default:                                    /* (Latin-1) */
        *uc = c;
        return 1;
    }
}

/* the character into q (n bytes free): its length, 0 if no room, -1 if the set lacks it */
static int encode(int enc, uint32_t uc, uint8_t *q, size_t n) {
    int i;
    switch(enc) {
    case E_UTF8:
        if(uc < 0x80) { if(n < 1) return 0; q[0] = (uint8_t)uc; return 1; }
        if(uc < 0x800) { if(n < 2) return 0; q[0] = 0xC0 | uc >> 6; q[1] = 0x80 | (uc & 0x3F); return 2; }
        if(uc < 0x10000) {
            if(n < 3) return 0;
            q[0] = 0xE0 | uc >> 12; q[1] = 0x80 | (uc >> 6 & 0x3F); q[2] = 0x80 | (uc & 0x3F);
            return 3;
        }
        if(n < 4) return 0;
        q[0] = 0xF0 | uc >> 18; q[1] = 0x80 | (uc >> 12 & 0x3F); q[2] = 0x80 | (uc >> 6 & 0x3F); q[3] = 0x80 | (uc & 0x3F);
        return 4;
    case E_ASCII:
        if(uc >= 0x80) return -1;
        break;
    case E_LATIN9:
        for(i = 0; i < 8; i++) {
            if(latin9[i].uc == uc) { if(n < 1) return 0; q[0] = latin9[i].byte; return 1; }
            if(latin9[i].byte == uc) return -1;     /* (the Latin-1 characters it replaced) */
        }
        if(uc > 0xFF) return -1;
        break;
    case E_CP1252:
        if(uc >= 0x80 && uc < 0xA0) return -1;  /* (C1 controls: Windows-1252 has letters there) */
        if(uc > 0xFF) {
            for(i = 0; i < 32; i++)
                if(cp1252_hi[i] == uc) { if(n < 1) return 0; q[0] = (uint8_t)(0x80 + i); return 1; }
            return -1;
        }
        break;
    default:                                    /* (Latin-1) */
        if(uc > 0xFF) return -1;
        break;
    }
    if(n < 1) return 0;
    q[0] = (uint8_t)uc;
    return 1;
}

size_t iconv(iconv_t cd, char **__restrict inbuf, size_t *__restrict inleft, char **__restrict outbuf,
             size_t *__restrict outleft) {
    conv_t *c = (conv_t *)cd;
    size_t irreversible = 0;
    if(inbuf == NULL || *inbuf == NULL) return 0;   /* (no shift state to reset) */
    while(*inleft > 0) {
        uint32_t uc;
        int k = decode(c->from, (const uint8_t *)*inbuf, *inleft, &uc), m;
        if(k == 0) { errno = EINVAL; return (size_t)-1; }
        if(k < 0) {
            if(c->ignore) { (*inbuf)++; (*inleft)--; continue; }
            errno = EILSEQ;
            return (size_t)-1;
        }
        m = encode(c->to, uc, (uint8_t *)*outbuf, *outleft);
        if(m < 0) {
            if(c->ignore) { *inbuf += k; *inleft -= k; irreversible++; continue; }
            if(!c->translit) { errno = EILSEQ; return (size_t)-1; }
            m = encode(c->to, '?', (uint8_t *)*outbuf, *outleft);
            irreversible++;
        }
        if(m == 0) { errno = E2BIG; return (size_t)-1; }
        *outbuf += m;
        *outleft -= m;
        *inbuf += k;
        *inleft -= k;
    }
    return irreversible;
}
