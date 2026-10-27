/***********************************************************************************************************************
PicoMite MMBasic

GUI.c

<COPYRIGHT HOLDERS>  Geoff Graham, Peter Mather
Copyright (c) 2021, <COPYRIGHT HOLDERS> All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1.	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2.	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.
3.	The name MMBasic be used when referring to the interpreter in any documentation and promotional material and the original copyright message be displayed
    on the console at startup (additional copyright messages may be added).
4.	All advertising materials mentioning features or use of this software must display the following acknowledgement: This product includes software developed
    by the <copyright holder>.
5.	Neither the name of the <copyright holder> nor the names of its contributors may be used to endorse or promote products derived from this software
    without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDERS> AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDERS> BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

************************************************************************************************************************/
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
