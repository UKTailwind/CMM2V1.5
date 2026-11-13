/* pdckbd.c: the CMM2's keys and mouse for PDCursesMod (apps/pdcurses/cmm2), in place of the VT and Linux framebuffer
   ports' (vt/pdckbd.c, fb/pdckbd.c). Public domain, as PDCursesMod.

   A key is MMBasic's code (con_getc: ASCII, &H80-&H89 the arrows and editing keys, &H91-&H9C F1-F12, Shift's forms
   &HA0-&HA4 and &HB1-&HBC, a layout's letters as Latin-1), and Ctrl-C comes by break_take, as the firmware does not
   queue it. Shift, Ctrl and Alt are keys_held's, read as the key is taken: an arrow, Home, End, Page Up, Page Down,
   Insert, Delete, Tab, Enter, Backspace or an F key with them becomes PDCurses's key for it (KEY_SLEFT, CTL_LEFT,
   ALT_LEFT, KEY_F(13)...: the codes an ncurses program knows, Shift with Up and Down being KEY_SR and KEY_SF), and
   getch()'s PDC_get_key_modifiers() gives them for any key. Left Alt is Alt; right Alt is a layout's AltGr. Alt-= and
   Alt-Minus are the port's own (pdcscrn.c: the next mode, the next font).

   &HA0-&HA4 and &HB1-&HBC are also a layout's Latin-1 characters (the French layout's pound sign, 163, is Shift with
   Right, &HA3): the special key if it, or its unshifted form, is held as it is taken; otherwise the character
   (docs/Office_Apps_Plan.md, "System services"). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <assert.h>                             /* (before common/mouse.c, which asserts: built without NDEBUG, as
                                                    sc-im is, it was an undeclared function) */
#include "curspriv.h"
#include "common/mouse.c"
#include "cmm2.h"
#include "pdccmm2.h"

void PDC_check_for_blinking(void);              /* common/blink.c, in fb/pdcdisp.c */
int PDC_update_mouse(int *button);              /* pdcmouse.c */
int PDC_get_modifiers(void);
bool PDC_update_mouse_cursor(int left, int right, int top, int bottom, const bool draw_it);   /* fb/pdcdisp.c */
bool PDC_remove_mouse_cursor(void);
extern bool PDC_resize_occurred;

static int _key_modifiers;

/* fb/pdckbd.c's, as it is: the mouse's events from PDC_update_mouse, and the pointer moved */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "common/psf.h"
#include "fb/pdcfb.h"

static int _key_to_mouse_modifiers( const int key_modifs)
{
   return( (key_modifs & 7) << 3);
}

#define PDC_MOUSE_WHEEL_EVENTS (PDC_MOUSE_WHEEL_UP | PDC_MOUSE_WHEEL_DOWN \
                        | PDC_MOUSE_WHEEL_RIGHT | PDC_MOUSE_WHEEL_LEFT)

static void _check_mouse( void)
{
   const int mx = PDC_mouse_x, my = PDC_mouse_y;
   extern struct font_info PDC_font_info;
   extern int PDC_orientation;
   int button, event;
   const int xper = ((PDC_orientation & 1) ? PDC_font_info.height : PDC_font_info.width);
   const int yper = ((PDC_orientation & 1) ? PDC_font_info.width : PDC_font_info.height);
   const int xmax = SP->cols * xper;
   const int ymax = SP->lines * yper;
   long prev_t = 0;

   _key_modifiers = PDC_get_modifiers( );
   if( _get_mouse_event( NULL))      /* already got events queued up */
      return;
   while( (event = PDC_update_mouse( &button)) >= 0 ||
                        (prev_t && PDC_millisecs() - prev_t < SP->mouse_wait))
      {
      int x, y;

      if( PDC_mouse_x < 0)
         PDC_mouse_x = 0;
      if( PDC_mouse_x > xmax - 1)
         PDC_mouse_x = xmax - 1;
      if( PDC_mouse_y < 0)
         PDC_mouse_y = 0;
      if( PDC_mouse_y > ymax - 1)
         PDC_mouse_y = ymax - 1;
      x = PDC_mouse_x / xper;
      y = PDC_mouse_y / yper;
      if( event >= 0)
         {
         const int modifs = _key_to_mouse_modifiers( _key_modifiers);

         if( x != mx / xper || y != my / yper)
            _add_raw_mouse_event( 0, event, modifs, x, y);
         else if( event & PDC_MOUSE_WHEEL_EVENTS)
            {
            _add_raw_mouse_event( 0, event, modifs, x, y);
            break;
            }
         else if( event == BUTTON_PRESSED || event == BUTTON_RELEASED)
            {
            if( _add_raw_mouse_event( button - 1, event, modifs, x, y)
                        && SP->mouse_wait)
               prev_t = PDC_millisecs( );
            else
               break;
            }
         }
      else if( prev_t)
         napms( 10);
      }
   if( mx != PDC_mouse_x || my != PDC_mouse_y)
      {
      const int tx = PDC_mouse_x, ty = PDC_mouse_y;

      PDC_mouse_x = mx;
      PDC_mouse_y = my;
      PDC_remove_mouse_cursor( );
      PDC_mouse_x = tx;
      PDC_mouse_y = ty;
      PDC_update_mouse_cursor( 0, xmax, 0, ymax, 1);
      }
}
/* (fb/pdckbd.c's ends) */

/* the keys taken from the firmware and not yet given: an Enter's CR is followed by its LF, which is dropped, and the
   key read to see so is kept */
#define QSIZE 8
static int queue[QSIZE], qhead, qtail;

static void fill(void) {
    int k, n;
    cmm2->poll();
    if(cmm2->break_take()) {
        queue[qhead] = 3;
        qhead = (qhead + 1) % QSIZE;
    }
    while((qhead + 2) % QSIZE != qtail && (k = cmm2->con_getc()) >= 0) {
        queue[qhead] = k;
        qhead = (qhead + 1) % QSIZE;
        if(k == 13 && (n = cmm2->con_getc()) >= 0 && n != 10) {
            queue[qhead] = n;
            qhead = (qhead + 1) % QSIZE;
        }
    }
}

static bool check_key(int *c) {
    _check_mouse();
    PDC_check_for_blinking();
    if(qhead == qtail) fill();
    if(qhead == qtail) return FALSE;
    if(c) {
        *c = queue[qtail];
        qtail = (qtail + 1) % QSIZE;
    }
    return TRUE;
}

bool PDC_check_key(void) {
    if(_mlist_count || PDC_resize_occurred) return TRUE;
    return check_key(NULL);
}

void PDC_flushinp(void) {
    int thrown_away_char;
    _mlist_count = 0;
    while(check_key(&thrown_away_char)) {}
}

static bool held(const int *keys, int n, int a, int b) {        /* a or b among the keys held (any of the six:
                                                                   a key need not be in the first n) */
    int i;
    (void)n;
    for(i = 0; i < 6; i++)
        if(keys[i] == a || keys[i] == b) return TRUE;
    return FALSE;
}

/* 1 if the key with that HID usage is held (keys_raw, version 9), 0 if not, -1 if it cannot be known */
static int usage_held(int usage) {
    uint8_t r[8];
    int i;
    if(cmm2->version < 9 || cmm2->keys_raw(r) < 0) return -1;
    for(i = 2; i < 8; i++)
        if(r[i] == usage) return 1;
    return 0;
}

/* MMBasic's key code, with the keys held, as PDCurses's key; *m its modifiers (PDC_KEY_MODIFIER_*); -1 for a key
   with no meaning here */
static int translate(int k, const int *keys, int n, int *m) {
    static const int arrow[4] = {0x81, 0x82, 0x83, 0x80};       /* (&HA1 Down, &HA2 Left, &HA3 Right, &HA4 Up) */
    const int bits = keys[6];
    int shift = (bits & (8 | 128)) != 0;
    const int ctrl = (bits & (2 | 32)) != 0, alt = (bits & 1) != 0;
    int f;

    *m = (shift ? PDC_KEY_MODIFIER_SHIFT : 0) | (ctrl ? PDC_KEY_MODIFIER_CONTROL : 0) | (alt ? PDC_KEY_MODIFIER_ALT : 0);
    /* Shift's codes: the key itself, with Shift (Ctrl with Shift and Left is CTL_LEFT, Shift in its modifiers), unless
       the key is not held: then a layout's Latin-1 character */
    if(k >= 0xA1 && k <= 0xA4) {
        if(!held(keys, n, arrow[k - 0xA1], k)) return k;
        k = arrow[k - 0xA1];
        shift = 1;
    } else if(k == 0xA0) {                      /* Shift with Delete, or a no-break space */
        if(!held(keys, n, 0x7F, 0xA0)) return k;
        k = 0x7F;
        shift = 1;
    } else if(k >= 0xB1 && k <= 0xBC) {         /* Shift with an F key */
        if(!held(keys, n, k - 0x20, k)) return k;
        k -= 0x20;
        shift = 1;
    }
    *m = (shift ? PDC_KEY_MODIFIER_SHIFT : 0) | (ctrl ? PDC_KEY_MODIFIER_CONTROL : 0) | (alt ? PDC_KEY_MODIFIER_ALT : 0);
    switch(k) {
    case 0x80: return ctrl ? CTL_UP : alt ? ALT_UP : shift ? KEY_SR : KEY_UP;
    case 0x81: return ctrl ? CTL_DOWN : alt ? ALT_DOWN : shift ? KEY_SF : KEY_DOWN;
    case 0x82: return ctrl ? CTL_LEFT : alt ? ALT_LEFT : shift ? KEY_SLEFT : KEY_LEFT;
    case 0x83: return ctrl ? CTL_RIGHT : alt ? ALT_RIGHT : shift ? KEY_SRIGHT : KEY_RIGHT;
    case 0x84: return ctrl ? CTL_INS : alt ? ALT_INS : shift ? KEY_SIC : KEY_IC;
    case 0x7F: return ctrl ? CTL_DEL : alt ? ALT_DEL : shift ? KEY_SDC : KEY_DC;
    case 0x86: return ctrl ? CTL_HOME : alt ? ALT_HOME : shift ? KEY_SHOME : KEY_HOME;
    case 0x87: return ctrl ? CTL_END : alt ? ALT_END : shift ? KEY_SEND : KEY_END;
    case 0x88: return ctrl ? CTL_PGUP : alt ? ALT_PGUP : shift ? KEY_SPREVIOUS : KEY_PPAGE;
    case 0x89: return ctrl ? CTL_PGDN : alt ? ALT_PGDN : shift ? KEY_SNEXT : KEY_NPAGE;
    case 0x9F: *m |= PDC_KEY_MODIFIER_SHIFT; return KEY_BTAB;
    /* Tab, Enter, Backspace and Esc give the codes Ctrl gives with I, M, H and [ (9, 13, 8, 27), as on a terminal:
       with Ctrl held, the key itself (keys_raw's HID usage: Tab 0x2B, Enter 0x28 or the keypad's 0x58, Backspace
       0x2A) is CTL_TAB, CTL_ENTER or CTL_BKSP, and Ctrl with the letter is the control character (WordGrinder's
       Ctrl+I, italic, 12 Nov 2026) */
    case 9: return ctrl ? (usage_held(0x2B) == 1 ? CTL_TAB : 9) : alt ? ALT_TAB : 9;
    case 13: return ctrl ? (usage_held(0x28) == 1 || usage_held(0x58) == 1 ? CTL_ENTER : 13) : alt ? ALT_ENTER : 13;
    case 27: return alt ? ALT_ESC : 27;
    case ' ': return ctrl && !alt ? 0 : ' ';   /* Ctrl with Space: NUL, as a terminal sends it (WordGrinder's mark) */
    case 8:                                     /* Backspace, or Ctrl with H */
        if(ctrl) return usage_held(0x2A) == 1 ? CTL_BKSP : 8;
        return alt ? ALT_BKSP : KEY_BACKSPACE;
    }
    if(k >= 0x91 && k <= 0x9C) {                /* F1-F12, and with modifiers as ncurses numbers them */
        f = k - 0x90;
        if(alt) return KEY_F(f + 48);
        if(ctrl) return KEY_F(f + (shift ? 36 : 24));
        return KEY_F(f + (shift ? 12 : 0));
    }
    if(alt && k > ' ' && k < 0x7F) {            /* Alt with a letter, a digit or a punctuation key */
        static const char punct[] = "-='\",./\\[];`";
        static const int alts[] = {ALT_MINUS, ALT_EQUAL, ALT_FQUOTE, ALT_FQUOTE, ALT_COMMA, ALT_STOP, ALT_FSLASH,
                                   ALT_BSLASH, ALT_LBRACKET, ALT_RBRACKET, ALT_SEMICOLON, ALT_BQUOTE};
        const char *p;
        if(isalpha(k)) return ALT_A + (tolower(k) - 'a');
        if(isdigit(k)) return ALT_0 + (k - '0');
        if((p = strchr(punct, k)) != NULL) return alts[p - punct];
        return k;
    }
    if((k >= 0x85 && k <= 0x90) || k == 0x9D || k == 0x9E) return -1;   /* (no PDCurses key: Pause and the like) */
    return k;                                   /* ASCII, Ctrl's letters, a layout's Latin-1 (Unicode's first 256) */
}

int PDC_get_key(void) {
    int k, rval, mods, keys[7], n;

    if(PDC_resize_occurred) {
        PDC_resize_occurred = FALSE;
        return KEY_RESIZE;
    }
    if(_mlist_count) {
        _get_mouse_event(&SP->mouse_status);
        return KEY_MOUSE;
    }
    if(!check_key(&k)) return -1;
    memset(keys, 0, sizeof(keys));
    n = cmm2->keys_held(keys);
    rval = translate(k, keys, n, &mods);
    {
        static int debug = -1;                  /* debug=1 after RUN's comma: each key, and the heap, on the serial
                                                   console (stderr is there while curses has the screen) */
        if(debug < 0) debug = getenv("PDC_DEBUG") && atoi(getenv("PDC_DEBUG")) > 0;
        if(debug) {
            struct mallinfo mi = mallinfo();
            fprintf(stderr, "key %02X held %02X %02X %02X mods %02X -> %d (%lX) heap in use %u of %u\n", k, keys[0],
                    keys[1], keys[2], keys[6], rval, (unsigned long)mods, (unsigned)mi.uordblks, (unsigned)mi.arena);
        }
    }
    if(rval == ALT_EQUAL) {
        PDC_cmm2_cycle_mode();
        return -1;
    }
    if(rval == ALT_MINUS) {
        PDC_cmm2_cycle_font();
        return -1;
    }
    SP->key_modifiers = mods;
    return rval;
}

int PDC_modifiers_set(void) { return OK; }

bool PDC_has_mouse(void) {
    int m[4];
    return cmm2->mouse(m) == 1;
}

int PDC_mouse_set(void) {                       /* the pointer shown while the program asks for the mouse's events */
    PDC_cmm2_mouse_show(SP->_trap_mbe != 0);
    return OK;
}

void PDC_set_keyboard_binary(bool on) { INTENTIONALLY_UNUSED_PARAMETER(on); }
