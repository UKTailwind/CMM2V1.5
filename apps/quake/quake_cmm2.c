/* quake_cmm2.c: Quake on the Colour Maximite 2, a loadable program (RUN "quake.app") through quakegeneric, which make.py
   fetches. MIT License, as Inc/cmm2img.h; the program built with it is GPL-2.0, as Quake's source is.

     The picture: MODE 7,8 (320x240, 256 colours). Quake's frame is copied to the page, each line twice as the mode shows
       each one twice, and its palette to the CLUT when it changes.
     The memory: Quake's hunk is a block of MMBasic's heap (the alloc service), outside the 8 MB the program runs in.
       -mem n asks for n MB (12 by default).
     The keys: KEYDOWN's set, read each frame, its changes made Quake's key events. What is typed is the character the
       firmware gives, so OPTION USBKEYBOARD's layout holds; a shifted symbol still counts as its key for the bindings
       (Shift-2 is key 2) and Quake is told the character it stands for in the console and the menus (keyshift).
     The mouse: the USB devices are seen to every 2 ms, however long a frame takes, and its movement taken each time (a
       frame is too long: reports come at half the rate of the polls, and a mouse that sends the low byte of its count
       wraps, so a fast movement turns round; and the firmware's position stops at the edge of the screen).
     The sound: Quake's own mixer at 11,025 Hz, through the firmware's sound stream. There is no music: it was on the CD.
     The files: the game is id1/pak0.pak under the current directory: CHDIR "A:/quake" first.
     Ctrl-C quits (and stops the BASIC program that ran it). */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "cmm2.h"
#include "quakedef.h"
#include "d_local.h"
#include "quakegeneric.h"

qboolean isDedicated;
static uint64_t start;
static uint32_t tickgap;                                            // (the longest wait for USB, in us: -polls)

/* the files */
int Sys_FileOpenRead(char *path, int *hndl) {
    int h = cmm2->file_open(path, CMM2_O_READ);
    if(h < 0) { *hndl = -1; return -1; }
    *hndl = h;
    return cmm2->file_size(h);
}

int Sys_FileOpenWrite(char *path) {
    int h = cmm2->file_open(path, CMM2_O_WRITE | CMM2_O_CREATE);
    if(h < 0) Sys_Error("Error opening %s", path);
    return h;
}

void Sys_FileClose(int handle) { cmm2->file_close(handle); }
void Sys_FileSeek(int handle, int position) { cmm2->file_seek(handle, position); }
int Sys_FileRead(int handle, void *dest, int count) { return cmm2->file_read(handle, dest, count); }
int Sys_FileWrite(int handle, void *data, int count) { return cmm2->file_write(handle, data, count); }

int Sys_FileTime(char *path) {
    cmm2_stat st;
    return cmm2->file_stat(path, &st) == 0 ? 1 : -1;
}

void Sys_mkdir(char *path) { cmm2->dir_make(path); }

/* the system */
void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length) { (void)startaddr; (void)length; }

void Sys_Error(char *error, ...) {
    va_list a;
    printf("Sys_Error: ");
    va_start(a, error);
    vprintf(error, a);
    va_end(a);
    printf("\n");
    exit(1);
}

void Sys_Printf(char *fmt, ...) {
    va_list a;
    va_start(a, fmt);
    vprintf(fmt, a);
    va_end(a);
}

void Sys_Quit(void) {
    Host_Shutdown();
    if(COM_CheckParm("-polls")) printf("the longest wait for USB: %lu us\n", (unsigned long)tickgap);
    exit(0);
}

double Sys_FloatTime(void) { return (double)(cmm2->micros() - start) / 1000000.0; }
char *Sys_ConsoleInput(void) { return NULL; }
void Sys_Sleep(void) {}
void Sys_HighFPPrecision(void) {}
void Sys_LowFPPrecision(void) {}

/* the picture */
#define BASEWIDTH 320
#define BASEHEIGHT 240
viddef_t vid;
static cmm2_video v;
static byte vid_buffer[BASEWIDTH * BASEHEIGHT];
static short zbuffer[BASEWIDTH * BASEHEIGHT];

static void palette(unsigned char *p) { cmm2->video_palette(0, 256, p); }
void VID_SetPalette(unsigned char *p) { palette(p); }
void VID_ShiftPalette(unsigned char *p) { palette(p); }

void VID_Init(unsigned char *p) {
    int size;
    byte *cache;
    if(cmm2->video_mode(7, 8, &v) < 0 || v.width != BASEWIDTH || v.height != BASEHEIGHT) Sys_Error("MODE 7,8 could not be set");
    vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
    vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
    vid.aspect = 1.0;
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong(*((int *)vid.colormap + 2048));
    vid.buffer = vid.conbuffer = vid_buffer;
    vid.rowbytes = vid.conrowbytes = BASEWIDTH;
    d_pzbuffer = zbuffer;
    size = D_SurfaceCacheForRes(BASEWIDTH, BASEHEIGHT);
    cache = malloc(size);
    if(cache == NULL) Sys_Error("no room for the surface cache");
    D_InitCaches(cache, size);
    palette(p);
}

void VID_Shutdown(void) {}

void VID_Update(vrect_t *rects) {
    const byte *s = vid_buffer;
    uint8_t *d = v.fb;
    int i;
    (void)rects;
    for(i = 0; i < BASEHEIGHT; i++, s += BASEWIDTH, d += v.pitch) {
        memcpy(d, s, BASEWIDTH);
        if(v.copies == 2) memcpy(d + v.width, s, BASEWIDTH);
    }
}

void D_BeginDirectRect(int x, int y, byte *pbitmap, int width, int height) { (void)x; (void)y; (void)pbitmap; (void)width; (void)height; }
void D_EndDirectRect(int x, int y, int width, int height) { (void)x; (void)y; (void)width; (void)height; }

/* the sound: Quake mixes at its own 11,025 Hz, 16-bit stereo, into a ring as it would into a sound card's DMA buffer, and
   the firmware's stream takes it from there, each buffer's samples interpolated up to the stream's rate */
#define RING 4096                                                   // frames: 0.37 s
static int16_t ring[RING * 2];
static volatile uint32_t rpos;                                      // where the stream has read to, in frames, 16.16
static uint32_t rstep;

static void fill(int16_t *buf, uint32_t frames) {
    uint32_t p = rpos, i;
    for(i = 0; i < frames; i++, p += rstep) {
        uint32_t a = (p >> 16) & (RING - 1), b = (a + 1) & (RING - 1);
        int f = (p & 0xFFFF) >> 1;
        int l = ring[2 * a], r = ring[2 * a + 1];
        buf[2 * i] = l + (((ring[2 * b] - l) * f) >> 15);
        buf[2 * i + 1] = r + (((ring[2 * b + 1] - r) * f) >> 15);
    }
    rpos = p;
}

qboolean SNDDMA_Init(void) {
    int rate = cmm2->audio_start(fill);
    if(rate <= 0) return false;
    rstep = (uint32_t)(((uint64_t)11025 << 16) / rate);
    shm = &sn;
    shm->splitbuffer = 0;
    shm->samplebits = 16;
    shm->speed = 11025;
    shm->channels = 2;
    shm->samples = RING * 2;
    shm->samplepos = 0;
    shm->submission_chunk = 1;
    shm->buffer = (unsigned char *)ring;
    return true;
}

int SNDDMA_GetDMAPos(void) {
    shm->samplepos = ((rpos >> 16) & (RING - 1)) * 2;
    return shm->samplepos;
}

void SNDDMA_Submit(void) {}
void SNDDMA_Shutdown(void) { cmm2->audio_stop(); }

/* -DPROFILE: where the program is, sampled at each frame of the display from the interrupted PC on the process stack,
   counted in 64-byte buckets of the code, and written to profile.txt at the end */
#ifdef PROFILE
#define PROF_BUCKETS (512 * 1024 / 64)
static uint32_t prof[PROF_BUCKETS], prof_other, prof_base;
static void prof_tick(void) {
    uint32_t psp, pc;
    __asm volatile("mrs %0, psp" : "=r"(psp));
    pc = ((uint32_t *)psp)[6];
    if(pc >= prof_base && pc < prof_base + 512 * 1024) prof[(pc - prof_base) / 64]++;
    else prof_other++;
}
static void prof_write(void) {
    FILE *f;
    uint32_t i;
    cmm2->frame_hook(NULL);
    if((f = fopen("profile.txt", "w")) == NULL) return;
    fprintf(f, "base %lx other %lu
", (unsigned long)prof_base, (unsigned long)prof_other);
    for(i = 0; i < PROF_BUCKETS; i++) if(prof[i]) fprintf(f, "%lx %lu
", (unsigned long)(prof_base + i * 64), (unsigned long)prof[i]);
    fclose(f);
}
#endif

/* the keys: each KEYDOWN code held is turned into Quake's key when it is first seen, and the same key is released when
   it goes; the modifiers' bits are Quake's Ctrl, Shift and Alt, as Doom's front end has them */
#define KQ 64
extern int keyshift[256];
static uint32_t kq[KQ];                                             // what it types with Shift held << 16 | pressed << 8 | key
static int kqin, kqout, held[6], heldkey[6], mods, buttons;

static void kput_as(int pressed, int key, int typed) {
    if(key == 0 || (kqin + 1) % KQ == kqout) return;
    kq[kqin] = (uint32_t)(typed << 16 | pressed << 8 | key);
    kqin = (kqin + 1) % KQ;
}

static void kput(int pressed, int key) { kput_as(pressed, key, 0); }

static int quakekey(int c, int m, int *typed) {
    static const char shifted[] = "!1\"2@2#3$4%5^6&7*8(9)0_-+=<,>.?/:;{[}]|\\~`";
    const char *p;
    *typed = 0;
    if(c >= 1 && c <= 26 && (m & 0x22)) return 'a' + c - 1;         // a letter with Ctrl held: its control code
    switch(c) {
        case 0x80: return K_UPARROW;
        case 0x81: return K_DOWNARROW;
        case 0x82: return K_LEFTARROW;
        case 0x83: return K_RIGHTARROW;
        case 0x84: return K_INS;
        case 0x7F: return K_DEL;
        case 0x86: return K_HOME;
        case 0x87: return K_END;
        case 0x88: return K_PGUP;
        case 0x89: return K_PGDN;
        case 0x08: return K_BACKSPACE;
        case 0x09: return K_TAB;
        case 0x0A: case 0x0D: return K_ENTER;
        case 0x1B: return K_ESCAPE;
    }
    if(c >= 0x91 && c <= 0x9A) return K_F1 + c - 0x91;              // F1-F10, F11, F12
    if(c == 0x9B) return K_F11;
    if(c == 0x9C) return K_F12;
    if(c >= 'A' && c <= 'Z') return c + 32;
    if(m & 0x88) for(p = shifted; *p; p += 2) if(*p == c) { *typed = c; return p[1]; }   // (held Shift only: # is a key of its own in the UK)
    return c < 0x80 ? c : 0;
}

static void keys(void) {
    int k[7], i, j, m, typed;
    if(cmm2->poll()) Sys_Quit();                                    // Ctrl-C
    cmm2->keys_held(k);
    m = k[6];
    for(i = 0; i < 6; i++) {                                        // released: no longer in the set
        if(!held[i]) continue;
        for(j = 0; j < 6 && k[j] != held[i]; j++) {}
        if(j == 6) { kput(0, heldkey[i]); held[i] = 0; }
    }
    if((m ^ mods) & 0x22) kput((m & 0x22) != 0, K_CTRL);            // Ctrl, either
    if((m ^ mods) & 0x88) kput((m & 0x88) != 0, K_SHIFT);           // Shift
    if((m ^ mods) & 0x11) kput((m & 0x11) != 0, K_ALT);             // Alt
    mods = m;
    for(j = 0; j < 6; j++) {                                        // pressed: new in the set
        if(!k[j]) continue;
        for(i = 0; i < 6 && held[i] != k[j]; i++) {}
        if(i < 6) continue;
        for(i = 0; i < 6 && held[i]; i++) {}
        if(i == 6) break;
        held[i] = k[j];
        heldkey[i] = quakekey(k[j], m, &typed);
        kput_as(1, heldkey[i], typed);
    }
}

int QG_GetKey(int *down, int *key) {
    if(kqin == kqout) keys();
    if(kqin == kqout) return 0;
    *down = (kq[kqout] >> 8) & 1;
    *key = (unsigned char)kq[kqout];
    if(kq[kqout] >> 16) keyshift[*key] = kq[kqout] >> 16;           // (what Quake shows for it with Shift held)
    kqout = (kqout + 1) % KQ;
    return 1;
}

/* the keys given to Quake: each frame, and by the loops that wait for a key (SCR_ModalMessage, Con_NotifyBox), which
   have only this to call */
void Sys_SendKeyEvents(void) {
    int down, key;
    while(QG_GetKey(&down, &key)) Key_Event(key, down);
}

/* the mouse: its position, which the firmware keeps, read and put back in the middle at each tick, and the movement
   added up for Quake's next look */
static int movex, movey, wheel, wheelseen;
static uint64_t ticked;

static void mouse_take(void) {
    int m[4], b, dx, dy, cx = BASEWIDTH / 2, cy = BASEHEIGHT / 2;
    if(!cmm2->mouse(m)) return;
    dx = m[0] - cx;
    dy = m[1] - cy;
    if(dx || dy) cmm2->mouse_set(cx, cy);
    movex += dx;
    movey += dy;
    for(b = 0; b < 3; b++) {
        int bit = 1 << b;
        if((m[2] ^ buttons) & bit) kput((m[2] & bit) != 0, K_MOUSE1 + b);
    }
    buttons = m[2];
    if(!wheelseen) { wheelseen = 1; wheel = m[3]; }
    for(; wheel < m[3]; wheel++) { kput(1, K_MWHEELUP); kput(0, K_MWHEELUP); }
    for(; wheel > m[3]; wheel--) { kput(1, K_MWHEELDOWN); kput(0, K_MWHEELDOWN); }
}

/* called through the frame: the scan lines (patches/0001) and wherever Quake sees to its sound */
void Sys_Tick(void) {
    uint64_t now = cmm2->micros();
    uint32_t gap = (uint32_t)(now - ticked);
    if(gap < 2000) return;
    if(ticked && gap > tickgap) tickgap = gap;
    ticked = now;
    cmm2->poll();                                                   // (Ctrl-C it reports is still there for keys())
    mouse_take();
}

void __real_S_ExtraUpdate(void);
void __wrap_S_ExtraUpdate(void) {
    Sys_Tick();
    __real_S_ExtraUpdate();
}

void QG_GetMouseMove(int *x, int *y) {
    Sys_Tick();
    *x = movex;
    *y = movey;
    movex = movey = 0;
}

void QG_GetJoyAxes(float *axes) {
    int i;
    for(i = 0; i < QUAKEGENERIC_JOY_MAX_AXES; i++) axes[i] = 0;
}

int main(int argc, char **argv) {
    static quakeparms_t parms;
    double then, now;
    int i, mb = 12;
    start = cmm2->micros();
    cmm2->con_screen(0);
    COM_InitArgv(argc, argv);
    if((i = COM_CheckParm("-mem")) != 0 && i + 1 < com_argc) mb = Q_atoi(com_argv[i + 1]);
    parms.memsize = mb << 20;
    parms.membase = cmm2->alloc(parms.memsize);
    if(parms.membase == NULL) Sys_Error("no room for %d MB in MMBasic's heap", mb);
    parms.basedir = ".";
    parms.argc = com_argc;
    parms.argv = com_argv;
    Host_Init(&parms);
#ifdef PROFILE
    prof_base = cmm2_start->base;
    atexit(prof_write);
    cmm2->frame_hook(prof_tick);
#endif
    then = Sys_FloatTime();
    for(;;) {
        now = Sys_FloatTime();
        Host_Frame(now - then);
        then = now;
    }
}
