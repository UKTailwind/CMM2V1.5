/* a8_cmm2.c: atari800's platform on the Colour Maximite 2, by the loader's services (Peter, 9 Nov 2026, for a user's request). It is a port of atari800 as its
   sdl/ and atari_basic.c are: main runs the emulator as sdl/main.c does, and the PLATFORM_ functions give it the display, the keys, the joysticks, the sound
   and the time. Its own menus (F1, ui_basic.c) draw on the Atari's screen and work as they do on a PC.

   - The display: MODE 7,8, 320 x 240 in 256 colours from the palette, each line shown twice: the middle 320 of the 384 x 240 atari800 draws, which is the
     Atari's normal playfield whole, and a little border above and below it. Atari800's colours (Colours_table, PAL or NTSC) are the palette, and its pixels
     their numbers, so a frame goes to the page as it is. Each finished frame is copied out of atari800's screen into one of two buffers, and the one last
     finished goes to the page at the start of the display's blanking, in the frame hook, by the DMA2D: it is never shown half drawn.
   - The sound: POKEY at the firmware's stream rate (44.1 kHz), mono or stereo as atari800's settings say, through a ring to the stream. Atari800 keeps its
     own speed in step with the sound (Sound_AdjustSpeed), as it does on a PC: the ring is its sound card.
   - The keyboard: what is on the key is what the Atari is given, in any OPTION USBKEYBOARD layout: the character the firmware's layout makes of a key becomes
     the Atari key that types it, with the Atari's Shift as that character needs; Ctrl with a key is the Atari's CTRL with it. A letter is shifted by Shift
     alone: Caps Lock is the Atari's CAPS key, which keeps its own state. atari800's keys (sdl/input.c's): F1 the menu, F2 OPTION, F3 SELECT, F4 START, F5
     RESET (Shift-F5 a cold start), F6 HELP, F7 and Pause BREAK, F9 leaves, F12 turbo; Alt with R, D, C, T, Y, O, S, L, J, A one of the menu's pages;
     Alt+B BASIC on or off, with a cold start (off at first, as atari800 has it: most programs need it off);
     Insert, Delete, Home (CLEAR) and the arrows as the Atari's (CTRL with - = + *); ` and the left Windows key the Atari key (inverse video). Ctrl+Alt+End
     leaves too, as it does the other emulators; Ctrl-C is not MMBasic's here.
   - The joysticks: the first USB pad is joystick 1 (port 1, where nearly every game looks), the second joystick 2: the d-pad or the left stick, any of the
     four face buttons the fire, and the pad's Start, Select and L buttons the console's START, SELECT and OPTION. With one pad, its right stick is joystick 2
     and R1 or R2 its fire, for the twin-stick games (the firmware's gamepad_axes, services version 16). The keypad is joystick 1 as well, as in
     atari800's own defaults: its 1-9 the directions and 0 or 5 the fire, which are not passed on as keys. A DS3 sends nothing until its PS button has been
     pressed.
   - The mouse: atari800 makes it the device chosen in its Controller Configuration (Alt+J): paddles, an ST or Amiga mouse, a trak-ball, a light pen, a
     Koala pad (mouse_frame).
   - The serial console types too, for remote use: a character is the Atari key that types it; Enter, Esc, Tab and Backspace as they are; Ctrl-U, Ctrl-D,
     Ctrl-L and Ctrl-R the arrows; Ctrl-F then 1-9, a, b, c for F1-F12 (F2-F4 held for a moment); Ctrl-] leaves.
   - The ROMs: atari800 looks for Atari's own in the folder it was started in, by their checksums whatever their names, and uses AltirraOS and Altirra
     BASIC, built in, for any it does not find (Peter, 9 Nov 2026: Altirra's are the ones given out; the user's own are used when they are there).
   RUN "atari800.app", -stats prints the frames a second, how much of the time the emulator has to spare, the sound ring's level and the heap in use, every
   5 s; -keys prints each key as it goes down. Other words go to atari800 as its command line (-xl, -5200, -run file.xex, and so on). */
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmm2.h"

#include "config.h"
#include "atari.h"
#include "akey.h"
#include "colours.h"
#include "input.h"
#include "log.h"
#include "monitor.h"
#include "platform.h"
#include "screen.h"
#include "sound.h"
#include "ui.h"

static int has_arg(const char *a) {
    for(int i = 1; i < cmm2_start->argc; i++) if(strcmp(cmm2_start->argv[i], a) == 0) return 1;
    return 0;
}

/* ---- -stats ---- */
static int stats;
static uint32_t stat_frames;
static uint64_t stat_idle, stat_from;
static volatile uint32_t ring_in, ring_out;     /* (the sound's, in frames) */

static void stat_frame(void) {
    uint64_t now = cmm2->micros();
    if(!stats) return;
    stat_frames++;
    if(now - stat_from >= 5000000) {
        uint32_t ms = (uint32_t)((now - stat_from) / 1000), idle = (uint32_t)(stat_idle / 1000);
        printf("atari800: %lu.%lu frames a second, %lu%% spare, sound ring %lu, heap %lu\n", (unsigned long)(stat_frames * 1000 / ms),
               (unsigned long)(stat_frames * 10000 / ms % 10), (unsigned long)(idle * 100 / ms), (unsigned long)(ring_in - ring_out),
               (unsigned long)mallinfo().uordblks);
        stat_frames = 0;
        stat_idle = 0;
        stat_from = now;
    }
}

/* ---- the time ---- */
double PLATFORM_Time(void) { return (double)cmm2->micros() * 1e-6; }

void PLATFORM_Sleep(double s) {
    if(s <= 0) return;
    uint64_t now = cmm2->micros(), until = now + (uint64_t)(s * 1e6);
    if(stats) stat_idle += until - now;
    if(until - now > 1500) usleep((useconds_t)(until - now - 1000));
    while(cmm2->micros() < until) { }
}

/* ---- the display ---- */
#define PIC_W 320
#define PIC_H 240
#define CROP_X ((Screen_WIDTH - PIC_W) / 2)     /* (32: the normal playfield's left edge) */
static cmm2_video video;
static uint8_t __attribute__((aligned(32))) shown_buf[2][PIC_W * PIC_H];
static int shown_next;
static const uint8_t *volatile ready;           /* a whole frame not yet on the page */
static uint8_t palette[256 * 3], palette_shown[256 * 3];
static volatile int palette_new;

/* in PendSV, at the start of each frame's blanking */
static void frame(void) {
    const uint8_t *s = ready;
    if(s == NULL) return;
    ready = NULL;
    cmm2->dma2d_copy(s, PIC_W, video.fb, video.pitch, PIC_W, PIC_H);
    if(video.copies == 2) cmm2->dma2d_copy(s, PIC_W, video.fb + video.width, video.pitch, PIC_W, PIC_H);
}

static void palette_show(void) {                /* (video_palette waits for the next frame) */
    if(!palette_new) return;
    palette_new = 0;
    if(memcmp(palette, palette_shown, sizeof(palette)) == 0) return;
    memcpy(palette_shown, palette, sizeof(palette));
    cmm2->video_palette(0, 256, palette_shown);
}

void PLATFORM_PaletteUpdate(void) {
    for(int i = 0; i < 256; i++) {
        palette[i * 3] = Colours_GetR(i);
        palette[i * 3 + 1] = Colours_GetG(i);
        palette[i * 3 + 2] = Colours_GetB(i);
    }
    palette_new = 1;
}

void PLATFORM_DisplayScreen(void) {
    const uint8_t *s = (const uint8_t *)Screen_atari + CROP_X;
    uint8_t *d = shown_buf[shown_next];
    palette_show();
    for(int y = 0; y < PIC_H; y++, s += Screen_WIDTH, d += PIC_W) memcpy(d, s, PIC_W);
    ready = shown_buf[shown_next];
    shown_next ^= 1;
    stat_frame();
}

/* ---- the sound ---- */
#define RING 16384                              /* frames, a power of 2 */
#define RING_HW 3072                            /* what atari800 may put in it: its "sound card" (the stream takes 1024 at a time) */
static int16_t ring[RING * 2];
static unsigned int ring_chans = 1;
static int audio_rate, sound_paused = 1;

/* in PendSV: no services */
static void fill(int16_t *buf, uint32_t frames) {
    static int16_t l, r;
    uint32_t out = ring_out, in = ring_in;
    while(frames--) {
        if(!sound_paused && out != in) {
            l = ring[(out & (RING - 1)) * 2];
            r = ring[(out & (RING - 1)) * 2 + 1];
            out++;
        } else if(sound_paused) l = r = 0;
        *buf++ = l;
        *buf++ = r;
    }
    ring_out = out;
}

int PLATFORM_SoundSetup(Sound_setup_t *setup) {
    if(audio_rate <= 0) {
        audio_rate = cmm2->audio_start(fill);
        if(audio_rate <= 0) return FALSE;
    }
    setup->freq = audio_rate;
    setup->sample_size = 2;
    if(setup->channels < 1 || setup->channels > 2) setup->channels = 1;
    ring_chans = setup->channels;
    setup->buffer_frames = 1024;
    return TRUE;
}

void PLATFORM_SoundExit(void) {
    if(audio_rate > 0) cmm2->audio_stop();
    audio_rate = 0;
}

void PLATFORM_SoundPause(void) { sound_paused = 1; }
void PLATFORM_SoundContinue(void) { sound_paused = 0; }

unsigned int PLATFORM_SoundAvailable(void) {
    uint32_t level = ring_in - ring_out;
    return level >= RING_HW ? 0 : (RING_HW - level) * ring_chans * 2;
}

void PLATFORM_SoundWrite(UBYTE const *buffer, unsigned int size) {
    const int16_t *s = (const int16_t *)buffer;
    uint32_t in = ring_in, frames = size / (ring_chans * 2);
    while(frames--) {
        int16_t l = *s++, r = ring_chans == 2 ? *s++ : l;
        if(in - ring_out >= RING) break;
        ring[(in & (RING - 1)) * 2] = l;
        ring[(in & (RING - 1)) * 2 + 1] = r;
        in++;
    }
    ring_in = in;
}

/* ---- the keyboard ---- */
/* MMBasic's key codes (Inc/Hardware_Includes.h), as keys_held gives them */
#define K_UP 0x80
#define K_DOWN 0x81
#define K_LEFT 0x82
#define K_RIGHT 0x83
#define K_INSERT 0x84
#define K_DEL 0x7F
#define K_HOME 0x86
#define K_END 0x87
#define K_F1 0x91
#define M_LALT 1                                /* keys_held's modifier bits */
#define M_CTRL (2 | 32)
#define M_LGUI 4
#define M_SHIFT (8 | 128)
/* HID usages, in keys_raw's report */
#define H_CAPS 0x39
#define H_PAUSE 0x48
#define H_END 0x4D
#define H_KP1 0x59                              /* the keypad's 1-9, then 0 */
#define H_KP0 0x62

static int keytrace;

/* the Atari key that types the character c, Shift included as it needs it, or AKEY_NONE */
static int atari_char(int c) {
    static const struct { char c; int k; } other[] = {
        {' ', AKEY_SPACE}, {'!', AKEY_EXCLAMATION}, {'"', AKEY_DBLQUOTE}, {'#', AKEY_HASH}, {'$', AKEY_DOLLAR}, {'%', AKEY_PERCENT},
        {'&', AKEY_AMPERSAND}, {'\'', AKEY_QUOTE}, {'@', AKEY_AT}, {'(', AKEY_PARENLEFT}, {')', AKEY_PARENRIGHT}, {'<', AKEY_LESS},
        {'>', AKEY_GREATER}, {'=', AKEY_EQUAL}, {'?', AKEY_QUESTION}, {'-', AKEY_MINUS}, {'+', AKEY_PLUS}, {'*', AKEY_ASTERISK},
        {'/', AKEY_SLASH}, {':', AKEY_COLON}, {';', AKEY_SEMICOLON}, {',', AKEY_COMMA}, {'.', AKEY_FULLSTOP}, {'_', AKEY_UNDERSCORE},
        {'[', AKEY_BRACKETLEFT}, {']', AKEY_BRACKETRIGHT}, {'^', AKEY_CIRCUMFLEX}, {'\\', AKEY_BACKSLASH}, {'|', AKEY_BAR}, {'`', AKEY_ATARI} };
    static const int digit[10] = { AKEY_0, AKEY_1, AKEY_2, AKEY_3, AKEY_4, AKEY_5, AKEY_6, AKEY_7, AKEY_8, AKEY_9 };
    static const int letter[26] = { AKEY_a, AKEY_b, AKEY_c, AKEY_d, AKEY_e, AKEY_f, AKEY_g, AKEY_h, AKEY_i, AKEY_j, AKEY_k, AKEY_l, AKEY_m,
                                    AKEY_n, AKEY_o, AKEY_p, AKEY_q, AKEY_r, AKEY_s, AKEY_t, AKEY_u, AKEY_v, AKEY_w, AKEY_x, AKEY_y, AKEY_z };
    if(c >= 'a' && c <= 'z') return letter[c - 'a'];
    if(c >= 'A' && c <= 'Z') return letter[c - 'A'] | AKEY_SHFT;
    if(c >= '0' && c <= '9') return digit[c - '0'];
    for(unsigned i = 0; i < sizeof(other) / sizeof(other[0]); i++) if(other[i].c == c) return other[i].k;
    return AKEY_NONE;
}

/* the 5200's keypad and keys (sdl/input.c's) */
static int key_5200(int c) {
    switch(c) {
        case 'p': case 'P': return AKEY_5200_PAUSE;
        case 'r': case 'R': return AKEY_5200_RESET;
        case '#': case '=': return AKEY_5200_HASH;
        case '*': return AKEY_5200_ASTERISK;
    }
    if(c >= '0' && c <= '9') {
        static const int k[10] = { AKEY_5200_0, AKEY_5200_1, AKEY_5200_2, AKEY_5200_3, AKEY_5200_4, AKEY_5200_5, AKEY_5200_6, AKEY_5200_7,
                                   AKEY_5200_8, AKEY_5200_9 };
        return k[c - '0'];
    }
    return AKEY_NONE;
}

/* the menu's pages on Alt with a letter (sdl/input.c's) */
static int alt_page(int c) {
    switch(c | 0x20) {
        case 'r': return UI_MENU_RUN;
        case 'd': return UI_MENU_DISK;
        case 'c': return UI_MENU_CARTRIDGE;
        case 't': return UI_MENU_CASSETTE;
        case 'y': return UI_MENU_SYSTEM;
        case 'o': return UI_MENU_SOUND;
        case 's': return UI_MENU_SAVESTATE;
        case 'l': return UI_MENU_LOADSTATE;
        case 'j': return UI_MENU_CONTROLLER;
        case 'a': return UI_MENU_ABOUT;
    }
    return -1;
}

/* the serial console's keys, each pressed for a moment then let go: c is a character or a key code as keys_held's */
#define TYPED_US 70000
static int typed_q[256], typed_key;
static uint32_t typed_in, typed_out;
static uint64_t typed_until;

static void typed_put(int c) {
    if(typed_in - typed_out < 256) typed_q[typed_in++ & 255] = c;
}

static void console_keys(int usb_held) {
    static int prefix, quiet;
    int c, ok = !usb_held && quiet;
    quiet = !usb_held;
    while((c = cmm2->con_getc()) >= 0) {
        if(!ok) continue;                       /* (a USB key's character reaches the console as well) */
        if(prefix) {
            prefix = 0;
            if(c >= '1' && c <= '9') typed_put(K_F1 + c - '1');
            else if(c >= 'a' && c <= 'c') typed_put(K_F1 + 9 + c - 'a');
            continue;
        }
        switch(c) {
            case 6: prefix = 1; break;          /* Ctrl-F */
            case 0x15: typed_put(K_UP); break;
            case 0x04: typed_put(K_DOWN); break;
            case 0x0C: typed_put(K_LEFT); break;
            case 0x12: typed_put(K_RIGHT); break;
            case 0x1D: exit(0);                 /* Ctrl-] */
            case 10: typed_put(13); break;
            case 127: typed_put(8); break;
            default: typed_put(c);
        }
    }
    uint64_t now = cmm2->micros();
    if(now < typed_until) return;
    if(typed_key) {                             /* let go, and wait as long */
        typed_key = 0;
        typed_until = now + TYPED_US;
    } else if(typed_out != typed_in) {
        typed_key = typed_q[typed_out++ & 255];
        typed_until = now + TYPED_US;
    }
}

/* the joysticks' state, made as the keys are read */
static int stick[2] = { INPUT_STICK_CENTRE, INPUT_STICK_CENTRE }, trig[2] = { 1, 1 }, pad_consol = INPUT_CONSOL_NONE;

#define SLOTS 8
#define STICK 64                                /* how far from the centre a stick is the d-pad */
static int stick_dir(int x, int y) {            /* a stick, 0-255 centred on 128, as an Atari joystick's bits */
    int s = INPUT_STICK_CENTRE;
    if(y < 128 - STICK) s &= ~1;
    if(y > 128 + STICK) s &= ~2;
    if(x < 128 - STICK) s &= ~4;
    if(x > 128 + STICK) s &= ~8;
    return s;
}

static void joysticks(const uint8_t *raw) {
    int n = 0, b, v[6], b1 = 0, rx1 = 128, ry1 = 128;
    stick[0] = stick[1] = INPUT_STICK_CENTRE;
    trig[0] = trig[1] = 1;
    pad_consol = INPUT_CONSOL_NONE;
    for(int slot = 1; slot <= SLOTS && n < 2; slot++) {
        v[0] = v[1] = v[2] = v[3] = 128;
        if(cmm2->version >= 16) b = cmm2->gamepad_axes(slot, v);   /* (both sticks: firmware of 9 Nov 2026 on) */
        else b = cmm2->gamepad(slot, &v[0], &v[1]);
        if(b < 0) continue;
        int s = stick_dir(v[0], v[1]);           /* (DEVICE(GAMEPAD n, B)'s bits: 7 up, 5 down, 8 left, 6 right; 10-13 the face buttons) */
        if(b >> 7 & 1) s &= ~1;
        if(b >> 5 & 1) s &= ~2;
        if(b >> 8 & 1) s &= ~4;
        if(b >> 6 & 1) s &= ~8;
        stick[n] = s;
        if(b & (0xF << 10)) trig[n] = 0;
        if(b >> 1 & 1) pad_consol &= ~INPUT_CONSOL_START;
        if(b >> 3 & 1) pad_consol &= ~INPUT_CONSOL_SELECT;
        if(b >> 4 & 1) pad_consol &= ~INPUT_CONSOL_OPTION;
        if(n == 0) {
            b1 = b;
            rx1 = v[2];
            ry1 = v[3];
        }
        n++;
    }
    if(n == 1) {                                /* one pad: its right stick is joystick 2, and R1 or R2 its fire, for the twin-stick games
                                                   (Robotron: 2084, Space Dungeon, Firefighter: Peter, 9 Nov 2026) */
        stick[1] = stick_dir(rx1, ry1);
        if(b1 & (1 | 1 << 9)) trig[1] = 0;
    }
    for(int i = 2; i < 8; i++) {                /* the keypad: joystick 1 */
        int h = raw[i];
        if(h >= H_KP1 && h < H_KP1 + 9) {
            int d = h - H_KP1;                  /* 0-8: 1-9 */
            if(d >= 6) stick[0] &= ~1;          /* 7 8 9 */
            if(d <= 2) stick[0] &= ~2;          /* 1 2 3 */
            if(d % 3 == 0) stick[0] &= ~4;      /* 1 4 7 */
            if(d % 3 == 2) stick[0] &= ~8;      /* 3 6 9 */
            if(d == 4) trig[0] = 0;             /* 5 */
        } else if(h == H_KP0) trig[0] = 0;
    }
}

static int in_raw(const uint8_t *raw, int h) {
    for(int i = 2; i < 8; i++) if(raw[i] == h) return 1;
    return 0;
}

int PLATFORM_Keyboard(void) {
    static int last = -1, caps_was, gui_was;
    int keys[7], mods, shift, ctrl, c, k, once;
    uint8_t raw[8];
    cmm2->poll();
    cmm2->break_take();                         /* (Ctrl-C is CTRL and C) */
    int held = cmm2->keys_held(keys);
    cmm2->keys_raw(raw);
    console_keys(held != 0);
    joysticks(raw);
    mods = keys[6];
    if((mods & M_CTRL) && (mods & (M_LALT | 16)) && in_raw(raw, H_END)) return AKEY_EXIT;   /* Ctrl+Alt+End leaves */
    c = keys[0];
    if(c == 0 && typed_key) {                   /* the serial console's key, if no USB key is down */
        c = typed_key;
        if(c >= 'A' && c <= 'Z') mods |= 8;
    }
    shift = (mods & M_SHIFT) != 0;
    ctrl = (mods & M_CTRL) != 0;
    INPUT_key_shift = shift;

    /* the console's keys: F2-F4 held, and the pads' */
    INPUT_key_consol = pad_consol;
    for(int i = 0; i < 6; i++) {
        if(keys[i] == K_F1 + 1) INPUT_key_consol &= ~INPUT_CONSOL_OPTION;
        if(keys[i] == K_F1 + 2) INPUT_key_consol &= ~INPUT_CONSOL_SELECT;
        if(keys[i] == K_F1 + 3) INPUT_key_consol &= ~INPUT_CONSOL_START;
    }
    if(typed_key >= K_F1 + 1 && typed_key <= K_F1 + 3) INPUT_key_consol &= ~(1 << (2 - (typed_key - K_F1 - 1)));
    if(Atari800_machine_type == Atari800_MACHINE_5200 && !UI_is_active && !(INPUT_key_consol & INPUT_CONSOL_START)) return AKEY_5200_START;

    once = c != last;                           /* (the menu, reset, leaving and turbo: once for each press) */
    last = c;
    {                                           /* Caps Lock and the Windows key (a modifier) go down on their own */
        int caps = in_raw(raw, H_CAPS), gui = (mods & M_LGUI) != 0, r = AKEY_NONE;
        if(caps && !caps_was) r = AKEY_CAPSTOGGLE;
        else if(gui && !gui_was) r = AKEY_ATARI;
        caps_was = caps;
        gui_was = gui;
        if(r != AKEY_NONE) return r;
    }
    if(in_raw(raw, H_PAUSE)) return AKEY_BREAK;
    if(c == 0) return AKEY_NONE;
    for(int i = 2; i < 8; i++) if(raw[i] && (i == 7 || raw[i + 1] == 0)) {   /* the newest key, keys_held's first, as it is */
        if(!UI_is_active && raw[i] >= H_KP1 && raw[i] <= H_KP0) return AKEY_NONE;   /* (the keypad's joystick keys are not keys: atari800's rule) */
        if(ctrl && c >= 1 && c <= 26 && raw[i] >= 0x04 && raw[i] <= 0x1D) c += 'a' - 1;   /* (Ctrl with a letter, not Backspace, Tab or Enter) */
        break;
    }

    if(c >= K_F1 && c <= K_F1 + 11) {
        switch(c - K_F1 + 1) {
            case 1: return once ? AKEY_UI : AKEY_NONE;
            case 5: return once ? (shift ? AKEY_COLDSTART : AKEY_WARMSTART) : AKEY_NONE;
            case 6: return AKEY_HELP | (shift ? AKEY_SHFT : 0) | (ctrl ? AKEY_CTRL : 0);
            case 7: return AKEY_BREAK;
            case 9: return once ? AKEY_EXIT : AKEY_NONE;
            case 12: return once ? AKEY_TURBO : AKEY_NONE;
        }
        return AKEY_NONE;
    }
    if((mods & M_LALT) && !ctrl) {              /* Alt with a letter: a page of the menu (the right Alt is AltGr) */
        int page = c >= 'A' && c <= 'z' ? alt_page(c) : -1;
        if((c | 0x20) == 'b' && !UI_is_active) {   /* Alt+B: BASIC on or off, and the Atari started again (Peter, 9 Nov 2026: BASIC and a cartridge
                                                   changed without leaving; the menu's Emulator Configuration has it too, and Save Configuration keeps it) */
            if(!once) return AKEY_NONE;
            Atari800_disable_basic = !Atari800_disable_basic;
            printf("atari800: BASIC %s\n", Atari800_disable_basic ? "off" : "on");
            return AKEY_COLDSTART;
        }
        if(page < 0 || !once) return AKEY_NONE;
        UI_alt_function = page;
        return AKEY_UI;
    }
    if(Atari800_machine_type == Atari800_MACHINE_5200 && !UI_is_active) return key_5200(c);

    switch(c) {
        case 10: case 13: k = AKEY_RETURN; break;   /* (the firmware's Enter is 10: Ctrl-J was made a letter above) */
        case 27: k = AKEY_ESCAPE; break;
        case 8: k = AKEY_BACKSPACE; break;
        case 9: k = AKEY_TAB; break;
        case K_UP: return AKEY_UP;
        case K_DOWN: return AKEY_DOWN;
        case K_LEFT: return AKEY_LEFT;
        case K_RIGHT: return AKEY_RIGHT;
        case K_INSERT: return shift ? AKEY_INSERT_LINE : AKEY_INSERT_CHAR;
        case K_DEL: return shift ? AKEY_DELETE_LINE : AKEY_DELETE_CHAR;
        case K_HOME: return ctrl ? AKEY_LESS | AKEY_CTRL : AKEY_CLEAR;
        default:
            if(c >= 'a' && c <= 'z' && shift) c -= 0x20;
            else if(c >= 'A' && c <= 'Z' && !shift) c += 0x20;   /* (the PC's Caps Lock is not the Atari's) */
            k = atari_char(c);
            if(k == AKEY_NONE) return AKEY_NONE;
            if(ctrl) k |= AKEY_CTRL;
            if(keytrace && once) printf("key %d (modifiers %02X): Atari key %02X\n", keys[0], keys[6], k);
            return k;
    }
    if(shift) k |= AKEY_SHFT;                   /* (Return, Esc, Backspace and Tab with Shift or Ctrl are the Atari's own: Tab's set and clear) */
    if(ctrl) k |= AKEY_CTRL;
    return k;
}

/* the mouse (USB or PS/2, the firmware's), each frame: its movement and buttons, which atari800 makes the device chosen in its Controller Configuration
   (Alt+J): paddles, an ST or Amiga mouse, a trak-ball, a light pen or gun, a Koala pad or touch tablet, or a joystick (off at first, as atari800 has it).
   The firmware keeps the mouse's position, clamped to the screen: it is put back in the middle each frame, and the movement is how far it went */
static void mouse_frame(void) {
    static int here;
    int m[4], cx = video.width / 2, cy = video.height / 2;
    INPUT_mouse_delta_x = INPUT_mouse_delta_y = 0;
    if(!cmm2->mouse(m)) {
        here = 0;
        INPUT_mouse_buttons = 0;
        return;
    }
    if(here) {
        INPUT_mouse_delta_x = m[0] - cx;
        INPUT_mouse_delta_y = m[1] - cy;
    }
    here = 1;
    if(m[0] != cx || m[1] != cy) cmm2->mouse_set(cx, cy);
    INPUT_mouse_buttons = m[2] & 7;             /* (1 left, 2 right, 4 middle: atari800's order too) */
}

int PLATFORM_PORT(int num) {
    if(num != 0) return 0xFF;
    return (stick[1] << 4) | stick[0];
}

int PLATFORM_TRIG(int num) {
    return num < 2 ? trig[num] : 1;
}

/* ---- the start and the end ---- */
int PLATFORM_Initialise(int *argc, char *argv[]) {
    if(!Sound_Initialise(argc, argv)) return FALSE;
    return TRUE;
}

int PLATFORM_Exit(int run_monitor) {
    Log_flushlog();
    if(run_monitor && MONITOR_Run()) return TRUE;   /* (the monitor is on the serial console) */
    return FALSE;
}

int main(int argc, char **argv) {
    static char *args[64];
    int n = 0;
    stats = has_arg("-stats");
    keytrace = has_arg("-keys");
    /* BASIC stays off unless asked for (-basic, or the menu's Emulator Configuration), as atari800 has it: most programs load where BASIC would be
       ($A000-$BFFF) and do not run with it on, as on a real XL started without OPTION held (Firefighter, 9 Nov 2026). A BASIC program turns it on
       while it loads */
    for(int i = 0; i < argc && n < 63; i++)     /* (atari800 is given the other words: it stops at one it does not know) */
        if(i == 0 || (strcmp(argv[i], "-stats") != 0 && strcmp(argv[i], "-keys") != 0)) args[n++] = argv[i];
    args[n] = NULL;
    argc = n;
    argv = args;
    stat_from = cmm2->micros();
    if(cmm2->video_mode(7, 8, &video) < 0) {
        fprintf(stderr, "atari800: cannot set MODE 7,8\n");
        return 1;
    }
    cmm2->con_screen(0);                        /* (its messages to the serial console alone) */
    cmm2->frame_hook(frame);
    if(!Atari800_Initialise(&argc, argv)) return 3;
    PLATFORM_PaletteUpdate();
    for(;;) {
        INPUT_key_code = PLATFORM_Keyboard();
        mouse_frame();
        Atari800_Frame();
        if(Atari800_display_screen) PLATFORM_DisplayScreen();
    }
}
