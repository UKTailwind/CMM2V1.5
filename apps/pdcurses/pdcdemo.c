/* pdcdemo.c: PDCursesMod's demos on the CMM2 (apps/pdcurses/make.py), the one named after RUN's comma:

       RUN "pdcdemo.app", testcurs mode=9 font=12x24

   mode= and font= are the port's (pdcargs.c); the demo sees the words after its name. Public domain, as PDCursesMod. */
#include <stdio.h>
#include <string.h>
#include "pdccmm2.h"

#define DEMO(n) int n##_main(int argc, char **argv);
#include "demos.h"
#undef DEMO

static const struct { const char *name; int (*main)(int, char **); } demos[] = {
#define DEMO(n) {#n, n##_main},
#include "demos.h"
#undef DEMO
};

int main(int argc, char **argv) {
    unsigned i;
    argc = PDC_cmm2_args(argc, argv);
    if(argc > 1)
        for(i = 0; i < sizeof(demos) / sizeof(demos[0]); i++)
            if(strcmp(argv[1], demos[i].name) == 0) return demos[i].main(argc - 1, argv + 1);
    printf("RUN \"pdcdemo.app\", NAME [mode=N] [font=WxH], NAME one of:\n");
    for(i = 0; i < sizeof(demos) / sizeof(demos[0]); i++) printf("  %s\n", demos[i].name);
    return argc > 1;
}
