/* PicoMode.h - OPTION PICOMITE: the PicoMite's MODE and RESOLUTION on the CMM2 (PicoMode.c) */
#ifndef PICOMODE_H
#define PICOMODE_H

#define PICO_NONE 0
#define PICO_VGA  1
#define PICO_HDMI 2

extern int PicoActive;                          // a PicoMite mode is on the display (MODE under OPTION PICOMITE, or RUN)
extern int PicoModeNow;                         // its number, 1 to 5
extern int PicoResNow;                          // the PicoMite resolution the modes map from (RESOLUTION changes it)

extern int ModeReqW, ModeReqH, ModeReqFont;     // a display size (and font) for the next setmode (ltdc.c)
extern int ModeReqHs, ModeReqVs;                // and the pixel clock's divider and the rows' repeat for it
extern int ModeHs, ModeVs, DisplayMerged;       // the current ones; the LTDC shows the merge's display
int ModeRowScale(int mode);                     // how many times the display shows each row
int ModeXres(int mode);                         // the display's size: the mode's, or OPTION PICOMITE's (ltdc.c)
int ModeYres(int mode);
int PicoFlavour(void);                          // OPTION PICOMITE: PICO_NONE, PICO_VGA or PICO_HDMI
const char *PicoDeviceName(void);               // and the PicoMite's MM.DEVICE$ it names (NULL: NONE)
int PicoMaxGP(void);                            // and its MM.INFO(MAX GP)
void PicoModeSet(int mode);
void PicoModeEnd(void);
void PicoOption(char *tp);
void PicoOptionList(void);
#endif
