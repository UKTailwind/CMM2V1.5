/* crt0.c: the start of a loadable program (tools/sdk). MIT License, as cmm2img.h.

   The loader calls _start(start) on the program's own stack with its .bss zeroed. _start notes the services, runs the
   constructors, calls main(), runs the destructors and leaves through exit(), which flushes stdio and calls _exit()
   (syscalls.c), which goes back to MMBasic with main's value. Built --xip (build.py: a runtime the firmware runs in
   place from its flash) it begins with the cmm2xip_header the loader starts it by, its .data copied and .bss zeroed. */
#include <stdlib.h>
#include "cmm2.h"

const struct cmm2_services *cmm2;
cmm2img_start *cmm2_start;

extern void (*__preinit_array_start[])(void), (*__preinit_array_end[])(void);
extern void (*__init_array_start[])(void), (*__init_array_end[])(void);
extern void (*__fini_array_start[])(void), (*__fini_array_end[])(void);
extern int main(int argc, char **argv);
extern void cmm2_mem_bind(void);

int _start(cmm2img_start *s) __attribute__((section(".text.entry"), used));

#ifdef CMM2_XIP
extern char __data_load__[], __data_start__[], __data_end__[], __bss_start__[], __bss_end__[];
const cmm2xip_header cmm2_xip __attribute__((section(".xip.header"), used)) = {
    CMM2XIP_MAGIC, CMM2_XIP_API, (uint32_t)_start, (uint32_t)__data_load__, (uint32_t)__data_start__,
    (uint32_t)__data_end__, (uint32_t)__bss_start__, (uint32_t)__bss_end__, CMM2_XIP_STACK, CMM2_XIP_HEAP
};
#endif

#ifndef CMM2_XIP
/* .axi (app.ld, build.py --axi): copied into the AXI RAM, which for the run is ordinary memory, cached (write-back,
   write-allocate, not shareable) and executable, where the firmware's MPU region 12 makes it shareable, which the M7's
   data cache does not cache, and not executable: the firmware's display memory in its modes that fit it. Only once
   the program's mode has its pages in the SDRAM (video_mode: a mode's change clears the AXI RAM), and before any code
   of .axi's runs; cmm2_axi_unload() before the program ends puts region 12 back, the cache emptied of the RAM */
extern char __axi_load[], __axi_start[], __axi_end[], __itcm_load[], __itcm_start[], __itcm_end[];
#define MPU_RNR  (*(volatile uint32_t *)0xE000ED98u)
#define MPU_RBAR (*(volatile uint32_t *)0xE000ED9Cu)
#define MPU_RASR (*(volatile uint32_t *)0xE000EDA0u)
#define ICIALLU  (*(volatile uint32_t *)0xE000EF50u)
#define DCCMVAC  (*(volatile uint32_t *)0xE000EF68u)
#define DCCIMVAC (*(volatile uint32_t *)0xE000EF70u)
static uint32_t axi_rasr;
static int axi_loaded;
int cmm2_axi_load(void) {
    if(axi_loaded) return 0;
    MPU_RNR = 12;
    if((MPU_RBAR & ~0x1Fu) != 0x24000000u || ((MPU_RASR >> 1) & 0x1F) != 18) return -1;   /* (512 KB at 0x24000000) */
    axi_rasr = MPU_RASR;
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RASR = (axi_rasr & ~((1u << 28) | (7u << 19) | (1u << 18) | (1u << 17) | (1u << 16))) | (1u << 19) | (1u << 17) | (1u << 16);
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
    axi_loaded = 1;
    for(char *p = __axi_start, *q = __axi_load; p < __axi_end; ) *p++ = *q++;
    for(char *p = __itcm_start, *q = __itcm_load; p < __itcm_end; ) *p++ = *q++;   /* (.itcm: build.py --itcm) */
    for(uintptr_t a = (uintptr_t)__axi_start & ~31u; a < (uintptr_t)__axi_end; a += 32) DCCMVAC = (uint32_t)a;
    __asm volatile ("dsb 0xF" ::: "memory");
    ICIALLU = 0;                                /* (the code fetched from what is there now) */
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
    return 0;
}
void cmm2_axi_unload(void) {
    if(!axi_loaded) return;
    for(uint32_t a = 0x24000000u; a < 0x24080000u; a += 32) DCCIMVAC = a;
    __asm volatile ("dsb 0xF" ::: "memory");
    MPU_RNR = 12;
    MPU_RASR = axi_rasr;
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
    ICIALLU = 0;
    __asm volatile ("dsb 0xF\n isb 0xF" ::: "memory");
    axi_loaded = 0;
}
#endif

/* The whole newlib (build.py --libc full) has a constructor that gives this to atexit, so that exit() would run the
   destructors (and _fini, which -nostartfiles leaves out): _start runs them itself, as with newlib-nano, so here it
   does nothing, and they run once (12 Nov 2026) */
void __libc_fini_array(void) {}

int _start(cmm2img_start *s) {
    void (**f)(void);
    int r;
    cmm2 = s->svc;
    cmm2_start = s;
    cmm2_mem_bind();
    for(f = __preinit_array_start; f < __preinit_array_end; f++) (*f)();
    for(f = __init_array_start; f < __init_array_end; f++) (*f)();
    r = main(s->argc, s->argv);
    for(f = __fini_array_end; f > __fini_array_start; ) (*--f)();
    exit(r);
}
