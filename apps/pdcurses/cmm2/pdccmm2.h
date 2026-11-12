/* pdccmm2.h: what the CMM2 port's files share (apps/pdcurses/cmm2). Public domain, as PDCursesMod. */
#ifndef PDCCMM2_H
#define PDCCMM2_H

/* pdcscrn.c: Alt-= and Alt-Minus while a program runs. Each sets the next mode or font, saves the choice in the
   settings file and has getch() give KEY_RESIZE (resize_term then makes the screen the new size) */
void PDC_cmm2_cycle_mode(void);
void PDC_cmm2_cycle_font(void);

/* pdcmouse.c: the pointer shown only while the program asks for mouse events (mousemask), and its position */
void PDC_cmm2_mouse_show(int on);

/* pdcargs.c: words after RUN's comma that are the port's, not the program's: mode=N, font=WxH. Each is taken out of
   argv (argc returned less them) and set as PDC_MODE or PDC_FONT; PDC_CONFIG is set to the settings file beside the
   program (argv[0]'s folder, else the current directory), before the program can change directory. A program's
   main calls it first. */
int PDC_cmm2_args(int argc, char **argv);

#endif
