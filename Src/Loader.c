/***********************************************************************************************************************
Loadable programs (G2 only, 2 Nov 2026: Peter). docs/Loader_Plan.md has the design, Inc/cmm2img.h the file format and
the services, tools/sdk the tools that build a program.

  RUN "name.app" [, words]   loads the program into the 8 MB at LOADER_BASE (Memory.h; MPU region 8 makes it executable
                             and write-back cached), runs it, and goes on with the next statement when it ends (typed at
                             the prompt: back to the prompt). The words after the comma are its argv[1] on.

Only a name ending in .APP is looked at, so RUN of a BASIC program is as it was, and nothing of the BASIC program in
memory is touched: the 8 MB is the program's alone.

The program runs on its own stack (the process stack, at the top of the 8 MB), so the interrupts keep the main stack in
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
// a file only read is given a map of its clusters (FatFs's fast seek): a seek, or the next cluster, is then found in the
// table, not by following the FAT from the file's start over the card's SPI each time (Doom seeks to each lump of a
// 28 MB WAD); 64 entries first, then as many as FatFs says it needs; without room it is read as before
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
    if(!(m & FA_WRITE)) ld_map(f);
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
    FRESULT r = f_write(f, buf, n, &put);
    return r ? -(int)r : (int32_t)put;
}
static int ld_seek(int h, uint32_t pos) { FIL *f = ld_fil(h); return f ? ld_fail(f_lseek(f, pos)) : -FR_INVALID_OBJECT; }
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
static int ld_vmode(int mode, int depth, cmm2_video *v) {
    if(mode < 1 || mode > MAX_MODES || depth != 8) return -1;
    if(mode == 16 && !(G1Hardware == 0 && HAL_GetREVID() != 0x1003)) return -1;
    if(VideoColour != depth && PageTable[WPN].address != NULL) return -2;   // (a FRAMEBUFFER of another depth)
    uint32_t old = __get_BASEPRI();
    __set_BASEPRI(LD_PRI_FILL);                                     // (no refill: setmode turns the MPU off for a moment,
    setmode(mode, depth, 0, 0);                                     // when the program's code in SDRAM can't be fetched)
    __set_BASEPRI(old);
    v->fb = PageTable[0].address;
    v->width = PageTable[0].xmax;
    v->height = PageTable[0].ymax;
    v->copies = PageTable[0].expand ? 2 : 1;                        // (a doubled mode's page holds each line twice: Draw.c)
    v->pitch = v->width * v->copies;
    v->bpp = 8;
    memset(v->fb, 0, v->pitch * v->height);                         // (the same mode again is not cleared by setmode)
    return 0;
}
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

static int ld_keys(int keys[7]) {
    int i, n = 0;
    for(i = 0; i < 7; i++) keys[i] = KeyDown[i];
    for(i = 0; i < 6; i++) if(keys[i]) n++;
    return n;
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
    ld_astop();                                                     // its sound, and its music's synth (before its blocks)
    OPLClose();
    __set_BASEPRI(0);                                               // (audio_lock left on)
    for(i = 0; i < LD_FILES; i++) if(ldfile[i]) { f_close(ldfile[i]); ld_free(ldfile[i]); ldfile[i] = NULL; }
    for(i = 0; i < LD_DIRS; i++) if(lddir[i]) { f_closedir(lddir[i]); FreeMemory(lddir[i]); lddir[i] = NULL; }
    for(i = 0; i < LD_BLOCKS; i++) if(ldblock[i]) { FreeMemory(ldblock[i]); ldblock[i] = NULL; }
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
    if(name) strcpy(fname, name);                                   // RUN f$: its value
    else {                                                          // RUN "name": the quoted name of the line's text
        p = runcmd;
        if(*p++ != '"') return 0;
        for(n = 0; *p && *p != '"' && n < STRINGSIZE - 1; ) fname[n++] = *p++;
        fname[n] = 0;
    }
    n = strlen(fname);
    if(n < 4 || strcasecmp(fname + n - 4, ".APP")) return 0;
    if(G1Hardware) error("Only available on a G2");
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
    if((uint64_t)end + stack + h.heap_min > LOADER_SIZE) { FileClose(fnbr); error("Needs % KB, more than the 8 MB", (int)(((uint64_t)end + stack + h.heap_min) / 1024)); }
    for(off = 0; off < h.image_size; off += got) {                  // the image, straight into the 8 MB
        i = h.image_size - off;
        if(i > 0x10000) i = 0x10000;
        if(f_read(f, base + off, i, &got) != FR_OK || got != i) { FileClose(fnbr); error("Cannot read the program"); }
    }
    if(h.reloc_count) {
        rel = GetMemory(h.reloc_count * 4);
        if(f_read(f, rel, h.reloc_count * 4, &got) != FR_OK || got != h.reloc_count * 4) { FileClose(fnbr); error("Cannot read the program"); }
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
    if(crc != h.crc32) error("The program file is damaged");
    if(LOADER_BASE != h.link_base) {                                // moved by a multiple of 64 KB: ABS32 words and MOVTs
        uint32_t delta = LOADER_BASE - h.link_base;
        for(i = 0; i < h.reloc_count; i++) {
            off = CMM2IMG_REL_OFFSET(rel[i]);
            if(off + 4 > h.image_size) error("The program file is damaged");
            if(CMM2IMG_REL_KIND(rel[i]) == CMM2IMG_REL_ABS32) {
                uint32_t w;
                memcpy(&w, base + off, 4);
                w += delta;
                memcpy(base + off, &w, 4);
            } else if(CMM2IMG_REL_KIND(rel[i]) == CMM2IMG_REL_MOVT) {
                uint16_t *ins = (uint16_t *)(base + off);
                ld_movt_set(ins, ld_movt_imm(ins) + (delta >> 16));
            } else error("The program file is damaged");
        }
    }
    if(rel) FreeMemory(rel);
    memset(base + h.image_size, 0, h.bss_size);
    SCB_CleanDCache_by_Addr((uint32_t *)base, h.image_size + h.bss_size);   // the code to the SDRAM, where it is fetched
    SCB_InvalidateICache();                                         // (a program run before may be in the cache)
    argv = ld_args(fname, p, &argc);
    ldstart.svc = &ldsvc;
    ldstart.argc = argc;
    ldstart.argv = argv;
    ldstart.heap = base + end;
    ldstart.heap_size = LOADER_SIZE - end - stack;
    ldstart.base = LOADER_BASE;
    MMAbort = 0;
    int oldmode = VideoMode, oldcolour = VideoColour, oldpico = PicoActive;   // BASIC's screen, put back at the end: a
    int oldfont = gui_font, oldfc = gui_fcolour, oldbc = gui_bcolour;   // mode change clears it, but its mode, font,
    uint32_t oldclut[256];                                          // colours and palette (MAP's) are as they were
    memcpy(oldclut, CLUT, sizeof(oldclut));
    ldpalette = 0;
    ldscreen = 1;
    code = LoaderCall(base + h.entry, &ldstart, LOADER_BASE + LOADER_SIZE);
    ld_tidy();                                                      // (a Ctrl-C stays: the BASIC program stops next)
    ConsoleRxBufTail = ConsoleRxBufHead;                            // (the keys the program had, not typed into BASIC)
    int changed = VideoMode != oldmode || VideoColour != oldcolour || PicoActive != oldpico;
    if(changed) {
        if(oldpico) PicoModeSet(PicoModeNow);                       // (OPTION PICOMITE's window: its MODE again)
        else setmode(oldmode, oldcolour, 0, 0);
        SetFont(oldfont);
        gui_fcolour = oldfc;
        gui_bcolour = oldbc;
    }
    if(oldcolour == 8 && (changed || ldpalette)) {
        memcpy(CLUT, oldclut, sizeof(CLUT));
        ld_frame();
        HAL_LTDC_ConfigCLUT(&hltdc, CLUT, 256, 0);
    }
    FreeMemory(argv);
    if(code) {
        char b[STRINGSIZE + 40];
        snprintf(b, sizeof(b), "%s ended with code %d\r\n", fname, code);
        MMPrintString(b);
    }
    return 1;
}
