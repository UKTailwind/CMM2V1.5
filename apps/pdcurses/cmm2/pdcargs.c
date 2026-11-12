/* pdcargs.c: the port's own words after RUN's comma (apps/pdcurses/cmm2). Public domain, as PDCursesMod.

       RUN "A:/office/wordgrinder.app", mode=9 font=12x24 letter.wg

   mode=N and font=WxH are taken out of argv and set as PDC_MODE and PDC_FONT, which pdcscrn.c reads; the program sees
   the rest. debug=1 (PDC_DEBUG) has pdckbd.c report each key and the heap on the serial console. PDC_CONFIG, the settings file pdcscrn.c keeps the last mode and font in, is set to pdcurses.cfg beside the
   program (argv[0]'s folder, made whole from the current directory), before the program can change directory. */
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "pdccmm2.h"

#define PATH 300

static void settings_file(const char *prog) {
    char path[PATH + 16];
    const char *slash = strrchr(prog, '/'), *bslash = strrchr(prog, '\\');
    size_t n;
    if(bslash > slash) slash = bslash;
    path[0] = 0;
    if(!(prog[0] && prog[1] == ':') && prog[0] != '/' && prog[0] != '\\') {     /* relative: from the current directory */
        if(getcwd(path, PATH) == NULL) return;
        n = strlen(path);
        if(n && path[n - 1] != '/' && path[n - 1] != '\\') strcat(path, "/");
    }
    if(slash) {                                 /* its folder */
        n = strlen(path);
        if(n + (size_t)(slash - prog) + 1 >= PATH) return;
        memcpy(path + n, prog, slash - prog + 1);
        path[n + (slash - prog) + 1] = 0;
    } else if(prog[0] && prog[1] == ':') {      /* "A:name.app": the drive's current directory is not known here */
        if(getcwd(path, PATH) == NULL) return;
        if(path[strlen(path) - 1] != '/') strcat(path, "/");
    }
    strcat(path, "pdcurses.cfg");
    setenv("PDC_CONFIG", path, 1);
}

int PDC_cmm2_args(int argc, char **argv) {
    int i, j;
    if(argc > 0 && argv[0] && getenv("PDC_CONFIG") == NULL) settings_file(argv[0]);
    for(i = j = 1; i < argc; i++) {
        if(strncasecmp(argv[i], "mode=", 5) == 0) setenv("PDC_MODE", argv[i] + 5, 1);
        else if(strncasecmp(argv[i], "font=", 5) == 0) setenv("PDC_FONT", argv[i] + 5, 1);
        else if(strncasecmp(argv[i], "debug=", 6) == 0) setenv("PDC_DEBUG", argv[i] + 6, 1);
        else argv[j++] = argv[i];
    }
    if(argc > 0) argv[j] = NULL;
    return argc > 0 ? j : argc;
}
