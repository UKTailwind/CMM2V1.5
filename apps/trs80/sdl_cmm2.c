/* sdl_cmm2.c: the part of SDL 1.2 that sdltrs uses (sdl/SDL.h), on the Colour Maximite 2, by the loader's services (Peter, 11 Nov 2026: a TRS-80 for
   the CMM2). sdltrs is built as it is, its own screen code, menus and keyboard tables included (trs_sdl_interface.c, trs_sdl_gui.c,
   trs_sdl_keyboard.c): this file is what they stand on, as SDL is on a PC.

   - The display: sdltrs asks for a window the size of the TRS-80's screen (SDL_SetVideoMode), and gets the smallest of MODE 8 (640 x 480), MODE 1
     (800 x 600) and MODE 9 (1024 x 768) that holds it, in 256 colours, with the picture in the middle. The Model III's and 4's 80 x 24 screen is
     640 x 480, the whole of MODE 8; the Model I's 64 x 16 is 512 x 384. The window's surface is in the program's memory, and the rectangles sdltrs
     has drawn are copied to the page when it says so (SDL_UpdateRects), as SDL does: the page itself is slow to read and write. A colour is given
     the next free place in the palette the first time it is asked for (SDL_MapRGB).
   - The keyboard: each key of the USB keyboard as it goes down and up (keys_raw), with SDL's name for the key (by its place, as on a US keyboard)
     and the character it types in the firmware's layout (OPTION USBKEYBOARD), which is what sdltrs types on the TRS-80. A key held repeats, for
     sdltrs's menus. Ctrl-C is not MMBasic's here. Ctrl+Alt+End leaves, as it does the other emulators (sdltrs's own keys: F7 its menu, F8 leaves).
   - The serial console types too, for remote use: a character is the key that types it, pressed for a moment; Enter, Esc, Tab and Backspace as
     they are; the terminal's arrow keys; Ctrl-F then 1-9, a, b, c for F1-F12 (with S first, shifted: Ctrl-F S a is Shift+F10); Ctrl-] leaves.
   - The mouse (the firmware's, USB or PS/2) and the first USB gamepad (SDL's joystick 0: the d-pad its hat, the left stick axes 0 and 1, the face
     buttons 0-3).
   - The sound: sdltrs's stream (16 bits, mono or stereo, 44.1 kHz) to the firmware's, by its callback as SDL calls it.
   - The ROM's questions as BASIC starts (MEMORY SIZE?, and a Model III's Cass?) are answered for the user: all the memory, unless -himem gives
     the highest address BASIC may use (-himem 32000); -ask leaves them to the user.
   RUN "trs80.app", -keys prints each key, mouse and gamepad event as sdltrs is given it; -stats prints how much of the time the emulator has to spare and the heap in use, every 5 s. The other words are sdltrs's
   command line (-model 3, -disk0 file.dsk, and so on).

   MIT License, as the SDK's files. None of SDL's own code is here. */
#include <errno.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cmm2.h"
#include "SDL.h"

/* ---- the time, and -stats ---- */
static int stats, trace;
static uint64_t stat_idle, stat_from;
static uint32_t stat_rects;
static int stat_colours = 1;
static volatile uint32_t stat_sound;            /* (the stream's frames that sdltrs's callback filled) */

static void stat_tick(void) {
    uint64_t now = cmm2->micros();
    if(!stats || now - stat_from < 5000000) return;
    printf("trs80: %lu%% spare, heap %lu, %lu rectangles shown, %d colours, %lu frames of sound\n",
           (unsigned long)(stat_idle * 100 / (now - stat_from)), (unsigned long)mallinfo().uordblks, (unsigned long)stat_rects, stat_colours,
           (unsigned long)stat_sound);
    stat_sound = 0;
    stat_rects = 0;
    stat_idle = 0;
    stat_from = now;
}

Uint32 SDL_GetTicks(void) { return (Uint32)(cmm2->micros() / 1000); }

static void mouse_poll(void);                   /* (the mouse, below) */
static void mouse_place(int x, int y);

void SDL_Delay(Uint32 ms) {
    uint64_t until = cmm2->micros() + (uint64_t)ms * 1000;
    uint64_t now, polled = 0;
    stat_idle += (uint64_t)ms * 1000;
    while((now = cmm2->micros()) < until) {     /* (sdltrs waits here for most of each of the TRS-80's clock ticks, 25 to 33 ms: the USB
                                                   devices are seen to every 2 ms meanwhile) */
        if(now - polled >= 2000 && until - now > 500) {
            cmm2->poll();
            mouse_poll();
            polled = now;
        }
    }
}

int SDL_Init(Uint32 flags) { (void)flags; return 0; }
void SDL_Quit(void) { }
const char *SDL_GetError(void) { return "not possible on the Colour Maximite 2"; }

/* ---- the palette: a colour's place is given out the first time it is asked for (0 is black, the border) ---- */
static uint8_t pal[256 * 3];
static int npal = 1, pal_new;

static int pal_index(int r, int g, int b) {
    int best = 0, bestd = 0x7FFFFFFF;
    for(int i = 0; i < npal; i++) {
        int dr = pal[i * 3] - r, dg = pal[i * 3 + 1] - g, db = pal[i * 3 + 2] - b, d = dr * dr + dg * dg + db * db;
        if(d == 0) return i;
        if(d < bestd) { bestd = d; best = i; }
    }
    if(npal == 256) return best;
    pal[npal * 3] = (uint8_t)r;
    pal[npal * 3 + 1] = (uint8_t)g;
    pal[npal * 3 + 2] = (uint8_t)b;
    pal_new = 1;
    stat_colours = npal + 1;
    return npal++;
}

static void pal_show(void) {                    /* (video_palette waits for the next frame) */
    if(!pal_new) return;
    pal_new = 0;
    cmm2->video_palette(0, npal, pal);
}

/* ---- surfaces: 8 bits (the screen's: the palette's numbers), 32 (0x00RRGGBB) or 1 (two colours, the left pixel the top bit) ---- */
static SDL_PixelFormat fmt8 = { NULL, 8, 1, 0, 0, 0, 0 };
static SDL_Surface screen;                      /* the window: its pixels in the program's memory */
static cmm2_video video;
static int video_set, off_x, off_y;             /* the mode set (0: none yet), and where the window is on its page */

static SDL_Surface *surface_new(void *pixels, int w, int h, int depth, int pitch) {
    SDL_Surface *s = calloc(1, sizeof(*s));
    SDL_PixelFormat *f = calloc(1, sizeof(*f));
    if(depth != 1 && depth != 8) depth = 32;
    if(s == NULL || f == NULL) { free(s); free(f); return NULL; }
    f->BitsPerPixel = (Uint8)depth;
    f->BytesPerPixel = (Uint8)(depth == 32 ? 4 : 1);
    if(depth == 32) { f->Rmask = 0xFF0000; f->Gmask = 0xFF00; f->Bmask = 0xFF; }
    if(depth == 1) {
        f->palette = calloc(1, sizeof(SDL_Palette));
        if(f->palette) {
            f->palette->ncolors = 2;
            f->palette->colors = calloc(2, sizeof(SDL_Color));
        }
        s->map[1] = 0;
    }
    s->format = f;
    s->w = w;
    s->h = h;
    if(pixels) {
        s->pixels = pixels;
        s->pitch = (Uint16)pitch;
    } else {
        s->pitch = (Uint16)(depth == 32 ? w * 4 : depth == 8 ? w : (w + 7) / 8);
        s->pixels = calloc(1, (size_t)s->pitch * h + 4);
        s->own_pixels = 1;
        if(s->pixels == NULL) { SDL_FreeSurface(s); return NULL; }
    }
    return s;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth, Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    (void)flags; (void)r; (void)g; (void)b; (void)a;
    return surface_new(NULL, w, h, depth, 0);
}

SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int w, int h, int depth, int pitch, Uint32 r, Uint32 g, Uint32 b, Uint32 a) {
    (void)r; (void)g; (void)b; (void)a;
    return surface_new(pixels, w, h, depth, pitch);
}

void SDL_FreeSurface(SDL_Surface *s) {
    if(s == NULL || s == &screen) return;
    if(s->own_pixels) free(s->pixels);
    if(s->format) {
        if(s->format->palette) {
            free(s->format->palette->colors);
            free(s->format->palette);
        }
        free(s->format);
    }
    free(s);
}

Uint32 SDL_MapRGB(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b) {
    if(f->BitsPerPixel == 8) return (Uint32)pal_index(r, g, b);
    return (Uint32)r << 16 | (Uint32)g << 8 | b;
}

Uint32 SDL_MapRGBA(const SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b, Uint8 a) { (void)a; return SDL_MapRGB(f, r, g, b); }

int SDL_SetPalette(SDL_Surface *s, int flags, SDL_Color *colours, int first, int n) {
    (void)flags;
    if(s == NULL || s->format->palette == NULL) return 0;
    for(int i = 0; i < n && first + i < 2; i++) {
        s->format->palette->colors[first + i] = colours[i];
        s->map[first + i] = (Uint8)pal_index(colours[i].r, colours[i].g, colours[i].b);
    }
    return 1;
}

int SDL_LockSurface(SDL_Surface *s) { (void)s; return 0; }
void SDL_UnlockSurface(SDL_Surface *s) { (void)s; }

int SDL_FillRect(SDL_Surface *dst, SDL_Rect *r, Uint32 colour) {
    int x = r ? r->x : 0, y = r ? r->y : 0, w = r ? r->w : dst->w, h = r ? r->h : dst->h;
    if(x < 0) { w += x; x = 0; }
    if(y < 0) { h += y; y = 0; }
    if(x + w > dst->w) w = dst->w - x;
    if(y + h > dst->h) h = dst->h - y;
    if(w <= 0 || h <= 0) return 0;
    for(int j = 0; j < h; j++) {
        uint8_t *d = (uint8_t *)dst->pixels + (size_t)(y + j) * dst->pitch;
        if(dst->format->BitsPerPixel == 8) memset(d + x, (int)colour, (size_t)w);
        else if(dst->format->BitsPerPixel == 32) {
            Uint32 *p = (Uint32 *)d + x;
            for(int i = 0; i < w; i++) p[i] = colour;
        }
    }
    return 0;
}

int SDL_UpperBlit(SDL_Surface *src, SDL_Rect *sr, SDL_Surface *dst, SDL_Rect *dr) {
    int sx = sr ? sr->x : 0, sy = sr ? sr->y : 0, w = sr ? sr->w : src->w, h = sr ? sr->h : src->h;
    int dx = dr ? dr->x : 0, dy = dr ? dr->y : 0;
    const int sb = src->format->BitsPerPixel, db = dst->format->BitsPerPixel;
    if(sx < 0) { w += sx; dx -= sx; sx = 0; }
    if(sy < 0) { h += sy; dy -= sy; sy = 0; }
    if(sx + w > src->w) w = src->w - sx;
    if(sy + h > src->h) h = src->h - sy;
    if(dx < 0) { w += dx; sx -= dx; dx = 0; }
    if(dy < 0) { h += dy; sy -= dy; dy = 0; }
    if(dx + w > dst->w) w = dst->w - dx;
    if(dy + h > dst->h) h = dst->h - dy;
    if(w <= 0 || h <= 0) {
        if(dr) dr->w = dr->h = 0;
        return 0;
    }
    if(dr) { dr->x = (Sint16)dx; dr->y = (Sint16)dy; dr->w = (Uint16)w; dr->h = (Uint16)h; }
    for(int j = 0; j < h; j++) {
        const uint8_t *s = (const uint8_t *)src->pixels + (size_t)(sy + j) * src->pitch;
        uint8_t *d = (uint8_t *)dst->pixels + (size_t)(dy + j) * dst->pitch;
        if(sb == db && sb != 1) memcpy(d + dx * (db / 8), s + sx * (sb / 8), (size_t)w * (sb / 8));
        else if(sb == 32 && db == 8) {          /* (a character's two colours: the last two looked up are kept) */
            static Uint32 c0 = 0xFFFFFFFF, c1 = 0xFFFFFFFF;
            static uint8_t i0, i1;
            const Uint32 *p = (const Uint32 *)s + sx;
            for(int i = 0; i < w; i++) {
                const Uint32 c = p[i] & 0xFFFFFF;
                if(c == c0) d[dx + i] = i0;
                else if(c == c1) d[dx + i] = i1;
                else {
                    c1 = c0; i1 = i0;
                    c0 = c;
                    i0 = (uint8_t)pal_index((int)(c >> 16), (int)(c >> 8 & 0xFF), (int)(c & 0xFF));
                    d[dx + i] = i0;
                }
            }
        } else if(sb == 1 && db == 8) {
            for(int i = 0; i < w; i++) d[dx + i] = src->map[s[(sx + i) >> 3] >> (7 - ((sx + i) & 7)) & 1];
        } else if(sb == 8 && db == 32) {
            Uint32 *p = (Uint32 *)d + dx;
            for(int i = 0; i < w; i++) {
                const uint8_t *c = pal + s[sx + i] * 3;
                p[i] = (Uint32)c[0] << 16 | (Uint32)c[1] << 8 | c[2];
            }
        }
    }
    return 0;
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, SDL_PixelFormat *fmt, Uint32 flags) {
    SDL_Surface *s = surface_new(NULL, src->w, src->h, fmt->BitsPerPixel, 0);
    (void)flags;
    if(s) SDL_UpperBlit(src, NULL, s, NULL);
    return s;
}

/* the window: the smallest of these modes that holds it (and that this board can set: a G1 lent its display memory has no MODE 9 in 8 bits) */
SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags) {
    static const struct { int mode, w, h; } modes[] = { {8, 640, 480}, {1, 800, 600}, {9, 1024, 768} };
    int want = -1;
    (void)bpp; (void)flags;
    for(int i = 0; i < 3 && want < 0; i++) if(w <= modes[i].w && h <= modes[i].h) want = i;
    if(want < 0) want = 2;
    for(; want >= 0; want--) {
        if(video_set == modes[want].mode) {
            memset(video.fb, 0, (size_t)video.pitch * video.height);
            break;
        }
        if(cmm2->video_mode(modes[want].mode, 8, &video) >= 0) {
            video_set = modes[want].mode;
            pal_new = 1;
            break;
        }
    }
    if(want < 0) {
        fprintf(stderr, "trs80: cannot set a display mode\n");
        return NULL;
    }
    if(screen.pixels) free(screen.pixels);
    screen.pixels = calloc(1, (size_t)w * h + 4);
    if(screen.pixels == NULL) return NULL;
    screen.format = &fmt8;
    screen.w = w;
    screen.h = h;
    screen.pitch = (Uint16)w;
    off_x = w < (int)video.width ? ((int)video.width - w) / 2 : 0;
    off_y = h < (int)video.height ? ((int)video.height - h) / 2 : 0;
    mouse_place(w / 2, h / 2);                  /* (the pointer in the middle of the new window) */
    return &screen;
}

void SDL_UpdateRect(SDL_Surface *s, Sint32 x, Sint32 y, Uint32 w, Uint32 h) {
    if(s != &screen || !video_set) return;
    if(w == 0 && h == 0) { x = y = 0; w = (Uint32)s->w; h = (Uint32)s->h; }
    if(x < 0) { w += x; x = 0; }
    if(y < 0) { h += y; y = 0; }
    if(x + (int)w > s->w) w = (Uint32)(s->w - x);
    if(y + (int)h > s->h) h = (Uint32)(s->h - y);
    if(x + (int)w > (int)video.width - off_x) w = (Uint32)((int)video.width - off_x - x);   /* (a window larger than the page) */
    if(y + (int)h > (int)video.height - off_y) h = (Uint32)((int)video.height - off_y - y);
    if((int)w <= 0 || (int)h <= 0) return;
    pal_show();
    stat_rects++;
    for(Uint32 j = 0; j < h; j++)
        memcpy(video.fb + (size_t)(off_y + y + j) * video.pitch + off_x + x, (uint8_t *)s->pixels + (size_t)(y + j) * s->pitch + x, w);
}

void SDL_UpdateRects(SDL_Surface *s, int n, SDL_Rect *rects) {
    for(int i = 0; i < n; i++) if(rects[i].w && rects[i].h) SDL_UpdateRect(s, rects[i].x, rects[i].y, rects[i].w, rects[i].h);
}

/* a 24-bit .BMP of a surface (sdltrs's screen shot) */
int SDL_SaveBMP(SDL_Surface *s, const char *name) {
    const int row = (s->w * 3 + 3) & ~3;
    const uint32_t size = 54 + (uint32_t)row * s->h;
    uint8_t head[54] = { 'B', 'M' }, *line = malloc((size_t)row);
    FILE *f = line ? fopen(name, "wb") : NULL;
    if(f == NULL) { free(line); return -1; }
    head[2] = (uint8_t)size; head[3] = (uint8_t)(size >> 8); head[4] = (uint8_t)(size >> 16); head[5] = (uint8_t)(size >> 24);
    head[10] = 54;
    head[14] = 40;
    head[18] = (uint8_t)s->w; head[19] = (uint8_t)(s->w >> 8);
    head[22] = (uint8_t)s->h; head[23] = (uint8_t)(s->h >> 8);
    head[26] = 1;
    head[28] = 24;
    fwrite(head, 1, 54, f);
    memset(line, 0, (size_t)row);
    for(int y = s->h - 1; y >= 0; y--) {
        const uint8_t *p = (const uint8_t *)s->pixels + (size_t)y * s->pitch;
        for(int x = 0; x < s->w; x++) {
            if(s->format->BitsPerPixel == 32) {
                const Uint32 c = ((const Uint32 *)p)[x];
                line[x * 3] = (uint8_t)c; line[x * 3 + 1] = (uint8_t)(c >> 8); line[x * 3 + 2] = (uint8_t)(c >> 16);
            } else {
                const uint8_t *c = pal + p[x] * 3;
                line[x * 3] = c[2]; line[x * 3 + 1] = c[1]; line[x * 3 + 2] = c[0];
            }
        }
        fwrite(line, 1, (size_t)row, f);
    }
    free(line);
    return fclose(f) == 0 ? 0 : -1;
}

void SDL_WM_SetCaption(const char *title, const char *icon) { (void)title; (void)icon; }
SDL_GrabMode SDL_WM_GrabInput(SDL_GrabMode mode) { return mode; }
int SDL_ShowCursor(int toggle) { (void)toggle; return 0; }

/* ---- events ---- */
#define QUEUE 64
static SDL_Event queue[QUEUE];
static uint32_t q_in, q_out;

static void push(const SDL_Event *e) {
    if(q_in - q_out < QUEUE) queue[q_in++ % QUEUE] = *e;
    if(!trace) return;                          /* -keys: each event as sdltrs is given it */
    switch(e->type) {
        case SDL_KEYDOWN: case SDL_KEYUP:
            printf("key %s: place %02X, SDL key %d, character %d, modifiers %04X%s", e->type == SDL_KEYDOWN ? "down" : "up", e->key.keysym.scancode,
                   e->key.keysym.sym, e->key.keysym.unicode, (unsigned)e->key.keysym.mod, "\n");
            break;
        case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
            printf("mouse button %d %s at %d, %d%s", e->button.button, e->button.state ? "down" : "up", e->button.x, e->button.y, "\n");
            break;
        case SDL_MOUSEMOTION: {
            static uint64_t last;
            const uint64_t now = cmm2->micros();
            if(now - last > 500000) printf("mouse at %d, %d%s", e->motion.x, e->motion.y, "\n");
            last = now;
            break;
        }
        case SDL_JOYAXISMOTION: printf("pad axis %d: %d%s", e->jaxis.axis, e->jaxis.value, "\n"); break;
        case SDL_JOYHATMOTION: printf("pad hat: %d%s", e->jhat.value, "\n"); break;
        case SDL_JOYBUTTONDOWN: case SDL_JOYBUTTONUP: printf("pad button %d %s%s", e->jbutton.button, e->jbutton.state ? "down" : "up", "\n"); break;
        case SDL_QUIT: printf("leave%s", "\n"); break;
    }
}

/* -- the keyboard -- */
static int kmod;                                /* SDL's modifier bits, as they are now */
static int typed_shift;

static void key_event(int down, int scancode, int sym, int unicode) {
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    e.key.state = (Uint8)down;
    e.key.keysym.scancode = (Uint8)scancode;
    e.key.keysym.sym = sym;
    e.key.keysym.mod = kmod | (typed_shift ? KMOD_LSHIFT : 0);
    e.key.keysym.unicode = (Uint16)(down ? unicode : 0);
    push(&e);
}

SDLMod SDL_GetModState(void) { return (SDLMod)(kmod | (typed_shift ? KMOD_LSHIFT : 0)); }
int SDL_EnableKeyRepeat(int delay, int interval) { (void)delay; (void)interval; return 0; }
int SDL_EnableUNICODE(int enable) { (void)enable; return 1; }

/* SDL's key for a HID usage: the key in that place on a US keyboard */
static int hid_sym(int u) {
    static const short other[] = {              /* from 0x28 */
        SDLK_RETURN, SDLK_ESCAPE, SDLK_BACKSPACE, SDLK_TAB, SDLK_SPACE, SDLK_MINUS, SDLK_EQUALS, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET,
        SDLK_BACKSLASH, SDLK_HASH, SDLK_SEMICOLON, SDLK_QUOTE, SDLK_BACKQUOTE, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_CAPSLOCK,
        SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12,
        SDLK_PRINT, SDLK_SCROLLOCK, SDLK_PAUSE, SDLK_INSERT, SDLK_HOME, SDLK_PAGEUP, SDLK_DELETE, SDLK_END, SDLK_PAGEDOWN, SDLK_RIGHT,
        SDLK_LEFT, SDLK_DOWN, SDLK_UP, SDLK_NUMLOCK, SDLK_KP_DIVIDE, SDLK_KP_MULTIPLY, SDLK_KP_MINUS, SDLK_KP_PLUS, SDLK_KP_ENTER,
        SDLK_KP1, SDLK_KP2, SDLK_KP3, SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP7, SDLK_KP8, SDLK_KP9, SDLK_KP0, SDLK_KP_PERIOD, SDLK_BACKSLASH };
    if(u >= 0x04 && u <= 0x1D) return SDLK_a + u - 0x04;
    if(u >= 0x1E && u <= 0x26) return SDLK_1 + u - 0x1E;
    if(u == 0x27) return SDLK_0;
    if(u >= 0x28 && u <= 0x64) return other[u - 0x28];
    return 0;
}

/* the character a US keyboard types with that key, if the firmware gives none */
static int us_char(int u, int shift) {
    static const char plain[] = "1234567890\r\033\b\t -=[]\\#;'`,./", shifted[] = "!@#$%^&*()\r\033\b\t _+{}|~:\"~<>?";
    const int i = u - 0x1E;
    if(u >= 0x04 && u <= 0x1D) return (shift ? 'A' : 'a') + u - 0x04;
    if(i >= 0 && i < (int)sizeof(plain) - 1) return (shift ? shifted : plain)[i];
    return 0;
}

static int in_list(const int *list, int n, int v) {
    for(int i = 0; i < n; i++) if(list[i] == v) return 1;
    return 0;
}

static int in_raw(const uint8_t *raw, int u) {
    for(int i = 2; i < 8; i++) if(raw[i] == u) return 1;
    return 0;
}

#define REPEAT_DELAY 500000
#define REPEAT_EVERY 40000
static int usb_keys(void) {
    static const short mod_sym[8] = { SDLK_LCTRL, SDLK_LSHIFT, SDLK_LALT, SDLK_LSUPER, SDLK_RCTRL, SDLK_RSHIFT, SDLK_RALT, SDLK_RSUPER };
    static const short mod_bit[8] = { KMOD_LCTRL, KMOD_LSHIFT, KMOD_LALT, KMOD_LMETA, KMOD_RCTRL, KMOD_RSHIFT, KMOD_RALT, KMOD_RMETA };
    static uint8_t was[8];
    static int codes_was[6], n_was, rep_scan, rep_sym, rep_uni;
    static uint64_t rep_at;
    uint8_t raw[8];
    int keys[7], n = cmm2->keys_held(keys), held = cmm2->keys_raw(raw);
    uint64_t now = cmm2->micros();
    if(n > 6) n = 6;
    if(raw[2] >= 1 && raw[2] <= 3) return held;   /* (too many keys held: the report says nothing) */
    for(int b = 0; b < 8; b++) if((raw[0] ^ was[0]) >> b & 1) {
        const int down = raw[0] >> b & 1;
        if(down) kmod |= mod_bit[b];
        else kmod &= ~mod_bit[b];
        key_event(down, 0xE0 + b, mod_sym[b], 0);
    }
    if((kmod & KMOD_CTRL) && (kmod & KMOD_ALT) && in_raw(raw, 0x4D) && !in_raw(was, 0x4D)) {   /* Ctrl+Alt+End */
        SDL_Event e;
        memset(&e, 0, sizeof(e));
        e.type = SDL_QUIT;
        push(&e);
    }
    for(int i = 2; i < 8; i++) if(was[i] > 3 && !in_raw(raw, was[i])) {
        key_event(0, was[i], hid_sym(was[i]), 0);
        if(was[i] == rep_scan) rep_scan = 0;
    }
    for(int i = 2; i < 8; i++) if(raw[i] > 3 && !in_raw(was, raw[i])) {
        const int u = raw[i], sym = hid_sym(u), shift = (kmod & KMOD_SHIFT) != 0;
        int uni = 0;
        if(sym >= 0x20 && sym < 0x7F) {          /* a key that types: the firmware's character for it, the newest of keys_held's */
            int c = 0;
            for(int k = 0; k < n && c == 0; k++) if(!in_list(codes_was, n_was, keys[k])) c = keys[k];
            if(c >= 1 && c <= 26 && (kmod & KMOD_CTRL)) c += 'a' - 1;   /* (Ctrl with a letter) */
            if(c < 0x20 || c > 0x7E) c = us_char(u, shift);
            if(c >= 'a' && c <= 'z' && shift) c -= 0x20;   /* (a letter's case is Shift's alone: Caps Lock is the TRS-80's own key) */
            else if(c >= 'A' && c <= 'Z' && !shift) c += 0x20;
            uni = c;
        }
        key_event(1, u, sym, uni);
        rep_scan = u; rep_sym = sym; rep_uni = uni;
        rep_at = now + REPEAT_DELAY;
    }
    if(rep_scan && now >= rep_at && q_in == q_out) {
        key_event(1, rep_scan, rep_sym, rep_uni);
        rep_at = now + REPEAT_EVERY;
    }
    memcpy(was, raw, 8);
    n_was = n;
    memcpy(codes_was, keys, sizeof(codes_was));
    return held || raw[0];
}

/* -- the serial console's keys, each pressed for a moment then let go -- */
#define TYPED_DOWN_US 60000
#define TYPED_UP_US 40000
#define TYPED_SCAN 0xFE
static struct { short sym, uni, shift; } typed_q[256], typed_now;
static uint32_t typed_in, typed_out;
static int typed_is_down;
static uint64_t typed_until;

static void typed_put(int sym, int uni, int shift) {
    if(typed_in - typed_out < 256) {
        typed_q[typed_in & 255].sym = (short)sym;
        typed_q[typed_in & 255].uni = (short)uni;
        typed_q[typed_in++ & 255].shift = (short)shift;
    }
}

static void console_keys(int usb_held) {
    static int fprefix, fshift, esc, after_cr, quiet;
    static uint64_t esc_at;
    int c, ok = !usb_held && quiet;
    uint64_t now = cmm2->micros();
    quiet = !usb_held;
    while((c = cmm2->con_getc()) >= 0) {
        const int lf = c == '\n' && after_cr;
        if(!ok) continue;                       /* (a USB key's character reaches the console as well) */
        after_cr = c == '\r';
        if(lf) continue;
        if(esc == 1) {                          /* Esc, then [ and a letter: a terminal's arrow key */
            if(c == '[') { esc = 2; continue; }
            typed_put(SDLK_ESCAPE, 0, 0);
            esc = 0;
        } else if(esc == 2) {
            static const short arrow[4] = { SDLK_UP, SDLK_DOWN, SDLK_RIGHT, SDLK_LEFT };
            esc = 0;
            if(c >= 'A' && c <= 'D') typed_put(arrow[c - 'A'], 0, 0);
            else if(c == 'H') typed_put(SDLK_HOME, 0, 0);
            else if(c == 'F') typed_put(SDLK_END, 0, 0);
            continue;
        }
        if(fprefix) {
            if(c == 'S') { fshift = 1; continue; }
            fprefix = 0;
            if(c >= '1' && c <= '9') typed_put(SDLK_F1 + c - '1', 0, fshift);
            else if(c >= 'a' && c <= 'c') typed_put(SDLK_F10 + c - 'a', 0, fshift);
            fshift = 0;
            continue;
        }
        switch(c) {
            case 6: fprefix = 1; break;         /* Ctrl-F */
            case 0x1D: {                        /* Ctrl-] */
                SDL_Event e;
                memset(&e, 0, sizeof(e));
                e.type = SDL_QUIT;
                push(&e);
                break;
            }
            case 27: esc = 1; esc_at = now; break;
            case '\r': case '\n': typed_put(SDLK_RETURN, 0, 0); break;
            case 8: case 127: typed_put(SDLK_BACKSPACE, 0, 0); break;
            case '\t': typed_put(SDLK_TAB, 0, 0); break;
            default:
                if(c >= 0x20 && c <= 0x7E) typed_put(c >= 'A' && c <= 'Z' ? c + 0x20 : c, c, 0);
        }
    }
    if(esc == 1 && now - esc_at > 50000) {      /* Esc on its own */
        typed_put(SDLK_ESCAPE, 0, 0);
        esc = 0;
    }
    if(now < typed_until) return;
    if(typed_is_down) {
        typed_shift = typed_now.shift;
        key_event(0, TYPED_SCAN, typed_now.sym, 0);
        typed_shift = 0;
        typed_is_down = 0;
        typed_until = now + TYPED_UP_US;
    } else if(typed_out != typed_in) {
        typed_now = typed_q[typed_out++ & 255];
        typed_shift = typed_now.shift;
        key_event(1, TYPED_SCAN, typed_now.sym, typed_now.uni);
        typed_shift = 0;
        typed_is_down = 1;
        typed_until = now + TYPED_DOWN_US;
    }
}

/* -- the mouse: its place in the window, as SDL gives it -- */
/* As the Amiga app's (Peter, 11 Nov 2026): the firmware keeps the mouse's place, clamped to its screen, so it is read every 2 ms (mouse_task:
   from SDL_Delay too, where sdltrs spends most of its time), how far it went is added up, and it is put back in the middle of the page. A fast
   movement is then neither lost between two readings nor stopped at an edge, and the pointer's place in the window is this file's own */
static int mouse_x, mouse_y, mouse_b;
static int mouse_there, mouse_dx, mouse_dy;

static void mouse_task(void) {
    int m[4];
    if(!video_set) return;
    const int cx = (int)video.width / 2, cy = (int)video.height / 2;
    if(!cmm2->mouse(m)) { mouse_there = 0; return; }
    if(mouse_there) {
        mouse_dx += m[0] - cx;
        mouse_dy += m[1] - cy;
    }
    mouse_there = 1;                            /* (first seen: from the middle) */
    if(m[0] != cx || m[1] != cy) cmm2->mouse_set(cx, cy);
}

static void mouse_events(void) {
    static const uint8_t sdl_button[3] = { SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT, SDL_BUTTON_MIDDLE };   /* (the firmware's bits: 1, 2, 4) */
    static int wheel_was, here;
    int m[4], x, y, dx, dy;
    SDL_Event e;
    mouse_task();
    if(!video_set || !mouse_there || !cmm2->mouse(m)) { here = 0; mouse_dx = mouse_dy = 0; return; }
    dx = mouse_dx;
    dy = mouse_dy;
    mouse_dx = mouse_dy = 0;
    x = mouse_x + dx;
    y = mouse_y + dy;
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(x >= screen.w) x = screen.w - 1;
    if(y >= screen.h) y = screen.h - 1;
    memset(&e, 0, sizeof(e));
    if(here && (dx || dy)) {                    /* (how far it moved, even with the pointer at the window's edge) */
        e.type = SDL_MOUSEMOTION;
        e.motion.x = (Uint16)x;
        e.motion.y = (Uint16)y;
        e.motion.xrel = (Sint16)dx;
        e.motion.yrel = (Sint16)dy;
        push(&e);
    }
    mouse_x = x;
    mouse_y = y;
    for(int b = 0; b < 3; b++) if(here && ((m[2] ^ mouse_b) >> b & 1)) {
        memset(&e, 0, sizeof(e));
        e.type = (m[2] >> b & 1) ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        e.button.button = sdl_button[b];
        e.button.state = (Uint8)(m[2] >> b & 1);
        e.button.x = (Uint16)x;
        e.button.y = (Uint16)y;
        push(&e);
    }
    mouse_b = m[2] & 7;
    if(here && m[3] != wheel_was) {             /* the wheel: SDL 1.2's buttons 4 and 5 */
        memset(&e, 0, sizeof(e));
        e.type = SDL_MOUSEBUTTONDOWN;
        e.button.button = m[3] > wheel_was ? SDL_BUTTON_WHEELUP : SDL_BUTTON_WHEELDOWN;
        e.button.state = 1;
        push(&e);
        e.type = SDL_MOUSEBUTTONUP;
        e.button.state = 0;
        push(&e);
    }
    wheel_was = m[3];
    here = 1;
}

static void mouse_poll(void) { mouse_task(); }

static void mouse_place(int x, int y) {
    mouse_x = x;
    mouse_y = y;
    mouse_dx = mouse_dy = 0;
    mouse_there = 0;
}

Uint8 SDL_GetMouseState(int *x, int *y) {
    if(x) *x = mouse_x;
    if(y) *y = mouse_y;
    return (Uint8)((mouse_b & 1 ? SDL_BUTTON(SDL_BUTTON_LEFT) : 0) | (mouse_b & 2 ? SDL_BUTTON(SDL_BUTTON_RIGHT) : 0) |
                   (mouse_b & 4 ? SDL_BUTTON(SDL_BUTTON_MIDDLE) : 0));
}

void SDL_WarpMouse(Uint16 x, Uint16 y) {
    mouse_x = x;
    mouse_y = y;
}

/* -- the first USB gamepad: SDL's joystick 0 -- */
#define SLOTS 8
static int pad_slot(void) {
    for(int slot = 1; slot <= SLOTS; slot++) if(cmm2->gamepad(slot, NULL, NULL) >= 0) return slot;
    return 0;
}

static void pad_events(void) {
    /* DEVICE(GAMEPAD n, B)'s bits: 7 up, 5 down, 8 left, 6 right; 10-13 the face buttons, then the others */
    static const uint8_t button_bit[12] = { 10, 11, 12, 13, 1, 3, 4, 0, 9, 2, 14, 15 };   /* (14: L2, 15: a PS4's touch pad) */
    static int b_was, hat_was, axis_was[2];
    int lx = 128, ly = 128, v[6] = { 128, 128, 128, 128, 0, 0 }, slot = pad_slot(), b, hat, axis[2];
    SDL_Event e;
    if(!slot) b = 0;
    else if(cmm2->version >= 16) b = cmm2->gamepad_axes(slot, v);   /* (both sticks: firmware of 9 Nov 2026 on) */
    else b = cmm2->gamepad(slot, &v[0], &v[1]);
    if(b < 0) b = 0;
    lx = v[0];
    ly = v[1];
    /* the TRS-80 has one joystick, and sdltrs reads SDL's axes 0 and 1 alone: the right stick is that joystick too, when the left is
       not pushed (Peter, 11 Nov 2026) */
    if(lx - 128 > -24 && lx - 128 < 24 && ly - 128 > -24 && ly - 128 < 24) {
        lx = v[2];
        ly = v[3];
    }
    hat = (b >> 7 & 1 ? SDL_HAT_UP : 0) | (b >> 5 & 1 ? SDL_HAT_DOWN : 0) | (b >> 8 & 1 ? SDL_HAT_LEFT : 0) | (b >> 6 & 1 ? SDL_HAT_RIGHT : 0);
    lx -= 128;                                  /* (a stick at rest is not quite in the middle, and never still: seen at -9 to -14 on a G2's
                                                   pad, an event every 20 ms; within 24 of the middle is the middle, which sdltrs needs to see
                                                   the stick let go) */
    ly -= 128;
    axis[0] = lx > -24 && lx < 24 ? 0 : lx * 256;
    axis[1] = ly > -24 && ly < 24 ? 0 : ly * 256;
    for(int a = 0; a < 2; a++) if(axis[a] != axis_was[a]) {
        memset(&e, 0, sizeof(e));
        e.type = SDL_JOYAXISMOTION;
        e.jaxis.axis = (Uint8)a;
        e.jaxis.value = (Sint16)(axis[a] > 32767 ? 32767 : axis[a]);
        push(&e);
        axis_was[a] = axis[a];
    }
    if(hat != hat_was) {
        memset(&e, 0, sizeof(e));
        e.type = SDL_JOYHATMOTION;
        e.jhat.value = (Uint8)hat;
        push(&e);
        hat_was = hat;
    }
    for(int i = 0; i < 12; i++) if((b ^ b_was) >> button_bit[i] & 1) {
        memset(&e, 0, sizeof(e));
        e.type = (b >> button_bit[i] & 1) ? SDL_JOYBUTTONDOWN : SDL_JOYBUTTONUP;
        e.jbutton.button = (Uint8)i;
        e.jbutton.state = (Uint8)(b >> button_bit[i] & 1);
        push(&e);
    }
    b_was = b;
}

int SDL_NumJoysticks(void) { return pad_slot() ? 1 : 0; }
SDL_Joystick *SDL_JoystickOpen(int index) { return index == 0 ? (SDL_Joystick *)&video : NULL; }
void SDL_JoystickClose(SDL_Joystick *j) { (void)j; }
const char *SDL_JoystickName(int index) { (void)index; return "USB gamepad"; }

/* -- the ROM's questions when BASIC starts, answered for the user (Peter, 11 Nov 2026): MEMORY SIZE? on a Model I, Cass? then Memory Size? on a
   Model III or 4. Enter (all the memory; the cassette's fast speed) unless RUN's -himem gave the address to answer with; -ask leaves them to
   the user. A question is known by its text at the top of a screen otherwise empty, where the ROM puts it, and is answered once each time it
   appears (after a reset as well) -- */
extern int mem_read(int address);               /* (sdltrs's: trs_memory.c) */
extern int trs_model;
static int rom_ask;
static char rom_himem[8];

static int video_char(int pos) {
    int c = mem_read(0x3C00 + pos) & 0xFF;
    if(c < 0x20) c += 0x40;                     /* (a Model I without lower case keeps its capitals as 0x00-0x1F) */
    if(c >= 'a' && c <= 'z') c -= 0x20;
    return c;
}

static int video_is(int pos, const char *s) {
    for(; *s; s++, pos++) if(video_char(pos) != *s) return 0;
    return 1;
}

static int video_empty(int pos, int n) {         /* nothing typed or printed there: no letter or digit */
    for(; n > 0; n--, pos++) {
        const int c = video_char(pos);
        if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) return 0;
    }
    return 1;
}

static void rom_questions(void) {
    static int answered;
    static int seen, tidy;
    static uint64_t seen_at, tidy_at;
    const uint64_t now = cmm2->micros();
    const int top = video_empty(0, 64) ? 64 : 0;   /* (a Model III's ROM starts on the screen's second line) */
    int q = 0;                                  /* 1: the memory size, 2: the cassette's speed */
    if(trs_model == 1) {
        if(video_is(top, "MEMORY SIZE?") && video_empty(top + 12, 64 - 12 + 64)) q = 1;
    } else if(trs_model == 3 || trs_model == 4) {
        if(video_is(top, "CASS?") && video_empty(top + 5, 64 - 5)) {
            if(video_empty(top + 64, 128)) q = 2;
            else if(video_is(top + 64, "MEMORY SIZE?") && video_empty(top + 64 + 12, 64 - 12 + 64)) q = 1;
        }
    }
    if(q != seen) { seen = q; seen_at = now; }
    if(tidy && q == 0) {                        /* answered: once BASIC is at READY, CLS, so that the questions are not left on the screen
                                                   (Peter, 11 Nov 2026) */
        if(now - tidy_at > 5000000) tidy = 0;
        else if(typed_in == typed_out && !typed_is_down) for(int r = top + 64; r < top + 64 * 6; r += 64)
            if(video_is(r, "READY") && video_empty(r + 5, 59) && video_char(r + 64) == '>' && video_empty(r + 65, 63 + 64)) {
                typed_put('c', 'C', 0);
                typed_put('l', 'L', 0);
                typed_put('s', 'S', 0);
                typed_put(SDLK_RETURN, 0, 0);
                tidy = 0;
                break;
            }
    }
    if(q == 0) { answered = 0; return; }
    if(q == answered || typed_in != typed_out || typed_is_down) return;
    if(now - seen_at < 500000) return;          /* (once it has been there half a second: the ROM is then reading the keyboard) */
    answered = q;
    if(q == 1) {
        tidy = 1;
        tidy_at = now;
        for(const char *p = rom_himem; *p; p++) typed_put(*p, *p, 0);
    }
    typed_put(SDLK_RETURN, 0, 0);
}

/* -- a DOS's questions as it starts, Date and Time, answered from the Colour Maximite 2's clock (Peter, 11 Nov 2026: "why does it prompt for
   the time when it can read it from the rtc": a TRS-80 has no clock of its own, so LDOS, LS-DOS and the others ask). A question is the last
   line with anything on it, starting Date or Time and ending in a question mark with nothing typed after it. The time is answered once after
   a reset and the date twice at most (see below): a DOS that still refuses asks again, and that is left to the user. -ask leaves them all -- */
extern Uint8 *cmm2_trs_video(void);             /* (trs_memory.c's screen memory, by patches/) */
extern int text80x24;                           /* (sdltrs's: the Model 4's 80 x 24 screen) */
static int dos_dates, dos_times;                /* how many times each has been answered since the reset */

static void dos_questions(void) {
    static int seen;
    static uint64_t seen_at;
    const Uint8 *v = cmm2_trs_video();
    const int cols = text80x24 ? 80 : 64, rows = text80x24 ? 24 : 16;
    const uint64_t now = cmm2->micros();
    char line[81];
    int q = 0, n = 0, t[6];
    for(int r = rows - 1; r >= 0 && n == 0; r--) {  /* the last line with a letter or a digit on it, without its leading spaces */
        for(int c = 0; c < cols; c++) {
            int ch = v[r * cols + c];
            if(ch < 0x20) ch += 0x40;
            if(ch >= 'a' && ch <= 'z') ch -= 0x20;
            if(ch < 0x20 || ch > 0x7E) ch = ' ';
            if(n > 0 || ch != ' ') line[n++] = (char)ch;
        }
        while(n > 0 && (line[n - 1] == ' ' || line[n - 1] == '_')) n--;   /* (and without the cursor) */
        line[n] = 0;
        for(int i = 0, any = 0; i < n && !any; i++) {
            any = (line[i] >= '0' && line[i] <= '9') || (line[i] >= 'A' && line[i] <= 'Z');
            if(i == n - 1 && !any) n = 0;
        }
    }
    if(n >= 5 && line[n - 1] == '?') q = strncmp(line, "DATE", 4) == 0 ? 1 : strncmp(line, "TIME", 4) == 0 ? 2 : 0;
    if(q != seen) { seen = q; seen_at = now; }
    if(q == 0 || typed_in != typed_out || typed_is_down || now - seen_at < 500000) return;
    if(q == 2 ? dos_times >= 1 : dos_dates >= 2) return;
    cmm2->datetime(t);
    /* the date: the year as it is first; asked again (LS-DOS 6.3.1 knows 1980 to 2011), the year 28 before, whose calendar is the same,
       day for day: 2026 as 98 */
    if(q == 1) snprintf(line, sizeof(line), "%02d/%02d/%02d", t[1], t[2], (t[0] - 28 * dos_dates++) % 100);
    else {
        dos_times++;
        snprintf(line, sizeof(line), "%02d:%02d:%02d", t[3], t[4], t[5]);
    }
    for(const char *p = line; *p; p++) typed_put(*p, *p, 0);
    typed_put(SDLK_RETURN, 0, 0);
}

static void pump(void) {
    cmm2->poll();
    cmm2->break_take();                         /* (Ctrl-C is the TRS-80's) */
    if(!rom_ask) {
        rom_questions();
        dos_questions();
    }
    console_keys(usb_keys());
    mouse_events();
    pad_events();
    stat_tick();
}

int SDL_PollEvent(SDL_Event *e) {
    if(q_in == q_out) pump();
    if(q_in == q_out) return 0;
    if(e) *e = queue[q_out % QUEUE];
    q_out++;
    return 1;
}

int SDL_WaitEvent(SDL_Event *e) {
    pal_show();
    while(!SDL_PollEvent(e)) {
        uint64_t until = cmm2->micros() + 2000;
        stat_idle += 2000;
        while(cmm2->micros() < until) { }
    }
    return 1;
}

/* ---- the sound: sdltrs's callback fills the firmware's stream, from its interrupt (as SDL calls it from its own thread) ---- */
static void (*volatile audio_cb)(void *userdata, Uint8 *stream, int len);
static void *audio_ud;
static volatile int audio_paused = 1;
static int audio_rate, audio_chans = 1;
static uint32_t audio_step = 0x10000;           /* sdltrs's frames for each of the stream's, in 65536ths */
#define AUDIO_TMP 4096
static int16_t audio_tmp[AUDIO_TMP * 2];

/* in PendSV: no services */
static void fill(int16_t *buf, uint32_t frames) {
    static uint32_t acc;
    void (*cb)(void *, Uint8 *, int) = audio_cb;
    if(cb == NULL || audio_paused) {
        memset(buf, 0, frames * 4);
        return;
    }
    stat_sound += frames;
    if(audio_chans == 2 && audio_step == 0x10000) {
        cb(audio_ud, (Uint8 *)buf, (int)(frames * 4));
        return;
    }
    acc += frames * audio_step;
    uint32_t n = acc >> 16;
    acc &= 0xFFFF;
    if(n > AUDIO_TMP) n = AUDIO_TMP;
    if(n == 0) n = 1;
    cb(audio_ud, (Uint8 *)audio_tmp, (int)(n * audio_chans * 2));
    for(uint32_t i = 0; i < frames; i++) {
        const uint32_t j = i * n / frames;
        if(audio_chans == 2) {
            *buf++ = audio_tmp[j * 2];
            *buf++ = audio_tmp[j * 2 + 1];
        } else {
            *buf++ = audio_tmp[j];
            *buf++ = audio_tmp[j];
        }
    }
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
    if(desired->format != AUDIO_S16) return -1;
    if(audio_rate <= 0) {
        audio_rate = cmm2->audio_start(fill);
        if(audio_rate <= 0) return -1;
    }
    cmm2->audio_lock();
    audio_chans = desired->channels == 2 ? 2 : 1;
    audio_step = (uint32_t)(((uint64_t)desired->freq << 16) / (uint32_t)audio_rate);
    if(audio_step > 0xFF00 && audio_step < 0x10100) audio_step = 0x10000;   /* (44.1 kHz as the firmware's timer makes it) */
    audio_ud = desired->userdata;
    audio_paused = 1;
    audio_cb = desired->callback;
    cmm2->audio_unlock();
    desired->silence = 0;
    desired->size = (Uint32)desired->samples * audio_chans * 2;
    if(obtained) *obtained = *desired;
    return 0;
}

void SDL_CloseAudio(void) { audio_cb = NULL; }  /* (the stream goes on, silent: the firmware ends it with the program) */
void SDL_PauseAudio(int on) { audio_paused = on; }
void SDL_LockAudio(void) { if(audio_rate > 0) cmm2->audio_lock(); }
void SDL_UnlockAudio(void) { if(audio_rate > 0) cmm2->audio_unlock(); }

/* ---- the TRS-80's memory (trs_memory.c's array, by patches/): sdltrs's 4 MB and 64 KB, of which all above 1 MB and 64 KB is the MegaMem
   expansion's, from MMBasic's heap where that has the room: a G2 (Peter, 11 Nov 2026: the MegaMem a G2's alone). A G1's heap is lent to the
   program, so there it is 1 MB and 64 KB of the program's own, and the MegaMem is not there ---- */
Uint8 *cmm2_trs_memory(Uint32 *size) {
    Uint32 n = 4 * 1024 * 1024 + 65536;
    Uint8 *p = cmm2->alloc(n + 1);
    if(p == NULL) {
        n = 1 * 1024 * 1024 + 65536;
        p = malloc(n + 1);
    }
    if(p == NULL) {
        fprintf(stderr, "trs80: no room for the TRS-80's memory\n");
        exit(1);
    }
    memset(p, 0, n + 1);
    if(stats) printf("trs80: %lu KB for the TRS-80's memory%s\n", (unsigned long)(n >> 10), n > 2 * 1024 * 1024 ? " (the MegaMem's too)" : "");
    *size = n;
    return p;
}

/* ---- what the TRS-80 is when RUN gives no words to say (Peter, 11 Nov 2026: a bare RUN showed a blank screen, a Model I waiting for a disk
   that was not there). Two rules, each given way to by sdltrs's own option on RUN's line:
   - the model (-model): if the ROM of the one chosen is not in the folder, a model whose ROM is: the Model I (level2.rom), else the
     Model III (model3.rom), else the Model 4P, which needs none (sdltrs's free boot ROM);
   - the disk controller (-fdc, -nofdc), at each reset of a Model I, III or 4: there if there is a disk in drive 0, which the TRS-80 then
     starts from, and not there if the drive is empty, when its ROM goes to BASIC instead of waiting for a disk. A disk put in from
     sdltrs's menu is started from at the next reset (F10) ---- */
extern int trs_disk_controller;                 /* (sdltrs's: trs_disk.c, trs_options.c) */
extern const char *trs_disk_getfilename(int unit);
extern char romfile1[], romfile3[];
static int said_model, said_fdc;

static int file_there(const char *name) {
    FILE *f = fopen(name, "rb");
    if(f) fclose(f);
    return f != NULL;
}

void cmm2_trs_start(void) {
    if(said_model) return;
    if(trs_model == 5 || file_there(trs_model == 1 ? romfile1 : romfile3)) return;
    trs_model = file_there(romfile1) ? 1 : file_there(romfile3) ? 3 : 5;
}

void cmm2_trs_reset(void) {
    dos_dates = dos_times = 0;
    if(said_fdc || trs_model == 5) return;
    trs_disk_controller = trs_disk_getfilename(0)[0] != '\0';
}

/* ---- what sdltrs calls of Unix that the SDK's C library has not ---- */
/* a file's length changed: sdltrs shortens a JV3 disk image whose last sector is freed, which only saves room, so a shorter length is left
   alone here (the card's files cannot be cut short by a service); a longer one is filled with zeros */
int ftruncate(int fd, off_t length) {
    static const char zeros[256];
    const off_t here = lseek(fd, 0, SEEK_CUR), size = lseek(fd, 0, SEEK_END);
    int r = 0;
    if(here < 0 || size < 0) return -1;
    for(off_t n = size; n < length && r == 0;) {
        const int part = length - n > (off_t)sizeof(zeros) ? (int)sizeof(zeros) : (int)(length - n);
        if(write(fd, zeros, (size_t)part) != part) r = -1;
        n += part;
    }
    lseek(fd, here, SEEK_SET);
    return r;
}

/* a file's permissions, by which sdltrs's menu write-protects a disk image: the card's files have none to set */
int chmod(const char *name, mode_t mode) { (void)name; (void)mode; errno = ENOSYS; return -1; }
mode_t umask(mode_t mask) { (void)mask; return 0; }

/* ---- the start: sdltrs's main (main.c's, named sdltrs_main by patches/), with this port's own words taken out ---- */
int sdltrs_main(int argc, char **argv);

int main(int argc, char **argv) {
    static char *args[64];
    int n = 0;
    for(int i = 0; i < argc && n < 63; i++) {
        if(i > 0 && strcmp(argv[i], "-stats") == 0) stats = 1;
        else if(i > 0 && strcmp(argv[i], "-keys") == 0) trace = 1;
        else if(i > 0 && strcmp(argv[i], "-ask") == 0) rom_ask = 1;
        else if(i > 0 && strcmp(argv[i], "-himem") == 0 && i + 1 < argc) {
            const long v = strtol(argv[++i], NULL, 10);
            if(v > 0 && v <= 65535) snprintf(rom_himem, sizeof(rom_himem), "%ld", v);
            else fprintf(stderr, "trs80: -himem is an address, 1 to 65535\n");
        } else {
            static const char *const model[] = { "-model", "-m", "-m1", "-m3", "-m4", "-m4p" };
            static const char *const fdc[] = { "-fdc", "-floppy", "-nofdc", "-nofloppy" };
            for(int k = 0; k < 6; k++) if(strcasecmp(argv[i], model[k]) == 0) said_model = 1;
            for(int k = 0; k < 4; k++) if(strcasecmp(argv[i], fdc[k]) == 0) said_fdc = 1;
            args[n++] = argv[i];
        }
    }
    args[n] = NULL;
    stat_from = cmm2->micros();
    cmm2->con_screen(0);                        /* (sdltrs's messages to the serial console alone) */
    return sdltrs_main(n, args);
}
