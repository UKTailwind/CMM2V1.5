/***********************************************************************************************************************
Console.c - what the editor (the PicoMite's, Src/Editor.c; kilo.c until 19 Oct 2026) shares with the rest of MMBasic:
the display console's helpers (MX470PutS, MX470Cursor, MX470Display), the serial terminal's size (setterminal), the
case-blind search (fstrstr), where EDIT opens a program (StartEditLine, StartEditCharacter, which error() sets) and
the break key the transfers restore (BreakKeySave). Moved here from kilo.c unchanged (Editor_Port_Plan.md, E1), so
that either editor could be the one in the build; kilo's backward search (bstrstr) went with it (S, 19 Oct 2026).
***********************************************************************************************************************/
#include <ctype.h>
#include <string.h>
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"

#define GUI_C_LINE              GRAY
extern BYTE MDD_SDSPI_CardDetectState(void);

int StartEditLine = 0, StartEditCharacter = 0;                     // EDIT with no file opens the program here
int BreakKeySave;

void MX470PutS(char *s, int fc, int bc) {
    int tfc, tbc;
    tfc = gui_fcolour; tbc = gui_bcolour;
    gui_fcolour = fc; gui_bcolour = bc;
    DisplayPutS(s);
    gui_fcolour = tfc; gui_bcolour = tbc;
}

void MX470Cursor(int x, int y) {
    if((OptionConsole & 2)) {
        CurrentX = x;
        CurrentY = y;
    }
}

void MX470Display(int fn) {
    int t;
    int maxW=PageTable[WritePage].xmax;
    int maxH=PageTable[WritePage].ymax;
    if(!(OptionConsole & 2)) return;
    switch(fn) {
        case DISPLAY_CLS:       ClearScreen(gui_bcolour);
                                break;
        case REVERSE_VIDEO:     t = gui_fcolour;
                                gui_fcolour = gui_bcolour;
                                gui_bcolour = t;
                                break;
        case CLEAR_TO_EOL:      DrawBox(CurrentX, CurrentY, maxW, CurrentY + gui_font_height, 0, 0, gui_bcolour);
                                break;
        case CLEAR_TO_EOS:      DrawBox(CurrentX, CurrentY, maxW, CurrentY + gui_font_height, 0, 0, gui_bcolour);
                                DrawRectangle(0, CurrentY + gui_font_height, maxW-1, maxH-1, gui_bcolour);
                                break;
        case SCROLL_DOWN:
                                break;
        case DRAW_LINE:         DrawBox(0, gui_font_height * (Option.Height - 2), maxW - 1, maxH - 1, 0, 0, gui_bcolour);
                                DrawLine(0, maxH - gui_font_height - 6, maxW - 1, maxH - gui_font_height - 6, 1, GUI_C_LINE);
                                CurrentX = 0; CurrentY = maxH - gui_font_height;
                                break;
    }
}

char *strcasechr(const char *p, int ch)
{
	char c;

	c = toupper(ch);
	for (;; ++p) {
		if (toupper(*p) == c)
			return ((char *)p);
		if (*p == '\0')
			return (NULL);
	}
	/* NOTREACHED */
}

char *fstrstr (const char *s1, const char *s2)
{
  const char *p = s1;
  const size_t len = strlen (s2);

  for (; (p = strcasechr (p, *s2)) != 0; p++)
    {
      if (strncasecmp (p, s2, len) == 0)
	return (char *)p;
    }
  return (0);
}

/* Print Screen in the editor and the file manager (the key gives 0x9D: KeyboardMap.c; Peter, 19 Oct 2026): the
   display saved as A:/SCREENnnn.BMP, with the first number not used yet. The editor's tests send the same code over
   the serial console to photograph the display (tools/edit_test.py). */
void ScreenShot(void) {
    char name[24], cmd[40], *saved_cmdline = cmdline;
    FILINFO fno;
    int n;
    if(!MDD_SDSPI_CardDetectState()) return;
    for(n = 0; n < 1000; n++) {
        sprintf(name, "A:/SCREEN%03d.BMP", n);
        if(f_stat(name, &fno) != FR_OK) break;
    }
    if(n == 1000) return;
    sprintf(cmd, "IMAGE \"%s\"", name);
    int oldabort = OptionFileErrorAbort;                // a file error not thrown (FM's way): from the editor it
    OptionFileErrorAbort = 0;                           // would leave it and lose the edits (E3)
    cmdline = cmd;
    cmd_save();
    cmdline = saved_cmdline;
    OptionFileErrorAbort = oldabort;
}

// the serial terminal asked to be the console's size (xterm's resize: ESC [ 8 ; rows ; cols t)
void setterminal(void){
	  char sp[50]={0};
	  strcpy(sp,"\033[8;");
	  IntToStr(&sp[strlen(sp)],Option.Height,10);
	  strcat(sp,";");
	  IntToStr(&sp[strlen(sp)],Option.Width+1,10);
	  strcat(sp,"t");
	  SerUSBPutS(sp);						//
}
