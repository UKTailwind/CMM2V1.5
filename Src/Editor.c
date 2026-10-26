/***********************************************************************************************************************
Editor.c - EDIT, the PicoMite's full-screen editor (misc/Editor.c without its MMBASIC_FM part), for the CMM2 in place
of kilo.c (Editor_Port_Plan.md; Peter, 19 Oct 2026). Made by tools/port_editor.py from the PicoMite's source, then
fixed by hand where the CMM2 differs; the platform layer is Inc/Editor_cmm2.h. It replaced kilo.c on 19 Oct 2026
(Editor_Port_Plan.md, S); kilo.c is in git history before then.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Editor_cmm2.h"
#include "UiMouse.h"                               // CMM2: the mouse, shared with FM

// (CMM2: where EDIT opens a program is StartEditLine and StartEditCharacter, Console.c, which error() sets; the
// PicoMite's StartEditPoint is a place in its program memory)
static bool markmode = false;

static int mark_lcd_reverse = 0;
static uint32_t mark_saved_fcolour = 0;
static uint32_t mark_saved_bcolour = 0;

int OriginalFC, OriginalBC; // the original fore/background colours used by MMBasic
static void (*PrintString)(char *buff) = EdSerialPutS;
static char (*SSputchar)(char buff, int flush) = EdSerialPutC;
#define MX470PutC(c) DisplayPutC(c);
// (CMM2: MX470Scroll is the display's own scroll, Editor_cmm2.h)

static inline void EdPutS(char *s, int fc, int bc)
{
    if (!(OptionConsole & 2))
        return;
    int tfc, tbc;
    tfc = gui_fcolour;
    tbc = gui_bcolour;
    gui_fcolour = fc;
    gui_bcolour = bc;
    DisplayPutS(s);
    gui_fcolour = tfc;
    gui_bcolour = tbc;
}

static inline void EdCursor(int x, int y)
{
    if (!(OptionConsole & 2))
        return;
    CurrentX = x;
    CurrentY = y;
}

static void EdDisplay(int fn)
{
    if (!(OptionConsole & 2))
        return;
    switch (fn)
    {
    case DISPLAY_CLS:
        ClearScreen(gui_bcolour);
        break;
    case REVERSE_VIDEO:
        if (!mark_lcd_reverse)
        {
            mark_saved_fcolour = gui_fcolour;
            mark_saved_bcolour = gui_bcolour;
            if (EdColourCode)
            {
                gui_bcolour = BLUE;
            }
            else
            {
                gui_fcolour = BLACK;
                gui_bcolour = WHITE;
            }
            mark_lcd_reverse = 1;
        }
        else
        {
            gui_fcolour = mark_saved_fcolour;
            gui_bcolour = mark_saved_bcolour;
            mark_lcd_reverse = 0;
        }
        break;
    case CLEAR_TO_EOL:
        DrawBox(CurrentX, CurrentY, EdHRes - 1, CurrentY + gui_font_height - 1, 0, 0, gui_bcolour);
        break;
    case CLEAR_TO_EOS:
        DrawBox(CurrentX, CurrentY, EdHRes - 1, CurrentY + gui_font_height - 1, 0, 0, gui_bcolour);
        DrawRectangle(0, CurrentY + gui_font_height, EdHRes - 1, VResEdit - 1, gui_bcolour);
        break;
    case SCROLL_DOWN:
        break;
    case DRAW_LINE:
        DrawBox(0, gui_font_height * (Option.Height - 2), EdHRes - 1, VResEdit - 1, 0, 0, gui_bcolour);
        DrawLine(0, (VResEdit / gui_font_height) * gui_font_height - gui_font_height - 6, EdHRes - 1, (VResEdit / gui_font_height) * gui_font_height - gui_font_height - 6, 1, EdColourCode ? GUI_C_LINE : gui_fcolour);
        CurrentX = 0;
        CurrentY = (VResEdit / gui_font_height) * gui_font_height - gui_font_height;
        break;
    }
}

/********************************************************************************************************************************************
 THE EDIT COMMAND
********************************************************************************************************************************************/

char *EdBuff = NULL; // the buffer used for editing the text
int nbrlines;                 // size of the text held in the buffer (in lines)
int VWidth, VHeight;          // editing screen width and height in characters
int edx, edy;                 // column and row at the top left hand corner of the editing screen (in characters)
int curx, cury;               // column and row of the current cursor (in characters) relative to the top left hand corner of the editing screen
char *txtp;          // position of the current cursor in the text being edited
int drawstatusline;           // true if the status line needs to be redrawn on the next keystroke
static bool edit_is_bas;      // true when editing a .bas file or in-memory program (enforces 255-char line limit)
int insert;                   // true if the editor is in insert text mode
int tempx;                    // used to track the prefered x position when up/down arrowing
int TextChanged;              // true if the program has been modified and therefor a save might be required

#define EDIT 1 // used to select the status line string
#define MARK 2

void FullScreenEditor(int x, int y, char *fname, int edit_buff_size);
char *findLine(int ln, int *inmulti);
void SetColour(char *p, int DoVT100);
void restoreColourFromLineStart(char *target);
void printLine(int ln);
void printScreen(void);
void SCursor(int x, int y);
int editInsertChar(unsigned char c, char *multi, int edit_buff_size);
void PrintFunctKeys(int);
void EdPrintStatus(void);
void editDisplayMsg(char *msg);
void GetInputString(char *prompt);
static void EdGetInput(char *prompt, char *def);
void Scroll(void);
void ScrollDown(void);
static void EdMarkScroll(int down);
void MarkMode(char *buf);
void PositionCursor(char *curp);
static int multilinecomment = false;
static char *EdFileName = "";                           // CMM2: the file being edited, for the status bar
/* CMM2: the message on the message line, while it is up (editDisplayMsg; PrintFunctKeys draws the legend over it), so
   that it can be put back after a redraw: mark mode's F10 export ends with one, which edit mode's redraw after mark
   mode hid */
static char EdLastMsg[128];
static int EdMsgUp;

/* ---- CMM2 (Editor_Port_Plan.md E6): where lines are, kept ---------------------------------------------------------
   The PicoMite's editor finds a line by counting the text's newlines from its start: findLine for every row drawn
   (twice: the display and the terminal), PositionCursor and editInsertChar for every key. Near the end of a 284 KB file
   a redraw took 0.4 s more than at its top. Places are kept instead, and counted on from:
   - findLine's: the screen's top line when it was last looked up, and the last line looked up, each with the block
     comment state at its start (EdFind[]);
   - the line of the place last asked for by EdLineOf (PositionCursor's): EdPosP, EdPosN.
   The text before a line does not change while the editor edits after it, so a kept place is dropped only when the
   text changes before it, or where its state is read from (EdChanged). */
static struct
{
    char *p;                                            // the start of line n (NULL: none kept)
    char *dep;                                          // how far its comment state was read from (its first characters)
    int n, m;                                           // the line, and findLine's inmulti at its start
} EdFind[2];                                            // [0] the screen's top line, [1] the last line looked up
static char *EdPosP;                                    // the start of line EdPosN (NULL: none kept)
static int EdPosN;

// none kept (a new text, or one rewritten whole)
static void EdCacheReset(void)
{
    EdFind[0].p = EdFind[1].p = NULL;
    EdPosP = NULL;
}

// the text changes at p (characters put in or taken out there): a place kept after it has moved
static void EdChanged(char *p)
{
    for (int i = 0; i < 2; i++)
        if (EdFind[i].p && p < EdFind[i].dep)
            EdFind[i].p = NULL;
    if (EdPosP && p < EdPosP)
        EdPosP = NULL;
}

// the line of at (a place in the text), and in *colp its column, counted from the kept place either way
static int EdLineOf(char *at, int *colp)
{
    char *p, *ls;
    int ln;
    if (EdPosP && at < EdPosP)
    {                                                   // back from the kept line: its number less the newlines
        for (p = at, ln = EdPosN; p < EdPosP; p++)       // between
            if (*p == '\n')
                ln--;
        for (ls = at; ls > EdBuff && ls[-1] != '\n'; ls--)
            ;
    }
    else
    {
        p = EdPosP ? EdPosP : EdBuff;
        ln = EdPosP ? EdPosN : 0;
        for (ls = p; p < at; p++)
            if (*p == '\n')
                ln++, ls = p + 1;
    }
    EdPosP = ls;
    EdPosN = ln;
    if (colp)
        *colp = at - ls;
    return ln;
}
#define MAXCLIP 1024
#define EDIT_BUFFER_BYTES (1024 * 1024)         // CMM2: the edit buffer, from the SDRAM heap (Editor_Port_Plan.md D5)

/* CMM2: the display as kilo set it up for EDIT (cmd_newedit): what the last program left (its variables, sprites, the
   I2C and the mouse) cleared away, the heap given back unless audio plays, then the mode, the editor's font and the
   CLUT, so that Option.Width and Option.Height are the display's columns and rows when it is a console. */
static void EdDisplaySetup(void)
{
    SerialClose(1);
    SerialClose(2);
    SerialClose(3);
    i2c_disable();
    i2c2_disable();
    i2c3_disable();
    mouse0release();                                    // (OPTION MOUSE 0's mouse goes on running)
    IrClose();                                          // (IR writes into a variable: off before they are cleared)
    ClearVars(0);
    closeallsprites();
    closeall3d();
    hidecursor(0);
    cursorenable = 0;
    FreeMemorySafe((void **)&cursorsave);
    SymBindFree();
    if (CurrentlyPlaying == P_NOTHING)
    {
        CloseAllFiles();
        InitHeap();
    }
    setmode(Option.mode, DEFCOLOUR, 0, Option.mode == 12 ? 1 : 0);
    SetFont(((Option.editfont - 1) << 4) | 1);
    reset_CLUT();
    pagesetdone = 0;
    if (Option.colourmode == -1)
    {
        CLUT[0] = 0xFFFFFF;
        CLUT[255] = 0;
        CLUT[3] = 0xFFFF;
        while (!pagesetdone)
            CheckAbort();
        HAL_LTDC_ConfigCLUT(&hltdc, CLUT, 256, 0);
    }
}

// CMM2: and back as the prompt has it (kilo's way out)
static void EdDisplayRestore(void)
{
    gui_fcolour = PromptFC;
    gui_bcolour = PromptBC;
    MX470Display(DISPLAY_CLS);
    MX470Cursor(0, 0);
    SetFont(Option.DefaultFont);
    setterminal();
    setmode(Option.mode, DEFCOLOUR, 0, Option.mode == 12 ? 1 : 0);
    reset_CLUT();
}

/* CMM2: the editor's keys are read here, in any state of the editor (editing, mark mode and the prompts):
   - Enter is '\n', whatever the terminal sends: a CR is Enter, and the LF just after it (a terminal's CR LF, or the
     one MMInkey makes 2 ms after a CR when nothing else has come) is dropped, as MMgetchar drops it. Waiting for that
     LF alone would lose the Enters of a CR typed ahead or pasted with more after it.
   - Print Screen (0x9D) and Ctrl-\ (0x1C) save the display (ScreenShot, Console.c) and are not keys. */
/* keys looked at and given back: to MMInkey (main.c), which returns them before any other, so that those typed after
   an Esc that leaves the editor reach the prompt (as kilo's did, through MMInkey's own look-ahead). A queue of the
   editor's own kept them for its next session instead, where they were typed into the file. */
#define EdUnget(c) MMInkeyUnget(c)

static int EdKeyNow = -1;                               // the key being handled (-1 until the first: EdDropHeldRepeats)
// the byte at *pos in the console queue, left there; -1 at the end
static int EdQueueByte(int *pos, int head)
{
    if (*pos == head)
        return -1;
    int c = (unsigned char)ConsoleRxBuf[*pos];
    *pos = (*pos + 1) % CONSOLE_RX_BUF_SIZE;
    return c;
}
/* The key at *pos in the console queue, decoded as MMInkey decodes it but left in the queue; *pos moves past it. -2 for
   an escape sequence this does not decode, or one that has not all arrived (the PicoMite's EdPeekKey). */
static int EdPeekKey(int *pos, int head)
{
    int c = EdQueueByte(pos, head);
    if (c != ESC)
        return c;
    c = EdQueueByte(pos, head);
    if (c == 'O')
    {
        c = EdQueueByte(pos, head);
        return (c >= 'P' && c <= 'T') ? F1 + (c - 'P') : -2;
    }
    if (c != '[')
        return -2;
    c = EdQueueByte(pos, head);
    if (c >= 'A' && c <= 'D')
    {
        static const int arrow_keys[] = {UP, DOWN, RIGHT, LEFT};
        return arrow_keys[c - 'A'];
    }
    if (c >= '1' && c <= '6' && EdQueueByte(pos, head) == '~')
    {
        static const int four_char_keys[] = {HOME, INSERT, DEL, END, PUP, PDOWN};
        return four_char_keys[c - '1'];
    }
    return -2;
}
/* After a redraw: the keys that arrived meanwhile are dropped only when every one of them is the key that caused it, a
   held key repeating, which would otherwise run on after it is let go. Anything else, typing after Enter or a paste, is
   kept (the PicoMite's EdDropHeldRepeats). Before the first key EDIT's start has emptied the queue already. */
static void EdDropHeldRepeats(void)
{
    int head = ConsoleRxBufHead, pos = ConsoleRxBufTail, n = 0;
    if (EdKeyNow == -1)
        return;
    while (pos != head)
    {
        int start = pos;
        if (EdPeekKey(&pos, head) != EdKeyNow)
            return;
        n += (pos - start + CONSOLE_RX_BUF_SIZE) % CONSOLE_RX_BUF_SIZE;
    }
    while (n--)
        getConsole();
}

static int EdInkey(void)
{
    static int lastcr = 0;
    int c = MMInkey();
    if (c == -1)
        return -1;
    if (KEY_IS_PRTSCR(c))
    {
        ScreenShot();
        return -1;
    }
    if (c == '\n' && lastcr)
    {
        lastcr = 0;
        return -1;
    }
    lastcr = (c == '\r');
    return lastcr ? '\n' : c;
}

/* CMM2: the mouse (Editor_Port_Plan.md D2), with OPTION MOUSE set and the display a console. A USB mouse, when one is
   plugged in, is the one read: only its channel, 2, can be open then. Its state is mousestruct[] in the display's pixels.
   While the editor waits for a key:
   - the pointer (the GUI cursor) follows it, and is hidden while the editor draws;
   - the wheel is Up or Down, and a double click at the left or right edge Home or End (kilo's);
   - left: the cursor to the character under the pointer; in mark mode, the selection's end;
   - left held for 0.5 s, or dragged to another character: mark mode, the selection following the pointer while the
     button is down (kilo's drag, which needed the hold);
   - right: mark mode from the pointer; in mark mode, cut (F4);
   - middle: paste at the pointer (F5); in mark mode, copy (F5).
   The PicoMite's right and middle buttons extend a selection to the pointer before the cut or the copy; here they act
   on the selection as it is, the left button being the one that moves it. The mouse is opened, its pointer drawn and a
   terminal's reports read by UiMouse.c, which FM shares (22 Oct 2026). */
#define EDMOUSE_CELL 9999                               // (not a key) the left button at EdMouseX, EdMouseY
static int EdMouseX, EdMouseY;                          // the editing area's cell under the pointer

/* what the mouse has done since it was last read, as a key (UP, DOWN, HOME, END, F4, F5), EDMOUSE_CELL, or -1. A key
   that follows a move of the cursor (right and middle in edit mode) is given back (EdUnget) for the next read. */
static int EdMouse(int marking)
{
    static int lastL, lastR, lastC, drag, held;
    static unsigned long long pressed;
    volatile struct s_nunstruct *m;
    int x, y, k = -1, inside, right, middle;
    if (UiMouseChan < 0)
        return -1;
    m = &mousestruct[UiMouseChan];
    x = m->ax / gui_font_width;
    y = m->ay / gui_font_height;
    if (x >= VWidth)
        x = VWidth - 1;
    inside = (y < VHeight);                             // (not on the status bar or the message line)
    if (m->az)
    {                                                   // the wheel: the PS/2 and I2C mice count up toward the user, a
        int down = (UiMouseChan == 2 && UsbMouse) ? m->az < 0 : m->az > 0;   // USB mouse (HID) away from them
        m->az = 0;
        return down ? DOWN : UP;
    }
    if (m->Z)
    {                                                   // a double click
        m->Z = 0;
        if (inside && m->ax == 0)
            k = HOME;
        else if (inside && m->ax >= EdHRes - 1)
            k = END;
    }
    if (m->L && !lastL)
    {                                                   // left pressed
        lastL = 1;
        held = 0;
        pressed = mSecTimer;
        drag = (k == -1 && inside);                     // (from the cell EdMouseX, EdMouseY)
        if (drag)
        {
            EdMouseX = x;
            EdMouseY = y;
            k = EDMOUSE_CELL;
        }
    }
    else if (!m->L)
        lastL = drag = 0;
    else if (drag && inside)
    {                                                   // left held down
        int moved = (x != EdMouseX || y != EdMouseY);
        if (!marking && !held && (moved || mSecTimer - pressed >= 500))
        {
            held = 1;
            k = F4;                                     // mark mode from the press, where the cursor is
        }
        else if (marking && moved)
        {
            EdMouseX = x;
            EdMouseY = y;
            k = EDMOUSE_CELL;
        }
    }
    right = m->R && !lastR;
    middle = m->C && !lastC;
    lastR = m->R;
    lastC = m->C;
    if (k == -1 && (right || middle) && inside)         // (on the status bar or the message line, nothing)
    {
        k = right ? F4 : F5;
        if (!marking)
        {                                               // in edit mode, at the pointer
            EdUnget(k);
            EdMouseX = x;
            EdMouseY = y;
            k = EDMOUSE_CELL;
        }
    }
    return k;
}

/* the text at the editing area's cell (x, y), found from p, the text in row cury; curx and cury become the cell's.
   The PicoMite's, for its mouse and Tera Term's clicks. */
static char *EdTextAt(char *p, int x, int y)
{
    while (*p != 0 && y > cury)
        if (*p++ == '\n')
            cury++;
    while (p != EdBuff && y < cury)
        if (*--p == '\n')
            cury--;
    while (p != EdBuff && *(p - 1) != '\n')
        p--;
    for (curx = 0; curx < edx + x && *p && *p != '\n'; curx++)
        p++;
    curx -= edx;
    if (curx < 0)
        curx = 0;
    return p;
}

/* edit mode: the cursor to the cell (x, y). Only the cursor's line is scrolled sideways (as Up and Down leave it), so
   for a click on another line the old one goes back to its start first. */
static void EdClick(int x, int y)
{
    if (y != cury && edx != 0)
    {
        edx = 0;
        SCursor(0, cury);
        printLine(edy + cury);
    }
    txtp = EdTextAt(txtp, x, y);
    PositionCursor(txtp);
    tempx = curx;
    EdPrintStatus();
}

/* CMM2 (kilo's two-stage PgUp and PgDn, Peter 19 Oct): the cursor to row y of the screen, or the last line if the
   text ends before it, at the column Up and Down keep (tempx, with the line not scrolled sideways, as they leave it) */
static void EdToRow(int y)
{
    if (edx != 0)
    {
        edx = 0;
        SCursor(0, cury);
        printLine(edy + cury);
    }
    txtp = EdTextAt(txtp, 0, y);
    for (curx = 0; curx < tempx && *txtp && *txtp != '\n'; curx++)
        txtp++;
    PositionCursor(txtp);
}

/* ---- CMM2 (Editor_Port_Plan.md E4): the clipboard, as large as what it holds ---------------------------------------
   The PicoMite's is 1 KB ("MARKED TEXT EXCEEDS BUFFER SIZE"). Temporary memory, so that it is freed at the prompt
   however the editor ends. At least 8 KB + 1, so that it comes from the heap (GetTempMemory takes smaller blocks from
   the internal heap), whose free space is checked first: GetMemory throws when there is no room. */
static char *EdClip;                                    // the clipboard's text, NULL when it is empty
static int EdClipLen;

// the clipboard set to len characters from from; 0, and the clipboard empty, when the heap has no room for them
static int EdSetClip(char *from, int len)
{
    int size = len + 1 > 8193 ? len + 1 : 8193;
    if (EdClip)
        ClearSpecificTempMemory(EdClip);
    EdClip = NULL;
    EdClipLen = 0;
    if (FreeSpaceOnHeap() < size + 8192)
        return 0;
    EdClip = GetTempMemory(size);
    memcpy(EdClip, from, len);
    EdClip[len] = 0;
    EdClipLen = len;
    return 1;
}

/* after a change that may have moved the cursor anywhere: the screen redrawn with the cursor's line on it (centred if
   it was off the screen), scrolled sideways if the cursor, and the ahead characters after it (a search's match), are
   past the right edge (only the cursor's line, as the editor does elsewhere) */
static void EdShowCursor(int ahead)
{
    int col, ln = EdLineOf(txtp, &col);                 // (E6)
    if (ln < edy || ln >= edy + VHeight)
    {
        edy = ln - VHeight / 2;
        if (edy < 0)
            edy = 0;
    }
    edx = 0;
    printScreen();
    if (col + ahead > VWidth - 1)
    {
        edx = col + ahead - (VWidth - 1 - SCROLLCHARS);
        if (edx > col)
            edx = col;                                  // (the cursor itself on the screen)
        SCursor(0, ln - edy);
        printLine(ln);
    }
    PositionCursor(txtp);
}

/* the clipboard put in at the cursor in one go, the cursor after it. The PicoMite's paste fed it through the
   type-ahead buffer a key at a time, scanning the whole text and redrawing for each, and a TAB in it became spaces. */
static void EdPaste(int edit_buff_size)
{
    char *end = txtp + strlen(txtp);
    int i;
    if ((end - EdBuff) + EdClipLen >= edit_buff_size - 10)
    {
        editDisplayMsg(" OUT OF MEMORY ");
        return;
    }
    EdChanged(txtp);                                    // (E6: the lines kept after it move)
    memmove(txtp + EdClipLen, txtp, end - txtp + 1);     // (the zero after the text too)
    memcpy(txtp, EdClip, EdClipLen);
    for (i = 0; i < EdClipLen; i++)
        if (EdClip[i] == '\n')
            nbrlines++;
    txtp += EdClipLen;
    TextChanged = true;
    EdShowCursor(0);
}

// CMM2: is there a file (not a directory) of this name, and its size
static int EdFileExists(char *name, FSIZE_t *size)
{
    FILINFO fno;
    if (f_stat(name, &fno) != FR_OK || (fno.fattrib & AM_DIR))
        return 0;
    if (size)
        *size = fno.fsize;
    return 1;
}

// CMM2: name with .bas added when it has no extension (the PicoMite's AppendDefaultExtension)
static void EdDefaultExtension(char *name)
{
    char *s = strrchr(name, '/');
    if (strchr(s ? s : name, '.') == NULL && strlen(name) + 4 < FF_MAX_LFN)
        strcat(name, ".bas");
}

/* ---- CMM2 (Editor_Port_Plan.md E3): files written and read without leaving the editor ----------------------------
   With FatFs's own calls, which return an error, so that a failure is reported on the message line and the editor
   stays open with the text. The PicoMite's BasicFileOpen, FilePutChar and FileClose end in error() on a FatFs error,
   which left the editor and lost the edits (its save did not look at what BasicFileOpen returned either). */
#define EDIO_BYTES 4096                                 // (files are written and read this much at a time)

// the drive a name is on: its own, or the current one
static char EdDrive(char *name)
{
    return toupper((name[0] && name[1] == ':') ? name[0] : filepath[0]);
}

// open name, its drive checked first: FatFs does not know a card has gone until it reads it
static FRESULT EdOpen(FIL *fp, char *name, BYTE mode)
{
    if (EdDrive(name) == 'C' ? !usb_msc_is_mounted() : !MDD_SDSPI_CardDetectState())
        return FR_NOT_READY;
    return f_open(fp, name, mode);
}

// what, " what: the reason " on the message line, cut to the screen's width (a longer one wraps and scrolls it)
static void EdFileMsg(char *what, FRESULT r, char *name)
{
    char msg[128];
    const char *why = r == FR_NOT_READY ? FErrorMsg[EdDrive(name) == 'C' ? 21 : 20] :
                      (r > 0 && r < 20 ? FErrorMsg[r] : "error");
    snprintf(msg, sizeof(msg), " %s: %s ", what, why);
    if (VWidth > 1 && VWidth - 1 < (int)sizeof(msg))
        msg[VWidth - 1] = 0;
    editDisplayMsg(msg);
}

/* write the text from from to end as the file name, each '\n' as CR LF, and with crlf a CR LF after the last line;
   FR_OK, or what failed (the file is then deleted) */
static FRESULT EdWriteFile(char *name, char *from, char *end, int crlf)
{
    FIL *fp = GetMemory(sizeof(FIL) + EDIO_BYTES);
    char *b = (char *)(fp + 1);
    UINT n = 0, w;
    FRESULT r = EdOpen(fp, name, FA_WRITE | FA_CREATE_ALWAYS), rc;
    if (r == FR_OK)
    {
        while (r == FR_OK && (from < end || crlf || n))
        {
            if (from < end)
            {
                if (*from == '\n')
                    b[n++] = '\r';
                b[n++] = *from++;
            }
            else if (crlf)
            {
                b[n++] = '\r';
                b[n++] = '\n';
                crlf = 0;
            }
            if (n >= EDIO_BYTES - 2 || (from >= end && !crlf && n))
            {
                r = f_write(fp, b, n, &w);
                if (r == FR_OK && w != n)
                    r = FR_DENIED;                      // (the disk is full)
                n = 0;
            }
        }
        rc = f_close(fp);
        if (r == FR_OK)
            r = rc;
        if (r != FR_OK)
            f_unlink(name);
    }
    FreeMemory(fp);
    return r;
}

/* the text saved as fname: written as fname.$$$, then the old file kept as fname.bak (an older .bak deleted) and the
   new one renamed fname. Whatever fails, the file or its .bak is as it was, and the text is still in the editor. */
static FRESULT EdSave(char *fname)
{
    char tmp[FF_MAX_LFN + 8], bak[FF_MAX_LFN + 8];
    FRESULT r;
    if (strlen(fname) + 5 > FF_MAX_LFN)
        return FR_INVALID_NAME;
    sprintf(tmp, "%s.$$$", fname);
    sprintf(bak, "%s.bak", fname);
    if ((r = EdWriteFile(tmp, EdBuff, EdBuff + strlen(EdBuff), *EdBuff != 0)) != FR_OK)   // (an empty text is an empty file)
        return r;
    if (EdFileExists(fname, NULL))
    {
        if (EdFileExists(bak, NULL) && (r = f_unlink(bak)) != FR_OK)
            goto failed;
        if ((r = f_rename(fname, bak)) != FR_OK)
            goto failed;
    }
    if ((r = f_rename(tmp, fname)) == FR_OK)
        return FR_OK;
failed:
    f_unlink(tmp);
    return r;
}

/* ---- CMM2 (Editor_Port_Plan.md E5): help on F11 (Peter, 19 Oct) ---------------------------------------------------
   The word at the cursor looked up as HELP does: in A:/help.txt when the card has one (each topic a line ~NAME and the
   text after it), otherwise in the built-in help (help.h: a name and its syntax). Every topic named the word, or the
   word and more after a space (DIM FLOAT for DIM), is shown in the editing area, which Up, Down, PgUp and PgDn
   scroll; Esc or F11 goes back to the text as it was. kilo's was on F12, the PicoMite's Beautify. */
#define EDHELP_BYTES 32768                              // (the most topic text shown at once)

// the name at the cursor, or just before it, with its type suffix, in w; its length (0: none)
static int EdHelpWord(char *w, int size)
{
    char *s = txtp;
    int n = 0;
    while (s > EdBuff && isnamechar(s[-1]))
        s--;
    if (!isnamestart(*s))
        return 0;
    while (isnamechar(*s) && n < size - 2)
        w[n++] = *s++;
    if (*s == '$' || *s == '%' || *s == '!')
        w[n++] = *s;
    w[n] = 0;
    return n;
}

/* is a topic's name (len characters) the word, or the word and more after a space? Blind to case; a name's trailing
   spaces and '(' (the built-in help's: "LEFT$(  ") left out */
static int EdHelpMatch(const char *name, int len, const char *w, int wl)
{
    while (len > 0 && (name[len - 1] == ' ' || name[len - 1] == '('))
        len--;
    if (len < wl || strncasecmp(name, w, wl))
        return 0;
    return len == wl || name[wl] == ' ';
}

// the topics' text for the word w, each line ended by '\n', a blank line between topics, in out; its length
static int EdHelpText(char *w, char *out, int size)
{
    int wl = strlen(w), n = 0, in = 0, ll = 0, i;
    char line[STRINGSIZE];
    if (EdFileExists("A:/help.txt", NULL))
    {
        FIL *fp = GetMemory(sizeof(FIL) + EDIO_BYTES);
        char *b = (char *)(fp + 1);
        UINT got = 0;
        if (EdOpen(fp, "A:/help.txt", FA_READ) == FR_OK)
        {
            do
            {
                if (f_read(fp, b, EDIO_BYTES, &got) != FR_OK)
                    got = 0;
                for (UINT k = 0; k <= got; k++)
                {
                    char c = k < got ? b[k] : (got < EDIO_BYTES && ll ? '\n' : 0);   // (a last line with no end)
                    if (c == 0 || c == '\r')
                        continue;
                    if (c != '\n')
                    {
                        if (ll < STRINGSIZE - 1)
                            line[ll++] = c;
                        continue;
                    }
                    line[ll] = 0;
                    ll = 0;
                    if (line[0] == '~')
                    {
                        if ((in = EdHelpMatch(line + 1, strlen(line + 1), w, wl)) && n && n < size - 1)
                            out[n++] = '\n';
                        continue;
                    }
                    for (i = 0; in && line[i] && n < size - 2; i++)
                        out[n++] = line[i];
                    if (in && n < size - 1)
                        out[n++] = '\n';
                }
            } while (got == EDIO_BYTES);
            f_close(fp);
        }
        FreeMemory(fp);
    }
    else
        for (i = 0; *help[i] != 'Z'; i += 2)
            if (EdHelpMatch(help[i], strlen(help[i]), w, wl) && n + strlen(help[i + 1]) + 1 < size)
            {
                strcpy(out + n, help[i + 1]);
                n += strlen(help[i + 1]);
                out[n++] = '\n';
            }
    out[n] = 0;
    return n;
}

// one row of the screen: s on the display and the terminal, cleared to the end; reverse for the title row (s, a row[]
// of 256, padded to the width, so that the reverse bar covers the status bar that was there)
static void EdHelpRow(int y, char *s, int reverse)
{
    if (reverse)
    {
        int n = strlen(s);
        while (n < VWidth && n < 255)
            s[n++] = ' ';
        s[n] = 0;
    }
    SCursor(0, y);
    PrintString(reverse ? "\033[7m" : "\033[0m");
    PrintString(s);
    PrintString("\033[K\033[0m");
    EdPutS(s, reverse ? BLACK : GUI_C_NORMAL, reverse ? WHITE : GUI_C_BCOLOUR);
    if (!reverse)
        EdDisplay(CLEAR_TO_EOL);
}

// F11: the help page for the word at the cursor, until Esc or F11
static void EdHelp(void)
{
    char w[64], row[256], *text;
    int *start, len, nl = 0, top = 0, i, r, c, width = VWidth - 1 < (int)sizeof(row) - 1 ? VWidth - 1 : (int)sizeof(row) - 1;
    if (!EdHelpWord(w, sizeof(w)))
    {
        editDisplayMsg(" NO WORD AT THE CURSOR ");
        return;
    }
    text = GetTempMemory(EDHELP_BYTES);
    if ((len = EdHelpText(w, text, EDHELP_BYTES)) == 0)
    {
        ClearSpecificTempMemory(text);
        snprintf(row, sizeof(row), " NO HELP FOR %.40s ", w);
        editDisplayMsg(row);
        return;
    }
    for (i = 0, c = 0; i < len; i++)                    // the rows: each line of the text cut at the screen's width
        if (text[i] == '\n' || ++c == width)
            nl++, c = 0;
    start = GetTempMemory((nl + 2) * sizeof(int));
    for (i = 0, c = 0, nl = 0, start[0] = 0; i < len; i++)
        if (text[i] == '\n' || ++c == width)
            start[++nl] = i + 1, c = 0;
    while (1)
    {
        for (r = 0; r < VHeight; r++)
        {
            int k = 0;
            if (top + r < nl)
                for (i = start[top + r]; i < len && text[i] != '\n' && k < width && i < start[top + r + 1]; i++)
                    row[k++] = text[i];
            row[k] = 0;
            EdHelpRow(r, row, 0);
        }
        snprintf(row, sizeof(row), " HELP: %.40s   %d-%d of %d ", w, top + 1, top + VHeight < nl ? top + VHeight : nl, nl);
        row[width] = 0;
        EdHelpRow(VHeight, row, 1);
        strcpy(row, "Esc:Back  Up Dn PgUp PgDn:Scroll");
        row[width] = 0;
        EdHelpRow(VHeight + 1, row, 0);
        c = EdGetKey();
        if (c == ESC)
        {
            uSec(50000);
            if (UiMouseReport())
            {                                           // (a terminal's mouse report: ignored)
                MMInkey();
                MMInkey();
                MMInkey();
                continue;
            }
            break;
        }
        if (c == F11)
            break;
        if (c == UP || c == CTRLKEY('E'))
            top--;
        else if (c == DOWN || c == CTRLKEY('X'))
            top++;
        else if (c == PUP || c == CTRLKEY('P'))
            top -= VHeight;
        else if (c == PDOWN || c == CTRLKEY('L'))
            top += VHeight;
        if (top > nl - VHeight)
            top = nl - VHeight;
        if (top < 0)
            top = 0;
    }
    ClearSpecificTempMemory(start);
    ClearSpecificTempMemory(text);
    EdDisplay(DISPLAY_CLS);
    printScreen();
    PrintFunctKeys(EDIT);
    PositionCursor(txtp);
}

/* EDIT [file$] (CMM2): the file, or with none the program's own file (ProgName) at the line an error left in it
   (StartEditLine and StartEditCharacter, which error() sets); FM's EDIT opens at the line its error status names
   (fm_pending_edit_seek_valid). A file that is not there is made by the first save, and a path where it could not be
   is refused before the screen changes (kilo made the file and killed it to prove the path). The PicoMite's EDIT edits
   its program memory, and its EDIT FILE a file with the program's variables kept: the CMM2 has neither (D4). */
static void edit(char *cmdline)
{
    char *p = NULL;
    int y = 0, x = 0, edit_buff_size = EDIT_BUFFER_BYTES;
    int seekline = 0, seekchar = 0;
    char filename[FF_MAX_LFN] = {0};
    FSIZE_t fsize = 0;
    int exists;

    if (CurrentLinePtr)
        error("Invalid in a program");
    if (!(*cmdline == 0 || *cmdline == '\''))
    {
        char *c = getCstring(cmdline);
        if (strlen(c) >= FF_MAX_LFN - 5)
            error("Pathname too long");
        getfullfilepath(c, filename);
        if (!InitSDCard())
            return;
        if (strchr(filename, '.') == NULL && !EdFileExists(filename, NULL))
            strcat(filename, ".BAS");                   // (as kilo: a name that is not there without .BAS gets it)
        if (fm_pending_edit_seek_valid)
        {
            seekline = fm_pending_edit_seek_line;
            seekchar = fm_pending_edit_seek_char;
            fm_pending_edit_seek_valid = 0;
        }
    }
    else
    {
        char *c = ProgName();
        if (c == NULL)
            error("Nothing to edit");
        strcpy(filename, c);
        if (!InitSDCard())
            return;
        if (!EdFileExists(filename, NULL))
            error("File not found");
        seekline = StartEditLine;
        seekchar = StartEditCharacter;
    }
    exists = EdFileExists(filename, &fsize);
    if (exists && fsize > edit_buff_size - 10)
        error("File too large to edit");
    if (!exists)
    {                                                   // a new file: its path proved now, while the prompt is there
        int fnbr = FindFreeFileNbr();
        BasicFileOpen(filename, fnbr, FA_WRITE | FA_CREATE_ALWAYS);
        FileClose(fnbr);
        f_unlink(filename);
    }

    EdDisplaySetup();
    sendCRLF = 3;                                       // (kilo's: the keyboard's Enter as CR LF, and the prompt's status
    docheck = 1;                                        // line to look for the program's file again)
    MM_Delay(3);                                        // CMM2: the keys typed with EDIT gone, its Enter's '\n' too (MMInkey
    while (MMInkey() != -1)                             // makes one 2 ms after a CR): as kilo's cmd_newedit
        ;
    EdColourCode = (Option.colourmode == 1) && fstrstr(filename, ".bas") != NULL;
    multilinecomment = false;
    EdClip = NULL;                                      // (E4: empty; a clipboard of an earlier EDIT went at the prompt)
    EdClipLen = 0;
    EdCacheReset();                                     // (E6: no line kept from an earlier EDIT's text)
    EdBuff = GetTempMemory(edit_buff_size);             // (from the heap, which EdDisplaySetup gave back; freed at the prompt)
    *EdBuff = 0;
    nbrlines = 0;

    VHeight = Option.Height - 2;
    VWidth = Option.Width;
    edx = edy = curx = cury = y = x = tempx = 0;
    txtp = EdBuff;
    *tknbuf = 0;
    p = EdBuff;
    if (exists)
    {
        char c;
        int fnbr1 = FindFreeFileNbr();
        BasicFileOpen(filename, fnbr1, FA_READ);
        while (!FileEOF(fnbr1))
        { // while waiting for the end of file
            c = FileGetChar(fnbr1);
            if (c == '\n')
                nbrlines++;
            if (c == '\r')
                continue;
            *p++ = c;
        }
        FileClose(fnbr1);
    }
    /* CMM2: the last line's terminator is not kept (the save writes it back), or the text would end in an empty line
       that is not in the file, and that the cursor could go to; a last line without one is a line as well */
    if (p > EdBuff && p[-1] == '\n')
        --p;
    else if (p > EdBuff)
        nbrlines++;
    *p = 0;
    txtp = EdBuff;
    tempx = x = 0;
    {                                                   // to the line and column the editor opens at
        if (seekline < 0)
            seekline = 0;
        if (seekchar < 0)
            seekchar = 0;
        y = 0;
        while (*txtp && y < seekline)
        {
            if (*txtp == '\n')
                y++;
            txtp++;
        }
        x = 0;
        while (*txtp && *txtp != '\n' && x < seekchar)
        {
            txtp++;
            x++;
        }
        tempx = x;
    }
    if (nbrlines == 0)
        nbrlines++;
    UiMouseOpen();                                      // CMM2: (after the buffer and the load, which can end in an error)
    setterminal();                                      // CMM2: the serial terminal asked to be the console's size
    UiTermMouse(1);                    // Tera Term turn on mouse click report in VT200 mode
    PrintString("\033[?7l");           // disable autowrap while in editor
    PrintString("\0337\033[2J\033[H"); // vt100 clear screen and home cursor
    EdDisplay(DISPLAY_CLS);         // clear screen on the MX470 display only
    SCursor(0, 0);
    PrintFunctKeys(EDIT);

    if (nbrlines > VHeight)
    {
        edy = y - VHeight / 2; // edy is the line displayed at the top
        if (edy < 0)
            edy = 0; // compensate if we are near the start
        y = y - edy; // y is the line on the screen
    }
    FullScreenEditor(x, y, filename, edit_buff_size);
    MMCharPos = 0;
}

/*  @endcond */
// EDIT [file$] (the CMM2's name for it: kilo's command was cmd_newedit)
void cmd_newedit(void)
{
    edit(cmdline);
}


int find_longest_line_length(const char *text, int *linein)
{
    int current_length = 0;
    int max_length = 0;
    const char *ptr = text;
    int line = 0;
    while (*ptr)
    {
        if (*ptr == '\n')
        {
            line++;
            if (ptr > text && *(ptr - 1) == '_' && *(ptr - 2) == ' ' && 0)
            {
                // Line continuation, do not reset length
            }
            else
            {
                // If this line exceeds the max, update
                if (current_length > max_length)
                {
                    max_length = current_length;
                    *linein = line;
                }
                current_length = 0; // Reset for a new line
            }
        }
        else
        {
            // Increase length for this segment of the line
            current_length++;
        }

        ptr++;
    }

    // Final check in case the last line was the longest
    if (current_length > max_length)
    {
        max_length = current_length;
        *linein = line + 1;                             // (CMM2: its number as well; the text's last line has no '\n')
    }

    return max_length;
}
/*
 * @cond
 * The following section will be excluded from the documentation.
 */

/* ----------------------------------------------------------------------------
 * editBeautify - Re-indent the editor buffer with 2-space block indentation.
 *
 * Recognised block structures:
 *   IF ... THEN  /  ELSE / ELSEIF / END IF / ENDIF       (multi-line only)
 *   FOR ... NEXT
 *   DO[/WHILE/UNTIL] ... LOOP[/WHILE/UNTIL]
 *   SELECT CASE ... CASE / CASE ELSE ... END SELECT
 *   FUNCTION ... END FUNCTION
 *   SUB ... END SUB
 *   TYPE ... END TYPE                                    (STRUCTENABLED only)
 *
 * Single-line IF (with statements after THEN) is left at the prevailing
 * indent without opening a block.  Blank lines are emitted with no indent.
 * Comment-only lines are indented at the current level.
 * --------------------------------------------------------------------------*/
static int beautify_token_match(const char *up, int idx, const char *kw)
{
    int klen = (int)strlen(kw);
    for (int i = 0; i < klen; i++)
    {
        if (up[idx + i] != kw[i])
            return 0;
    }
    char nxt = up[idx + klen];
    return (nxt == 0 || nxt == ' ' || nxt == '\t' || nxt == ':');
}

/* Returns non-zero if the buffer contains any blank line that is NOT
 * immediately preceded by an END SUB / END FUNCTION line.  Used by the
 * F12 handler to decide whether to prompt the user about preserving
 * blank lines. */
static int beautify_has_stray_blanks(void)
{
    char *p = EdBuff;
    int prev_was_end_sub_or_func = 0;
    while (*p)
    {
        char *line_start = p;
        char *eol = p;
        while (*eol && *eol != '\n')
            eol++;

        char *t = line_start;
        while (t < eol && (*t == ' ' || *t == '\t'))
            t++;
        int blank = (t >= eol);

        if (blank)
        {
            if (!prev_was_end_sub_or_func)
                return 1;
            prev_was_end_sub_or_func = 0; /* don't allow a 2nd freebie */
        }
        else
        {
            char up[16] = {0};
            int ulen = 0;
            for (char *q = t; q < eol && ulen < (int)sizeof(up) - 1; q++)
            {
                char ch = (char)*q;
                if (ch >= 'a' && ch <= 'z')
                    ch = (char)(ch - 32);
                up[ulen++] = ch;
            }
            prev_was_end_sub_or_func =
                (beautify_token_match(up, 0, "END") &&
                 (beautify_token_match(up, 4, "SUB") ||
                  beautify_token_match(up, 4, "FUNCTION")));
        }
        p = eol + (*eol == '\n' ? 1 : 0);
    }
    return 0;
}

static void editBeautify(int edit_buff_size, int keep_blanks)
{
    int line_number_base = 0; /* if >0, programs uses line numbers and this
                               * is the column at which statements start
                               * (= longest line-number width + 1).        */
    /* ---- Pass 1: optionally remove blank/whitespace-only lines, and at the
     *               same time work out whether the program is line-numbered.
     *               It is considered numbered iff the first non-blank,
     *               non-comment line begins with a run of digits followed by
     *               whitespace, ':' or end-of-line.  When that is the case
     *               we record the width of the longest such leading number
     *               so that all subsequent lines can be aligned past it.
     *               When keep_blanks is non-zero we leave blank lines in
     *               place (they are still emitted to the output buffer) but
     *               otherwise behave identically.                          */
    {
        char *src = EdBuff;
        char *dst = EdBuff;
        int saw_first = 0;
        int numbered_program = 0;
        int max_num_len = 0;
        int prev_emitted_blank = 0;
        while (*src)
        {
            char *line_start = src;
            char *eol = src;
            while (*eol && *eol != '\n')
                eol++;
            char *t = line_start;
            while (t < eol && (*t == ' ' || *t == '\t'))
                t++;
            int blank = (t >= eol);
            int line_len = (int)(eol - line_start) + (*eol == '\n' ? 1 : 0);
            if (blank)
            {
                if (keep_blanks && !prev_emitted_blank)
                {
                    /* Emit a single normalised blank line ('\n').  Because
                     * dst <= line_start (we only ever shrink), writing one
                     * byte is always safe. */
                    *dst++ = '\n';
                    prev_emitted_blank = 1;
                }
                /* otherwise: drop the blank line entirely */
            }
            else
            {
                /* Is this line a comment-only line? */
                int is_cmt = 0;
                if (*t == '\'')
                    is_cmt = 1;
                else if ((eol - t) >= 3 &&
                         (t[0] == 'R' || t[0] == 'r') &&
                         (t[1] == 'E' || t[1] == 'e') &&
                         (t[2] == 'M' || t[2] == 'm') &&
                         ((eol - t) == 3 || t[3] == ' ' || t[3] == '\t'))
                    is_cmt = 1;

                /* Measure leading line-number, if any. */
                int num_len = 0;
                {
                    const char *r = t;
                    while (r < eol && *r >= '0' && *r <= '9')
                        r++;
                    if (r > t && (r >= eol || *r == ' ' || *r == '\t' ||
                                  *r == ':'))
                        num_len = (int)(r - t);
                }
                if (!saw_first && !is_cmt)
                {
                    saw_first = 1;
                    if (num_len > 0)
                        numbered_program = 1;
                }
                if (numbered_program && num_len > max_num_len)
                    max_num_len = num_len;

                if (dst != line_start)
                    memmove(dst, line_start, line_len);
                dst += line_len;
                prev_emitted_blank = 0;
            }
            src = line_start + line_len;
        }
        *dst = 0;
        if (numbered_program)
            line_number_base = max_num_len + 1;
    }

    /* ---- Pass 2: insert one blank line after each END SUB / END FUNCTION
     *               (except at end of buffer).  Inserting after the end of
     *               a routine - rather than before its SUB/FUNCTION header -
     *               keeps any leading comment block attached to the routine
     *               that it documents.                                     */
    {
        char *p = EdBuff;
        while (*p)
        {
            char *line_start = p;
            char *eol = p;
            while (*eol && *eol != '\n')
                eol++;

            char *s = line_start;
            while (s < eol && (*s == ' ' || *s == '\t'))
                s++;

            char up[16] = {0};
            int ulen = 0;
            for (char *q = s; q < eol && ulen < (int)sizeof(up) - 1; q++)
            {
                char ch = (char)*q;
                if (ch >= 'a' && ch <= 'z')
                    ch = (char)(ch - 32);
                up[ulen++] = ch;
            }

            int is_end_sub_or_func = (beautify_token_match(up, 0, "END") &&
                                      (beautify_token_match(up, 4, "SUB") ||
                                       beautify_token_match(up, 4, "FUNCTION")));

            /* Move to start of the next line. */
            char *next_line = eol + (*eol == '\n' ? 1 : 0);

            if (is_end_sub_or_func && *next_line)
            {
                /* If the next line is already blank, no need to insert one. */
                int next_is_blank = 0;
                {
                    char *u = next_line;
                    while (*u == ' ' || *u == '\t')
                        u++;
                    if (*u == '\n' || *u == 0)
                        next_is_blank = 1;
                }
                if (!next_is_blank)
                {
                    /* Insert a single '\n' at next_line.  Need 1 byte of room. */
                    char *bufend = next_line;
                    while (*bufend)
                        bufend++;
                    if ((bufend - EdBuff) + 1 >= edit_buff_size - 10)
                    {
                        editDisplayMsg(" BEAUTIFY: BUFFER FULL ");
                        return;
                    }
                    int tail_len = (int)(bufend - next_line) + 1; /* incl. NUL */
                    memmove(next_line + 1, next_line, tail_len);
                    *next_line = '\n';
                    next_line++;
                }
            }

            p = next_line;
        }
    }

    /* ---- Pass 3: re-indent each line based on block structure ---- */
    int level = 0;
    char *p = EdBuff;

    while (*p)
    {
        char *line_start = p;

        /* Find current end-of-line and the existing leading whitespace */
        char *eol = p;
        while (*eol && *eol != '\n')
            eol++;

        char *s = line_start;
        while (s < eol && (*s == ' ' || *s == '\t'))
            s++;

        /* Build a small uppercase token buffer for keyword sniffing */
        char up[40] = {0};
        int ulen = 0;
        for (char *q = s; q < eol && ulen < (int)sizeof(up) - 1; q++)
        {
            char ch = (char)*q;
            if (ch >= 'a' && ch <= 'z')
                ch = (char)(ch - 32);
            up[ulen++] = ch;
        }

        int dedent_self = 0;
        int indent_after = 0;
        int is_blank = (s >= eol);
        int is_comment = 0;
        int is_label = 0;
        int is_line_number = 0;
        int line_number_len = 0;
        if (!is_blank)
        {
            if (*s == '\'')
                is_comment = 1;
            else if (ulen >= 3 && up[0] == 'R' && up[1] == 'E' && up[2] == 'M' &&
                     (ulen == 3 || up[3] == ' ' || up[3] == '\t'))
                is_comment = 1;
            else if (*s >= '0' && *s <= '9')
            {
                /* Detect a leading line number: a run of digits followed by
                 * whitespace, end-of-line or ':'.  Treated like a label so
                 * that classic line-numbered BASIC is not pushed off to the
                 * right by surrounding block structure. */
                const char *r = s;
                while (r < eol && *r >= '0' && *r <= '9')
                    r++;
                if (r > s && (r >= eol || *r == ' ' || *r == '\t' || *r == ':'))
                {
                    is_line_number = 1;
                    line_number_len = (int)(r - s);
                }
            }
            else
            {
                /* Detect a leading label: identifier ([A-Za-z_][A-Za-z0-9_]*)
                 * immediately followed by ':'.  Such lines are kept flush
                 * left so that targets of GOTO/GOSUB stay visible even when
                 * they appear inside an indented block. */
                unsigned char c0 = *s;
                if ((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z') ||
                    c0 == '_')
                {
                    const char *r = s + 1;
                    while (r < eol)
                    {
                        unsigned char ch = *r;
                        if ((ch >= 'A' && ch <= 'Z') ||
                            (ch >= 'a' && ch <= 'z') ||
                            (ch >= '0' && ch <= '9') || ch == '_')
                            r++;
                        else
                            break;
                    }
                    if (r < eol && *r == ':')
                        is_label = 1;
                }
            }
        }

        if (!is_blank && !is_comment)
        {
            /* Walk the line statement-by-statement, splitting at ':' outside
             * string literals and stopping at the start of any trailing
             * comment.  For each statement classify whether it opens
             * (FOR, DO, SUB, FUNCTION, TYPE, SELECT CASE, multi-line IF),
             * closes (NEXT, LOOP, END IF / ENDIF, END SELECT / SUB / FUNCTION /
             * TYPE) or is a middle clause (ELSE, ELSEIF, CASE).  Accumulate
             * the net indent change for the line so that single-line forms
             * such as  FOR i=1 TO 9:a(i)=i:NEXT  cancel out and do not
             * indent following lines.                                       */
            const char *q = s;
            /* Skip a leading line number so that the keyword classifier
             * sees the actual statement (e.g. DO/LOOP/SUB/END SUB) that
             * follows the digits. */
            if (is_line_number)
                q = s + line_number_len;
            int first_stmt = 1;
            int in_str = 0;
            while (q < eol)
            {
                while (q < eol && (*q == ' ' || *q == '\t'))
                    q++;
                const char *stmt_start = q;
                const char *stmt_end = NULL;

                while (q < eol)
                {
                    char ch = (char)*q;
                    if (ch == '"')
                        in_str = !in_str;
                    else if (ch == '\'' && !in_str)
                    {
                        stmt_end = q;
                        q = eol; /* trailing comment ends scanning */
                        break;
                    }
                    else if (ch == ':' && !in_str)
                        break;
                    q++;
                }
                if (stmt_end == NULL)
                    stmt_end = q;
                if (q < eol && *q == ':')
                    q++;

                if (stmt_end <= stmt_start)
                {
                    first_stmt = 0;
                    continue;
                }

                char kw[16] = {0};
                int klen = 0;
                for (const char *r = stmt_start;
                     r < stmt_end && klen < (int)sizeof(kw) - 1; r++)
                {
                    char ch = (char)*r;
                    if (ch >= 'a' && ch <= 'z')
                        ch = (char)(ch - 32);
                    kw[klen++] = ch;
                }

                int open_kw = 0, close_kw = 0, middle_kw = 0;

                if (beautify_token_match(kw, 0, "ENDIF") ||
                    beautify_token_match(kw, 0, "NEXT") ||
                    beautify_token_match(kw, 0, "LOOP") ||
                    (beautify_token_match(kw, 0, "END") &&
                     (beautify_token_match(kw, 4, "IF") ||
                      beautify_token_match(kw, 4, "SELECT") ||
                      beautify_token_match(kw, 4, "FUNCTION") ||
                      beautify_token_match(kw, 4, "SUB")
                      || beautify_token_match(kw, 4, "TYPE")
                          )))
                    close_kw = 1;
                else if (beautify_token_match(kw, 0, "ELSE") ||
                         beautify_token_match(kw, 0, "ELSEIF") ||
                         beautify_token_match(kw, 0, "CASE"))
                    middle_kw = 1;

                if (beautify_token_match(kw, 0, "FOR") ||
                    beautify_token_match(kw, 0, "DO") ||
                    beautify_token_match(kw, 0, "FUNCTION") ||
                    beautify_token_match(kw, 0, "SUB")
                    || beautify_token_match(kw, 0, "TYPE")
                    || (beautify_token_match(kw, 0, "SELECT") &&
                        beautify_token_match(kw, 7, "CASE")))
                    open_kw = 1;
                else if (beautify_token_match(kw, 0, "IF"))
                {
                    int found_after = -1;
                    for (const char *r = stmt_start + 2;
                         r + 4 < stmt_end; r++)
                    {
                        if ((r[0] == ' ' || r[0] == '\t') &&
                            (r[1] == 'T' || r[1] == 't') &&
                            (r[2] == 'H' || r[2] == 'h') &&
                            (r[3] == 'E' || r[3] == 'e') &&
                            (r[4] == 'N' || r[4] == 'n'))
                        {
                            const char *after = r + 5;
                            if (after == stmt_end || *after == ' ' || *after == '\t')
                            {
                                found_after = (int)(after - stmt_start);
                                break;
                            }
                        }
                    }
                    if (found_after >= 0)
                    {
                        const char *a = stmt_start + found_after;
                        while (a < stmt_end && (*a == ' ' || *a == '\t'))
                            a++;
                        if (a >= stmt_end)
                            open_kw = 1;
                    }
                }

                if (first_stmt)
                {
                    if (close_kw)
                        dedent_self = 1;
                    else if (middle_kw)
                    {
                        dedent_self = 1;
                        indent_after += 1;
                    }
                    if (open_kw)
                        indent_after += 1;
                }
                else
                {
                    if (close_kw)
                        indent_after -= 1;
                    else if (open_kw)
                        indent_after += 1;
                    /* middle clauses inside a colon-joined line are
                     * structurally unusual; treat as no-op.                */
                }

                first_stmt = 0;
            }
        }

        int line_indent = level - dedent_self;
        if (line_indent < 0)
            line_indent = 0;
        int want_spaces;
        if (is_blank)
            want_spaces = 0;
        else if (is_label)
            want_spaces = 0; /* labels stay flush left */
        else if (is_line_number)
            want_spaces = 0; /* digits go to col 0; gap fixed up below */
        else
            want_spaces = line_number_base + line_indent * 2;
        int have_spaces = (int)(s - line_start);

        /* Adjust leading whitespace in place by shifting the tail of the buffer */
        if (want_spaces != have_spaces)
        {
            int diff = want_spaces - have_spaces; /* >0 grow, <0 shrink */

            /* Find end of buffer (NUL terminator) */
            char *bufend = s;
            while (*bufend)
                bufend++;
            int tail_len = (int)(bufend - s); /* bytes from s through, not incl. NUL */

            if (diff > 0)
            {
                /* Need to grow: ensure there's room (10-byte slack like editInsertChar) */
                if ((bufend - EdBuff) + diff >= edit_buff_size - 10)
                {
                    editDisplayMsg(" BEAUTIFY: BUFFER FULL ");
                    return;
                }
                /* Shift tail (including NUL) right by diff */
                memmove(s + diff, s, tail_len + 1);
            }
            else
            {
                /* Shrink: shift tail (including NUL) left by -diff */
                memmove(s + diff, s, tail_len + 1);
            }

            /* Fill new leading spaces */
            for (int i = 0; i < want_spaces; i++)
                line_start[i] = ' ';

            /* Recompute eol after shift */
            eol = line_start + want_spaces;
            while (*eol && *eol != '\n')
                eol++;
        }
        else
        {
            /* Same width - just rewrite spaces (in case some were tabs) */
            for (int i = 0; i < want_spaces; i++)
                line_start[i] = ' ';
        }

        /* For numbered lines, normalise the gap between the line number and
         * the following statement so the statement aligns at
         * line_number_base + line_indent*2 (with a minimum of one space). */
        if (is_line_number)
        {
            char *num_end = line_start + line_number_len;
            char *gap_end = num_end;
            while (*gap_end == ' ' || *gap_end == '\t')
                gap_end++;
            if (*gap_end != 0 && *gap_end != '\n')
            {
                int have_gap = (int)(gap_end - num_end);
                int target_col = line_number_base + line_indent * 2;
                int want_gap = target_col - line_number_len;
                if (want_gap < 1)
                    want_gap = 1;
                int diff = want_gap - have_gap;
                if (diff != 0)
                {
                    char *bufend = gap_end;
                    while (*bufend)
                        bufend++;
                    int tail_len = (int)(bufend - gap_end);
                    if (diff > 0 &&
                        (bufend - EdBuff) + diff >= edit_buff_size - 10)
                    {
                        editDisplayMsg(" BEAUTIFY: BUFFER FULL ");
                        return;
                    }
                    memmove(gap_end + diff, gap_end, tail_len + 1);
                }
                for (int i = 0; i < want_gap; i++)
                    num_end[i] = ' ';
                eol = num_end + want_gap;
                while (*eol && *eol != '\n')
                    eol++;
            }
        }

        /* Advance to the character after the newline (or end) */
        p = eol;
        if (*p == '\n')
            p++;

        if (dedent_self)
            level--;
        if (indent_after)
            level++;
        if (level < 0)
            level = 0;
    }
}

/* Shared scratch buffer for the editor's import / export filename handling
   (FullScreenEditor + MarkMode). The import and the two export paths are
   separate one-shot user commands that never run concurrently, so they
   share one buffer rather than each carrying its own static
   char[STRINGSIZE+8] (saves ~528 bytes of BSS). Kept at unconditional file
   scope — the import/export commands are part of the editor on every build,
   including PICOMITEMIN. */
static char editor_io_filename[STRINGSIZE + 8];

void FullScreenEditor(int xx, int yy, char *fname, int edit_buff_size)
{
    edit_is_bas = (fname == NULL || fstrstr(fname, ".bas") != NULL);
    EdFileName = fname;
    int c = -1, i;
    // CMM2: the type-ahead buffer from the heap (1 KB the stack, with the heap in 18 KB of DTCM, cannot spare:
    // Editor_Port_Plan.md E1); temporary memory, freed at the prompt. The clipboard is EdClip (E4).
    char *buf = GetTempMemory(MAXCLIP + 2);
    char *p, *tp, BreakKeySave;
    char currdel = 0, nextdel = 0, lastdel = 0;
    char multi = false;
    bool foundit = false;
    printScreen(); // draw the screen
    SCursor(xx, yy);
    drawstatusline = true;
    unsigned char lastkey = 0;
    int statuscount;
    buf[0] = 0;
    insert = true;
    TextChanged = false;
    EdKeyNow = -1;                                      // (no key yet: EdDropHeldRepeats)
    BreakKeySave = BreakKey;
    BreakKey = 0;
    while (1)
    {
        statuscount = 0;
        do
        {
            ShowCursor(true);
            UiPointer(1);                               // (CMM2: the mouse's pointer, and its actions: EdMouse)
            c = EdInkey();                              // (CMM2: Enter is '\n', EdInkey)
            if (c == -1 && (c = EdMouse(0)) == EDMOUSE_CELL)
            {
                ShowCursor(false);
                UiPointer(0);
                EdClick(EdMouseX, EdMouseY);
                c = -1;
            }
            if (statuscount++ == 5000)
            {
                UiPointer(0);
                EdPrintStatus();
            }
        } while (c == -1);
        ShowCursor(false);
        UiPointer(0);
        EdKeyNow = c;

        if (drawstatusline)
            PrintFunctKeys(EDIT);
        drawstatusline = false;
        if (c == TAB)
        {
            strcpy((char *)buf, "        ");
            buf[Option.Tab - ((edx + curx) % Option.Tab)] = 0;
        }
        else
        {
            buf[0] = c;
            buf[1] = 0;
        }
        if (!(buf[0] == SHIFT_F5 || buf[0] == SHIFT_F4 || buf[0] == F7 || buf[0] == F8))
            foundit = false;
        do
        {
            if (buf[0] == BreakKeySave)
                buf[0] = ESC; // if the user tried to break turn it into an escape
            switch (buf[0])
            {

            case '\r':
            case '\n': // first count the spaces at the beginning of the line
                if (txtp != EdBuff && (*txtp == '\n' || *txtp == 0))
                { // we only do this if we are at the end of the line
                    for (tp = txtp - 1, i = 0; *tp != '\n' && tp >= EdBuff; tp--)
                        if (*tp != ' ')
                            i = 0; // not a space
                        else
                            i++; // potential space at the start
                    if (tp == EdBuff && *tp == ' ')
                        i++; // correct for a counting error at the start of the buffer
                    if (buf[1] != 0)
                        i = 0; // do not insert spaces if buffer too small or has something in it
                    else
                        buf[i + 1] = 0; // make sure that the end of the buffer is zeroed
                    while (i)
                        buf[i--] = ' '; // now, place our spaces in the typeahead buffer
                }
                if (!editInsertChar('\n', &multi, edit_buff_size))
                    break; // insert the newline
                TextChanged = true;
                nbrlines++;
                if (!(cury < VHeight - 1)) // if we are NOT at the bottom
                    edy++;                 // otherwise scroll
                edx = 0;                   // reset horizontal scroll for the new line
                printScreen();             // redraw everything
                PositionCursor(txtp);
                break;

            case CTRLKEY('E'):
            case UP:
                if (cury == 0 && edy == 0)
                    break;
                if (edx != 0)
                {
                    edx = 0;
                    SCursor(0, cury);
                    printLine(edy + cury);
                }
                if (*txtp == '\n')
                    txtp--; // step back over the terminator if we are right at the end of the line
                while (txtp != EdBuff && *txtp != '\n')
                    txtp--; // move to the beginning of the line
                if (txtp != EdBuff)
                {
                    txtp--; // step over the terminator to the end of the previous line
                    while (txtp != EdBuff && *txtp != '\n')
                        txtp--; // move to the beginning of that line
                    if (*txtp == '\n')
                        txtp++; // and position at the start
                }
                for (i = 0; i < edx + tempx && *txtp != 0 && *txtp != '\n'; i++, txtp++)
                    ; // move the cursor to the column

                if (cury > 2 || edy == 0)
                { // if we are more that two lines from the top
                    if (cury > 0)
                        SCursor(i, cury - 1); // just move the cursor up
                }
                else if (edy > 0)
                { // if we are two lines or less from the top
                    curx = i;
                    ScrollDown();
                }
                PositionCursor(txtp);
                break;

            case CTRLKEY('X'):
            case DOWN:
                if (edx != 0)
                {
                    edx = 0;
                    SCursor(0, cury);
                    printLine(edy + cury);
                }
                p = txtp;
                while (*p != 0 && *p != '\n')
                    p++; // move to the end of this line
                if (*p == 0)
                    break; // skip if it is at the end of the file
                p++;       // step over the line terminator to the start of the next line
                for (i = 0; i < edx + tempx && *p != 0 && *p != '\n'; i++, p++)
                    ; // move the cursor to the column
                txtp = p;

                if (cury < VHeight - 3 || edy + VHeight == nbrlines)
                {
                    if (cury < VHeight - 1)
                        SCursor(i, cury + 1);
                }
                else if (edy + VHeight < nbrlines)
                {
                    curx = i;
                    Scroll();
                }
                PositionCursor(txtp);
                break;

            case CTRLKEY('S'):
            case LEFT:
                if (txtp == EdBuff)
                    break;
                if (*(txtp - 1) == '\n')
                { // if at the beginning of the line wrap around
                    buf[1] = UP;
                    buf[2] = END;
                    buf[3] = 0;
                    buf[4] = 0;
                }
                else
                {
                    txtp--;
                    if (edx > 0 && curx <= SCROLLCHARS)
                    {
                        edx--;
                        printLine(edy + cury);
                    }
                    PositionCursor(txtp);
                }
                break;

            case CTRLKEY('D'):
            case RIGHT:
                if (*txtp == '\n')
                { // if at the end of the line wrap around
                    buf[1] = HOME;
                    buf[2] = DOWN;
                    buf[3] = 0;
                    break;
                }
                if (*txtp != 0)
                {
                    txtp++;
                    if (curx >= VWidth - 1 - SCROLLCHARS)
                    {
                        edx++;
                        printLine(edy + cury);
                    }
                }
                PositionCursor(txtp);
                break;

            // backspace
            case BKSP:
                if (txtp == EdBuff)
                    break;
                if (*(txtp - 1) == '\n')
                { // if at the beginning of the line wrap around
                    buf[1] = UP;
                    buf[2] = END;
                    buf[3] = DEL;
                    buf[4] = 0;
                    break;
                }
                // find how many spaces are between the cursor and the start of the line
                for (p = txtp - 1; *p == ' ' && p != EdBuff; p--)
                    ;
                if ((p == EdBuff || *p == '\n') && txtp - p > 1)
                {
                    i = txtp - p - 1;
                    // we have have the number of continuous spaces between the cursor and the start of the line
                    // now figure out the number of backspaces to the nearest tab stop

                    i = (i % Option.Tab);
                    if (i == 0)
                        i = Option.Tab;
                    // load the corresponding number of deletes in the type ahead buffer
                    buf[i + 1] = 0;
                    while (i--)
                    {
                        buf[i + 1] = DEL;
                        txtp--;
                    }
                    // and let the delete case take care of deleting the characters
                    PositionCursor(txtp);
                    break;
                }
                // this is just a normal backspace (not a tabbed backspace)
                txtp--;
                if (edx > 0 && curx <= SCROLLCHARS)
                    edx--;
                PositionCursor(txtp);
                // fall through to delete the char

            case CTRLKEY(']'):
            case DEL:
                if (*txtp == 0)
                    break;
                p = txtp;
                c = *p;
                currdel = *p;
                if (p != EdBuff + edit_buff_size - 1)
                    nextdel = p[1];
                else
                    nextdel = 0;
                if (p != EdBuff)
                {
                    lastdel = *(--p);
                    p++;
                }
                else
                    lastdel = 0;
                EdChanged(txtp);                        // (CMM2 (E6): the lines kept after it move)
                while (*p)
                {
                    p[0] = p[1];
                    p++;
                }
                if (c == '\n')
                {
                    // Measure the cursor's column (= length of original line n).
                    // txtp still points to the now-deleted '\n' position, so count
                    // chars back to the start of the line.
                    int abs_col = 0;
                    {
                        char *q = txtp;
                        while (q > EdBuff && *(q - 1) != '\n')
                        {
                            abs_col++;
                            q--;
                        }
                    }
                    int saved_cury = cury;
                    int cur_ln = edy + saved_cury;
                    // Reset horizontal scroll before full redraw so every line
                    // is drawn left-justified, not offset by edx from the END key.
                    edx = 0;
                    printScreen();
                    nbrlines--;
                    // If the join point is off-screen to the right, scroll only
                    // the current (joined) line to bring the cursor into view.
                    if (abs_col > VWidth - 1)
                    {
                        edx = abs_col - (VWidth - 1 - SCROLLCHARS);
                        SCursor(0, saved_cury);
                        printLine(cur_ln);
                    }
                }
                else
                    printLine(edy + cury);
                TextChanged = true;
                PositionCursor(txtp);
                if (currdel == '/' && nextdel == '*' && EdColourCode)
                    printScreen();
                if (currdel == '*' && nextdel == '/' && EdColourCode)
                    printScreen();
                if (currdel == '/' && lastdel == '*' && EdColourCode)
                    printScreen();
                if (currdel == '*' && lastdel == '/' && EdColourCode)
                    printScreen();
                break;

            case SHIFT_TAB:                             // CMM2 (kilo's): up to a tab stop's worth of the spaces at the
                for (i = 0; i < Option.Tab && txtp[i] == ' '; i++)  // cursor deleted (at a line's start, it
                    buf[i + 1] = DEL;                   // takes the indent back a stop)
                buf[i + 1] = 0;
                break;

            case SHIFT_DEL:                             // CMM2 (kilo's): at the start of a line, its leading spaces
                i = 0;                                  // deleted; anywhere else Del
                if (txtp == EdBuff || txtp[-1] == '\n')
                    while (txtp[i] == ' ' && i < MAXCLIP - 1)
                        i++;
                if (i == 0)
                    i = 1;
                buf[i + 1] = 0;
                while (i)
                    buf[i--] = DEL;
                break;

            case CTRLKEY('N'):
            case INSERT:
                insert = !insert;
                break;

            case CTRLKEY('U'):
            case HOME:
                if (txtp == EdBuff)
                    break;
                if (lastkey == HOME || lastkey == CTRLKEY('U'))
                {
                    edx = edy = curx = cury = 0;
                    txtp = EdBuff;
                    PrintString("\033[2J\033[H"); // vt100 clear screen and home cursor
                    EdDisplay(DISPLAY_CLS);    // clear screen on the MX470 display only
                    printScreen();
                    PrintFunctKeys(EDIT);
                    PositionCursor(txtp);
                    break;
                }
                if (*txtp == '\n')
                    txtp--; // step back over the terminator if we are right at the end of the line
                while (txtp != EdBuff && *txtp != '\n')
                    txtp--; // move to the beginning of the line
                if (*txtp == '\n')
                    txtp++; // skip if no more lines above this one
                if (edx != 0)
                {
                    edx = 0;
                    SCursor(0, cury);
                    printLine(edy + cury);
                }
                PositionCursor(txtp);
                break;

            case CTRLKEY('K'):
            case END:
                if (*txtp == 0)
                    break; // already at the end
                if (lastkey == END || lastkey == CTRLKEY('K'))
                { // jump to the end of the file
                    i = 0;
                    p = txtp = EdBuff;
                    while (*txtp != 0)
                    {
                        if (*txtp == '\n')
                        {
                            p = txtp + 1;
                            i++;
                        }
                        txtp++;
                    }

                    if (i >= VHeight)
                    {
                        edy = i - VHeight + 1;
                        printScreen();
                        cury = VHeight - 1;
                    }
                    else
                    {
                        cury = i;
                    }
                    txtp = p;
                    curx = 0;
                }

                while (*txtp != 0 && *txtp != '\n')
                    txtp++;
                {
                    int abs_col = 0;
                    char *q = txtp;
                    while (q > EdBuff && *(q - 1) != '\n')
                    {
                        abs_col++;
                        q--;
                    }
                    if (abs_col > VWidth - 1)
                    {
                        edx = abs_col - (VWidth - 1 - SCROLLCHARS);
                        SCursor(0, cury);
                        printLine(edy + cury);
                    }
                    else if (edx != 0)
                    {
                        edx = 0;
                        SCursor(0, cury);
                        printLine(edy + cury);
                    }
                }
                PositionCursor(txtp);
                break;

            case CTRLKEY('P'):
            case PUP:
                if (cury > 0)
                {                                       // CMM2 (kilo's): first to the screen's top row
                    EdToRow(0);
                    break;
                }
                if (edy == 0)
                {                  // if we are already showing the top of the text
                    buf[1] = HOME; // force the editing point to the start of the text
                    buf[2] = HOME;
                    buf[3] = 0;
                    break;
                }
                else if (edy >= VHeight - 1)
                { // if we can scroll a full screenfull
                    i = VHeight + 1;
                    edy -= VHeight;
                }
                else
                { // if it is less than a full screenfull
                    i = edy + 1;
                    edy = 0;
                }
                while (i--)
                {
                    if (*txtp == '\n')
                        txtp--; // step back over the terminator if we are right at the end of the line
                    while (txtp != EdBuff && *txtp != '\n')
                        txtp--; // move to the beginning of the line
                    if (txtp == EdBuff)
                        break; // skip if no more lines above this one
                }
                if (txtp != EdBuff)
                    txtp++; // and position at the start of the line
                for (i = 0; i < edx + curx && *txtp != 0 && *txtp != '\n'; i++, txtp++)
                    ; // move the cursor to the column
                printScreen();
                PositionCursor(txtp);
                break;

            case CTRLKEY('L'):
            case PDOWN:
                if (cury < VHeight - 1 && edy + cury < nbrlines - 1)
                {                                       // CMM2 (kilo's): first to the screen's bottom row (or the
                    EdToRow(VHeight - 1);               // last line, when the text ends before it)
                    break;
                }
                if (nbrlines <= edy + VHeight + 1)
                {                 // if we are already showing the end of the text
                    buf[1] = END; // force the editing point to the end of the text
                    buf[2] = END;
                    buf[3] = 0;
                    break; // cursor to the top line
                }
                else if (nbrlines - edy - VHeight >= VHeight)
                { // if we can scroll a full screenfull
                    edy += VHeight;
                    i = VHeight;
                }
                else
                { // if it is less than a full screenfull
                    i = nbrlines - VHeight - edy;
                    edy = nbrlines - VHeight;
                }
                if (*txtp == '\n')
                    i--; // compensate if we are right at the end of a line
                while (i--)
                {
                    if (*txtp == '\n')
                        txtp++; // step over the terminator if we are right at the start of the line
                    while (*txtp != 0 && *txtp != '\n')
                        txtp++; // move to the end of the line
                    if (*txtp == 0)
                        break; // skip if no more lines after this one
                }
                if (txtp != EdBuff)
                    txtp++; // and position at the start of the line
                for (i = 0; i < edx + curx && *txtp != 0 && *txtp != '\n'; i++, txtp++)
                    ; // move the cursor to the column
                // y = cury;
                printScreen();
                PositionCursor(txtp);
                break;

            // Abort without saving
            case ESC:
                uSec(50000); // wait 50ms to see if anything more is coming
                routinechecks();
                if (UiMouseReport())                    // (CMM2: the keys after Esc given back if not)
                {
                    // received escape code for Tera Term reporting a mouse click or scroll wheel movement
                    int c, x, y;
                    c = MMInkey();
                    x = MMInkey() - '!';
                    y = MMInkey() - '!';
                    if (c == 'e' || c == 'a')
                    { // Tera Term - SHIFT + mouse-wheel-rotate-down
                        buf[1] = UP;
                        buf[2] = 0;
                    }
                    else if (c == 'd' || c == '`')
                    { // Tera Term - SHIFT + mouse-wheel-rotate-up
                        buf[1] = DOWN;
                        buf[2] = 0;
                    }
                    else if (c == ' ' && x >= 0 && x < VWidth && y >= 0 && y < VHeight)
                    { // c == ' ' means mouse down and no shift, ctrl, etc
                        // first position on the y axis
                        while (*txtp != 0 && y > cury) // assume we have to move down the screen
                            if (*txtp++ == '\n')
                                cury++;
                        while (txtp != EdBuff && y < cury) // assume we have to move up the screen
                            if (*--txtp == '\n')
                                cury--;
                        while (txtp != EdBuff && *(txtp - 1) != '\n')
                            txtp--; // move to the beginning of the line
                        // x is a screen column; the character under it is at
                        // absolute column edx+x once the line has scrolled.
                        for (curx = 0; curx < edx + x && *txtp && *txtp != '\n'; curx++)
                            txtp++; // now position on the x axis (absolute column)
                        curx -= edx; // convert back to a screen-relative column
                        if (curx < 0)
                            curx = 0;
                        PositionCursor(txtp);
                        EdPrintStatus(); // the line/column readout moved too
                    }
                    break;
                }
                // this must be an ordinary escape (not part of an escape code)
                if (TextChanged)
                {
                    GetInputString("Exit and discard all changes (Y/N): ");
                    if (toupper(*inpbuf) != 'Y')
                        break;
                }
                // fall through to the normal exit

            case CTRLKEY('Q'): // Save and exit
            case F1:           // Save and exit
            case CTRLKEY('W'): // Save, exit and run
            case F2:           // Save, exit and run
            {
                c = buf[0];
                if (c != ESC)                           // CMM2: only a save needs the lines checked
                {
                    int line = 0;
                    int i = find_longest_line_length((char *)EdBuff, &line);
                    if (i > 255 && edit_is_bas)
                    {
                        char buff[24] = {};
                        sprintf(buff, " LINE %d TOO LONG", line);
                        editDisplayMsg((char *)buff);
                        break;
                    }
                    /* CMM2: every key but Esc saves, as kilo's F1 and F2 do whether or not the text changed, and
                       (E3) while the editor is still up: if it fails, the editor stays open with the text */
                    FRESULT r = EdSave(fname);
                    if (r != FR_OK)
                    {
                        EdFileMsg("SAVE FAILED", r, fname);
                        break;
                    }
                    StartEditLine = edy + cury;         // EDIT with no file opens here next (kilo's editorSave)
                    StartEditCharacter = edx + curx;
                }
                UiTermMouse(0);                    // Tera Term turn off mouse click report in vt200 mode
                PrintString("\033[?7h");           // restore autowrap (was disabled by setterminal on entry)
                PrintString("\0338\033[2J\033[H"); // vt100 clear screen and home cursor
                gui_fcolour = GUI_C_NORMAL;
                PrintString(VT100_C_NORMAL);
                PrintString("\033[0m");
                BreakKey = BreakKeySave;
                UiMouseClose();
                EdDisplayRestore();
                ClearTempMemory();                      // (the edit buffer, the clipboard)
                if (c == ESC)
                    return;
                if (edit_is_bas)                        // F1 and F2 load the program (kilo's editorSave)
                    FileLoadProgram(fname, 0);
                if (c == CTRLKEY('Q') || c == F1 || !edit_is_bas)
                    return;
                if (fm_editing)
                {                                       // FM's EDIT: FM runs the file and comes back to FM after it
                    fm_editing = 2;
                    return;
                }
                strcpy(inpbuf, "RUN\r\n");              // kilo's F2: the program run as from the prompt
                tokenise(true);
                ExecuteProgram(tknbuf);
                cleanend();
                return;
            }

            // Search
            case CTRLKEY('R'):
            case F3:
                EdGetInput("Find (Use SHIFT-F3 to repeat): ", (char *)tknbuf);    // (CMM2: the last search offered)
                if (*inpbuf == 0 || *inpbuf == ESC)
                    break;
                if (!(*inpbuf == 0xb3 || *inpbuf == F3))
                    strcpy((char *)tknbuf, (char *)inpbuf);
                // fall through

            case CTRLKEY('G'):
            case SHIFT_F3:
            case F6:
                p = txtp;
                if (*p == 0)
                    p = EdBuff - 1;
                i = strlen((char *)tknbuf);
                while (1)
                {
                    p++;
                    if (p == txtp)
                        break;
                    if (*p == 0)
                        p = EdBuff;
                    if (p == txtp)
                        break;
                    if (mem_equal(p, tknbuf, i))
                        break;
                }
                if (p == txtp)
                {
                    editDisplayMsg(" NOT FOUND ");
                    break;
                }
                txtp = p;                               // CMM2 (E5): the match brought into view, sideways too (the
                EdShowCursor(i);                        // PicoMite's set edy but not edx, so a match past the right
                foundit = true;                         // edge was off the screen)
                break;

            // Mark
            case UPSEL:                                 // CMM2 (kilo's): Shift and an arrow start mark mode, the
            case DOWNSEL:                               // arrow being its first move (given back for MarkMode)
            case LEFTSEL:
            case RIGHTSEL:
                EdUnget(buf[0]);
                // fall through
            case CTRLKEY('T'):
            case F4:
                MarkMode(&buf[1]);
                {
                    int msgup = EdMsgUp;                // (CMM2: a message mark mode ended with, put back below)
                    // While selecting along a long line, mark-mode navigation drags
                    // edx (the horizontal scroll) to the right but only redraws the
                    // current line.  If we now redraw the whole screen with that
                    // stale edx every background line is left scrolled and short
                    // lines become unreachable.  Mirror the line-join fix: redraw
                    // left-justified, then scroll only the cursor's own line if its
                    // column is off-screen to the right.
                    int abs_col = 0;
                    char *q = txtp;
                    while (q > EdBuff && *(q - 1) != '\n')
                    {
                        abs_col++;
                        q--;
                    }
                    edx = 0;
                    printScreen();
                    PrintFunctKeys(EDIT);
                    if (abs_col > VWidth - 1)
                    {
                        edx = abs_col - (VWidth - 1 - SCROLLCHARS);
                        SCursor(0, cury);
                        printLine(edy + cury);
                    }
                    curx = abs_col - edx;
                    if (msgup)
                        editDisplayMsg(EdLastMsg);
                }
                PositionCursor(txtp);
                break;
            case SHIFT_F4:
            case F7:
            case CTRLKEY('F'):
                GetInputString("Replace: ");
                if (*inpbuf == 0 || *inpbuf == ESC)
                    break;
                if (!(*inpbuf == 0xb3 || *inpbuf == F3) && !EdSetClip((char *)inpbuf, strlen((char *)inpbuf)))
                {                                       // (CMM2 (E4): the replacement goes in the clipboard)
                    editDisplayMsg(" NOT ENOUGH MEMORY ");
                    break;
                }
            case CTRLKEY('I'):
            case SHIFT_F5:
            case F8:
                if (!foundit)
                {
                    editDisplayMsg(" NOTHING TO REPLACE ");
                    break;
                }
                if (EdClipLen == 0)
                {
                    editDisplayMsg(" CLIPBOARD IS EMPTY ");
                    break;
                }
                for (i = 0; i < strlen((char *)tknbuf); i++)
                {
                    buf[i + 1] = DEL;
                }
                buf[i + 1] = F5;
                buf[i + 2] = 0;
                break;
            case CTRLKEY('Y'):
            case CTRLKEY('V'):
            case F5:
                if (EdClipLen == 0)
                {
                    editDisplayMsg(" CLIPBOARD IS EMPTY ");
                    break;
                }
                EdPaste(edit_buff_size);                // CMM2 (E4): in one go, not a key at a time
                break;

            // F9 - Import file at current position
            case CTRLKEY('O'):
            case F9:
            {
                // Get filename from user
                GetInputString("Import file: ");
                if (*inpbuf == 0 || *inpbuf == ESC)
                    break;

                // Copy filename to the shared editor I/O buffer
                char *importfile = editor_io_filename;
                strcpy(importfile, (char *)inpbuf);

                // Add .bas extension if no extension and file not found
                FSIZE_t fsize = 0;
                if (!EdFileExists(importfile, NULL))
                    EdDefaultExtension(importfile);

                // Check file exists and get size
                if (!EdFileExists(importfile, &fsize))
                {
                    editDisplayMsg(" FILE NOT FOUND ");
                    break;
                }

                char *endp;
                for (endp = EdBuff; *endp; endp++)
                    ; // find end of current text
                int current_size = endp - EdBuff;

                if (current_size + fsize >= edit_buff_size - 10)
                {
                    editDisplayMsg(" FILE TOO LARGE ");
                    break;
                }

                // Save current cursor position
                char *saved_txtp = txtp;

                // Open the file (CMM2 (E3): FatFs's calls, so that a failure is reported and the editor stays open)
                FIL *fp = GetMemory(sizeof(FIL) + EDIO_BYTES);
                char *fb = (char *)(fp + 1);
                UINT got = 0;
                FRESULT r = EdOpen(fp, importfile, FA_READ);
                if (r != FR_OK)
                {
                    FreeMemory(fp);
                    EdFileMsg("CANNOT OPEN FILE", r, importfile);
                    break;
                }

                // Read file and insert each character
                char ch;
                int insert_count = 0, full = 0;
                do
                {
                    r = f_read(fp, fb, EDIO_BYTES, &got);
                    for (UINT k = 0; r == FR_OK && k < got && !full; k++)
                    {
                        ch = fb[k];
                        if (ch == '\r')
                            continue; // skip carriage returns
                        if (!editInsertChar((unsigned char)ch, &multi, edit_buff_size))
                            full = 1;
                        else
                        {
                            if (ch == '\n')
                                nbrlines++;
                            insert_count++;
                        }
                    }
                } while (r == FR_OK && got == EDIO_BYTES && !full);
                f_close(fp);
                FreeMemory(fp);

                // Restore cursor to original position (before inserted text)
                txtp = saved_txtp;

                TextChanged = true;
                printScreen();
                PositionCursor(txtp);

                // Display success message (CMM2: or why it stopped short, the characters before it inserted)
                char msg[48];
                if (r != FR_OK)
                    EdFileMsg("READ FAILED", r, importfile);
                else if (full)
                {
                    sprintf(msg, " OUT OF MEMORY: %d CHARS IMPORTED ", insert_count);   // (the PicoMite's)
                    editDisplayMsg((char *)msg);
                }
                else
                {
                    sprintf(msg, " IMPORTED %d CHARS ", insert_count);
                    editDisplayMsg((char *)msg);
                }
            }
            break;

            // F10 - Export clipboard to file
            case CTRLKEY('B'):
            case F10:
            {
                // Check if clipboard has content
                if (EdClipLen == 0)
                {
                    editDisplayMsg(" CLIPBOARD IS EMPTY ");
                    break;
                }

                // Get filename from user
                GetInputString("Export to file: ");
                if (*inpbuf == 0 || *inpbuf == ESC)
                    break;

                // Copy filename to the shared editor I/O buffer
                char *exportfile = editor_io_filename;
                strcpy(exportfile, (char *)inpbuf);

                // Add .bas extension if no extension specified
                EdDefaultExtension(exportfile);

                // Write clipboard contents to file, CR LF line ends (CMM2 (E3): a failure reported, the editor open)
                int export_count = EdClipLen;
                FRESULT r = EdWriteFile(exportfile, EdClip, EdClip + export_count, 0);
                if (r != FR_OK)
                {
                    EdFileMsg("CANNOT CREATE FILE", r, exportfile);
                    break;
                }

                // Display success message
                char msg[40];
                sprintf(msg, " EXPORTED %d CHARS ", export_count);
                editDisplayMsg((char *)msg);
            }
            break;

            // F11 - CMM2 (E5, Peter 19 Oct): help on the word at the cursor
            case F11:
                EdHelp();
                break;

            // F12 / Ctrl-A - Beautify (re-indent block structures)
            case CTRLKEY('A'):
            case F12:
            {
                int keep_blanks = 0;
                if (beautify_has_stray_blanks())
                {
                    /* Prompt the user.  Y (default) keeps blank lines,
                     * N removes them. */
                    editDisplayMsg(" KEEP BLANK LINES? (Y/N) ");
                    while (1)
                    {
                        int kc = EdGetKey();
                        if (kc == 'y' || kc == 'Y' || kc == '\n')   // (CMM2: Enter is the '\n' after a CR)
                        {
                            keep_blanks = 1;
                            break;
                        }
                        if (kc == 'n' || kc == 'N')
                        {
                            keep_blanks = 0;
                            break;
                        }
                        if (kc == ESC)
                        {
                            /* Cancel beautify entirely. */
                            EdPrintStatus();
                            goto beautify_done;
                        }
                    }
                }
                editBeautify(edit_buff_size, keep_blanks);
                EdCacheReset();                         // (CMM2 (E6): the text rewritten whole)
                TextChanged = true;
                for (nbrlines = 1, p = EdBuff; *p; p++)     // CMM2: Beautify adds and removes blank lines
                    if (*p == '\n')
                        nbrlines++;
                /* After buffer rewrite, restore cursor to start of buffer */
                txtp = EdBuff;
                edx = edy = curx = cury = 0;
                printScreen();
                PositionCursor(txtp);
                EdPrintStatus();
                editDisplayMsg(" BEAUTIFIED ");
            beautify_done:;
            }
            break;

            // a normal character
            default:
                c = buf[0];
                if (c < ' ' || c > '~')
                    break; // make sure that this is valid
                TextChanged = true;
                if (insert || *txtp == '\n' || *txtp == 0)
                {
                    if (!editInsertChar(c, &multi, edit_buff_size))
                        break; // insert it
                }
                else
                {
                    EdChanged(txtp);                    // (CMM2 (E6))
                    *txtp++ = c; // or just overtype
                }
                if (curx >= VWidth - 1 - SCROLLCHARS)
                    edx++;
                printLine(edy + cury); // redraw the whole line so that colour coding will occur
                PositionCursor(txtp);
                // SCursor(x, cury);
                tempx = cury; // used to track the preferred cursor position
                if (multi && EdColourCode)
                    printScreen();
                EdPrintStatus();
                break;
            }
            lastkey = buf[0];
            if (buf[0] != UP && buf[0] != DOWN && buf[0] != CTRLKEY('E') && buf[0] != CTRLKEY('X'))
                tempx = curx;
            buf[MAXCLIP + 1] = 0;
            for (i = 0; i < MAXCLIP + 1; i++)
                buf[i] = buf[i + 1]; // suffle down the buffer to get the next char
        } while (*buf);
        if (!drawstatusline)
            EdPrintStatus();                              // CMM2: the status bar after every key, as kilo's
    }
}

/*******************************************************************************************************************
  UTILITY FUNCTIONS USED BY THE FULL SCREEN EDITOR
*******************************************************************************************************************/

void PositionCursor(char *curp)
{
    int ln, col;

    ln = EdLineOf(curp, &col);                          // CMM2 (E6): from the line kept, not the text's start
    if (ln < edy || ln >= edy + VHeight)
        return;
    SCursor(col - edx, ln - edy);
}

// mark mode
// implement the mark mode (when the user presses F4)
void MarkMode(char *buf)
{
    char *p, *mark, *oldmark;
    int c = -1, x, y, i, oldx, oldy, txtpx, txtpy, errmsg = false;
    int edx_save = edx, edy_save = edy;
    PrintFunctKeys(MARK);
    oldmark = mark = txtp;
    txtpx = oldx = curx;
    txtpy = oldy = cury;
    while (1)
    {
        c = EdInkey();
        if (c == -1)
            c = EdMouse(1);                             // (CMM2: the mouse, EdMouse)
        if (c == -1)
        {
            UiPointer(1);
            continue;
        }
        UiPointer(0);
        if (c == EDMOUSE_CELL)
            mark = EdTextAt(mark, EdMouseX, EdMouseY);  // (the selection's end to the pointer)
        if (c != -1 && errmsg)
        {
            PrintFunctKeys(MARK);
            errmsg = false;
        }
        switch (c)
        {
        case ESC:
            uSec(50000); // wait 50ms to see if anything more is coming
            if (UiMouseReport())                    // (CMM2: the keys after Esc given back if not)
            {
                // received escape code for Tera Term reporting a mouse click.  in mark mode we ignore it
                MMInkey();
                MMInkey();
                MMInkey();
                break;
            }
            curx = txtpx;
            cury = txtpy; // just an escape key
            edx = edx_save;
            edy = edy_save;
            SCursor(curx, cury);
            return;

        case UPSEL:                                     // (CMM2: the shifted arrows as the arrows, kilo's)
        case CTRLKEY('E'):
        case UP:
            if (cury <= 0 && edy == 0)
                continue; // at very top of file
            p = mark;
            if (*p == '\n')
                p--; // step back over the terminator if we are right at the end of the line
            while (p != EdBuff && *p != '\n')
                p--; // move to the beginning of the line
            if (p != EdBuff)
            {
                p--; // step over the terminator to the end of the previous line
                for (i = 0; p != EdBuff && *p != '\n'; p--, i++)
                    ; // move to the beginning of that line
                if (*p == '\n')
                    p++; // and position at the start
            }
            mark = p;
            for (i = 0; i < edx + curx && *mark != 0 && *mark != '\n'; i++, mark++)
                ; // move the cursor to the preferred absolute column
            {
                int need_redraw = 0;
                if (i >= edx + VWidth - SCROLLCHARS)
                {
                    edx = i - (VWidth - 1 - SCROLLCHARS);
                    need_redraw = 1;
                }
                else if (edx > 0 && i < edx + SCROLLCHARS)
                {
                    edx = (i > SCROLLCHARS) ? i - SCROLLCHARS : 0;
                    need_redraw = 1;
                }
                curx = i - edx;
                if (cury > 0)
                    cury--;
                else if (edy > 0)
                {
                    if (need_redraw)
                        edy--;
                    else
                        EdMarkScroll(0);                // (CMM2 (E6): a line, not the whole screen)
                }
                if (need_redraw)
                {
                    printScreen();
                    PrintFunctKeys(MARK);
                }
            }
            break;

        case DOWNSEL:
        case CTRLKEY('X'):
        case DOWN:
            for (p = mark; *p != 0 && *p != '\n'; p++)
                ; // move to the end of this line
            if (*p == 0)
                continue; // skip if it is at the end of the file
            mark = p + 1; // step over the line terminator to the start of the next line
            for (i = 0; i < edx + curx && *mark != 0 && *mark != '\n'; i++, mark++)
                ; // move the cursor to the preferred absolute column
            {
                int need_redraw = 0;
                if (i >= edx + VWidth - SCROLLCHARS)
                {
                    edx = i - (VWidth - 1 - SCROLLCHARS);
                    need_redraw = 1;
                }
                else if (edx > 0 && i < edx + SCROLLCHARS)
                {
                    edx = (i > SCROLLCHARS) ? i - SCROLLCHARS : 0;
                    need_redraw = 1;
                }
                curx = i - edx;
                if (cury < VHeight - 1)
                    cury++;
                else if (edy + VHeight < nbrlines)
                {
                    if (need_redraw)
                        edy++;
                    else
                        EdMarkScroll(1);                // (CMM2 (E6): a line, not the whole screen)
                }
                if (need_redraw)
                {
                    printScreen();
                    PrintFunctKeys(MARK);
                }
            }
            break;

        case LEFTSEL:
        case CTRLKEY('S'):
        case LEFT:
            if (curx == 0 && edx == 0)
                continue;
            mark--;
            if (edx > 0 && curx <= SCROLLCHARS)
            {
                edx--;
                printLine(edy + cury);
            }
            else
                curx--;
            break;

        case RIGHTSEL:
        case CTRLKEY('D'):
        case RIGHT:
            if (*mark == 0 || *mark == '\n')
                continue;
            mark++;
            if (curx >= VWidth - 1 - SCROLLCHARS)
            {
                edx++;
                printLine(edy + cury);
            }
            else
                curx++;
            break;

        case CTRLKEY('U'):
        case HOME:
            if (mark == EdBuff)
                break;
            if (*mark == '\n')
                mark--; // step back over the terminator if we are right at the end of the line
            while (mark != EdBuff && *mark != '\n')
                mark--; // move to the beginning of the line
            if (*mark == '\n')
                mark++; // skip if no more lines above this one
            if (edx != 0)
            {
                edx = 0;
                printScreen();
                PrintFunctKeys(MARK);
            }
            curx = 0;
            break;

        case CTRLKEY('K'):
        case END:
            if (*mark == 0)
                break;
            for (p = mark; *p != 0 && *p != '\n'; p++)
                ; // move to the end of this line
            mark = p;
            {
                int abs_col = 0;
                char *q = mark;
                while (q > EdBuff && *(q - 1) != '\n')
                {
                    abs_col++;
                    q--;
                }
                if (abs_col > VWidth - 1)
                {
                    edx = abs_col - (VWidth - 1 - SCROLLCHARS);
                    printScreen();
                    PrintFunctKeys(MARK);
                }
                else if (edx != 0)
                {
                    edx = 0;
                    printScreen();
                    PrintFunctKeys(MARK);
                }
                curx = abs_col - edx;
            }
            break;

        case CTRLKEY('P'):
        case PUP:
            if (cury > 0)
            {                                           // CMM2 (kilo's): first to the screen's top row
                mark = EdTextAt(mark, curx, 0);
                break;
            }
            if (edy == 0)
            {
                // Already at top, move mark to beginning of file
                mark = EdBuff;
                cury = 0;
                curx = 0;
            }
            else
            {
                // Scroll up by VHeight lines
                int scrollamt = (edy >= VHeight) ? VHeight : edy;
                edy -= scrollamt;
                // Move mark up by scrollamt lines
                for (i = 0; i < scrollamt && mark != EdBuff; i++)
                {
                    if (*mark == '\n')
                        mark--;
                    while (mark != EdBuff && *mark != '\n')
                        mark--;
                    if (*mark == '\n' && mark != EdBuff)
                        mark--;
                    while (mark != EdBuff && *mark != '\n')
                        mark--;
                    if (*mark == '\n')
                        mark++;
                }
                // Position at start of line
                while (mark != EdBuff && *(mark - 1) != '\n')
                    mark--;
                for (i = 0; i < edx + curx && *mark != 0 && *mark != '\n'; i++, mark++)
                    ;
                if (i >= edx + VWidth - SCROLLCHARS)
                    edx = i - (VWidth - 1 - SCROLLCHARS);
                else if (edx > 0 && i < edx + SCROLLCHARS)
                    edx = (i > SCROLLCHARS) ? i - SCROLLCHARS : 0;
                curx = i - edx;
                printScreen();
                PrintFunctKeys(MARK);
            }
            break;

        case CTRLKEY('L'):
        case PDOWN:
            if (cury < VHeight - 1 && edy + cury < nbrlines - 1)
            {                                           // CMM2 (kilo's): first to the screen's bottom row (or the
                mark = EdTextAt(mark, curx, VHeight - 1);   // last line)
                break;
            }
            if (edy + VHeight >= nbrlines)
            {
                // Already showing end, move mark to end of file
                while (*mark)
                    mark++;
                cury = VHeight - 1;
            }
            else
            {
                // Scroll down by VHeight lines
                int scrollamt = VHeight;
                if (edy + VHeight + scrollamt > nbrlines)
                    scrollamt = nbrlines - edy - VHeight;
                edy += scrollamt;
                // Move mark down by scrollamt lines
                for (i = 0; i < scrollamt && *mark; i++)
                {
                    while (*mark && *mark != '\n')
                        mark++;
                    if (*mark == '\n')
                        mark++;
                }
                // Position at column
                for (i = 0; i < edx + curx && *mark != 0 && *mark != '\n'; i++, mark++)
                    ;
                if (i >= edx + VWidth - SCROLLCHARS)
                    edx = i - (VWidth - 1 - SCROLLCHARS);
                else if (edx > 0 && i < edx + SCROLLCHARS)
                    edx = (i > SCROLLCHARS) ? i - SCROLLCHARS : 0;
                curx = i - edx;
                printScreen();
                PrintFunctKeys(MARK);
            }
            break;

        case CTRLKEY('Y'):
        case CTRLKEY('T'):
        case F5:
        case F4:
        case CTRLKEY('B'):
        case F10:
            if (c != F10)
            {                                           // CMM2 (E4): the clipboard made as large as the selection
                char *lo = mark <= txtp ? mark : txtp;
                if (!EdSetClip(lo, (mark <= txtp ? txtp : mark) - lo))
                {
                    editDisplayMsg(" NOT ENOUGH MEMORY ");
                    errmsg = true;
                    SCursor(curx, cury);
                    continue;
                }
            }
            if (c == F5 || c == CTRLKEY('Y') || c == F10)
            {
                // For F10, also export to file before returning
                if (c == F10)
                {
                    // Get filename from user
                    GetInputString("Export to file: ");
                    if (*inpbuf != 0 && *inpbuf != ESC)
                    {
                        // Copy filename to the shared editor I/O buffer
                        char *exportfile = editor_io_filename;
                        strcpy(exportfile, (char *)inpbuf);

                        // Add .bas extension if no extension specified
                        EdDefaultExtension(exportfile);

                        // Write the selection to the file, CR LF line ends (CMM2 (E3): a failure reported, the editor
                        // open)
                        char *cp = (mark <= txtp) ? mark : txtp;
                        char *cpend = (mark <= txtp) ? txtp : mark;
                        FRESULT r = EdWriteFile(exportfile, cp, cpend, 0);
                        if (r == FR_OK)
                        {
                            char msg[40];
                            sprintf(msg, " EXPORTED %d CHARS ", (int)(cpend - cp));
                            editDisplayMsg((char *)msg);
                        }
                        else
                            EdFileMsg("CANNOT CREATE FILE", r, exportfile);
                    }
                }
                // Calculate line number of txtp and adjust edy if needed
                int ln = EdLineOf(txtp, NULL);          // (CMM2 (E6): from the line kept)
                // If txtp is off-screen, adjust edy to center it
                if (ln < edy || ln >= edy + VHeight)
                {
                    edy = ln - VHeight / 2;
                    if (edy < 0)
                        edy = 0;
                    if (edy + VHeight > nbrlines)
                        edy = (nbrlines > VHeight) ? nbrlines - VHeight : 0;
                }
                cury = ln - edy;
                PositionCursor(txtp);
                return;
            }
            // fall through

        case CTRLKEY(']'):
        case DEL:
            if (mark < txtp)
            {
                p = txtp;
                txtp = mark;
                mark = p; // swap txtp and mark
            }
            for (p = txtp; p < mark; p++)
                if (*p == '\n')
                    nbrlines--;
            EdChanged(txtp);                            // (CMM2 (E6): the lines kept after it move)
            for (p = txtp; *mark;)
                *p++ = *mark++;
            *p++ = 0;
            *p++ = 0;
            TextChanged = true;
            // Calculate line number of txtp and adjust edy if needed
            {
                int ln = EdLineOf(txtp, NULL);          // (CMM2 (E6): from the line kept)
                // If txtp is off-screen, adjust edy to center it
                if (ln < edy || ln >= edy + VHeight)
                {
                    edy = ln - VHeight / 2;
                    if (edy < 0)
                        edy = 0;
                    if (edy + VHeight > nbrlines)
                        edy = (nbrlines > VHeight) ? nbrlines - VHeight : 0;
                }
                cury = ln - edy;
            }
            PositionCursor(txtp);
            return;
        case EDMOUSE_CELL:
            break;
        default:
            continue;
        }

        x = curx;
        y = cury;
        markmode = true;

        // Calculate visible line range
        char *visStart, *visEnd;
        int ln;
        visStart = findLine(edy, &ln);                  // (CMM2 (E6): from the lines kept, not the text's start)
        visEnd = findLine(edy + VHeight, &ln);

        // Determine the marked range (low to high)
        char *markLow = (mark < txtp) ? mark : txtp;
        char *markHigh = (mark < txtp) ? txtp : mark;

        // first unmark the area not marked as a result of the keystroke (only visible portion)
        // col tracks the absolute text column so we clip output to [edx, edx+VWidth)
        if (oldmark < mark)
        {
            p = oldmark;
            int col = 0;
            {
                char *q = p;
                while (q > EdBuff && *(q - 1) != '\n')
                {
                    col++;
                    q--;
                }
            }
            if (p >= visStart && p < visEnd)
            {
                while (p < mark && *p != '\n' && col < edx)
                {
                    col++;
                    p++;
                }
                if (p < mark && p >= visStart && p < visEnd && *p != '\n')
                {
                    PositionCursor(p);
                    if (EdColourCode)
                        restoreColourFromLineStart(p);
                }
            }
            while (p < mark)
            {
                if (p >= visStart && p < visEnd)
                {
                    if (*p == '\n')
                    {
                        col = 0;
                        p++;
                        while (p < mark && p < visEnd && *p != '\n' && col < edx)
                        {
                            col++;
                            p++;
                        }
                        if (p < mark && p >= visStart && p < visEnd && *p != '\n')
                        {
                            PositionCursor(p);
                            if (EdColourCode)
                                restoreColourFromLineStart(p);
                        }
                        continue;
                    }
                    if (col >= edx && col < edx + VWidth)
                    {
                        if (EdColourCode)
                            SetColour(p, true);
                        MX470PutC(*p);
                        SSputchar(*p, 0);
                    }
                }
                if (*p == '\n')
                    col = 0;
                else
                    col++;
                p++;
            }
        }
        else if (oldmark > mark)
        {
            p = mark;
            int col = 0;
            {
                char *q = p;
                while (q > EdBuff && *(q - 1) != '\n')
                {
                    col++;
                    q--;
                }
            }
            if (p >= visStart && p < visEnd)
            {
                while (oldmark > p && *p != '\n' && col < edx)
                {
                    col++;
                    p++;
                }
                if (oldmark > p && p >= visStart && p < visEnd && *p != '\n')
                {
                    PositionCursor(p);
                    if (EdColourCode)
                        restoreColourFromLineStart(p);
                }
            }
            while (oldmark > p)
            {
                if (p >= visStart && p < visEnd)
                {
                    if (*p == '\n')
                    {
                        col = 0;
                        p++;
                        while (oldmark > p && p < visEnd && *p != '\n' && col < edx)
                        {
                            col++;
                            p++;
                        }
                        if (oldmark > p && p >= visStart && p < visEnd && *p != '\n')
                        {
                            PositionCursor(p);
                            if (EdColourCode)
                                restoreColourFromLineStart(p);
                        }
                        continue;
                    }
                    if (col >= edx && col < edx + VWidth)
                    {
                        if (EdColourCode)
                            SetColour(p, true);
                        MX470PutC(*p);
                        SSputchar(*p, 0);
                    }
                }
                if (*p == '\n')
                    col = 0;
                else
                    col++;
                p++;
            }
        }
        oldmark = mark;
        oldx = x;
        oldy = y;

        // now draw the marked area (only visible portion)
        if (markLow < markHigh)
        {
            // Find where to start drawing (intersection of marked range and visible range)
            char *drawStart = (markLow > visStart) ? markLow : visStart;
            char *drawEnd = (markHigh < visEnd) ? markHigh : visEnd;

            if (drawStart < drawEnd)
            {
                PrintString(EdColourCode ? "\033[44m" : "\033[0m\033[7m");
                EdDisplay(REVERSE_VIDEO);
                p = drawStart;
                int col = 0;
                {
                    char *q = p;
                    while (q > EdBuff && *(q - 1) != '\n')
                    {
                        col++;
                        q--;
                    }
                }
                while (p < drawEnd && *p != '\n' && col < edx)
                {
                    col++;
                    p++;
                }
                if (p < drawEnd && *p != '\n')
                    PositionCursor(p);
                while (p < drawEnd)
                {
                    if (*p == '\n')
                    {
                        col = 0;
                        p++;
                        while (p < drawEnd && *p != '\n' && col < edx)
                        {
                            col++;
                            p++;
                        }
                        if (p < drawEnd && *p != '\n')
                            PositionCursor(p);
                        continue;
                    }
                    if (col >= edx && col < edx + VWidth)
                    {
                        MX470PutC(*p);
                        SSputchar(*p, 0);
                    }
                    col++;
                    p++;
                }
                EdDisplay(REVERSE_VIDEO);
            }
        }
        markmode = false;
        PrintString("\033[0m"); // normal video

        oldx = x;
        oldy = y;
        oldmark = mark;
        PositionCursor(mark);
    }
}

// search through the text in the editing buffer looking for a specific line
// enters with ln = the line required
// exits pointing to the start of the line or pointing to a zero char if not that many lines in the buffer
// inmulti return values:
//   0 = normal
//   1 = inside /* */ multi-line comment
//   2 = (internal) */ found at start of line
//   3 = continuation line inside a quoted string
//   4 = continuation line inside a single-line comment
char *findLine(int ln, int *inmulti)
{
    char *p, *q;
    int want = ln, k = -1;
    *inmulti = false;
    int inquote = false;
    int incomment = false;
    for (int i = 0; i < 2; i++)                         // CMM2 (E6): on from the nearest kept line at or before ln
        if (EdFind[i].p && EdFind[i].n <= ln && (k < 0 || EdFind[i].n > EdFind[k].n))
            k = i;
    if (k >= 0)
    {
        p = EdFind[k].p;
        *inmulti = EdFind[k].m;
        ln -= EdFind[k].n;
    }
    else
    {
        p = q = EdBuff;
        skipspace(q);
        if (q[0] == '/' && q[1] == '*')
            *inmulti = true;
        if (q[0] == '*' && q[1] == '/')
            *inmulti = false;
    }
    while (ln && *p)
    {
        if (*p == '\n')
        {
            // Check if this line continues to the next via continuation character
            if (0 && p >= EdBuff + 2 &&
                *(p - 1) == 0 && *(p - 2) == ' ')
            {
                // Continuation line - carry quote/comment state to next line
            }
            else
            {
                // Not a continuation - reset quote/comment state
                inquote = false;
                incomment = false;
            }
            if (*inmulti == 2)
                *inmulti = false;
            ln--;
            q = &p[1];
            skipspace(q);
            if (q[0] == '/' && q[1] == '*')
                *inmulti = true;
            if (q[0] == '*' && q[1] == '/')
                *inmulti = 2;
        }
        else if (!*inmulti)
        {
            // Track quote and comment state through the text
            if (*p == '\'' && !inquote)
                incomment = true;
            else if (*p == '"' && !incomment)
                inquote = !inquote;
        }
        p++;
    }
    if (ln == 0)
    {                                                   // CMM2 (E6): kept, as the last line looked up, and as the
        q = p;                                          // screen's top line when it is that
        skipspace(q);
        EdFind[1].p = p;
        EdFind[1].dep = q + 2;
        EdFind[1].n = want;
        EdFind[1].m = *inmulti;
        if (want == edy)
            EdFind[0] = EdFind[1];
    }
    // Report continuation colour state via inmulti
    if (!*inmulti && inquote)
        *inmulti = 3;
    else if (!*inmulti && incomment)
        *inmulti = 4;
    return (char *)p;
}

int EditCompStr(char *p, char *tkn)
{
    while (*tkn && (toupper(*tkn) == toupper(*p)))
    {
        if (*tkn == '(' && *p == '(')
            return true;
        if (*tkn == '$' && *p == '$')
            return true;
        tkn++;
        p++;
    }
    if (*tkn == 0 && !isnamechar((unsigned char)*p))
        return true; // return the string if successful

    return false; // or NULL if not
}

// this function does the syntax colour coding
// p = pointer to the current character to be printed
//     or NULL if the colour coding is to be cmdfile to normal
//
// it keeps track of where it is in the line using static variables
// so it must be fed all chars from the start of the line
void SetColour(char *p, int DoVT100)
{
    int i;
    char **pp;
    static int intext = false;
    static int incomment = false;
    static int inkeyword = false;
    static char *twokeyword = NULL;
    static int inquote = false;
    static int innumber = false;

    if (!EdColourCode)
        return;

    // this is a list of keywords that can come after the OPTION and GUI commands
    // the list must be terminated with a NULL
    char *twokeywordtbl[] = {
        "BASE", "EXPLICIT", "DEFAULT", "BREAK", "AUTORUN", "BAUDRATE", "DISPLAY",
        NULL};

    // this is a list of common keywords that should be highlighted as such
    // the list must be terminated with a NULL
    char *specialkeywords[] = {
        "SELECT", "INTEGER", "FLOAT", "STRING", "DISPLAY", "SDCARD", "USBDRIVE", "OUTPUT", "APPEND", "WRITE", "SLAVE", "TARGET", "PROGRAM",
        //        ".PROGRAM", ".END PROGRAM", ".SIDE", ".LABEL" , ".LINE",".WRAP", ".WRAP TARGET",
        NULL};

    // these functions are rewritten to other tokens during tokenise() (see MMBasic.c)
    // so they no longer have their own entry in the command or token tables and would
    // otherwise not be colour coded.  The MM.xxx functions handled by fun_tilde() are
    // dealt with separately below using the overlaid_functions[] table.
    // the list must be terminated with a NULL
    // CMM2: the folded functions are the tokeniser's own list (FoldList, MMBasic.c), below; this is the rest
    char *hiddenfunctions[] = {
        "MM.INFO$(",
        NULL};

    // cmdfile everything back to normal
    if (p == NULL)
    {
        innumber = inquote = inkeyword = incomment = intext = false;
        twokeyword = NULL;
        if (!multilinecomment)
        {
            gui_fcolour = GUI_C_NORMAL;
            if (DoVT100)
                PrintString(VT100_C_NORMAL);
        }
        return;
    }

    // Special init for continuation line colour state
    if (p == (char *)1)
    {
        // Init in quote mode (for continuation lines split inside a string)
        inquote = true;
        gui_fcolour = GUI_C_QUOTE;
        if (DoVT100)
            PrintString(VT100_C_QUOTE);
        return;
    }
    if (p == (char *)2)
    {
        // Init in comment mode (for continuation lines split inside a comment)
        incomment = true;
        gui_fcolour = GUI_C_COMMENT;
        if (DoVT100)
            PrintString(VT100_C_COMMENT);
        return;
    }

    if (*p == '*' && p[1] == '/' && !inquote)
    {
        multilinecomment = 2;
        return;
    }

    if (*p == '/' && !inquote && multilinecomment == 2)
    {
        multilinecomment = false;
        return;
    }

    // check for a comment char
    if (*p == '\'' && !inquote)
    {
        gui_fcolour = GUI_C_COMMENT;
        if (DoVT100)
            PrintString(VT100_C_COMMENT);
        incomment = true;
        return;
    }
    if (*p == '/' && p[1] == '*' && !inquote)
    {
        if (p == EdBuff || p[-1] == (unsigned char)'\n')
        {
            gui_fcolour = GUI_C_COMMENT;
            if (DoVT100)
                PrintString(VT100_C_COMMENT);
            multilinecomment = true;
        }
        return;
    }

    // once in a comment all following chars must be comments also
    if (incomment || multilinecomment)
        return;

    // check for a quoted string
    if (*p == '\"')
    {
        if (!inquote)
        {
            inquote = true;
            gui_fcolour = GUI_C_QUOTE;
            if (DoVT100)
                PrintString(VT100_C_QUOTE);
            return;
        }
        else
        {
            inquote = false;
            return;
        }
    }

    if (inquote)
        return;

    // if we are displaying a keyword check that it is still actually in the keyword and cmdfile if not
    if (inkeyword)
    {
        if (isnamechar(*p) || *p == '$')
            return;
        gui_fcolour = GUI_C_NORMAL;
        if (DoVT100)
            PrintString(VT100_C_NORMAL);
        inkeyword = false;
        return;
    }

    // if we are displaying a number check that we are still actually in it and cmdfile if not
    // this is complicated because numbers can be in hex or scientific notation
    if (innumber)
    {
        if (!isdigit(*p) && !(toupper(*p) >= 'A' && toupper(*p) <= 'F') && toupper(*p) != 'O' && toupper(*p) != 'H' && *p != '.')
        {
            gui_fcolour = GUI_C_NORMAL;
            if (DoVT100)
                PrintString(VT100_C_NORMAL);
            innumber = false;
            return;
        }
        else
        {
            return;
        }
        // check if we are starting a number
    }
    else if (!intext)
    {
        if (isdigit(*p) || *p == '&' || ((*p == '-' || *p == '+' || *p == '.') && isdigit(p[1])))
        {
            gui_fcolour = GUI_C_NUMBER;
            if (DoVT100)
                PrintString(VT100_C_NUMBER);
            innumber = true;
            return;
        }
        // check if this is an 8 digit hex number as used in CFunctions
        for (i = 0; i < 8; i++)
            if (!isxdigit(p[i]))
                break;
        if (i == 8 && (p[8] == ' ' || p[8] == '\'' || p[8] == 0))
        {
            gui_fcolour = GUI_C_NUMBER;
            if (DoVT100)
                PrintString(VT100_C_NUMBER);
            innumber = true;
            return;
        }
    }

    // check if this is the start of a keyword
    if (isnamechar(*p) && !intext)
    {
        for (i = 0; i < CommandTableSize - 1; i++)
        { // check the command table for a match
            if (EditCompStr((char *)p, (char *)commandtbl[i].name) != 0 ||
                ((EditCompStr((char *)&p[1], (char *)&commandtbl[i].name[1]) != 0) && *p == '.' && *commandtbl[i].name == '_'))
            {
                if (EditCompStr((char *)p, "REM") != 0)
                { // special case, REM is a comment
                    gui_fcolour = GUI_C_COMMENT;
                    if (DoVT100)
                        PrintString(VT100_C_COMMENT);
                    incomment = true;
                }
                else
                {
                    gui_fcolour = GUI_C_KEYWORD;
                    if (DoVT100)
                        PrintString(VT100_C_KEYWORD);
                    inkeyword = true;
                    if (EditCompStr((char *)p, "GUI") ||
                        EditCompStr((char *)p, "OPTION"))
                    {
                        twokeyword = p;
                        while (isalnum(*twokeyword))
                            twokeyword++;
                        while (*twokeyword == ' ')
                            twokeyword++;
                    }
                    return;
                }
            }
        }
        for (i = 0; i < TokenTableSize - 1; i++)
        { // check the token table for a match
            if (EditCompStr((char *)p, (char *)tokentbl[i].name) != 0)
            {
                gui_fcolour = GUI_C_KEYWORD;
                if (DoVT100)
                    PrintString(VT100_C_KEYWORD);
                inkeyword = true;
                return;
            }
        }

        // check for the second keyword in two keyword commands
        if (p == twokeyword)
        {
            for (pp = (char **)twokeywordtbl; *pp; pp++)
                if (EditCompStr((char *)p, (char *)*pp))
                    break;
            if (*pp)
            {
                gui_fcolour = GUI_C_KEYWORD;
                if (DoVT100)
                    PrintString(VT100_C_KEYWORD);
                inkeyword = true;
                return;
            }
        }
        if (p >= twokeyword)
            twokeyword = NULL;

        // check for a range of common keywords
        for (pp = (char **)specialkeywords; *pp; pp++)
            if (EditCompStr((char *)p, (char *)*pp))
                break;
        if (*pp)
        {
            gui_fcolour = GUI_C_KEYWORD;
            if (DoVT100)
                PrintString(VT100_C_KEYWORD);
            inkeyword = true;
            return;
        }

        // check for functions that tokenise() rewrites to other tokens
        for (pp = (char **)hiddenfunctions; *pp; pp++)
            if (EditCompStr((char *)p, (char *)*pp))
                break;
        for (i = 0; *pp == NULL && i < FoldCount; i++) // CMM2: and the tokeniser's folds (LEFT$(, MAX(, BIT( ...)
            if (EditCompStr((char *)p, (char *)FoldList[i]))
                pp = (char **)&FoldList[i];
        if (*pp)
        {
            gui_fcolour = GUI_C_KEYWORD;
            if (DoVT100)
                PrintString(VT100_C_KEYWORD);
            inkeyword = true;
            return;
        }

        // check for the MM.xxx functions handled by fun_tilde(); these are rewritten
        // to the ~() token during tokenise() so are not in the token table either
        for (i = 0; i < MMEND; i++)
            if (EditCompStr((char *)p, (char *)overlaid_functions[i]))
            {
                gui_fcolour = GUI_C_KEYWORD;
                if (DoVT100)
                    PrintString(VT100_C_KEYWORD);
                inkeyword = true;
                return;
            }
    }

    // try to keep track of if we are in general text or not
    // this is to avoid recognising keywords or numbers inside variables
    if (isnamechar(*p))
    {
        intext = true;
    }
    else
    {
        intext = false;
        gui_fcolour = GUI_C_NORMAL;
        if (DoVT100)
            PrintString(VT100_C_NORMAL);
    }
}

// Set up SetColour state for the character at 'target' and emit the matching
// VT100 colour escape.  Used by the unmark loops so the terminal colour is
// correct when visible characters are output after an edx-skip.
void restoreColourFromLineStart(char *target)
{
    int inmulti = 0;
    int ln = EdLineOf(target, NULL);                    // (CMM2 (E6): from the line kept)
    char *q;
    SetColour(NULL, false);
    char *ls = (char *)findLine(ln, &inmulti);
    if (inmulti == 3)
        SetColour((char *)1, false);
    else if (inmulti == 4)
        SetColour((char *)2, false);
    for (q = ls; q < target && *q && *q != '\n'; q++)
        if (!inmulti || inmulti == 3)
            SetColour(q, false);
    if (gui_fcolour == GUI_C_COMMENT)
        PrintString(VT100_C_COMMENT);
    else if (gui_fcolour == GUI_C_QUOTE)
        PrintString(VT100_C_QUOTE);
    else if (gui_fcolour == GUI_C_KEYWORD)
        PrintString(VT100_C_KEYWORD);
    else if (gui_fcolour == GUI_C_NUMBER)
        PrintString(VT100_C_NUMBER);
    else
        PrintString(VT100_C_NORMAL);
}

// print a line starting at the current column (edx) at the current cursor.
// if the line is beyond the end of the text then just clear to the end of line
// enters with the line number to be printed
void printLine(int ln)
{
    char *p;
    int i;
    int inmulti = false;
    // we always colour code the output to the LCD panel on the MX470 (when used as the console)
    if ((OptionConsole & 2))
    {
        MX470PutC('\r'); // print on the MX470 display
        p = findLine(ln, &inmulti);
        // Init colour state for continuation lines split inside a string or comment
        if (inmulti == 3)
            SetColour((char *)1, false); // init quote state
        else if (inmulti == 4)
            SetColour((char *)2, false); // init comment state
        // skip edx characters, advancing colour state without displaying
        for (i = edx; i > 0 && *p && *p != '\n'; i--)
        {
            if (!inmulti || inmulti == 3)
                SetColour((char *)p, false);
            p++;
        }
        i = VWidth;
        while (i && *p && *p != '\n')
        {
            if (!inmulti || inmulti == 3)
                SetColour((char *)p, false); // set the colour for the LCD display only
            else if (EdColourCode)                  // (CMM2: only when colour coding: SetColour, which puts the colour
                gui_fcolour = GUI_C_COMMENT;        // back, does nothing without it, so a /* block left the rest yellow)
            MX470PutC(*p++); // print on the MX470 display
            i--;
        }
        EdDisplay(CLEAR_TO_EOL); // clear to the end of line on the MX470 display only
    }
    SetColour(NULL, false);

    p = findLine(ln, &inmulti);
    if (EdColourCode || edx > 0)
    {
        if (EdColourCode)
        {
            // Init colour state for continuation lines split inside a string or comment
            if (inmulti == 3)
                SetColour((char *)1, true); // init quote state
            else if (inmulti == 4)
                SetColour((char *)2, true); // init comment state
            // skip edx characters, advancing colour state without emitting codes
            for (i = edx; i > 0 && *p && *p != '\n'; i--)
            {
                if (!inmulti || inmulti == 3)
                    SetColour((char *)p, false);
                p++;
            }
            // Re-emit the current colour after skipping off-screen chars so the
            // terminal reflects state set by syntax elements that are now off-screen
            // (e.g. a comment ' that has scrolled left of the visible area).
            if (edx > 0)
            {
                if (gui_fcolour == GUI_C_COMMENT)
                    PrintString(VT100_C_COMMENT);
                else if (gui_fcolour == GUI_C_QUOTE)
                    PrintString(VT100_C_QUOTE);
                else if (gui_fcolour == GUI_C_KEYWORD)
                    PrintString(VT100_C_KEYWORD);
                else if (gui_fcolour == GUI_C_NUMBER)
                    PrintString(VT100_C_NUMBER);
                else
                    PrintString(VT100_C_NORMAL);
            }
        }
        else
        {
            // no colour coding: just skip edx characters
            for (i = edx; i > 0 && *p && *p != '\n'; i--)
                p++;
        }
        SSputchar('\r', 0); // go to start of line on the VT100 emulator
        i = VWidth;
    }
    else
    {
        // if we are NOT colour coding we can start drawing at the current cursor position
        i = curx;
        while (i-- && *p && *p != '\n')
            p++; // find the editing point in the buffer
        i = VWidth - curx;
    }

    while (i && *p && *p != '\n')
    {
        if (EdColourCode)
        {
            if (!inmulti || inmulti == 3)
                SetColour((char *)p, true); // if colour coding is used set the colour for the VT100 emulator
            else
            {
                gui_fcolour = GUI_C_COMMENT;
                PrintString(VT100_C_COMMENT);
            }
        }
        SSputchar(*p++, 0); // display the chars after the editing point
        i--;
    }

    PrintString("\033[K"); // all done, clear to the end of the line on a vt100 emulator
    if (EdColourCode)
        SetColour(NULL, true);
    curx = VWidth - 1;
}

// print a full screen starting with the top left corner specified by edx, edy
// this draws the full screen including blank areas so there is no need to clear the screen first
// it then returns the cursor to its original position
void printScreen(void)
{
    int i;

    SCursor(0, 0);
    for (i = 0; i < VHeight; i++)
    {
        printLine(i + edy);
        PrintString("\r\n");
        EdPutS("\r\n", gui_fcolour, gui_bcolour);
        curx = 0;
        cury = i + 1;
    }
    EdDropHeldRepeats();                                // (a held key's repeats queued while redrawing the screen)
}

// position the cursor on the screen
void SCursor(int x, int y)
{
    char s[12];

    PrintString("\033[");
    IntToStr(s, y + 1, 10);
    PrintString(s);
    PrintString(";");
    IntToStr(s, x + 1, 10);
    PrintString(s);
    PrintString("H");
    EdCursor(x * gui_font_width, y * gui_font_height); // position the cursor on the MX470 display only
    curx = x;
    cury = y;
}

// move the text down by one char starting at the current position in the text
// and insert a character
int editInsertChar(unsigned char c, char *multi, int edit_buff_size)
{
    char *p;

    p = txtp + strlen(txtp); // find the end of the text in memory (CMM2 (E6): from the cursor, not the start)
    if (p >= EdBuff + edit_buff_size - 10)
    { // and check that we have the space (allow 10 bytes for slack)
        editDisplayMsg(" OUT OF MEMORY ");
        return false;
    }
    EdChanged(txtp);                                    // (CMM2 (E6): the lines kept after it move)
    for (; p >= txtp; p--)
        *(p + 1) = *p; // shift everything down
    *multi = 0;
    if (txtp > EdBuff)
    {
        unsigned char prev = *(txtp - 1);
        if ((c == '/' && prev == '*') || (c == '*' && prev == '/'))
            *multi = 1;
    }
    {
        unsigned char next = *(txtp + 1);
        if ((c == '/' && next == '*') || (c == '*' && next == '/'))
            *multi = 1;
    }
    *txtp++ = c; // and insert our char
    return true;
}

// return the character length of the line that txtp sits on
static int current_line_length(void)
{
    char *p = txtp;
    while (p > EdBuff && *(p - 1) != '\n')
        p--;
    int len = 0;
    while (*p && *p != '\n')
    {
        p++;
        len++;
    }
    return len;
}

// Pick the longest function-key legend that fits in `avail` columns. The
// shortest entry ("EDIT MODE"/"MARK MODE") is always the safe fallback.
static const char *pickFunctKeyString(int typ, int avail)
{
    static const char *edit_keys[] = {
        "ESC:Exit F1:Save F2:Run F3/6:Find/r F4:Mrk F5:Paste F7/8:Rpl/r F9:In F10:Out F11:Help F12/^A:Beautify",
        "F1:Save F2:Run F3:Find F4:Mark F5:Paste F7/8:Rpl/r F11:Help F12/^A:Btfy",    // (CMM2: F11, E5)
        "F1:Save F2:Run F3:Find F4:Mrk F5:Pst F9:In F10:Out F11:Hlp",
        "F1:Save F2:Run F3:Find F4:Mrk F5:Pst F11:Hlp ^A:Btfy",
        "EDIT MODE",
    };
    static const char *mark_keys[] = {
        "MARK MODE  ESC=Exit DEL:Delete F4:Cut F5:Copy F10:Export",
        "MARK MODE  ESC:Exit DEL:Del F4:Cut F5:Cpy F10:Out",
        "MARK MODE",
    };
    const char **list = (typ == EDIT) ? edit_keys : mark_keys;
    int n = (typ == EDIT) ? (int)(sizeof(edit_keys) / sizeof(edit_keys[0]))
                          : (int)(sizeof(mark_keys) / sizeof(mark_keys[0]));
    for (int i = 0; i < n; i++)
    {
        if ((int)strlen(list[i]) <= avail)
            return list[i];
    }
    return list[n - 1];
}

/* CMM2: the status bar as kilo has it (Editor_Port_Plan.md, The screen), on the row under the text: the file, its
   lines and "(modified)" at the left; INS or OVR, the file type and column/row/lines at the right (and LONG on a
   BASIC line longer than a program line may be); in reverse video. The PicoMite's has the keys there, and the cursor's
   line and column at the right of the row under them. */
static void EdStatusBar(void)
{
    char left[128], right[48], line[256], clip[32] = "";
    int x = curx, y = cury, n, r, w = VWidth < (int)sizeof(line) ? VWidth : (int)sizeof(line) - 1;
    if (EdClipLen)                                      // (E4: the clipboard's size, as kilo showed it)
        snprintf(clip, sizeof(clip), ", Paste Buffer %d chars", EdClipLen);
    snprintf(left, sizeof(left), "%.60s - %d lines%s%s", EdFileName, nbrlines, TextChanged ? " (modified)" : "", clip);
    r = snprintf(right, sizeof(right), "%s%s,%s | %d/%d/%d", (edit_is_bas && current_line_length() > MAXSTRLEN) ? "LONG " : "",
                 insert ? "INS" : "OVR", edit_is_bas ? "bas" : "no ft", edx + curx + 1, edy + cury + 1, nbrlines);
    memset(line, ' ', w);
    line[w] = 0;
    n = strlen(left);
    if (n > w)
        n = w;
    memcpy(line, left, n);
    if (r < w - n)
        memcpy(line + w - r, right, r);
    SCursor(0, VHeight);
    PrintString("\033[7m");
    PrintString(line);
    PrintString("\033[m");
    EdCursor(0, VHeight * gui_font_height);
    EdPutS(line, BLACK, WHITE);
    SCursor(x, y);
}

// print the function keys at the bottom of the screen (CMM2: and the status bar above them)
void PrintFunctKeys(int typ)
{
    int x = curx, y = cury;
    const char *p = pickFunctKeyString(typ, VWidth);

    EdMsgUp = 0;                                        // (CMM2: the message line is the legend again)
    EdStatusBar();
    EdCursor(0, (VHeight + 1) * gui_font_height);
    EdPutS((char *)p, GUI_C_STATUS, gui_bcolour); // display the string on the display attached to the MX470
    EdDisplay(CLEAR_TO_EOL);                      // clear to the end of line on the MX470 display only
    SCursor(0, VHeight + 1);
    if (EdColourCode)
        PrintString(VT100_C_STATUS);
    PrintString((char *)p);
    if (EdColourCode)
        PrintString(VT100_C_NORMAL);
    PrintString("\033[K"); // clear to the end of the line on a vt100 emulator
    SCursor(x, y);
}

// print the current status (CMM2: the status bar)
void EdPrintStatus(void)
{
    EdStatusBar();
    PositionCursor(txtp);
}

// display a message in the status line
void editDisplayMsg(char *msg)
{
    if (msg != EdLastMsg)
        snprintf(EdLastMsg, sizeof(EdLastMsg), "%s", msg);
    EdMsgUp = 1;
    SCursor(0, VHeight + 1);
    if (EdColourCode)
        PrintString(VT100_C_ERROR);
    PrintString("\033[7m");
    EdCursor(0, (VHeight + 1) * gui_font_height);
    PrintString((char *)msg);
    EdPutS((char *)msg, BLACK, RED);
    if (EdColourCode)
        PrintString(VT100_C_NORMAL);
    PrintString("\033[0m");
    PrintString("\033[K");      // clear to the end of the line on a vt100 emulator
    EdDisplay(CLEAR_TO_EOL); // clear to the end of line on the MX470 display only
    PositionCursor(txtp);
    drawstatusline = true;
}

// save the program in the editing buffer into the program memory
// get an input string from the user and save into inpbuf
/* CMM2 (E5): with def, that text is offered: shown after the prompt, Enter takes it, a typed character replaces it and
   Backspace edits it (F3's Find offers the last search). F3, SHIFT-F3 and Esc return alone in inpbuf, as the callers
   test them (after typed text they were added to it). */
static void EdGetInput(char *prompt, char *def)
{
    int i, fresh = 0;
    SCursor(0, VHeight + 1);
    PrintString((char *)prompt);
    EdCursor(0, (VHeight + 1) * gui_font_height);
    EdPutS((char *)prompt, gui_fcolour, gui_bcolour);
    for (i = 0; i < VWidth - strlen((char *)prompt); i++)
    {
        SSputchar(' ', 1);
        MX470PutC(' ');
    }
    SCursor(strlen((char *)prompt), VHeight + 1);
    EdCursor(strlen((char *)prompt) * gui_font_width, (VHeight + 1) * gui_font_height);
    int len = 0;
    int maxlen = STRINGSIZE - 1;
    if (def && *def)
    {                                                   // (as much of it as the line has room for)
        for (len = 0; def[len] && len < maxlen && len < VWidth - (int)strlen(prompt) - 1; len++)
            inpbuf[len] = def[len];
        inpbuf[len] = 0;
        PrintString((char *)inpbuf);
        EdPutS((char *)inpbuf, gui_fcolour, gui_bcolour);
        fresh = 1;
    }
    while (1)
    { // get the input
        unsigned char ch = EdGetKey();
        if (ch == '\n')                                 // (CMM2: Enter is '\n', EdInkey)
            break;
        if (ch == 0xb3 || ch == F3 || ch == ESC)
        {
            inpbuf[0] = ch;
            len = 1;
            break;
        } // return if it is SHIFT-F3, F3 or ESC
        if (isprint(ch) && fresh)
        {                                               // (a character typed: the offered text goes)
            while (len > 0)
            {
                len--;
                PrintString("\b \b");
                EdPutS("\b \b", gui_fcolour, gui_bcolour);
            }
        }
        fresh = 0;
        if (ch == '\b')
        {
            if (len > 0)
            {
                len--;
                PrintString("\b \b");                         // erase on the screen
                EdPutS("\b \b", gui_fcolour, gui_bcolour); // erase on the MX470 display
            }
            continue;
        }
        if (isprint(ch))
        {
            if (len < maxlen)
            {
                inpbuf[len++] = ch;
                SSputchar(ch, 1); // echo the char
                MX470PutC(ch);    // echo the char on the MX470 display
            }
        }
    }
    inpbuf[len] = 0; // terminate the input string
    PositionCursor(txtp);                               // CMM2: (first: the status bar gives the cursor's place)
    PrintFunctKeys(EDIT);
}

void GetInputString(char *prompt)
{
    EdGetInput(prompt, NULL);
}

// scroll up the video screen
void Scroll(void)
{
    edy++;
    SCursor(0, VHeight);
    PrintString("\033[J\033[99B\n"); // clear to end of screen, move to the end of the screen and force a scroll of one line
    EdCursor(0, VHeight * gui_font_height);
    MX470Scroll(gui_font_height);
    SCursor(0, VHeight);
    curx = 0;
    cury = VHeight - 1;
    PrintFunctKeys(EDIT);
    printLine(VHeight - 1 + edy);
    PositionCursor(txtp);
    // (CMM2: the keys typed meanwhile kept: see printScreen)
}

// scroll down the video screen
void ScrollDown(void)
{
    SCursor(0, VHeight);   // go to the end of the editing area
    PrintString("\033[J"); // clear to end of screen
    edy--;
    SCursor(0, 0);
    PrintString("\033M"); // scroll window down one line
    MX470Scroll(-gui_font_height);
    printLine(edy);
    PrintFunctKeys(EDIT);
    PositionCursor(txtp);
    // (CMM2: the keys typed meanwhile kept: see printScreen)
}

/* CMM2 (E6): mark mode's scroll by a line, as Scroll and ScrollDown are edit mode's: the screen moved and the new
   line drawn, where mark mode redrew the whole screen for every line (a held arrow outran the console's queue).
   curx and cury, the selection's end, are kept; MarkMode then draws the selection. */
static void EdMarkScroll(int down)
{
    int x = curx, y = cury;
    SCursor(0, VHeight);
    PrintString("\033[J");                              // (the status bar and the key line, drawn again below)
    if (down)
    {
        edy++;
        PrintString("\033[99B\n");                      // to the end of the screen, where a line feed scrolls it
        EdCursor(0, VHeight * gui_font_height);
        MX470Scroll(gui_font_height);
        SCursor(0, VHeight - 1);
        printLine(edy + VHeight - 1);
    }
    else
    {
        edy--;
        SCursor(0, 0);
        PrintString("\033M");                           // reverse index at the top: the screen moves down a line
        MX470Scroll(-gui_font_height);
        SCursor(0, 0);
        printLine(edy);
    }
    curx = x;                                           // (first: the status bar gives the selection's end)
    cury = y;
    PrintFunctKeys(MARK);
}
