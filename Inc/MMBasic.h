/***************************************************************************
CMM2 MMBasic
MMBasic.h

Include file that contains the globals and defines for MMBasic.c in MMBasic.

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

#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

	#include "Configuration.h"

// Types used to define an item of data.  Often they are ORed together.
// Used in tokens, variables and arguments to functions
#define T_NOTYPE       0                            // type not set or discovered
#define T_NBR       0x01                            // number (or float) type
#define T_STR       0x02                            // string type
#define T_INT       0x04                            // 64 bit integer type
#define T_PTR       0x08                            // the variable points to another variable's data
#define T_IMPLIED   0x10                            // the variables type does not have to be specified with a suffix
#define T_CONST     0x20                            // the contents of this variable cannot be changed
#define T_BLOCKED   0x40                            // Hash table entry blocked after ERASE
#define T_STRUCT    0x80                            // a structure (TYPE ... END TYPE): its record's size is the type's number

#define TypeMask(a) ((a) & (T_NBR | T_INT | T_STR | T_STRUCT)) // macro to isolate the variable type bits

// types of tokens.  These are or'ed with the data types above to fully define a token
#define T_INV       0                               // an invalid token
#define T_NA        0                               // an invalid token
#define T_CMD       0x10                            // a command
#define T_OPER      0x20                            // an operator
#define T_FUN       0x40                            // a function (also used for a function that can operate as a command)
#define T_FNA       0x80                            // a function that has no arguments

#define C_BASETOKEN 0x80                            // the base of the token numbers

// flags used in the program lines
#define T_CMDEND    0                               // end of a command
#define T_NEWLINE   1                               // the start of a line: the first byte of its header
#define T_LINENBR   2                               // three bytes for a line number
#define T_LABEL     3                               // variable length indicating a label (the PicoMite's value: 0x04-0x07
                                                    // are its symbol introducers)

// Every line of a program image starts with a header of T_NEWLINE_HDR bytes:
//   T_NEWLINE, skip, file, line_hi, line_lo
// skip is the distance to the next line's T_NEWLINE (or 0xFE when that is more than 0xFD), file the
// index of its source file (1 = the main program, then the #INCLUDE files in the order they appear),
// and line its line in that file.  The four bytes after T_NEWLINE are never 0, 1-4 or 0xFF, the values
// that mark structure (end of element, T_NEWLINE, T_LINENBR, T_LABEL, end of program): code that walks
// an image byte by byte looking for them steps over a header unharmed.  file and the line bytes are
// stored as LHDR_MIN + value, the line number in base 250.  A line number of 0 marks the comment lines
// at the top of the image that hold the file names; on the first it is also the format's stamp.
// A name line is ' then IMAGE_TAG then the image format IMAGE_FORMAT, then the file name: format 1 (the
// header of 15 Oct) had ' then the name straight away, with T_LABEL 4.
#define IMAGE_TAG     '*'                           // (a character no FAT file name holds)
#define IMAGE_FORMAT  '5'                           // 2: T_LABEL 3; 3: the function-token folds (SCHANGE$( TOPBOTTOM(); 4: ERASE, I2C2... as commands; 5: LMID, SYNC, LINPUT(
#define IMAGE_NAME    3                             // the file name's offset after the header: ' IMAGE_TAG IMAGE_FORMAT
#define T_NEWLINE_HDR 5
#define LHDR_MIN      5                             // the smallest value a header byte after T_NEWLINE takes
#define LHDR_MAXLINE  62499                         // the largest line number a header can hold
#define LHDR_SKIPMAX  0xFD                          // a longer line stores LHDR_SKIPMAX + 1
#define LHDR_SKIP(p)  ((unsigned char)(p)[1])
#define LHDR_FILE(p)  ((unsigned char)(p)[2] - LHDR_MIN + 1)
#define LHDR_LINE(p)  (((unsigned char)(p)[3] - LHDR_MIN) * 250 + ((unsigned char)(p)[4] - LHDR_MIN))

#define E_END       255                             // dummy last operator in an expression

// these constants are used in the second argument of the findvar() function, they should be or'd together
#define V_FIND              0x0000                    // a straight forward find, if the variable is not found it is created and set to zero
#define V_NOFIND_ERR        0x0200                    // throw an error if not found
#define V_NOFIND_NULL       0x0400                    // return a null pointer if not found
#define V_DIM_VAR           0x0800                    // dimension an array
#define V_LOCAL             0x1000                    // create a local variable
#define V_EMPTY_OK          0x2000                    // allow an empty array variable.  ie, var()
#define V_FUNCT             0x4000                    // we are defining the name of a function

// these flags are used in the last argument in expression()
#define E_NOERROR           true
#define E_ERROR             0
#define E_DONE_GETVAL       0b10

// this flag is used to signal that automatic precision is to be used in FloatToStr()
#define STR_AUTO_PRECISION  999

typedef struct s_vartbl {                               // structure of the variable table
	char name[MAXVARLEN];                       // variable's name
    char type;                                  // its type (T_NUM, T_INT or T_STR)
    char level;                                 // its subroutine or function level (used to track local variables)
    unsigned char size;                         // the number of chars to allocate for each element in a string array
    char dummy;
    int __attribute__ ((aligned (4))) dimtbl[MAXDIM];                   // the upper bounds: read through the accessors below
    union u_val{
        MMFLOAT f;                              // the value if it is a float
        long long int i;                        // the value if it is an integer
        MMFLOAT *fa;                            // pointer to the allocated memory if it is an array of floats
        long long int *ia;                      // pointer to the allocated memory if it is an array of integers
        char *s;                                // pointer to the allocated memory if it is a string
    }  __attribute__ ((aligned (8))) val;
} __attribute__ ((aligned (8))) xval;

/* ============================================================================
 * Array dimension access (the PicoMite's accessor layer)
 *
 * dimtbl[i] holds the DECLARED UPPER BOUND of dimension i.  Two values are
 * reserved and can never be a real upper bound:
 *      0   not an array, and it terminates the scan that counts dimensions
 *     -1   an unbound empty array parameter, eg  SUB s(a())
 * The element count of a dimension is DimElements(): the upper bound plus one,
 * less OPTION BASE.  Because zero is reserved, an upper bound of zero (a one
 * element dimension under OPTION BASE 0) has no natural encoding; DIM_ONE is
 * the value reserved to carry it.
 *
 * Read a slot through the accessors below rather than indexing dimtbl[].
 * RAW_DIM() is for the places that must see the value as stored: the dimension
 * scan, whole table copies, the reserved value tests, and the short string that
 * findvar() stores in dimtbl[1..].
 * ============================================================================ */

#define DIM_ONE (-2) // reserved encoding for an upper bound of 0

// Set to 0 and no DIM_ONE is ever stored, every accessor below expands to the
// expression it replaced, and single element dimensions are refused as they
// always were - useful for bisecting a regression back to this change.
#define DIM_DECODE_ENABLED 1

#define RAW_DIM(v, i) ((v).dimtbl[i]) // the value as stored
#define DIM_TABLE(v) ((v).dimtbl)     // the whole table, for copies

#if DIM_DECODE_ENABLED
#define DimUpper(d) ((d) == DIM_ONE ? 0 : (d))
#define DimIsRealArray(d) ((d) > 0 || (d) == DIM_ONE)
#define DimEncode(d) ((d) == 0 ? DIM_ONE : (d)) // apply on every store of an upper bound
#else
#define DimUpper(d) (d)
#define DimIsRealArray(d) ((d) > 0)
#define DimEncode(d) (d)
#endif

// DimIsRealArray() asks whether a slot holds a real dimension: on dimtbl[0]
// that means the variable is an array (and not an unbound parameter), on
// dimtbl[i] that dimension i is present.
#define DimElements(d) (DimUpper(d) + 1 - OptionBase)
#define DimIsScalar(d) ((d) == 0)     // dimtbl[0]: this variable is not an array
#define DimIsEnd(d) ((d) == 0)        // dimtbl[i]: no further dimension
#define DimIsAllocated(d) ((d) != 0)  // dimtbl[0]: an array or an array parameter
#define DimIsEmptyParam(d) ((d) == -1)

/* A variable's record is reached through its number: VREC(i) (the PicoMite's compact variable table).
   The locals' records are vartbl, in D3 SRAM.  A global's record is in a chunk of VARCHUNK records taken
   from the heap (on-chip RAM first) as globals are made, and never moves while the variable exists; a
   free or blocked global slot points at a shared placeholder record, so every probe and walker reads a
   slot the same way.  slotrec holds each number's record, beside vartbl. */
extern struct s_vartbl *vartbl;                 // the locals' records (MAXLOCALVARS)
extern struct s_vartbl **slotrec;               // the record of each variable number (MAXVARS)
#define VREC(i) (slotrec[(i)])                  // the record of variable i
#define SLOTRECBASE (SRAMBASE + MAXLOCALVARS * sizeof(struct s_vartbl))    // where slotrec is, after the locals' records
#define VARCHUNK 32                             // globals' records per chunk (2 KB)
// a string kept inside its own record (DIM s$ LENGTH 16 or less, a short CONST), not in the heap: never free it
#define VAR_INLINE_STR(r) ((char *)(r)->val.s >= (char *)(r) && (char *)(r)->val.s < (char *)((r) + 1))
void InitVarSlots(void);
void VarChunksForget(void);
void VarRecordRelease(int slot, int blocked);   // ERASE: a global's record back to the free list
int VarChunkBytes(void);                        // the SDRAM heap the globals' record chunks take (MEMORY)

// a member of a structure type (TYPE ... END TYPE: see Struct.c)
struct s_structmember {
    char name[MAXVARLEN + 1];                   // its name, in capitals
    unsigned char type;                         // T_NBR, T_INT, T_STR or T_STRUCT
    unsigned char size;                         // a string's maximum length; a nested structure's type number
    int offset;                                 // its offset in the structure
    int dimtbl[MAXDIM];                         // its dimensions, as a variable's (read through the accessors); 0 if none
};
// a structure type
struct s_structdef {
    char name[MAXVARLEN + 1];                   // its name, in capitals
    int num_members;
    struct s_structmember members[MAX_STRUCT_MEMBERS];
    int total_size;                             // its size in bytes, padded so the elements of an array stay aligned
};
extern struct s_structdef *g_structtbl[MAX_STRUCT_TYPES];
extern int g_structcnt;                         // the number of types
extern int g_StructArg;                         // the type of a structure being declared (AS type), or -1
extern int g_StructMemberType;                  // the type of the member the last findvar() reached, 0 for none
extern int g_StructMemberOffset;                // its offset in the structure
extern int g_StructMemberSize;                  // its size: a string's length, a nested structure's type
extern int g_ExprStructType;                    // the type of the whole structure an expression gave, or -1
void StructTableForget(void);
void JsonArenaForget(void);
int StructTableStart(void);
char *StructDefine(char *p, int ErrAbort);
const char *ParseStructMember(char *p, struct s_structdef *sd);
int FindStructType(char *name);
int FindStructMember(int struct_idx, char *membername, int *member_type, int *member_offset, int *member_size, int *member_dims);
void ListTypes(char *p);
void *ResolveStructMember(char *struct_ptr, int struct_idx, char *member_path, char **pp, int *member_type);
void *StructMemberAt(char *struct_ptr, int struct_idx, char **pp);
void StructParam(int vindex, int argvindex, char *data, int argtype, int argstruct, char *argname);
// what the last findvar() found: a structure member's own type and size (a string's length), else the variable's
#define FoundType() (g_StructMemberType ? g_StructMemberType : VREC(VarIndex)->type)
#define FoundSize() ((g_StructMemberType & T_STR) ? g_StructMemberSize : VREC(VarIndex)->size)
// after findvar() of a command's array argument: a member of every element of an array of structures (pts().x) is
// not an array the command can read (each element's is in a structure apart)
#define CHECK_STRUCT_MEMBER_ARRAY() do { if(g_StructMemberType && (VREC(VarIndex)->type & T_STRUCT) && DimIsRealArray(VREC(VarIndex)->dimtbl[0])) error("Struct member arrays not supported for this command"); } while(0)
char *StructReturn(char *data, int type);
char *StructInit(char *p, int sidx, char *data, int n);
char *SetValue(char *p, int t, void *v, int size);
extern int varcnt;
extern int Localvarcnt;                              // number of LOCAL variables defined (eg, largest index into the variable table)
extern int Globalvarcnt;                              // number of GLOBAL variables defined (eg, largest index into the variable table)
extern int VarIndex;                            // index of the current variable.  set after the findvar() function has found/created a variable
extern int LocalIndex;                          // used to track the level of local variables

extern int OptionBase;                          // value of OPTION BASE
extern char OptionExplicit,OptionEscape ;       // true if OPTION EXPLICIT has been used
extern char DefaultType;                        // the default type if a variable is not specifically typed
extern char syscheck;
extern int emptyarray;

//#if !defined(BOOL_ALREADY_DEFINED)
//    #define BOOL_ALREADY_DEFINED
//    typedef enum _BOOL { FALSE = 0, TRUE } BOOL;    // Undefined size
//#endif

#ifndef true
    #define true        1
#endif

#ifndef false
    #define false       0
#endif

#define MAXLINENBR          65001                                   // maximim acceptable line number

// skip whitespace
// finishes with x pointing to the next non space char
#define skipspace(x)    while(*x == ' ') x++

// skip to the next element
// finishes pointing to the zero char that preceeds an element
#define skipelement(x)  while(*x) x++

// skip to the next line
// skips text and and element separators until it is pointing to the zero char marking the start of a new line.
// the next byte will be either the newline token or zero char if end of program
#define skipline(x)     while(!(x[-1] == 0 && (x[0] == T_NEWLINE || x[0] == 0)))x++
extern const char namestart[], namein[], nameend[];
// find a token
// finishes pointing to the token or zero char if not found in the line
#define findtoken(x)    while(*x != (tkn) && *x)x++
#define IsDigitinline(a)	( a >= '0' && a <= '9' )

#define isnamestart(c)  (namestart[(uint8_t)c])                    // true if valid start of a variable name
#define isnamechar(c)   (namein[(uint8_t)c])        // true if valid part of a variable name
#define isnameend(c)    (nameend[(uint8_t)c])        // true if valid at the end of a variable name

//#define isnamestart(c)  (((c & 0xDF)>='A' && (c & 0xDF)<='Z') || c == '_')                    // true if valid start of a variable name
//#define isnamechar(c)   (isnamestart(c) || (c>=48 && c<=57) || c == '.')        // true if valid part of a variable name
//#define isnameend(c)    (isnamechar(c) || c == '$' || c == '!' || c == '%')        // true if valid at the end of a variable name

#define tokentype(i)    ((i >= C_BASETOKEN && i < TokenTableSize - 1 + C_BASETOKEN) ? (tokentbl[i - C_BASETOKEN].type) : 0)             // get the type of a token
#define tokenfunction(i)((i >= C_BASETOKEN && i < TokenTableSize - 1 + C_BASETOKEN) ? (tokentbl[i - C_BASETOKEN].fptr) : (tokentbl[0].fptr))    // get the function pointer  of a token
#define tokenname(i)    ((i >= C_BASETOKEN && i < TokenTableSize - 1 + C_BASETOKEN) ? (char *)(tokentbl[i - C_BASETOKEN].name) : "")            // get the name of a token

// Command tokens, in the PicoMite's form.  A command is stored in a program as sizeof(CommandToken)
// bytes, each 0x80 plus seven bits of its index in commandtbl, the low seven bits first; a CommandToken
// holds the index itself.  Function and operator tokens stay one byte, 0x80 plus the index in tokentbl.
// Every byte of a command token is 0x80 or more, and the bytes of a line's text are below 0x80.
typedef uint16_t CommandToken;                      // two bytes: up to 16384 commands
#define CMD_NOTOKEN ((CommandToken)~0)              // what commandtbl_at() returns when p holds no command

// the index of the command whose token starts at p
static inline CommandToken commandtbl_decode(const char *p) {
    const unsigned char *q = (const unsigned char *)p;
    if(sizeof(CommandToken) == 1) return (CommandToken)(q[0] & 0x7f);
    return (CommandToken)((q[0] & 0x7f) | ((q[1] & 0x7f) << 7));
}

// the command at p, or CMD_NOTOKEN if p does not start with a command token (a name, a number...)
static inline CommandToken commandtbl_at(const char *p) {
    const unsigned char *q = (const unsigned char *)p;
    if(q[0] < C_BASETOKEN || (sizeof(CommandToken) > 1 && q[1] < C_BASETOKEN)) return CMD_NOTOKEN;
    return commandtbl_decode(p);
}

// store the token for command i at p and return the address after it
static inline char *commandtbl_put(char *p, CommandToken i) {
    *p++ = (char)((i & 0x7f) + C_BASETOKEN);
    if(sizeof(CommandToken) > 1) *p++ = (char)((i >> 7) + C_BASETOKEN);
    return p;
}

#define commandtype(i)  ((i) < CommandTableSize - 1 ? (commandtbl[i].type) : 0)                      // the type of command i
#define commandfunction(i)((i) < CommandTableSize - 1 ? (commandtbl[i].fptr) : (commandtbl[0].fptr))  // the function of command i
#define commandname(i)  ((i) < CommandTableSize - 1 ? (char *)(commandtbl[i].name) : "")              // the name of command i

// this macro will allocate temporary memory space and build an argument table in it
// x = pointer to the basic text to be split up (char *)
// y = maximum number of args (will throw an error if exceeded) (int)
// s = a string of characters to be used in detecting where to split the text (char *)
#define getargs(x, y, s) char argbuf[STRINGSIZE + STRINGSIZE/2]; char *argv[y]; int argc; makeargs(x, y, argbuf, argv, &argc, s)

extern int CommandTableSize, TokenTableSize;

extern volatile int MMAbort;
extern jmp_buf mark;                            // longjump to recover from an error
extern char BreakKey;                           // console break key (defaults to CTRL-C)
extern jmp_buf run;                            // longjump to recover from an error

extern int ProgMemSize;

extern int NextData;                            // used to track the next item to read in DATA & READ stmts
extern char *NextDataLine;                      // used to track the next line to read in DATA & READ stmts
extern char *CurrentLinePtr;                    // pointer to the current line being executed
extern char *ContinuePoint;                     // Where to continue from if using the continue statement

extern char inpbuf[];                           // used to store user keystrokes until we have a line
extern char tknbuf[];                           // used to store the tokenised representation of the users input line
// tknbuf holds twice a source line, room for what a line that fits in a line buffer tokenises to,
// two-byte command tokens and all; tokenise() refuses anything longer rather than run past the end
#define TKNBUFSIZE (2 * STRINGSIZE)
extern char tknerrmsg[];                        // why tokenise() refused a line ("Line 12 is too long")
extern int tknfile, tknline;                    // the source of the line tokenise() makes for an image
extern char *tknfilename;
int ImageIsCurrent(void);                       // does program memory hold an image of this format?
void SubLayoutForget(void);                     // the SUBs' parameter layouts are gone with the heap (P6 F1)
void NameIndexForget(void);                     // the index of SUBs, FUNCTIONs and labels is gone with the heap
char *ProgName(void);                           // the program's file name from the image, or NULL
char *LineFileName(int file);                   // the name of source file 'file' (1 = the program)
char *NextLine(char *line);                     // the line after the one at 'line'
extern int strreplace_overflow;                 // set when a substitution would overflow its line buffer
extern char lastcmd[];                          // used to store the command history in case the user uses the up arrow at the command prompt

extern MMFLOAT farg1, farg2, fret;              // Global floating point variables used by operators
extern long long int iarg1, iarg2, iret;        // Global integer variables used by operators
extern char *sarg1, *sarg2, *sret;              // Global string pointers used by operators
extern int targ;                                // Global type of argument (string or MMFLOAT) returned by an operator

extern int cmdtoken;                            // the command being run (its index in commandtbl)
extern char *CmdTokenPtr;                       // where its token is
extern char *cmdline;                           // Command line terminated with a zero char and trimmed of spaces
extern char *nextstmt;                          // Pointer to the next statement to be executed.
extern char *ep;                                // Pointer to the argument to a function

extern int OptionErrorSkip;                    // value of OPTION ERROR
extern int MMerrno;
extern char *MMErrMsg;

extern char *subfun[];                          // Table of subroutines and functions built when the program starts running
struct s_tokentbl {                             // structure of the token table
	char name[16];                                 // the string (eg, PRINT, FOR, ASC(, etc)
    char type;                                  // the type returned (T_NBR, T_STR, T_INT)
    char precedence;                            // precedence used by operators only.  operators with equal precedence are processed left to right.
    void (*fptr)(void);                         // pointer to the function that will interpret that token
};
struct s_hash {                             // structure of the token table
	short hash;                                 // the string (eg, PRINT, FOR, ASC(, etc)
    short level;                                  // the type returned (T_NBR, T_STR, T_INT)
};
extern const struct s_tokentbl tokentbl[];
extern const struct s_tokentbl commandtbl[];

// used for the trace function
#define TRACE_BUFF_SIZE  128 //must be a multiple of 2
extern int TraceOn;
extern char *TraceBuff[TRACE_BUFF_SIZE];
extern int TraceBuffIndex;

// used to store commonly used tokens for faster token checking
extern char tokenTHEN, tokenELSE, tokenGOTO, tokenEQUAL, tokenTO, tokenSTEP, tokenWHILE, tokenUNTIL, tokenGOSUB, tokenAS, tokenFOR;
extern CommandToken cmdIF, cmdENDIF, cmdELSEIF, cmdELSE, cmdLOOP, cmdWHILE, cmdFOR, cmdNEXT, cmdENDSUB, cmdLOCAL, cmdSTATIC, cmdENDFUNCTION, cmdDO, cmdSELECT_CASE, cmdCASE, cmdCASE_ELSE, cmdEND_SELECT;
extern CommandToken cmdSUB, cmdFUN, cmdCSUB, cmdIRET, cmdTYPE, cmdEND_TYPE, cmdSTRUCT;

// void error(char *msg) ;
void error(char *, ...);
/* ============================================================================
 * Function declarations - Error handling ( From picomites)
 * ============================================================================ */
///void error(char *msg, ...);
void SyntaxError(void);
int IsGlobalData(void *data);                   // a pointer kept past the statement must be to a global's data
void RequireGlobal(void *data);                 // (the error when it is not)
void StandardError(int n);
void StandardErrorParam(int n, int m);
void StandardErrorParam2(int n, int m, int l);
void StandardErrorParam3(int n, int m, int l, int h);
void StandardErrorParamS(int n, char *m);


void MIPS16 InitBasic(void);
int FloatToInt32(MMFLOAT);
long long int FloatToInt64(MMFLOAT x);
void makeargs(char **tp, int maxargs, char *argbuf, char *argv[], int *argc, char *delim);
void *findvar(char *, int);
void erasearray(char *n);
void MIPS16 ClearVars(int level);
void MIPS16 ClearStack(void);
void MIPS16 ClearRuntime(void);
void MIPS16 ClearProgram(void);
void *DoExpression(char *p, int *t);
char *evaluate(char *p, MMFLOAT *fa, long long int *ia, char **sa, int *ta, int noerror);
char *doexpr(char *p, MMFLOAT *fa, long long int *ia, char **sa, int *oo, int *t);
MMFLOAT getnumber(char *p);
long long int getinteger(char *p);
//int getint(char *p, int min, int max);
long long int getint(char *p, long long int min, long long int max);
char *getstring(char *p);
int MIPS16 tokenise(int console);                // 0, or 1 when a program line is too long (console: an error)
void ExecuteProgram(char *);
//void AddProgramLine(int append);
char *findline(int, int);
char *findlabel(char *labelptr);
char *skipvar(char *p, int noerror);
char *skipexpression(char *p);
char *GetNextCommand(char *p, char **CLine, char *EOFMsg);
int FunctionType(char *p);
char *getclosebracket(char *p);
void makeupper(char *p);
void checkend(char *p);
int GetCommandValue(char *n);
int GetTokenValue(char *n);
char *checkstring(char *p, char *tkn);
char *MtoC(char *p);
char *CtoM(char *p);
void Mstrcpy(char *dest, char *src);
void Mstrcat(char *dest, char *src);
int Mstrcmp(char *s1, char *s2);
char *getCstring(char *p);
char *getFstring(char *p);
void InsertLastcmd(char *s);
int MIPS16 TraceLines(char *target);
void DefinedSubFun(int iscmd, char *cmd, int index, MMFLOAT *fa, long long int *i64, char **sa, int *t);
int FindSubFun(char *p, int type);
void MIPS16 PrepareProgram(int);
void MMPrintString(char* s);
void MMfputs(char *p, int filenbr);
void IntToStrPad(char *p, long long int nbr, signed char padch, int maxch, int radix);
void IntToStr(char *strr, long long int nbr, unsigned int base);
void FloatToStr(char *p, MMFLOAT f, int m, int n, unsigned char ch);
void STR_REPLACE(char *target, const char *needle, const char *replacement, uint8_t ignoresurround);
void ListUnfold(char *b);
// the cores of string and other functions, shared with CSUBs through the CallTable (Functions.c, MM_Misc.c, External.c)
char *StrLeft(char *dst, const char *s, int n);
char *StrRight(char *dst, const char *s, int n);
char *StrCase(char *dst, const char *s, int upper);
char *StrMid(char *dst, const char *s, int spos, int nbr);
char *StrChar(char *dst, int c);
char *StrFill(char *dst, int ch, int n);
int StrInstr(const char *s1, const char *s2, int start);
char *StrFormat(char *dst, MMFLOAT f, long long int i64, int isint, int m, int n, int ch);
int StrVal(const char *p, MMFLOAT *f, long long int *i);
MMFLOAT RndVal(void);
MMFLOAT TimerVal(void);
long long int GetuSec(void);
void TimerSet(long long int ms);
MMFLOAT PinVal(int pin);
void PinPut(int pin, int val);
extern const char *const FoldList[];                // the functions that share a token, as LIST shows them
extern const int FoldCount;
int str_equal(char *s1, char *s2);
#define mem_equal(a,b,c) !strncasecmp(a,b,c)
