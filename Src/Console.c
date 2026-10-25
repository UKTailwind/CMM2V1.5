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

/* Print Screen (the key gives 0x9D: KeyboardMap.c; Peter, 19 Oct 2026) and Ctrl-\ (0x1C, from any keyboard or
   terminal), as the PicoMite's (its FileIO.c; Peter, 25 Oct 2026): at the command prompt, in the editor and in the
   file manager, and in a program under OPTION PRINTSCREEN ON (below), the display saved in the root of the SD card as
   A:/SCREEN_yyyymmdd_hhmmss.PNG by SAVE PNG, or as .BMP by SAVE IMAGE under OPTION PRINTSCREEN FORMAT BMP. The name is
   from the clock, not searched for (A:/SCREENnnn.BMP was up to 1000 lookups first): one already taken, by two saves in
   one second or a clock set back, gets _1, _2 ... _99 rather than being overwritten. Nothing is printed, and an error
   (no card, no room, no memory) is caught (FileTry), so the editor and the file manager keep their screens and their
   text; the save's temporary memory is freed at once (the editor never clears it, and a program's statement may still
   be using its own). The editor's tests send the code over the serial console to photograph the display
   (tools/edit_test.py). */
static int ScreenShotSave(void *arg) {
    char name[48], *cmd = arg;
    FILINFO fno;
    int len, n, png = Option.PrintScreenFormat != 1;                // (1 BMP; erased flash and 0 PNG, the CMM2's default)
    RtcGetTime();
    len = sprintf(name, "A:/SCREEN_%04d%02d%02d_%02d%02d%02d", year, month, day, hour, minute, second);
    for(n = 0; n < 100; n++) {
        if(n == 0) strcpy(name + len, png ? ".PNG" : ".BMP");
        else sprintf(name + len, png ? "_%d.PNG" : "_%d.BMP", n);
        if(f_stat(name, &fno) != FR_OK) break;
    }
    if(n == 100) return 0;
    sprintf(cmd, "%s \"%s\"", png ? "PNG" : "IMAGE", name);
    cmdline = cmd;
    cmd_save();
    return 1;
}

void ScreenShot(void) {
    char cmd[64];
    int mark = TempMemoryMark();
    if((void *)ReadBuffer == (void *)DisplayNotSet || !MDD_SDSPI_CardDetectState()) return;
    FileTry(ScreenShotSave, cmd);
    ClearTempMemoryTo(mark);
}

/* OPTION PRINTSCREEN ON, a temporary option (off at RUN and at the prompt): while a program runs, Print Screen and
   Ctrl-\ save the screen too. Each console producer takes the key where it arrives (PrtScrTake: the USB keyboard,
   KeyboardMap.c, and the serial consoles' interrupts, stm32h7xx_it.c), so the program never reads it, and the screen is
   saved where the program can stop for it: after a statement and in a PAUSE (check_interrupt), and in INPUT's wait
   (MMgetchar). */
volatile int PrintScreenOn, PrintScreenPending;
int PrtScrTakeKey(int c) {
    if(!KEY_IS_PRTSCR(c)) return 0;
    PrintScreenPending = 1;
    IntSignal();                                                    // so that check_interrupt looks after this statement
    return 1;
}

// the save, keeping the program's MM.ERRNO and MM.ERRMSG$ (FileTry clears them)
void PrintScreenNow(void) {
    static char msg[256];
    int e = MMerrno;
    PrintScreenPending = 0;
    if(MMErrMsg != msg) {
        strncpy(msg, MMErrMsg, sizeof(msg) - 1);
        msg[sizeof(msg) - 1] = 0;
    }
    ScreenShot();
    MMerrno = e;
    MMErrMsg = msg;
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
