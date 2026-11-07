/* vt_term.c: libvterm's screen drawn in the frame buffer (vt_term.h). With vtt_open the mode is 8-bit and its palette xterm's 256 colours, so
   a colour number from the host is a pixel. With vtt_attach the screen is someone else's: pix[] gives each colour number the mode's own pixel. Damage from libvterm is drawn cell by cell from its own screen, so no copy of the screen is kept here; scrolls are moved in
   the frame buffer, merged over each batch of input (VTERM_DAMAGE_SCROLL), so that a fast host costs one move per frame and not one per line.
   The cursor and the mouse pointer are drawn over a cell and put back round every update. MIT License, as cmm2img.h. */
#define _GNU_SOURCE                                                 /* (memmem) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"
#include "vterm.h"
#include "vt_term.h"

#define WHEEL_LINES 3
#define BLINK_US 500000u
#define BELL_MS 150

typedef struct __attribute__((packed)) { uint16_t ch; uint8_t fg, bg, at; } sbcell;     /* a line of the scrollback: at's bits below */
enum { A_BOLD = 1, A_UNDER = 2, A_REV = 4, A_STRIKE = 8, A_CONCEAL = 16 };

void (*vtt_resize_request)(int rows, int cols);
void (*vtt_bell_hook)(void);

static vtt_config cfg;
static void (*sendout)(const char *bytes, size_t n);
static cmm2_video v;
static int bp = 1;                                  /* bytes in a pixel */
static uint16_t pix[256];                           /* a colour number's pixel */
static int cursor_off;                              /* vtt_cursor_show(0) */
static int frozen;                                  /* vtt_freeze(1) */
static const vtt_font *fnt;
static int cw, ch, stride;                          /* a cell's size, and a glyph row's bytes */
static int scols, srows;                            /* the screen in cells */
static int tcols, trows, ox, oy;                    /* the terminal in cells, and its corner in pixels */
static VTerm *vt;
static VTermScreen *vs;
static VTermState *vst;
static int deffg = 7, defbg = 0;                    /* the colours "default" means */
static int curvis = 1, curblink = 1, curshape = VTERM_PROP_CURSORSHAPE_BLOCK, mousemode;
static VTermPos curpos;
static int drawn_cursor, drawn_mouse;               /* what is over the screen now */
static VTermPos drawn_cpos, drawn_mpos;
static int mcol = -1, mrow = -1, mbuttons;
static int blinkon = 1, anyblink;
static uint64_t blinkt;
static char statustext[256];
static sbcell *sb;                                  /* the scrollback, a ring of sblines lines of sbw cells */
static int sblines, sbw, sbhead, sbcount, view;     /* view: how far back the screen is showing, 0 live */
static int bellleft;
static int oldmouse[4];
static int vt52, vt52state, vt52row;                /* VT52 mode, and how far through a sequence */
static int dirty = 1;                               /* something may have changed since vtt_update: it leaves the screen alone if not */
static uint32_t lastc = 0xFFFFFFFF;                 /* the last glyph found */
static const uint8_t *lastg;

/* the 256 colours, as xterm's */
static const uint8_t ansi16[16][3] = {{0, 0, 0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0}, {0, 0, 238}, {205, 0, 205}, {0, 205, 205}, {229, 229, 229},
    {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0}, {92, 92, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};

static uint8_t rgb[256 * 3];

static void colours(void) {
    static const uint8_t lev[6] = {0, 95, 135, 175, 215, 255};
    int i;
    memcpy(rgb, ansi16, sizeof(ansi16));
    for(i = 0; i < 216; i++) {
        rgb[(16 + i) * 3] = lev[i / 36];
        rgb[(16 + i) * 3 + 1] = lev[i / 6 % 6];
        rgb[(16 + i) * 3 + 2] = lev[i % 6];
    }
    for(i = 0; i < 24; i++) rgb[(232 + i) * 3] = rgb[(232 + i) * 3 + 1] = rgb[(232 + i) * 3 + 2] = 8 + i * 10;
}

static void palette(void) {                                         /* the mode's palette made xterm's: a colour number is its pixel */
    int i;
    colours();
    cmm2->video_palette(0, 256, rgb);
    for(i = 0; i < 256; i++) pix[i] = i;
    bp = 1;
}

static void pixels(void) {                                          /* the screen's own: RGB332 in 8 bits, as MMBasic's colours are there, or RGB565 */
    int i;
    colours();
    bp = v.bpp / 8;
    for(i = 0; i < 256; i++) {
        int r = rgb[i * 3], g = rgb[i * 3 + 1], b = rgb[i * 3 + 2];
        pix[i] = bp == 1 ? ((r * 7 + 127) / 255) << 5 | ((g * 7 + 127) / 255) << 2 | (b * 3 + 127) / 255 : (r >> 3) << 11 | (g >> 2) << 5 | b >> 3;
    }
}

static int nearest(int r, int g, int b) {                           /* the cube's colour or the grey ramp's, whichever is the closer */
    int cr = r < 48 ? 0 : r < 115 ? 1 : (r - 35) / 40, cg = g < 48 ? 0 : g < 115 ? 1 : (g - 35) / 40, cb = b < 48 ? 0 : b < 115 ? 1 : (b - 35) / 40;
    static const uint8_t lev[6] = {0, 95, 135, 175, 215, 255};
    int grey = (r + g + b) / 3, gi = grey < 8 ? 0 : grey > 238 ? 23 : (grey - 3) / 10, gv = 8 + gi * 10;
    int dc = (r - lev[cr]) * (r - lev[cr]) + (g - lev[cg]) * (g - lev[cg]) + (b - lev[cb]) * (b - lev[cb]);
    int dg = (r - gv) * (r - gv) + (g - gv) * (g - gv) + (b - gv) * (b - gv);
    return dg < dc ? 232 + gi : 16 + cr * 36 + cg * 6 + cb;
}

static int colour(const VTermColor *c, int isbg, int bold) {
    if(VTERM_COLOR_IS_DEFAULT_FG(c) && !isbg) return bold && deffg == 7 ? 15 : deffg;
    if(VTERM_COLOR_IS_DEFAULT_BG(c) && isbg) return defbg;
    if(VTERM_COLOR_IS_INDEXED(c)) return c->indexed.idx;
    return nearest(c->rgb.red, c->rgb.green, c->rgb.blue);
}

/* the glyphs: each font's characters sorted, found by halving. Anything not there is a box */
extern const uint16_t vtt_map[];
extern const int vtt_map_count;
static int glyph_index(uint32_t c) {
    int lo = 0, hi = vtt_map_count - 1;
    while(lo <= hi) {
        int m = (lo + hi) / 2;
        if(vtt_map[m] == c) return m;
        if(vtt_map[m] < c) lo = m + 1;
        else hi = m - 1;
    }
    return -1;
}

/* a cell at pixel x,y. dw: double width; dh: 0, or 1 and 2 for the top and bottom halves of double height. Everything drawn goes through here */
static void draw_glyph(int x, int y, uint32_t c, int fg, int bg, int at, int dw, int dh) {
    const uint8_t *g;
    int r, i, w = dw ? cw * 2 : cw;
    uint32_t pf = pix[fg], pb = pix[bg];
    if(frozen) return;
    if(x + w > (int)v.width || y + ch > (int)v.height) return;
    if(c == lastc) g = lastg;
    else {
        int gi = c < 32 ? glyph_index(' ') : glyph_index(c);
        if(gi < 0) gi = glyph_index(0xFFFD);
        if(gi < 0) gi = glyph_index('?');
        g = fnt->bits + gi * ch * stride;
        lastc = c;
        lastg = g;
    }
    for(r = 0; r < ch; r++) {
        int gr = dh == 0 ? r : dh == 1 ? r / 2 : (ch + r) / 2;
        const uint8_t *row = g + gr * stride;
        uint8_t *p = v.fb + (uint32_t)(y + r) * v.pitch + x * bp;
        uint32_t bits = row[0] << 8 | (stride > 1 ? row[1] : 0);
        if((at & A_UNDER) && gr == ch - 2) bits = 0xFFFF;
        if((at & A_STRIKE) && gr == ch / 2) bits = 0xFFFF;
        if(at & A_CONCEAL) bits = 0;
        if((at & A_BOLD) && cw >= 8) bits |= bits >> 1;
        if(bp == 2) {
            uint16_t *q = (uint16_t *)p;
            if(dw) for(i = 0; i < cw; i++, bits <<= 1) q[i * 2] = q[i * 2 + 1] = bits & 0x8000 ? pf : pb;
            else for(i = 0; i < cw; i++, bits <<= 1) q[i] = bits & 0x8000 ? pf : pb;
        } else if(dw) {
            for(i = 0; i < cw; i++, bits <<= 1) p[i * 2] = p[i * 2 + 1] = bits & 0x8000 ? pf : pb;
        } else {
            for(i = 0; i < cw; i++, bits <<= 1) p[i] = bits & 0x8000 ? pf : pb;
        }
        if(v.copies == 2) memcpy(p + v.width * bp, p, (size_t)w * bp);      /* (a line shown twice is stored twice) */
    }
}

static void put_pixel(int x, int y, int colour) {
    uint8_t *p = v.fb + (uint32_t)y * v.pitch + x * bp;
    uint32_t c;
    if(frozen) return;
    for(c = 0; c < v.copies; c++, p += v.width * bp) {
        if(bp == 2) *(uint16_t *)p = pix[colour];
        else *p = pix[colour];
    }
}

static void draw_live(int row, int col, int inverse) {
    VTermPos pos = {row, col};
    VTermScreenCell c;
    int fg, bg, at = 0, x;
    vterm_screen_get_cell(vs, pos, &c);
    if(c.attrs.dwl && col >= tcols / 2) return;                     /* past the edge of a double-width line: a narrower screen keeps them */
    if(c.chars[0] == (uint32_t)-1) c.chars[0] = ' ';                /* the right half of a wide character: there are no wide glyphs */
    fg = colour(&c.fg, 0, c.attrs.bold);
    bg = colour(&c.bg, 1, 0);
    if(c.attrs.bold) at |= A_BOLD;
    if(c.attrs.underline) at |= A_UNDER;
    if(c.attrs.strike) at |= A_STRIKE;
    if(c.attrs.conceal) at |= A_CONCEAL;
    if(c.attrs.blink) {
        anyblink = 1;
        if(!blinkon) at |= A_CONCEAL, at &= ~(A_UNDER | A_STRIKE);
    }
    x = ox + col * cw * (c.attrs.dwl ? 2 : 1);
    if(!!c.attrs.reverse ^ inverse) draw_glyph(x, oy + (row + view) * ch, c.chars[0], bg, fg, at, c.attrs.dwl, c.attrs.dhl);
    else draw_glyph(x, oy + (row + view) * ch, c.chars[0], fg, bg, at, c.attrs.dwl, c.attrs.dhl);
}

/* a line of the scrollback in row */
static void draw_sb(int row, int line) {
    const sbcell *p = sb + (size_t)((sbhead + sblines - line + 1) % sblines) * sbw;     /* line 1: the newest, at sbhead */
    int col;
    for(col = 0; col < tcols; col++) {
        const sbcell *c = col < sbw ? p + col : NULL;
        if(c == NULL) draw_glyph(ox + col * cw, oy + row * ch, ' ', deffg, defbg, 0, 0, 0);
        else if(c->at & A_REV) draw_glyph(ox + col * cw, oy + row * ch, c->ch, c->bg, c->fg, c->at, 0, 0);
        else draw_glyph(ox + col * cw, oy + row * ch, c->ch, c->fg, c->bg, c->at, 0, 0);
    }
}

static void draw_rows(int from, int to) {
    int r, c;
    for(r = from; r < to; r++) {
        if(r < view) draw_sb(r, view - r);
        else for(c = 0; c < tcols; c++) draw_live(r - view, c, 0);
    }
}

static void status_draw(void) {
    int y = oy + (srows - 1) * ch, x = ox, i, n = strlen(statustext);
    if(!cfg.status) return;
    if(view) {
        char b[48];
        snprintf(b, sizeof(b), "  Scrollback: -%d of %d", view, sbcount);
        for(i = 0; i < scols; i++) draw_glyph(x + i * cw, y, i < (int)strlen(b) ? (uint8_t)b[i] : ' ', 0, 11, 0, 0, 0);
        return;
    }
    for(i = 0; i < scols; i++) draw_glyph(x + i * cw, y, i < n ? (uint8_t)statustext[i] : ' ', 0, 250, 0, 0, 0);
}

static void cover(int on) {                                         /* the cursor and pointer on or off the screen */
    if(!on) {
        if(drawn_cursor) draw_live(drawn_cpos.row, drawn_cpos.col, 0);
        if(drawn_mouse) draw_live(drawn_mpos.row, drawn_mpos.col, 0);
        drawn_cursor = drawn_mouse = 0;
        return;
    }
    if(view) return;
    if(curvis && !cursor_off && (!curblink || blinkon)) {
        drawn_cpos = curpos;
        if(curpos.row < trows && curpos.col < tcols) {
            drawn_cursor = 1;
            if(curshape == VTERM_PROP_CURSORSHAPE_BLOCK) draw_live(curpos.row, curpos.col, 1);
            else {
                VTermScreenCell c;
                int x, y, i, j, w, h, col, dbl;
                vterm_screen_get_cell(vs, curpos, &c);
                dbl = c.attrs.dwl ? 2 : 1;
                col = colour(&c.fg, 0, 0);
                x = ox + curpos.col * cw * dbl;
                y = oy + curpos.row * ch;
                w = curshape == VTERM_PROP_CURSORSHAPE_BAR_LEFT ? 2 : cw * dbl;
                h = curshape == VTERM_PROP_CURSORSHAPE_BAR_LEFT ? ch : 2;
                if(curshape == VTERM_PROP_CURSORSHAPE_UNDERLINE) y += ch - 2;
                for(j = 0; j < h; j++) for(i = 0; i < w; i++) put_pixel(x + i, y + j, col);
            }
        }
    }
    if(mousemode && mcol >= 0 && mrow < trows && mcol < tcols && !(drawn_cursor && drawn_cpos.row == mrow && drawn_cpos.col == mcol)) {
        drawn_mpos.row = mrow;
        drawn_mpos.col = mcol;
        drawn_mouse = 1;
        draw_live(mrow, mcol, 1);
    }
}

/* libvterm's screen callbacks */
static int on_damage(VTermRect r, void *user) {
    int row, col;
    if(view) return 1;
    for(row = r.start_row; row < r.end_row && row < trows; row++) {
        VTermPos p = {row, 0};
        VTermScreenCell c;
        vterm_screen_get_cell(vs, p, &c);
        if(c.attrs.dwl) {                                           /* the half of the line that a double-width line leaves empty stays so */
            for(col = r.start_col; col < r.end_col && col < tcols / 2; col++) draw_live(row, col, 0);
        } else {
            for(col = r.start_col; col < r.end_col && col < tcols; col++) draw_live(row, col, 0);
        }
    }
    return 1;
}

static int on_moverect(VTermRect d, VTermRect s, void *user) {
    int h = (s.end_row - s.start_row) * ch, w = (s.end_col - s.start_col) * cw * bp, i;
    uint32_t pitch = v.pitch, k;
    uint8_t *from = v.fb + (uint32_t)(oy + s.start_row * ch) * pitch + (ox + s.start_col * cw) * bp;
    uint8_t *to = v.fb + (uint32_t)(oy + d.start_row * ch) * pitch + (ox + d.start_col * cw) * bp;
    if(view || frozen) return 1;
    if((uint32_t)w * v.copies == pitch) {                           /* whole lines, each one's copy with it */
        memmove(to, from, (size_t)h * pitch);
        return 1;
    }
    for(k = 0; k < v.copies; k++, from += v.width * bp, to += v.width * bp) {
        if(to < from) for(i = 0; i < h; i++) memmove(to + i * pitch, from + i * pitch, w);
        else for(i = h - 1; i >= 0; i--) memmove(to + i * pitch, from + i * pitch, w);
    }
    return 1;
}

static int on_movecursor(VTermPos pos, VTermPos old, int visible, void *user) {
    curpos = pos;
    return 1;
}

static const int cursorshapes[3] = {VTERM_PROP_CURSORSHAPE_BLOCK, VTERM_PROP_CURSORSHAPE_UNDERLINE, VTERM_PROP_CURSORSHAPE_BAR_LEFT};

static int on_termprop(VTermProp p, VTermValue *val, void *user) {
    switch(p) {
    case VTERM_PROP_CURSORVISIBLE: curvis = val->boolean; break;
    case VTERM_PROP_CURSORBLINK: curblink = val->boolean; break;
    case VTERM_PROP_CURSORSHAPE:                                    /* (libvterm's reset says block: the setting, then) */
        curshape = val->number == VTERM_PROP_CURSORSHAPE_BLOCK ? cursorshapes[cfg.cursor] : val->number;
        break;
    case VTERM_PROP_MOUSE: mousemode = val->number; break;
    default: break;                                                 /* (libvterm applies DECSCNM to the cells itself) */
    }
    return 1;
}

static int on_bell(void *user) {
    if(vtt_bell_hook) vtt_bell_hook();
    else vtt_bell();
    return 1;
}

static int on_resize(int rows, int cols, void *user) {
    trows = rows;
    tcols = cols;
    return 1;
}

static int on_pushline(int cols, const VTermScreenCell *cells, void *user) {
    sbcell *p;
    int i;
    if(sb == NULL) return 0;
    sbhead = (sbhead + 1) % sblines;
    p = sb + (size_t)sbhead * sbw;
    for(i = 0; i < sbw; i++) {
        if(i < cols) {
            const VTermScreenCell *c = cells + i;
            p[i].ch = c->chars[0] > 0xFFFF ? 0xFFFD : c->chars[0] ? c->chars[0] : ' ';
            p[i].fg = colour(&c->fg, 0, c->attrs.bold);
            p[i].bg = colour(&c->bg, 1, 0);
            p[i].at = (c->attrs.bold ? A_BOLD : 0) | (c->attrs.underline ? A_UNDER : 0) | (c->attrs.reverse ? A_REV : 0) |
                      (c->attrs.strike ? A_STRIKE : 0) | (c->attrs.conceal ? A_CONCEAL : 0);
        } else {
            p[i].ch = ' ';
            p[i].fg = deffg;
            p[i].bg = defbg;
            p[i].at = 0;
        }
    }
    if(sbcount < sblines) sbcount++;
    if(view && view < sbcount) view++;                              /* what is being read stays where it is */
    return 1;
}

static int on_sbclear(void *user) {
    sbcount = view = 0;
    return 1;
}

static const VTermScreenCallbacks callbacks = {.damage = on_damage, .moverect = on_moverect, .movecursor = on_movecursor, .settermprop = on_termprop,
    .bell = on_bell, .resize = on_resize, .sb_pushline = on_pushline, .sb_clear = on_sbclear};

/* what libvterm does not know: CSI 8;rows;cols t, and DECCOLM, which the patch passes on. The rest of the window operations are left alone */
static int on_csi(const char *leader, const long args[], int argcount, const char *intermed, char command, void *user) {
    if(leader && leader[0] == '?' && (command == 'h' || command == 'l') && argcount == 1 && args[0] == 3) {
        if(vtt_resize_request) vtt_resize_request(0, command == 'h' ? 132 : 80);
        return 1;
    }
    if(leader && leader[0] == '?' && command == 'l' && argcount == 1 && args[0] == 2) {
        vt52 = 1;
        vt52state = 0;
        return 1;
    }
    if(leader == NULL && intermed == NULL && command == 'x') {      /* DECREQTPARM: no parity, 8 bits, 19200 baud: a VT100's fixed answer for a speed it has no code for */
        char b[32];
        int a = CSI_ARG_OR(args[0], 0);
        if(a > 1) return 0;
        snprintf(b, sizeof(b), "\033[%d;1;1;120;120;1;0x", a + 2);
        if(sendout) sendout(b, strlen(b));
        return 1;
    }
    if(leader == NULL && intermed == NULL && command == 't' && argcount >= 3 && CSI_ARG(args[0]) == 8) {
        if(vtt_resize_request) vtt_resize_request(CSI_ARG(args[1]), CSI_ARG(args[2]));
        return 1;
    }
    return 0;
}
static const VTermStateFallbacks fallbacks = {.csi = on_csi};

static void on_output(const char *s, size_t n, void *user) {
    if(sendout) sendout(s, n);
}

static void send(const char *s) {
    if(sendout) sendout(s, strlen(s));
}

static void geometry(void) {
    int avail;
    fnt = &vtt_fonts[cfg.font];
    cw = fnt->w;
    ch = fnt->h;
    stride = (cw + 7) / 8;
    lastc = 0xFFFFFFFF;
    scols = v.width / cw;
    srows = v.height / ch;
    avail = srows - (cfg.status ? 1 : 0);
    tcols = cfg.cols > 0 && cfg.cols < scols ? cfg.cols : scols;
    trows = cfg.rows > 0 && cfg.rows < avail ? cfg.rows : avail;
    ox = (v.width - scols * cw) / 2;                                /* the cells in the middle of the screen, and the terminal in their corner, */
    oy = (v.height - srows * ch) / 2;                               /* so that the host changing its size moves nothing that stays */
}

static void scrollback_alloc(void) {
    int want = cfg.scrollback;
    if(sb && sbw == tcols) return;                                  /* (lines of the same width stay) */
    free(sb);
    sb = NULL;
    sbcount = sbhead = view = 0;
    sbw = tcols;
    while(want >= 64 && (sb = malloc((size_t)want * sbw * sizeof(sbcell))) == NULL) want /= 2;
    sblines = sb ? want : 0;
}

void vtt_set(const vtt_config *c) {
    static const uint8_t fgs[3] = {7, 46, 214};
    cfg.charset = c->charset;
    cfg.colour = c->colour;
    cfg.backspace_del = c->backspace_del;
    cfg.cursor = c->cursor >= 0 && c->cursor < 3 ? c->cursor : 0;
    curshape = cursorshapes[cfg.cursor];
    deffg = fgs[cfg.colour];
}

static int start(const vtt_config *c);

int vtt_open(const vtt_config *c, void (*out)(const char *bytes, size_t n)) {
    int r, same = vt != NULL && c->mode == cfg.mode;                /* (a new mode makes the monitor lock on again: only if it must) */
    cfg = *c;
    if(out) sendout = out;
    if(!same) {
        r = cmm2->video_mode(cfg.mode, 8, &v);
        if(r < 0) return r;
        if(v.copies != 1) return -1;
        palette();
    }
    return start(c);
}

int vtt_attach(const vtt_config *c, const cmm2_video *video, void (*out)(const char *bytes, size_t n)) {
    if(video->bpp != 8 && video->bpp != 16) return -1;
    cfg = *c;
    if(out) sendout = out;
    v = *video;
    pixels();
    return start(c);
}

static int start(const vtt_config *c) {
    vtt_set(c);
    geometry();
    drawn_cursor = drawn_mouse = 0;
    {                                                               /* libvterm does not look at what malloc gives it: its two screens, and room over */
        void *room = malloc((size_t)tcols * trows * 100);
        if(room == NULL && sb) {                                    /* the screen matters more than what has scrolled off it */
            free(sb);
            sb = NULL;
            room = malloc((size_t)tcols * trows * 100);
        }
        if(room == NULL) return -3;
        free(room);
    }
    if(vt == NULL) {
        vt = vterm_new(trows, tcols);
        if(vt == NULL) return -3;
        vterm_set_utf8(vt, 1);                                      /* (vtt_write makes a single-byte character set UTF-8) */
        vs = vterm_obtain_screen(vt);
        vst = vterm_obtain_state(vt);
        vterm_screen_set_callbacks(vs, &callbacks, NULL);
        vterm_screen_set_unrecognised_fallbacks(vs, &fallbacks, NULL);
        vterm_output_set_callback(vt, on_output, NULL);
        vterm_screen_set_damage_merge(vs, VTERM_DAMAGE_SCROLL);
        vterm_screen_enable_altscreen(vs, 1);
        vterm_state_set_bold_highbright(vst, 1);
        vterm_screen_reset(vs, 1);
    } else {
        vterm_set_size(vt, trows, tcols);
    }
    scrollback_alloc();
    vtt_redraw();
    return 0;
}

void vtt_close(void) {
    if(vt) {
        if(!frozen) memset(v.fb, 0, (size_t)v.height * v.pitch);    /* (the prompt comes back on a clear screen) */
        vterm_free(vt);
    }
    vt = NULL;
    free(sb);
    sb = NULL;
}

/* CP437's characters above 127: those a PicoMite or CMM2's font 1 has there */
static const uint16_t cp437[128] = {
    0xC7, 0xFC, 0xE9, 0xE2, 0xE4, 0xE0, 0xE5, 0xE7, 0xEA, 0xEB, 0xE8, 0xEF, 0xEE, 0xEC, 0xC4, 0xC5,
    0xC9, 0xE6, 0xC6, 0xF4, 0xF6, 0xF2, 0xFB, 0xF9, 0xFF, 0xD6, 0xDC, 0xA2, 0xA3, 0xA5, 0x20A7, 0x192,
    0xE1, 0xED, 0xF3, 0xFA, 0xF1, 0xD1, 0xAA, 0xBA, 0xBF, 0x2310, 0xAC, 0xBD, 0xBC, 0xA1, 0xAB, 0xBB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x3B1, 0xDF, 0x393, 0x3C0, 0x3A3, 0x3C3, 0xB5, 0x3C4, 0x3A6, 0x398, 0x3A9, 0x3B4, 0x221E, 0x3C6, 0x3B5, 0x2229,
    0x2261, 0xB1, 0x2265, 0x2264, 0x2320, 0x2321, 0xF7, 0x2248, 0xB0, 0x2219, 0xB7, 0x221A, 0x207F, 0xB2, 0x25A0, 0xA0};

/* VT52 mode: its sequences are few, so each is turned into the ANSI one and libvterm is left to it */
static void feed(const char *bytes, size_t n);

static void vt52_write(const char *bytes, size_t n) {
    static const char *const seq[128] = {['A'] = "\033[A", ['B'] = "\033[B", ['C'] = "\033[C", ['D'] = "\033[D", ['F'] = "\033(0", ['G'] = "\033(B", ['H'] = "\033[H",
        ['I'] = "\033M", ['J'] = "\033[J", ['K'] = "\033[K", ['='] = "\033=", ['>'] = "\033>"};
    size_t i, run = 0;
    for(i = 0; i < n; i++) {
        uint8_t c = bytes[i];
        if(vt52state == 0) {
            if(c != 27) continue;
            if(i > run) feed(bytes + run, i - run);
            vt52state = 1;
        } else if(vt52state == 1) {
            vt52state = 0;
            if(c == 'Y') vt52state = 2;
            else if(c == 'Z') send("\033/Z");
            else if(c == '<') {                                     /* back to ANSI: the rest of the bytes as such */
                vt52 = 0;
                if(i + 1 < n) feed(bytes + i + 1, n - i - 1);
                return;
            } else if(c < 128 && seq[c]) feed(seq[c], strlen(seq[c]));
        } else if(vt52state == 2) {
            vt52row = c - 31;
            vt52state = 3;
        } else {
            char b[16];
            snprintf(b, sizeof(b), "\033[%d;%dH", vt52row, c - 31);
            feed(b, strlen(b));
            vt52state = 0;
        }
        run = i + 1;
    }
    if(vt52state == 0 && n > run) feed(bytes + run, n - run);
}

/* ESC [ ? 2 l turns everything after it into VT52, so it ends a run of bytes for libvterm */
void vtt_write(const char *bytes, size_t n) {
    if(vt == NULL) return;
    dirty = 1;
    while(n) {
        const char *p;
        size_t len;
        if(vt52) {
            vt52_write(bytes, n);
            return;
        }
        p = memmem(bytes, n, "?2l", 3);
        len = p ? (size_t)(p + 3 - bytes) : n;
        feed(bytes, len);
        bytes += len;
        n -= len;
    }
}

static void feed(const char *bytes, size_t n) {
    static char out[768];
    size_t i;
    int o = 0;
    if(cfg.charset == VTT_UTF8) {
        vterm_input_write(vt, bytes, n);
        return;
    }
    for(i = 0; i < n; i++) {
        uint8_t c = bytes[i];
        if(c < 128) out[o++] = c;
        else {
            uint32_t u = cfg.charset == VTT_CP437 ? cp437[c - 128] : c;
            if(u < 0x800) {
                out[o++] = 0xC0 | u >> 6;
                out[o++] = 0x80 | (u & 63);
            } else {
                out[o++] = 0xE0 | u >> 12;
                out[o++] = 0x80 | (u >> 6 & 63);
                out[o++] = 0x80 | (u & 63);
            }
        }
        if(o > (int)sizeof(out) - 4) {
            vterm_input_write(vt, out, o);
            o = 0;
        }
    }
    if(o) vterm_input_write(vt, out, o);
}

void vtt_update(void) {
    uint64_t t;
    int toggled = 0;
    if(vt == NULL) return;
    t = cmm2->micros();
    if(t - blinkt >= BLINK_US) {
        blinkt = t;
        blinkon = !blinkon;
        toggled = 1;
    }
    if(!dirty && !(toggled && (anyblink || (curvis && curblink)))) return;      /* (the cursor is not taken off and put back for nothing: it would flicker) */
    dirty = 0;
    cover(0);
    vterm_screen_flush_damage(vs);
    if(toggled && anyblink && !view) {                              /* the blinking text redrawn: only if there was some */
        int r, c;
        anyblink = 0;
        for(r = 0; r < trows; r++) for(c = 0; c < tcols; c++) {
            VTermPos p = {r, c};
            VTermScreenCell cell;
            vterm_screen_get_cell(vs, p, &cell);
            if(cell.attrs.blink) draw_live(r, c, 0);
        }
    }
    cover(1);
}

static void view_to(int lines) {
    if(lines < 0) lines = 0;
    if(lines > sbcount) lines = sbcount;
    if(lines == view) return;
    cover(0);
    view = lines;
    draw_rows(0, trows);
    status_draw();
    cover(1);
}

/* the keys, by the firmware's codes (Inc/Hardware_Includes.h). The PicoMite's MMInkey wants what Tera Term sends: Home and End as ESC [ 1 ~ and
   ESC [ 4 ~, and Shift-F3 to Shift-F10 as VT220's F13-F20, which it reads as its own Shift-F codes */
int vtt_key(int key, int mods) {
    int shift = (mods & (8 | 128)) != 0, alt = (mods & 1) != 0;
    VTermModifier vm = shift && key >= 0x80 ? VTERM_MOD_SHIFT : VTERM_MOD_NONE;
    if(vt == NULL) return 0;
    if(shift && (key == 0x88 || key == 0x89 || key == 0x86 || key == 0x87)) {       /* Shift with Page Up, Page Down, Home, End: the scrollback */
        view_to(key == 0x88 ? view + trows - 1 : key == 0x89 ? view - (trows - 1) : key == 0x86 ? sbcount : 0);
        return 1;
    }
    if(view) view_to(0);                                            /* any other key goes back to the live screen */
    if(vt52) {                                                      /* the arrows and PF1-PF4 are ESC and a letter */
        static const char *const k52[] = {"\033A", "\033B", "\033D", "\033C"};
        static const char *const pf[] = {"\033P", "\033Q", "\033R", "\033S"};
        if(key >= 0x80 && key <= 0x83) { send(k52[key - 0x80]); return 1; }
        if(key >= 0x91 && key <= 0x94) { send(pf[key - 0x91]); return 1; }
    }
    switch(key) {
    case 13: vterm_keyboard_key(vt, VTERM_KEY_ENTER, VTERM_MOD_NONE); break;
    case 10: send("\n"); break;                                     /* an LF is an LF, not Enter (the one a keyboard's Enter brings is dropped by the app) */
    case 8: if(cfg.backspace_del) send("\x7F"); else send("\b"); break;
    case 9: vterm_keyboard_key(vt, VTERM_KEY_TAB, VTERM_MOD_NONE); break;
    case 0x9F: vterm_keyboard_key(vt, VTERM_KEY_TAB, VTERM_MOD_SHIFT); break;
    case 27: send("\033"); break;
    case 0x80: vterm_keyboard_key(vt, VTERM_KEY_UP, vm); break;
    case 0x81: vterm_keyboard_key(vt, VTERM_KEY_DOWN, vm); break;
    case 0x82: vterm_keyboard_key(vt, VTERM_KEY_LEFT, vm); break;
    case 0x83: vterm_keyboard_key(vt, VTERM_KEY_RIGHT, vm); break;
    case 0xA4: vterm_keyboard_key(vt, VTERM_KEY_UP, VTERM_MOD_SHIFT); break;
    case 0xA1: vterm_keyboard_key(vt, VTERM_KEY_DOWN, VTERM_MOD_SHIFT); break;
    case 0xA2: vterm_keyboard_key(vt, VTERM_KEY_LEFT, VTERM_MOD_SHIFT); break;
    case 0xA3: vterm_keyboard_key(vt, VTERM_KEY_RIGHT, VTERM_MOD_SHIFT); break;
    case 0x84: vterm_keyboard_key(vt, VTERM_KEY_INS, VTERM_MOD_NONE); break;
    case 0x7F: case 0xA0: vterm_keyboard_key(vt, VTERM_KEY_DEL, VTERM_MOD_NONE); break;
    case 0x86: send(cfg.home_xterm ? "\033[H" : "\033[1~"); break;
    case 0x87: send(cfg.home_xterm ? "\033[F" : "\033[4~"); break;
    case 0x88: vterm_keyboard_key(vt, VTERM_KEY_PAGEUP, VTERM_MOD_NONE); break;
    case 0x89: vterm_keyboard_key(vt, VTERM_KEY_PAGEDOWN, VTERM_MOD_NONE); break;
    default:
        if(key >= 0x91 && key <= 0x9C) vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(key - 0x90), VTERM_MOD_NONE);
        else if(key >= 0xB3 && key <= 0xBA) {                       /* Shift-F3 to Shift-F10 */
            static const char *const sf[8] = {"25", "26", "28", "29", "31", "32", "33", "34"};
            send("\033[");
            send(sf[key - 0xB3]);
            send("~");
        } else if(key >= 1 && key < 0x7F) {
            if(alt) send("\033");
            char b[2] = {key, 0};
            if(sendout) sendout(b, 1);
        } else if(key >= 0xC0 && key <= 0xFF) {                     /* a layout's own letters: as the character set has them */
            char b[2] = {key, 0};
            if(sendout) sendout(b, 1);
        } else return 0;
    }
    return 1;
}

void vtt_mouse(void) {
    int m[4], col, row, wheel, changed;
    if(vt == NULL || cmm2->version < 4 || !cmm2->mouse(m)) return;
    col = (m[0] - ox) / (cw ? cw : 1);
    row = (m[1] - oy) / (ch ? ch : 1);
    if(col < 0) col = 0;
    if(row < 0) row = 0;
    if(col >= tcols) col = tcols - 1;
    if(row >= trows) row = trows - 1;
    wheel = m[3] - oldmouse[3];
    oldmouse[3] = m[3];
    if(!mousemode) {
        if(wheel) view_to(view + wheel * WHEEL_LINES);
        mbuttons = m[2];
        return;
    }
    changed = col != mcol || row != mrow;
    if(changed) {
        mcol = col;
        mrow = row;
        dirty = 1;
        vterm_mouse_move(vt, row, col, VTERM_MOD_NONE);
    }
    if((m[2] ^ mbuttons) & 1) vterm_mouse_button(vt, 1, m[2] & 1, VTERM_MOD_NONE);
    if((m[2] ^ mbuttons) & 4) vterm_mouse_button(vt, 2, (m[2] & 4) != 0, VTERM_MOD_NONE);
    if((m[2] ^ mbuttons) & 2) vterm_mouse_button(vt, 3, (m[2] & 2) != 0, VTERM_MOD_NONE);
    mbuttons = m[2];
    while(wheel > 0) { vterm_mouse_button(vt, 4, 1, VTERM_MOD_NONE); wheel--; }
    while(wheel < 0) { vterm_mouse_button(vt, 5, 1, VTERM_MOD_NONE); wheel++; }
}

void vtt_status(const char *text) {
    snprintf(statustext, sizeof(statustext), "%s", text);
    status_draw();
}

void vtt_reset(int hard) {
    if(vt == NULL) return;
    vterm_screen_reset(vs, hard);
    if(hard) sbcount = view = 0;
    vtt_redraw();
}

void vtt_redraw(void) {
    drawn_cursor = drawn_mouse = 0;
    if(frozen) return;
    if(bp == 1 || pix[defbg] == 0) memset(v.fb, pix[defbg], (size_t)v.height * v.pitch);
    else {
        uint16_t *q = (uint16_t *)v.fb;
        uint32_t i, n = v.height * v.pitch / 2;
        for(i = 0; i < n; i++) q[i] = pix[defbg];
    }
    draw_rows(0, trows);
    status_draw();
    cover(1);
}

void vtt_freeze(int on) {
    frozen = on;
    if(on) drawn_cursor = drawn_mouse = 0;
}

void vtt_cursor_show(int on) {
    if(cursor_off == !on) return;
    cursor_off = !on;
    dirty = 1;
}

void vtt_size(int *cols, int *rows) {
    *cols = tcols;
    *rows = trows;
}

void vtt_screen_cells(int *cols, int *rows) {
    *cols = scols;
    *rows = srows;
}

void vtt_text(int col, int row, const char *s, int fg, int bg) {
    for(; *s; s++, col++) draw_glyph(ox + col * cw, oy + row * ch, (uint8_t)*s, fg, bg, 0, 0, 0);
}

/* the bell: a short tone by the sound stream, if nothing else has it */
static volatile int bellpos;
static void bellfill(int16_t *buf, uint32_t frames) {
    uint32_t i;
    for(i = 0; i < frames; i++) {
        int on = bellleft > 0 ? (bellpos / 28) & 1 : 0;             /* about 790 Hz at 44.1 kHz */
        int16_t s = bellleft > 0 ? (on ? 6000 : -6000) : 0;
        buf[i * 2] = buf[i * 2 + 1] = s;
        bellpos++;
        if(bellleft > 0) bellleft--;
    }
}
void vtt_bell(void) {
    static int started;
    if(cmm2->version < 3) return;
    if(!started) {
        if(cmm2->audio_start(bellfill) < 0) return;
        started = 1;
    }
    cmm2->audio_lock();
    bellleft = 44100 * BELL_MS / 1000;
    cmm2->audio_unlock();
}
