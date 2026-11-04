/* pc386_cmm2.c: tiny386, a 386 PC, on the Colour Maximite 2: the platform hooks pc.h asks for and the front end, as
   tiny386's sdl/main.c and esp/main/esp_main.c are for theirs (Peter, 3 Nov 2026).

   RUN "pc386.app" [, file.ini] [-stats]: the PC that file.ini describes (pc386.ini by default, in the current
   directory), in tiny386's own format: [pc] bios, vga_bios, mem_size, vga_mem_size, hda-hdd, cda-cdd, fda, fdb,
   fill_cmos; [cpu] gen, fpu. And this front end's [pc386]: floppies, the images that A: can be given in turn (the
   first at the start, if fda names none; "none" is an empty drive), and new_disk, a size (64M): hda is made, empty,
   if there is no such file.
   The guest's RAM, its video RAM and the rest are on the program's heap. -stats prints the instructions the PC ran
   each second (its MIPS) to the serial console every 5 s; -keys each keyboard report and console character, -mouse
   each movement of the mouse (the time in ms, x, y, the wheel and the buttons).

   - The screen: tiny386's VGA draws the PC's picture, centred, straight into the CMM2's page: MODE 8,8 (640x480, 256
     colours) on every CMM2, a page small enough for a G1's internal RAM while it lends the program its display memory.
     Each pixel is the number of the VGA DAC's entry it is shown in (tiny386 built with BPP 8), and the CMM2's palette
     is the DAC's 256 colours, set again when they change (a fade redraws nothing); VBE's direct-colour modes show
     through a 3-3-2 RGB palette. Text is drawn with 8-dot characters (vga_force_8dm), so 80 columns are 640 pixels.
   - The keyboard: keys_raw's USB HID usage codes, turned into the Linux input codes tiny386's i8042 takes (the PC's
     scan codes), so DOS sees each key's place and applies its own layout (KEYB). Ctrl+Alt with End leaves, and with
     PgDn or PgUp puts the next or the previous of the floppies in A: (the PC is not given these keys); what A: holds
     is shown at the top of the screen for 3 s.
   - The mouse: the CMM2's, as the PC's PS/2 mouse (CuteMouse, MOUSE.COM, Windows' own driver).
   - The sound: the Adlib, the Sound Blaster 16 and the PC speaker, mixed into the CMM2's sound stream at 44.1 kHz.
   - The serial console types into the PC too, for remote use: its characters as a US keyboard's keys, while no USB
     key is held (the USB keyboard's own characters reach the console as well, and are left out so). Ctrl-] leaves,
     Ctrl-N puts the next floppy in A:, and Ctrl-T prints the PC's text screen there. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include "pc.h"
#include "cmm2.h"

/* the platform hooks (pc.h), and the POSIX calls tiny386 makes */
uint32_t get_uticks(void) { return (uint32_t)cmm2->micros(); }

int clock_gettime(clockid_t clock_id, struct timespec *tp) {
    uint64_t us = cmm2->micros();
    (void)clock_id;
    tp->tv_sec = us / 1000000;
    tp->tv_nsec = (long)(us % 1000000) * 1000;
    return 0;
}

int usleep(useconds_t us) {
    if(us) cmm2->delay_us(us);
    return 0;
}

int tcgetattr(int fd, struct termios *t) { (void)fd; memset(t, 0, sizeof(*t)); return 0; }
int tcsetattr(int fd, int action, const struct termios *t) { (void)fd; (void)action; (void)t; return 0; }
int ioctl(int fd, unsigned long request, ...) { (void)fd; (void)request; return -1; }

void *bigmalloc(size_t size) { return malloc(size); }
void *pcmalloc(long size) { return malloc(size); }

int load_rom(void *phys_mem, const char *file, uword addr, int backward) {
    FILE *fp = fopen(file, "rb");
    if(fp == NULL) {
        fprintf(stderr, "pc386: cannot open %s\n", file);
        exit(2);
    }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);
    char *to = (char *)phys_mem + addr - (backward ? len : 0);
    if(fread(to, 1, len, fp) != (size_t)len) {
        fprintf(stderr, "pc386: cannot read %s\n", file);
        exit(2);
    }
    fclose(fp);
    return (int)len;
}

/* the screen: a line of text over the picture for a while (what A: holds), in the VGA's own 8x16 font, at the top
   left, where a 400-line picture leaves 40 lines of border; put back by a full redraw when it goes. Its colours are
   the palette's nearest to dark blue and yellow, as the PC has set it */
extern const uint8_t vgafont16[256 * 16];
static PC *thepc;
static cmm2_video v;
static uint8_t shown[768], colours[768];                            // the CMM2's palette, and the PC's colours now
static char note[64];
static uint64_t note_until;

static int nearest(int r, int g, int b) {
    int best = 0, bestd = 1 << 30;
    for(int i = 0; i < 256; i++) {
        int dr = shown[3 * i] - r, dg = shown[3 * i + 1] - g, db = shown[3 * i + 2] - b, d = dr * dr + dg * dg + db * db;
        if(d < bestd) { bestd = d; best = i; }
    }
    return best;
}

static void note_draw(void) {
    uint8_t *fb = v.fb;
    int n = strlen(note), bg = nearest(0, 0, 128), fg = nearest(255, 255, 0);
    for(int y = 0; y < 20; y++)                                     // (a dark blue strip, the text in yellow)
        for(int x = 0; x < 8 * n + 16; x++) {
            int c = bg, gx = x - 8, gy = y - 2;
            if(gx >= 0 && gx < 8 * n && gy >= 0 && gy < 16 && (vgafont16[(uint8_t)note[gx / 8] * 16 + gy] & (0x80 >> (gx & 7))))
                c = fg;
            fb[(4 + y) * v.pitch + 8 + x] = c;
        }
}

static void note_set(const char *s) {
    snprintf(note, sizeof(note), "%s", s);
    note_until = cmm2->micros() + 3000000;
    note_draw();
}

static void note_step(PC *pc) {                                     // gone after its time: the strip cleared, all redrawn
    if(!note[0] || cmm2->micros() < note_until) return;
    int n = strlen(note);
    for(int y = 0; y < 20; y++) memset(v.fb + (4 + y) * v.pitch + 8, 0, 8 * n + 16);
    note[0] = 0;
    pc->full_update = 2;
}

static void redraw(void *opaque, int x, int y, int w, int h) {    // (the picture is drawn in the page shown: the
    (void)opaque; (void)x; (void)y; (void)w; (void)h;               // palette set when the PC's colours have changed,
    vga_palette_rgb(thepc->vga, colours);                           // which video_palette shows from the next frame, and
    if(memcmp(colours, shown, sizeof(shown))) {                     // the note put over the picture again)
        memcpy(shown, colours, sizeof(shown));
        cmm2->video_palette(0, 256, shown);
    }
    if(note[0]) note_draw();
}

/* the sound: the Adlib, the Sound Blaster and the PC speaker mixed as tiny386's mixer_callback mixes them, but in the
   CMM2's sound stream's fill (an interrupt on MMBasic's stack: its buffers here, not on the stack), which asks for each
   1024 frames as it wants them, as SDL's audio thread does; tiny386's devices make 44.1 kHz, the stream's rate */
static int16_t mixmono[1024];
static uint8_t mixspk[1024];

static void fill(int16_t *buf, uint32_t frames) {
    PC *pc = thepc;
    if(frames > 1024) frames = 1024;
    memset(buf, 0, frames * 4);
    sb16_audio_callback(pc->sb16, (uint8_t *)buf, frames * 4);      // (s16 stereo)
    memset(mixmono, 0, frames * 2);
    adlib_callback(pc->adlib, (uint8_t *)mixmono, frames * 2);      // (s16 mono)
    int spk = pcspk_get_active_out(pc->pcspk);
    if(spk) {
        memset(mixspk, 0x80, frames);
        pcspk_callback(pc->pcspk, mixspk, frames);                  // (u8 mono)
    }
    for(uint32_t i = 0; i < frames; i++) {
        int m = mixmono[i] + (spk ? ((int)mixspk[i] - 0x80) << 5 : 0);
        for(int k = 0; k < 2; k++) {
            int r = buf[2 * i + k] + m;
            buf[2 * i + k] = r > 32767 ? 32767 : r < -32768 ? -32768 : r;
        }
    }
}

/* the floppies A: is given in turn ([pc386] floppies) */
#define MAXFLOPPY 16
static char *floppy[MAXFLOPPY];
static int floppies, floppy_now;

static void floppy_step(PC *pc, int by) {
    char s[64];
    if(!floppies) return;
    floppy_now = (floppy_now + by + floppies) % floppies;
    if(strcmp(floppy[floppy_now], "none") == 0) {                  // (an empty drive)
        emulink_attach_floppy(pc->emulink, 0, NULL);
        snprintf(s, sizeof(s), "A: empty");
    } else {
        int r = emulink_attach_floppy(pc->emulink, 0, floppy[floppy_now]);
        snprintf(s, sizeof(s), r ? "A: %s cannot be used" : "A: %s", floppy[floppy_now]);
    }
    fprintf(stderr, "pc386: %s\n", s);
    note_set(s);
}

/* the ini file: this front end's [pc386], then tiny386's own */
static long new_disk;

static int ini_entry(void *user, const char *section, const char *name, const char *value) {
    if(strcmp(section, "pc386") == 0) {
        if(strcmp(name, "floppies") == 0) {
            char *s = strdup(value), *t;
            for(t = strtok(s, " ,"); t && floppies < MAXFLOPPY; t = strtok(NULL, " ,")) floppy[floppies++] = t;
        } else if(strcmp(name, "new_disk") == 0) {
            new_disk = atol(value);
            if(strchr(value, 'M') || strchr(value, 'm')) new_disk <<= 20;
            else if(strchr(value, 'K') || strchr(value, 'k')) new_disk <<= 10;
        }
        return 1;
    }
    return parse_conf_ini(user, section, name, value);
}

// a new hard disk image, all zeros, where the ini names one that is not there; 0 if it could not be made
static int disk_make(const char *name, long size) {
    FILE *fp;
    static uint8_t zero[32768];
    if((fp = fopen(name, "rb")) != NULL) {
        fclose(fp);
        return 1;
    }
    fprintf(stderr, "pc386: making %s, %ld MB", name, size >> 20);
    if((fp = fopen(name, "wb")) == NULL) {
        fprintf(stderr, ": cannot\n");
        return 0;
    }
    for(long done = 0; done < size; done += sizeof(zero)) {
        if(fwrite(zero, 1, sizeof(zero), fp) != sizeof(zero)) {
            fclose(fp);
            fprintf(stderr, ": the card is full?\n");
            return 0;
        }
        if(!(done & 0x3FFFFF)) fprintf(stderr, ".");
    }
    fclose(fp);
    fprintf(stderr, "\n");
    return 1;
}

/* the keyboard: HID usage codes (keys_raw) as Linux input codes, Linux's own hid_keyboard[] (drivers/hid/hid-input.c)
   to 0x65; 0 for those the PC has no key for */
static const uint8_t hid_to_linux[0x66] = {
      0,   0,   0,   0,  30,  48,  46,  32,  18,  33,  34,  35,  23,  36,  37,  38,
     50,  49,  24,  25,  16,  19,  31,  20,  22,  47,  17,  45,  21,  44,   2,   3,
      4,   5,   6,   7,   8,   9,  10,  11,  28,   1,  14,  15,  57,  12,  13,  26,
     27,  43,  43,  39,  40,  41,  51,  52,  53,  58,  59,  60,  61,  62,  63,  64,
     65,  66,  67,  68,  87,  88,  99,  70, 119, 110, 102, 104, 111, 107, 109, 106,
    105, 108, 103,  69,  98,  55,  74,  78,  96,  79,  80,  81,  75,  76,  77,  71,
     72,  73,  82,  83,  86, 127,
};
static const uint8_t mod_to_linux[8] = { 29, 42, 56, 125, 97, 54, 100, 126 };   // the modifier bits: LCtrl LShift
                                                                    // LAlt LGUI RCtrl RShift RAlt RGUI
#define HID_END  0x4D                                               // the keys Ctrl+Alt makes the front end's, and
#define HID_PGUP 0x4B                                               // their places on the keypad (1, 9 and 3)
#define HID_PGDN 0x4E
#define HID_KP1  0x59
#define HID_KP3  0x5B
#define HID_KP9  0x61

static uint8_t held[8];                                             // the last report sent on to the PC
static uint8_t seen[8];                                             // the last report as it came
static int debugkeys;                                               // (-keys: each report and console character shown)

static int report_has(const uint8_t *r, uint8_t code) {
    for(int i = 2; i < 8; i++) if(r[i] == code) return 1;
    return 0;
}

// the keys pressed and released since the last report, but for the front end's (Ctrl+Alt+End, PgUp, PgDn); 1 to leave
static int keys(PC *pc) {
    uint8_t r[8];
    int i, leave = 0;
    cmm2->keys_raw(r);
    for(i = 2; i < 8; i++) if(r[i] < 4) r[i] = 0;                  // (a keyboard's error codes: no keys)
    uint8_t was[8];
    memcpy(was, seen, sizeof(was));
    if(debugkeys && memcmp(r, seen, sizeof(seen)))
        fprintf(stderr, "pc386: keys %02X %02X %02X %02X %02X %02X %02X %02X\n", r[0], r[1], r[2], r[3], r[4], r[5],
                r[6], r[7]);
    memcpy(seen, r, sizeof(seen));                                  // (as it came: a key held is acted on once)
    if((r[0] & 0x11) && (r[0] & 0x44))                              // either Ctrl, either Alt
        for(i = 2; i < 8; i++) {
            int k = r[i] == HID_KP1 ? HID_END : r[i] == HID_KP3 ? HID_PGDN : r[i] == HID_KP9 ? HID_PGUP : r[i];
            if(k != HID_END && k != HID_PGUP && k != HID_PGDN) continue;
            if(!report_has(was, r[i])) {                            // (as it goes down)
                if(k == HID_END) leave = 1;
                else floppy_step(pc, k == HID_PGDN ? 1 : -1);
            }
            r[i] = 0;
        }
    for(i = 0; i < 8; i++) {                                        // the modifiers
        int was = (held[0] >> i) & 1, now = (r[0] >> i) & 1;
        if(was != now) ps2_put_keycode(pc->kbd, now, mod_to_linux[i]);
    }
    for(i = 2; i < 8; i++)                                          // released
        if(held[i] && !report_has(r, held[i]) && held[i] < sizeof(hid_to_linux) && hid_to_linux[held[i]])
            ps2_put_keycode(pc->kbd, 0, hid_to_linux[held[i]]);
    for(i = 2; i < 8; i++)                                          // pressed
        if(r[i] && !report_has(held, r[i]) && r[i] < sizeof(hid_to_linux) && hid_to_linux[r[i]])
            ps2_put_keycode(pc->kbd, 1, hid_to_linux[r[i]]);
    memcpy(held, r, sizeof(held));
    return leave;
}

/* the mouse: the CMM2's (USB or PS/2: the mouse service) as the PC's PS/2 mouse, which reports movements. The
   firmware keeps a position clamped to the screen, so it is put back in the middle after each look, and the movement
   is how far it went from there; the buttons' bits are the same (1 left, 2 right, 4 middle), and the wheel's count
   gives its turns */
static int debugmouse;                                              // (-mouse: each movement shown, with the time in ms)
static void mouse(PC *pc) {
    static int started, wheel;
    int m[4], cx = v.width / 2, cy = v.height / 2;
    if(!cmm2->mouse(m)) return;
    if(!started) {
        started = 1;
        wheel = m[3];
        cmm2->mouse_set(cx, cy);
        return;
    }
    int dx = m[0] - cx, dy = m[1] - cy, dz = m[3] - wheel;
    wheel = m[3];
    if(debugmouse && (dx || dy || dz))
        fprintf(stderr, "pc386: mouse %lu %d %d %d %d\n", (unsigned long)(cmm2->micros() / 1000), dx, dy, dz, m[2] & 7);
    if(dx || dy) cmm2->mouse_set(cx, cy);
    ps2_mouse_event(pc->mouse, dx, dy, dz, m[2] & 7);
}

/* the serial console's characters as a US keyboard's keys: the Linux code, with 0x100 for Shift */
static int ascii_key(int c) {
    static const char *row1 = "1234567890-=", *row1s = "!@#$%^&*()_+";
    static const uint8_t row1k[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
    static const char *letters = "qwertyuiopasdfghjklzxcvbnm";
    static const uint8_t letterk[] = { 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 30, 31, 32, 33, 34, 35, 36, 37, 38, 44,
                                       45, 46, 47, 48, 49, 50 };
    static const char *punct = "[];'`\\,./", *puncts = "{}:\"~|<>?";
    static const uint8_t punctk[] = { 26, 27, 39, 40, 41, 43, 51, 52, 53 };
    const char *p;
    if(c == '\r' || c == '\n') return 28;
    if(c == 8 || c == 127) return 14;
    if(c == 27) return 1;
    if(c == '\t') return 15;
    if(c == ' ') return 57;
    if(c >= 'a' && c <= 'z') return letterk[strchr(letters, c) - letters];
    if(c >= 'A' && c <= 'Z') return letterk[strchr(letters, c - 'A' + 'a') - letters] | 0x100;
    if((p = strchr(row1, c)) != NULL) return row1k[p - row1];
    if((p = strchr(row1s, c)) != NULL) return row1k[p - row1s] | 0x100;
    if((p = strchr(punct, c)) != NULL) return punctk[p - punct];
    if((p = strchr(puncts, c)) != NULL) return punctk[p - puncts] | 0x100;
    return 0;
}

// the PC's text screen to the serial console (Ctrl-T there), as DOS sees it: the BIOS's mode, width, rows and page
// (its data area at 0x449-0x484) and the characters at 0xB8000 on, read as the PC would read them
static void dump_text(PC *pc) {
    const uint8_t *bda = (const uint8_t *)pc->phys_mem + 0x400;
    int mode = bda[0x49] & 0x7F, cols = bda[0x4A], rows = bda[0x84] + 1;
    uint32_t base = (mode == 7 ? 0x10000 : 0x18000) + (bda[0x4E] | (bda[0x4F] << 8));
    char line[136];
    if(mode > 3 && mode != 7) {
        fprintf(stderr, "pc386: video mode %02Xh, not text\n", mode);
        return;
    }
    if(cols > 132) cols = 132;
    if(rows < 25 || rows > 60) rows = 25;
    fprintf(stderr, "---- the PC's screen, mode %02Xh, %dx%d\n", mode, cols, rows);
    for(int y = 0; y < rows; y++) {
        int n = 0;
        for(int x = 0; x < cols; x++) {
            int ch = vga_mem_read(pc->vga, base + 2 * (y * cols + x));
            line[n++] = ch >= 32 && ch < 127 ? ch : ch ? '.' : ' ';
        }
        while(n && line[n - 1] == ' ') n--;
        line[n] = 0;
        fprintf(stderr, "%s\n", line);
    }
    fprintf(stderr, "----\n");
}

// the serial console's characters typed in; 1 if Ctrl-] (leave)
static int serial(PC *pc) {
    static int quiet;                                               // no USB key held at the last look, nor now
    int c, k, usb = held[0] || held[2];
    int ok = !usb && quiet;
    quiet = !usb;
    if(cmm2->break_take() && !(held[0] & 0x11)) {                   // Ctrl-C (seen as a break, not a character)
        ps2_put_keycode(pc->kbd, 1, 29);
        ps2_put_keycode(pc->kbd, 1, 46);
        ps2_put_keycode(pc->kbd, 0, 46);
        ps2_put_keycode(pc->kbd, 0, 29);
    }
    while((c = cmm2->con_getc()) >= 0) {
        if(debugkeys) fprintf(stderr, "pc386: console %02X (%s)\n", c, ok ? "typed" : "a USB key's, left out");
        if(c == 0x1D) return 1;
        if(c == 0x14) { dump_text(pc); continue; }
        if(c == 0x0E) { floppy_step(pc, 1); continue; }
        if(!ok || (k = ascii_key(c)) == 0) continue;
        if(k & 0x100) ps2_put_keycode(pc->kbd, 1, 42);
        ps2_put_keycode(pc->kbd, 1, k & 0xFF);
        ps2_put_keycode(pc->kbd, 0, k & 0xFF);
        if(k & 0x100) ps2_put_keycode(pc->kbd, 0, 42);
    }
    return 0;
}

int main(int argc, char **argv) {
    const char *ini = "pc386.ini";
    int stats = 0;
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-stats") == 0) stats = 1;
        else if(strcmp(argv[i], "-keys") == 0) debugkeys = 1;
        else if(strcmp(argv[i], "-mouse") == 0) debugmouse = 1;
        else ini = argv[i];
    }
    PCConfig conf;
    memset(&conf, 0, sizeof(conf));
    conf.mem_size = 4 << 20;
    conf.vga_mem_size = 256 << 10;
    conf.cpu_gen = 3;
    conf.fpu = 1;
    conf.fill_cmos = 1;
    conf.vga_force_8dm = 1;
    cmm2->con_screen(0);
    int err = ini_parse(ini, ini_entry, &conf);
    if(err) {
        fprintf(stderr, err < 0 ? "pc386: cannot open %s\n" : "pc386: %s: an error in line %d\n", ini, err);
        return 1;
    }
    if(floppies && !conf.fdd[0] && strcmp(floppy[0], "none") != 0) conf.fdd[0] = floppy[0];
    if(new_disk && conf.disks[0] && !conf.iscd[0] && !disk_make(conf.disks[0], new_disk)) return 1;
    long room = (long)cmm2_start->heap_size - conf.vga_mem_size - (1536L << 10);   // (the rest of the PC, stdio's
#ifdef FAST_PD                                                                      // buffers and the C library's)
    room = (room - (2L << 20)) * 32 / 33;                           // (the pre-decoder's blocks, fast_pd.h's FAST_PD_SIZE,
#endif                                                              // and its tables: RAM/32)
    if(conf.mem_size > room) conf.mem_size = room & ~0xFFFFL;
    if(conf.mem_size < (1L << 20)) {
        fprintf(stderr, "pc386: only %lu KB of heap\n", (unsigned long)(cmm2_start->heap_size >> 10));
        return 1;
    }
    if(cmm2->video_mode(8, 8, &v) < 0 || v.pitch != v.width) {
        fprintf(stderr, "pc386: MODE 8,8 cannot be set\n");
        return 1;
    }
    cmm2->video_palette(0, 256, shown);                             // (black, as the PC's DAC starts)
    conf.width = v.width;
    conf.height = v.height;
    fprintf(stderr, "pc386: %ld KB of RAM, %dx%d, %s\n", conf.mem_size >> 10, conf.width, conf.height, ini);
    PC *pc = pc_new(redraw, NULL, v.fb, &conf);
    thepc = pc;
    int rate = cmm2->audio_start(fill);
    if(rate < 0) fprintf(stderr, "pc386: no sound (%d)\n", rate);
    else if(rate != 44100) fprintf(stderr, "pc386: the sound stream is at %d Hz, not 44100: its pitch is off\n", rate);
    load_bios_and_reset(pc);
    pc->boot_start_time = get_uticks();
    uint64_t last = 0, polled = 0, shown = cmm2->micros(), drawing = 0;
    long cycles = 0;
    while(pc->shutdown_state != 8) {
        pc_step(pc);
        uint64_t t = cmm2->micros();
        pc_vga_step(pc);
        uint64_t now = cmm2->micros();
        drawing += now - t;
        if(now - polled >= 2000) {                                  // (the USB devices seen to every 2 ms, near the
            polled = now;                                           // prompt's rate: a mouse lost its resolution
            cmm2->poll();                                           // setting at 10 ms, 3 Nov 2026)
        }
        if(now - last >= 10000) {                                   // (the keys, the mouse, every 10 ms)
            last = now;
            if(keys(pc) || serial(pc)) break;
            mouse(pc);
            note_step(pc);
            if(stats && now - shown >= 5000000) {
                long c = cpui386_get_cycle(pc->cpu);
                fprintf(stderr, "pc386: %ld.%02ld MIPS, %d%% drawing\n", (c - cycles) / 5000000, (c - cycles) / 50000 % 100,
                        (int)(drawing * 100 / (now - shown)));
                cycles = c;
                shown = now;
                drawing = 0;
            }
        }
    }
    fprintf(stderr, "pc386: off\n");
    return 0;
}
