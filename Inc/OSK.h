/* OSK.h - the PicoMite's on-screen keyboard (its graphics/GUI.h), copied by tools/port_osk.py: do not edit, run it
   again. Included by GUI.h. */
#ifndef OSK_H_
#define OSK_H_

#include <stdbool.h>

/* ============================================================================
 * On-Screen Keyboard (OSK) — see GUI.c for the implementation.
 * ============================================================================ */
void OSK_DrawAll(void);
void OSK_Erase(void);
void OSK_Invalidate(void);                   /* call after operations that wipe the strip */
void OSK_DropState(void);                    /* clear state without redrawing (caller already cleared pixels) */
void OSK_OnPromptIdle(void);                 /* prompt loop / editor entry */
void OSK_OnRunStart(void);                   /* invoked from do_run() */
void OSK_SetProgramActive(bool on);          /* KEYBOARD ON / KEYBOARD OFF */
bool OSK_IsProgramActive(void);
bool OSK_IsActive(void);
bool OSK_IsUserDisabled(void);               /* KEYBOARD OFF at prompt latch */

/* the CMM2's: GUI.c's ProcessTouch and the 1 mS tick call these */
void OSK_HandleTouchDown(int x, int y);
bool OSK_HandleTouchUp(void);
void OSK_PollFinger2(void);
extern int osk_state;                        /* not OSK_STATE_OFF (0): the keyboard is up (the tick reads it) */
#endif
