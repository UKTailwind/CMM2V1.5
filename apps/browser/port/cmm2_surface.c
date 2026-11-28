/* cmm2_surface.c: libnsfb's surface for the Colour Maximite 2 (apps/browser, docs/Web_Browser_Study.md), by the name
   "cmm2", so the framebuffer front end's -f cmm2 selects it. libnsfb's surface kinds are a fixed list, so it takes
   SDL's (not built here). Not the RAM surface's: the front end makes each image's bitmap a RAM surface
   (frontends/framebuffer/bitmap.c), and with the screen in that slot every image failed to the red placeholder.
   - NetSurf draws into a buffer in the program's memory, RGB565. update notes each changed rectangle, and when
     NetSurf next waits for input (flush) what it drew goes to the page shown, by the DMA2D, all at once: the page is
     not cached, and drawing there a pixel at a time would be slow.
   - The screen's mode is used as it is (gfx_target: its page, size and depth), never changed (Peter, 27 Nov 2026: a
     mode change, two for each run, is a large event on the SDRAM's bus). It must be of 16-bit colour and the size
     asked for (cmm2_main.c asks for its own). The screen is cleared when NetSurf ends.
   - input pumps the network (services version 21's net_poll: nothing else does while NetSurf waits) and the USB
     devices, for up to its timeout, and gives NetSurf the mouse (MM.MOUSE's, its position kept by the firmware) and
     the keys (con_getc's: the USB keyboard's, and the serial console's with a terminal's escape sequences made into
     MMBasic's codes). Ctrl-C asks NetSurf to quit.
   - The pointer is drawn by libnsfb into the buffer (claim, update and cursor keep it right).
   - A test run (cmm2_main.c's --shot) is finished here once the page is drawn: cmm2_shot_poll.
   MIT License, as Inc/cmm2img.h. */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "libnsfb.h"
#include "libnsfb_plot.h"
#include "libnsfb_plot_util.h"
#include "libnsfb_event.h"
#include "nsfb.h"
#include "surface.h"
#include "plot.h"
#include "cursor.h"
#include "cmm2.h"

int cmm2_shot_poll(void);                       /* cmm2_main.c: 1 once a test run has saved its picture */
void cmm2_memory_poll(void);                    /* cmm2_main.c: a page that ran out of memory stopped */

static cmm2_video v;
uint8_t *volatile cmm2_page_buffer;            /* (NetSurf's buffer, for reading through the ST-Link beside the page) */

static int cmm2_defaults(nsfb_t *nsfb) {
    nsfb->width = 800;
    nsfb->height = 600;
    nsfb->format = NSFB_FMT_RGB565;
    select_plotters(nsfb);
    return 0;
}

static int cmm2_initialise(nsfb_t *nsfb) {
    cmm2_target t;
    uint8_t *p;
    if(nsfb->format != NSFB_FMT_RGB565 || cmm2->version < 21 || cmm2->gfx_target(&t) != 0) return -1;
    if(t.bpp != 16 || t.width != (uint32_t)nsfb->width || t.height != (uint32_t)nsfb->height || t.pitch != t.width * 2)
        return -1;
    v.fb = t.fb;
    v.width = t.width;
    v.height = t.height;
    v.pitch = t.pitch;
    if((p = realloc(nsfb->ptr, (size_t)nsfb->width * nsfb->height * 2)) == NULL) return -1;
    memset(p, 0xFF, (size_t)nsfb->width * nsfb->height * 2);
    nsfb->ptr = p;
    cmm2_page_buffer = p;
    nsfb->linelen = nsfb->width * 2;
    cmm2->con_screen(0);                        /* (the console to serial only: the page is NetSurf's) */
    return 0;
}

static int cmm2_geometry(nsfb_t *nsfb, int width, int height, enum nsfb_format_e format) {
    if(nsfb->ptr != NULL) return -1;            /* (set before initialise only: the mode is chosen there) */
    if(width > 0) nsfb->width = width;
    if(height > 0) nsfb->height = height;
    if(format != NSFB_FMT_ANY) nsfb->format = format;
    select_plotters(nsfb);
    nsfb->linelen = nsfb->width * nsfb->bpp / 8;
    return 0;
}

static int cmm2_finalise(nsfb_t *nsfb) {
    while(cmm2->dma2d_busy()) {}
    cmm2->gfx_cls(0);                           /* (the mode stays: the page cleared for the prompt) */
    free(nsfb->ptr);
    nsfb->ptr = NULL;
    return 0;
}

/* a rectangle from the buffer to the page, by the DMA2D (three copies may wait; more wait here) */
static void to_page(nsfb_t *nsfb, nsfb_bbox_t *box) {
    int x0 = box->x0 < 0 ? 0 : box->x0, y0 = box->y0 < 0 ? 0 : box->y0;
    int x1 = box->x1 > nsfb->width ? nsfb->width : box->x1, y1 = box->y1 > nsfb->height ? nsfb->height : box->y1;
    if(x1 <= x0 || y1 <= y0) return;
    while(cmm2->dma2d_copy(nsfb->ptr + y0 * nsfb->linelen + x0 * 2, nsfb->linelen,
                           v.fb + y0 * v.pitch + x0 * 2, v.pitch, (x1 - x0) * 2, y1 - y0) < 0) {}
}

/* What NetSurf drew: noted, and sent to the page when NetSurf next waits for input (flush), all of it at once. A
   scroll moves what stays visible within the buffer (libnsfb's copy, which updates at once) and only then draws the
   strip it uncovered: sent as they came, the screen showed the moved part beside the strip's old pixels until the
   strip was drawn (28 Nov 2026, the wheel: "screen glitching", Peter). */
static nsfb_bbox_t dirty;
static int dirty_set;

static int cmm2_update(nsfb_t *nsfb, nsfb_bbox_t *box) {
    nsfb_bbox_t u;
    (void)nsfb;
    if(!dirty_set) {
        dirty = *box;
        dirty_set = 1;
    } else {
        nsfb_plot_add_rect(&dirty, box, &u);
        dirty = u;
    }
    return 0;
}

/* the noted rectangle to the page, the pointer put back first if drawing took it off */
static void flush(nsfb_t *nsfb) {
    struct nsfb_cursor_s *c = nsfb->cursor;
    if(!dirty_set) return;
    dirty_set = 0;
    if(c != NULL && !c->plotted) nsfb_cursor_plot(nsfb, c);
    to_page(nsfb, &dirty);
}

/* ---- the mouse and the keys (step 3) ---- */
/* The events found, queued for input to give one at a time: a key is its press and its release, with a Shift round
   it where NetSurf's toolkit (fbtk/event.c's keymaps) makes the character from a key and Shift. */
#define QN 64
static nsfb_event_t q[QN];
static int qh, qt;

static void push(enum nsfb_event_type_e type, int code) {
    if((qt + 1) % QN == qh) return;             /* (full: dropped) */
    q[qt].type = type;
    q[qt].value.keycode = (enum nsfb_key_code_e)code;
    qt = (qt + 1) % QN;
}

static void push_key(int code, int shift) {
    if(shift) push(NSFB_EVENT_KEY_DOWN, NSFB_KEY_LSHIFT);
    push(NSFB_EVENT_KEY_DOWN, code);
    push(NSFB_EVENT_KEY_UP, code);
    if(shift) push(NSFB_EVENT_KEY_UP, NSFB_KEY_LSHIFT);
}

/* The console's keys in MMBasic's codes. The USB keyboard's arrive so already (its driver queues them); a terminal on
   the serial console sends escape sequences for its arrows, Home, End, Insert, Delete, Page Up and Down and function
   keys, which con_getc gives as they came. They are made into the same codes here, as MMInkey makes them (main.c):
   the byte after ESC waited for up to 30 ms, the whole sequence up to 90 ms. ESC [ H and F, and ESC O with an arrow,
   Home or End, which some terminals send, too. Bytes that make no sequence come through as they were, ESC first. */
static int held[4], nheld;

static int con_byte(uint64_t until) {
    int c;
    while((c = cmm2->con_getc()) < 0 && cmm2->micros() < until) {}
    return c;
}

static int hold(int a, int b, int c, int d) {    /* (what came after an ESC that made no sequence: given next) */
    int in[4] = {a, b, c, d};
    for(int i = 0; i < 4; i++)
        if(in[i] >= 0) held[nheld++] = in[i];
    return 0x1b;
}

static int getkey(void) {
    if(nheld) {
        int c = held[0];
        for(int i = 1; i < nheld; i++) held[i - 1] = held[i];
        nheld--;
        return c;
    }
    int c = cmm2->con_getc();
    if(c != 0x1b) return c;
    uint64_t t0 = cmm2->micros();
    int c2 = con_byte(t0 + 30000);
    if(c2 < 0) return 0x1b;                     /* (Esc on its own) */
    if(c2 != '[' && c2 != 'O') return hold(c2, -1, -1, -1);
    int c3 = con_byte(t0 + 50000);
    switch(c3) {
    case 'A': return 0x80;                      /* the arrows: up, down, right, left */
    case 'B': return 0x81;
    case 'C': return 0x83;
    case 'D': return 0x82;
    case 'H': return 0x86;                      /* Home and End */
    case 'F': return 0x87;
    }
    if(c2 == 'O') {
        if(c3 >= 'P' && c3 <= 'T') return 0x91 + (c3 - 'P');   /* F1 to F5 */
        return hold(c2, c3, -1, -1);
    }
    if(c3 < '1' || c3 > '6') return hold(c2, c3, -1, -1);
    int c4 = con_byte(t0 + 70000);
    if(c4 == '~') {                             /* Home, Insert, Delete, End, Page Up, Page Down */
        static const int k[6] = {0x86, 0x84, 0x7F, 0x87, 0x88, 0x89};
        return k[c3 - '1'];
    }
    if(c4 >= '0' && c4 <= '9') {
        int c5 = con_byte(t0 + 90000);
        if(c5 == '~') {                         /* the function keys: 11-15 F1-F5, 17-21 F6-F10, 23-24 F11-F12 */
            int n = (c3 - '0') * 10 + (c4 - '0');
            if(n >= 11 && n <= 15) return 0x91 + n - 11;
            if(n >= 17 && n <= 21) return 0x96 + n - 17;
            if(n == 23 || n == 24) return 0x9B + n - 23;
            return 0;                           /* (another: nothing) */
        }
        return hold(c2, c3, c4, c5);
    }
    return hold(c2, c3, c4, -1);
}

/* a key in MMBasic's codes (getkey): ASCII, and &H80-&H89 the arrows, Insert, Home, End, Page Up and Down */
static void key(int k) {
    static const short special[10] = {NSFB_KEY_UP, NSFB_KEY_DOWN, NSFB_KEY_LEFT, NSFB_KEY_RIGHT, NSFB_KEY_INSERT, -1,
                                      NSFB_KEY_HOME, NSFB_KEY_END, NSFB_KEY_PAGEUP, NSFB_KEY_PAGEDOWN};
    if(k >= 0x80 && k <= 0x89) {
        if(special[k - 0x80] >= 0) push_key(special[k - 0x80], 0);
    } else if(k == 8 || k == 9 || k == 13 || k == 27 || k == 0x7F) {
        push_key(k, 0);                         /* (Backspace, Tab, Enter, Esc and Delete are their own codes) */
    } else if(k >= 'A' && k <= 'Z') {
        push_key(k + 32, 1);
    } else if(k >= 32 && k < 127) {
        static const char shifted[] = "%^{|}", base[] = {'5', '6', '[', '\\', ']'};
        const char *s = strchr(shifted, k);
        if(s) push_key(base[s - shifted], 1);
        else push_key(k, 0);
    }
}

/* the mouse: its position as an absolute move, its buttons as the keys MOUSE_1 (left), _3 (right) and _2 (middle),
   each step of the wheel as MOUSE_4 (up) or MOUSE_5 (down) pressed and released */
static void mouse(nsfb_t *nsfb) {
    static int lx = -1, ly = -1, lb, lw, started;
    int m[4];
    if(!cmm2->mouse(m)) return;
    if(!started) {                              /* (the wheel's count as it was: only steps from here on) */
        started = 1;
        lw = m[3];
    }
    if(m[0] >= nsfb->width) m[0] = nsfb->width - 1;
    if(m[1] >= nsfb->height) m[1] = nsfb->height - 1;
    if(m[0] != lx || m[1] != ly) {
        if((qt + 1) % QN != qh) {
            q[qt].type = NSFB_EVENT_MOVE_ABSOLUTE;
            q[qt].value.vector.x = m[0];
            q[qt].value.vector.y = m[1];
            q[qt].value.vector.z = 0;
            qt = (qt + 1) % QN;
        }
        lx = m[0];
        ly = m[1];
    }
    static const struct { int bit, code; } b[3] = {{1, NSFB_KEY_MOUSE_1}, {2, NSFB_KEY_MOUSE_3}, {4, NSFB_KEY_MOUSE_2}};
    for(int i = 0; i < 3; i++)
        if((m[2] ^ lb) & b[i].bit) push((m[2] & b[i].bit) ? NSFB_EVENT_KEY_DOWN : NSFB_EVENT_KEY_UP, b[i].code);
    lb = m[2];
    for(; lw < m[3]; lw++) push_key(NSFB_KEY_MOUSE_4, 0);
    for(; lw > m[3]; lw--) push_key(NSFB_KEY_MOUSE_5, 0);
}

static bool cmm2_input(nsfb_t *nsfb, nsfb_event_t *event, int timeout) {
    static uint64_t last_usb;
    uint64_t end = cmm2->micros() + (timeout > 0 ? (uint64_t)timeout * 1000u : 0), now;
    int brk = 0, k;
    flush(nsfb);                                /* (what NetSurf drew since it last waited: to the screen) */
    for(;;) {
        cmm2->net_poll();
        now = cmm2->micros();
        if(now - last_usb >= 20000) {           /* (the USB devices every 20 ms, as port/cmm2_net.c's pump: poll is
                                                   MMBasic's whole routinechecks) */
            last_usb = now;
            brk = cmm2->poll();
            mouse(nsfb);
            while((k = getkey()) >= 0) key(k);
            cmm2_memory_poll();                 /* (a page that ran out of memory: stopped, and why said) */
        }
        if(brk || cmm2_shot_poll()) {
            event->type = NSFB_EVENT_CONTROL;
            event->value.controlcode = NSFB_CONTROL_QUIT;
            return true;
        }
        if(qh != qt) {
            *event = q[qh];
            qh = (qh + 1) % QN;
            return true;
        }
        if(timeout >= 0 && cmm2->micros() >= end) return false;
        cmm2->delay_us(500);
    }
}

/* the pointer, which libnsfb draws into the buffer (cursor.c): taken off before NetSurf draws where it is (claim),
   put back before a rectangle goes to the page (update), and moved by taking it off, drawing it at its new place and
   sending both places to the page (as libnsfb's SDL surface does) */
static void to_page(nsfb_t *nsfb, nsfb_bbox_t *box);

static int cmm2_claim(nsfb_t *nsfb, nsfb_bbox_t *box) {
    struct nsfb_cursor_s *c = nsfb->cursor;
    if(c != NULL && c->plotted && nsfb_plot_bbox_intersect(box, &c->loc)) nsfb_cursor_clear(nsfb, c);
    return 0;
}

static int cmm2_cursor(nsfb_t *nsfb, struct nsfb_cursor_s *c) {
    nsfb_bbox_t redraw, now, area = {0, 0, nsfb->width, nsfb->height};
    if(c == NULL || !c->plotted) return 0;
    now = c->loc;
    now.x0 -= c->hotspot_x;
    now.y0 -= c->hotspot_y;
    now.x1 -= c->hotspot_x;
    now.y1 -= c->hotspot_y;
    nsfb_plot_add_rect(&c->savloc, &now, &redraw);
    nsfb_plot_clip(&area, &redraw);
    nsfb_cursor_clear(nsfb, c);
    nsfb_cursor_plot(nsfb, c);
    to_page(nsfb, &redraw);
    return 0;
}

const nsfb_surface_rtns_t cmm2_rtns = {
    .defaults = cmm2_defaults,
    .initialise = cmm2_initialise,
    .finalise = cmm2_finalise,
    .input = cmm2_input,
    .geometry = cmm2_geometry,
    .update = cmm2_update,
    .claim = cmm2_claim,
    .cursor = cmm2_cursor,
};

NSFB_SURFACE_DEF(cmm2, NSFB_SURFACE_SDL, &cmm2_rtns)
