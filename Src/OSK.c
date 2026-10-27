/* OSK.c - the PicoMite's on-screen keyboard (its graphics/GUI.c), copied by tools/port_osk.py: do not edit, run it
   again. What the CMM2 changes is listed there. In bank 2 (STM32H743II_FLASH.ld): nothing an interrupt runs is here. */
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "keyboard.h"

#define CLICK_DURATION 3 /* (GUI.c's: a click's mS) */
extern void SpecialDrawRBox(int x1, int y1, int x2, int y2, int radius, int c, int fill, int status);
extern void SpecialPrintString(int x, int y, int jh, int jv, int jo, int fc, int bc, char *str, int status);

/* the PicoMite's CursorSuspend and CursorHide, the CMM2's way: the cursor off the display while the keyboard draws
   (its saved pixels would be the half-drawn keys'), and back after; nested, as OSK_DrawAll's OSK_DrawKeyAt are */
static int osk_cursor_depth = 0, osk_cursor_was = 0;
static void OSK_CursorOff(void)
{
    if (osk_cursor_depth++ == 0)
    {
        osk_cursor_was = cursoron;
        if (cursoron)
            hidecursor(1);
    }
}
static void OSK_CursorBack(void)
{
    if (--osk_cursor_depth == 0 && osk_cursor_was && cursorenable && !cursoron)
        showcursor(1, xcursor, ycursor);
}

/* ============================================================================
 * On-Screen Keyboard (OSK)
 *
 * Lives inside the bottom strip reserved by OptionVResreserved. Two modes:
 *   - System mode (osk_state==OSK_STATE_SYSTEM): drawn automatically at the
 *     command prompt and in the editor.
 *   - Program mode (osk_state==OSK_STATE_PROGRAM): only when a running program
 *     calls KEYBOARD ON.
 * Keys are injected via USR_KEYBRD_ProcessData() so INKEY$ / INPUT / KEY
 * interrupts all see them through the same path as the USB HID host.
 * ============================================================================ */
extern void USR_KEYBRD_ProcessData(uint8_t data);

#define OSK_STATE_OFF 0
#define OSK_STATE_SYSTEM 1
#define OSK_STATE_PROGRAM 2

#define OSK_COLS 12
#define OSK_ROWS 4
#define OSK_KEYS (OSK_COLS * OSK_ROWS)

#define OSK_GAP_LARGE 3
#define OSK_GAP_SMALL 2

/* Special caption-only marker values stored in the value table. Anything
   in 0x01..0xFF is emitted verbatim via USR_KEYBRD_ProcessData. */
#define OSK_V_NONE 0 /* dead cell (e.g. extra Space cells) */
#define OSK_V_SHIFT 0x100
#define OSK_V_CTRL 0x101
#define OSK_V_SYM 0x102
#define OSK_V_FN 0x103

int osk_state = OSK_STATE_OFF; /* (the CMM2: the 1 mS tick reads it, GUI.c) */
static bool osk_shift = false;
static bool osk_ctrl = false;
static bool osk_sym = false;
static bool osk_fn = false;        /* Fn page: F-keys + Home/End/PgUp/PgDn/Ins/Del */
static int osk_pressed = -1;       /* key index currently shown pressed, -1 = none */
static int osk_drawn_reserved = 0; /* what OptionVResreserved was when last drawn */
static bool osk_dirty = false;     /* pixels invalidated by an external operation */
/* ---- Multi-touch (Option.Multi) state -----------------------------------
   In multi-touch mode the modifier keys (Sh, Ctl, Sym, Fn) behave like
   physical keys: pressed-and-held by one finger while another finger taps
   the target key. We track the second contact's edges across ProcessTouch
   calls via the USB touch driver's usb_touch_active2 + x2/y2 globals. */
static bool osk_finger2_was_active = false;
static int osk_finger2_key = -1; /* the cell finger 2 last landed on */
/* True while finger 1 is holding a momentary modifier in multi-touch mode.
   Used so the TouchUp release of that finger correctly disengages the
   modifier instead of latch-toggling it (the toggle-mode behaviour). */
static bool osk_finger1_holds_modifier = false;
/* Set by KEYBOARD OFF at the command prompt; cleared by KEYBOARD ON. While
   set, the prompt's automatic OSK_OnPromptIdle path skips both the restore
   of OptionVResreserved and the redraw, so KEYBOARD OFF at the prompt
   actually persists instead of being undone on the very next iteration. */
static bool osk_user_disabled = false;

/* Option.Height = (VRes - VRes*OptionVResreserved/100) / gui_font_height —
   it's computed by SetFont using the current OptionVResreserved. When we
   change OptionVResreserved we must re-derive it, otherwise consumers like
   FM that read Option.Height directly see a stale row count and lay out as
   if the strip weren't reserved (or weren't unreserved). Calling
   SetFont(gui_font) is the cheapest way to recompute, since SetFont also
   re-derives Option.Width — we don't need that, but it's harmless. */
static void OSK_RefreshOptionHeight(void)
{
    if (gui_font)
        SetFont(gui_font);
}

/* Reset all modifier latches AND multi-touch tracking state. Used by
   state-transition helpers (OSK_OnRunStart / OSK_OnPromptIdle / etc.) so
   stale shift/fn/etc. don't carry across program runs and a held finger
   isn't remembered after the OSK has been redrawn. */
static void OSK_ResetModifiers(void)
{
    osk_shift = osk_ctrl = osk_sym = osk_fn = false;
    osk_finger1_holds_modifier = false;
    osk_finger2_was_active = false;
    osk_finger2_key = -1;
}

static void OSK_ZeroReserved(void)
{
    OptionVResreserved = 0;
    OSK_RefreshOptionHeight();
}
static void OSK_RestoreReserved(void)
{
    OptionVResreserved = VideoColour == 1 ? 0 : Option.VRes_reserved; /* (none in MODE n,1: Peter) */
    OSK_RefreshOptionHeight();
}

/* Scroll the screen up by the about-to-be-reserved strip's height so the
   prompt's command history (or whatever's at the bottom of the screen) is
   preserved instead of overwritten when the OSK appears. Must be called
   while OptionVResreserved is still 0 — ScrollLCD reads it to decide the
   scrollable region, so calling after the strip has been reserved would
   only scroll within the already-shrunk editable area and leave the strip
   contents untouched. Rounds the scroll amount up to a whole font cell so
   text rows stay aligned. */
static void OSK_ScrollForStrip(void)
{
    if (OptionVResreserved != 0)
        return; /* strip already reserved */
    if (Option.VRes_reserved == 0)
        return; /* no strip configured */
    if (gui_font_height <= 0)
        return;
    int reservedH = VRes * Option.VRes_reserved / 100;
    int scroll_amount = ((reservedH + gui_font_height - 1) / gui_font_height) * gui_font_height;
    if (scroll_amount <= 0)
        return;
    if (ScrollLCD != NULL)
        ScrollLCD(scroll_amount, 1);
    if (CurrentY >= scroll_amount)
        CurrentY -= scroll_amount;
    else
        CurrentY = 0;
}

/* Key indices follow the GetSingleKeyCoord convention: k%COLS is the column,
   k/COLS is the row counted from the BOTTOM (so row 3 is visually on top).
   Bottom row layout (constant across all pages):
     col 0: Sym    col 1: Fn     col 2: Ctl   col 3: Esc   col 4: Tab
     col 5..7: space bar (one wide button, three cells)
     col 8..11: arrows left/down/up/right */
#define OSK_K(row, col) ((row) * OSK_COLS + (col))
#define OSK_KEY_SHIFT_INDEX OSK_K(1, 0)
#define OSK_KEY_SYM_INDEX OSK_K(0, 0)
#define OSK_KEY_FN_INDEX OSK_K(0, 1)
#define OSK_KEY_CTRL_INDEX OSK_K(0, 2)

/* Bottom row is identical in every page. Captioned "Space" so the wide
   button shows a clear label; the empty-caption alternative also works but
   reads less obviously as the space bar. */
#define OSK_BOTTOM_CAPS \
    "Sym", "Fn", "Ctl", "Esc", "Tab", "Space", OSK_DEAD_CAP, OSK_DEAD_CAP, "<-", "v", "^", "->"
#define OSK_BOTTOM_VALS \
    OSK_V_SYM, OSK_V_FN, OSK_V_CTRL, 0x1B, '\t', ' ', OSK_V_NONE, OSK_V_NONE, 0x82, 0x81, 0x80, 0x83
#define OSK_DEAD_CAP "" /* caption for cells absorbed by a wide button */

static const char *const osk_cap_alpha_lo[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_CAPS,
    /* row 1 */ "Sh",
    "z",
    "x",
    "c",
    "v",
    "b",
    "n",
    "m",
    ",",
    ".",
    "/",
    "Del",
    /* row 2 */ "a",
    "s",
    "d",
    "f",
    "g",
    "h",
    "j",
    "k",
    "l",
    ";",
    "'",
    "Ent",
    /* row 3 */ "q",
    "w",
    "e",
    "r",
    "t",
    "y",
    "u",
    "i",
    "o",
    "p",
    "\"",
    "Bsp",
};
static const char *const osk_cap_alpha_hi[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_CAPS,
    /* row 1 */ "Sh",
    "Z",
    "X",
    "C",
    "V",
    "B",
    "N",
    "M",
    "<",
    ">",
    "?",
    "Del",
    /* row 2 */ "A",
    "S",
    "D",
    "F",
    "G",
    "H",
    "J",
    "K",
    "L",
    ":",
    "'",
    "Ent",
    /* row 3 */ "Q",
    "W",
    "E",
    "R",
    "T",
    "Y",
    "U",
    "I",
    "O",
    "P",
    "\"",
    "Bsp",
};
static const char *const osk_cap_sym_lo[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_CAPS,
    /* row 1 */ "Sh",
    "!",
    "@",
    "#",
    "$",
    "%",
    "^",
    "&",
    "*",
    "(",
    ")",
    "Del",
    /* row 2 */ "`",
    "=",
    "[",
    "]",
    "\\",
    "\"",
    "'",
    ",",
    ".",
    "/",
    "+",
    "Ent",
    /* row 3 */ "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "-",
    "Bsp",
};
static const char *const osk_cap_sym_hi[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_CAPS,
    /* row 1 */ "Sh",
    "!",
    "@",
    "#",
    "$",
    "%",
    "^",
    "&",
    "*",
    "(",
    ")",
    "Del",
    /* row 2 */ "~",
    "+",
    "{",
    "}",
    "|",
    ":",
    "\"",
    "<",
    ">",
    "?",
    "=",
    "Ent",
    /* row 3 */ "!",
    "@",
    "#",
    "$",
    "%",
    "^",
    "&",
    "*",
    "(",
    ")",
    "_",
    "Bsp",
};
static const char *const osk_cap_fn[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_CAPS,
    /* row 1 */ "Sh",
    "<",
    ">",
    "|",
    ":",
    ";",
    "'",
    ",",
    ".",
    "/",
    "+",
    "Del",
    /* row 2 */ "F12",
    "Hm",
    "End",
    "PU",
    "PD",
    "Ins",
    "Del",
    "\"",
    "`",
    "~",
    "\\",
    "Ent",
    /* row 3 */ "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "Bsp",
};

/* Corresponding value tables. OSK_V_NONE entries are dead cells; bytes
   0x01..0xFF are emitted verbatim. Single-byte arrow / nav / function
   key codes match Hardware_Includes.h. */
static const int osk_val_alpha_lo[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_VALS,
    /* row 1 */ OSK_V_SHIFT,
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0x7F,
    /* row 2 */ 'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '\r',
    /* row 3 */ 'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '"',
    '\b',
};
static const int osk_val_alpha_hi[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_VALS,
    /* row 1 */ OSK_V_SHIFT,
    'Z',
    'X',
    'C',
    'V',
    'B',
    'N',
    'M',
    '<',
    '>',
    '?',
    0x7F,
    /* row 2 */ 'A',
    'S',
    'D',
    'F',
    'G',
    'H',
    'J',
    'K',
    'L',
    ':',
    '\'',
    '\r',
    /* row 3 */ 'Q',
    'W',
    'E',
    'R',
    'T',
    'Y',
    'U',
    'I',
    'O',
    'P',
    '"',
    '\b',
};
static const int osk_val_sym_lo[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_VALS,
    /* row 1 */ OSK_V_SHIFT,
    '!',
    '@',
    '#',
    '$',
    '%',
    '^',
    '&',
    '*',
    '(',
    ')',
    0x7F,
    /* row 2 */ '`',
    '=',
    '[',
    ']',
    '\\',
    '"',
    '\'',
    ',',
    '.',
    '/',
    '+',
    '\r',
    /* row 3 */ '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '\b',
};
static const int osk_val_sym_hi[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_VALS,
    /* row 1 */ OSK_V_SHIFT,
    '!',
    '@',
    '#',
    '$',
    '%',
    '^',
    '&',
    '*',
    '(',
    ')',
    0x7F,
    /* row 2 */ '~',
    '+',
    '{',
    '}',
    '|',
    ':',
    '"',
    '<',
    '>',
    '?',
    '=',
    '\r',
    /* row 3 */ '!',
    '@',
    '#',
    '$',
    '%',
    '^',
    '&',
    '*',
    '(',
    ')',
    '_',
    '\b',
};
static const int osk_val_fn[OSK_KEYS] = {
    /* row 0 */ OSK_BOTTOM_VALS,
    /* row 1 */ OSK_V_SHIFT,
    '<',
    '>',
    '|',
    ':',
    ';',
    '\'',
    ',',
    '.',
    '/',
    '+',
    0x7F,
    /* row 2 */ 0x9C,
    0x86,
    0x87,
    0x88,
    0x89,
    0x84,
    0x7F,
    '"',
    '`',
    '~',
    '\\',
    '\r',
    /* row 3 */ 0x91,
    0x92,
    0x93,
    0x94,
    0x95,
    0x96,
    0x97,
    0x98,
    0x99,
    0x9A,
    0x9B,
    '\b',
};

static const char *const *OSK_CaptionTable(void)
{
    if (osk_fn)
        return osk_cap_fn;
    if (osk_sym)
        return osk_shift ? osk_cap_sym_hi : osk_cap_sym_lo;
    return osk_shift ? osk_cap_alpha_hi : osk_cap_alpha_lo;
}
static const int *OSK_ValueTable(void)
{
    if (osk_fn)
        return osk_val_fn;
    if (osk_sym)
        return osk_shift ? osk_val_sym_hi : osk_val_sym_lo;
    return osk_shift ? osk_val_alpha_hi : osk_val_alpha_lo;
}

/* Reserved-area top edge — same formula used by Editor.c and Draw.c. */
static int OSK_ReservedTop(void)
{
    return VRes - (VRes * OptionVResreserved / 100);
}

/* Layout: compute the box for key k. Mirrors GetSingleKeyCoord's math but
   anchored to the reserved strip rather than to half the screen. */
static void OSK_GetCoord(int k, int *x1, int *y1, int *x2, int *y2)
{
    int reservedH = VRes - OSK_ReservedTop();
    int gap = (HRes >= 480) ? OSK_GAP_LARGE : OSK_GAP_SMALL;
    int width = (HRes - gap * (OSK_COLS + 1)) / OSK_COLS;
    int height = (reservedH - gap * (OSK_ROWS + 1)) / OSK_ROWS;
    int baseHoriz = HRes - gap;
    int baseVert = VRes - gap;
    *x1 = baseHoriz - (OSK_COLS - (k % OSK_COLS)) * (width + gap);
    *y1 = baseVert - ((k / OSK_COLS) + 1) * (height + gap);
    *x2 = *x1 + width;
    *y2 = *y1 + height;
}

/* Color choices: highlight latched modifiers so the user can see the state. */
static void OSK_KeyColors(int k, int *fc, int *bc, bool pressed)
{
    int normal_fc = WHITE;
    int normal_bc = BLACK;
    int latched_fc = BLACK;
    int latched_bc = GREEN;

    bool latched = false;
    if (k == OSK_KEY_SHIFT_INDEX && osk_shift)
        latched = true;
    if (k == OSK_KEY_CTRL_INDEX && osk_ctrl)
        latched = true;
    if (k == OSK_KEY_SYM_INDEX && osk_sym)
        latched = true;
    if (k == OSK_KEY_FN_INDEX && osk_fn)
        latched = true;

    if (pressed)
    {
        *fc = normal_bc;
        *bc = normal_fc;
        return;
    }
    if (latched)
    {
        *fc = latched_fc;
        *bc = latched_bc;
        return;
    }
    *fc = normal_fc;
    *bc = normal_bc;
}

static void OSK_DrawKeyAt(int k)
{
    int x1, y1, x2, y2, fc, bc;
    const char *const *caps = OSK_CaptionTable();
    const int *vals = OSK_ValueTable();
    /* NONE cells are dead — drawn as part of their left-anchor's wide box. */
    if (vals[k] == OSK_V_NONE)
        return;
    /* Take the cursor off the screen and hold CursorRefresh off for the
       whole redraw.  Pressing a key inverts it; if the cursor is sitting
       on that key its sprite is inverted along with the key AND its save
       buffer still holds the pre-invert pixels, so the next move restores
       a non-inverted block into the inverted key - anything from a single
       white line to a full cursor-sized white rectangle depending on how
       far the pointer travelled.  The release only inverts the key back,
       so the corrupt patch survives.  Touch never shows this because
       there is no cursor to save and restore. */
    OSK_CursorOff();
    /* DrawSingleKey calls GUIPrintString which clobbers CurrentX/CurrentY
       (they become the caption's drawing coordinates). Save and restore so
       the prompt's text cursor doesn't get yanked into the keyboard. */
    short savedX = CurrentX, savedY = CurrentY;
    OSK_GetCoord(k, &x1, &y1, &x2, &y2);
    /* Extend the box rightward to absorb adjacent NONE cells in the same
       row — this is how the space bar covers three columns as one button. */
    int nk = k + 1;
    while (nk < OSK_KEYS && (nk % OSK_COLS) != 0 && vals[nk] == OSK_V_NONE)
    {
        int xx1, yy1, xx2, yy2;
        OSK_GetCoord(nk, &xx1, &yy1, &xx2, &yy2);
        x2 = xx2;
        nk++;
    }
    OSK_KeyColors(k, &fc, &bc, k == osk_pressed);
    /* SpecialPrintString treats '~' inside the caption as a line-split marker
       and mutates the string in place. Our captions live in flash, so pass a
       writable stack copy. Keep the buffer comfortably larger than any
       caption we use (max 3 chars today). */
    char captionbuf[8];
    const char *src = caps[k] ? caps[k] : "";
    int i = 0;
    while (i < (int)sizeof(captionbuf) - 1 && src[i])
    {
        captionbuf[i] = src[i];
        i++;
    }
    captionbuf[i] = 0;
    /* Draw inline rather than via DrawSingleKey: DrawSingleKey hard-codes the
       font via SetFont(HRes>400 ? 0x31 : 0x01) which overrides whatever font
       the user / prompt has chosen. We want the OSK to render in the current
       gui_font so it stays visually consistent with the rest of the UI. */
    SpecialDrawRBox(x1, y1, x2, y2, 10, fc, bc, 0);
    SpecialPrintString(x1 + (x2 - x1) / 2, y1 + (y2 - y1) / 2,
                       JUSTIFY_CENTER, JUSTIFY_MIDDLE, ORIENT_NORMAL,
                       fc, bc, captionbuf, 0);
    CurrentX = savedX;
    CurrentY = savedY;
    /* Key fully drawn: cursor_painted is false, so the next CursorRefresh
       repaints the cursor with a fresh save of the finished key. */
    OSK_CursorBack();
}

void OSK_DrawAll(void)
{
    if (!OptionVResreserved)
        return;
    /* See OSK_DrawKeyAt: keep the cursor out of the way for the whole
       repaint so it cannot save half-drawn keys as its background. */
    OSK_CursorOff();
    /* If we previously drew at a larger reserve, wipe the band above the
       new top that would otherwise be left with stale OSK pixels. */
    if (osk_drawn_reserved > OptionVResreserved)
    {
        int old_top = VRes - (VRes * osk_drawn_reserved / 100);
        int new_top = OSK_ReservedTop();
        DrawRectangle(0, old_top, HRes - 1, new_top - 1, BLACK);
    }
    /* Wipe the strip first in case it has stale pixels from the editor or
       a program draw. OSK_ReservedTop() is a percentage of VRes and is
       generally NOT a multiple of gui_font_height, so a thin orphan band
       (a few scanlines) sits between the last whole console text row
       (Option.Height*gui_font_height) and the strip top. The console only
       writes/scrolls whole gui_font_height rows, so that band is never
       cleared by text and shows stale junk just above the keyboard (most
       visible at low VRes / mode 5). Extend the wipe up to the text
       boundary so the orphan band is cleared too. This only ever reaches
       UP by < gui_font_height into the orphan band, never into a live
       text line. */
    int wipe_top = OSK_ReservedTop();
    if (gui_font_height > 0)
    {
        int text_bottom = (wipe_top / gui_font_height) * gui_font_height;
        if (text_bottom < wipe_top)
            wipe_top = text_bottom;
    }
    DrawRectangle(0, wipe_top, HRes - 1, VRes - 1, BLACK);
    for (int k = 0; k < OSK_KEYS; k++)
        OSK_DrawKeyAt(k);
    osk_drawn_reserved = OptionVResreserved;
    OSK_CursorBack();
}

void OSK_Erase(void)
{
    /* Use the geometry we last drew at, not the current option, in case
       OptionVResreserved has just changed underneath us. */
    int reserved = osk_drawn_reserved;
    if (reserved <= 0)
        return;
    OSK_CursorOff();
    int top = VRes - (VRes * reserved / 100);
    DrawRectangle(0, top, HRes - 1, VRes - 1, BLACK);
    osk_pressed = -1;
    osk_drawn_reserved = 0;
    OSK_CursorBack();
}

/* Tell the OSK its on-screen pixels are no longer valid (e.g. after a
   screen-mode change or NEW). The next OSK_OnPromptIdle will repaint. */
void OSK_Invalidate(void)
{
    osk_dirty = true;
}

static int OSK_HitTest(int x, int y)
{
    int x1, y1, x2, y2;
    const int *vals = OSK_ValueTable();
    if (y < OSK_ReservedTop())
        return -1;
    for (int k = 0; k < OSK_KEYS; k++)
    {
        OSK_GetCoord(k, &x1, &y1, &x2, &y2);
        if (x >= x1 && x <= x2 && y >= y1 && y <= y2)
        {
            /* If we hit a NONE cell (part of a multi-cell wide button),
               walk left within the same row to the anchor that owns it. */
            while (vals[k] == OSK_V_NONE && (k % OSK_COLS) != 0)
                k--;
            return (vals[k] == OSK_V_NONE) ? -1 : k;
        }
    }
    return -1;
}

/* Emit a keystroke into the same console RX path used by the USB HID host.
   For Ctrl-letter, fold to 0x01..0x1F; for Alt we'd prefix ESC but we don't
   expose Alt in v1. */
static void OSK_EmitValue(int v)
{
    if (v == OSK_V_NONE)
        return;
    if (osk_ctrl)
    {
        if (v >= 'a' && v <= 'z')
            v = v - 'a' + 1;
        else if (v >= 'A' && v <= 'Z')
            v = v - 'A' + 1;
        /* For non-letter keys, Ctrl is a no-op pass-through. */
    }
    USR_KEYBRD_ProcessData((uint8_t)(v & 0xFF));
}

/* Called from ProcessTouch on the touch DOWN edge when the tap lands in
   the reserved strip and the OSK is active. */
void OSK_HandleTouchDown(int x, int y)
{
    int k = OSK_HitTest(x, y);
    if (k < 0)
        return;
    const int *vals = OSK_ValueTable();
    int v = vals[k];
    if (v == OSK_V_NONE)
        return;
    /* Multi-touch (Option.Multi): a finger-1 press on a modifier engages
       it on the DOWN edge (so a finger-2 tap that follows is modified) and
       a release on UP disengages it. Tap-toggle behaviour is preserved
       when Option.Multi is off. */
    if (Option.Multi &&
        (v == OSK_V_SHIFT || v == OSK_V_CTRL || v == OSK_V_SYM || v == OSK_V_FN))
    {
        if (v == OSK_V_SHIFT)
            osk_shift = true;
        else if (v == OSK_V_CTRL)
            osk_ctrl = true;
        else if (v == OSK_V_SYM)
        {
            osk_sym = true;
            osk_fn = false;
        }
        else
        {
            osk_fn = true;
            osk_sym = false;
        }
        osk_pressed = k;
        osk_finger1_holds_modifier = true;
        /* Sym / Fn change the page so the whole keyboard must repaint;
           Shift / Ctrl only need their own cell redrawn (page-2 captions
           also shift, so for shift do a full redraw). */
        if (v == OSK_V_SYM || v == OSK_V_FN || v == OSK_V_SHIFT)
            OSK_DrawAll();
        else
            OSK_DrawKeyAt(k);
        ClickTimer += CLICK_DURATION;
        return;
    }
    /* Visual: invert the pressed key. */
    if (osk_pressed >= 0 && osk_pressed != k)
    {
        int prev = osk_pressed;
        osk_pressed = -1;
        OSK_DrawKeyAt(prev);
    }
    osk_pressed = k;
    OSK_DrawKeyAt(k);
    ClickTimer += CLICK_DURATION;
}

/* Polled each ProcessTouch cycle (only when Option.Multi). Detects edges of
   the second-contact (usb_touch_active2). On a finger-2 down-edge, if it
   lands on a non-modifier and finger 1 is holding a modifier, the tapped
   key is emitted with the modifier applied — without disengaging finger
   1's hold, so the user can chord multiple keys without re-pressing Shift
   etc. Returns nothing; harmless when no second contact is active. */
void OSK_PollFinger2(void)
{
    if (!Option.Multi)
        return;
    if (!OSK_IsActive())
        return;
    bool now_active = usb_touch_active2;
    if (now_active && !osk_finger2_was_active)
    {
        /* finger 2 just landed — only act if it's inside the strip */
        int reserved_top = VRes - (VRes * OptionVResreserved / 100);
        if (usb_touch_y2 >= reserved_top)
        {
            int k = OSK_HitTest(usb_touch_x2, usb_touch_y2);
            if (k >= 0)
            {
                const int *vals = OSK_ValueTable();
                int v = vals[k];
                if (v != OSK_V_NONE && v != OSK_V_SHIFT && v != OSK_V_CTRL && v != OSK_V_SYM && v != OSK_V_FN)
                {
                    OSK_EmitValue(v);
                    /* Brief visual feedback — show the key as pressed
                       until finger 2 lifts. */
                    osk_finger2_key = k;
                    int prev = osk_pressed;
                    osk_pressed = k;
                    OSK_DrawKeyAt(k);
                    osk_pressed = prev;
                    ClickTimer += CLICK_DURATION;
                }
            }
        }
    }
    else if (!now_active && osk_finger2_was_active)
    {
        /* finger 2 lifted — un-highlight if we'd drawn it pressed */
        if (osk_finger2_key >= 0)
        {
            int prev = osk_finger2_key;
            osk_finger2_key = -1;
            OSK_DrawKeyAt(prev);
        }
    }
    osk_finger2_was_active = now_active;
}

/* Called from ProcessTouch on the touch UP edge.
   Returns true iff the OSK consumed the event (i.e. there was a pressed key
   waiting to be released). */
bool OSK_HandleTouchUp(void)
{
    if (osk_pressed < 0)
        return false;
    int k = osk_pressed;
    osk_pressed = -1;

    const int *vals = OSK_ValueTable();
    int v = vals[k];

    /* Multi-touch (Option.Multi) — modifier was engaged on the DOWN edge
       (in OSK_HandleTouchDown), so on UP we DISENGAGE it rather than
       latch-toggling. Any finger-2 chords that happened in between have
       already emitted via OSK_PollFinger2. */
    if (Option.Multi && osk_finger1_holds_modifier &&
        (v == OSK_V_SHIFT || v == OSK_V_CTRL || v == OSK_V_SYM || v == OSK_V_FN))
    {
        osk_finger1_holds_modifier = false;
        if (v == OSK_V_SHIFT)
            osk_shift = false;
        else if (v == OSK_V_CTRL)
            osk_ctrl = false;
        else if (v == OSK_V_SYM)
            osk_sym = false;
        else
            osk_fn = false;
        OSK_DrawAll();
        return true;
    }

    /* Tap-toggle behaviour (Option.Multi off): modifier keys latch on tap
       so the user can press them with a single finger. Sym and Fn are
       mutually exclusive page selectors. */
    if (v == OSK_V_SHIFT)
    {
        osk_shift = !osk_shift;
        OSK_DrawAll();
        return true;
    }
    if (v == OSK_V_SYM)
    {
        osk_sym = !osk_sym;
        if (osk_sym)
            osk_fn = false;
        OSK_DrawAll();
        return true;
    }
    if (v == OSK_V_FN)
    {
        osk_fn = !osk_fn;
        if (osk_fn)
            osk_sym = false;
        OSK_DrawAll();
        return true;
    }
    if (v == OSK_V_CTRL)
    {
        osk_ctrl = !osk_ctrl;
        OSK_DrawKeyAt(OSK_KEY_CTRL_INDEX);
        return true;
    }

    /* Non-modifier: emit, then unlatch one-shot modifiers (Shift, Ctrl).
       Sym is a sticky page toggle — leave it alone. */
    OSK_EmitValue(v);

    bool need_full_redraw = false;
    if (osk_ctrl)
    {
        osk_ctrl = false;
        need_full_redraw = true;
    }
    if (osk_shift)
    {
        osk_shift = false;
        need_full_redraw = true;
    }

    if (need_full_redraw)
        OSK_DrawAll();
    else
        OSK_DrawKeyAt(k); /* repaint un-pressed */
    return true;
}

bool OSK_IsActive(void) { return osk_state != OSK_STATE_OFF; }

/* State-transition helpers — called from interpreter entry points. */
void OSK_OnPromptIdle(void)
{
    /* User explicitly issued KEYBOARD OFF at the prompt — leave OSK off
       and keep OptionVResreserved at 0. Only an explicit KEYBOARD ON
       clears this and re-enables the prompt's automatic OSK. */
    if (osk_user_disabled)
    {
        osk_state = OSK_STATE_OFF;
        osk_dirty = false;
        return;
    }
    /* Coming back to the prompt — scroll the screen up by the about-to-
       be-reserved strip's height (must happen while OptionVResreserved is
       still 0) so prompt output / program tail isn't overwritten. Then
       restore the user's reserved-strip value. */
    OSK_ScrollForStrip();
    OSK_RestoreReserved();
    if (!OptionVResreserved)
    {
        if (osk_drawn_reserved > 0)
            OSK_Erase();
        osk_state = OSK_STATE_OFF;
        osk_dirty = false;
        return;
    }
    /* Safety net: after the scroll the cursor should already be above the
       strip, but if the scroll fell short (e.g. CurrentY was at the very
       bottom) clamp so the cursor doesn't sit inside the OSK. */
    {
        int reserved_top = VRes - (VRes * OptionVResreserved / 100);
        if (gui_font_height > 0 && CurrentY + gui_font_height > reserved_top)
        {
            CurrentY = reserved_top - gui_font_height;
            if (CurrentY < 0)
                CurrentY = 0;
            osk_dirty = true;
        }
    }
    /* Already drawn at the current size and pixels still valid — skip. */
    if (osk_state == OSK_STATE_SYSTEM && !osk_dirty && osk_drawn_reserved == OptionVResreserved)
        return;
    osk_state = OSK_STATE_SYSTEM;
    OSK_ResetModifiers();
    osk_dirty = false;
    /* Clear the console area between the bottom of the cursor's line and the
       strip top. OSK_ScrollForStrip scrolled the screen up to make room but
       only cleared the strip band itself, so stale content scrolled up into
       the console area just above the keyboard remains. It is NOT caught by
       OSK_DrawAll's font-boundary wipe when ReservedTop happens to be an
       exact multiple of gui_font_height (e.g. mode 5: VRes 150 * 30% ->
       top 105 = 7*15, so the junk sits inside the last "valid" console line
       the cursor never reaches). Mirrors the editor's clear in Editor.c.
       Nothing legitimately lives below the prompt cursor. */
    {
        int strip_top = VRes - (VRes * OptionVResreserved / 100);
        int clear_from = CurrentY + gui_font_height;
        if (gui_font_height > 0 && clear_from < strip_top)
            DrawRectangle(0, clear_from, HRes - 1, strip_top - 1,
                          VideoColour == 1 ? 0 : gui_bcolour);
    }
    OSK_DrawAll();
}
void OSK_OnRunStart(void)
{
    /* Erase pixels we drew (if any) and surrender the strip to the program. */
    if (osk_state != OSK_STATE_OFF)
        OSK_Erase();
    osk_state = OSK_STATE_OFF;
    OSK_ResetModifiers();
    OSK_ZeroReserved();
}
/* Drop OSK state without touching pixels — for callers that just
   cleared the screen themselves (e.g. CLS) and only need to stop the
   touch dispatcher from firing on now-invisible buttons. */
void OSK_DropState(void)
{
    osk_state = OSK_STATE_OFF;
    osk_pressed = -1;
    osk_drawn_reserved = 0;
    osk_dirty = false;
    OSK_ResetModifiers();
    OSK_ZeroReserved();
}
void OSK_SetProgramActive(bool on)
{
    /* osk_user_disabled is a *prompt-time* latch — it represents the user
       explicitly typing KEYBOARD OFF or KEYBOARD ON at the prompt. A
       running program's own KEYBOARD ON / OFF must not touch it, otherwise
       a program that toggles the OSK mid-run could overrule the user's
       prompt-time preference (and prevent the auto-restore on program
       exit that the prompt loop performs when user_disabled is false). */
    bool at_prompt = (CurrentLinePtr == NULL);
    if (on)
    {
        if (at_prompt)
            osk_user_disabled = false;
        /* Scroll the screen up before we reserve the strip so existing
           content (prompt history, program output) survives instead of
           being painted over by the OSK. */
        OSK_ScrollForStrip();
        OSK_RestoreReserved();
        if (!OptionVResreserved)
            error("Set OPTION SCREEN KEYBOARD first");
        if (osk_state == OSK_STATE_PROGRAM || osk_state == OSK_STATE_SYSTEM)
        {
            /* Already visible — nothing further to do. */
            return;
        }
        osk_state = OSK_STATE_PROGRAM;
        OSK_ResetModifiers();
        OSK_DrawAll();
    }
    else
    {
        /* Idempotent OFF: erase any visible OSK, drop to OFF, zero the
           reserved strip. Only latch user_disabled if this was a
           prompt-side KEYBOARD OFF. */
        if (osk_state != OSK_STATE_OFF)
            OSK_Erase();
        osk_state = OSK_STATE_OFF;
        osk_pressed = -1;
        OSK_ResetModifiers();
        OSK_ZeroReserved();
        if (at_prompt)
            osk_user_disabled = true;
    }
}
bool OSK_IsProgramActive(void) { return osk_state == OSK_STATE_PROGRAM; }
bool OSK_IsUserDisabled(void) { return osk_user_disabled; }

