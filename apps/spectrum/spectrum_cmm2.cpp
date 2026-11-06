/* spectrum_cmm2.cpp: pico-spec's drivers on the Colour Maximite 2, by the loader's services (Peter, 5 Nov 2026). Its main.cpp runs as it is, built for a USB keyboard.

   - The display: MODE 7,8, 320 x 240 in 256 colours from the palette, the size of pico-spec's own picture: the Spectrum's 256 x 192 and its border. At the end of each
     Spectrum frame its lines are copied to one of two buffers, turned round as they go (pico-spec keeps each four pixels in the order its HDMI driver wants), and the frame
     hook copies that to the screen in the blanking, so a frame is never shown half drawn. When no frames are coming, in the menus, it copies the lines themselves.
   - The sound: the samples of each frame go through a ring to the firmware's sound stream, at its rate by a phase step, trimmed to hold the ring's level: the emulator keeps
     its own time, the Spectrum's 50 frames a second, the display's 59.94 not coming into it.
   - The keyboard: the firmware's report (keys_raw) is a USB boot keyboard's, which main.cpp's process_kbd_report takes as it stands. Ctrl is Symbol Shift, so Ctrl-C is a
     Spectrum key here and is taken from the firmware before it can end the program: Ctrl+Alt+End leaves, as it does pc386.
   - The gamepads: the first two USB pads as pico-spec's NES pads, nespad_state.
   - The serial console types too, for remote use: keys as they are, Ctrl-F then 1-9, a, b, c for F1-F12, Ctrl-Y then a key for Symbol Shift and the key, Ctrl-U, D, L and R
     for the arrows, Ctrl-] to leave. While a USB key is held its character reaches the console as well, and is left out.
   - Starting again: pico-spec reboots after a change of machine (the watchdog). cmm2_restart does as much: the program's data as it was loaded, .bss clear, and _start.
   RUN "spectrum.app", -stats prints the frames a second, how much of the time the emulator has to spare and the sound ring's level, every 5 s. */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "cmm2.h"
#include "pico.h"
#include "pico/multicore.h"
#include "ff.h"
#include "tusb.h"
#include "ps2kbd_mrmltr.h"
#include "graphics.h"
#include "nespad.h"
#include "psram_spi.h"
#include "pwm_audio.h"
#include "Config.h"
#include "ESPectrum.h"

/* ---- time ---- */
uint64_t time_us_64(void) { return cmm2->micros(); }
void sleep_us(uint64_t us) { usleep((useconds_t)us); }
void sleep_ms(uint32_t ms) { usleep((useconds_t)ms * 1000); }
void tight_loop_contents(void) { }
uint32_t psram_size(void) { return 0; }
void init_psram(void) { }
extern "C" char *cmm2_heap_limit(void) { return (char *)cmm2_start->heap + cmm2_start->heap_size; }
extern "C" void *__real_malloc(size_t n) { return malloc(n); }
extern "C" void __real_free(void *p) { free(p); }
extern "C" bool usb_keyboard_mounted(void) { return true; }
extern "C" { uint16_t control_bits; void send_to_595(uint16_t data) { (void)data; } }   /* (the AY chip a board can have) */

/* ---- starting again ---- */
extern "C" char __fini_array_end[], __image_end[], __bss_start__[], __bss_end__[];
extern "C" int _start(cmm2img_start *s);
extern "C" void ff_close_all(void);
static uint32_t *pristine;                      /* the program's .data, as loaded, from the top of its heap (which a start again keeps) */
static uint32_t first_sp;

static void keep_data(void) {
    uint32_t n = __image_end - __fini_array_end;
    first_sp = (uint32_t)__builtin_frame_address(0);
    if(pristine == NULL && (pristine = (uint32_t *)cmm2_heap_take(n)) != NULL) memcpy(pristine, __fini_array_end, n);
}
__attribute__((section(".preinit_array"), used)) static void (*const keep_data_call)(void) = keep_data;

extern "C" __attribute__((noreturn, noinline)) void cmm2_restart(void) {
    const struct cmm2_services *svc = cmm2;
    cmm2img_start *s = cmm2_start;
    uint32_t *keep = pristine, sp = first_sp;
    if(keep == NULL) exit(0);
    svc->frame_hook(NULL);
    svc->audio_stop();
    ff_close_all();
    volatile uint32_t *d = (volatile uint32_t *)__fini_array_end;   /* (word by word: memcpy and memset are about to change) */
    for(const uint32_t *p = keep; d < (volatile uint32_t *)__image_end; ) *d++ = *p++;
    for(d = (volatile uint32_t *)__bss_start__; d < (volatile uint32_t *)__bss_end__; ) *d++ = 0;
    pristine = keep;
    __asm volatile("mov sp, %0" :: "r"(sp) : "memory");
    _start(s);
    for(;;) { }
}

/* ---- the display ---- */
#define PIC_W 320
#define PIC_H 240
extern "C" uint8_t *getLineBuffer(int line);
extern "C" int get_framebuffer_width(void), get_framebuffer_height(void);
bool SELECT_VGA;
uint8_t linkVGA01;
int graphics_max_tft_freq_mhz;
static cmm2_video video;
static uint32_t pic[2][PIC_W * PIC_H / 4];
static uint32_t *volatile ready;                /* a whole frame not yet shown */
static volatile uint32_t idle_frames;           /* the display's frames since the Spectrum's last */
static uint8_t palette[256 * 3], shown[256 * 3];
static volatile bool palette_new;

static inline uint32_t turned(uint32_t w) { return w >> 16 | w << 16; }

/* the lines' four pixels at a time in the screen's order */
static inline void line_copy(uint32_t *d, const uint32_t *s, int words) {
    while(words--) *d++ = turned(*s++);
}

/* in PendSV, at the start of each frame's blanking */
static void frame(void) {
    const uint32_t *s = ready;
    int w = get_framebuffer_width(), h = get_framebuffer_height();
    int words = (w < PIC_W ? w : PIC_W) / 4, x = (PIC_W - words * 4) / 2, sx = (w - words * 4) / 2 / 4;
    int lines = h < PIC_H ? h : PIC_H, y0 = (PIC_H - lines) / 2;
    if(video.fb == NULL) return;
    if(s != NULL) {
        ready = NULL;
        idle_frames = 0;
        for(int y = 0; y < PIC_H; y++, s += PIC_W / 4) {
            uint8_t *d = video.fb + y * video.pitch;
            memcpy(d, s, PIC_W);
            if(video.copies == 2) memcpy(d + video.width, s, PIC_W);
        }
        return;
    }
    if(++idle_frames < 4) return;               /* (the menus draw on the lines themselves) */
    for(int y = 0; y < lines; y++) {
        const uint32_t *l = (const uint32_t *)getLineBuffer(y);
        if(l == NULL) return;
        uint8_t *d = video.fb + (y + y0) * video.pitch + x;
        line_copy((uint32_t *)d, l + sx, words);
        if(video.copies == 2) memcpy(d + video.width, d, words * 4);
    }
}

/* a Spectrum frame is complete */
static void frame_done(void) {
    static int next;
    int w = get_framebuffer_width(), h = get_framebuffer_height();
    int words = (w < PIC_W ? w : PIC_W) / 4, x = (PIC_W - words * 4) / 2 / 4, sx = (w - words * 4) / 2 / 4;
    int lines = h < PIC_H ? h : PIC_H, y0 = (PIC_H - lines) / 2;
    uint32_t *p = pic[next];
    if(p == ready) p = pic[next ^= 1];          /* (the one not waiting to be shown) */
    for(int y = 0; y < lines; y++) {
        const uint32_t *l = (const uint32_t *)getLineBuffer(y);
        if(l == NULL) return;
        line_copy(p + (y + y0) * (PIC_W / 4) + x, l + sx, words);
    }
    ready = p;
    next ^= 1;
}

void graphics_init(void) {
    if(video.fb != NULL) return;
    if(cmm2->video_mode(7, 8, &video) < 0) {
        fprintf(stderr, "spectrum: cannot set MODE 7,8\n");
        exit(1);
    }
    cmm2->con_screen(0);                        /* (its messages to the serial console alone) */
    cmm2->frame_hook(frame);
}

/* (the picture is getLineBuffer's) */
void graphics_set_buffer(uint8_t *buffer, uint16_t width, uint16_t height) { (void)buffer; (void)width; (void)height; }
void graphics_set_bgcolor(uint32_t rgb) { (void)rgb; }
void graphics_set_flashmode(bool a, bool b) { (void)a; (void)b; }
void graphics_set_pio_clk_div(float d) { (void)d; }
extern "C" void graphics_set_scanlines(uint8_t level) { (void)level; }
extern "C" void graphics_set_dither(bool enabled) { (void)enabled; }
extern "C" void hdmi_reinit(void) { }
extern "C" void hdmi_poll_reinit(void) { }
extern "C" void vga_reinit(void) { }

void graphics_set_palette(uint8_t i, uint32_t rgb) {
    palette[i * 3] = rgb >> 16;
    palette[i * 3 + 1] = rgb >> 8;
    palette[i * 3 + 2] = rgb;
    palette_new = true;
}

extern "C" void vga_set_palette_entry_solid(uint8_t i, uint32_t rgb) { graphics_set_palette(i, rgb); }

/* what main.cpp's second core starts: all but the display, which testPins has done */
void cmm2_core1(void) { graphics_init(); }

/* just before ESPectrum::setup, which can talk to the user: so the display is on by then */
extern "C" int testPins(uint32_t pin0, uint32_t pin1) {
    (void)pin0; (void)pin1;
    graphics_init();
    return 0;
}

/* ---- the sound ---- */
#define RING 8192                               /* a power of 2 */
#define RING_LEVEL 1400                         /* samples: a buffer of 1024 and a frame more, at the Spectrum's rate */
#define RING_FULL 6000                          /* (only if the emulator ran ahead of the stream for long) */
static int16_t ring_l[RING], ring_r[RING];
static volatile uint32_t ring_in, ring_out;     /* ring_out in samples, ring_phase its fraction */
static uint32_t ring_phase, ring_step, out_rate;
static int audio_on;
static volatile uint8_t vol = 16;
uint8_t link_i2s_code = 0xFF;
bool is_i2s_enabled = false;

/* in PendSV: no services. The emulator keeps its own time and the stream the board's, so the step is trimmed, by up to 1%, to hold the ring at RING_LEVEL */
static void fill(int16_t *buf, uint32_t frames) {
    uint32_t out = ring_out, in = ring_in, ph = ring_phase, level = in - out;
    int32_t err = (int32_t)level - RING_LEVEL;
    static bool playing;
    static int16_t l, r;
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
    else out_rate = r;
}

void init_sound() { }
void pcm_call() { }
void pcm_cleanup(void) { }
bool pcm_data_in(void) { return false; }
void pwm_audio_in_frame_started(void) { }

void pcm_setup(int hz) {
    audio_begin();
    ring_step = out_rate ? (uint32_t)(((uint64_t)hz << 16) / out_rate) : 0x10000;
}

esp_err_t pwm_audio_set_volume(int8_t volume) {
    if(volume < -16) vol = 0;
    else vol = (volume > 16 ? 16 : volume) + 16;
    return ESP_OK;
}

/* -stats: the frames, the time the emulator had to spare and the sound ring, since the last report */
static int stats = -1;
static uint32_t stat_frames;
static uint64_t stat_idle, stat_from;

static void stat_frame(void) {
    uint64_t now = cmm2->micros();
    if(stats < 0) {
        stats = 0;
        for(int i = 1; i < cmm2_start->argc; i++) if(strcmp(cmm2_start->argv[i], "-stats") == 0) stats = 1;
        stat_from = now;
    }
    if(!stats) return;
    stat_frames++;
    if(ESPectrum::idle > 0) stat_idle += ESPectrum::idle;
    if(now - stat_from >= 5000000) {
        uint32_t ms = (uint32_t)((now - stat_from) / 1000), idle = (uint32_t)(stat_idle / 1000);
        printf("spectrum: %lu.%lu frames a second, %lu%% spare, sound ring %lu\n", (unsigned long)(stat_frames * 1000 / ms),
               (unsigned long)(stat_frames * 10000 / ms % 10), (unsigned long)(idle * 100 / ms), (unsigned long)(ring_in - ring_out));
        stat_frames = 0;
        stat_idle = 0;
        stat_from = now;
    }
}

/* each frame's samples, from ESPectrum::loop; the menus' clicks too */
esp_err_t pwm_audio_write(uint8_t *bufL, uint8_t *bufR, size_t len, size_t *bytes_written, uint32_t wait_ms) {
    (void)wait_ms;
    if(len > 100) {
        frame_done();
        stat_frame();
    }
    audio_begin();
    if(audio_on > 0) {
        uint32_t vol8 = (uint32_t)(vol + Config::audio_boost) << 3, in;
        while(ring_in - ring_out > RING_FULL) usleep(200);
        in = ring_in;
        for(size_t i = 0; i < len; i++, in++) {
            int32_t l = (int32_t)bufL[i] * (int32_t)vol8, r = (int32_t)bufR[i] * (int32_t)vol8;
            ring_l[in & (RING - 1)] = l > 32767 ? 32767 : (int16_t)l;
            ring_r[in & (RING - 1)] = r > 32767 ? 32767 : (int16_t)r;
        }
        ring_in = in;
    }
    if(bytes_written) *bytes_written = len;
    return ESP_OK;
}

/* ---- the serial console's keys: each pressed for a moment, then let go ---- */
#define TYPED_US 70000
static uint8_t typed_key, typed_mod;
static uint64_t typed_until;
static uint8_t typed_q[256], typed_qm[256];
static uint32_t typed_in, typed_out;

static void typed_put(uint8_t key, uint8_t mod) {
    if(key == 0 || typed_in - typed_out >= sizeof(typed_q)) return;
    typed_q[typed_in & 255] = key;
    typed_qm[typed_in++ & 255] = mod;
}

/* a character's key on a UK or US board, and whether it takes Shift */
static void ascii_key(int c, uint8_t mod) {
    static const char *row = "1234567890", *rows = "!@#$%^&*()", *punct = "-=[]\\;'`,./", *puncts = "_+{}|:\"~<>?";
    static const uint8_t punctk[] = { 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38 };
    const char *p;
    if(c >= 'a' && c <= 'z') typed_put(4 + c - 'a', mod);
    else if(c >= 'A' && c <= 'Z') typed_put(4 + c - 'A', mod | KEYBOARD_MODIFIER_LEFTSHIFT);
    else if(c == '0') typed_put(0x27, mod);
    else if(c >= '1' && c <= '9') typed_put(0x1E + c - '1', mod);
    else if(c == 13 || c == 10) typed_put(0x28, mod);
    else if(c == 27) typed_put(0x29, mod);
    else if(c == 8 || c == 127) typed_put(0x2A, mod);
    else if(c == 9) typed_put(0x2B, mod);
    else if(c == ' ') typed_put(0x2C, mod);
    else if(c && (p = strchr(rows, c)) != NULL) typed_put(p - rows == 9 ? 0x27 : 0x1E + (p - rows) % 10 - ((p - rows) == 9 ? 0 : 0), mod | KEYBOARD_MODIFIER_LEFTSHIFT);
    else if(c && (p = strchr(punct, c)) != NULL) typed_put(punctk[p - punct], mod);
    else if(c && (p = strchr(puncts, c)) != NULL) typed_put(punctk[p - puncts], mod | KEYBOARD_MODIFIER_LEFTSHIFT);
    (void)row;
}

static void console_keys(bool usb_held) {
    static bool quiet;
    static int prefix;
    int c;
    bool ok = !usb_held && quiet;
    quiet = !usb_held;
    while((c = cmm2->con_getc()) >= 0) {
        if(!ok) continue;
        if(prefix == 6) {                       /* Ctrl-F then 1-9, a, b, c */
            prefix = 0;
            if(c >= '1' && c <= '9') typed_put(0x3A + c - '1', 0);
            else if(c >= 'a' && c <= 'c') typed_put(0x43 + c - 'a', 0);
            continue;
        }
        if(prefix == 25) {                      /* Ctrl-Y then a key */
            prefix = 0;
            ascii_key(c, KEYBOARD_MODIFIER_LEFTCTRL);
            continue;
        }
        switch(c) {
            case 6: case 25: prefix = c; break;
            case 0x15: typed_put(0x52, 0); break;
            case 0x04: typed_put(0x51, 0); break;
            case 0x0C: typed_put(0x50, 0); break;
            case 0x12: typed_put(0x4F, 0); break;
            case 0x1D: exit(0);
            default: ascii_key(c, 0);
        }
    }
    uint64_t now = cmm2->micros();
    if(now < typed_until) return;
    if(typed_key) {                             /* let go, and wait as long */
        typed_key = typed_mod = 0;
        typed_until = now + TYPED_US;
    } else if(typed_out != typed_in) {
        typed_key = typed_q[typed_out & 255];
        typed_mod = typed_qm[typed_out++ & 255];
        typed_until = now + TYPED_US;
    }
}

/* ---- the USB keyboard ---- */
#define HID_END 0x4D
#define HID_KP1 0x59

void Ps2Kbd_Mrmltr::tick() {
    static hid_keyboard_report_t prev;
    hid_keyboard_report_t now;
    uint8_t raw[8];
    int held;
    cmm2->poll();
    cmm2->break_take();                         /* (Ctrl-C is Symbol Shift and C) */
    if(palette_new) {                           /* (as video_palette waits for the next frame) */
        palette_new = false;
        if(memcmp(palette, shown, sizeof(shown)) != 0) {
            memcpy(shown, palette, sizeof(shown));
            cmm2->video_palette(0, 256, shown);
        }
    }
    held = cmm2->keys_raw(raw);
    if((raw[0] & 0x11) && (raw[0] & 0x44)) {    /* Ctrl+Alt+End (the keypad's too) leaves */
        for(int i = 2; i < 8; i++) if(raw[i] == HID_END || raw[i] == HID_KP1) exit(0);
    }
    console_keys(held != 0);
    memcpy(&now, raw, sizeof(now));
    if(typed_key) {
        now.modifier |= typed_mod;
        for(int i = 0; i < 6; i++) if(now.keycode[i] == 0) { now.keycode[i] = typed_key; break; }
    }
    if(memcmp(&now, &prev, sizeof(now)) != 0) {
        cb(&now, &prev);
        prev = now;
    }
}

/* ---- the USB gamepads: the first two in the firmware's HID slots ---- */
#define SLOTS 8
#define STICK 64                                /* how far from the centre a stick is the d-pad */
uint32_t nespad_state, nespad_state2;

bool nespad_begin(uint32_t cpu_khz, uint8_t clk, uint8_t data, uint8_t lat) {
    (void)cpu_khz; (void)clk; (void)data; (void)lat;
    return true;
}

/* DEVICE(GAMEPAD n, B)'s bitmap, as a NES pad's */
static uint32_t pad_state(int b, int lx, int ly) {
    uint32_t s = 0;
    if((b >> 7 & 1) || ly < 128 - STICK) s |= DPAD_UP;
    if((b >> 5 & 1) || ly > 128 + STICK) s |= DPAD_DOWN;
    if((b >> 8 & 1) || lx < 128 - STICK) s |= DPAD_LEFT;
    if((b >> 6 & 1) || lx > 128 + STICK) s |= DPAD_RIGHT;
    if(b >> 11 & 1) s |= DPAD_A;                /* a/circle */
    if(b >> 13 & 1) s |= DPAD_B;                /* b/cross */
    if(b >> 10 & 1) s |= DPAD_X;                /* x/triangle */
    if(b >> 12 & 1) s |= DPAD_Y;                /* y/square */
    if(b >> 4 & 1) s |= DPAD_LT;
    if(b & 1) s |= DPAD_RT;
    if(b >> 1 & 1) s |= DPAD_START;
    if(b >> 3 & 1) s |= DPAD_SELECT;
    return s;
}

void nespad_read(void) {
    uint32_t st[2] = { 0, 0 };
    int n = 0, lx, ly, b;
    for(int slot = 1; slot <= SLOTS && n < 2; slot++) {
        lx = ly = 128;
        if((b = cmm2->gamepad(slot, &lx, &ly)) >= 0) st[n++] = pad_state(b, lx, ly);
    }
    nespad_state = st[0];
    nespad_state2 = st[1];
}
