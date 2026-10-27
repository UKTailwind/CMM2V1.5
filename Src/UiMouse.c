/* UiMouse.c - the mouse of the full-screen programs, EDIT and FM (22 Oct 2026: moved out of Editor.c for FM, whose
   mouse the old fm.c had and the PicoMite's FM has not). OPTION MOUSE's mouse or a USB mouse, opened without letting an
   error end the program; the pointer (the GUI cursor) shown while it waits for a key and hidden while it draws; and a
   serial terminal's mouse reports (Tera Term's ESC [ M b x y). What each program does with the mouse is its own. */
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "UiMouse.h"

extern void cmd_mouse(char *cmd);                       // MiscSTM32.c: CONTROLLER MOUSE (OPEN, CLOSE)
extern volatile struct s_nunstruct mousestruct[4];      // the mice's state, in the display's pixels
extern jmp_buf ErrNext;

int UiMouseChan = -1;
static int UiMouseUsb = 0;                              // the mouse open is a USB one (its pointer goes while it is unplugged)
static int UiTermOn = 0;                                // a terminal's reports are on (UiTermMouse)
static int UiFailedSlot = 0;                            // the USB mouse whose open failed: not tried again (UiMouseRecheck)

/* fn(arg) (CONTROLLER MOUSE OPEN, or the PS/2 mouse's start), its errors kept from ending the program: the mouse OPTION
   MOUSE names may not be there (OPTION MOUSE 2 with the USB mouse unplugged tries an I2C mouse: "Mouse not connected"),
   and the program then goes on without one. Under OptionErrorSkip error() prints nothing and longjmps to ErrNext, so
   both are borrowed for the call, and MM.ERRNO and MM.ERRMSG$ put back after it. 1 if it worked. */
static int UiTry(void (*fn)(char *), char *arg)
{
    jmp_buf save;
    int skip = OptionErrorSkip, errno_was = MMerrno, line_was = MMerrline, ok = 1;
    char msg[STRINGSIZE];
    strcpy(msg, MMErrMsg);
    memcpy(save, ErrNext, sizeof(jmp_buf));
    OptionErrorSkip = 1;
    if (setjmp(ErrNext) == 0)
        fn(arg);
    else
        ok = 0;
    memcpy(ErrNext, save, sizeof(jmp_buf));
    OptionErrorSkip = skip;
    if (!ok) {
        mT4IntEnable(1);                                // (some of the open's failures left the controllers' tick off)
        MMerrno = errno_was;
        MMerrline = line_was;
        strcpy(MMErrMsg, msg);
    }
    return ok;
}

/* with OPTION MOUSE set or a USB mouse plugged in, and the display a console: the mouse opened (a USB mouse is the one
   read: only its channel, 2, can be open then) and the pointer on. None: UiMouseChan -1, and the program without one. */
void UiMouseOpen(void)
{
    char open[24];
    UiMouseChan = -1;
    if ((Option.Mouse < 0 && !UsbMouse) || !(OptionConsole & 2) || VideoColour < 8)
        return;                                         // (a USB mouse needs no OPTION MOUSE; EDIT and FM are in 8 bits)
    sprintf(open, "OPEN %d,,,%d", UsbMouse ? USBMOUSE_CHANNEL : Option.Mouse, Option.Sensitivity);
    CursorPlain();                                      // (the white arrow, whatever the program left)
    if (!UiTry(cmd_mouse, open)) {                      // (kilo's GUI CURSOR ON:CONTROLLER MOUSE OPEN n,,,s)
        cmd_cursor("OFF");                              // no mouse: the program without one
        UiFailedSlot = UsbMouse;
        return;
    }
    UiMouseChan = UsbMouse ? USBMOUSE_CHANNEL : Option.Mouse;
    UiMouseUsb = (UsbMouse != 0);
}

// none open: a USB mouse plugged in since the program started is opened (once: not again after a failed open of it)
void UiMouseRecheck(void)
{
    if (UiMouseChan < 0 && UsbMouse && UsbMouse != UiFailedSlot)
        UiMouseOpen();
}

void UiMouseClose(void)
{
    char close[24];
    UiFailedSlot = 0;
    if (UiMouseChan < 0)
        return;
    sprintf(close, "CLOSE %d", UiMouseChan);
    UiMouseChan = -1;
    cmd_mouse(close);
    cmd_cursor("OFF");
}

// the pointer where the mouse is (show), or hidden while the program draws (!show); hidden too while the USB mouse it
// follows is unplugged (it comes back with the mouse: its channel stays open)
void UiPointer(int show)
{
    volatile struct s_nunstruct *m;
    if (UiMouseChan < 0)
        return;
    m = &mousestruct[UiMouseChan];
    if (!show || (UiMouseUsb && !UsbMouse))
        hidecursor(1);
    else if (!cursoron || m->ax != xcursor || m->ay != ycursor)
    {
        hidecursor(1);
        showcursor(1, m->ax, m->ay);
    }
}

/* after Esc, a terminal's mouse report (ESC [ M b x y, Tera Term's) or not: its next two keys are looked at and given
   back unless they are '[' 'M'. The PicoMite's editor took them with MMInkey and so lost a key typed straight after Esc
   (the Y of "Exit and discard"). 1 when they are, and the report's three bytes are next. */
int UiMouseReport(void)
{
    int a = MMInkey(), b;
    if (a != '[')
    {
        MMInkeyUnget(a);
        return 0;
    }
    b = MMInkey();
    if (b != 'M')
    {
        MMInkeyUnget(b);                                // (given back in the order they came: MMInkey returns the
        MMInkeyUnget(a);                                // last given first)
        return 0;
    }
    return 1;
}

// a serial terminal's mouse reports on or off (Tera Term's VT200 mouse: a press and a release each, ESC [ M b x y)
void UiTermMouse(int on)
{
    if (on == UiTermOn)
        return;
    UiTermOn = on;
    if (OptionConsole & 1)
        SerUSBPutS(on ? "\033[?1000h" : "\033[?1000l");
}

void UiMouseEnd(void)
{
    UiMouseClose();
    UiTermMouse(0);
}

static void UiStartPS2(char *unused)
{
    (void)unused;
    mouse0start(Option.Sensitivity);
}

/* OPTION MOUSE 0: the PS/2 mouse is the system's, as a USB mouse is (Peter, 22 Oct 2026): started at boot, and when
   something wants the mouse (FM, EDIT, MOUSE(), GUI CURSOR LINK MOUSE) if it did not answer then, PS/2 telling no one a
   mouse was plugged in. Its errors (no mouse: "Mouse timeout", after half a second) are kept from the caller, and a try
   that failed is not made again within a second. 1 if it is running. */
int MouseOptionStart(void)
{
    static unsigned long long failed = 0;
    if (Option.Mouse != 0)
        return 0;
    if (!mouse0 && (failed == 0 || mSecTimer - failed >= 1000))
        failed = UiTry(UiStartPS2, NULL) ? 0 : (mSecTimer ? mSecTimer : 1);
    return mouse0;
}
