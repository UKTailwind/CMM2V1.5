/* pdcmouse.c: the CMM2's mouse and modifier keys for PDCursesMod (apps/pdcurses/cmm2), in place of the Linux
   framebuffer port's (fb/pdcmouse.c, which reads Linux's input devices). Public domain, as PDCursesMod.

   The mouse is the firmware's (the mouse service: USB, PS/2 or I2C, MM.MOUSE's), its position kept in the screen's
   pixels. Its pointer is fb/pdcdisp.c's, drawn at PDC_mouse_x, PDC_mouse_y, and is shown only while the program asks
   for mouse events (mousemask): otherwise it is put far off the screen, where fb/pdcdisp.c draws none of it. */
#include <stdint.h>
#include "curspriv.h"
#include "common/psf.h"
#include "cmm2.h"
#include "pdccmm2.h"

extern int PDC_mouse_x, PDC_mouse_y;            /* fb/pdcdisp.c */
extern struct font_info PDC_font_info;
bool PDC_update_mouse_cursor(int left, int right, int top, int bottom, const bool draw_it);
bool PDC_remove_mouse_cursor(void);

#define OFF_SCREEN (-10000)

static int shown;                               /* the pointer on the screen */
static int last_x, last_y, last_b, last_w;      /* the mouse as last seen: position, buttons, wheel count */

void PDC_cmm2_mouse_show(int on) {
    int m[4];
    if(!on || cmm2->version < 4 || cmm2->mouse(m) != 1) {
        if(shown) {
            PDC_remove_mouse_cursor();
            shown = 0;
        }
        PDC_mouse_x = PDC_mouse_y = OFF_SCREEN;
        return;
    }
    if(shown) return;
    shown = 1;
    last_x = PDC_mouse_x = m[0];
    last_y = PDC_mouse_y = m[1];
    last_b = m[2];
    last_w = m[3];
    if(SP && curscr && PDC_font_info.width)
        PDC_update_mouse_cursor(0, SP->cols * PDC_font_info.width, 0, SP->lines * PDC_font_info.height, TRUE);
}

/* the next change: BUTTON_MOVED (PDC_mouse_x and _y moved), BUTTON_PRESSED or BUTTON_RELEASED (*button curses's:
   1 left, 2 middle, 3 right), a wheel's step; or -1 */
int PDC_update_mouse(int *button) {
    static const int bit[3] = {1, 4, 2};        /* the service's buttons: 1 left, 2 right, 4 middle */
    int m[4], i;
    if(!shown && SP && SP->_trap_mbe) PDC_cmm2_mouse_show(1);  /* (a mouse plugged in since) */
    if(!shown || cmm2->mouse(m) != 1) return -1;
    if(m[0] != last_x || m[1] != last_y) {
        last_x = PDC_mouse_x = m[0];
        last_y = PDC_mouse_y = m[1];
        return BUTTON_MOVED;
    }
    for(i = 0; i < 3; i++)
        if((m[2] ^ last_b) & bit[i]) {
            last_b ^= bit[i];
            *button = i + 1;
            return (m[2] & bit[i]) ? BUTTON_PRESSED : BUTTON_RELEASED;
        }
    if(m[3] != last_w) {                        /* a step at a time */
        const int up = m[3] > last_w;
        last_w += up ? 1 : -1;
        return up ? PDC_MOUSE_WHEEL_UP : PDC_MOUSE_WHEEL_DOWN;
    }
    return -1;
}

int PDC_get_modifiers(void) {                   /* keys_held's: 1 left Alt, 2 left Ctrl, 8 left Shift, 32 right Ctrl, 128 right Shift */
    int keys[7] = {0}, m = 0;
    cmm2->keys_held(keys);
    if(keys[6] & (8 | 128)) m |= PDC_KEY_MODIFIER_SHIFT;
    if(keys[6] & (2 | 32)) m |= PDC_KEY_MODIFIER_CONTROL;
    if(keys[6] & 1) m |= PDC_KEY_MODIFIER_ALT;
    return m;
}
