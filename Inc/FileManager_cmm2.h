/* FileManager_cmm2.h - the CMM2's platform layer for FM (Src/FileManager.c, which tools/port_fm.py makes from the
   PicoMite's misc/Editor.c): the PicoMite's names for what the CMM2 has, and stand-ins for what it has not. Only
   FileManager.c includes it (19 Oct 2026). */
#ifndef FILEMANAGER_CMM2_H
#define FILEMANAGER_CMM2_H
#include <setjmp.h>
#include "FileManager.h"

/* ---- the drives ---------------------------------------------------------------------------------------------------
   FM numbers the drives as the PicoMite does: 0 its flash drive (its A:, LittleFS), which the CMM2 has not; 1 the SD
   card, the CMM2's A: (B: on the PicoMite, and B: here as well); 2 a USB flash drive, C:. FatFSFileSystem is the
   drive FM's next FatFs call is for: InitSDCard() readies it and makes it FatFs's current drive, so that FM's paths,
   "/dir/name" from the root, are on it. The CMM2's own current drive and directory (filepath) are left as they are,
   and the next InitSDCard of the rest of MMBasic makes FatFs's current drive filepath's again. */
extern char filepath[];
extern BYTE MDD_SDSPI_CardDetectState(void);
extern bool usb_msc_is_mounted(void);
extern const char *DriveDir(char d);                    // FileIO.c: drive d's current directory, "A:/DIR"
static int FatFSFileSystem = 1;

static int fm_init_drive(int fs) {
    int ok, abort = OptionFileErrorAbort;
    char cur = filepath[0];
    if(fs == 1 && !MDD_SDSPI_CardDetectState()) return 0;
    if(fs == 2 && !usb_msc_is_mounted()) return 0;
    if(fs != 1 && fs != 2) return 0;
    OptionFileErrorAbort = 0;                           // (FM shows a drive that is not ready: it does not stop)
    filepath[0] = (fs == 2) ? 'C' : 'A';                // the CMM2's InitSDCard readies filepath's drive
    ok = InitSDCard();
    filepath[0] = cur;
    OptionFileErrorAbort = abort;
    return ok;
}
#define InitSDCard()        fm_init_drive(FatFSFileSystem)

/* ---- the PicoMite's flash drive (LittleFS) ------------------------------------------------------------------------
   FM's code for it (filesystem 0) stays as the PicoMite has it; these make it compile, and fail if it were reached
   (FM never selects drive 0 here: fm_filesystem_supported(0) is 0). */
typedef struct { int unused; } lfs_t, lfs_dir_t, lfs_file_t;
struct lfs_info { int type; uint32_t size; char name[FF_MAX_LFN + 1]; };
enum { LFS_TYPE_REG = 1, LFS_TYPE_DIR = 2 };
enum { LFS_ERR_NOENT = -2, LFS_ERR_EXIST = -17, LFS_ERR_NOTEMPTY = -39 };
enum { LFS_O_RDONLY = 1, LFS_O_WRONLY = 2, LFS_O_CREAT = 0x0100, LFS_O_TRUNC = 0x0400 };
static lfs_t lfs;
static inline int lfs_stat(lfs_t *l, const char *p, struct lfs_info *i) { return LFS_ERR_NOENT; }
static inline int lfs_dir_open(lfs_t *l, lfs_dir_t *d, const char *p) { return LFS_ERR_NOENT; }
static inline int lfs_dir_read(lfs_t *l, lfs_dir_t *d, struct lfs_info *i) { return LFS_ERR_NOENT; }
static inline int lfs_dir_close(lfs_t *l, lfs_dir_t *d) { return 0; }
static inline int lfs_getattr(lfs_t *l, const char *p, uint8_t t, void *b, uint32_t n) { return LFS_ERR_NOENT; }
static inline int lfs_rename(lfs_t *l, const char *a, const char *b) { return LFS_ERR_NOENT; }
static inline int lfs_remove(lfs_t *l, const char *p) { return LFS_ERR_NOENT; }
static inline int lfs_mkdir(lfs_t *l, const char *p) { return LFS_ERR_NOENT; }
static inline int lfs_file_open(lfs_t *l, lfs_file_t *f, const char *p, int flags) { return LFS_ERR_NOENT; }
static inline int lfs_file_close(lfs_t *l, lfs_file_t *f) { return 0; }

/* ---- the console and the display ----------------------------------------------------------------------------------
   PrintString is the serial console's (FM draws the display itself, through MX470PutS), as the PicoMite's is. */
#define PrintString(s)      SerUSBPutS((char *)(s))
extern void MX470PutS(char *s, int fc, int bc);         // kilo.c
extern void MX470Cursor(int x, int y);
extern void MX470Display(int fn);
extern void setterminal(void);
extern volatile int ConsoleRxBufHead, ConsoleRxBufTail;
extern volatile uint8_t pagesetdone;
extern char errstring[];                                // MMBasic.c's error(): the message the prompt shows
extern int errpos;
#define MMgetchar()         MMgetchar(1)
#define routinechecks()     routinechecks(1)
#define mytoupper(c)        toupper(c)
#define time_us_64()        ((uint64_t)GetuSec())

// the cursor to column x, row y on both consoles (the PicoMite's Editor.c has it)
static void SCursor(int x, int y) {
    char s[32];
    sprintf(s, "\033[%d;%dH", y + 1, x + 1);
    SerUSBPutS(s);
    MX470Cursor(x * gui_font_width, y * gui_font_height);
}

/* ---- the rest of MMBasic ------------------------------------------------------------------------------------------ */
#define AUDIO_CONFIGURED    1                           // (the CMM2 always has its audio)
extern a_flist *alist;
extern int PlayAlbumTrack(int n);                       // Audio.c
extern void cmd_newedit(void);                          // kilo.c: EDIT
extern void ListFile(char *pp, int all);                // Commands.c
extern void cmd_chdir(void);                            // FileIO.c
extern void CloseAllFiles(void);
extern void SymBindFree(void);
extern void mouse0close(void);
extern void i2c_disable(void), i2c2_disable(void), i2c3_disable(void);

#endif
