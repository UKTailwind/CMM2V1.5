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
