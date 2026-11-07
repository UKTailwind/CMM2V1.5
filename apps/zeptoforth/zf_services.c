/* zf_services.c: the firmware's services for Forth. forth/cmm2.fs has the words, and each comes here through the kernel's one word, cmm2-call (port/services.s). MIT licence, as
   zeptoforth.

   A shim is given the data stack as it was under the index, sp[0] the top cell, and returns where it left it: a word's stack effect is its shim's. An answer that may be an error
   comes back as the firmware gave it, and forth/cmm2.fs raises the exception. Strings arrive as ( addr bytes ) and leave here with a NUL. A double is ( lo hi ).

   This file is linked after the kernel, so that adding to it moves no kernel word and a saved image stays good. The table only grows, as an image holds its numbers.

   All of it runs in the thread, on C's own stack, with preemption held off by zf_in_c, as the services are not reentrant: one buffer of each kind does for the length of a call.
   Text that comes back to Forth stays in a buffer of the word's own until that word next runs. No service is called from the tick or an interrupt: the one thing here that runs
   in an interrupt is fill, which only copies.

   What the end of a program would put back, reboot must: the firmware does not know of it, and what was opened may be held only by RAM words that are gone. So what is open is
   noted here, for zf_svc_reset. */
#include <stdio.h>
#include <string.h>
#include "cmm2.h"

typedef uint32_t cell;
typedef cell *shim(cell *sp);

#define TEXT_MAX  256                           /* MMBasic's strings; a name is no longer than this either */
#define NAME_BAD  (-6)                          /* FatFs's FR_INVALID_NAME, negated */
#define TOO_LONG  (-3)                          /* as basic_cmd says it */
#define FLAG(x)   ((x) ? (cell)-1 : 0)

/* n arguments off the stack, one answer on */
#define RET(n, v) do { cell r_ = (cell)(v); sp += (n) - 1; sp[0] = r_; return sp; } while(0)

extern volatile uint32_t zf_frames;

/* zf_console.c */
extern void zf_con_mode(void), zf_con_colour(uint32_t fg, uint32_t bg), zf_con_cls(uint32_t colour), zf_con_shown(int on), zf_con_close(void);
extern int zf_con_font(int font), zf_con_size(int cols, int rows);
extern void zf_con_sync(void);

static char text1[TEXT_MAX], text2[TEXT_MAX];

/* the ( addr bytes ) at sp as C's, or NULL if it is too long */
static const char *cstr(char *to, const cell *sp) {
    uint32_t n = sp[0];
    if(n >= TEXT_MAX) return NULL;
    memcpy(to, (const void *)sp[1], n);
    to[n] = 0;
    return to;
}

static double dbl(const cell *sp) {
    union { double d; cell c[2]; } u;
    u.c[0] = sp[1];
    u.c[1] = sp[0];
    return u.d;
}

/* the console and the time */

static cell *s_con_screen(cell *sp) {           /* ( flag -- ) */
    zf_con_shown(sp[0] != 0);
    return sp + 1;
}

static cell *s_con_font(cell *sp) {             /* ( index -- status ) */
    RET(1, zf_con_font(sp[0]));
}

static cell *s_con_size(cell *sp) {             /* ( cols rows -- status ) */
    RET(2, zf_con_size(sp[1], sp[0]));
}

static cell *s_micros(cell *sp) {               /* ( -- lo hi ) */
    uint64_t t = cmm2->micros();
    sp -= 2;
    sp[1] = (cell)t;
    sp[0] = (cell)(t >> 32);
    return sp;
}

static cell *s_delay_us(cell *sp) {             /* ( us -- ) */
    cmm2->delay_us(sp[0]);
    return sp + 1;
}

static cell *s_datetime(cell *sp) {             /* ( -- year month day hour minute second ) */
    int t[6], i;
    cmm2->datetime(t);
    sp -= 6;
    for(i = 0; i < 6; i++) sp[5 - i] = t[i];
    return sp;
}

/* files */

#define FILES 8
#define DIRS  4
static uint32_t files_open, dirs_open;          /* a bit for each handle */
static cmm2_dirent entry[DIRS];                 /* each directory's own, so that a task listing one is not disturbed by another */
static char cwd[TEXT_MAX];

static char file_names[FILES][TEXT_MAX];         /* (file_truncate reopens them) */

static cell *s_file_open(cell *sp) {            /* ( addr bytes mode -- handle ) */
    const char *name = cstr(text1, sp + 1);
    int h = name ? cmm2->file_open(name, sp[0]) : NAME_BAD;
    if(h >= 0) {
        files_open |= 1u << h;
        if(h < FILES) strcpy(file_names[h], name);
    }
    RET(3, h);
}

/* ( handle -- handle' ): the file ended where the handle is. The firmware has no service for that, so what is to stay is copied to a new file, which takes the old one's name. The
   handle may change. If the firmware fails part of the way, the file is left as it was as far as can be */
static cell *s_file_truncate(cell *sp) {
    static char tmp[TEXT_MAX + 8], block[4096];
    int h = sp[0], out, pos, left;
    if(h < 0 || h >= FILES || !(files_open & 1u << h)) RET(1, -9);
    pos = cmm2->file_tell(h);
    if(pos < 0 || pos >= cmm2->file_size(h)) RET(1, pos < 0 ? pos : h);
    snprintf(tmp, sizeof(tmp), "%s.tmp", file_names[h]);
    out = cmm2->file_open(tmp, CMM2_O_WRITE | CMM2_O_CREATE);
    if(out < 0) RET(1, out);
    cmm2->file_seek(h, 0);
    for(left = pos; left > 0; ) {
        int n = cmm2->file_read(h, block, left < (int)sizeof(block) ? left : (int)sizeof(block));
        if(n <= 0 || cmm2->file_write(out, block, n) != n) {
            cmm2->file_close(out);
            cmm2->file_remove(tmp);
            cmm2->file_seek(h, pos);
            RET(1, -1);
        }
        left -= n;
    }
    cmm2->file_close(out);
    cmm2->file_close(h);
    files_open &= ~(1u << h);
    cmm2->file_remove(file_names[h]);
    cmm2->file_rename(tmp, file_names[h]);
    strcpy(tmp, file_names[h]);
    h = cmm2->file_open(tmp, CMM2_O_READ | CMM2_O_WRITE);
    if(h < 0) RET(1, h);
    files_open |= 1u << h;
    strcpy(file_names[h], tmp);
    cmm2->file_seek(h, pos);
    RET(1, h);
}

static cell *s_files_sync(cell *sp) {           /* ( -- ) */
    int i;
    for(i = 0; i < FILES; i++) if(files_open & 1u << i) cmm2->file_sync(i);
    return sp;
}

static cell *s_file_close(cell *sp) {           /* ( handle -- status ) */
    int h = sp[0];
    if(h >= 0 && h < FILES) files_open &= ~(1u << h);
    RET(1, cmm2->file_close(h));
}

static cell *s_file_read(cell *sp) {            /* ( addr bytes handle -- count ) */
    RET(3, cmm2->file_read(sp[0], (void *)sp[2], sp[1]));
}

static cell *s_file_write(cell *sp) {           /* ( addr bytes handle -- count ) */
    RET(3, cmm2->file_write(sp[0], (const void *)sp[2], sp[1]));
}

static cell *s_file_seek(cell *sp) {            /* ( pos handle -- status ) */
    RET(2, cmm2->file_seek(sp[0], sp[1]));
}

static cell *s_file_tell(cell *sp) {            /* ( handle -- pos ) */
    RET(1, cmm2->file_tell(sp[0]));
}

static cell *s_file_size(cell *sp) {            /* ( handle -- size ) */
    RET(1, cmm2->file_size(sp[0]));
}

static cell *s_file_sync(cell *sp) {            /* ( handle -- status ) */
    RET(1, cmm2->file_sync(sp[0]));
}

static cell *s_file_remove(cell *sp) {          /* ( addr bytes -- status ) */
    const char *name = cstr(text1, sp);
    RET(2, name ? cmm2->file_remove(name) : NAME_BAD);
}

static cell *s_file_rename(cell *sp) {          /* ( from-addr from-bytes to-addr to-bytes -- status ) */
    const char *to = cstr(text1, sp), *from = cstr(text2, sp + 2);
    RET(4, to && from ? cmm2->file_rename(from, to) : NAME_BAD);
}

static cell *s_file_stat(cell *sp) {            /* ( addr bytes -- size dir? year month day hour minute second status ) */
    cmm2_stat st;
    const char *name = cstr(text1, sp);
    int r = name ? cmm2->file_stat(name, &st) : NAME_BAD;
    if(r < 0) memset(&st, 0, sizeof(st));
    sp -= 7;
    sp[8] = st.size;
    sp[7] = FLAG(st.is_dir);
    sp[6] = st.year;
    sp[5] = st.month;
    sp[4] = st.day;
    sp[3] = st.hour;
    sp[2] = st.minute;
    sp[1] = st.second;
    sp[0] = r;
    return sp;
}

static cell *s_exists(cell *sp) {               /* ( addr bytes -- 1 | 0 | error ): not being there is an answer here, not an error */
    cmm2_stat st;
    const char *name = cstr(text1, sp);
    int r = name ? cmm2->file_stat(name, &st) : NAME_BAD;
    RET(2, r == 0 ? 1 : r == -4 || r == -5 ? 0 : r);
}

static cell *s_dir_make(cell *sp) {             /* ( addr bytes -- status ) */
    const char *name = cstr(text1, sp);
    RET(2, name ? cmm2->dir_make(name) : NAME_BAD);
}

static cell *s_dir_open(cell *sp) {             /* ( addr bytes -- handle ) */
    const char *name = cstr(text1, sp);
    int h = name ? cmm2->dir_open(name) : NAME_BAD;
    if(h >= 0) dirs_open |= 1u << h;
    RET(2, h);
}

static cell *s_dir_read(cell *sp) {             /* ( handle -- addr bytes size dir? status ) */
    int h = sp[0], r = -9;                      /* (-9: not an open directory, FatFs's FR_INVALID_OBJECT) */
    cmm2_dirent *e = entry;
    if(h >= 0 && h < DIRS) {
        e = entry + h;
        r = cmm2->dir_read(h, e);
    }
    if(r <= 0) e = NULL;
    sp -= 4;
    sp[4] = (cell)(e ? e->name : text1);
    sp[3] = e ? strlen(e->name) : 0;
    sp[2] = e ? e->size : 0;
    sp[1] = FLAG(e && e->is_dir);
    sp[0] = r;
    return sp;
}

static cell *s_dir_close(cell *sp) {            /* ( handle -- status ) */
    int h = sp[0];
    if(h >= 0 && h < DIRS) dirs_open &= ~(1u << h);
    RET(1, cmm2->dir_close(h));
}

static cell *s_dir_change(cell *sp) {           /* ( addr bytes -- status ) */
    const char *name = cstr(text1, sp);
    RET(2, name ? cmm2->dir_change(name) : TOO_LONG);
}

static cell *s_dir_current(cell *sp) {          /* ( -- addr bytes status ) */
    int r = cmm2->dir_current(cwd, sizeof(cwd));
    sp -= 3;
    sp[2] = (cell)cwd;
    sp[1] = r < 0 ? 0 : strlen(cwd);
    sp[0] = r;
    return sp;
}

/* the screen */

static cell *s_mode(cell *sp) {                 /* ( mode depth -- status ) */
    cmm2_video v;
    int r = cmm2->video_mode(sp[1], sp[0], &v);
    if(r >= 0) zf_con_mode();
    RET(2, r);
}

static cell *s_mode_scaled(cell *sp) {          /* ( mode depth scale -- status ) */
    cmm2_video v;
    int r = cmm2->video_scaled(sp[2], sp[1], sp[0], &v);
    if(r >= 0) zf_con_mode();
    RET(3, r);
}

static cell *s_palette(cell *sp) {              /* ( start count addr -- status ) */
    RET(3, cmm2->video_palette(sp[2], sp[1], (const uint8_t *)sp[0]));
}

/* a frame's count, not video_wait: that spins here with preemption held off, and a task drawing a frame at a time would leave the others hardly any time. wait-frame
   watches this with pause. It is the same moment: the firmware's frame hook and video_wait go by the one interrupt */
static cell *s_frames(cell *sp) {               /* ( -- count ) */
    RET(0, zf_frames);
}

static cell *s_target(cell *sp) {               /* ( -- fb width height pitch bpp copies status ) */
    cmm2_target t;
    int r = cmm2->gfx_target(&t);
    uint32_t line;
    if(r < 0) memset(&t, 0, sizeof(t));
    line = t.width * t.bpp / 8;
    sp -= 7;
    sp[6] = (cell)t.fb;
    sp[5] = t.width;
    sp[4] = t.height;
    sp[3] = t.pitch;
    sp[2] = t.bpp;
    sp[1] = line && t.pitch >= 2 * line ? 2 : 1;
    sp[0] = r;
    return sp;
}

static cell *s_dma2d_copy(cell *sp) {           /* ( src src-pitch dst dst-pitch width height -- status ) */
    RET(6, cmm2->dma2d_copy((const void *)sp[5], sp[4], (void *)sp[3], sp[2], sp[1], sp[0]));
}

static cell *s_dma2d_busy(cell *sp) {           /* ( -- count ) */
    RET(0, cmm2->dma2d_busy());
}

static cell *s_cls(cell *sp) {                  /* ( colour -- status ) */
    int r = cmm2->gfx_cls(sp[0]);
    if(r >= 0) zf_con_cls(sp[0]);
    RET(1, r);
}

static cell *s_pixel(cell *sp) {                /* ( x y colour -- status ) */
    RET(3, cmm2->gfx_pixel(sp[2], sp[1], sp[0]));
}

static cell *s_line(cell *sp) {                 /* ( x1 y1 x2 y2 width colour -- status ) */
    RET(6, cmm2->gfx_line(sp[5], sp[4], sp[3], sp[2], sp[1], sp[0]));
}

static cell *s_box(cell *sp) {                  /* ( x y w h width colour fill -- status ) */
    RET(7, cmm2->gfx_box(sp[6], sp[5], sp[4], sp[3], sp[2], sp[1], sp[0]));
}

static cell *s_rbox(cell *sp) {                 /* ( x y w h radius colour fill -- status ) */
    RET(7, cmm2->gfx_rbox(sp[6], sp[5], sp[4], sp[3], sp[2], sp[1], sp[0]));
}

static cell *s_circle(cell *sp) {               /* ( x y r width aspect-lo aspect-hi colour fill -- status ) */
    RET(8, cmm2->gfx_circle(sp[7], sp[6], sp[5], sp[4], (float)dbl(sp + 2), sp[1], sp[0]));
}

static cell *s_triangle(cell *sp) {             /* ( x1 y1 x2 y2 x3 y3 colour fill -- status ) */
    RET(8, cmm2->gfx_triangle(sp[7], sp[6], sp[5], sp[4], sp[3], sp[2], sp[1], sp[0]));
}

static cell *s_text(cell *sp) {                 /* ( x y addr bytes just-addr just-bytes font scale colour bg -- status ) */
    const char *s = cstr(text1, sp + 6), *just = cstr(text2, sp + 4);
    RET(10, s && just ? cmm2->gfx_text(sp[9], sp[8], s, just, sp[3], sp[2], sp[1], sp[0]) : TOO_LONG);
}

static cell *s_font(cell *sp) {                 /* ( font scale -- width height status ) */
    int w = 0, h = 0, r = cmm2->gfx_font(sp[1], sp[0], &w, &h);
    sp -= 1;
    sp[2] = w;
    sp[1] = h;
    sp[0] = r;
    return sp;
}

static cell *s_font_size(cell *sp) {            /* ( font -- width height status ) */
    int w = 0, h = 0, r = cmm2->font_size(sp[0], &w, &h);
    sp -= 2;
    sp[2] = w;
    sp[1] = h;
    sp[0] = r;
    return sp;
}

static uint32_t fonts_set;                      /* a bit for each font the program gave */

static cell *s_font_define(cell *sp) {          /* ( font addr -- status ) */
    int font = sp[1], r = cmm2->font_define(font, (const uint8_t *)sp[0]);
    if(r == 0) fonts_set |= 1u << font;
    RET(2, r);
}

static cell *s_colour(cell *sp) {               /* ( colour bg -- status ) */
    int r = cmm2->gfx_colour(sp[1], sp[0]);
    if(r >= 0) zf_con_colour(sp[1], sp[0]);
    RET(2, r);
}

static cell *s_blit_read(cell *sp) {            /* ( x y w h addr -- status ) */
    RET(5, cmm2->gfx_read(sp[4], sp[3], sp[2], sp[1], (uint8_t *)sp[0]));
}

static cell *s_blit_write(cell *sp) {           /* ( x y w h addr -- status ) */
    RET(5, cmm2->gfx_write(sp[4], sp[3], sp[2], sp[1], (const uint8_t *)sp[0]));
}

static cell *s_pixel_read(cell *sp) {           /* ( x y -- colour status ) */
    uint8_t bgr[3] = {0, 0, 0};
    int r = cmm2->gfx_read(sp[1], sp[0], 1, 1, bgr);
    sp[1] = (cell)bgr[2] << 16 | (cell)bgr[1] << 8 | bgr[0];
    sp[0] = r;
    return sp;
}

static cell *s_save(cell *sp) {                 /* ( addr bytes -- status ) */
    const char *name = cstr(text1, sp);
    RET(2, name ? cmm2->gfx_save(name) : TOO_LONG);
}

static cell *s_image_resize(cell *sp) {         /* ( x y w h new-x new-y new-w new-h page keep-black -- status ) */
    long long a[10];
    int i;
    for(i = 0; i < 10; i++) a[i] = (int32_t)sp[9 - i];
    RET(10, cmm2->image_resize(a));
}

/* sound. A fill runs in an interrupt, so it cannot be Forth: this one takes what Forth left in the ring, and silence when there is none, which is all music needs of it.
   The two counts run on and never go back: the thread alone moves ring_in and fill alone ring_out, so neither needs audio_lock. 186 ms of sound */

#define RING 8192                               /* frames of two 16-bit samples; a power of 2 */
static uint32_t ring[RING];
static volatile uint32_t ring_in, ring_out;
static int stream_rate;

static void fill(int16_t *buf, uint32_t frames) {
    uint32_t *o = (uint32_t *)buf, in = ring_in, out = ring_out, i;
    for(i = 0; i < frames; i++) o[i] = out != in ? ring[out++ % RING] : 0;
    ring_out = out;
}

/* the rate, or the firmware's error. A PLAY STOP through basic may have ended the stream behind this, so sound_state says if it is still going */
static int stream_start(void) {
    int r;
    if(stream_rate && (cmm2->sound_state() & 0xFF)) return stream_rate;
    ring_in = ring_out = 0;
    r = cmm2->audio_start(fill);
    stream_rate = r > 0 ? r : 0;
    return r;
}

static cell *s_audio_start(cell *sp) {          /* ( -- rate ) */
    RET(0, stream_start());
}

static cell *s_audio_stop(cell *sp) {           /* ( -- ) */
    cmm2->audio_stop();
    stream_rate = 0;
    return sp;
}

static cell *s_audio_write(cell *sp) {          /* ( addr frames -- taken ) */
    const uint32_t *p = (const uint32_t *)sp[1];
    uint32_t in = ring_in, n = sp[0], room = RING - (in - ring_out), i;
    if(n > room) n = room;
    for(i = 0; i < n; i++) ring[(in + i) % RING] = p[i];
    __asm volatile("" ::: "memory");            /* (the samples before the count that says they are there) */
    ring_in = in + n;
    RET(2, n);
}

static cell *s_audio_room(cell *sp) {           /* ( -- frames ) */
    RET(0, RING - (ring_in - ring_out));
}

/* music needs the stream under it, so it starts it: an error of audio_start's comes back 10 lower, as music_play's own -2 and -3 mean something else */
static cell *s_music_play(cell *sp) {           /* ( addr bytes loop? bank -- status ) */
    int r = stream_start();
    if(r < 0) RET(4, r - 10);
    RET(4, cmm2->music_play((const void *)sp[3], sp[2], sp[1] != 0, (const uint8_t *)sp[0]));
}

static cell *s_music_stop(cell *sp) {           /* ( -- ) */
    cmm2->music_stop();
    return sp;
}

static cell *s_music_volume(cell *sp) {         /* ( volume -- ) */
    cmm2->music_volume(sp[0]);
    return sp + 1;
}

static cell *s_music_pause(cell *sp) {          /* ( paused? -- ) */
    cmm2->music_pause(sp[0] != 0);
    return sp + 1;
}

static cell *s_music_playing(cell *sp) {        /* ( -- flag ) */
    RET(0, FLAG(cmm2->music_playing()));
}

static cell *s_music_rate(cell *sp) {           /* ( divisor -- ) */
    cmm2->music_rate(sp[0]);
    return sp + 1;
}

static cell *s_sound_state(cell *sp) {          /* ( -- state ) */
    RET(0, cmm2->sound_state());
}

static cell *s_tone(cell *sp) {                 /* ( left right ms -- status ): PLAY TONE with numbers, which basic's constants cannot be without Forth making the line */
    snprintf(text1, sizeof(text1), "PLAY TONE %d,%d,%d", (int)sp[2], (int)sp[1], (int)sp[0]);
    RET(3, cmm2->basic_cmd(text1));
}

/* memory: alloc's blocks and the OPL chips, which live in the firmware's heap and in a table of its that has 64 places. A reboot leaves them lost otherwise */

#define BLOCKS 64
static struct { void *p; int chip; } blocks[BLOCKS];

static void block_add(void *p, int chip) {
    int i;
    for(i = 0; i < BLOCKS && blocks[i].p; i++) { }
    if(i < BLOCKS) {
        blocks[i].p = p;
        blocks[i].chip = chip;
    }
}

static void block_drop(void *p) {
    int i;
    for(i = 0; i < BLOCKS; i++) if(blocks[i].p == p) blocks[i].p = NULL;
}

static cell *s_alloc(cell *sp) {                /* ( bytes -- addr | 0 ) */
    void *p = cmm2->alloc(sp[0]);
    if(p) block_add(p, 0);
    RET(1, p);
}

static cell *s_release(cell *sp) {              /* ( addr -- ) */
    block_drop((void *)sp[0]);
    cmm2->release((void *)sp[0]);
    return sp + 1;
}

static cell *s_opl_new(cell *sp) {              /* ( rate -- chip | 0 ) */
    void *c = cmm2->opl_new(sp[0]);
    if(c) block_add(c, 1);
    RET(1, c);
}

static cell *s_opl_write(cell *sp) {            /* ( chip reg value -- ) */
    cmm2->opl_write((void *)sp[2], sp[1], sp[0]);
    return sp + 3;
}

static cell *s_opl_generate(cell *sp) {         /* ( chip addr frames -- ) */
    cmm2->opl_generate((void *)sp[2], (int16_t *)sp[1], sp[0]);
    return sp + 3;
}

static cell *s_opl_free(cell *sp) {             /* ( chip -- ) */
    block_drop((void *)sp[0]);
    cmm2->opl_free((void *)sp[0]);
    return sp + 1;
}

/* the keyboard, the mouse and the gamepads */

static cell *s_keydown(cell *sp) {              /* ( n -- value ): KEYDOWN(n), 0 to 7 */
    int k[7], count = cmm2->keys_held(k), n = sp[0];
    sp[0] = n == 0 ? count : n >= 1 && n <= 7 ? k[n - 1] : 0;
    return sp;
}

static cell *s_keys_raw(cell *sp) {             /* ( addr -- count ) */
    RET(1, cmm2->keys_raw((uint8_t *)sp[0]));
}

static cell *s_mouse(cell *sp) {                /* ( -- x y buttons wheel flag ) */
    int m[4], r = cmm2->mouse(m);
    sp -= 5;
    sp[4] = m[0];
    sp[3] = m[1];
    sp[2] = m[2];
    sp[1] = m[3];
    sp[0] = FLAG(r);
    return sp;
}

static cell *s_mouse_set(cell *sp) {            /* ( x y -- ) */
    cmm2->mouse_set(sp[1], sp[0]);
    return sp + 2;
}

static cell *s_gamepad(cell *sp) {              /* ( n -- lx ly buttons ) */
    int lx = 128, ly = 128, b = cmm2->gamepad(sp[0], &lx, &ly);
    sp -= 2;
    sp[2] = lx;
    sp[1] = ly;
    sp[0] = b;
    return sp;
}

/* I/O */

static uint64_t pins_set;                       /* a bit for each GP set up */

static cell *s_setpin(cell *sp) {               /* ( gp mode pull -- status ) */
    int gp = sp[2], mode = sp[1], r = cmm2->pin_setup(gp, mode, sp[0]);
    if(r == 0 && gp >= 0 && gp < 64) {
        if(mode == CMM2_PIN_OFF) pins_set &= ~(1ULL << gp);
        else pins_set |= 1ULL << gp;
    }
    RET(3, r);
}

static cell *s_pin_read(cell *sp) {             /* ( gp -- level ) */
    RET(1, cmm2->pin_read(sp[0]));
}

static cell *s_pin_write(cell *sp) {            /* ( level gp -- ) */
    cmm2->pin_write(sp[0], sp[1] != 0);
    return sp + 2;
}

static cell *s_adc(cell *sp) {                  /* ( gp -- microvolts value ) */
    uint32_t uv = 0;
    int v = cmm2->adc_read(sp[0], &uv);
    sp -= 1;
    sp[1] = uv;
    sp[0] = v;
    return sp;
}

static cell *s_pwm(cell *sp) {                  /* ( gp freq duty -- status ) */
    RET(3, cmm2->pwm_set(sp[2], sp[1], sp[0]));
}

static cell *s_pwm_stop(cell *sp) {             /* ( gp -- status ) */
    RET(1, cmm2->pwm_stop(sp[0]));
}

static cell *s_i2c_open(cell *sp) {             /* ( bus khz timeout -- status ) */
    RET(3, cmm2->i2c_open(sp[2], sp[1], sp[0]));
}

static cell *s_i2c_close(cell *sp) {            /* ( bus -- status ) */
    RET(1, cmm2->i2c_close(sp[0]));
}

static cell *s_i2c_xfer(cell *sp) {             /* ( bus addr w wn r rn stop? -- status ) */
    RET(7, cmm2->i2c_xfer(sp[6], sp[5], (const uint8_t *)sp[4], sp[3], (uint8_t *)sp[2], sp[1], sp[0] != 0));
}

static cell *s_spi_open(cell *sp) {             /* ( bus hz mode -- status ) */
    RET(3, cmm2->spi_open(sp[2], sp[1], sp[0]));
}

static cell *s_spi_close(cell *sp) {            /* ( bus -- status ) */
    RET(1, cmm2->spi_close(sp[0]));
}

static cell *s_spi_xfer(cell *sp) {             /* ( bus tx rx n -- status ) */
    RET(4, cmm2->spi_xfer(sp[3], (const uint8_t *)sp[2], (uint8_t *)sp[1], sp[0]));
}

static cell *s_com_open(cell *sp) {             /* ( com baud bits parity stop rxbuf -- status ) */
    RET(6, cmm2->com_open(sp[5], sp[4], sp[3], sp[2], sp[1], sp[0]));
}

static cell *s_com_close(cell *sp) {            /* ( com -- status ) */
    RET(1, cmm2->com_close(sp[0]));
}

static cell *s_com_write(cell *sp) {            /* ( addr bytes com -- count ) */
    RET(3, cmm2->com_write(sp[0], (const uint8_t *)sp[2], sp[1]));
}

static cell *s_com_read(cell *sp) {             /* ( addr bytes com -- count ) */
    RET(3, cmm2->com_read(sp[0], (uint8_t *)sp[2], sp[1]));
}

static cell *s_com_avail(cell *sp) {            /* ( com -- count ) */
    RET(1, cmm2->com_avail(sp[0]));
}

static cell *s_io_error(cell *sp) {             /* ( -- addr bytes ) */
    const char *m = cmm2->io_error();
    sp -= 2;
    sp[1] = (cell)m;
    sp[0] = strlen(m);
    return sp;
}

/* the system */

static char value[TEXT_MAX];

static cell *s_cpu_hz(cell *sp) {               /* ( -- hz ) */
    RET(0, cmm2->cpu_hz());
}

static cell *s_basic(cell *sp) {                /* ( addr bytes -- status ) */
    const char *line = cstr(text1, sp);
    RET(2, line ? cmm2->basic_cmd(line) : TOO_LONG);
}

static cell *s_basic_eval(cell *sp) {           /* ( addr bytes -- addr' bytes' status ) */
    const char *expr = cstr(text1, sp);
    int r = expr ? cmm2->basic_eval(expr, value) : TOO_LONG;
    sp -= 1;
    sp[2] = (cell)value;
    sp[1] = r < 0 ? 0 : strlen(value);
    sp[0] = r;
    return sp;
}

static cell *s_basic_prep(cell *sp) {           /* ( addr bytes tokens size -- length ) */
    const char *line = cstr(text1, sp + 2);
    RET(4, line ? cmm2->basic_prep(line, (char *)sp[1], sp[0]) : TOO_LONG);
}

static cell *s_basic_exec(cell *sp) {           /* ( tokens -- status ) */
    RET(1, cmm2->basic_exec((char *)sp[0]));
}

static cell *s_bye_code(cell *sp) {             /* ( code -- ) */
    extern void zf_faults_back(void);
    zf_con_close();
    zf_faults_back();
    cmm2->leave(sp[0]);
    return sp + 1;
}

static shim *const table[] = {
    /*  0 */ s_con_screen, s_micros, s_delay_us, s_datetime,
    /*  4 */ s_file_open, s_file_close, s_file_read, s_file_write, s_file_seek, s_file_tell, s_file_size, s_file_sync,
    /* 12 */ s_file_remove, s_file_rename, s_file_stat, s_exists,
    /* 16 */ s_dir_make, s_dir_open, s_dir_read, s_dir_close, s_dir_change, s_dir_current,
    /* 22 */ s_mode, s_mode_scaled, s_palette, s_frames, s_target, s_dma2d_copy, s_dma2d_busy,
    /* 29 */ s_cls, s_pixel, s_line, s_box, s_rbox, s_circle, s_triangle, s_text,
    /* 37 */ s_font, s_font_size, s_font_define, s_colour, s_blit_read, s_blit_write, s_pixel_read, s_save, s_image_resize,
    /* 46 */ s_audio_start, s_audio_stop, s_audio_write, s_audio_room,
    /* 50 */ s_music_play, s_music_stop, s_music_volume, s_music_pause, s_music_playing, s_music_rate, s_sound_state, s_tone,
    /* 58 */ s_alloc, s_release, s_opl_new, s_opl_write, s_opl_generate, s_opl_free,
    /* 64 */ s_keydown, s_keys_raw, s_mouse, s_mouse_set, s_gamepad,
    /* 69 */ s_setpin, s_pin_read, s_pin_write, s_adc, s_pwm, s_pwm_stop,
    /* 75 */ s_i2c_open, s_i2c_close, s_i2c_xfer, s_spi_open, s_spi_close, s_spi_xfer,
    /* 81 */ s_com_open, s_com_close, s_com_write, s_com_read, s_com_avail, s_io_error,
    /* 87 */ s_cpu_hz, s_basic, s_basic_eval, s_basic_prep, s_basic_exec, s_bye_code,
    /* 93 */ s_con_font, s_con_size, s_file_truncate, s_files_sync,
};

/* from cmm2-call */
cell *zf_call(uint32_t index, cell *sp) {
    zf_con_sync();
    return index < sizeof(table) / sizeof(table[0]) ? table[index](sp) : sp;
}

/* reboot: all that Forth had of the firmware given back, as the end of a program would. Each close does nothing for what was not opened. The screen stays as it is: the console
   is on it, and a mode change would only lose what is there */
void zf_svc_reset(void) {
    int i;
    cmm2->audio_stop();                         /* (and the music with it) */
    if(cmm2->sound_state() & 0xFF) cmm2->basic_cmd("PLAY STOP");
    stream_rate = 0;
    for(i = 0; i < FILES; i++) if(files_open & 1u << i) cmm2->file_close(i);
    for(i = 0; i < DIRS; i++) if(dirs_open & 1u << i) cmm2->dir_close(i);
    files_open = dirs_open = 0;
    for(i = 0; i < BLOCKS; i++) {
        if(blocks[i].p == NULL) continue;
        if(blocks[i].chip) cmm2->opl_free(blocks[i].p);
        else cmm2->release(blocks[i].p);
        blocks[i].p = NULL;
    }
    if(fonts_set) {                             /* (a console font held in Forth's memory, or one that is about to go) */
        cmm2->gfx_font(1, 1, NULL, NULL);
        for(i = 0; i < 32; i++) if(fonts_set & 1u << i) cmm2->font_define(i, NULL);
        fonts_set = 0;
    }
    for(i = 2; i <= 5; i++) cmm2->pwm_stop(i);
    for(i = 0; i < 2; i++) {
        cmm2->i2c_close(i);
        cmm2->spi_close(i);
    }
    for(i = 1; i <= 7; i++) cmm2->com_close(i); /* (4-7: USB serial, in version 14 of the services; a firmware without them says -1) */
    for(i = 0; i < 64; i++) if(pins_set & 1ULL << i) cmm2->pin_setup(i, CMM2_PIN_OFF, CMM2_PULL_NONE);
    pins_set = 0;
}
