/* vt100.c: the CMM2 as a terminal, for a PicoMite or anything else on a serial line. vt_term.c is the screen; this file is the line (COM1, COM2,
   and from version 14 of the services COM4-COM7, the USB serial devices), the setup menu, XMODEM, sending a text file and logging.

     RUN "vt100.app" [, COM2 115200 ...]

   The words after RUN's comma: comN, a number (the speed), and any setting as name=value, as vt100.cfg has them. vt100.cfg is read from the
   current directory first. Shift-F12 or Pause is the setup menu: all twelve function keys go to the host, as MMBasic's editor uses them, and
   Ctrl-C goes too, so the only way out is the menu's Quit. MIT License, as cmm2img.h. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "cmm2.h"
#include "vt_term.h"

#define RXBUF 16384
#define KEY_SETUP 0xBC                                  /* Shift-F12 */
#define KEY_PAUSE 0x9E
#define CFGFILE "vt100.cfg"
#define K_UP 0x80
#define K_DOWN 0x81
#define K_LEFT 0x82
#define K_RIGHT 0x83
#define K_PUP 0x88
#define K_PDOWN 0x89
#define K_HOME 0x86
#define K_END 0x87

/* colours of the menu, as xterm's numbers */
#define C_BOX 252
#define C_TEXT 234
#define C_SEL 25
#define C_SELT 15
#define C_DIM 240

typedef struct {
    int port, baud, format;
    int mode, font, cols, rows, status;
    int charset, colour, bsdel, entercrlf, lfcrlf, echo, linedelay, scrollback, cursor;
} settings;

static settings set = {.port = 1, .baud = 115200, .format = 0, .mode = 1, .font = 2, .cols = 0, .rows = 0, .status = 1, .charset = VTT_CP437,
                       .colour = VTT_WHITE, .linedelay = 5, .scrollback = 2000, .cursor = 1};

static const int bauds[] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
#define NBAUDS (int)(sizeof(bauds) / sizeof(bauds[0]))
static const struct { const char *name; int bits, parity, stop; } formats[] = {{"8N1", 8, 0, 1}, {"8E1", 8, 1, 1}, {"8O1", 8, 2, 1}, {"7E1", 7, 1, 1},
    {"7O1", 7, 2, 1}, {"8N2", 8, 0, 2}};
#define NFORMATS (int)(sizeof(formats) / sizeof(formats[0]))
static const struct { int mode; const char *name; } modes[] = {{2, "640x400"}, {8, "640x480"}, {1, "800x600"}, {10, "848x480"}, {14, "960x540"}, {18, "1024x600"},
    {9, "1024x768"}, {11, "1280x720"}, {15, "1280x1024"}, {16, "1920x1080"}};
#define NMODES (int)(sizeof(modes) / sizeof(modes[0]))
static const int delays[] = {0, 1, 2, 5, 10, 20, 50, 100, 200};
#define NDELAYS (int)(sizeof(delays) / sizeof(delays[0]))

static int open_com;                                    /* the COM port open, 0 none, -1 the console (for tests) */
static char line_error[64];
static int line_gone;                                   /* the open port's USB device is away (services 15): the port stays open, and
                                                           the firmware sets the line up again when it comes back */
static int quit;
static int logfile = -1, textfile = -1;
static uint64_t textnext;
static char logname[256];
static int in_key;

/* The next key, or -1. A CMM2 keyboard's Enter is queued as CR and LF together for BASIC (sendCRLF): it was sent twice, and a
   menu's choice made twice. The LF waiting behind a CR is dropped; one that comes later is a key of its own (Ctrl-J). */
static int key_get(void) {
    static int held = -1;
    int k = held, next;
    if(k >= 0) held = -1;
    else k = cmm2->con_getc();
    if(k == 13 && (next = cmm2->con_getc()) != 10) held = next;
    return k;
}

/* the line */
static void line_close(void) {
    if(open_com > 0) cmm2->com_close(open_com);
    open_com = 0;
}

static int line_open(void) {
    int r;
    line_close();
    line_error[0] = 0;
    line_gone = 0;
    if(set.port == 0) {                                 /* the serial console as the line, for testing from the PC */
        cmm2->con_screen(0);
        open_com = -1;
        return 0;
    }
    if(set.port >= 4 && cmm2->version < 14) {
        snprintf(line_error, sizeof(line_error), "USB serial needs newer firmware");
        return -1;
    }
    r = cmm2->com_open(set.port, set.baud, formats[set.format].bits, formats[set.format].parity, formats[set.format].stop, RXBUF);
    if(r < 0) {
        snprintf(line_error, sizeof(line_error), "%s", r == -4 ? cmm2->io_error() : "can't open it");
        return r;
    }
    open_com = set.port;
    return 0;
}

static int line_read(uint8_t *buf, int n) {
    int i = 0, c;
    if(open_com > 0) return cmm2->com_read(open_com, buf, n);
    if(open_com < 0) while(i < n && (c = cmm2->con_getc()) >= 0) buf[i++] = c;
    return i;
}

static void line_write(const uint8_t *buf, int n) {
    int i;
    if(open_com > 0) cmm2->com_write(open_com, buf, n);
    else if(open_com < 0) for(i = 0; i < n; i++) cmm2->con_putc(buf[i]);
}

static int line_getc(uint32_t ms) {                     /* a byte, or -1 after ms, or -2 if Esc or Ctrl-C was pressed */
    uint64_t end = cmm2->micros() + (uint64_t)ms * 1000;
    uint8_t c;
    do {
        cmm2->poll();
        if(cmm2->break_take()) return -2;
        if(open_com >= 0 && key_get() == 27) return -2;
        if(line_read(&c, 1) == 1) return c;
    } while(cmm2->micros() < end);
    return -1;
}

/* the terminal's output: the keys, and its replies to the host */
static void term_out(const char *s, size_t n) {
    size_t i;
    if(in_key && set.entercrlf) {
        for(i = 0; i < n; i++) {
            line_write((const uint8_t *)s + i, 1);
            if(s[i] == '\r') line_write((const uint8_t *)"\n", 1);
        }
    } else line_write((const uint8_t *)s, n);
    if(in_key && set.echo) vtt_write(s, n);
}

static void status(void) {
    char b[160];
    int c, r;
    vtt_size(&c, &r);
    if(line_error[0]) snprintf(b, sizeof(b), " COM%d: %s  |  Shift-F12 setup", set.port, line_error);
    else if(line_gone) snprintf(b, sizeof(b), " COM%d disconnected, waiting for it to come back  |  Shift-F12 setup", set.port);
    else snprintf(b, sizeof(b), " COM%d %d %s  |  %dx%d  |  %s%sShift-F12 setup", set.port, set.baud, formats[set.format].name, c, r,
                  logfile >= 0 ? "Logging  |  " : "", textfile >= 0 ? "Sending  |  " : "");
    vtt_status(b);
}

/* the menu's drawing */
static int scrc, scrr;

static void fill_box(int col, int row, int w, int h) {
    static char blank[256];
    int i;
    memset(blank, ' ', sizeof(blank) - 1);
    blank[w < 255 ? w : 255] = 0;
    for(i = 0; i < h; i++) vtt_text(col, row + i, blank, C_TEXT, C_BOX);
}

static int waitkey(void) {
    int k;
    for(;;) {
        cmm2->poll();
        if(cmm2->break_take()) return 27;
        if((k = key_get()) >= 0) return k;
    }
}

static void message(const char *title, const char *text) {
    int w = (int)strlen(text) + 4, col, row = scrr / 2 - 2;
    if(w < (int)strlen(title) + 4) w = strlen(title) + 4;
    if(w > scrc - 2) w = scrc - 2;
    col = (scrc - w) / 2;
    fill_box(col, row, w, 5);
    vtt_text(col + 2, row + 1, title, C_SEL, C_BOX);
    vtt_text(col + 2, row + 3, text, C_TEXT, C_BOX);
}

static int getname(const char *title, char *name, int max) {
    int n = strlen(name), w = scrc > 64 ? 60 : scrc - 4, col = (scrc - w) / 2, row = scrr / 2 - 2, k;
    char shown[256];
    fill_box(col, row, w, 5);
    vtt_text(col + 2, row + 1, title, C_SEL, C_BOX);
    for(;;) {
        int from = n > w - 6 ? n - (w - 6) : 0;
        snprintf(shown, sizeof(shown), "%-*s", w - 4, name + from);
        vtt_text(col + 2, row + 3, shown, C_SELT, C_SEL);
        vtt_text(col + 2 + n - from, row + 3, "_", C_SELT, C_SEL);
        k = waitkey();
        if(k == 27) return 0;
        if(k == 13 || k == 10) return n > 0;
        if((k == 8 || k == 127) && n > 0) name[--n] = 0;
        else if(k >= 32 && k < 127 && n < max - 1) {
            name[n++] = k;
            name[n] = 0;
        }
    }
}

/* a file chosen from the directories: returns 1 with its name in out */
#define MAXFILES 512
typedef struct { char name[64]; uint32_t size; uint8_t dir; } entry;
static entry *files;

static int cmp_entry(const void *a, const void *b) {
    const entry *x = a, *y = b;
    if(x->dir != y->dir) return y->dir - x->dir;
    return strcasecmp(x->name, y->name);
}

static int pickfile(const char *title, char *out, int max) {
    char dir[256] = "";
    int n, top = 0, sel = 0, h, w, col, row, i, k, d;
    cmm2_dirent e;
    if(files == NULL && (files = malloc(MAXFILES * sizeof(entry))) == NULL) return 0;
    cmm2->dir_current(dir, sizeof(dir));
    h = scrr - 6 > 20 ? 20 : scrr - 6;
    w = scrc > 64 ? 60 : scrc - 4;
    col = (scrc - w) / 2;
    row = (scrr - h - 3) / 2;
    for(;;) {
        n = 0;
        if(strlen(dir) > 3) {
            strcpy(files[0].name, "..");
            files[0].dir = 1;
            files[0].size = 0;
            n = 1;
        }
        d = cmm2->dir_open(dir);
        if(d >= 0) {
            while(n < MAXFILES && cmm2->dir_read(d, &e) == 1) {
                if(strlen(e.name) >= sizeof(files[0].name)) continue;
                strcpy(files[n].name, e.name);
                files[n].size = e.size;
                files[n].dir = e.is_dir;
                n++;
            }
            cmm2->dir_close(d);
        }
        qsort(files, n, sizeof(entry), cmp_entry);
        top = sel = 0;
        fill_box(col, row, w, h + 3);
        vtt_text(col + 2, row, title, C_SEL, C_BOX);
        vtt_text(col + 2, row + 1, dir, C_DIM, C_BOX);
        for(;;) {
            char b[128];
            for(i = 0; i < h; i++) {
                int f = top + i;
                if(f >= n) snprintf(b, sizeof(b), "%-*s", w - 4, "");
                else if(files[f].dir) snprintf(b, sizeof(b), "%-*.*s <DIR>", w - 10, w - 10, files[f].name);
                else snprintf(b, sizeof(b), "%-*.*s %9lu", w - 14, w - 14, files[f].name, (unsigned long)files[f].size);
                vtt_text(col + 2, row + 2 + i, b, f == sel ? C_SELT : C_TEXT, f == sel ? C_SEL : C_BOX);
            }
            k = waitkey();
            if(k == 27) return 0;
            if(k == K_UP && sel > 0) sel--;
            else if(k == K_DOWN && sel < n - 1) sel++;
            else if(k == K_PUP) sel = sel > h ? sel - h : 0;
            else if(k == K_PDOWN) sel = sel + h < n - 1 ? sel + h : n - 1;
            else if(k == K_HOME) sel = 0;
            else if(k == K_END) sel = n ? n - 1 : 0;
            else if((k == 13 || k == 10) && n) {
                if(files[sel].dir) {
                    if(strcmp(files[sel].name, "..") == 0) {
                        char *p = strrchr(dir, '/');
                        if(p) {
                            if(p - dir <= 2) p[1] = 0;
                            else *p = 0;
                        }
                    } else {
                        if(dir[strlen(dir) - 1] != '/') strcat(dir, "/");
                        strncat(dir, files[sel].name, sizeof(dir) - strlen(dir) - 1);
                    }
                    break;
                }
                snprintf(out, max, "%s%s%s", dir, dir[strlen(dir) - 1] == '/' ? "" : "/", files[sel].name);
                return 1;
            }
            if(sel < top) top = sel;
            if(sel >= top + h) top = sel - h + 1;
        }
    }
}

/* XMODEM: 128-byte blocks, CRC-16 if the receiver asks with 'C' and a checksum if with NAK. MMBasic's asks with NAK and sends what it is asked for */
#define SOH 1
#define EOT 4
#define ACK 6
#define NAK 0x15
#define CAN 0x18
#define XPAD 0x1A

static uint16_t crc16(const uint8_t *p, int n) {
    uint16_t crc = 0;
    int i;
    while(n--) {
        crc ^= (uint16_t)*p++ << 8;
        for(i = 0; i < 8; i++) crc = crc & 0x8000 ? crc << 1 ^ 0x1021 : crc << 1;
    }
    return crc;
}

static void xprogress(const char *what, long bytes, int retries) {
    char b[64];
    snprintf(b, sizeof(b), "%ld bytes, %d retries. Esc stops it", bytes, retries);
    message(what, b);
}

static void xcancel(void) {
    static const uint8_t c[3] = {CAN, CAN, CAN};
    line_write(c, 3);
}

static void flush_line(uint32_t ms) {
    while(line_getc(ms) >= 0) {}
}

static const char *xsend(const char *name) {
    uint8_t pkt[133], block = 1;
    int f = cmm2->file_open(name, CMM2_O_READ), c, n, crc = -1, i, tries, retries = 0, len;
    long sent = 0;
    uint64_t wait;
    if(f < 0) return "Can't open the file";
    message("XMODEM send", "Waiting for the receiver. Esc stops it");
    flush_line(50);
    wait = cmm2->micros() + 60000000;
    while(crc < 0) {
        c = line_getc(1000);
        if(c == -2) { cmm2->file_close(f); xcancel(); return "Stopped"; }
        if(c == 'C') crc = 1;
        else if(c == NAK) crc = 0;
        else if(c == CAN) { cmm2->file_close(f); return "Cancelled by the receiver"; }
        if(cmm2->micros() > wait) { cmm2->file_close(f); return "No receiver"; }
    }
    for(;;) {
        n = cmm2->file_read(f, pkt + 3, 128);
        if(n <= 0) break;
        if(n < 128) memset(pkt + 3 + n, XPAD, 128 - n);
        pkt[0] = SOH;
        pkt[1] = block;
        pkt[2] = ~block;
        if(crc) {
            uint16_t v = crc16(pkt + 3, 128);
            pkt[131] = v >> 8;
            pkt[132] = v;
            len = 133;
        } else {
            uint8_t s = 0;
            for(i = 0; i < 128; i++) s += pkt[3 + i];
            pkt[131] = s;
            len = 132;
        }
        for(tries = 0;; tries++) {
            if(tries == 10) { cmm2->file_close(f); xcancel(); return "Too many retries"; }
            line_write(pkt, len);
            c = line_getc(10000);
            if(c == ACK) break;
            if(c == -2) { cmm2->file_close(f); xcancel(); return "Stopped"; }
            if(c == CAN) { cmm2->file_close(f); return "Cancelled by the receiver"; }
            retries++;
        }
        block++;
        sent += n;
        if((block & 7) == 0) xprogress("XMODEM send", sent, retries);
    }
    cmm2->file_close(f);
    for(tries = 0; tries < 10; tries++) {
        static const uint8_t e = EOT;
        line_write(&e, 1);
        c = line_getc(3000);
        if(c == ACK) return "Sent";
        if(c == -2) return "Stopped";
    }
    return "The file went, but its end was not acknowledged";
}

/* No length is sent, so what ends the last block may be padding or the file's own bytes. If what comes before them is text they
   are padding (Ctrl-Z, or NUL as a PicoMite pads) and they go. Otherwise it is a binary file and all of the block is kept: a few
   bytes too many do no harm, and a .app ending in 00 00 lost them (Peter, 7 Nov 2026). */
static int xlast(const uint8_t *b, int n) {
    int i, keep = n;
    while(keep > 0 && (b[keep - 1] == XPAD || b[keep - 1] == 0)) keep--;
    for(i = 0; i < keep; i++)
        if(b[i] < 32 && b[i] != 9 && b[i] != 10 && b[i] != 12 && b[i] != 13 && b[i] != 27) return n;
    return keep;
}

static const char *xreceive(const char *name) {
    static uint8_t pkt[133], held[128];
    uint8_t expect = 1;
    int f, c, i, usecrc = 1, retries = 0, started = 0, have = 0, tries = 0, len;
    long got = 0;
    f = cmm2->file_open(name, CMM2_O_WRITE | CMM2_O_CREATE);
    if(f < 0) return "Can't create the file";
    message("XMODEM receive", "Waiting for the sender. Esc stops it");
    flush_line(50);
    for(;;) {
        if(!started) {
            static const uint8_t cc = 'C', nk = NAK;
            if(tries >= 3) usecrc = 0;
            if(tries == 30) { cmm2->file_close(f); return "No sender"; }
            line_write(usecrc ? &cc : &nk, 1);
            tries++;
        }
        c = line_getc(started ? 10000 : 3000);
        if(c == -2) { cmm2->file_close(f); xcancel(); return "Stopped"; }
        if(c == -1) {
            if(started) {
                static const uint8_t nk = NAK;
                if(++retries > 10) { cmm2->file_close(f); xcancel(); return "Too many retries"; }
                line_write(&nk, 1);
            }
            continue;
        }
        if(c == CAN) { cmm2->file_close(f); return "Cancelled by the sender"; }
        if(c == EOT) {
            static const uint8_t ak = ACK;
            line_write(&ak, 1);
            have = xlast(held, have);
            if(have) cmm2->file_write(f, held, have);
            got += have;
            cmm2->file_close(f);
            return "Received";
        }
        if(c != SOH) continue;
        started = 1;
        len = usecrc ? 132 : 131;
        pkt[0] = SOH;
        for(i = 1; i <= len; i++) {
            c = line_getc(1000);
            if(c < 0) break;
            pkt[i] = c;
        }
        if(i <= len) {
            static const uint8_t nk = NAK;
            if(c == -2) { cmm2->file_close(f); xcancel(); return "Stopped"; }
            retries++;
            flush_line(100);
            line_write(&nk, 1);
            continue;
        }
        {
            int good = pkt[1] == (uint8_t)~pkt[2];
            static const uint8_t ak = ACK, nk = NAK;
            if(good) {
                if(usecrc) good = crc16(pkt + 3, 128) == (pkt[131] << 8 | pkt[132]);
                else {
                    uint8_t s = 0;
                    for(i = 0; i < 128; i++) s += pkt[3 + i];
                    good = s == pkt[131];
                }
            }
            if(!good) {
                if(++retries > 50) { cmm2->file_close(f); xcancel(); return "Too many retries"; }
                line_write(&nk, 1);
                continue;
            }
            if(pkt[1] == expect) {
                if(have) {                              /* the block before: this one shows that it was not the last */
                    if(cmm2->file_write(f, held, have) != have) { cmm2->file_close(f); xcancel(); return "Can't write the file"; }
                    got += have;
                }
                memcpy(held, pkt + 3, 128);
                have = 128;
                expect++;
                if((expect & 7) == 0) xprogress("XMODEM receive", got, retries);
            }
            line_write(&ak, 1);                         /* (a repeat of the last block, whose ACK was lost, is acknowledged and dropped) */
        }
    }
}

static void result(const char *title, const char *text) {
    char b[120];
    snprintf(b, sizeof(b), "%s. Press a key", text);
    message(title, b);
    waitkey();
}

/* the settings: read and written as name=value, one to a line */
static const struct { const char *name; int *value; } names[] = {{"port", &set.port}, {"baud", &set.baud}, {"format", &set.format}, {"mode", &set.mode},
    {"font", &set.font}, {"cols", &set.cols}, {"rows", &set.rows}, {"status", &set.status}, {"charset", &set.charset}, {"colour", &set.colour}, {"cursor", &set.cursor},
    {"bsdel", &set.bsdel}, {"entercrlf", &set.entercrlf}, {"lfcrlf", &set.lfcrlf}, {"echo", &set.echo}, {"linedelay", &set.linedelay},
    {"scrollback", &set.scrollback}};
#define NNAMES (int)(sizeof(names) / sizeof(names[0]))

static void setting(const char *s) {
    const char *eq = strchr(s, '=');
    int i;
    if(eq == NULL) {
        if(strncasecmp(s, "com", 3) == 0) set.port = atoi(s + 3);
        else if(atoi(s) > 0) set.baud = atoi(s);
        return;
    }
    if(strncasecmp(s, "log=", 4) == 0) {                /* logging from the start */
        snprintf(logname, sizeof(logname), "%s", eq + 1);
        return;
    }
    for(i = 0; i < NNAMES; i++)
        if(strncasecmp(s, names[i].name, eq - s) == 0 && names[i].name[eq - s] == 0) *names[i].value = atoi(eq + 1);
}

static void load(void) {
    static char b[1024];
    int f = cmm2->file_open(CFGFILE, CMM2_O_READ), n;
    char *p, *e;
    if(f < 0) return;
    n = cmm2->file_read(f, b, sizeof(b) - 1);
    cmm2->file_close(f);
    if(n <= 0) return;
    b[n] = 0;
    for(p = b; p && *p; p = e) {
        e = strpbrk(p, "\r\n");
        if(e) while(*e == '\r' || *e == '\n') *e++ = 0;
        setting(p);
    }
}

static int save(void) {
    char b[64];
    int f = cmm2->file_open(CFGFILE, CMM2_O_WRITE | CMM2_O_CREATE), i;
    if(f < 0) return f;
    for(i = 0; i < NNAMES; i++) {
        snprintf(b, sizeof(b), "%s=%d\r\n", names[i].name, *names[i].value);
        cmm2->file_write(f, b, strlen(b));
    }
    cmm2->file_close(f);
    return 0;
}

static void term_config(vtt_config *c) {
    c->mode = set.mode;
    c->font = set.font;
    c->cols = set.cols;
    c->rows = set.rows;
    c->status = set.status;
    c->charset = set.charset;
    c->colour = set.colour;
    c->cursor = set.cursor;
    c->backspace_del = set.bsdel;
    c->scrollback = set.scrollback;
}

/* the screen as the settings have it, or if that mode is refused (a G1 has no MODE 16) as it was */
static int screen_as(vtt_config *c) {
    if(vtt_open(c, NULL) < 0) return -1;
    vtt_screen_cells(&scrc, &scrr);
    return 0;
}

static int apply_screen(void) {
    vtt_config c;
    term_config(&c);
    return screen_as(&c);
}

/* the host's change of size: CSI 8;rows;cols t, which a PicoMite sends at OPTION DISPLAY and at each EDIT and FM, and DECCOLM (rows 0, so the rows stay). A new mode makes the
   monitor lock on again, so this is the last thing tried: the mode set up in the font set up, then in a smaller font, as a VT100's 132 columns are narrower, and only then
   another mode. It lasts until the menu changes the screen, and is not saved */
static const uint16_t size[][2] = {{640, 400}, {640, 480}, {800, 600}, {848, 480}, {960, 540}, {1024, 600}, {1024, 768}, {1280, 720}, {1280, 1024}, {1920, 1080}};

static int resize_to(vtt_config *c, int m, int f, int rows, int cols) {
    if(size[m][0] / vtt_fonts[f].w < cols || size[m][1] / vtt_fonts[f].h < rows + (set.status ? 1 : 0)) return 0;
    c->mode = modes[m].mode;
    c->font = f;
    if(screen_as(c) < 0) return 0;
    status();
    return 1;
}

static void resize_request(int rows, int cols) {
    vtt_config c;
    int f, m, here, nowc, nowr;
    vtt_size(&nowc, &nowr);
    if(rows < 1) rows = nowr;
    if(cols < 1 || (rows == nowr && cols == nowc)) return;
    term_config(&c);
    c.cols = cols;
    c.rows = rows;
    for(here = 0; here < NMODES && modes[here].mode != set.mode; here++) {}
    if(here < NMODES) for(f = set.font; f >= 0; f--) if(resize_to(&c, here, f, rows, cols)) return;
    for(f = set.font; f >= 0; f--) for(m = 0; m < NMODES; m++) if(resize_to(&c, m, f, rows, cols)) return;
}

/* the menu */
enum { M_PORT, M_BAUD, M_FORMAT, M_MODE, M_FONT, M_COLS, M_ROWS, M_STATUS, M_CHARSET, M_COLOUR, M_CURSOR, M_BS, M_ENTER, M_LF, M_ECHO, M_DELAY,
       M_TEXT, M_XSEND, M_XRECV, M_LOG, M_RESET, M_SAVE, M_QUIT, M_COUNT };
static const char *const labels[M_COUNT] = {"Port", "Speed", "Format", "Screen", "Font", "Columns", "Rows", "Status line", "Characters", "Text colour", "Cursor", "Backspace sends",
    "Enter sends", "LF received as", "Local echo", "Text line delay", "Send text file...", "XMODEM send...", "XMODEM receive...", "Log to file...", "Reset the terminal",
    "Save settings", "Quit"};

static void value(int i, char *b, int n) {
    int j;
    b[0] = 0;
    switch(i) {
    case M_PORT:
        if(set.port >= 4) snprintf(b, n, "COM%d (USB)", set.port);
        else snprintf(b, n, "COM%d", set.port);
        break;
    case M_BAUD: snprintf(b, n, "%d", set.baud); break;
    case M_FORMAT: snprintf(b, n, "%s", formats[set.format].name); break;
    case M_MODE:
        for(j = 0; j < NMODES; j++) if(modes[j].mode == set.mode) snprintf(b, n, "MODE %d, %s", set.mode, modes[j].name);
        break;
    case M_FONT: snprintf(b, n, "%s", vtt_fonts[set.font].name); break;
    case M_COLS: if(set.cols) snprintf(b, n, "%d", set.cols); else snprintf(b, n, "All"); break;
    case M_ROWS: if(set.rows) snprintf(b, n, "%d", set.rows); else snprintf(b, n, "All"); break;
    case M_STATUS: snprintf(b, n, "%s", set.status ? "On" : "Off"); break;
    case M_CHARSET: snprintf(b, n, "%s", set.charset == VTT_CP437 ? "CP437" : set.charset == VTT_LATIN1 ? "Latin-1" : "UTF-8"); break;
    case M_COLOUR: snprintf(b, n, "%s", set.colour == VTT_WHITE ? "White" : set.colour == VTT_GREEN ? "Green" : "Amber"); break;
    case M_CURSOR: snprintf(b, n, "%s", set.cursor == 0 ? "Block" : set.cursor == 1 ? "Underline" : "Bar"); break;
    case M_BS: snprintf(b, n, "%s", set.bsdel ? "DEL (127)" : "BS (8)"); break;
    case M_ENTER: snprintf(b, n, "%s", set.entercrlf ? "CR LF" : "CR"); break;
    case M_LF: snprintf(b, n, "%s", set.lfcrlf ? "CR LF" : "LF"); break;
    case M_ECHO: snprintf(b, n, "%s", set.echo ? "On" : "Off"); break;
    case M_DELAY: snprintf(b, n, "%d ms", set.linedelay); break;
    case M_LOG: if(logfile >= 0) snprintf(b, n, "Stop"); break;
    case M_TEXT: if(textfile >= 0) snprintf(b, n, "Stop"); break;
    default: break;
    }
}

static int step(int v, const int *list, int n, int d) {
    int i;
    for(i = 0; i < n; i++) if(list[i] == v) return list[(i + d + n) % n];
    return list[0];
}

static void change(int i, int d) {
    int j;
    switch(i) {
    case M_PORT: {
        static const int ports[] = {1, 2, 4, 5, 6, 7};
        set.port = step(set.port, ports, cmm2->version >= 14 ? 6 : 2, d);
        break;
    }
    case M_BAUD: set.baud = step(set.baud, bauds, NBAUDS, d); break;
    case M_FORMAT: set.format = (set.format + d + NFORMATS) % NFORMATS; break;
    case M_MODE:
        for(j = 0; j < NMODES && modes[j].mode != set.mode; j++) {}
        set.mode = modes[(j + d + NMODES) % NMODES].mode;
        break;
    case M_FONT: set.font = (set.font + d + vtt_font_count) % vtt_font_count; break;
    case M_COLS: {
        static const int c[] = {0, 40, 80, 100, 120, 132, 160};
        set.cols = step(set.cols, c, 7, d);
        break;
    }
    case M_ROWS: {
        static const int r[] = {0, 24, 25, 30, 36, 40, 43, 48, 50, 60};
        set.rows = step(set.rows, r, 10, d);
        break;
    }
    case M_STATUS: set.status = !set.status; break;
    case M_CHARSET: set.charset = (set.charset + d + 3) % 3; break;
    case M_COLOUR: set.colour = (set.colour + d + 3) % 3; break;
    case M_CURSOR: set.cursor = (set.cursor + d + 3) % 3; break;
    case M_BS: set.bsdel = !set.bsdel; break;
    case M_ENTER: set.entercrlf = !set.entercrlf; break;
    case M_LF: set.lfcrlf = !set.lfcrlf; break;
    case M_ECHO: set.echo = !set.echo; break;
    case M_DELAY: set.linedelay = step(set.linedelay, delays, NDELAYS, d); break;
    default: break;
    }
}

static void stoplog(void) {
    if(logfile >= 0) cmm2->file_close(logfile);
    logfile = -1;
}

static void menu(void) {
    static int sel;
    settings old = set;
    char b[100], v[48];
    int i, w = 46, h = M_COUNT + 3, col, row, k, redraw = 1;
    if(scrr < h) h = scrr;
    col = (scrc - w) / 2;
    row = (scrr - h) / 2;
    for(;;) {
        int first = sel > h - 4 ? sel - (h - 4) : 0;
        if(redraw) {
            vtt_update();
            fill_box(col, row, w, h);
            vtt_text(col + 2, row, "Terminal setup: Esc goes back", C_SEL, C_BOX);
            redraw = 0;
        }
        for(i = first; i < M_COUNT && i - first < h - 3; i++) {
            value(i, v, sizeof(v));
            snprintf(b, sizeof(b), " %-20s %-21s", labels[i], v);
            vtt_text(col + 1, row + 2 + i - first, b, i == sel ? C_SELT : C_TEXT, i == sel ? C_SEL : C_BOX);
        }
        k = waitkey();
        if(k == 27 || k == KEY_SETUP || k == KEY_PAUSE) break;
        if(k == K_UP) { sel = (sel + M_COUNT - 1) % M_COUNT; redraw = 1; }
        else if(k == K_DOWN) { sel = (sel + 1) % M_COUNT; redraw = 1; }
        else if(k == K_LEFT) change(sel, -1);
        else if(k == K_RIGHT) change(sel, 1);
        else if(k == 13 || k == 10) {
            static char name[256];
            const char *r;
            if(sel <= M_DELAY) change(sel, 1);
            else if(sel == M_QUIT) { quit = 1; break; }
            else if(sel == M_RESET) { vtt_reset(1); break; }
            else if(sel == M_SAVE) {
                result("Settings", save() == 0 ? "Saved in " CFGFILE : "Can't write " CFGFILE);
                redraw = 1;
            } else if(sel == M_XSEND) {
                if(pickfile("XMODEM send: choose a file", name, sizeof(name))) {
                    r = xsend(name);
                    result("XMODEM send", r);
                }
                redraw = 1;
            } else if(sel == M_XRECV) {
                name[0] = 0;
                if(getname("XMODEM receive: the file's name", name, sizeof(name))) {
                    r = xreceive(name);
                    result("XMODEM receive", r);
                }
                redraw = 1;
            } else if(sel == M_LOG) {
                if(logfile >= 0) stoplog();
                else {
                    strcpy(name, logname[0] ? logname : "vt100.log");
                    if(getname("Log received bytes to (added to its end)", name, sizeof(name))) {
                        logfile = cmm2->file_open(name, CMM2_O_WRITE | CMM2_O_APPEND);
                        if(logfile < 0) result("Log", "Can't open the file");
                        else strcpy(logname, name);
                    }
                }
                redraw = 1;
            } else if(sel == M_TEXT) {
                if(textfile >= 0) {
                    cmm2->file_close(textfile);
                    textfile = -1;
                } else if(pickfile("Send text file: choose a file", name, sizeof(name))) {
                    textfile = cmm2->file_open(name, CMM2_O_READ);
                    if(textfile < 0) result("Send text file", "Can't open the file");
                    else {
                        textnext = 0;
                        break;
                    }
                }
                redraw = 1;
            }
        }
    }
    if(quit) return;
    if(set.mode != old.mode || set.font != old.font || set.cols != old.cols || set.rows != old.rows || set.status != old.status) {
        if(apply_screen() < 0) {
            set.mode = old.mode;
            set.font = old.font;
            set.cols = old.cols;
            set.rows = old.rows;
            set.status = old.status;
            apply_screen();
        }
    } else {
        vtt_config c;
        term_config(&c);
        vtt_set(&c);
        vtt_redraw();
    }
    if(set.port != old.port || set.baud != old.baud || set.format != old.format) line_open();
    status();
}

/* a text file, a line at a time with the delay after each: a PicoMite's AUTOSAVE keeps up at 115200 without it, but not while it is compiling a long line */
static void text_step(void) {
    uint8_t b[256];
    int n, i, cut;
    uint64_t t;
    if(textfile < 0) return;
    t = cmm2->micros();
    if(t < textnext) return;
    n = cmm2->file_read(textfile, b, sizeof(b));
    if(n <= 0) {
        cmm2->file_close(textfile);
        textfile = -1;
        status();
        return;
    }
    cut = n;
    for(i = 0; i < n; i++) if(b[i] == '\n') { cut = i + 1; break; }
    line_write(b, cut);
    if(cut < n) cmm2->file_seek(textfile, cmm2->file_tell(textfile) - (n - cut));
    if(b[cut - 1] == '\n') textnext = t + (uint64_t)set.linedelay * 1000;
}

int main(int argc, char **argv) {
    static uint8_t buf[2048], tr[4096];
    vtt_config c;
    int i, n, k, keys[7];
    uint64_t lastdraw = 0, retry = 0, linecheck = 0;

    if(cmm2->version < 5) {
        printf("this firmware's services are version %lu\n", (unsigned long)cmm2->version);
        return 1;
    }
    load();
    for(i = 1; i < argc; i++) setting(argv[i]);
    if(set.font < 0 || set.font >= vtt_font_count) set.font = 2;
    if(set.format < 0 || set.format >= NFORMATS) set.format = 0;
    term_config(&c);
    if(vtt_open(&c, term_out) < 0) {
        printf("MODE %d,8 can't be set\n", set.mode);
        return 2;
    }
    vtt_screen_cells(&scrc, &scrr);
    vtt_resize_request = resize_request;
    if(logname[0]) logfile = cmm2->file_open(logname, CMM2_O_WRITE | CMM2_O_APPEND);
    line_open();
    status();
    while(!quit) {
        uint64_t t;
        cmm2->poll();
        if(cmm2->break_take()) {                        /* Ctrl-C is the host's (but with the console as the line there is no menu to quit by) */
            static const uint8_t ctrlc = 3;
            if(open_com < 0) break;
            line_write(&ctrlc, 1);
        }
        n = line_read(buf, sizeof(buf));
        if(n > 0) {
            if(logfile >= 0) cmm2->file_write(logfile, buf, n);
            if(set.lfcrlf) {
                int o = 0;
                for(i = 0; i < n; i++) {
                    if(buf[i] == '\n') tr[o++] = '\r';
                    tr[o++] = buf[i];
                }
                vtt_write((char *)tr, o);
            } else vtt_write((char *)buf, n);
        }
        while(open_com >= 0 && (k = key_get()) >= 0) {
            if(k == KEY_SETUP || k == KEY_PAUSE) {
                menu();
                if(quit) break;
                continue;
            }
            cmm2->keys_held(keys);
            in_key = 1;
            vtt_key(k, keys[6]);
            in_key = 0;
        }
        vtt_mouse();
        text_step();
        t = cmm2->micros();
        if(t - lastdraw > 16000) {                      /* a frame's worth: however fast the text comes, it is drawn this often and the scrolls merged */
            vtt_update();
            lastdraw = t;
        }
        if(open_com > 0 && cmm2->version >= 15 && t - linecheck > 100000) {   /* a USB device may go, and come back */
            int gone = cmm2->com_connected(open_com) == 0;
            linecheck = t;
            if(gone != line_gone) {
                line_gone = gone;
                status();
            }
        }
        if(open_com == 0 && t - retry > 2000000) {      /* a USB device may arrive */
            retry = t;
            if(line_open() == 0) status();
        }
    }
    stoplog();
    if(textfile >= 0) cmm2->file_close(textfile);
    line_close();
    vtt_close();
    return 0;
}
