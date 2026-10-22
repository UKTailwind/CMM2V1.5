/***************************************************************************

CMM2 MMBasic
MMBasic.c

Provides the core functions used in MMBasic.  These include parsing the command line and converting the key
words into tokens, storage and management of the program in memory, storage and management of variables,
the expression execution engine and other useful functions.

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

#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include "MMBasic.h"
#include "Symbols.h"

#include "Functions.h"
#include "Commands.h"
#include "Operators.h"
#include "Custom.h"
#include "FileManager.h"
#include "allcommands.h"
#include "Hardware_Includes.h"

extern int ListCnt;
extern int MMCharPos;
// this is the command table that defines the various tokens for commands in the source code
// most of them are listed in the .h files so you should not add your own here but instead add
// them to the appropriate .h file
#define INCLUDE_COMMAND_TABLE
const struct s_tokentbl commandtbl[] = {
#include "allcommands.h"
};
#undef INCLUDE_COMMAND_TABLE


// this is the token table that defines the other tokens in the source code
// most of them are listed in the .h files so you should not add your own here
// but instead add them to the appropriate .h file
#define INCLUDE_TOKEN_TABLE
const struct s_tokentbl tokentbl[] = {
#include "allcommands.h"
};
#undef INCLUDE_TOKEN_TABLE

// these are initialised at startup
int CommandTableSize, TokenTableSize;
struct s_vartbl *vartbl;                                            // the locals' records (MAXLOCALVARS), in D3 SRAM
struct s_vartbl **slotrec;                                          // the record of each variable number (VREC), beside them
static struct s_vartbl emptyrec;                                    // every free global slot's record
static struct s_vartbl blockedrec = {.name = "~", .type = T_BLOCKED};   // every blocked one's (see VarRecordRelease)
static struct {                                                     // the globals' record chunks, in the heap
    struct s_vartbl *chunk[MAXGLOBALVARS / VARCHUNK + 1];
    int chunks;
    int cur;                                                        // the chunk records are being handed out from
    struct s_vartbl *next, *end;                                    // its records not yet used
    struct s_vartbl *free;                                          // erased globals' records (newest first, linked through the name)
} varmem;
int varcnt;
int Localvarcnt;                                                         // number of LOCAL variables
int Globalvarcnt;                                                         // number of GLOBAL variables
int VarIndex;                                                       // Global set by findvar after a variable has been created or found
int LocalIndex;                                                     // used to track the level of local variables
char OptionExplicit,OptionEscape;                                                // used to force the declaration of variables before their use
char DefaultType;                                                   // the default type if a variable is not specifically typed
int emptyarray=0;
struct s_hash hashlist[MAXLOCALVARS];
char *subfun[MAXSUBFUN];                                            // table used to locate all subroutines and functions
jmp_buf run;                                                       // longjump to recover from an error and abort
jmp_buf mark;                                                       // longjump to recover from an error and abort
jmp_buf ErrNext;                                                    // longjump to recover from an error and continue
char __attribute__ ((aligned (8))) inpbuf[STRINGSIZE];                                            // used to store user keystrokes until we have a line
char __attribute__ ((aligned (8))) tknbuf[TKNBUFSIZE];                                          // used to store the tokenised representation of the users input line
int hashlistpointer=0;
/* P6 F2 (the PicoMite's a7154a1): the local region (slots 0 to MAXLOCALVARS - 1) is a stack.  Each local is
   made in the next slot up, and hashlist lists the live ones in the order they were made, each with its
   level: a level's locals are the topmost entries of that level.  localtop is the next slot to use. */
static int localtop = 0;
int NextData;                                                       // used to track the next item to read in DATA & READ stmts
char *NextDataLine;                                                 // used to track the next line to read in DATA & READ stmts
int OptionBase;
// The stack check, the PicoMite's (Peter, 21 Oct: "There should be a proper stack check like we have now in the
// Picomite").  The stack grows down from _estack in DTCM, and below it are the static variables (.bss, to _ebss), so
// StackFloor is STACK_GUARD above them: the room for what the stack takes between two checks (a check comes at each
// value and argument list; a built-in can go some KB deeper before its next one, and interrupts' handlers run on the
// same stack).  Below StackFloor is the error; in the 1K above it, stackwarn, the stack is noted and the prompt warns.
#define STACK_GUARD 4096
extern char _ebss[];
uint32_t StackFloor;                                                // MM.INFO(HEAPEND): the PicoMite's heapend
uint32_t stackwarn;                                                 // StackFloor + 1K: a stack below this is noted
uint32_t StackLow;                                                  // the lowest noted since the last prompt, 0 for none
int StackLowDepth;                                                  // LocalIndex when it was noted
// track the state of OPTION BASE
extern char errstring[256];
extern int errpos;
extern int StartEditLine,StartEditCharacter;
extern long long int CallCFunction(char *CmdPtr, char *ArgList, char *DefP, char *CallersLinePtr);
extern volatile int ConsoleTxBufHead;
extern volatile int ConsoleTxBufTail;
extern int TouchRoutineFound;
extern int ProcessingMSG;
extern int BeepRoutineFound;
extern int GuiCall;
unsigned long long int statement_time;
extern volatile uint64_t FastTimer;

char *firststmt;
///////////////////////////////////////////////////////////////////////////////////////////////
// Global information used by operators and functions
//
int targ;                                                           // the type of the returned value
MMFLOAT  __attribute__ ((aligned (8))) farg1,  __attribute__ ((aligned (8))) farg2,  __attribute__ ((aligned (8))) fret;                                           // the two float arguments and returned value
long long int  __attribute__ ((aligned (8))) iarg1,  __attribute__ ((aligned (8))) iarg2,  __attribute__ ((aligned (8))) iret;                                   // the two integer arguments and returned value
char *sarg1, *sarg2, *sret;                                         // the two string arguments and returned value

////////////////////////////////////////////////////////////////////////////////////////////////
// Global information used by functions
// functions use targ, fret and sret as defined for operators (above)
char *ep;                                                           // pointer to the argument to the function terminated with a zero byte.
                                                                    // it is NOT trimmed of spaces

////////////////////////////////////////////////////////////////////////////////////////////////
// Global information used by commands
//
int cmdtoken;                                                       // the command being run (its index in commandtbl)
char *CmdTokenPtr;                                                  // and where its token is (PAUSE re-runs it after an interrupt)
char *cmdline;                                                      // Command line terminated with a zero char and trimmed of spaces
char *nextstmt;                                                     // Pointer to the next statement to be executed.
char *CurrentLinePtr;                                               // Pointer to the current line (used in error reporting)
char *ContinuePoint;                                                // Where to continue from if using the continue statement
int ContinueLocalIndex;                                             // the LocalIndex it was at (a SUB's level)

uint32_t DefinedSubFunMem;         // Records memory allocated to DefinedSubFun incase of an error
int DefinedSubFunLocalIndex;       // Records LocalIndex at start of DefinedSubFun incase of an error

const char namestart[256]={
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x10
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x20
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x30
		0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, //0x40
		1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1, //0x50
		0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, //0x60
		1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0, //0x70
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x80
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x90
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xA0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xB0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xC0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xD0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xE0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  //0xF0
};
// 0x1E is a name character: STATIC's hidden name holds it between the SUB's name and the variable's
const char namein[256]={
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0, //0x10
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0, //0x20
		1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, //0x30
		0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, //0x40
		1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1, //0x50
		0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, //0x60
		1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0, //0x70
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x80
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x90
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xA0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xB0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xC0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xD0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xE0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  //0xF0
};
const char nameend[256]={
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0, //0x10
		0,1,0,0,1,1,0,0,0,0,0,0,0,0,1,0, //0x20
		1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, //0x30
		0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, //0x40
		1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1, //0x50
		0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, //0x60
		1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0, //0x70
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x80
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x90
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xA0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xB0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xC0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xD0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xE0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  //0xF0
};
const char digit[256]={
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x10
		0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,0, //0x20
		1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, //0x30
		0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0, //0x40
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x50
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x60
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x70
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x80
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0x90
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xA0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xB0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xC0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xD0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, //0xE0
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  //0xF0
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// Functions only used within MMBasic.c
//
void getexpr(char *);
void checktype(int *, int);
char ITCM_CODE *getvalue(char *p, MMFLOAT *fa, long long int *ia, char **sa, int *oo, int *ta);
char tokenTHEN, tokenELSE, tokenGOTO, tokenEQUAL, tokenTO, tokenSTEP, tokenWHILE, tokenUNTIL, tokenGOSUB, tokenAS, tokenFOR;
CommandToken cmdIF, cmdENDIF,  cmdELSEIF, cmdELSE, cmdSELECT_CASE, cmdFOR, cmdNEXT, cmdWHILE, cmdENDSUB, cmdENDFUNCTION, cmdLOCAL, cmdSTATIC, cmdCASE, cmdDO, cmdLOOP, cmdCASE_ELSE, cmdEND_SELECT;
CommandToken cmdSUB, cmdFUN, cmdCSUB, cmdIRET, cmdTYPE, cmdEND_TYPE, cmdSTRUCT;
char syscheck;

/********************************************************************************************************************************************
 Program management
 Includes the routines to initialise MMBasic, start running the interpreter, and to run a program in memory
*********************************************************************************************************************************************/

// Initialise MMBasic
void MIPS16 InitBasic(void) {
    StackFloor = (uint32_t)_ebss + STACK_GUARD;                     // (the stack check: see StackNearLimit)
    stackwarn = StackFloor + 1024;
    DefaultType = T_NBR;
    CommandTableSize =  (sizeof(commandtbl)/sizeof(struct s_tokentbl));
    TokenTableSize =  (sizeof(tokentbl)/sizeof(struct s_tokentbl));

    //ClearProgram();

    // load the commonly used tokens
    // by looking them up once here performance is improved considerably
    tokenTHEN  = GetTokenValue("Then");
    tokenELSE  = GetTokenValue("Else");
    tokenGOTO  = GetTokenValue("GoTo");
    tokenEQUAL = GetTokenValue("=");
    tokenTO    = GetTokenValue("To");
    tokenSTEP  = GetTokenValue("Step");
    tokenWHILE = GetTokenValue("While");
    tokenUNTIL = GetTokenValue("Until");
    tokenGOSUB = GetTokenValue("GoSub");
    tokenAS    = GetTokenValue("As");
    tokenFOR   = GetTokenValue("For");
    cmdLOOP  = GetCommandValue("Loop");
    cmdIF      = GetCommandValue("If");
    cmdENDIF   = GetCommandValue("EndIf");
    cmdELSEIF  = GetCommandValue("ElseIf");
    cmdELSE    = GetCommandValue("Else");
    cmdSELECT_CASE = GetCommandValue("Select Case");
    cmdCASE        = GetCommandValue("Case");
    cmdCASE_ELSE   = GetCommandValue("Case Else");
    cmdEND_SELECT  = GetCommandValue("End Select");
    cmdWHILE = GetCommandValue("While");
	cmdSUB = GetCommandValue("Sub");
	cmdFUN = GetCommandValue("Function");
    cmdIRET = GetCommandValue("IReturn");
    cmdCSUB = GetCommandValue("CSub");
    cmdTYPE = GetCommandValue("Type");
    cmdEND_TYPE = GetCommandValue("End Type");
    cmdSTRUCT = GetCommandValue("Struct");
    cmdLOCAL = GetCommandValue("Local");
    cmdSTATIC = GetCommandValue("Static");
    cmdENDSUB= GetCommandValue("End Sub");
    cmdENDFUNCTION = GetCommandValue("End Function");
    cmdDO=  GetCommandValue("Do");
    cmdFOR=  GetCommandValue("For");
    cmdNEXT= GetCommandValue("Next");
    SymInit();                                                      // the symbol-aware commands and functions (Symbols.c)

//	PInt(CommandTableSize);
//	PIntComma(TokenTableSize);
//	MMPrintString("\r\n");
}

int CheckEmpty(char *p){
        int emptyarray=0;
        char *pp = strchr((char *)p, '(');
        if(pp == NULL) return 0;                                    // no bracket at all: an address (the loop
                                                                    // below stepped a NULL pointer: the PicoMite's #116)
        if(pp){
            pp++;
            skipspace(pp);
            if(*pp == ')')emptyarray=1;
        }
        while(*(++pp)){
            if(*pp=='(')return 1; // can't be a function call with an implied opening
            if(*pp==')')return 0; // closing bracket without open so much be implied in a function call e.g. PEEK(
        }
        return emptyarray;
}

// run a program
// this will continuously execute a program until the end (marked by TWO zero chars)
// the argument p must point to the first line to be executed
/* OPTION PROFILING: each line's count and time, by line: entry ProfileBase[file] + line (the line's header gives both).
   Made at RUN from the heap, for the lines the program has, when the option is on; LIST PROFILE reads it at the
   prompt; the next RUN or load wipes the heap and ProfileForget drops it. */
struct s_profent *ProfileTable = NULL;
static int *ProfileBase, *ProfileTop;                               // a file's first entry, and its last line (in the block too)
static int ProfileFiles;                                            // the files: 1 the program, then its #INCLUDE files
static char *ProfileLast;                                           // the line that was being run

void ProfileForget(void) {
    ProfileTable = NULL;
    ProfileFiles = 0;
}

// RUN, the heap cleared and the program prepared: the table for its lines, zeroed, with each file's base and last
// line after it (one block from the heap: nothing of it is kept while profiling is off)
void ProfileStart(void) {
    char *p;
    int f, n = 0, top[256];
    ProfileForget();
    ProfileLast = NULL;
    memset(top, 0, sizeof(top));
    for(p = (char *)ProgMemory; *p == T_NEWLINE; p = NextLine(p)) {
        f = LHDR_FILE(p);
        if(f > ProfileFiles) ProfileFiles = f;
        if(LHDR_LINE(p) > top[f]) top[f] = LHDR_LINE(p);
    }
    for(f = 1; f <= ProfileFiles; f++) n += top[f] + 1;
    ProfileTable = GetMemory((n + 1) * sizeof(struct s_profent) + 2 * (ProfileFiles + 1) * sizeof(int));
    ProfileBase = (int *)(ProfileTable + n + 1);
    ProfileTop = ProfileBase + ProfileFiles + 1;
    for(f = 1, n = 0; f <= ProfileFiles; f++) {
        ProfileBase[f] = n;
        ProfileTop[f] = top[f];
        n += top[f] + 1;
    }
}

// the entry of the line at p (a T_NEWLINE), or NULL
struct s_profent *ProfileEntry(char *p) {
    int f, l;
    if(ProfileTable == NULL || p < (char *)ProgMemory || p >= (char *)ProgMemory + PROG_FLASH_SIZE || *p != T_NEWLINE || InLibrary(p)) return NULL;
    f = LHDR_FILE(p);
    l = LHDR_LINE(p);
    if(f < 1 || f > ProfileFiles || l > ProfileTop[f]) return NULL;
    return &ProfileTable[ProfileBase[f] + l];
}

int ExecDepth = 0;                                                  // how deep ExecuteProgram is (a FUNCTION's body runs one deeper)
void ITCM_CODE ExecuteProgram(char *p) {
    int i, SaveLocalIndex = 0;
    int MyDepth = ++ExecDepth;                                      // (an error skipped to this loop comes back to it)
    jmp_buf SaveErrNext;
    memcpy(SaveErrNext, ErrNext, sizeof(jmp_buf));                  // we call ExecuteProgram() recursively so we need to store/restore old jump buffer between calls
    ShortScroll=0;
    skipspace(p);
    // just in case, skip any white space
    while(1) {
        if(RBLive && RBInImage(p)) {                                // OPTION COMPILE: on in the compiled stream, back here where it
            p = RunStream(p);                                       // cannot (Stream.c)
            if((p[0] == 0 && p[1] == 0) || (p[0] == 0xff && p[1] == 0xff)) break;
        }
        if(*p == 0) p++;                                            // step over the zero byte marking the beginning of a new element
        if(*p == T_NEWLINE) {
            CurrentLinePtr = p;                                     // and pointer to the line for error reporting
            TraceBuff[TraceBuffIndex] = p;                          // used by TRACE LIST
            if(++TraceBuffIndex >= TRACE_BUFF_SIZE) TraceBuffIndex = 0;
            if(ProfileTable != NULL) {                              // OPTION PROFILING: the line that has finished counts, and its time
            	struct s_profent *e = ProfileEntry(ProfileLast);
            	if(e != NULL) {
            		e->count++;
            		e->us += ((ReadCoreTimer() + FastTimer) - statement_time) / ticks_per_microsecond;
            	}
            	statement_time = ReadCoreTimer() + FastTimer;
            	ProfileLast = p;
            }
            if(TraceOn && (InProgMemory(p) || InLibrary(p)) && LHDR_LINE(p)) {   // (line 0: a file name line)
            	char buff[10];
          		MMPrintString("[");
          		if(LHDR_FILE(p) > 1 || InLibrary(p)) {             // an #INCLUDE or library file: [name:line]
          			MMPrintString(LineFileNameAt(p));
          			MMPrintString(":");
          		}
          		IntToStr(buff, LHDR_LINE(p), 10);
          		MMPrintString(buff);
       			MMPrintString("]");
                uSec(1000);
            }
            p += T_NEWLINE_HDR;                                     // and step over the line's header
        }
        if(*p == T_LINENBR) p += 3;                                 // and step over the number
        skipspace(p);                                               // and skip any trailing white space
        if(p[0] == T_LABEL) {                                       // got a label
            p += p[1] + 2;                                          // skip over the label
            skipspace(p);                                           // and any following spaces
        }

        if(*p) {                                                    // if p is pointing to a command
            CommandToken tkn = commandtbl_at(p);                    // CMD_NOTOKEN for a SUB call, which is a name
            nextstmt = cmdline = (tkn == CMD_NOTOKEN ? p : p + sizeof(CommandToken));
            skipspace(cmdline);
            skipelement(nextstmt);
            if(*p && *p != '\'') {                                  // ignore a comment line
				SaveLocalIndex = LocalIndex;                    // save this if we need to cleanup after an error
            	if(OptionErrorSkip == 0){
					if(tkn != CMD_NOTOKEN && tkn < CommandTableSize - 1 && (commandtbl[tkn].type & T_CMD)) {
						cmdtoken = tkn;
						CmdTokenPtr = p;
						targ = T_CMD;
						if(!SymAwareCommand(tkn)) cmdline = SymExpandStatement(cmdline);   // its arguments as text (Symbols.c)
						commandtbl[tkn].fptr();                     // execute the command
					} else {
						if(!isnamestartsym(*p)) error("Invalid character: @", (int)(*p));
						i = FindSubFun(p, false);                   // it could be a defined command
						if(i >= 0) {                                // >= 0 means it is a user defined command
							DefinedSubFun(false, p, i, NULL, NULL, NULL, NULL);
						}
						else
							error("Unknown command");
					}
            	} else {
					if(setjmp(ErrNext) == 0) {                          // return to the else leg of this if error and OPTION ERROR SKIP/IGNORE is in effect
						if(tkn != CMD_NOTOKEN && tkn < CommandTableSize - 1 && (commandtbl[tkn].type & T_CMD)) {
							cmdtoken = tkn;
							CmdTokenPtr = p;
							targ = T_CMD;
							if(!SymAwareCommand(tkn)) cmdline = SymExpandStatement(cmdline);   // its arguments as text (Symbols.c)
							commandtbl[tkn].fptr();                     // execute the command
						} else {
							if(!isnamestartsym(*p)) error("Invalid character: @", (int)(*p));
							i = FindSubFun(p, false);                   // it could be a defined command
							if(i >= 0) {                                // >= 0 means it is a user defined command
								DefinedSubFun(false, p, i, NULL, NULL, NULL, NULL);
							}
							else
								error("Unknown command");
						}
					} else {
						LocalIndex = SaveLocalIndex;                    // restore so that we can clean up any memory leaks
						ExecDepth = MyDepth;                            // (from deeper, a FUNCTION's body)
						ClearTempMemory();
					}
            	}
				// ON ERROR SKIP counts the statements this loop runs, a SUB's body included (it runs here, as on the
				// PicoMite), but not those of a FUNCTION the protected statement calls: its body runs in an
				// ExecuteProgram of its own, deeper (PicoMite issue 34)
				if(OptionErrorSkip > 0 && OptionErrorSkip < 100000 && ExecDepth <= ErrorSkipDepth) {
					OptionErrorSkip--;
					ErrorSkipDepth = ExecDepth;                     // (a count left when its FUNCTION returns goes on in the caller)
				}
                if(TempMemoryIsChanged) ClearTempMemory();          // at the end of each command we need to clear any temporary string vars
                if(USBtime | deferredcopy | MMAbort) CheckAbort();      // (it does nothing unless one is set: the 1 ms tick sets USBtime)
                if(IntReady.any || autocursor || GUIactive)check_interrupt();                                  // check for an MMBasic interrupt and handle it
            }
            p = nextstmt;
        }
        if((p[0] == 0 && p[1] == 0) || (p[0] == 0xff && p[1] == 0xff)) {
//        	sendCRLF = 3;
//            SCB_CleanInvalidateDCache();
       		break;      // the end of the program is marked by TWO zero chars, empty flash by two 0xff
         }
    }
    memcpy(ErrNext, SaveErrNext, sizeof(jmp_buf));                  // restore old jump buffer
    ExecDepth = MyDepth - 1;
}


/********************************************************************************************************************************************
 Code associated with processing user defined subroutines and functions
********************************************************************************************************************************************/
void cleanup(void){
	int i=3;
	ClearVars(0);
    ClearSavedVars();                                               // clear any saved variables
    while(FlashWriteInit((uint32_t)ProgMemory) && i)i--;                     // erase program memory
    if(i==0)error("Failed to erase flash memory");
    FlashWriteByte(0); FlashWriteByte(0); FlashWriteByte(0);    // terminate the program in flash
    FlashWriteClose();
	ClearProgram();
    WatchdogSet = false;
    Option.Autorun = false;
    mymemset(inpbuf,0,STRINGSIZE);
//    mymemset(lastcmd,0,STRINGSIZE*4);
	SCB_CleanInvalidateDCache();
    longjmp(mark, 1);
}



/* ---------------------------------------------------------------------------------------------------------
   The name index (the PicoMite's 825ade6): every SUB, FUNCTION, CSUB and label of the program, by name, for
   FindSubFun, findlabel and findvar's check that a new variable is not named like a SUB.  Made in the heap
   by PrepareProgram, a power of two of 8-byte slots at most half full.  A slot keeps the top half of the
   name's FNV-1a hash (of the name in capitals, without a type suffix), its length, its kind and where it
   is; not the name, which is read from the definition when the rest matches.  It replaces funtbl (18 KB,
   a copy of every name) and hashlabels.  Labels are not counted against MAXSUBFUN.
   --------------------------------------------------------------------------------------------------------- */
#define NK_SUB 1                                                    // a SUB or CSUB: target is its subfun[] index
#define NK_FUN 2                                                    // a FUNCTION: likewise
#define NK_LABEL 3                                                  // a label: target is its line's T_NEWLINE
struct s_nameslot {
    uint16_t hash16;
    uint8_t len;
    uint8_t kind;                                                   // 0: empty
    uint32_t target;
};
static struct s_nameslot NameNone[1];                               // the index of a program with no names: one empty slot
static struct s_nameslot *NameIndex = NameNone;                     // in the heap, or NameNone
static uint32_t NameMask;                                           // its slots - 1
static int NameSubs;                                                // the SUBs, FUNCTIONs and CSUBs in subfun[]
static int NameBuilt;                                               // made for this heap (PrepareProgram makes it once)

// the heap has been wiped: the index went with it
void NameIndexForget(void) {
    NameIndex = NameNone;
    NameMask = 0;
    NameBuilt = 0;
}

static inline uint32_t NameHash(const char *nm, int nl) {
    uint32_t hash = FNV_offset_basis;
    while(nl--) {
        hash ^= toupper(*nm++);
        hash *= FNV_prime;
    }
    return hash;
}

// a label's name, from its line's T_NEWLINE (tokenise puts a label first on its line, after any line number)
static const char *NameOfLabel(char *line, int *len) {
    char *q = line + T_NEWLINE_HDR;
    while(*q == ' ') q++;
    if(*q == T_LINENBR) {
        q += 3;
        while(*q == ' ') q++;
    }
    *len = (unsigned char)q[1];                                     // (q[0] is T_LABEL)
    return q + 2;
}

// the spelling of SUB/FUNCTION i (without a type suffix)
static const char *NameOfSub(int i, int *len) {
    char *p = subfun[i] + sizeof(CommandToken);
    const char *s;
    skipspace(p);
    NameView(p, &s, len);                                           // a symbol or text (Symbols.h)
    return s;
}

// is slot s the name nm (nl characters)?
static inline int NameIs(const struct s_nameslot *s, const char *nm, int nl, uint32_t hash) {
    int l;
    const char *sv;
    if(s->hash16 != (hash >> 16) || s->len != nl) return 0;
    sv = s->kind == NK_LABEL ? NameOfLabel((char *)s->target, &l) : NameOfSub(s->target, &l);
    while(nl--)
        if(toupper(*sv++) != toupper(*nm++)) return 0;
    return 1;
}

// put a name in the index: returns the slot of the same name already there (a duplicate, left as it was), or
// NULL when it was added.  SUBs, FUNCTIONs and CSUBs share one set of names, labels have their own.
static struct s_nameslot *NameAdd(const char *nv, int nl, int kind, uint32_t target) {
    uint32_t i, hash = NameHash(nv, nl);
    for(i = hash & NameMask; NameIndex[i].kind; i = (i + 1) & NameMask)
        if((NameIndex[i].kind == NK_LABEL) == (kind == NK_LABEL) && NameIs(&NameIndex[i], nv, nl, hash))
            return &NameIndex[i];                                   // (a SUB and a FUNCTION may not share a name either)
    NameIndex[i].hash16 = hash >> 16;
    NameIndex[i].len = nl;
    NameIndex[i].kind = kind;
    NameIndex[i].target = target;
    return NULL;
}

// "Error: Duplicate ... NAME" as PrepareProgram has always reported a SUB defined twice
static void NameDuplicate(const char *what, const char *nv, int nl) {
    MMPrintString((char *)what);
    while(nl--) MMputchar(*nv++);
    cleanup();
}

// the labels of the program: counted (add == 0) or added
static int NameLabelsIn(char *p, int add, int ErrAbort) {
    int n = 0;
    while(*p == T_NEWLINE) {
        char *q = p + T_NEWLINE_HDR;
        while(*q == ' ') q++;
        if(*q == T_LINENBR) {
            q += 3;
            while(*q == ' ') q++;
        }
        if(*q == T_LABEL) {
            n++;
            if(add && NameAdd(q + 2, (unsigned char)q[1], NK_LABEL, (uint32_t)p) != NULL && ErrAbort)
                NameDuplicate("Error: Duplicate label ", q + 2, (unsigned char)q[1]);
        }
        p = NextLine(p);
    }
    return n;
}

// the labels of the library and of the program, which share one namespace (the PicoMite's)
static int NameLabels(int add, int ErrAbort) {
    int n = LibPresent() ? NameLabelsIn(LibMemory, add, ErrAbort) : 0;
    return n + NameLabelsIn((char *)ProgMemory, add, ErrAbort);
}

// make the index for the nsubs entries of subfun[] and the labels of the program; with ErrAbort a name
// defined twice is an error.  Its memory: on-chip RAM while there is room, then the SDRAM heap.
static void NameIndexBuild(int nsubs, int ErrAbort) {
    int n, size, i, nl;
    const char *nv;
    NameIndexForget();                                              // (a heap wipe or a new heap: nothing to free)
    NameBuilt = 1;
    NameSubs = nsubs;
    n = nsubs + NameLabels(0, 0);
    if(n == 0) return;                                              // (NameNone)
    for(size = 16; size < 2 * n; size <<= 1);
    if((NameIndex = GetInternalMemoryNull(size * sizeof(struct s_nameslot))) == NULL)
        NameIndex = GetMemory(size * sizeof(struct s_nameslot));
    NameMask = size - 1;
    for(i = 0; i < nsubs; i++) {
        nv = NameOfSub(i, &nl);
        if(NameAdd(nv, nl, commandtbl_at(subfun[i]) == cmdFUN ? NK_FUN : NK_SUB, i) != NULL && ErrAbort)
            NameDuplicate("Error: Duplicate name ", nv, nl);
    }
    NameLabels(1, ErrAbort);
}

// the first slot of a name of the kinds in kinds (a bit per kind), or NULL
static struct s_nameslot *NameFind(const char *nm, int nl, int kinds) {
    uint32_t hash = NameHash(nm, nl);
    for(uint32_t i = hash & NameMask; NameIndex[i].kind; i = (i + 1) & NameMask)
        if(((kinds >> NameIndex[i].kind) & 1) && NameIs(&NameIndex[i], nm, nl, hash))
            return &NameIndex[i];
    return NULL;
}

// Scan through the program loaded in flash and build a table pointing to the definition of all user defined subroutines and functions.
// This pre processing speeds up the program when using defined subroutines and functions
// this routine also looks for embedded fonts and adds them to the font table
static int ScanTypes;                                               // PrepareProgramExt makes the TYPE table as it goes
void MIPS16 PrepareProgram(int ErrAbort) {
    int MIPS16 PrepareProgramExt(char *, int, unsigned char **, int);
    int i, NbrFuncts;
    RBLive = 0;                                                     // a compiled stream is trusted only after RUN checks its stamp (Stream.h)
    for(i = FONT_BUILTIN_NBR; i < FONT_TABLE_SIZE; i++)
        FontTable[i] = NULL;                                        // clear the font table
    {   // fonts 8 and 9 built in again (the PicoMite's): a program's LOAD FONT or DEFINEFONT #8 or #9 replaced them
        extern const uint32_t TinyFont[];
        extern const unsigned char font8x10[];
        FontTable[7] = (unsigned char *)TinyFont;
        FontTable[8] = (unsigned char *)font8x10;
    }

    CFunctionFlash = CFunctionLibrary = NULL;
    SymSetLibrary(LibPresent() ? LibMemory : NULL);                 // the library's symbol table (Library.c), and
    SymSetProgram((char *)ProgMemory);                              // the program's symbol table (Symbols.h), and
    SymOffLine = SymFindOff((char *)ProgMemory);                    // OPTION SYMBOLS OFF at its top (a no-op: SymFindOff)
    ScanTypes = StructTableStart();                                 // the TYPE table too, once for a heap as the index below
    NbrFuncts = 0;                                                  // the library's SUBs first, then the program's (the PicoMite's)
    if(LibPresent()) NbrFuncts = PrepareProgramExt(LibMemory, 0, &CFunctionLibrary, ErrAbort);
    NbrFuncts = PrepareProgramExt((char *)ProgMemory, NbrFuncts, &CFunctionFlash, ErrAbort);
    // the index of every SUB, FUNCTION, CSUB and label by name, which also finds a name defined twice: made
    // once for a heap (RUN and loading a program wipe it), not before every line typed at the prompt
    if(!NameBuilt) NameIndexBuild(NbrFuncts, ErrAbort);
    SymBindInit();                                                  // the symbols' bindings, empty (Symbols.c)
    IfTableBuild();                                                 // the IF/ELSEIF/ELSE/ENDIF jump table (Commands.c)
    copy_clock();
    if(ErrAbort) RBPrepare();                                       // OPTION COMPILE: the stream's stamp checked, and the program compiled
}                                                                   // if it changed (Stream.c: RUN, CHAIN, FLASH/RAM RUN, the library)


// This scans one area (main program or the library area) for user defined subroutines and functions.
// It is only used by PrepareProgram() above.
int MIPS16 PrepareProgramExt(char *p, int i, unsigned char **CFunPtr, int ErrAbort) {
    unsigned int *cfp;
    char *image = p;                                                // (the symbol table's textlen is relative to it)
    while(*p != 0xff) {
        p = GetNextCommand(p, &CurrentLinePtr, NULL);
        if(*p == 0) break;                                          // end of the program or module
        CommandToken tkn = commandtbl_at(p);
        if(tkn == cmdSUB || tkn == cmdFUN || tkn == cmdCSUB) {      // found a SUB, FUN, CFUNCTION or CSUB token
            if(i >= MAXSUBFUN) {
                if(ErrAbort){
                	MMPrintString("Error: Too many subroutines and functions");
                	cleanup();
                }
                continue;
            }
            subfun[i++] = p;                                        // save the address
            p += sizeof(CommandToken);                              // and step over the token
            skipspace(p);
            if(!isnamestartsym(*p)) {
                if(ErrAbort) {
                	char eb[STRINGSIZE];                                // (the rest of the element, its symbols spelt out)
                	int en = SymExpand(eb, p, strlen(p), STRINGSIZE - 1);
                	eb[en < 0 ? 0 : en] = 0;
                	MMPrintString("Error: Invalid identifier ");MMPrintString(eb);
                	cleanup();
               }
                i--;
                continue;
            }
        } else if(tkn == cmdTYPE && ScanTypes) {
            p = StructDefine(p, ErrAbort);                          // a TYPE block: its type (Struct.c)
        }
        while(*p) p++;                                              // look for the zero marking the start of the next element
    }
    while(*p == 0) p++;                                             // the end of the program can have multiple zeros
    p++;                                                            // step over the terminating 0xff
    *CFunPtr = (unsigned char *)(((unsigned int)p + 0b11) & ~0b11); // CFunction flash (if it exists) starts on the next word address after the program in flash
    {                                                               // a symbol table (Symbols.h) sits between the program and the records
        const symtab_t *st = (const symtab_t *)*CFunPtr;
        if(st->magic == SYM_MAGIC && st->textlen == (uint32_t)((char *)st - image) && st->size < PROG_FLASH_SIZE)
            *CFunPtr += st->size;
    }
    if(i < MAXSUBFUN) subfun[i] = NULL;
    CurrentLinePtr = NULL;

    // now, step through the CFunction area looking for fonts to add to the font table
    cfp = (unsigned int *)*CFunPtr;                                 // read as the unsigned char * it is: through an unsigned int ** the
                                                                    // compiler may reuse the value from before the table was skipped
                                                                    // (the PicoMite's e0515d3)
    while(*cfp != 0xffffffff) {
        if(*cfp < FONT_TABLE_SIZE || ((*cfp & LIB_FONT_BIT) && (*cfp & ~LIB_FONT_BIT) < FONT_TABLE_SIZE))
            FontTable[*cfp & ~LIB_FONT_BIT] = (unsigned char *)(cfp + 2);   // (a library font's word: its number - 1 | LIB_FONT_BIT)
        cfp++;
        cfp += (*cfp + 4) / sizeof(unsigned int);
    }
    return i;
}

// searches the subfun[] table to locate a defined sub or fun
// returns with the index of the sub/function in the table or -1 if not found
// if type = 0 then look for a sub otherwise a function
int FindSubFun(char *p, int type) {
    char name[MAXVARLEN + 1];
    int nl = 0;
    struct s_nameslot *n;
    int k = -1;
    if(issymbol(*p)) {                                              // a symbol (Symbols.h)
        const char *s;
        if((k = SymCanonAt(p)) >= 0 && SymS[k] != SYM_UNBOUND) {   // looked up before: the SUB, or -1 for none
            int i = SymS[k];
            if(i >= 0 && commandtbl_at(subfun[i]) != (type ? cmdFUN : cmdSUB) && !(type == 0 && commandtbl_at(subfun[i]) == cmdCSUB)) return -1;
            return i;                                               // (the kind is checked every time: the PicoMite's 4fe3615)
        }
        NameView(p, &s, &nl);
        if(nl > MAXVARLEN) error("Variable name too long");
        memcpy(name, s, nl);
    } else
        do {
            if(nl >= MAXVARLEN) error("Variable name too long");
            name[nl++] = *p++;
        } while(isnamechar(*p));
    n = NameFind(name, nl, (1 << NK_SUB) | (1 << NK_FUN));
    if(k >= 0) SymS[k] = (n != NULL) ? n->target : -1;              // the binding, of any kind
    if(n == NULL) return -1;
    // it must also be the kind asked for: a SUB or CSUB for a statement (type 0), a FUNCTION for an
    // expression (type 1).  Otherwise a FUNCTION called as a statement, or a SUB used in an expression, ran
    // part way and failed inside itself with a misleading error.
    if(n->kind != (type ? NK_FUN : NK_SUB)) return -1;
    return n->target;
}


// the value of each argument while DefinedSubFun() processes a call's arguments (also read by error()'s cleanup)
union u_argval {
    MMFLOAT f;                                                    // the value if it is a float
    long long int i;                                            // the value if it is an integer
    MMFLOAT *fa;                                                  // pointer to the allocated memory if it is an array of floats
    long long int *ia;                                          // pointer to the allocated memory if it is an array of integers
    char *s;                                                    // pointer to the allocated memory if it is a string
};

/* P6 F1 (the PicoMite's 960ba97, packed as in a856559): a SUB's or FUNCTION's parameter list as
   DefinedSubFun reads it, read on its first call by the code that read it on every call before, and kept
   for the run: each parameter's name (to its AS, terminated), its BYVAL or BYREF and its AS type, and where
   the body starts.  The layouts go one after another into 1 KB heap blocks linked through their first word;
   InitHeap() forgets them with the rest of the heap (SubLayoutForget). */
#define SL_BYVAL 1
#define SL_BYREF 2
struct s_subparam {
    char *name;                 // the parameter's text as findvar is given it (to its AS, terminated)
    int type;                   // its AS type, or T_NOTYPE
    int by;                     // SL_BYVAL, SL_BYREF or 0
    int structarg;              // AS a TYPE's name: its number, else -1
};
struct s_sublayout {
    char *def;                  // subfun[] as it was read (a stale layout is read again)
    char *body;                 // the body's first statement
    int argc2;                  // makeargs' count for the parameter list
    struct s_subparam p[];      // each parameter, (argc2 + 1) / 2 of them
};
static struct s_sublayout **SubLay;                                 // per subfun[] index (SubLayN of them), NULL = not read yet
static int SubLayN;                                                 // the table's entries: the program's SUBs when it was made
#define SL_BLOCK 1024
static char *SubLayBlock;                                           // the newest block, or NULL
static char *SubLayNext;                                            // where in it the next layout goes
static int SubLayRoom;                                              // and the bytes left after that

void SubLayoutForget(void) {
    SubLay = NULL;
    SubLayN = 0;
    SubLayBlock = SubLayNext = NULL;
    SubLayRoom = 0;
}

// room for a layout of size bytes in the blocks (a new block if the newest has too little left)
static struct s_sublayout *SubLayoutPlace(int size) {
    size = (size + 7) & ~7;
    if(size > SubLayRoom) {
        int n = size + (int)sizeof(void *) > SL_BLOCK ? size + (int)sizeof(void *) : SL_BLOCK;
        char *b = GetMemory(n);
        *(char **)b = SubLayBlock;
        SubLayBlock = b;
        SubLayNext = b + 8;                                         // (8: the layouts stay 8-aligned)
        SubLayRoom = n - 8;
    }
    SubLayNext += size;
    SubLayRoom -= size;
    return (struct s_sublayout *)(SubLayNext - size);
}

// The parameter list at p of subfun[index], read as DefinedSubFun read it, errors and all (in the argument
// buffers it passes), and kept.
static struct s_sublayout *SubLayoutBuild(int index, char *p, char *argbuf2, char **argv2);

// BYVAL (kind 'V') or BYREF (kind 'R') at the start of a parameter in a SUB/FUNCTION definition, written as one word or
// as BY VAL (through checkstring, as a symbol has no letters to test). The start of the parameter's name, or NULL if
// the keyword is not there. SubLayoutBuild, and the compiler's survey (Stream.c): the PicoMite's.
char *CheckByKeyword(char *p, int kind) {
    char *q;
    if((q = checkstring(p, kind == 'V' ? "BYVAL" : "BYREF")) != NULL) return q;
    return checkstring(p, kind == 'V' ? "BY VAL" : "BY REF");
}

static struct s_sublayout *SubLayoutBuild(int index, char *p, char *argbuf2, char **argv2) {
    struct s_sublayout *L = NULL;
    char *q, *tp, *names = NULL;
    int argc2 = 0, i, n, size, pass, by, type;
    if(*p) makeargs(&p, MAX_ARG_COUNT, argbuf2, argv2, &argc2, (*p == '(') ? "(," : ",");
    if(argc2 && (argc2 & 1) == 0) error("Argument list");
    n = (argc2 + 1) / 2;
    size = sizeof(struct s_sublayout) + n * sizeof(struct s_subparam);
    for(pass = 0; pass < 2; pass++) {                               // twice: reading (every error raised) and sizing, then filling
        for(i = 0; i < argc2; i += 2) {
            q = argv2[i];
            skipspace(q);
            by = 0;
            {                                                       // BYVAL or BYREF (BY VAL, BY REF)
                char *bq;
                if((bq = CheckByKeyword(q, 'V')) != NULL) by = SL_BYVAL;
                else if((bq = CheckByKeyword(q, 'R')) != NULL) by = SL_BYREF;
                if(by) q = bq;                                      // the variable's start
            }
            type = T_NOTYPE;
            tp = skipvar(q, false);                                 // point to after the variable
            skipspace(tp);
            if(*tp == tokenAS) {                                    // are we using Microsoft syntax (eg, AS INTEGER)?
                CheckIfTypeSpecified(tp + 1, &type, true);          // and get the type
                if(!(type & T_IMPLIED)) error("Variable type");
            } else
                tp = q + strlen(q);                                 // (the text findvar is given ends at AS, or at the end)
            if(pass == 0)
                size += tp - q + 1;
            else {
                struct s_subparam *sp = &L->p[i >> 1];
                memcpy(names, q, tp - q);
                names[tp - q] = 0;
                sp->name = names;
                names += tp - q + 1;
                sp->type = type;
                sp->structarg = (type & T_STRUCT) ? g_StructArg : -1;   // (CheckIfTypeSpecified left it, above)
                sp->by = by;
            }
        }
        if(pass == 0) {
            if(SubLay == NULL) {
                SubLayN = NameSubs;                                 // (index is one of them)
                SubLay = GetMemory(SubLayN * sizeof(*SubLay));
            }
            if(index < SubLayN) L = SubLay[index] = SubLayoutPlace(size);   // a stale layout's space is left until the heap is wiped
            else L = GetTempMemory(size);                           // (not in the table: read again on each call)
            L->def = subfun[index];
            L->argc2 = argc2;
            names = (char *)&L->p[n];
        }
    }
    skipelement(p);                                                 // point to the body of the sub/fun
    L->body = p;
    return L;
}

// This function is responsible for executing a defined subroutine or function.
// As these two are similar they are processed in the one lump of code.
//
// The arguments when called are:
//   isfun    = true if we are executing a function
//   cmd      = pointer to the command name used by the caller (in program memory)
//   index    = index into subfun[i] which points to the definition of the sub or funct
//   fa, i64a, sa and typ are pointers to where the return value is to be stored (used by functions only)
//   BYREF and BYVAL qualifiers added per Geoff's email


/* FUNCTIONs that return a whole array (the PicoMite's): FUNCTION f(args) AS INTEGER(d1 [,d2 ...]) or AS FLOAT(...).
   The result can only be the source of a whole-array assignment, b() = f(...) (LetArrayCopy, Commands.c), which
   sanctions the call by setting FunReturnArrayCount to -1 first; DefinedSubFun refuses any other call before the
   body runs, and on return leaves the element count there and the elements in temporary memory. */
int FunReturnArrayCount = 0;

// the result array: the function's name and the definition's "(d1 [,d2 ...])" joined, so that findvar's own
// dimension parsing (expressions, OPTION BASE, MAXDIM) makes it; *count is its number of elements
static void *FunArrayCreate(char *fun_name, char *dims, int FunType, int *count) {
    char nbuf[MAXVARLEN + 84];
    int nl = strlen(fun_name), depth = 0, inquote = false, d;
    char *dp = dims;
    memcpy(nbuf, fun_name, nl);
    do {
        if(*dp == 0 || nl >= (int)sizeof(nbuf) - 1) error("Dimensions");
        if(*dp == '"') inquote = !inquote;
        if(!inquote) {
            if(*dp == '(' || (tokentype(*dp) & T_FUN)) depth++;     // (a function's token carries its opening bracket)
            else if(*dp == ')') depth--;
        }
        nbuf[nl++] = *dp++;
    } while(depth > 0);
    nbuf[nl] = 0;
    findvar(nbuf, FunType | V_FUNCT);                               // the local array
    *count = 1;
    for(d = 0; d < MAXDIM && !DimIsEnd(VREC(VarIndex)->dimtbl[d]); d++) *count *= DimElements(VREC(VarIndex)->dimtbl[d]);
    return VREC(VarIndex)->val.s;                                   // its data
}

// after AS <type>: the dimensions, if the function returns an array (else NULL); an unsanctioned call is refused
// here, before gosubindex moves or the body runs (the error would otherwise leave a frame, and the body's effects)
static char *CheckFunArrayDef(char *p, int FunType, int sanctioned, char *callerlp) {
    skipspace(p);
    if(*p != '(') return NULL;
    if(!(FunType & (T_INT | T_NBR))) error("Variable type");        // INTEGER or FLOAT arrays only
    if(!sanctioned) {
        CurrentLinePtr = callerlp;                                  // reported at the call
        error("Array function is only valid in an array assignment");
    }
    return p;
}

// the result, copied to temporary memory at the caller's level (the locals go when the function ends)
static char *FunArrayReturn(void *base, int count) {
    char *tmp;
    LocalIndex--;
    tmp = GetTempMemory(count * 8);
    LocalIndex++;
    memcpy(tmp, base, count * 8);                                   // (MMFLOAT and long long int are both 8 bytes)
    FunReturnArrayCount = count;
    return tmp;
}

void DefinedSubFun(int isfun, char *cmd, int index, MMFLOAT *fa, long long int *i64a, char **sa, int *typ) {

	char *p, *s, *tp, *ttp;
	char *FunArrayDims = NULL;                                  // "(d1 [,d2 ...])" in the definition: it returns an array
	int FunArrayCount = 0;                                      // and its number of elements
	int FunArraySanctioned = (FunReturnArrayCount < 0);         // b() = f(...) called it (the calls inside it are not)
	FunReturnArrayCount = 0;
	int tcmdtoken;                                              // the caller's command, across a FUNCTION body
	char *CallersLinePtr, *SubLinePtr = NULL;
    char *argbuf1; char **argv1; int argc1;
    char *argbuf2; char **argv2; int argc2;
    char *argbyref;
    char fun_name[MAXVARLEN + 2];                                  // the name and its suffix (it held one byte too few)
    char defsuffix = 0, callsuffix = 0;
	int i=0;
    int ArgType, FunType;
    int *argtype;
    union u_argval *argval;
    int *argVarIndex;
    int *argsize;                                                   // a variable argument's size (a string's length: a member's own)
    int FunStruct = -1;                                             // a FUNCTION AS a TYPE's name: its number
    struct s_sublayout *L;                                          // the definition's parameter list, as read on its first call (P6 F1)
    // Any errors generated after gosubindex is incremented need to restore the original value
    // Any variables created if LocalIndex was incremented also need to be cleared
    // Memory allocated to *argval needs to be recovered.
    // This allows unit tests to recover cleanly from skipped errors.i.e. ON ERROR SKIP
    DefinedSubFunLocalIndex=LocalIndex;  //save the LocalIndex

    CallersLinePtr = CurrentLinePtr;
    SubLinePtr = subfun[index];                                     // used for error reporting
    p =  SubLinePtr + sizeof(CommandToken);                         // point to the sub or function definition
    skipspace(p);
    ttp = p;

    // copy the sub/fun name from the definition into temp storage and terminate
    // p is left pointing to the end of the name (ie, start of the argument list in the definition)
    CurrentLinePtr = SubLinePtr;                                    // report errors at the definition
    tp = fun_name;
    {                                                               // the name, a symbol or text (Symbols.h)
        const char *s;
        int l;
        p = NameView(p, &s, &l);
        if(l > MAXVARLEN) l = MAXVARLEN;
        memcpy(tp, s, l);
        tp += l;
    }
    if(*p == '$' || *p == '%' || *p == '!') {
        if(!isfun) {
        	error("Type specification is invalid: @", (int)(*p));
        }
        defsuffix = *p;
        *tp++ = *p++;
    }
    *tp = 0;

    //if(isfun && *p != '(' && (*SubLinePtr != cmdCFUN)) error("Function definition");
    if(isfun && *p != '(' /*&& (*SubLinePtr != cmdCFUN)*/) error("Function definition");

    // find the end of the caller's identifier, tp is left pointing to the start of the caller's argument list
    CurrentLinePtr = CallersLinePtr;                                // report errors at the caller
    if(issymbol(*cmd))
        tp = cmd + symbolsize(*cmd);                                // a symbol (Symbols.h)
    else {
        tp = cmd + 1;
        while(isnamechar(*tp)) tp++;
    }
    if(*tp == '$' || *tp == '%' || *tp == '!') {
        if(!isfun) error("Type specification");
        callsuffix = *tp;
        tp++;
    }
    if(defsuffix != callsuffix) error("Inconsistent type suffix");  // (it compared the names' last bytes: a symbol's are not letters)

    // if this is a function we check to find if the function's type has been specified with AS <type> and save it
    CurrentLinePtr = SubLinePtr;                                    // report errors at the definition
    FunType = T_NOTYPE;
    if(isfun) {
        ttp = skipvar(ttp, false);                                  // point to after the function name and bracketed arguments
        skipspace(ttp);
        if(*ttp == tokenAS) {                                       // are we using Microsoft syntax (eg, AS INTEGER)?
            ttp++;                                                  // step over the AS token
            ttp = CheckIfTypeSpecified(ttp, &FunType, true);        // get the type
            if(!(FunType & T_IMPLIED)) error("Variable type");
            FunStruct = g_StructArg;                                // AS a TYPE's name (reading the parameters changes g_StructArg)
            FunArrayDims = CheckFunArrayDef(ttp, FunType, FunArraySanctioned, CallersLinePtr);   // AS INTEGER(6), say
        }
        FunType |= (V_FIND | V_DIM_VAR | V_LOCAL | V_EMPTY_OK);
    }


    // from now on
    // tp  = the caller's argument list
    // p   = the argument list for the definition
    skipspace(tp); skipspace(p);

    // if this is a CFUNCTION we can skip all the rest and just execute the CFUNCTION and return its value
    /*
           if(*SubLinePtr == cmdCFUN) {
               skipspace(p);
               if(*p != '(')
                   *typ = T_INT;
               else {                                                      // find the type
                   char *pp = p;
                   while(*pp != ')' && *pp != 0) pp++;
                   if(*pp == 0) error("Syntax");
                   pp++; skipspace(pp);
                   CheckIfTypeSpecified(pp, typ, false);
                   *typ &= ~T_IMPLIED;
               }
               switch(*typ) {                                              // return the correct type of value
                   union {
                       float ftmp;
                       int itmp;
                   } u;
                   case T_INT:  *i64a = CallCFunction(SubLinePtr, tp, p, CallersLinePtr); break;
                   case T_NBR:  u.itmp = (int)CallCFunction(SubLinePtr, tp, p, CallersLinePtr);
                                *fa = u.ftmp;
                               // #if !defined(MX170)
                               //   RoundDoubleFloat(fa);
                               // #endif
                                break;
                   case T_STR:  *sa = (char *)((unsigned int)CallCFunction(SubLinePtr, tp, p, CallersLinePtr)); break;
               }
               TempMemoryIsChanged = true;                                 // signal that temporary memory should be checked
               return;
           }
  */

    // similar if this is a CSUB
    if(commandtbl_at(SubLinePtr) == cmdCSUB) {
        CallCFunction(SubLinePtr, tp, p, CallersLinePtr);           // run the CSUB
        TempMemoryIsChanged = true;                                 // signal that temporary memory should be checked
        return;
    }

    // from now on we have a user defined sub or function (not a C routine)

    if(gosubindex >= MAXGOSUB) error("Too many nested SUB/FUN");


    errorstack[gosubindex] = CallersLinePtr;
    substack[gosubindex] = SubLinePtr;                              // for STATIC: the variables belong to this SUB/FUNCTION
	gosubstack[gosubindex++] = isfun ? NULL : nextstmt;             // NULL signifies that this is returned to by ending ExecuteProgram()

	#define buffneeded MAX_ARG_COUNT*(sizeof(union u_argval)+ 2*sizeof(int)+3*sizeof(char *)+sizeof(char))+ 2*STRINGSIZE
    // allocate memory for processing the arguments
    //argval=GetMemory(buffneeded);
    //argval=GetSystemMemory(buffneeded);
    argval=GetInternalMemoryNull(buffneeded);                       // on-chip while there is room (2.8 KB a call, MAX_ARG_COUNT
    if(argval == NULL) argval=GetMemory(buffneeded);                // 75: about 80 deep), then SDRAM up to MAXGOSUB
    DefinedSubFunMem=(uint32_t)argval;      //save pointer to memory for cleanup on error
    argtype=(void *)argval+MAX_ARG_COUNT * sizeof(union u_argval);
    argVarIndex = (void *)argtype+MAX_ARG_COUNT * sizeof(int);
    argbuf1 = (void *)argVarIndex+MAX_ARG_COUNT * sizeof(int);
    argv1 = (void *)argbuf1+STRINGSIZE;
    argbuf2 = (void *)argv1+MAX_ARG_COUNT * sizeof(char *);
    argv2 = (void *)argbuf2+STRINGSIZE;
    argsize = (void *)argv2+MAX_ARG_COUNT * sizeof(char *);        // (in buffneeded's third char * slot)
    argbyref=(void *)argsize+MAX_ARG_COUNT * sizeof(int);

    // now split up the arguments in the caller
    CurrentLinePtr = CallersLinePtr;                                // report errors at the caller
    argc1 = 0;
    if(*tp) makeargs(&tp, MAX_ARG_COUNT, argbuf1, argv1, &argc1, (*tp == '(') ? "(," : ",");

    // the arguments in the definition, as read on its first call (P6 F1)
    CurrentLinePtr = SubLinePtr;                                    // any errors must be at the definition
    L = (SubLay != NULL && index < SubLayN && SubLay[index] != NULL && SubLay[index]->def == subfun[index]) ? SubLay[index] : SubLayoutBuild(index, p, argbuf2, argv2);
    argc2 = L->argc2;

    // error checking
    CurrentLinePtr = CallersLinePtr;                                // report errors at the caller
    if(argc1 > argc2 || (argc1 && (argc1 & 1) == 0)) { error("Argument list");}

	// step through the arguments supplied by the caller and get the value supplied
    // these can be:
    //    - missing (ie, caller did not supply that parameter)
    //    - a variable, in which case we need to get a pointer to that variable's data and save its index so later we can get its type
    //    - an expression, in which case we evaluate the expression and get its value and type
    for(i = 0; i < argc2; i += 2) { // count through the arguments in the definition of the sub/fun
    	if(i < argc1 && *argv1[i]) {

             // check if the argument is a valid variable
   			 if(i < argc1 && isnamestartsym(*argv1[i]) && *skipvar(argv1[i], false) == 0){
                 // yes, it is a variable (or perhaps a user defined function which looks the same)?
                 if(!(FindSubFun(argv1[i], 1) >= 0 && strchr(argv1[i], '(') != NULL)) {
                     // yes, this is a valid variable.  set argvalue to point to the variable's data and argtype to its type
                	 argval[i].s = findvar(argv1[i], V_FIND | V_EMPTY_OK);        // get a pointer to the variable's data
                     argtype[i] = VREC(VarIndex)->type;                          // and the variable's type
                     argsize[i] = VREC(VarIndex)->size;
                     if((argtype[i] & T_STRUCT) && g_StructMemberType != 0) {    // a structure's member: its own type and size
                         argtype[i] = g_StructMemberType;
                         argsize[i] = g_StructMemberSize;
                     }
                     argVarIndex[i] = VarIndex;
                     if(argtype[i] & T_CONST) {
                         argtype[i] = 0;                                          // we don't want to point to a constant
                     } else {
                    	 argtype[i] |= T_PTR;                                     // flag this as a pointer
                     }
                 }
             }

            // BYVAL or BYREF in the sub/fun definition (as read on its first call)
             argbyref[i]=0;
             if(L->p[i >> 1].by == SL_BYVAL) {                      // if BYVAL: an array is refused, and only a variable argument
                    // can be one (an expression's argVarIndex was never set: the PicoMite's ByValArrayCheck)
                	if((argtype[i] & T_PTR) && DimIsRealArray(VREC(argVarIndex[i])->dimtbl[0])){
              	    	/*
              	    	 * Set tp to point to the current parameter
              	    	 * See if we have an array or an array element
              	    	 */
                		tp=argv1[i];
                		do { tp++;} while(*tp != '(');  // We should find a '(' because it must be an array or an array element to get here
                  	    tp++;
                  	    skipspace(tp);
                  	    if(*tp == ')') error("Array as BYVAL not allowed $",argv1[i]);
                 	}
              	    argtype[i] = 0;                                     // (and the caller's pointer flag removed)
             } else if(L->p[i >> 1].by == SL_BYREF) {               // if BYREF
                	  if((argtype[i] & T_PTR) == 0)error("Variable required for BYREF $", argv1[i]);
            	      argbyref[i]=1;
             }

             // if argument is present and is not a pointer to a variable then evaluate it as an expression
            if(argtype[i] == 0) {
                long long int ia;
                int argt = T_NOTYPE;
                evaluate(argv1[i], &argval[i].f, &ia, &s, &argt, false);   // get the value and type of the argument
                if(argt & T_INT)
                    argval[i].i = ia;
                else if(argt & T_STR) {
                    argval[i].s = GetMemory(STRINGSIZE);
                    Mstrcpy(argval[i].s, s);
                }
                argtype[i] = argt;                                  // only now: error() frees argval[i].s of a T_STR argument
            }

        }
    }



    // now we step through the parameters in the definition of the sub/fun
    // for each one we create the local variable and compare its type to that supplied in the callers list

    CurrentLinePtr = SubLinePtr;                                    // any errors must be at the definition
    if(CallersLinePtr == NULL && LocalIndex == 0 && hashlistpointer) {  // a call typed at the prompt, over the locals an
        ClearVars(1);                                               // error or a break left in a SUB or FUNCTION on the level
        ContinuePoint = NULL;                                       // this call takes ("Local variable already declared"):
    }                                                               // that call is over, and CONTINUE with it. (Not at the
                                                                    // prompt itself: CONTINUE needs them, and their loops.)
    LocalIndex++;
    for(i = 0; i < argc2; i += 2) {                                 // count through the arguments in the definition of the sub/fun
        ArgType = L->p[i >> 1].type;                                // its AS type, as read on the first call
        g_StructArg = L->p[i >> 1].structarg;                       // (and a TYPE's number)
        ArgType |= (V_FIND | V_DIM_VAR | V_LOCAL | V_EMPTY_OK);
        tp = findvar(L->p[i >> 1].name, ArgType);                   // declare the local variable
        if(DimIsRealArray(VREC(VarIndex)->dimtbl[0])){error("Argument list"); }   // if it is an array it must be an empty array
        CurrentLinePtr = CallersLinePtr;                            // report errors at the caller

        // if the definition called for an array, special processing and checking will be required
       	if(DimIsEmptyParam(VREC(VarIndex)->dimtbl[0])) {
        	int j;
            if(DimIsScalar(VREC(argVarIndex[i])->dimtbl[0]))  { error("Expected an array");}

            if(TypeMask(VREC(VarIndex)->type) != TypeMask(argtype[i])){error("Incompatible type: $", argv1[i]);}
            if((VREC(VarIndex)->type & T_STRUCT) && VREC(VarIndex)->size != argsize[i]) error("Structure type mismatch: $", argv1[i]);
            VREC(VarIndex)->val.s = NULL;
            for(j = 0; j < MAXDIM; j++)                             // copy the dimensions of the supplied variable into our local variable
                VREC(VarIndex)->dimtbl[j] = VREC(argVarIndex[i])->dimtbl[j];

        } else if(VREC(VarIndex)->type & T_STRUCT) {                // a structure, by reference: the caller's (Struct.c)
            StructParam(VarIndex, argVarIndex[i], argval[i].s, argtype[i], argsize[i], argv1[i]);
            continue;
        }


        // if this is a pointer check if the type is NOT the same as that requested in the sub/fun definition
        if((argtype[i] & T_PTR) && TypeMask(VREC(VarIndex)->type) != TypeMask(argtype[i])) {
        	if(argbyref[i]) {error("BYREF requires same types: $", argv1[i]);}          //BYREF requires same types.
        	if((TypeMask(VREC(VarIndex)->type) & T_STR) || (TypeMask(argtype[i]) & T_STR))
        	   {error("Incompatible type: $", argv1[i]);}
            // make this into an ordinary argument: argval[i].s points at the data (findvar resolved an element, a
            // member, or what a pointer variable points to; reading the variable's own pointer read element 0)
            argval[i].i = *(long long int *)argval[i].s;
            argtype[i] &= ~T_PTR;                                   // and remove the pointer flag
        }


        // if this is a pointer (note: at this point the caller type and the required type must be the same)
        if(argtype[i] & T_PTR) {
            // the argument supplied was a variable so we must setup the local variable as a pointer
            if((VREC(VarIndex)->type & T_STR) && VREC(VarIndex)->val.s != NULL) {
                if(!VAR_INLINE_STR(VREC(VarIndex))) FreeMemorySafe((void *)&VREC(VarIndex)->val.s);     // free up the local variable's memory if it is a pointer to a string
            }
            VREC(VarIndex)->val.s = argval[i].s;                              // point to the data of the variable supplied as an argument
            VREC(VarIndex)->type |= T_PTR;                                    // set the type to a pointer
            VREC(VarIndex)->size = argsize[i];                                // just in case it is a string copy the size (a member's own)
        // this is not a pointer
        } else if(argtype[i] != 0) {                                           // in getting the memory argtype[] is initialised to zero
            // the parameter was an expression or a just straight variables with different types (therefore not a pointer))
            if((VREC(VarIndex)->type & T_STR) && (argtype[i] & T_STR)) {      // both are a string
                Mstrcpy(VREC(VarIndex)->val.s, argval[i].s);
                FreeMemorySafe((void *)&argval[i].s);
            } else if((VREC(VarIndex)->type & T_NBR) && (argtype[i] & T_NBR)) // both are a float
                VREC(VarIndex)->val.f = argval[i].f;
            else if((VREC(VarIndex)->type & T_NBR) && (argtype[i] & T_INT))   // need a float but supplied an integer
                VREC(VarIndex)->val.f = argval[i].i;
            else if((VREC(VarIndex)->type & T_INT) && (argtype[i] & T_INT))   // both are integers
                VREC(VarIndex)->val.i = argval[i].i;
            else if((VREC(VarIndex)->type & T_INT) && (argtype[i] & T_NBR))   // need an integer but was supplied with a MMFLOAT
                VREC(VarIndex)->val.i = FloatToInt64(argval[i].f);
            else
                { error("Incompatible type: $", argv1[i]);}
        }

    }


    // temp memory used in setting up the arguments can be deleted now
    FreeMemory((void*)argval);
    DefinedSubFunMem=0;                                             //we got here so we wont need to cleanup any memory
    // if it is a defined command we simply point to the first statement in our command and allow ExecuteProgram() to carry on as before
    // exit from the sub is via cmd_return which will decrement LocalIndex
    if(!isfun) {
        nextstmt = L->body;                                         // point to the body of the subroutine
        return;
    }

    // if it is a defined function we have a lot more work to do.  We must:
    //   - Create a local variable for the function's name
    //   - Save the globals being used by the current command that caused the function to be called
    //   - Invoke another instance of ExecuteProgram() to execute the body of the function
    //   - When that returns we need to restore the global variables
    //   - Get the variable's value and save that in the return value globals (fret or sret)
    //   - Return to the expression parser
    g_StructArg = FunStruct;
    {                                                               // declare the local variable: from the definition's symbol when it
        char funsym[5], *fn = fun_name;                             // has one, so that it is bound (from text it would turn off the
        char *dn = SubLinePtr + sizeof(CommandToken);               // global bindings inside every FUNCTION: Symbols.h)
        skipspace(dn);
        if(issymbol(*dn)) {
            int k = symbolsize(*dn);
            memcpy(funsym, dn, k);
            if(defsuffix) funsym[k++] = defsuffix;
            funsym[k] = 0;
            fn = funsym;
        }
        if(FunArrayDims != NULL) tp = FunArrayCreate(fn, FunArrayDims, FunType, &FunArrayCount);   // the result array
        else tp = findvar(fn, FunType | V_FUNCT);
    }
    FunType = VREC(VarIndex)->type;
    if(FunType & T_STR) {
        if(!VAR_INLINE_STR(VREC(VarIndex))) FreeMemorySafe((void *)&VREC(VarIndex)->val.s);   // free the memory if it is a string
        VREC(VarIndex)->type |= T_PTR;
        LocalIndex--;                                               // allocate the memory at the previous level
        VREC(VarIndex)->val.s = tp = GetTempMemory(STRINGSIZE);    // and use our own memory
        LocalIndex++;
    }
    p = L->body;                                                    // point to the body of the function

    ttp = nextstmt;                                                 // save the globals used by commands
    tcmdtoken = cmdtoken;
    s = cmdline;

    ExecuteProgram(p);                                              // execute the function's code
    CurrentLinePtr = CallersLinePtr;                                // report errors at the caller

    cmdline = s;                                                    // restore the globals
    cmdtoken = tcmdtoken;
    nextstmt = ttp;

    // return the value of the function's variable to the caller
    if(FunArrayCount > 0)
        *sa = FunArrayReturn(tp, FunArrayCount);                    // a whole array, through temporary memory
    else if(FunType & T_NBR)
        *fa = *(MMFLOAT *)tp;
    else if(FunType & T_INT)
        *i64a = *(long long int *)tp;
    else if(FunType & T_STRUCT)
        *sa = StructReturn(tp, FunStruct);                          // a structure: copied out of the locals (Struct.c)
    else
        *sa = tp;                                                   // for a string we just need to return the local memory
    *typ = FunType;                                                 // save the function type for the caller
	ClearVars(LocalIndex--);                                        // delete any local variables
    TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
	gosubindex--;

}

#ifdef OLD

// This function is responsible for executing a defined subroutine or function.
// As these two are similar they are processed in the one lump of code.
//
// The arguments when called are:
//   isfun    = true if we are executing a function
//   cmd      = pointer to the command name used by the caller (in program memory)
//   index    = index into subfun[i] which points to the definition of the sub or funct
//   fa, i64a, sa and typ are pointers to where the return value is to be stored (used by functions only)
void DefinedSubFun(int isfun, char *cmd, int index, MMFLOAT *fa, long long int *i64a, char **sa, int *typ) {
	char *p, *s, *tp, *ttp, tcmdtoken;
	char *CallersLinePtr, *SubLinePtr = NULL;
    char *argbuf1; char **argv1; int argc1;
    char *argbuf2; char **argv2; int argc2;
    bool *argbyref;
    char fun_name[MAXVARLEN + 1];
	int i;
    int ArgType, FunType;
    int *argtype;
    union u_argval {
        MMFLOAT f;                                                    // the value if it is a float
        long long int i;                                            // the value if it is an integer
        MMFLOAT *fa;                                                  // pointer to the allocated memory if it is an array of floats
        long long int *ia;                                          // pointer to the allocated memory if it is an array of integers
        char *s;                                                    // pointer to the allocated memory if it is a string
    } *argval;
    int *argVarIndex;

    CallersLinePtr = CurrentLinePtr;
    SubLinePtr = subfun[index];                                     // used for error reporting
    p =  SubLinePtr + 1;                                            // point to the sub or function definition
    skipspace(p);
    ttp = p;

    // copy the sub/fun name from the definition into temp storage and terminate
    // p is left pointing to the end of the name (ie, start of the argument list in the definition)
    CurrentLinePtr = SubLinePtr;                                    // report errors at the definition
    tp = fun_name;
    *tp++ = *p++; while(isnamechar(*p)) *tp++ = *p++;
    if(*p == '$' || *p == '%' || *p == '!') {
        if(!isfun) {
            error("Type specification is invalid: @", (int)(*p));
        }
        *tp++ = *p++;
    }
    *tp = 0;

    if(isfun && *p != '(' /*&& (*SubLinePtr != cmdCFUN)*/) error("Function definition");

    // find the end of the caller's identifier, tp is left pointing to the start of the caller's argument list
    CurrentLinePtr = CallersLinePtr;                                // report errors at the caller
    tp = cmd + 1;
    while(isnamechar(*tp)) tp++;
    if(*tp == '$' || *tp == '%' || *tp == '!') {
        if(!isfun) error("Type specification");
        tp++;
    }
    if((*(p-1)) != (*(tp-1))) error("Inconsistent type suffix");

    // if this is a function we check to find if the function's type has been specified with AS <type> and save it
    CurrentLinePtr = SubLinePtr;                                    // report errors at the definition
    FunType = T_NOTYPE;
    if(isfun) {
        ttp = skipvar(ttp, false);                                  // point to after the function name and bracketed arguments
        skipspace(ttp);
        if(*ttp == tokenAS) {                                       // are we using Microsoft syntax (eg, AS INTEGER)?
            ttp++;                                                  // step over the AS token
            ttp = CheckIfTypeSpecified(ttp, &FunType, true);        // get the type
            if(!(FunType & T_IMPLIED)) error("Variable type");
        }
        FunType |= (V_FIND | V_DIM_VAR | V_LOCAL | V_EMPTY_OK);
    }


    // from now on
    // tp  = the caller's argument list
    // p   = the argument list for the definition
    skipspace(tp); skipspace(p);
    if(*SubLinePtr == cmdCSUB) {
        CallCFunction(SubLinePtr, tp, p, CallersLinePtr);           // run the CSUB
        TempMemoryIsChanged = true;                                 // signal that temporary memory should be checked
        return;
    }

    // from now on we have a user defined sub or function (not a C routine)

    if(gosubindex >= MAXGOSUB) error("Too many SUB and FUN");
    errorstack[gosubindex] = CallersLinePtr;
	gosubstack[gosubindex++] = isfun ? NULL : nextstmt;             // NULL signifies that this is returned to by ending ExecuteProgram()
#define buffneeded MAX_ARG_COUNT*(sizeof(union u_argval)+ 2*sizeof(int)+3*sizeof(unsigned char *)+sizeof(unsigned char))+ 2*STRINGSIZE
    // allocate memory for processing the arguments
    argval=GetMemory(buffneeded);
    argtype=(void *)argval+MAX_ARG_COUNT * sizeof(union u_argval);
    argVarIndex = (void *)argtype+MAX_ARG_COUNT * sizeof(int);
    argbuf1 = (void *)argVarIndex+MAX_ARG_COUNT * sizeof(int);
    argv1 = (void *)argbuf1+STRINGSIZE;
    argbuf2 = (void *)argv1+MAX_ARG_COUNT * sizeof(unsigned char *);
    argv2 = (void *)argbuf2+STRINGSIZE;
    argbyref=(void *)argv2+MAX_ARG_COUNT * sizeof(unsigned char *);
    // now split up the arguments in the caller
    CurrentLinePtr = CallersLinePtr;                                // report errors at the caller
    argc1 = 0;
    if(*tp) makeargs(&tp, MAX_ARG_COUNT, argbuf1, argv1, &argc1, (*tp == '(') ? "(," : ",");

    // split up the arguments in the definition
    CurrentLinePtr = SubLinePtr;                                    // any errors must be at the definition
    argc2 = 0;
    if(*p) makeargs(&p, MAX_ARG_COUNT, argbuf2, argv2, &argc2, (*p == '(') ? "(," : ",");

    // error checking
    if(argc2 && (argc2 & 1) == 0) error("Argument list");
    CurrentLinePtr = CallersLinePtr;                                // report errors at the caller
    if(argc1 > argc2 || (argc1 && (argc1 & 1) == 0)) error("Argument list");

	// step through the arguments supplied by the caller and get the value supplied
    // these can be:
    //    - missing (ie, caller did not supply that parameter)
    //    - a variable, in which case we need to get a pointer to that variable's data and save its index so later we can get its type
    //    - an expression, in which case we evaluate the expression and get its value and type
    for(i = 0; i < argc2; i += 2) {                                 // count through the arguments in the definition of the sub/fun
        if(i < argc1 && *argv1[i]) {
            // check if the argument is a valid variable
            if(i < argc1 && isnamestart(*argv1[i]) && *skipvar(argv1[i], false) == 0) {
                // yes, it is a variable (or perhaps a user defined function which looks the same)?
                if(!(FindSubFun(argv1[i], 1) >= 0 && strchr(argv1[i], '(') != NULL)) {
                    // yes, this is a valid variable.  set argvalue to point to the variable's data and argtype to its type
                    argval[i].s = findvar(argv1[i], V_FIND | V_EMPTY_OK);        // get a pointer to the variable's data
                    argtype[i] = VREC(VarIndex)->type;                          // and the variable's type
                    argVarIndex[i] = VarIndex;
                    if(argtype[i] & T_CONST) {
                        argtype[i] = 0;                                          // we don't want to point to a constant
                    } else {
                        argtype[i] |= T_PTR;                                     // flag this as a pointer
                    }
                }
            }
			// check for BYVAL or BYREF in sub/fun definition
            argbyref[i]=0;
			skipspace(argv2[i]);
			if(toupper(*argv2[i]) == 'B' && toupper(*(argv2[i]+1)) == 'Y') {
				if((checkstring(argv2[i] + 2, "VAL")) != NULL) {        // if BYVAL
					argtype[i] = 0;	                                    // remove any pointer flag in the caller
					argv2[i] += 5;										// skip to the variable start
				} else {
					if((checkstring(argv2[i] + 2, "REF")) != NULL) {    // if BYREF
						if((argtype[i] & T_PTR) == 0) error("Variable required for BYREF");
						argv2[i] += 5;									// skip to the variable start
					}
                    argbyref[i]=1;
				}
				skipspace(argv2[i]);
			}


            // if argument is present and is not a pointer to a variable then evaluate it as an expression
            if(argtype[i] == 0) {
                long long int ia;
                evaluate(argv1[i], &argval[i].f, &ia, &s, &argtype[i], false);   // get the value and type of the argument
                if(argtype[i] & T_INT)
                    argval[i].i = ia;
                else if(argtype[i] & T_STR) {
                    argval[i].s = GetStringMemory();
                    Mstrcpy(argval[i].s, s);
                }
            }
        }
    }

    // now we step through the parameters in the definition of the sub/fun
    // for each one we create the local variable and compare its type to that supplied in the callers list
    CurrentLinePtr = SubLinePtr;                                    // any errors must be at the definition
    LocalIndex++;
    for(i = 0; i < argc2; i += 2) {                                 // count through the arguments in the definition of the sub/fun
        ArgType = T_NOTYPE;
        tp = skipvar(argv2[i], false);                              // point to after the variable
        skipspace(tp);
        if(*tp == tokenAS) {                                        // are we using Microsoft syntax (eg, AS INTEGER)?
            *tp++ = 0;                                              // terminate the string and step over the AS token
            tp = CheckIfTypeSpecified(tp, &ArgType, true);          // and get the type
            if(!(ArgType & T_IMPLIED)) error("Variable type");
        }
        ArgType |= (V_FIND | V_DIM_VAR | V_LOCAL | V_EMPTY_OK);
        tp = findvar(argv2[i], ArgType);                            // declare the local variable
        if(DimIsRealArray(VREC(VarIndex)->dimtbl[0])) error("Argument list");    // if it is an array it must be an empty array

        CurrentLinePtr = CallersLinePtr;                            // report errors at the caller

        // if the definition called for an array, special processing and checking will be required
        if(DimIsEmptyParam(VREC(VarIndex)->dimtbl[0])) {
            int j;
            if(DimIsScalar(VREC(argVarIndex[i])->dimtbl[0])) error("Expected an array");
            if(TypeMask(VREC(VarIndex)->type) != TypeMask(argtype[i])) error("Incompatible type: $", argv1[i]);
            VREC(VarIndex)->val.s = NULL;
            for(j = 0; j < MAXDIM; j++)                             // copy the dimensions of the supplied variable into our local variable
                VREC(VarIndex)->dimtbl[j] = VREC(argVarIndex[i])->dimtbl[j];
        }

        // if this is a pointer check and the type is NOT the same as that requested in the sub/fun definition
        if((argtype[i] & T_PTR) && TypeMask(VREC(VarIndex)->type) != TypeMask(argtype[i])) {
            if(argbyref[i]){ error("BYREF requires same types: $", argv1[i]);}
            if((TypeMask(VREC(VarIndex)->type) & T_STR) || (TypeMask(argtype[i]) & T_STR))
                error("Incompatible type: $", argv1[i]);
            // make this into an ordinary argument
            if(VREC(argVarIndex[i])->type & T_PTR) {
                argval[i].i = *VREC(argVarIndex[i])->val.ia;       // get the value if the supplied argument is a pointer
            } else {
                argval[i].i = *(long long int *)argval[i].s;        // get the value if the supplied argument is an ordinary variable
            }
            argtype[i] &= ~T_PTR;                                   // and remove the pointer flag
        }

        // if this is a pointer (note: at this point the caller type and the required type must be the same)
        if(argtype[i] & T_PTR) {
            // the argument supplied was a variable so we must setup the local variable as a pointer
            if((VREC(VarIndex)->type & T_STR) && VREC(VarIndex)->val.s != NULL) {
                FreeMemorySafe((void *)&VREC(VarIndex)->val.s);                            // free up the local variable's memory if it is a pointer to a string
                }
            VREC(VarIndex)->val.s = argval[i].s;                              // point to the data of the variable supplied as an argument
            VREC(VarIndex)->type |= T_PTR;                                    // set the type to a pointer
            VREC(VarIndex)->size = VREC(argVarIndex[i])->size;               // just in case it is a string copy the size
        // this is not a pointer
        } else if(argtype[i] != 0) {                                           // in getting the memory argtype[] is initialised to zero
            // the parameter was an expression or a just straight variables with different types (therefore not a pointer))
            if((VREC(VarIndex)->type & T_STR) && (argtype[i] & T_STR)) {      // both are a string
                Mstrcpy(VREC(VarIndex)->val.s, argval[i].s);
                FreeMemorySafe((void *)&argval[i].s);
            } else if((VREC(VarIndex)->type & T_NBR) && (argtype[i] & T_NBR)) // both are a float
                VREC(VarIndex)->val.f = argval[i].f;
            else if((VREC(VarIndex)->type & T_NBR) && (argtype[i] & T_INT))   // need a float but supplied an integer
                VREC(VarIndex)->val.f = argval[i].i;
            else if((VREC(VarIndex)->type & T_INT) && (argtype[i] & T_INT))   // both are integers
                VREC(VarIndex)->val.i = argval[i].i;
            else if((VREC(VarIndex)->type & T_INT) && (argtype[i] & T_NBR))   // need an integer but was supplied with a MMFLOAT
                VREC(VarIndex)->val.i = FloatToInt64(argval[i].f);
            else
                error("Incompatible type: $", argv1[i]);
        }
    }

    // temp memory used in setting up the arguments can be deleted now
    FreeMemory(argval);

    varnamecopy(CurrentSubFunName, fun_name);
    // if it is a defined command we simply point to the first statement in our command and allow ExecuteProgram() to carry on as before
    // exit from the sub is via cmd_return which will decrement LocalIndex
    if(!isfun) {
        skipelement(p);
        nextstmt = p;                                               // point to the body of the subroutine
        return;
    }

    // if it is a defined function we have a lot more work to do.  We must:
    //   - Create a local variable for the function's name
    //   - Save the globals being used by the current command that caused the function to be called
    //   - Invoke another instance of ExecuteProgram() to execute the body of the function
    //   - When that returns we need to restore the global variables
    //   - Get the variable's value and save that in the return value globals (fret or sret)
    //   - Return to the expression parser
    tp = findvar(fun_name, FunType | V_FUNCT);                      // declare the local variable
    FunType = VREC(VarIndex)->type;
    if(FunType & T_STR) {
        if(!VAR_INLINE_STR(VREC(VarIndex))) FreeMemorySafe((void *)&VREC(VarIndex)->val.s);   // free the memory if it is a string
        VREC(VarIndex)->type |= T_PTR;
        LocalIndex--;                                               // allocate the memory at the previous level
        VREC(VarIndex)->val.s = tp = GetTempStrMemory();    // and use our own memory
        LocalIndex++;
    }
    skipelement(p);                                                 // point to the body of the function

    ttp = nextstmt;                                                 // save the globals used by commands
    tcmdtoken = cmdtoken;
    s = cmdline;

    ExecuteProgram(p);                                              // execute the function's code
    CurrentLinePtr = CallersLinePtr;                                // report errors at the caller

    cmdline = s;                                                    // restore the globals
    cmdtoken = tcmdtoken;
    nextstmt = ttp;

    // return the value of the function's variable to the caller
    if(FunType & T_NBR)
        *fa = *(MMFLOAT *)tp;
    else if(FunType & T_INT)
        *i64a = *(long long int *)tp;
    else
        *sa = tp;                                                   // for a string we just need to return the local memory
    *typ = FunType;                                                 // save the function type for the caller
	ClearVars(LocalIndex--);                                        // delete any local variables
    TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
	gosubindex--;
}

#endif

// The substitutions made on a line before it is tokenised (the PicoMite's, corrected: PicoMite issue #8).  Every
// target is a line buffer of STRINGSIZE bytes (inpbuf, or the loader's line).  A result that would not fit leaves
// the target as it was and sets strreplace_overflow, which the callers report as "Line ... is too long" (one
// 251-character line of DHT22s, each made BITBANG DHT22, ran past the buffers and hung the board).
int strreplace_overflow = 0;
extern char *fstrstr(const char *s1, const char *s2);                // strstr blind to case (Console.c): a DATA line keeps its case

// Replace each needle in target that is a whole word: no name character before it (bit 0 of ignoresurround waives
// that, as the start of the line does) and nothing after it that could continue a name (bit 1 waives that).  An
// occurrence that is not a whole word is left as it was.
void str_replace(char *target, const char *needle, const char *replacement, uint8_t ignoresurround)
{
    char buffer[STRINGSIZE];
    char *insert_point = buffer;
    const char *limit = &buffer[STRINGSIZE - 1];                    // the most a line buffer holds, less its zero
    const char *tmp = target, *p, *from;
    size_t needle_len = strlen(needle), repl_len = strlen(replacement), n;
    int whole;

    if(needle_len == 0) return;                                     // (an empty #DEFINE name: it would match forever)
    while((p = fstrstr(tmp, needle)) != NULL) {
        whole = (p == target || !isnamechar(p[-1]) || (ignoresurround & 1)) && (!isnameend(p[needle_len]) || (ignoresurround & 2))
            && !((ignoresurround & 4) && p[needle_len] == '(');    // 4: not a function of that name (SPI()
        from = whole ? replacement : p;                             // (not replaced: the text as it was, in its case)
        n = whole ? repl_len : needle_len;
        if(insert_point + (p - tmp) + n > limit) goto toolong;
        memcpy(insert_point, tmp, p - tmp);                         // the part before the needle
        insert_point += p - tmp;
        memcpy(insert_point, from, n);
        insert_point += n;
        tmp = p + needle_len;
    }
    if(insert_point + strlen(tmp) > limit) goto toolong;            // the rest
    strcpy(insert_point, tmp);
    strcpy(target, buffer);
    return;

toolong:
    strreplace_overflow = 1;                                        // the target is unchanged
}

// is the word (in capitals) at p, and not the start of a longer name?
static int IsWordAt(const char *p, const char *word) {
    int n = strlen(word);
    return strncasecmp(p, word, n) == 0 && !isnamechar(p[n]);
}

// Hide from the substitutions what they must not change: each character of a string gets bit 7 (the line holds
// only 7-bit characters by now), which no needle can match; a ' comment, and with stmts (tokenise) a REM statement,
// is taken off the line into comment.  False for a REM line, which needs no substitutions.
static int SubstHide(char *target, char *comment, int stmts) {
    char *ip = target;
    int inquote = false, stmt = true;
    *comment = 0;
    skipspace(ip);
    if(IsWordAt(ip, "REM")) return false;
    for( ; *ip; ip++) {
        if(*ip == '"') {
            inquote = !inquote;
            continue;
        }
        if(inquote) {
            *ip |= 0x80;
            continue;
        }
        if(*ip == '\'' || (stmts && stmt && IsWordAt(ip, "REM"))) {
            strcpy(comment, ip);                                    // a comment: the rest of the line
            *ip = 0;
            break;
        }
        if(*ip == ':')
            stmt = true;
        else if(*ip != ' ')
            stmt = false;
    }
    return true;
}

// and hide each DATA statement too, to the next : (one in a string is hidden already)
static void SubstHideData(char *target) {
    char *ip = target;
    int stmt = true;
    for( ; *ip; ip++) {
        if(*ip == ':')
            stmt = true;
        else if(stmt && *ip != ' ') {
            stmt = false;
            if(IsWordAt(ip, "DATA"))
                for( ; *ip && *ip != ':'; ip++) *ip |= 0x80;
            if(*ip == ':') stmt = true;
            if(*ip == 0) break;
        }
    }
}

// and show it again
static void SubstShow(char *target, char *comment) {
    char *ip;
    for(ip = target; *ip; ip++) *ip &= 0x7f;
    if(*comment) {
        if(strlen(target) + strlen(comment) > STRINGSIZE - 1)
            strreplace_overflow = 1;                                // the substitution left no room for the comment
        else
            strcat(target, comment);
    }
}

// one substitution (the loader's #DEFINEs and its spacing), never in a string or a comment
void STR_REPLACE(char *target, const char *needle, const char *replacement, uint8_t ignoresurround){
    char comment[STRINGSIZE];
    if(!SubstHide(target, comment, false)) return;
    str_replace(target, needle, replacement, ignoresurround);
    SubstShow(target, comment);
}

// The functions tokenise() folds into one token for each family (the PicoMite's): BIN$( is written BASE$(2, and so on.
// A function's own table entry allowed spaces before its bracket (LEFT$ (a$, 2)), so they are closed up before the fold.
// RAY( is the CMM2's, to keep the last one-byte tokens: SPRITE( has its types, and fun_sprite hands it to fun_ray (Ray.c)
static const char *const FoldFrom[] = {"BIN$", "OCT$", "HEX$", "LCASE$", "UCASE$", "LEFT$", "RIGHT$", "MIN", "MAX", "BIT", "BYTE", "FLAG",
    "RAY"};
static const char *const FoldTo[] = {"BASE$(2,", "BASE$(8,", "BASE$(16,", "SCHANGE$(L,", "SCHANGE$(U,", "SCHANGE$(E,",
    "SCHANGE$(R,", "TOPBOTTOM(I,", "TOPBOTTOM(A,", "BITBYTEFLAG(1,", "BITBYTEFLAG(2,", "BITBYTEFLAG(3,",   // (a digit: no symbol is made of it)
    "SPRITE(RAY,"};
const char *const FoldList[] = {"Bin$(", "Oct$(", "Hex$(", "LCase$(", "UCase$(", "Left$(", "Right$(", "Min(", "Max(", "Bit(", "Byte(", "Flag(",
    "Ray("};   // (LIST FUNCTIONS too)
#define FOLDS (sizeof(FoldFrom) / sizeof(FoldFrom[0]))
const int FoldCount = FOLDS;

// tokenise()'s half, on a line whose strings and comments are hidden (SubstHide)
static void SubstFold(char *target) {
    char from[10];
    str_replace(target, "PEEK(BYTE", "PEEK(INT8", 2);             // PEEK's BYTE, not the BYTE( function (the PicoMite's: PETSCII
                                                                    // Robots' PEEK(BYTE(y)*129+x)); LIST turns it back
    for(int i = 0; i < FOLDS; i++) {
        int n = strlen(FoldFrom[i]);
        char *p = target, *q;
        while((p = fstrstr(p, FoldFrom[i])) != NULL) {              // LEFT$ ( closed up to LEFT$(
            q = p + n;
            if((p == target || !isnamechar(p[-1])) && *q == ' ') {
                char *r = q;
                while(*r == ' ') r++;
                if(*r == '(') memmove(q, r, strlen(r) + 1);
            }
            p = q;
        }
        strcpy(from, FoldFrom[i]);
        strcat(from, "(");
        str_replace(target, from, FoldTo[i], 2);                    // (2: not the end of a longer name, such as a FUNCTION TOHEX$()
    }
}

// and LIST's: each turned back, in the case LIST uses for a function (OPTION LIST CASE), never in a string or a comment,
// nor at the end of a longer name (a FUNCTION TOBASE$( called with 2 stays as it is)
void ListUnfold(char *b) {
    char comment[STRINGSIZE], to[10];
    if(!SubstHide(b, comment, true)) return;
    for(int i = 0; i < FOLDS; i++) {
        int k;
        for(k = 0; FoldList[i][k]; k++)
            to[k] = Option.Listcase == CONFIG_LOWER ? tolower(FoldList[i][k]) : Option.Listcase == CONFIG_UPPER ? toupper(FoldList[i][k]) : FoldList[i][k];
        to[k] = 0;
        str_replace(b, FoldTo[i], to, 2);
    }
    str_replace(b, "PEEK(INT8", "PEEK(BYTE", 2);
    SubstShow(b, comment);
}

/********************************************************************************************************************************************
 take an input line and turn it into a line with tokens suitable for saving into memory
********************************************************************************************************************************************/

//take an input string in inpbuf[] and copy it to tknbuf[] and:
// - convert the line number to a binary number
// - convert a label to the token format
// - convert keywords to tokens
// - convert the colon to a zero char
//the result in tknbuf[] is terminated with double zero chars
// if the arg console is true then do not add a line number

// every byte tokenise() writes goes through TKNPUT, which stops at the end of tknbuf
#define TKNPUT(c) do { if(op >= tknend) goto toolong; *op++ = (c); } while(0)
#define TKNPUTCMD(i) do { if(op + sizeof(CommandToken) > tknend) goto toolong; op = commandtbl_put(op, (i)); } while(0)
char tknerrmsg[80];
// the source of the line being tokenised for a program image: its file (1 = the main program) and line,
// which go into the line's header, and the file's name (NULL for the main program) for messages
int tknfile = 1, tknline = 0;
char *tknfilename = NULL;
int MIPS16 tokenise(int console) {
    char *p, *op, *tp, *tknend = tknbuf + TKNBUFSIZE - 3;       // room for the three terminating zeros
    int i=0;
    int firstnonwhite;
    int labelvalid;
    // symbols: names in a program being saved become symbols (see Symbols.h), except in DATA statements, CSUB
    // and DefineFont blocks, the letter after ~( and &H/&O/&B numbers.  A console line never makes them.
    static int tkDATA = -1, tkDEFINEFONT, tkENDCSUB, tkENDDEFINEFONT, tkTILDE, tkSCHANGE, tkTOPBOTTOM;
    int symbols = !console && SymMode != SYM_OFF;
    int symrawstmt = false;                                         // this statement is DATA: its names stay text
    int symrawnext = false;                                         // the next name is the letter after ~(, SCHANGE$( or TOPBOTTOM(
    if(tkDATA < 0) {
        tkDATA = GetCommandValue("Data");
        tkDEFINEFONT = GetCommandValue("DefineFont");
        tkENDCSUB = GetCommandValue("End CSub");
        tkENDDEFINEFONT = GetCommandValue("End DefineFont");
        tkTILDE = GetTokenValue("~(");
        tkSCHANGE = GetTokenValue("SChange$(");
        tkTOPBOTTOM = GetTokenValue("TopBottom(");
    }
    routinechecks(1);
    strreplace_overflow = 0;
    // first, make sure that only printable characters are in the line (the substitutions mark text with bit 7)
    for(p = inpbuf; *p; p++) {
        *p = *p & 0x7f;
        if(*p < ' ' || *p == 0x7f)  *p = ' ';
    }
    // HELP topic at the prompt: the topic becomes a string, so that a keyword in it is not tokenised (HELP PRINT is
    // HELP "PRINT"; Gerry's b14 did this by re-running the line from cmd_help)
    if(console) {
        char *h = inpbuf;
        skipspace(h);
        if(strncasecmp(h, "HELP ", 5) == 0) {
            char *q = h + 5;
            int n;
            char *c;
            skipspace(q);
            if(*q != '"' && (c = strchr(q, '\'')) != NULL) *c = 0;      // a comment is not the topic (the PicoMite's #113)
            n = strlen(q);
            while(n && q[n - 1] == ' ') q[--n] = 0;
            if(n && *q != '"' && strlen(inpbuf) + 2 < STRINGSIZE) {
                memmove(q + 1, q, n + 1);
                q[0] = '"';
                strcat(q, "\"");
            }
        }
    }
    // then the substitutions, none of them in a string, a comment or a REM statement (hidden once for all)
    {
        char comment[STRINGSIZE];
        if(SubstHide(inpbuf, comment, true)) {
            // the functions and operators, in a DATA statement too: READ evaluates a number there (DATA MM.HRES/2)
            for(i = 0; i < MMEND; i++) {
                char buff[]="~( )";
                buff[2]=i+'A';
                str_replace(inpbuf,overlaid_functions[i],buff, false);
            }
            str_replace(inpbuf,"MM.INFO$","MM.INFO",0);
            str_replace(inpbuf,"TOUCH(","CLICK(",2);                // the PicoMite's name for CLICK( (its GUI controls')
            SubstFold(inpbuf);                                      // BIN$( LEFT$( MAX( and the rest, one token a family
            str_replace(inpbuf,"=>",">=", 3);
            str_replace(inpbuf,"=<","<=", 3);
            // the commands, which a word in a DATA statement is not (DATA spi, erase)
            SubstHideData(inpbuf);
            // (ERASE, I2C2, I2C3, SPI2, HUMID, DHT22, WS2812, BITSTREAM and ZMODEM were rewritten here into other commands while the
            // table was full; with two-byte command tokens they have their own entries, and LIST shows what was typed)
            SubstShow(inpbuf, comment);
        }
    }
    if(strreplace_overflow) goto toolong;                           // a substitution would not fit in the line

    // setup the input and output buffers
    p = inpbuf;
    op = tknbuf;
    if(!console) {                                                  // a program line starts with its header (T_NEWLINE_HDR)
        if(tknfile < 1 || tknfile > 250 || tknline < 0 || tknline > LHDR_MAXLINE) goto toolong;
        TKNPUT(T_NEWLINE);
        TKNPUT(LHDR_SKIPMAX + 1);                                   // skip: filled in when the line is complete
        TKNPUT(LHDR_MIN + tknfile - 1);
        TKNPUT(LHDR_MIN + tknline / 250);
        TKNPUT(LHDR_MIN + tknline % 250);
    }

    // get the line number if it exists
    tp = p;
    skipspace(tp);
    for(i = 0; i < 8; i++) if(!IsxDigit(tp[i])) break;              // test if this is eight hex digits
    if(IsDigitinline(*tp) && i < 8) {                                     // if it a digit and not an 8 digit hex number (ie, it is CFUNCTION data) then try for a line number
        i = strtol(tp, &tp, 10);
        if(!console && i > 0 && i <= MAXLINENBR) {
            TKNPUT(T_LINENBR);
            TKNPUT((i>>8));
            TKNPUT((i & 0xff));
        }
        p = tp;
    }
    char *body = op;                                                // the header and line number end here: nothing looks back past it

    // process the rest of the line
    firstnonwhite = true;
    labelvalid = true;
    while(*p) {

        // just copy a space
        if(*p == ' ') {
            TKNPUT(*p++);
            continue;
        }

        // first look for quoted text and copy it across
        // a string without its closing quote is closed here, as the PicoMite does (the processor was restarted)
        if(*p == '"') {
            do {
                TKNPUT(*p++);
            } while(*p != '"' && *p);
            TKNPUT('"');
            if(*p == '"') p++;
            continue;
        }

        // copy anything after a comment (')
        if(*p == '\'') {
            do {
                TKNPUT(*p++);
            } while(*p);
            continue;
        }

        // check for multiline separator (colon) and replace with a zero char
        if(*p == ':') {
            TKNPUT(0);
            p++;
            while(*p == ':') {                                      // insert a space between consecutive colons
                TKNPUT(' ');
                TKNPUT(0);
                p++;
            }
            firstnonwhite = true;
            symrawstmt = false;
            continue;
        }

        // a structure member after an array element, eg a(1).x: the dot is copied on its own, so that the member
        // is read as a name (as a symbol, and not as part of a number when it starts with E)
        if(*p == '.' && op > body && op[-1] == ')' && isnamestart(p[1])) {
            TKNPUT(*p++);
            firstnonwhite = false;
            continue;
        }

        // not white space or string or comment  - try a number
        if(IsDigitinline(*p) || *p == '.') {                              // valid chars at the start of a number
            while(IsDigitinline(*p) || *p == '.' || *p == 'E' || *p == 'e')
                if (*p == 'E' || *p == 'e') {   // check for '+' or '-' as part of the exponent
                    TKNPUT(*p++);                                   // copy the number
                    if (*p == '+' || *p == '-') {                   // BUGFIX by Gerard Sexton
                        TKNPUT(*p++);                               // copy the '+' or '-'
                    }
                } else {
                    TKNPUT(*p++);                                   // copy the number
                }
            firstnonwhite = false;
            continue;
        }

        // not white space or string or comment or number - see if we can find a label or a token identifier
        if(firstnonwhite) {                                         // first entry on the line must be a command
            // these variables are only used in the search for a command code
            char *tp2, *match_p = NULL, *cmdstart = p;
            int match_i = -1, match_l = 0;
            // first test if it is a print shortcut char (?) - this needs special treatment
            if(*p == '?') {
                match_i = GetCommandValue("Print");
                if(*++p == ' ') p++;                                // eat a trailing space
                match_p = p;
             // translate US spelling of COLOUR and therefor save a token slot
            } else if((tp2 = checkstring(p, "COLOR")) != NULL) {
                    match_i = GetCommandValue("Colour");
                    match_p = p = tp2;
            } else if((tp2 = checkstring(p, "WII")) != NULL) {
                    match_i = GetCommandValue("Controller");
                    match_p = p = tp2;
            } else if((tp2 = checkstring(p, "PIXEL FAST")) != NULL) {
                    match_i = GetCommandValue("CLS");
                    match_p = p = tp2;
             } else if((tp2 = checkstring(p, "ELSE IF")) != NULL) {
                    match_i = GetCommandValue("ElseIf");
                    match_p = p = tp2;
            } else if((tp2 = checkstring(p, "END IF")) != NULL) {
                    match_i = GetCommandValue("EndIf");
                    match_p = p = tp2;
            } else if((tp2 = checkstring(p, "EXIT DO")) != NULL) {
                    match_i = GetCommandValue("Exit");
                    match_p = p = tp2;
            } else if((tp2 = checkstring(p, "CAT")) != NULL) {
                    match_i = GetCommandValue("Inc");
                    match_p = p = tp2;
            } else {
                // now try for a command in the command table
                // this works by scanning the entire table looking for the match with the longest command name
                // this is needed because we need to differentiate between END and END SUB for example.
                // without looking for the longest match we might think that we have a match when we found just END.
                for(i = 0 ; i < CommandTableSize - 1; i++) {
                    tp2 = p;
                    tp = (char *)commandtbl[i].name;
                    while(toupper(*tp2) == toupper(*tp) && *tp != 0) {
                        if(*tp == ' ')
                            skipspace(tp2);                         // eat up any extra spaces between keywords
                        else
                            tp2++;
                        tp++;
                        if(*tp == '(') skipspace(tp2);              // eat up space between a keyword and bracket
                    }
                    // we have a match
                    if(*tp == 0 && (!isnamechar(*tp2) || (commandtbl[i].type & T_FUN))) {
                        if(*(tp - 1) != '(' && isnamechar(*tp2)) continue;   // skip if not the function
                        // save the details if it is the longest command found so far
                        if(strlen(commandtbl[i].name) > match_l) {
                            match_p = tp2;
                            match_l = strlen(commandtbl[i].name);
                            match_i = i;
                        }
                    }
                }
            }

            // a command's name used as a name is left as text: a member of a TYPE block (COLOUR AS INTEGER: only
            // DIM, LOCAL and STATIC have AS straight after them; FLAGS(1, 2) AS INTEGER, an array member, has its bounds
            // first), and a variable called TYPE (type = 3, type(1) = 2,
            // type$ = "a") as before structures came
            if(match_i > -1 && isnamestart(*cmdstart)) {
                char *q = match_p;
                if((match_i == cmdTYPE || match_i == cmdSTRUCT) && (*q == '$' || *q == '%' || *q == '!'))
                    match_i = -1;
                else {
                    skipspace(q);
                    if((match_i == cmdTYPE || match_i == cmdSTRUCT) && (*q == '=' || *q == '('))
                        match_i = -1;
                    else if(checkstring(q, "AS") != NULL && match_i != cmdLOCAL && match_i != cmdSTATIC && match_i != GetCommandValue("Dim"))
                        match_i = -1;
                    else if(*q == '(' && commandtbl[match_i].name[strlen((char *)commandtbl[match_i].name) - 1] != '(') {
                        int depth = 0;                              // a member array: FLAGS(1, 2) AS INTEGER, the bounds then AS
                        do {
                            if(*q == '(') depth++;
                            else if(*q == ')') depth--;
                            q++;
                        } while(*q && depth);
                        skipspace(q);
                        if(depth == 0 && checkstring(q, "AS") != NULL) match_i = -1;
                    }
                }
                if(match_i == -1) p = cmdstart;
            }

            if(match_i > -1) {
                // we have found a command
                TKNPUTCMD(match_i);                                 // insert the token found
                p = match_p;                                        // step over the command in the source
                if(IsAlpha(*(p-1)) && *p == ' ') p++;               // if the command is followed by a space skip over it
                if(match_i == GetCommandValue("Rem"))               // check if it is a REM command
                    while(*p) TKNPUT(*p++);                         // and in that case just copy everything
                firstnonwhite = false;
                labelvalid = false;                                 // we do not want any labels after this
                if(symbols) {
                    if(match_i == tkDATA)
                        symrawstmt = true;
                    else if(match_i == cmdCSUB || match_i == tkDEFINEFONT)
                        SymRawBlock = true;                         // the declaration and the hex that follows stay text
                    else if(match_i == tkENDCSUB || match_i == tkENDDEFINEFONT)
                        SymRawBlock = false;
                }
                continue;
            }

            // next test if it is a label
            if(labelvalid && isnamestart(*p)) {
                for(i = 0, tp = p + 1; i < MAXVARLEN - 1; i++, tp++)
                    if(!isnamechar(*tp)) break;                     // search for the first invalid char
                if(*tp == ':') {                                    // Yes !!  It is a label
                    labelvalid = false;                             // we do not want any more labels
                    TKNPUT(T_LABEL);                                // insert the token
                    TKNPUT(tp - p);                                 // insert the length of the label
                    for(i = tp - p; i > 0; i--) TKNPUT(*p++);       // copy the label
                    p++;                                            // step over the terminating colon
                    continue;
                }
            }
        } else {
            // check to see if it is a function or keyword
            char *tp2 = NULL;
            for(i = 0 ; i < TokenTableSize - 1; i++) {
                tp2 = p;
                tp = (char *)tokentbl[i].name;
                // check this entry
                while(toupper(*tp2) == toupper(*tp) && *tp != 0) {
                    tp++; tp2++;
                    if(*tp == '(') skipspace(tp2);
                }
                if(*tp == 0 && (!isnameend(*(tp - 1)) || !isnamechar(*tp2))) break;
            }
            if(i != TokenTableSize - 1) {
                // we have a  match
                i += C_BASETOKEN;
                TKNPUT(i);                                          // insert the token found
                p = tp2;                                            // and step over it in the source text
                symrawnext = (i == tkTILDE || i == tkSCHANGE || i == tkTOPBOTTOM);   // MM.HRES is ~(A), MAX( TOPBOTTOM(A,: the letter is read as a character
                if(i == tokenTHEN || i == tokenELSE)
                    firstnonwhite = true;                           // a command is valid after a THEN or ELSE
                else
                    firstnonwhite = false;
                continue;
            }
        }

        // not white space or string or comment or token identifier or number
        // try for a variable name which could be a user defined subroutine or an implied let
        if(isnamestart(*p)) {                                       // valid chars at the start of a variable name
            if(firstnonwhite) {                                     // first entry on the line?
                tp = skipvar(p, true);                              // find the char after the variable
                skipspace(tp);
                if(*tp == '=')                                      // is it an implied let?
                    TKNPUTCMD(GetCommandValue("Let"));              // find let's token value and copy into memory
                }
            if(symbols && !symrawstmt && !SymRawBlock && !symrawnext && !(op > body && op[-1] == '&')) {
                tp = p;
                while(isnamechar(*tp)) tp++;
                if(op + (tp - p > 3 ? tp - p : 3) > tknend) goto toolong;  // a symbol is at most 3 bytes, the text tp - p
                op = SymName(op, p, tp - p);                        // the name as a symbol (or text if it cannot be one)
                p = tp;
            } else
                while(isnamechar(*p)) TKNPUT(*p++);                 // copy the variable name
            symrawnext = false;
            firstnonwhite = false;
            labelvalid = false;                                     // we do not want any labels after this
            continue;
        }

        // special case where the character to copy is an opening parenthesis
        // we search back to see if the previous non space char was the end of an identifier and, if it is, we remove any spaces following the identifier
        // this enables the programmer to put spaces after a function name or array identifier without causing a confusing error
        if(*p == '(') {
            tp = op - 1;
            if(tp >= body && *tp == ' ') {
                while(tp >= body && *tp == ' ') tp--;
                if(tp >= body && isnameend(*tp)) op = tp + 1;
            }
        }

        // something else, so just copy the one character
        TKNPUT(*p++);
       labelvalid = false;                                          // we do not want any labels after this
       firstnonwhite = false;

    }
    // end of loop, trim any trailing blanks (but not part of the header or a line number)
    while(op > body && *(op - 1) == ' ') *--op = 0;               // (OPTION PROFILING's padded lines needed them kept)
    // make sure that it is terminated properly
    *op++ = 0;  *op++ = 0;  *op++ = 0;                              // terminate with  zero chars
    if(!console) {                                                  // skip: the line's length as the image will hold it -
        int k = 0;                                                  // the writer copies up to the first pair of zeros and adds one
        while(!(tknbuf[k] == 0 && tknbuf[k + 1] == 0)) k++;
        tknbuf[1] = (k + 1 > LHDR_SKIPMAX) ? LHDR_SKIPMAX + 1 : k + 1;
    }
    return 0;

    // the line will not fit: name it by its file and line
toolong:
    if(!console && tknfilename)
        snprintf(tknerrmsg, 80, "Line %d of %.40s is too long", tknline, tknfilename);
    else if(!console)
        snprintf(tknerrmsg, 80, "Line %d is too long", tknline);
    else
        strcpy(tknerrmsg, "Line is too long");
    tknbuf[0] = tknbuf[1] = tknbuf[2] = 0;
    if(console) error("$", tknerrmsg);
    return 1;                                                       // a program being saved: the writer closes its image first
}
#undef TKNPUT
#undef TKNPUTCMD




/********************************************************************************************************************************************
 routines for evaluating expressions
 the main functions are getnumber(), getinteger() and getstring()
********************************************************************************************************************************************/



// A convenient way of evaluating an expression
// it takes two arguments:
//     p = pointer to the expression in memory (leading spaces will be skipped)
//     t = pointer to the type
//         if *t = T_STR or T_NBR or T_INT will throw an error if the result is not the correct type
//         if *t = T_NOTYPE it will not throw an error and will return the type found in *t
// it returns with a void pointer to a float, integer or string depending on the value returned in *t
// this will check that the expression is terminated correctly and throw an error if not
void *DoExpression(char *p, int *t) {
    static MMFLOAT f;
    static long long int i64;
    static char *s;

    evaluate(p, &f, &i64, &s, t, false);
    if(*t & T_INT) return &i64;
    if(*t & T_NBR) return &f;
    if(*t & T_STR) return s;

    error("Internal fault 4(sorry)");
    return NULL;                                                    // to keep the compiler happy
}



// evaluate an expression.  p points to the start of the expression in memory
// returns either the float or string in the pointer arguments
// *t points to an integer which holds the type of variable we are looking for
//  if *t = T_STR or T_NBR or T_INT will throw an error if the result is not the correct type
//  if *t = T_NOTYPE it will not throw an error and will return the type found in *t
// this will check that the expression is terminated correctly and throw an error if not.  flags & E_NOERROR will suppress that check
char ITCM_CODE *evaluate(char *p, MMFLOAT *fa, long long int *ia, char **sa, int *ta, int flags) {
    int o;
    int t = *ta;
    char *s;

    p = getvalue(p, fa, ia, &s, &o, &t);                            // get the left hand side of the expression, the operator is returned in o
    while(o != E_END) p = doexpr(p, fa, ia, &s, &o, &t);            // get the right hand side of the expression and evaluate the operator in o

    // check that the types match and convert them if we can
    if((*ta & (T_NBR |T_INT)) && t & T_STR) error("Expected a number");
    if(*ta & T_STR && (t & (T_NBR | T_INT))) error("Expected a string");
    if(o != E_END) error("Argument count");
    if((*ta & T_NBR) && (t & T_INT)) *fa = *ia;
    if((*ta & T_INT) && (t & T_NBR)) *ia = FloatToInt64(*fa);
    *ta = t;
    *sa = s;

    // check that the expression is terminated correctly
    if(!(flags & E_NOERROR)) {
        skipspace(p);
        if(!(*p == 0 || *p == ',' || *p == ')' || *p == '\''))  error("Expression syntax");
    }
    return p;
}


// evaluate an expression to get a number
MMFLOAT getnumber(char *p) {
    int t = T_NBR;
    MMFLOAT f;
    long long int i64;
    char *s;
    evaluate(p, &f, &i64, &s, &t, false);
    if(t & T_INT) return (MMFLOAT)i64;
    return f;
}


// evaluate an expression and return a 64 bit integer
long long int getinteger(char *p) {
    int t = T_INT;
    MMFLOAT f;
    long long int i64;
    char *s;
    evaluate(p, &f, &i64, &s, &t, false);
    if(t & T_NBR) return FloatToInt64(f);
    return i64;
}


/*
// evaluate an expression and return an integer
// this will throw an error is the integer is outside a specified range
// this will correctly round the number if it is a fraction of an integer
int getint(char *p, int min, int max) {
    long long int i;
    int t = T_INT;
    MMFLOAT f;
    long long int i64;
    char *s;
    evaluate(p, &f, &i64, &s, &t, false);
    if(t & T_NBR) i= FloatToInt64(f);
    else i=i64;
    if(i < min || i > max) error("% is invalid (valid is % to %)", (int)i, min, max);
    return i;
}
*/

// evaluate an expression and return an integer
// this will throw an error is the integer is outside a specified range
// this will correctly round the number if it is a fraction of an integer

// Updated to use ~ in error messages.
long long int getint(char *p, long long int min, long long int max) {
    long long int  i;
    int t = T_INT;
    MMFLOAT f;
    long long int i64;
    char *s;
    evaluate(p, &f, &i64, &s, &t, false);
    if(t & T_NBR) i= FloatToInt64(f);
    else i=i64;
    if(i < min || i > max) error("~ is invalid (valid is ~ to ~)", i, min, max);
    return i;
}



// evaluate an expression to get a string
char *getstring(char *p) {
    int t = T_STR;
    MMFLOAT f;
    long long int i64;
    char *s;

    evaluate(p, &f, &i64, &s, &t, false);
    return s;
}



// evaluate an expression to get a string using the C style for a string
// as against the MMBasic style returned by getstring()
// Program memory holds an image of the current format: its first line is the program's name line, whose
// header carries file 1 and line 0, and whose text is ' IMAGE_TAG IMAGE_FORMAT name (see T_NEWLINE_HDR).
// An image written by an older firmware has another format digit, or the name straight after the ' (the
// header of 15 Oct), or the name's ' straight after its T_NEWLINE (before that).
int ImageIsCurrent(void) {
    return ImageIsCurrentAt(ProgMemory);
}

// the same for the image at p (a flash slot's: ImageSlots.c)
int ImageIsCurrentAt(const char *image) {
    const unsigned char *p = (const unsigned char *)image;
    return p[0] == T_NEWLINE && p[2] == LHDR_MIN && p[3] == LHDR_MIN && p[4] == LHDR_MIN && p[5] == '\''
        && p[6] == IMAGE_TAG && p[7] == IMAGE_FORMAT;
}

// the program's file name as the image records it (for example "/DIR/PROG.BAS"), or NULL
char *ProgName(void) {
    return ImageIsCurrent() ? (char *)ProgMemory + T_NEWLINE_HDR + IMAGE_NAME : NULL;
}

// the line after the one starting at 'line' (its T_NEWLINE), or the zero that ends the program: by the
// line's skip byte, or for a longer line by walking its elements (a T_LINENBR's bytes can be zero)
char *NextLine(char *line) {
    char *p;
    if(LHDR_SKIP(line) <= LHDR_SKIPMAX) return line + LHDR_SKIP(line);
    p = line + T_NEWLINE_HDR;
    if(*p == T_LINENBR) p += 3;
    while(1) {
        while(*p) p++;                                              // the zero that ends this element
        if(p[1] == T_NEWLINE || p[1] == 0) return p + 1;
        p++;
    }
}

// the name of source file 'file' (1 = the program, then its #INCLUDE files), from the name lines at the
// top of the image; "?" if it is not there
char *LineFileName(int file) {
    char *p = (char *)ProgMemory;
    if(!ImageIsCurrent()) return "?";
    while(*p == T_NEWLINE && LHDR_LINE(p) == 0) {                   // the name lines come first
        if(LHDR_FILE(p) == file) return p + T_NEWLINE_HDR + IMAGE_NAME;
        if(LHDR_SKIP(p) > LHDR_SKIPMAX) break;                      // (a name line is never that long)
        p += LHDR_SKIP(p);
    }
    return "?";
}

char *getCstring(char *p) {
    char *tp;
    tp = GetTempStrMemory();                                        // this will last for the life of the command
    Mstrcpy(tp, getstring(p));                                      // get the string and save in a temp place
    MtoC(tp);                                                       // convert to a C style string
    return tp;
}

char *getFstring(char *p) {
    char *tp;
    tp = GetTempStrMemory();                                        // this will last for the life of the command
    for(int i=1;i<=*p;i++)if(p[i]=='\\')p[i]='/';
    Mstrcpy(tp, getstring(p));                                      // get the string and save in a temp place
    MtoC(tp);                                                       // convert to a C style string
    return tp;
}

// recursively evaluate an expression observing the rules of operator precedence
inline char ITCM_CODE *doexpr(char *p, MMFLOAT *fa, long long int *ia, char **sa, int *oo, int *ta) {
    MMFLOAT fa1, fa2;
    long long int ia1, ia2;
    int o1, o2;
    int t1, t2;
    char *sa1, *sa2;
	TestStackOverflow();                                            // (the PicoMite's: MMBasic.h)
    fa1 = *fa;
    ia1 = *ia;
    sa1 = *sa;
    t1 = TypeMask(*ta);
    o1 = *oo;
    p = getvalue(p, &fa2, &ia2, &sa2, &o2, &t2);
    while(1) {
        if(o2 == E_END || tokentbl[o1].precedence <= tokentbl[o2].precedence) {
            if((t1 & T_STR) != (t2 & T_STR)) error("Incompatible types in expression");
            targ = tokentbl[o1].type & (T_NBR | T_INT);
            if(targ == T_NBR) {                                     // if the operator does not work with ints convert the args to floats
                if(t1 & T_INT) { fa1 = ia1; t1 = T_NBR; }           // at this time the only example of this is op_div (/)
                if(t2 & T_INT) { fa2 = ia2; t2 = T_NBR; }
            }
            if(targ == T_INT) {                                     // if the operator does not work with floats convert the args to ints
                if(t1 & T_NBR) { ia1 = FloatToInt64(fa1); t1 = T_INT; }
                if(t2 & T_NBR) { ia2 = FloatToInt64(fa2); t2 = T_INT; }
            }
            if(targ == (T_NBR | T_INT)) {                               // if the operator will work with both floats and ints
                if(t1 & T_NBR && t2 & T_INT) { fa2 = ia2; t2 = T_NBR; } // if one arg is float convert the other to a float
                if(t1 & T_INT && t2 & T_NBR) { fa1 = ia1; t1 = T_NBR; }
            }
            if(!(tokentbl[o1].type & T_OPER) || !(tokentbl[o1].type & t1)) {
                error("Invalid operator");
            }
            farg1 = fa1; farg2 = fa2;                               // setup the float args (incase it is a float)
            sarg1 = sa1; sarg2 = sa2;                               // ditto string args
            iarg1 = ia1; iarg2 = ia2;                               // ditto integer args
            targ = t1;                                              // this is what both args are
            tokentbl[o1].fptr();                                    // call the operator function
            *fa = fret;
            *ia = iret;
            *sa = sret;
            *oo = o2;
            *ta = targ;
            return p;
        }
        // the next operator has a higher precedence, recursive call to evaluate it
        else
            p = doexpr(p, &fa2, &ia2, &sa2, &o2, &t2);
    }
}


// the powers after a unary minus's value, applied before the minus (getvalue): out of line and out of ITCM, as it is
// rare. getvalue's speed depends on where its hot paths fall in ITCM: written inline (68 bytes), or with the test as one
// compare (56), they moved and cost every expression ~1%; this way (64) none, measured on the G1.
static char * __attribute__((noinline)) NegPowers(char *p, MMFLOAT *f, long long int *i64, char **s, int *ro, int *t) {
    while(*ro != E_END && tokentbl[*ro].fptr == op_exp) p = doexpr(p, f, i64, s, ro, t);
    return p;
}

// get a value, either from a constant, function or variable
// also returns the next operator to the right of the value or E_END if no operator
// Added OptionEscape
//#ifdef NEW
inline char ITCM_CODE *getvalue(char *p, MMFLOAT *fa, long long int *ia, char **sa, int *oo, int *ta)  {

    MMFLOAT f = 0;
    long long int i64 = 0;
    char *s = NULL;
    int t = T_NOTYPE;
    char *tp, *p1, *p2;
    int i;
	TestStackOverflow();                                            // (the PicoMite's: MMBasic.h)
     skipspace(p);
    if(*p>=C_BASETOKEN){ //don't waste time if not a built-in function
		// special processing for the NOT operator
		// just get the next value and invert its logical value
		if(tokenfunction(*p) == op_not) {
			int ro;
			p++; t = T_NOTYPE;
			p = getvalue(p, &f, &i64, &s, &ro, &t);                     // get the next value
			if(t &T_NBR)
				f = (MMFLOAT)((f != 0)?0:1);                              // invert the value returned
			else if(t & T_INT)
				i64 = ((i64 != 0)?0:1);
		   else
				error("Expected a number");
			skipspace(p);
			*fa = f;                                                    // save what we have
			*ia = i64;
			*sa = s;
			*ta = t;
			*oo = ro;
			return p;                                                   // return straight away as we already have the next operator
		}
		if(tokenfunction(*p) == op_inv) {
			int ro;
			p++; t = T_NOTYPE;
			p = getvalue(p, &f, &i64, &s, &ro, &t);                     // get the next value
			if(t & T_NBR)
				i64 = FloatToInt64(f);
			else if(!(t & T_INT))
				error("Expected a number");
			i64 = ~i64;
			t = T_INT;
			skipspace(p);
			*fa = f;                                                    // save what we have
			*ia = i64;
			*sa = s;
			*ta = t;
			*oo = ro;
			return p;                                                   // return straight away as we already have the next operator
		}

		// special processing for the unary - operator
		// get the next value, with any powers after it, and negate it: -2^2 is -(2^2), -4, as in maths and most languages
		// (it was (-2)^2, as in Excel and BBC BASIC). The powers are applied left to right, as doexpr applies them.
		if(tokenfunction(*p) == op_subtract) {
			int ro;
			p++; t = T_NOTYPE;
			p = getvalue(p, &f, &i64, &s, &ro, &t);                     // get the next value
			if(ro != E_END && tokentbl[ro].fptr == op_exp) p = NegPowers(p, &f, &i64, &s, &ro, &t);
			if(t & T_NBR)
				f = -f;                                                 // negate the MMFLOAT returned
			else if(t & T_INT)
				i64 = -i64;                                             // negate the integer returned
		   else
				error("Expected a number");
			skipspace(p);
			*fa = f;                                                    // save what we have
			*ia = i64;
			*sa = s;
			*ta = t;
			*oo = ro;
			return p;                                                   // return straight away as we already have the next operator
		}
		if(tokenfunction(*p) == op_add) {
			int ro;
			p++; t = T_NOTYPE;
			p = getvalue(p, &f, &i64, &s, &ro, &t);                     // get the next value
			skipspace(p);
			*fa = f;                                                    // save what we have
			*ia = i64;
			*sa = s;
			*ta = t;
			*oo = ro;
			return p;                                                   // return straight away as we already have the next operator
		}


		// if a function execute it and save the result
		if(tokentype(*p) & (T_FUN | T_FNA)) {
			int tmp;
			tp = p;
			// if it is a function with arguments we need to locate the closing bracket and copy the argument to
			// a temporary variable so that functions like getarg() will work.
			if(tokentype(*p) & T_FUN) {
				p1 = p + 1;
				p = getclosebracket(p);                                 // find the closing bracket
				p2 = ep = GetTempStrMemory();                           // this will last for the life of the command
				if(!SymAwareFunction(*tp) && SymAny(p1, p - p1)) {      // its arguments with the symbols spelt out (Symbols.c)
					int n = SymExpand(ep, p1, p - p1, STRINGSIZE - 1);
					if(n < 0) error("Line is too long");
					ep[n] = 0;
				} else
					while(p1 != p) *p2++ = *p1++;
			} else ep=NULL;
			p++;                                                        // point to after the function (without argument) or after the closing bracket
			tmp = targ = TypeMask(tokentype(*tp));                      // set the type of the function (which might need to know this)
			tokenfunction(*tp)();                                       // execute the function
			if((tmp & targ) == 0) error("Internal fault 3(sorry)");      // as a safety check the function must return a type the same as set in the header
			t = targ;                                                   // save the type of the function
			f = fret; i64 = iret; s = sret;                             // save the result
		}
    } else {
		// if it is a variable or a defined function, find it and get its value
		if(isnamestartsym(*p)) {
			// first check if it is terminated with a bracket
			if(issymbol(*p))
				tp = p + symbolsize(*p);                                // a symbol (Symbols.h)
			else {
				tp = p + 1;
				while(isnamechar(*tp)) tp++;                            // search for the end of the identifier
			}
			if(*tp == '$' || *tp == '%' || *tp == '!') tp++;
			i = -1;
			if(*tp == '(') i = FindSubFun(p, 1);                        // if terminated with a bracket it could be a function
			if(i >= 0) {                                                // >= 0 means it is a user defined function
				char *SaveCurrentLinePtr = CurrentLinePtr;              // in case the code in DefinedSubFun messes with this
				DefinedSubFun(true, p, i, &f, &i64, &s, &t);
				CurrentLinePtr = SaveCurrentLinePtr;
			} else {
				s = (char *)findvar(p, V_FIND);                         // if it is a string then the string pointer is automatically set
				tp = FindvarEnd;                                        // (where it ended, if skipvar would say the same)
				g_ExprStructType = -1;
				if(g_StructMemberType != 0) {                           // a structure's member
					t = TypeMask(g_StructMemberType);
					if(t & T_STRUCT) g_ExprStructType = g_StructMemberSize;    // a nested structure: its own type
				} else {
					t = TypeMask(VREC(VarIndex)->type);
					if(t & T_STRUCT) g_ExprStructType = VREC(VarIndex)->size;  // a whole structure (or element): its type
				}
				if(t & T_NBR) f = (*(MMFLOAT *)s);
				if(t & T_INT) i64 = (*(long long int *)s);
#ifdef FINDVAREND_CHECK
				if(tp != NULL && tp != skipvar(p, false)) error("FindvarEnd differs from skipvar");
#endif
				if(tp != NULL) { p = tp; goto getvalue_varend; }    // (skipvar would walk the name and indices again)
			}
			p = skipvar(p, false);
getvalue_varend:;
		}
		// is it an ordinary numeric constant?  get its value if yes
		// a leading + or - might have been converted to a token so we need to check for them also
		else if(IsDigitinline(*p) || *p == '.') {
			char ts[31], *tsp;
			int isi64 = true;
			tsp = ts;
			int isf=true;
			long long int scale=0;
			// copy the first digit of the string to a temporary place
			if(*p == '.') {
				isi64 = false;
				scale=1;
			} else if(IsDigitinline(*p)){
				i64=(*p - '0');
			}
			*tsp++ = *p++;

			// now concatenate the remaining digits
			while((digit[(uint8_t)*p]) && (tsp - ts) < 30) {
				if(*p >= '0' && *p <= '9'){
					i64 = i64 * 10 + (*p - '0');
					if(scale)scale*=10;
				} else {
					if((*p) == '.'){
						isi64 = false;
						scale =1;
					} else {
						if(toupper(*p) == 'E' || *p == '-' || *p == '+' ){
							isi64 = false;
							isf=false;
						}
					}
				}
				*tsp++ = *p++;                                          // copy the string to a temporary place
			}
			*tsp = 0;                                                   // terminate it
			if(isi64) {
				t = T_INT;
			} else if(isf && (tsp - ts) < 18) {
				f=(MMFLOAT)i64/(MMFLOAT)scale;
				t = T_NBR;
			} else {
				f = (MMFLOAT)strtod(ts, &tsp);                          // and convert to a MMFLOAT
				t = T_NBR;
			}
		}

		// if it is a numeric constant starting with the & character then get its base and convert to an integer
		else if(*p == '&') {
			p++; i64 = 0;
			switch(toupper(*p++)) {                                 // &h as well as &H: DATA keeps its case
				case 'H':   while(IsxDigit(*p)) {
								i64 = (i64 << 4) | ((toupper(*p) >= 'A') ? toupper(*p) - 'A' + 10 : *p - '0');
								p++;
							} break;
				case 'O':   while(*p >= '0' && *p <= '7') {
								i64 = (i64 << 3) | (*p++ - '0');
							} break;
				case 'B':   while(*p == '0' || *p == '1') {
								i64 = (i64 << 1) | (*p++ - '0');
							} break;
				default:    error("Type prefix");
			}
			t = T_INT;
		}
		// if opening bracket then first evaluate the contents of the bracket
    	else if(*p == '(') {
			p++;                                                        // step over the bracket
			p = evaluate(p, &f, &i64, &s, &t, true);                    // recursively get the contents
			if(*p != ')') error("No closing bracket");
			++p;                                                        // step over the closing bracket
		}
		// if it is a string constant, return a pointer to that.  Note: tokenise() guarantees that strings end with a quote
		else if(*p == '"') {
			p++;                                                        // step over the quote
			p1 = s = GetTempMemory(STRINGSIZE);                                // this will last for the life of the command
			tp = strchr(p, '"');
                int toggle=0;
                while(p != tp){
                    if(*p=='\\' && tp>p+1 && OptionEscape)toggle^=1;
                    if(toggle){
                        if(*p=='\\' && IsDigit(p[1]) && IsDigit(p[2]) && IsDigit(p[3])){
                            p++;
                            i=(*p++)-48;
                            i*=10;
                            i+=(*p++)-48;
                            i*=10;
                            i+=(*p++)-48;
                            if(i==0)error("Illegal escape sequence, use CHR$(0) for the Null character","$");
                            *p1++=i;
                        } else {
                            p++;
                            switch(*p){
                                case '\\':
                                    *p1++='\\';
                                    p++;
                                    break;
                                case 'a':
                                    *p1++='\a';
                                    p++;
                                    break;
                                case 'b':
                                    *p1++='\b';
                                    p++;
                                    break;
                                case 'e':
                                    *p1++='\e';
                                    p++;
                                    break;
                                case 'f':
                                    *p1++='\f';
                                    p++;
                                    break;
                                case 'n':
                                    *p1++='\n';
                                    p++;
                                    break;
                                case 'q':
                                    *p1++='\"';
                                    p++;
                                    break;
                                case 'r':
                                    *p1++='\r';
                                    p++;
                                    break;
                                case 't':
                                    *p1++='\t';
                                    p++;
                                    break;
                                case 'v':
                                    *p1++='\v';
                                    p++;
                                    break;
                                case '&':
                                    p++;
                                    if(IsxDigit(*p) && IsxDigit(p[1])){
                                        i=0;
                                        i = (i << 4) | ((toupper(*p) >= 'A') ? toupper(*p) - 'A' + 10 : *p - '0');
                                        p++;
                                        i = (i << 4) | ((toupper(*p) >= 'A') ? toupper(*p) - 'A' + 10 : *p - '0');
                                        p++;
                                         if(i==0)error("Illegal escape sequence, use CHR$(0) for the Null character","$");
                                         *p1++=i;
                                    } else *p1++='&';             // (without two hex digits: the & itself, as on the PicoMite; it was 'x')
                                    break;
                                default:
                                    *p1++=*p++;
                            }
                        }
                        toggle=0;
                    } else *p1++ = *p++;
                }
            p++;
            CtoM(s);                                                    // convert to a MMBasic string
			t = T_STR;
		}
		else if(*p == T_VALUE)
			p = RBSpliceValue(p, &f, &i64, &s, &t);                    // a value the compiler's code spliced in (Stream.h)
		else
			error("Syntax");
    }
	skipspace(p);
	*fa = f;                                                        // save what we have
	*ia = i64;
	*sa = s;
	*ta = t;

	// get the next operator, if there is not an operator set the operator to end of expression (E_END)
	if(tokentype(*p) & T_OPER)
		*oo = *p++ - C_BASETOKEN;
	else
		*oo = E_END;

	return p;
}
//#endif

#ifdef OLD
inline char ITCM_CODE *getvalue(char *p, MMFLOAT *fa, long long int *ia, char **sa, int *oo, int *ta) {
    MMFLOAT f = 0;
    long long int i64 = 0;
    char *s = NULL;
    int t = T_NOTYPE;
    char *tp, *p1, *p2;
    int i;
	TestStackOverflow();                                            // (the PicoMite's: MMBasic.h)
     skipspace(p);
    if(*p>=C_BASETOKEN){ //don't waste time if not a built-in function
		// special processing for the NOT operator
		// just get the next value and invert its logical value
		if(tokenfunction(*p) == op_not) {
			int ro;
			p++; t = T_NOTYPE;
			p = getvalue(p, &f, &i64, &s, &ro, &t);                     // get the next value
			if(t &T_NBR)
				f = (MMFLOAT)((f != 0)?0:1);                              // invert the value returned
			else if(t & T_INT)
				i64 = ((i64 != 0)?0:1);
		   else
				error("Expected a number");
			skipspace(p);
			*fa = f;                                                    // save what we have
			*ia = i64;
			*sa = s;
			*ta = t;
			*oo = ro;
			return p;                                                   // return straight away as we already have the next operator
		}
		if(tokenfunction(*p) == op_inv) {
			int ro;
			p++; t = T_NOTYPE;
			p = getvalue(p, &f, &i64, &s, &ro, &t);                     // get the next value
			if(t & T_NBR)
				i64 = FloatToInt64(f);
			else if(!(t & T_INT))
				error("Expected a number");
			i64 = ~i64;
			t = T_INT;
			skipspace(p);
			*fa = f;                                                    // save what we have
			*ia = i64;
			*sa = s;
			*ta = t;
			*oo = ro;
			return p;                                                   // return straight away as we already have the next operator
		}

		// special processing for the unary - operator
		// just get the next value and negate it
		if(tokenfunction(*p) == op_subtract) {
			int ro;
			p++; t = T_NOTYPE;
			p = getvalue(p, &f, &i64, &s, &ro, &t);                     // get the next value
			if(t & T_NBR)
				f = -f;                                                 // negate the MMFLOAT returned
			else if(t & T_INT)
				i64 = -i64;                                             // negate the integer returned
		   else
				error("Expected a number");
			skipspace(p);
			*fa = f;                                                    // save what we have
			*ia = i64;
			*sa = s;
			*ta = t;
			*oo = ro;
			return p;                                                   // return straight away as we already have the next operator
		}
		if(tokenfunction(*p) == op_add) {
			int ro;
			p++; t = T_NOTYPE;
			p = getvalue(p, &f, &i64, &s, &ro, &t);                     // get the next value
			skipspace(p);
			*fa = f;                                                    // save what we have
			*ia = i64;
			*sa = s;
			*ta = t;
			*oo = ro;
			return p;                                                   // return straight away as we already have the next operator
		}


		// if a function execute it and save the result
		if(tokentype(*p) & (T_FUN | T_FNA)) {
			int tmp;
			tp = p;
			// if it is a function with arguments we need to locate the closing bracket and copy the argument to
			// a temporary variable so that functions like getarg() will work.
			if(tokentype(*p) & T_FUN) {
				p1 = p + 1;
				p = getclosebracket(p);                                 // find the closing bracket
				p2 = ep = GetTempStrMemory();                           // this will last for the life of the command
				while(p1 != p) *p2++ = *p1++;
			} else ep=NULL;
			p++;                                                        // point to after the function (without argument) or after the closing bracket
			tmp = targ = TypeMask(tokentype(*tp));                      // set the type of the function (which might need to know this)
			tokenfunction(*tp)();                                       // execute the function
			if((tmp & targ) == 0) error("Internal fault 3(sorry)");      // as a safety check the function must return a type the same as set in the header
			t = targ;                                                   // save the type of the function
			f = fret; i64 = iret; s = sret;                             // save the result
		}
    } else {
		// if it is a variable or a defined function, find it and get its value
		if(isnamestart(*p)) {
			// first check if it is terminated with a bracket
			tp = p + 1;
			while(isnamechar(*tp)) tp++;                                // search for the end of the identifier
			if(*tp == '$' || *tp == '%' || *tp == '!') tp++;
			i = -1;
			if(*tp == '(') i = FindSubFun(p, 1);                        // if terminated with a bracket it could be a function
			if(i >= 0) {                                                // >= 0 means it is a user defined function
				char *SaveCurrentLinePtr = CurrentLinePtr;              // in case the code in DefinedSubFun messes with this
				DefinedSubFun(true, p, i, &f, &i64, &s, &t);
				CurrentLinePtr = SaveCurrentLinePtr;
			} else {
				s = (char *)findvar(p, V_FIND);                         // if it is a string then the string pointer is automatically set
				t = TypeMask(VREC(VarIndex)->type);
				if(t & T_NBR) f = (*(MMFLOAT *)s);
				if(t & T_INT) i64 = (*(long long int *)s);
#ifdef FINDVAREND_CHECK
				if(tp != NULL && tp != skipvar(p, false)) error("FindvarEnd differs from skipvar");
#endif
				if(tp != NULL) { p = tp; goto getvalue_varend; }    // (skipvar would walk the name and indices again)
			}
			p = skipvar(p, false);
getvalue_varend:;
		}
		// is it an ordinary numeric constant?  get its value if yes
		// a leading + or - might have been converted to a token so we need to check for them also
		else if(IsDigitinline(*p) || *p == '.') {
			char ts[31], *tsp;
			int isi64 = true;
			tsp = ts;
			int isf=true;
			long long int scale=0;
			// copy the first digit of the string to a temporary place
			if(*p == '.') {
				isi64 = false;
				scale=1;
			} else if(IsDigitinline(*p)){
				i64=(*p - '0');
			}
			*tsp++ = *p++;

			// now concatenate the remaining digits
			while((digit[(uint8_t)*p]) && (tsp - ts) < 30) {
				if(*p >= '0' && *p <= '9'){
					i64 = i64 * 10 + (*p - '0');
					if(scale)scale*=10;
				} else {
					if((*p) == '.'){
						isi64 = false;
						scale =1;
					} else {
						if(toupper(*p) == 'E' || *p == '-' || *p == '+' ){
							isi64 = false;
							isf=false;
						}
					}
				}
				*tsp++ = *p++;                                          // copy the string to a temporary place
			}
			*tsp = 0;                                                   // terminate it
			if(isi64) {
				t = T_INT;
			} else if(isf && (tsp - ts) < 18) {
				f=(MMFLOAT)i64/(MMFLOAT)scale;
				t = T_NBR;
			} else {
				f = (MMFLOAT)strtod(ts, &tsp);                          // and convert to a MMFLOAT
				t = T_NBR;
			}
		}

		// if it is a numeric constant starting with the & character then get its base and convert to an integer
		else if(*p == '&') {
			p++; i64 = 0;
			switch((*p++)) {
				case 'H':   while(IsxDigit(*p)) {
								i64 = (i64 << 4) | (((*p) >= 'A') ? (*p) - 'A' + 10 : *p - '0');
								p++;
							} break;
				case 'O':   while(*p >= '0' && *p <= '7') {
								i64 = (i64 << 3) | (*p++ - '0');
							} break;
				case 'B':   while(*p == '0' || *p == '1') {
								i64 = (i64 << 1) | (*p++ - '0');
							} break;
				default:    error("Type prefix");
			}
			t = T_INT;
		}
		// if opening bracket then first evaluate the contents of the bracket
    	else if(*p == '(') {
			p++;                                                        // step over the bracket
			p = evaluate(p, &f, &i64, &s, &t, true);                    // recursively get the contents
			if(*p != ')') error("No closing bracket");
			++p;                                                        // step over the closing bracket
		}
		// if it is a string constant, return a pointer to that.  Note: tokenise() guarantees that strings end with a quote
		else if(*p == '"') {
			p++;                                                        // step over the quote
			p1 = s = GetTempMemory(STRINGSIZE);                                // this will last for the life of the command
			tp = strchr(p, '"');
            int toggle=0;
			while(p != tp){
                if(*p=='|')toggle^=1;
                if(*p=='|' && isdigit((unsigned char)p[1])){
                    if(toggle){
                    p++;
                    i=(*p++)-48;
                    while(isdigit((unsigned char)*p)){
                        i*=10;
                        i+=(*p++)-48;
                    }
                    *p1++=i;
                    if(*p=='|')p++;
                    } else {
                        p++;
                    }
                    toggle=0;
                } else *p1++ = *p++;
            }
			p++;
			CtoM(s);                                                    // convert to a MMBasic string
			t = T_STR;
		}
		else
			error("Syntax");
    }
	skipspace(p);
	*fa = f;                                                        // save what we have
	*ia = i64;
	*sa = s;
	*ta = t;

	// get the next operator, if there is not an operator set the operator to end of expression (E_END)
	if(tokentype(*p) & T_OPER)
		*oo = *p++ - C_BASETOKEN;
	else
		*oo = E_END;

	return p;
}

#endif



// search through program memory looking for a line number. Stops when it has a matching or larger number
// returns a pointer to the T_NEWLINE token or a pointer to the two zero characters representing the end of the program
char *findline(int nbr, int mustfind) {
    char *p;
    int i;

    p = (char *)ProgMemory;
    while(1) {
        if(p[0] == 0 && p[1] == 0) {
            i = MAXLINENBR;
            break;
        }

        if(p[0] == T_NEWLINE) {
            p += T_NEWLINE_HDR;                                     // step over the line's header
            continue;
        }

        if(p[0] == T_LINENBR) {
            i = (p[1] << 8) | p[2];
            if(mustfind) {
                if(i == nbr) break;
            } else {
                if(i >= nbr) break;
            }
            p += 3;
            continue;
        }

        if(p[0] == T_LABEL) {
            p += p[1] + 2;
            continue;
        }

        p++;
    }
    if(mustfind && i != nbr)
        error("Line number");
    return p;
}
char *findlabel(char *labelptr) {
    char name[MAXVARLEN + 1];
    int nl = 0;
    struct s_nameslot *n;
    // first, just exit we have a NULL argument
    if(labelptr == NULL) return NULL;
    if(issymbol(*labelptr)) {                                       // a symbol (Symbols.h)
        const char *s;
        int k = SymCanonAt(labelptr);
        if(k >= 0 && SymCold[k].labind != NULL) return SymCold[k].labind;   // found before
        NameView(labelptr, &s, &nl);
        if(nl > MAXVARLEN) error("Label too long");
        memcpy(name, s, nl);
        if((n = NameFind(name, nl, 1 << NK_LABEL)) == NULL) error("Cannot find label");
        if(k >= 0) SymCold[k].labind = (char *)n->target;           // (a miss is an error, never cached)
        return (char *)n->target;
    } else {
        // this assumes that the first character has already been verified as a valid label character
        name[nl++] = *labelptr++;
        while(isnamechar(*labelptr)) {
            if(nl >= MAXVARLEN) error("Label too long");            // too long, not a correctly formed label
            name[nl++] = *labelptr++;
        }
    }
    if((n = NameFind(name, nl, 1 << NK_LABEL)) == NULL) error("Cannot find label");
    return (char *)n->target;

/*


    // point to the main program memory
    p = (char *)ProgMemory;

    // now do the search
    while(1) {
        if(p[0] == 0 && p[1] == 0)                                  // end of the program
            error("Cannot find label");

        if(p[0] == T_NEWLINE) {
            lastp = p;                                              // save in case this is the right line
            p++;                                                    // and step over the line number
            continue;
        }

        if(p[0] == T_LINENBR) {
            p += 3;                                                 // and step over the line number
            continue;
        }

        if(p[0] == T_LABEL) {
            p++;                                                    // point to the length of the label
            if(mem_equal(p, label, label[0] + 1)){                   // compare the strings including the length byte
            	return lastp;                                       // and if successful return pointing to the beginning of the line
            }
                p += p[0] + 1;                                          // still looking! skip over the label
            continue;
        }

        p++;
    }*/
}

// count the number of lines up to and including the line pointed to by the argument
// used for error reporting in programs that do not use line numbers
// TRACE LIST: print the line that starts at target.  The image is walked by lines (NextLine): a walk byte by
// byte stopped at any 0xFF, which END TYPE's command token starts with, so every line after the first END TYPE
// printed nothing.
int MIPS16 TraceLines(char *target) {
    char *p, *next;
    int cnt;

    p = (char *)ProgMemory;
    cnt = 0;
    if(target==NULL)return cnt;
    while(*p == T_NEWLINE) {
        next = NextLine(p);
        {
            if(p >= target) {                                       // the line TRACE LIST is asking about
                if(LHDR_LINE(p)) {                                  // (line 0: a file name line)
                    char buff[10];
                    MMPrintString("[");
                    if(LHDR_FILE(p) > 1) {                          // an #INCLUDE file: [name:line]
                        MMPrintString(LineFileName(LHDR_FILE(p)));
                        MMPrintString(":");
                    }
                    IntToStr(buff, LHDR_LINE(p), 10);
                    MMPrintString(buff);
                    MMPrintString("] ");
                }
                return cnt;
            }
        }
        if(next > target) return cnt;                               // target is inside this line, not at a line's start
        p = next;
    }
    return cnt;
}



/********************************************************************************************************************************************
routines for storing and manipulating variables
********************************************************************************************************************************************/


// find or create a variable
// the action parameter can be the following (these can be ORed together)
// - V_FIND    a straight forward find, if the variable is not found it is created and set to zero
// - V_NOFIND_ERR    throw an error if not found
// - V_NOFIND_NULL   return a null pointer if not found
// - V_DIM_VAR    dimension an array
// - V_LOCAL   create a local variable
//
// there are four types of variable:
//  - T_NOTYPE a free slot that was used but is now free for reuse
//  - T_STR string variable
//  - T_NBR holds a float
//  - T_INT integer variable
//
// A variable can have a number of characteristics
//  - T_PTR the variable points to another variable's data
//  - T_IMPLIED  the variables type does not have to be specified with a suffix
//  - T_CONST the contents of this variable cannot be changed
//  - T_FUNCT this variable represents the return value from a function
//
// storage of the variable's data:
//      if it is type T_NBR or T_INT the value is held in the variable slot
//      for T_STR a block of memory of MAXSTRLEN size (or size determined by the LENGTH keyword) will be malloc'ed and the pointer stored in the variable slot.


//__attribute__((section(".fastcode")))
/* The compact variable table (the PicoMite's 509622a): a global's record is taken from a chunk when the
   global is made, and a free or blocked slot points at a placeholder. */

// every variable number's record with no global made: a local's in vartbl, a global slot free
void InitVarSlots(void) {
    for(int i = 0; i < MAXVARS; i++) slotrec[i] = i < MAXLOCALVARS ? &vartbl[i] : &emptyrec;
}

// a record of its own, not one of the placeholders shared by free and blocked slots
static inline int VarRecordReal(struct s_vartbl *r) {
    return r != &emptyrec && r != &blockedrec;
}

// the heap has been wiped: the chunks went with it, and so did every global
void VarChunksForget(void) {
    memset(&varmem, 0, sizeof(varmem));
    InitVarSlots();
}

// the globals have gone (CLEAR, RUN...; their data already freed): every slot free, and the chunks' records
// handed out again from the first.  The chunks stay until the heap is wiped: an interrupt (SETPIN, IR,
// KEYPAD...) may still be writing to a global's value, so that memory must not become something else's.
static void VarRecordsReset(void) {
    varmem.free = NULL;
    varmem.cur = 0;
    varmem.next = varmem.chunks ? varmem.chunk[0] : NULL;
    varmem.end = varmem.chunks ? varmem.chunk[0] + VARCHUNK : NULL;
    InitVarSlots();
}

// a chunk of globals' records: on-chip RAM while there is room, then the SDRAM heap
static struct s_vartbl *VarChunkMemory(void) {
    struct s_vartbl *r = GetInternalMemoryNull(VARCHUNK * sizeof(struct s_vartbl));
    if(r == NULL) r = GetMemory(VARCHUNK * sizeof(struct s_vartbl));
    return r;
}

// a zeroed record for the global about to be made in slot: an erased global's, else the next unused one in
// the chunks, else one of a new chunk.  Called before anything else is allocated for the variable or counted,
// so "not enough memory" leaves nothing behind; a record already in the slot (a failed DIM's) is used again.
static void VarRecordTake(int slot) {
    struct s_vartbl *r = slotrec[slot];
    if(!VarRecordReal(r)) {
        if((r = varmem.free) != NULL)
            memcpy(&varmem.free, r->name, sizeof(varmem.free));
        else {
            if(varmem.next == varmem.end) {
                if(varmem.cur + 1 < varmem.chunks)
                    r = varmem.chunk[++varmem.cur];                 // one a CLEAR left
                else {
                    r = VarChunkMemory();
                    varmem.chunk[varmem.chunks] = r;
                    varmem.cur = varmem.chunks++;
                }
                varmem.next = r;
                varmem.end = r + VARCHUNK;
            }
            r = varmem.next++;
        }
        slotrec[slot] = r;
    }
    memset(r, 0, sizeof(struct s_vartbl));
}

// the heap the globals' record chunks take (MEMORY counts them as variables, not General)
// the SDRAM heap the globals' record chunks take: the chunks in on-chip RAM are not in UsedHeap() (MEMORY)
int VarChunkBytes(void) {
    int i, n = 0;
    for(i = 0; i < varmem.chunks; i++)
        if((uint32_t)varmem.chunk[i] >= (uint32_t)SDRAMBASE && (uint32_t)varmem.chunk[i] < (uint32_t)SDRAMEND) n++;
    return n * VARCHUNK * sizeof(struct s_vartbl);
}

// does r, a global's record, own a block of the heap: an array's elements, a string or a structure (not one inside
// its own record, a short string)
static int VarOwnsBlock(struct s_vartbl *r) {
    return ((r->type & (T_STR | T_STRUCT)) || DimIsAllocated(r->dimtbl[0])) && !(r->type & T_PTR) && !VAR_INLINE_STR(r) && r->val.s != NULL;
}

// CHAIN: the heap wiped as for RUN, but for the blocks the globals own (their records' chunks and their data),
// listed first and left in use. The list is in the heap: InitHeapKeeping reads it before anything is allocated.
void InitHeapKeepGlobals(void) {
    int max = varmem.chunks + MAXGLOBALVARS, n = 0, i, sz;
    void **a = GetMemory(max * (sizeof(void *) + sizeof(int)));
    int *s = (int *)(a + max);
    for(i = 0; i < varmem.chunks; i++) {
        a[n] = varmem.chunk[i];
        s[n++] = MemSize(varmem.chunk[i]);
    }
    for(i = MAXLOCALVARS; i < MAXVARS; i++) {
        struct s_vartbl *r = VREC(i);
        if(!VarRecordReal(r) || !VarOwnsBlock(r) || (sz = MemSize(r->val.s)) == 0) continue;   // (0: not in a heap)
        a[n] = r->val.s;
        s[n++] = sz;
    }
    InitHeapKeeping(a, s, n);
}

// ERASE: the global in slot has gone, its data freed; its record goes on the free list and the slot is left
// free, or blocked when a probe must pass it.  The link is kept in the name, which no interrupt writes.
void VarRecordRelease(int slot, int blocked) {
    struct s_vartbl *r = slotrec[slot];
    memcpy(r->name, &varmem.free, sizeof(varmem.free));
    varmem.free = r;
    slotrec[slot] = blocked ? &blockedrec : &emptyrec;
}

// P6 F2: the local named name (in capitals, as findvar holds it) at the current level: the creation list
// walked down from the newest local while the levels are not below the current one, passing over a deeper
// level's (left by an error).  Its slot, or -1.
static inline int find_local_slot(const char *name, int namelen) {
    for(int k = hashlistpointer - 1; k >= 0; k--) {
        int lev = hashlist[k].level;
        if(lev < LocalIndex) break;                                 // the frames below this one
        if(lev != LocalIndex) continue;
        const char *ip = name, *tp = VREC(hashlist[k].hash)->name;
        if(*ip++ == *tp++) {                                        // preliminary quick check
            int j = namelen - 1;
            while(j > 0 && *ip == *tp) {                            // compare each letter
                j--; ip++; tp++;
            }
            if(j == 0 && (*tp == 0 || namelen == MAXVARLEN)) return hashlist[k].hash;   // a matching local at this level
        }
    }
    return -1;
}

// The global named name (namelen characters, in capitals), probed from its hash slot: returns its slot with
// *gfree -1, or, when there is none, the slot that ended the probe with *gfree the slot a new global takes (the
// last blocked slot passed, else that empty one).  The structure code looks a base variable up the same way.
static int probe_global_slot(char *name, int namelen, int slot, int *gfree) {
    int j, blocked = -1, stop = slot - 1;
    char *ip, *tp;
    if(stop < MAXLOCALVARS) stop += MAXGLOBALVARS;
    *gfree = -1;
    if(VREC(slot)->type == T_NOTYPE) {
        *gfree = slot;
        return slot;
    }
    while(VREC(slot)->name[0] != 0) {
        ip = name;
        tp = VREC(slot)->name;
        if(VREC(slot)->type == T_BLOCKED) blocked = slot;
        if(*ip++ == *tp++) {                                        // preliminary quick check
            j = namelen - 1;
            while(j > 0 && *ip == *tp) {                            // compare each letter
                j--; ip++; tp++;
            }
            if(j == 0 && (*(char *)tp == 0 || namelen == MAXVARLEN)) return slot;    // found a matching name
        }
        slot++;
        if(slot == MAXVARS) slot = MAXLOCALVARS;
        if(slot == stop) error("Too many global variables");       // full of variables and markers: no empty slot ends the search
    }
    *gfree = slot;                                                  // not found
    if(blocked != -1) {
        *gfree = blocked;
        VREC(slot)->type = T_NOTYPE;
        VREC(slot)->name[0] = 0;
    }
    return slot;
}

// the structure variable named basename (baselen characters, in capitals) that a member path starts with, found as
// findvar finds a variable (this level's locals, then the globals): its type, with *pvindex its slot; or -1
static int FindStructBase(char *basename, int baselen, int *pvindex) {
    int i, vindex = -1, gfree;
    uint32_t hash = FNV_offset_basis;
    for(i = 0; i < baselen; i++) {
        hash ^= basename[i];
        hash *= FNV_prime;
    }
    if(LocalIndex) vindex = find_local_slot(basename, baselen);
    if(vindex < 0) {
        vindex = probe_global_slot(basename, baselen, MAXLOCALVARS + (hash & (MAXGLOBALVARS - 1)), &gfree);
        if(gfree != -1) return -1;                                  // no such variable
    }
    if(!(VREC(vindex)->type & T_STRUCT)) return -1;
    *pvindex = vindex;
    return VREC(vindex)->size;
}

// the variable named at p (its text: a command's statement is spelt out, SymExpandStatement), found as findvar finds
// it (this level's locals, then the globals; a structure's member, P.X, by its base) but with nothing made and no
// error: its slot, or -1.  RESTORE's test of a bare name, a label unless it is a variable (the PicoMite's #103).
int FindVarSlot(char *p) {
    char name[MAXVARLEN + 1];
    int namelen = 0, baselen = 0, vindex = -1, gfree;
    uint32_t hash = FNV_offset_basis;
    skipspace(p);
    if(!isnamestart(*p)) return -1;
    do {
        char c = toupper(*p++);
        if(namelen == MAXVARLEN) return -1;                         // too long for a variable's name
        if(c == '.' && baselen == 0) baselen = namelen;
        hash ^= c;
        hash *= FNV_prime;
        name[namelen++] = c;
    } while(isnamechar(*p));
    if(namelen != MAXVARLEN) name[namelen] = 0;
    if(LocalIndex) vindex = find_local_slot(name, namelen);
    if(vindex < 0) {
        vindex = probe_global_slot(name, namelen, MAXLOCALVARS + (hash & (MAXGLOBALVARS - 1)), &gfree);
        if(gfree != -1) vindex = -1;
    }
    if(vindex < 0 && baselen && g_structcnt > 0) FindStructBase(name, baselen, &vindex);
    return vindex;
}

// where the reference findvar read ends, when nothing follows it that skipvar would step over too (getvalue's)
char *FindvarEnd;
static void __attribute__((noinline)) *findvar_body(char *p, int action, char **endp);

/* findvar_local (the PicoMite's a644dd8): what findvar_body does to make a local integer or float for a bound symbol
   at p - a SUB's or FUNCTION's parameter, a FUNCTION's result, a LOCAL - the same entry, counts and bindings, in the
   same order, without its search.  With no locals made from text, the symbol's newest local (SymL) is all the search
   would look at, and a name checked against the SUBs once (SYMC_NOSUB) stays checked.  NULL, before anything has
   changed, for anything else, which findvar_body then makes with its errors: a string, an array, a TYPE, a suffix
   that clashes, OPTION DEFAULT NONE, OPTION EXPLICIT, a name already made at this level (or left deeper by an error),
   a name's first check against the SUBs, a full local stack.  In ITCM, as the PicoMite keeps it in RAM. */
static void ITCM_CODE __attribute__((noinline)) *findvar_local(char *p, int action) {
    struct s_vartbl *vr;
    const char *nv;
    char *q;
    int symk, i, k, n, vtype, suffix = 1;
    skipspace(p);
    if(!issymbol(*p) || !LocalIndex || SymTextLocals || (action & (V_NOFIND_ERR | V_NOFIND_NULL | T_STRUCT))) return NULL;
    q = p + symbolsize(*p);                                         // the cheap tests first: a string or an array leaves here
    if(*q == '%') { vtype = T_INT; q++; }
    else if(*q == '!') { vtype = T_NBR; q++; }
    else if(*q == '$') return NULL;
    else {
        suffix = 0;
        vtype = (action & T_IMPLIED) ? (action & (T_NBR | T_INT | T_STR)) : DefaultType;
    }
    if((suffix && (action & T_IMPLIED) && !(action & vtype)) || (vtype != T_INT && vtype != T_NBR) || *q == '(' ||
       (i = localtop) >= MAXLOCALVARS || Localvarcnt + 1 >= MAXLOCALVARS || (symk = SymCanonAt(p)) < 0) return NULL;
    if(g_structcnt > 0 && (SymCold[symk].flags & SYMC_DOT)) return NULL;
    if((!(action & V_FUNCT) && !(SymCold[symk].flags & SYMC_NOSUB)) || (OptionExplicit && !(action & V_DIM_VAR)) ||
       ((k = SymL[symk]) >= 0 && VREC(k)->level >= LocalIndex)) return NULL;
    nv = SymSpelling(p, &n);
    if(n >= MAXVARLEN) return NULL;
    g_StructMemberType = g_StructMemberOffset = g_StructMemberSize = 0;
    emptyarray = 0;
    Localvarcnt++;
    varcnt = Globalvarcnt + Localvarcnt;
    VarIndex = i;
    vr = VREC(i);
    for(k = 0; k < n; k++) vr->name[k] = toupper(nv[k]);
    if(n < MAXVARLEN) vr->name[n] = 0;
    vr->type = vtype | (action & (T_IMPLIED | T_CONST));
    hashlist[hashlistpointer].level = LocalIndex;
    hashlist[hashlistpointer++].hash = i;
    localtop = i + 1;
    vr->level = LocalIndex;
    SymLocalMade(i, symk);
    for(k = 0; k < MAXDIM; k++) vr->dimtbl[k] = 0;
    if(vtype & T_NBR) {
        vr->val.f = 0;
        return &vr->val.f;
    }
    vr->val.i = 0;
    return &vr->val.i;
}

/* findvar in two parts, as the PicoMite's: a reference to an existing variable through a bound symbol, nearly every
   reference a program saved with symbols makes, is answered here, in ITCM. Anything else (a name in text, a
   declaration, a variable to be made, a TYPE, a suffix that does not match the variable) goes to findvar_body, in
   flash, which is findvar as it was: it is handed over before an index is evaluated, so nothing is evaluated twice,
   and the errors are findvar_body's. */
void ITCM_CODE *findvar(char *p, int action) {
    char *q = p, *e = NULL;
    struct s_vartbl *vr;
    int symk, i, n, vtype = 0, dnbr = 0, deferred = 0;
    int dim[MAXDIM];
    skipspace(q);
    if((action & V_LOCAL) && (e = findvar_local(q, action)) != NULL) {  // a local made for a bound symbol
        FindvarEnd = NULL;
        return e;
    }
    if(!issymbol(*q) || (action & (V_LOCAL | V_DIM_VAR | V_FUNCT | T_IMPLIED))) goto slow;
    if((symk = SymCanonAt(q)) < 0) goto slow;
    if(g_structcnt > 0 && (SymCold[symk].flags & SYMC_DOT)) goto slow;    // (a dotted name may be a member's path)
    i = SymL[symk];                                                 // its local at this level, or else its global if no
    if(i < 0 || VREC(i)->level != LocalIndex) {                     // local made from text could hide it
        i = SymG[symk];
        if(i < 0 || (LocalIndex && SymTextLocals)) goto slow;
    }
    vr = VREC(i);
    q += symbolsize(*q);
    if(*q == '$') { vtype = T_STR; q++; }
    else if(*q == '%') { vtype = T_INT; q++; }
    else if(*q == '!') { vtype = T_NBR; q++; }
    if(vr->name[0] == 0 || (vr->type & T_STRUCT) || !(vr->type & (vtype ? vtype : (DefaultType | T_IMPLIED)))) goto slow;
    emptyarray = 0;
    if(*q == '(') {
        char *pp = q + 1;
        skipspace(pp);
        if((action & V_EMPTY_OK) && *pp == ')') {                   // an empty array, a()
            emptyarray = 1;
            dnbr = -1;
        } else {                                                    // the indices, as findvar_body reads them
            int strindex = 0;
            q++;
            while(1) {
                MMFLOAT f;
                long long int in;
                char *s;
                int targ = T_NOTYPE;
                if(dnbr >= MAXDIM) error("Dimensions");
                skipspace(q);
                if(*q == ',' || *q == ')') error("Dimensions");
                q = evaluate(q, &f, &in, &s, &targ, false);
                if(targ == T_STR) strindex = 1;
                if(targ == T_NBR) in = FloatToInt32(f);
                else if(targ == T_INT && in != (int)in) error("Index out of bounds");
                dim[dnbr] = in;
                if(dim[dnbr] < OptionBase) {
                    if(action & V_NOFIND_NULL) deferred = 1;
                    else error("Dimensions");
                }
                dnbr++;
                skipspace(q);
                if(*q == ',') { q++; continue; }
                if(*q == ')') { q++; break; }
                error("Expected closing bracket");
            }
            if(strindex) dnbr = MAXDIM;
        }
    }
    if(!(*q == ' ' || *q == '.' || *q == '(')) e = q;               // where it ends, as findvar_body says it
    FindvarEnd = e;                                                 // (after the indices: their own findvar set it too)
    VarIndex = i;
    g_StructMemberType = g_StructMemberOffset = g_StructMemberSize = 0;    // (likewise: an index may be a member, a(p.x))
    for(n = 0; n < MAXDIM && !DimIsEnd(vr->dimtbl[n]); n++);        // the dimensions must match
    if(dnbr == -1) {
        if(n == 0) error("Array dimensions");
    } else if(n != dnbr) error("Array dimensions");
    if(dnbr == -1 || DimIsScalar(vr->dimtbl[0])) {                  // a plain variable, or an empty array
        if(dnbr == -1 || (vr->type & (T_PTR | T_STR))) return vr->val.s;
        if(vr->type & T_INT) return &(vr->val.i);
        return &(vr->val.f);
    }
    if(deferred) error("Dimensions");
    for(n = 0; n < dnbr; n++)
        if(dim[n] > DimUpper(vr->dimtbl[n]) || dim[n] < OptionBase) error("Index out of bounds");
    int nbr = dim[0] - OptionBase, j = 1;                           // the element (Gerard Sexton's formula)
    for(n = 1; n < dnbr; n++) {
        j *= (DimUpper(vr->dimtbl[n - 1]) + 1 - OptionBase);
        nbr += (dim[n] - OptionBase) * j;
    }
    if(vr->type & T_NBR) return vr->val.s + (nbr * sizeof(MMFLOAT));
    if(vr->type & T_INT) return vr->val.s + (nbr * sizeof(long long int));
    return vr->val.s + (nbr * (vr->size + 1));
slow:
    {
        void *r = findvar_body(p, action, &e);
        FindvarEnd = e;                                             // (after the body: an index's own findvar has set it too)
        return r;
    }
}

static void __attribute__((noinline)) *findvar_body(char *p, int action, char **endp) {
    char name[MAXVARLEN + 1];
    int i=0, j, size, ifree, globalifree, localifree, nbr, vtype, vindex, namelen, tmp;
    char *s, *x;
    void *mptr = NULL;
//    int hashIndex=0;
    int GlobalhashIndex, structarg;
    int LocalhashIndex;
    uint32_t hash=FNV_offset_basis;
    int dim[MAXDIM]={0}, dnbr;
    int dim_error_deferred = 0;                                     // a bad index, reported once the name is known to be an array
    int symk = -1;                                                  // the symbol's canonical entry (Symbols.h), -1: text or no bindings
    char *symp = NULL;                                              // the symbol, when its name has not been read yet
//	if(__get_MSP() < (uint32_t)&stackcheck-0x5000){
//		error("Expression is too complex at depth %",LocalIndex);
//	}
    vtype = dnbr = emptyarray = 0;
    g_StructMemberType = g_StructMemberOffset = g_StructMemberSize = 0;    // not a structure's member (yet)
    structarg = g_StructArg;                                        // a declaration's type (a FUNCTION in a bound can change it)
    // first zero the array used for holding the dimension values
//    for(i = 0; i < MAXDIM; i++) dim[i] = 0;
    ifree = -1;

    // check the first char for a legal variable name
    skipspace(p);
    s = name; namelen = 0;
    if(issymbol(*p)) {                                              // a symbol (Symbols.h)
        symk = SymCanonAt(p);
        if(symk >= 0 && g_structcnt > 0 && (SymCold[symk].flags & SYMC_DOT)) symk = -1;   // a dotted name may be a member path: by its name
        if(symk >= 0) {
            symp = p;                                               // bound: its name is read only if the binding misses
            p += symbolsize(*p);
        } else {
            int sl;
            const char *sp = SymSpelling(p, &sl);
            p += symbolsize(*p);
            if(sl > MAXVARLEN) error("Variable name too long");
            while(sl--) {
                char c = toupper(*sp++);
                hash ^= c;
                hash*=FNV_prime;
                *s++ = c;
                namelen++;
            }
        }
    } else {
        if(!isnamestart(*p)) error("Variable name");
        // copy the variable name into name
        do {                                                        // in capitals, whatever the text (a DATA-first
            char c = toupper(*p++);                                 // line keeps its case: READ evaluates its numbers)
            hash ^= c;
            hash*=FNV_prime;
            *s++ = c;
            if(++namelen > MAXVARLEN) error("Variable name too long");
        } while(isnamechar(*p));
    }
	GlobalhashIndex = MAXLOCALVARS + (hash & (MAXGLOBALVARS - 1));   // the global slot the probe starts at
	if(namelen!=MAXVARLEN)*s=0;
    // check the terminating char and set the type
    if(*p == '$') {
        if((action & T_IMPLIED) && !(action & T_STR)) error("Conflicting variable type");
        vtype = T_STR;
        p++;
    } else if(*p == '%') {
        if((action & T_IMPLIED) && !(action & T_INT)) error("Conflicting variable type");
        vtype = T_INT;
        p++;
    } else if(*p == '!') {
        if((action & T_IMPLIED) && !(action & T_NBR)) error("Conflicting variable type");
        vtype = T_NBR;
        p++;
    } else if((action & V_DIM_VAR) && DefaultType == T_NOTYPE && !(action & T_IMPLIED))
        error("Variable type not specified");
    else
        vtype = 0;

    // a structure's member: '.' can be in a name, so the name holds the path (P.X, A.B.C).  Only when the program
    // has types: otherwise a dot is just part of a name.
    if(g_structcnt > 0) {
        char *dot = memchr(name, '.', namelen);
        int struct_idx, member_type;
        if(dot != NULL && (struct_idx = FindStructBase(name, dot - name, &vindex)) >= 0) {
            char path[MAXVARLEN + 1];
            j = namelen - (dot - name) - 1;
            memcpy(path, dot + 1, j);
            path[j] = 0;
            mptr = ResolveStructMember(VREC(vindex)->val.s, struct_idx, path, &p, &member_type);
            VarIndex = vindex;
            g_StructMemberType = member_type;
            return mptr;
        }                                                           // (else not a structure: a name with a dot)
    }

    // check if this is an array
    if(*p == '(') {
        char *pp = p + 1;
        skipspace(pp);
        if(action & V_EMPTY_OK && *pp == ')')  {                     // if this is an empty array.  eg  ()
        	emptyarray=1;
            dnbr = -1;                                              // flag this
        }  else {                                                      // else, get the dimensions
            // each index evaluated where it is, as far as the ',' or ')' after it: no copy of the list (getargs)
            // and no second pass to find the closing bracket
            int strindex = 0;
            p++;
            dnbr = 0;
            while(1) {
                MMFLOAT f;
                long long int in;
                char *s;
                int targ = T_NOTYPE;
                if(dnbr >= MAXDIM) error("Dimensions");
                skipspace(p);
                if(*p == ',' || *p == ')') error("Dimensions");      // an index left out
                p = evaluate(p, &f, &in, &s, &targ, false);         // get the value and type of the argument
                if(targ == T_STR) strindex = 1;                     // an error later, with the correct message
                if(targ == T_NBR) in = FloatToInt32(f);
                else if(targ == T_INT && in != (int)in) error("Index out of bounds");   // a(4294967297) must not be a(1)
                dim[dnbr] = in;
                if(dim[dnbr] < OptionBase) {
                    if(action & V_NOFIND_NULL) dim_error_deferred = 1;  // a FUNCTION's argument, SX(-24), is probed as an array first
                    else error("Dimensions");
                }
                dnbr++;
                skipspace(p);
                if(*p == ',') { p++; continue; }
                if(*p == ')') { p++; break; }
                error("Expected closing bracket");
            }
            if(strindex) dnbr = MAXDIM;                             // (force an error to be thrown later)
        }
    }
    // the reference ends here unless skipvar would go on (a space before a bracket, a member, an empty array's ())
    if(!(*p == ' ' || *p == '.' || *p == '(')) *endp = p;
    g_StructMemberType = 0;                                         // (an index may have been a structure's member: a(p.x))

    // a bound symbol: its local at this level, or its global (unless locals made from text could hide it in a SUB)
    if(symk >= 0 && !(action & (V_LOCAL | V_DIM_VAR | V_FUNCT))) {
        i = SymL[symk];
        if(i >= 0 && VREC(i)->level == LocalIndex) { ifree = -1; goto findvar_found; }
        i = SymG[symk];
        if(i >= 0 && (!LocalIndex || !SymTextLocals) && VREC(i)->name[0] != 0) { ifree = -1; goto findvar_found; }
    }
    if(symp != NULL) {                                              // the binding missed: the name, for the search
        int sl;
        const char *sp = SymSpelling(symp, &sl);
        if(sl > MAXVARLEN) error("Variable name too long");
        while(sl--) {
            char c = toupper(*sp++);
            hash ^= c;
            hash*=FNV_prime;
            *s++ = c;
            namelen++;
        }
        if(namelen != MAXVARLEN) *s = 0;
        GlobalhashIndex = MAXLOCALVARS + (hash & (MAXGLOBALVARS - 1));
        symp = NULL;
    }

    // we now have the variable name and, if it is an array, the parameters
    // search the table looking for a match

    LocalhashIndex=-1;
    localifree=-1;
	globalifree=-1;
    if(LocalIndex){ //search
        // this level's locals first: the top of the local stack (P6 F2)
        if((LocalhashIndex = find_local_slot(name, namelen)) < 0)
            localifree = localtop;                                  // where a new local would go
		if(LocalhashIndex < 0 && !(action & V_LOCAL))               // not a local here, so try the global (a local being made has no use for it: P6 F3)
            GlobalhashIndex = probe_global_slot(name, namelen, GlobalhashIndex, &globalifree);
    } else {
    	localifree=9999; //set a marker that a local variable is irrelevant
        GlobalhashIndex = probe_global_slot(name, namelen, GlobalhashIndex, &globalifree);
    }
	//MMPrintString("search status : ");PInt(LocalIndex);PIntComma(localifree);PIntComma(LocalhashIndex);PIntComma(globalifree);PIntComma(GlobalhashIndex);
	//MMPrintString((action & V_LOCAL ? " LOCAL" : "      "));MMPrintString((action & V_LOCAL ? " DIM" : "    "));PRet();
	// At this point we know if a local variable has been found or if a global variable has been found
    if(action & V_LOCAL) {
        // if we declared the variable as LOCAL within a sub/fun and an existing local was found
        if(localifree==-1) error("$ Local variable already declared", name);
    } else if(action & V_DIM_VAR) {
        // if are using DIM to declare a global variable and an existing global variable was found
        if(globalifree==-1 ) error("$ Global variable already declared", name);
    }
	// we are not declaring the variable but it may need to be created
    if(action & V_LOCAL) {
        if(localifree >= MAXLOCALVARS) error("Too many local variables");   // the local stack is full (P6 F2)
    	ifree = i = localifree;
    } else if(localifree==-1){ // can only happen when a local variable has been found so we can ignore everything global
		ifree= -1;
		i = LocalhashIndex;
	} else if(globalifree==-1){ //A global variable has been found
		ifree= -1;
		i = GlobalhashIndex;
	} else { //nothing has been found so we are going to create a global unless EXPLICIT is set
		ifree = i = globalifree;
	}

    //MMPrintString(name);PIntComma(i);MMPrintString((ifree==-1 ? " - found" : " - not there"));PRet();

    // if we found an existing and matching variable
    // set the global VarIndex indicating the index in the table
    if(ifree==-1 && VREC(i)->name[0] != 0) {
        if(symk >= 0 && i >= MAXLOCALVARS) SymG[symk] = i;          // a global found by its name: bound now
findvar_found:
        VarIndex = vindex = i;

        // check that the dimensions match
        for(i = 0; i < MAXDIM && !DimIsEnd(VREC(vindex)->dimtbl[i]); i++);
        if(dnbr == -1) {
            if(i == 0) error("Array dimensions");
        } else {
            if(i != dnbr) error("Array dimensions");
        }

        if(vtype == 0 ? !(VREC(vindex)->type & (DefaultType | T_IMPLIED)) : !(VREC(vindex)->type & vtype)) {
            if(symp != NULL) {                                      // (bound: its name is read only for the message)
                int sl;
                const char *sp = SymSpelling(symp, &sl);
                if(sl > MAXVARLEN) sl = MAXVARLEN;
                for(namelen = 0; namelen < sl; namelen++) name[namelen] = toupper(sp[namelen]);
                if(namelen != MAXVARLEN) name[namelen] = 0;
            }
            error("$ Different type already declared", name);
        }

        // if it is a non arrayed variable or an empty array it is easy, just calculate and return a pointer to the value
        if(dnbr == -1 || DimIsScalar(VREC(vindex)->dimtbl[0])) {
            if(VREC(vindex)->type & T_STRUCT) {
                if(dnbr == -1 && *p == '(') {                       // a(): followed by a member in each element, a().x
                    p++;
                    skipspace(p);
                    if(*p == ')') p++;
                    skipspace(p);
                }
                if(*p == '.') return StructMemberAt(VREC(vindex)->val.s, VREC(vindex)->size, &p);
                return VREC(vindex)->val.s;                         // the whole structure
            }
            if(dnbr == -1 || VREC(vindex)->type & (T_PTR | T_STR))
                return VREC(vindex)->val.s;                        // if it is a string or pointer just return the pointer to the data
            else
                if(VREC(vindex)->type & (T_INT))
                    return &(VREC(vindex)->val.i);                 // must be an integer, point to its value
                else
                    return &(VREC(vindex)->val.f);                 // must be a straight number (float), point to its value
         }

        // if we reached this point it must be a reference to an existing array
        // check that we are not using DIM and that all parameters are within the dimensions
        if(dim_error_deferred) error("Dimensions");
        if(action & V_DIM_VAR) error("Cannot re dimension array");
        for(i = 0; i < dnbr; i++) {
            if(dim[i] > DimUpper(VREC(vindex)->dimtbl[i]) || dim[i] < OptionBase)
                error("Index out of bounds");
        }

        // then calculate the index into the array.  Bug fix by Gerard Sexton.
        nbr = dim[0] - OptionBase;
        j = 1;
        for(i = 1; i < dnbr; i++) {
            j *= (DimUpper(VREC(vindex)->dimtbl[i - 1]) + 1 - OptionBase);
            nbr += (dim[i] - OptionBase) * j;
        }
        // an element of an array of structures, or its member: points(1).x
        if(VREC(vindex)->type & T_STRUCT) {
            char *sp = VREC(vindex)->val.s + nbr * g_structtbl[VREC(vindex)->size]->total_size;
            skipspace(p);
            if(*p == '.') return StructMemberAt(sp, VREC(vindex)->size, &p);
            return sp;
        }
        // finally return a pointer to the value
        if(VREC(vindex)->type & T_NBR)
            return VREC(vindex)->val.s + (nbr * sizeof(MMFLOAT));
        else
            if(VREC(vindex)->type & T_INT)
                return VREC(vindex)->val.s + (nbr * sizeof(long long int));
            else
                return VREC(vindex)->val.s + (nbr * (VREC(vindex)->size + 1));
    }

    // we reached this point if no existing variable has been found
    if(action & V_NOFIND_ERR) error("Cannot find $", name);
    if(action & V_NOFIND_NULL) return NULL;
    if((OptionExplicit || dnbr != 0) && !(action & V_DIM_VAR))
        error("$ is not declared", name);
    if(vtype == 0) {
        if(action & T_IMPLIED)
            vtype = (action & (T_NBR | T_INT | T_STR | T_STRUCT));
        else
            vtype = DefaultType;
    }

    // now scan the sub/fun table to make sure that there is not a sub/fun with the same name (not for a FUNCTION's
    // own local); once it has passed for a symbol's name it holds for the run (the PicoMite's P6 F3: PrepareProgram
    // starts the symbols afresh, so a CHAIN or a new RUN checks again)
    if(!(action & V_FUNCT) && !(symk >= 0 && (SymCold[symk].flags & SYMC_NOSUB))) {
        if(NameFind(name, namelen, (1 << NK_SUB) | (1 << NK_FUN)) != NULL) error("A sub/fun has the same name: $", name);
        if(symk >= 0) SymCold[symk].flags |= SYMC_NOSUB;
    }

    // set a default string size
    size = MAXSTRLEN;

    // if it is an array we must be dimensioning it
    // if it is a string array we skip over the dimension values and look for the LENGTH keyword
    // and if found find the string size and change the vartbl entry
      if(action & V_DIM_VAR) {
          if(vtype & T_STR) {
            i = 0;
            if(*p == '(') p = getclosebracket(p) + 1;                  // (past a quoted bracket too: the PicoMite's #104)
            skipspace(p);
            if((s = checkstring(p, "LENGTH")) != NULL)
                size = getint(s, 1, MAXSTRLEN) ;
            else
                if(!(*p == ',' || *p == 0 || tokenfunction(*p) == op_equal || tokenfunction(*p) == op_invalid)) error("Unexpected text: $", p);
        }
    }


    // at this point we need to create the variable
    // as a result of the previous search ifree is the index to the entry that we should use

    // its bounds are checked first: an error from here on would leave a half-made variable behind
    for(i = 0; i < dnbr; i++)
#if DIM_DECODE_ENABLED
        // an upper bound equal to OPTION BASE is a single element dimension
        if(dim[i] < OptionBase)
#else
        if(dim[i] <= OptionBase)
#endif
            error("Dimensions");
    if((vtype & T_STRUCT) && dnbr != -1) {                          // (DIM ... AS type gave it; a parameter's comes with the call)
        if(structarg < 0 || structarg >= g_structcnt) error("Invalid structure type");
        if(dnbr == 0) mptr = GetMemory(g_structtbl[structarg]->total_size);    // a structure's data (zeroed)
    }

 // if we are adding to the top, increment the number of vars
	if(ifree>=MAXLOCALVARS){                                        // the limit is tested before counting: a refused variable is not counted
		if(Globalvarcnt+1>=MAXGLOBALVARS)error("Not enough Global variable memory");
		VarRecordTake(ifree);                                       // its record, before anything is counted or allocated
		Globalvarcnt++;
	} else {
		if(Localvarcnt+1>=MAXLOCALVARS)error("Not enough Local variable memory");
		Localvarcnt++;
	}
	varcnt=Globalvarcnt+Localvarcnt;
    VarIndex = vindex = ifree;

    // initialise it: save the name, set the initial value to zero and set the type
    s = name;  x = VREC(ifree)->name; j = namelen;
    while(j--) *x++ = *s++;
    if(namelen < MAXVARLEN)*x++ = 0;
    VREC(ifree)->type = vtype | (action & (T_IMPLIED | T_CONST));
    if(ifree<MAXLOCALVARS){
    	hashlist[hashlistpointer].level=LocalIndex;
    	hashlist[hashlistpointer++].hash=ifree;
        localtop = ifree + 1;                                       // the stack's next slot (P6 F2)
        VREC(ifree)->level = LocalIndex;
        SymLocalMade(ifree, symk);                                  // its binding (Symbols.h; -1: made from text)
    } else {
        VREC(ifree)->level = 0;
        if(symk >= 0) SymG[symk] = ifree;                           // a global made from a symbol is bound at once (the PicoMite's 040c21c)
    }
//    cleardims(&VREC(ifree)->dimtbl[0]);
    for(j = 0; j < MAXDIM; j++) VREC(ifree)->dimtbl[j] = 0;
//    MMPrintString("Creating variable : ");MMPrintString(VREC(ifree)->name);MMPrintString(", at depth : ");PInt(VREC(ifree)->level);MMPrintString(", hash key : ");PInt(ifree);PRet();
    // the easy request is for is a non array numeric variable, so just initialise to
    // zero and return the pointer
    if(dnbr == 0) {
        if(vtype & T_NBR) {
            VREC(ifree)->val.f = 0;
            return &(VREC(ifree)->val.f);
        } else if(vtype & T_INT) {
            VREC(ifree)->val.i = 0;
            return &(VREC(ifree)->val.i);
        } else if(vtype & T_STRUCT) {
            VREC(ifree)->size = structarg;                          // a structure's size is its type
            VREC(ifree)->val.s = mptr;
            return mptr;
        }
    }

    // if this is a definition of an empty array (only used in the parameter list for a sub/function)
    if(dnbr == -1) {
        VREC(vindex)->dimtbl[0] = -1;                                // let the caller know that this is an empty array and needs more work
        if(vtype & T_STRUCT) VREC(vindex)->size = structarg;        // an array of structures as a parameter: its type
        return VREC(vindex)->val.s;                                // just return a pointer to the data element as it will be replaced in the sub/fun with a pointer
    }

    // if this is an array copy the array dimensions and calculate the overall size
    // for a non array string this will leave nbr = 1 which is just what we want
    for(nbr = 1, i = 0; i < dnbr; i++) {
        VREC(vindex)->dimtbl[i] = DimEncode(dim[i]);
        nbr *= (dim[i] + 1 - OptionBase);
    }

    // we now have a string, an array of strings or an array of numbers
    // all need some memory to be allocated (note: GetMemory() zeros the memory)

    // First, set the important characteristics of the variable to indicate that the
    // variable is not allocated.  Thus, if GetMemory() fails with "not enough memory",
    // the variable will remain not allocated
    VREC(ifree)->val.s = NULL;
    VREC(ifree)->type = T_BLOCKED;
    i = *VREC(ifree)->name;   *VREC(ifree)->name = 0;
	j = VREC(ifree)->dimtbl[0]; VREC(ifree)->dimtbl[0] = 0;


	// Now, grab the memory
    if(vtype & (T_NBR | T_INT)) {
    	tmp=(nbr * sizeof(MMFLOAT));
        if(tmp<=256)mptr = GetStringMemoryBottom();
        else mptr = GetMemory(tmp);
    } else if(vtype & T_STRUCT) {                                   // an array of structures
        mptr = GetMemory(nbr * g_structtbl[structarg]->total_size);
        size = structarg;                                           // its size is its type
    }  else {
    	tmp=(nbr * (size + 1));
    	if(tmp<=16 && j==0)mptr = (void *)&VREC(ifree)->dimtbl[1];  // a short string scalar in its record (the PicoMite's);
    	                                                // never an array: counts multiply all of dimtbl (parsestringarray)
    	else if(tmp<=256)mptr = GetStringMemoryBottom();  // <=256 into Heap
        else mptr = GetMemory(tmp);                 // else SDRAM
    }




    // If we reached here the memory request was successful, so restore the details of
    // the variable that were saved previously and set the variables pointer to the
    // allocated memory
    VREC(ifree)->type = vtype | (action & (T_IMPLIED | T_CONST));
    *VREC(ifree)->name = i;
    VREC(ifree)->dimtbl[0] = j;
    VREC(ifree)->size = size;
    VREC(ifree)->val.s = mptr;
    return mptr;
}




/********************************************************************************************************************************************
 utility routines
 these routines form a library of functions that any command or function can use when dealing with its arguments
 by centralising these routines it is hoped that bugs can be more easily found and corrected (unlike bwBasic !)
*********************************************************************************************************************************************/

// take a line of basic code and split it into arguments
// this function should always be called via the macro getargs
//
// a new argument is created by any of the chars in the string delim (not in brackets or quotes)
// with this function commands have much less work to do to evaluate the arguments
//
// The arguments are:
//   pointer to a pointer which points to the string to be broken into arguments.
//   the maximum number of arguments that are expected.  an error will be thrown if more than this are found.
//   buffer where the returned strings are to be stored
//   pointer to an array of strings that will contain (after the function has returned) the values of each argument
//   pointer to an integer that will contain (after the function has returned) the number of arguments found
//   pointer to a string that contains the characters to be used in spiting up the line.  If the first char of that
//       string is an opening bracket '(' this function will expect the arg list to be enclosed in brackets.


//__attribute__((section(".fastcode")))
//void makeargs(char **p, int maxargs, char *argbuf, char *argv[], int *argc, char *delim) {
inline void ITCM_CODE makeargs(char **p, int maxargs, char *argbuf, char *argv[], int *argc, char *delim) {
    char *op;
    int inarg, expect_cmd, expect_bracket, then_tkn, else_tkn;
    char *tp;

	TestStackOverflow();                                            // (the PicoMite's: MMBasic.h)

    tp = *p;
    op = argbuf;
    *argc = 0;
    inarg = false;
    expect_cmd = false;
    expect_bracket = false;
    then_tkn = tokenTHEN;
    else_tkn = tokenELSE;

    // skip leading spaces
    while(*tp == ' ') tp++;

    // check if we are processing a list enclosed in brackets and if so
    //  - skip the opening bracket
    //  - flag that a closing bracket should be found
    if(*delim == '(') {
        if(*tp != '(')
            error("Syntax");
        expect_bracket = true;
        delim++;
        tp++;
    }

    // the main processing loop
    while(*tp) {

        if(expect_bracket == true && *tp == ')') break;

        // comment char causes the rest of the line to be skipped
        if(*tp == '\'') {
            break;
        }

        // the special characters that cause the line to be split up are in the string delim
        // any other chars form part of the one argument
        if(strchr(delim, *tp) != NULL && !expect_cmd) {
            if(*tp == then_tkn || *tp == else_tkn) expect_cmd = true;
            if(inarg) {                                             // if we have been processing an argument
                while(op > argbuf && *(op - 1) == ' ') op--;        // trim trailing spaces
                *op++ = 0;                                          // terminate it
            } else if(*argc) {                                      // otherwise we have two delimiters in a row (except for the first argument)
                argv[(*argc)++] = op;                               // create a null argument to go between the two delimiters
                *op++ = 0;                                          // and terminate it
            }

            inarg = false;
            if(*argc >= maxargs) error("Syntax");
            argv[(*argc)++] = op;                                   // save the pointer for this delimiter
            *op++ = *tp++;                                          // copy the token or char (always one)
            *op++ = 0;                                              // terminate it
            continue;
        }

        // check if we have a THEN or ELSE token and if so flag that a command should be next
        if(*tp == then_tkn || *tp == else_tkn) expect_cmd = true;


        // remove all spaces (outside of quoted text and bracketed text)
        if(!inarg && *tp == ' ') {
            tp++;
            continue;
        }

        // not a special char so we must start a new argument
        if(!inarg) {
            if(*argc >= maxargs) error("Syntax");
            argv[(*argc)++] = op;                                   // save the pointer for this arg
            inarg = true;
        }

        // if an opening bracket '(' copy everything until we hit the matching closing bracket
        // this includes special characters such as , and ; and keeps track of any nested brackets
        if(*tp == '(' || ((tokentype(*tp) & T_FUN) && !expect_cmd)) {
            int x;
            x = (getclosebracket(tp) - tp) + 1;
            mycpy(op, tp, x);
            op += x; tp += x;
            continue;
        }

        // if quote mark (") copy everything until the closing quote
        // this includes special characters such as , and ;
        // the tokenise() function will have ensured that the closing quote is always there
        if(*tp == '"') {
            do {
                *op++ = *tp++;
                if(*tp == 0) error("Syntax");
            } while(*tp != '"');
            *op++ = *tp++;
            continue;
        }

        // anything else is just copied into the argument
        *op++ = *tp++;
        // after THEN or ELSE that can be a command token: copy the rest of it as well
        if(expect_cmd && sizeof(CommandToken) > 1 && (unsigned char)tp[-1] >= C_BASETOKEN && (unsigned char)*tp >= C_BASETOKEN) *op++ = *tp++;

        expect_cmd = false;
    }
    if(expect_bracket && *tp != ')') error("Syntax");
    while(op - 1 > argbuf && *(op-1) == ' ') --op;                  // trim any trailing spaces on the last argument
    *op = 0;                                                        // terminate the last argument
    // argv[] past argc held whatever the stack did: a command that read an argument it was not given
    // (FRAMEBUFFER CREATE with no size) evaluated garbage and could fault.  They are empty strings now.
    for(int i = *argc; i < maxargs; i++) argv[i] = op;
}
// the error's text, into errstring (256 bytes): what does not fit is lost, but the line's end keeps its room (an
// ERROR message of 255 characters after "Error in line N: " ran past it: the PicoMite's #110)
void MMErrorchar(char c){
    if(errpos < (int)sizeof(errstring) - ((c == '\r' || c == '\n') ? 1 : 3)) errstring[errpos++] = c;
    errstring[errpos] = 0;
}
void MMErrorString(char *msg){
    while(*msg) MMErrorchar(*msg++);
}
// throw an error
// displays the error message and aborts the program
// the message can contain variable text which is indicated by a special character in the message string
//  $ = insert a string at this place
//  @ = insert a character
//  % = insert a number
// the optional data to be inserted is the second argument to this function
// this uses longjump to skip back to the command input and cleanup the stack
// is p in program memory's image (or a flash slot's program running in place: ImageSlots.c)? Both bounds: under OPTION
// RAM, or with a slot's program in the heap, the token buffer at the prompt is below the program, not above it
int InProgMemory(const char *p) {
    return p >= (const char *)ProgMemory && p < (const char *)ProgMemory + PROG_FLASH_SIZE;
}

// the evaluator's stack test, for a recursion through the compiler's calls, which never reaches the evaluator (Stream.c)
// the stack is in its last 1K above StackFloor (TestStackOverflow, MMBasic.h): below StackFloor it has overflowed,
// otherwise the lowest is kept for a warning at the prompt (the PicoMite's)
void __attribute__((noinline)) StackNearLimit(uint32_t stack) {
    if(stack < StackFloor) {
        StackLow = 0;
        error("Stack overflow, at depth %, stack &, heap &", LocalIndex, (int)stack, (int)StackFloor);
    }
    if(StackLow == 0 || stack < StackLow) {
        StackLow = stack;
        StackLowDepth = LocalIndex;
    }
}

void error(char *msg, ...) {
    va_list ap;
    int fmline = 0;                                                 // the program's line, for FM (FmNoteError)
	ScrewUpTimer=0;
    if(MMerrno == 0) MMerrno = 16;                                  // indicate an error
    MMerrline = 0;                                                  // (the line, below, if the error is in one)
    mymemset(errstring,0,sizeof(errstring));
    LoadOptions();                                                  // make sure that the option struct is in a clean state

    if((OptionConsole & 2) && !OptionErrorSkip) {
        SetFont(PromptFont);
        gui_fcolour = PromptFC;
        gui_bcolour = PromptBC;
        if(CurrentX != 0) MMErrorString("\r\n");                   // error message should be on a new line
    }

    if(MMCharPos > 1 && !OptionErrorSkip) MMErrorString("\r\n");
    if(CurrentLinePtr) {
        int inlib = InLibrary(CurrentLinePtr);                      // a library line: its own image, and its file always named
        if(*CurrentLinePtr != T_NEWLINE && (inlib || InProgMemory(CurrentLinePtr))) {
            // normally CurrentLinePtr points to a T_NEWLINE token but in this case it does not
            // so we have to search for the start of the line and set CurrentLinePtr to that
            char *line = inlib ? LibMemory : (char *)ProgMemory, *next;
            while(*line == T_NEWLINE) {
                next = NextLine(line);
                if(CurrentLinePtr < next) { CurrentLinePtr = line; break; }
                line = next;
            }
        }
        // the line's header names its file and line (see T_NEWLINE_HDR)
       	if((inlib || InProgMemory(CurrentLinePtr)) && *CurrentLinePtr == T_NEWLINE) {
       		if(MMCharPos > 1)MMErrorString("\r\n");
       		MMErrorString("Error in ");
       		if(LHDR_FILE(CurrentLinePtr) > 1 || inlib) {           // an #INCLUDE file, or a library's
       			MMErrorString(LineFileNameAt(CurrentLinePtr));
       			MMErrorString(" ");
       		}
       		MMErrorString("line ");
       		MMerrline = LHDR_LINE(CurrentLinePtr);                  // MM.ERRLINE: the number this message gives
       		IntToStr(inpbuf, LHDR_LINE(CurrentLinePtr), 10);
       		MMErrorString(inpbuf);
       		if(LHDR_FILE(CurrentLinePtr) == 1 && !inlib && !OptionErrorSkip) {
       			StartEditLine = LHDR_LINE(CurrentLinePtr) - 1;     // EDIT opens the program at the line
       			StartEditCharacter = 0;
       			fmline = LHDR_LINE(CurrentLinePtr);                 // (and FM's E, for a program FM ran)
       		}
       		MMErrorString(": ");
        }
    }
//    if(!OptionErrorSkip)MMErrorString("Error");
    int msgstart = errpos;                                          // (MM.ERRMSG$ is what follows the "Error in line N: ")
    if(*msg) {
//    	if(!OptionErrorSkip)MMErrorString(": ");
        va_start(ap, msg);
        while(*msg) {
            if(*msg == '$') {                                       // a string: any symbols in it spelt out (Symbols.h)
                char *a = va_arg(ap, char *), sb[STRINGSIZE];
                int n = SymExpand(sb, a, strlen(a), STRINGSIZE - 1);
                if(n < 0) MMErrorString(a);
                else { sb[n] = 0; MMErrorString(sb); }
            }
            else if(*msg == '@')
                MMErrorchar(va_arg(ap, int));
            else if(*msg == '%' || *msg == '|') {
                char buf[20];
                IntToStr(buf, va_arg(ap, int), 10);
                MMErrorString(buf);
            } else if(*msg == '~'){                                    // insert a long long integer
            	char buf[20];
            	IntToStr(buf, va_arg(ap, int64_t), 10);
            	MMErrorString(buf);

             } else if(*msg == '&') {
                    char buf[20];
                    IntToStr(buf, va_arg(ap, int), 16);
                    MMErrorString(buf);
            } else {
                MMErrorchar(*msg);
            }
            msg++;
        }
        if(!OptionErrorSkip)MMErrorString("\r\n");
    }
    {   // MM.ERRMSG$: the message alone, as on the PicoMite (18 Oct, Peter); its line is MM.ERRLINE. The console
        // still shows errstring, "Error in line N: " and all.
        static char errmsg[256];
        char *s = errstring + msgstart;
        int n = strlen(s);
        while(n && (s[n - 1] == '\r' || s[n - 1] == '\n')) n--;
        memcpy(errmsg, s, n);
        errmsg[n] = 0;
        MMErrMsg = errmsg;
    }
    // Clean up after and error in DefindedSubFun, or in a call the compiler made (DefinedSubFunMem 2: no argument
    // block, Stream.c)
    if(DefinedSubFunMem){
       	if(LocalIndex != DefinedSubFunLocalIndex) ClearVars(LocalIndex);
    	gosubindex--;
    	if(DefinedSubFunMem != 2) {   // free the string arguments copied so far (GetMemory(STRINGSIZE) each), or every error skipped
    		// by ON ERROR SKIP/IGNORE leaks them.  argtype[] follows argval[] in the block, as in DefinedSubFun().
    		union u_argval *av = (union u_argval *)DefinedSubFunMem;
    		int *at = (int *)((char *)av + MAX_ARG_COUNT * sizeof(union u_argval));
    		for(int i = 0; i < MAX_ARG_COUNT; i++)
    			if((at[i] & T_STR) && !(at[i] & T_PTR)) FreeMemorySafe((void *)&av[i].s);
    		FreeMemory((void*)DefinedSubFunMem);
    	}
    	DefinedSubFunMem=0;
    }
    if(OptionErrorSkip > 100000) {                                  // ON ERROR RESTART: what was printed goes out first
        uint32_t t = HAL_GetTick();
        while(ConsoleTxBufHead != ConsoleTxBufTail && HAL_GetTick() - t < 500) {}
        SoftReset();
    }
    if(OptionErrorSkip) {
    	SCB_CleanInvalidateDCache();
    	errpos=0;
        longjmp(ErrNext, 1);
    }
	int maxH=PageTable[WritePage].ymax;
    deferredcopy=0;
	if(Option.showstatus && CurrentY > maxH-(gui_font_height<<1)){
		MX470PutS("\r\n",WHITE,BLACK);
		CurrentY=maxH-(gui_font_height*2);
		ShortScroll=Option.showstatus;
	}
	FmNoteError(fmline);                                            // a program FM ran: FM, back, shows the error
	SCB_CleanInvalidateDCache();
    longjmp(mark, 1);
}


void SyntaxError(void)
{
    error("Invalid syntax");
}
const char *errorstring[] = {
    "",                                                            // 0
    "Not available on this display",                               // 1
    "Argument count",                                              // 2
    "PIO 0 not available",                                         // 3
    "PIO 1 not available",                                         // 4
    "PIO 2 not available",                                         // 5
    "Invalid variable",                                            // 6
    "Object % does not exist",                                     // 7
    "Invalid configuration",                                       // 8
    "Invalid pin",                                                 // 9
    "Invalid in a program",                                        // 10
    "Invalid on this display",                                     // 11
    "Pin not set for PWM",                                         // 12
    "No memory allocated for GUI controls",                        // 13
    "String length",                                               // 14
    "Overflow",                                                    // 15
    "Array size mismatch",                                         // 16
    "Array size",                                                  // 17
    "File number",                                                 // 18
    "File number is not open",                                     // 19
    "Expected a number",                                           // 20
    "Number out of bounds",                                        // 21
    "Cannot change a constant",                                    // 22
    "Destination array too small",                                 // 23
    "Already Set to pin %",                                        // 24
    "Library is using Slot % ",                                    // 25
    "% is invalid (valid is % to %)",                              // 26
    "Pin %/| is in use",                                           // 27
    "Insufficient data",                                           // 28
    "Not enough memory",                                           // 29
    "RTC not responding",                                          // 30
    "Already open",                                                // 31
    "Insufficient space in array",                                 // 32
    "Maximum % characters",                                        // 33
    "Coordinates",                                                 // 34
    "Argument 1 must be integer array",                            // 35
    "Unknown command",                                             // 36
    "Invalid buffer",                                              // 37
    "Frame buffer not created",                                    // 38
    "Variable > 255",                                              // 39
    "Invalid for this screen mode",                                // 40
    "Argument % must be a 5 element floating point array",         // 41
    "Pin $ is not off or an ADC input",                            // 42
    "Pin %/| is not off or an output",                             // 43
    "SYSTEM I2C not configured",                                   // 44
    "System SPI not configured",                                   // 45
    "Illegal escape sequence, use CHR$(0) for the Null character", // 46
    "Struct member arrays not supported for this command",         // 47
    "Not enough memory for % bytes",                               // 48
};
// For a command that keeps a pointer to the variable it was given, for an interrupt or a later statement to use
// (PID INIT): call it straight after the findvar or parse*array that found the variable (VarIndex), with what that
// returned.  A LOCAL's record is cleared and its data freed when its SUB returns, while the pointer would still be
// used.  A parameter or a STATIC is a local that points at another variable's data: it is allowed when that is a
// global's.  (The PicoMite's, 57b74f8; a global's record is found by its address here, not by its chunk.)
int IsGlobalData(void *data) {
    if(VarIndex >= MAXLOCALVARS) return true;                      // a global
    if(VREC(VarIndex)->type & T_PTR) {
        char *d = (char *)data;
        for(int i = MAXLOCALVARS; i < MAXVARS; i++) {
            struct s_vartbl *r = VREC(i);
            if(r->name[0] == 0 || r->type == T_BLOCKED) continue;
            if(r->val.s == d) return true;                         // a global array's or string's data, passed whole
            if(d >= (char *)r && d < (char *)(r + 1)) return true; // inside a global's record: a scalar, a short string
        }
    }
    return false;
}
void RequireGlobal(void *data) {
    if(!IsGlobalData(data)) error("Must be a global variable");
}

void StandardError(int n)
{
    error((char *)errorstring[n]);
}
void StandardErrorParam(int n, int m)
{
    error((char *)errorstring[n], m);
}
void StandardErrorParam2(int n, int m, int l)
{
    error((char *)errorstring[n], m, l);
}
void StandardErrorParamS(int n, char *m)
{
    error((char *)errorstring[n], m);
}

void StandardErrorParam3(int n, int m, int l, int h)
{
    error((char *)errorstring[n], m, l, h);
}

/***************************************************************************************************
 The data of the I2C, SPI and ONEWIRE READ and WRITE commands, as the PicoMite takes it. The caller has run getargs()
 and checked its own arguments before the data; 'dataidx' is the argv index of the first data argument and 'len' the
 number of items. The data is one of:
   1. a list, one an item: expressions to WRITE, variables or array elements to READ into;
   2. a whole numeric or integer array, with empty brackets (a()), from its first element;
   3. a string variable, with enough bytes to WRITE or room for those READ.
****************************************************************************************************/
// the 'len' values to send, into buf[0..len-1]
void GetCommsTxData(char *argv[], int argc, int dataidx, int len, unsigned int *buf)
{
    void *ptr = NULL;
    int i, t = 0;
    char *a = argc > dataidx ? argv[dataidx] : NULL;
    // one data argument may be a string variable or a whole array (a variable: not a function, nor an expression);
    // anything else, a single expression or array element too, is a list
    if(argc == dataidx + 1 && isnamestart(*a) && *skipvar(a, false) == 0 && !(FindSubFun(a, 1) >= 0 && strchr(a, '(') != NULL)) {
        ptr = findvar(a, V_NOFIND_NULL | V_EMPTY_OK);
        if(ptr != NULL) {
            t = FoundType();
            if(!((t & T_STR) && !emptyarray) &&
               !(emptyarray && !g_StructMemberType && (t & (T_NBR | T_INT)) && DimIsEnd(VREC(VarIndex)->dimtbl[1])))
                ptr = NULL;
        }
    }
    if(ptr == NULL) {                                               // a list of expressions
        if(len != ((argc - dataidx + 1) >> 1)) StandardError(2);
        for(i = 0; i < len; i++) buf[i] = getinteger(argv[dataidx + i + i]);
        return;
    }
    CHECK_STRUCT_MEMBER_ARRAY();
    if(t & T_STR) {                                                 // a string: its bytes
        unsigned char *cp = (unsigned char *)ptr;
        if(*cp < len) StandardError(28);
        for(i = 0; i < len; i++) buf[i] = cp[i + 1];
    } else if(t & T_NBR) {                                          // a whole array
        if(((MMFLOAT *)ptr - VREC(VarIndex)->val.fa) + len > DimElements(VREC(VarIndex)->dimtbl[0])) StandardError(28);
        for(i = 0; i < len; i++) buf[i] = FloatToInt32(((MMFLOAT *)ptr)[i]);
    } else {
        if(((long long int *)ptr - VREC(VarIndex)->val.ia) + len > DimElements(VREC(VarIndex)->dimtbl[0])) StandardError(28);
        for(i = 0; i < len; i++) buf[i] = (unsigned int)((long long int *)ptr)[i];
    }
}

// where 'len' received values go, checked and recorded in 'dest'
void GetCommsRxDest(char *argv[], int argc, int dataidx, int len, CommsRxDest *dest)
{
    void *ptr;
    int i, t;
    dest->len = len;
    if(argc > dataidx + 1) {                                        // a list of variables or array elements, one an item
        if(len != ((argc - dataidx + 1) >> 1)) StandardError(2);
        dest->list = GetTempMemory(len * sizeof(CommsRxItem));      // (found now: argv may be gone when they are set)
        for(i = 0; i < len; i++) {
            dest->list[i].ptr = findvar(argv[dataidx + i + i], V_FIND | V_EMPTY_OK);
            t = FoundType();
            if(VREC(VarIndex)->type & T_CONST) StandardError(22);
            if(emptyarray || !(t & (T_NBR | T_INT))) StandardError(6);   // (a whole array is not one item)
            dest->list[i].isint = (t & T_INT) ? 1 : 0;
        }
        dest->kind = COMMS_RXD_LIST;
        return;
    }
    ptr = findvar(argv[dataidx], V_FIND | V_EMPTY_OK);             // one: a string, a whole array, or one variable
    t = FoundType();
    if(VREC(VarIndex)->type & T_CONST) StandardError(22);
    if(ptr == NULL) StandardError(6);
    CHECK_STRUCT_MEMBER_ARRAY();
    if(t & T_STR) {
        if(emptyarray) StandardError(6);
        if(len < 1 || len > FoundSize()) StandardError(21);         // (its size: a LENGTH can make it shorter than 255)
        *(char *)ptr = len;
        dest->kind = COMMS_RXD_STRING;
        dest->ptr = (char *)ptr + 1;                                // past the length byte
    } else if(t & (T_NBR | T_INT)) {
        if(emptyarray) {                                            // a whole array
            if(g_StructMemberType || !DimIsEnd(VREC(VarIndex)->dimtbl[1])) StandardError(6);
            if((t & T_NBR) ? ((MMFLOAT *)ptr - VREC(VarIndex)->val.fa) + len > DimElements(VREC(VarIndex)->dimtbl[0])
                           : ((long long int *)ptr - VREC(VarIndex)->val.ia) + len > DimElements(VREC(VarIndex)->dimtbl[0]))
                StandardError(32);
        } else if(len != 1) StandardError(6);                       // one variable or element
        dest->kind = (t & T_INT) ? COMMS_RXD_INT : COMMS_RXD_FLOAT;
        dest->ptr = ptr;
    } else StandardError(6);
}

// the 'len' values received, from buf[], to where 'dest' says
void PutCommsRxData(CommsRxDest *dest, unsigned int *buf)
{
    int i;
    switch(dest->kind) {
    case COMMS_RXD_FLOAT:
        for(i = 0; i < dest->len; i++) ((MMFLOAT *)dest->ptr)[i] = (MMFLOAT)buf[i];
        break;
    case COMMS_RXD_INT:
        for(i = 0; i < dest->len; i++) ((long long int *)dest->ptr)[i] = (long long int)buf[i];
        break;
    case COMMS_RXD_STRING:
        for(i = 0; i < dest->len; i++) ((char *)dest->ptr)[i] = (char)buf[i];
        break;
    case COMMS_RXD_LIST:
        for(i = 0; i < dest->len; i++) {
            if(dest->list[i].isint) *((long long int *)dest->list[i].ptr) = (long long int)buf[i];
            else *((MMFLOAT *)dest->list[i].ptr) = (MMFLOAT)buf[i];
        }
        break;
    }
}




/**********************************************************************************************
 Routines to convert floats and integers to formatted strings
 These replace the sprintf() libraries with much less flash usage
**********************************************************************************************/

#define IntToStrBufSize 65

// convert a integer to a string.
// sstr is a buffer where the chars are to be written to
// sum is the number to be converted
// base is the numbers base radix (10 = decimal, 16 = hex, etc)
// if base 10 the number will be signed otherwise it will be unsigned
void IntToStr(char *strr, long long int nbr, unsigned int base) {
    int i, negative;
    unsigned char digit;
    unsigned long long int sum;
    extern long long int llabs (long long int n);

    unsigned char str[IntToStrBufSize];

    if(nbr < 0 && base == 10) {                                     // we can have negative numbers in base 10 only
        nbr = llabs(nbr);
        negative = true;
    } else
        negative = false;

    // this generates the digits in reverse order
    sum = (unsigned long long int) nbr;
    i = 0;
    do {
        digit = sum % base;
        if (digit < 0xA)
            str[i++] = '0' + digit;
        else
            str[i++] = 'A' + digit - 0xA;
        sum /= base;
    } while (sum && i < IntToStrBufSize);

    if(negative) *strr++ = '-';

    // we now need to reverse the digits into their correct order
    for(i--; i >= 0; i--) *strr++ = str[i];
    *strr = 0;
}


// convert an integer to a string padded with a leading character
// p is a pointer to the destination
// nbr is the number to convert (can be signed in which case the number is preceeded by '-')
// padch is the leading padding char (usually a space)
// maxch is the desired width of the resultant string (incl padding chars)
// radix is the base of the number.  Base 10 is signed, all others are unsigned
// Special case (used by FloatToStr() only):
//     if padch is negative and nbr is zero prefix the number with the - sign
void IntToStrPad(char *p, long long int nbr, signed char padch, int maxch, int radix) {
    int  j;
    char sign, buf[IntToStrBufSize];

    sign = 0;
    if((nbr < 0 && radix == 10)|| padch < 0) {                      // if the number is negative or we are forced to use a - symbol
        sign = '-';                                                 // set the sign
        if(nbr != LLONG_MIN) nbr *= -1;                             // convert to a positive nbr (-LLONG_MIN does not fit)
        padch = abs(padch);
    } else {
        if(nbr >= 0 && maxch < 0 && radix == 10)                    // should we display the + sign?
            sign = '+';
    }

    if(nbr == LLONG_MIN && radix == 10)
        strcpy(buf, "9223372036854775808");                         // its digits: negated it stayed negative and printed "--9223..."
    else
        IntToStr(buf, nbr, radix);
    j = abs(maxch) - strlen(buf);                                   // calc padding required
    if(j <= 0) j = 0;
    else mymemset(p, padch, abs(maxch));                              // fill the buffer with the padding char
	if(sign != 0) {                                                 // if we need a sign
		if(j == 0) j = 1;                                           // make space if necessary
	    if(padch == '0')
	        p[0] = sign;                                            // for 0 padding the sign is before the padding
	    else
	        p[j - 1] = sign;                                        // for anything else the padding is before the sign
	 }
    strcpy(&p[j], buf) ;
}


// convert a float to a string including scientific notation if necessary
// p is the buffer to store the string
// f is the number
// m is the nbr of chars before the decimal point (if negative print the + sign)
// n is the nbr chars after the point
//     if n == STR_AUTO_PRECISION we should automatically determine the precision
//     if n is negative always use exponential format
// ch is the leading pad char
void FloatToStr(char *p, MMFLOAT f, int m, int n, unsigned char ch) {
    int exp, trim = false, digit;
    MMFLOAT rounding;
    char *pp;

    if(isinf(f) || isnan(f)) {                                      // the code below cannot format these
        strcpy(p, isnan(f) ? "NAN" : (f > 0 ? "INF" : "-INF"));
        return;
    }
    ch &= 0x7f;                                                     // make sure that ch is an ASCII char
    if(f == 0)
        exp = 0;
    else
        exp = floor(log10(fabs(f)));                             // get the exponent part
    if(((fabs(f) < 0.0001 || fabs(f) >= 1000000) && f != 0 && n == STR_AUTO_PRECISION) || n < 0) {
        // we must use scientific notation
        f /= pow(10, exp);                                         // scale the number to 1.2345
        if(f >= 10) { f /= 10; exp++; }
        if(n < 0) n = -n;                                           // negative indicates always use exponantial format
        FloatToStr(p, f, m, n, ch);                                 // recursively call ourself to convert that to a string
        p = p + strlen(p);
        *p++ = 'e';                                                 // add the exponent
        if(exp >= 0) {
            *p++ = '+';
            IntToStrPad(p, exp, '0', 2, 10);                        // add a positive exponent
        } else {
            *p++ = '-';
            IntToStrPad(p, exp * -1, '0', 2, 10);                   // add a negative exponent
        }
    } else {
        // we can treat it as a normal number

        // first figure out how many decimal places we want.
        // n == STR_AUTO_PRECISION means that we should automatically determine the precision
        if(n == STR_AUTO_PRECISION) {
            trim = true;
            n = STR_SIG_DIGITS - exp;
            if(n < 0) n = 0;
        }

        // calculate rounding to hide the vagaries of floating point
        if(n > 0)
            rounding = 0.5/pow(10, n);
        else
            rounding = 0.5;
        if(f > 0) f += rounding;                                    // add rounding for positive numbers
        if(f < 0) f -= rounding;                                    // add rounding for negative numbers

        // convert the digits before the decimal point
        if((int)f == 0 && f < 0)
            IntToStrPad(p, 0, -ch, m, 10);                          // convert -0 incl padding if necessary
        else
            IntToStrPad(p, f, ch, m, 10);                           // convert the integer incl padding if necessary
        p += strlen(p);                                             // point to the end of the integer
        pp = p;

        // convert the digits after the decimal point
        if(f < 0) f = -f;                                           // make the number positive
        if(n > 0) {                                                 // if we need to have a decimal point and following digits
            *pp++ = '.';                                            // add the decimal point
            f -= floor(f);                                         // get just the fractional part
            while(n--) {
                f *= 10;
                digit = floor(f);                                  // get the next digit for the string
                f -= digit;
                *pp++ = digit + '0';
            }

            // if we do not have a fixed number of decimal places step backwards removing trailing zeros and the decimal point if necessary
            while(trim && pp > p) {
                pp--;
                if(*pp == '.') break;
                if(*pp != '0') { pp++; break; }
            }
        }
        *pp = 0;
    }
}



/**********************************************************************************************
Various routines to clear memory or the interpreter's state
**********************************************************************************************/


// clear (or delete) variables
// if level is not zero it will only delete local variables at that level or greater
// if level is zero to will delete all variables and reset global settings
// P6 F2: after ClearVars freed a local below one that stays (a deeper level's local, left by an error, under a
// local of the level that goes on): keep the ones that stay, in order.  The list's length.
static int LocalListCompact(void) {
    int i, top;
    for(i = 0, top = 0; i < hashlistpointer; i++)
        if(hashlist[i].level != -1) hashlist[top++] = hashlist[i];
    return top;
}

void MIPS16 ClearVars(int level) {
    int i, top, compact, hashcurrent;

    // first delete the local variables at that level or greater: the top of the local stack, newest first (P6 F2)
	if(level){
		top = hashlistpointer;
		compact = 0;
		for(i=hashlistpointer-1;i>=0;i--){
			if(hashlist[i].level>= level){
				hashcurrent = hashlist[i].hash;
				if(((VREC(hashcurrent)->type & (T_STR | T_STRUCT)) || DimIsAllocated(VREC(hashcurrent)->dimtbl[0])) && !(VREC(hashcurrent)->type & T_PTR) && !VAR_INLINE_STR(VREC(hashcurrent)) && ((uint32_t)VREC(hashcurrent)->val.s>0x24000000)) {
					FreeMemorySafe((void *)&VREC(hashcurrent)->val.s);
					// free any memory (if allocated)
				}
				hashlist[i].level=-1;
				SymLocalFreed(hashcurrent);                         // its binding back to what it hid (newest first: Symbols.h)
				clearvar(VREC(hashcurrent));
				Localvarcnt--;
				if(i == top - 1) top--;                             // still the top of the stack
				else compact = 1;                                   // below one that stays: a deeper level's local left by an error
			}
		}
		if(compact) top = LocalListCompact();
		hashlistpointer = top;
		localtop = top ? hashlist[top - 1].hash + 1 : 0;
	} else {
		for(i = 0; i < MAXVARS; i++) {
			if(!VarRecordReal(VREC(i))) continue;                   // a free or blocked global slot: a placeholder, nothing of its own
			if(((VREC(i)->type & (T_STR | T_STRUCT)) || DimIsAllocated(VREC(i)->dimtbl[0])) && !(VREC(i)->type & T_PTR) && !VAR_INLINE_STR(VREC(i))) {
				if((uint32_t)VREC(i)->val.s>0x24000000) FreeMemorySafe((void *)&VREC(i)->val.s);                        // free any memory (if allocated)
			}
			if(i < MAXLOCALVARS) clearvar(VREC(i));                 // (a global's record goes back with its chunk)
		}
		VarRecordsReset();                                          // every global slot free again, the records to be used again
		SymBindReset();                                             // and every variable binding
	}
   // then step through the for...next table and remove any loops at the level or greater
    for(i = 0; i < forindex; i++) {
        if(forstack[i].level >= level) {
            forindex = i;
            break;
        }
    }

    // also step through the do...loop table and remove any loops at the level or greater
    for(i = 0; i < doindex; i++) {
        if(dostack[i].level >= level) {
            doindex = i;
            break;
        }
    }

    if(level != 0) return;

    forindex = doindex = 0;
    LocalIndex = 0;                                                 // signal that all space is to be cleared
    ClearTempMemory();                                              // clear temp string space
    // we can now delete all variables by zeroing the counters
    Localvarcnt = 0;
    Globalvarcnt = 0;
    OptionBase = 0;
    DimUsed = false;
    hashlistpointer=0;
    localtop = 0;
}


// clear all stack pointers (eg, FOR/NEXT stack, DO/LOOP stack, GOSUB stack, etc)
// this is done at the command prompt or at any break
void MIPS16 ClearStack(void) {
    NextData = 0;
	NextDataLine = (char *)ProgMemory;
    forindex = 0;
    doindex = 0;
    gosubindex = 0;
    LocalIndex = 0;
    TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
    InterruptReturn = NULL;
}

extern int VideoColour;

// clear the runtime (eg, variables, external I/O, etc) includes ClearStack() and ClearVars()
// this is done before running a program
extern char *CSubInterrupt;
extern volatile char CSubComplete;
extern unsigned int CFuncInt1, CFuncInt2, CFuncInt3, CFuncInt4, CFuncmSec;
static void MIPS16 ClearRuntimeAll(int keep) {
    int i;
#ifdef PCSAMPLER
    PcsAbandon();                                                   // its tables are in the heap about to go
#endif
    //have to stop audio before we clear variables to avoid exception
	CloseAudio(1);
	vol_left = 100; vol_right = 100;
	vol_target_left = 100; vol_target_right = 100;
	CloseAllFiles();  //also clears sprites and 3D objects
    OptionFileErrorAbort = true;
    ClearStack();
    if(keep) {
        RamLibRelease();                                            // (a RAM library goes with the heap)
        ClearVars(1);                                               // CHAIN: the locals go, the globals stay where they are
        InitHeapKeepGlobals();                                      // and so do the blocks they own
    } else {
        void *lb;
        int ls;
        ClearVars(0);
        if(RamLibBlock(&lb, &ls)) InitHeapKeeping(&lb, &ls, 1);    // LIBRARY LOAD's restart: the RAM library stays
        else {
            RamLibRelease();                                        // RUN, NEW, a load: a RAM library goes with the heap
	        InitHeap();
        }
    }
    OptionExplicit = false;
    OptionEscape = false;
    DefaultType = T_NBR;
    SymBindGlobalsChanged();                                        // what an unsuffixed name may bind to (Symbols.h)
    ds18b20Timers = NULL;                                           // InitHeap() will recover the memory allocated to this array
    findlabel(NULL);                                                // clear the label cache
    ClearExternalIO();                                              // this MUST come before InitHeap()
    CSubInterrupt = NULL; CSubComplete = 0;                         // INTERRUPT's SUB and the CSUBs' interrupt vectors go with
    CFuncInt1 = CFuncInt2 = CFuncInt3 = CFuncInt4 = CFuncmSec = 0;  // the program (a pin or the 1 ms tick called them after NEW)
    OptionErrorSkip = 0;
    MMerrline = 0;
    MMerrno = 0;                                                    // (MM.ERRNO too, as MM.ERRLINE and MM.ERRMSG$: the PicoMite's)
    FunReturnArrayCount = 0;
    InMMEND = false;
    *MMErrMsg = 0;
    if(!keep) m_alloc(0);
    varcnt = keep ? Globalvarcnt : 0;
    for(i=0;i<7;i++)KeyDown[i]=0;
    CurrentLinePtr = ContinuePoint = NULL;
    for(i = 0;  i < MAXSUBFUN; i++)  subfun[i] = NULL;
	clearrepeat();
//    turtle_init_not_done=1;
//    main_turtle_polyX=NULL;
//    main_turtle_polyY=NULL;
    PageTable[WPN].address=NULL;
    PageTable[BPN].address=NULL;
    SDMemory=NULL;
    cursorsave=NULL;
    loadcursordata=NULL;
	optionangle=1.0;
	optiony=0;
	cursoron=0;
	cursorenable=0;
//	GuiCall=0;
	CMM1 = false;
	hashlistpointer=0;
	for(i=0;i<TRACE_BUFF_SIZE;i++)TraceBuff[i]=0;
	TraceBuffIndex=0;
	SetFont(Option.DefaultFont);
    memset(datastore, 0, sizeof(struct sa_data) * MAXRESTORE);
    restorepointer = 0;
//Refresh the tokens to get them into cache
    tokenTHEN  = GetTokenValue("Then");
    tokenELSE  = GetTokenValue("Else");
    tokenGOTO  = GetTokenValue("GoTo");
    tokenEQUAL = GetTokenValue("=");
    tokenTO    = GetTokenValue("To");
    tokenSTEP  = GetTokenValue("Step");
    tokenWHILE = GetTokenValue("While");
    tokenUNTIL = GetTokenValue("Until");
    tokenGOSUB = GetTokenValue("GoSub");
    tokenAS    = GetTokenValue("As");
    tokenFOR   = GetTokenValue("For");
    cmdLOOP  = GetCommandValue("Loop");
    cmdIF      = GetCommandValue("If");
    cmdENDIF   = GetCommandValue("EndIf");
    cmdELSEIF  = GetCommandValue("ElseIf");
    cmdELSE    = GetCommandValue("Else");
    cmdSELECT_CASE = GetCommandValue("Select Case");
    cmdCASE        = GetCommandValue("Case");
    cmdCASE_ELSE   = GetCommandValue("Case Else");
    cmdEND_SELECT  = GetCommandValue("End Select");
    cmdWHILE = GetCommandValue("While");
	cmdSUB = GetCommandValue("Sub");
	cmdFUN = GetCommandValue("Function");
    cmdIRET = GetCommandValue("IReturn");
    cmdCSUB = GetCommandValue("CSub");
    cmdTYPE = GetCommandValue("Type");
    cmdEND_TYPE = GetCommandValue("End Type");
    cmdSTRUCT = GetCommandValue("Struct");
    cmdLOCAL = GetCommandValue("Local");
    cmdSTATIC = GetCommandValue("Static");
    cmdENDSUB= GetCommandValue("End Sub");
    cmdENDFUNCTION = GetCommandValue("End Function");
    cmdDO=  GetCommandValue("Do");
    cmdFOR=  GetCommandValue("For");
    cmdNEXT= GetCommandValue("Next");
}

void MIPS16 ClearRuntime(void) {
    ClearRuntimeAll(false);
}

// CHAIN: as ClearRuntime, but the global variables stay (the locals go)
void MIPS16 ClearRuntimeKeepGlobals(void) {
    ClearRuntimeAll(true);
}

/* SAVE CONTEXT [CLEAR] and LOAD CONTEXT [KEEP] (the PicoMite's): the global variables, each one's record and its data,
   in A:/.vars on the SD card (the PicoMite keeps a copy of its whole heap in PSRAM or on its A: drive). Only the
   globals: the locals belong to the calls running now. LOAD CONTEXT deletes the globals there are and makes the
   saved ones again through findvar, so each is laid out as any other; one of a structure type refers to the type
   by its number, so the program must define the same TYPEs in the same order. Both are for the program's top level:
   a SUB's parameter that refers to a global would refer to nothing after LOAD CONTEXT. */
#define CONTEXT_FILE "A:/.vars"
#define CONTEXT_MAGIC 0x31585443                                    // "CTX1"
struct s_ctxhead { uint32_t magic; int count, optionbase, dimused; };

// a global SAVE CONTEXT writes
static int CtxSaved(struct s_vartbl *r) {
    return VarRecordReal(r) && r->name[0] && r->type != T_NOTYPE && r->type != T_BLOCKED && !(r->type & T_PTR) && !DimIsEmptyParam(r->dimtbl[0]);
}

// the bytes of data a global holds outside its record's value: an array's elements, a string, a structure
static int CtxDataBytes(struct s_vartbl *r) {
    int esize, nbr = 1, d;
    if(r->type & T_STR) esize = r->size + 1;
    else if(r->type & T_STRUCT) {
        if(r->size >= g_structcnt) error("Invalid structure type");
        esize = g_structtbl[r->size]->total_size;
    } else esize = 8;
    if(!DimIsRealArray(r->dimtbl[0])) return (r->type & (T_STR | T_STRUCT)) ? esize : 0;
    for(d = 0; d < MAXDIM && !DimIsEnd(r->dimtbl[d]); d++) nbr *= DimElements(r->dimtbl[d]);
    return nbr * esize;
}

// a FatFs error: the file closed, then the error
static void CtxCheck(FIL *f, FRESULT r) {
    if(r == FR_OK) return;
    f_close(f);
    FSerror = r;
    ErrorCheck(0);
    error("Context file");
}

// n bytes to or from the file, through a buffer of its own (the data may be anywhere: SDRAM, a record)
static void CtxXfer(FIL *f, void *p, int n, int out) {
    char buf[512];
    UINT done;
    while(n > 0) {
        int k = n > (int)sizeof(buf) ? (int)sizeof(buf) : n;
        if(out) {
            memcpy(buf, p, k);
            CtxCheck(f, f_write(f, buf, k, &done));
            if((int)done != k) { f_close(f); error("Not enough space on the SD card"); }
        } else {
            CtxCheck(f, f_read(f, buf, k, &done));
            if((int)done != k) { f_close(f); error("Invalid context file"); }
            memcpy(p, buf, k);
        }
        p = (char *)p + k;
        n -= k;
    }
}

// every global deleted, the locals kept (LOAD CONTEXT, SAVE CONTEXT CLEAR)
void ClearGlobals(void) {
    int i;
    for(i = MAXLOCALVARS; i < MAXVARS; i++) {
        struct s_vartbl *r = VREC(i);
        if(VarRecordReal(r) && VarOwnsBlock(r) && (uint32_t)r->val.s > 0x24000000) FreeMemorySafe((void *)&r->val.s);
    }
    VarRecordsReset();                                              // (the locals' slots are theirs again: unchanged)
    SymBindResetGlobals();
    Globalvarcnt = 0;
    varcnt = Localvarcnt;
}

void SaveContext(void) {
    FIL f;
    struct s_ctxhead h = {CONTEXT_MAGIC, 0, OptionBase, DimUsed};
    int i, n;
    if(!InitACard()) return;                                       // (A: whatever the current drive)
    for(i = MAXLOCALVARS; i < MAXVARS; i++) if(CtxSaved(VREC(i))) h.count++;
    CtxCheck(&f, f_open(&f, CONTEXT_FILE, FA_WRITE | FA_CREATE_ALWAYS));
    CtxXfer(&f, &h, sizeof(h), 1);
    for(i = MAXLOCALVARS; i < MAXVARS; i++) {
        struct s_vartbl *r = VREC(i);
        if(!CtxSaved(r)) continue;
        n = CtxDataBytes(r);
        CtxXfer(&f, r, sizeof(*r), 1);
        CtxXfer(&f, &n, sizeof(n), 1);                              // (its data's length: a TYPE of another size is caught)
        if(n > 0) CtxXfer(&f, r->val.s, n, 1);
    }
    CtxCheck(&f, f_close(&f));
}

void RestoreContext(int keep) {
    FIL f;
    struct s_ctxhead h;
    struct s_vartbl r;
    char text[MAXVARLEN + 8 * (MAXDIM + 1) + 16];
    int i, d, n, SaveLocalIndex = LocalIndex;
    if(!InitACard()) return;
    FRESULT fr = f_open(&f, CONTEXT_FILE, FA_READ);
    if(fr == FR_NO_FILE) error("No context saved");
    CtxCheck(&f, fr);
    CtxXfer(&f, &h, sizeof(h), 0);
    if(h.magic != CONTEXT_MAGIC || h.count < 0 || h.count > MAXGLOBALVARS) { f_close(&f); error("Invalid context file"); }
    ClearGlobals();
    OptionBase = h.optionbase;                                      // (the dimensions are as they were saved)
    LocalIndex = 0;                                                 // findvar makes globals, past any local of the same name
    for(i = 0; i < h.count; i++) {
        int len;
        CtxXfer(&f, &r, sizeof(r), 0);
        CtxXfer(&f, &len, sizeof(len), 0);
        n = strnlen(r.name, MAXVARLEN);                             // the name, its dimensions, a string's length
        memcpy(text, r.name, n);
        if(DimIsRealArray(r.dimtbl[0])) {
            for(d = 0; d < MAXDIM && !DimIsEnd(r.dimtbl[d]); d++) {
                text[n++] = d ? ',' : '(';
                IntToStr(text + n, DimUpper(r.dimtbl[d]), 10);
                n += strlen(text + n);
            }
            text[n++] = ')';
        }
        text[n] = 0;
        if(r.type & T_STR) {
            strcat(text, " LENGTH ");
            IntToStr(text + strlen(text), r.size, 10);
        }
        g_StructArg = (r.type & T_STRUCT) ? r.size : -1;
        void *p = findvar(text, V_FIND | V_DIM_VAR | T_IMPLIED | (r.type & (T_NBR | T_INT | T_STR | T_STRUCT)));
        g_StructArg = -1;
        struct s_vartbl *v = VREC(VarIndex);
        if(CtxDataBytes(v) != len) { f_close(&f); error("$ is not as it was saved (its TYPE?)", text); }
        if(len > 0) CtxXfer(&f, p, len, 0);
        else v->val = r.val;                                        // a number
        v->type = r.type & (T_NBR | T_INT | T_STR | T_STRUCT | T_IMPLIED | T_CONST);
    }
    LocalIndex = SaveLocalIndex;
    DimUsed = h.dimused;
    CtxCheck(&f, f_close(&f));
    if(!keep) f_unlink(CONTEXT_FILE);
}



// clear everything including program memory (includes ClearStack() and ClearRuntime())
// this is used before loading a program
void MIPS16 ClearProgram(void) {
    ClearRuntime();
    TraceOn = false;
    sendCRLF=3;
}



// round a float to an integer
int FloatToInt32(MMFLOAT x) {
    if(isnan(x) || x < LONG_MIN - 0.5 || x > LONG_MAX + 0.5)          // NaN fails every comparison
        error("Number too large");
    return (x >= 0 ? (int)(x + 0.5) : (int)(x - 0.5)) ;
}



long long int FloatToInt64(MMFLOAT x) {
    // NaN fails every comparison, and 2^63 itself is out of range: as a double the old bound
    // 0x7fffffffffffffff + 0.5 rounded to 2^63 and let it through
    if(isnan(x) || x < -9223372036854775808.0 || x >= 9223372036854775808.0)
        error("Number too large");
    if ((x < -0xfffffffffffff) || (x > 0xfffffffffffff))
        return (long long int)(x);
    else
        return (x >= 0 ? (long long int)(x + 0.5) : (long long int)(x - 0.5)) ;
}



// make a string uppercase
void makeupper(char *p) {
    while(*p) {
        *p = toupper(*p);                                           // (it copied each character to itself)
        p++;
    }
}


// find the value of a command token given its name
int GetCommandValue(char *n) {
    int i;
    for(i = 0; i < CommandTableSize - 1; i++)
        if(str_equal(n, (char *)commandtbl[i].name))
            return i;                                               // the index: commandtbl_put() makes the token
//    MMPrintString(n);
    error("Internal fault 1(sorry)");
    return 0;
}



// find the value of a token given its name
int GetTokenValue(char *n) {
    int i;
    for(i = 0; i < TokenTableSize - 1; i++)
        if(str_equal(n, (char *)tokentbl[i].name))
            return i + C_BASETOKEN;
//    MMPrintString(n);
    error("Internal fault 2(sorry)");
    return 0;
}



// skip to the end of a variable
char ITCM_CODE *skipvar(char *p, int noerror) {
    char *pp, *tp;
    int i;
    int inquote = false;

    int adj = 0;                                                    // a symbol: its spelling's length less its bytes

    tp = p;
    // check the first char for a legal variable name
    skipspace(p);
    if(issymbol(*p)) {                                              // a symbol (Symbols.h): its length matters to the tests
        if(SymLongest > MAXVARLEN - 8) {                            // below only when a spelling could be that long
            int nl;
            SymSpelling(p, &nl);
            adj = nl - symbolsize(*p);
        }
        p += symbolsize(*p);
    } else {
        if(!isnamestart(*p)) return tp;
        do {
            p++;
        } while(isnamechar(*p));
    }

    // check the terminating char.
    if(*p == '$' || *p == '%' || *p == '!') p++;

    if(p - tp + adj > MAXVARLEN) {
        if(noerror) return p;
        error("Variable name too long");
    }

    pp = p; skipspace(pp); if(*pp == '(') p = pp;
    if(*p == '(') {
        // this is an array

        p++;
        if(p - tp + adj > MAXVARLEN) {
            if(noerror) return p;
            error("Variable name too long");
        }

        // step over the parameters keeping track of nested brackets
        i = 1;
        while(1) {
            if(*p == '\"') inquote = !inquote;
            if(*p == 0) {
                if(noerror) return p;
                error("Expected closing bracket");
            }
            if(!inquote) {
                if(*p == ')') if(--i == 0) break;
                if(*p == '(' || (tokentype(*p) & T_FUN)) i++;
            }
            p++;
        }
        p++;        // step over the closing bracket
    }

    // a structure's member, perhaps an element of it, perhaps more members: pts(0).x, p.a(1).b.  Whether the
    // variable is a structure is not known here (the tokeniser finds the = of pts(0).x = 1 with this), but
    // nothing else can follow a variable with a '.'
    while(1) {
        pp = p; skipspace(pp);
        if(*pp != '.') break;
        p = pp + 1;
        if(issymbol(*p))
            p += symbolsize(*p);                                    // a member's symbol
        else {
            if(!isnamestart(*p)) {
                if(noerror) return p;
                error("Expected member name after '.'");
            }
            do {
                p++;
            } while(isnamechar(*p));
        }
        if(*p == '$' || *p == '%' || *p == '!') p++;
        pp = p; skipspace(pp);
        if(*pp == '(') {
            p = pp + 1;
            i = 1;
            inquote = false;
            while(1) {
                if(*p == '\"') inquote = !inquote;
                if(*p == 0) {
                    if(noerror) return p;
                    error("Expected closing bracket");
                }
                if(!inquote) {
                    if(*p == ')') if(--i == 0) break;
                    if(*p == '(' || (tokentype(*p) & T_FUN)) i++;
                }
                p++;
            }
            p++;
        }
    }
    return p;
}



// skip to the end of an expression (terminates on null, comma, comment or unpaired ')'
char *skipexpression(char *p) {
    int i, inquote;

    for(i = inquote = 0; *p; p++) {
        if(*p == '\"') inquote = !inquote;
        if(!inquote) {
            if(*p == ')') i--;
            if(*p == '(' || (tokentype(*p) & T_FUN)) i++;
        }
        if(i < 0 || (i == 0 && (*p == ',' || *p == '\''))) break;
    }
    return p;
}


// find the next command in the program
// this contains the logic for stepping over a line number and label (if present)
// p is the current place in the program to start the search from
// CLine is a pointer to a char pointer which in turn points to the start of the current line for error reporting (if NULL it will be ignored)
// EOFMsg is the error message to use if the end of the program is reached
// returns a pointer to the next command
char *GetNextCommand(char *p, char **CLine, char *EOFMsg) {
    do {
        if(*p != T_NEWLINE) {                                       // if we are not already at the start of a line
            while(*p) p++;                                          // look for the zero marking the start of an element
            p++;                                                    // step over the zero
        }
        if(*p == 0) {
            if(EOFMsg == NULL) return p;
            error(EOFMsg);
        }
        if(*p == T_NEWLINE) {
            if(CLine) *CLine = p;                                   // and a pointer to the line also for error reporting
            p += T_NEWLINE_HDR;                                     // step over the line's header
        }
        if(*p == T_LINENBR) p += 3;

        skipspace(p);
        if(p[0] == T_LABEL) {                                       // got a label
            p += p[1] + 2;                                          // skip over the label
            skipspace(p);                                           // and any following spaces
        }
    } while(*p < C_BASETOKEN);
    return p;
}


// scans text looking for the matching closing bracket
// it will handle nested strings, brackets and functions
// it expects to be called pointing at the opening bracket or a function token
char ITCM_CODE *getclosebracket(char *p) {
    int i = 0;
    int inquote = false;

    do {
        if(*p == 0) error("Expected closing bracket");
        if(*p == '\"') inquote = !inquote;
        if(!inquote) {
            if(*p == ')') i--;
            if(*p == '(' || (tokentype(*p) & T_FUN)) i++;
        }
        p++;
    } while(i);
    return p - 1;
}


// NAME(args) = value, for a command whose token carries its '(' (MID$(, LMID(, BITBYTEFLAG(, PIN(, PORT(, MAP(,
// CTRLVAL(): p is just after the token. The value's text, after the ')' that matches it and the '=' straight after
// that; msg is the command's message for anything else. A '=' inside the brackets is a comparison
// (CHOICE(k = 1, 1, 2)): the scans for the statement's first '=' took it (the PicoMite's issue #101).
char *GetAssignedValue(char *p, char *msg) {
    int i = 1, inquote = false;
    for(;; p++) {
        if(*p == 0) error(msg);
        if(*p == '\"') inquote = !inquote;
        else if(!inquote) {
            if(*p == ')') { if(--i == 0) break; }
            else if(*p == '(' || (tokentype(*p) & T_FUN)) i++;
        }
    }
    p++;
    skipspace(p);
    if(tokenfunction(*p) != op_equal) error(msg);
    p++;
    skipspace(p);
    if(*p == 0 || *p == '\'') error(msg);
    return p;
}


// check that there is no excess text following an element
// will skip spaces and abort if a zero char is not found
void checkend(char *p) {
    skipspace(p);
    if(*p == '\'') return;
    if(*p)
        error("Unexpected text: $", p);
}


// check if the next text in an element (a basic statement) corresponds to an alpha string
// leading white space is skipped and the string must be terminated with a valid terminating
// character (space, null, comma or comment). Returns a pointer a pointer to the next
// non space character after the matched string if found or NULL is not

char *checkstring(char *p, char *tkn) {
    skipspace(p);                                           // skip leading spaces
    while(*tkn) {                                                   // compare the strings
        if(issymbol(*p)) {
            // a name stored as a symbol matches if its spelling is the next part of the keyword (see Symbols.h)
            const char *s;
            int len;
            s = SymSpelling(p, &len);
            while(len && *tkn && toupper(*tkn) == toupper(*s)) { tkn++; s++; len--; }
            if(len) return NULL;                                    // the name differs, or goes on past the keyword
            p += symbolsize(*p);
            continue;
        }
        // blind to case, as the PicoMite's: a DATA statement keeps its case (READ evaluated DATA RGB(red) and found no
        // colour), and so do strings (22 Oct 2026, a user's report). Elsewhere tokenise() has made the text capitals.
        if(toupper((unsigned char)*tkn) != toupper((unsigned char)*p)) break;
        tkn++; p++;
    }
    if(*tkn == 0 && (*p == ' ' || *p == ',' || *p == '\'' || *p == '(' || *p == 0)) {
        skipspace(p);
        return p;                                                   // if successful return a pointer to the next non space character after the matched string
    }
    return NULL;                                                    // or NULL if not
}

// the keyword tkn as the whole of an argument, or of the rest of a statement (a comment may follow): L, but not L(1)
// or L + x$, which checkstring matches as L and more (the PicoMite's #107 and #112: an expression taken for the word)
int checkword(char *p, char *tkn) {
    char *tp = checkstring(p, tkn);
    return tp != NULL && (*tp == 0 || *tp == '\'');
}


/********************************************************************************************************************************************
A couple of I/O routines that do not belong anywhere else
*********************************************************************************************************************************************/


// print a string to the console interfaces
void MMPrintString(char* s) {
	if(!CurrentLinePtr)ShortScroll=Option.showstatus;
	while(*s) {
		MMputchar(*s);
		s++;
	}
}


// output a string to a file
// the string must be a MMBasic string
void MMfputs(char *p, int filenbr) {
	int i;
	i = *p++;
    if(FileTable[filenbr].com > MAXCOMPORTS){
    	FilePutStr(i, p, filenbr);
    } else {
    	while(i--) MMfputc(*p++, filenbr);
    }
}





/********************************************************************************************************************************************
 string routines
 these routines form a library of functions for manipulating MMBasic strings.  These strings differ from ordinary C strings in that the length
 of the string is stored in the first byte and the string is NOT terminated with a zero valued byte.  This type of string can store the full
 range of binary values (0x00 to 0xff) in each character.
*********************************************************************************************************************************************/

// convert a MMBasic string to a C style string
// if the MMstr contains a null byte that byte is skipped and not copied
char *MtoC(char *p) {
    int i;
    char *p1, *p2;
    i = *p;
    p1 = p + 1; p2 = p;
    while(i) {
        if(p1) *p2++ = *p1;
        p1++;
        i--;
    }
    *p2 = 0;
    return p;
}


// convert a c style string to a MMBasic string
char *CtoM(char *p) {
    int len, i;
    char *p1, *p2;
    len = i = strlen(p);
    if(len > MAXSTRLEN) error("String too long1");
    p1 = p + len; p2 = p + len - 1;
    while(i--) *p1-- = *p2--;
    *p = len;
    return p;
}


// copy a MMBasic string to a new location
void Mstrcpy(char *dest, char *src) {
    int i;
    i = *src + 1;
    mycpy(dest,src,i);
//    while(i--) *dest++ = *src++;
}



// concatenate two MMBasic strings
void Mstrcat(char *dest, char *src) {
    int i;
    i = *src;
    *dest += i;
    dest += *dest + 1 - i; src++;
    while(i--) *dest++ = *src++;
}



// compare two MMBasic style strings
// returns 1 if s1 > s2  or  0 if s1 = s2  or  -1 if s1 < s2
int Mstrcmp(char *s1, char *s2) {
    register int i;
    register char *p1, *p2;

    // get the smaller length
    i = *s1 < *s2 ? *s1 : *s2;

    // skip the length byte and point to the char array
    p1 = s1 + 1; p2 = s2 + 1;

    // compare each char
    while(i--) {
        if(*p1 > *p2) return 1;
        if(*p1 < *p2) return -1;
        p1++; p2++;
    }
    // up to this point the strings matched - make the decision based on which one is shorter
    if(*s1 > *s2) return 1;
    if(*s1 < *s2) return -1;
    return 0;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// these library functions went missing in the PIC32 C compiler ver 1.12 and later
////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * strncasecmp.c --
 *
 *  Source code for the "strncasecmp" library routine.
 *
 * Copyright (c) 1988-1993 The Regents of the University of California.
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: strncasecmp.c,v 1.3 2007/04/16 13:36:34 dkf Exp $
 */

/*
 * This array is designed for mapping upper and lower case letter together for
 * a case independent comparison. The mappings are based upon ASCII character
 * sequences.
 */


/*
 *----------------------------------------------------------------------
 *
 * strncasecmp --
 *
 *  Compares two strings, ignoring case differences.
 *
 * Results:
 *  Compares up to length chars of s1 and s2, returning -1, 0, or 1 if s1
 *  is lexicographically less than, equal to, or greater than s2 over
 *  those characters.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

// Compare two strings, ignoring case differences.
// Returns true if the strings are equal (ignoring case) otherwise returns false.
int __attribute__((always_inline)) inline str_equal(char *s1, char *s2) {
    if((*s1) != (*s2)) return 0;
    for ( ; ; ) {
        if(*s2 == '\0') {
            if(*s1 == '\0') return 1;
            else return 0;
        }
        s1++; s2++;
        if((*s1) != (*s2)) return 0;
    }
}


