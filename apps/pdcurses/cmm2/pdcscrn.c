/* pdcscrn.c: the CMM2's screen for PDCursesMod (apps/pdcurses/cmm2), in place of the Linux framebuffer port's
   (fb/pdcscrn.c, whose layout it follows). Public domain, as PDCursesMod.

   The screen is an 8-bit MODE, which fb/pdcdisp.c draws into as into a Linux framebuffer of 8 bits a pixel: each
   colour an index, the 256 being PDCurses's palette (xterm's), put on the display with video_palette.
   - The mode: PDC_MODE (mode= after RUN's comma), else the settings file's (PDC_CONFIG: written when Alt-= or
     Alt-Minus changes it), else the mode BASIC is in, MM.INFO(MODE). Only the modes below are used: those whose lines
     are shown twice (3, 5, 6, 7...) are too coarse for text, and one that cannot be set (MODE 16 on a G1) gives way
     to BASIC's, then to MODE 1.
   - The font: PDC_FONT (font=), else the file's, else by the width: 8x16 up to 1280 pixels and 12x24 above (8x16 on
     1920 x 1080 would be 240 x 67 characters). Spleen's, made by pdcurses.py (cmm2_fonts.c).
   - Alt-= steps through the modes, Alt-Minus through the fonts (pdckbd.c), and getch() then gives KEY_RESIZE. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <assert.h>
#include "curspriv.h"
#include "fb/pdcfb.h"
#include "common/psf.c"
#include "common/pdccolor.h"
#include "common/pdccolor.c"
#include "cmm2.h"
#include "cmm2_fonts.h"
#include "pdccmm2.h"

struct video_info PDC_fb;                       /* the page fb/pdcdisp.c draws into */
struct font_info PDC_font_info;
int PDC_rows = -1, PDC_cols = -1;
bool PDC_resize_occurred = FALSE;
int PDC_orientation = 0;                        /* (fb/pdcdisp.c can turn the screen; here it is never turned) */
static mmask_t _stored_trap_mbe;

static const struct { int mode, w, h; } modes[] = {
    {2, 640, 400}, {8, 640, 480}, {1, 800, 600}, {10, 848, 480}, {14, 960, 540}, {18, 1024, 600}, {9, 1024, 768},
    {11, 1280, 720}, {15, 1280, 1024}, {16, 1920, 1080}};
#define NMODES (int)(sizeof(modes) / sizeof(modes[0]))

static int cur_mode = -1, cur_font = -1;        /* modes[] and cmm2_fonts[] indexes */
static int cfg_mode = -1, cfg_font = -1;        /* the settings file's: a MODE's number, a font's index */

static int font_by_name(const char *s) {
    int i, n = (int)strcspn(s, " \t\r\n");
    for(i = 0; i < cmm2_n_fonts; i++)
        if((int)strlen(cmm2_fonts[i].name) == n && strncasecmp(s, cmm2_fonts[i].name, n) == 0) return i;
    return -1;
}

/* the settings file: "MODE 9" and "FONT 12x24", a line each */
static void read_settings(void) {
    const char *path = getenv("PDC_CONFIG");
    char line[64];
    FILE *f;
    if(path == NULL || (f = fopen(path, "r")) == NULL) return;
    while(fgets(line, sizeof(line), f)) {
        if(strncasecmp(line, "MODE ", 5) == 0) cfg_mode = atoi(line + 5);
        else if(strncasecmp(line, "FONT ", 5) == 0) cfg_font = font_by_name(line + 5);
    }
    fclose(f);
}

static void save_settings(void) {
    const char *path = getenv("PDC_CONFIG");
    FILE *f;
    if(path == NULL || cur_mode < 0 || cur_font < 0 || (f = fopen(path, "w")) == NULL) return;
    fprintf(f, "MODE %d\nFONT %s\n", modes[cur_mode].mode, cmm2_fonts[cur_font].name);
    fclose(f);
}

static int basic_mode(void) {                   /* the mode BASIC is in ("1.8": MODE 1,8) */
    char out[256];
    if(cmm2->version < 12 || cmm2->basic_eval("MM.INFO(MODE)", out) != 0) return 1;
    return atoi(out);
}

static void send_palette(void) {
    uint8_t rgb[256 * 3];
    int i;
    for(i = 0; i < 256; i++) {
        const PACKED_RGB c = PDC_get_palette_entry(i);
        rgb[i * 3] = (uint8_t)Get_RValue(c);
        rgb[i * 3 + 1] = (uint8_t)Get_GValue(c);
        rgb[i * 3 + 2] = (uint8_t)Get_BValue(c);
    }
    cmm2->video_palette(0, 256, rgb);
}

void PDC_draw_rectangle(const int xpix, const int ypix,           /* fb/pdcdisp.c */
                        const int xsize, const int ysize, const uint32_t color);

/* the pixels right of the last column and below the last row, which no character covers */
static void clear_unused_part_of_screen(void) {
    const int right_edge = PDC_fb.xres % PDC_font_info.width;
    const int bottom_edge = PDC_fb.yres % PDC_font_info.height;
    PDC_draw_rectangle(0, PDC_fb.yres - bottom_edge, PDC_fb.xres, bottom_edge, 0);
    PDC_draw_rectangle(PDC_fb.xres - right_edge, 0, right_edge, PDC_fb.yres, 0);
}

/* MODE mode,8: 0, or -1 if it is not one of ours or cannot be set (then the screen is as it was) */
static int set_mode(int mode) {
    cmm2_video v;
    int i;
    for(i = 0; i < NMODES && modes[i].mode != mode; i++) {}
    if(i == NMODES || cmm2->video_mode(mode, 8, &v) < 0 || v.copies != 1 || v.bpp != 8) return -1;
    PDC_fb.framebuf = v.fb;
    PDC_fb.xres = v.width;
    PDC_fb.yres = v.height;
    PDC_fb.bits_per_pixel = 8;
    PDC_fb.line_length = v.pitch;
    PDC_fb.smem_len = v.pitch * v.height;
    memset(v.fb, 0, PDC_fb.smem_len);
    cur_mode = i;
    send_palette();
    return 0;
}

static void load_font(int i) {
    if(PDC_font_info.unicode_info) {
        free(PDC_font_info.unicode_info);
        PDC_font_info.unicode_info = NULL;
    }
    load_psf_or_vgafont(&PDC_font_info, cmm2_fonts[i].psf, cmm2_fonts[i].size);
    cur_font = i;
    PDC_cols = PDC_fb.xres / PDC_font_info.width;
    PDC_rows = PDC_fb.yres / PDC_font_info.height;
    clear_unused_part_of_screen();
}

static int default_font(void) {
    int i = font_by_name(PDC_fb.xres > 1280 ? "12x24" : "8x16");
    return i < 0 ? 0 : i;
}

/* A new mode or font, from a key. curses's windows are made the new size at once, before anything draws (the
   cursor's and blinking text's redraw would otherwise put the old size's rows and columns into the new page, past
   its end if it is smaller), and the program is told by KEY_RESIZE, from getch(), to draw its screen again */
static void changed(void) {
    resize_term(0, 0);
    PDC_resize_occurred = TRUE;
    if(SP) SP->resized = TRUE;
    save_settings();
    PDC_cmm2_mouse_show(SP && SP->_trap_mbe);   /* (video_mode put the mouse in the middle) */
}

void PDC_cmm2_cycle_mode(void) {
    int n;
    PDC_cmm2_mouse_show(0);                     /* (the pointer taken off while the screen is the size it was) */
    for(n = 1; n < NMODES; n++)
        if(set_mode(modes[(cur_mode + n) % NMODES].mode) == 0) break;
    load_font(cur_font);
    changed();
}

void PDC_cmm2_cycle_font(void) {
    PDC_cmm2_mouse_show(0);
    load_font((cur_font + 1) % cmm2_n_fonts);
    changed();
}

void PDC_reset_prog_mode(void) {
    SP->_trap_mbe = _stored_trap_mbe;
    PDC_mouse_set();
    PDC_resize_occurred = FALSE;
}

void PDC_reset_shell_mode(void) {}

int PDC_resize_screen(int nlines, int ncols) {  /* (the size is the mode's and the font's: resize_term(0, 0) takes it) */
    INTENTIONALLY_UNUSED_PARAMETER(nlines);
    INTENTIONALLY_UNUSED_PARAMETER(ncols);
    return OK;
}

void PDC_restore_screen_mode(int i) { INTENTIONALLY_UNUSED_PARAMETER(i); }
void PDC_save_screen_mode(int i) { INTENTIONALLY_UNUSED_PARAMETER(i); }

void PDC_scr_close(void) {                      /* (the loader puts BASIC's mode and palette back when the program ends) */
    _stored_trap_mbe = SP->_trap_mbe;
    SP->_trap_mbe = 0;
    PDC_mouse_set();
    PDC_draw_rectangle(0, 0, PDC_fb.xres, PDC_fb.yres, 0);
}

void PDC_scr_free(void) {
    PDC_free_palette();
    if(PDC_font_info.unicode_info) {
        free(PDC_font_info.unicode_info);
        PDC_font_info.unicode_info = NULL;
    }
}

int PDC_scr_open(void) {
    const char *s;
    char path[300];
    int font = -1;

    PDC_LOG(("PDC_scr_open called\n"));
    if(getenv("PDC_CONFIG") == NULL && getcwd(path, 280)) {  /* (a program that has not called PDC_cmm2_args) */
        strcat(path, path[strlen(path) - 1] == '/' ? "pdcurses.cfg" : "/pdcurses.cfg");
        setenv("PDC_CONFIG", path, 1);
    }
    read_settings();
    COLORS = 256;
    assert(SP);
    if(!SP || PDC_init_palette()) return ERR;
    if(((s = getenv("PDC_MODE")) == NULL || set_mode(atoi(s)) != 0) && (cfg_mode < 0 || set_mode(cfg_mode) != 0) &&
       set_mode(basic_mode()) != 0 && set_mode(1) != 0)
        return ERR;
    if((s = getenv("PDC_FONT")) != NULL) font = font_by_name(s);
    if(font < 0) font = cfg_font;
    if(font < 0) font = default_font();
    load_font(font);

    SP->mouse_wait = PDC_CLICK_PERIOD;
    SP->visibility = 0;                         /* no cursor, by default */
    SP->curscol = SP->cursrow = 0;
    SP->audible = TRUE;
    SP->mono = FALSE;
    SP->orig_attr = TRUE;
    SP->orig_fore = SP->orig_back = -1;
    SP->termattrs = A_COLOR | WA_ITALIC | WA_UNDERLINE | WA_LEFT | WA_RIGHT |
                    WA_REVERSE | WA_STRIKEOUT | WA_TOP | WA_BLINK | WA_DIM | WA_BOLD;
    SP->lines = PDC_get_rows();
    SP->cols = PDC_get_columns();
    SP->_preserve = FALSE;
    PDC_reset_prog_mode();
    PDC_LOG(("PDC_scr_open exit\n"));
    return 0;
}

void PDC_set_resize_limits(const int new_min_lines, const int new_max_lines, const int new_min_cols,
                           const int new_max_cols) {
    INTENTIONALLY_UNUSED_PARAMETER(new_min_lines);
    INTENTIONALLY_UNUSED_PARAMETER(new_max_lines);
    INTENTIONALLY_UNUSED_PARAMETER(new_min_cols);
    INTENTIONALLY_UNUSED_PARAMETER(new_max_cols);
}

bool PDC_can_change_color(void) { return TRUE; }

int PDC_color_content(int color, int *red, int *green, int *blue) {
    const PACKED_RGB col = PDC_get_palette_entry(color);
    *red = DIVROUND(Get_RValue(col) * 1000, 255);
    *green = DIVROUND(Get_GValue(col) * 1000, 255);
    *blue = DIVROUND(Get_BValue(col) * 1000, 255);
    return OK;
}

int PDC_init_color(int color, int red, int green, int blue) {
    const PACKED_RGB new_rgb = PACK_RGB(DIVROUND(red * 255, 1000), DIVROUND(green * 255, 1000),
                                        DIVROUND(blue * 255, 1000));
    if(!PDC_set_palette_entry(color, new_rgb)) curscr->_clear = TRUE;
    if(color >= 0 && color < 256) {             /* (the display's from its next frame) */
        uint8_t rgb[3] = {(uint8_t)Get_RValue(new_rgb), (uint8_t)Get_GValue(new_rgb), (uint8_t)Get_BValue(new_rgb)};
        cmm2->video_palette(color, 1, rgb);
    }
    return OK;
}
