/* zf_console.c: Forth's console: apps/vt100's terminal on the screen, so that colours, the line editor and zeptoed work there as they do on a terminal at the end of a serial line.
   MIT licence, as zeptoforth.

   What Forth emits goes out raw on the serial port (con_screen(0): con_putc no longer draws), and to the terminal, which draws it on the screen as it stands: the mode and palette
   are left alone, for the graphics words' sake. Keys arrive as the firmware's codes and become what a terminal sends, through vtt_key, so Forth sees one kind of key from a USB
   keyboard and from the serial port, in a queue that the terminal's replies to the host join.

   The terminal answers the questions (ESC [ 6 n, for the cursor), so they are kept from the serial port: a terminal there would answer as well, and the second reply would be typed
   at Forth. It means that the size Forth sees is the screen's.

   If the screen becomes one the terminal cannot draw on, a mode of 1 or 4 bits, the firmware's console draws, with colours off, and the terminal goes on reading and answering, frozen,
   until there is a screen it can use. With no display at all at the start, the firmware's console is all there is.

   All of it runs in the thread, in C, as zf_cmm2.c's calls do: the tick only notes that time has passed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"
#include "vt_term.h"

#define OUT_MAX    1024                         /* bytes from Forth, given to the terminal together */
#define KEYS_MAX   1024
#define DRAW_US    16000                        /* a frame's worth, however fast the text comes */
#define ASKED_US   100000                       /* a key looked for this recently: the cursor is shown */
#define MIN_COLS   80
#define MIN_ROWS   24
#define SCROLLBACK 500

int zf_con_live;                                /* 1: the terminal has the screen (port/console.s: colours on) */

static vtt_config cfg = {.font = -1, .charset = VTT_UTF8, .colour = VTT_WHITE, .cursor = 1, .backspace_del = 1, .scrollback = SCROLLBACK, .home_xterm = 1};
static uint32_t screen_w, screen_h, screen_bpp; /* the mode the terminal is on */
static int detached;                            /* the firmware's console is drawing, as the terminal cannot */
static int shown = 1;                           /* console-screen */
static char pending[OUT_MAX];
static int npending;
static uint8_t keys[KEYS_MAX];
static int khead, ktail;
static int held = -1;
static int seq;                                 /* how much of the question has been held back from the serial port */
static uint64_t drawn, asked;
static int undrawn;                             /* something emitted is not on the screen yet */
static const char question[] = "\033[6n";

static void to_forth(const char *s, size_t n) {
    while(n--) {
        int next = (khead + 1) % KEYS_MAX;
        if(next == ktail) return;
        keys[khead] = *s++;
        khead = next;
    }
}

static void flush_out(void) {
    if(npending) vtt_write(pending, npending);
    npending = 0;
}

/* the font for a screen, if none has been chosen: the largest that gives a screen of 80 x 24, or the smallest. Not 8x16: 8x14 has the same width and more lines */
static int font_for(uint32_t w, uint32_t h) {
    static const int sizes[] = {5, 4, 2, 1, 0};
    int i;
    for(i = 0; i < 5; i++) {
        const vtt_font *f = &vtt_fonts[sizes[i]];
        if((int)w / f->w >= MIN_COLS && (int)h / f->h >= MIN_ROWS) return sizes[i];
    }
    return 0;
}

/* the screen as gfx_target has it. MODE n,12 is 16 bits to gfx_target, in another format. At the start that and a mode of 1 or 4 bits have 8 bits set in their place */
static int attach(int starting) {
    cmm2_target t;
    cmm2_video v;
    vtt_config c = cfg;
    char mode[40];
    int twelve;
    if(cmm2->gfx_target(&t) < 0) return -1;
    screen_w = t.width;
    screen_h = t.height;
    screen_bpp = t.bpp;
    twelve = t.bpp == 16 && cmm2->basic_eval("STR$(MM.INFO(MODE))", mode) >= 0 && strstr(mode, ".12");
    if(!starting && (twelve || (t.bpp != 8 && t.bpp != 16))) return -1;
    if(twelve || (t.bpp != 8 && t.bpp != 16)) {
        if(!twelve && cmm2->basic_eval("STR$(MM.INFO(MODE))", mode) < 0) return -1;
        if(cmm2->video_mode(atoi(mode), 8, &v) < 0) return -1;
        screen_bpp = 8;
    } else {
        v.fb = t.fb;
        v.width = t.width;
        v.height = t.height;
        v.pitch = t.pitch;
        v.bpp = t.bpp;
        v.copies = t.pitch / (t.width * (t.bpp / 8));
    }
    if(c.font < 0) c.font = font_for(v.width, v.height);
    screen_w = v.width;
    screen_h = v.height;
    return vtt_attach(&c, &v, to_forth);
}

/* the terminal on the screen as it now is, or the firmware's console if it cannot be: it comes back when it can */
static int place(void) {
    extern const uint32_t zf_mem[5];
    int r;
    vtt_freeze(!shown);
    r = attach(0);
    if(r < 0 && !detached) {
        detached = 1;
        vtt_freeze(1);
        *(uint32_t *)zf_mem[4] = 0;
        cmm2->con_screen(shown);
    } else if(r >= 0 && detached) {
        detached = 0;
        *(uint32_t *)zf_mem[4] = -1;
        cmm2->con_screen(0);
    }
    return r;
}

/* PLAY TONE: it says nothing if sound is busy, which a bell can bear */
static void bell(void) {
    cmm2->basic_cmd("PLAY TONE 790,790,150");
}

void zf_con_open(void) {
    vtt_bell_hook = bell;
    if(attach(1) < 0) return;
    cmm2->con_screen(0);
    zf_con_live = 1;
}

void zf_con_close(void) {
    if(!zf_con_live) return;
    flush_out();
    zf_con_live = 0;
    vtt_close();
    cmm2->con_screen(1);
}

static void serial_out(int c) {
    int i;
    if(c == question[seq]) {
        if(++seq == 4) seq = 0;
        return;
    }
    for(i = 0; i < seq; i++) cmm2->con_putc(question[i]);
    seq = 0;
    if(c == question[0]) seq = 1;
    else cmm2->con_putc(c);
}

void zf_con_emit(int c) {
    if(!zf_con_live) {
        cmm2->con_putc(c);
        return;
    }
    serial_out(c);
    pending[npending++] = c;
    undrawn = 1;
    if(npending == OUT_MAX) flush_out();
}

/* before any service: what has been printed is drawn first, so that text and graphics reach the screen in the order the program made them, and a program can read back what it
   printed. It also keeps the drawing out of a service that is timing something */
void zf_con_sync(void) {
    if(!undrawn) return;
    undrawn = 0;
    flush_out();
    if(shown && !detached) vtt_update();
}

/* The next key from the firmware, or -1. A CMM2 keyboard's Enter is queued as CR and LF together for BASIC: the LF waiting behind a CR is dropped, as vt100.c drops it */
static int key_get(void) {
    int k = held, next;
    if(k >= 0) held = -1;
    else k = cmm2->con_getc();
    if(k == 13 && (next = cmm2->con_getc()) != 10) held = next;
    return k;
}

/* The firmware applies Ctrl to the letters and \ only, so Ctrl-Space comes as a space and Ctrl-? as a ?. A terminal sends control codes for them, and zeptoed selects with the one and
   gives its help with the other. mods is keys_held's: 2 and 32 are the two Ctrls */
static int with_ctrl(int k, int mods) {
    if(!(mods & (2 | 32))) return k;
    switch(k) {
    case ' ': case '@': return 0;
    case '[': return 27;
    case ']': return 29;
    case '^': return 30;
    case '_': case '/': case '?': return 31;
    default: return k;
    }
}

/* the next byte for Forth, or -1 */
int zf_con_key(void) {
    int c;
    if(!zf_con_live) return cmm2->con_getc();
    asked = cmm2->micros();
    flush_out();                                /* (a question emitted just before this is answered now) */
    if(khead == ktail) {
        int k, held_keys[7];
        while(khead == ktail && (k = key_get()) >= 0) {
            cmm2->keys_held(held_keys);
            k = with_ctrl(k, held_keys[6]);
            if(k == 0) to_forth("", 1);         /* (vtt_key has no NUL) */
            else vtt_key(k, held_keys[6]);
        }
    }
    if(khead == ktail) return -1;
    c = keys[ktail];
    ktail = (ktail + 1) % KEYS_MAX;
    return c;
}

/* every few milliseconds, from zf_cmm2.c's look_after. A mode set by something that does not say so (a MODE through basic) is noticed here */
void zf_con_service(void) {
    cmm2_target t;
    uint64_t now;
    if(!zf_con_live) return;
    if(shown && !detached) vtt_mouse();
    now = cmm2->micros();
    if(now - drawn < DRAW_US) return;
    drawn = now;
    flush_out();
    undrawn = 0;
    if(cmm2->gfx_target(&t) == 0 && (t.width != screen_w || t.height != screen_h || t.bpp != screen_bpp)) place();
    if(shown && !detached) {
        vtt_cursor_show(now - asked < ASKED_US);
        vtt_update();
    }
}

/* the screen's words, which have changed what the terminal is drawn on */

void zf_con_mode(void) {
    if(!zf_con_live) return;
    flush_out();
    vtt_write("\033[H\033[2J", 7);              /* (a new mode is a clear screen) */
    place();
}

static int sgr(char *b, int size, int which, uint32_t colour) {
    return snprintf(b, size, "\033[%d;2;%lu;%lu;%lum", which, (unsigned long)(colour >> 16 & 255), (unsigned long)(colour >> 8 & 255), (unsigned long)(colour & 255));
}

static uint32_t con_bg;

void zf_con_colour(uint32_t fg, uint32_t bg) {
    char b[64];
    int n;
    if(!zf_con_live) return;
    flush_out();
    con_bg = bg;
    n = sgr(b, sizeof(b), 38, fg);
    n += sgr(b + n, sizeof(b) - n, 48, bg);
    vtt_write(b, n);
}

void zf_con_cls(uint32_t colour) {
    char b[80];
    int n;
    if(!zf_con_live) return;
    flush_out();
    n = sgr(b, sizeof(b), 48, colour);
    n += snprintf(b + n, sizeof(b) - n, "\033[2J\033[H");
    n += sgr(b + n, sizeof(b) - n, 48, con_bg);
    vtt_write(b, n);
}

/* console-screen: 0 leaves the screen to a graphics program. The terminal goes on reading, so that a question is still answered, and what is printed meanwhile is not drawn */
void zf_con_shown(int on) {
    if(!zf_con_live || detached) cmm2->con_screen(on);
    shown = on;
    if(!zf_con_live) return;
    if(!on) vtt_cursor_show(0);
    if(!detached) vtt_freeze(!on);
}

/* the font, as an index into vtt_fonts, or -1 for the screen's choice */
int zf_con_font(int font) {
    if(font < -1 || font >= vtt_font_count) return -1;
    if(!zf_con_live) return -1;
    flush_out();
    cfg.font = font;
    return place();
}

/* the size, 0 for all there is */
int zf_con_size(int cols, int rows) {
    if(!zf_con_live) return -1;
    flush_out();
    cfg.cols = cols;
    cfg.rows = rows;
    return place();
}

/* reboot, and Ctrl-C: a program may have left the terminal anywhere, in an editor's second screen with the cursor off. The text stays, and the picture goes */
void zf_con_reset(void) {
    if(!zf_con_live) return;
    flush_out();
    khead = ktail = 0;
    held = -1;
    seq = 0;
    shown = 1;
    vtt_freeze(detached);
    if(detached) cmm2->con_screen(1);
    vtt_reset(0);
    if(!detached) cmm2->con_puts("\033[m\033[?25h");
}
