/* cpc_cmm2.c: frank-cpc's drivers on the Colour Maximite 2, by the loader's services (Peter, 6 Nov 2026). This file has what frank-cpc's main.c gives the rest of it (that file is
   the RP2350's start-up, and is not built): the frame buffers, the sound ring and main, which runs cpc_pico_main of platform.c as it is.

   - The display: MODE 8,8, 640 x 480 in 256 colours from the palette. frank-cpc's picture is 320 x 240, which loses half of Mode 2's pixels; patches/ has it make all 640 of each
     of its 240 lines. A frame goes to the page at the start of the display's blanking, in the frame hook, by the DMA2D (the even lines, then the odd, keeping ahead of the
     display), each line twice, so it is never shown half drawn; the CPU's copy took 1.5 ms a frame of a G1. The display's 59.94 frames a second do not come into the
     emulator's 50. The menus and the welcome screen are drawn at their own 320 x 240, as frank-cpc draws them, and each pixel put down twice.
   - The sound: the AY's samples go through a ring to the firmware's sound stream, at its rate by a phase step, trimmed to hold the ring's level: the emulator keeps its own time,
     a frame each 20 ms.
   - The keyboard: what is on the key is what comes out, in any OPTION USBKEYBOARD layout (Peter, 6 Nov 2026): each key becomes the CPC key that types the character the
     firmware's layout makes of it (keys_by_character), and the USB keyboard's reports (keys_raw) so changed become usbhid.h's key actions, which frank-cpc's usbhid_wrapper.c and
     platform.c turn into the CPC's matrix. Ctrl is the CPC's CONTROL and Alt its COPY, so Ctrl-C is a CPC key here, taken from the firmware before it can end the program:
     Ctrl+Alt+End leaves, as it does pc386. F11 is the disk, tape and cartridge menu, F12 the settings, Ctrl+Alt+Del resets the CPC.
   - The gamepads: the first two USB pads are the CPC's joystick (usbhid.h's state, which platform.c puts in the matrix's row 9).
   - The serial console types too, for remote use, and more than the keyboard can, as the CPC's keys are not a PC's:
       a character is the CPC key that types it, with Shift if it needs it (" is Shift 2, @ the key PC's have ' on, : the one with ;, and so on);
       Enter, Esc, Tab, Backspace (the CPC's DEL) as they are;
       Ctrl-U, Ctrl-D, Ctrl-L, Ctrl-R the arrows;
       Ctrl-F then 1-9, a, b, c for F1-F12: Ctrl-F b is F11, Ctrl-F c is F12;
       Ctrl-Y then a key: CONTROL and that key;
       Ctrl-O: COPY for a moment;
       Ctrl-P then a line ended by Enter: a command of frank-cpc's serial console (cpc_serial_console.c: HELP, STATUS, RESET, DISK A INSERT path, TAPE INSERT path, CART INSERT path,
       MEM addr count, POKE addr value, Z80, CRTC, TYPE text, KEY row bit PRESS...), the answer on the console;
       Ctrl-] leaves.
     While a USB key is held its character reaches the console as well, and is left out.
   RUN "cpc.app", -keys prints each key as it goes down: its code, the character the firmware made of it and what the CPC is given.
   RUN "cpc.app", -stats prints the frames a second, how much of the time the emulator has to spare, the sound ring's level and the heap in use, every 5 s. */
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmm2.h"
#include "HDMI.h"
#include "board_config.h"
#include "cpc_boot.h"
#include "cpc_ui.h"
#include "nespad.h"
#include "usbhid.h"
#include "usbhid_wrapper.h"

extern void cpc_pico_main(void);

/* ---- time ---- */
uint64_t time_us_64(void) { return cmm2->micros(); }
void sleep_us(uint64_t us) { usleep((useconds_t)us); }
void sleep_ms(uint32_t ms) { usleep((useconds_t)ms * 1000); }

/* -stats: the frames, the time the emulator had to spare and the sound ring, since the last report */
static int stats = -1;
static uint32_t stat_frames;
static uint64_t stat_idle, stat_from, spin_last;
static volatile uint32_t ring_in, ring_out;     /* (the sound's: ring_out in samples, ring_phase its fraction) */

/* cpc_frame_sync, waiting for the next 20 ms: all the spinning there is. A gap means a moment taken by the display's or the sound's interrupt */
void tight_loop_contents(void) {
    uint64_t now = cmm2->micros();
    if(now - spin_last < 100) stat_idle += now - spin_last;
    spin_last = now;
}

static void stat_frame(void) {
    uint64_t now = cmm2->micros();
    if(stats < 0) {
        stats = 0;
        for(int i = 1; i < cmm2_start->argc; i++) if(strcmp(cmm2_start->argv[i], "-stats") == 0) stats = 1;
        stat_from = now;
    }
    if(!stats) return;
    stat_frames++;
    if(now - stat_from >= 5000000) {
        uint32_t ms = (uint32_t)((now - stat_from) / 1000), idle = (uint32_t)(stat_idle / 1000);
        printf("cpc: %lu.%lu frames a second, %lu%% spare, sound ring %lu, heap %lu\n", (unsigned long)(stat_frames * 1000 / ms),
               (unsigned long)(stat_frames * 10000 / ms % 10), (unsigned long)(idle * 100 / ms), (unsigned long)(ring_in - ring_out), (unsigned long)mallinfo().uordblks);
        stat_frames = 0;
        stat_idle = 0;
        stat_from = now;
    }
}

/* ---- the display ---- */
#define PIC_W CPC_FB_WIDTH
#define PIC_H CPC_SCREEN_LINES
#define UI_W (PIC_W / 2)
static uint8_t __attribute__((aligned(4))) screen_mem[2][PIC_W * PIC_H];
uint8_t *SCREEN[2] = { screen_mem[0], screen_mem[1] };
volatile uint32_t current_buffer = 1;           /* (SCREEN[0] is shown first) */
bool SELECT_VGA;
uint8_t *volatile pending_vga_fb;
static cmm2_video video;
static const uint8_t *volatile ready;           /* a whole frame not yet shown */
static bool welcome;                            /* cpc_boot_welcome's frames, 320 across */
static uint8_t palette[256 * 3], shown[256 * 3];
static volatile bool palette_new;
static uint8_t ui_layer[PIC_H][UI_W];
static bool ui_skip;                            /* a frame the menus left out */

/* in PendSV, at the start of each frame's blanking */
static void frame(void) {
    const uint8_t *s = ready;
    uint8_t *d = video.fb;
    if(s == NULL) return;
    ready = NULL;
    cmm2->dma2d_copy(s, PIC_W, d, video.pitch * 2, PIC_W, PIC_H);
    cmm2->dma2d_copy(s, PIC_W, d + video.pitch, video.pitch * 2, PIC_W, PIC_H);
}

void graphics_set_buffer(uint8_t *buffer) {
    if(welcome) {                               /* (from the end: nothing is written before it has been read) */
        for(int i = UI_W * PIC_H - 1; i >= 0; i--) {
            int y = i / UI_W, x = i % UI_W;
            buffer[y * PIC_W + x * 2] = buffer[y * PIC_W + x * 2 + 1] = buffer[i];
        }
    }
    if(palette_new) {                           /* (as video_palette waits for the next frame) */
        palette_new = false;
        if(memcmp(palette, shown, sizeof(shown)) != 0) {
            memcpy(shown, palette, sizeof(shown));
            cmm2->video_palette(0, 256, shown);
        }
    }
    if(!ui_skip) ready = buffer;
    ui_skip = false;
    if(!welcome) stat_frame();
}

void graphics_set_bgcolor(uint32_t rgb) { (void)rgb; }   /* (the picture fills the screen) */

void graphics_set_palette(uint8_t i, uint32_t rgb) {
    palette[i * 3] = rgb >> 16;
    palette[i * 3 + 1] = rgb >> 8;
    palette[i * 3 + 2] = rgb;
    palette_new = true;
}

uint32_t graphics_get_palette(uint8_t i) { return (uint32_t)palette[i * 3] << 16 | palette[i * 3 + 1] << 8 | palette[i * 3 + 2]; }

/* the menus over a frame: they use the colours from 240 up, so 0 shows what is under them. Every other frame, and the one between not shown: a whole 3.5 ms of a G1's frame, and
   the emulator, with the sound, goes on at its pace behind them */
void cmm2_ui_render(uint8_t *dst) {
    static bool odd;
    odd = !odd;
    if(odd) {
        ui_skip = true;
        return;
    }
    memset(ui_layer, 0, sizeof(ui_layer));
    cpc_ui_render(&ui_layer[0][0], UI_W, PIC_H);
    for(int y = 0; y < PIC_H; y++, dst += PIC_W) {
        const uint32_t *s = (const uint32_t *)ui_layer[y];
        uint32_t *d = (uint32_t *)dst;
        for(int x = 0; x < UI_W / 4; x++, s++, d += 2) {
            uint32_t w = *s;
            if(w == 0) continue;
            if((w & 0xFF) && (w & 0xFF00) && (w & 0xFF0000) && (w >> 24)) {   /* (most of a menu: four pixels, each twice, by two words) */
                d[0] = (w & 0xFF) * 0x101 | (w >> 8 & 0xFF) * 0x1010000;
                d[1] = (w >> 16 & 0xFF) * 0x101 | (w >> 24) * 0x1010000;
            } else {
                for(int i = 0; i < 4; i++, w >>= 8) {
                    if(w & 0xFF) ((uint8_t *)d)[i * 2] = ((uint8_t *)d)[i * 2 + 1] = w & 0xFF;
                }
            }
        }
    }
}

/* ---- the sound ---- */
#define RING 8192                               /* a power of 2 */
#define RING_LEVEL 3000                         /* samples: a stream buffer of 1024, the AY's buffer of 1024 and a frame more */
#define RING_FULL 7000                          /* (only with the fast tape running ahead: the rest is dropped) */
#define AY_HZ 44030                             /* its 44,100 a second of the CPC's time, of which a frame of 19,968 us is run every 20 ms */
static int16_t ring_l[RING], ring_r[RING];
static uint32_t ring_phase, ring_step, out_rate;
static int audio_on;

/* in PendSV: no services. The emulator keeps its own time and the stream the board's, so the step is trimmed, by up to 1%, to hold the ring at RING_LEVEL. The AY's samples come
   1024 at a time, so it is the level over several buffers that counts */
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

static void audio_begin(void) {
    int r;
    if(audio_on) return;
    audio_on = 1;
    r = cmm2->audio_start(fill);
    if(r < 0) audio_on = -1;
    else {
        out_rate = r;
        ring_step = (uint32_t)(((uint64_t)AY_HZ << 16) / out_rate);
    }
}

unsigned audio_ring_free(void) { return RING_FULL - (ring_in - ring_out); }

/* cpc_frame_sync skips its wait when this is low, to catch up with a stream it follows. Here the stream follows it */
unsigned audio_ring_avail(void) { return 882 * 2; }

unsigned audio_ring_push_stereo(const int16_t *samples, unsigned count) {
    uint32_t level, in;
    audio_begin();
    if(audio_on < 0) return count;
    in = ring_in;
    level = in - ring_out;
    if(level + count > RING_FULL) count = level > RING_FULL ? 0 : RING_FULL - level;
    for(unsigned i = 0; i < count; i++, in++) {
        ring_l[in & (RING - 1)] = samples[i * 2];
        ring_r[in & (RING - 1)] = samples[i * 2 + 1];
    }
    ring_in = in;
    return count;
}

/* ---- the serial console's keys: each pressed for a moment, then let go ---- */
#define TYPED_US 70000
#define SHIFT 0x02                              /* a report's modifier bits */
#define CTRL 0x01
#define ALT 0x04
static uint8_t typed_key, typed_mod;
static uint64_t typed_until;
static uint8_t typed_q[256], typed_qm[256];
static uint32_t typed_in, typed_out;
static char line_q[512];                        /* Ctrl-P's line, for cpc_serial_console.c */
static uint32_t line_in, line_out;

static void typed_put(uint8_t key, uint8_t mod) {
    if(typed_in - typed_out >= sizeof(typed_q)) return;
    typed_q[typed_in & 255] = key;
    typed_qm[typed_in++ & 255] = mod;
}

/* the USB key at the place of the CPC's key for a character (platform.c's map goes by a key's place), and in shift whether it takes Shift. 0 if the CPC has no such character */
static uint8_t char_place(int c, int *shift) {
    static const char digits[] = "!\"#$%&'()_", plain[] = "-;[]\\:@^,./", shifted[] = "=+{}`*|\x7F<>?";
    static const uint8_t place[] = { 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 };
    const char *p;
    *shift = 0;
    if(c >= 'a' && c <= 'z') return 4 + c - 'a';
    if(c >= 'A' && c <= 'Z') { *shift = 1; return 4 + c - 'A'; }
    if(c == '0') return 0x27;
    if(c >= '1' && c <= '9') return 0x1E + c - '1';
    if(c <= 0 || c >= 127) return 0;
    if((p = strchr(plain, c)) != NULL) return place[p - plain];
    *shift = 1;
    if((p = strchr(digits, c)) != NULL) return 0x1E + (p - digits);
    if((p = strchr(shifted, c)) != NULL) return place[p - shifted];
    return 0;
}

static int keytrace = -1;                       /* -keys */
#define SHIFT_SETTLE 3                          /* frames the CPC's Shift is up before a key that needs it up goes down */

/* The report as the CPC should see it: each key the CPC key for the character it makes (keys_held has them, the last key of the report first), the CPC's Shift as that
   character needs and not as the PC's is: Shift and = is +, which on a CPC is Shift and the ; key, but Shift and ; is :, which is the : key with no Shift. Decided as the key goes
   down, so that a Shift let go first does not change a key still held. Letters keep the Shift held (Caps Lock is the CPC's own), and only the main block of keys is looked at:
   the keypad stays the CPC's, and Enter and the like are in their places.
   Shift going down with a key is seen at once, but the CPC's firmware goes on seeing a Shift let go for a scan or so, which no hand on its keyboard would ever show: : came
   out as * and @ as |, with the matrix right. So a key that needs Shift up is held back SHIFT_SETTLE frames from when it went up; with no Shift held there is no wait. */
static void keys_by_character(uint8_t *r) {
    static uint8_t was[6], place[256];
    static int8_t shift[256];
    uint8_t out[6] = {0};
    static int shift_up = SHIFT_SETTLE;         /* frames the CPC's Shift has been up */
    int keys[7], total = 0, force = -1, i, fresh = 0, mods = r[0];
    if(keytrace < 0) {
        keytrace = 0;
        for(i = 1; i < cmm2_start->argc; i++) if(strcmp(cmm2_start->argv[i], "-keys") == 0) keytrace = 1;
    }
    for(i = 0; i < 6 && r[i + 2]; i++) total++;
    cmm2->keys_held(keys);
    for(i = 0; i < total; i++) {
        uint8_t h = r[i + 2];
        if(memchr(was, h, sizeof(was)) == NULL) {
            int c = keys[total - 1 - i], s = 0;
            uint8_t p = 0;
            bool letter = h >= 0x04 && h <= 0x1D;
            if(c >= 1 && c <= 26) c += 96;                      /* (Ctrl with a letter) */
            if(h < 0x28 || (h >= 0x2D && h <= 0x38) || h == 0x64) p = char_place(c, &s);
            place[h] = p ? p : h;
            shift[h] = !p || (p >= 0x04 && p <= 0x1D && letter) ? -1 : s;
            fresh = 1;
            if(keytrace) printf("key %02X of %d: character %d, place %02X, shift %d\n", h, total, c, place[h], shift[h]);
        }
        out[i] = place[h];
        if(shift[h] >= 0) force = shift[h];
    }
    if(force == 0) r[0] &= ~0x22;
    else if(force == 1) r[0] |= 0x02;
    if(r[0] & 0x22) shift_up = 0;
    else if(shift_up < SHIFT_SETTLE) {
        shift_up++;
        for(i = 0; i < total; i++) if(shift[r[i + 2]] == 0) out[i] = 0;     /* (the next frames offer it again) */
    }
    memcpy(was, r + 2, sizeof(was));
    memcpy(r + 2, out, sizeof(out));
    if(force >= 0) r[0] &= ~0x40;                               /* (AltGr made the character: it is not COPY) */
    if(keytrace && fresh) printf("  modifiers %02X became %02X, keys %02X %02X\n", mods, r[0], r[2], r[3]);
}

static void ascii_key(int c, uint8_t mod) {
    int s;
    uint8_t p;
    if(c == 13 || c == 10) typed_put(0x28, mod);
    else if(c == 27) typed_put(0x29, mod);
    else if(c == 8 || c == 127) typed_put(0x2A, mod);
    else if(c == 9) typed_put(0x2B, mod);
    else if(c == ' ') typed_put(0x2C, mod);
    else if((p = char_place(c, &s)) != 0) typed_put(p, s ? mod | SHIFT : mod);
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
        if(prefix == 0x10) {                    /* Ctrl-P's line */
            if(line_in - line_out < sizeof(line_q)) line_q[line_in++ % sizeof(line_q)] = c;
            if(c == 13) prefix = 0;
            continue;
        }
        if(prefix == 6) {                       /* Ctrl-F then 1-9, a, b, c */
            prefix = 0;
            if(c >= '1' && c <= '9') typed_put(0x3A + c - '1', 0);
            else if(c >= 'a' && c <= 'c') typed_put(0x43 + c - 'a', 0);
            continue;
        }
        if(prefix == 25) {                      /* Ctrl-Y then a key */
            prefix = 0;
            ascii_key(c, CTRL);
            continue;
        }
        switch(c) {
            case 6: case 25: case 0x10: prefix = c; break;
            case 0x15: typed_put(0x52, 0); break;
            case 0x04: typed_put(0x51, 0); break;
            case 0x0C: typed_put(0x50, 0); break;
            case 0x12: typed_put(0x4F, 0); break;
            case 0x0F: typed_put(0, ALT); break;
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

int getchar_timeout_us(uint32_t timeout_us) {
    (void)timeout_us;
    return line_out == line_in ? -1 : (uint8_t)line_q[line_out++ % sizeof(line_q)];
}

/* ---- the USB keyboard: the reports made key actions, as frank-cpc's hid_app.c makes them ---- */
#define HID_END 0x4D
#define HID_KP1 0x59
static struct { uint8_t key, down; } actions[32];
static uint32_t actions_in, actions_out;

static void action(uint8_t key, int down) {
    if(actions_in - actions_out >= 32) return;
    actions[actions_in & 31].key = key;
    actions[actions_in++ & 31].down = down;
}

/* a modifier is the one key to frank-cpc whichever side it is on */
static void modifier(uint8_t was, uint8_t now, uint8_t mask, uint8_t key) {
    if(!(was & mask) != !(now & mask)) action(key, (now & mask) != 0);
}

static bool in_report(const uint8_t *r, uint8_t key) {
    for(int i = 2; i < 8; i++) if(r[i] == key) return true;
    return false;
}

/* ---- the USB gamepads: the first two in the firmware's HID slots ---- */
#define SLOTS 8
#define STICK 64                                /* how far from the centre a stick is the d-pad */
static usbhid_gamepad_state_t pad[2];

/* DEVICE(GAMEPAD n, B)'s bitmap, to frank-cpc's d-pad and buttons */
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

void usbhid_init(void) { }

/* each frame */
void usbhid_task(void) {
    static uint8_t prev[8];
    uint8_t now[8];
    int n = 0, lx, ly, b, held;
    cmm2->poll();
    cmm2->break_take();                         /* (Ctrl-C is CONTROL and C) */
    held = cmm2->keys_raw(now);
    if((now[0] & 0x11) && (now[0] & 0x44) && (in_report(now, HID_END) || in_report(now, HID_KP1))) exit(0);   /* Ctrl+Alt+End (the keypad's too) leaves */
    keys_by_character(now);
    console_keys(held != 0);
    now[0] |= typed_mod;
    if(typed_key) {
        for(int i = 2; i < 8; i++) if(now[i] == 0) { now[i] = typed_key; break; }
    }
    modifier(prev[0], now[0], 0x11, 0xE0);
    modifier(prev[0], now[0], 0x22, 0xE1);
    modifier(prev[0], now[0], 0x44, 0xE2);
    for(int i = 2; i < 8; i++) if(prev[i] && !in_report(now, prev[i])) action(prev[i], 0);
    for(int i = 2; i < 8; i++) if(now[i] && !in_report(prev, now[i])) action(now[i], 1);
    memcpy(prev, now, sizeof(prev));
    memset(pad, 0, sizeof(pad));
    for(int slot = 1; slot <= SLOTS && n < 2; slot++) {
        lx = ly = 128;
        if((b = cmm2->gamepad(slot, &lx, &ly)) >= 0) pad_state(&pad[n++], b, lx, ly);
    }
}

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

void usbhid_get_gamepad_state_idx(int idx, usbhid_gamepad_state_t *state) {
    if(idx >= 0 && idx < 2) *state = pad[idx];
    else memset(state, 0, sizeof(*state));
}

/* ---- the wired NES pads it can have ---- */
uint32_t nespad_state;
bool nespad_begin(uint32_t cpu_khz, uint8_t clkPin, uint8_t dataPin, uint8_t data2Pin, uint8_t latPin) {
    (void)cpu_khz; (void)clkPin; (void)dataPin; (void)data2Pin; (void)latPin;
    return false;
}
void nespad_read(void) { }

/* ---- the start, and a stop with a message on the screen ---- */
void cmm2_stop(const char *why) {
    cmm2->frame_hook(NULL);
    cmm2->con_screen(1);
    cmm2->gfx_cls(0);
    cmm2->con_puts(why);
    cmm2->con_puts("\r\n");
    sleep_ms(6000);
    exit(1);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    usbhid_wrapper_init();
    if(cmm2->video_mode(8, 8, &video) < 0) {
        fprintf(stderr, "cpc: cannot set MODE 8,8\n");
        return 1;
    }
    cmm2->con_screen(0);                        /* (its messages to the serial console alone) */
    cmm2->frame_hook(frame);
    welcome = true;
    cpc_boot_welcome(10000);
    welcome = false;
    cpc_pico_main();
    return 0;
}
