/* curses.h for sc-im (apps/scim/compat, before PDCursesMod on the include path): sc-im uses ncurses's mouse calls,
   getmouse(&event) and MEVENT, which PDCursesMod gives under PDC_NCMOUSE. Only sc-im's own files may have it: the
   library's mouse.c defines both kinds, and does not build with it. Each library file includes curspriv.h, which
   defines CURSES_LIBRARY before it includes curses.h. Public domain, as PDCursesMod. */
#if !defined(CURSES_LIBRARY) && !defined(PDC_NCMOUSE)
#define PDC_NCMOUSE
#endif
#include_next <curses.h>
#ifndef CURSES_LIBRARY
int set_escdelay(int ms);           /* ncurses's, which PDCurses has not: compat.c */
#endif
