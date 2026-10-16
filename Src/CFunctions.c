/***************************************************************************
CMM2 MMBasic
CFunctions.c

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

*******************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
//Vector to CFunction static RAM
#define CFUNCRAM_SIZE   256
#define CFUNC_ARGS      16                                          // the most arguments a CSUB takes (the PicoMite's)
int CFuncRam[CFUNCRAM_SIZE/sizeof(int)];

//Vector to CFunction routine called every mSec
unsigned int CFuncmSec = (unsigned int)NULL;
extern volatile uint64_t uSecTimer;
extern volatile uint64_t FastTimer;
extern TIM_HandleTypeDef htim16;
extern TIM_HandleTypeDef htim5;
extern uint32_t ticks_per_microsecond;
void CallExecuteProgram(char *p);
MMFLOAT IntToFloat(long long int a){ return a; }
void CallCFuncmSec(void);
extern void routinechecksExternal(void);
//Vector to CFunction routine called every command (ie, from the BASIC interrupt checker)
unsigned int CFuncInt1 = (unsigned int)NULL;
//Vector to CFunction routine called by the interrupt 2 handler
unsigned int CFuncInt2 = (unsigned int)NULL;
static int map(int cl) {return ((cl & 0b11100000)<<16) | ((cl & 0b00011100)<<11) | ((cl & 0b00000011)<<6);}
static uint64_t timer(void){ return (__HAL_TIM_GET_COUNTER(&htim16) + (uSecTimer*50000));}
static uint64_t fasttimer(void){ return (__HAL_TIM_GET_COUNTER(&htim5) + (FastTimer));}
void routinechecksExternal(void){
	routinechecks(1);
}

const void * const CallTable[] __attribute__((section(".cspace")))  = {	(void *)uSec,
																		(void *)putConsole,
																		(void *)getConsole,
																		(void *)ExtCfg,
																		(void *)ExtSet,
																		(void *)ExtInp,
																		(void *)PinSetBit,
																		(void *)PinRead,
																		(void *)MMPrintString,
																		(void *)IntToStr,
																		(void *)CheckAbort,
																		(void *)GetMemory,
																		(void *)GetTempMemory,
																		(void *)FreeMemory,
																		(void *)&DrawRectangle,
																		(void *)&DrawBitmap,
																		(void *)DrawLine,
																		(void *)FontTable,
																		(void *)&ExtCurrentConfig,
																		(void *)&HRes,
																		(void *)&VRes,
																		(void *)SoftReset,
																		(void *)error,
																		(void *)&ProgMemory,
																		(void *)&slotrec,                   // each variable number's record (VREC); was &vartbl
																		(void *)&varcnt,
																		(void *)&DrawBuffer,
																		(void *)&ReadBuffer,
																		(void *)&FloatToStr,
                                                                        (void *)CallExecuteProgram,
                                                                        (void *)&CFuncmSec,                 // the variable, as the header reads it (was the function CallCFuncmSec)
                                                                        (void *)CFuncRam,
                                                                        (void *)&ScrollLCD,
																		(void *)&ScrollBufferV,
																		(void *)&ScrollBufferH,
																		(void *)&DrawBufferFast,
																		(void *)&ReadBufferFast,
																		(void *)&MoveBufferNormal,
																		(void *)DrawPixelExternal,
																		(void *)routinechecksExternal,
																		(void *)GetPageAddress,
																		(void *)mycopysafe,
																		(void *)IntToFloat,
																		(void *)FloatToInt64,
																		(void *)&Option,
																		(void *)&ReadPageAddressExternal,
																		(void *)&WritePageAddressExternal,
																		(void *)timer,
                                                                        (void *)&CFuncInt1,
                                                                        (void *)&CFuncInt2,
																		(void *)fasttimer,
																		(void *)&ticks_per_microsecond,
																		(void *)map,
																		(void *)sin,
																		(void *)&VideoColour,
																		(void *)DrawCircle,
																		(void *)DrawTriangle,
                                                                       // (void *)CallExecuteProgram,  //Duplicate ???
									   	   	   	   	   	   	   	   	   	   };


/* The CallTable in the PicoMite's layout: PicoCFunctions.h, every slot at the same offset.  It serves the CSUBs
   that mmb2csub and armcfgen build for the M7 (docs/mmb2csub_CMM2_Plan.md), which find it through slot 7 of the
   vector table.  The startup file fills that slot at link time: VTOR is in flash here, so it cannot be written
   at run time as the PicoMite does.  The offsets are the ABI, so the slots are append-only.  Every function here
   is hard-float, taking its doubles in d0-d7 as a CSUB compiled for the M7 passes them.  The old table above stays,
   for the CSUBs built for it.  Where the PicoMite's slot holds a function with another signature, a wrapper below
   adapts it; hardware the CMM2 lacks gets a harmless stand-in, or an error for PIO. */
static void uSecP(unsigned long long us) { uSec(us); }
static void ExtSetP(int pin, int val) { ExtSet(pin, val); }
static int ExtInpP(int pin) { return ExtInp(pin); }
static void DrawBuffer5(int x1, int y1, int x2, int y2, char *c) { DrawBuffer(x1, y1, x2, y2, c, 0); }
static void (*const DrawBufferP)(int, int, int, int, char *) = DrawBuffer5;
static MMFLOAT FMul(MMFLOAT a, MMFLOAT b) { return a * b; }
static MMFLOAT FAdd(MMFLOAT a, MMFLOAT b) { return a + b; }
static MMFLOAT FSub(MMFLOAT a, MMFLOAT b) { return a - b; }
static MMFLOAT FDiv(MMFLOAT a, MMFLOAT b) { return a / b; }
static int FCmp(MMFLOAT a, MMFLOAT b) { return a > b ? 1 : a < b ? -1 : 0; }
static MMFLOAT LoadFloat(unsigned long long c) { union { unsigned long long a; MMFLOAT b; } f; f.a = c; return f.b; }
static int IDiv(int a, int b) { return a / b; }
static int IMod(int a, int b) { return a % b; }
static float SAdd(float a, float b) { return a + b; }
static float SSub(float a, float b) { return a - b; }
static float SMul(float a, float b) { return a * b; }
static float SDiv(float a, float b) { return a / b; }
static int SCmp(float a, float b) { return a > b ? 1 : a < b ? -1 : 0; }
static float DtoS(double a) { return (float)a; }
static double StoD(float a) { return (double)a; }
static long long StoI(float a) { return (long long)(a >= 0 ? a + 0.5f : a - 0.5f); }
static float ItoS(long long a) { return (float)a; }
static long long LMul(long long a, long long b) { return a * b; }
static long long LDiv(long long a, long long b) { return b ? a / b : 0; }      // (0 for a zero divisor, as __aeabi_ldiv0)
static long long LMod(long long a, long long b) { return b ? a % b : 0; }
static long long LShl(long long a, int n) { return a << n; }
static long long LAsr(long long a, int n) { return a >> n; }
static unsigned long long LLsr(unsigned long long a, int n) { return a >> n; }
static char CSubCompleteP;                                          // the PicoMite's CSUB interrupt flag: nothing sets it here
static void NoAudio(uint16_t left, uint16_t right) { (void)left; (void)right; }
static void (*const AudioOutputP)(uint16_t, uint16_t) = NoAudio;
static uint16_t AudioWrapP;
static unsigned int CFuncInt3P, CFuncInt4P;                         // nothing calls these here
static void PIOExecuteP(int pio, int sm, unsigned int ins) { (void)pio; (void)sm; (void)ins; error("PIO is not on the CMM2"); }
static unsigned char *const NoBuffer = NULL;                        // the PicoMite's frame, layer and display buffers
static void NoRefresh(void) { }                                     // the CMM2's display needs no refresh

const void * const CallTableP[] = {
    (void *)uSecP,                      // 0x00 void uSec(unsigned long long us)
    (void *)putConsole,                 // 0x04
    (void *)getConsole,                 // 0x08
    (void *)ExtCfg,                     // 0x0c
    (void *)ExtSetP,                    // 0x10 void ExtSet(int pin, int val)
    (void *)ExtInpP,                    // 0x14 int ExtInp(int pin)
    (void *)PinSetBit,                  // 0x18
    (void *)PinRead,                    // 0x1c
    (void *)MMPrintString,              // 0x20
    (void *)IntToStr,                   // 0x24
    (void *)CheckAbort,                 // 0x28
    (void *)GetMemory,                  // 0x2c
    (void *)GetTempMemory,              // 0x30
    (void *)FreeMemory,                 // 0x34
    (void *)&DrawRectangle,             // 0x38
    (void *)&DrawBitmap,                // 0x3c
    (void *)DrawLine,                   // 0x40
    (void *)FontTable,                  // 0x44
    (void *)&ExtCurrentConfig,          // 0x48
    (void *)&HRes,                      // 0x4c
    (void *)&VRes,                      // 0x50
    (void *)SoftReset,                  // 0x54
    (void *)error,                      // 0x58
    (void *)&ProgMemory,                // 0x5c
    (void *)SLOTRECBASE,                // 0x60 the record of each variable number (VREC): the array itself
    (void *)&varcnt,                    // 0x64
    (void *)&DrawBufferP,               // 0x68 void (*)(int x1, int y1, int x2, int y2, char *c)
    (void *)&ReadBuffer,                // 0x6c
    (void *)FloatToStr,                 // 0x70
    (void *)CallExecuteProgram,         // 0x74
    (void *)&CFuncmSec,                 // 0x78
    (void *)CFuncRam,                   // 0x7c
    (void *)&ScrollLCD,                 // 0x80
    (void *)IntToFloat,                 // 0x84
    (void *)FloatToInt64,               // 0x88
    (void *)&Option,                    // 0x8c the CMM2's struct option_s, not the PicoMite's
    (void *)sin,                        // 0x90
    (void *)DrawCircle,                 // 0x94
    (void *)DrawTriangle,               // 0x98
    (void *)timer,                      // 0x9c unsigned long long, microseconds
    (void *)FMul,                       // 0xa0
    (void *)FAdd,                       // 0xa4
    (void *)FSub,                       // 0xa8
    (void *)FDiv,                       // 0xac
    (void *)FCmp,                       // 0xb0
    (void *)LoadFloat,                  // 0xb4
    (void *)&CFuncInt1,                 // 0xb8
    (void *)&CFuncInt2,                 // 0xbc
    (void *)&CSubCompleteP,             // 0xc0
    (void *)&AudioOutputP,              // 0xc4
    (void *)IDiv,                       // 0xc8
    (void *)&AudioWrapP,                // 0xcc
    (void *)&CFuncInt3P,                // 0xd0
    (void *)&CFuncInt4P,                // 0xd4
    (void *)PIOExecuteP,                // 0xd8
    (void *)&WritePageAddressExternal,  // 0xdc the write page (PAGE WRITE)
    (void *)&NoBuffer,                  // 0xe0
    (void *)&NoBuffer,                  // 0xe4
    (void *)&NoBuffer,                  // 0xe8
    (void *)&DrawPixel,                 // 0xec
    (void *)NoRefresh,                  // 0xf0
    (void *)cos,                        // 0xf4
    (void *)sqrt,                       // 0xf8
    (void *)atan2,                      // 0xfc
    (void *)pow,                        // 0x100
    (void *)SAdd,                       // 0x104
    (void *)SSub,                       // 0x108
    (void *)SMul,                       // 0x10c
    (void *)SDiv,                       // 0x110
    (void *)SCmp,                       // 0x114
    (void *)sinf,                       // 0x118
    (void *)cosf,                       // 0x11c
    (void *)sqrtf,                      // 0x120
    (void *)atan2f,                     // 0x124
    (void *)powf,                       // 0x128
    (void *)DtoS,                       // 0x12c
    (void *)StoD,                       // 0x130
    (void *)StoI,                       // 0x134
    (void *)ItoS,                       // 0x138
    (void *)IMod,                       // 0x13c
    (void *)LMul,                       // 0x140
    (void *)LDiv,                       // 0x144
    (void *)LMod,                       // 0x148
    (void *)LShl,                       // 0x14c
    (void *)LAsr,                       // 0x150
    (void *)LLsr,                       // 0x154
    (void *)memcpy,                     // 0x158
    (void *)memset,                     // 0x15c
    (void *)memmove,                    // 0x160
    (void *)MtoC,                       // 0x164
    (void *)CtoM,                       // 0x168
    (void *)Mstrcpy,                    // 0x16c
    (void *)Mstrcat,                    // 0x170
    (void *)Mstrcmp,                    // 0x174
    (void *)StrLeft,                    // 0x178 the string cores (Functions.c)
    (void *)StrRight,                   // 0x17c
    (void *)StrCase,                    // 0x180
    (void *)StrMid,                     // 0x184
    (void *)StrChar,                    // 0x188
    (void *)StrFill,                    // 0x18c
    (void *)StrInstr,                   // 0x190
    (void *)StrFormat,                  // 0x194
    (void *)TimerVal,                   // 0x198
    (void *)RndVal,                     // 0x19c
    (void *)log,                        // 0x1a0
    (void *)tan,                        // 0x1a4
    (void *)StrVal,                     // 0x1a8
    (void *)TimerSet,                   // 0x1ac
    (void *)&gui_fcolour,               // 0x1b0
    (void *)&gui_bcolour,               // 0x1b4
    (void *)PinVal,                     // 0x1b8
    (void *)PinPut,                     // 0x1bc
};

// used by CallCFunction() below to find a CFunction or CSub in program flash or the library
unsigned int *FindCFunction(unsigned int *p, char *CmdPtr) {
    while(*p != 0xffffffff) {
        if(*p++ == (unsigned int)CmdPtr) return p;
        p += (*p + 4) / sizeof(unsigned int);
    }
    return p;
}

long long int CallCFunction(char *CmdPtr, char *ArgList, char *DefP, char *CallersLinePtr) {
    void *arg[CFUNC_ARGS] = { NULL };                               // (the PicoMite's 16)
    int typ[CFUNC_ARGS] = { 0 };
#if defined(MX170)
    MMFLOAT *ffarg[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
#endif
    long long int ret, i64[CFUNC_ARGS];
    MMFLOAT ff[CFUNC_ARGS];
    char *pp;
    int i,type;
    uint32_t ii;
    unsigned int *p=(void * const )CallTable;
    MMFLOAT ftmp;
    if((uint32_t)p > 0x10000000)error("Internal error");
    // find the C code in flash
    if(*ArgList == '(') ArgList++;                                  // and step over it
    p = FindCFunction((unsigned int *)CFunctionFlash, CmdPtr);      // search through the program flash looking for a match to the function being called
    if(*p == 0xffffffff) error("Internal fault 5(sorry)");

    // next, get the argument types (if specified)
    {  // first copy the type list to a buffer and trim the following closing bracket (if there)
        char buf[MAXSTRLEN];
        char *p = buf;
        if(*DefP == '(') DefP++;
        while(*DefP && *DefP != ')' && *DefP != '\'') *p++ = *DefP++;
        *p = 0;
        p = buf;
        skipspace(p);
        CheckIfTypeSpecified(p, &i, true);
        if(i != DefaultType) {
            // if there is a type list get each entry
            getargs(&p, CFUNC_ARGS * 2 - 1, ",");
            for(i = 0; i < argc; i+=2) {                            // get each definition
                CheckIfTypeSpecified(argv[i], &typ[i/2], false);
                typ[i/2] &= ~T_IMPLIED;
            }
        }
    }

    // we have found the CFunction or CSub and the types on its command line
    CurrentLinePtr = CallersLinePtr;                                // report errors at the caller
    if(*ArgList != ')') {
        getargs(&ArgList, CFUNC_ARGS * 2 - 1, ",");                 // expand the command line of the caller
        if(argc % 2 == 0) error("Syntax");
        for(i = 0; i < argc; i += 2) {
            // if this is a straight variable we want to pass a pointer to its value in RAM
            if(isnamestartsym(*argv[i]) && (*skipvar(argv[i], false) == 0 || *skipvar(argv[i], false) == ')') && !(FindSubFun(argv[i], 1) >= 0 && strchr(argv[i], '(') != NULL)) {
                arg[i/2] = findvar(argv[i], V_FIND | V_EMPTY_OK /* | V_NOFIND_ERR */ );   // if the argument
                if(typ[i/2] != 0 && !(TypeMask(VREC(VarIndex)->type) & typ[i/2])) error("Incompatible type");
#if defined(MX170)
                // if we are using double floats we must make special provision when a non array float variable is in the argument list (we don't touch arrays of floats)
                if(typ[i/2] == T_NBR && DimIsScalar(VREC(VarIndex)->dimtbl[0])) {
                  ffarg[i/2] = (MMFLOAT *)arg[i/2];                 // save a pointer to the arg
                  ff[i/2] = *(MMFLOAT *)arg[i/2];                   // convert to a single precision value and save for the CFun
                    arg[i/2] = &ff[i/2];                            // place a pointer to it in the CFun's parameter list
                }
#endif
            } else {
                // else it must be an expression of some sort
                // get the value based on the type specified in the definition
                switch(typ[i/2]) {
                    case T_INT: i64[i/2] = getinteger(argv[i]);
                                arg[i/2] = &i64[i/2];
                                break;
                    case T_NBR: ftmp = getnumber(argv[i]);
                                ff[i/2] = ftmp;
                                arg[i/2] = &ff[i/2];
                                break;
                    case T_STR: arg[i/2] = GetTempStrMemory();
                                Mstrcpy(arg[i/2], getstring(argv[i]));
                                break;
                    default:    // the type has not been specified (old style CFunction)
                                type = T_NOTYPE;
                                evaluate(argv[i], &ftmp, &i64[i/2], &pp, &type, false);
                                ff[i/2] = ftmp;
                                if(type & T_NBR) {
                                    arg[i/2] = &ff[i/2];
                                } else if(type & T_INT)
                                    arg[i/2] = &i64[i/2];
                                else {
                                    arg[i/2] = GetTempStrMemory();
                                    Mstrcpy(arg[i/2], pp);
                                }
                                break;
                }
            }
        }
    }
    p++;      // step over the size word

    // run the function in flash
    ii = *p++;
    p = (unsigned int *)((unsigned int) p | 0x1);
    ret = ((long long int (*)(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *, void *)) (p + ii))
        (arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14], arg[15]);   // run the CFunction

#if defined(MX170)
    // if we have a double float variable as an argument we must copy the CFun's possible return value back into that variable
    for(i = 0; i < 10; i++) {
        if(ffarg[i] != NULL) {
            *ffarg[i] = ff[i];                                      // convert the single precision arg back to a double
            RoundDoubleFloat(ffarg[i]);
        }
    }
#endif
    return ret;
}
// If the CFuncmSec vector is set then call the CFunction
void CallCFuncmSec(void){
    typedef void func(void);
    func* f=(func*)(void *)CFuncmSec;
    f();
}

// save the interpreter state if re entering it
void CallExecuteProgram(char *p) {
    char *nextstmtSaved = nextstmt;
    LocalIndex++;
    ExecuteProgram(p);
    nextstmt = nextstmtSaved;
    LocalIndex--;
    TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
}

// If the CFuncmInt1 vector is set then call the CFunction
void CallCFuncInt1(void){
    typedef void func(void);
    func* f=(func*)(void *)CFuncInt1;
    f();
}

// If the CFuncmInt2 vector is set then call the CFunction
void CallCFuncInt2(void){
    typedef void func(void);
    func* f=(func*)(void *)CFuncInt2;
    f();
}


