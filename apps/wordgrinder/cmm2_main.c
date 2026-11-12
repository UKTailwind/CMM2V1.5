/* cmm2_main.c: WordGrinder's start on the CMM2 (apps/wordgrinder). MIT License, as WordGrinder.

       RUN "A:/wordgrinder/wordgrinder.app" [, letter.wg] [mode=N] [font=WxH]

   mode= and font= are the curses port's (apps/pdcurses: PDC_cmm2_args), which also puts its settings file,
   pdcurses.cfg, beside the program. WordGrinder's Lua code runs as on Windows (make.py: ARCH "windows", for the file
   browser's drive letters and names in any case), and is given what that needs: WINDOWS_EXE, the program's whole
   name, from which main.lua takes its folder (the dictionary's place); and HOME, that folder, where its settings go
   (.wordgrinder). Then WordGrinder's main (make.py's out/wg_main.c renames it wg_main).

   At the first start, with no settings of its own yet, WordGrinder is given Look and Feel on: the text at most 80
   columns wide on the large screens it is mostly used on, and dense paragraphs, indented with no blank line between
   (Peter, 12 Nov 2026). Its own saved settings are never touched. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "pdccmm2.h"

int wg_main(int argc, const char *argv[]);

/* folder/.wordgrinder/settings.dat, in WordGrinder's own form (fileio.lua's SaveToStream), if there is none: the add-ons
   fill in the rest of their settings as they start (MergeTables) */
static void first_settings(const char *folder) {
    char dir[320], file[340];
    FILE *f;
    snprintf(dir, sizeof(dir), "%s%s.wordgrinder", folder, folder[strlen(folder) - 1] == '/' ? "" : "/");
    snprintf(file, sizeof(file), "%s/settings.dat", dir);
    if((f = fopen(file, "r")) != NULL) {
        fclose(f);
        return;
    }
    mkdir(dir, 0777);
    if((f = fopen(file, "w")) == NULL) return;
    fputs("WordGrinder dumpfile v3: this is a text file; diff me!\n"
          ".globalSettings.lookandfeel.denseparagraphs: true\n"
          ".globalSettings.lookandfeel.enabled: true\n", f);
    fclose(f);
}

int main(int argc, char **argv) {
    static char folder[300], exe[300];
    const char *cfg, *name, *slash;

    argc = PDC_cmm2_args(argc, argv);
    cfg = getenv("PDC_CONFIG");                 /* (the program's folder, made whole, and "pdcurses.cfg") */
    if(cfg && strlen(cfg) < sizeof(folder)) {
        strcpy(folder, cfg);
        folder[strlen(folder) - strlen("pdcurses.cfg")] = 0;
        name = argv[0];
        if((slash = strrchr(name, '/')) != NULL) name = slash + 1;
        if((slash = strrchr(name, '\\')) != NULL) name = slash + 1;
        if(name[0] && name[1] == ':') name += 2;
        if(strlen(folder) + strlen(name) < sizeof(exe)) {
            strcpy(exe, folder);
            strcat(exe, name);
            setenv("WINDOWS_EXE", exe, 1);
        }
        if(strlen(folder) > 3) folder[strlen(folder) - 1] = 0;     /* (no / at the end, but for a drive's root) */
        if(getenv("HOME") == NULL) setenv("HOME", folder, 1);
        first_settings(getenv("HOME"));
    }
    return wg_main(argc, (const char **)argv);
}
