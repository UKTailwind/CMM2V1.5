/* cmm2_main.c: sc-im's start on the CMM2 (apps/scim). BSD 4-clause, as sc-im.

       RUN "A:/scim/scim.app" [, budget.sc] [mode=N] [font=WxH] [sc-im's own options]

   mode= and font= are the curses port's (apps/pdcurses: PDC_cmm2_args), which also puts its settings file,
   pdcurses.cfg, beside the program. sc-im is given HOME, the program's folder, where make.py's flags put its
   configuration (scimrc), its history (sc-iminfo) and its help (sc-im_help, by HELP_PATH); and LANG C.UTF-8, which
   its setlocale(LC_CTYPE, "") takes, so that it reads and writes UTF-8 (the Arm GNU Toolchain's newlib converts it:
   make.py). Then sc-im's main (make.py's out/scim_main.c renames it scim_main). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pdccmm2.h"

int scim_main(int argc, char **argv);

int main(int argc, char **argv) {
    static char folder[300];
    const char *cfg;

    argc = PDC_cmm2_args(argc, argv);
    cfg = getenv("PDC_CONFIG");                 /* (the program's folder, made whole, and "pdcurses.cfg") */
    if(cfg && strlen(cfg) < sizeof(folder)) {
        strcpy(folder, cfg);
        folder[strlen(folder) - strlen("pdcurses.cfg")] = 0;
        if(strlen(folder) > 3) folder[strlen(folder) - 1] = 0;     /* (no / at the end, but for a drive's root) */
        if(getenv("HOME") == NULL) setenv("HOME", folder, 1);
    }
    if(getenv("LANG") == NULL) setenv("LANG", "C.UTF-8", 1);
    return scim_main(argc, argv);
}
