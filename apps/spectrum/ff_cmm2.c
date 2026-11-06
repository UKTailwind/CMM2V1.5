/* ff_cmm2.c: the part of FatFs pico-spec uses (stub/ff.h), on the loader's file services. pico-spec names its files from its SD card, /spec/... and /.config/..., and here that
   card is the folder the program started in: all it has is in A:/spectrum, and that is as far up as its file browser goes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmm2.h"
#include "ff.h"

static char root[128];

static const char *card(const char *path, char *buf, int n) {
    if(root[0] == 0) {
        size_t l;
        if(getcwd(root, sizeof(root)) == NULL) strcpy(root, "A:");
        l = strlen(root);
        if(l && root[l - 1] == '/') root[l - 1] = 0;
    }
    if(path[0] != '/' && path[0] != 0) return path;
    snprintf(buf, n, "%s%s", root, path);
    return buf;
}

/* what is open, for a start again */
#define HANDLES 8
static FIL *open_files[HANDLES];

void ff_close_all(void) {
    for(int h = 0; h < HANDLES; h++) {
        if(open_files[h] != NULL) cmm2->file_close(h);
        open_files[h] = NULL;
    }
}

static FRESULT result(int r) { return r >= 0 ? FR_OK : (FRESULT)-r; }

FRESULT f_mount(FATFS *fs, const TCHAR *path, BYTE opt) {
    (void)fs; (void)path; (void)opt;
    return FR_OK;                               /* (the firmware's card is mounted) */
}

FRESULT f_open(FIL *fp, const TCHAR *path, BYTE mode) {
    char b[300];
    int m = 0;
    if(mode & FA_READ) m |= CMM2_O_READ;
    if(mode & FA_WRITE) m |= CMM2_O_WRITE;
    if(mode & FA_CREATE_ALWAYS) m |= CMM2_O_CREATE;
    else if(mode & FA_OPEN_ALWAYS) m |= CMM2_O_APPEND;   /* (FA_OPEN_APPEND has this bit too) */
    else if(mode & FA_CREATE_NEW) m |= CMM2_O_CREATE;
    fp->obj.fs = NULL;
    fp->h = cmm2->file_open(card(path, b, sizeof(b)), m);
    if(fp->h < 0) return (FRESULT)-fp->h;
    fp->obj.fs = fp;
    if(fp->h < HANDLES) open_files[fp->h] = fp;
    fp->size = cmm2->file_size(fp->h);
    fp->pos = 0;
    if((mode & FA_OPEN_APPEND) == FA_OPEN_APPEND) fp->pos = fp->size;
    else if(mode & FA_OPEN_ALWAYS) cmm2->file_seek(fp->h, 0);
    return FR_OK;
}

FRESULT f_close(FIL *fp) {
    if(fp->obj.fs == NULL) return FR_OK;
    fp->obj.fs = NULL;
    if(fp->h < HANDLES) open_files[fp->h] = NULL;
    return result(cmm2->file_close(fp->h));
}

FIL *fopen2(const TCHAR *path, BYTE mode) {
    FIL *fp = malloc(sizeof(FIL));
    if(fp == NULL) return NULL;
    if(f_open(fp, path, mode) != FR_OK) {
        free(fp);
        return NULL;
    }
    return fp;
}

void fclose2(FIL *fp) {
    if(fp == NULL) return;
    f_close(fp);
    free(fp);
}

/* the menu's Hardware Info: the firmware does not say */
FRESULT f_getfree(const TCHAR *path, DWORD *nclst, FATFS **fatfs) { (void)path; (void)nclst; (void)fatfs; return FR_NOT_ENABLED; }
FRESULT f_getlabel(const TCHAR *path, TCHAR *label, DWORD *vsn) { (void)path; (void)label; (void)vsn; return FR_NOT_ENABLED; }

FRESULT f_read(FIL *fp, void *buf, UINT n, UINT *br) {
    int32_t r = cmm2->file_read(fp->h, buf, n);
    if(br) *br = r > 0 ? r : 0;
    if(r < 0) return (FRESULT)-r;
    fp->pos += r;
    return FR_OK;
}

FRESULT f_write(FIL *fp, const void *buf, UINT n, UINT *bw) {
    int32_t r = cmm2->file_write(fp->h, buf, n);
    if(bw) *bw = r > 0 ? r : 0;
    if(r < 0) return (FRESULT)-r;
    fp->pos += r;
    if(fp->pos > fp->size) fp->size = fp->pos;
    return FR_OK;
}

FRESULT f_lseek(FIL *fp, FSIZE_t ofs) {
    int r = cmm2->file_seek(fp->h, ofs);
    if(r < 0) return (FRESULT)-r;
    fp->pos = ofs;
    return FR_OK;
}

FRESULT f_sync(FIL *fp) { (void)fp; return FR_OK; }

FRESULT f_opendir(DIR *dp, const TCHAR *path) {
    char b[300];
    dp->h = cmm2->dir_open(card(path, b, sizeof(b)));
    return dp->h < 0 ? (FRESULT)-dp->h : FR_OK;
}

FRESULT f_closedir(DIR *dp) { return result(cmm2->dir_close(dp->h)); }

FRESULT f_readdir(DIR *dp, FILINFO *fno) {
    cmm2_dirent e;
    int r = cmm2->dir_read(dp->h, &e);
    if(r < 0) return (FRESULT)-r;
    memset(fno, 0, sizeof(*fno));
    if(r == 0) return FR_OK;                    /* (fname empty: the end) */
    strncpy(fno->fname, e.name, sizeof(fno->fname) - 1);
    fno->fsize = e.size;
    fno->fattrib = e.is_dir ? AM_DIR : AM_ARC;
    return FR_OK;
}

FRESULT f_stat(const TCHAR *path, FILINFO *fno) {
    char b[300];
    cmm2_stat st;
    int r = cmm2->file_stat(card(path, b, sizeof(b)), &st);
    if(r < 0) return (FRESULT)-r;
    if(fno) {
        memset(fno, 0, sizeof(*fno));
        fno->fsize = st.size;
        fno->fattrib = st.is_dir ? AM_DIR : AM_ARC;
    }
    return FR_OK;
}

FRESULT f_mkdir(const TCHAR *path) {
    char b[300];
    return result(cmm2->dir_make(card(path, b, sizeof(b))));
}

FRESULT f_unlink(const TCHAR *path) {
    char b[300];
    return result(cmm2->file_remove(card(path, b, sizeof(b))));
}

FRESULT f_rename(const TCHAR *from, const TCHAR *to) {
    char a[300], b[300];
    return result(cmm2->file_rename(card(from, a, sizeof(a)), card(to, b, sizeof(b))));
}

TCHAR *f_gets(TCHAR *buf, int len, FIL *fp) {
    int n = 0;
    char c;
    UINT got;
    while(n < len - 1) {
        if(f_read(fp, &c, 1, &got) != FR_OK || got == 0) break;
        if(c == '\r') continue;
        buf[n++] = c;
        if(c == '\n') break;
    }
    buf[n] = 0;
    return n ? buf : NULL;
}

int f_puts(const TCHAR *str, FIL *fp) {
    UINT n = strlen(str), put;
    return f_write(fp, str, n, &put) == FR_OK ? (int)put : -1;
}
