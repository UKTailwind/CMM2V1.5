"""port_osk.py PICOMITE_DIR CMM2_DIR - the PicoMite's on-screen keyboard (OPTION SCREEN KEYBOARD, KEYBOARD ON/OFF)
copied intact for the CMM2, as Src/OSK.c and Inc/OSK.h (Peter, 27 Oct 2026: "Include it in bank 2"; the rule as for
tools/port_usbtouch.py: copy the code across intact and change only what the CMM2 environment mandates).

Copied verbatim, by their text: from graphics/GUI.c the on-screen keyboard (its #if USBKEYBOARD && GUICONTROLS &&
PICOMITEVGA section, before ProcessTouch); from graphics/GUI.h its declarations.

The CMM2 environment's changes, and only these:
  - the section's build guard goes: here it is always built (a USB host, GUI controls, a framebuffer display)
  - osk_state is not static: the 1 mS tick (GuiPressTick, GUI.c) reads it, as the PicoMite's tick calls OSK_IsActive(),
    which is in bank 2 here, where nothing an interrupt runs may be (STM32H743II_FLASH.ld)
  - the PicoMite's cursor (CursorSuspend, CursorHide, CursorRefresh) is the CMM2's (hidecursor, showcursor): hidden
    while the keyboard draws and shown again after, nested as the keyboard's drawing nests
  - no SPI panel refresh (Option.Refresh, Display_Refresh); ScrollLCD takes the CMM2's blank argument; the PicoMite's
    MODE 1 (DISPLAY_TYPE == SCREENMODE1) is the CMM2's MODE n,1 (VideoColour 1)
  - GUI.c's SpecialDrawRBox, SpecialPrintString and CLICK_DURATION are declared here
  - Peter, 27 Oct 2026: no keyboard in MODE n,1 (GUI controls don't really work there): its strip is restored as 0
  - until the PicoMite fixes its issue #143: KEYBOARD ON's error names OPTION SCREEN KEYBOARD, not the OPTION SCREEN
    OFFSET that does not exist
OptionVResreserved, the strip's runtime height, is the CMM2's Draw.c's, as the PicoMite's is its Draw.c's.
"""
import os
import sys

pico, cmm2 = sys.argv[1], sys.argv[2]


def section(text, start, end, what, include_end=False):
    a = text.find(start)
    if a < 0:
        sys.exit('%s: start not found' % what)
    b = text.find(end, a + len(start))
    if b < 0:
        sys.exit('%s: end not found' % what)
    return text[a:b + (len(end) if include_end else 0)]


def once(d, old, new, what):
    n = d.count(old)
    if n != 1:
        sys.exit('%s: %d matches' % (what, n))
    return d.replace(old, new)


def every(d, old, new, what, count):
    n = d.count(old)
    if n != count:
        sys.exit('%s: %d matches, not %d' % (what, n, count))
    return d.replace(old, new)


gui = open(os.path.join(pico, 'graphics', 'GUI.c'), encoding='utf-8').read()
guih = open(os.path.join(pico, 'graphics', 'GUI.h'), encoding='utf-8').read()

guard = '#if defined(USBKEYBOARD) && defined(GUICONTROLS) && defined(PICOMITEVGA)\n'
osk = section(gui, guard + '/* ============================================================================\n * On-Screen Keyboard (OSK)',
              '#endif /* USBKEYBOARD && GUICONTROLS && PICOMITEVGA */\n', 'keyboard', True)
osk = osk[len(guard):]
osk = osk[:osk.rindex('#endif /* USBKEYBOARD && GUICONTROLS && PICOMITEVGA */\n')]
decl = section(guih, guard + '/* ============================================================================\n * On-Screen Keyboard (OSK)',
               '#endif\n', 'declarations')
decl = decl[len(guard):]

osk = once(osk, 'static int osk_state = OSK_STATE_OFF;',
           'int osk_state = OSK_STATE_OFF; /* (the CMM2: the 1 mS tick reads it, GUI.c) */', 'osk_state')
osk = every(osk, '    CursorSuspend = true;\n    CursorHide();\n', '    OSK_CursorOff();\n', 'cursor off', 3)
osk = every(osk, '    CursorSuspend = false;\n', '    OSK_CursorBack();\n', 'cursor back', 3)
osk = every(osk, '    if (Option.Refresh)\n        Display_Refresh();\n', '', 'refresh', 2)
osk = once(osk, '        ScrollLCD(scroll_amount);', '        ScrollLCD(scroll_amount, 1);', 'scroll')
osk = once(osk, 'DISPLAY_TYPE == SCREENMODE1 ? 0 : gui_bcolour', 'VideoColour == 1 ? 0 : gui_bcolour', 'mode 1')
if 'Set OPTION SCREEN OFFSET first' in osk:
    osk = once(osk, 'Set OPTION SCREEN OFFSET first', 'Set OPTION SCREEN KEYBOARD first', 'PicoMite #143')
osk = once(osk, '    OptionVResreserved = Option.VRes_reserved;\n    OSK_RefreshOptionHeight();',
           '    OptionVResreserved = VideoColour == 1 ? 0 : Option.VRes_reserved; /* (none in MODE n,1: Peter) */\n'
           '    OSK_RefreshOptionHeight();', 'no keyboard in MODE n,1')
if 'CursorSuspend' in osk or 'CursorHide(' in osk:
    sys.exit('keyboard: a PicoMite cursor call left')

h = '''/* OSK.h - the PicoMite's on-screen keyboard (its graphics/GUI.h), copied by tools/port_osk.py: do not edit, run it
   again. Included by GUI.h. */
#ifndef OSK_H_
#define OSK_H_

#include <stdbool.h>

''' + decl + '''
/* the CMM2's: GUI.c's ProcessTouch and the 1 mS tick call these */
void OSK_HandleTouchDown(int x, int y);
bool OSK_HandleTouchUp(void);
void OSK_PollFinger2(void);
extern int osk_state;                        /* not OSK_STATE_OFF (0): the keyboard is up (the tick reads it) */
#endif
'''

c = '''/* OSK.c - the PicoMite's on-screen keyboard (its graphics/GUI.c), copied by tools/port_osk.py: do not edit, run it
   again. What the CMM2 changes is listed there. In bank 2 (STM32H743II_FLASH.ld): nothing an interrupt runs is here. */
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "keyboard.h"

#define CLICK_DURATION 3 /* (GUI.c's: a click's mS) */
extern void SpecialDrawRBox(int x1, int y1, int x2, int y2, int radius, int c, int fill, int status);
extern void SpecialPrintString(int x, int y, int jh, int jv, int jo, int fc, int bc, char *str, int status);

/* the PicoMite's CursorSuspend and CursorHide, the CMM2's way: the cursor off the display while the keyboard draws
   (its saved pixels would be the half-drawn keys'), and back after; nested, as OSK_DrawAll's OSK_DrawKeyAt are */
static int osk_cursor_depth = 0, osk_cursor_was = 0;
static void OSK_CursorOff(void)
{
    if (osk_cursor_depth++ == 0)
    {
        osk_cursor_was = cursoron;
        if (cursoron)
            hidecursor(1);
    }
}
static void OSK_CursorBack(void)
{
    if (--osk_cursor_depth == 0 && osk_cursor_was && cursorenable && !cursoron)
        showcursor(1, xcursor, ycursor);
}

''' + osk

open(os.path.join(cmm2, 'Inc', 'OSK.h'), 'w', encoding='utf-8', newline='\n').write(h)
open(os.path.join(cmm2, 'Src', 'OSK.c'), 'w', encoding='utf-8', newline='\n').write(c)
print('Src/OSK.c %d lines, Inc/OSK.h %d lines' % (c.count('\n'), h.count('\n')))
