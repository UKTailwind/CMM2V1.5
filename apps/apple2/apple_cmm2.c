/* apple_cmm2.c: frank-apple's drivers on the Colour Maximite 2, by the loader's services (Peter, 9 Nov 2026). This file has what frank-apple's main.c and
   mii_audio_i2s.c give the rest of it (those files are the RP2350's start-up, its second core and its sound, and are not built): main, which starts the
   emulator and runs it a frame at a time as main.c does, the display, the speaker's sound, the keys and the gamepads.

   - The display: MODE 8,8, 640 x 480 in 256 colours from the palette. The Apple's screen is drawn at its full 560 x 192 (apple_video.c, MII's own way: 80
     columns and double hi-res whole, where frank-apple's RP2350 halves them to fit 320 pixels) in the middle of a 640 x 240 picture, each line shown twice.
     Each frame the emulator has run is drawn into one of two buffers, and the one last finished goes to the page at the start of the display's blanking, in
     the frame hook, by the DMA2D, so it is never shown half drawn. The display's 75 frames a second do not come into the emulator's 60. frank-apple's start
     screen and disk menu draw in a 320 x 240 picture of 4 bits a pixel, as on its RP2350, which is shown with each pixel twice across.
   - The sound: the speaker's clicks ($C030), at the cycle each was made, become 44,100 samples a second of the Apple's time, each the speaker's level
     averaged over the sample (so no click is lost between samples), with its direct current taken out as a speaker's capacitor does; they go through a
     ring to the firmware's sound stream, at its rate by a phase step trimmed to hold the ring's level: the emulator keeps its own time, 17,030 cycles a
     frame at 1,023,000 a second (frank-apple's).
   - The keyboard: the character the firmware's layout (OPTION USBKEYBOARD) makes of a key is what the Apple is given; Return, Esc, Tab, Backspace (the
     left arrow, as on an Apple), Delete (DELETE) and the arrows as the Apple's. The Apple //e's Caps Lock is down at the start, as programs of the time
     want capitals: the PC's Caps Lock key lets it up and puts it down again, and with it up, Shift makes the capitals. A key held repeats as an Apple's
     does. The left Alt and Windows keys are the Open Apple key (button 0), the right ones the Closed Apple (button 1). frank-apple's keys: F11 the disk
     menu, F12 held runs as fast as it can, Scroll Lock does so until pressed again, F9 shows the speed; Ctrl+Alt+Delete starts the Apple again from
     cold. Ctrl+Pause (Ctrl+Break) is Ctrl+Reset: with the left Alt (Open Apple) held too, a //e starts again from cold. F10 changes MII's colour sets:
     NTSC, NTSC 2, Mega II, green and amber screens. Ctrl+Alt+End leaves, as it does the other emulators; Ctrl-C is the Apple's here.
   - The joystick: the first USB gamepad, as frank-apple has it: its left stick the joystick, where it is, or the d-pad moving it, and its a and x buttons
     (circle and triangle) button 0, b and y (cross and square) button 1, Start button 2; Select opens the disk menu, where the d-pad and a and b move
     about; Start, a and b together start the Apple again from cold. The keypad's 8, 2 (or 5), 4, 6 and 7, 9, 1, 3 are the joystick too, 0 button 2.
   - The serial console types too, for remote use: each character as it is, Return (13) as Return, Backspace and Delete the left arrow (the Apple's own
     control keys are its arrows: Ctrl-H, Ctrl-U, Ctrl-K, Ctrl-J, so a line feed is the down arrow); Ctrl-F then 1-9, a, b, c for F1-F12 (Ctrl-F b: the
     disk menu), Ctrl-F then r for Ctrl+Reset; Ctrl-] leaves. A character goes to the Apple when it has taken the last, so a program can be pasted.
   RUN "apple2.app", -stats prints the frames a second, how much of the time the emulator has to spare, the sound ring's level and the heap in use, every 5 s;
   -log lets frank-apple's own messages through to the serial console. */
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmm2.h"
#include "mii.h"
#include "mii_sw.h"
#include "mii_bank.h"
#include "mii_rom.h"
#include "mii_slot.h"
#include "disk_loader.h"
#include "disk_ui.h"
#include "mii_startscreen.h"
#include "apple_video.h"

/* frank-apple's messages: its sources are built with printf named apple_printf (make.py), which says nothing unless -log is given */
#undef printf
int printf(const char *fmt, ...);
static int logging;

int apple_printf(const char *fmt, ...) {
    va_list ap;
    int n;
    if(!logging) return 0;
    va_start(ap, fmt);
    n = vprintf(fmt, ap);
    va_end(ap);
    return n;
}

static bool has_arg(const char *a) {
    for(int i = 1; i < cmm2_start->argc; i++) if(strcmp(cmm2_start->argv[i], a) == 0) return true;
    return false;
}

mii_t g_mii;                                    /* the Apple (mii.c's speaker click asks for it by this name) */
static bool mii_ready;

/* frank-apple's NES pad's bits (nespad.h), as main.c joins the pads and the keypad's joystick */
#define DPAD_UP 0x01
#define DPAD_DOWN 0x02
#define DPAD_LEFT 0x04
#define DPAD_RIGHT 0x08
#define DPAD_A 0x10
#define DPAD_B 0x20
#define DPAD_SELECT 0x40
#define DPAD_START 0x80

/* ---- the time ---- */
uint64_t time_us_64(void) { return cmm2->micros(); }
void sleep_us(uint64_t us) { usleep((useconds_t)us); }
void sleep_ms(uint32_t ms) { usleep((useconds_t)ms * 1000); }

/* -stats: the frames, the time the emulator had to spare and the sound ring, since the last report */
static int stats;
static uint32_t stat_frames;
static uint64_t stat_idle, stat_from;
static volatile uint32_t ring_in, ring_out;     /* (the sound's: ring_out in samples, ring_phase its fraction) */

static void stat_frame(void) {
    uint64_t now = cmm2->micros();
    if(!stats) return;
    stat_frames++;
    if(now - stat_from >= 5000000) {
        uint32_t ms = (uint32_t)((now - stat_from) / 1000), idle = (uint32_t)(stat_idle / 1000);
        printf("apple2: %lu.%lu frames a second, %lu%% spare, sound ring %lu, heap %lu\n", (unsigned long)(stat_frames * 1000 / ms),
               (unsigned long)(stat_frames * 10000 / ms % 10), (unsigned long)(idle * 100 / ms), (unsigned long)(ring_in - ring_out),
               (unsigned long)mallinfo().uordblks);
        stat_frames = 0;
        stat_idle = 0;
        stat_from = now;
    }
}

/* ---- what frank-apple puts in a board's PSRAM: the second drive's disk (disk_loader.c's drive 1 cache, at PSRAM_DATA + BDSK_BYTES: patches/) ---- */
uint8_t *cmm2_psram;
static uint8_t *drive1_cache;

unsigned int butter_psram_size(void) { return drive1_cache ? 2 * BDSK_BYTES : 0; }

/* ---- the display ---- */
#define PIC_W 640                               /* the picture: 240 lines, each shown twice */
#define PIC_H 240
#define APPLE_X ((PIC_W - APPLE_W) / 2)         /* the Apple's 560 x 192 in the middle */
#define APPLE_Y ((PIC_H - APPLE_H) / 2)
#define UI_W 320                                /* frank-apple's start screen and disk menu: 320 x 240, 4 bits a pixel */
#define UI_H 240
#define UI 128                                  /* the palette: those 16 colours from here (frank-apple's), MII's below */
static cmm2_video video;
static uint8_t __attribute__((aligned(32))) pic[2][PIC_W * PIC_H];
static int pic_next;
static const uint8_t *volatile ready;           /* a whole picture not yet on the page */
static uint8_t __attribute__((aligned(4))) ui_fb[UI_W * UI_H / 2];
static uint8_t palette[256 * 3], palette_shown[256 * 3];
static bool palette_new;
static int colour_set = -1;
volatile int lock_y = -1;                       /* (frank-apple's HDMI driver's, which its own renderer waits on: not used here) */

uint8_t *graphics_get_buffer(void) { return ui_fb; }

/* in PendSV, at the start of each frame's blanking: the picture's lines, each twice */
static void frame(void) {
    const uint8_t *s = ready;
    if(s == NULL) return;
    ready = NULL;
    uint8_t *d = video.fb + (video.height > 2 * PIC_H ? (video.height - 2 * PIC_H) / 2 * video.pitch : 0);
    cmm2->dma2d_copy(s, PIC_W, d, 2 * video.pitch, PIC_W, PIC_H);
    cmm2->dma2d_copy(s, PIC_W, d + video.pitch, 2 * video.pitch, PIC_W, PIC_H);
}

static void palette_show(void) {               /* (in the program: video_palette waits for the next frame) */
    if(!palette_new) return;
    palette_new = false;
    if(memcmp(palette, palette_shown, sizeof(palette)) == 0) return;
    memcpy(palette_shown, palette, sizeof(palette));
    cmm2->video_palette(0, 256, palette_shown);
}

/* main.c's colours for its start screen and disk menu, by the lo-res colours' numbers */
static void ui_palette(void) {
    static const uint32_t rgb[16] = { 0x000000, 0xDD0033, 0x000099, 0xDD22DD, 0x007722, 0x555555, 0x2222FF, 0x66AAFF,
                                      0x885500, 0xFF6600, 0xAAAAAA, 0xFF9988, 0x11DD00, 0xFFFF00, 0x44FF99, 0xFFFFFF };
    for(int i = 0; i < 16; i++) {
        palette[(UI + i) * 3] = rgb[i] >> 16;
        palette[(UI + i) * 3 + 1] = rgb[i] >> 8;
        palette[(UI + i) * 3 + 2] = rgb[i];
    }
    palette_new = true;
}

/* MII's colour set, when it is new */
static void apple_palette(void) {
    if(mii_ready && g_mii.video.color_mode != colour_set) {
        colour_set = g_mii.video.color_mode;
        apple_video_palette(&g_mii.video, palette);
        palette_new = true;
    }
}

/* the picture to be shown next, with the Apple's border black (it is cleared when the 320 x 240 one has been in it) */
static uint8_t *pic_take(bool border) {
    static bool dirty[2] = { true, true };
    uint8_t *p = pic[pic_next];
    if(border && dirty[pic_next]) memset(p, 0, PIC_W * PIC_H);
    dirty[pic_next] = !border;
    return p;
}

static void pic_give(void) {
    ready = pic[pic_next];
    pic_next ^= 1;
}

/* lines y0 to y1 of the 320 x 240 picture, each pixel twice across */
static void ui_expand(uint8_t *p, int y0, int y1) {
    for(int y = y0; y < y1; y++) {
        const uint8_t *s = ui_fb + y * (UI_W / 2);
        uint8_t *d = p + y * PIC_W;
        for(int x = 0; x < UI_W / 2; x++, d += 4) {
            uint8_t a = UI + (s[x] & 15), b = UI + (s[x] >> 4);
            d[0] = d[1] = a;
            d[2] = d[3] = b;
        }
    }
}

/* the start screen or the disk menu, shown */
static void ui_show(void) {
    palette_show();
    uint8_t *p = pic_take(false);
    ui_expand(p, 0, UI_H);
    pic_give();
}

/* the start screen's wait (a 3 s loop of tight_loop_contents): it is shown once */
static bool start_screen;
void tight_loop_contents(void) {
    if(start_screen) {
        start_screen = false;
        ui_show();
    }
}

/* frank-apple's disk light: drive 1 green, drive 2 orange, flashing in the bottom border while its motor runs (mii_video.c's, there in its RP2350 renderer) */
int mii_disk2_get_motor_state(void);
static void disk_lights(uint8_t *p, uint32_t frame_count) {
    static const uint16_t icon[2][10] = {
        { 0x1FE, 0x201, 0x271, 0x271, 0x271, 0x201, 0x279, 0x279, 0x279, 0x1FE },
        { 0x3FC, 0x206, 0x276, 0x276, 0x266, 0x202, 0x27A, 0x27A, 0x202, 0x3FC } };
    int motor = mii_disk2_get_motor_state();
    for(int d = 0; d < 2; d++) {
        uint8_t *q = p + 222 * PIC_W + 572 + d * 28;
        bool on = (motor >> d & 1) && !((frame_count >> 3) & 1);
        for(int y = 0; y < 10; y++, q += PIC_W)
            for(int x = 0; x < 10; x++) q[2 * x] = q[2 * x + 1] = on && (icon[d][y] & (1 << (9 - x))) ? UI + (d ? 9 : 12) : 0;
    }
}

/* ---- the sound: the speaker ---- */
#define RING 8192                               /* a power of 2 */
#define RING_LEVEL 3000                         /* samples: a stream buffer of 1024, a frame of 736 and more */
#define RING_FULL 7000
#define APPLE_HZ 44100                          /* the speaker's samples a second, of the Apple's time */
#define CPU_HZ 1023000                          /* the Apple's cycles a second, and a frame of them (frank-apple's main.c) */
#define FRAME_CYCLES 17030
#define CLICKS 8192
static int16_t ring[RING];
static uint32_t ring_phase, ring_step;
static int audio_on;
static uint64_t clicks[CLICKS];
static int nclicks;
static bool clicks_lost;

/* in PendSV: no services. The emulator keeps its own time and the stream the board's, so the step is trimmed, by up to 1%, to hold the ring at RING_LEVEL */
static void fill(int16_t *buf, uint32_t frames) {
    static bool playing;
    static int16_t v;
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
                v = ring[out & (RING - 1)];
                ph += step;
                out += ph >> 16;
                ph &= 0xFFFF;
                if((int32_t)(in - out) < 0) out = in;
            }
        }
        *buf++ = v;
        *buf++ = v;
    }
    ring_out = out;
    ring_phase = ph;
}

static void sound_start(void) {
    int r = cmm2->audio_start(fill);
    audio_on = r > 0;
    if(audio_on) ring_step = (uint32_t)(((uint64_t)APPLE_HZ << 16) / (uint32_t)r);
}

/* mii.c's, at each access to $C030: the cycle the speaker changed at */
void mii_speaker_click(mii_speaker_t *speaker) {
    (void)speaker;
    if(nclicks < CLICKS) clicks[nclicks++] = g_mii.cpu.total_cycle + g_mii.cpu.cycle;
    else clicks_lost = true;
}

/* the samples up to cycle end: each the speaker's level (+1 or -1) averaged over its 23.2 cycles, then through a DC blocker (a pole at 0.995, 35 Hz) */
#define SPEAKER_AMP 9000.0f
static void speaker_samples(uint64_t end) {
    static double at = -1;                      /* the cycle the next sample starts at */
    static float level = 1, last_in, last_out;
    const double per = (double)CPU_HZ / APPLE_HZ;
    int i = 0;
    if(at < 0 || at > (double)end || (double)end - at > 4.0 * FRAME_CYCLES) at = (double)end - FRAME_CYCLES;   /* (the start, or a gap) */
    while(at + per <= (double)end) {
        double from = at, to = at + per, t = from;
        float sum = 0;
        while(i < nclicks && (double)clicks[i] < to) {
            double c = (double)clicks[i] < from ? from : (double)clicks[i];
            sum += level * (float)(c - t);
            t = c;
            level = -level;
            i++;
        }
        sum += level * (float)(to - t);
        float x = sum / (float)per, y = x - last_in + 0.995f * last_out;
        last_in = x;
        last_out = y;
        at = to;
        uint32_t in = ring_in;
        if(audio_on && in - ring_out < RING_FULL) {
            float s = y * SPEAKER_AMP;
            ring[in & (RING - 1)] = (int16_t)(s > 32767 ? 32767 : s < -32768 ? -32768 : s);
            ring_in = in + 1;
        }
    }
    if(i) memmove(clicks, clicks + i, (nclicks - i) * sizeof(clicks[0]));   /* (the clicks of a sample not yet ended stay) */
    nclicks -= i;
    clicks_lost = false;
}

/* ---- frank-apple's key flags (its ps2kbd_wrapper's and usbhid_wrapper's) ---- */
bool turbo_latched, turbo_momentary, show_speed;
bool ps2kbd_is_turbo(void) { return turbo_latched || turbo_momentary; }
bool ps2kbd_is_show_speed(void) { return show_speed; }

/* main.c's: what the disk menu calls after a disk is booted or put in */
static uint8_t currently_held_key;
static uint32_t key_hold_frames;
void clear_held_key(void) {
    currently_held_key = 0;
    key_hold_frames = 0;
}
uint8_t get_held_key(void) { return currently_held_key; }

/* ---- the keyboard ---- */
#define K_UP 0x80                               /* MMBasic's key codes, as keys_held gives them */
#define K_DOWN 0x81
#define K_LEFT 0x82
#define K_RIGHT 0x83
#define K_DEL 0x7F
#define K_PGUP 0x88
#define K_PGDN 0x89
#define K_F1 0x91
#define M_LALT 1                                /* keys_held's modifier bits */
#define M_CTRL (2 | 32)
#define M_LGUI 4
#define M_SHIFT (8 | 128)
#define M_RALT 16
#define M_RGUI 64
#define H_CAPS 0x39                             /* HID usages, in keys_raw's report */
#define H_SCROLL 0x47
#define H_PAUSE 0x48
#define H_DELETE 0x4C
#define H_END 0x4D
#define H_KP1 0x59                              /* the keypad's 1-9, then 0 */
#define H_KP0 0x62
#define F11 0xFB                                /* frank-apple's codes for F11, Page Up and Page Down (disk_ui.c's) */
#define PGUP 0xFD
#define PGDN 0xFE
#define KEY_REPEAT_INITIAL_DELAY 30             /* main.c's: 0.5 s, then 15 a second */
#define KEY_REPEAT_RATE 4

static bool caps = true;                        /* the Apple's Caps Lock: down */

static bool in_raw(const uint8_t *raw, uint8_t h) {
    for(int i = 2; i < 8; i++) if(raw[i] == h) return true;
    return false;
}

/* the Apple's code for MMBasic's key c, or the F-key and page codes frank-apple's disk menu knows; 0 for none */
static uint8_t apple_key(int c, int mods) {
    switch(c) {
        case 10: case 13: return 0x0D;
        case 8: return 0x08;
        case 9: return 0x09;
        case 27: return 0x1B;
        case K_DEL: return 0x7F;
        case K_UP: return 0x0B;
        case K_DOWN: return 0x0A;
        case K_LEFT: return 0x08;
        case K_RIGHT: return 0x15;
        case K_PGUP: return PGUP;
        case K_PGDN: return PGDN;
    }
    if(c >= K_F1 && c < K_F1 + 12) return 0xF1 + c - K_F1;
    if(c >= 1 && c <= 26) return (uint8_t)c;    /* (Ctrl with a letter) */
    if(c < 0x20 || c > 0x7E) return 0;
    if(mods & M_CTRL) return c >= 0x40 && c <= 0x7E ? (c & 0x1F) : 0;
    if(c >= 'a' && c <= 'z' && (caps || (mods & M_SHIFT))) c -= 0x20;
    else if(c >= 'A' && c <= 'Z' && !caps && !(mods & M_SHIFT)) c += 0x20;   /* (the PC's Caps Lock is not the Apple's) */
    return (uint8_t)c;
}

/* a key going down: frank-apple's F-keys, the disk menu's keys while it is up, else the Apple's (main.c's process_keyboard) */
static void colour_next(void);
static void key_down(uint8_t key) {
    switch(key) {
        case F11:
            disk_ui_toggle();
            return;
        case 0xF9:                              /* F9 */
            show_speed = !show_speed;
            return;
        case 0xFA:                              /* F10 */
            colour_next();
            return;
        case 0xFC:                              /* F12: held (key_frame) */
            return;
    }
    if(disk_ui_is_visible()) {
        disk_ui_handle_key(key);
        currently_held_key = key;
        key_hold_frames = 0;
        return;
    }
    if(key >= 0x80) return;
    mii_keypress(&g_mii, key);
    currently_held_key = key;
    key_hold_frames = 0;
}

static void reset(bool cold) {
    mii_reset(&g_mii, cold);
    clear_held_key();
}

/* the serial console's characters, one at a time as the Apple takes them */
static uint8_t typed_q[1024];
static uint32_t typed_in, typed_out;

static void console_keys(bool usb_held) {
    static bool quiet = true;                   /* (so that what was typed during the start screen is kept) */
    static int prefix;
    int c;
    bool ok = !usb_held && quiet;
    quiet = !usb_held;
    while((c = cmm2->con_getc()) >= 0) {
        if(!ok) continue;                       /* (a USB key's character reaches the console as well) */
        if(prefix) {                            /* Ctrl-F then 1-9, a, b, c; r Ctrl+Reset */
            prefix = 0;
            if(c >= '1' && c <= '9') c = 0xF1 + c - '1';
            else if(c >= 'a' && c <= 'c') c = 0xFA + c - 'a';
            else if(c == 'r') {
                reset(false);
                continue;
            } else continue;
        } else if(c == 6) {
            prefix = 1;
            continue;
        } else if(c == 0x1D) exit(0);           /* Ctrl-] */
        else if(c == 127) c = 8;                /* (Ctrl-J, line feed, is the Apple's down arrow: Return is 13) */
        else if(c >= 0x80) continue;
        if(typed_in - typed_out < sizeof(typed_q)) typed_q[typed_in++ % sizeof(typed_q)] = (uint8_t)c;
    }
    if(typed_out == typed_in) return;
    uint8_t k = typed_q[typed_out % sizeof(typed_q)];
    if(!disk_ui_is_visible() && k < 0x80 && (mii_bank_peek(&g_mii.bank[MII_BANK_SW], SWAKD) & 0x80)) return;   /* (the last not yet taken) */
    typed_out++;
    key_down(k);
    currently_held_key = 0;                     /* (typed: no repeat) */
}

/* each frame: the USB keyboard (keys_held's newest key, as the Atari's), the serial console, and the joystick buttons and paddles */
static uint8_t key_buttons;                     /* the Open and Closed Apple keys: buttons 0 and 1 */
static uint32_t keypad_state;                   /* the keypad's joystick, as frank-apple's DPAD_ bits */
static void keyboard_frame(void) {
    static int last = 0;
    static bool caps_was, scroll_was, reset_was;
    int keys[7], held, mods, c;
    uint8_t raw[8];
    cmm2->poll();
    cmm2->break_take();                         /* (Ctrl-C is the Apple's) */
    held = cmm2->keys_held(keys);
    cmm2->keys_raw(raw);
    mods = keys[6];
    if((mods & M_CTRL) && (mods & (M_LALT | M_RALT)) && in_raw(raw, H_END)) exit(0);   /* Ctrl+Alt+End leaves */
    key_buttons = ((mods & (M_LALT | M_LGUI)) ? 1 : 0) | ((mods & (M_RALT | M_RGUI)) ? 2 : 0);
    {
        bool cad = (mods & M_CTRL) && (mods & (M_LALT | M_RALT)) && in_raw(raw, H_DELETE), cp = (mods & M_CTRL) && in_raw(raw, H_PAUSE);
        if(cad) key_buttons = 0;                /* (frank-apple's start again: not the //e's self-test, which the right Alt held would be) */
        if((cad || cp) && !reset_was) reset(cad);   /* Ctrl+Alt+Delete: from cold; Ctrl+Pause: Ctrl+Reset, with the Apple keys as they are */
        reset_was = cad || cp;
        if(reset_was) {
            last = keys[0];
            return;
        }
    }
    bool caps_now = in_raw(raw, H_CAPS), scroll_now = in_raw(raw, H_SCROLL);
    if(caps_now && !caps_was) caps = !caps;
    if(scroll_now && !scroll_was) turbo_latched = !turbo_latched;
    caps_was = caps_now;
    scroll_was = scroll_now;
    turbo_momentary = false;
    for(int i = 0; i < 6 && i < held; i++) if(keys[i] == K_F1 + 11) turbo_momentary = true;
    keypad_state = 0;
    for(int i = 2; i < 8; i++) {
        int h = raw[i];
        if(h >= H_KP1 && h < H_KP1 + 9) {
            int d = h - H_KP1;                  /* 0-8: 1-9 */
            if(d >= 6) keypad_state |= DPAD_UP;
            if(d <= 2 || d == 4) keypad_state |= DPAD_DOWN;   /* (5 is down too, as frank-apple has it) */
            if(d % 3 == 0) keypad_state |= DPAD_LEFT;
            if(d % 3 == 2) keypad_state |= DPAD_RIGHT;
        } else if(h == H_KP0) keypad_state |= DPAD_START;
    }
    console_keys(held != 0);
    c = keys[0];
    bool keypad = false;
    for(int i = 2; i < 8; i++) if(raw[i] && (i == 7 || raw[i + 1] == 0)) {   /* the newest key, keys_held's first */
        keypad = raw[i] >= H_KP1 && raw[i] <= H_KP0;   /* (the keypad's joystick keys are not keys) */
        break;
    }
    if(c != last) {
        last = c;
        clear_held_key();
        if(c && !keypad) {
            uint8_t k = apple_key(c, mods);
            if(c == 10 && !in_raw(raw, 0x28) && !in_raw(raw, 0x58)) k = 0x0A;   /* (10 is Enter's, and Ctrl+J's: the Apple's down arrow) */
            if(k) key_down(k);
        }
        return;
    }
    if(currently_held_key == 0 || c == 0) return;
    if(++key_hold_frames > KEY_REPEAT_INITIAL_DELAY && (key_hold_frames - KEY_REPEAT_INITIAL_DELAY) % KEY_REPEAT_RATE == 0) {   /* main.c's repeat */
        if(disk_ui_is_visible()) disk_ui_handle_key(currently_held_key);
        else if(!(mii_bank_peek(&g_mii.bank[MII_BANK_SW], SWAKD) & 0x80)) mii_keypress(&g_mii, currently_held_key);
    }
}

/* ---- the gamepad and the joystick (main.c's) ---- */
#define SLOTS 8
#define STICK 32                                /* the stick's dead zone about its centre */
#define PADDLE_CENTER 127
#define PADDLE_SPEED 4
#define GAMEPAD_REPEAT_INITIAL 30
#define GAMEPAD_REPEAT_RATE 4

/* the first pad: DEVICE(GAMEPAD n, B)'s bits as frank-apple's NES ones, and its left stick */
static uint32_t pad_state(int *lx, int *ly) {
    for(int slot = 1; slot <= SLOTS; slot++) {
        int x = 128, y = 128, b = cmm2->gamepad(slot, &x, &y);
        if(b < 0) continue;
        uint32_t s = 0;
        if(b >> 7 & 1) s |= DPAD_UP;
        if(b >> 5 & 1) s |= DPAD_DOWN;
        if(b >> 8 & 1) s |= DPAD_LEFT;
        if(b >> 6 & 1) s |= DPAD_RIGHT;
        if(b & (1 << 11 | 1 << 10)) s |= DPAD_A;   /* a/circle, x/triangle */
        if(b & (1 << 13 | 1 << 12)) s |= DPAD_B;   /* b/cross, y/square */
        if(b >> 1 & 1) s |= DPAD_START;
        if(b >> 3 & 1) s |= DPAD_SELECT;
        *lx = x;
        *ly = y;
        return s;
    }
    *lx = *ly = 128;
    return 0;
}

static void gamepad_frame(void) {
    static uint32_t prev, hold_frames, held_button;
    static bool reset_active;
    static uint8_t joy_x = PADDLE_CENTER, joy_y = PADDLE_CENTER;
    int lx, ly;
    uint32_t state = pad_state(&lx, &ly) | keypad_state;
    uint32_t pressed = state & ~prev;
    if((state & (DPAD_START | DPAD_A | DPAD_B)) == (DPAD_START | DPAD_A | DPAD_B)) {   /* Start+A+B: reset */
        if(!reset_active) reset(true);
        reset_active = true;
        prev = state;
        return;
    }
    reset_active = false;
    if(pressed & DPAD_SELECT) disk_ui_toggle();
    if(disk_ui_is_visible()) {                  /* the d-pad, a and b in the menu, with main.c's repeat */
        static const struct { uint32_t b; uint8_t k; } dir[4] = { {DPAD_UP, 0x0B}, {DPAD_DOWN, 0x0A}, {DPAD_LEFT, 0x08}, {DPAD_RIGHT, 0x15} };
        for(int i = 0; i < 4; i++) if(pressed & dir[i].b) {
            disk_ui_handle_key(dir[i].k);
            held_button = dir[i].b;
            hold_frames = 0;
        }
        if(state & held_button) {
            if(++hold_frames > GAMEPAD_REPEAT_INITIAL && (hold_frames - GAMEPAD_REPEAT_INITIAL) % GAMEPAD_REPEAT_RATE == 0)
                for(int i = 0; i < 4; i++) if(held_button == dir[i].b) disk_ui_handle_key(dir[i].k);
        } else held_button = hold_frames = 0;
        if(pressed & DPAD_A) disk_ui_handle_key(0x0D);
        if(pressed & DPAD_B) disk_ui_handle_key(0x1B);
        prev = state;
        return;
    }
    prev = state;
    mii_bank_t *sw = &g_mii.bank[MII_BANK_SW];
    mii_bank_poke(sw, 0xc061, (state & DPAD_A) || (key_buttons & 1) ? 0x80 : 0);
    mii_bank_poke(sw, 0xc062, (state & DPAD_B) || (key_buttons & 2) ? 0x80 : 0);
    mii_bank_poke(sw, 0xc063, (state & DPAD_START) ? 0x80 : 0);
    if(lx < 128 - STICK || lx > 128 + STICK || ly < 128 - STICK || ly > 128 + STICK) {   /* the stick where it is */
        joy_x = lx > 255 ? 255 : lx;
        joy_y = ly > 255 ? 255 : ly;
    } else {                                    /* the d-pad moving the paddles, for paddle games */
        if(state & DPAD_LEFT) joy_x = joy_x > PADDLE_SPEED ? joy_x - PADDLE_SPEED : 0;
        else if(state & DPAD_RIGHT) joy_x = joy_x < 255 - PADDLE_SPEED ? joy_x + PADDLE_SPEED : 255;
        else joy_x = PADDLE_CENTER;
        if(state & DPAD_UP) joy_y = joy_y > PADDLE_SPEED ? joy_y - PADDLE_SPEED : 0;
        else if(state & DPAD_DOWN) joy_y = joy_y < 255 - PADDLE_SPEED ? joy_y + PADDLE_SPEED : 255;
        else joy_y = PADDLE_CENTER;
    }
    g_mii.analog.v[0].value = joy_x;
    g_mii.analog.v[1].value = joy_y;
}

/* ---- the speed, as main.c shows it (F9) ---- */
void draw_string(uint8_t *fb, int fb_width, int x, int y, const char *str, uint8_t color);
#define CPU_STAT_WINDOW 16
static struct { uint32_t cycles, time_us; } cpu_stat[CPU_STAT_WINDOW];
static int cpu_stat_pos, cpu_stat_count;

static void speed_line(uint8_t *p) {
    uint64_t cycles = 0, us = 0;
    char tmp[32];
    for(int i = 0; i < cpu_stat_count; i++) {
        cycles += cpu_stat[i].cycles;
        us += cpu_stat[i].time_us;
    }
    uint32_t khz = us ? (uint32_t)(cycles * 1000 / us) : 0;
    snprintf(tmp, sizeof(tmp), "CPU %4lu kHz %3lu%%", (unsigned long)khz, (unsigned long)(khz * 100 / 1023));
    memset(ui_fb, 0, UI_W * 16 / 2);
    draw_string(ui_fb, UI_W, 0, 8, tmp, 15);
    ui_expand(p, 0, 16);
}

/* F10: MII's next colour set */
static void colour_next(void) {
    mii_video_set_mode(&g_mii, g_mii.video.color_mode + 1);
}

/* ---- the start ---- */
static mii_rom_t main_rom = { .name = "iiee", .class = "main", .description = "Apple IIe Enhanced ROM", .len = 16384 };
extern const uint8_t mii_rom_iiee[16384];

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    stats = has_arg("-stats");
    logging = has_arg("-log");
    if(cmm2->video_mode(8, 8, &video) < 0) {
        fprintf(stderr, "apple2: cannot set MODE 8,8\n");
        return 1;
    }
    cmm2->con_screen(0);                        /* (its messages to the serial console alone) */
    drive1_cache = malloc(BDSK_BYTES);
    if(drive1_cache) cmm2_psram = drive1_cache - BDSK_BYTES;   /* (drive 1's disk is at PSRAM_DATA + BDSK_BYTES; drive 0's is disk_loader.c's own) */
    memset(pic, 0, sizeof(pic));
    ui_palette();
    cmm2->frame_hook(frame);
    {
        mii_startscreen_info_t info = { .title = "FRANK Apple", .subtitle = "Apple IIe Emulator", .version = "v1.00", .cpu_mhz = cmm2->cpu_hz() / 1000000 };
        start_screen = true;
        mii_startscreen_show(&info);            /* (shown by its wait's tight_loop_contents) */
    }
    disk_loader_init();
    mii_init(&g_mii);
    mii_bank_poke(&g_mii.bank[MII_BANK_SW], SWAN3_REGISTER, 1);   /* main.c's: double hi-res in colour */
    g_mii.video.an3_mode = 1;
    mii_slot_drv_register(&g_mii, 6, "disk2");
    mii_slot_drv_register(&g_mii, 5, "smartport");
    disk_ui_init_with_emulator(&g_mii, 6);
    main_rom.rom = mii_rom_iiee;
    mii_rom_register(&main_rom);
    if(g_mii.video.rom == NULL || g_mii.video.rom->rom == NULL) {
        fprintf(stderr, "apple2: no character ROM\n");
        return 2;
    }
    mii_reset(&g_mii, true);
    mii_ready = true;
    sound_start();
    stat_from = cmm2->micros();
    uint64_t next = cmm2->micros();
    bool menu = false;
    for(;;) {
        uint64_t frame_start = cmm2->micros();
        keyboard_frame();
        gamepad_frame();
        apple_palette();
        if(disk_ui_is_visible()) {
            menu = true;
            disk_ui_render(ui_fb, UI_W, UI_H);
            ui_show();
            next = cmm2->micros() + 22000;      /* (main.c's, while the menu is up) */
        } else {
            uint64_t before = g_mii.cpu.total_cycle;
            menu = false;
            mii_run_cycles(&g_mii, FRAME_CYCLES);
            speaker_samples(g_mii.cpu.total_cycle + g_mii.cpu.cycle);
            uint8_t *p = pic_take(true);
            apple_video_render(&g_mii, p + APPLE_Y * PIC_W + APPLE_X, PIC_W);
            disk_lights(p, g_mii.video.frame_count);
            if(show_speed) speed_line(p);
            palette_show();
            pic_give();
            stat_frame();
            cpu_stat[cpu_stat_pos].cycles = (uint32_t)(g_mii.cpu.total_cycle - before);
            cpu_stat[cpu_stat_pos].time_us = 0;
            next += 1000000ull * FRAME_CYCLES / CPU_HZ;
            if(ps2kbd_is_turbo()) next = cmm2->micros();
        }
        uint64_t now = cmm2->micros();
        if(now < next) {
            if(stats) stat_idle += next - now;
            if(next - now > 1500) usleep((useconds_t)(next - now - 1000));
            while(cmm2->micros() < next) { }
        } else if(now > next + 50000) next = now;   /* (late: not caught up) */
        if(!menu) {
            cpu_stat[cpu_stat_pos].time_us = (uint32_t)(cmm2->micros() - frame_start);
            cpu_stat_pos = (cpu_stat_pos + 1) % CPU_STAT_WINDOW;
            if(cpu_stat_count < CPU_STAT_WINDOW) cpu_stat_count++;
        }
    }
    return 0;
}
