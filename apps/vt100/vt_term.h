/* vt_term.h: a terminal on the CMM2's screen, libvterm's emulation drawn straight into the frame buffer. Nothing here knows where the
   bytes come from: vt100.c joins it to a serial port with a mode and palette of its own (vtt_open), and zeptoforth's console to Forth on
   the screen as it finds it (vtt_attach). MIT License, as cmm2img.h. */
#ifndef VT_TERM_H
#define VT_TERM_H
#include <stddef.h>
#include <stdint.h>
#include "cmm2.h"

enum { VTT_CP437, VTT_LATIN1, VTT_UTF8 };                   /* how a byte above 127 from the host is read */
enum { VTT_WHITE, VTT_GREEN, VTT_AMBER };                   /* the default text colour */

typedef struct {
    int mode;               /* MODE n,8: not one that shows its lines twice */
    int font;               /* an index into vtt_fonts[] */
    int cols, rows;         /* 0: as many as fit */
    int status;             /* a line kept below the terminal, for vtt_status() */
    int charset;            /* VTT_CP437... */
    int colour;             /* VTT_WHITE... */
    int cursor;             /* 0 a block, 1 an underline, 2 a bar: what a host that has not chosen, or asks for a block, gets */
    int backspace_del;      /* the Backspace key sends 127, not 8 */
    int scrollback;         /* lines kept: fewer if there is not the memory */
    int home_xterm;         /* Home and End as xterm's ESC [ H and ESC [ F, not the ESC [ 1 ~ and ESC [ 4 ~ a PicoMite wants */
} vtt_config;

typedef struct { const char *name; int w, h; const uint8_t *bits; } vtt_font;
extern const vtt_font vtt_fonts[];
extern const int vtt_font_count;

/* out: what the terminal sends to the host, keys and the replies to the host's questions. Returns 0, or negative if the mode can't
   be set or there is no memory. Called again it changes the screen and keeps what is on it, as far as it fits. */
int  vtt_open(const vtt_config *cfg, void (*out)(const char *bytes, size_t n));
void vtt_close(void);

/* As vtt_open, for a program with graphics of its own on the same screen: the screen as it is, 8 or 16 bits, lines shown twice or not, and its
   palette left alone, xterm's colours drawn in the nearest the mode has. cfg's mode is not used. Called again it moves to the new screen */
int  vtt_attach(const vtt_config *cfg, const cmm2_video *video, void (*out)(const char *bytes, size_t n));
void vtt_cursor_show(int on);                               /* 0: the cursor kept off the screen, whatever the host wants */
void vtt_freeze(int on);                                    /* 1: nothing drawn, and what changes meanwhile not drawn after, though the terminal goes on reading and answering */

void vtt_write(const char *bytes, size_t n);                /* bytes from the host */
void vtt_update(void);                                      /* the screen brought up to date: call it at every frame or so */
int  vtt_key(int key, int mods);                            /* a key from con_getc, mods from keys_held [6]: 1 if the terminal used it */
void vtt_mouse(void);                                       /* the mouse's state read, reported if the host asked: call it often */
void vtt_status(const char *text);                          /* the status line */
void vtt_reset(int hard);                                   /* as RIS (hard) or DECSTR, and the screen cleared */
void vtt_redraw(void);                                      /* after something else has drawn on the screen */
void vtt_size(int *cols, int *rows);
void vtt_set(const vtt_config *cfg);                        /* the settings that need no new screen: charset, colour, backspace */

/* something for vt100.c to draw with, over the terminal, until vtt_redraw: in cells of the screen, colours as xterm's 256 */
void vtt_text(int col, int row, const char *s, int fg, int bg);
void vtt_screen_cells(int *cols, int *rows);
void vtt_bell(void);

/* the host's request to change the terminal's size (CSI 8;rows;cols t): the program decides (a new mode, or none) */
extern void (*vtt_resize_request)(int rows, int cols);
extern void (*vtt_bell_hook)(void);                         /* the bell, if vtt_bell's tone by the sound stream won't do */
#endif
