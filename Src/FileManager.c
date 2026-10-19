/***********************************************************************************************************************
FileManager.c - FM, the PicoMite's two-panel file manager (misc/Editor.c, its MMBASIC_FM part), for the CMM2 (Peter,
19 Oct 2026). Made by tools/port_fm.py from the PicoMite's source, then fixed by hand where the CMM2 differs; the
platform layer is FileManager_cmm2.h. The CMM2's drives: A: (or B:, the PicoMite's name for it) the SD card, C: a
USB flash drive; the PicoMite's A:, its flash drive, does not exist here.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "FileManager_cmm2.h"

#define FM_MIN_WIDTH 50
#define FM_CONTEXT_FILE "/.fm"
#define FM_CONTEXT_MAGIC "FMCTX1"
#define FM_VOLUME_STEP 5
#define FM_SORT_NAME 0
#define FM_SORT_TIME 1
#define FM_SORT_TYPE 2
#define FM_MIN_CACHE_NAME_CHARS 32
// Buffers that hold a constructed full or child path (drive + directory + '/' +
// filename) must be sized for a whole path, not a single name. FF_MAX_LFN is the
// limit for one filename component only (63 on RP2040), so building a path into an
// FF_MAX_LFN buffer overflows and falsely reports "Path too long". Use FM_PATH_MAX
// for any buffer that receives the result of fm_make_child_path/fm_make_full_filename
// or otherwise stores a complete path.
//
// Size = worst-case single-level full filename produced by fm_make_full_filename:
//   "X:" + panel->path(<= FF_MAX_LFN-1) + "/" + name(<= FF_MAX_LFN) + NUL
// = FF_MAX_LFN*2 + 5, rounded up with margin. Deliberately NOT STRINGSIZE(256):
// fm_copy_dir_recursive recurses with four of these buffers per frame, so worst-case
// stack is ~quadratic in this value; FF_MAX_LFN*2 keeps it bounded while still holding
// any legal path (deeper recursive paths fail gracefully via fm_make_child_path).
#define FM_PATH_MAX (FF_MAX_LFN * 2 + 8)

typedef struct
{
    char *name;
    int is_dir;
    uint32_t size;
    uint16_t fdate;
    uint16_t ftime;
} fm_entry_t;

typedef struct
{
    int filesystem;
    char path[FF_MAX_LFN];
    char filter[FF_MAX_LFN];
    int selected;
    int top;
    int count;
    int sortorder;
    fm_entry_t *entries;
    char *name_pool;
    int name_stride;
    int *type_next;
    char *marks;
    /* Heap-allocated with the rest of the panel cache.  This was an inline
       int[256] - 1 KB per panel, 2 KB across the two panels in cmd_fm - which
       helped push cmd_fm's frame to 5824 bytes, 72% of the 8 KB core0 stack.
       A recursive directory copy then ran the stack past __StackLimit into the
       C heap and corrupted malloc's arena, surfacing as a silent panic
       ("Out of memory") - see the dr_wav/dr_flac stack arrays for the same
       remedy: move the big buffer to the MMBasic heap. */
    int *type_head;
    int entry_count;
    int marked_count;
    int cache_valid;
} fm_panel_t;

#define FM_TYPE_HEAD_N 256
#define FM_TYPE_HEAD_BYTES ((int)(FM_TYPE_HEAD_N * sizeof(int)))

typedef struct
{
    char name[FF_MAX_LFN + 1];
    int is_dir;
} fm_mark_item_t;

// what the rest of MMBasic sees (FileManager.h): the PicoMite has these in Commands.c
int fm_active = 0;
int fm_program_launched_from_fm = 0;
char fm_relaunch_status[STRINGSIZE] = {0};
int fm_relaunch_status_valid = 0;
int fm_editing = 0;
char fm_last_launched_bas[FF_MAX_LFN] = {0};
static char fm_error_file[FF_MAX_LFN] = {0};
static int fm_error_line = 0;
static int fm_error_char = 0;
static int fm_error_location_valid = 0;
int fm_pending_edit_seek_valid = 0;
int fm_pending_edit_seek_line = 0;
int fm_pending_edit_seek_char = 0;

static const char *FM_ANSI_NORMAL = "\033[37;40m";
static const char *FM_ANSI_TITLE = "\033[36;40m";
static const char *FM_ANSI_ACTIVE = "\033[30;46m";
static const char *FM_ANSI_INACTIVE = "\033[30;47m";
static const char *FM_ANSI_STATUS = "\033[33;40m";
static unsigned char fm_break_key_for_list = BREAK_KEY;
static int fm_skip_serial_output = 0;
static int fm_local_cache_valid = 0;
static int fm_local_cache_w = 0;
static int fm_local_cache_h = 0;
static char *fm_local_chars = NULL;
static char *fm_local_fc = NULL;
static char *fm_local_bc = NULL;

static int fm_copy_file_path(int srcfs, const char *srcfile, int dstfs, const char *dstfile,
                             char *status, int statuslen, int *file_count, int *dir_count);
static int fm_path_contains_icase(const char *parent, const char *child);
static int fm_copy_dir_recursive(int srcfs, const char *srcdir, int dstfs, const char *dstdir,
                                 char *status, int statuslen, int *file_count, int *dir_count);
static int fm_get_entry_at(fm_panel_t *panel, int index, fm_entry_t *entry, char *errmsg, int errmsglen);
static const char *fm_top_help_line(int width);
static void fm_format_panel_entry(const fm_panel_t *panel, int idx, char *out, int outlen);

static void fm_local_cache_release(void)
{
    FreeMemorySafe((void **)&fm_local_chars);
    FreeMemorySafe((void **)&fm_local_fc);
    FreeMemorySafe((void **)&fm_local_bc);
    fm_local_cache_w = 0;
    fm_local_cache_h = 0;
    fm_local_cache_valid = 0;
}

static void fm_local_cache_invalidate(void)
{
    fm_local_cache_valid = 0;
}

static void fm_local_cache_prepare(void)
{
    int cells;

    if (Option.Width <= 0 || Option.Height <= 0)
    {
        fm_local_cache_valid = 0;
        return;
    }

    if (fm_local_cache_w != Option.Width || fm_local_cache_h != Option.Height ||
        fm_local_chars == NULL || fm_local_fc == NULL || fm_local_bc == NULL)
    {
        fm_local_cache_release();
        cells = Option.Width * Option.Height;
        fm_local_chars = (char *)GetMemory(cells);
        fm_local_fc = (char *)GetMemory(cells);
        fm_local_bc = (char *)GetMemory(cells);
        fm_local_cache_w = Option.Width;
        fm_local_cache_h = Option.Height;
        fm_local_cache_valid = 0;
    }

    if (!fm_local_cache_valid)
    {
        memset(fm_local_chars, 0, fm_local_cache_w * fm_local_cache_h);
        memset(fm_local_fc, 0xFF, fm_local_cache_w * fm_local_cache_h);
        memset(fm_local_bc, 0xFF, fm_local_cache_w * fm_local_cache_h);
        fm_local_cache_valid = 1;
    }
}

/* Fold a 24-bit RGB colour into a single distinct byte (RGB3:3:2) for the
 * local-display dirty-check cache.  A plain (unsigned char) cast kept only the
 * blue channel, so CYAN (0x00FFFF) and WHITE (0xFFFFFF) - the active/inactive
 * highlight pair - both stored 0xFF.  That made an active<->inactive colour
 * change with unchanged text invisible to the dirty check, so the drive header
 * bar never repainted on Tab on the HDMI panel (serial has no cache and was
 * fine).  RGB3:3:2 keeps the whole FM palette (BLACK/WHITE/CYAN/YELLOW) distinct. */
static inline unsigned char fm_color_key(int c)
{
    unsigned int u = (unsigned int)c;
    return (unsigned char)(((u >> 16) & 0xE0) | (((u >> 8) & 0xE0) >> 3) | ((u & 0xFF) >> 6));
}

static inline char fm_drive_letter(int filesystem)
{
    return filesystem == 2 ? 'C' : 'A';                 // CMM2: the SD card (drive 1) is A:, as B: is
}

static int fm_path_is_root(const char *path)
{
    return path == NULL || path[0] == 0 || strcmp(path, "/") == 0;
}

static void fm_set_panel_path_from_cwd(fm_panel_t *panel)
{
    const char *src = DriveDir(fm_drive_letter(panel->filesystem)) + 2;   // CMM2: each drive's current directory
    if (src[0] == 0)
    {
        strcpy(panel->path, "/");
        return;
    }
    if (src[0] != '/')
    {
        panel->path[0] = '/';
        panel->path[1] = 0;
        strncat(panel->path, src, FF_MAX_LFN - 2);
    }
    else
    {
        strncpy(panel->path, src, FF_MAX_LFN - 1);
        panel->path[FF_MAX_LFN - 1] = 0;
    }
}

static void fm_parent_path(char *path)
{
    int i;
    if (fm_path_is_root(path))
        return;
    i = strlen(path) - 1;
    while (i > 0 && path[i] != '/')
        i--;
    if (i <= 0)
        strcpy(path, "/");
    else
        path[i] = 0;
}

static int fm_make_child_path(const char *base, const char *name, char *out, int outlen)
{
    if (fm_path_is_root(base))
        return snprintf(out, outlen, "/%s", name) < outlen;
    return snprintf(out, outlen, "%s/%s", base, name) < outlen;
}

static int fm_make_full_filename(int filesystem, const char *path, const char *name, char *out, int outlen)
{
    if (fm_path_is_root(path))
        return snprintf(out, outlen, "%c:/%s", fm_drive_letter(filesystem), name) < outlen;
    return snprintf(out, outlen, "%c:%s/%s", fm_drive_letter(filesystem), path, name) < outlen;
}

static int fm_stricmp(const char *a, const char *b)
{
    for (;; a++, b++)
    {
        int d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a)
            return d;
    }
}

static void fm_flush_keyboard_buffer(void)
{
    // Reset repeat state and drain queued keycodes accumulated during redraw.
    clearrepeat();
    while (MMInkey() != -1)
        ;
}

static int fm_name_is_bas(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot != NULL && fm_stricmp(dot, ".bas") == 0;
}

static int fm_panel_name_limit(const fm_panel_t *panel)
{
    int limit = FM_MIN_CACHE_NAME_CHARS;

    if (panel && panel->name_stride > 1)
    {
        int observed = panel->name_stride - 1;
        if (observed > limit)
            limit = observed;
    }
    if (limit > FF_MAX_LFN)
        limit = FF_MAX_LFN;
    return limit;
}

enum
{
    FM_AUDIO_NONE = 0,
    FM_AUDIO_WAV,
    FM_AUDIO_FLAC,
    FM_AUDIO_MP3,
    FM_AUDIO_MIDI,
    FM_AUDIO_MOD
};

static int fm_name_audio_type(const char *name)
{
    const char *dot = strrchr(name, '.');

    if (dot == NULL)
        return FM_AUDIO_NONE;
    if (fm_stricmp(dot, ".wav") == 0)
        return FM_AUDIO_WAV;
    if (fm_stricmp(dot, ".flac") == 0)
        return FM_AUDIO_FLAC;
    if (fm_stricmp(dot, ".mp3") == 0)
        return FM_AUDIO_MP3;
    if (fm_stricmp(dot, ".mid") == 0 || fm_stricmp(dot, ".midi") == 0)
        return FM_AUDIO_MIDI;
    if (fm_stricmp(dot, ".mod") == 0)
        return FM_AUDIO_MOD;
    return FM_AUDIO_NONE;
}

enum
{
    FM_IMAGE_NONE = 0,
    FM_IMAGE_BMP,
    FM_IMAGE_JPG,
    FM_IMAGE_PNG
};

static int fm_name_image_type(const char *name)
{
    const char *dot = strrchr(name, '.');

    if (dot == NULL)
        return FM_IMAGE_NONE;
    if (fm_stricmp(dot, ".bmp") == 0)
        return FM_IMAGE_BMP;
    if (fm_stricmp(dot, ".jpg") == 0 || fm_stricmp(dot, ".jpeg") == 0)
        return FM_IMAGE_JPG;
    if (fm_stricmp(dot, ".png") == 0)
        return FM_IMAGE_PNG;
    return FM_IMAGE_NONE;
}

static void fm_wait_for_escape(void)
{
    int c = -1;

    while (1)
    {
        routinechecks();
        c = MMInkey();
        if (c != -1)
            break;
    }
}

static int fm_drive_available(int filesystem)
{
    int oldfs = FatFSFileSystem;
    int oldabort = OptionFileErrorAbort;
    int ok;

    if (filesystem == 0)
        return 0;                                       // CMM2: no flash drive (the SD card's slot is fixed)

    FatFSFileSystem = filesystem;
    OptionFileErrorAbort = 0;
    ok = InitSDCard();
    OptionFileErrorAbort = oldabort;
    FatFSFileSystem = oldfs;
    return ok ? 1 : 0;
}

static int fm_filesystem_supported(int filesystem)
{
    if (filesystem == 1)                                // CMM2: A: (or B:), the SD card, and C:
        return 1;
    if (filesystem == 2)
        return 1;
    return 0;
}

static int fm_path_exists_dir(int filesystem, const char *path)
{
    int oldfs = FatFSFileSystem;
    int oldabort = OptionFileErrorAbort;
    int ok = 0;

    if (!fm_filesystem_supported(filesystem))
        return 0;
    if (filesystem != 0 && !fm_drive_available(filesystem))
        return 0;
    if (path == NULL || path[0] == 0 || strcmp(path, "/") == 0)
        return 1;
    if (path[0] != '/')
        return 0;

    FatFSFileSystem = filesystem;
    if (filesystem == 0)
    {
        struct lfs_info info;
        memset(&info, 0, sizeof(info));
        if (lfs_stat(&lfs, path, &info) == 0 && info.type == LFS_TYPE_DIR)
            ok = 1;
    }
    else
    {
        FILINFO info;
        FRESULT fr;
        char q[FF_MAX_LFN];
        size_t qlen;
        memset(&info, 0, sizeof(info));
        strncpy(q, path, sizeof(q) - 1);
        q[sizeof(q) - 1] = 0;
        qlen = strlen(q);
        if (qlen > 0 && q[qlen - 1] == '/' && qlen + 1 < sizeof(q))
            strcat(q, ".");
        OptionFileErrorAbort = 0;
        fr = f_stat(q, &info);
        OptionFileErrorAbort = oldabort;
        if (fr == FR_OK && (info.fattrib & AM_DIR))
            ok = 1;
    }

    OptionFileErrorAbort = oldabort;
    FatFSFileSystem = oldfs;
    return ok;
}

static int fm_parse_panel_context_line(const char *line, fm_panel_t *panel)
{
    char tmp[FF_MAX_LFN * 2 + 64];
    char *f0, *f1, *f2, *f3, *f4, *f5;
    int fs, sel, top, sortorder = FM_SORT_NAME;

    if (line == NULL || panel == NULL)
        return 0;
    strncpy(tmp, line, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    f0 = tmp;
    f1 = strchr(f0, '|');
    if (!f1)
        return 0;
    *f1++ = 0;
    f2 = strchr(f1, '|');
    if (!f2)
        return 0;
    *f2++ = 0;
    f3 = strchr(f2, '|');
    if (!f3)
        return 0;
    *f3++ = 0;
    f4 = strchr(f3, '|');
    if (!f4)
        return 0;
    *f4++ = 0;
    f5 = strchr(f4, '|');
    if (f5)
    {
        *f5++ = 0;
        sortorder = atoi(f5);
    }

    fs = atoi(f0);
    sel = atoi(f3);
    top = atoi(f4);

    if (!fm_filesystem_supported(fs))
        return 0;
    if (!fm_drive_available(fs))
        return 0;
    if (!fm_path_exists_dir(fs, f1))
        return 0;

    panel->filesystem = fs;
    strncpy(panel->path, f1, FF_MAX_LFN - 1);
    panel->path[FF_MAX_LFN - 1] = 0;
    if (f2[0])
    {
        strncpy(panel->filter, f2, FF_MAX_LFN - 1);
        panel->filter[FF_MAX_LFN - 1] = 0;
    }
    else
        strcpy(panel->filter, "*");
    panel->selected = sel < 0 ? 0 : sel;
    panel->top = top < 0 ? 0 : top;
    panel->count = 0;
    panel->sortorder = (sortorder >= FM_SORT_NAME && sortorder <= FM_SORT_TYPE) ? sortorder : FM_SORT_NAME;
    panel->entries = NULL;
    panel->name_pool = NULL;
    panel->name_stride = 0;
    panel->type_next = NULL;
    panel->marks = NULL;
    panel->type_head = NULL;
    panel->entry_count = 0;
    panel->marked_count = 0;
    panel->cache_valid = 0;
    return 1;
}

/* CMM2: the context is kept in RAM, until a restart (the PicoMite writes FM_CONTEXT_FILE on its flash drive; a file on
   the SD card would be in FM's listings) */
static char fm_context[FF_MAX_LFN * 2 + 256];

static int fm_load_context(fm_panel_t *panels, int *active)
{
    char buf[FF_MAX_LFN * 2 + 256];
    char *line;
    int restored_any = 0;
    int degraded = 0;

    if (!panels || !active)
        return 0;

    if (fm_context[0] == 0)
        return 0;
    strcpy(buf, fm_context);
    line = strtok(buf, "\r\n");
    if (!line || strcmp(line, FM_CONTEXT_MAGIC) != 0)
        return 0;

    while ((line = strtok(NULL, "\r\n")) != NULL)
    {
        if (strncmp(line, "P0=", 3) == 0)
        {
            if (fm_parse_panel_context_line(line + 3, &panels[0]))
                restored_any = 1;
            else
                degraded = 1;
        }
        else if (strncmp(line, "P1=", 3) == 0)
        {
            if (fm_parse_panel_context_line(line + 3, &panels[1]))
                restored_any = 1;
            else
                degraded = 1;
        }
        else if (strncmp(line, "ACTIVE=", 7) == 0)
        {
            int a = atoi(line + 7);
            if (a == 0 || a == 1)
                *active = a;
            else
                degraded = 1;
        }
    }

    return restored_any ? (degraded ? 2 : 1) : 0;
}

static void fm_save_context(const fm_panel_t *panels, int active)
{
    char data[FF_MAX_LFN * 2 + 256] = {0};
    int n;

    if (!panels)
        return;

    n = snprintf(data, sizeof(data),
                 "%s\n"
                 "P0=%d|%s|%s|%d|%d|%d\n"
                 "P1=%d|%s|%s|%d|%d|%d\n"
                 "ACTIVE=%d\n",
                 FM_CONTEXT_MAGIC,
                 panels[0].filesystem,
                 panels[0].path,
                 panels[0].filter,
                 panels[0].selected < 0 ? 0 : panels[0].selected,
                 panels[0].top < 0 ? 0 : panels[0].top,
                 (panels[0].sortorder >= FM_SORT_NAME && panels[0].sortorder <= FM_SORT_TYPE) ? panels[0].sortorder : FM_SORT_NAME,
                 panels[1].filesystem,
                 panels[1].path,
                 panels[1].filter,
                 panels[1].selected < 0 ? 0 : panels[1].selected,
                 panels[1].top < 0 ? 0 : panels[1].top,
                 (panels[1].sortorder >= FM_SORT_NAME && panels[1].sortorder <= FM_SORT_TYPE) ? panels[1].sortorder : FM_SORT_NAME,
                 (active == 1) ? 1 : 0);

    if (n <= 0 || n >= (int)sizeof(data))
        return;
    strcpy(fm_context, data);
}

static void fm_draw_field(int x, int y, int width, const char *text, const char *ansi, int fc, int bc)
{
    char line[STRINGSIZE * 2];
    int n;
    /* Audio launched from FM uses a producer/consumer double buffer fed by
     * checkWAVinput().  A full FM redraw can spend tens to hundreds of ms in
     * filesystem reads, serial blasts and framebuffer paints; if no buffer
     * refill happens in that window the IRQ replays the same buffer and the
     * playback stutters at startup.  Pump the audio at every field draw -
     * this is the lowest-level call site shared by every FM repaint path.   */
    if (CurrentlyPlaying != P_NOTHING)
        checkWAVinput();
    if (width <= 0)
        return;
    if (width >= (int)sizeof(line))
        width = sizeof(line) - 1;
    memset(line, ' ', width);
    line[width] = 0;
    if (text != NULL)
    {
        n = strlen(text);
        if (n > width)
            n = width;
        memcpy(line, text, n);
    }
    if (!fm_skip_serial_output)
    {
        SCursor(x, y);
        PrintString((char *)ansi);
        PrintString(line);
        PrintString("\033[0m");
    }
    if ((OptionConsole & 2))
    {
        int draw_needed = 1;
        int cached_width = width;
        int maxw;

        fm_local_cache_prepare();
        if (fm_local_cache_valid && x >= 0 && x < fm_local_cache_w && y >= 0 && y < fm_local_cache_h)
        {
            int i;
            maxw = fm_local_cache_w - x;
            if (x + cached_width > fm_local_cache_w)
                cached_width = maxw;
            for (i = 0; i < cached_width; i++)
            {
                int idx = y * fm_local_cache_w + (x + i);
                if (fm_local_chars[idx] != line[i] ||
                    fm_local_fc[idx] != fm_color_key(fc) ||
                    fm_local_bc[idx] != fm_color_key(bc))
                {
                    draw_needed = 1;
                    break;
                }
                draw_needed = 0;
            }
            if (cached_width <= 0)
                draw_needed = 1;
        }

        if (draw_needed)
        {
            MX470Cursor(x * gui_font_width, y * gui_font_height);
            MX470PutS(line, fc, bc);
        }

        if (fm_local_cache_valid && x >= 0 && x < fm_local_cache_w && y >= 0 && y < fm_local_cache_h)
        {
            int i;
            maxw = fm_local_cache_w - x;
            if (x + cached_width > fm_local_cache_w)
                cached_width = maxw;
            for (i = 0; i < cached_width; i++)
            {
                int idx = y * fm_local_cache_w + (x + i);
                fm_local_chars[idx] = line[i];
                fm_local_fc[idx] = fm_color_key(fc);
                fm_local_bc[idx] = fm_color_key(bc);
            }
        }
    }
}

/* The cursor of a status-line prompt.  FM hides the terminal's cursor when it
   opens (ESC[?25l), and a field paint leaves both consoles' text position after
   the padded line, so a prompt that reads keys shows the cursor where the
   typing goes - the terminal's through SCursor and ESC[?25h, the local
   display's through the text position MMgetchar's ShowCursor draws at - and
   hides it again when it has read its key. */
static void fm_prompt_cursor(int col, int show)
{
    if (col > Option.Width - 1)
        col = Option.Width - 1;
    SCursor(col, Option.Height - 1);
    PrintString(show ? "[?25h" : "[?25l");
}

static void fm_fill_padded_line(char *out, int width, const char *text)
{
    int n = 0;

    if (width <= 0)
        return;
    memset(out, ' ', width);
    out[width] = 0;

    if (text != NULL)
    {
        n = strlen(text);
        if (n > width)
            n = width;
        if (n > 0)
            memcpy(out, text, n);
    }
}

static void fm_serial_repaint_ui(fm_panel_t *panels, int active, const char *status,
                                 int left_ok, const char *left_err,
                                 int right_ok, const char *right_err)
{
    char line[STRINGSIZE * 2];
    char left_line[STRINGSIZE * 2];
    char right_line[STRINGSIZE * 2];
    char left_text[STRINGSIZE * 2];
    char right_text[STRINGSIZE * 2];
    int list_rows = Option.Height - 4;
    int left_width = (Option.Width - 1) / 2;
    int right_width = Option.Width - (left_width + 1);
    int row;

    if (Option.Width <= 0 || Option.Height <= 0)
        return;

    PrintString("\033[H");

    fm_fill_padded_line(line, Option.Width, fm_top_help_line(Option.Width));
    PrintString((char *)FM_ANSI_TITLE);
    PrintString(line);
    PrintString("\033[0m");

    if (Option.Height > 1)
        PrintString("\r\n");

    snprintf(left_text, sizeof(left_text), "%c:%s [%s]",
             fm_drive_letter(panels[0].filesystem), panels[0].path, panels[0].filter);
    snprintf(right_text, sizeof(right_text), "%c:%s [%s]",
             fm_drive_letter(panels[1].filesystem), panels[1].path, panels[1].filter);
    fm_fill_padded_line(left_line, left_width, left_text);
    fm_fill_padded_line(right_line, right_width, right_text);

    PrintString((char *)(active == 0 ? FM_ANSI_ACTIVE : FM_ANSI_INACTIVE));
    PrintString(left_line);
    PrintString((char *)FM_ANSI_TITLE);
    PrintString("|");
    PrintString((char *)(active == 1 ? FM_ANSI_ACTIVE : FM_ANSI_INACTIVE));
    PrintString(right_line);
    PrintString("\033[0m");

    for (row = 0; row < list_rows; row++)
    {
        int li = panels[0].top + row;
        int ri = panels[1].top + row;
        const char *left_ansi = FM_ANSI_NORMAL;
        const char *right_ansi = FM_ANSI_NORMAL;

        /* Pump the audio double-buffer: serial PrintString of the full
         * two-panel UI can take 100+ ms on slow consoles, long enough for
         * the IRQ to drain the only buffer the file callback primed and
         * begin replaying it (the audible "warble" at start of playback). */
        if (CurrentlyPlaying != P_NOTHING)
            checkWAVinput();

        if (Option.Height > 2 || row > 0)
            PrintString("\r\n");

        left_text[0] = 0;
        if (!left_ok)
        {
            if (row == 0 && left_err)
                snprintf(left_text, sizeof(left_text), "%s", left_err);
            left_ansi = FM_ANSI_STATUS;
        }
        else if (li >= 0 && li < panels[0].count)
        {
            fm_format_panel_entry(&panels[0], li, left_text, sizeof(left_text));
            if (li == panels[0].selected)
                left_ansi = active == 0 ? FM_ANSI_ACTIVE : FM_ANSI_INACTIVE;
        }
        fm_fill_padded_line(left_line, left_width, left_text);

        right_text[0] = 0;
        if (!right_ok)
        {
            if (row == 0 && right_err)
                snprintf(right_text, sizeof(right_text), "%s", right_err);
            right_ansi = FM_ANSI_STATUS;
        }
        else if (ri >= 0 && ri < panels[1].count)
        {
            fm_format_panel_entry(&panels[1], ri, right_text, sizeof(right_text));
            if (ri == panels[1].selected)
                right_ansi = active == 1 ? FM_ANSI_ACTIVE : FM_ANSI_INACTIVE;
        }
        fm_fill_padded_line(right_line, right_width, right_text);

        PrintString((char *)left_ansi);
        PrintString(left_line);
        PrintString((char *)FM_ANSI_TITLE);
        PrintString("|");
        PrintString((char *)right_ansi);
        PrintString(right_line);
        PrintString("\033[0m");
    }

    if (Option.Height > 4)
        PrintString("\r\n\r\n");
    fm_fill_padded_line(line, Option.Width, status ? status : "");
    PrintString((char *)FM_ANSI_STATUS);
    PrintString(line);
    PrintString("\033[0m");
}

static void fm_free_panel_cache(fm_panel_t *panel)
{
    if (!panel)
        return;
    FreeMemorySafe((void **)&panel->entries);
    FreeMemorySafe((void **)&panel->name_pool);
    FreeMemorySafe((void **)&panel->type_next);
    FreeMemorySafe((void **)&panel->marks);
    FreeMemorySafe((void **)&panel->type_head);
    panel->name_stride = 0;
    panel->entry_count = 0;
    panel->count = 0;
    panel->marked_count = 0;
    panel->cache_valid = 0;
}

// If the inactive panel is showing the same directory as the active panel,
// invalidate its cache so it refreshes on the next redraw.
static void fm_sync_other_panel_cache(fm_panel_t *panels, int active)
{
    fm_panel_t *cur = &panels[active];
    fm_panel_t *other = &panels[active ^ 1];
    if (other->filesystem == cur->filesystem &&
        fm_stricmp(other->path, cur->path) == 0)
        other->cache_valid = 0;
}

static const char *fm_find_extension(const char *name)
{
    const char *dot = NULL;

    while (*name)
    {
        if (*name == '.')
            dot = name;
        name++;
    }
    return dot;
}

static int fm_entry_cmp_name(const void *a, const void *b)
{
    const fm_entry_t *ea = (const fm_entry_t *)a;
    const fm_entry_t *eb = (const fm_entry_t *)b;

    if (ea->is_dir != eb->is_dir)
        return eb->is_dir - ea->is_dir;
    return fm_stricmp(ea->name, eb->name);
}

static int fm_entry_cmp_time(const void *a, const void *b)
{
    const fm_entry_t *ea = (const fm_entry_t *)a;
    const fm_entry_t *eb = (const fm_entry_t *)b;
    uint32_t da = ((uint32_t)ea->fdate << 16) | ea->ftime;
    uint32_t db = ((uint32_t)eb->fdate << 16) | eb->ftime;

    if (da < db)
        return 1;
    if (da > db)
        return -1;
    return fm_entry_cmp_name(a, b);
}

static int fm_entry_cmp_type(const void *a, const void *b)
{
    const fm_entry_t *ea = (const fm_entry_t *)a;
    const fm_entry_t *eb = (const fm_entry_t *)b;
    const char *xa, *xb;
    int r;

    if (ea->is_dir != eb->is_dir)
        return eb->is_dir - ea->is_dir;

    xa = fm_find_extension(ea->name);
    xb = fm_find_extension(eb->name);

    if (!xa && !xb)
        return fm_stricmp(ea->name, eb->name);
    if (!xa)
        return -1;
    if (!xb)
        return 1;

    r = fm_stricmp(xa, xb);
    if (r != 0)
        return r;
    return fm_stricmp(ea->name, eb->name);
}

static int fm_build_panel_cache(fm_panel_t *panel, char *errmsg, int errmsglen)
{
    int oldfs = FatFSFileSystem;
    int count = 1;
    int idx = 1;
    int max_name_len = FM_MIN_CACHE_NAME_CHARS;
    int i;

    if (!panel)
        return 0;
    if (errmsglen > 0)
        errmsg[0] = 0;

    if (panel->cache_valid)
        return 1;

    fm_free_panel_cache(panel);

    FatFSFileSystem = panel->filesystem;
    if (!fm_drive_available(panel->filesystem))
    {
        snprintf(errmsg, errmsglen, "Drive %c: not ready", fm_drive_letter(panel->filesystem));
        FatFSFileSystem = oldfs;
        return 0;
    }

    if (panel->filesystem == 0)
    {
        lfs_dir_t dir;
        struct lfs_info info;
        int rc;

        memset(&dir, 0, sizeof(dir));
        memset(&info, 0, sizeof(info));
        rc = lfs_dir_open(&lfs, &dir, panel->path);
        if (rc < 0)
        {
            snprintf(errmsg, errmsglen, "Cannot open %c:%s", fm_drive_letter(panel->filesystem), panel->path);
            FatFSFileSystem = oldfs;
            return 0;
        }
        while ((rc = lfs_dir_read(&lfs, &dir, &info)) > 0)
        {
            int namelen;
            if (CurrentlyPlaying != P_NOTHING)
                checkWAVinput();
            if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
                continue;
            if (!pattern_matching(panel->filter, info.name, 0, 0))
                continue;
            count++;
            namelen = strlen(info.name);
            if (namelen > max_name_len)
                max_name_len = namelen;
        }
        lfs_dir_close(&lfs, &dir);
        if (rc < 0)
        {
            snprintf(errmsg, errmsglen, "Read error on %c:%s", fm_drive_letter(panel->filesystem), panel->path);
            FatFSFileSystem = oldfs;
            return 0;
        }
    }
    else
    {
        DIR dir;
        FILINFO info;
        FRESULT fr;

        memset(&dir, 0, sizeof(dir));
        memset(&info, 0, sizeof(info));
        fr = f_findfirst(&dir, &info, panel->path, "*");
        if (fr != FR_OK)
        {
            snprintf(errmsg, errmsglen, "Cannot open %c:%s", fm_drive_letter(panel->filesystem), panel->path);
            FatFSFileSystem = oldfs;
            return 0;
        }
        while (fr == FR_OK && info.fname[0])
        {
            int namelen;
            if (CurrentlyPlaying != P_NOTHING)
                checkWAVinput();
            if (strcmp(info.fname, ".") == 0 || strcmp(info.fname, "..") == 0)
            {
                fr = f_findnext(&dir, &info);
                continue;
            }
            if (!(info.fattrib & (AM_SYS | AM_HID)) && pattern_matching(panel->filter, info.fname, 0, 0))
            {
                count++;
                namelen = strlen(info.fname);
                if (namelen > max_name_len)
                    max_name_len = namelen;
            }
            fr = f_findnext(&dir, &info);
        }
        f_closedir(&dir);
        if (fr != FR_OK)
        {
            snprintf(errmsg, errmsglen, "Read error on %c:%s", fm_drive_letter(panel->filesystem), panel->path);
            FatFSFileSystem = oldfs;
            return 0;
        }
    }

    if (max_name_len < FM_MIN_CACHE_NAME_CHARS)
        max_name_len = FM_MIN_CACHE_NAME_CHARS;
    if (max_name_len > FF_MAX_LFN)
        max_name_len = FF_MAX_LFN;
    panel->name_stride = max_name_len + 1;

    panel->entries = GetMemoryNull(sizeof(fm_entry_t) * count);  // CMM2: GetMemory stops with an error
    panel->name_pool = GetMemoryNull(panel->name_stride * count);
    panel->type_next = GetMemoryNull(sizeof(int) * count);
    panel->marks = GetMemoryNull(count);
    panel->type_head = GetMemoryNull(FM_TYPE_HEAD_BYTES);
    if (!panel->entries || !panel->name_pool || !panel->type_next || !panel->marks || !panel->type_head)
    {
        fm_free_panel_cache(panel);
        snprintf(errmsg, errmsglen, "Not enough memory for FM list");
        FatFSFileSystem = oldfs;
        return 0;
    }

    for (i = 0; i < count; i++)
        panel->entries[i].name = panel->name_pool + (i * panel->name_stride);

    strcpy(panel->entries[0].name, fm_path_is_root(panel->path) ? "." : "..");
    panel->entries[0].is_dir = 1;
    panel->entries[0].size = 0;
    panel->entries[0].fdate = 0xFFFF;
    panel->entries[0].ftime = 0xFFFF;

    if (panel->filesystem == 0)
    {
        lfs_dir_t dir;
        struct lfs_info info;
        int rc;

        memset(&dir, 0, sizeof(dir));
        memset(&info, 0, sizeof(info));
        rc = lfs_dir_open(&lfs, &dir, panel->path);
        if (rc < 0)
        {
            fm_free_panel_cache(panel);
            snprintf(errmsg, errmsglen, "Cannot open %c:%s", fm_drive_letter(panel->filesystem), panel->path);
            FatFSFileSystem = oldfs;
            return 0;
        }
        while ((rc = lfs_dir_read(&lfs, &dir, &info)) > 0)
        {
            fm_entry_t *e;
            if (CurrentlyPlaying != P_NOTHING)
                checkWAVinput();
            if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
                continue;
            if (!pattern_matching(panel->filter, info.name, 0, 0))
                continue;
            if (idx >= count)
                break;

            e = &panel->entries[idx++];
            strncpy(e->name, info.name, panel->name_stride - 1);
            e->name[panel->name_stride - 1] = 0;
            e->is_dir = (info.type == LFS_TYPE_DIR);
            e->size = info.size;
            if (e->is_dir)
            {
                e->fdate = 0xFFFF;
                e->ftime = 0xFFFF;
            }
            else if (panel->sortorder == FM_SORT_TIME)
            {
                int dt = 0;
                char fullfilename[FM_PATH_MAX];
                if (fm_make_child_path(panel->path, info.name, fullfilename, sizeof(fullfilename)) &&
                    lfs_getattr(&lfs, fullfilename, 'A', &dt, 4) == 4)
                {
                    WORD *wp = (WORD *)&dt;
                    e->fdate = wp[1];
                    e->ftime = wp[0];
                }
                else
                {
                    e->fdate = 0;
                    e->ftime = 0;
                }
            }
            else
            {
                e->fdate = 0;
                e->ftime = 0;
            }
        }
        lfs_dir_close(&lfs, &dir);
        if (rc < 0)
        {
            fm_free_panel_cache(panel);
            snprintf(errmsg, errmsglen, "Read error on %c:%s", fm_drive_letter(panel->filesystem), panel->path);
            FatFSFileSystem = oldfs;
            return 0;
        }
    }
    else
    {
        DIR dir;
        FILINFO info;
        FRESULT fr;

        memset(&dir, 0, sizeof(dir));
        memset(&info, 0, sizeof(info));
        fr = f_findfirst(&dir, &info, panel->path, "*");
        if (fr != FR_OK)
        {
            fm_free_panel_cache(panel);
            snprintf(errmsg, errmsglen, "Cannot open %c:%s", fm_drive_letter(panel->filesystem), panel->path);
            FatFSFileSystem = oldfs;
            return 0;
        }
        while (fr == FR_OK && info.fname[0])
        {
            fm_entry_t *e;
            if (CurrentlyPlaying != P_NOTHING)
                checkWAVinput();
            if (strcmp(info.fname, ".") == 0 || strcmp(info.fname, "..") == 0)
            {
                fr = f_findnext(&dir, &info);
                continue;
            }
            if ((info.fattrib & (AM_SYS | AM_HID)) || !pattern_matching(panel->filter, info.fname, 0, 0))
            {
                fr = f_findnext(&dir, &info);
                continue;
            }
            if (idx >= count)
                break;

            e = &panel->entries[idx++];
            strncpy(e->name, info.fname, panel->name_stride - 1);
            e->name[panel->name_stride - 1] = 0;
            e->is_dir = (info.fattrib & AM_DIR) != 0;
            e->size = info.fsize;
            if (e->is_dir)
            {
                e->fdate = 0xFFFF;
                e->ftime = 0xFFFF;
            }
            else
            {
                e->fdate = info.fdate;
                e->ftime = info.ftime;
            }
            fr = f_findnext(&dir, &info);
        }
        f_closedir(&dir);
        if (fr != FR_OK)
        {
            fm_free_panel_cache(panel);
            snprintf(errmsg, errmsglen, "Read error on %c:%s", fm_drive_letter(panel->filesystem), panel->path);
            FatFSFileSystem = oldfs;
            return 0;
        }
    }

    panel->entry_count = idx;
    panel->count = idx;

    if (panel->entry_count > 2)
    {
        if (panel->sortorder == FM_SORT_TIME)
            qsort(&panel->entries[1], panel->entry_count - 1, sizeof(fm_entry_t), fm_entry_cmp_time);
        else if (panel->sortorder == FM_SORT_TYPE)
            qsort(&panel->entries[1], panel->entry_count - 1, sizeof(fm_entry_t), fm_entry_cmp_type);
        else
            qsort(&panel->entries[1], panel->entry_count - 1, sizeof(fm_entry_t), fm_entry_cmp_name);
    }

    memset(panel->type_head, 0xFF, FM_TYPE_HEAD_BYTES);
    for (i = panel->entry_count - 1; i >= 1; i--)
    {
        int key = tolower((unsigned char)panel->entries[i].name[0]);
        panel->type_next[i] = panel->type_head[key];
        panel->type_head[key] = i;
    }
    panel->type_next[0] = -1;
    memset(panel->marks, 0, panel->entry_count > 0 ? panel->entry_count : 1);
    panel->marked_count = 0;
    panel->cache_valid = 1;
    FatFSFileSystem = oldfs;
    return 1;
}

static void fm_format_entry(const fm_entry_t *entry, char *out, int outlen)
{
    if (!out || outlen <= 0)
        return;
    out[0] = 0;

    if (entry->is_dir)
    {
        if (strcmp(entry->name, "..") == 0)
            snprintf(out, outlen, "[..]");
        else if (strcmp(entry->name, ".") == 0)
            snprintf(out, outlen, "[.]");
        else
        {
            strncpy(out, "<DIR> ", outlen - 1);
            out[outlen - 1] = 0;
            strncat(out, entry->name, outlen - strlen(out) - 1);
        }
    }
    else
    {
        char tmp[16];
        snprintf(tmp, sizeof(tmp), "%10u ", (unsigned int)entry->size);
        strncpy(out, tmp, outlen - 1);
        out[outlen - 1] = 0;
        strncat(out, entry->name, outlen - strlen(out) - 1);
    }
}

static int fm_entry_is_markable(const fm_entry_t *entry)
{
    return entry && strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0;
}

static int fm_mark_toggle(fm_panel_t *panel, int index, char *errmsg, int errmsglen)
{
    if (!fm_build_panel_cache(panel, errmsg, errmsglen))
        return 0;
    if (index < 0 || index >= panel->entry_count)
        return 0;
    if (!fm_entry_is_markable(&panel->entries[index]))
        return 0;

    if (panel->marks[index])
    {
        panel->marks[index] = 0;
        if (panel->marked_count > 0)
            panel->marked_count--;
    }
    else
    {
        panel->marks[index] = 1;
        panel->marked_count++;
    }
    return 1;
}

static int fm_mark_all(fm_panel_t *panel, char *errmsg, int errmsglen)
{
    int i;
    int changed = 0;

    if (!fm_build_panel_cache(panel, errmsg, errmsglen))
        return 0;

    panel->marked_count = 0;
    for (i = 1; i < panel->entry_count; i++)
    {
        if (fm_entry_is_markable(&panel->entries[i]))
        {
            if (!panel->marks[i])
                changed = 1;
            panel->marks[i] = 1;
            panel->marked_count++;
        }
        else
        {
            panel->marks[i] = 0;
        }
    }
    if (panel->entry_count > 0)
        panel->marks[0] = 0;
    return changed;
}

static int fm_mark_clear_all(fm_panel_t *panel, char *errmsg, int errmsglen)
{
    if (!fm_build_panel_cache(panel, errmsg, errmsglen))
        return 0;
    if (panel->entry_count > 0)
        memset(panel->marks, 0, panel->entry_count);
    panel->marked_count = 0;
    return 1;
}

static void fm_format_panel_entry(const fm_panel_t *panel, int idx, char *out, int outlen)
{
    char item[STRINGSIZE * 2];

    item[0] = 0;
    if (idx < 0 || idx >= panel->entry_count)
    {
        out[0] = 0;
        return;
    }

    fm_format_entry(&panel->entries[idx], item, sizeof(item));
    if (outlen <= 0)
        return;
    out[0] = panel->marks && panel->marks[idx] ? '*' : ' ';
    if (outlen == 1)
        return;
    out[1] = ' ';
    out[2] = 0;
    if (outlen > 3)
        strncat(out, item, outlen - 3);
}

static void fm_clamp_panel(fm_panel_t *panel, int list_rows)
{
    if (panel->count < 0)
        panel->count = 0;
    if (panel->selected >= panel->count)
        panel->selected = panel->count > 0 ? panel->count - 1 : 0;
    if (panel->selected < 0)
        panel->selected = 0;
    if (panel->top > panel->selected)
        panel->top = panel->selected;
    if (panel->top < 0)
        panel->top = 0;
    if (panel->count > list_rows && panel->top > panel->count - list_rows)
        panel->top = panel->count - list_rows;
}

static int fm_get_entry_at(fm_panel_t *panel, int index, fm_entry_t *entry, char *errmsg, int errmsglen)
{
    if (errmsglen > 0)
        errmsg[0] = 0;
    if (index < 0)
        return 0;

    if (!fm_build_panel_cache(panel, errmsg, errmsglen))
        return 0;
    if (index >= panel->entry_count)
    {
        snprintf(errmsg, errmsglen, "Entry not found");
        return 0;
    }

    *entry = panel->entries[index];
    return 1;
}

static void fm_set_status_to_selected(fm_panel_t *panel, char *status, int statuslen)
{
    fm_entry_t entry;
    char errmsg[STRINGSIZE];

    if (!status || statuslen <= 0)
        return;

    if (panel && panel->count > 0 && panel->selected >= 0 && panel->selected < panel->count &&
        fm_get_entry_at(panel, panel->selected, &entry, errmsg, sizeof(errmsg)))
    {
        snprintf(status, statuslen, "%s", entry.name);
    }
    else if (panel)
    {
        snprintf(status, statuslen, "%c:%s", fm_drive_letter(panel->filesystem), panel->path);
    }
    else
    {
        snprintf(status, statuslen, "FM");
    }
}

static int fm_draw_panel_entries(fm_panel_t *panel, int active, int x, int width, int list_top, int list_rows, char *errmsg, int errmsglen)
{
    int idx, drawn = 0, row;
    char item[STRINGSIZE * 2];
    if (errmsglen > 0)
        errmsg[0] = 0;

    for (row = 0; row < list_rows; row++)
    {
        fm_draw_field(x, list_top + row, width, "", FM_ANSI_NORMAL, WHITE, BLACK);
        /* Painting a full panel can take many milliseconds; service the
         * audio double-buffer so a MOD/WAV/FLAC/MP3 launched from FM does
         * not underrun and re-play its first buffer until the redraw
         * completes.                                                       */
        if (CurrentlyPlaying != P_NOTHING)
            checkWAVinput();
    }

    if (!fm_build_panel_cache(panel, errmsg, errmsglen))
    {
        panel->count = 0;
        fm_draw_field(x, list_top, width, errmsg, FM_ANSI_STATUS, YELLOW, BLACK);
        return 0;
    }

    for (row = 0; row < list_rows; row++)
    {
        idx = panel->top + row;
        item[0] = 0;
        if (idx >= 0 && idx < panel->count)
        {
            fm_format_panel_entry(panel, idx, item, sizeof(item));
            drawn++;
        }
        fm_draw_field(x, list_top + row, width, item,
                      idx == panel->selected ? (active ? FM_ANSI_ACTIVE : FM_ANSI_INACTIVE) : FM_ANSI_NORMAL,
                      (idx == panel->selected ? BLACK : WHITE),
                      (idx == panel->selected ? (active ? CYAN : WHITE) : BLACK));
        if (CurrentlyPlaying != P_NOTHING)
            checkWAVinput();
    }

    if (drawn == 0 && panel->count == 0)
        fm_draw_field(x, list_top, width, "<empty>", FM_ANSI_NORMAL, WHITE, BLACK);
    return 1;
}

static const char *fm_top_help_line(int width)
{
    if (width >= 78)
        return "F1:Help  A/C:Drive  S:Sort  F3:List  SPACE:Mark  /:Type-select";
    if (width >= 64)
        return "F1:Help  A/C:Drive  S:Sort  F3:List  SPACE:Mark";
    if (width >= 50)
        return "F1:Help  A/C:Drv  S:Sort  F3:List";
    return "F1:Help  S:Sort  F3:List";
}

/* Draw one help-table line: up to two (key, action) pairs in aligned columns.
 * Keys are highlighted (yellow), actions normal (white).  Each cell is drawn
 * with fm_draw_field which pads/truncates to its width, so columns stay aligned
 * regardless of the text length.  Pass an empty k2 for a single-pair row.     */
static int fm_help_row(int y, int half, int kw,
                       const char *k1, const char *a1,
                       const char *k2, const char *a2)
{
    fm_draw_field(0, y, kw, k1, FM_ANSI_STATUS, YELLOW, BLACK);
    fm_draw_field(kw, y, half - kw, a1, FM_ANSI_NORMAL, WHITE, BLACK);
    if (k2 != NULL && k2[0])
    {
        fm_draw_field(half, y, kw, k2, FM_ANSI_STATUS, YELLOW, BLACK);
        fm_draw_field(half + kw, y, Option.Width - half - kw, a2, FM_ANSI_NORMAL, WHITE, BLACK);
    }
    else
    {
        fm_draw_field(half, y, Option.Width - half, "", FM_ANSI_NORMAL, WHITE, BLACK);
    }
    return y + 1;
}

static void fm_show_help_screen(void)
{
    int i, c, y;
    int half = Option.Width / 2;
    int kw = 9; /* key column width (fits "Enter F2", "Home/End", etc.) */
    int bottom = Option.Height - 1;
    int clipped = 0;

    if (half < kw + 4)
        half = kw + 4; /* keep an action column even on the narrowest screens */

    for (i = 0; i < Option.Height; i++)
        fm_draw_field(0, i, Option.Width, "", FM_ANSI_NORMAL, WHITE, BLACK);

    fm_draw_field(0, 0, Option.Width, "FM HELP", FM_ANSI_TITLE, CYAN, BLACK);

    y = 2;
/* Draw a command row, but stop (and flag truncation) once we reach the
 * footer line so the listing never overruns a short screen.                */
#define FM_HELP_ROW(k1, a1, k2, a2)                                  \
    do                                                               \
    {                                                                \
        if (y < bottom)                                              \
            y = fm_help_row(y, half, kw, (k1), (a1), (k2), (a2));    \
        else                                                         \
            clipped = 1;                                             \
    } while (0)
#define FM_HELP_HEAD(txt)                                                          \
    do                                                                            \
    {                                                                             \
        if (y < bottom)                                                           \
            fm_draw_field(0, y++, Option.Width, (txt), FM_ANSI_TITLE, CYAN, BLACK);\
        else                                                                      \
            clipped = 1;                                                          \
    } while (0)

    FM_HELP_ROW("Enter F2", "Open / run", "L  F3", "List file");
    FM_HELP_ROW("E  F4", "Edit file", "Y  F5", "Copy");
    FM_HELP_ROW("D", "Duplicate", "M", "Move");
    FM_HELP_ROW("N", "New file", "R", "Rename");
    FM_HELP_ROW("DEL", "Delete", "X", "Recursive del");
    FM_HELP_ROW("K  F11", "Make dir", "G", "Go to path");
    FM_HELP_ROW("A C", "Change drive", "S", "Sort");
    FM_HELP_ROW("F  F9", "Set filter", "W  F10", "Clear filter");
    FM_HELP_ROW("/", "Type-select", "H  ?", "Help");
    FM_HELP_ROW("T  F6", "Stop audio", "- +", "Volume up/dn");
    FM_HELP_ROW("Space", "Mark/unmark", "* \\", "Mark all/clr");
    FM_HELP_ROW("ESC", "Exit FM", ", .", "Prev/next track");

    if (y < bottom)
        y++; /* blank separator before the navigation block */
    FM_HELP_HEAD("Move / panel (no arrow or function keys needed)");
    FM_HELP_ROW("Up/Dn", "Move  ^E ^X", "PgUp/Dn", "Page  ^P ^L");
    FM_HELP_ROW("Home/End", "Ends  ^U ^K", "Tab L/R", "Panel ^S ^D");
    FM_HELP_ROW("Bksp", "Parent dir", "", "");

#undef FM_HELP_ROW
#undef FM_HELP_HEAD

    if (clipped)
        fm_draw_field(0, bottom, Option.Width, "Press any key to return to FM (help truncated)", FM_ANSI_STATUS, YELLOW, BLACK);
    else
        fm_draw_field(0, bottom, Option.Width, "Press any key to return to FM", FM_ANSI_STATUS, YELLOW, BLACK);

    c = -1;
    while (c == -1)
    {
        routinechecks();
        c = MMInkey();
    }
}

static int fm_is_valid_leaf_name(const char *name)
{
    const char *p;
    if (name == NULL || name[0] == 0)
        return 0;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return 0;
    for (p = name; *p; p++)
    {
        if (*p == '/' || *p == '\\' || *p == ':')
            return 0;
    }
    return 1;
}

static int fm_prompt_text(const char *label, const char *initial, char *out, int outlen)
{
    int c;
    int len;
    char temp[FF_MAX_LFN + 1];
    char line[STRINGSIZE * 2];

    if (outlen <= 0)
        return 0;

    temp[0] = 0;
    if (initial && initial[0])
    {
        strncpy(temp, initial, FF_MAX_LFN);
        temp[FF_MAX_LFN] = 0;
    }
    len = strlen(temp);

    while (1)
    {
        line[0] = 0;
        if (label)
        {
            strncpy(line, label, sizeof(line) - 1);
            line[sizeof(line) - 1] = 0;
        }
        strncat(line, temp, sizeof(line) - strlen(line) - 1);
        fm_draw_field(0, Option.Height - 1, Option.Width, line, FM_ANSI_STATUS, YELLOW, BLACK);
        fm_prompt_cursor(strlen(line), 1);
        c = MMgetchar();
        fm_prompt_cursor(strlen(line), 0);
        if (c == '\r' || c == '\n')
        {
            strncpy(out, temp, outlen - 1);
            out[outlen - 1] = 0;
            return 1;
        }
        if (c == ESC)
            return 0;
        if (c == BKSP)
        {
            if (len > 0)
                temp[--len] = 0;
            continue;
        }
        if (isprint(c) && len < FF_MAX_LFN)
        {
            temp[len++] = c;
            temp[len] = 0;
        }
    }
}

static int fm_confirm_action(const char *prompt)
{
    int c = -1;
    char line[STRINGSIZE * 2];
    const char *suffix = " [Y/N]";
    int room;

    strncpy(line, prompt ? prompt : "Confirm", sizeof(line) - 1);
    line[sizeof(line) - 1] = 0;

    room = (int)sizeof(line) - (int)strlen(line) - 1;
    if (room > 0)
        strncat(line, suffix, room);

    fm_draw_field(0, Option.Height - 1, Option.Width, line, FM_ANSI_STATUS, YELLOW, BLACK);

    while (1)
    {
        routinechecks();
        c = MMInkey();
        if (c == -1)
            continue;
        if (c >= 'a' && c <= 'z')
            c = mytoupper(c);
        if (c == 'Y')
            return 1;
        if (c == 'N' || c == ESC)
            return 0;
    }
}

static int fm_rename_path(int filesystem, const char *oldpath, const char *newpath, char *status, int statuslen)
{
    int oldfs = FatFSFileSystem;
    int oldabort = OptionFileErrorAbort;

    if (filesystem == 0)
    {
        int rc = lfs_rename(&lfs, oldpath, newpath);
        if (rc == 0)
            return 1;
        if (rc == LFS_ERR_EXIST)
            snprintf(status, statuslen, "Target already exists");
        else if (rc == LFS_ERR_NOENT)
            snprintf(status, statuslen, "Source not found");
        else
            snprintf(status, statuslen, "Rename failed (%d)", rc);
        return 0;
    }

    FatFSFileSystem = filesystem;
    OptionFileErrorAbort = 0;
    if (!InitSDCard())
    {
        OptionFileErrorAbort = oldabort;
        FatFSFileSystem = oldfs;
        snprintf(status, statuslen, "Drive %c: not ready", fm_drive_letter(filesystem));
        return 0;
    }
    {
        FRESULT fr = f_rename(oldpath, newpath);
        OptionFileErrorAbort = oldabort;
        FatFSFileSystem = oldfs;
        if (fr == FR_OK)
            return 1;
        if (fr == FR_EXIST)
            snprintf(status, statuslen, "Target already exists");
        else if (fr == FR_NO_FILE || fr == FR_NO_PATH)
            snprintf(status, statuslen, "Source not found");
        else if (fr == FR_DENIED)
            snprintf(status, statuslen, "Rename denied");
        else
            snprintf(status, statuslen, "Rename failed (%d)", (int)fr);
        return 0;
    }
}

static int fm_delete_path(int filesystem, const char *path, int is_dir, char *status, int statuslen)
{
    int oldfs = FatFSFileSystem;
    int oldabort = OptionFileErrorAbort;

    if (filesystem == 0)
    {
        int rc = lfs_remove(&lfs, path);
        if (rc == 0)
            return 1;
        if (rc == LFS_ERR_NOTEMPTY)
            snprintf(status, statuslen, "Directory not empty");
        else if (rc == LFS_ERR_NOENT)
            snprintf(status, statuslen, "Not found");
        else
            snprintf(status, statuslen, "Delete failed (%d)", rc);
        return 0;
    }

    FatFSFileSystem = filesystem;
    OptionFileErrorAbort = 0;
    if (!InitSDCard())
    {
        OptionFileErrorAbort = oldabort;
        FatFSFileSystem = oldfs;
        snprintf(status, statuslen, "Drive %c: not ready", fm_drive_letter(filesystem));
        return 0;
    }
    {
        FRESULT fr = f_unlink(path);
        OptionFileErrorAbort = oldabort;
        FatFSFileSystem = oldfs;
        if (fr == FR_OK)
            return 1;
        if (fr == FR_NO_FILE || fr == FR_NO_PATH)
            snprintf(status, statuslen, "Not found");
        else if (fr == FR_DENIED && is_dir)
            snprintf(status, statuslen, "Directory not empty");
        else if (fr == FR_DENIED)
            snprintf(status, statuslen, "Delete denied");
        else
            snprintf(status, statuslen, "Delete failed (%d)", (int)fr);
        return 0;
    }
}

static int fm_mkdir_path(int filesystem, const char *path, char *status, int statuslen)
{
    int oldfs = FatFSFileSystem;
    int oldabort = OptionFileErrorAbort;

    if (filesystem == 0)
    {
        int rc = lfs_mkdir(&lfs, path);
        if (rc == 0)
            return 1;
        if (rc == LFS_ERR_EXIST)
            snprintf(status, statuslen, "Directory already exists");
        else
            snprintf(status, statuslen, "Create directory failed (%d)", rc);
        return 0;
    }

    FatFSFileSystem = filesystem;
    OptionFileErrorAbort = 0;
    if (!InitSDCard())
    {
        OptionFileErrorAbort = oldabort;
        FatFSFileSystem = oldfs;
        snprintf(status, statuslen, "Drive %c: not ready", fm_drive_letter(filesystem));
        return 0;
    }
    {
        FRESULT fr = f_mkdir(path);
        OptionFileErrorAbort = oldabort;
        FatFSFileSystem = oldfs;
        if (fr == FR_OK)
            return 1;
        if (fr == FR_EXIST)
            snprintf(status, statuslen, "Directory already exists");
        else
            snprintf(status, statuslen, "Create directory failed (%d)", (int)fr);
        return 0;
    }
}

static int fm_rename_selected(fm_panel_t *panel, char *status, int statuslen)
{
    fm_entry_t entry;
    char errmsg[STRINGSIZE];
    char oldpath[FM_PATH_MAX];
    char newpath[FM_PATH_MAX];
    char newname[FF_MAX_LFN + 1];
    char prompt[STRINGSIZE * 2];

    if (panel->count <= 0 || panel->selected < 0 || panel->selected >= panel->count)
    {
        snprintf(status, statuslen, "No file selected");
        return 0;
    }
    if (!fm_get_entry_at(panel, panel->selected, &entry, errmsg, sizeof(errmsg)))
    {
        snprintf(status, statuslen, "%s", errmsg);
        return 0;
    }
    if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0)
    {
        snprintf(status, statuslen, "Select a file or directory");
        return 0;
    }

    snprintf(prompt, sizeof(prompt), "Rename %s to: ", entry.name);
    if (!fm_prompt_text(prompt, entry.name, newname, sizeof(newname)))
    {
        snprintf(status, statuslen, "Rename cancelled");
        return 0;
    }
    if (!fm_is_valid_leaf_name(newname))
    {
        snprintf(status, statuslen, "Invalid name");
        return 0;
    }
    if (fm_stricmp(entry.name, newname) == 0)
    {
        snprintf(status, statuslen, "Name unchanged");
        return 0;
    }

    if (!fm_make_child_path(panel->path, entry.name, oldpath, sizeof(oldpath)) ||
        !fm_make_child_path(panel->path, newname, newpath, sizeof(newpath)))
    {
        snprintf(status, statuslen, "Path too long");
        return 0;
    }

    if (!fm_rename_path(panel->filesystem, oldpath, newpath, status, statuslen))
        return 0;

    snprintf(status, statuslen, "Renamed %s -> %s", entry.name, newname);
    return 1;
}

static int fm_delete_selected(fm_panel_t *panel, char *status, int statuslen)
{
    fm_entry_t entry;
    char errmsg[STRINGSIZE];
    char fullpath[FM_PATH_MAX];
    char prompt[STRINGSIZE * 2];

    if (panel->count <= 0 || panel->selected < 0 || panel->selected >= panel->count)
    {
        snprintf(status, statuslen, "No file selected");
        return 0;
    }
    if (!fm_get_entry_at(panel, panel->selected, &entry, errmsg, sizeof(errmsg)))
    {
        snprintf(status, statuslen, "%s", errmsg);
        return 0;
    }
    if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0)
    {
        snprintf(status, statuslen, "Cannot delete this entry");
        return 0;
    }
    if (!fm_make_child_path(panel->path, entry.name, fullpath, sizeof(fullpath)))
    {
        snprintf(status, statuslen, "Path too long");
        return 0;
    }

    snprintf(prompt, sizeof(prompt), "Delete %s?", entry.name);
    if (!fm_confirm_action(prompt))
    {
        snprintf(status, statuslen, "Delete cancelled");
        return 0;
    }

    if (!fm_delete_path(panel->filesystem, fullpath, entry.is_dir, status, statuslen))
        return 0;

    if (panel->selected > 0)
        panel->selected--;

    snprintf(status, statuslen, "Deleted %s", entry.name);
    return 1;
}

static int fm_delete_recursive_path(int filesystem, const char *path, char *status, int statuslen, int *file_count, int *dir_count)
{
    if (path == NULL || path[0] == 0 || strcmp(path, "/") == 0)
    {
        snprintf(status, statuslen, "Refusing to delete root directory");
        return 0;
    }

    if (filesystem == 0)
    {
        lfs_dir_t dir;
        struct lfs_info info;
        int rc;

        memset(&dir, 0, sizeof(dir));
        memset(&info, 0, sizeof(info));
        rc = lfs_dir_open(&lfs, &dir, path);
        if (rc < 0)
        {
            snprintf(status, statuslen, "Cannot open %c:%s", fm_drive_letter(filesystem), path);
            return 0;
        }

        while ((rc = lfs_dir_read(&lfs, &dir, &info)) > 0)
        {
            char child[FM_PATH_MAX];
            if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
                continue;
            if (!fm_make_child_path(path, info.name, child, sizeof(child)))
            {
                lfs_dir_close(&lfs, &dir);
                snprintf(status, statuslen, "Path too long");
                return 0;
            }
            if (info.type == LFS_TYPE_DIR)
            {
                if (!fm_delete_recursive_path(filesystem, child, status, statuslen, file_count, dir_count))
                {
                    lfs_dir_close(&lfs, &dir);
                    return 0;
                }
            }
            else
            {
                if (!fm_delete_path(filesystem, child, 0, status, statuslen))
                {
                    lfs_dir_close(&lfs, &dir);
                    return 0;
                }
                if (file_count)
                    (*file_count)++;
            }
        }

        lfs_dir_close(&lfs, &dir);
        if (rc < 0)
        {
            snprintf(status, statuslen, "Read error on %c:%s", fm_drive_letter(filesystem), path);
            return 0;
        }

        if (!fm_delete_path(filesystem, path, 1, status, statuslen))
            return 0;
        if (dir_count)
            (*dir_count)++;
        return 1;
    }
    else
    {
        int oldfs = FatFSFileSystem;
        int oldabort = OptionFileErrorAbort;
        DIR dir;
        FILINFO info;
        FRESULT fr;

        FatFSFileSystem = filesystem;
        OptionFileErrorAbort = 0;
        if (!InitSDCard())
        {
            OptionFileErrorAbort = oldabort;
            FatFSFileSystem = oldfs;
            snprintf(status, statuslen, "Drive %c: not ready", fm_drive_letter(filesystem));
            return 0;
        }

        memset(&dir, 0, sizeof(dir));
        memset(&info, 0, sizeof(info));
        fr = f_findfirst(&dir, &info, path, "*");
        if (fr != FR_OK)
        {
            OptionFileErrorAbort = oldabort;
            FatFSFileSystem = oldfs;
            snprintf(status, statuslen, "Cannot open %c:%s", fm_drive_letter(filesystem), path);
            return 0;
        }

        while (fr == FR_OK && info.fname[0])
        {
            char child[FM_PATH_MAX];
            if (strcmp(info.fname, ".") == 0 || strcmp(info.fname, "..") == 0)
            {
                fr = f_findnext(&dir, &info);
                continue;
            }
            if (!fm_make_child_path(path, info.fname, child, sizeof(child)))
            {
                f_closedir(&dir);
                OptionFileErrorAbort = oldabort;
                FatFSFileSystem = oldfs;
                snprintf(status, statuslen, "Path too long");
                return 0;
            }

            if (info.fattrib & AM_DIR)
            {
                if (!fm_delete_recursive_path(filesystem, child, status, statuslen, file_count, dir_count))
                {
                    f_closedir(&dir);
                    OptionFileErrorAbort = oldabort;
                    FatFSFileSystem = oldfs;
                    return 0;
                }
            }
            else
            {
                if (!fm_delete_path(filesystem, child, 0, status, statuslen))
                {
                    f_closedir(&dir);
                    OptionFileErrorAbort = oldabort;
                    FatFSFileSystem = oldfs;
                    return 0;
                }
                if (file_count)
                    (*file_count)++;
            }

            fr = f_findnext(&dir, &info);
        }

        f_closedir(&dir);
        OptionFileErrorAbort = oldabort;
        FatFSFileSystem = oldfs;
        if (fr != FR_OK)
        {
            snprintf(status, statuslen, "Read error on %c:%s", fm_drive_letter(filesystem), path);
            return 0;
        }

        if (!fm_delete_path(filesystem, path, 1, status, statuslen))
            return 0;
        if (dir_count)
            (*dir_count)++;
        return 1;
    }
}

static int fm_confirm_recursive_delete(const char *name)
{
    char prompt[STRINGSIZE * 2];

    snprintf(prompt, sizeof(prompt), "Recursive delete %s and contents?", name);
    if (!fm_confirm_action(prompt))
        return 0;

    snprintf(prompt, sizeof(prompt), "Final confirmation: permanently delete %s?", name);
    if (!fm_confirm_action(prompt))
        return 0;

    return 1;
}

static int fm_delete_selected_recursive(fm_panel_t *panel, char *status, int statuslen)
{
    fm_entry_t entry;
    char errmsg[STRINGSIZE];
    char fullpath[FM_PATH_MAX];
    int files = 0;
    int dirs = 0;

    if (panel->count <= 0 || panel->selected < 0 || panel->selected >= panel->count)
    {
        snprintf(status, statuslen, "No file selected");
        return 0;
    }
    if (!fm_get_entry_at(panel, panel->selected, &entry, errmsg, sizeof(errmsg)))
    {
        snprintf(status, statuslen, "%s", errmsg);
        return 0;
    }
    if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0)
    {
        snprintf(status, statuslen, "Cannot delete this entry");
        return 0;
    }
    if (!fm_make_child_path(panel->path, entry.name, fullpath, sizeof(fullpath)))
    {
        snprintf(status, statuslen, "Path too long");
        return 0;
    }

    if (!fm_confirm_recursive_delete(entry.name))
    {
        snprintf(status, statuslen, "Recursive delete cancelled");
        return 0;
    }

    if (entry.is_dir)
    {
        if (!fm_delete_recursive_path(panel->filesystem, fullpath, status, statuslen, &files, &dirs))
            return 0;
        if (panel->selected > 0)
            panel->selected--;
        snprintf(status, statuslen, "Deleted %s (%d files, %d dirs)", entry.name, files, dirs);
        return 1;
    }

    if (!fm_delete_path(panel->filesystem, fullpath, 0, status, statuslen))
        return 0;
    if (panel->selected > 0)
        panel->selected--;
    snprintf(status, statuslen, "Deleted %s", entry.name);
    return 1;
}

static int fm_delete_marked(fm_panel_t *panel, int recursive, char *status, int statuslen)
{
    fm_mark_item_t *items = NULL;
    char prompt[STRINGSIZE * 2];
    int i, n = 0;
    int files = 0, dirs = 0;

    if (!fm_build_panel_cache(panel, status, statuslen))
        return 0;
    if (panel->marked_count <= 0)
        return recursive ? fm_delete_selected_recursive(panel, status, statuslen)
                         : fm_delete_selected(panel, status, statuslen);

    items = GetMemoryNull(sizeof(fm_mark_item_t) * panel->marked_count);
    if (items == NULL)
    {
        snprintf(status, statuslen, "Not enough memory");
        return 0;
    }
    for (i = 1; i < panel->entry_count; i++)
    {
        if (panel->marks[i] && fm_entry_is_markable(&panel->entries[i]))
        {
            strncpy(items[n].name, panel->entries[i].name, FF_MAX_LFN);
            items[n].name[FF_MAX_LFN] = 0;
            items[n].is_dir = panel->entries[i].is_dir;
            n++;
        }
    }
    if (n <= 0)
    {
        FreeMemorySafe((void **)&items);
        snprintf(status, statuslen, "No marked entries");
        return 0;
    }

    if (!recursive)
    {
        snprintf(prompt, sizeof(prompt), "Delete %d marked item%s?", n, n == 1 ? "" : "s");
        if (!fm_confirm_action(prompt))
        {
            FreeMemorySafe((void **)&items);
            snprintf(status, statuslen, "Delete cancelled");
            return 0;
        }
    }
    else
    {
        snprintf(prompt, sizeof(prompt), "Recursive delete %d marked item%s and contents?", n, n == 1 ? "" : "s");
        if (!fm_confirm_action(prompt))
        {
            FreeMemorySafe((void **)&items);
            snprintf(status, statuslen, "Recursive delete cancelled");
            return 0;
        }
        snprintf(prompt, sizeof(prompt), "Final confirmation: permanently delete %d marked item%s?", n, n == 1 ? "" : "s");
        if (!fm_confirm_action(prompt))
        {
            FreeMemorySafe((void **)&items);
            snprintf(status, statuslen, "Recursive delete cancelled");
            return 0;
        }
    }

    for (i = 0; i < n; i++)
    {
        char fullpath[FM_PATH_MAX];

        if (!fm_make_child_path(panel->path, items[i].name, fullpath, sizeof(fullpath)))
        {
            FreeMemorySafe((void **)&items);
            snprintf(status, statuslen, "Path too long");
            panel->cache_valid = 0;
            panel->count = 0;
            return 0;
        }

        if (items[i].is_dir && recursive)
        {
            if (!fm_delete_recursive_path(panel->filesystem, fullpath, status, statuslen, &files, &dirs))
            {
                FreeMemorySafe((void **)&items);
                panel->cache_valid = 0;
                panel->count = 0;
                return 0;
            }
        }
        else
        {
            if (!fm_delete_path(panel->filesystem, fullpath, items[i].is_dir, status, statuslen))
            {
                FreeMemorySafe((void **)&items);
                panel->cache_valid = 0;
                panel->count = 0;
                return 0;
            }
            if (items[i].is_dir)
                dirs++;
            else
                files++;
        }
    }

    FreeMemorySafe((void **)&items);
    panel->cache_valid = 0;
    panel->count = 0;
    panel->selected = 0;
    panel->top = 0;
    if (recursive)
        snprintf(status, statuslen, "Deleted %d marked (%d files, %d dirs)", n, files, dirs);
    else
        snprintf(status, statuslen, "Deleted %d marked item%s", n, n == 1 ? "" : "s");
    return 1;
}

static int fm_make_directory(fm_panel_t *panel, char *status, int statuslen)
{
    char name[FF_MAX_LFN + 1];
    char path[FM_PATH_MAX];
    char prompt[STRINGSIZE * 2];

    snprintf(prompt, sizeof(prompt), "New directory name: ");
    if (!fm_prompt_text(prompt, "", name, sizeof(name)))
    {
        snprintf(status, statuslen, "Create directory cancelled");
        return 0;
    }
    if (!fm_is_valid_leaf_name(name))
    {
        snprintf(status, statuslen, "Invalid directory name");
        return 0;
    }
    if (!fm_make_child_path(panel->path, name, path, sizeof(path)))
    {
        snprintf(status, statuslen, "Path too long");
        return 0;
    }
    if (!fm_mkdir_path(panel->filesystem, path, status, statuslen))
        return 0;

    snprintf(status, statuslen, "Created directory %s", name);
    return 1;
}

static int fm_normalize_path(const char *in, char *out, int outlen)
{
    const char *p;
    char seg[FF_MAX_LFN];

    if (!in || !out || outlen < 2)
        return 0;
    if (in[0] != '/')
        return 0;

    out[0] = '/';
    out[1] = 0;
    p = in;

    while (*p)
    {
        int n;
        while (*p == '/')
            p++;
        if (*p == 0)
            break;

        n = 0;
        while (p[n] && p[n] != '/')
        {
            if (n >= FF_MAX_LFN - 1)
                return 0;
            seg[n] = p[n];
            n++;
        }
        seg[n] = 0;
        p += n;

        if (strcmp(seg, ".") == 0)
            continue;
        if (strcmp(seg, "..") == 0)
        {
            if (out[1] != 0)
            {
                char *slash = strrchr(out, '/');
                if (slash == out)
                    out[1] = 0;
                else if (slash)
                    *slash = 0;
            }
            continue;
        }

        if (out[1] != 0)
        {
            size_t len = strlen(out);
            if ((int)len + 1 >= outlen)
                return 0;
            out[len] = '/';
            out[len + 1] = 0;
        }

        if ((int)(strlen(out) + strlen(seg)) >= outlen)
            return 0;
        strcat(out, seg);
    }

    return 1;
}

static int fm_resolve_path(const char *base, const char *input, char *out, int outlen)
{
    char combined[FM_PATH_MAX];

    if (!base || !input || !out)
        return 0;
    if (strchr(input, ':'))
        return 0;

    if (input[0] == '/')
        return fm_normalize_path(input, out, outlen);

    if (!fm_make_child_path(base, input, combined, sizeof(combined)))
        return 0;
    return fm_normalize_path(combined, out, outlen);
}

static int fm_go_to_path(fm_panel_t *panel, char *status, int statuslen)
{
    char input[FF_MAX_LFN + 1];
    char target[FF_MAX_LFN];

    if (!fm_prompt_text("Go to path: ", panel->path, input, sizeof(input)))
    {
        snprintf(status, statuslen, "Go to path cancelled");
        return 0;
    }
    if (input[0] == 0)
    {
        snprintf(status, statuslen, "Path unchanged");
        return 0;
    }
    if (!fm_resolve_path(panel->path, input, target, sizeof(target)))
    {
        snprintf(status, statuslen, "Invalid path");
        return 0;
    }
    if (!fm_path_exists_dir(panel->filesystem, target))
    {
        snprintf(status, statuslen, "Path not found");
        return 0;
    }

    strncpy(panel->path, target, FF_MAX_LFN - 1);
    panel->path[FF_MAX_LFN - 1] = 0;
    panel->selected = 0;
    panel->top = 0;
    panel->count = 0;
    snprintf(status, statuslen, "%c:%s", fm_drive_letter(panel->filesystem), panel->path);
    return 1;
}

static int fm_create_empty_file(int filesystem, const char *dir, const char *name, char *status, int statuslen)
{
    char path[FM_PATH_MAX];

    if (!fm_make_child_path(dir, name, path, sizeof(path)))
    {
        snprintf(status, statuslen, "Path too long");
        return 0;
    }

    if (filesystem == 0)
    {
        lfs_file_t file;
        int rc;
        memset(&file, 0, sizeof(file));
        rc = lfs_file_open(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
        if (rc < 0)
        {
            snprintf(status, statuslen, "Create file failed (%d)", rc);
            return 0;
        }
        lfs_file_close(&lfs, &file);
        return 1;
    }
    else
    {
        int oldfs = FatFSFileSystem;
        int oldabort = OptionFileErrorAbort;
        FIL f;
        FRESULT fr;

        FatFSFileSystem = filesystem;
        OptionFileErrorAbort = 0;
        if (!InitSDCard())
        {
            OptionFileErrorAbort = oldabort;
            FatFSFileSystem = oldfs;
            snprintf(status, statuslen, "Drive %c: not ready", fm_drive_letter(filesystem));
            return 0;
        }

        fr = f_open(&f, path, FA_WRITE | FA_CREATE_ALWAYS);
        if (fr == FR_OK)
            f_close(&f);
        OptionFileErrorAbort = oldabort;
        FatFSFileSystem = oldfs;

        if (fr != FR_OK)
        {
            snprintf(status, statuslen, "Create file failed (%d)", (int)fr);
            return 0;
        }
        return 1;
    }
}

static void fm_suggest_duplicate_name(const char *name, char *out, int outlen)
{
    const char *dot = strrchr(name, '.');

    if (dot && dot != name)
    {
        int baselen = (int)(dot - name);
        if (snprintf(out, outlen, "%.*s_copy%s", baselen, name, dot) < outlen)
            return;
    }

    if (snprintf(out, outlen, "%s_copy", name) >= outlen)
    {
        strncpy(out, name, outlen - 1);
        out[outlen - 1] = 0;
    }
}

static int fm_duplicate_selected(fm_panel_t *panel, char *status, int statuslen)
{
    fm_entry_t entry;
    char errmsg[STRINGSIZE];
    // srcpath/dstpath are reused for the full-filename file copy below (the dir
    // and file branches are mutually exclusive) — no separate srcfile/dstfile.
    char srcpath[FM_PATH_MAX], dstpath[FM_PATH_MAX];
    char newname[FF_MAX_LFN + 1];
    int files = 0, dirs = 0;

    if (panel->count <= 0 || panel->selected < 0 || panel->selected >= panel->count)
    {
        snprintf(status, statuslen, "No file selected");
        return 0;
    }
    if (!fm_get_entry_at(panel, panel->selected, &entry, errmsg, sizeof(errmsg)))
    {
        snprintf(status, statuslen, "%s", errmsg);
        return 0;
    }
    if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0)
    {
        snprintf(status, statuslen, "Select a file or directory");
        return 0;
    }

    fm_suggest_duplicate_name(entry.name, newname, sizeof(newname));
    if (!fm_prompt_text("Duplicate as: ", newname, newname, sizeof(newname)))
    {
        snprintf(status, statuslen, "Duplicate cancelled");
        return 0;
    }
    if (!fm_is_valid_leaf_name(newname))
    {
        snprintf(status, statuslen, "Invalid name");
        return 0;
    }
    if (fm_stricmp(newname, entry.name) == 0)
    {
        snprintf(status, statuslen, "Name unchanged");
        return 0;
    }

    if (!fm_make_child_path(panel->path, entry.name, srcpath, sizeof(srcpath)) ||
        !fm_make_child_path(panel->path, newname, dstpath, sizeof(dstpath)))
    {
        snprintf(status, statuslen, "Path too long");
        return 0;
    }

    if (entry.is_dir)
    {
        if (fm_path_contains_icase(srcpath, dstpath))
        {
            snprintf(status, statuslen, "Destination inside source not allowed");
            return 0;
        }
        if (!fm_copy_dir_recursive(panel->filesystem, srcpath, panel->filesystem, dstpath,
                                   status, statuslen, &files, &dirs))
            return 0;
        snprintf(status, statuslen, "Duplicated directory %s", newname);
        return 1;
    }

    if (!fm_make_full_filename(panel->filesystem, panel->path, entry.name, srcpath, sizeof(srcpath)) ||
        !fm_make_full_filename(panel->filesystem, panel->path, newname, dstpath, sizeof(dstpath)))
    {
        snprintf(status, statuslen, "Path too long");
        return 0;
    }
    if (!fm_copy_file_path(panel->filesystem, srcpath, panel->filesystem, dstpath,
                           status, statuslen, &files, &dirs))
        return 0;

    snprintf(status, statuslen, "Duplicated %s", newname);
    return 1;
}

static int fm_move_selected(fm_panel_t *src, fm_panel_t *dst, char *status, int statuslen)
{
    fm_entry_t entry;
    char errmsg[STRINGSIZE];
    // dstpath doubles as the cross-drive copy destination filename (it is unused
    // in that branch as a child path); srcpath stays live there for the delete.
    char srcpath[FM_PATH_MAX], dstpath[FM_PATH_MAX];
    char srcfile[FM_PATH_MAX];
    int files = 0, dirs = 0;

    if (src->count <= 0 || src->selected < 0 || src->selected >= src->count)
    {
        snprintf(status, statuslen, "No file selected");
        return 0;
    }
    if (!fm_get_entry_at(src, src->selected, &entry, errmsg, sizeof(errmsg)))
    {
        snprintf(status, statuslen, "%s", errmsg);
        return 0;
    }
    if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0)
    {
        snprintf(status, statuslen, "Select a file or directory");
        return 0;
    }

    if (!fm_make_child_path(src->path, entry.name, srcpath, sizeof(srcpath)) ||
        !fm_make_child_path(dst->path, entry.name, dstpath, sizeof(dstpath)))
    {
        snprintf(status, statuslen, "Path too long");
        return 0;
    }

    if (src->filesystem == dst->filesystem)
    {
        if (fm_stricmp(srcpath, dstpath) == 0)
        {
            snprintf(status, statuslen, "Source and destination are the same");
            return 0;
        }
        if (entry.is_dir && fm_path_contains_icase(srcpath, dstpath))
        {
            snprintf(status, statuslen, "Destination inside source not allowed");
            return 0;
        }
        if (!fm_rename_path(src->filesystem, srcpath, dstpath, status, statuslen))
            return 0;
    }
    else
    {
        if (entry.is_dir)
        {
            snprintf(status, statuslen, "Cross-drive move for directories not supported");
            return 0;
        }

        if (!fm_make_full_filename(src->filesystem, src->path, entry.name, srcfile, sizeof(srcfile)) ||
            !fm_make_full_filename(dst->filesystem, dst->path, entry.name, dstpath, sizeof(dstpath)))
        {
            snprintf(status, statuslen, "Path too long");
            return 0;
        }
        if (!fm_copy_file_path(src->filesystem, srcfile, dst->filesystem, dstpath,
                               status, statuslen, &files, &dirs))
            return 0;
        if (!fm_delete_path(src->filesystem, srcpath, 0, status, statuslen))
            return 0;
    }

    if (src->selected > 0)
        src->selected--;

    snprintf(status, statuslen, "Moved %s", entry.name);
    return 1;
}

static void fm_draw_ui(fm_panel_t *panels, int active, const char *status)
{
    char line[STRINGSIZE * 2];
    char errmsg[STRINGSIZE];
    char left_err[STRINGSIZE];
    char right_err[STRINGSIZE];
    int left_ok;
    int right_ok;
    int list_top = 2;
    int list_rows = Option.Height - 4;
    int left_width = (Option.Width - 1) / 2;
    int right_x = left_width + 1;
    int right_width = Option.Width - right_x;
    int row;

    left_err[0] = 0;
    right_err[0] = 0;
    left_ok = fm_build_panel_cache(&panels[0], left_err, sizeof(left_err));
    right_ok = fm_build_panel_cache(&panels[1], right_err, sizeof(right_err));
    if (!left_ok)
        panels[0].count = 0;
    if (!right_ok)
        panels[1].count = 0;

    fm_clamp_panel(&panels[0], list_rows);
    fm_clamp_panel(&panels[1], list_rows);

    if (CurrentlyPlaying != P_NOTHING)
        checkWAVinput();

    fm_serial_repaint_ui(panels, active, status, left_ok, left_err, right_ok, right_err);

    fm_skip_serial_output = 1;

    snprintf(line, sizeof(line), "%s", fm_top_help_line(Option.Width));
    fm_draw_field(0, 0, Option.Width, line, FM_ANSI_TITLE, CYAN, BLACK);

    snprintf(line, sizeof(line), "%c:%s [%s]", fm_drive_letter(panels[0].filesystem), panels[0].path, panels[0].filter);
    fm_draw_field(0, 1, left_width, line, active == 0 ? FM_ANSI_ACTIVE : FM_ANSI_INACTIVE,
                  active == 0 ? BLACK : BLACK, active == 0 ? CYAN : WHITE);
    snprintf(line, sizeof(line), "%c:%s [%s]", fm_drive_letter(panels[1].filesystem), panels[1].path, panels[1].filter);
    fm_draw_field(right_x, 1, right_width, line, active == 1 ? FM_ANSI_ACTIVE : FM_ANSI_INACTIVE,
                  active == 1 ? BLACK : BLACK, active == 1 ? CYAN : WHITE);
    for (row = 1; row < Option.Height - 1; row++)
        fm_draw_field(left_width, row, 1, "|", FM_ANSI_TITLE, CYAN, BLACK);

    fm_draw_panel_entries(&panels[0], active == 0, 0, left_width, list_top, list_rows, errmsg, sizeof(errmsg));
    fm_draw_panel_entries(&panels[1], active == 1, right_x, right_width, list_top, list_rows, errmsg, sizeof(errmsg));

    fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);

    fm_skip_serial_output = 0;
}

static int fm_prompt_filter(fm_panel_t *panel)
{
    int c;
    int len;
    char temp[FF_MAX_LFN];
    char line[STRINGSIZE * 2];

    if (strcmp(panel->filter, "*") == 0)
        temp[0] = 0;
    else
        strncpy(temp, panel->filter, FF_MAX_LFN - 1);
    temp[FF_MAX_LFN - 1] = 0;
    len = strlen(temp);

    while (1)
    {
        snprintf(line, sizeof(line), "Filter %c: %s", fm_drive_letter(panel->filesystem), temp);
        fm_draw_field(0, Option.Height - 1, Option.Width, line, FM_ANSI_STATUS, YELLOW, BLACK);
        fm_prompt_cursor(strlen(line), 1);
        c = MMgetchar();
        fm_prompt_cursor(strlen(line), 0);
        if (c == '\r' || c == '\n')
        {
            if (len == 0)
                strcpy(panel->filter, "*");
            else
                strcpy(panel->filter, temp);
            return 1;
        }
        if (c == ESC)
            return 0;
        if (c == BKSP)
        {
            if (len > 0)
                temp[--len] = 0;
            continue;
        }
        if (isprint(c) && len < FF_MAX_LFN - 1)
        {
            temp[len++] = c;
            temp[len] = 0;
        }
    }
}

static void fm_draw_panel_row(fm_panel_t *panel, int active, int x, int width, int list_top, int list_rows, int row, char *errmsg, int errmsglen)
{
    int idx;
    fm_entry_t e;
    char item[STRINGSIZE * 2] = {0};
    idx = panel->top + row;
    if (row < 0 || row >= list_rows)
        return;
    if (idx >= 0 && idx < panel->count)
    {
        if (fm_get_entry_at(panel, idx, &e, errmsg, errmsglen))
            fm_format_panel_entry(panel, idx, item, sizeof(item));
    }
    fm_draw_field(x, list_top + row, width, item,
                  idx == panel->selected ? (active ? FM_ANSI_ACTIVE : FM_ANSI_INACTIVE) : FM_ANSI_NORMAL,
                  (idx == panel->selected ? BLACK : WHITE),
                  (idx == panel->selected ? (active ? CYAN : WHITE) : BLACK));
}

static int fm_name_starts_with_icase(const char *name, const char *prefix)
{
    while (*prefix)
    {
        if (*name == 0)
            return 0;
        if (tolower((unsigned char)*name) != tolower((unsigned char)*prefix))
            return 0;
        name++;
        prefix++;
    }
    return 1;
}

static int fm_find_prefix_match(fm_panel_t *panel, const char *prefix, int *match_index, char *errmsg, int errmsglen)
{
    int i;
    int key;

    if (!panel || !prefix || !prefix[0])
        return 0;

    if (!fm_build_panel_cache(panel, errmsg, errmsglen))
        return 0;

    key = tolower((unsigned char)prefix[0]);

    /* fm_build_panel_cache() can return success with an empty panel; the
       cache pointers are only guaranteed once there are entries. */
    if (!panel->type_head || !panel->type_next || !panel->entries)
        return 0;

    for (i = panel->type_head[key]; i != -1; i = panel->type_next[i])
    {
        if (fm_name_starts_with_icase(panel->entries[i].name, prefix))
        {
            if (match_index)
                *match_index = i;
            return 1;
        }
    }

    return 0;
}

static int fm_mkdir_one(int filesystem, const char *path)
{
    int oldfs = FatFSFileSystem;
    int oldabort = OptionFileErrorAbort;
    int ok = 0;

    if (path == NULL || path[0] == 0 || (path[0] == '/' && path[1] == 0))
        return 1;

    if (filesystem == 0)
    {
        int rc = lfs_mkdir(&lfs, path);
        ok = (rc == 0 || rc == LFS_ERR_EXIST);
    }
    else
    {
        FatFSFileSystem = filesystem;
        OptionFileErrorAbort = 0;
        if (InitSDCard())
        {
            FRESULT fr = f_mkdir(path);
            ok = (fr == FR_OK || fr == FR_EXIST);
        }
    }

    OptionFileErrorAbort = oldabort;
    FatFSFileSystem = oldfs;
    return ok;
}

static int fm_ensure_dir(int filesystem, const char *path)
{
    char partial[FM_PATH_MAX];
    int i;

    if (path == NULL || path[0] == 0 || (path[0] == '/' && path[1] == 0))
        return 1;
    if (strlen(path) >= sizeof(partial))
        return 0;

    strcpy(partial, path);
    for (i = 1; partial[i]; i++)
    {
        if (partial[i] == '/')
        {
            partial[i] = 0;
            if (partial[0] && !fm_mkdir_one(filesystem, partial))
                return 0;
            partial[i] = '/';
        }
    }
    return fm_mkdir_one(filesystem, partial);
}

static void fm_copy_progress(const char *item, int files, int dirs)
{
    char line[STRINGSIZE * 2];
    snprintf(line, sizeof(line), "Copying... %d file%s, %d dir%s : %s",
             files, files == 1 ? "" : "s",
             dirs, dirs == 1 ? "" : "s",
             item ? item : "");
    fm_draw_field(0, Option.Height - 1, Option.Width, line, FM_ANSI_STATUS, YELLOW, BLACK);
    routinechecks();
}

/* CMM2: a file copied with FatFs, srcfile to dstfile (each names its drive: "A:/dir/name", "C:/name"), both drives
   being FatFs here; the PicoMite copies through its A2A, A2B, B2A and B2B. 0, with the reason in status, if it was
   not copied. */
#define FM_COPY_BUFSIZE 4096
extern const int ErrorMap[];
extern const char *FErrorMsg[];

static int fm_copy_one(const char *srcfile, const char *dstfile, char *status, int statuslen)
{
    FIL *in = GetInternalMemoryNull(sizeof(FIL));       // (as BasicFileOpen has its files)
    FIL *out = GetInternalMemoryNull(sizeof(FIL));
    char *buf = GetMemoryNull(FM_COPY_BUFSIZE);
    FRESULT fr = FR_OK, fc;
    UINT nr = 0, nw = 0;
    int full = 0, ok = 0;

    if (in == NULL || out == NULL || buf == NULL)
        snprintf(status, statuslen, "Not enough memory to copy");
    else if ((fr = f_open(in, srcfile, FA_READ)) != FR_OK)
        snprintf(status, statuslen, "%s: %s", srcfile, FErrorMsg[ErrorMap[fr]]);
    else
    {
        if ((fr = f_open(out, dstfile, FA_WRITE | FA_CREATE_ALWAYS)) != FR_OK)
            snprintf(status, statuslen, "%s: %s", dstfile, FErrorMsg[ErrorMap[fr]]);
        else
        {
            while ((fr = f_read(in, buf, FM_COPY_BUFSIZE, &nr)) == FR_OK && nr > 0)
            {
                if ((fr = f_write(out, buf, nr, &nw)) != FR_OK)
                    break;
                if (nw < nr)
                {
                    full = 1;
                    break;
                }
                if (CurrentlyPlaying != P_NOTHING)
                    checkWAVinput();
                routinechecks();
            }
            fc = f_close(out);
            if (fr == FR_OK)
                fr = fc;
            if (fr != FR_OK)
                snprintf(status, statuslen, "%s: %s", dstfile, FErrorMsg[ErrorMap[fr]]);
            else if (full)
                snprintf(status, statuslen, "%s: disk full", dstfile);
            else
                ok = 1;
            if (!ok)
                f_unlink(dstfile);                      // (not left part-written)
        }
        f_close(in);
    }
    FreeMemorySafe((void **)&buf);
    FreeMemorySafe((void **)&out);
    FreeMemorySafe((void **)&in);
    return ok;
}

static int fm_copy_file_path(int srcfs, const char *srcfile, int dstfs, const char *dstfile, char *status, int statuslen, int *file_count, int *dir_count)
{
    if (!fm_init_drive(srcfs) || !fm_init_drive(dstfs))
    {
        snprintf(status, statuslen, "Drive not ready");
        return 0;
    }
    if (!fm_copy_one(srcfile, dstfile, status, statuslen))
        return 0;
    if (file_count)
    {
        (*file_count)++;
        fm_copy_progress(dstfile, *file_count, dir_count ? *dir_count : 0);
    }
    return 1;
}

static int fm_path_contains_icase(const char *parent, const char *child)
{
    int i = 0;
    while (parent[i] && child[i])
    {
        if (tolower(parent[i]) != tolower(child[i]))
            return 0;
        i++;
    }
    if (parent[i] == 0)
        return child[i] == 0 || child[i] == '/';
    return 0;
}

static int fm_copy_dir_recursive(int srcfs, const char *srcdir, int dstfs, const char *dstdir, char *status, int statuslen, int *file_count, int *dir_count)
{
    if (!fm_ensure_dir(dstfs, dstdir))
    {
        snprintf(status, statuslen, "Cannot create %c:%s", fm_drive_letter(dstfs), dstdir);
        return 0;
    }
    if (dir_count)
    {
        (*dir_count)++;
        fm_copy_progress(dstdir, file_count ? *file_count : 0, *dir_count);
    }

    if (srcfs == 0)
    {
        lfs_dir_t dir;
        struct lfs_info info;
        int rc;
        memset(&dir, 0, sizeof(dir));
        memset(&info, 0, sizeof(info));
        rc = lfs_dir_open(&lfs, &dir, srcdir);
        if (rc < 0)
        {
            snprintf(status, statuslen, "Cannot open %c:%s", fm_drive_letter(srcfs), srcdir);
            return 0;
        }
        while ((rc = lfs_dir_read(&lfs, &dir, &info)) > 0)
        {
            // One src/dst pair, reused for either the child-dir recursion or the
            // file copy (mutually exclusive). Halves the per-recursion-level
            // path-buffer footprint, which is what bounds deep-tree copy depth.
            char src[FM_PATH_MAX], dst[FM_PATH_MAX];
            if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
                continue;
            if (info.type == LFS_TYPE_DIR)
            {
                if (!fm_make_child_path(srcdir, info.name, src, sizeof(src)) ||
                    !fm_make_child_path(dstdir, info.name, dst, sizeof(dst)))
                {
                    lfs_dir_close(&lfs, &dir);
                    snprintf(status, statuslen, "Path too long");
                    return 0;
                }
                if (!fm_copy_dir_recursive(srcfs, src, dstfs, dst, status, statuslen, file_count, dir_count))
                {
                    lfs_dir_close(&lfs, &dir);
                    return 0;
                }
            }
            else
            {
                if (!fm_make_full_filename(srcfs, srcdir, info.name, src, sizeof(src)) ||
                    !fm_make_full_filename(dstfs, dstdir, info.name, dst, sizeof(dst)))
                {
                    lfs_dir_close(&lfs, &dir);
                    snprintf(status, statuslen, "Path too long");
                    return 0;
                }
                if (!fm_copy_file_path(srcfs, src, dstfs, dst, status, statuslen, file_count, dir_count))
                {
                    lfs_dir_close(&lfs, &dir);
                    return 0;
                }
            }
        }
        lfs_dir_close(&lfs, &dir);
        if (rc < 0)
        {
            snprintf(status, statuslen, "Read error on %c:%s", fm_drive_letter(srcfs), srcdir);
            return 0;
        }
    }
    else
    {
        int oldfs = FatFSFileSystem;
        int oldabort = OptionFileErrorAbort;
        DIR dir;
        FILINFO info;
        FRESULT fr;
        FatFSFileSystem = srcfs;
        OptionFileErrorAbort = 0;
        if (!InitSDCard())
        {
            OptionFileErrorAbort = oldabort;
            FatFSFileSystem = oldfs;
            snprintf(status, statuslen, "Drive %c: not ready", fm_drive_letter(srcfs));
            return 0;
        }
        memset(&dir, 0, sizeof(dir));
        memset(&info, 0, sizeof(info));
        fr = f_findfirst(&dir, &info, srcdir, "*");
        while (fr == FR_OK && info.fname[0])
        {
            // See the littlefs branch: one src/dst pair reused for the child-dir
            // recursion or the file copy, to bound per-level stack use.
            char src[FM_PATH_MAX], dst[FM_PATH_MAX];
            if (strcmp(info.fname, ".") == 0 || strcmp(info.fname, "..") == 0)
            {
                fr = f_findnext(&dir, &info);
                continue;
            }
            if (info.fattrib & (AM_SYS | AM_HID))
            {
                fr = f_findnext(&dir, &info);
                continue;
            }
            if (info.fattrib & AM_DIR)
            {
                if (!fm_make_child_path(srcdir, info.fname, src, sizeof(src)) ||
                    !fm_make_child_path(dstdir, info.fname, dst, sizeof(dst)))
                {
                    f_closedir(&dir);
                    OptionFileErrorAbort = oldabort;
                    FatFSFileSystem = oldfs;
                    snprintf(status, statuslen, "Path too long");
                    return 0;
                }
                if (!fm_copy_dir_recursive(srcfs, src, dstfs, dst, status, statuslen, file_count, dir_count))
                {
                    f_closedir(&dir);
                    OptionFileErrorAbort = oldabort;
                    FatFSFileSystem = oldfs;
                    return 0;
                }
            }
            else
            {
                if (!fm_make_full_filename(srcfs, srcdir, info.fname, src, sizeof(src)) ||
                    !fm_make_full_filename(dstfs, dstdir, info.fname, dst, sizeof(dst)))
                {
                    f_closedir(&dir);
                    OptionFileErrorAbort = oldabort;
                    FatFSFileSystem = oldfs;
                    snprintf(status, statuslen, "Path too long");
                    return 0;
                }
                if (!fm_copy_file_path(srcfs, src, dstfs, dst, status, statuslen, file_count, dir_count))
                {
                    f_closedir(&dir);
                    OptionFileErrorAbort = oldabort;
                    FatFSFileSystem = oldfs;
                    return 0;
                }
            }
            fr = f_findnext(&dir, &info);
        }
        f_closedir(&dir);
        OptionFileErrorAbort = oldabort;
        FatFSFileSystem = oldfs;
        if (fr != FR_OK)
        {
            snprintf(status, statuslen, "Read error on %c:%s", fm_drive_letter(srcfs), srcdir);
            return 0;
        }
    }

    return 1;
}

static int fm_copy_selected(fm_panel_t *src, fm_panel_t *dst, char *status, int statuslen)
{
    fm_entry_t entry;
    char errmsg[STRINGSIZE];
    // One src/dst pair reused for the dir branch (child paths) and the file
    // branch (full filenames) — they are never live at the same time. Keeps
    // this frame ~0.5 KB smaller in the FM copy stack chain.
    char srcpath[FM_PATH_MAX];
    char dstpath[FM_PATH_MAX];
    int file_count = 0;
    int dir_count = 0;
    if (src->count <= 0 || src->selected < 0 || src->selected >= src->count)
    {
        snprintf(status, statuslen, "No file selected");
        return 0;
    }
    if (!fm_get_entry_at(src, src->selected, &entry, errmsg, sizeof(errmsg)))
    {
        snprintf(status, statuslen, "%s", errmsg);
        return 0;
    }
    if (entry.is_dir)
    {
        if (!fm_make_child_path(src->path, entry.name, srcpath, sizeof(srcpath)) ||
            !fm_make_child_path(dst->path, entry.name, dstpath, sizeof(dstpath)))
        {
            snprintf(status, statuslen, "Path too long");
            return 0;
        }
        if (src->filesystem == dst->filesystem && fm_path_contains_icase(srcpath, dstpath))
        {
            snprintf(status, statuslen, "Destination inside source not allowed");
            return 0;
        }
        if (!fm_copy_dir_recursive(src->filesystem, srcpath, dst->filesystem, dstpath, status, statuslen, &file_count, &dir_count))
            return 0;
        snprintf(status, statuslen, "Copied directory %s (%d files, %d dirs)", entry.name, file_count, dir_count);
        return 1;
    }
    if (!fm_make_full_filename(src->filesystem, src->path, entry.name, srcpath, sizeof(srcpath)) ||
        !fm_make_full_filename(dst->filesystem, dst->path, entry.name, dstpath, sizeof(dstpath)))
    {
        snprintf(status, statuslen, "Path too long");
        return 0;
    }
    if (fm_stricmp(srcpath, dstpath) == 0)
    {
        snprintf(status, statuslen, "Source and destination are the same");
        return 0;
    }

    if (!fm_copy_file_path(src->filesystem, srcpath, dst->filesystem, dstpath, status, statuslen, &file_count, &dir_count))
        return 0;

    snprintf(status, statuslen, "Copied %s", entry.name);
    return 1;
}

static int fm_copy_marked(fm_panel_t *src, fm_panel_t *dst, char *status, int statuslen)
{
    int i;
    int copied = 0;
    int old_selected = src->selected;

    if (!fm_build_panel_cache(src, status, statuslen))
        return 0;
    if (src->marked_count <= 0)
        return fm_copy_selected(src, dst, status, statuslen);

    for (i = 1; i < src->entry_count; i++)
    {
        if (!src->marks[i] || !fm_entry_is_markable(&src->entries[i]))
            continue;
        src->selected = i;
        if (!fm_copy_selected(src, dst, status, statuslen))
        {
            src->selected = old_selected;
            return 0;
        }
        copied++;
    }

    src->selected = old_selected;
    snprintf(status, statuslen, "Copied %d marked item%s", copied, copied == 1 ? "" : "s");
    return copied > 0;
}

/* CMM2: the program runs as RUN "file" at the prompt does: FM leaves it there as END cmd$ leaves its command, and
   returns; at the program's end (or its error) the prompt runs FM again (fm_program_launched_from_fm). The PicoMite
   loads and starts the program itself. */
static int fm_run_bas_program(const char *filename, const char *fullpath)
{
    if (fullpath == NULL || fullpath[0] == 0 || strlen(fullpath) + 7 > MAXSTRLEN)
        return 0;
    strcpy(EndLine, "RUN \"");
    strcat(EndLine, fullpath);
    strcat(EndLine, "\"");
    return 1;
}

static void fm_set_default_drive_dir(const fm_panel_t *panel)
{
    char cwd[FF_MAX_LFN + 3];

    FatFSFileSystem = panel->filesystem;

    cwd[0] = fm_drive_letter(panel->filesystem);
    cwd[1] = ':';
    if (fm_path_is_root(panel->path))
    {
        cwd[2] = '/';
        cwd[3] = 0;
    }
    else
    {
        strncpy(&cwd[2], panel->path, FF_MAX_LFN);
        cwd[FF_MAX_LFN + 2] = 0;
    }

    {                                                   // CMM2: CHDIR cwd, the drive and its directory
        char arg[FF_MAX_LFN + 8];
        char *saved_cmdline = cmdline;
        snprintf(arg, sizeof(arg), "\"%s\"", cwd);
        cmdline = arg;
        cmd_chdir();
        cmdline = saved_cmdline;
    }
}

/* CMM2: fn(arg) with MMBasic's errors caught, as the PicoMite's FM catches them around LOAD and LIST: 1 if it ran to
   its end, else 0 with the error's message in status. The message is not shown at the prompt (errstring cleared),
   and what error() changed for the prompt is put back: the options (it reloads them: Option.Width and Option.Height
   are the display's again with the font) and the font and colours. */
static int fm_try(void (*fn)(void *), void *arg, char *status, int status_len)
{
    jmp_buf saved_mark;
    char *saved_cmdline = cmdline;
    int font = gui_font, fc = gui_fcolour, bc = gui_bcolour;
    volatile int ok = 1;

    memcpy(saved_mark, mark, sizeof(jmp_buf));
    MMErrMsg = "";
    if (setjmp(mark) != 0)
    {
        ok = 0;
        snprintf(status, status_len, "Error: %s", MMErrMsg[0] ? MMErrMsg : "failed");
        errpos = 0;
        errstring[0] = 0;
        SetFont(font);
        gui_fcolour = fc;
        gui_bcolour = bc;
    }
    else
        fn(arg);
    memcpy(mark, saved_mark, sizeof(jmp_buf));
    cmdline = saved_cmdline;
    ClearTempMemory();                                  // (a command's temporary memory goes at the prompt's next line)
    return ok;
}

// a command (its cmd_ function) with args, as typed after its name: PLAY WAV "A:/x.wav" is cmd_play, WAV "A:/x.wav"
static void (*fm_try_cmd)(void);
static void fm_run_command(void *args)
{
    cmdline = (char *)args;
    fm_try_cmd();
}

static int fm_try_command(void (*cmd)(void), char *args, char *status, int status_len)
{
    fm_try_cmd = cmd;
    return fm_try(fm_run_command, args, status, status_len);
}

static int fm_audio_uses_vs1053(void)
{
    return 0;
}

static void fm_adjust_volume(int delta, char *status, int status_len)
{
    int v = vol_target_left + delta;

    if (v < 0)
        v = 0;
    if (v > 100)
        v = 100;

    vol_target_left = v;
    vol_target_right = v;


    snprintf(status, status_len, "Volume %d%%", v);
}

/* Can a file of this audio type play on this board?  0 with the reason in
   status - the checks ENTER on a file and a track list share. */
static int fm_audio_type_ready(int audio_type, char *status, int status_len)
{
    if (!AUDIO_CONFIGURED)
    {
        snprintf(status, status_len, "Audio not enabled");
        return 0;
    }

    if (audio_type == FM_AUDIO_MIDI && !fm_audio_uses_vs1053())
    {
        snprintf(status, status_len, "MIDI files cannot be played");   // CMM2: no MIDI player
        return 0;
    }

    return 1;                                           // (CMM2: MOD and MP3 always play)
}

static int fm_play_audio_file(const fm_panel_t *panel, const fm_entry_t *entry, char *status, int status_len)
{
    char full[FM_PATH_MAX];
    int audio_type = fm_name_audio_type(entry->name);

    if (audio_type == FM_AUDIO_NONE)
        return 0;

    if (!fm_audio_type_ready(audio_type, status, status_len))
        return 1;

    if (!fm_make_full_filename(panel->filesystem, panel->path, entry->name, full, sizeof(full)))
    {
        snprintf(status, status_len, "Path too long");
        return 1;
    }

    fm_set_default_drive_dir(panel);
    if (CurrentlyPlaying != P_NOTHING)
        CloseAudio(1);

    {                                                   // CMM2: PLAY WAV, FLAC, MP3 or MODFILE (the PicoMite calls the
        char args[FM_PATH_MAX + 16];                    // players' callbacks)
        const char *how = audio_type == FM_AUDIO_WAV ? "WAV" : audio_type == FM_AUDIO_FLAC ? "FLAC" :
                          audio_type == FM_AUDIO_MP3 ? "MP3" : audio_type == FM_AUDIO_MOD ? "MODFILE" : NULL;
        if (how == NULL)
            return 0;
        snprintf(args, sizeof(args), "%s \"%s\"", how, full);
        if (!fm_try_command(cmd_play, args, status, status_len))
            return 1;
    }

    if (CurrentlyPlaying == P_NOTHING)
        snprintf(status, status_len, "Unable to play %s", entry->name);
    else
        snprintf(status, status_len, "Playing %s", entry->name);
    return 1;
}

/* The track list: ENTER with marked entries that are all audio files plays
   them in panel order as the PLAY command's album (alist), so a finished
   track starts the next and , . step back and forward, as PLAY PREVIOUS and
   PLAY NEXT do.  Any other mix of marks leaves ENTER acting on the selected
   entry as usual. */
static int fm_marked_all_audio(const fm_panel_t *panel)
{
    int i;

    if (panel->marked_count <= 0 || !panel->cache_valid || panel->marks == NULL)
        return 0;
    for (i = 1; i < panel->entry_count; i++)
    {
        if (!panel->marks[i])
            continue;
        if (panel->entries[i].is_dir || fm_name_audio_type(panel->entries[i].name) == FM_AUDIO_NONE)
            return 0;
    }
    return 1;
}

static int fm_album_playing(void)
{
    return alist != NULL && (CurrentlyPlaying == P_WAV || CurrentlyPlaying == P_FLAC ||
                             CurrentlyPlaying == P_MP3);   // (CMM2: no MIDI)
}

static void fm_album_status(char *status, int status_len)
{
    const char *fn = alist[trackplaying].fn;
    const char *base = strrchr(fn, '/');

    snprintf(status, status_len, "Playing %d/%d %s", trackplaying + 1, trackstoplay + 1, base ? base + 1 : fn);
}

static void fm_play_first_track(void *unused)
{
    PlayAlbumTrack(0);
}

/* Play the marked files as a track list.  1 when the first track is playing,
   0 with the reason in status; nothing is started unless every file passes. */
static int fm_play_marked(fm_panel_t *panel, char *status, int status_len)
{
    char full[FM_PATH_MAX];
    int i;
    int n = 0;

    if (!AUDIO_CONFIGURED)
    {
        snprintf(status, status_len, "Audio not enabled");
        return 0;
    }
    if (panel->marked_count > MAXALBUM)
    {
        snprintf(status, status_len, "Too many marked for a track list (max %d)", MAXALBUM);
        return 0;
    }
    for (i = 1; i < panel->entry_count; i++)
    {
        int audio_type;

        if (!panel->marks[i])
            continue;
        audio_type = fm_name_audio_type(panel->entries[i].name);
        if (audio_type == FM_AUDIO_MOD)
        {
            snprintf(status, status_len, "MOD files cannot be in a track list");
            return 0;
        }
        if (!fm_audio_type_ready(audio_type, status, status_len))
            return 0;
        if (!fm_make_full_filename(panel->filesystem, panel->path, panel->entries[i].name, full, sizeof(full)) ||
            strlen(full) >= sizeof(alist[0].fn))
        {
            snprintf(status, status_len, "Path too long: %s", panel->entries[i].name);
            return 0;
        }
    }

    fm_set_default_drive_dir(panel);
    if (CurrentlyPlaying != P_NOTHING)
        CloseAudio(1);
    alist = GetInternalMemoryNull(sizeof(a_flist) * MAXALBUM);   // (as PLAY's own track lists; CloseAudio frees it)
    if (alist == NULL)
    {
        snprintf(status, status_len, "Not enough memory for a track list");
        return 0;
    }
    for (i = 1; i < panel->entry_count; i++)
    {
        if (!panel->marks[i])
            continue;
        fm_make_full_filename(panel->filesystem, panel->path, panel->entries[i].name, alist[n].fn, sizeof(alist[n].fn));
        n++;
    }
    trackstoplay = n - 1;
    if (!fm_try(fm_play_first_track, NULL, status, status_len))   // CMM2: a file that does not play stops with an error
    {
        CloseAudio(1);
        return 0;
    }
    if (CurrentlyPlaying == P_NOTHING)
    {
        snprintf(status, status_len, "Unable to play %s", strrchr(alist[0].fn, '/') + 1);
        CloseAudio(1);
        return 0;
    }
    fm_album_status(status, status_len);
    return 1;
}

static int fm_show_image_file(const fm_panel_t *panel, const fm_entry_t *entry, char *status, int status_len)
{
    char full[FM_PATH_MAX];
    char loadline[FM_PATH_MAX + 16];
    int image_type = fm_name_image_type(entry->name);
    int fm_font_local = gui_font;                       // CMM2: FM's font is the editor's (PromptFont is the prompt's)

    if (image_type == FM_IMAGE_NONE)
        return 0;


    if (!fm_make_full_filename(panel->filesystem, panel->path, entry->name, full, sizeof(full)))
    {
        snprintf(status, status_len, "Path too long");
        return 1;
    }

    fm_set_default_drive_dir(panel);

    if (image_type == FM_IMAGE_BMP)
    {
        if (snprintf(loadline, sizeof(loadline), "BMP \"%s\"", full) >= (int)sizeof(loadline))
        {
            snprintf(status, status_len, "Path too long");
            return 1;
        }
    }
    else if (image_type == FM_IMAGE_JPG)
    {
        if (snprintf(loadline, sizeof(loadline), "JPG \"%s\"", full) >= (int)sizeof(loadline))
        {
            snprintf(status, status_len, "Path too long");
            return 1;
        }
    }
    else
    {
        if (snprintf(loadline, sizeof(loadline), "PNG \"%s\"", full) >= (int)sizeof(loadline))
        {
            snprintf(status, status_len, "Path too long");
            return 1;
        }
    }


    if (fm_try_command(cmd_load, loadline, status, status_len))   // CMM2: LOAD BMP, JPG or PNG, its errors caught
    {
        fm_wait_for_escape();
        fm_flush_keyboard_buffer();
        snprintf(status, status_len, "%s", entry->name);
    }

    SetFont(fm_font_local);

    ClearScreen(gui_bcolour);
    CurrentX = 0;
    CurrentY = 0;
    fm_local_cache_invalidate();

    return 1;
}

/* CMM2: the file listed as LIST "file" does, a page at a time, and a key waited for at its end (the PicoMite's
   ListFilePaged(file, 1)) */
static void fm_list_paged(void *file)
{
    ListFile((char *)file, 0);
    MMPrintString("PRESS ANY KEY ...");
    fm_wait_for_escape();
}

static int fm_list_file(const fm_panel_t *panel, const fm_entry_t *entry, char *status, int status_len)
{
    char full[FM_PATH_MAX];
    unsigned char old_break_key = BreakKey;

    if (entry->is_dir)
    {
        snprintf(status, status_len, "Select a file to list");
        return 1;
    }

    if (!fm_make_full_filename(panel->filesystem, panel->path, entry->name, full, sizeof(full)))
    {
        snprintf(status, status_len, "Path too long");
        return 1;
    }

    fm_set_default_drive_dir(panel);

    MMAbort = false;
    fm_flush_keyboard_buffer();

    if ((OptionConsole & 2))
    {
        ClearScreen(gui_bcolour);
        CurrentX = 0;
        CurrentY = 0;
        fm_local_cache_invalidate();
    }

    BreakKey = fm_break_key_for_list;

    fm_try(fm_list_paged, full, status, status_len);    // CMM2: LIST "file", its errors caught
    BreakKey = old_break_key;

    MMAbort = false;

    clearrepeat();
    fm_flush_keyboard_buffer();

    // Always return to the selected filename status after LIST exits (including CTRL-C).
    snprintf(status, status_len, "%s", entry->name);

    return 1;
}

/* CMM2: the display as the editor sets it up (kilo.c's cmd_newedit): the mode, the editor's font and the CLUT, so
   that Option.Width and Option.Height are the display's columns and rows; and what the last program left (its
   variables, sprites, the I2C and the mouse) cleared away, as EDIT clears it. The PicoMite clears the runtime and
   keeps its display as the program left it. */
static void fm_display_setup(void)
{
    i2c_disable();
    i2c2_disable();
    i2c3_disable();
    mouse0close();
    ClearVars(0);
    closeallsprites();
    closeall3d();
    hidecursor(0);
    cursorenable = 0;
    FreeMemorySafe((void **)&cursorsave);
    SymBindFree();
    if (CurrentlyPlaying == P_NOTHING)
    {
        CloseAllFiles();
        InitHeap();
    }
    setmode(Option.mode, DEFCOLOUR, 0, Option.mode == 12 ? 1 : 0);
    SetFont(((Option.editfont - 1) << 4) | 1);
    reset_CLUT();
    pagesetdone = 0;
    if (Option.colourmode == -1)
    {
        CLUT[0] = 0xFFFFFF;
        CLUT[255] = 0;
        CLUT[3] = 0xFFFF;
        while (!pagesetdone)
            CheckAbort();
        HAL_LTDC_ConfigCLUT(&hltdc, CLUT, 256, 0);
    }
}

// CMM2: and back as the prompt has it, as the editor leaves it
static void fm_display_restore(void)
{
    gui_fcolour = PromptFC;
    gui_bcolour = PromptBC;
    MX470Display(DISPLAY_CLS);
    MX470Cursor(0, 0);
    SetFont(Option.DefaultFont);
    setterminal();
    setmode(Option.mode, DEFCOLOUR, 0, Option.mode == 12 ? 1 : 0);
    reset_CLUT();
}

// CMM2: a track of the list, its errors caught (, and . in FM)
static int fm_play_track_n;
static void fm_play_track(void *unused)
{
    PlayAlbumTrack(fm_play_track_n);
}

/* CMM2: error(), for a program FM ran: FM, back, shows the error, and E (or F4) on the program edits it at the line.
   line is the program's line the error was in (0 if none, or a line of an #INCLUDE file or the library). */
void FmNoteError(int line)
{
    if (!fm_program_launched_from_fm)
        return;
    fm_error_location_valid = 0;
    fm_error_line = 0;
    fm_error_char = 0;
    fm_error_file[0] = 0;
    if (line > 0 && fm_last_launched_bas[0])
    {
        fm_error_location_valid = 1;
        fm_error_line = line;
        fm_error_char = 0;
        strncpy(fm_error_file, fm_last_launched_bas, sizeof(fm_error_file) - 1);
        fm_error_file[sizeof(fm_error_file) - 1] = 0;
    }
    if (*MMErrMsg)
    {
        if (fm_error_location_valid)
            snprintf(fm_relaunch_status, STRINGSIZE, "Error: %s (E to edit line %d)", MMErrMsg, fm_error_line);
        else
            snprintf(fm_relaunch_status, STRINGSIZE, "Error: %s", MMErrMsg);
    }
    else if (fm_error_location_valid)
        snprintf(fm_relaunch_status, STRINGSIZE, "Error (E to edit line %d)", fm_error_line);
    else
        strcpy(fm_relaunch_status, "Error");
    fm_relaunch_status_valid = 1;
}

void cmd_fm(void)
{
    fm_panel_t panels[2];
    char status[STRINGSIZE * 2];
    char errmsg[STRINGSIZE];
    char run_bas_file[FF_MAX_LFN + 1];
    char run_bas_full[FM_PATH_MAX];
    char run_edit_file[FM_PATH_MAX + 3];
    int active = 0;
    int context_state = 0;
    int done = 0;
    int run_from_fm = 0;
    int edit_from_fm = 0;
    int need_full_redraw = 1;
    int need_panel_redraw = 0;
    int type_select_active = 0;
    int type_select_dirty = 0;
    uint64_t type_select_last_input_us = 0;
    int album_shown = -1; /* track the status line shows, -1 when no track list is playing */
    int pi;
    unsigned char break_key_save = BreakKey;
    char type_select_prefix[FF_MAX_LFN + 1];

fm_relaunch:

    if (CurrentLinePtr)
        error("Invalid in a program");
    if (*cmdline && *cmdline != '\'')
        SyntaxError();
    fm_display_setup();                                 // CMM2: (the PicoMite clears its runtime here)
    fm_active = 1;                                      // (the prompt loop clears it, if an error ends FM)

    if (Option.Height < 8)
        error("FM needs at least 8 rows");
    setterminal();                                      // CMM2: the serial terminal asked to be the display's size

    // Trap and ignore Ctrl-C while FM is active; ESC is the exit key.
    fm_break_key_for_list = break_key_save;
    BreakKey = 0;

    run_bas_file[0] = 0;
    run_bas_full[0] = 0;
    run_edit_file[0] = 0;
    type_select_prefix[0] = 0;

    memset(panels, 0, sizeof(panels));
    for (pi = 0; pi < 2; pi++)
    {
        panels[pi].sortorder = FM_SORT_NAME;
    }

    panels[0].filesystem = 1;                           // CMM2: A: (the PicoMite's is its flash drive), and C: if
    if (fm_drive_available(2))                          // there is a USB drive
        panels[1].filesystem = 2;
    else
        panels[1].filesystem = 1;
    panels[0].selected = panels[1].selected = 0;
    panels[0].top = panels[1].top = 0;
    panels[0].count = panels[1].count = 0;
    strcpy(panels[0].filter, "*");
    strcpy(panels[1].filter, "*");
    fm_set_panel_path_from_cwd(&panels[0]);
    fm_set_panel_path_from_cwd(&panels[1]);

    active = 0;
    context_state = fm_load_context(panels, &active);

    if (context_state == 2)
        strcpy(status, "FM context restored (some entries unavailable)");
    else if (context_state == 1)
        strcpy(status, "FM context restored");
    else
        strcpy(status, "FM ready");
    if (fm_relaunch_status_valid)
    {
        strncpy(status, fm_relaunch_status, sizeof(status) - 1);
        status[sizeof(status) - 1] = 0;
        fm_relaunch_status_valid = 0;
    }
    PrintString("\0337\033[2J\033[H\033[?25l");
    MX470Display(DISPLAY_CLS);
    fm_local_cache_invalidate();
    clearrepeat();

    while (!done)
    {
        int c;
        fm_panel_t *p;
        fm_entry_t entry;
        int old_selected;
        int old_top;
        int list_rows = Option.Height - 4;
        int list_top = 2;
        int left_width = (Option.Width - 1) / 2;
        int right_x = left_width + 1;
        int right_width = Option.Width - right_x;

        p = &panels[active];

        if (need_full_redraw)
        {
            fm_draw_ui(panels, active, status);
            // Keep queued printable keys during type-select so multi-letter prefixes are not lost.
            if (!type_select_active)
                fm_flush_keyboard_buffer();
            need_full_redraw = 0;
        }
        else if (need_panel_redraw)
        {
            char left_err[STRINGSIZE];
            char right_err[STRINGSIZE];
            int left_ok;
            int right_ok;

            left_err[0] = 0;
            right_err[0] = 0;
            left_ok = fm_build_panel_cache(&panels[0], left_err, sizeof(left_err));
            right_ok = fm_build_panel_cache(&panels[1], right_err, sizeof(right_err));
            if (!left_ok)
                panels[0].count = 0;
            if (!right_ok)
                panels[1].count = 0;
            fm_clamp_panel(&panels[0], list_rows);
            fm_clamp_panel(&panels[1], list_rows);

            // Serial always uses a single full repaint to avoid many cursor repositions.
            fm_serial_repaint_ui(panels, active, status, left_ok, left_err, right_ok, right_err);

            // Local display uses targeted redraw to avoid unnecessary flashing.
            fm_skip_serial_output = 1;
            fm_draw_panel_entries(p, active == 1 ? 0 : 1,
                                  active == 0 ? 0 : right_x,
                                  active == 0 ? left_width : right_width,
                                  list_top, list_rows,
                                  errmsg, sizeof(errmsg));
            fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);
            fm_skip_serial_output = 0;
            if (!type_select_active)
                fm_flush_keyboard_buffer();
            need_panel_redraw = 0;
        }
        /* Type-select reads its prefix here with MMInkey, so it shows the
           typing cursor itself: after the status text while it waits, hidden
           again before anything repaints or acts on the key. */
        if (type_select_active)
            fm_prompt_cursor(strlen(status), 1);
        c = -1;
        while (c == -1)
        {
            routinechecks();
            /* A track list is playing: keep the status line on the current
               track, and say when the list has finished. */
            if (!type_select_active)
            {
                if (fm_album_playing() && trackplaying != album_shown)
                {
                    album_shown = trackplaying;
                    fm_album_status(status, sizeof(status));
                    fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);
                }
                else if (album_shown >= 0 && CurrentlyPlaying == P_NOTHING)
                {
                    album_shown = -1;
                    snprintf(status, sizeof(status), "Track list finished");
                    fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);
                }
            }
            if (type_select_active && type_select_dirty)
            {
                if (time_us_64() - type_select_last_input_us >= 300000)
                {
                    type_select_dirty = 0;
                    if (type_select_prefix[0])
                    {
                        int match_index = -1;
                        if (fm_find_prefix_match(p, type_select_prefix, &match_index, errmsg, sizeof(errmsg)))
                        {
                            p->selected = match_index;
                            if (p->selected < p->top)
                                p->top = p->selected;
                            else if (p->selected >= p->top + list_rows)
                                p->top = p->selected - list_rows + 1;
                            snprintf(status, sizeof(status), "Type-select: %s", type_select_prefix);
                        }
                        else
                        {
                            snprintf(status, sizeof(status), "No match: %s", type_select_prefix);
                        }
                    }
                    else
                    {
                        snprintf(status, sizeof(status), "Type-select: type filename prefix");
                    }
                    ShowCursor(0);
                    fm_prompt_cursor(strlen(status), 0);
                    fm_draw_ui(panels, active, status);
                    need_full_redraw = 0;
                    fm_prompt_cursor(strlen(status), 1);
                    continue;
                }
            }
            if (type_select_active)
                ShowCursor(1); // the local display's cursor blinks from here, as MMgetchar's does
            c = MMInkey();
        }

        if (type_select_active)
        {
            int consumed = 0;
            int len = strlen(type_select_prefix);

            ShowCursor(0);
            fm_prompt_cursor(strlen(status), 0);
            if (c == ESC)
            {
                type_select_active = 0;
                type_select_dirty = 0;
                type_select_prefix[0] = 0;
                snprintf(status, sizeof(status), "Type-select cancelled");
                need_full_redraw = 1;
                continue;
            }
            if (c == '\r' || c == '\n')
            {
                type_select_active = 0;
                type_select_dirty = 0;
                type_select_prefix[0] = 0;
                snprintf(status, sizeof(status), "Type-select: open selected item");
            }
            if (c == BKSP)
            {
                if (len > 0)
                    type_select_prefix[--len] = 0;
                consumed = 1;
            }
            else if (isprint(c) && c != '/' && len < FF_MAX_LFN)
            {
                type_select_prefix[len++] = (char)((c >= 'a' && c <= 'z') ? mytoupper(c) : c);
                type_select_prefix[len] = 0;
                consumed = 1;
            }

            if (consumed)
            {
                type_select_dirty = 1;
                type_select_last_input_us = time_us_64();
                continue;
            }

            type_select_active = 0;
            type_select_dirty = 0;
            type_select_prefix[0] = 0;
        }

        if (c >= 'a' && c <= 'z')
            c = mytoupper(c);

        old_selected = p->selected;
        old_top = p->top;

        switch (c)
        {
        case ESC:
            done = 1;
            break;
        case '\t':
            active ^= 1;
            fm_set_status_to_selected(&panels[active], status, sizeof(status));
            need_full_redraw = 1;
            break;
        case CTRLKEY('S'):
        case LEFT:
            active = 0;
            fm_set_status_to_selected(&panels[active], status, sizeof(status));
            need_full_redraw = 1;
            break;
        case CTRLKEY('D'):
        case RIGHT:
            active = 1;
            fm_set_status_to_selected(&panels[active], status, sizeof(status));
            need_full_redraw = 1;
            break;
        case CTRLKEY('E'):
        case UP:
            if (p->selected > 0)
                p->selected--;
            if (p->selected < p->top)
                p->top = p->selected;
            if (p->selected != old_selected)
            {
                fm_set_status_to_selected(p, status, sizeof(status));
                if (p->top == old_top)
                {
                    fm_draw_panel_row(p, 1,
                                      active == 0 ? 0 : right_x,
                                      active == 0 ? left_width : right_width,
                                      list_top, list_rows,
                                      old_selected - p->top,
                                      errmsg, sizeof(errmsg));
                    fm_draw_panel_row(p, 1,
                                      active == 0 ? 0 : right_x,
                                      active == 0 ? left_width : right_width,
                                      list_top, list_rows,
                                      p->selected - p->top,
                                      errmsg, sizeof(errmsg));
                    fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);
                    fm_flush_keyboard_buffer();
                }
                else
                    need_panel_redraw = 1;
            }
            break;
        case CTRLKEY('X'):
        case DOWN:
            if (p->selected < p->count - 1)
                p->selected++;
            if (p->selected >= p->top + list_rows)
                p->top = p->selected - list_rows + 1;
            if (p->selected != old_selected)
            {
                fm_set_status_to_selected(p, status, sizeof(status));
                if (p->top == old_top)
                {
                    fm_draw_panel_row(p, 1,
                                      active == 0 ? 0 : right_x,
                                      active == 0 ? left_width : right_width,
                                      list_top, list_rows,
                                      old_selected - p->top,
                                      errmsg, sizeof(errmsg));
                    fm_draw_panel_row(p, 1,
                                      active == 0 ? 0 : right_x,
                                      active == 0 ? left_width : right_width,
                                      list_top, list_rows,
                                      p->selected - p->top,
                                      errmsg, sizeof(errmsg));
                    fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);
                    fm_flush_keyboard_buffer();
                }
                else
                    need_panel_redraw = 1;
            }
            break;
        case CTRLKEY('P'):
        case PUP:
            if (p->selected > 0)
            {
                p->selected -= list_rows;
                if (p->selected < 0)
                    p->selected = 0;
            }
            if (p->top > 0)
            {
                p->top -= list_rows;
                if (p->top < 0)
                    p->top = 0;
            }
            if (p->selected < p->top)
                p->top = p->selected;
            need_panel_redraw = 1;
            break;
        case CTRLKEY('L'):
        case PDOWN:
            if (p->selected < p->count - 1)
            {
                p->selected += list_rows;
                if (p->selected >= p->count)
                    p->selected = p->count - 1;
            }
            if (p->selected >= p->top + list_rows)
                p->top = p->selected - list_rows + 1;
            if (p->count > list_rows && p->top > p->count - list_rows)
                p->top = p->count - list_rows;
            if (p->top < 0)
                p->top = 0;
            need_panel_redraw = 1;
            break;
        case CTRLKEY('U'):
        case HOME:
            strcpy(p->path, "/");
            p->selected = 0;
            p->top = 0;
            p->count = 0;
            p->cache_valid = 0;
            snprintf(status, sizeof(status), "%c:%s", fm_drive_letter(p->filesystem), p->path);
            need_full_redraw = 1;
            break;
        case CTRLKEY('K'):
        case END:
            p->selected = p->count - 1;
            if (p->selected < 0)
                p->selected = 0;
            if (p->selected >= list_rows)
                p->top = p->selected - list_rows + 1;
            else
                p->top = 0;
            need_panel_redraw = 1;
            break;
        case BKSP:
            if (!fm_path_is_root(p->path))
            {
                fm_parent_path(p->path);
                p->selected = p->top = 0;
                p->cache_valid = 0;
                snprintf(status, sizeof(status), "%c:%s", fm_drive_letter(p->filesystem), p->path);
                need_full_redraw = 1;
            }
            break;
        case ' ':
            if (fm_mark_toggle(p, p->selected, errmsg, sizeof(errmsg)))
            {
                if (p->selected < p->count - 1)
                    p->selected++;
                if (p->selected >= p->top + list_rows)
                    p->top = p->selected - list_rows + 1;
                snprintf(status, sizeof(status), "%d marked", p->marked_count);
            }
            else
            {
                snprintf(status, sizeof(status), "Select a file or directory to mark");
            }
            need_full_redraw = 1;
            break;
        case '*':
            if (!fm_build_panel_cache(p, errmsg, sizeof(errmsg)))
            {
                snprintf(status, sizeof(status), "%s", errmsg);
            }
            else
            {
                fm_mark_all(p, errmsg, sizeof(errmsg));
                snprintf(status, sizeof(status), "%d marked", p->marked_count);
            }
            need_full_redraw = 1;
            break;
        case '\\':
            if (!fm_build_panel_cache(p, errmsg, sizeof(errmsg)))
            {
                snprintf(status, sizeof(status), "%s", errmsg);
            }
            else
            {
                fm_mark_clear_all(p, errmsg, sizeof(errmsg));
                snprintf(status, sizeof(status), "Marks cleared");
            }
            need_full_redraw = 1;
            break;
        case F2:
        case '\r':
        case '\n':
            if (fm_marked_all_audio(p))
            {
                album_shown = fm_play_marked(p, status, sizeof(status)) ? trackplaying : -1;
                need_full_redraw = 1;
                break;
            }
            if (p->count > 0 && p->selected >= 0 && p->selected < p->count &&
                fm_get_entry_at(p, p->selected, &entry, errmsg, sizeof(errmsg)))
            {
                if (entry.is_dir)
                {
                    if (strcmp(entry.name, "..") == 0)
                    {
                        fm_parent_path(p->path);
                    }
                    else if (strcmp(entry.name, ".") == 0)
                    {
                        break;
                    }
                    else
                    {
                        char np[FF_MAX_LFN];
                        if (fm_make_child_path(p->path, entry.name, np, sizeof(np)))
                            strcpy(p->path, np);
                    }
                    p->selected = p->top = 0;
                    p->cache_valid = 0;
                    snprintf(status, sizeof(status), "%c:%s", fm_drive_letter(p->filesystem), p->path);
                    need_full_redraw = 1;
                }
                else if (fm_name_is_bas(entry.name))
                {
                    if (!fm_make_full_filename(p->filesystem, p->path, entry.name, run_bas_full, sizeof(run_bas_full)))
                    {
                        snprintf(status, sizeof(status), "Path too long");
                        break;
                    }
                    if (snprintf(run_bas_file, sizeof(run_bas_file), "%s", entry.name) >= (int)sizeof(run_bas_file))
                    {
                        snprintf(status, sizeof(status), "Path too long");
                        break;
                    }
                    snprintf(status, sizeof(status), "Launching %s", entry.name);
                    run_from_fm = 1;
                    done = 1;
                    BreakKey = break_key_save;
                }
                else if (fm_play_audio_file(p, &entry, status, sizeof(status)))
                {
                    album_shown = -1; /* a single file: the track list, if any, is gone */
                    need_full_redraw = 1;
                }
                else if (fm_show_image_file(p, &entry, status, sizeof(status)))
                {
                    need_full_redraw = 1;
                }
            }
            break;
        case 'A':
        case 'B':
        case 'C':
        {
            int newfs = c == 'C' ? 2 : 1;                 // CMM2: A: and B: are the SD card
            if (!fm_drive_available(newfs))
            {
                snprintf(status, sizeof(status), "Drive %c: not ready", fm_drive_letter(newfs));
                fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);
                break;
            }
            p->filesystem = newfs;
            p->selected = p->top = 0;
            p->count = 0;
            p->cache_valid = 0;
            fm_set_panel_path_from_cwd(p);
            if (strncmp(status, "Drive not ready", 15) != 0)
                snprintf(status, sizeof(status), "Switched to %c:", fm_drive_letter(p->filesystem));
            need_full_redraw = 1;
            break;
        }
        case 'G':
            fm_go_to_path(p, status, sizeof(status));
            p->cache_valid = 0;
            need_full_redraw = 1;
            break;
        case 'S':
            p->sortorder++;
            if (p->sortorder > FM_SORT_TYPE)
                p->sortorder = FM_SORT_NAME;
            p->cache_valid = 0;
            p->selected = p->top = 0;
            if (p->sortorder == FM_SORT_TIME)
                snprintf(status, sizeof(status), "Sort: datetime");
            else if (p->sortorder == FM_SORT_TYPE)
                snprintf(status, sizeof(status), "Sort: type then name");
            else
                snprintf(status, sizeof(status), "Sort: name");
            need_full_redraw = 1;
            break;
        case 'N':
        {
            char newname[FF_MAX_LFN + 1];
            int name_limit;

            if (!fm_prompt_text("New file name: ", "", newname, sizeof(newname)))
            {
                snprintf(status, sizeof(status), "Create file cancelled");
                need_full_redraw = 1;
                break;
            }
            if (!fm_is_valid_leaf_name(newname))
            {
                snprintf(status, sizeof(status), "Invalid file name");
                need_full_redraw = 1;
                break;
            }

            if (!fm_build_panel_cache(p, errmsg, sizeof(errmsg)))
            {
                snprintf(status, sizeof(status), "%s", errmsg);
                need_full_redraw = 1;
                break;
            }
            name_limit = fm_panel_name_limit(p);
            if ((int)strlen(newname) > name_limit)
            {
                snprintf(status, sizeof(status), "Name too long (max %d chars here)", name_limit);
                need_full_redraw = 1;
                break;
            }

            if (!fm_create_empty_file(p->filesystem, p->path, newname, status, sizeof(status)))
            {
                need_full_redraw = 1;
                break;
            }
            if (!fm_make_full_filename(p->filesystem, p->path, newname, run_bas_full, sizeof(run_bas_full)))
            {
                snprintf(status, sizeof(status), "Path too long");
                need_full_redraw = 1;
                break;
            }
            if (snprintf(run_edit_file, sizeof(run_edit_file), "\"%s\"", run_bas_full) >= (int)sizeof(run_edit_file))
            {
                snprintf(status, sizeof(status), "Path too long");
                need_full_redraw = 1;
                break;
            }

            fm_pending_edit_seek_valid = 0;
            snprintf(status, sizeof(status), "Created %s", newname);
            edit_from_fm = 1;
            done = 1;
            break;
        }
        case 'D':
            fm_duplicate_selected(p, status, sizeof(status));
            p->count = 0;
            p->cache_valid = 0;
            need_full_redraw = 1;
            break;
        case 'M':
            fm_move_selected(&panels[active], &panels[active ^ 1], status, sizeof(status));
            panels[active].count = 0;
            panels[active ^ 1].count = 0;
            panels[active].cache_valid = 0;
            panels[active ^ 1].cache_valid = 0;
            need_full_redraw = 1;
            break;
        case '/':
            if (p->sortorder != FM_SORT_NAME)
            {
                p->sortorder = FM_SORT_NAME;
                p->cache_valid = 0;
                p->count = 0;
            }
            type_select_active = 1;
            type_select_dirty = 0;
            type_select_prefix[0] = 0;
            snprintf(status, sizeof(status), "Type-select: type filename prefix (ENTER=open, ESC=cancel)");
            need_full_redraw = 1;
            break;
        case 'L':
        case F3:
            if (p->count <= 0 || p->selected < 0 || p->selected >= p->count ||
                !fm_get_entry_at(p, p->selected, &entry, errmsg, sizeof(errmsg)))
            {
                snprintf(status, sizeof(status), "No file selected");
            }
            else
            {
                PrintString("\0337\033[2J\033[H\033[?25l");
                fm_list_file(p, &entry, status, sizeof(status));
            }
            need_full_redraw = 1;
            break;
        case 'F':
        case F9:
            if (fm_prompt_filter(p))
            {
                p->selected = p->top = 0;
                p->count = 0;
                p->cache_valid = 0;
                snprintf(status, sizeof(status), "Filter set to %s", p->filter);
            }
            else
                snprintf(status, sizeof(status), "Filter unchanged");
            need_full_redraw = 1;
            break;
        case 'H':
        case '?':
        case F1:
            fm_show_help_screen();
            need_full_redraw = 1;
            break;
        case 'E':
        case F4:
            if (p->count <= 0 || p->selected < 0 || p->selected >= p->count ||
                !fm_get_entry_at(p, p->selected, &entry, errmsg, sizeof(errmsg)))
            {
                snprintf(status, sizeof(status), "No file selected");
                need_full_redraw = 1;
                break;
            }
            if (entry.is_dir)
            {
                snprintf(status, sizeof(status), "Select a file to edit");
                need_full_redraw = 1;
                break;
            }
            if (!fm_make_full_filename(p->filesystem, p->path, entry.name, run_bas_full, sizeof(run_bas_full)))
            {
                snprintf(status, sizeof(status), "Path too long");
                need_full_redraw = 1;
                break;
            }
            if (snprintf(run_edit_file, sizeof(run_edit_file), "\"%s\"", run_bas_full) >= (int)sizeof(run_edit_file))
            {
                snprintf(status, sizeof(status), "Path too long");
                need_full_redraw = 1;
                break;
            }
            fm_pending_edit_seek_valid = 0;
            if (fm_error_location_valid && fm_error_file[0] && fm_stricmp(run_bas_full, fm_error_file) == 0)
            {
                fm_pending_edit_seek_valid = 1;
                fm_pending_edit_seek_line = fm_error_line > 0 ? fm_error_line - 1 : 0;
                fm_pending_edit_seek_char = fm_error_char;
                if (fm_error_char > 0)
                    snprintf(status, sizeof(status), "Editing %s at %d:%d", entry.name, fm_error_line, fm_error_char + 1);
                else
                    snprintf(status, sizeof(status), "Editing %s at line %d", entry.name, fm_error_line);
            }
            else
            {
                snprintf(status, sizeof(status), "Editing %s", entry.name);
            }
            edit_from_fm = 1;
            done = 1;
            break;
        case 'W':
        case F10:
            strcpy(p->filter, "*");
            p->selected = p->top = 0;
            p->count = 0;
            p->cache_valid = 0;
            snprintf(status, sizeof(status), "Filter cleared");
            need_full_redraw = 1;
            break;
        case 'Y':
        case F5:
            if (!fm_build_panel_cache(&panels[active], errmsg, sizeof(errmsg)))
                snprintf(status, sizeof(status), "%s", errmsg);
            else if (panels[active].marked_count > 0)
                fm_copy_marked(&panels[active], &panels[active ^ 1], status, sizeof(status));
            else
                fm_copy_selected(&panels[active], &panels[active ^ 1], status, sizeof(status));
            panels[active].cache_valid = 0;
            panels[active ^ 1].cache_valid = 0;
            need_full_redraw = 1;
            break;
        case 'T':
        case F6:
            if (CurrentlyPlaying != P_NOTHING)
            {
                CloseAudio(1);
                album_shown = -1;
                snprintf(status, sizeof(status), "Audio stopped");
            }
            else
            {
                snprintf(status, sizeof(status), "No audio playing");
            }
            need_full_redraw = 1;
            break;
        case F7:
        case '-':
            fm_adjust_volume(-FM_VOLUME_STEP, status, sizeof(status));
            fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);
            break;
        case F8:
        case '+':
        case '=':
            fm_adjust_volume(FM_VOLUME_STEP, status, sizeof(status));
            fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);
            break;
        case ',':
        case '<':
        case '.':
        case '>':
            /* Step through the track list, as PLAY PREVIOUS and PLAY NEXT do. */
            if (!fm_album_playing())
                snprintf(status, sizeof(status), "No track list playing");
            else if ((c == ',' || c == '<') && trackplaying == 0)
                snprintf(status, sizeof(status), "First track is playing");
            else if ((c == '.' || c == '>') && trackplaying == trackstoplay)
                snprintf(status, sizeof(status), "Last track is playing");
            else
            {
                fm_play_track_n = trackplaying + ((c == '.' || c == '>') ? 1 : -1);
                if (fm_try(fm_play_track, NULL, status, sizeof(status)))   // CMM2: (its errors caught)
                {
                    album_shown = trackplaying;
                    fm_album_status(status, sizeof(status));
                }
            }
            fm_draw_field(0, Option.Height - 1, Option.Width, status, FM_ANSI_STATUS, YELLOW, BLACK);
            break;
        case 'R':
            fm_rename_selected(p, status, sizeof(status));
            p->cache_valid = 0;
            fm_sync_other_panel_cache(panels, active);
            need_full_redraw = 1;
            break;
        case DEL:
            if (!fm_build_panel_cache(p, errmsg, sizeof(errmsg)))
                snprintf(status, sizeof(status), "%s", errmsg);
            else if (p->marked_count > 0)
                fm_delete_marked(p, 0, status, sizeof(status));
            else
                fm_delete_selected(p, status, sizeof(status));
            p->cache_valid = 0;
            fm_sync_other_panel_cache(panels, active);
            need_full_redraw = 1;
            break;
        case 'X':
            if (!fm_build_panel_cache(p, errmsg, sizeof(errmsg)))
                snprintf(status, sizeof(status), "%s", errmsg);
            else if (p->marked_count > 0)
                fm_delete_marked(p, 1, status, sizeof(status));
            else
                fm_delete_selected_recursive(p, status, sizeof(status));
            p->cache_valid = 0;
            fm_sync_other_panel_cache(panels, active);
            need_full_redraw = 1;
            break;
        case 'K':
        case F11:
            fm_make_directory(p, status, sizeof(status));
            p->cache_valid = 0;
            fm_sync_other_panel_cache(panels, active);
            need_full_redraw = 1;
            break;
        default:
            break;
        }
    }

    fm_save_context(panels, active);

    fm_active = 0;
    PrintString("\033[0m\033[?25h\0338\033[2J\033[H");
    fm_display_restore();                               // CMM2: the prompt's display, as the editor leaves it

    clearrepeat();
    memset(inpbuf, 0, STRINGSIZE);
    for (pi = 0; pi < 2; pi++)
        fm_free_panel_cache(&panels[pi]);
    fm_local_cache_release();

    BreakKey = break_key_save;

fm_check_run:
    if (run_from_fm)
    {
        if (CurrentlyPlaying != P_NOTHING)
            CloseAudio(1);
        fm_set_default_drive_dir(&panels[active]);
        MMAbort = false;
        ConsoleRxBufHead = ConsoleRxBufTail;
        fm_flush_keyboard_buffer();
        routinechecks();
        fm_flush_keyboard_buffer();
        MMAbort = false;
        ConsoleRxBufHead = ConsoleRxBufTail;
        fm_error_location_valid = 0;
        fm_error_file[0] = 0;
        fm_error_line = 0;
        fm_error_char = 0;
        fm_pending_edit_seek_valid = 0;
        strncpy(fm_last_launched_bas, run_bas_full, sizeof(fm_last_launched_bas) - 1);
        fm_last_launched_bas[sizeof(fm_last_launched_bas) - 1] = 0;
        fm_program_launched_from_fm = 1;
        if (!fm_run_bas_program(run_bas_file, run_bas_full))
        {
            fm_program_launched_from_fm = 0;
            fm_last_launched_bas[0] = 0;
        }
    }
    if (edit_from_fm)
    {
        char *saved_cmdline = cmdline;
        int wants_run;
        fm_set_default_drive_dir(&panels[active]);
        ClearTempMemory();
        /* CMM2: EDIT "file" (kilo.c): ESC and F1 come back here; F2 saves the file and comes back with fm_editing 2,
           for FM to run it (the PicoMite's edit() sets fm_edit_wants_run) */
        fm_editing = 1;
        cmdline = run_edit_file;
        cmd_newedit();
        wants_run = (fm_editing == 2);
        fm_editing = 0;
        ClearTempMemory();
        cmdline = saved_cmdline;

        if (wants_run && fm_name_is_bas(run_bas_full))
        {
            // F2 pressed on a .bas file — run it instead of returning to FM.
            // run_bas_full is still valid; derive run_bas_file from it.
            const char *bn = strrchr(run_bas_full, '/');
            strncpy(run_bas_file, bn ? bn + 1 : run_bas_full, sizeof(run_bas_file) - 1);
            run_bas_file[sizeof(run_bas_file) - 1] = 0;
            run_edit_file[0] = 0;
            type_select_prefix[0] = 0;
            type_select_active = 0;
            type_select_dirty = 0;
            type_select_last_input_us = 0;
            edit_from_fm = 0;
            run_from_fm = 1;
            goto fm_check_run;
        }
        else
        {
            // ESC/F1 or non-.bas file — return to FM as before.
            run_bas_file[0] = 0;
            run_bas_full[0] = 0;
            run_edit_file[0] = 0;
            type_select_prefix[0] = 0;
            type_select_active = 0;
            type_select_dirty = 0;
            type_select_last_input_us = 0;
            done = 0;
            run_from_fm = 0;
            edit_from_fm = 0;
            need_full_redraw = 1;
            goto fm_relaunch;
        }
    }
}

