/* zf_cmm2.c: zeptoforth's kernel on the Colour Maximite 2: the program's start, and what the kernel's assembly calls for the firmware's services (port/console.s).

   The kernel takes over from main(): zf_reset sets its stacks and never returns, and bye leaves through the leave service. What it knows as flash is RAM here, so the
   flash dictionary is read from zeptoforth.img in the current folder at the start and written back by save-image. The file holds code compiled against this kernel's
   addresses, so it is refused if the kernel has moved. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"

extern void zf_reset(void);
extern const uint32_t zf_mem[5];                /* flash_dict_start, flash_dict_end, ram_start, ram_end (port/config.s), and where color_enabled is */

#ifdef ZF_G1
#define IMAGE      "zeptoforth_g1.img"
#else
#define IMAGE      "zeptoforth.img"
#endif
#define MAGIC      0x31464A5Au                  /* its first word; then the kernel's zf_mem address, then the bytes that follow */
#define POLL_US    5000
#define CHUNK      65536

#define SCB_VTOR    (*(volatile uint32_t *)0xE000ED08)
#define SCB_SHCSR   (*(volatile uint32_t *)0xE000ED24)
#define SCB_CFSR    (*(volatile uint32_t *)0xE000ED28)
#define SCB_HFSR    (*(volatile uint32_t *)0xE000ED2C)
#define SCB_CCSIDR  (*(volatile uint32_t *)0xE000ED80)
#define SCB_CSSELR  (*(volatile uint32_t *)0xE000ED84)
#define SCB_ICIALLU (*(volatile uint32_t *)0xE000EF50)
#define SCB_DCCSW   (*(volatile uint32_t *)0xE000EF6C)

/* shared with port/tasks.s */
volatile uint32_t zf_armed, zf_in_c, zf_in_switch, zf_pending, zf_resume_pc;
uint32_t zf_tick_xt, zf_switch_xt, zf_c_sp;
extern void zf_preempt(void);

volatile uint32_t zf_frames;                    /* the display's frames, for wait-frame (zf_services.c) */
extern void zf_svc_reset(void);

/* zf_console.c */
extern void zf_con_open(void), zf_con_close(void), zf_con_emit(int c), zf_con_service(void), zf_con_reset(void);
extern int zf_con_key(void);
void zf_faults_back(void);

static int key_waiting = -1, broken;
static uint64_t polled;

/* poll is wanted every 10 to 20 ms: the tick does it, with the keys and the output between ticks. A Ctrl-C is kept for whoever looks next, as output may see it first */
static void look_after(void) {
    uint64_t now = cmm2->micros();
    if(now - polled < POLL_US) return;
    polled = now;
    cmm2->poll();
    if(cmm2->break_take()) broken = 1;
    zf_con_service();
}

static int take_break(void) {
    int b = broken;
    broken = 0;
    return b;
}

/* The tick: the one place that says time has passed, so another source could call it in place of the frame hook. It is at PendSV's level, the lowest, so what it interrupted is
   the thread, whose frame is on the process stack. The pc in that frame becomes zf_preempt, and the thread goes there as this returns (port/tasks.s says why). In a task switch
   there is nothing to do. In C it is left for the thread to take on the way out, or a task spending its time in the maths library would hardly ever be preempted. A thread in an
   IT block (xPSR's ICI/IT bits) cannot be returned to from thread mode: the next tick does as well */
static void zf_frame(void) {
    uint32_t *frame;
    zf_frames++;
    if(!zf_armed || zf_in_switch) return;
    if(zf_in_c) {
        zf_pending = 1;
        return;
    }
    __asm volatile("mrs %0, psp" : "=r"(frame));
    if(frame[7] & 0x0600FC00u) return;
    zf_resume_pc = frame[6];
    zf_in_switch = 1;
    frame[6] = (uint32_t)zf_preempt & ~1u;
}

/* in thread mode, from zf_preempt: 1 for a Ctrl-C */
int zf_tick(void) {
    polled = 0;
    look_after();
    return take_break();
}

/* with no task ready: an interrupt ends the wait, the firmware's 1 ms tick at the latest */
int zf_idle(void) {
    look_after();
    __asm volatile("wfi");
    return take_break();
}

/* source files (port/files.s): a buffer for each that may be open at once, as a file includes another */
#define LINE_MAX  512
#define FILE_BUF  4096
#define FILE_MAX  4
static struct { int h, pos, len; char buf[FILE_BUF], line[LINE_MAX]; } src[FILE_MAX];
char *zf_line_addr;

int zf_open(const char *s, int n) {
    char name[128];
    int i, h;
    if(n < 1 || n >= (int)sizeof(name)) return -1;
    memcpy(name, s, n);
    name[n] = 0;
    for(i = 0; i < FILE_MAX && src[i].h > 0; i++) { }
    if(i == FILE_MAX || (h = cmm2->file_open(name, CMM2_O_READ)) < 0) return -1;
    src[i].h = h + 1;
    src[i].pos = src[i].len = 0;
    return i;
}

/* the next line in the file's own line buffer, and its length, or -1 at the end. Evaluating it may include another file, so each file keeps its own */
int zf_line(int f) {
    int n = 0, any = 0;
    if(f < 0 || f >= FILE_MAX || src[f].h <= 0) return -1;
    zf_line_addr = src[f].line;
    for(;;) {
        char c;
        if(src[f].pos == src[f].len) {
            src[f].len = cmm2->file_read(src[f].h - 1, src[f].buf, FILE_BUF);
            src[f].pos = 0;
            if(src[f].len <= 0) {
                src[f].len = 0;
                return any ? n : -1;
            }
        }
        c = src[f].buf[src[f].pos++];
        any = 1;
        if(c == 10) return n;
        if(c != 13 && n < LINE_MAX - 1) src[f].line[n++] = c;
    }
}

void zf_close(int f) {
    if(f < 0 || f >= FILE_MAX || src[f].h <= 0) return;
    cmm2->file_close(src[f].h - 1);
    src[f].h = 0;
}

/* double precision (port/float64.s) */
int zf_vparse_ok;

int zf_vformat(double x, char *buf) {
    return snprintf(buf, 40, "%.16g", x);
}

double zf_vparse(const char *s, int n) {
    char text[64], *end;
    double v;
    int i, m = 0;
    zf_vparse_ok = 0;
    if(n < 1 || n >= (int)sizeof(text)) return 0;
    for(i = 0; i < n; i++)                      // the d of 1.5de0 is Forth's: strtod reads 1.5e0
        if((s[i] | 0x20) != 'd' || i + 1 == n || (s[i + 1] | 0x20) != 'e') text[m++] = s[i];
    text[m] = 0;
    v = strtod(text, &end);
    if(end == text + m) zf_vparse_ok = -1;
    return v;
}

double zf_fix2d(uint32_t lo, int32_t hi) {
    return (double)(((int64_t)hi << 32) | lo) / 4294967296.0;
}

int64_t zf_d2fix(double x) {
    double v = x * 4294967296.0;
    if(v != v) return 0;
    if(v >= 9223372036854775807.0) return INT64_MAX;
    if(v <= -9223372036854775808.0) return INT64_MIN;
    return (int64_t)v;
}

uint32_t zf_ticks(void) {
    return (uint32_t)(cmm2->micros() / 100);
}

void zf_emit(int c) {
    look_after();
    zf_con_emit(c);
}

int zf_key_q(void) {
    look_after();
    if(take_break()) return -1;
    if(key_waiting < 0) key_waiting = zf_con_key();
    return key_waiting >= 0;
}

int zf_key(void) {
    int c = key_waiting;
    key_waiting = -1;
    return c;
}

/* from the kernel's start, each time reboot comes round: a file an interrupted include left open is shut, and what Forth had of the firmware put back (the end of the program
   does that for a bye, but a reboot is no end as far as the firmware knows) */
uint32_t zf_cpu_hz(void) {
    extern void zf_close(int f);
    for(int i = 0; i < 4; i++) zf_close(i);
    zf_svc_reset();
    zf_con_reset();
    return cmm2->cpu_hz();
}

void zf_bye(void) {
    zf_con_close();
    zf_faults_back();
    cmm2->leave(0);
}

/* what the data cache holds out to the memory, and the instruction cache emptied, so that code just written is what is fetched. By set and way: it is quicker than
   by address for a word's few lines, the cache being 16 KB, and it needs no idea of where the compiler has been */
void zf_sync_code(void) {
    uint32_t ccsidr, sets, ways, w;
    SCB_CSSELR = 0;
    __asm volatile("dsb" ::: "memory");
    ccsidr = SCB_CCSIDR;
    sets = ccsidr >> 13 & 0x7FFF;
    ways = ccsidr >> 3 & 0x3FF;
    do {
        w = ways;
        do {
            SCB_DCCSW = (sets << 5 & 0x3FE0) | (w << 30);
        } while(w--);
    } while(sets--);
    __asm volatile("dsb" ::: "memory");
    SCB_ICIALLU = 0;
    __asm volatile("dsb\n\tisb" ::: "memory");
}

/* A bad address. The firmware's fault handlers wait for ever, so this program's vectors are a copy of the firmware's with three replaced, put back when it ends. Those three are turned
   on, as they are not normally: what would be a HardFault cannot be handled here, as the MPU is off in a HardFault handler and without it the SDRAM cannot be executed from. A fault
   in the thread goes on where port/console.s says: as an exception if it was in Forth's own code, or Forth starts again if it was in C or a task switch, or if another comes straight
   away, as Forth's stacks may be what is wrong. In an interrupt there is nothing to go back to, so the firmware's handler has it */
#define FAULTS_ON       (7u << 16)              /* SHCSR: usage, bus and memory management faults */
#define VECTORS         256                     /* (the H743 has 166) */
#define FAULT_REPEAT_US 100000
static uint32_t vectors[VECTORS] __attribute__((aligned(1024)));
static uint32_t firmware_vectors, firmware_faults;
static uint64_t last_fault;
extern void zf_fault_forth(void), zf_fault_restart(void);
void zf_sync_code(void);

void zf_fault(uint32_t exc_return) {
    uint32_t *frame, target = (uint32_t)zf_fault_forth;
    uint64_t now = cmm2->micros();
    if(!(exc_return & 8)) ((void (*)(void))((uint32_t *)firmware_vectors)[3])();
    if(exc_return & 4) __asm volatile("mrs %0, psp" : "=r"(frame));
    else __asm volatile("mrs %0, msp" : "=r"(frame));
    if(zf_in_c || zf_in_switch || now - last_fault < FAULT_REPEAT_US) target = (uint32_t)zf_fault_restart;
    last_fault = now;
    zf_in_c = zf_pending = 0;
    SCB_CFSR = SCB_CFSR;
    SCB_HFSR = SCB_HFSR;
    frame[6] = target & ~1u;
    frame[7] = 0x01000000u;                     /* Thumb, and no IT block */
}

static void __attribute__((naked)) fault_entry(void) {
    __asm volatile("mov r0, lr\n\tpush {r0, r1}\n\tbl zf_fault\n\tpop {r0, r1}\n\tbx r0");
}

/* from port/console.s, on C's stack */
void zf_fault_said(void) {
    static const char said[] = "bad address in the firmware: Forth starts again\r\n";
    const char *p;
    for(p = said; *p; p++) zf_con_emit(*p);
}

static void faults_mine(void) {
    int i;
    firmware_vectors = SCB_VTOR;
    memcpy(vectors, (const void *)firmware_vectors, sizeof(vectors));
    for(i = 4; i <= 6; i++) vectors[i] = (uint32_t)fault_entry | 1;
    zf_sync_code();
    SCB_VTOR = (uint32_t)vectors;
    __asm volatile("dsb\n\tisb" ::: "memory");
    firmware_faults = SCB_SHCSR & FAULTS_ON;
    SCB_SHCSR |= FAULTS_ON;
}

void zf_faults_back(void) {
    SCB_SHCSR = (SCB_SHCSR & ~FAULTS_ON) | firmware_faults;
    SCB_VTOR = firmware_vectors;
    __asm volatile("dsb\n\tisb" ::: "memory");
}

static void say(const char *s) {
    for(; *s; s++) zf_con_emit(*s);
}

static uint32_t image_size(void) {
    const uint32_t *lo = (const uint32_t *)zf_mem[0], *p = (const uint32_t *)zf_mem[1];
    while(p > lo && p[-1] == 0xFFFFFFFFu) p--;
    return (uint32_t)((const char *)p - (const char *)lo);
}

void zf_save(void) {
    uint32_t head[3] = { MAGIC, (uint32_t)zf_mem, image_size() }, done = 0;
    const char *p = (const char *)zf_mem[0];
    int h = cmm2->file_open(IMAGE, CMM2_O_WRITE | CMM2_O_CREATE);
    if(h < 0) {
        say("save-image: cannot create " IMAGE "\r\n");
        return;
    }
    if(cmm2->file_write(h, head, sizeof(head)) != (int32_t)sizeof(head)) done = 1;
    else while(done < head[2]) {
        uint32_t n = head[2] - done > CHUNK ? CHUNK : head[2] - done;
        if(cmm2->file_write(h, p + done, n) != (int32_t)n) break;
        done += n;
        cmm2->poll();
    }
    cmm2->file_close(h);
    say(done == head[2] ? "saved to " IMAGE "\r\n" : "save-image: the write failed\r\n");
}

static void load_image(void) {
    uint32_t head[3];
    int h = cmm2->file_open(IMAGE, CMM2_O_READ);
    if(h < 0) return;
    if(cmm2->file_read(h, head, sizeof(head)) != (int32_t)sizeof(head) || head[0] != MAGIC || head[1] != (uint32_t)zf_mem || head[2] > zf_mem[1] - zf_mem[0])
        say(IMAGE " is not for this kernel: left out\r\n");
    else if(cmm2->file_read(h, (void *)zf_mem[0], head[2]) != (int32_t)head[2]) {
        memset((void *)zf_mem[0], 0xFF, head[2]);
        say(IMAGE " is short: left out\r\n");
    }
    cmm2->file_close(h);
}

int main(int argc, char **argv) {
    uint32_t heap = (uint32_t)cmm2_start->heap, sp;
    (void)argc;
    (void)argv;
    __asm volatile("mov %0, sp" : "=r"(sp));
    zf_c_sp = (sp - 256) & ~7u;                 /* (this stack: main() does not come back to it) */
    if(heap > zf_mem[0] || heap + cmm2_start->heap_size < zf_mem[3]) {
        cmm2->con_puts("zeptoforth: the memory it is linked for is not free\r\n");
        return 1;
    }
    cmm2_heap_take(heap + cmm2_start->heap_size - zf_mem[0]);    /* (malloc keeps below it: the dictionary and RAM are Forth's) */
    memset((void *)zf_mem[0], 0xFF, zf_mem[1] - zf_mem[0]);
    zf_con_open();
    load_image();
    faults_mine();
    cmm2->frame_hook(zf_frame);
    zf_reset();
    return 0;
}
