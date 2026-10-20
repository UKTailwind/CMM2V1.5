/***************************************************************************
CMM2 MMBasic
Commands.h

Include file that contains the globals and defines for commands.c in MMBasic.

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



/**********************************************************************************
 the C language function associated with commands, functions or operators should be
 declared here
**********************************************************************************/


#if !defined(INCLUDE_COMMAND_TABLE) && !defined(INCLUDE_TOKEN_TABLE)

struct s_forstack {
    char *forptr;                           // pointer to the FOR command in program memory
    char *nextptr;                          // pointer to the NEXT command in program memory
    void *var;                              // value of the FOR variable
    char vartype;                           // type of the variable
    char level;                             // the sub/function level that the loop was created
    union u_totype {
        MMFLOAT f;                          // the TO value if it is a MMFLOAT
        long long int i;                    // the TO value if it is an integer
    }  __attribute__ ((aligned (8))) tovalue;
    union u_steptype {
        MMFLOAT f;                          // the STEP value if it is a MMFLOAT
        long long int i;                    // the STEP value if it is an integer
    }  __attribute__ ((aligned (8))) stepvalue;
} __attribute__ ((aligned (8))) ;

extern struct s_forstack forstack[MAXFORLOOPS + 1] ;
extern int forindex;

struct s_dostack {
    char *evalptr;                          // pointer to the expression to be evaluated
    char *loopptr;                          // pointer to the loop statement
    char *doptr;                            // pointer to the DO statement
    char level;                             // the sub/function level that the loop was created
    unsigned char untiltest;                // 1 if DO UNTIL (exits when the condition becomes true)
    // the DO fast path (DoFastCompile in Commands.c, the PicoMite's): a "var OP number" condition resolved once, so
    // LOOP compares without the evaluator
    unsigned char fast_state;               // DOFAST_UNTRIED, DOFAST_ON or DOFAST_OFF
    unsigned char fast_op;                  // DOFAST_LT .. DOFAST_NE, applied to (var - limit)
    unsigned char fast_until;               // 1: the condition is an UNTIL, inverted
    unsigned char fast_flags;               // DOFAST_VARINT: the variable is an integer; DOFAST_INTCMP: compare as integers
    int fast_varindex;                      // the variable's slot (ERASE switches the fast path off)
    void *fast_var;                         // the variable's value
    union {
        long long int i;
        MMFLOAT f;
    } fast_limit;
} __attribute__ ((aligned (8)));
#define DOFAST_UNTRIED 0                    // s_dostack.fast_state
#define DOFAST_ON 1
#define DOFAST_OFF 2
extern char *DoFindLoop(char *p, CommandToken dotoken, char *errmsg);
// the IF/ELSEIF/ELSE/ENDIF jump table (Commands.c, the PicoMite's): for each multi-line IF, ELSEIF and ELSE token, the
// next arm (ELSEIF, ELSE or ENDIF) and the ENDIF
struct iftab_entry {
    char *tok;                              // the IF, ELSEIF or ELSE command token
    char *next_arm;                         // the next ELSEIF, ELSE or ENDIF of this IF (NULL: none found, scan)
    char *endif_tok;                        // the ENDIF (NULL: none found, scan)
    char *line_ptr;                         // the line holding next_arm, for an error in the next ELSEIF's condition
};
extern void IfTableBuild(void);
extern void IfTableFree(void);
extern void IfTableForget(void);
extern struct iftab_entry *IfTableLookup(char *tok);
extern void DoFastForget(int slot);
struct sa_data{
    char* SaveNextDataLine;
    int SaveNextData;
};
extern struct sa_data datastore[MAXRESTORE];
extern int restorepointer;

extern struct s_dostack dostack[MAXDOLOOPS];
extern int doindex;

extern char *gosubstack[MAXGOSUB];
extern char *errorstack[MAXGOSUB];
extern char *substack[MAXGOSUB];
extern int gosubindex;
extern char DimUsed;

//extern char *GetFileName(char* CmdLinePtr, char *LastFilePtr);
//extern void mergefile(char *fname, char *MemPtr);
extern void MIPS16 ListProgram(char *p, int all);
extern void MIPS16 ListProgramFlash(char *p, int all);
extern char MIPS16 *llist(char *b, char *p);
extern char *CheckIfTypeSpecified(char *p, int *type, int AllowDefaultType);

#define CONFIG_TITLE		0
#define CONFIG_LOWER		1
#define CONFIG_UPPER		2

extern unsigned int BusSpeed;
extern char *OnKeyGOSUB;
extern char EchoOption;
extern void do_help(char *p);
void array_set(char *tp);                       // ARRAY SET / MATH SET (Commands.c)
void array_add(char *tp);
void array_insert(char *tp);
void array_slice(char *tp);
#endif
