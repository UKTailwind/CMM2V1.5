/***************************************************************************
CMM2 MMBasic
GUI.c

Does all the LCD display commands and I/O in MMBasic.

Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holders nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

4. The name MMBasic be used when referring to the interpreter in any
   documentation and promotional material and the original copyright message
  be displayed  on the console at startup (additional copyright messages may
   be added).

5. All advertising materials mentioning features or use of this software must
   display the following acknowledgement: This product includes software
   developed by Geoff Graham and Peter Mather.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

------------------------------------------------------------------------------
  * In addition the software components from STMicroelectronics are provided
  * subject to the license as detailed below:
------------------------------------------------------------------------------
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
*******************************************************************************/


#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "float.h"
#define BTN_SIDE_BRIGHT     -25
#define BTN_SIDE_DULL       -50
#define BTN_SIDE_WIDTH      4
#define BTN_CAPTION_SHIFT   2

#define CLICK_DURATION      3           // the duration of a "click" in mSec

#define CTRL_NORMAL         0b0000000   // the control should be displayed as normal
#define CTRL_DISABLED       0b0000001   // the control is disabled and displayed in dull colours
#define CTRL_DISABLED2      0b0000010   // as above but only used when a keyboard is active
#define CTRL_HIDDEN         0b0000100   // the control is hidden
#define CTRL_HIDDEN2        0b0001000   // only used when setting a control to hidden
#define CTRL_SPINUP         0b0010000   // the spinbox up arrow is touched
#define CTRL_SPINDOWN       0b0100000   // ditto down
#define CTRL_SELECTED       0b1000000   // for a textbox or numberbox indicated that the box is selected

#define BTN_DISABLED        -55

// define what the function DrawKeyboard() will do
#define KEY_OPEN            1
#define KEY_DRAW_ALL        2
#define KEY_KEY_DWN         3
#define KEY_KEY_UP          4
#define KEY_KEY_CANCEL      5

#define CTRL_BUTTON         1
#define CTRL_SWITCH         2
#define CTRL_RADIOBTN       3
#define CTRL_CHECKBOX       4
#define CTRL_LED            5
#define CTRL_SPINNER        6
#define CTRL_FRAME          7
#define CTRL_NBRBOX         8
#define CTRL_TEXTBOX        9
#define CTRL_FMTBOX         10
#define CTRL_DISPLAYBOX     11
#define CTRL_CAPTION        12
#define CTRL_AREA           13
#define CTRL_GAUGE          14
#define CTRL_BARGAUGE       15
#define CTRL_LISTBOX        16
#define CTRL_SLIDER         17

#define MAX_PAGES           32          // the number of pages that can be specifies (this must not exceed 32)
extern void cmd_guiMX170(void);
extern volatile struct s_nunstruct mousestruct[4];


// used by the gauge control to store extra data in the allocated string space
#define GAUGE_UNITS_SIZE 32
struct s_GaugeS {
    char units[GAUGE_UNITS_SIZE];
    MMFLOAT ta, tb, tc;
    int c1, c2, c3, c4;
    int laststrlen, cval, csaved, lastfc, lastbc;
};

// The LISTBOX control (the PicoMite's) keeps this in its allocated string space (Ctrl[r].s, MAXSTRLEN bytes), as the
// GAUGE keeps s_GaugeS. It is bound live to a BASIC string array: 'items' points at the variable's data, so the array
// must be global for the life of the control (GetCtrlParams refuses a LOCAL one). The selected index is Ctrl[r].value
// and 'top' the first row shown in the pop-up.
struct s_ListBoxS {
    unsigned char *items;                   // the string array's data: each element a length byte and the characters
    int stride;                             // bytes an element (its declared length + 1)
    int count;                              // the number of elements
    int top;                                // the first row shown while the pop-up is open
    int maxrows;                            // the most rows the pop-up shows before it scrolls (0: as many as fit)
    short pop_x1, pop_y1, pop_x2, pop_y2;   // the pop-up's rectangle while it is open
    short vis_rows;                         // the data rows shown in the pop-up
    short pend;                             // the item touched (selected on release), -1 none, -2 the opening touch,
                                            // -3 an arrow
    unsigned char arrows;                   // 1 if the scroll arrows are drawn above and below the rows
};


int SetupPage;
unsigned int CurrentPages;


int display_backlight;                  // the brightness of the backlight (1 to 100)

int gui_click_pin = 0;                  // the sound pin

//volatile int CursorTimer;               // used to time the flashing cursor
volatile int ClickTimer = 0;            // used to time the click when touch occurs
volatile int TouchTimer;                // used to time the response to touch
int CheckGuiFlag = 0;                   // used to tell the mSec timer to call CheckGui()

int CurrentRef;                         // if the pen is down this is the control (or zero if not on a control)
int LastRef;                            // this is the last control touched
int LastX;                              // this is the x coord when the pen was lifted
int LastY;                              // ditto for y

MMFLOAT CtrlSavedVal;                   // a temporary place to save a control's value

int TouchX, TouchY;
volatile int TouchDown = false;
volatile int TouchUp = false;
volatile int TouchState = false;


int last_x2, last_y2;                   // defaults used when creating controls
MMFLOAT last_inc, last_min, last_max;
int last_fcolour, last_bcolour;
MMFLOAT last_ta, last_tb, last_tc;
int last_c1 = -1, last_c2, last_c3, last_c4;
char last_units[32];

// used for keypads
int GUIKeyDown = 0;                        // true if a key is down
int KeyAltShift = 0;                    // true if in alt keypad layout
int InvokingCtrl = 0;                   // the number of the control that invoked the keypad
//int InCallback = 0;                     // true if we are running MM.KEYPRESS
//int InPause = 0;                        // true if we are inside a PAUSE command (used to suppress calling MM.KEYPRESS)
char CancelValue[256];                  // save the value of the control in case the user cancels

int gui_int_down;                       // true if the touch has triggered an interrupt
char *GuiIntDownVector = NULL;          // address of the interrupt routine or NULL if no interrupt
int gui_int_up;                         // true if the release of the touch has triggered an interrupt
char *GuiIntUpVector = NULL;            // address of the interrupt routine or NULL if no interrupt
volatile int DelayedDrawKeyboard = false;        // a flag to indicate that the pop-up keyboard should be drawn AFTER the pen down interrupt
volatile int DelayedDrawFmtBox = false;          // a flag to indicate that the pop-up formatted keyboard should be drawn AFTER the pen down interrupt
volatile int DelayedDrawListbox = false;         // a flag to indicate that the pop-up listbox should be drawn AFTER the pen down interrupt
volatile int ListBoxArrowRepeat = false;         // set by the listbox pop-up while a scroll arrow is held, so ProcessTouch repeats it
int SliderDrag = 0;                              // the slider being dragged: it follows the pointer while the button is held

struct s_ctrl *Ctrl=(void*)0x30008000;                    // list of the controls


int ChangeBright(int c, int pct);
void SpecialWritePixel(int x, int y, unsigned int tc, int status);
void SpecialDrawLine(int x1, int y1, int x2, int y2, int w, int tc, int status);
void SpecialDrawBox(int x1, int y1, int x2, int y2, int w, int c, int fill, int status);
void SpecialDrawRBox(int x1, int y1, int x2, int y2, int radius, int c, int fill, int status);
void SpecialPrintString(int x, int y, int jh, int jv, int jo, int fc, int bc, char *str, int status);
void SpecialDrawCircle(int x, int y, int radius, int w, int c, int fill, MMFLOAT aspect, int status);
void SpecialDrawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int c, int fill, int status);
void DrawBorder(int x1, int y1, int x2, int y2, int w, int tc, int bc, int status);
void DrawBasicButton(int x1, int y1, int x2, int y2, int w, int up, int c, int status);
void DrawButton(int r);
void DrawSwitch(int r);
void DrawCheckBox(int r);
void DrawRadioBtn(int r);
void DrawLED(int r);
void DrawSpinner(int r);
void DrawFrame(int r);
void DrawCaption(int r);
void DrawGauge(int r);
void DrawBarGauge(int r);
char *GetCaption(int k, int is_alpha, int alt);
void GetSingleKeyCoord(int k, int is_alpha, int *x1, int *y1, int *x2, int *y2);
void DrawDisplayBox(int r);
void PopUpRedrawAll(int r, int disabled);
void KeyPadErase(int is_alpha);
void DrawSingleKey(int is_alpha, int x1, int y1, int x2, int y2, char *s, int fc, int bc);
void DrawKeyboard(int mode);
void DrawControl(int r);
void UpdateControl(int r);
void SetCtrlState(int r, int state, int err);
void DoCallback(int InvokingCtrl, char *key);
void DrawFmtBox(int mode);
void DrawListBox(int r);
void DrawListBoxPopup(int mode);
void DrawSlider(int r);
extern void cmd_mouse(char *cmd);
int GUIactive=0;

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The GUI command is the base of all the sophisticated GUI drawing features in the Micromite Plus
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
#define GET_X_AXIS 1
#define GET_Y_AXIS 2
#define TOUCH_ERROR -1
// GUI CLICK (the PicoMite's): a click the program makes, or a button on a pin makes, where the pointer is. The timer
// (Timers.c) takes these with the mouse's left button.
volatile int GuiClickDown = 0;                                      // GUI CLICK DOWN, or a momentary click's down half
int GuiClickPin = 0, GuiClickPinInv = 0;                            // GUI CLICK PIN: its pin (0 none), and active high
int GuiClickPhase = 0;                                              // a momentary GUI CLICK: 1 its down half, 2 its up half
// is GUI CLICK PIN's button pressed (from the timer interrupt too)
int ClickPinDown(void){
	return GuiClickPin && (PinRead(GuiClickPin) ? GuiClickPinInv : !GuiClickPinInv);
}
// is the GUI's button down: a mouse's left button, GUI CLICK's, GUI CLICK PIN's, or a USB touch screen touched (the timer's
// touch events, a held control's repeat in ProcessTouch, and CLICK(DOWN) all read this); a press ends when none holds it
int GuiButtonDown(void){
	return TOUCH_DOWN || GuiClickDown || ClickPinDown() || usb_touch_active;
}
volatile int GuiClickFromMouse = 0;                                 // the PicoMite's gui_click_from_mouse and gui_click_emulated
volatile int GuiClickEmulated = 0;
static void GuiPointerAt(int *x, int *y);
// the 1 mS tick's GUI press, the PicoMite's two detectors (PicoMite.c): a press is owned by what made it, a finger on the
// USB touch screen, or a mouse's left button, GUI CLICK or GUI CLICK PIN, and only its release ends it. A mouse's press
// is taken where it was made (GUI CLICK's and GUI CLICK PIN's at the cursor's point); a finger's is read by ProcessTouch.
void GuiPressTick(void){
    if(usb_touch_present){
        if(usb_touch_active && !TouchState){
            TouchState = TouchDown = true;
            GuiClickFromMouse = GuiClickEmulated = 0;               // (a real pointing device)
        } else if(!usb_touch_active && TouchState && !GuiClickFromMouse){
            TouchState = false;
            TouchUp = true;
        }
    }
    if((GUI_INPUT_ON && Option.MaxCtrls>0) || osk_state){           // (the on-screen keyboard is usable with no controls)
        int pin = ClickPinDown(), m = GuiMouse();
        int btn = (TOUCH_DOWN && m >= 0) || GuiClickDown || pin;
        if(btn && !TouchState){
            if(TOUCH_DOWN && m >= 0){ TouchX = mousestruct[m].ax; TouchY = mousestruct[m].ay; }
            else GuiPointerAt(&TouchX, &TouchY);
            TouchState = TouchDown = true;
            GuiClickFromMouse = 1;
            GuiClickEmulated = !(TOUCH_DOWN && m >= 0);
        } else if(!btn && TouchState && GuiClickFromMouse){
            TouchState = false;
            TouchUp = true;
        }
    }
}
// where GUI CLICK and GUI CLICK PIN press: the cursor's point, shown or not (the PicoMite's cursor_x, cursor_y: GUI CURSOR
// x, y and GUI CLICK x, y set it, and a linked cursor's follows the mouse)
static void GuiPointerAt(int *x, int *y){
	if(autocursor){                                                 // linked: the mouse's position, where the cursor is shown
		int m = MouseOpenChannel();                                 // (it is drawn there between statements; this is read in a
		if(m < 0) m = GuiMouse();                                   // PAUSE too: the PicoMite's linked cursor_x is the mouse's)
		if(m >= 0){ *x = mousestruct[m].ax; *y = mousestruct[m].ay; return; }
	}
	*x = xcursor + xoffcursor;
	*y = ycursor + yoffcursor;
}
int GetTouch(int axis){
	int m = GuiMouse(), x, y;
    if (usb_touch_active && !GuiClickFromMouse)                     // a USB touch screen (the PicoMite's VGA/HDMI GetTouch; a press
                                                                    // a mouse owns is the mouse's: its fun_touch)
    {
        if (axis == GET_X_AXIS)
            return usb_touch_x;
        if (axis == GET_Y_AXIS)
            return usb_touch_y;
    }
	if(TOUCH_DOWN && m >= 0){ x = mousestruct[m].ax; y = mousestruct[m].ay; }
	else if(GuiClickDown || ClickPinDown()) GuiPointerAt(&x, &y);
	else return TOUCH_ERROR;
	return axis == GET_X_AXIS ? x : y;
}
// the MMBasic TOUCH() function
void fun_touch(void) {
    char *tp;
    if(checkstring(ep, "X"))
        iret = GetTouch(GET_X_AXIS);
    else if(checkstring(ep, "Y"))
        iret = GetTouch(GET_Y_AXIS);
    else if(checkstring(ep, "REF"))
        iret = CurrentRef;
    else if(checkstring(ep, "LASTREF"))
        iret = LastRef;
    else if(checkstring(ep, "LASTX"))
        iret = LastX;
    else if(checkstring(ep, "LASTY"))
        iret = LastY;
    else if(checkstring(ep, "DOWN"))
        iret = GuiButtonDown();
    else if(checkstring(ep, "UP"))
        iret = !GuiButtonDown();
    /* a USB touch screen's: the PicoMite's X2, Y2, XN, YN and gestures (graphics/Pointer.c; the gestures USBTouch.c) */
    else if (checkstring(ep, "X2"))
    {
        int x2 = TOUCH_ERROR;
        if (x2 == TOUCH_ERROR && usb_touch_active2)
            x2 = usb_touch_x2;
        iret = x2;
    }
    else if (checkstring(ep, "Y2"))
    {
        int y2 = TOUCH_ERROR;
        if (y2 == TOUCH_ERROR && usb_touch_active2)
            y2 = usb_touch_y2;
        iret = y2;
    }
    /* nth contact - TOUCH(XN n) / TOUCH(YN n), n = 1..MAX_TOUCH_CONTACTS.
       Same two-source merge as X2/Y2 (hardware panel first, then USB
       multi-touch), but generalised to every contact: n=1 matches X/Y,
       n=2 matches X2/Y2, and a GT911 or USB panel can carry more. Returns
       TOUCH_ERROR for any contact that isn't currently touching, so a BASIC
       loop can count fingers by walking n until it sees -1. */
    else if ((tp = checkstring(ep, "XN")))
    {
        int n = getint(tp, 0, MAX_TOUCH_CONTACTS);
        if (n == 0)
        {
            /* Contact count - hardware panel first, USB as fallback, the
               same source precedence used per-contact below. */
            int cnt = 0;
            if (cnt == 0)
                cnt = usb_touch_count;
            iret = cnt;
        }
        else
        {
            int xn = TOUCH_ERROR;
            if (xn == TOUCH_ERROR && (n - 1) < usb_touch_count)
                xn = usb_touch_xn[n - 1];
            iret = xn;
        }
    }
    else if ((tp = checkstring(ep, "YN")))
    {
        int n = getint(tp, 0, MAX_TOUCH_CONTACTS);
        if (n == 0)
        {
            int cnt = 0;
            if (cnt == 0)
                cnt = usb_touch_count;
            iret = cnt;
        }
        else
        {
            int yn = TOUCH_ERROR;
            if (yn == TOUCH_ERROR && (n - 1) < usb_touch_count)
                yn = usb_touch_yn[n - 1];
            iret = yn;
        }
    }
    /* Single-finger swipe gestures. Latched at the touch-up edge by
       touch_gesture_on_up() (see gesture section above). Each one-shot
       accessor consumes its matching direction on read (mirroring the
       mouse double-click flag): reading TOUCH(SWL) on a left swipe
       returns 1 and clears; reading TOUCH(SWR) on the same left swipe
       returns 0 without clearing, so a polling loop that tests SWL
       first then SWR doesn't accidentally lose the event.
       TOUCH(SWIPE) returns the code (0/1/2/3/4) AND clears, so it's
       the natural form for a SELECT CASE dispatch. */
    else if (checkstring(ep, "SWL"))
    {
        if (touch_swipe_dir == 1)
        {
            iret = 1;
            touch_swipe_dir = 0;
        }
        else
            iret = 0;
    }
    else if (checkstring(ep, "SWR"))
    {
        if (touch_swipe_dir == 2)
        {
            iret = 1;
            touch_swipe_dir = 0;
        }
        else
            iret = 0;
    }
    else if (checkstring(ep, "SWU"))
    {
        if (touch_swipe_dir == 3)
        {
            iret = 1;
            touch_swipe_dir = 0;
        }
        else
            iret = 0;
    }
    else if (checkstring(ep, "SWD"))
    {
        if (touch_swipe_dir == 4)
        {
            iret = 1;
            touch_swipe_dir = 0;
        }
        else
            iret = 0;
    }
    else if (checkstring(ep, "SWIPE"))
    {
        iret = touch_swipe_dir;
        touch_swipe_dir = 0;
    }
    /* Two-finger pinch gestures. Latched when the second contact
       lifts (or the first does, whichever ends the dual-touch).
       Same clear-on-matching-read semantics as the swipes. */
    else if (checkstring(ep, "EXPAND"))
    {
        if (touch_pinch_dir == 1)
        {
            iret = 1;
            touch_pinch_dir = 0;
        }
        else
            iret = 0;
    }
    else if (checkstring(ep, "CONTRACT"))
    {
        if (touch_pinch_dir == 2)
        {
            iret = 1;
            touch_pinch_dir = 0;
        }
        else
            iret = 0;
    }
    else if (checkstring(ep, "PINCH"))
    {
        iret = touch_pinch_dir; /* 0=none, 1=expand, 2=contract */
        touch_pinch_dir = 0;
    }
    /* Single-finger tap / long-press / double-tap. All clear-on-read. */
    else if (checkstring(ep, "TAP"))
    {
        iret = touch_tap;
        touch_tap = 0;
    }
    else if (checkstring(ep, "HOLD"))
    {
        iret = touch_longpress;
        touch_longpress = 0;
    }
    else if (checkstring(ep, "DTAP"))
    {
        iret = touch_doubletap;
        touch_doubletap = 0;
    }
    /* Two-finger rotate / two-finger tap. Same clear-on-read pattern. */
    else if (checkstring(ep, "CW"))
    {
        if (touch_rotate_dir == 1)
        {
            iret = 1;
            touch_rotate_dir = 0;
        }
        else
            iret = 0;
    }
    else if (checkstring(ep, "CCW"))
    {
        if (touch_rotate_dir == 2)
        {
            iret = 1;
            touch_rotate_dir = 0;
        }
        else
            iret = 0;
    }
    else if (checkstring(ep, "ROTATE"))
    {
        iret = touch_rotate_dir; /* 0=none, 1=CW, 2=CCW */
        touch_rotate_dir = 0;
    }
    else if (checkstring(ep, "TTAP"))
    {
        iret = touch_twotap;
        touch_twotap = 0;
    }
    else
        error("Invalid argument");

    targ = T_INT;
}
// CLICK(), the PicoMite's (graphics/Pointer.c): whether anything is clicking, a mouse's left button, GUI CLICK or GUI CLICK
// PIN (a USB touch screen is TOUCH()'s), and where: the cursor's point, which a linked cursor's mouse and GUI CLICK set
#define GUI_TEST_DOUBLETAP_MIN_US 100000ull
#define GUI_TEST_DOUBLETAP_MAX_US 500000ull
#define GUI_TEST_DOUBLETAP_RADIUS 50
// GUI TEST's way out with the on-screen keyboard configured: a double tap (the PicoMite's graphics/Draw.c)
static bool GuiTestDoubleTap(bool *wasdown, uint64_t *lastdown)
{
    static int lastx, lasty; // position of the previous down edge
    bool down, tapped = false;
    if (Option.VRes_reserved == 0)
        return false; // no OSK configured: keypress remains the only exit
    int x = GetTouch(GET_X_AXIS);
    down = (x != TOUCH_ERROR);
    if (down && !*wasdown)
    {
        int y = GetTouch(GET_Y_AXIS);
        uint64_t gap = Getuptime() - *lastdown;
        tapped = (gap >= GUI_TEST_DOUBLETAP_MIN_US && gap <= GUI_TEST_DOUBLETAP_MAX_US &&
                  abs(x - lastx) <= GUI_TEST_DOUBLETAP_RADIUS && abs(y - lasty) <= GUI_TEST_DOUBLETAP_RADIUS);
        *lastdown = Getuptime();
        lastx = x;
        lasty = y;
    }
    *wasdown = down;
    if (!tapped)
        return false;
    /* Wait for the finger to lift before returning so the still-held second
       tap can't register as a key press on the OSK that the post-test
       ClearScreen() is about to redraw. */
    while (GetTouch(GET_X_AXIS) != TOUCH_ERROR)
        CheckAbort();
    return true;
}
void fun_click(void) {
    int btn = ((TOUCH_DOWN && GuiMouse() >= 0) || GuiClickDown || ClickPinDown()) ? 1 : 0;
    if(checkstring(ep, "DOWN"))
        iret = btn;
    else if(checkstring(ep, "UP"))
        iret = !btn;
    else if(checkstring(ep, "X") || checkstring(ep, "Y")) {
        int x, y;
        GuiPointerAt(&x, &y);                                       // (the cursor's point)
        iret = btn ? (checkstring(ep, "X") ? x : y) : TOUCH_ERROR;
    }
    else if(checkstring(ep, "REF"))
        iret = CurrentRef;
    else if(checkstring(ep, "LASTREF"))
        iret = LastRef;
    else if(checkstring(ep, "LASTX"))
        iret = LastX;
    else if(checkstring(ep, "LASTY"))
        iret = LastY;
    else
        SyntaxError();
    targ = T_INT;
}
/*********************************************************************************************
 Most controls have similar argument lists, so this function is a "universal" argument
 collector for all controls.
 It returns with the control's reference number in case the caller wants to make some
 more adjustments.
 ********************************************************************************************/
// the GUI controls' mouse: a USB mouse when one is plugged in (channel 2: it takes effect at once, as for GUI CURSOR
// LINK MOUSE, and no other channel can be opened while it is), else OPTION MOUSE's channel, else one the program opened
int GuiMouse(void){
	if(UsbMouse) return USBMOUSE_CHANNEL;
	return Option.Mouse >= 0 ? Option.Mouse : MouseOpenChannel();
}
// the GUI's pointer, shown once there is a mouse (one plugged in after the GUI started too: check_interrupt calls this),
// in every depth (in a 4 or 1-bit one over the display: Rgb121.c). It is GUI CURSOR ON : GUI CURSOR LINK MOUSE, at the
// mouse (Peter, 27 Oct 2026), so GUI CURSOR's commands work on it; one the program put on is left as it is
static int GuiPointerWanted = 0;
void GuiPointer(void){
	if(!GuiPointerWanted || GuiMouse() < 0) return;
	GuiPointerWanted = 0;
	if(!cursorenable){
		int m = GuiMouse();
		CursorOn(cursor == 2 && VideoColour < 8 ? 0 : cursor, mousestruct[m].ax, mousestruct[m].ay);
		autocursor = 1;
	}
}
// the GUI started by its first command: OPTION MOUSE's mouse opened if it is not (a USB one needs nothing), and the
// touch events taken from then on. A mouse need not be there yet: a USB mouse plugged in later is used at once, and
// OPTION MOUSE 0's PS/2 mouse is started as EDIT and FM start it, with no error if none answers (UiMouse.c).
void mouseon(void){
    if(!GUIactive){
		int m = GuiMouse();
		if(m == 0) MouseOptionStart();
		else if(m >= 0 && MouseOpenChannel() != m){
			char mouseopen[20]={0};
			strcpy(mouseopen,"OPEN n,,,m");
			mouseopen[5]=m+48;
	    	if(Option.Sensitivity<10)mouseopen[9]=Option.Sensitivity+48;
	    	else {
	    		mouseopen[9]='1';                                    // (sensitivity 10: it was written into inpbuf)
	    		mouseopen[10]='0';
	    	}
	    	cmd_mouse(mouseopen);
		}
		TouchDown = TouchUp = TouchState = false;                  // (a press before the GUI started is not its)
		GuiPointerWanted = 1;
		GUIactive=1;
		GuiPointer();
    }
}
int GetCtrlParams(int type, char *p) {
    int r, a;
    struct s_GaugeS *GaugeS;                                        // in case we are dealing with a GAUGE control
    if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
    mouseon();
    getargs(&p, 40, ",");
    if((argc & 1) != 1) error("Argument count");
    if(*argv[0] == '#') argv[0]++;
    r = getint(argv[0], 1, Option.MaxCtrls);
    if(Ctrl[r].type) error("GUI reference number #% is in use", r);
    if(argc < 5) error("Argument count");
    a = 0;

    // setup the defaults
    Ctrl[r].x2 = last_x2; Ctrl[r].y2 = last_y2;
    if(type == CTRL_CAPTION) {
        Ctrl[r].fc = gui_fcolour;                                   // a caption defaults to the system colours set by the COLOUR command
        Ctrl[r].bc = gui_bcolour;
    } else {
        Ctrl[r].fc = last_fcolour;                                  // the others take the last colours used in the last command
        Ctrl[r].bc = last_bcolour;
    }
    Ctrl[r].inc = last_inc; Ctrl[r].min = last_min; Ctrl[r].max = last_max;

    // save the current font, needed if we redraw the control
    Ctrl[r].font = gui_font;

    // get string space
    Ctrl[r].s = GetMemory(MAXSTRLEN);

    // and the caption if the control needs it
    switch(type) {                  // these need a caption
        case CTRL_CAPTION:
        case CTRL_BUTTON:
        case CTRL_LED:
        case CTRL_SWITCH:
        case CTRL_FRAME:
        case CTRL_RADIOBTN:
        case CTRL_CHECKBOX:         strcpy(Ctrl[r].s, getCstring(argv[a += 2]));
    }

    if(type == CTRL_FMTBOX) {
        // get string space
        a += 2;
        Ctrl[r].fmt = NULL;
        if(*argv[a] != '"') {
            // Format String:
            // xc where x is the maximum value digit and c is the ghost char
            // these are concatenated to make a string of digits (eg, seconds = 5s9s)
            // Special constructs: 1 to 32 = 3dDd       1 to 12 = 1mMm
            //                     1 to 24 = 2hHh       0 to 179 = 1dLd9d
            // A separator is in brackets.  Eg (:)   or   (' )
            // Special chars: A = AM/PM    N = N/S    E = E/W
            if(checkword(argv[a], "DATE1")) Ctrl[r].fmt = "3dDd(/)1mMm(/)9y9y";
            if(checkword(argv[a], "DATE2")) Ctrl[r].fmt = "1mMm(/)3dDd(/)9y9y";
            if(checkword(argv[a], "DATE3")) Ctrl[r].fmt = "9y9y9y9y(/)1mMm(/)3dDd";
            if(checkword(argv[a], "TIME1")) Ctrl[r].fmt = "2hHh(:)5m9m";
            if(checkword(argv[a], "TIME2")) Ctrl[r].fmt = "2hHh(:)5m9m(:)5s9s";
            if(checkword(argv[a], "TIME3")) Ctrl[r].fmt = "1hMh(:)5m9m( )A";
            if(checkword(argv[a], "TIME4")) Ctrl[r].fmt = "1hMh(:)5m9m(:)5s9s( )A";
            if(checkword(argv[a], "DATETIME1")) Ctrl[r].fmt = "3dDd(/)1mMm(/)9y9y( )1hMh(:)5m9m( )A";
            if(checkword(argv[a], "DATETIME2")) Ctrl[r].fmt = "3dDd(/)1mMm(/)9y9y( )2hHh(:)5m9m";
            if(checkword(argv[a], "DATETIME3")) Ctrl[r].fmt = "1mMm(/)3dDd(/)9y9y( )1hMh(:)5m9m( )A";
            if(checkword(argv[a], "DATETIME4")) Ctrl[r].fmt = "1mMm(/)3dDd(/)9y9y( )2hHh(:)5m9m";
            if(checkword(argv[a], "LAT1")) Ctrl[r].fmt = "8d9d(` )5m9m(' )5s9s(\" )N";
            if(checkword(argv[a], "LAT2")) Ctrl[r].fmt = "8d9d(` )5m9m(' )5s9s(.)9s(\" )N";
            if(checkword(argv[a], "LONG1")) Ctrl[r].fmt = "1dLd9d(` )5m9m(' )5s9s(\" )E";
            if(checkword(argv[a], "LONG2")) Ctrl[r].fmt = "1dLd9d(` )5m9m(' )5s9s(.)9s(\" )E";
            if(checkword(argv[a], "ANGLE1")) Ctrl[r].fmt = "9d9d9d(` )5m9m(')";
        }
        if(Ctrl[r].fmt == NULL) {                                   // a string, quoted or any expression (f$): it
            char *s = getCstring(argv[a]);                          // stayed unset, with no error (the PicoMite's #112)
            if(*s == 0) error("Invalid format");
            Ctrl[r].fmt = GetMemory(MAXSTRLEN);
            strcpy(Ctrl[r].fmt, s);
        }
    }

    if(type == CTRL_LISTBOX) {
        // bound to the string array given as the second argument, its binding kept in the string space (as a GAUGE)
        struct s_ListBoxS *lb = (struct s_ListBoxS *)Ctrl[r].s;
        unsigned char str_size = 0;
        int dims[MAXDIM] = {0};
        a += 2;
        memset(lb, 0, sizeof(struct s_ListBoxS));
        lb->count = parsestringarray(argv[a], (char **)&lb->items, 2, 1, dims, false, &str_size);
        RequireGlobal(lb->items);                                   // read whenever the control is drawn or touched
        lb->stride = (int)str_size + 1;
    }


    // get x1 and y1 - all controls require these
    Ctrl[r].x1 = getint(argv[a += 2], 0, HRes);
    if(argc < a) error("Argument count");
    Ctrl[r].y1 = getint(argv[a += 2], 0, VRes);

    // the fourth argument in GUI CAPTION is the justification
    if(type == CTRL_CAPTION) {
        a += 2;
        if(!(argc < a || *argv[a] == 0)) {                          // if justification is specified
            int jh = 0, jv = 0, jo = 0;
            if(!GetJustification(argv[a], &jh, &jv, &jo))
                if(!GetJustification(getCstring(argv[a]), &jh, &jv, &jo))
                    error("Justification");
            Ctrl[r].x2 = jh | jv << 2 | jo << 4;                    // stuff the justification parameters into the short int
        }
        else
            Ctrl[r].x2 = 0;
    } else {
        // get the width - all controls except CAPTION need this
        // for GAUGE, LED, etc this is the radius
        if(argc > a + 2) if(*argv[a += 2] != 0) last_x2 = Ctrl[r].x2 = getint(argv[a], BTN_SIDE_WIDTH, HRes);

        // now get or set the height
        switch(type) {
            case CTRL_NBRBOX:          *Ctrl[r].s = '0';            // this needs to be set, then fall thru
            case CTRL_BUTTON:
            case CTRL_SWITCH:
            case CTRL_FRAME:
            case CTRL_TEXTBOX:
            case CTRL_FMTBOX:
            case CTRL_DISPLAYBOX:
            case CTRL_SPINNER:
            case CTRL_AREA:
            case CTRL_LISTBOX:
            case CTRL_SLIDER:
            case CTRL_BARGAUGE:         // these all need the height in addition to the width
                                        if(argc > a + 2) if(*argv[a += 2] != 0) last_y2 = Ctrl[r].y2 = getint(argv[a], BTN_SIDE_WIDTH, VRes);
                                        if(type != CTRL_BARGAUGE) { Ctrl[r].x2 +=  Ctrl[r].x1; Ctrl[r].y2 +=  Ctrl[r].y1; }
                                        break;

            case CTRL_CHECKBOX:         // the check box does not need the height and is a special case
                                        // its touch sensitive area is different from its drawing parameters
                                        // we have the width/height in x2 and it is saved in Ctrl[r].inc
                                        Ctrl[r].inc = Ctrl[r].x2;
                                        Ctrl[r].y2 =  Ctrl[r].y1 + Ctrl[r].x2;  // calculate the touch sensitive area
                                        Ctrl[r].x2 = Ctrl[r].x1 + Ctrl[r].x2 + (gui_font_width * (strlen(Ctrl[r].s) + 1));  // calculate the touch sensitive area
                                        break;

            case CTRL_LED:
            case CTRL_RADIOBTN:         // the LED and radio button also do not need the height and are a special case
                                        // their touch sensitive area is stored in x1, y1, x2 and y2
                                        // the radius is stored in Ctrl[r].max and the X and Y centre of the button is calculated when drawing the control
                                        Ctrl[r].max = Ctrl[r].x2;
                                        Ctrl[r].x2 = Ctrl[r].x1 + Ctrl[r].max + (gui_font_width * (strlen(Ctrl[r].s) + 1));  // calculate the touch sensitive area
                                        Ctrl[r].x1 -= Ctrl[r].max;
                                        Ctrl[r].y2 = Ctrl[r].y1 + Ctrl[r].max;
                                        Ctrl[r].y1 -= Ctrl[r].max;
                                        break;

        }
    }

    // get the foreground colour - all controls except the area control need this
    if(type != CTRL_AREA && argc > a + 2) if(*argv[a += 2] != 0) last_fcolour = Ctrl[r].fc = getint(argv[a], BLACK, WHITE);

    switch(type) {                  // these all need the background colour
        case CTRL_SPINNER:
        case CTRL_BUTTON:
        case CTRL_SWITCH:
        case CTRL_TEXTBOX:
        case CTRL_FMTBOX:
        case CTRL_NBRBOX:
        case CTRL_LISTBOX:
        case CTRL_SLIDER:
        case CTRL_DISPLAYBOX:       if(argc > a + 2) if(*argv[a += 2] != 0) last_bcolour = Ctrl[r].bc = getint(argv[a], 0, WHITE);
                                    break;
        case CTRL_CAPTION:
        case CTRL_GAUGE:
        case CTRL_BARGAUGE:         if(argc > a + 2) if(*argv[a += 2] != 0) last_bcolour = Ctrl[r].bc = getint(argv[a], -1, WHITE);
                                    break;
    }

    if(type == CTRL_SPINNER) {
        if(argc > a + 2) if(*argv[a += 2] != 0) last_inc = Ctrl[r].inc = getnumber(argv[a]);
        if(argc > a + 2) if(*argv[a += 2] != 0) last_min = Ctrl[r].min = getnumber(argv[a]);
        if(argc > a + 2) last_max = Ctrl[r].max = getnumber(argv[a += 2]);
    }

    if(type == CTRL_LISTBOX) {                                      // the most rows the pop-up shows before it scrolls
        struct s_ListBoxS *lb = (struct s_ListBoxS *)Ctrl[r].s;
        if(argc > a + 2) if(*argv[a += 2] != 0) lb->maxrows = getint(argv[a], 1, 100);
    }

    if(type == CTRL_SLIDER) {                                       // min, max and the step (default 0 to 100, no step)
        Ctrl[r].min = 0; Ctrl[r].max = 100; Ctrl[r].inc = 0;
        if(argc > a + 2) if(*argv[a += 2] != 0) Ctrl[r].min = getnumber(argv[a]);
        if(argc > a + 2) if(*argv[a += 2] != 0) Ctrl[r].max = getnumber(argv[a]);
        if(argc > a + 2) if(*argv[a += 2] != 0) Ctrl[r].inc = getnumber(argv[a]);
    }

    if(type == CTRL_GAUGE || type == CTRL_BARGAUGE) {
        // special processing for a gauge
        // Note that a GAUGE uses the allocated string memory (Ctrl[r].s) for also storing other data
        if(argc > a + 2) if(*argv[a += 2] != 0) last_min = Ctrl[r].min = getnumber(argv[a]);
        if(argc > a + 2) if(*argv[a += 2] != 0) last_max = Ctrl[r].max = getnumber(argv[a]);
        if(type == CTRL_GAUGE) { if(argc > a + 2) if(*argv[a += 2] != 0) last_inc = Ctrl[r].inc = getnumber(argv[a]); }

        strcpy(Ctrl[r].s, last_units);                              // setup the default default
        if(type == CTRL_GAUGE && argc > a + 2) {
            if(*argv[a += 2] != 0) {
                strcpy(Ctrl[r].s, getCstring(argv[a]));             // get the units caption
                Ctrl[r].s[GAUGE_UNITS_SIZE - 1] = 0;                // truncate to max size
               strcpy(last_units, Ctrl[r].s);                       // save as default
            } else
                *last_units = *Ctrl[r].s = 0;
        }
        GaugeS = (void *)(Ctrl[r].s);
        GaugeS->c1 = -1; GaugeS->c2 = GaugeS->c3 = GaugeS->c4 = gui_fcolour;
        GaugeS->ta = GaugeS->tb = GaugeS->tc = Ctrl[r].max;
        if(argc > a + 2) if(*argv[a += 2] != 0) GaugeS->c1 = getint(argv[a], 0, WHITE);
        if(argc > a + 2) if(*argv[a += 2] != 0) GaugeS->ta = getnumber(argv[a]);
        if(argc > a + 2) if(*argv[a += 2] != 0) GaugeS->c2 = getint(argv[a], 0, WHITE);
        if(argc > a + 2) if(*argv[a += 2] != 0) GaugeS->tb = getnumber(argv[a]);
        if(argc > a + 2) if(*argv[a += 2] != 0) GaugeS->c3 = getint(argv[a], 0, WHITE);
        if(argc > a + 2) if(*argv[a += 2] != 0) GaugeS->tc = getnumber(argv[a]);
        if(argc > a + 2) if(*argv[a += 2] != 0) GaugeS->c4 = getint(argv[a], 0, WHITE);
        if(type == CTRL_GAUGE) Ctrl[r].y2 = -1;                     // on first use draw the full gauge
    }

    if(argc > a + 1) error("Argument count");
    Ctrl[r].type = type;
    if(type == CTRL_GAUGE || type == CTRL_BARGAUGE || type == CTRL_SLIDER) {
        Ctrl[r].value = Ctrl[r].min;
    } else if(type == CTRL_LISTBOX) {                               // the first item selected, or none if there are none
        Ctrl[r].value = ((struct s_ListBoxS *)Ctrl[r].s)->count > 0 ? 0 : -1;
    } else {
        Ctrl[r].value = 0;
    }
    Ctrl[r].page = SetupPage;
    Ctrl[r].fcc = gui_fcolour;
    SetCtrlState(r, CTRL_NORMAL, false);
    return r;
}


// the mouse moved to the cursor's point (the PicoMite's cursor_sync_mouse_to_cursor): the mouse the cursor follows
void CursorSyncMouse(void){
    int m = MouseOpenChannel();
    if(m < 0) m = GuiMouse();
    if(m >= 0){ mousestruct[m].ax = xcursor + xoffcursor; mousestruct[m].ay = ycursor + yoffcursor; }
}
// the cursor's point, shown or not, and the mouse moved to x, y, for GUI CLICK x, y
static void GuiClickAt(int x, int y){
    if(cursoron){ hidecursor(1); showcursor(1, x - xoffcursor, y - yoffcursor); }
    xcursor = x - xoffcursor;
    ycursor = y - yoffcursor;
    CursorSyncMouse();
}

// GUI CLICK PIN pin [, INV] | PIN OFF | DOWN | UP | [x, y], the PicoMite's. The momentary click is PAUSE's re-entry:
// when an interrupt is due (the GUI's, which the click itself may cause) the statement returns to let it run and its
// IRETURN runs the statement again, which carries on from GuiClickPhase. Its down half lasts at least 50mS and until the
// GUI has taken the touch (ProcessTouch reads it when it gets to it); its up half at least 80mS and until the release is
// taken; each at most 500mS.
static void GuiClick(char *p){
    char *clicktoken = CmdTokenPtr;                                 // this statement, to run again after an interrupt
    char *q;
    static long long int until, cap;
    if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
    if((q = checkstring(p, "PIN"))) {                               // (only sets up the pin: an interrupt may use it)
        if(checkstring(q, "OFF")) {
            if(GuiClickPin) {
                ExtCfg(GuiClickPin, EXT_NOT_CONFIG, 0);
                GuiClickPin = GuiClickPinInv = 0;
            }
            return;
        }
        if(GuiClickPin) error("GUI CLICK PIN already set; use GUI CLICK PIN OFF first");
        getargs(&q, 3, ",");
        if(argc != 1 && argc != 3) error("Syntax");
        int pin = GetPinArg(argv[0]), inv = 0;
        if(argc == 3) {
            char *ip = argv[2];                                     // INV is an operator, held as its token (the text
            skipspace(ip);                                          // never matched: the PicoMite's #112)
            if(*ip == GetTokenValue("INV")) {
                ip++;
                skipspace(ip);
                if(*ip == 0) inv = 1;
            }
            if(!inv) error("Syntax");
        }
        CheckPin(pin, CP_CHECKALL);
        ExtCfg(pin, EXT_DIG_IN, inv ? CNPDSET : CNPUSET);           // (active low pulled up; INV active high pulled down)
        GuiClickPinInv = inv;
        GuiClickPin = pin;
        mouseon();
        return;
    }
    if(InterruptReturn != NULL) error("GUI CLICK / DOWN / UP cannot be used in an interrupt routine");
    if(checkstring(p, "DOWN")) {
        mouseon();
        GuiClickDown = 1;
        return;
    }
    if(checkstring(p, "UP")) {
        GuiClickDown = 0;
        return;
    }
    if(GuiClickPhase == 0) {
        skipspace(p);
        if(*p && *p != '\'') {
            getargs(&p, 3, ",");
            if(argc != 3) error("Syntax");
            int x = getint(argv[0], 0, HRes - 1), y = getint(argv[2], 0, VRes - 1);
            mouseon();
            GuiClickAt(x, y);
        } else {
            mouseon();
            CursorSyncMouse();                                          // (the PicoMite's: the mouse goes to the cursor's point)
        }
        GuiClickDown = 1;
        until = mSecTimer + 50;
        cap = mSecTimer + 500;
        GuiClickPhase = 1;
    }
    if(GuiClickPhase == 1) {
        while(mSecTimer < until || ((TouchDown || TouchState == 0) && mSecTimer < cap)) {
            CheckAbort();
            if(check_interrupt()) {
                InterruptReturn = clicktoken;                       // this statement again after the interrupt
                return;
            }
        }
        GuiClickDown = 0;
        until = mSecTimer + 80;
        cap = mSecTimer + 500;
        GuiClickPhase = 2;
    }
    if(GuiClickPhase == 2) {
        while(mSecTimer < until || ((TouchUp || TouchState) && mSecTimer < cap)) {
            CheckAbort();
            if(check_interrupt()) {
                InterruptReturn = clicktoken;
                return;
            }
        }
        GuiClickPhase = 0;
    }
}


// GUI DELETE of a control whose pop-up (keyboard, keypad or list) is open closes it first: otherwise InvokingCtrl names
// a deleted control, which every touch then goes to
static void ClosePopUp(int r) {
    if(r == 0 || r != InvokingCtrl) return;
    if(Ctrl[r].type == CTRL_FMTBOX) DrawFmtBox(KEY_KEY_CANCEL);
    else if(Ctrl[r].type == CTRL_LISTBOX) DrawListBoxPopup(KEY_KEY_CANCEL);
    else DrawKeyboard(KEY_KEY_CANCEL);
}


void cmd_gui(void) {
    int r;
    char *p;
    if((p = checkstring(cmdline, "PAGE"))) {
    	cmd_GUIpage(p);
    	return;
    }

    if((p = checkstring(cmdline, "BUTTON"))) {
        r = GetCtrlParams(CTRL_BUTTON, p);
        return;
    }


    if((p = checkstring(cmdline, "SWITCH"))) {
        r = GetCtrlParams(CTRL_SWITCH, p);
        return;
    }


    if((p = checkstring(cmdline, "CHECKBOX"))) {
        r = GetCtrlParams(CTRL_CHECKBOX, p);
        return;
    }


    if((p = checkstring(cmdline, "RADIO"))) {
        r = GetCtrlParams(CTRL_RADIOBTN, p);

        return;
    }


    if((p = checkstring(cmdline, "LED"))) {
        r = GetCtrlParams(CTRL_LED, p);
        Ctrl[r].inc = 0;
        return;
    }


    if((p = checkstring(cmdline, "FRAME"))) {
        r = GetCtrlParams(CTRL_FRAME, p);
        return;
    }


    if((p = checkstring(cmdline, "NUMBERBOX"))) {
        char *pp;
        if((pp = checkstring(p, "CANCEL"))) {
            if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
            mouseon();
            if(!InvokingCtrl) return;
            DrawKeyboard(KEY_KEY_CANCEL);
        } else
            r = GetCtrlParams(CTRL_NBRBOX, p);
        return;
    }


    if((p = checkstring(cmdline, "TEXTBOX"))) {
        char *pp;
        if((pp = checkstring(p, "CANCEL"))) {
            if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
            mouseon();
            if(!InvokingCtrl) return;
            DrawKeyboard(KEY_KEY_CANCEL);
        } else
            r = GetCtrlParams(CTRL_TEXTBOX, p);
        return;
    }


    if((p = checkstring(cmdline, "FORMATBOX"))) {
        char *pp;
        if((pp = checkstring(p, "CANCEL"))) {
            if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
            mouseon();
            if(!InvokingCtrl) return;
            DrawFmtBox(KEY_KEY_CANCEL);
        } else
            r = GetCtrlParams(CTRL_FMTBOX, p);
        return;
    }


    if((p = checkstring(cmdline, "LISTBOX"))) {
        if(checkstring(p, "CANCEL")) {                              // close an open pop-up, the selection unchanged
            if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
            mouseon();
            if(!InvokingCtrl || Ctrl[InvokingCtrl].type != CTRL_LISTBOX) return;
            DrawListBoxPopup(KEY_KEY_CANCEL);
        } else
            r = GetCtrlParams(CTRL_LISTBOX, p);
        return;
    }


    if((p = checkstring(cmdline, "SLIDER"))) {
        r = GetCtrlParams(CTRL_SLIDER, p);
        return;
    }


    if((p = checkstring(cmdline, "SPINBOX"))) {
        r = GetCtrlParams(CTRL_SPINNER, p);
        Ctrl[r].value = 0;
        if(Ctrl[r].value < Ctrl[r].min) Ctrl[r].value = Ctrl[r].min;
        if(Ctrl[r].value > Ctrl[r].max) Ctrl[r].value = Ctrl[r].max;
        FloatToStr(Ctrl[r].s, Ctrl[r].value, 0, STR_AUTO_PRECISION, ' ');
        UpdateControl(r);                                           // update the displayed string
        return;
    }


    if((p = checkstring(cmdline, "DISPLAYBOX"))) {
        r = GetCtrlParams(CTRL_DISPLAYBOX, p);
        return;
    }


    if((p = checkstring(cmdline, "CAPTION"))) {
        r = GetCtrlParams(CTRL_CAPTION, p);
        return;
    }


    if((p = checkstring(cmdline, "GAUGE"))) {
        r = GetCtrlParams(CTRL_GAUGE, p);
        return;
    }


    if((p = checkstring(cmdline, "BARGAUGE"))) {
        r = GetCtrlParams(CTRL_BARGAUGE, p);
        return;
    }


    if((p = checkstring(cmdline, "AREA"))) {
        r = GetCtrlParams(CTRL_AREA, p);
        return;
    }


    if((p = checkstring(cmdline, "DELETE"))) {
        int i, r;
        getargs(&p, MAX_ARG_COUNT, ",");
        if(!(argc & 1)) error("Argument count");
        if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
        mouseon();
        if(checkstring(argv[0], "ALL")) {
            for(r = 1; r < Option.MaxCtrls; r++)
                if(Ctrl[r].type != 0) {
                    ClosePopUp(r);
                    SetCtrlState(r, CTRL_HIDDEN, true);
                    FreeMemorySafe((void **)&Ctrl[r].s);
                    if(Ctrl[r].fmt) FreeMemorySafe((void **)&Ctrl[r].fmt);
                    memset(&Ctrl[r],0,sizeof(struct s_ctrl));
                }
            return;
        } else {
            for(i = 0; i < argc; i += 2) {
                if(*argv[i] == '#') argv[i]++;
                r = getint(argv[i], 1, Option.MaxCtrls - 1);
                ClosePopUp(r);
                SetCtrlState(r, CTRL_HIDDEN, true);
                FreeMemorySafe((void **)&Ctrl[r].s);
                if(Ctrl[r].fmt) FreeMemorySafe((void **)&Ctrl[r].fmt);
                memset(&Ctrl[r],0,sizeof(struct s_ctrl));
            }
        }
        return;
    }


    if((p = checkstring(cmdline, "DISABLE"))) {
        int i, r;
        getargs(&p, MAX_ARG_COUNT, ",");
        if(!(argc & 1)) error("Argument count");
        if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
        mouseon();
        if(checkstring(argv[0], "ALL")) {
            for(r = 1; r < Option.MaxCtrls; r++)
                if(CurrentPages & (1 << Ctrl[r].page))
                    SetCtrlState(r, CTRL_DISABLED, false);
            return;
        }
        for(i = 0; i < argc; i += 2) {
            if(*argv[i] == '#') argv[i]++;
            r = getint(argv[i], 1, Option.MaxCtrls - 1);
            SetCtrlState(r, CTRL_DISABLED, true);
        }
        return;
    }


    if((p = checkstring(cmdline, "HIDE"))) {
        int i, r;
        getargs(&p, MAX_ARG_COUNT, ",");
        if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
        mouseon();
        if(!(argc & 1)) error("Argument count");
        if(checkstring(argv[0], "ALL")) {
            for(r = 1; r < Option.MaxCtrls; r++)
                if(CurrentPages & (1 << Ctrl[r].page))
                    SetCtrlState(r, CTRL_HIDDEN, false);
            return;
        }
        for(i = 0; i < argc; i += 2) {
            if(*argv[i] == '#') argv[i]++;
            r = getint(argv[i], 1, Option.MaxCtrls - 1);
            SetCtrlState(r, CTRL_HIDDEN, true);
        }
        return;
    }


    if((p = checkstring(cmdline, "ENABLE"))) {
        int i, r;
        getargs(&p, MAX_ARG_COUNT, ",");
        if(!(argc & 1)) error("Argument count");
        if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
        mouseon();
        if(checkstring(argv[0], "ALL")) {
            for(r = 1; r < Option.MaxCtrls; r++) {
                if(CurrentPages & (1 << Ctrl[r].page)) {
                    Ctrl[r].state &= ~(CTRL_DISABLED | CTRL_DISABLED2);
                    SetCtrlState(r, CTRL_NORMAL, false);
                }
            }
            return;
        }
        for(i = 0; i < argc; i += 2) {
            if(*argv[i] == '#') argv[i]++;
            r = getint(argv[i], 1, Option.MaxCtrls - 1);
            Ctrl[r].state &= ~(CTRL_DISABLED | CTRL_DISABLED2);
            SetCtrlState(r, CTRL_NORMAL, true);
        }
        return;
    }


    if((p = checkstring(cmdline, "SHOW"))) {
        int i, r;
        getargs(&p, MAX_ARG_COUNT, ",");
        if(!(argc & 1)) error("Argument count");
        if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
        mouseon();
        if(checkstring(argv[0], "ALL")) {
            for(r = 1; r < Option.MaxCtrls; r++) {
                if(CurrentPages & (1 << Ctrl[r].page)) {
                    Ctrl[r].state &= ~CTRL_HIDDEN;
                    SetCtrlState(r, CTRL_NORMAL, false);
                }
            }
            return;
        }
        for(i = 0; i < argc; i += 2) {
            if(*argv[i] == '#') argv[i]++;
            r = getint(argv[i], 1, Option.MaxCtrls - 1);
            Ctrl[r].state &= ~CTRL_HIDDEN;
            SetCtrlState(r, CTRL_NORMAL, true);
        }
        return;
    }


    if((p = checkstring(cmdline, "RESTORE"))) {
        int i, r;
        getargs(&p, MAX_ARG_COUNT, ",");
        if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
        mouseon();
        if(!(argc & 1)) error("Argument count");
        if(checkstring(argv[0], "ALL")) {
            for(r = 1; r < Option.MaxCtrls; r++) {
                    if(CurrentPages & (1 << Ctrl[r].page)) {
                    Ctrl[r].state &= ~(CTRL_DISABLED | CTRL_DISABLED2 | CTRL_HIDDEN);
                    SetCtrlState(r, CTRL_NORMAL, false);
                }
            }
            return;
        }
        for(i = 0; i < argc; i += 2) {
            if(*argv[i] == '#') argv[i]++;
            r = getint(argv[i], 1, Option.MaxCtrls - 1);
            Ctrl[r].state &= ~(CTRL_DISABLED | CTRL_DISABLED2 | CTRL_HIDDEN);
            SetCtrlState(r, CTRL_NORMAL, true);
        }
        return;
    }


    if((p = checkstring(cmdline, "REDRAW"))) {
        int i, r;
        getargs(&p, MAX_ARG_COUNT, ",");
        if(!(argc & 1)) error("Argument count");
        mouseon();
        if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
        if(checkstring(argv[0], "ALL")) {
            ClearScreen(gui_bcolour);
            for(r = 1; r < Option.MaxCtrls; r++)
                if(CurrentPages & (1 << Ctrl[r].page)) {
                    if(Ctrl[r].type == CTRL_GAUGE) Ctrl[r].y2 = -1; // this will force a full redraw of the gauge
                    UpdateControl(r);
                }
            return;
        }
        for(i = 0; i < argc; i += 2) {
            if(*argv[i] == '#') argv[i]++;
            r = getint(argv[i], 1, Option.MaxCtrls - 1);
            if(Ctrl[r].type == CTRL_GAUGE) Ctrl[r].y2 = -1;         // this will force a full redraw of the gauge
            UpdateControl(r);
        }
        return;
    }


    if((p = checkstring(cmdline, "FCOLOUR")) || (p = checkstring(cmdline, "BCOLOUR"))) {
        int i, r, c;
        getargs(&p, MAX_ARG_COUNT, ",");
        if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
        mouseon();
        if(!(argc & 1) || argc < 3) error("Argument count");
        c = getint(argv[0], BLACK, WHITE);
        for(i = 2; i < argc; i += 2) {
            if(*argv[i] == '#') argv[i]++;
            r = getint(argv[i], 1, Option.MaxCtrls - 1);
            if(Ctrl[r].type == 0) error("Control #% does not exist", r);
            if(checkstring(cmdline, "FCOLOUR"))
                Ctrl[r].fc = c;
            else
                Ctrl[r].bc = c;
            UpdateControl(r);
        }
        return;
    }


/*    if((p = checkstring(cmdline, "BEEP"))) {
        if(Option.TOUCH_Click == 0) error("Click option not set");
        ClickTimer = getint(p, 0, INT_MAX) + 1;
      return;
  }*/


    if((p = checkstring(cmdline, "INTERRUPT"))) {
        getargs(&p, 3, ",");
        mouseon();
//        if(!Option.TOUCH_XZERO && !Option.TOUCH_YZERO) error("Touch not calibrated");
      if(*argv[0] == '0' && !IsDigit(*(argv[0]+1)))
            GuiIntDownVector = GuiIntUpVector = NULL;
        else {
            GuiIntDownVector = GetIntAddress(argv[0]);              // get a pointer to the down interrupt routine
            if(argc == 3)
                GuiIntUpVector = GetIntAddress(argv[2]);            // and for the up routine
            else
                GuiIntUpVector = NULL;
            IntSignal();
        }
        gui_int_down = gui_int_up = false;
      return;
  }


    if((p = checkstring(cmdline, "SETUP"))) {
        if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
        mouseon();
        SetupPage = getint(p, 1, MAX_PAGES) - 1;
    return;
  }

    if((p = checkstring(cmdline, "CLICK"))) {
        GuiClick(p);
        return;
    }
    /* GUI TEST LCDPANEL / GUI TEST TOUCH run until a console keypress. When the
       on-screen keyboard is the only keyboard (OPTION SCREEN KEYBOARD set, no
       external console) there is no key to press - the test pattern owns the
       panel - so the tests would be inescapable. While the OSK is configured a
       double tap (two pen-down edges 100..500 ms apart, landing within 50
       pixels of each other) exits instead (GuiTestDoubleTap, above). */
    /* GUI TEST LCDPANEL | TOUCH, the PicoMite's (its graphics/Draw.c): circles until a key, then how many a second;
       or each touch drawn where it lands (a second finger in cyan) until a key */
    if ((p = checkstring(cmdline, "TEST")))
    {
        if ((checkstring(p, "LCDPANEL")))
        {
            int t, count = 0;
            bool tapdown = false;
            uint64_t taptime = 0;
            uint64_t start = Getuptime();
            t = ((HRes > VRes) ? HRes : VRes) / 7;
            while (getConsole() < '\r')
            {
                if (GuiTestDoubleTap(&tapdown, &taptime))
                    break;
                routinechecks(1);
                DrawCircle(rand() % HRes, rand() % VRes, (rand() % t) + t / 5, 1, 1, rgb((rand() % 8) * 256 / 8, (rand() % 8) * 256 / 8, (rand() % 8) * 256 / 8, 15), 1);
                count++;
            }
            ClearScreen(gui_bcolour);
            PFlt(((MMFLOAT)count) / (((MMFLOAT)(Getuptime() - start)) / 1000000.0));
            MMPrintString(" Circles per Second");
            return;
        }
        if ((checkstring(p, "TOUCH")))
        {
            int x, y, x2, y2;
            bool tapdown = false;
            uint64_t taptime = 0;
            ClearScreen(gui_bcolour);
            while (getConsole() < '\r')
            {
                if (GuiTestDoubleTap(&tapdown, &taptime))
                    break;
                x = GetTouch(GET_X_AXIS);
                y = GetTouch(GET_Y_AXIS);
                if (x != TOUCH_ERROR && y != TOUCH_ERROR)
                    DrawBox(x - 1, y - 1, x + 1, y + 1, 0, WHITE, WHITE);
                /* Second contact, so two-finger panels can be exercised.
                   Sources mirror TOUCH(X2)/TOUCH(Y2): USB multi-touch contact 1.
                   Drawn in a distinct colour so the two fingers are distinguishable. */
                x2 = TOUCH_ERROR;
                y2 = TOUCH_ERROR;
                if (x2 == TOUCH_ERROR && usb_touch_active2)
                {
                    x2 = usb_touch_x2;
                    y2 = usb_touch_y2;
                }
                if (x2 != TOUCH_ERROR && y2 != TOUCH_ERROR)
                    DrawBox(x2 - 1, y2 - 1, x2 + 1, y2 + 1, 0, CYAN, CYAN);
            }
            ClearScreen(gui_bcolour);
            return;
        }
        error("Syntax");
    }

    // the final few commands are common to the MX170 and MX470 so execute them in Draw.c
    cmd_guiMX170();
}




void cmd_GUIpage(char *p) {
    int i, r, OldPages;

    getargs(&p, MAX_ARG_COUNT, ",");
    if(!(argc & 1)) error("Argument count");
    if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
    mouseon();
    OldPages = CurrentPages;
    CurrentPages = 0;
    for(i = 0; i < argc; i += 2) {                                  // get the new set of pages
        if(*argv[i] == '#') argv[i]++;
        CurrentPages |= (1 << (getint(argv[i], 1, MAX_PAGES) - 1));
    }

    // hide any that are showing but not on the new pages
    for(r = 0; r < Option.MaxCtrls; r++) {                          // step thru the controls
        if(Ctrl[r].type != 0) {                                     // if the control is active
            if((!(CurrentPages & (1 << Ctrl[r].page))) && (OldPages & (1 << Ctrl[r].page)) && (!(Ctrl[r].state & CTRL_HIDDEN))) {
                Ctrl[r].state |= CTRL_HIDDEN;
                if(r == CurrentRef) {
                    if(Ctrl[r].type == CTRL_BUTTON) Ctrl[r].value = 0;
                    if(Ctrl[r].type == CTRL_SPINNER) Ctrl[r].state &= ~(CTRL_SPINUP | CTRL_SPINDOWN);
                    LastRef = CurrentRef = 0;
                    gui_int_down = gui_int_up = false;
                }
                DrawControl(r);
                Ctrl[r].state &= ~CTRL_HIDDEN;
            }
        }
    }

    // show any that are on the new pages but not currently showing and are not hidden
    for(r = 0; r < Option.MaxCtrls; r++) {                          // step thru the controls
        if(Ctrl[r].type != 0) {                                     // if the control is active
            if((!(OldPages & (1 << Ctrl[r].page))) && (CurrentPages & (1 << Ctrl[r].page)) && (!(Ctrl[r].state & CTRL_HIDDEN))) {
                DrawControl(r);
            }
        }
    }
}



////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This is the detailed implementation of the GUI controls in the Micromite Plus
//
////////////////////////////////////////////////////////////////////////////////////////////////////////




int ChangeBright(int c, int pct) {
    int r, g, b, t=0xF;
    if(pct == 0 || c <= 0) return c;
    r = (c >> 16) & 0xff;
    g = (c >> 8) & 0xff;
    b = c & 0xff;
    r += (r * pct) / 100; if(r > 255) r = 255; if(r < 0) r = 0;
    g += (g * pct) / 100; if(g > 255) g = 255; if(g < 0) g = 0;
    b += (b * pct) / 100; if(b > 255) b = 255; if(b < 0) b = 0;
    return RGB(r, g, b, t);
}

void SpecialDrawLine(int x1, int y1, int x2, int y2, int w, int tc, int status){
	hidecursor(0);
    if(status & CTRL_HIDDEN) {
        tc = gui_bcolour;
    } else if(status & (CTRL_DISABLED | CTRL_DISABLED2)) {
        tc = ChangeBright(tc, BTN_DISABLED);
    }
    DrawLine(x1, y1, x2, y2, w, tc);
    showcursor(0, xcursor,ycursor);
}


void SpecialDrawBox(int x1, int y1, int x2, int y2, int w, int c, int fill, int status) {
	hidecursor(0);
    if(status & CTRL_HIDDEN) {
        c = gui_bcolour;
        fill = gui_bcolour;
    } else if(status & (CTRL_DISABLED | CTRL_DISABLED2)) {
        c = ChangeBright(c, BTN_DISABLED);
        if(fill != gui_bcolour) fill = ChangeBright(fill, BTN_DISABLED);
    }
    DrawBox(x1, y1, x2, y2, w, c, fill);
    showcursor(0, xcursor,ycursor);
}


void SpecialDrawRBox(int x1, int y1, int x2, int y2, int radius, int c, int fill, int status) {
	hidecursor(0);
    if(status & CTRL_HIDDEN) {
        c = gui_bcolour;
        fill = gui_bcolour;
    } else if(status & (CTRL_DISABLED | CTRL_DISABLED2)) {
        c = ChangeBright(c, BTN_DISABLED);
        if(fill != gui_bcolour) fill = ChangeBright(fill, BTN_DISABLED);
    }
    DrawRBox(x1, y1, x2, y2, radius, c, fill);
    showcursor(0, xcursor,ycursor);
}


void SpecialPrintString(int x, int y, int jh, int jv, int jo, int fc, int bc, char *str, int status) {
    int lines, i, y2;
    char t, *p, *idx[MAX_CAPTION_LINES + 1];

    // first check if the string contains one or more line split chars ('~')
    // while we are doing this save their addresses in idx[]
    idx[0] = p = str;
    lines = 1;
    while((p = strchr(p, '~')) != NULL && lines < MAX_CAPTION_LINES) {
        idx[lines] = p++;
        lines++;
    }

    if(lines > 1) {
        // this is two or more lines
        // recursively call this function to print each line
        idx[lines] = str + strlen(str);
        y2 = y - (gui_font_height / 2) * (lines - 1);
        for(i = 0; i < lines; i++) {
            t = *idx[i + 1];
            *idx[i + 1] = 0;
            SpecialPrintString(x, y2, jh, jv, jo, fc, bc, idx[i], status);
            *idx[i + 1] = t;
            if(t == '~') idx[i + 1]++;
            y2 += gui_font_height;
        }
    } else {
        // this is just a single line, so print it
        if(status & CTRL_HIDDEN) {
            fc = gui_bcolour;
            bc = gui_bcolour;
        } else if(status & (CTRL_DISABLED | CTRL_DISABLED2)) {
            fc = ChangeBright(fc, BTN_DISABLED);
            if(bc != gui_bcolour) bc = ChangeBright(bc, BTN_DISABLED);
        }
        GUIPrintString(x, y, gui_font, jh, jv, jo, fc, bc, str);
    }
}



void SpecialDrawCircle(int x, int y, int radius, int w, int c, int fill, MMFLOAT aspect, int status) {
	hidecursor(0);
    if(status & CTRL_HIDDEN) {
        c = gui_bcolour;
        fill = gui_bcolour;
    } else if(status & (CTRL_DISABLED | CTRL_DISABLED2)) {
        c = ChangeBright(c, BTN_DISABLED);
        if(fill != gui_bcolour) fill = ChangeBright(fill, BTN_DISABLED);
    }
    DrawCircle(x, y, radius, w, c, fill, aspect);
    showcursor(0, xcursor,ycursor);
}



void SpecialDrawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int c, int fill, int status) {
	hidecursor(0);
    if(status & CTRL_HIDDEN) {
        c = gui_bcolour;
        fill = gui_bcolour;
    } else if(status & (CTRL_DISABLED | CTRL_DISABLED2)) {
        c = ChangeBright(c, BTN_DISABLED);
        if(fill != gui_bcolour) fill = ChangeBright(fill, BTN_DISABLED);
    }
    DrawTriangle(x0, y0, x1, y1, x2, y2, c, fill);
    showcursor(0, xcursor,ycursor);
}



void DrawBorder(int x1, int y1, int x2, int y2, int w, int tc, int bc, int status) {
    int i;
    for(i = 0; i < w; i++) {
        SpecialDrawLine(x1 + i, y1 + i, x2 - i, y1 + i, 1, tc, status);     // top border
        SpecialDrawLine(x1 + i, y1 + i, x1 + i, y2 - i, 1, tc, status);     // left border
        SpecialDrawLine(x1 + i, y2 - i, x2 - i, y2 - i, 1, bc, status);     // bottom
        SpecialDrawLine(x2 - i, y1 + i, x2 - i, y2 - i, 1, bc, status);     // and right
    }
}



void DrawBasicButton(int x1, int y1, int x2, int y2, int w, int up, int c, int status) {
    DrawBorder(x1, y1, x2, y2, w, ChangeBright(c, up ? BTN_SIDE_BRIGHT:BTN_SIDE_DULL), ChangeBright(c, BTN_SIDE_DULL), status);
    SpecialDrawBox(x1 + w, y1 + w, x2 - w, y2 - w, 0, 0, c, status);    // fill in the face of the button
}



void DrawButton(int r) {
    char *p, *pp, s[MAXSTRLEN];
    int bs = 0;

    DrawBasicButton(Ctrl[r].x1, Ctrl[r].y1, Ctrl[r].x2, Ctrl[r].y2, BTN_SIDE_WIDTH, (Ctrl[r].value == 0), Ctrl[r].bc, Ctrl[r].state);

    // extract the up/down strings if necessary
    for(p = Ctrl[r].s, pp = s; *p != '|';) {
        if(*p == 0) {
            p = s - 1;
            bs = BTN_CAPTION_SHIFT;
            break;
        }
        *pp++ = *p++;
    }
    p++;
    *pp = 0;

    // draw the caption
    if(Ctrl[r].value == 0)
        SpecialPrintString(Ctrl[r].x1 + (Ctrl[r].x2 - Ctrl[r].x1)/2, Ctrl[r].y1 + (Ctrl[r].y2 - Ctrl[r].y1)/2, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, Ctrl[r].fc, Ctrl[r].bc, s, Ctrl[r].state);
    else
        SpecialPrintString(Ctrl[r].x1 + (Ctrl[r].x2 - Ctrl[r].x1)/2 + bs, Ctrl[r].y1 + (Ctrl[r].y2 - Ctrl[r].y1)/2 + bs, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, Ctrl[r].fc, Ctrl[r].bc, p, Ctrl[r].state);
}



void DrawSwitch(int r) {
    char *p, *pp, s[MAXSTRLEN];
    int half, on, twobtn, shift;

    // extract the up/down strings if necessary
    twobtn = true;
    for(p = Ctrl[r].s, pp = s; *p != '|';) {
        if(*p == 0) {
            p = s - 1;
            twobtn = false;
            break;
        }
        *pp++ = *p++;
    }
    p++;
    *pp = 0;

    on = (Ctrl[r].value == 0);
    if(on) shift = 0; else shift = BTN_CAPTION_SHIFT;
    if(twobtn) {
        half = Ctrl[r].x1 + (Ctrl[r].x2 - Ctrl[r].x1)/2;
        if(!(on)) {                                                    // use the min, max elements to store the active x area of the button
            Ctrl[r].min = half;
            Ctrl[r].max = Ctrl[r].x2;
        } else {
            Ctrl[r].min = Ctrl[r].x1;
            Ctrl[r].max = half;
        }          
    } else {
        Ctrl[r].min = Ctrl[r].x1;
        Ctrl[r].max = half = Ctrl[r].x2;
    }
    
    DrawBasicButton(Ctrl[r].x1, Ctrl[r].y1, half, Ctrl[r].y2, BTN_SIDE_WIDTH, on, ChangeBright(Ctrl[r].bc, on ? 0 : -25), Ctrl[r].state);
    if(twobtn) DrawBasicButton(half, Ctrl[r].y1, Ctrl[r].x2, Ctrl[r].y2, BTN_SIDE_WIDTH, !on, ChangeBright(Ctrl[r].bc, on ? -25 : 0), Ctrl[r].state);

    // draw the captions
    if(on) shift = 0; else shift = BTN_CAPTION_SHIFT;
    SpecialPrintString(Ctrl[r].x1 + (half - Ctrl[r].x1)/2 + shift, Ctrl[r].y1 + (Ctrl[r].y2 - Ctrl[r].y1)/2 + shift,      JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, ChangeBright(Ctrl[r].fc, (on || !twobtn) ? 0:-25), ChangeBright(Ctrl[r].bc, on ? 0:-25), s, Ctrl[r].state);
    if(!on) shift = 0; else shift = BTN_CAPTION_SHIFT;
    if(twobtn) SpecialPrintString(half + (Ctrl[r].x2 - half)/2 + shift, Ctrl[r].y1 + (Ctrl[r].y2 - Ctrl[r].y1)/2 + shift, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, ChangeBright(Ctrl[r].fc, on ? -25:0),  ChangeBright(Ctrl[r].bc, on ? -25:0),  p, Ctrl[r].state);
}



// for the checkbox the width/height is saved in Ctrl[r].inc
void DrawCheckBox(int r) {
    int i, w;

    SpecialDrawBox(Ctrl[r].x1, Ctrl[r].y1, Ctrl[r].x1 + Ctrl[r].inc, Ctrl[r].y1 + Ctrl[r].inc, BTN_SIDE_WIDTH, ChangeBright(Ctrl[r].fc, -30), gui_bcolour, Ctrl[r].state) ;
    SpecialPrintString(Ctrl[r].x1 + Ctrl[r].inc + Ctrl[r].inc/2, Ctrl[r].y1 + Ctrl[r].inc/2, JUSTIFY_LEFT, JUSTIFY_MIDDLE, ORIENT_NORMAL, Ctrl[r].fcc, gui_bcolour, Ctrl[r].s, Ctrl[r].state);

    // draw the tick
    if(Ctrl[r].value != 0) {
        w = (Ctrl[r].inc / 28) + 1;                                     // vary the tick size according to the box size
        for(i = -w; i <= w; i++) {
            SpecialDrawLine(Ctrl[r].x1 + i + BTN_SIDE_WIDTH*2, Ctrl[r].y1 + BTN_SIDE_WIDTH*2, Ctrl[r].x1 + Ctrl[r].inc + i - BTN_SIDE_WIDTH*2, Ctrl[r].y2 - BTN_SIDE_WIDTH*2, 1, Ctrl[r].fc, Ctrl[r].state);
            SpecialDrawLine(Ctrl[r].x1 + i + BTN_SIDE_WIDTH*2, Ctrl[r].y2 - BTN_SIDE_WIDTH*2, Ctrl[r].x1 + Ctrl[r].inc + i - BTN_SIDE_WIDTH*2, Ctrl[r].y1 + BTN_SIDE_WIDTH*2, 1, Ctrl[r].fc, Ctrl[r].state);
        }
    }
}



// the radio button is a special case, its touch sensitive area is different from its drawing parameters
// the touch sensitive area is stored in x1, y1 to x2, y2
// the buttons radius is stored in Ctrl[r].max and the X and Y centre of the button (xc and yc) are calculated using that
void DrawRadioBtn(int r) {
    int i, frame;
    int xc = Ctrl[r].x1 + Ctrl[r].max;
    int yc = Ctrl[r].y1 + Ctrl[r].max;

    SpecialDrawCircle(xc, yc, Ctrl[r].max, BTN_SIDE_WIDTH, ChangeBright(Ctrl[r].fc, -30), gui_bcolour, 1.0, Ctrl[r].state);
    SpecialPrintString(xc + Ctrl[r].max + gui_font_width - (gui_font_width/4), yc, JUSTIFY_LEFT, JUSTIFY_MIDDLE, ORIENT_NORMAL, Ctrl[r].fcc, gui_bcolour, Ctrl[r].s, Ctrl[r].state);

    if(Ctrl[r].value != 0) {
        // draw the button if this control has been selected
        SpecialDrawCircle(xc, yc, Ctrl[r].max - ((BTN_SIDE_WIDTH * 3) / 2), 0, 0, Ctrl[r].fc, 1.0, Ctrl[r].state);

        // make sure that all the other radio buttons are in the up state
        // first find the frame (if any) that the current button is in
        for(frame = 1; frame < Option.MaxCtrls; frame++)
            if(Ctrl[frame].type == CTRL_FRAME && Ctrl[r].page == Ctrl[frame].page && xc > Ctrl[frame].x1 && xc < Ctrl[frame].x2 && yc > Ctrl[frame].y1 && yc < Ctrl[frame].y2)
                break;
        // next look for any other radio buttons that are not the current one and is down and is in the same frame... and set it up
        for(i = 1; i < Option.MaxCtrls; i++) {
            if(Ctrl[i].type == CTRL_RADIOBTN && Ctrl[i].page == Ctrl[r].page && i != r && Ctrl[r].value != 0) {
                // if frame == MAX_CTRL this means that there is no frame and the whole screen can be considered the frame
                if(frame == Option.MaxCtrls || (Ctrl[i].x1 + Ctrl[i].max > Ctrl[frame].x1 && Ctrl[i].x1 + Ctrl[i].max < Ctrl[frame].x2 && Ctrl[i].y1 + Ctrl[i].max > Ctrl[frame].y1 && Ctrl[i].y1 + Ctrl[i].max < Ctrl[frame].y2)) {
                    Ctrl[i].value = 0;
                    UpdateControl(i);
                }
            }
        }
    }
}



// the LED is a special case, its touch sensitive area is different from its drawing parameters
// the touch sensitive area is stored in x1, y1 to x2, y2
// the LED's radius is stored in Ctrl[r].max and the X and Y centre of the button (xc and yc) are calculated using that
void DrawLED(int r) {
    int xc = Ctrl[r].x1 + Ctrl[r].max;
    int yc = Ctrl[r].y1 + Ctrl[r].max;

    SpecialDrawCircle(xc, yc, Ctrl[r].max, 0, 0, RGB(160,160,160,0xF), 1.0, Ctrl[r].state);
    SpecialDrawCircle(xc, yc, Ctrl[r].max - BTN_SIDE_WIDTH/2, 0, 0, ChangeBright(Ctrl[r].fc, (Ctrl[r].value == 0) ? -65:0), 1.0, Ctrl[r].state);
    SpecialPrintString(xc + Ctrl[r].max + gui_font_width - (gui_font_width/4), yc, JUSTIFY_LEFT, JUSTIFY_MIDDLE, ORIENT_NORMAL, Ctrl[r].fcc, gui_bcolour, Ctrl[r].s, Ctrl[r].state);
}


void DrawSpinner(int r) {
    int x1, x2, y1, y2;
    int h, z;

    h = (Ctrl[r].y2 - Ctrl[r].y1);
    z = h/6;
    x1 = Ctrl[r].x1; x2 = Ctrl[r].x2;
    y1 = Ctrl[r].y1; y2 = Ctrl[r].y2;
    SpecialDrawRBox(x1 + h, y1, x2 - h, y2, 10,  Ctrl[r].fc, Ctrl[r].bc, Ctrl[r].state);
    FloatToStr(Ctrl[r].s, Ctrl[r].value, 0, STR_AUTO_PRECISION, ' ');               // convert the value to a string for display
    Ctrl[r].s[(x2 - x1 - h * 2) / gui_font_width] = 0;              // truncate to the display width
    SpecialPrintString(x1 + (x2 - x1)/2, y1 + h/2, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, Ctrl[r].fc, Ctrl[r].bc, Ctrl[r].s, Ctrl[r].state);
    if(Ctrl[r].state & CTRL_SPINUP)
        SpecialDrawTriangle(x2 - h/2, y1 + z, x2 - z, y1 + h - z, x2 - h + z, y1 + h - z, Ctrl[r].bc, Ctrl[r].fc, Ctrl[r].state);  // touched up pointing spin button
    else
        SpecialDrawTriangle(x2 - h/2, y1 + z, x2 - z, y1 + h - z, x2 - h + z, y1 + h - z, Ctrl[r].fc, Ctrl[r].bc, Ctrl[r].state);  // up pointing spin button
    if(Ctrl[r].state & CTRL_SPINDOWN)
        SpecialDrawTriangle(x1 + h/2, y1 + h - z, x1 + z, y1 + z, x1 + h - z, y1 + z, Ctrl[r].bc, Ctrl[r].fc, Ctrl[r].state);      // touched down pointing spin button
    else
        SpecialDrawTriangle(x1 + h/2, y1 + h - z, x1 + z, y1 + z, x1 + h - z, y1 + z, Ctrl[r].fc, Ctrl[r].bc, Ctrl[r].state);      // down pointing spin button
}



void DrawFrame(int r) {
    SpecialDrawRBox(Ctrl[r].x1, Ctrl[r].y1, Ctrl[r].x2, Ctrl[r].y2, 10, Ctrl[r].fc, -1, Ctrl[r].state);     // the border
    SpecialPrintString(Ctrl[r].x1 + 15, Ctrl[r].y1, JUSTIFY_LEFT, JUSTIFY_MIDDLE, ORIENT_NORMAL, Ctrl[r].fc, gui_bcolour, Ctrl[r].s, Ctrl[r].state);
}


void DrawCaption(int r) {
    SpecialPrintString(Ctrl[r].x1, Ctrl[r].y1, Ctrl[r].x2 & 0b0000011, (Ctrl[r].x2 >> 2) & 0b0000011, (Ctrl[r].x2 >> 4) & 0b0000111, Ctrl[r].fc, Ctrl[r].bc, Ctrl[r].s, Ctrl[r].state);
}



void DrawGauge(int r) {
    int x, y, c;
    MMFLOAT radius;
    int xo1, yo1, xo2, yo2, xi1, yi1, xi2, yi2;
    int i, v1, v2, v, ta, tb, tc;
    char buf[GAUGE_UNITS_SIZE * 2];
    struct s_GaugeS *GaugeS;                                        // we store extra info in the string allocated to this control


    // these define the span of the gauge in degrees
    const int gstart = -225;
    const int gend = 45;

    // this defines the width of the gauge as a percentage of the radius
    const MMFLOAT width = 0.25;

    // this is the number of degrees resolution
    // smaller nbr gives a cleaner graph but is slower
    const int stepsize = 3;

    x = Ctrl[r].x1; y = Ctrl[r].y1; radius = Ctrl[r].x2;
    GaugeS = (void *)Ctrl[r].s;
    c = Ctrl[r].fc;

    // scale the value of the gauge and the thresholds to degrees within the span of the gauge
    v = (((Ctrl[r].value - Ctrl[r].min) / (Ctrl[r].max - Ctrl[r].min)) * (gend - gstart)) + gstart;
    ta = (((GaugeS->ta - Ctrl[r].min) / (Ctrl[r].max - Ctrl[r].min)) * (gend - gstart)) + gstart;
    tb = (((GaugeS->tb - Ctrl[r].min) / (Ctrl[r].max - Ctrl[r].min)) * (gend - gstart)) + gstart;
    tc = (((GaugeS->tc - Ctrl[r].min) / (Ctrl[r].max - Ctrl[r].min)) * (gend - gstart)) + gstart;

    // make sure that the value and the thresholds are within the range
    if(v < gstart) v = gstart;
    if(v > gend) v = gend;
    if(ta <= gstart) ta = gstart + 1;
    if(tb <= ta + 1) tb = ta + 2;
    if(tc <= tb + 1) tc = tb + 2;
    if(ta > gend) ta = gend;
    if(tb > gend) tb = gend;
    if(tc > gend) tc = gend;

    // set v1 and v2 so that we are drawing only the part that changed while making sure v1 < v2
    if(v > GaugeS->cval) {
      v1 = GaugeS->cval; v2 = v;
    } else {
      v2 = GaugeS->cval; v1 = v;
    }

    if(Ctrl[r].y2 != Ctrl[r].state || Ctrl[r].fc != GaugeS->lastfc || Ctrl[r].bc != GaugeS->lastbc ) {
        // if the state of this control has changed we then need to redraw the complete gauge rather than just update the content of the gauge graph
        v1 = gstart;
        v2 = gend;
        GaugeS->laststrlen = GAUGE_UNITS_SIZE;
        Ctrl[r].y2 = Ctrl[r].state;
        GaugeS->cval = gend + 1;
        GaugeS->lastfc = Ctrl[r].fc; GaugeS->lastbc = Ctrl[r].bc;
    }

    if(v != GaugeS->cval) {  // only draw the gauge part if something has changed
        // draw the gauge
        // this only draws the part changed since the previous call
        for(i = v1; i <= v2; i = i+stepsize) {
            xo1 = x + (cosf(Rad(i)) * radius);
            yo1 = y + (sinf(Rad(i)) * radius);
            xo2 = x + (cosf(Rad(i+stepsize)) * radius);
            yo2 = y + (sinf(Rad(i+stepsize)) * radius);
            xi1 = x + (cosf(Rad(i)) * (radius * (1 - width)));
            yi1 = y + (sinf(Rad(i)) * (radius * (1 - width)));
            xi2 = x + (cosf(Rad(i+stepsize)) * (radius * (1 - width)));
            yi2 = y + (sinf(Rad(i+stepsize)) * (radius * (1 - width)));

            c = Ctrl[r].fc;
            if(GaugeS->c1 >= 0) c = GaugeS->c1;
            if(i > ta) c = GaugeS->c2;
            if(i > tb) c = GaugeS->c3;
            if(i > tc) c = GaugeS->c4;

            if(v > i || v == gend) {
                // drawing a filled in part of the gauge
                SpecialDrawTriangle(xo1, yo1, xo2, yo2, xi1, yi1, c, c, Ctrl[r].state);
                SpecialDrawTriangle(xo2, yo2, xi1, yi1, xi2, yi2, c, c, Ctrl[r].state);
            } else {
                GaugeS->csaved = c;
                // erasing part of the gauge
                c = ChangeBright(c, -70);
                if(GaugeS->c1 < 0 || ta >= gend) {
                    c = Ctrl[r].bc;
                    GaugeS->csaved = Ctrl[r].fc;
                }
                SpecialDrawTriangle(xo1, yo1, xo2, yo2, xi1, yi1, c, c, Ctrl[r].state);
                SpecialDrawTriangle(xo2, yo2, xi1, yi1, xi2, yi2, c, c, Ctrl[r].state);
            }
            // draw the outside frame
            SpecialDrawLine(xo1, yo1, xo2, yo2, 1, Ctrl[r].fc, Ctrl[r].state);
            SpecialDrawLine(xi1, yi1, xi2, yi2, 1, Ctrl[r].fc, Ctrl[r].state);

            // draw the end stops
            if(i == gstart) SpecialDrawLine(xi1, yi1, xo1, yo1, 1, Ctrl[r].fc, Ctrl[r].state);
            if(i == gend) SpecialDrawLine(xi2, yi2, xo2, yo2, 1, Ctrl[r].fc, Ctrl[r].state);
        }
    }
    
    FloatToStr(buf, Ctrl[r].value, 0, Ctrl[r].inc, ' ');            // convert the value to a string for display
    if(strlen(buf) < GaugeS->laststrlen) {                          // erase the last text if the length of the text has shrunk
        SpecialDrawCircle(x, y, (radius * 0.75) - 2, 0, Ctrl[r].bc, Ctrl[r].bc, 1.0, Ctrl[r].state) ;
    }
    GaugeS->laststrlen = strlen(buf);

    // get the colour of the text for the current value
    c = Ctrl[r].fc;
    if(GaugeS->c1 >= 0) c = GaugeS->c1;
    if(v > ta) c = GaugeS->c2;
    if(v > tb) c = GaugeS->c3;
    if(v > tc) c = GaugeS->c4;
    
    // draw the text
    if(*Ctrl[r].s) { strcat(buf, "~"); strcat(buf, Ctrl[r].s); }    // append the units string
    SpecialPrintString(x, y, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, c, Ctrl[r].bc, buf, Ctrl[r].state);

    GaugeS->cval = v;
}



void DrawBarGauge(int r) {
    int vert, x, y, x2 = 0, y2 = 0, start, end, len;
    int v, ta, tb, tc;
    struct s_GaugeS *GaugeS;                                        // we store extra info in the string allocated to this control

    if(Ctrl[r].x2 < Ctrl[r].y2) {
        // vertical bar
        // x, y refer to the bottom left of the bar (not the outline)
        // x2 is the width of the bar and len the overall length
        vert = true;
        x = Ctrl[r].x1 + 1;
        y = Ctrl[r].y1 + Ctrl[r].y2 - 1;
        x2 = Ctrl[r].x2 + x - 2;
        len = Ctrl[r].y2 - 2;
    } else {
        // horizontal bar
        // x, y refer to the top left coord of the bar
        // x2 is the width of the bar and len the overall length
        vert = false;
        x = Ctrl[r].x1 + 1;
        y = Ctrl[r].y1 + 1;
        y2 = Ctrl[r].y2 + y - 2;
        len = Ctrl[r].x2 - 2;
    }

    GaugeS = (void *)Ctrl[r].s;

    // scale the value of the gauge and the thresholds to pixels within the span of the gauge
    v = ((Ctrl[r].value - Ctrl[r].min) / (Ctrl[r].max - Ctrl[r].min)) * (MMFLOAT)len;
    ta = ((GaugeS->ta - Ctrl[r].min) / (Ctrl[r].max - Ctrl[r].min)) * (MMFLOAT)len;
    tb = ((GaugeS->tb - Ctrl[r].min) / (Ctrl[r].max - Ctrl[r].min)) * (MMFLOAT)len;
    tc = ((GaugeS->tc - Ctrl[r].min) / (Ctrl[r].max - Ctrl[r].min)) * (MMFLOAT)len;

    // make sure that the value and the thresholds are within the range
    if(v < 0) v = 0;
    if(v > len) v = len;
    if(ta <= 0) ta = 1;
    if(tb <= ta + 1) tb = ta + 2;
    if(tc <= tb + 1) tc = tb + 2;
    if(ta > len) ta = len;
    if(tb > len) tb = len;
    if(tc > len) tc = len;

    SpecialDrawLine(Ctrl[r].x1, Ctrl[r].y1, Ctrl[r].x1 + Ctrl[r].x2, Ctrl[r].y1, 1, Ctrl[r].fc, Ctrl[r].state);    // draw the top of the outline
    SpecialDrawLine(Ctrl[r].x1, Ctrl[r].y1, Ctrl[r].x1, Ctrl[r].y1 + Ctrl[r].y2, 1, Ctrl[r].fc, Ctrl[r].state);    // draw the left side of the outline
    SpecialDrawLine(Ctrl[r].x1, Ctrl[r].y1 + Ctrl[r].y2, Ctrl[r].x1 + Ctrl[r].x2, Ctrl[r].y1 + Ctrl[r].y2, 1, Ctrl[r].fc, Ctrl[r].state);    // draw the botton of the outline
    SpecialDrawLine(Ctrl[r].x1 + Ctrl[r].x2, Ctrl[r].y1, Ctrl[r].x1 + Ctrl[r].x2, Ctrl[r].y1 + Ctrl[r].y2, 1, Ctrl[r].fc, Ctrl[r].state);    // draw the right side of the outline

    if(vert) {
        if(GaugeS->c1 < 0) GaugeS->c1 = Ctrl[r].fc;
        // draw the first bar
        if(v > ta) end = ta; else end = v;
        if(end > 0) {
            SpecialDrawBox(x, y, x2, y - end, 0, GaugeS->c1, GaugeS->c1, Ctrl[r].state);
            // draw the second bar
            start = end + 1; if(v > tb) end = tb; else end = v;
            if(end > start) {
                SpecialDrawBox(x, y - start, x2, y - end, 0, GaugeS->c2, GaugeS->c2, Ctrl[r].state);
                // draw the third bar
                start = end + 1; if(v > tc) end = tc; else end = v;
                if(end > start) {
                    SpecialDrawBox(x, y - start, x2, y - end, 0, GaugeS->c3, GaugeS->c3, Ctrl[r].state);
                    // finally draw the fourth bar
                    start = end + 1; if(v > tc) end = v;
                    if(end > start) {
                        SpecialDrawBox(x, y - start, x2, y - end, 0, GaugeS->c4, GaugeS->c4, Ctrl[r].state);
                    }
                }
            }
        }
        // erase the remaining part of the bar
        if(end < len) SpecialDrawBox(x, y - end, x2, y - len, 0, Ctrl[r].bc, Ctrl[r].bc, Ctrl[r].state);
    } else {
        if(GaugeS->c1 < 0) GaugeS->c1 = Ctrl[r].fc;
        // draw the first bar
        if(v > ta) end = ta; else end = v;
        if(end > 0) {
            SpecialDrawBox(x, y, x + end, y2, 0, GaugeS->c1, GaugeS->c1, Ctrl[r].state);
            // draw the second bar
            start = end + 1; if(v > tb) end = tb; else end = v;
            if(end > start) {
                SpecialDrawBox(x + start, y, x + end, y2, 0, GaugeS->c2, GaugeS->c2, Ctrl[r].state);
                // draw the third bar
                start = end + 1; if(v > tc) end = tc; else end = v;
                if(end > start) {
                    SpecialDrawBox(x + start, y, x + end, y2, 0, GaugeS->c3, GaugeS->c3, Ctrl[r].state);
                    // finally draw the fourth bar
                    start = end + 1; if(v > tc) end = v;
                    if(end > start) {
                        SpecialDrawBox(x + start, y, x + end, y2, 0, GaugeS->c4, GaugeS->c4, Ctrl[r].state);
                    }
                }
            }
        }
        // erase the remaining part of the bar
        if(end < len) SpecialDrawBox(x + end, y, x + len, y2, 0, Ctrl[r].bc, Ctrl[r].bc, Ctrl[r].state);
    }
}



char *KeypadCaption[12][2] = {  {"Alt", "123"},
                                {"0", "Can"},
                                {"Ent", "Ent"},
                                {"1", "."},
                                {"2", ""},
                                {"3", ""},
                                {"4", "+"},
                                {"5", "-"},
                                {"6", "E"},
                                {"7", "Del"},
                                {"8", "CE"},
                                {"9", ""}
                              };


char *KeyboardCaption[33][4] = {
                                // this is the bottom row of keys
                                {"&12", "ABC", "&12", "ABC"},
                                {"Z", "+" , "z", "["},
                                {"X", "-", "x" , "]"},
                                {"C", "=", "c", "{" },
                                {"V", "?", "v", "}" },
                                {"B", ":", "b", "|" },
                                {"N", "/", "n", "\\" },
                                {"M", ",", "m", "," },
                                {".", ".", ".", "." },
                                {"[ ]", "[ ]", "[ ]", "[ ]" },
                                {"Ent", "Ent", "Ent", "Ent" },

                                // this is the middle row
                                {"Can", "Can", "Can", "Can" },
                                {"A", "!", "a", "_" },
                                {"S", "@", "s", "/" },
                                {"D", "#", "d", "<" },
                                {"F", "$", "f", ">" },
                                {"G", "%", "g", ":" },
                                {"H", "&", "h", ";" },
                                {"J", "*", "j", "^" },
                                {"K", "(", "k", "'" },
                                {"L", ")", "l", "-" },
                                {" ", " ", " ", " " },

                                // and this is the top row
                                {"Q", "1", "q", "1" },
                                {"W", "2", "w", "2" },
                                {"E", "3", "e", "3" },
                                {"R", "4", "r", "4" },
                                {"T", "5", "t", "5" },
                                {"Y", "6", "y", "6" },
                                {"U", "7", "u", "7" },
                                {"I", "8", "i", "8" },
                                {"O", "9", "o", "9" },
                                {"P", "0", "p", "0" },
                                {"Del", "Del", "Del", "Del" }
                              };



char *GetCaption(int k, int is_alpha, int alt) {
    if(is_alpha)
        return KeyboardCaption[k][alt];
    else
        return KeypadCaption[k][alt];
}


// returns the top left coord of a key (x1 & y1) and the bottom right (x2 & y2)
// these must be passed as pointers to ints
// k is the key number
// is_alpha is true if keypad is the text keyboard
void GetSingleKeyCoord(int k, int is_alpha, int *x1, int *y1, int *x2, int *y2) {
    int horiznbr, width, height, keygap, BaseHoriz, BaseVert;

    if(is_alpha) {                                                  // if this is the keyboard
        horiznbr = 11;                                              // number of horizontal buttons
        if(HRes == 800) {
            width = 69;
            height = 77;
            keygap = 4;
        } else if(HRes == 480) {
            width = 40;
            height = 41;
            keygap = 4;
        } else {
            keygap = 2;
            width = (HRes - keygap*10)/11;
            height = ((VRes/2) - keygap*2) / 3;
        }
        BaseVert = (Ctrl[InvokingCtrl].y2 > VRes/2) ? (height * 3) + (keygap * 3) : VRes + keygap;  // what half of the screen?
        BaseHoriz = HRes + keygap/2;
    } else {                                                        // if this is the number pad
        horiznbr = 3;                                               // number of horizontal buttons
        keygap = 5;
        height = VRes < HRes ? VRes : HRes;                         // find the smallest resolution
        height = width = ((height - keygap*3)/4 < 80) ? (height - keygap*3)/4 : 80;               // scale the button size to the smallest resolution
        if(Ctrl[InvokingCtrl].y1 + height/2 > VRes/2) {
            BaseVert = (width * 4) + (keygap * 4);                  // should it be in the top part of the screen?
        } else {
            BaseVert = VRes + keygap;                               // or in the bottom
        }
        if(Ctrl[InvokingCtrl].x1 + width/2 > HRes/2) {
            BaseHoriz = (width * 3) + (keygap * 3);
        } else {
            BaseHoriz = HRes + keygap;
        }
    }

    *x1 = BaseHoriz - (horiznbr - (k % horiznbr)) * (width + keygap);
    *y1 = BaseVert - ((k / horiznbr) + 1) * (height + keygap);
    *x2 = *x1 + width;
    *y2 = *y1 + height;
}


void DrawDisplayBox(int r) {
    int fc, bc;
    if(Ctrl[r].state & CTRL_SELECTED) {
        fc = Ctrl[r].bc;
        bc = Ctrl[r].fc;
    } else {
        fc = Ctrl[r].fc;
        bc = Ctrl[r].bc;
    }
    SpecialDrawRBox(Ctrl[r].x1, Ctrl[r].y1, Ctrl[r].x2, Ctrl[r].y2, 10,  fc, bc, Ctrl[r].state);
    if(strlen(Ctrl[r].s) > 2 && Ctrl[r].s[0] == '#' && Ctrl[r].s[1] == '#')
        SpecialPrintString(Ctrl[r].x1 + (Ctrl[r].x2 - Ctrl[r].x1)/2, Ctrl[r].y1 + (Ctrl[r].y2 - Ctrl[r].y1)/2, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, ChangeBright(fc, BTN_DISABLED), bc, Ctrl[r].s + 2, Ctrl[r].state);
    else
        SpecialPrintString(Ctrl[r].x1 + (Ctrl[r].x2 - Ctrl[r].x1)/2, Ctrl[r].y1 + (Ctrl[r].y2 - Ctrl[r].y1)/2, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, fc, bc, Ctrl[r].s , Ctrl[r].state);
}


/*********************************************************************************************
 The LISTBOX control (the PicoMite's)
 The control shows the selected item with a drop-down marker. Touching it opens a modal pop-up of the bound array's
 items, a window of them with scroll arrows when they do not all fit. The pop-up is the keyboard's pattern: InvokingCtrl
 takes every touch, DelayedDrawListbox draws it after the pen-down interrupt, and closing it erases it to the background
 and redraws every control (PopUpRedrawAll). The selected index is Ctrl[r].value.
 ********************************************************************************************/
#define LB_HIT_OUTSIDE      (-1)
#define LB_HIT_UPARROW      (-2)
#define LB_HIT_DOWNARROW    (-3)

// item idx of the bound array as a C string (an element is its length then its characters)
static void ListBoxGetItem(struct s_ListBoxS *lb, int idx, char *buf, int buflen) {
    buf[0] = 0;
    if(idx < 0 || idx >= lb->count || lb->items == NULL) return;
    unsigned char *e = lb->items + (size_t)idx * lb->stride;
    int len = e[0];
    if(len > buflen - 1) len = buflen - 1;
    memcpy(buf, e + 1, len);
    buf[len] = 0;
}

// the pop-up's row height: the control's own
static int ListBoxRowH(int r) {
    int h = Ctrl[r].y2 - Ctrl[r].y1;
    if(h < gui_font_height + 2) h = gui_font_height + 2;
    return h;
}

// the scroll window kept within the items, so that the last page is full
static void ListBoxClampTop(struct s_ListBoxS *lb) {
    int maxtop = lb->count - lb->vis_rows;
    if(maxtop < 0) maxtop = 0;
    if(lb->top > maxtop) lb->top = maxtop;
    if(lb->top < 0) lb->top = 0;
}

void DrawListBox(int r) {
    struct s_ListBoxS *lb = (struct s_ListBoxS *)Ctrl[r].s;
    char buf[MAXSTRLEN];
    int x1 = Ctrl[r].x1, y1 = Ctrl[r].y1, x2 = Ctrl[r].x2, y2 = Ctrl[r].y2;
    int h = y2 - y1, z = h / 4, sel = (int)Ctrl[r].value;
    int cx = x2 - h / 2;                                            // the centre of the marker (a square at the right)
    SpecialDrawRBox(x1, y1, x2, y2, 10, Ctrl[r].fc, Ctrl[r].bc, Ctrl[r].state);
    SpecialDrawTriangle(cx, y2 - z, x2 - h + z, y1 + z, x2 - z, y1 + z, Ctrl[r].fc, Ctrl[r].bc, Ctrl[r].state);
    if(sel >= 0 && sel < lb->count) {                               // the selected item, cut to the space left of the marker
        int avail = (x2 - h) - (x1 + gui_font_width / 2);
        ListBoxGetItem(lb, sel, buf, sizeof(buf));
        if(avail > 0 && (int)strlen(buf) > avail / gui_font_width) buf[avail / gui_font_width] = 0;
        SpecialPrintString(x1 + gui_font_width / 2, y1 + h / 2, JUSTIFY_LEFT, JUSTIFY_MIDDLE, ORIENT_NORMAL, Ctrl[r].fc, Ctrl[r].bc, buf, Ctrl[r].state);
    }
}

// what a touch at x, y meets in control r's open pop-up: an item, an arrow, or outside it
static int ListBoxHit(int r, int x, int y) {
    struct s_ListBoxS *lb = (struct s_ListBoxS *)Ctrl[r].s;
    int rowh = ListBoxRowH(r), data_y1, row, idx;
    if(x < lb->pop_x1 || x > lb->pop_x2 || y < lb->pop_y1 || y > lb->pop_y2) return LB_HIT_OUTSIDE;
    if(lb->arrows) {
        if(y < lb->pop_y1 + rowh) return LB_HIT_UPARROW;
        if(y >= lb->pop_y2 - rowh) return LB_HIT_DOWNARROW;
    }
    data_y1 = lb->pop_y1 + (lb->arrows ? rowh : 0);
    row = (y - data_y1) / rowh;
    if(row < 0) row = 0;
    if(row >= lb->vis_rows) row = lb->vis_rows - 1;
    idx = lb->top + row;
    if(idx >= lb->count) return LB_HIT_OUTSIDE;                     // the blank space past the last item
    return idx;
}

// the open pop-up drawn: its box, the rows shown (the selected or touched one inverted) and the scroll arrows
static void ListBoxDrawPopup(int r) {
    struct s_ListBoxS *lb = (struct s_ListBoxS *)Ctrl[r].s;
    char buf[MAXSTRLEN];
    int rowh = ListBoxRowH(r);
    int data_y1 = lb->pop_y1 + (lb->arrows ? rowh : 0);
    int hl = (lb->pend >= 0) ? lb->pend : (int)Ctrl[r].value;       // the row highlighted
    int i, idx, ry1, fc, bc, cx, z, aw;
    SpecialDrawBox(lb->pop_x1, lb->pop_y1, lb->pop_x2, lb->pop_y2, 1, Ctrl[r].fc, gui_bcolour, Ctrl[r].state);
    for(i = 0; i < lb->vis_rows; i++) {
        idx = lb->top + i;
        ry1 = data_y1 + i * rowh;
        if(idx == hl && idx < lb->count) {
            fc = Ctrl[r].bc;                                        // the selected row inverted
            bc = Ctrl[r].fc;
        } else {
            fc = Ctrl[r].fc;
            bc = Ctrl[r].bc;
        }
        SpecialDrawBox(lb->pop_x1 + 1, ry1, lb->pop_x2 - 1, ry1 + rowh, 0, bc, bc, Ctrl[r].state);
        if(idx < lb->count) {
            int avail = (lb->pop_x2 - lb->pop_x1) - gui_font_width;
            ListBoxGetItem(lb, idx, buf, sizeof(buf));
            if(avail > 0 && (int)strlen(buf) > avail / gui_font_width) buf[avail / gui_font_width] = 0;
            SpecialPrintString(lb->pop_x1 + gui_font_width / 2, ry1 + rowh / 2, JUSTIFY_LEFT, JUSTIFY_MIDDLE, ORIENT_NORMAL, fc, bc, buf, Ctrl[r].state);
        }
    }
    if(lb->arrows) {
        cx = (lb->pop_x1 + lb->pop_x2) / 2;
        z = rowh / 4;
        aw = rowh / 3;
        fc = (lb->top > 0) ? Ctrl[r].fc : ChangeBright(Ctrl[r].fc, BTN_DISABLED);   // the up arrow, greyed at the top
        SpecialDrawBox(lb->pop_x1 + 1, lb->pop_y1 + 1, lb->pop_x2 - 1, lb->pop_y1 + rowh, 0, Ctrl[r].bc, Ctrl[r].bc, Ctrl[r].state);
        SpecialDrawTriangle(cx, lb->pop_y1 + z, cx - aw, lb->pop_y1 + rowh - z, cx + aw, lb->pop_y1 + rowh - z, fc, Ctrl[r].bc, Ctrl[r].state);
        fc = (lb->top < lb->count - lb->vis_rows) ? Ctrl[r].fc : ChangeBright(Ctrl[r].fc, BTN_DISABLED);   // the down arrow
        SpecialDrawBox(lb->pop_x1 + 1, lb->pop_y2 - rowh, lb->pop_x2 - 1, lb->pop_y2 - 1, 0, Ctrl[r].bc, Ctrl[r].bc, Ctrl[r].state);
        SpecialDrawTriangle(cx, lb->pop_y2 - z, cx - aw, lb->pop_y2 - rowh + z, cx + aw, lb->pop_y2 - rowh + z, fc, Ctrl[r].bc, Ctrl[r].state);
    }
}

// the pop-up closed: erased to the background, and every control drawn again (what was under it is not saved)
static void ListBoxClose(int r) {
    struct s_ListBoxS *lb = (struct s_ListBoxS *)Ctrl[r].s;
    DrawBox(lb->pop_x1, lb->pop_y1, lb->pop_x2, lb->pop_y2, 0, 0, gui_bcolour);
    lb->pend = -1;
    ListBoxArrowRepeat = false;
    InvokingCtrl = 0;
    LastRef = r;
    PopUpRedrawAll(0, false);                                       // the other controls enabled and drawn again
}

// the pop-up of the listbox InvokingCtrl: KEY_OPEN opens it (below the control, or above it if there is more room
// there), KEY_DRAW_ALL draws it again, KEY_KEY_DWN and KEY_KEY_UP are a touch and its release, KEY_KEY_CANCEL closes it
void DrawListBoxPopup(int mode) {
    int r = InvokingCtrl;
    struct s_ListBoxS *lb;
    int rowh, fitrows, below, above, total, hit, sel;
    if(r == 0 || Ctrl[r].type != CTRL_LISTBOX) return;
    lb = (struct s_ListBoxS *)Ctrl[r].s;
    rowh = ListBoxRowH(r);

    if(mode == KEY_OPEN) {
        int data = lb->count;                                       // the rows to show, and on which side
        if(lb->maxrows && data > lb->maxrows) data = lb->maxrows;
        if(data < 1) data = 1;
        below = (VRes - Ctrl[r].y2) / rowh;
        above = Ctrl[r].y1 / rowh;
        fitrows = (below >= above) ? below : above;
        if(fitrows < 1) fitrows = 1;
        if(data > fitrows) data = fitrows;
        lb->arrows = (lb->count > data) ? 1 : 0;
        if(lb->arrows && data + 2 > fitrows) {
            data = fitrows - 2;
            if(data < 1) {
                data = fitrows;
                lb->arrows = 0;                                     // no room for the arrows: what fits, unscrolled
            }
        }
        lb->vis_rows = data;
        total = data + (lb->arrows ? 2 : 0);
        lb->pop_x1 = Ctrl[r].x1;
        lb->pop_x2 = Ctrl[r].x2;
        if(below >= above) {
            lb->pop_y1 = Ctrl[r].y2;
            lb->pop_y2 = lb->pop_y1 + total * rowh;
        } else {
            lb->pop_y2 = Ctrl[r].y1;
            lb->pop_y1 = lb->pop_y2 - total * rowh;
        }
        if(lb->pop_y1 < 0) lb->pop_y1 = 0;
        if(lb->pop_y2 > VRes) lb->pop_y2 = VRes;
        sel = (int)Ctrl[r].value;                                   // the selection scrolled into view
        if(sel < 0) sel = 0;
        if(sel < lb->top) lb->top = sel;
        if(sel >= lb->top + lb->vis_rows) lb->top = sel - lb->vis_rows + 1;
        ListBoxClampTop(lb);
        lb->pend = -2;                                              // no touch inside the pop-up yet
        ListBoxDrawPopup(r);
        return;
    }

    if(mode == KEY_DRAW_ALL) {
        ListBoxDrawPopup(r);
        return;
    }

    if(mode == KEY_KEY_CANCEL) {
        ListBoxClose(r);
        return;
    }

    if(mode == KEY_KEY_DWN) {
        ListBoxArrowRepeat = false;
        hit = ListBoxHit(r, TouchX, TouchY);
        if(hit == LB_HIT_OUTSIDE) {
            ListBoxClose(r);                                        // a touch outside closes it, the selection unchanged
            return;
        }
        if(hit == LB_HIT_UPARROW) {
            lb->pend = -3;
            if(lb->top > 0) {
                lb->top--;
                ListBoxArrowRepeat = true;
                ListBoxDrawPopup(r);
            }
            return;
        }
        if(hit == LB_HIT_DOWNARROW) {
            lb->pend = -3;
            if(lb->top < lb->count - lb->vis_rows) {
                lb->top++;
                ListBoxArrowRepeat = true;
                ListBoxDrawPopup(r);
            }
            return;
        }
        lb->pend = hit;                                             // an item: highlighted, and selected on release
        ListBoxDrawPopup(r);
        return;
    }

    if(mode == KEY_KEY_UP) {
        ListBoxArrowRepeat = false;
        if(lb->pend >= 0) {
            Ctrl[r].value = lb->pend;                               // the selection made, and the pop-up closed
            ListBoxClose(r);
        } else {
            lb->pend = -1;                                          // the opening touch or an arrow released: it stays open
            ListBoxDrawPopup(r);
        }
        return;
    }
}


/*********************************************************************************************
 The SLIDER control (the PicoMite's)
 A thumb on a track for a value from min to max, horizontal if the control is wider than it is tall (min at the left),
 vertical otherwise (min at the bottom). It uses the control's own min, max, inc and value.
 ********************************************************************************************/
void DrawSlider(int r) {
    int x1 = Ctrl[r].x1, y1 = Ctrl[r].y1, x2 = Ctrl[r].x2, y2 = Ctrl[r].y2;
    int w = x2 - x1, h = y2 - y1;
    int bc = (Ctrl[r].bc >= 0) ? Ctrl[r].bc : gui_bcolour;
    int fc = Ctrl[r].fc;
    int groove = ChangeBright(fc, BTN_DISABLED);
    MMFLOAT span = Ctrl[r].max - Ctrl[r].min;
    MMFLOAT frac = (span != 0) ? (Ctrl[r].value - Ctrl[r].min) / span : 0;
    if(frac < 0) frac = 0;
    if(frac > 1) frac = 1;
    SpecialDrawBox(x1, y1, x2, y2, 0, bc, bc, Ctrl[r].state);      // the old thumb erased
    if(w >= h) {                                                    // horizontal
        int tw = h, t1 = x1 + tw / 2, t2 = x2 - tw / 2, cx, trk1, trk2;   // (a square thumb)
        if(t2 <= t1) t2 = t1 + 1;
        cx = t1 + (int)(frac * (t2 - t1) + 0.5);
        trk1 = y1 + h / 2 - h / 8;
        trk2 = y1 + h / 2 + h / 8;
        SpecialDrawRBox(t1 - tw / 4, trk1, t2 + tw / 4, trk2, (trk2 - trk1) / 2, groove, groove, Ctrl[r].state);
        if(cx > t1 - tw / 4) SpecialDrawRBox(t1 - tw / 4, trk1, cx, trk2, (trk2 - trk1) / 2, fc, fc, Ctrl[r].state);   // the part up to the thumb
        SpecialDrawRBox(cx - tw / 2, y1, cx + tw / 2, y2, tw / 3, ChangeBright(fc, -20), fc, Ctrl[r].state);           // the thumb
    } else {                                                        // vertical
        int tw = w, t1 = y1 + tw / 2, t2 = y2 - tw / 2, cy, trk1, trk2;
        if(t2 <= t1) t2 = t1 + 1;
        cy = t2 - (int)(frac * (t2 - t1) + 0.5);
        trk1 = x1 + w / 2 - w / 8;
        trk2 = x1 + w / 2 + w / 8;
        SpecialDrawRBox(trk1, t1 - tw / 4, trk2, t2 + tw / 4, (trk2 - trk1) / 2, groove, groove, Ctrl[r].state);
        if(cy < t2 + tw / 4) SpecialDrawRBox(trk1, cy, trk2, t2 + tw / 4, (trk2 - trk1) / 2, fc, fc, Ctrl[r].state);   // from the bottom up to the thumb
        SpecialDrawRBox(x1, cy - tw / 2, x2, cy + tw / 2, tw / 3, ChangeBright(fc, -20), fc, Ctrl[r].state);           // the thumb
    }
}

// slider r set from the pointer (TouchX, TouchY), snapped to inc from min if inc is set, and drawn if it changed
static void SliderTrack(int r) {
    int sw = Ctrl[r].x2 - Ctrl[r].x1, sh = Ctrl[r].y2 - Ctrl[r].y1;
    MMFLOAT frac, v;
    float fv;
    if(sw >= sh) {
        int t1 = Ctrl[r].x1 + sh / 2, t2 = Ctrl[r].x2 - sh / 2;
        if(t2 <= t1) t2 = t1 + 1;
        frac = (MMFLOAT)(TouchX - t1) / (t2 - t1);
    } else {
        int t1 = Ctrl[r].y1 + sw / 2, t2 = Ctrl[r].y2 - sw / 2;
        if(t2 <= t1) t2 = t1 + 1;
        frac = (MMFLOAT)(t2 - TouchY) / (t2 - t1);                  // (the top is max)
    }
    if(frac < 0) frac = 0;
    if(frac > 1) frac = 1;
    v = Ctrl[r].min + frac * (Ctrl[r].max - Ctrl[r].min);
    if(Ctrl[r].inc > 0) v = Ctrl[r].min + (MMFLOAT)((long long)((v - Ctrl[r].min) / Ctrl[r].inc + 0.5)) * Ctrl[r].inc;
    if(v < Ctrl[r].min) v = Ctrl[r].min;
    if(v > Ctrl[r].max) v = Ctrl[r].max;
    fv = (float)v;                                                  // (compared as stored: a control's value is a float)
    CurrentRef = r;
    if(Ctrl[r].value != fv) {
        Ctrl[r].value = fv;
        DrawControl(r);
    }
}


// this will set all controls (except the control r) to disabled or non disabled
// note: r must always be a reference to the text box (never the keypad)
void PopUpRedrawAll(int r, int disabled) {
    int i;
    for(i = 1; i < Option.MaxCtrls; i++) {                          // find all the other controls and set to disabled
        if(Ctrl[i].type && i != r) {
            if(disabled)
                Ctrl[i].state |= CTRL_DISABLED2;                    // set it to disable
            else
                Ctrl[i].state &= ~CTRL_DISABLED2;                   // set it to not disabled
            UpdateControl(i);
        }
    }
}



void KeyPadErase(int is_alpha) {
    int x1, y1, x2, y2;
    int j1, j2;

    GetSingleKeyCoord(is_alpha ? 22:9, is_alpha, &x1, &y1, &j1, &j2);// get the top left coordinate of the key that is top left
    GetSingleKeyCoord(is_alpha ? 10:2, is_alpha, &j1, &j2, &x2, &y2);// get the bottom right coordinate of the key which is bottom right
    DrawBox(x1, y1, x2, y2, 0, 0, gui_bcolour);                     // erase the whole keyboard

    GUIKeyDown = -1;
}


void DrawSingleKey(int is_alpha, int x1, int y1, int x2, int y2, char *s, int fc, int bc) {
    int fnt, i;
    fnt = gui_font;
    if(is_alpha) {
        if(HRes > 600)
            SetFont(0x31);                                          // set a readable font
        else if(HRes > 400)
            SetFont(0x11);
        else
             SetFont(0x01);
    } else {
        if(VRes > 240)
            SetFont(0x31);                                          // set a readable font
        else if(VRes > 160)
            SetFont(0x11);
        else
             SetFont(0x01);
    }
    SpecialDrawRBox(x1, y1, x2, y2, 10,  fc, bc, 0);     // the border
    SpecialPrintString(x1 + (x2 - x1)/2, y1 + (y2 - y1)/2, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, fc, bc, s, 0);
    if(is_alpha && *s == ' ') {                                                 // a space means that we should draw the shift symbol inside the button
        for(i = 0; i < ((x2 - x1) / 3); i++)
            SpecialDrawLine(x1 + (x2 - x1)/2 - i/2, y1 + i + (y2 - y1)/3, x1 + (x2 - x1)/2 + i/2, y1 + i + (y2 - y1)/3, 1, fc, 0);
    }
    SetFont(fnt);
}


// invoke the callback routine so that the programmer has the option of manipulating the result
//void DoCallback(int InvokingCtrl, char *key) {
//    char callstr[32];
//    char *nextstmtSaved = nextstmt;
//    if(!InPause && FindSubFun("MM.KEYPRESS", 0) >= 0) {
//        strcpy(callstr, "MM.KEYPRESS"); IntToStrPad(callstr + strlen(callstr), InvokingCtrl, ' ', 4, 10);
//        strcat(callstr, ", \""); strcat(callstr, key); strcat(callstr, "\"");
//        callstr[strlen(callstr)+1] = 0;                             // two NULL chars required to terminate the call
//        LocalIndex++;
//        InCallback = true;
//        ExecuteProgram(callstr);
//        InCallback = false;
//        LocalIndex--;
//        TempMemoryIsChanged = true;                                 // signal that temporary memory should be checked
//        nextstmt = nextstmtSaved;
//    }
//}


void DrawKeyboard(int mode) {
    int x1, y1, x2, y2;
    int is_alpha, nbr_buttons;
    int i;
    char *p;

    if(Ctrl[InvokingCtrl].type == CTRL_TEXTBOX) {
        nbr_buttons = 33;
        is_alpha = true;
    } else {
        nbr_buttons = 12;
        is_alpha = false;
    }

    if(mode == KEY_OPEN) {
//        DoCallback(InvokingCtrl, "Open");
        KeyPadErase(Ctrl[InvokingCtrl].type == CTRL_TEXTBOX);       // erase the background
        mode = KEY_DRAW_ALL;
    }

    // special processing for KEY_KEY_UP when the numeric keypad is in alt mode
    // this switches back to number mode for everything except for backspace when there are still chars in the buffer
    if(mode == KEY_KEY_UP && !is_alpha && KeyAltShift && GUIKeyDown > 0 && (GUIKeyDown != 9 || *Ctrl[InvokingCtrl].s == 0)) {
        KeyAltShift = false;                                        // redraw all keys in number mode
        mode = KEY_DRAW_ALL;                                        // drop thru to redraw all
    }

    if(mode == KEY_DRAW_ALL) {
        // just draw all the buttons
        for(i = 0; i < nbr_buttons; i++) {
            GetSingleKeyCoord(i, is_alpha, &x1, &y1, &x2, &y2);
            DrawSingleKey(is_alpha, x1, y1, x2, y2, GetCaption(i, is_alpha, KeyAltShift), Ctrl[InvokingCtrl].fc, Ctrl[InvokingCtrl].bc);
        }
        GUIKeyDown = -1;                                               // no key to be down
        return;
    }

    // if a key has been released simply redraw it in normal mode and return immediately
    if(mode == KEY_KEY_UP) {
        if(GUIKeyDown >= 0) {
            GetSingleKeyCoord(GUIKeyDown, is_alpha, &x1, &y1, &x2, &y2);
            DrawSingleKey(is_alpha, x1, y1, x2, y2, GetCaption(GUIKeyDown, is_alpha, KeyAltShift), Ctrl[InvokingCtrl].fc, Ctrl[InvokingCtrl].bc);
        }
        GUIKeyDown = -1;
        return;
    }

    if(mode != KEY_KEY_CANCEL) {
        for(i = 0; i < nbr_buttons; i++) {                                  // search through the keys
            GetSingleKeyCoord(i, is_alpha, &x1, &y1, &x2, &y2);             // find the coordinates for this key
            if(TouchX > x1 && TouchX < x2 && TouchY > y1 && TouchY < y2) {  // if the touch is on this key
                if(!is_alpha && KeyAltShift) {                              // if this is the number keypad
                    // this ignores the invalid keys on the number pad when in the alt mode
                    switch(i) {
                        case 4:
                        case 5:
                        case 11: return;
                        case 3:  if(*Ctrl[InvokingCtrl].s == 0) break; else if(strchr(Ctrl[InvokingCtrl].s, '.') != NULL) return; break;
                        case 8:  if(*Ctrl[InvokingCtrl].s == 0) break; else if(strchr(Ctrl[InvokingCtrl].s, 'E') != NULL) return; break;
                        case 9:
                        case 10: if(*Ctrl[InvokingCtrl].s == 0) return;
                    }
                }
                GUIKeyDown = i;                                        // record the key that has been touched
                ClickTimer += CLICK_DURATION;                       // make a click
                DrawSingleKey(is_alpha, x1, y1, x2, y2, GetCaption(i, is_alpha, KeyAltShift), Ctrl[InvokingCtrl].bc, Ctrl[InvokingCtrl].fc);
            }
        }

        if(GUIKeyDown == -1) return;

        if(GUIKeyDown == 0) {                                          // this is the alt key
            KeyAltShift = !(KeyAltShift & 1);                       // toggle alt/text bit, also resets shift to no shift
            DrawKeyboard(KEY_DRAW_ALL);                             // redraw the keyboard/keypad
            return;
        }

        if(GUIKeyDown == 21) {                                                         // if this is the shift key on a textbox
            KeyAltShift = ((!(KeyAltShift & 0b10) << 1) | (KeyAltShift & 0b01));    // toggle the shift key
            DrawKeyboard(KEY_DRAW_ALL);                                             // redraw the keyboard/keypad
            return;
        }
    }

    if(mode == KEY_KEY_CANCEL || (is_alpha && GUIKeyDown == 11) || (!is_alpha && KeyAltShift && GUIKeyDown == 1)) {        // the user has hit the cancel key
        strcpy(Ctrl[InvokingCtrl].s, CancelValue);                  // restore the current value
        GUIKeyDown = (is_alpha ? 10 : 2);                              // fake the Enter key
    }

    if(GUIKeyDown == (is_alpha ? 10 : 2)) {                            // this is the enter key
//        DoCallback(InvokingCtrl, GetCaption(GUIKeyDown, is_alpha, KeyAltShift));
        if(!is_alpha) {
            int t = STR_AUTO_PRECISION;
            // get the number of digits that have been entered after the decimal point and use that to determine the precision of the display
            if(strchr(Ctrl[InvokingCtrl].s, 'e') == NULL && strchr(Ctrl[InvokingCtrl].s, 'E') == NULL && strchr(Ctrl[InvokingCtrl].s, '.') != NULL) {
                t = strlen(strchr(Ctrl[InvokingCtrl].s, '.') + 1);
                if(t < 1 || t > 16) t = STR_AUTO_PRECISION;
            }
            FloatToStr(Ctrl[InvokingCtrl].s, Ctrl[InvokingCtrl].value = atof(Ctrl[InvokingCtrl].s), 1, t, ' ');     // convert to a float and put back on the display
        }
        KeyPadErase(is_alpha);                                      // hide the keypad
        Ctrl[InvokingCtrl].state &= ~CTRL_SELECTED;                 // deselect the text box
        PopUpRedrawAll(0, false);                                   // redraw all the controls
        LastRef = InvokingCtrl;
        InvokingCtrl = 0;
        return;
    }

    p = Ctrl[InvokingCtrl].s + strlen(Ctrl[InvokingCtrl].s);
    if(!is_alpha) {
        // this is the number pad
        if(KeyAltShift) {
            // the keypad is in the alt mode
            // process the special keys
            switch(GUIKeyDown) {
                case 3:  *p++ = '.'; break;
                case 6:  *p++ = '+'; break;
                case 7:  *p++ = '-'; break;
                case 8:  *p++ = 'E'; break;
                case 9:  *(--p) = 0; break;
                case 10: *Ctrl[InvokingCtrl].s = 0; break;
                default: return;
            }
        } else {
            *p++ = *GetCaption(GUIKeyDown, is_alpha, KeyAltShift);     // otherwise use the caption
        }
    } else {
        // this is the text keyboard
        if(GUIKeyDown == 32) {                                         // is this the delete key
            if(strlen(Ctrl[InvokingCtrl].s)) *(--p) = 0;            // delete the char
        } else if(GUIKeyDown == 9) {                                   // if it is space
            *p++ = ' ';                                             // insert a space
        } else {
            *p++ = *GetCaption(GUIKeyDown, is_alpha, KeyAltShift);     // otherwise use the caption
        }
    }
    *p = 0;

//    DoCallback(InvokingCtrl, GetCaption(GUIKeyDown, is_alpha, KeyAltShift));
    DrawControl(InvokingCtrl);
}


char *FmtKeyCaption[12] = { "<<<", "0", "Can", "1", "2", "3", "4", "5", "6", "7", "8", "9" } ;
char FmtKeyEnabled[12];


void DrawFmtKeypad(char x, char prev, int GUIKeyDown) {
    int i, dimfc, x1, y1, x2, y2;
//    dp("1"); uSec(10000);

    for(i = 0; i < 12; i++) FmtKeyEnabled[i] = false;
    FmtKeyEnabled[0] = FmtKeyEnabled[2] = true;
    FmtKeyCaption[7] = "5"; FmtKeyCaption[10] = "8";

    if(!isdigit(x)) {
        switch(x) {
            case '+': FmtKeyCaption[10] = "+";  FmtKeyCaption[7] = "-"; x = 0; break;
            case 'A': FmtKeyCaption[10] = "AM"; FmtKeyCaption[7] = "PM"; x = 0; break;
            case 'N': FmtKeyCaption[10] = "N";  FmtKeyCaption[7] = "S"; x = 0; break;
            case 'E': FmtKeyCaption[10] = "E";  FmtKeyCaption[7] = "W"; x = 0; break;
            case 'M': if(prev == '0') {
                          x = '9';
                      } else {
                          x = '2';
                          FmtKeyEnabled[1] = true;
                      }
                      break;
            case 'D': if(prev == '0') {
                          x = '9';
                      } else {
                          FmtKeyEnabled[1] = true;
                          if(prev == '3')
                              x = '1';
                          else
                              x = '9';

                      }
                      break;
            case 'H': FmtKeyEnabled[1] = true;
                      if(prev == '0' || prev == '1')
                          x = '9';
                      else
                          x = '3';
                      break;
            case 'L': FmtKeyEnabled[1] = true;
                      if(prev == '0')
                          x = '9';
                      else
                          x = '7';
                      break;
        }
        if(x == 0)
            FmtKeyEnabled[10] = FmtKeyEnabled[7] = true;
        else
            for(i = 3 ; i < x - '0' + 3 ; i++)
                FmtKeyEnabled[i] = true;
    } else {
        FmtKeyEnabled[1] = true;
        for(i = 3 ; i < x - '0' + 3 ; i++)
            FmtKeyEnabled[i] = true;
    }

//    dp("1"); uSec(10000);
    dimfc = ChangeBright(Ctrl[InvokingCtrl].fc, BTN_DISABLED);
    for(i = 0; i < 12; i++){
//    dp("2"); uSec(10000);
        GetSingleKeyCoord(i, false, &x1, &y1, &x2, &y2);
//    dp("3"); uSec(10000);
        if(i == GUIKeyDown)
            DrawSingleKey(false, x1, y1, x2, y2, FmtKeyCaption[i], Ctrl[InvokingCtrl].bc, Ctrl[InvokingCtrl].fc);
        else
            DrawSingleKey(false, x1, y1, x2, y2, FmtKeyCaption[i], FmtKeyEnabled[i] ? Ctrl[InvokingCtrl].fc:dimfc, Ctrl[InvokingCtrl].bc);
    }
}


void DrawFmtControl(int r, char *pfmt) {
    char *p;
    p = Ctrl[r].s + strlen(Ctrl[r].s);
    if(*pfmt) *p++ = 0xff;                                                    // marker to change the following text brightness to dim
    while(*pfmt) {
        if(*pfmt == '(') {
            pfmt++;
            while(*pfmt != ')') *p++ = *pfmt++;
            pfmt++;
        } else {
            if(*(pfmt + 1) == 0)
                *p++ = *pfmt++;
            else {
                *p++ = *(pfmt + 1);
                pfmt += 2;
            }
        }
    }
    *p = 0;
    DrawControl(r);
}


void DrawFmtBox(int mode) {
    int i, x1, y1, x2, y2;
    static char *pfmt, *p;

    if(mode == KEY_OPEN) {
        KeyPadErase(false);                                         // erase the background
        pfmt = Ctrl[InvokingCtrl].fmt;
        p = Ctrl[InvokingCtrl].s;
        DrawFmtKeypad(*pfmt, 0, 99);                                // just draw all the buttons
        DrawFmtControl(InvokingCtrl, pfmt);
        GUIKeyDown = -1;                                               // no key to be down
        return;
    }

    if(mode == KEY_KEY_UP) {
        if(GUIKeyDown < 0) return;
        DrawFmtKeypad(*pfmt, *(p - 1), 99);                         // just draw all the buttons to show that the key is up
        return;
    }

    if(mode == KEY_KEY_DWN) {
        // if we are here we have the simple case of a key down
        for(i = 0; i < 12; i++) {                                           // search through the keys
            GetSingleKeyCoord(i, false, &x1, &y1, &x2, &y2);                // find the coordinates for this key
            if(TouchX > x1 && TouchX < x2 && TouchY > y1 && TouchY < y2) {  // if the touch is on this key
                // get the key and do something with it
                if(FmtKeyEnabled[i] == false || (i == 0 && pfmt == Ctrl[InvokingCtrl].fmt)) return;                      // do nothing if key is disabled
                DrawFmtKeypad(*pfmt, *(p - 1), i);                      // show that the key is down
                ClickTimer += CLICK_DURATION;  // make a click

                if(i == 0) {
                    // it is the backspace key
                    if(*(pfmt - 1) == ')') {
                        pfmt -= 2;
                        while(*pfmt-- != '(') p--;
                        pfmt--; p--;
                    } else {
                        pfmt -= 2;
                        p--;
                    }
                    *p = 0;
                } else if(i == 2) {
                    mode = KEY_KEY_CANCEL;
                    break;
                } else {
                    // this is an ordinary key (eg, 0 to 9)
                    p += strlen(strcpy(p, FmtKeyCaption[i]));          // add they key caption to the display box string
                    pfmt++;
                    if(*pfmt) pfmt++;
                    if(*pfmt) {
                        if(*pfmt == '(') {
                            pfmt++;
                            while(*pfmt && *pfmt != ')') *p++ = *pfmt++;
                            *p = 0;
                            if(*pfmt) pfmt++;
                        }
                    }
                    if(*pfmt == 0) break;                           // *pfmt == 0 means that we have reached the end of the format string
                }
                GUIKeyDown = i;
                DrawFmtControl(InvokingCtrl, pfmt);
                return;
            }
        }
    }


    if(*pfmt == 0 || mode == KEY_KEY_CANCEL) {
        if(mode == KEY_KEY_CANCEL) strcpy(Ctrl[InvokingCtrl].s, CancelValue);       // if we are cancelling first restore the current value
        KeyPadErase(false);                                         // hide the keypad
        DrawControl(InvokingCtrl);
        Ctrl[InvokingCtrl].state &= ~CTRL_SELECTED;                 // deselect the text box
        PopUpRedrawAll(0, false);                                   // redraw all the controls
        LastRef = InvokingCtrl;
        InvokingCtrl = 0;
        return;
    }
}


// draw a control on the screen
void DrawControl(int r) {
    int fnt;
    fnt = gui_font;
    SetFont(Ctrl[r].font);
    switch(Ctrl[r].type) {
        case CTRL_BUTTON:       DrawButton(r);   break;
        case CTRL_SWITCH:       DrawSwitch(r);   break;
        case CTRL_CHECKBOX:     DrawCheckBox(r); break;
        case CTRL_RADIOBTN:     DrawRadioBtn(r); break;
        case CTRL_LED:          DrawLED(r);      break;
        case CTRL_SPINNER:      DrawSpinner(r);  break;
        case CTRL_FRAME:        DrawFrame(r);    break;
        case CTRL_TEXTBOX:
        case CTRL_FMTBOX:
        case CTRL_NBRBOX:
        case CTRL_DISPLAYBOX:   DrawDisplayBox(r);  break;
        case CTRL_CAPTION:      DrawCaption(r);  break;
        case CTRL_GAUGE:        DrawGauge(r);    break;
        case CTRL_BARGAUGE:     DrawBarGauge(r); break;
        case CTRL_LISTBOX:      DrawListBox(r);  break;
        case CTRL_SLIDER:       DrawSlider(r);   break;
        default:                break;
    }
    SetFont(fnt);
}


// similar to DrawControl() but it will only redraw the control if it is in the current
// set of pages for display and if it is not hidden
void UpdateControl(int r) {
    if(Ctrl[r].type && (CurrentPages & (1 << Ctrl[r].page)) && !(Ctrl[r].state & CTRL_HIDDEN)) DrawControl(r);
}


// check if the pen has touched or been lifted and animate the GUI elements as required
// this is called after every command (from check_interrupt()), in the getchar() loop and repeatedly in a pause
// TouchDown and TouchUp are set in the Timer 4 interrupt
void ProcessTouch(void) {
    static int repeat = 0;
    static int waiting = false;
    int r, spinup;
    if(!GUI_INPUT_ON)return;
    if(DelayedDrawKeyboard || DelayedDrawFmtBox || DelayedDrawListbox) return;   // a pop-up is drawn after the pen-down
                                                                    // interrupt: its touches wait until then (the PicoMite
                                                                    // guards check_interrupt's call; here every caller)

    /* Multi-touch poll. Must run on every ProcessTouch entry - including
       cycles with no TouchDown/TouchUp edge - because a finger-2 contact
       on a USB touch panel doesn't toggle the primary TouchDown/Up state
       (those follow contact 0 only). (The PicoMite's.) */
    if(osk_state) OSK_PollFinger2();
    int again = 0;                                                  // a held press's repeat, not a new press
    if(repeat) {
        if(GuiClickFromMouse ? (TOUCH_DOWN || GuiClickDown || ClickPinDown()) : usb_touch_active)   // (held: by the press's owner)
            if(TouchTimer < repeat)
                return;
            else
                TouchDown = again = true;
        else
            repeat = 0;
    } else {
        if(waiting) {
            if(TouchTimer < 50 && !GuiClickFromMouse)               // wait 50mS before a processing touch (allows the touch to settle;
                                                                    // a mouse and GUI CLICK do not bounce: the PicoMite's)
                return;
            else
                waiting = false;
        } else {
            waiting = true;
            TouchTimer = 0;                                         // TouchTimer is incremented every mS in the Timer 4 interrupt
        }
    }

    if(!(TouchUp || TouchDown)) return;                             // quick exit if there is nothing to do
    if(TouchDown) {
        // touch has just occurred
        if(!GuiClickFromMouse || again) {                           // (a mouse's press was taken where it was made: GuiPressTick;
                                                                    // its repeat is where the pointer is now: a slider dragged)
            TouchX = GetTouch(GET_X_AXIS);
            TouchY = GetTouch(GET_Y_AXIS);
        }
        touch_gesture_on_down(TouchX, TouchY);                      // (the PicoMite's, for a mouse, GUI CLICK or the USB touch
        LastRef = CurrentRef = 0;                                   // screen: each press here is one of those)
        TouchUp = TouchDown = false;
        if(!GuiClickFromMouse && TouchX == TOUCH_ERROR) return;     // abort if the pen was lifted
        if(InvokingCtrl) {                                          // the keyboard/keypad takes complete control when activated
            if(Ctrl[InvokingCtrl].type == CTRL_FMTBOX)
                DrawFmtBox(KEY_KEY_DWN);
            else if(Ctrl[InvokingCtrl].type == CTRL_LISTBOX) {
                DrawListBoxPopup(KEY_KEY_DWN);
                if(ListBoxArrowRepeat) {                            // a scroll arrow held: it repeats
                    repeat = (repeat == 0) ? 500 : 100;
                    TouchTimer = 0;
                } else
                    repeat = 0;
            } else
                DrawKeyboard(KEY_KEY_DWN);
            gui_int_down = false;
            return;
        }

        if (OSK_IsActive() && OptionVResreserved && TouchY >= (VRes - (VRes * OptionVResreserved / 100)))
        {                                                           // a tap on the on-screen keyboard's strip is its (the PicoMite's)
            OSK_HandleTouchDown(TouchX, TouchY);
            gui_int_down = false;
            return;
        }
        gui_int_down = true;                                        // signal that a MMBasic interrupt is valid
        if(GuiIntDownVector) IntSignal();
        if(SliderDrag) {                                            // a slider being dragged follows the mouse while the
            r = SliderDrag;                                         // button is held, past its ends too
            if(repeat && Ctrl[r].type == CTRL_SLIDER && (CurrentPages & (1 << Ctrl[r].page)) &&
               !(Ctrl[r].state & (CTRL_DISABLED | CTRL_DISABLED2 | CTRL_HIDDEN))) {
                repeat = 50;
                TouchTimer = 0;
                SliderTrack(r);
                return;
            }
            SliderDrag = 0;
        }
        for(r = 1; r < Option.MaxCtrls; r++) {
            if(Ctrl[r].type && TouchX >= Ctrl[r].x1 && TouchY >= Ctrl[r].y1 && TouchX <= Ctrl[r].x2 && TouchY <= Ctrl[r].y2) {
                if(!(CurrentPages & (1 << Ctrl[r].page))) continue;                            // ignore if the page is not displayed
                if(Ctrl[r].state & (CTRL_DISABLED | CTRL_DISABLED2 | CTRL_HIDDEN)) continue;   // ignore if control is disabled
                switch(Ctrl[r].type) {
                    case CTRL_SWITCH:   if(!(TouchX >= Ctrl[r].min && TouchX <= Ctrl[r].max)) return;   // skip if it is not the touch sensitive area (depends on the switch state)
                                        Ctrl[r].value = !Ctrl[r].value;
                                        break;
                    
                    case CTRL_BUTTON:
                    case CTRL_RADIOBTN:;
                    case CTRL_AREA:     Ctrl[r].value = 1;
                                        break;

                    case CTRL_FRAME:    continue;                   // a frame does not respond to touch but it might contain controls that do

                    case CTRL_CAPTION:  continue;                   // a caption does not respond to touch but it might be overlayed by an AREA control

                    case CTRL_DISPLAYBOX:
                    case CTRL_LED:      CurrentRef = r;
                                        return;                     // these controls do not respond to touch

                    case CTRL_SPINNER:  if(TouchX < (Ctrl[r].x1 + (Ctrl[r].y2 - Ctrl[r].y1)))               // if it is in the left side
                                                spinup = false;                                             // it is spin down
                                         else if(TouchX > (Ctrl[r].x2 - (Ctrl[r].y2 - Ctrl[r].y1)))         // if it is in the right side
                                                spinup = true;                                              // it is spin up
                                        else
                                            return;
                                        if(spinup) {
                                            if(Ctrl[r].state & CTRL_SPINDOWN) return;
                                            if(Ctrl[r].value + Ctrl[r].inc > Ctrl[r].max) { repeat = 0; break; }
                                            Ctrl[r].state |= CTRL_SPINUP;
                                            Ctrl[r].value += Ctrl[r].inc;
                                        } else {
                                            if(Ctrl[r].state & CTRL_SPINUP) return;
                                            if(Ctrl[r].value - Ctrl[r].inc < Ctrl[r].min) { repeat = 0; break; }
                                            Ctrl[r].state |= CTRL_SPINDOWN;
                                            Ctrl[r].value -= Ctrl[r].inc;
                                        }
                                        if(Ctrl[r].value < Ctrl[r].inc &&  Ctrl[r].value > -Ctrl[r].inc) Ctrl[r].value = 0;
                                        if(repeat == 0)
                                            repeat = 500;
                                        else
                                            repeat = 100;
                                        TouchTimer = 0;
                                        break;

                    case CTRL_SLIDER:   if(repeat == 0) ClickTimer += CLICK_DURATION;                       // a click on first contact only
                                        repeat = 50;                                                        // then it follows the pointer every 50mS
                                        TouchTimer = 0;
                                        SliderDrag = r;
                                        SliderTrack(r);
                                        return;                                                             // (the down interrupt fires each time)

                    case CTRL_LISTBOX:  if(((struct s_ListBoxS *)Ctrl[r].s)->count <= 0) return;           // nothing to choose from
                                        PopUpRedrawAll(r, true);                                            // the other controls disabled
                                        CurrentRef = InvokingCtrl = r;                                      // this control has the pop-up
                                        ClickTimer += CLICK_DURATION;
                                        if(GuiIntDownVector == NULL)
                                            DrawListBoxPopup(KEY_OPEN);                                     // the pop-up drawn
                                        else
                                            DelayedDrawListbox = true;                                      // after the pen-down interrupt
                                        return;

                    case CTRL_NBRBOX:
                                        FloatToStr(Ctrl[r].s, Ctrl[r].value, 0, STR_AUTO_PRECISION, ' ');   // set the string value to be saved
                                        // fall thru

                    case CTRL_TEXTBOX:  strcpy(CancelValue, Ctrl[r].s);                                     // save the current value in case the user cancels
                                        *Ctrl[r].s = 0;                                                     // set it to an empty string
                                        Ctrl[r].state |= CTRL_SELECTED;                                     // select the number text/box
                                        PopUpRedrawAll(r, true);
                                        CurrentRef = InvokingCtrl = r;                                      // tell the keypad what text/number box it is servicing
                                        GUIKeyDown = -1;
                                        KeyAltShift = 0;
                                        DrawControl(r);
                                        ClickTimer += CLICK_DURATION;
                                        if(GuiIntDownVector == NULL)
                                            DrawKeyboard(KEY_OPEN);                                          // initial draw of the keypad
                                        else
                                            DelayedDrawKeyboard = true;  
                                        return;

                    case CTRL_FMTBOX:   strcpy(CancelValue, Ctrl[r].s);                                     // save the current value in case the user cancels
                                        *Ctrl[r].s = 0;                                                     // set it to an empty string
                                        Ctrl[r].state |= CTRL_SELECTED;                                     // select the number text/box
                                        PopUpRedrawAll(r, true);
                                        CurrentRef = InvokingCtrl = r;                                      // save the format box being serviced
                                        GUIKeyDown = -1;
                                        ClickTimer += CLICK_DURATION;
                                        if(GuiIntDownVector == NULL)
                                            DrawFmtBox(KEY_OPEN);                                           // initial draw of the keypad
                                        else
                                            DelayedDrawFmtBox = true;                                       // leave it until after the keyboard interrupt
                                        return;

                    default:            Ctrl[r].value = !Ctrl[r].value;
                                        break;
                }
                CurrentRef = r;
                DrawControl(r);
                ClickTimer += CLICK_DURATION;                       // sound a "click"
                return;
            }
        }
    }

    if(TouchUp) {
        // the touch has just been released
        TouchUp = TouchDown = false;
        LastX=TouchX;
        LastY=TouchY;
        {                                                           // a swipe: from the USB touch screen's last position, else
            int16_t end_x = TouchX, end_y = TouchY;                 // the mouse's (the PicoMite's)
            int gm = GuiMouse();
            if (usb_touch_present)
            {
                end_x = usb_touch_x;
                end_y = usb_touch_y;
            }
            else if (gm >= 0)
            {
                end_x = mousestruct[gm].ax;
                end_y = mousestruct[gm].ay;
            }
            touch_gesture_on_up(end_x, end_y);
        }
        SliderDrag = 0;
        if(InvokingCtrl) {                                          // the keyboard/keypad takes complete control when activated
            if(Ctrl[InvokingCtrl].type == CTRL_FMTBOX)
                DrawFmtBox(KEY_KEY_UP);
            else if(Ctrl[InvokingCtrl].type == CTRL_LISTBOX) {
                DrawListBoxPopup(KEY_KEY_UP);
                if(InvokingCtrl == 0) {                             // an item selected: the up interrupt
                    gui_int_up = true;
                    if(GuiIntUpVector) IntSignal();
                } else
                    gui_int_down = false;
                return;
            } else
                DrawKeyboard(KEY_KEY_UP);
            gui_int_down = false;
            return;
        }

        if (OSK_IsActive() && OSK_HandleTouchUp())                  // (the on-screen keyboard's key released: the PicoMite's)
        {
            gui_int_down = false;
            return;
        }
        gui_int_up = true;
        if(GuiIntUpVector) IntSignal();
        if(CurrentRef) {
            if(Ctrl[CurrentRef].type) {
                if((CurrentPages & (1 << Ctrl[CurrentRef].page)) && !(Ctrl[CurrentRef].state & (CTRL_DISABLED | CTRL_DISABLED2 | CTRL_HIDDEN))) {   // ignore if control is disabled or the page is not displayed
                    if(CurrentRef) LastRef = CurrentRef;
                    switch(Ctrl[CurrentRef].type) {
                        case CTRL_AREA:
                        case CTRL_BUTTON:   Ctrl[CurrentRef].value = 0;
                                            DrawControl(CurrentRef);
                                            break;

                        case CTRL_SPINNER:  Ctrl[CurrentRef].state &= ~(CTRL_SPINUP | CTRL_SPINDOWN);
                                            DrawControl(CurrentRef);
                                            break;

                        default:            break;
                    }
                }
            }
        }
        CurrentRef = 0;
    }
}



void SetCtrlState(int r, int state, int err) {
    if(Ctrl[r].type == 0) {
        if(err) error("Control #% does not exist", r);
    } else {
        if(r == CurrentRef) {
            if(Ctrl[r].type == CTRL_BUTTON) {
                Ctrl[r].value = 0;
                UpdateControl(r);
            }
            if(Ctrl[r].type == CTRL_SPINNER) {
                Ctrl[r].state &= ~(CTRL_SPINUP | CTRL_SPINDOWN);
                UpdateControl(r);
            }
            LastRef = CurrentRef = 0;
            gui_int_down = gui_int_up = false;
        }
        Ctrl[r].state |= state;
        if(state & CTRL_HIDDEN)
            DrawControl(r);
        else
            UpdateControl(r);
    }
}



// this function is used by the CLS command
void HideAllControls(void) {
    int r;
    for(r = 1; r < Option.MaxCtrls; r++)
        if((CurrentPages & (1 << Ctrl[r].page)) && !(Ctrl[r].state & CTRL_HIDDEN))
            SetCtrlState(r, CTRL_HIDDEN, false);
}


// This will check if an interrupt is pending and will service if yes.
// Essentially this runs a short program which does nothing but it gives the
// interrupt checking routines in ExecuteProgram() a chance to do their job.
// This routine should be called repeatedly during long delays.
void ServiceInterrupts(void) {
    char *ttp, *s;
    int tcmdtoken;
    char p[4];

    CheckAbort();
    LocalIndex++;                                                   // preserve the current temporary string memory allocations
    ttp = nextstmt;                                                 // save the globals used by commands
    tcmdtoken = cmdtoken;
    s = cmdline;

    memset(p, 0, sizeof(p));
    commandtbl_put(p, cmdENDIF);                                    // setup a short program that does nothing
    ExecuteProgram(p);                                              // execute the program's code

    cmdline = s;                                                    // restore the globals
    cmdtoken = tcmdtoken;
    nextstmt = ttp;
    LocalIndex--;
    TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
}



void fun_msgbox(void) {
    int i, j, k;
    int x, y, x1, y1, x2, y2, msgnbr, btnnbr, btnwidth;
    char *p, *msg;
    char *btn;
    int btnx1[4], btny1, btnx2[4], btny2;
    long long int timeout;
    if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
    if(GuiClickEmulated)                                            // (the PicoMite's: nothing could steer the pointer to its buttons)
        error("MsgBox needs a real touch or mouse - cursor cannot be steered while MsgBox is open");
    getargs(&ep, 9, ",");
    if(argc < 3) error("Argument count");
    mouseon();
    msg = GetMemory(MAX_CAPTION_LINES * MAXSTRLEN);
    btn = GetMemory(4 * MAXSTRLEN);

    p = getCstring(argv[0]);
    i = msgnbr = 0;
    while(*p) {
        if(*p == '~') {
            if(msgnbr + 1 >= MAX_CAPTION_LINES) break;
            msg[msgnbr++ * MAXSTRLEN + i] = 0;
            i = 0; p++;
        } else {
            msg[msgnbr * MAXSTRLEN + i++] = *p++;
        }
    }
    msg[msgnbr++ * MAXSTRLEN + i]= 0;

    btnnbr = ((argc - 2) / 2) + 1;
    for(i = 0; i < btnnbr; i++)
        strcpy(&btn[i * MAXSTRLEN], getCstring(argv[(i * 2) + 2]));

    for(j = i = 0; i < msgnbr; i++)
        if(strlen(&msg[i * MAXSTRLEN]) > j) j = strlen(&msg[i * MAXSTRLEN]);
    j *= gui_font_width;

    for(k = i = 0; i < btnnbr; i++)
        if(strlen(&btn[i * MAXSTRLEN]) > k) k = strlen(&btn[i * MAXSTRLEN]);
    btnwidth = (k + 2) * gui_font_width;
    k = (btnwidth * btnnbr) + ((btnnbr - 1) * 2 * gui_font_width);
    if(k > j) j = k;
    j += 4 * gui_font_width;

    i = (((5 + msgnbr) * gui_font_height) / 2) + gui_font_height / 4;
    x1 = HRes/2 - j/2; y1 = VRes/2 - i; x2 = HRes/2 + j/2; y2 = VRes/2 + i;

    // wait for any pen touch to be lifted
    timeout = mSecTimer + 10000;
    do { ServiceInterrupts();
    } while(TouchState && mSecTimer < timeout) ;

    PopUpRedrawAll(0, true);
    SpecialDrawRBox(x1, y1, x2, y2, 25, gui_fcolour, gui_bcolour, CTRL_NORMAL);

    for(i = 0; i < msgnbr; i++)
      SpecialPrintString(HRes/2, y1 + gui_font_height + (gui_font_height * i), JUSTIFY_CENTER, JUSTIFY_TOP, ORIENT_NORMAL, gui_fcolour, gui_bcolour, &msg[i * MAXSTRLEN], CTRL_NORMAL);

    btny1 = y2 - (gui_font_height * 3);
    btny2 = y2 - gui_font_height;
    for(i = 0; i < btnnbr; i++) {
        btnx1[i] = HRes/2 - (k/2) + (btnwidth * i) + (i * 2 * gui_font_width);  btnx2[i] = btnx1[i] + btnwidth;
        DrawBasicButton(btnx1[i], btny1, btnx2[i], btny2, BTN_SIDE_WIDTH, 1, gui_fcolour, CTRL_NORMAL);
        SpecialPrintString(btnx1[i] + btnwidth/2, btny1 + gui_font_height, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, gui_bcolour, gui_fcolour, &btn[i * MAXSTRLEN], CTRL_NORMAL);
    }
    while(1) {
        ServiceInterrupts();
        x = GetTouch(GET_X_AXIS);  y = GetTouch(GET_Y_AXIS);
        for(i = 0; i < btnnbr; i++)
            if(x >= btnx1[i] && x <= btnx2[i] && y >= btny1 && y <= btny2)
                break;
        if(i < btnnbr) {
            DrawBasicButton(btnx1[i], btny1, btnx2[i], btny2, BTN_SIDE_WIDTH, 0, gui_fcolour, CTRL_NORMAL);
            SpecialPrintString(btnx1[i] + btnwidth/2 + BTN_CAPTION_SHIFT, btny1 + gui_font_height + BTN_CAPTION_SHIFT, JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL, gui_bcolour, gui_fcolour, &btn[i * MAXSTRLEN], CTRL_NORMAL);
            ClickTimer += CLICK_DURATION;                        // sound a "click"
            while(GetTouch(GET_X_AXIS) != TOUCH_ERROR) ServiceInterrupts();
            SpecialDrawRBox(x1, y1, x2, y2, 25, gui_bcolour, gui_bcolour, CTRL_NORMAL);
            PopUpRedrawAll(0, false);
            iret = i + 1;
            targ = T_INT;
            FreeMemory(msg);  FreeMemory(btn);
            return;
        }
    }
}


void fun_ctrlval(void) {
    int r;
    if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
    mouseon();
    if(*ep == '#') ep++;
    r = getint(ep, 1, Option.MaxCtrls);
    if(Ctrl[r].type == 0) error("Control #% does not exist", r);
    if(Ctrl[r].type == CTRL_NBRBOX) {
        if(r == InvokingCtrl)                                       // is the keypad for the number box being displayed?
                fret = atof(CancelValue);                           // if NOT in the call back just return the value saved in case of a cancel
        else
            fret = Ctrl[r].value;
        targ = T_NBR;
    } else if(Ctrl[r].type == CTRL_TEXTBOX || Ctrl[r].type == CTRL_FMTBOX) {
        sret = GetTempMemory(STRINGSIZE);                                  // this will last for the life of the command
        if(r == InvokingCtrl)                                       // is the keypad for the number box being displayed?
                strcpy(sret, CancelValue);                          // just return the value saved in case of a cancel
          else
            strcpy(sret, Ctrl[r].s);
        if(Ctrl[r].s[0] == '#' && Ctrl[r].s[1] == '#') *sret = 0;   // return a zero length string if it is just "ghost text"
        CtoM(sret);                                                 // convert to a MMBasic string
        targ = T_STR;
    } else if(Ctrl[r].type == CTRL_DISPLAYBOX || Ctrl[r].type == CTRL_CAPTION) {
        sret = GetTempMemory(STRINGSIZE);                                  // this will last for the life of the command
        strcpy(sret, Ctrl[r].s);                                    // copy the string
        CtoM(sret);                                                 // convert to a MMBasic string
        targ = T_STR;
    } else {
        fret = Ctrl[r].value;
        targ = T_NBR;
    }
}


void cmd_ctrlval(void) {
    int r;
    MMFLOAT v;
    if(HRes == 0) error("LCD Panel not configured");
    if(!Option.MaxCtrls)error("No memory allocated for GUI controls");
    mouseon();
    if(*cmdline == '#') cmdline++;
    r = getint(cmdline, 1, Option.MaxCtrls);
    if(Ctrl[r].type == 0) error("Control #% does not exist", r);
    cmdline = GetAssignedValue(cmdline, "Syntax");          // the = after the ) (#101)
    switch(Ctrl[r].type) {
        case CTRL_BUTTON:
        case CTRL_SWITCH:
        case CTRL_CHECKBOX:
        case CTRL_RADIOBTN: v = (getnumber(cmdline) != 0);
                            if(Ctrl[r].value == v) return;          // don't update if no change
                            Ctrl[r].value = v;
                            break;

        case CTRL_LED:      v = getnumber(cmdline);
                            if(v > 1) {
                                Ctrl[r].inc = v;
                                Ctrl[r].value = 1;
                                CheckGuiFlag = true;
                            } else {
                                int i;
                                CheckGuiFlag = false;
                                Ctrl[r].inc = 0;
                                // scan through all controls to see if any others are in a flash
                                // if there is, set the CheckGuiFlag which indicates that flash timing should continue
                                for(i = 1; i < Option.MaxCtrls; i++) {
                                    if(Ctrl[i].type == CTRL_LED && Ctrl[i].inc != 0) {
                                        CheckGuiFlag = true;
                                        break;
                                    }
                                }
                                if(Ctrl[r].value == v) return;      // don't update if no change
                                Ctrl[r].value = v;
                            }
                            break;

        case CTRL_SPINNER:
        case CTRL_GAUGE:
        case CTRL_SLIDER:
        case CTRL_BARGAUGE: v = getnumber(cmdline);
                            if(v < Ctrl[r].min) v = Ctrl[r].min;
                            if(v > Ctrl[r].max) v = Ctrl[r].max;
                            if(Ctrl[r].value == v) return;          // don't update if no change
                            Ctrl[r].value = v;
                            if(Ctrl[r].type == CTRL_SPINNER) FloatToStr(Ctrl[r].s, v, 0, STR_AUTO_PRECISION, ' ');   // update the displayed string
                            break;

        case CTRL_LISTBOX: {                                        // the selected index, kept within the items
                            struct s_ListBoxS *lb = (struct s_ListBoxS *)Ctrl[r].s;
                            v = getnumber(cmdline);
                            if(lb->count <= 0) v = -1;
                            else {
                                if(v < 0) v = 0;
                                if(v > lb->count - 1) v = lb->count - 1;
                            }
                            if(Ctrl[r].value == v) return;          // don't update if no change
                            Ctrl[r].value = v;
                            break;
                        }

        case CTRL_NBRBOX:   if(strlen(cmdline) > 3 && cmdline[0] == '"' && cmdline[1] == '#' && cmdline[2] == '#') {
                                // the user wants ghost text
                                strcpy(Ctrl[r].s, getCstring(cmdline));
                                Ctrl[r].value = 0;
                            } else {
                                // a normal number
                                v = getnumber(cmdline);
                                if(r != InvokingCtrl && Ctrl[r].value == v) return;     // don't update if no change
                                Ctrl[r].value = v;
                                FloatToStr(Ctrl[r].s, v, 0, STR_AUTO_PRECISION, ' ');   // update the displayed string
                            }
                            break;

        case CTRL_FRAME:
        case CTRL_CAPTION:
                            memset(Ctrl[r].s, ' ', strlen(Ctrl[r].s));  // set the caption to spaces
                            UpdateControl(r);                           // erase the existing text
                            // fall through

        case CTRL_DISPLAYBOX:
        case CTRL_TEXTBOX:
        case CTRL_FMTBOX:  strcpy(Ctrl[r].s, getCstring(cmdline));  break;
        }

    if(!(Ctrl[r].state & CTRL_DISABLED2)) UpdateControl(r);         // don't update if the gauge is disabled by a keypad (updating may mess they keypad)
}


/*void fun_mmhpos(void) {
    if(!Option.DISPLAY_TYPE) error("Display not configured");
    iret = CurrentX;
    targ = T_INT;
}


void fun_mmvpos(void) {
    if(!Option.DISPLAY_TYPE) error("Display not configured");
    iret = CurrentY;
    targ = T_INT;
}*/



/*void cmd_backlight(void) {
    if(HRes != 0 && Option.DISPLAY_TYPE > SSD_PANEL) error("SSD1963 display only");
    SetBacklightSSD1963(getint(cmdline, 0, 100));
}*/

void ResetGUI(void) {
    int i;
    InvokingCtrl = CurrentRef = LastRef = 0;
    DelayedDrawKeyboard = DelayedDrawFmtBox = DelayedDrawListbox = ListBoxArrowRepeat = false;   // (ProcessTouch waits on
    SliderDrag = 0;                                                 // these: a program stopped in its pen-down interrupt)
    LastX = LastY = TOUCH_ERROR;
    gui_int_down = gui_int_up = false;
    GuiIntDownVector = GuiIntUpVector = NULL;
    GUIKeyDown = 0;
    KeyAltShift = 0;
    last_x2 = 100;
    last_y2 = 100;
    last_fcolour = gui_fcolour;
    last_bcolour = gui_bcolour;
    last_inc = 1;
    last_min = -FLT_MAX;
    last_max = FLT_MAX;
	GUIactive=0;
	GuiPointerWanted = 0;
	GuiClickDown = GuiClickPhase = 0;                               // (a click cut short by an error)
    SetupPage = 0;
    CurrentPages = 1;
    for(i = 1; i < Option.MaxCtrls; i++) {
        if(Ctrl[i].s) FreeMemorySafe((void **)&Ctrl[i].s);
        if(Ctrl[i].fmt) FreeMemorySafe((void **)&Ctrl[i].fmt);
        memset(&Ctrl[i],0,sizeof(struct s_ctrl));
    }
}


// This is called by Timer.c every mSec
// it scans through all controls looking for a LED with a timeout set (in Ctrl[r].inc) and decrements it
void CheckGuiTimeouts(void) {
    int r;
    for(r = 1; r < Option.MaxCtrls; r++) {
        if(Ctrl[r].type == CTRL_LED && Ctrl[r].inc > 1) {
            Ctrl[r].inc--;                                          // decrement the timeout
        }
    }
}


// This implements a LED flash
// it is called by check_interrupt()
// it scans through all controls looking for a LED with a timeout set (in Ctrl[r].inc)
void CheckGui(void) {
    int r;
    CheckGuiFlag = false;
    for(r = 1; r < Option.MaxCtrls; r++) {
        if(Ctrl[r].type == CTRL_LED) {
            if(Ctrl[r].inc > 1)
                CheckGuiFlag = true;                                // keep calling this function while LEDs have timeouts
            if(Ctrl[r].inc == 1) {                                  // if this one has timed out
                Ctrl[r].inc = 0;
                Ctrl[r].value = 0;                                  // turn off the LED
                UpdateControl(r);
            }
        }
    }
}
