/* Editor_cmm2.h - the CMM2's platform layer for EDIT (Src/Editor.c, which tools/port_editor.py makes from the
   PicoMite's misc/Editor.c): the PicoMite's names for what the CMM2 has. Only Editor.c includes it (Editor_Port_Plan.md,
   E1; 19 Oct 2026). */
#ifndef EDITOR_CMM2_H
#define EDITOR_CMM2_H
#include "FileManager.h"
// the PicoMite's isdigit(*p) and the like on a char: safe here, where char is unsigned (the ARM's), so the source stays
// as the PicoMite has it
#pragma GCC diagnostic ignored "-Wchar-subscripts"

/* ---- the display ---------------------------------------------------------------------------------------------------
   The editing area is the write page, in the editor's font (EDIT sets the mode and the font as kilo did, so that
   Option.Width and Option.Height are the display's columns and rows when it is a console). The PicoMite keeps a part of
   its screen for an on-screen keyboard (VResEdit); the CMM2 has none. */
#define EdHRes              (PageTable[WritePage].xmax)
#define EdVRes              (PageTable[WritePage].ymax)
#define VResEdit            EdVRes
#define SCROLLCHARS         3
#define MX470Scroll(n)      if((OptionConsole & 2)) ScrollLCD(n, 1)    // (kilo's: the display's own scroll)

// the colours: on the display the CMM2's (kilo's), on a VT100 terminal the PicoMite's, which kilo's are as well
#define GUI_C_NORMAL        WHITE
#define GUI_C_BCOLOUR       BLACK
#define GUI_C_COMMENT       RGB(192, 192, 0, 0)
#define GUI_C_KEYWORD       CYAN
#define GUI_C_QUOTE         RGB(0, 200, 0, 0)
#define GUI_C_NUMBER        RGB(255, 128, 128, 0)
#define GUI_C_LINE          GRAY
#define GUI_C_STATUS        WHITE
#define VT100_C_NORMAL      "\033[37m"
#define VT100_C_COMMENT     "\033[33m"
#define VT100_C_KEYWORD     "\033[36m"
#define VT100_C_QUOTE       "\033[35m"
#define VT100_C_NUMBER      "\033[32m"
#define VT100_C_LINE        "\033[35m"
#define VT100_C_STATUS      "\033[37m"
#define VT100_C_ERROR       "\033[31m"

/* COLOURCODE: the CMM2's OPTION COLOURCODE ON is Option.colourmode 1; REVERSE (-1) is black on white through the CLUT,
   without colours, as kilo has it. The editor turns colour off for a file that is not BASIC (the PicoMite's sets its
   Option.ColourCode and puts it back), so it is a variable of its own. */
static int EdColourCode;

/* ---- the serial console --------------------------------------------------------------------------------------------
   The PicoMite's SSPrintString and SerialConsolePutC: the serial console only, and only when it is a console. */
static inline void EdSerialPutS(char *s) { if(OptionConsole & 1) SerUSBPutS(s); }
static inline char EdSerialPutC(char c, int flush) { if(OptionConsole & 1) SerUSBPutC(c); return c; }

// the shifted function keys: MMInkey gives a terminal's SHIFT-F3 to F10 as the key + 0x20 (main.c), as the PicoMite does
#define SHIFT_F3            (F3 + 0x20)
#define SHIFT_F4            (F4 + 0x20)
#define SHIFT_F5            (F5 + 0x20)

/* ---- the rest of MMBasic --------------------------------------------------------------------------------------- */
#define routinechecks()     routinechecks(1)

/* a key for the editor's prompts, the cursor shown while it waits: MMgetchar's way, but MMgetchar gives Enter as one
   '\r' and swallows the '\n' MMInkey makes after it only when it is called again, so that '\n' would be the editor's
   next key. The editor passes over the '\r' and takes the '\n' as Enter, as kilo does. */
static int EdInkey(void);                               // Editor.c: MMInkey, and the tests' screenshot key
static int EdGetKey(void)
{
    int c;
    do
    {
        ShowCursor(true);
        c = EdInkey();
    } while (c == -1);
    ShowCursor(false);
    return c;
}
extern const char *const FoldList[];                    // MMBasic.c: the functions tokenise() folds into other tokens
extern const int FoldCount;
extern int StartEditLine, StartEditCharacter;           // Console.c: where EDIT with no file opens the program
extern void setterminal(void);
extern void MX470PutS(char *s, int fc, int bc);         // Console.c: the display console's helpers (the CMM2's)
extern void MX470Cursor(int x, int y);
extern void MX470Display(int fn);
extern char *fstrstr(const char *s1, const char *s2);   // Console.c: strstr blind to case
extern void ScreenShot(void);                           // Console.c: Print Screen
extern void cleanend(void);
extern volatile uint8_t pagesetdone;
extern void closeall3d(void);
extern void closeallsprites(void);
extern void SymBindFree(void);
extern void mouse0close(void);
extern void i2c_disable(void), i2c2_disable(void), i2c3_disable(void);
extern void CloseAllFiles(void);

#endif
