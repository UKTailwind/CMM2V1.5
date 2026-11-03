/***********************************************************************************************************************
Loadable programs (2 Nov 2026: Peter; a G1 too since 3 Nov). docs/Loader_Plan.md has the design, Inc/cmm2img.h the
file format and the services, tools/sdk the tools that build a program.

  RUN "name.app" [, words]   loads the program into its area, runs it, and goes on with the next statement when it ends
                             (typed at the prompt: back to the prompt). The words after the comma are its argv[1] on.

Its area: on a G2, the 8 MB at LOADER_BASE (Memory.h; MPU region 8 makes it executable and write-back cached). A G1 has
no SDRAM to keep for it, so it borrows the heap's largest free run (all but LD_G1_RESERVE of it, from a 64 KB boundary)
while the program runs: the image is moved there by its relocations, and LoaderExec makes the heap executable meanwhile.
From the prompt that is about 4 MB; from a BASIC program, what the program leaves. An image flagged G2 is refused there.

Only a name ending in .APP is looked at, so RUN of a BASIC program is as it was, and nothing of the BASIC program in
memory is touched.

The program runs on its own stack (the process stack, at the top of its area), so the interrupts keep the main stack in
DTCM, with its heap between its .bss and that stack. LoaderCall switches to it and calls the entry with the start block;
the program comes back by returning, or from any depth through leave(), which goes to LoaderReturn: that puts back the
main stack and the registers LoaderCall saved. The services never raise an MMBasic error (its longjmp would leave the
program's stack in use and its files open): each returns a status. The files, directories and heap blocks a program
leaves open are closed and freed when it ends. A fault in it restarts the board, as any fault does.

The loader is in bank 2 (the linker script's .bank2): it runs in the foreground only.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "cmm2img.h"
#include "OPLChip.h"
#include "Loader.h"
#include "PicoMode.h"

extern TIM_HandleTypeDef htim16;
extern volatile uint64_t uSecTimer;
extern volatile int MMAbort;
extern volatile int second, minute, hour, day, month, year;
extern char filepath[];
extern char runcmd[];                                               // RUN's text, to a comment (Commands.c)
extern volatile char ConsoleRxBuf[];
extern volatile int ConsoleRxBufHead, ConsoleRxBufTail;
extern UART_HandleTypeDef huart1, huart2, huart4;
extern void CheckKeyboard(void);
extern void SerialConsolePutC(int c);                               // the serial console alone (main.c)
extern volatile uint8_t pagesetdone;                               // set by the LTDC's interrupt at each frame (ltdc.c)
extern const int xres[], yres[];                                    // each mode's size (ltdc.c)
extern int KeyDown[7];                                              // the keys held (KeyboardMap.c)

#define LD_FILES  8
#define LD_DIRS   4
#define LD_BLOCKS 64
#define LD_PRI_FILL (15 << (8 - __NVIC_PRIO_BITS))                  // PendSV's priority (the sound's refill), as BASEPRI masks it
static FIL *ldfile[LD_FILES];                                       // the program's open files, NULL when free
static DIR *lddir[LD_DIRS];
static void *ldblock[LD_BLOCKS];                                    // its blocks of the MMBasic heap (alloc, opl_new)
static uint32_t ldmsp __attribute__((used));                        // the main stack inside LoaderCall, for LoaderReturn
static cmm2img_start ldstart;
#define LD_G1_RESERVE 0x40000                                       // a G1's heap left beside the program: the services'
static void *ldarea;                                                // blocks and malloc's arena; ldarea the run it borrows
int LoaderExec;                                                     // 1 while that run is executable (MPU_Config_nCacheable)
int LoaderVram;                                                     // 1 while the display memory holds the program's code (the
static int ldlent;                                                  // same); ldlent: a G1's display memory lent (CMM2IMG_FLAG_VRAM)

// the time in microseconds since start-up: TIM16 counts 0-49999 at 1 MHz and uSecTimer its wraps (CFunctions.c's
// timer()), both read until they agree, so a wrap between the two reads is not lost
uint64_t MicrosNow(void) {
    uint64_t hi;
    uint32_t lo;
    do {
        hi = uSecTimer;
        lo = __HAL_TIM_GET_COUNTER(&htim16);
    } while(hi != uSecTimer);
    return lo + hi * 50000;
}

/*******************************************************************************************
 the services
*******************************************************************************************/
static int ldscreen;                                                // the console's output drawn on the screen too (con_screen)
static void ld_putc(int c) { if(ldscreen) putConsole(c); else SerialConsolePutC(c); }
static void ld_puts(const char *s) { if(ldscreen) MMPrintString((char *)s); else while(*s) SerialConsolePutC(*s++); }
static void ld_conscreen(int on) { ldscreen = on != 0; }
// Ctrl-C (version 5): taken by the program, which handles it, so MMAbort is cleared and the BASIC program goes on
static int ld_breaktake(void) {
    int b = MMAbort != 0;
    MMAbort = 0;
    return b;
}
// getConsole's queue without its CheckAbort (main.c): Ctrl-C is poll()'s to report
static int ld_getc(void) {
    int c = -1;
    USART_TypeDef *u = Option.ConsolePort == 3 ? huart1.Instance : Option.ConsolePort == 1 ? huart2.Instance :
                       Option.ConsolePort == 2 ? huart4.Instance : NULL;
    if(u == NULL) return -1;
    u->CR1 &= ~USART_CR1_RXNEIE;
    if(ConsoleRxBufHead != ConsoleRxBufTail) {
        c = ConsoleRxBuf[ConsoleRxBufTail];
        ConsoleRxBufTail = (ConsoleRxBufTail + 1) % CONSOLE_RX_BUF_SIZE;
    }
    u->CR1 |= USART_CR1_RXNEIE;
    return c;
}
static void ld_delay(uint32_t us) {                                 // (not uSec: its 16-bit count ends early across
    uint64_t end = MicrosNow() + us;                                // TIM16's wrap at 50000)
    while(MicrosNow() < end) {}
}
static void ld_datetime(int t[6]) {
    t[0] = year; t[1] = month; t[2] = day; t[3] = hour; t[4] = minute; t[5] = second;
}

// a name as FatFs takes it: one with a drive kept, "/x" on the current drive, anything else after the current directory
static void ld_path(const char *name, char *out) {
    int n;
    if(name[0] && name[1] == ':') {
        out[0] = name[0]; out[1] = ':'; out[2] = 0;
        name += 2;
        if(*name != '/' && *name != '\\') strcat(out, "/");
    } else if(*name == '/' || *name == '\\') {
        out[0] = filepath[0]; out[1] = ':'; out[2] = 0;
    } else {
        strcpy(out, filepath);
        if(out[strlen(out) - 1] != '/') strcat(out, "/");
    }
    n = strlen(out);
    while(*name && n < FF_MAX_LFN) { out[n++] = (*name == '\\' ? '/' : *name); name++; }
    out[n] = 0;
}
static int ld_fail(FRESULT r) { return r == FR_OK ? 0 : -(int)r; }
static FIL *ld_fil(int h) { return (h >= 0 && h < LD_FILES) ? ldfile[h] : NULL; }
// a file opened as it is (read, or read and written in place) is given a map of its clusters (FatFs's fast seek): a
// seek, or the next cluster, is then found in the table, not by following the FAT from the file's start over the card's
// SPI each time (Doom seeks to each lump of a 28 MB WAD; the 386 PC to each sector of its disk images, 3 Nov 2026). A
// write or seek past the end drops it (ld_unmap: with it a file cannot grow). 64 entries first, then as many as FatFs
// says it needs; without room it is read as before
static void ld_map(FIL *f) {
    DWORD n = 64, *t;
    FRESULT r;
    while((t = GetInternalMemoryNull(n * sizeof(DWORD))) != NULL) {
        t[0] = n;
        f->cltbl = t;
        if((r = f_lseek(f, CREATE_LINKMAP)) == FR_OK) return;
        f->cltbl = NULL;
        n = t[0];
        FreeMemory(t);
        if(r != FR_NOT_ENOUGH_CORE) return;
    }
}
static void ld_unmap(FIL *f) {
    if(f->cltbl) { FreeMemory(f->cltbl); f->cltbl = NULL; }
}
static void ld_free(FIL *f) {
    if(f->cltbl) FreeMemory(f->cltbl);
    FreeMemory(f);
}

static int ld_open(const char *name, int mode) {
    char p[FF_MAX_LFN + 4];
    BYTE m = 0;
    FRESULT r;
    int h;
    for(h = 0; h < LD_FILES && ldfile[h]; h++) {}
    if(h == LD_FILES) return -FR_TOO_MANY_OPEN_FILES;
    FIL *f = GetInternalMemoryNull(sizeof(FIL));                    // (where BasicFileOpen keeps its own)
    if(f == NULL) return -FR_NOT_ENOUGH_CORE;
    if(mode & CMM2_O_READ) m |= FA_READ;
    if(mode & CMM2_O_WRITE) m |= FA_WRITE | ((mode & CMM2_O_APPEND) ? FA_OPEN_APPEND : (mode & CMM2_O_CREATE) ? FA_CREATE_ALWAYS : FA_OPEN_EXISTING);
    ld_path(name, p);
    if((r = f_open(f, p, m)) != FR_OK) {
        FreeMemory(f);
        return -(int)r;
    }
    if(!(m & (FA_CREATE_ALWAYS | FA_OPEN_APPEND))) ld_map(f);
    ldfile[h] = f;
    return h;
}
static int ld_close(int h) {
    FIL *f = ld_fil(h);
    if(f == NULL) return -FR_INVALID_OBJECT;
    FRESULT r = f_close(f);
    ld_free(f);
    ldfile[h] = NULL;
    return ld_fail(r);
}
static int32_t ld_read(int h, void *buf, uint32_t n) {
    UINT got = 0;
    FIL *f = ld_fil(h);
    if(f == NULL) return -FR_INVALID_OBJECT;
    FRESULT r = f_read(f, buf, n, &got);
    return r ? -(int)r : (int32_t)got;
}
static int32_t ld_write(int h, const void *buf, uint32_t n) {
    UINT put = 0;
    FIL *f = ld_fil(h);
    if(f == NULL) return -FR_INVALID_OBJECT;
    if(f_tell(f) + n > f_size(f)) ld_unmap(f);                     // (the file grows: the FAT is followed again)
    FRESULT r = f_write(f, buf, n, &put);
    return r ? -(int)r : (int32_t)put;
}
static int ld_seek(int h, uint32_t pos) {
    FIL *f = ld_fil(h);
    if(f == NULL) return -FR_INVALID_OBJECT;
    if(pos > f_size(f)) ld_unmap(f);                                // (past the end: as without the map)
    return ld_fail(f_lseek(f, pos));
}
static int32_t ld_tell(int h) { FIL *f = ld_fil(h); return f ? (int32_t)f_tell(f) : -FR_INVALID_OBJECT; }
static int32_t ld_size(int h) { FIL *f = ld_fil(h); return f ? (int32_t)f_size(f) : -FR_INVALID_OBJECT; }
static int ld_sync(int h) { FIL *f = ld_fil(h); return f ? ld_fail(f_sync(f)) : -FR_INVALID_OBJECT; }
static int ld_remove(const char *name) { char p[FF_MAX_LFN + 4]; ld_path(name, p); return ld_fail(f_unlink(p)); }
static int ld_rename(const char *from, const char *to) {
    char a[FF_MAX_LFN + 4], b[FF_MAX_LFN + 4];
    ld_path(from, a);
    ld_path(to, b);
    return ld_fail(f_rename(a, b));
}
// a file's or directory's details (version 5): FatFs's f_stat, which has no entry for a drive's root ("A:/")
static int ld_stat(const char *name, cmm2_stat *st) {
    char p[FF_MAX_LFN + 4];
    FILINFO fno;
    FRESULT r;
    ld_path(name, p);
    memset(st, 0, sizeof(*st));
    if(p[0] && p[1] == ':' && (p[2] == 0 || (p[2] == '/' && p[3] == 0))) {
        DIR d;
        if((r = f_opendir(&d, p)) != FR_OK) return -(int)r;
        f_closedir(&d);
        st->is_dir = 1;
        return 0;
    }
    if((r = f_stat(p, &fno)) != FR_OK) return -(int)r;
    st->is_dir = (fno.fattrib & AM_DIR) != 0;
    st->size = st->is_dir ? 0 : (uint32_t)fno.fsize;
    st->year = (fno.fdate >> 9) + 1980;
    st->month = (fno.fdate >> 5) & 15;
    st->day = fno.fdate & 31;
    st->hour = fno.ftime >> 11;
    st->minute = (fno.ftime >> 5) & 63;
    st->second = (fno.ftime & 31) * 2;
    return 0;
}
static int ld_mkdir(const char *name) { char p[FF_MAX_LFN + 4]; ld_path(name, p); return ld_fail(f_mkdir(p)); }
static int ld_opendir(const char *path) {
    char p[FF_MAX_LFN + 4];
    FRESULT r;
    int h;
    for(h = 0; h < LD_DIRS && lddir[h]; h++) {}
    if(h == LD_DIRS) return -FR_TOO_MANY_OPEN_FILES;
    DIR *d = GetInternalMemoryNull(sizeof(DIR));
    if(d == NULL) return -FR_NOT_ENOUGH_CORE;
    ld_path(path, p);
    if((r = f_opendir(d, p)) != FR_OK) {
        FreeMemory(d);
        return -(int)r;
    }
    lddir[h] = d;
    return h;
}
static int ld_readdir(int h, cmm2_dirent *e) {
    FILINFO fno;
    if(h < 0 || h >= LD_DIRS || lddir[h] == NULL) return -FR_INVALID_OBJECT;
    FRESULT r = f_readdir(lddir[h], &fno);
    if(r != FR_OK) return -(int)r;
    if(fno.fname[0] == 0) return 0;
    strncpy(e->name, fno.fname, sizeof(e->name) - 1);
    e->name[sizeof(e->name) - 1] = 0;
    e->size = (uint32_t)fno.fsize;
    e->is_dir = (fno.fattrib & AM_DIR) != 0;
    return 1;
}
static int ld_closedir(int h) {
    if(h < 0 || h >= LD_DIRS || lddir[h] == NULL) return -FR_INVALID_OBJECT;
    FRESULT r = f_closedir(lddir[h]);
    FreeMemory(lddir[h]);
    lddir[h] = NULL;
    return ld_fail(r);
}

// the USB devices and keyboards seen to as routinechecks() does, without its SD card and GUI checks (either can raise
// an error); 1 once Ctrl-C has been pressed. MMAbort is left set, so when the program ends the BASIC program that ran
// it stops as well, as at any Ctrl-C (clearing it here let a RUN in a loop go on to the next program, 2 Nov)
static int ld_poll(void) {
    if(USBtime) {
        mT4IntEnable(0);
        CheckKeyboard();
        USBtime = 0;
        mT4IntEnable(1);
    }
    return MMAbort != 0;
}
static uint32_t ld_cpuhz(void) { return SystemCoreClock; }
void *LoaderAlloc(uint32_t size) {
    int i;
    for(i = 0; i < LD_BLOCKS && ldblock[i]; i++) {}
    if(i == LD_BLOCKS) return NULL;
    return ldblock[i] = GetMemoryNull(size);
}
void LoaderRelease(void *p) {
    int i;
    for(i = 0; i < LD_BLOCKS; i++)
        if(p != NULL && ldblock[i] == p) {
            FreeMemory(p);
            ldblock[i] = NULL;
        }
}
// the firmware's OPL3 synth for a program (ymfm: OPLChip.cpp): its own chips, on the heap (PLAY OPL's is OPL.c's)
static void *ld_oplnew(uint32_t rate) {
    void *c = LoaderAlloc(OPLChipSize());
    if(c) OPLChipInit(c, rate);
    return c;
}
static void ld_oplwrite(void *chip, uint32_t reg, uint32_t value) { OPLChipWrite(chip, reg & 0x1FF, value); }
static void ld_oplgen(void *chip, int16_t *buf, uint32_t frames) { OPLChipGenerate(chip, buf, frames); }
static void ld_oplfree(void *chip) { if(chip) { OPLChipDone(chip); LoaderRelease(chip); } }

// the screen (version 2): MODE's checks made here, so setmode raises no error; the mode and palette BASIC had are put
// back by LoaderRun when the program ends
static int ldpalette;                                               // the program changed the palette
static void ld_frame(void) {                                       // the next frame's start, or 50 ms
    uint64_t end = MicrosNow() + 50000;
    pagesetdone = 0;
    while(!pagesetdone && MicrosNow() < end) {}
}
static int ld_dma2dbusy(void);
static int ld_vscaled(int mode, int depth, int scale, cmm2_video *v) {
    if(mode < 1 || mode > MAX_MODES || (depth != 8 && depth != 16) || (scale != 1 && scale != 2)) return -1;
    if(mode == 16 && !(G1Hardware == 0 && HAL_GetREVID() != 0x1003)) return -1;
    if(depth == 16 && (mode == 15 || mode == 16)) return -1;        // (MODE's rule: 8 and 1-bit only)
    if(scale == 2 && (mode == 3 || mode == 5 || mode == 6 || mode == 7 || mode == 12 || mode == 13 || mode == 17))
        return -1;                                                  // (their lines shown twice already: ltdc.c's ModeRowScale)
    if(VideoColour != depth && PageTable[WPN].address != NULL) return -2;   // (a FRAMEBUFFER of another depth)
    if(ldlent) {                                                    // the display memory lent: page 0 must be in the internal
        uint32_t bytes = (uint32_t)(xres[mode] / scale) * (yres[mode] / scale) * (depth / 8);   // RAM (pagegen puts it there
        if(scale == 2 || ModeRowScale(mode) == 2) bytes *= 2;       // while it fits), not in the 3 MB the program has
        if(bytes > 0x80000) return -1;
    }
    uint32_t old = __get_BASEPRI();
    __set_BASEPRI(LD_PRI_FILL);                                     // (no refill or frame hook: setmode turns the MPU off for
    while(ld_dma2dbusy()) {}                                        // a moment, when the program's code in SDRAM can't be
    if(scale > 1) {                                                 // fetched; and no copy into the pages as they are laid
        ModeReqW = xres[mode] / scale;                              // out again). The smaller display is made as OPTION
        ModeReqH = yres[mode] / scale;                              // PICOMITE makes its modes (PicoMode.c): the pixel clock
        ModeReqHs = ModeReqVs = scale;                              // divided, each row shown twice, the pages holding it twice
    }
    setmode(mode, depth, 0, 0);
    __set_BASEPRI(old);
    v->fb = PageTable[0].address;
    v->width = PageTable[0].xmax;
    v->height = PageTable[0].ymax;
    v->copies = PageTable[0].expand ? 2 : 1;                        // (a doubled mode's page holds each line twice: Draw.c)
    v->bpp = depth;
    v->pitch = v->width * (depth / 8) * v->copies;
    memset(v->fb, 0, v->pitch * v->height);                         // (the same mode again is not cleared by setmode)
    return 0;
}
static int ld_vmode(int mode, int depth, cmm2_video *v) { return ld_vscaled(mode, depth, 1, v); }
static int ld_palette(int start, int count, const uint8_t *rgb) {
    int i;
    if(VideoColour != 8 || start < 0 || count < 0 || start + count > 256) return -1;
    for(i = 0; i < count; i++, rgb += 3) CLUT[start + i] = (uint32_t)rgb[0] << 16 | rgb[1] << 8 | rgb[2];
    ld_frame();
    HAL_LTDC_ConfigCLUT(&hltdc, CLUT, 256, 0);
    ldpalette = 1;
    return 0;
}
/* the sound stream (version 3): each buffer made by the program's fill in PendSV, at the lowest priority, which the
   sample interrupt pends as it changes buffer (stm32h7xx_it.c, MOD's branch); the program's music added (OPL.c), then
   PLAY VOLUME's level and the DACs' scale, as MOD's buffers are made. The refill is held off (BASEPRI at PendSV's
   level) while the program or a service changes what it uses */
#define LD_FRAMES   1024                                            // a buffer: 23 ms
extern TIM_HandleTypeDef htim4;
extern volatile int swingbuf, nextbuf, ppos, wav_filesize;
extern uint16_t *ibuff1, *ibuff2;
extern int nchannels;
extern void i2convert(int16_t *ibuff, int16_t *sbuff, int count);
static void (*ldfill)(int16_t *buf, uint32_t frames);

static void ld_make(int b) {
    int16_t *s = (int16_t *)(b == 1 ? sbuff1 : sbuff2);
    ldfill(s, LD_FRAMES);
    OPLStreamMix(s, LD_FRAMES);
    i2convert(s, s, LD_FRAMES * 2);
    bcount[b] = LD_FRAMES * 2;                                      // (last: the interrupt takes it once counted)
}
void LoaderPendSV(void) {                                           // the buffer the interrupt is not playing, if empty
    int b = swingbuf == 1 ? 2 : 1;
    if(CurrentlyPlaying == P_STREAM && ldfill && bcount[b] == 0) ld_make(b);
    if(LoaderFramePend) {                                           // and a frame's start: the program's hook (version 6)
        void (*fn)(void) = LoaderFrameFn;
        LoaderFramePend = 0;
        if(fn) fn();
    }
}
static void ld_alock(void) { __set_BASEPRI(LD_PRI_FILL); __ISB(); }
static void ld_aunlock(void) { __set_BASEPRI(0); }
static void ld_astop(void) {
    uint32_t old = __get_BASEPRI();
    __set_BASEPRI(LD_PRI_FILL);
    if(CurrentlyPlaying == P_STREAM) CloseAudio(1);                 // the timer stopped, the buffers and the synth freed
    ldfill = NULL;
    __set_BASEPRI(old);
}
static int ld_astart(void (*fill)(int16_t *buf, uint32_t frames)) {
    if(fill == NULL) return -1;
    if(CurrentlyPlaying != P_NOTHING) return -2;
    sbuff1 = GetInternalMemoryNull(LD_FRAMES * 4);
    sbuff2 = GetInternalMemoryNull(LD_FRAMES * 4);
    if(!sbuff1 || !sbuff2) { FreeMemorySafe((void **)&sbuff1); FreeMemorySafe((void **)&sbuff2); return -3; }
    ibuff1 = (uint16_t *)sbuff1;
    ibuff2 = (uint16_t *)sbuff2;
    htim4.Init.Prescaler = (PeripheralBusSpeed / 2) / 44100 - 1;    // (as PLAY OPL sets it: OPLRate is what it gives)
    htim4.Instance->PSC = htim4.Init.Prescaler;
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);                       // (CubeMX left it at 0, the highest: it was unused)
    ldfill = fill;
    playreadcomplete = 0;
    nchannels = 2;
    wav_filesize = LD_FRAMES * 2;
    ppos = 0;
    bcount[1] = bcount[2] = 0;
    swingbuf = 1;
    nextbuf = 2;
    ld_make(1);
    CurrentlyPlaying = P_STREAM;
    if(HAL_TIM_Base_Start_IT(&htim4) != HAL_OK) { ld_astop(); return -4; }
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;                             // (the second buffer)
    return (int)OPLRate();
}
static int ld_mplay(const void *midi, uint32_t size, int loop, const uint8_t *bank) {
    uint32_t old = __get_BASEPRI();
    int r;
    __set_BASEPRI(LD_PRI_FILL);
    r = OPLStreamPlay(midi, size, loop, bank, LD_FRAMES);
    __set_BASEPRI(old);
    return r;
}

static void ld_mrate(int n) {
    uint32_t old = __get_BASEPRI();
    __set_BASEPRI(LD_PRI_FILL);                                     // (not while the refill is making the music)
    OPLStreamDivide(n);
    __set_BASEPRI(old);
}

// the mouse (version 4): MM.MOUSE's, its state the driver's (USBMouse.c, mouse.c), in the screen's pixels
extern volatile struct s_nunstruct mousestruct[4];
static int ld_mouse(int m[4]) {
    int c = MouseOpenChannel();
    if(c < 0) { m[0] = m[1] = m[2] = m[3] = 0; return 0; }
    m[0] = mousestruct[c].ax;
    m[1] = mousestruct[c].ay;
    m[2] = mousestruct[c].x0;
    m[3] = mousestruct[c].az;
    return 1;
}
static void ld_mouseset(int x, int y) {
    int c = MouseOpenChannel();
    if(c < 0) return;
    mousestruct[c].ax = x < 0 ? 0 : x >= HRes ? HRes - 1 : x;
    mousestruct[c].ay = y < 0 ? 0 : y >= VRes ? VRes - 1 : y;
}

extern uint8_t KeyRaw[8];                                           // the keyboard's last report (USBKeyboard.c)
static int ld_keysraw(uint8_t r[8]) {                              // (version 9)
    int i, n = 0;
    memcpy(r, KeyRaw, 8);
    for(i = 2; i < 8; i++) if(r[i] >= 4) n++;                     // (1-3 are a keyboard's errors, not keys)
    return n;
}
static int ld_keys(int keys[7]) {
    int i, n = 0;
    for(i = 0; i < 7; i++) keys[i] = KeyDown[i];
    for(i = 0; i < 6; i++) if(keys[i]) n++;
    return n;
}

/* a display in the program's memory (version 6): its frame hook called in PendSV after the LTDC's interrupt at the start
   of each frame's blanking (ltdc.c sees LoaderFrameFn and pends it: this bank may not be fetched from there), and the
   DMA2D's copies, which it may queue there. Up to 4 wait; the DMA2D's interrupt starts each after the one before. The
   firmware leaves the DMA2D alone (CubeMX's init is not called), so it is the program's */
void (*volatile LoaderFrameFn)(void);
volatile int LoaderFramePend;
static void ld_framehook(void (*fn)(void)) {
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);                       // (below every other, as for the sound)
    LoaderFramePend = 0;
    LoaderFrameFn = fn;
}

#define LD_JOBS 4
static struct { uint32_t src, dst, fgor, oor, nlr, cm; } ldjob[LD_JOBS];
static volatile int ldjobin, ldjobout;                              // the queue: the next free, and the one running
static void ld_dma2dgo(int j) {
    DMA2D->FGMAR = ldjob[j].src;
    DMA2D->OMAR = ldjob[j].dst;
    DMA2D->FGOR = ldjob[j].fgor;
    DMA2D->OOR = ldjob[j].oor;
    DMA2D->NLR = ldjob[j].nlr;
    DMA2D->FGPFCCR = ldjob[j].cm;
    DMA2D->OPFCCR = ldjob[j].cm;
    DMA2D->CR = DMA2D_CR_TCIE | DMA2D_CR_TEIE | DMA2D_CR_CEIE | DMA2D_CR_START;   // (memory to memory: MODE 0)
}
void LoaderDMA2D(void) {                                            // the DMA2D's interrupt: a copy done, the next started
    DMA2D->IFCR = DMA2D_IFCR_CTCIF | DMA2D_IFCR_CTEIF | DMA2D_IFCR_CCEIF;
    if(ldjobout == ldjobin) return;
    ldjobout = (ldjobout + 1) % LD_JOBS;
    if(ldjobout != ldjobin) ld_dma2dgo(ldjobout);
}
static int ld_dma2dbusy(void) { return (ldjobin - ldjobout + LD_JOBS) % LD_JOBS; }
static int ld_dma2dcopy(const void *src, uint32_t sp, void *dst, uint32_t dp, uint32_t w, uint32_t h) {
    uint32_t s = (uint32_t)src, d = (uint32_t)dst, px = ((s | d | sp | dp | w) & 3) ? 2 : 4, a, next;
    if(((s | d | sp | dp | w) & 1) || sp < w || dp < w || w / px > 0x3FFF || (sp - w) / px > 0x3FFF ||
       (dp - w) / px > 0x3FFF || h > 0xFFFF) return -1;
    if(w == 0 || h == 0) return 0;
    a = s & ~31u;                                                   // the source from the cache to the SDRAM, which the DMA2D reads
    SCB_CleanDCache_by_Addr((uint32_t *)a, (int32_t)((s - a + sp * (h - 1) + w + 31) & ~31u));
    if(!(RCC->AHB3ENR & RCC_AHB3ENR_DMA2DEN)) {
        __HAL_RCC_DMA2D_CLK_ENABLE();
        HAL_NVIC_SetPriority(DMA2D_IRQn, 14, 0);                    // (above the frame hook's PendSV, which may wait for it)
        HAL_NVIC_EnableIRQ(DMA2D_IRQn);
    }
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    next = (ldjobin + 1) % LD_JOBS;
    if(next == ldjobout) { __set_PRIMASK(primask); return -1; }     // (4 waiting)
    ldjob[ldjobin].src = s;
    ldjob[ldjobin].dst = d;
    ldjob[ldjobin].fgor = (sp - w) / px;
    ldjob[ldjobin].oor = (dp - w) / px;
    ldjob[ldjobin].nlr = (w / px) << DMA2D_NLR_PL_Pos | h;
    ldjob[ldjobin].cm = px == 4 ? 0 : 2;                            // ARGB8888 or RGB565: only its size matters, unconverted
    int idle = ldjobin == ldjobout;
    ldjobin = next;
    if(idle) ld_dma2dgo(ldjobout);
    __set_PRIMASK(primask);
    return 0;
}
static void ld_dma2dstop(void) {                                    // (the program's end: its copies dropped)
    if(!(RCC->AHB3ENR & RCC_AHB3ENR_DMA2DEN)) return;
    HAL_NVIC_DisableIRQ(DMA2D_IRQn);
    if(DMA2D->CR & DMA2D_CR_START) {
        DMA2D->CR |= DMA2D_CR_ABORT;
        uint64_t end = MicrosNow() + 10000;
        while((DMA2D->CR & DMA2D_CR_START) && MicrosNow() < end) {}
    }
    DMA2D->IFCR = DMA2D_IFCR_CTCIF | DMA2D_IFCR_CTEIF | DMA2D_IFCR_CCEIF;
    ldjobin = ldjobout = 0;
    __HAL_RCC_DMA2D_CLK_DISABLE();
}

/* I/O (version 7), by the PicoMite's GP numbers (External.c's map). The firmware's own routines do the work: ExtCfg,
   PinAnalogRaw, cmd_pwm, i2cEnable, I2CTransfer, cmd_spi1, SerialOpen... An MMBasic error one of them raises is caught
   by ld_try as ON ERROR SKIP catches it, so nothing is printed and MM.ERRNO and a SUB the BASIC program is in are left
   alone; its message is kept for io_error(). What the program set up is undone when it ends (ld_iotidy). */
extern jmp_buf ErrNext;
extern uint32_t DefinedSubFunMem;
extern const MMFLOAT ADCdiv[];
extern SPI_HandleTypeDef hspi1, hspi2;
extern unsigned int I2C1_enabled, I2C2_enabled;
extern void cmd_spi1(char *spi), cmd_spi2(char *spi);
extern void i2cEnable(char *p), i2c2Enable(char *p), i2cDisable(char *p), i2c2Disable(char *p);
static char lderrmsg[128];
static uint64_t ldpins;                                             // the GPs the program set up
static uint8_t ldpwm, ldi2c, ldspi, ldcom;                          // its PWM outputs (bit n: GP2+n), buses and COM ports
static uint32_t ldpwmfreq[2];                                       // its PWM channels' frequencies

static int ld_try(void (*fn)(void *), void *arg) {
    jmp_buf save;
    int skip = OptionErrorSkip, en = MMerrno;
    uint32_t sub = DefinedSubFunMem;
    char *msg = MMErrMsg;
    volatile int failed = 0;
    memcpy(save, ErrNext, sizeof(jmp_buf));
    OptionErrorSkip = 1;                                            // (error() then goes to ErrNext, printing nothing,
    DefinedSubFunMem = 0;                                           // and leaves a SUB call's arguments alone)
    if(setjmp(ErrNext) == 0) fn(arg);
    else {
        failed = 1;
        strncpy(lderrmsg, MMErrMsg ? MMErrMsg : "", sizeof(lderrmsg) - 1);
    }
    memcpy(ErrNext, save, sizeof(jmp_buf));
    OptionErrorSkip = skip;
    MMerrno = en;
    DefinedSubFunMem = sub;
    MMErrMsg = msg;
    return failed ? -4 : 0;
}
static const char *ld_ioerror(void) { return lderrmsg; }

// a command's routine run with its arguments as text (numbers and words only), as the interpreter runs it
typedef struct { void (*cmd)(void); void (*cmdp)(char *); int token; char *args; } ldcmd;
static void ld_cmdbody(void *a) {
    ldcmd *c = a;
    if(c->cmdp) { c->cmdp(c->args); return; }
    cmdline = c->args;
    cmdtoken = c->token;
    c->cmd();
}
static int ld_cmd(void (*cmd)(void), void (*cmdp)(char *), const char *name, char *args) {
    char *line = cmdline;
    int tok = cmdtoken, r;
    ldcmd c = {cmd, cmdp, name ? GetCommandValue((char *)name) : 0, args};
    r = ld_try(ld_cmdbody, &c);
    cmdline = line;
    cmdtoken = tok;
    return r;
}

typedef struct { int pin, cfg, opt; } ldcfg;
static void ld_cfgbody(void *a) { ldcfg *c = a; ExtCfg(c->pin, c->cfg, c->opt); }
static int ld_pinsetup(int gp, int mode, int pull) {
    static const int cfgs[] = {EXT_NOT_CONFIG, EXT_DIG_IN, EXT_DIG_OUT, EXT_OC_OUT, EXT_ANA_IN};
    static const int need[] = {0, DIGITAL_IN, DIGITAL_OUT, DIGITAL_OUT, ANALOG_IN};
    int pin = GPPinNone(gp), r;
    ldcfg c;
    if(!pin) return -1;
    if(mode < CMM2_PIN_OFF || mode > CMM2_PIN_ANALOG) return -3;
    if(ExtCurrentConfig[pin] >= EXT_COM_RESERVED) return -2;
    if(need[mode] && !(PinDef[pin].mode & need[mode])) return -3;
    c.pin = pin;
    c.cfg = cfgs[mode];
    c.opt = mode == CMM2_PIN_IN ? (pull == CMM2_PULL_UP ? CNPUSET : pull == CMM2_PULL_DOWN ? CNPDSET : 0) :
            mode == CMM2_PIN_ANALOG ? 16 : 0;                       // (16 bits: SETPIN AIN's)
    r = ld_try(ld_cfgbody, &c);
    if(r == 0) {
        if(mode == CMM2_PIN_OFF) ldpins &= ~(1ULL << gp);
        else ldpins |= 1ULL << gp;
    }
    return r;
}
static int ld_pinread(int gp) {
    int pin = GPPinNone(gp);
    if(!pin) return -1;
    return (PinDef[pin].sfr->IDR & PinDef[pin].bitnbr) != 0;
}
static void ld_pinwrite(int gp, int v) {
    int pin = GPPinNone(gp), c;
    if(!pin) return;
    c = ExtCurrentConfig[pin];
    if(c == EXT_DIG_OUT || c == EXT_OC_OUT) PinDef[pin].sfr->BSRR = PinDef[pin].bitnbr << (v ? 0 : 16);
}

typedef struct { int pin; uint32_t v; } ldadc;
static void ld_adcbody(void *a) { ldadc *c = a; c->v = PinAnalogRaw(c->pin); }
static int ld_adcread(int gp, uint32_t *uv) {
    int pin = GPPinNone(gp), r;
    ldadc c = {pin, 0};
    double f;
    if(!pin) return -1;
    if(ExtCurrentConfig[pin] != EXT_ANA_IN) return -3;
    if((r = ld_try(ld_adcbody, &c)) < 0) return r;
    f = (double)c.v / ADCdiv[ADCbits[pin]];
    if(f > 1.0) f = 1.0;
    if(uv) *uv = (uint32_t)(f * VCC * 1e6 + 0.5);
    return (int)(f * 65535.0 + 0.5);
}

/* PWM: GP2-5 are PWM 1A, 1B, 2A, 2B (PWM channel, f, dutyA [, dutyB]: an empty duty leaves that output as it is).
   The channel's other output, if it runs, is given its duty again, as its count would otherwise stay as it was for
   the old frequency (PWM's own way with an output it is not given) */
static uint32_t ldpwmduty[4];                                       // each output's duty, in hundredths of a percent
static void ld_pct(char *s, uint32_t pct) { sprintf(s, "%u.%02u", (unsigned)(pct / 100), (unsigned)(pct % 100)); }
static int ld_pwmset(int gp, uint32_t freq, uint32_t duty) {
    char a[64], da[16] = "", db[16] = "";
    int ch = (gp - 2) / 2, out = gp - 2, r;
    if(gp < 2 || gp > 5) return -1;
    if(freq < 1 || freq > 20000000) return -3;
    ldpwmduty[out] = (uint32_t)(((uint64_t)(duty > 65535 ? 65535 : duty) * 10000 + 32767) / 65535);
    ld_pct((out & 1) ? db : da, ldpwmduty[out]);
    if(ldpwm & (1 << (out ^ 1))) ld_pct((out & 1) ? da : db, ldpwmduty[out ^ 1]);   // (the other output, running)
    if(*db) sprintf(a, "%d,%u,%s,%s", ch + 1, (unsigned)freq, da, db);   // (A may be empty: B alone, as PWM 1, f, , dB)
    else sprintf(a, "%d,%u,%s", ch + 1, (unsigned)freq, da);
    r = ld_cmd(cmd_pwm, NULL, "PWM", a);
    if(r == 0) {
        ldpwm |= 1 << out;
        ldpwmfreq[ch] = freq;
    }
    return r;
}
static void ld_pwmclosebody(void *a) { PWMClose(*(int *)a); }
static int ld_pwmstop(int gp) {
    char a[32];
    int ch = (gp - 2) / 2;
    if(gp < 2 || gp > 5) return -1;
    if(!(ldpwm & (1 << (gp - 2)))) return 0;
    ldpwm &= ~(1 << (gp - 2));
    if(ldpwm & (3 << (ch * 2))) {                                   // the channel's other output runs: this one at 0%
        sprintf(a, (gp - 2) & 1 ? "%d,%u,,0" : "%d,%u,0", ch + 1, (unsigned)ldpwmfreq[ch]);
        return ld_cmd(cmd_pwm, NULL, "PWM", a);
    }
    return ld_try(ld_pwmclosebody, &ch);
}

// I2C: bus 0 is I2C (GP16/17), 1 is I2C2 (GP14/15)
static int ld_i2cclose(int bus) {
    if(bus < 0 || bus > 1) return -1;
    if(!(ldi2c & (1 << bus))) return 0;
    ldi2c &= ~(1 << bus);
    return ld_cmd(NULL, bus ? i2c2Disable : i2cDisable, NULL, "");
}
static int ld_i2copen(int bus, int khz, int timeout) {
    char a[32];
    int r;
    if(bus < 0 || bus > 1) return -1;
    ld_i2cclose(bus);
    sprintf(a, "%d,%d", khz, timeout);
    r = ld_cmd(NULL, bus ? i2c2Enable : i2cEnable, NULL, a);
    if(r == 0) ldi2c |= 1 << bus;
    return r;
}
static int ld_i2cxfer(int bus, int addr, const uint8_t *w, uint32_t wn, uint8_t *r, uint32_t rn, int stop) {
    if(bus < 0 || bus > 1) return -1;
    if(!(bus ? I2C2_enabled : I2C1_enabled)) return -3;
    if(addr < 0 || addr > 127 || wn > 65535 || rn > 65535) return -3;
    return I2CTransfer(bus + 1, addr, w, wn, r, rn, !stop);
}

// SPI: bus 0 is SPI (GP18-20), 1 is SPI2 (GP10-12), 8 bits
static int ld_spiclose(int bus) {
    if(bus < 0 || bus > 1) return -1;
    if(!(ldspi & (1 << bus))) return 0;
    ldspi &= ~(1 << bus);
    return ld_cmd(NULL, bus ? cmd_spi2 : cmd_spi1, NULL, "CLOSE");
}
static int ld_spiopen(int bus, uint32_t hz, int mode) {
    char a[48];
    int r;
    if(bus < 0 || bus > 1) return -1;
    ld_spiclose(bus);
    sprintf(a, "OPEN %u,%d,8", (unsigned)hz, mode & 3);
    r = ld_cmd(NULL, bus ? cmd_spi2 : cmd_spi1, NULL, a);
    if(r == 0) ldspi |= 1 << bus;
    return r;
}
static int ld_spixfer(int bus, const uint8_t *tx, uint8_t *rx, uint32_t n) {
    static uint8_t zero[64], junk[64];
    SPI_HandleTypeDef *h = bus ? &hspi2 : &hspi1;
    if(bus < 0 || bus > 1) return -1;
    if(!(ldspi & (1 << bus)) || ExtCurrentConfig[bus ? SPI2_OUT_PIN : SPI_OUT_PIN] != EXT_COM_RESERVED) return -3;
    while(n) {
        uint32_t k = n > 32768 ? 32768 : n;
        if(!tx || !rx) k = k > sizeof(zero) ? sizeof(zero) : k;
        if(HAL_SPI_TransmitReceive(h, (uint8_t *)(tx ? tx : zero), rx ? rx : junk, k, 5000) != HAL_OK) return -3;
        if(tx) tx += k;
        if(rx) rx += k;
        n -= k;
    }
    return 0;
}

// COM1 (GP0/1) and COM2 (GP36/37), opened as OPEN "COMn:baud,buffer[,EVEN|ODD][,S2][,7BIT]" opens them
static int ld_comclose(int com) {
    if(com < 1 || com > 2) return -1;
    if(!(ldcom & (1 << com))) return 0;
    ldcom &= ~(1 << com);
    SerialClose(com);
    return 0;
}
static int ld_comopen(int com, uint32_t baud, int bits, int parity, int stop, uint32_t rxbuf) {
    char a[64];
    int r;
    if(com < 1 || com > 2) return -1;
    ld_comclose(com);
    sprintf(a, "COM%d:%u,%u%s%s%s", com, (unsigned)baud, (unsigned)(rxbuf ? rxbuf : COM_DEFAULT_BUF_SIZE),
            parity == 1 ? ",EVEN" : parity == 2 ? ",ODD" : "", stop == 2 ? ",S2" : "", bits == 7 ? ",7BIT" : "");
    r = ld_cmd(NULL, SerialOpen, NULL, a);
    if(r == 0) ldcom |= 1 << com;
    return r;
}
static int ld_comwrite(int com, const uint8_t *buf, uint32_t n) {
    uint32_t i;
    if(com < 1 || com > 2) return -1;
    if(!(ldcom & (1 << com))) return -3;
    for(i = 0; i < n; i++) SerialPutchar(com, buf[i]);
    return (int)n;
}
static int ld_comread(int com, uint8_t *buf, uint32_t n) {
    uint32_t i = 0;
    int c;
    if(com < 1 || com > 2) return -1;
    if(!(ldcom & (1 << com))) return -3;
    while(i < n && (c = SerialGetchar(com)) >= 0) buf[i++] = c;
    return (int)i;
}
static int ld_comavail(int com) {
    if(com < 1 || com > 2) return -1;
    if(!(ldcom & (1 << com))) return -3;
    return SerialRxStatus(com);
}

// what the program set up, undone (its end)
static void ld_iotidy(void) {
    int gp, ch;
    for(ch = 0; ch < 2; ch++) if(ldpwm & (3 << (ch * 2))) ld_try(ld_pwmclosebody, &ch);
    ldpwm = 0;
    ld_i2cclose(0);
    ld_i2cclose(1);
    ld_spiclose(0);
    ld_spiclose(1);
    ld_comclose(1);
    ld_comclose(2);
    for(gp = 0; gp < 64; gp++)
        if(ldpins & (1ULL << gp)) {
            int pin = GPPinNone(gp);
            if(pin && ExtCurrentConfig[pin] < EXT_COM_RESERVED) {
                ldcfg c = {pin, EXT_NOT_CONFIG, 0};
                ld_try(ld_cfgbody, &c);
            }
        }
    ldpins = 0;
}

/*******************************************************************************************
 calling the program on its own stack
*******************************************************************************************/
// LoaderCall(entry, start, psp): the registers a call must keep and d8-d15 saved on the main stack, which is noted in
// ldmsp; then the process stack set to psp and taken (CONTROL.SPSEL), and entry(start) called. Its return, or a jump
// to LoaderReturn from leave(), comes back with r0 the code: the main stack again, the registers restored.
__attribute__((naked)) static int LoaderCall(void *entry, cmm2img_start *start, uint32_t psp) {
    __asm volatile(
        "push   {r4-r11, lr}        \n"
        "vpush  {d8-d15}            \n"
        "sub    sp, sp, #4          \n"     /* (36 + 64 + 4 bytes: the main stack stays 8-byte aligned) */
        "ldr    r3, =ldmsp          \n"
        "str    sp, [r3]            \n"
        "msr    psp, r2             \n"
        "mrs    r3, control         \n"
        "orr    r3, r3, #2          \n"
        "msr    control, r3         \n"
        "isb                        \n"
        "mov    r3, r0              \n"
        "mov    r0, r1              \n"
        "blx    r3                  \n"
        ".global LoaderReturn       \n"
        "LoaderReturn:              \n"
        "mrs    r3, control         \n"
        "bic    r3, r3, #2          \n"
        "msr    control, r3         \n"
        "isb                        \n"
        "ldr    r3, =ldmsp          \n"
        "ldr    sp, [r3]            \n"
        "add    sp, sp, #4          \n"
        "vpop   {d8-d15}            \n"
        "pop    {r4-r11, pc}        \n"
        ".ltorg                     \n"
    );
}
__attribute__((naked, noreturn)) static void ld_leave(int code) {
    __asm volatile("b LoaderReturn");                               // (r0 is already the code)
}

static const struct cmm2_services ldsvc = {
    .version = CMM2IMG_API_VERSION, .size = sizeof(struct cmm2_services),
    .con_putc = ld_putc, .con_puts = ld_puts, .con_getc = ld_getc,
    .micros = MicrosNow, .delay_us = ld_delay, .datetime = ld_datetime,
    .file_open = ld_open, .file_close = ld_close, .file_read = ld_read, .file_write = ld_write, .file_seek = ld_seek,
    .file_tell = ld_tell, .file_size = ld_size, .file_sync = ld_sync, .file_remove = ld_remove,
    .file_rename = ld_rename, .dir_make = ld_mkdir, .dir_open = ld_opendir, .dir_read = ld_readdir,
    .dir_close = ld_closedir,
    .poll = ld_poll, .leave = ld_leave, .cpu_hz = ld_cpuhz, .alloc = LoaderAlloc, .release = LoaderRelease,
    .opl_new = ld_oplnew, .opl_write = ld_oplwrite, .opl_generate = ld_oplgen, .opl_free = ld_oplfree,
    .video_mode = ld_vmode, .video_palette = ld_palette, .video_wait = ld_frame, .keys_held = ld_keys,
    .audio_start = ld_astart, .audio_stop = ld_astop, .audio_lock = ld_alock, .audio_unlock = ld_aunlock,
    .music_play = ld_mplay, .music_stop = OPLStreamStop, .music_volume = OPLStreamVolume, .music_pause = OPLStreamPause,
    .music_playing = OPLStreamPlaying, .music_rate = ld_mrate,
    .mouse = ld_mouse, .mouse_set = ld_mouseset, .con_screen = ld_conscreen,
    .break_take = ld_breaktake, .file_stat = ld_stat,
    .video_scaled = ld_vscaled, .frame_hook = ld_framehook, .dma2d_copy = ld_dma2dcopy, .dma2d_busy = ld_dma2dbusy,
    .pin_setup = ld_pinsetup, .pin_read = ld_pinread, .pin_write = ld_pinwrite, .adc_read = ld_adcread,
    .pwm_set = ld_pwmset, .pwm_stop = ld_pwmstop, .i2c_open = ld_i2copen, .i2c_close = ld_i2cclose,
    .i2c_xfer = ld_i2cxfer, .spi_open = ld_spiopen, .spi_close = ld_spiclose, .spi_xfer = ld_spixfer,
    .com_open = ld_comopen, .com_close = ld_comclose, .com_write = ld_comwrite, .com_read = ld_comread,
    .com_avail = ld_comavail, .io_error = ld_ioerror,
    .keys_raw = ld_keysraw,
};

/*******************************************************************************************
 RUN "name.app"
*******************************************************************************************/
// zlib's CRC-32, from a table made in temporary memory
static uint32_t ld_crc(uint32_t *table, uint32_t crc, const unsigned char *p, uint32_t n) {
    crc = ~crc;
    while(n--) crc = table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

// argv: the file's name, then the words of text (a word in quotes may hold spaces), in one heap block
static char **ld_args(char *fname, char *text, int *argc) {
    int n = 1, len = strlen(fname) + 1;
    char *p, *s, **argv;
    for(p = text; ; ) {
        while(*p == ' ') p++;
        if(*p == 0) break;
        n++;
        if(*p == '"') { p++; while(*p && *p != '"') { p++; len++; } if(*p) p++; }
        else while(*p && *p != ' ') { p++; len++; }
        len++;
    }
    argv = GetMemory((n + 1) * sizeof(char *) + len);
    s = (char *)(argv + n + 1);
    argv[0] = s;
    strcpy(s, fname);
    s += strlen(fname) + 1;
    for(n = 1, p = text; ; ) {
        while(*p == ' ') p++;
        if(*p == 0) break;
        argv[n++] = s;
        if(*p == '"') { p++; while(*p && *p != '"') *s++ = *p++; if(*p) p++; }
        else while(*p && *p != ' ') *s++ = *p++;
        *s++ = 0;
    }
    argv[n] = NULL;
    *argc = n;
    return argv;
}

// what a program left open, closed and freed
static void ld_tidy(void) {
    int i;
    LoaderFrameFn = NULL;                                           // its frame hook and its copies (before its display goes)
    ld_dma2dstop();
    ld_iotidy();                                                    // its pins, PWM, buses and ports
    ld_astop();                                                     // its sound, and its music's synth (before its blocks)
    OPLClose();
    __set_BASEPRI(0);                                               // (audio_lock left on)
    for(i = 0; i < LD_FILES; i++) if(ldfile[i]) { f_close(ldfile[i]); ld_free(ldfile[i]); ldfile[i] = NULL; }
    for(i = 0; i < LD_DIRS; i++) if(lddir[i]) { f_closedir(lddir[i]); FreeMemory(lddir[i]); lddir[i] = NULL; }
    for(i = 0; i < LD_BLOCKS; i++) if(ldblock[i]) { FreeMemory(ldblock[i]); ldblock[i] = NULL; }
}

// BASIC's screen, kept before a program runs (or the display memory is lent) and put back after: a mode change clears
// it, but its mode, font, colours and palette (MAP's) are as they were
static int ldsaved, ldoldmode, ldoldcolour, ldoldpico, ldoldfont, ldoldfc, ldoldbc;
static uint32_t ldoldclut[256];
static void ld_screensave(void) {
    ldoldmode = VideoMode; ldoldcolour = VideoColour; ldoldpico = PicoActive;
    ldoldfont = gui_font; ldoldfc = gui_fcolour; ldoldbc = gui_bcolour;
    memcpy(ldoldclut, CLUT, sizeof(ldoldclut));
    ldpalette = 0;
    ldsaved = 1;
}
static void ld_screenback(void) {
    int changed;
    if(!ldsaved) return;
    ldsaved = 0;
    changed = VideoMode != ldoldmode || VideoColour != ldoldcolour || PicoActive != ldoldpico;
    if(changed) {
        if(ldoldpico) PicoModeSet(PicoModeNow);                     // (OPTION PICOMITE's window: its MODE again)
        else setmode(ldoldmode, ldoldcolour, 0, 0);
        SetFont(ldoldfont);
        gui_fcolour = ldoldfc;
        gui_bcolour = ldoldbc;
    }
    if(ldoldcolour == 8 && (changed || ldpalette)) {
        memcpy(CLUT, ldoldclut, sizeof(CLUT));
        ld_frame();
        HAL_LTDC_ConfigCLUT(&hltdc, CLUT, 256, 0);
    }
}

// a G1's borrowed run given back (after ld_tidy, or when a load fails): the heap no longer executable, the run freed, and
// the low mark malloc's arena may grow to put back to where it was before the load, or to the lowest block still in use.
// The display memory, if lent: cleared (MMBasic's pages, as after a MODE) and written out of the cache, execute-never
// again (Peter: those regions are set for the fastest reads and writes), and BASIC's screen put back
static void ld_giveback(void *oldmin) {
    LoaderNoSbrk = 0;
    if(ldlent) {
        ldlent = 0;
        memset((void *)LOADER_VRAM_BASE, 0, LOADER_VRAM_SIZE);
        SCB_CleanInvalidateDCache_by_Addr((uint32_t *)LOADER_VRAM_BASE, LOADER_VRAM_SIZE);
        if(LoaderVram) { LoaderVram = 0; MPU_Config_nCacheable(0); }
        ld_screenback();
    }
    if(ldarea == NULL) return;
    if(LoaderExec) { LoaderExec = 0; MPU_Config_nCacheable(0); }
    FreeMemory(ldarea);
    ldarea = NULL;
    HeapLowMark(oldmin);
}

// a word of a Thumb-2 MOVT (T1) given another immediate: hw1 11110 i 101100 imm4, hw2 0 imm3 Rd imm8
static uint16_t ld_movt_imm(uint16_t *ins) {
    return ((ins[0] & 0xF) << 12) | ((ins[0] & 0x400) << 1) | ((ins[1] & 0x7000) >> 4) | (ins[1] & 0xFF);
}
static void ld_movt_set(uint16_t *ins, uint16_t v) {
    ins[0] = (ins[0] & ~0x040F) | ((v >> 12) & 0xF) | ((v & 0x800) >> 1);
    ins[1] = (ins[1] & ~0x70FF) | ((v & 0x700) << 4) | (v & 0xFF);
}

int LoaderRun(char *name) {
    char fname[STRINGSIZE], *p, **argv;
    cmm2img_header h;
    uint32_t *rel = NULL, *table, i, stack, end, crc, off;
    int n, fnbr, argc, code;
    UINT got;
    FIL *f;
    char *base = (char *)LOADER_BASE;
    uint32_t area = LOADER_SIZE;
    uint64_t need;
    int room = 0;
    void *oldmin = minMMheap;
    if(name) strcpy(fname, name);                                   // RUN f$: its value
    else {                                                          // RUN "name": the quoted name of the line's text
        p = runcmd;
        if(*p++ != '"') return 0;
        for(n = 0; *p && *p != '"' && n < STRINGSIZE - 1; ) fname[n++] = *p++;
        fname[n] = 0;
    }
    n = strlen(fname);
    if(n < 4 || strcasecmp(fname + n - 4, ".APP")) return 0;
    p = runcmd;                                                     // the words after the name's comma (runcmd is "name",text)
    if(*p == '"') { p++; while(*p && *p != '"') p++; if(*p) p++; }
    while(*p == ' ') p++;
    if(*p == ',') p++;

    CloseAudio(1);                                                  // (first: its refills would stop while the program runs)
    if(!InitSDCard()) return 1;
    fnbr = FindFreeFileNbr();
    if(!BasicFileOpen(fname, fnbr, FA_READ)) return 1;
    f = FileTable[fnbr].fptr;
    if(f_read(f, &h, sizeof(h), &got) != FR_OK || got != sizeof(h) || memcmp(h.magic, CMM2IMG_MAGIC, 8)) {
        FileClose(fnbr);
        error("Not a loadable program");
    }
    stack = h.stack_size ? (h.stack_size + 7) & ~7 : CMM2IMG_STACK;
    end = (h.image_size + h.bss_size + 7) & ~7;
    if(h.hdr_version != CMM2IMG_HDR_VERSION || h.kind != CMM2IMG_KIND_APP || (h.link_base & 0xFFFF) ||
       h.image_size == 0 || (h.entry & ~1) >= h.image_size || f_size(f) != sizeof(h) + h.image_size + h.reloc_count * 4) {
        FileClose(fnbr);
        error("Not a program this firmware can load");
    }
    if(h.api_version > CMM2IMG_API_VERSION) { FileClose(fnbr); error("Needs a newer firmware"); }
    if(G1Hardware && (h.flags & CMM2IMG_FLAG_G2)) { FileClose(fnbr); error("Needs a G2"); }
    int vram = G1Hardware && (h.flags & CMM2IMG_FLAG_VRAM), invram = 0, joined = 0;
    if(G1Hardware) {
        area = 0;
        if(vram && SDRAMBASE == LOADER_VRAM_BASE + LOADER_VRAM_SIZE && SbrkUnused()) {
            room = ((HeapBottomFree() - LD_G1_RESERVE) / RAMPAGESIZE) * RAMPAGESIZE;   // built --vram: one area, as a G2's
            if(room > 0x10000 && (ldarea = HeapClaimBottom(room)) != NULL) {   // 8 MB (Peter): the display memory and the
                joined = invram = 1;                                // heap's bottom above it, where newlib's arena would
                base = (char *)LOADER_VRAM_BASE;                    // start, were it ever used (syscalls.c: it may not grow
                area = (LOADER_VRAM_SIZE + room) & ~7;              // meanwhile). Not with OPTION RAM or COMPILE's stream,
            }                                                       // which are in between
        }
        if(!joined) {                                               // a G1: the heap's largest free run but the reserve, from
            room = LargestFreeOnHeap() - LD_G1_RESERVE;             // its first 64 KB boundary (the image's relocations
            if(room < 0) room = 0;                                  // allow a move by whole 64 KBs)
            if(room > 0x10000 && (ldarea = GetMemoryNull(room)) != NULL) {
                base = (char *)(((uint32_t)ldarea + 0xFFFF) & ~0xFFFF);
                area = ((uint32_t)ldarea + room - (uint32_t)base) & ~7;
            }
        }
    }
    // where it goes: its image, .bss and stack in one area (the relocations are read in above the .bss), its heap the
    // area's rest. Else a G1 lends a program built --vram its display memory as a second block: the program there if it
    // fits, the run it borrows then all heap; else the program in the run, the display memory a heap. Of two heaps the
    // larger is the start block's heap (malloc's: a program that knows nothing of heap2 has the most), the other heap2
    uint32_t place = end + (stack > h.reloc_count * 4 ? stack : h.reloc_count * 4), heapsize = 0, heap2size = 0;
    char *heap = NULL, *heap2 = NULL;
    if(joined) {
        ;
    } else if(vram && place <= LOADER_VRAM_SIZE) {
        invram = 1;
        if(ldarea) {
            heap2 = (char *)(((uint32_t)ldarea + 7) & ~7);
            heap2size = ((uint32_t)ldarea + room - (uint32_t)heap2) & ~7;
        }
        base = (char *)LOADER_VRAM_BASE;
        area = LOADER_VRAM_SIZE;
    } else if(vram) {
        heap2 = (char *)LOADER_VRAM_BASE;
        heap2size = LOADER_VRAM_SIZE;
    }
    if(place <= area) {
        heap = base + end;
        heapsize = area - end - stack;
    }
    if(heap2size > heapsize) {
        char *t = heap; heap = heap2; heap2 = t;
        uint32_t s = heapsize; heapsize = heap2size; heap2size = s;
    }
    need = (uint64_t)end + stack + h.heap_min;
    if(place > area || heapsize < h.heap_min) {
        FileClose(fnbr);
        ld_giveback(oldmin);
        if(G1Hardware) error("Needs % KB, % KB free", (int)(need / 1024),
                             (int)(((area ? area : room) + (vram && !invram ? LOADER_VRAM_SIZE : 0)) / 1024));
        error("Needs % KB, more than the 8 MB", (int)(need / 1024));
    }
    ld_screensave();                                                // (BASIC's screen, put back at the end)
    if(vram) {                                                      // the display memory lent: the screen first to a mode
        cmm2_video v;                                               // whose page is in the internal RAM (MODE 7,8), so that
        ldlent = 1;                                                 // nothing is drawn where the program goes
        if(ld_vscaled(7, 8, 1, &v) < 0) { FileClose(fnbr); ld_giveback(oldmin); error("Cannot lend the display memory"); }
    }
    for(off = 0; off < h.image_size; off += got) {                  // the image, straight into its place
        i = h.image_size - off;
        if(i > 0x10000) i = 0x10000;
        if(f_read(f, base + off, i, &got) != FR_OK || got != i) { FileClose(fnbr); ld_giveback(oldmin); error("Cannot read the program"); }
    }
    if(h.reloc_count) {
        rel = (uint32_t *)(base + end);
        if(f_read(f, rel, h.reloc_count * 4, &got) != FR_OK || got != h.reloc_count * 4) { FileClose(fnbr); ld_giveback(oldmin); error("Cannot read the program"); }
    }
    FileClose(fnbr);
    table = GetTempMemory(256 * sizeof(uint32_t));
    for(i = 0; i < 256; i++) {
        crc = i;
        for(n = 0; n < 8; n++) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
        table[i] = crc;
    }
    crc = ld_crc(table, 0, (unsigned char *)base, h.image_size);
    if(rel) crc = ld_crc(table, crc, (unsigned char *)rel, h.reloc_count * 4);
    if(crc != h.crc32) { ld_giveback(oldmin); error("The program file is damaged"); }
    if((uint32_t)base != h.link_base) {                             // moved by a multiple of 64 KB: ABS32 words and MOVTs
        uint32_t delta = (uint32_t)base - h.link_base;
        for(i = 0; i < h.reloc_count; i++) {
            off = CMM2IMG_REL_OFFSET(rel[i]);
            if(off + 4 > h.image_size) { ld_giveback(oldmin); error("The program file is damaged"); }
            if(CMM2IMG_REL_KIND(rel[i]) == CMM2IMG_REL_ABS32) {
                uint32_t w;
                memcpy(&w, base + off, 4);
                w += delta;
                memcpy(base + off, &w, 4);
            } else if(CMM2IMG_REL_KIND(rel[i]) == CMM2IMG_REL_MOVT) {
                uint16_t *ins = (uint16_t *)(base + off);
                ld_movt_set(ins, ld_movt_imm(ins) + (delta >> 16));
            } else { ld_giveback(oldmin); error("The program file is damaged"); }
        }
    }
    memset(base + h.image_size, 0, h.bss_size);
    SCB_CleanDCache_by_Addr((uint32_t *)base, h.image_size + h.bss_size);   // the code to the SDRAM, where it is fetched
    SCB_InvalidateICache();                                         // (a program run before may be in the cache)
    argv = ld_args(fname, p, &argc);
    ldstart.svc = &ldsvc;
    ldstart.argc = argc;
    ldstart.argv = argv;
    ldstart.heap = heap;
    ldstart.heap_size = heapsize;
    ldstart.heap2 = heap2size ? heap2 : NULL;
    ldstart.heap2_size = heap2size;
    ldstart.base = (uint32_t)base;
    MMAbort = 0;
    ldscreen = 1;
    if(ldarea && (!invram || joined)) LoaderExec = 1;               // (a G1: the run it borrows executable while the program
    if(invram) LoaderVram = 1;                                      // runs, and the display memory when the program is there)
    LoaderNoSbrk = joined;
    if(LoaderExec || LoaderVram) MPU_Config_nCacheable(0);
    code = LoaderCall(base + h.entry, &ldstart, (uint32_t)base + area);
    ld_tidy();                                                      // (a Ctrl-C stays: the BASIC program stops next)
    ld_giveback(oldmin);
    ConsoleRxBufTail = ConsoleRxBufHead;                            // (the keys the program had, not typed into BASIC)
    ld_screenback();
    FreeMemory(argv);
    if(code) {
        char b[STRINGSIZE + 40];
        snprintf(b, sizeof(b), "%s ended with code %d\r\n", fname, code);
        MMPrintString(b);
    }
    return 1;
}
