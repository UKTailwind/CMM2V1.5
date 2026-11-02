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
#include "opl3.h"
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

#define LD_FILES  8
#define LD_DIRS   4
#define LD_BLOCKS 64
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
static void ld_putc(int c) { putConsole(c); }
static void ld_puts(const char *s) { MMPrintString((char *)s); }
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
    ldfile[h] = f;
    return h;
}
static int ld_close(int h) {
    FIL *f = ld_fil(h);
    if(f == NULL) return -FR_INVALID_OBJECT;
    FRESULT r = f_close(f);
    FreeMemory(f);
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
// an error); Ctrl-C reported once
static int ld_poll(void) {
    if(USBtime) {
        mT4IntEnable(0);
        CheckKeyboard();
        USBtime = 0;
        mT4IntEnable(1);
    }
    if(MMAbort) {
        MMAbort = 0;
        return 1;
    }
    return 0;
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
// the firmware's OPL3 synth for a program: its own chips, on the heap (PLAY OPL's is OPL.c's)
static void *ld_oplnew(uint32_t rate) {
    opl3_chip *c = LoaderAlloc(sizeof(opl3_chip));
    if(c) OPL3_Reset(c, rate);
    return c;
}
static void ld_oplwrite(void *chip, uint32_t reg, uint32_t value) { OPL3_WriteRegBuffered(chip, reg & 0x1FF, value); }
static void ld_oplgen(void *chip, int16_t *buf, uint32_t frames) { OPL3_GenerateStream(chip, buf, frames); }

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
    .opl_new = ld_oplnew, .opl_write = ld_oplwrite, .opl_generate = ld_oplgen, .opl_free = LoaderRelease,
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
    for(i = 0; i < LD_FILES; i++) if(ldfile[i]) { f_close(ldfile[i]); FreeMemory(ldfile[i]); ldfile[i] = NULL; }
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
    CloseAudio(1);                                                  // (its refills would stop while the program runs)
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
    code = LoaderCall(base + h.entry, &ldstart, LOADER_BASE + LOADER_SIZE);
    MMAbort = 0;                                                    // (a Ctrl-C the program did not poll for)
    ld_tidy();
    FreeMemory(argv);
    if(code) {
        char b[STRINGSIZE + 40];
        snprintf(b, sizeof(b), "%s ended with code %d\r\n", fname, code);
        MMPrintString(b);
    }
    return 1;
}
