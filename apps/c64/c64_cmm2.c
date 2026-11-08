/* c64_cmm2.c: frank-c64's drivers on the Colour Maximite 2, by the loader's services (Peter, 8 Nov 2026). This file has what frank-c64's main_rp2350.c and sid_i2s.cpp
   give the rest of it (those files are the RP2350's start-up and sound, and are not built): the frame pacing, the sound ring and main, which runs the emulator as
   main_rp2350.c does.

   - The display: MODE 7,8, 320 x 240 in 256 colours from the palette, each line shown twice: the 320 x 240 of the VIC's 384 x 272 that frank-c64 shows (the C64's
     screen and 32 pixels of border each side, 20 above and below). The VIC draws each frame in one of two buffers, frank-c64's g_pixels and vic2 here, and the one
     last finished goes to the page at the start of the display's blanking, in the frame hook, by the DMA2D, so it is never shown half drawn. The display's 59.94
     frames a second do not come into the emulator's 50. The start screen and the disk menu (F10) are drawn in g_pixels, and shown from it while they are up.
   - The sound: the SID's samples (44,100 a second of the C64's time, 882 a frame) go through a ring to the firmware's sound stream, at its rate by a phase step,
     trimmed to hold the ring's level: the emulator keeps its own time, a frame each 20 ms.
   - The keyboard: what is on the key is what comes out, in any OPTION USBKEYBOARD layout, as on the CPC (Peter, 6 Nov 2026): each key becomes the C64 key that types
     the character the firmware's layout makes of it, with the C64's Shift as that character needs (keys_by_character), and the USB keyboard's reports so changed
     become usbhid.h's key actions, which frank-c64's usbhid_wrapper.c and input_rp2350.cpp turn into the C64's matrix. Letters keep the Shift held, as on a C64:
     shifted, they are its capitals or its graphics. Characters the C64 has on keys of its own: ^ is its up arrow, _ its left arrow, \ and the pound sign its pound.
     The keys frank-c64 gives the rest: Esc RUN/STOP, Tab and the left Ctrl CTRL, the left Alt C=, Home CLR/HOME, Insert INST, Backspace and Delete DEL, F1-F8 the
     C64's F1-F8; F9 swaps the joystick ports, F10 the disk menu, F11 RESTORE, Ctrl+Alt+Del resets the C64. The arrows are the cursor keys and the first joystick
     as well, the right Ctrl and Alt its fire; the left arrow is the C64's Shift and CRSR right (frank-c64 made it DEL), and the arrows' joystick is taken from
     the keys as they are, not as they are given to the C64. Ctrl-C is not MMBasic's here: Ctrl+Alt+End leaves, as it does pc386 and the CPC.
   - The gamepads: the first USB pad is the first joystick (port 2), the second the other (frank-c64's second NES pad): the d-pad or the left stick, and any of
     the four face buttons the fire. A DS3 sends nothing until its PS button has been pressed.
   - The mouse (USB or PS/2, the firmware's): a Commodore 1351 on port 1 (GEOS's, and art programs'), by the SID's POT X and Y (mouse_task, Peter, 8 Nov 2026).
   - The serial console types too, for remote use:
       a character is the C64 key that types it, with Shift if it needs it (" is Shift 2, [ is Shift :, and so on), a letter in either case the C64's letter
       key without Shift, which is a capital, as C64 programs are typed (Ctrl-V then a letter: with Shift, its graphic, or in the lower case set its capital);
       Enter, Esc (RUN/STOP), Tab (CTRL), Backspace (DEL) as they are;
       Ctrl-U, Ctrl-D, Ctrl-L, Ctrl-R the arrows;
       Ctrl-F then 1-9, a, b, c for F1-F12: Ctrl-F a is F10, the disk menu, Ctrl-F b is F11, RESTORE;
       Ctrl-Y then a key: CTRL and that key; Ctrl-O then a key: C= and that key;
       Ctrl-] leaves.
     While a USB key is held its character reaches the console as well, and is left out.
   RUN "c64.app", -keys prints each key as it goes down: its code, the character the firmware made of it and what the C64 is given.
   RUN "c64.app", -stats prints the frames a second, how much of the time the emulator has to spare, the sound ring's level and the heap in use, every 5 s. */
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmm2.h"
#include "nespad/nespad.h"
#include "startscreen.h"
#include "usbhid.h"
#include "usbhid_wrapper.h"

/* frank-c64's, in its C++ files */
void c64_init(void);
bool c64_run_frame(void);
void input_rp2350_init(void);
void input_rp2350_poll_no_c64_acts(void);
bool disk_ui_is_visible(void);
void disk_ui_render(void);
void disk_loader_init(void);
void disk_loader_scan(void);

/* ---- time ---- */
static void palette_show(void);
uint64_t time_us_64(void) { return cmm2->micros(); }
void sleep_us(uint64_t us) { usleep((useconds_t)us); }
void sleep_ms(uint32_t ms) {                    /* (the start screen's wait: its colours are shown first) */
    palette_show();
    usleep((useconds_t)ms * 1000);
}

/* -stats: the frames, the time the emulator had to spare and the sound ring, since the last report */
static int stats = -1;
static uint32_t stat_frames;
static uint64_t stat_idle, stat_from;
static volatile uint32_t ring_in, ring_out;     /* (the sound's: ring_out in samples, ring_phase its fraction) */

void tight_loop_contents(void) { }

static bool has_arg(const char *a) {
    for(int i = 1; i < cmm2_start->argc; i++) if(strcmp(cmm2_start->argv[i], a) == 0) return true;
    return false;
}

static void stat_frame(void) {
    uint64_t now = cmm2->micros();
    if(stats < 0) {
        stats = has_arg("-stats");
        stat_from = now;
    }
    if(!stats) return;
    stat_frames++;
    if(now - stat_from >= 5000000) {
        uint32_t ms = (uint32_t)((now - stat_from) / 1000), idle = (uint32_t)(stat_idle / 1000);
        printf("c64: %lu.%lu frames a second, %lu%% spare, sound ring %lu, heap %lu\n", (unsigned long)(stat_frames * 1000 / ms),
               (unsigned long)(stat_frames * 10000 / ms % 10), (unsigned long)(idle * 100 / ms), (unsigned long)(ring_in - ring_out), (unsigned long)mallinfo().uordblks);
        stat_frames = 0;
        stat_idle = 0;
        stat_from = now;
    }
}

/* libstdc++'s statics (std::format's and std::filesystem's system_error) register their destructors with it: a program never unloads them */
void *__dso_handle = &__dso_handle;

#ifdef C64_TRACE_THROW                          /* (diagnostic: a C++ exception, which nothing here catches, named before the program stops) */
void __cxa_throw(void *obj, void *tinfo, void (*dest)(void *)) {
    const struct { void *vptr; const char *name; } *ti = tinfo;
    const char *(*what)(void *) = (const char *(*)(void *))((void ***)obj)[0][2];
    (void)dest;
    printf("c64: C++ exception %s from %p: %s\n", ti->name, __builtin_return_address(0), what(obj));
    sleep_ms(500);
    exit(3);
}
void *_Znwj(unsigned int n) {                  /* operator new(unsigned int) */
    void *p = malloc(n);
    if(p == NULL) printf("c64: new of %u bytes from %p failed, heap %lu in use\n", n, __builtin_return_address(0), (unsigned long)mallinfo().uordblks);
    return p;
}
void abort(void) {
    printf("c64: abort from %p\n", __builtin_return_address(0));
    sleep_ms(500);
    _exit(4);
}
#endif

/* ---- the heap for what frank-c64 puts in a board's PSRAM: the cartridges ---- */
void *psram_malloc(size_t size) { return malloc(size); }
void *psram_realloc(void *ptr, size_t size) { return realloc(ptr, size); }
void psram_free(void *ptr) { free(ptr); }

/* ---- the display ---- */
#define VIC_W 384                               /* Display_rp2350.h's DISPLAY_X, DISPLAY_Y */
#define VIC_H 272
#define PIC_W 320                               /* board_config.h's FB_WIDTH, FB_HEIGHT, C64_CROP_LEFT, C64_CROP_TOP */
#define PIC_H 240
#define CROP_X 32
#define CROP_Y 16
extern uint8_t g_pixels[VIC_W * VIC_H];         /* (Display_rp2350.cpp's: the start screen and the disk menu draw in it) */
static uint8_t __attribute__((aligned(4))) vic2[VIC_W * VIC_H];
static uint8_t *vic_next = g_pixels;            /* the buffer the VIC draws the next frame in */
static cmm2_video video;
static const uint8_t *volatile ready;           /* a whole frame not yet shown */
static const uint8_t *volatile live;            /* shown every frame (the start screen, the disk menu) */
static uint8_t palette[256 * 3], shown[256 * 3];
static volatile bool palette_new;

/* the VIC's, at the first line of each frame (Display::BitmapBase, patches/) */
uint8_t *cmm2_vic_bitmap(void) { return vic_next; }

/* in PendSV, at the start of each frame's blanking */
static void frame(void) {
    const uint8_t *s = live;
    if(s == NULL) {
        s = ready;
        ready = NULL;
    }
    if(s == NULL) return;
    s += CROP_Y * VIC_W + CROP_X;
    cmm2->dma2d_copy(s, VIC_W, video.fb, video.pitch, PIC_W, PIC_H);
    if(video.copies == 2) cmm2->dma2d_copy(s, VIC_W, video.fb + video.width, video.pitch, PIC_W, PIC_H);
}

static void palette_show(void) {               /* (in the program: video_palette waits for the next frame) */
    if(!palette_new) return;
    palette_new = false;
    if(memcmp(palette, shown, sizeof(shown)) == 0) return;
    memcpy(shown, palette, sizeof(shown));
    cmm2->video_palette(0, 256, shown);
}

/* after each frame the emulator has run: it goes to the screen, and the VIC draws the next in the other buffer */
static void frame_done(void) {
    palette_show();
    ready = vic_next;
    vic_next = vic_next == g_pixels ? vic2 : g_pixels;
    stat_frame();
}

void graphics_set_palette(uint8_t i, uint32_t rgb) {
    palette[i * 3] = rgb >> 16;
    palette[i * 3 + 1] = rgb >> 8;
    palette[i * 3 + 2] = rgb;
    palette_new = true;
}

uint32_t graphics_get_palette(uint8_t i) { return (uint32_t)palette[i * 3] << 16 | palette[i * 3 + 1] << 8 | palette[i * 3 + 2]; }

/* main_rp2350.c's init_c64_palette: the C64's 16 colours (Pepto's), the disk menu's and greys */
static void c64_palette(void) {
    static const uint32_t c64[16] = { 0x000000, 0xFFFFFF, 0x68372B, 0x70A4B2, 0x6F3D86, 0x588D43, 0x352879, 0xB8C76F,
                                      0x6F4F25, 0x433900, 0x9A6759, 0x444444, 0x6C6C6C, 0x9AD284, 0x6C5EB5, 0x959595 };
    for(int i = 0; i < 16; i++) graphics_set_palette(i, c64[i]);
    graphics_set_palette(16, 0xD0D0D0);
    graphics_set_palette(17, 0xF0F0F0);
    graphics_set_palette(18, 0x404040);
    graphics_set_palette(19, 0xF00000);
    graphics_set_palette(20, 0x300000);
    graphics_set_palette(21, 0x00C000);
    for(int i = 22; i < 240; i++) {
        uint8_t g = (i * 255) / 239;
        graphics_set_palette(i, (uint32_t)g << 16 | g << 8 | g);
    }
}

/* ---- the sound ---- */
#define RING 8192                               /* a power of 2 */
#define RING_LEVEL 3000                         /* samples: a stream buffer of 1024, a frame of 882 and more */
#define RING_FULL 7000
#define SID_HZ 44100                            /* its samples a second: 882 a frame of 20 ms */
static int16_t ring_l[RING], ring_r[RING];
static uint32_t ring_phase, ring_step, out_rate;
static int audio_on;

/* in PendSV: no services. The emulator keeps its own time and the stream the board's, so the step is trimmed, by up to 1%, to hold the ring at RING_LEVEL */
static void fill(int16_t *buf, uint32_t frames) {
    static bool playing;
    static int16_t l, r;
    static int32_t smooth = RING_LEVEL << 4;
    uint32_t out = ring_out, in = ring_in, ph = ring_phase, level = in - out;
    smooth += (int32_t)level - (smooth >> 4);
    int32_t err = (smooth >> 4) - RING_LEVEL;
    if(err > RING_LEVEL) err = RING_LEVEL;
    if(err < -RING_LEVEL) err = -RING_LEVEL;
    uint32_t step = ring_step + (int32_t)(((int64_t)ring_step * err) / (RING_LEVEL * 100));
    if(!playing && level >= RING_LEVEL) playing = true;
    while(frames--) {
        if(playing) {
            if(out == in) playing = false;      /* (empty: waits for RING_LEVEL again) */
            else {
                l = ring_l[out & (RING - 1)];
                r = ring_r[out & (RING - 1)];
                ph += step;
                out += ph >> 16;
                ph &= 0xFFFF;
                if((int32_t)(in - out) < 0) out = in;
            }
        }
        *buf++ = l;
        *buf++ = r;
    }
    ring_out = out;
    ring_phase = ph;
}

void sid_i2s_init(void) {
    int r;
    if(audio_on) return;
    audio_on = 1;
    r = cmm2->audio_start(fill);
    if(r < 0) audio_on = -1;
    else {
        out_rate = r;
        ring_step = (uint32_t)(((uint64_t)SID_HZ << 16) / out_rate);
    }
}

void sid_i2s_update(void) { }                   /* (the stream takes them as it needs them) */

/* SID_rp2350.cpp's mix is the voices times the volume >> 16, quiet (Peter, 8 Nov 2026: "quiet but correct"): one voice at full volume is +-1,900, all three
   +-5,700. Four times that, all three are +-23,000, and the filter's peaks beyond are clipped */
#define SID_GAIN 4
static int16_t sid_gain(int16_t s) {
    int32_t v = (int32_t)s * SID_GAIN;
    return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v;
}

void sid_add_sample(int16_t left, int16_t right) {
    uint32_t in = ring_in;
    if(audio_on <= 0 || in - ring_out >= RING_FULL) return;
    ring_l[in & (RING - 1)] = sid_gain(left);
    ring_r[in & (RING - 1)] = sid_gain(right);
    ring_in = in + 1;
}

int sid_get_buffer_fill(void) { return (int)(ring_in - ring_out); }

/* ---- the serial console's keys: each pressed for a moment, then let go ---- */
#define TYPED_US 70000
#define SHIFT 0x02                              /* a report's modifier bits */
#define CTRL 0x01
#define ALT 0x04
static uint8_t typed_key, typed_mod;
static uint64_t typed_until;
static uint8_t typed_q[256], typed_qm[256];
static uint32_t typed_in, typed_out;

static void typed_put(uint8_t key, uint8_t mod) {
    if(typed_in - typed_out >= sizeof(typed_q)) return;
    typed_q[typed_in & 255] = key;
    typed_qm[typed_in++ & 255] = mod;
}

/* The USB key whose code in usbhid_wrapper.c is the C64 key that types the character c, and in shift whether that takes the C64's Shift (1), not (0), or as held
   (-1: letters). 0 if the C64 has no key for it. usbhid_wrapper.c's keys go by place, a PC's US layout over the C64's: [ is @, ] is *, \ is =, ; is :, ' is ;,
   ` the left arrow, Page Up the up arrow, End the pound; and - is -, = is +: usbhid_wrapper.c says - gives + and = gives -, but input_rp2350.cpp's matrix has
   those two the other way round (+ is row 5 bit 0 on a C64, - row 5 bit 3), and it is the matrix that counts */
static uint8_t char_place(int c, int *shift) {
    static const char digits[] = "!\"#$%&'()";
    static const struct { char c; uint8_t place, shift; } other[] = {
        {'+', 0x2E, 0}, {'-', 0x2D, 0}, {'@', 0x2F, 0}, {'*', 0x30, 0}, {'=', 0x31, 0}, {':', 0x33, 0}, {';', 0x34, 0}, {',', 0x36, 0}, {'.', 0x37, 0},
        {'/', 0x38, 0}, {'<', 0x36, 1}, {'>', 0x37, 1}, {'?', 0x38, 1}, {'[', 0x33, 1}, {']', 0x34, 1}, {'^', 0x4B, 0}, {'_', 0x35, 0}, {'\\', 0x4D, 0},
        {(char)163, 0x4D, 0}, {' ', 0x2C, 0} };
    const char *p;
    *shift = 0;
    if(c >= 'a' && c <= 'z') { *shift = -1; return 4 + c - 'a'; }
    if(c >= 'A' && c <= 'Z') { *shift = -1; return 4 + c - 'A'; }
    if(c == '0') return 0x27;
    if(c >= '1' && c <= '9') return 0x1E + c - '1';
    if(c <= 0 || c > 255) return 0;
    if((p = strchr(digits, c)) != NULL) { *shift = 1; return 0x1E + (p - digits); }
    for(unsigned i = 0; i < sizeof(other) / sizeof(other[0]); i++)
        if((uint8_t)other[i].c == c) { *shift = other[i].shift; return other[i].place; }
    return 0;
}

static int keytrace = -1;                       /* -keys */

/* is h a USB key that types a character (the main block's, the keypad's), which goes by that character and not by its place? */
static bool by_character(uint8_t h) {
    return (h >= 0x04 && h <= 0x27) || (h >= 0x2D && h <= 0x38) || h == 0x64 || (h >= 0x54 && h <= 0x63 && h != 0x58);
}

/* The report as the C64 should see it: each key the C64 key for the character it makes (keys_held has them, the last key of the report first), the C64's Shift
   as that character needs and not as the PC's is: Shift and 2 is " on a C64 and on a UK keyboard, but @ on a US one, which on a C64 is a key of its own. Decided
   as the key goes down, so that a Shift let go first does not change a key still held. Keys that make no character keep their places; one that makes a
   character the C64 has not is left out. The keypad's Enter is RETURN. */
static void keys_by_character(uint8_t *r) {
    static uint8_t was[6], place[256];
    static int8_t shift[256];
    uint8_t out[6] = {0};
    int keys[7], total = 0, force = -1, i, fresh = 0, mods = r[0];
    if(keytrace < 0) keytrace = has_arg("-keys");
    for(i = 0; i < 6 && r[i + 2]; i++) total++;
    cmm2->keys_held(keys);
    for(i = 0; i < total; i++) {
        uint8_t h = r[i + 2];
        if(memchr(was, h, sizeof(was)) == NULL) {
            int c = keys[total - 1 - i], s = -1;
            uint8_t p = h;
            if(c >= 1 && c <= 26) c += 96;                      /* (Ctrl with a letter) */
            if(h == 0x58) p = 0x28;
            else if(h == 0x50) { p = 0x4F; s = 1; }             /* left: the C64's Shift and CRSR right (usbhid_wrapper.c makes it DEL) */
            else if(by_character(h)) p = char_place(c, &s);
            place[h] = p;
            shift[h] = p ? s : -1;
            fresh = 1;
            if(keytrace) printf("key %02X of %d: character %d, place %02X, shift %d\n", h, total, c, place[h], shift[h]);
        }
        out[i] = place[h];
        if(shift[h] >= 0) force = shift[h];
    }
    if(force == 0) r[0] &= ~0x22;
    else if(force == 1) r[0] |= 0x02;
    memcpy(was, r + 2, sizeof(was));
    for(i = 0; i < 6; i++) r[i + 2] = 0;
    for(i = 0, total = 0; i < 6; i++) if(out[i]) r[2 + total++] = out[i];
    if(force >= 0) r[0] &= ~0x40;                               /* (AltGr made the character: it is not the joystick's fire) */
    if(keytrace && fresh) printf("  modifiers %02X became %02X, keys %02X %02X\n", mods, r[0], r[2], r[3]);
}

static void ascii_key(int c, uint8_t mod) {
    int s;
    uint8_t p;
    if(c == 13 || c == 10) typed_put(0x28, mod);
    else if(c == 27) typed_put(0x29, mod);
    else if(c == 8 || c == 127) typed_put(0x2A, mod);
    else if(c == 9) typed_put(0x2B, mod);
    else if((p = char_place(c, &s)) != 0) typed_put(p, s == 1 ? mod | SHIFT : mod);   /* (a letter either way without Shift: the C64's capitals) */
}

static void console_keys(bool usb_held) {
    static bool quiet;
    static int prefix;
    int c;
    bool ok = !usb_held && quiet;
    quiet = !usb_held;
    while((c = cmm2->con_getc()) >= 0) {
        if(!ok) continue;
        if(keytrace > 0) printf("console %d\n", c);
        if(prefix == 6) {                       /* Ctrl-F then 1-9, a, b, c */
            prefix = 0;
            if(c >= '1' && c <= '9') typed_put(0x3A + c - '1', 0);
            else if(c >= 'a' && c <= 'c') typed_put(0x43 + c - 'a', 0);
            continue;
        }
        if(prefix == 25 || prefix == 15 || prefix == 22) {   /* Ctrl-Y (CTRL), Ctrl-O (C=) or Ctrl-V (Shift) then a key */
            ascii_key(c, prefix == 25 ? CTRL : prefix == 15 ? ALT : SHIFT);
            prefix = 0;
            continue;
        }
        switch(c) {
            case 6: case 25: case 15: case 22: prefix = c; break;
            case 0x15: typed_put(0x52, 0); break;
            case 0x04: typed_put(0x51, 0); break;
            case 0x0C: typed_put(0x4F, SHIFT); break;   /* (left: Shift and CRSR right, as the keyboard's) */
            case 0x12: typed_put(0x4F, 0); break;
            case 0x1D: exit(0);
            default: ascii_key(c, 0);
        }
    }
    uint64_t now = cmm2->micros();
    if(now < typed_until) return;
    if(typed_key || typed_mod) {                /* let go, and wait as long */
        typed_key = typed_mod = 0;
        typed_until = now + TYPED_US;
    } else if(typed_out != typed_in) {
        typed_key = typed_q[typed_out & 255];
        typed_mod = typed_qm[typed_out++ & 255];
        typed_until = now + TYPED_US;
    }
}

/* ---- the USB keyboard: the reports made key actions, as frank-c64's hid_app.c makes them ---- */
#define HID_END 0x4D
#define HID_KP1 0x59
static struct { uint8_t key, down; } actions[32];
static uint32_t actions_in, actions_out;
static usbhid_keyboard_state_t kbd;

static void action(uint8_t key, int down) {
    if(actions_in - actions_out >= 32) return;
    actions[actions_in & 31].key = key;
    actions[actions_in++ & 31].down = down;
}

static bool in_report(const uint8_t *r, uint8_t key) {
    for(int i = 2; i < 8; i++) if(r[i] == key) return true;
    return false;
}

/* ---- the USB gamepads: the first two in the firmware's HID slots ---- */
#define SLOTS 8
#define STICK 64                                /* how far from the centre a stick is the d-pad */
static usbhid_gamepad_state_t pad[2];
uint32_t nespad_state, nespad_state2;

/* DEVICE(GAMEPAD n, B)'s bitmap, to frank-c64's d-pad and buttons */
static void pad_state(usbhid_gamepad_state_t *g, int b, int lx, int ly) {
    g->connected = 1;
    g->axis_x = lx - 128 > 127 ? 127 : lx - 128;
    g->axis_y = ly - 128 > 127 ? 127 : ly - 128;
    g->dpad = (b >> 7 & 1) | (b >> 5 & 1) << 1 | (b >> 8 & 1) << 2 | (b >> 6 & 1) << 3;
    if(lx < 128 - STICK) g->dpad |= 4;
    if(lx > 128 + STICK) g->dpad |= 8;
    if(ly < 128 - STICK) g->dpad |= 1;
    if(ly > 128 + STICK) g->dpad |= 2;
    g->buttons = (b >> 11 & 1)                  /* a/circle */
               | (b >> 13 & 1) << 1             /* b/cross */
               | (b >> 10 & 1) << 2             /* x/triangle */
               | (b >> 12 & 1) << 3             /* y/square */
               | (b >> 4 & 1) << 4              /* L */
               | (b & 1) << 5                   /* R */
               | (b >> 1 & 1) << 6              /* start */
               | (b >> 3 & 1) << 7;             /* select */
}

/* the second pad as frank-c64's second NES pad: the d-pad, and any of the face buttons the fire (map_nes_to_c64 fires on A or B) */
static uint32_t pad_nes(const usbhid_gamepad_state_t *g) {
    uint32_t n = 0;
    if(!g->connected) return 0;
    if(g->dpad & 1) n |= DPAD_UP;
    if(g->dpad & 2) n |= DPAD_DOWN;
    if(g->dpad & 4) n |= DPAD_LEFT;
    if(g->dpad & 8) n |= DPAD_RIGHT;
    if(g->buttons & 0x0F) n |= DPAD_A;
    return n;
}

/* ---- the mouse, as a 1351 (proportional mode) on port 1 ---- */
/* A 1351 puts its position in the SID's POT X and Y, six bits each in bits 1-6, and its driver takes the difference from the last reading, mod 64: so the
   mouse is looked at each frame and when the program reads POT X or Y, and the movement goes on to the count at most 31 a reading, the rest at the next. Up is up (the screen's y goes down). The left button is fire, the right up, on
   port 1. The firmware keeps the mouse's position, clamped to the screen: it is put back in the middle each frame, and the movement is how far it went. With
   no mouse, POT X and Y are 255, as they were (no paddles) */
#define MOUSE_STEP 31
static bool mouse_here;
static int32_t mouse_x, mouse_y, mouse_dx, mouse_dy;
static int mouse_b;

static int32_t mouse_part(int32_t *left) {
    int32_t d = *left > MOUSE_STEP ? MOUSE_STEP : *left < -MOUSE_STEP ? -MOUSE_STEP : *left;
    *left -= d;
    return d;
}

static void mouse_task(void) {
    int m[4], cx = video.width / 2, cy = video.height / 2;
    if(!cmm2->mouse(m)) {
        mouse_here = false;
        mouse_b = 0;
        return;
    }
    if(!mouse_here) {                           /* (first seen: from the middle) */
        mouse_here = true;
        cmm2->mouse_set(cx, cy);
        return;
    }
    if(m[0] != cx || m[1] != cy) cmm2->mouse_set(cx, cy);
    mouse_dx += m[0] - cx;                      /* (taken up when POT X or Y is read: cmm2_pot) */
    mouse_dy -= m[1] - cy;
    mouse_b = m[2];
}

/* the SID's POT X (y 0) or POT Y (y 1), when the program reads it (SID.h, patches/). A real 1351's count goes on as the mouse moves, so the mouse is read
   again here, at most each millisecond, and not only each frame (Peter, 8 Nov 2026: "very jerky, needs sampling much more often") */
#define MOUSE_EVERY_US 1000
uint8_t cmm2_pot(int y) {
    static uint64_t last;
    uint64_t now = cmm2->micros();
    if(now - last >= MOUSE_EVERY_US) {
        last = now;
        mouse_task();
    }
    if(!mouse_here) return 0xFF;
    if(y) mouse_y += mouse_part(&mouse_dy);    /* (at most 31 from one reading to the next: more would look like a move the other way) */
    else mouse_x += mouse_part(&mouse_dx);
    return (uint8_t)(((y ? mouse_y : mouse_x) & 63) << 1);
}

void usbhid_init(void) { }

/* each frame, from usbhid_wrapper_poll */
void usbhid_task(void) {
    static uint8_t prev[8];
    uint8_t now[8], raw[8];
    int n = 0, lx, ly, b, held;
    cmm2->poll();
    cmm2->break_take();                         /* (Ctrl-C is CTRL and C) */
    held = cmm2->keys_raw(now);
    if((now[0] & 0x11) && (now[0] & 0x44) && (in_report(now, HID_END) || in_report(now, HID_KP1))) exit(0);   /* Ctrl+Alt+End (the keypad's too) leaves */
    memcpy(raw, now, sizeof(raw));
    keys_by_character(now);
    console_keys(held != 0);
    now[0] |= typed_mod;
    if(typed_key) {
        for(int i = 2; i < 8; i++) if(now[i] == 0) { now[i] = typed_key; break; }
    }
    for(int i = 2; i < 8; i++) if(prev[i] && !in_report(now, prev[i])) action(prev[i], 0);
    for(int i = 2; i < 8; i++) if(now[i] && !in_report(prev, now[i])) action(now[i], 1);
    memcpy(prev, now, sizeof(prev));
    kbd.modifier = now[0];                      /* (the C64's Shift, as the characters need it) */
    memcpy(kbd.keycode, raw + 2, 6);            /* the keys as they are: usbhid_wrapper.c takes the joystick's arrows and Ctrl+Alt+Del's Delete from them
                                                   (the console's arrows are the cursor keys only) */
    kbd.has_key = actions_in != actions_out;
    memset(pad, 0, sizeof(pad));
    for(int slot = 1; slot <= SLOTS && n < 2; slot++) {
        lx = ly = 128;
        if((b = cmm2->gamepad(slot, &lx, &ly)) >= 0) pad_state(&pad[n++], b, lx, ly);
    }
    nespad_state2 = pad_nes(&pad[1]);
    mouse_task();                               /* (its buttons with the second pad's, on port 1) */
    if(mouse_b & 1) nespad_state2 |= DPAD_A;
    if(mouse_b & 2) nespad_state2 |= DPAD_UP;
}

void usbhid_get_keyboard_state(usbhid_keyboard_state_t *state) { *state = kbd; }

int usbhid_get_key_action(uint8_t *keycode, int *down) {
    if(actions_out == actions_in) return 0;
    *keycode = actions[actions_out & 31].key;
    *down = actions[actions_out++ & 31].down;
    return 1;
}

int usbhid_keyboard_connected(void) { return 1; }
int usbhid_mouse_connected(void) { return 0; }
void usbhid_get_mouse_state(usbhid_mouse_state_t *state) { memset(state, 0, sizeof(*state)); }
int usbhid_gamepad_connected(void) { return pad[0].connected; }

/* the first pad: input_rp2350.cpp fires on its a or b (buttons & 3), and here x and y fire too, as all four do on the second (pad_nes; Peter, 8 Nov 2026: a
   DS4 fired with all its symbols, a DS3 with circle and cross only) */
void usbhid_get_gamepad_state(usbhid_gamepad_state_t *state) {
    *state = pad[0];
    if(state->buttons & 0x0C) state->buttons |= 0x01;
}

/* ---- the wired NES pads it can have: none (the second USB pad is put in nespad_state2) ---- */
bool nespad_begin(uint32_t cpu_khz, uint8_t clkPin, uint8_t dataPin, uint8_t latPin) {
    (void)cpu_khz; (void)clkPin; (void)dataPin; (void)latPin;
    return false;
}
void nespad_read(void) { }

/* ---- the start ---- */
int main(int argc, char **argv) {
    (void)argc; (void)argv;
    if(cmm2->video_mode(7, 8, &video) < 0) {
        fprintf(stderr, "c64: cannot set MODE 7,8\n");
        return 1;
    }
    cmm2->con_screen(0);                        /* (its messages to the serial console alone) */
    c64_palette();
    palette_show();
    cmm2->frame_hook(frame);
    live = g_pixels;
    {
        startscreen_info_t info = { .title = "FRANK C64", .subtitle = "Commodore 64 Emulator", .version = FIRMWARE_VERSION,
                                    .board_variant = "Colour Maximite 2", .cpu_mhz = cmm2->cpu_hz() / 1000000 };
        startscreen_show(&info);                /* (its palette is shown with its last frame) */
    }
    input_rp2350_init();
    disk_loader_init();
    disk_loader_scan();
    sid_i2s_init();
    c64_init();
    live = NULL;
    memset(g_pixels, 0, sizeof(g_pixels));
    uint64_t next = cmm2->micros();
    bool menu = false;
    for(;;) {
        if(!disk_ui_is_visible()) {
            if(menu) {                          /* (the menu's frame goes; the next is the C64's) */
                menu = false;
                live = NULL;
            }
            c64_run_frame();
            frame_done();
        } else {
            if(!menu) {                         /* the menu over the C64's last frame (in vic2 when the VIC is to draw the next in g_pixels) */
                menu = true;
                if(vic_next == g_pixels) memcpy(g_pixels, vic2, sizeof(vic2));
                live = g_pixels;
            }
            input_rp2350_poll_no_c64_acts();
            disk_ui_render();
            palette_show();
        }
        next += 20000;                          /* PAL: a frame each 20 ms */
        uint64_t now = cmm2->micros();
        if(now < next) {
            if(stats > 0) stat_idle += next - now;
            if(next - now > 1500) usleep((useconds_t)(next - now - 1000));
            while(cmm2->micros() < next) { }
        } else if(now > next + 40000) next = now;
    }
    return 0;
}
