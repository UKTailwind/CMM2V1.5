/* crt0.c: the start of a loadable program (tools/sdk). MIT License, as cmm2img.h.

   The loader calls _start(start) on the program's own stack with its .bss zeroed. _start notes the services, runs the
   constructors, calls main(), runs the destructors and leaves through exit(), which flushes stdio and calls _exit()
   (syscalls.c), which goes back to MMBasic with main's value. */
#include <stdlib.h>
#include "cmm2.h"

const struct cmm2_services *cmm2;
cmm2img_start *cmm2_start;

extern void (*__preinit_array_start[])(void), (*__preinit_array_end[])(void);
extern void (*__init_array_start[])(void), (*__init_array_end[])(void);
extern void (*__fini_array_start[])(void), (*__fini_array_end[])(void);
extern int main(int argc, char **argv);

int _start(cmm2img_start *s) __attribute__((section(".text.entry"), used));
int _start(cmm2img_start *s) {
    void (**f)(void);
    int r;
    cmm2 = s->svc;
    cmm2_start = s;
    for(f = __preinit_array_start; f < __preinit_array_end; f++) (*f)();
    for(f = __init_array_start; f < __init_array_end; f++) (*f)();
    r = main(s->argc, s->argv);
    for(f = __fini_array_end; f > __fini_array_start; ) (*--f)();
    exit(r);
}
