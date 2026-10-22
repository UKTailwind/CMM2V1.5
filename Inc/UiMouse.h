/* UiMouse.h - the mouse of the full-screen programs, EDIT and FM (UiMouse.c) */
#ifndef UIMOUSE_H
#define UIMOUSE_H

extern int UiMouseChan;                         // the mouse's channel while EDIT or FM has one open, else -1
void UiMouseOpen(void);                         // OPTION MOUSE's mouse, or a USB mouse: opened, the pointer on
void UiMouseRecheck(void);                      // none open: a USB mouse plugged in since, opened (FM, while it waits)
void UiMouseClose(void);                        // closed, the pointer off
void UiPointer(int show);                       // the pointer where the mouse is (show), or hidden while drawing
int UiMouseReport(void);                        // after an Esc: 1 if a terminal's mouse report follows (b x y next)
void UiTermMouse(int on);                       // a serial terminal's mouse reports on or off
void UiMouseEnd(void);                          // at the prompt: what EDIT or FM left on, off (an error ended it)

#endif
