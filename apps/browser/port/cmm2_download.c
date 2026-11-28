/* cmm2_download.c: downloads for the browser (apps/browser; patches/netsurf-0007 gives this table to the framebuffer
   front end, which has none: a link to a file NetSurf cannot show did nothing, 28 Nov 2026, Peter: "Will the browser
   download files?").
     - The file goes to /downloads on the current drive (the folder made if it is not there), under the name NetSurf
       finds for it (the server's, or the URL's last part), made fit for FAT; a name in use gets (1), (2)...
     - The data are written as they come: NetSurf streams a download (hlcache.c: llcache_handle_force_stream), so its
       size is limited by the card, not the memory.
     - The status bar shows how much has come, at most twice a second, then where the file was saved; the serial
       console has a line for each, for test runs.
     - A download that fails is deleted, and the status bar says why. There is no window to cancel one in: one still
       going when the browser ends is stopped and deleted (cmm2_downloads_stop).
   MIT License, as Inc/cmm2img.h. */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cmm2.h"
#include "utils/errors.h"
#include "netsurf/download.h"
#include "netsurf/window.h"
#include "netsurf/misc.h"
#include "desktop/download.h"
#include "desktop/gui_internal.h"

#define DL_FOLDER "/downloads"
#define DL_NAME_MAX 64                          /* (FAT takes 255; the status bar and MMBasic's FILES like it short) */
#define DL_BUFFER (16 * 1024)                   /* (the file written in pieces of this) */

struct gui_download_window {
    download_context *ctx;
    struct gui_window *parent;
    FILE *f;
    char *buf;                                  /* f's buffer (setvbuf) */
    char path[sizeof(DL_FOLDER) + DL_NAME_MAX + 8];
    unsigned long long got, total;
    uint64_t shown;                             /* when the status bar was last told */
    int failed;
    int finishing;                              /* a write failed: dl_finish_later is on its way */
    uint64_t start;
    uint64_t logged;                            /* when the console was last told (every 5 s) */
    struct gui_download_window *next;           /* the downloads going (cmm2_downloads_stop) */
};

static struct gui_download_window *going;

static void say(struct gui_download_window *dw, const char *text) {
    if(dw->parent != NULL) guit->window->set_status(dw->parent, text);
    fprintf(stderr, "browser: %s\n", text);
}

/* name: NetSurf's, with what FAT refuses made _, cut to DL_NAME_MAX keeping its extension */
static void fit_name(char *out, const char *name) {
    const char *dot;
    size_t n, ext;
    char *p;
    if(name == NULL || *name == 0) name = "download";
    while(*name == ' ' || *name == '.') name++;
    if(*name == 0) name = "download";
    n = strlen(name);
    dot = strrchr(name, '.');
    ext = dot != NULL && strlen(dot) <= 10 ? strlen(dot) : 0;
    if(n > DL_NAME_MAX) {
        memcpy(out, name, DL_NAME_MAX - ext);
        memcpy(out + DL_NAME_MAX - ext, name + n - ext, ext);
        out[DL_NAME_MAX] = 0;
    } else strcpy(out, name);
    for(p = out; *p; p++)
        if((unsigned char)*p < 32 || strchr("\\/:*?\"<>|", *p)) *p = '_';
    for(p = out + strlen(out); p > out && (p[-1] == ' ' || p[-1] == '.'); ) *--p = 0;
    if(*out == 0) strcpy(out, "download");
}

static struct gui_download_window *dl_create(download_context *ctx, struct gui_window *parent) {
    struct gui_download_window *dw;
    struct stat st;
    char name[DL_NAME_MAX + 1], *dot;
    int i;
    if((dw = calloc(1, sizeof(*dw))) == NULL) return NULL;
    dw->ctx = ctx;
    dw->parent = parent;
    dw->total = download_context_get_total_length(ctx);
    fit_name(name, download_context_get_filename(ctx));
    if(stat(DL_FOLDER, &st) != 0) mkdir(DL_FOLDER, 0777);
    snprintf(dw->path, sizeof(dw->path), DL_FOLDER "/%s", name);
    dot = strrchr(name, '.');
    for(i = 1; stat(dw->path, &st) == 0 && i < 100; i++)   /* (a name in use: name(1).ext, name(2).ext...) */
        snprintf(dw->path, sizeof(dw->path), DL_FOLDER "/%.*s(%d)%s", dot ? (int)(dot - name) : (int)strlen(name),
                 name, i, dot ? dot : "");
    dw->buf = malloc(DL_BUFFER);
    if((dw->f = fopen(dw->path, "wb")) == NULL) {
        char t[sizeof(dw->path) + 40];
        snprintf(t, sizeof(t), "Download: %s could not be made", dw->path);
        say(dw, t);
        free(dw->buf);
        free(dw);
        return NULL;                            /* (download.c then gives the download up) */
    }
    if(dw->buf != NULL) setvbuf(dw->f, dw->buf, _IOFBF, DL_BUFFER);
    {
        char t[sizeof(dw->path) + 40];
        snprintf(t, sizeof(t), "Downloading %s", dw->path);
        say(dw, t);
    }
    dw->shown = dw->start = dw->logged = cmm2->micros();
    dw->next = going;
    going = dw;
    return dw;
}

/* the file closed, and deleted if it failed; the window freed and the context destroyed: from the main loop when it
   failed in data (download.c is still using the context then) */
static void dl_finish(struct gui_download_window *dw) {
    struct gui_download_window **l;
    for(l = &going; *l != NULL; l = &(*l)->next)
        if(*l == dw) {
            *l = dw->next;
            break;
        }
    if(dw->f != NULL) {
        if(fclose(dw->f) != 0) dw->failed = 1;
        dw->f = NULL;
    }
    if(dw->failed) remove(dw->path);
    free(dw->buf);
    download_context_destroy(dw->ctx);
    free(dw);
}

static void dl_finish_later(void *p) {
    dl_finish(p);
}

static nserror dl_data(struct gui_download_window *dw, const char *data, unsigned int size) {
    uint64_t now;
    if(dw->failed) return NSERROR_SAVE_FAILED;
    if(fwrite(data, 1, size, dw->f) != size) {
        char t[sizeof(dw->path) + 100];
        dw->failed = 1;
        snprintf(t, sizeof(t), "Download of %s failed: the card is full, or could not be written", dw->path);
        say(dw, t);
        dw->finishing = 1;
        guit->misc->schedule(0, dl_finish_later, dw);   /* (download.c aborts the fetch, and says no more) */
        return NSERROR_SAVE_FAILED;
    }
    dw->got += size;
    now = cmm2->micros();
    if(now - dw->logged >= 5000000u) {          /* (the console: how it goes, for test runs) */
        dw->logged = now;
        fprintf(stderr, "browser: %s: %lu KB in %lu s\n", dw->path, (unsigned long)(dw->got / 1024),
                (unsigned long)((now - dw->start) / 1000000u));
    }
    if(now - dw->shown >= 500000u && dw->parent != NULL) {
        char t[sizeof(dw->path) + 100];
        dw->shown = now;
        if(dw->total > 0)
            snprintf(t, sizeof(t), "Downloading %s: %lu KB of %lu", dw->path, (unsigned long)(dw->got / 1024),
                     (unsigned long)(dw->total / 1024));   /* (newlib-nano prints no long long) */
        else snprintf(t, sizeof(t), "Downloading %s: %lu KB", dw->path, (unsigned long)(dw->got / 1024));
        guit->window->set_status(dw->parent, t);
    }
    return NSERROR_OK;
}

static void dl_error(struct gui_download_window *dw, const char *msg) {
    char t[sizeof(dw->path) + 120];
    if(dw->finishing) return;
    dw->failed = 1;
    snprintf(t, sizeof(t), "Download of %s failed: %s", dw->path, msg != NULL ? msg : "?");
    say(dw, t);
    dl_finish(dw);
}

static void dl_done(struct gui_download_window *dw) {
    char t[sizeof(dw->path) + 100], cwd[264];
    if(dw->finishing) return;
    if(fflush(dw->f) != 0) dw->failed = 1;
    if(dw->failed) {
        snprintf(t, sizeof(t), "Download of %s failed: the card is full, or could not be written", dw->path);
        say(dw, t);
    } else {
        if(getcwd(cwd, sizeof(cwd)) == NULL || cwd[1] != ':') cwd[0] = 0;
        else cwd[2] = 0;
        uint64_t us = cmm2->micros() - dw->start;
        snprintf(t, sizeof(t), "Saved %s%s (%lu KB in %lu.%lu s)", cwd, dw->path,
                 (unsigned long)((dw->got + 1023) / 1024), (unsigned long)(us / 1000000u),
                 (unsigned long)(us / 100000u % 10u));
        say(dw, t);
    }
    dl_finish(dw);
}

static struct gui_download_table download_table = {
    .create = dl_create,
    .data = dl_data,
    .error = dl_error,
    .done = dl_done,
};

struct gui_download_table *cmm2_download_table = &download_table;

/* (patches/netsurf-0007: the browser ending) the downloads still going stopped and their part files deleted, before
   NetSurf's caches are finalised: their fetches went on, and the first data after stopped the browser with llcache's
   assert (28 Nov 2026) */
void cmm2_downloads_stop(void) {
    while(going != NULL) {
        struct gui_download_window *dw = going;
        char t[sizeof(dw->path) + 60];
        dw->parent = NULL;                      /* (the console only: the window is going) */
        snprintf(t, sizeof(t), "Download of %s stopped: the browser ended", dw->path);
        say(dw, t);
        dw->failed = 1;
        download_context_abort(dw->ctx);
        dl_finish(dw);                          /* (which takes it off the list) */
    }
}
