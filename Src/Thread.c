/***************************************************************************
CMM2 MMBasic
Thread.c

Threads for a BASIC program: THREAD START, EXIT and YIELD (docs/Threads_Plan.md, Stage 1: threads that take turns).

Copyright 2011-2026 Geoff Graham, Peter Mather and Gerry Allardice.
See MMBasic.h for the licence.

A thread is a SUB running on a C stack of its own with its own copy of the interpreter's per-call state. The program
as it always ran is the main thread, number 0.

The running thread uses the interpreter's ordinary globals, where they always were. When it stops, what it has in
them is copied to its record (ContextSave), the parts of the stacks in use only; when it runs again they are copied
back (ContextLoad). The local variables' records are not copied and never move: each thread has a range of the local
slots (LocalBase to LocalLimit, MMBasic.c).

A thread is left only between two of its statements: the statement tail (check_interrupt, MM_Misc.c) calls
ThreadSchedule when INT_TICK_SLICE has been set. So nothing of the firmware's C code has to be re-entrant. A statement
that calls a FUNCTION has its own frames on the thread's C stack while the FUNCTION's statements run, which is why
each thread has a stack: ThreadSwap changes the stack pointer.

In bank 2 (the linker script): nothing here is on a statement's path until a program starts a thread.
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Symbols.h"
#include "Thread.h"

extern struct s_hash hashlist[MAXLOCALVARS];                        // (MMBasic.c: the running thread's locals, in the order made)
extern int localtop;                                                // (and the next local slot)
extern uint32_t DefinedSubFunMem;                                   // (a call whose arguments are being read)
extern int DefinedSubFunLocalIndex;
extern volatile char StrTmpLocalIndex[];                            // (Memory.c: each temporary block's level)
extern int StrTmpIndex;

#define T_FREE 0
#define T_READY 1
#define T_DONE 2                                                    // ended: its blocks are freed from another thread's stack
#define STACK_MARK 0x54485244                                       // the word at the bottom of a thread's stack

struct s_thread {
    uint32_t *sp;                                                   // its stack pointer while it is not running (ThreadSwap)
    unsigned char state, id;
    char *stack;                                                    // its C stack (NULL: the main program's, in the DTCM)
    uint32_t floor;                                                 // the stack check's floor (StackFloor while it runs)
    int lbase, llimit;                                              // its local slots
    int16_t *syml;                                                  // its names' local bindings (SymL while it runs)
    // the interpreter's state, as ContextSave left it
    char *nextstmt, *cmdline, *CmdTokenPtr, *CurrentLinePtr;
    int cmdtoken, ExecDepth, LocalIndex;
    int gosubindex, forindex, doindex, hashlistpointer, localtop, Localvarcnt, StrTmpIndex, TempMemoryIsChanged;
    int OptionErrorSkip, ErrorSkipDepth, MMerrno, MMerrline;
    uint32_t DefinedSubFunMem;
    int DefinedSubFunLocalIndex, FunReturnArrayCount;
    int NextData, restorepointer, SymTextLocals;
    char *NextDataLine;
    unsigned int screwup;                                           // EXECUTE's time left (ScrewUpTimer), stopped while it waits
    jmp_buf ErrNext;
    char errmsg[STRINGSIZE];                                        // MM.ERRMSG$
    char call[MAXVARLEN + 4];                                       // the statement that calls its SUB: the name, then zeros
    char tknbuf[TKNBUFSIZE];                                        // EXECUTE's line
    char *gosub[MAXGOSUB], *errs[MAXGOSUB], *subs[MAXGOSUB];
    struct s_forstack fors[MAXFORLOOPS + 1];
    struct s_dostack dos[MAXDOLOOPS];
    struct s_hash locals[MAXLOCALVARS];
    char *tmp[MAXTEMPSTRINGS];
    char tmplevel[MAXTEMPSTRINGS];
    struct sa_data data[MAXRESTORE];
};

int ThreadCount = 0;
static struct s_thread *Thr[MAXTHREADS + 1];                        // by number: Thr[0] the main program's, made with the first thread
static struct s_thread *Cur = NULL;                                 // the one running (NULL: no thread has been started)
static int Stopping = 0;                                            // the program is ending: no more switches
static char EndErrMsg[STRINGSIZE];                                  // MM.ERRMSG$'s home once the records have gone

/*******************************************************************************************
 the switch
*******************************************************************************************/
// The registers a C function must keep (r4-r11, d8-d15) and the return address pushed on the stack in use, its pointer
// kept in *from; then the stack pointer is 'to', and the same come back off that stack: the return is into the thread
// that was left there. Interrupts' handlers use whichever stack is current, and none is left on one that is not: this
// runs between two statements, not inside a handler.
__attribute__((naked, noinline)) static void ThreadSwap(uint32_t **from, uint32_t *to) {
    __asm volatile(
        "push   {r4-r11, lr}        \n"
        "vpush  {d8-d15}            \n"
        "str    sp, [r0]            \n"
        "mov    sp, r1              \n"
        "vpop   {d8-d15}            \n"
        "pop    {r4-r11, pc}        \n"
    );
}

static void ContextSave(struct s_thread *t) {
    t->nextstmt = nextstmt; t->cmdline = cmdline; t->CmdTokenPtr = CmdTokenPtr; t->CurrentLinePtr = CurrentLinePtr;
    t->cmdtoken = cmdtoken; t->ExecDepth = ExecDepth; t->LocalIndex = LocalIndex;
    t->gosubindex = gosubindex;
    memcpy(t->gosub, gosubstack, gosubindex * sizeof(char *));
    memcpy(t->errs, errorstack, gosubindex * sizeof(char *));
    memcpy(t->subs, substack, gosubindex * sizeof(char *));
    t->forindex = forindex;
    memcpy(t->fors, forstack, forindex * sizeof(struct s_forstack));
    t->doindex = doindex;
    memcpy(t->dos, dostack, doindex * sizeof(struct s_dostack));
    t->hashlistpointer = hashlistpointer;
    memcpy(t->locals, hashlist, hashlistpointer * sizeof(struct s_hash));
    t->localtop = localtop; t->Localvarcnt = Localvarcnt;
    t->StrTmpIndex = StrTmpIndex; t->TempMemoryIsChanged = TempMemoryIsChanged;
    memcpy(t->tmp, (void *)StrTmp, StrTmpIndex * sizeof(char *));
    memcpy(t->tmplevel, (void *)StrTmpLocalIndex, StrTmpIndex);
    t->OptionErrorSkip = OptionErrorSkip; t->ErrorSkipDepth = ErrorSkipDepth;
    t->MMerrno = MMerrno; t->MMerrline = MMerrline;
    if(MMErrMsg != t->errmsg) {                                     // (error() points it at its own buffer)
        strncpy(t->errmsg, MMErrMsg, STRINGSIZE - 1);
        t->errmsg[STRINGSIZE - 1] = 0;
    }
    memcpy(t->ErrNext, ErrNext, sizeof(jmp_buf));
    t->DefinedSubFunMem = DefinedSubFunMem; t->DefinedSubFunLocalIndex = DefinedSubFunLocalIndex;
    t->FunReturnArrayCount = FunReturnArrayCount;
    t->NextData = NextData; t->NextDataLine = NextDataLine; t->restorepointer = restorepointer;
    memcpy(t->data, datastore, restorepointer * sizeof(struct sa_data));
    t->SymTextLocals = SymTextLocals;
    t->screwup = ScrewUpTimer;                                      // EXECUTE's second runs only while its thread does
    ScrewUpTimer = 0;
}

// what the variables' code needs of a thread's state: its locals, its loops (ClearVars trims them), its temporary
// memory, its bindings
static void ContextLoadVars(struct s_thread *t) {
    LocalIndex = t->LocalIndex;
    forindex = t->forindex;
    memcpy(forstack, t->fors, forindex * sizeof(struct s_forstack));
    doindex = t->doindex;
    memcpy(dostack, t->dos, doindex * sizeof(struct s_dostack));
    hashlistpointer = t->hashlistpointer;
    memcpy(hashlist, t->locals, hashlistpointer * sizeof(struct s_hash));
    localtop = t->localtop; Localvarcnt = t->Localvarcnt;
    varcnt = Globalvarcnt + Localvarcnt;
    LocalBase = t->lbase; LocalLimit = t->llimit;
    StrTmpIndex = t->StrTmpIndex; TempMemoryIsChanged = t->TempMemoryIsChanged;
    memcpy((void *)StrTmp, t->tmp, StrTmpIndex * sizeof(char *));
    memcpy((void *)StrTmpLocalIndex, t->tmplevel, StrTmpIndex);
    if(SymL != NULL) SymL = t->syml;
    SymTextLocals = t->SymTextLocals;
}

static void ContextLoad(struct s_thread *t) {
    ContextLoadVars(t);
    nextstmt = t->nextstmt; cmdline = t->cmdline; CmdTokenPtr = t->CmdTokenPtr; CurrentLinePtr = t->CurrentLinePtr;
    cmdtoken = t->cmdtoken; ExecDepth = t->ExecDepth;
    gosubindex = t->gosubindex;
    memcpy(gosubstack, t->gosub, gosubindex * sizeof(char *));
    memcpy(errorstack, t->errs, gosubindex * sizeof(char *));
    memcpy(substack, t->subs, gosubindex * sizeof(char *));
    OptionErrorSkip = t->OptionErrorSkip; ErrorSkipDepth = t->ErrorSkipDepth;
    MMerrno = t->MMerrno; MMerrline = t->MMerrline;
    MMErrMsg = t->errmsg;
    memcpy(ErrNext, t->ErrNext, sizeof(jmp_buf));
    DefinedSubFunMem = t->DefinedSubFunMem; DefinedSubFunLocalIndex = t->DefinedSubFunLocalIndex;
    FunReturnArrayCount = t->FunReturnArrayCount;
    NextData = t->NextData; NextDataLine = t->NextDataLine; restorepointer = t->restorepointer;
    memcpy(datastore, t->data, restorepointer * sizeof(struct sa_data));
    StackFloor = t->floor;                                          // (from the record, never saved: PIXEL FILL turns the
    stackwarn = t->floor + 1024;                                    // check off for its length, and an error may leave it so)
    ScrewUpTimer = t->screwup;
    SymBindGlobalsChanged();                                        // a compiled record binds again the first time it runs in
}                                                                   // this thread: no stamp written before the switch matches

// the main program's limit of local slots: the lowest any thread has taken
static void MainLimit(void) {
    int i, lim = MAXLOCALVARS;
    for(i = 1; i <= MAXTHREADS; i++)
        if(Thr[i] != NULL && Thr[i]->state == T_READY && Thr[i]->lbase < lim) lim = Thr[i]->lbase;
    Thr[0]->llimit = lim;
    if(Cur == Thr[0]) LocalLimit = lim;
}

// the last thread has ended and the main program runs on alone: nothing of this is active again
static void ThreadsOff(void) {
    if(MMErrMsg == Thr[0]->errmsg) {
        strcpy(EndErrMsg, MMErrMsg);
        MMErrMsg = EndErrMsg;
    }
    LocalLimit = MAXLOCALVARS;
    FreeMemory(Thr[0]);
    Thr[0] = Cur = NULL;
    Stopping = 0;
}

// the blocks of the threads that have ended, freed by the thread that runs next (not from their own stacks)
static void ThreadReap(void) {
    int i, any = 0;
    for(i = 1; i <= MAXTHREADS; i++)
        if(Thr[i] != NULL && Thr[i]->state == T_DONE) {
            if(*(uint32_t *)Thr[i]->stack != STACK_MARK) {          // its stack went past its guard
                int n = i;
                Thr[i]->state = T_FREE;                             // (not reported twice)
                error("Thread % overflowed its stack", n);
            }
            FreeMemory(Thr[i]->stack);
            FreeMemory(Thr[i]);
            Thr[i] = NULL;
            any = 1;
        }
    if(!any) return;
    MainLimit();
    if(ThreadCount == 0 && Cur == Thr[0]) ThreadsOff();
}

static struct s_thread *NextReady(void) {
    int k;
    for(k = 1; k <= MAXTHREADS + 1; k++) {
        struct s_thread *t = Thr[(Cur->id + k) % (MAXTHREADS + 1)];
        if(t != NULL && t->state == T_READY) return t;
    }
    return NULL;
}

static void ThreadSwitchTo(struct s_thread *to) {
    struct s_thread *from = Cur;
    if(from->stack != NULL && *(uint32_t *)from->stack != STACK_MARK) error("Thread % overflowed its stack", (int)from->id);
    ContextSave(from);
    ContextLoad(to);
    Cur = to;
    ThreadSwap(&from->sp, to->sp);                                  // back here when 'from' next runs
    ThreadReap();
}

// the running thread has ended: the next one runs, and frees this one's blocks
__attribute__((noreturn)) static void ThreadFinish(void) {
    struct s_thread *old = Cur, *to;
    old->state = T_DONE;
    ThreadCount--;
    ScrewUpTimer = 0;
    if((to = NextReady()) == NULL) to = Thr[0];                     // (the main program at least)
    ContextLoad(to);
    Cur = to;
    ThreadSwap(&old->sp, to->sp);
    for(;;) {}                                                      // (nothing switches to a thread that has ended)
}

// a new thread's first run, on its own stack: its SUB called as a statement of its own, so that the call is made as
// any other (DefinedSubFun), and END SUB returns to the end of that statement, which ends this run of the interpreter
static void ThreadMain(void) {
    ThreadReap();
    ExecuteProgram(Cur->call);
    ThreadFinish();
}

/*******************************************************************************************
 what the rest of the firmware calls
*******************************************************************************************/
// The statement tail, when INT_TICK_SLICE was set (check_interrupt): the next thread that can run, if there is one.
// Returns when this thread's turn comes round again. Not while an interrupt routine runs (it is the main program's,
// and its saved state is single), nor once the program is ending.
void ThreadSchedule(void) {
    struct s_thread *to;
    if(Cur == NULL || Stopping || InterruptReturn != NULL) return;
    to = NextReady();
    if(to == NULL || to == Cur) return;
    ThreadSwitchTo(to);
}

int ThreadNumber(void) {
    return Cur == NULL ? 0 : Cur->id;
}

void ThreadMainOnly(void) {
    if(Cur != NULL && Cur->id != 0) error("Not allowed in a thread");
}

void ThreadsNone(void) {
    if(ThreadCount) error("Not allowed while a thread is running");
}

void ThreadsStop(void) {
    Stopping = 1;
}

char *ThreadTokenBuffer(void) {
    return (Cur == NULL || Cur->id == 0) ? tknbuf : Cur->tknbuf;
}

void ThreadStackLimits(void) {
    if(Cur == NULL) return;
    StackFloor = Cur->floor;
    stackwarn = Cur->floor + 1024;
}

static int OnMainStack(void) {
    uint32_t sp;
    __asm volatile("MRS %0, msp" : "=r"(sp));
    return (sp >> 17) == (0x20000000 >> 17);                        // the DTCM: no thread's stack is there
}

/* The program has stopped (an error, END, Ctrl-C: the prompt's landing, main.c) or is being replaced (RUN, CHAIN:
   ClearRuntime): every thread ends. Each one's locals and temporary memory are freed through the interpreter's own
   routines, with its state in the globals for the moment, then the blocks go. What is left is a program with no
   threads, no locals and empty stacks; the statement being run (nextstmt and the rest) is not touched. Called on the
   main stack only: a thread cannot free the stack it is running on. 1 if there were threads. */
int ThreadsEnd(void) {
    int i, eno = MMerrno, eline = MMerrline;
    if(Cur == NULL) return 0;
    if(!OnMainStack()) error("Not allowed in a thread");
    strncpy(EndErrMsg, MMErrMsg, STRINGSIZE - 1);
    EndErrMsg[STRINGSIZE - 1] = 0;
    ContextSave(Cur);
    for(i = 0; i <= MAXTHREADS; i++) {
        if(Thr[i] == NULL) continue;
        if(Thr[i]->state == T_READY) {
            ContextLoadVars(Thr[i]);
            ClearVars(1);
            LocalIndex = 0;
            ClearTempMemory();
        }
    }
    LocalBase = 0; LocalLimit = MAXLOCALVARS;
    localtop = 0; hashlistpointer = 0; Localvarcnt = 0;
    varcnt = Globalvarcnt;
    LocalIndex = 0; gosubindex = forindex = doindex = 0;
    if(SymL != NULL) SymL = Thr[0]->syml;
    SymTextLocals = 0;
    StackFloor = Thr[0]->floor;
    stackwarn = Thr[0]->floor + 1024;
    MMerrno = eno; MMerrline = eline;
    MMErrMsg = EndErrMsg;
    for(i = 1; i <= MAXTHREADS; i++)
        if(Thr[i] != NULL) {
            FreeMemory(Thr[i]->stack);
            FreeMemory(Thr[i]);
            Thr[i] = NULL;
        }
    FreeMemory(Thr[0]);
    Thr[0] = Cur = NULL;
    ThreadCount = 0;
    Stopping = 0;
    return 1;
}

/*******************************************************************************************
 THREAD
*******************************************************************************************/
// the on-chip heap's free bytes: a thread's stack goes there while plenty is left for the strings and the audio
static int OnChipFree(void) {
    unsigned char *a;
    int n = 0;
    for(a = (unsigned char *)RAMBASE; a < (unsigned char *)RAMEND; a += RAMPAGESIZE)
        if(!(MBitsGet(a) & PUSED)) n++;
    return n * RAMPAGESIZE;
}

// THREAD START n, sub
static void ThreadStart(int n, char *name) {
    struct s_thread *t;
    char *stack, sub[MAXVARLEN + 1];
    int i, len = 0, base, maintop, bindings;
    uint32_t *sp;
    if(CurrentLinePtr == NULL) error("Invalid at the command prompt");
    skipspace(name);
    if(!isnamestart(*name)) error("Expected the name of a SUB");
    while(isnamechar(name[len])) {
        if(len == MAXVARLEN) error("Variable name too long");
        sub[len] = name[len];
        len++;
    }
    sub[len] = 0;
    checkend(name + len);
    if(Thr[n] != NULL && Thr[n]->state == T_READY) error("Thread % is running", n);
    if((i = FindSubFun(sub, 0)) < 0 || commandtbl_at(subfun[i]) != cmdSUB) error("$ is not a SUB", sub);

    if(Cur != NULL) ThreadReap();                                   // (a number that has just ended is free to use again;
                                                                    // with the last of them gone, Cur is NULL after it)
    if(Cur == NULL) {                                               // the first: the main program's record
        if((t = GetMemoryNull(sizeof(struct s_thread))) == NULL) error("Not enough memory for a thread");
        t->state = T_READY;
        t->id = 0;
        t->floor = StackFloor;
        t->llimit = MAXLOCALVARS;
        t->syml = SymL;
        Thr[0] = Cur = t;
        Stopping = 0;
    }

    // its local slots: the highest range free, above everything the main program has in use
    maintop = (Cur == Thr[0]) ? localtop : Thr[0]->localtop;
    for(base = MAXLOCALVARS - THREAD_LOCALS; base >= maintop; base -= 16) {
        for(i = 1; i <= MAXTHREADS; i++)
            if(Thr[i] != NULL && base < Thr[i]->llimit && base + THREAD_LOCALS > Thr[i]->lbase) break;
        if(i > MAXTHREADS) break;
    }
    if(base < maintop) {
        if(ThreadCount == 0) ThreadsOff();
        error("Not enough local variables for a thread");
    }

    // its stack and its record, with its names' bindings after it
    bindings = SymL != NULL ? SymCanonCount * sizeof(int16_t) : 0;
    stack = (OnChipFree() >= THREAD_STACK + 96 * 1024) ? GetInternalMemoryNull(THREAD_STACK) : NULL;
    if(stack == NULL) stack = GetMemoryNull(THREAD_STACK);
    t = stack != NULL ? GetMemoryNull(sizeof(struct s_thread) + bindings) : NULL;
    if(t == NULL) {
        if(stack != NULL) FreeMemory(stack);
        if(ThreadCount == 0) ThreadsOff();
        error("Not enough memory for a thread");
    }
    t->state = T_READY;
    t->id = n;
    t->stack = stack;
    *(uint32_t *)stack = STACK_MARK;
    t->floor = (uint32_t)stack + THREAD_GUARD;
    t->lbase = t->localtop = base;
    t->llimit = base + THREAD_LOCALS;
    if(bindings) {
        t->syml = (int16_t *)(t + 1);
        memset(t->syml, 0xFF, bindings);                            // (-1: no name has a local)
    }
    t->CurrentLinePtr = CurrentLinePtr;                             // its SUB's call is reported at this line
    t->NextDataLine = (char *)ProgMemory;
    memcpy(t->call, sub, len);                                      // (the zeros after it end the statement and the "program")

    // its stack as ThreadSwap leaves one: d8-d15, r4-r11, then where to go on, ThreadMain, with the stack 8-byte aligned
    // there as a function's is on entry
    sp = (uint32_t *)(((uint32_t)stack + THREAD_STACK) & ~7u) - 25;
    memset(sp, 0, 24 * sizeof(uint32_t));
    sp[24] = (uint32_t)ThreadMain;
    t->sp = sp;

    Thr[n] = t;
    ThreadCount++;
    MainLimit();
}

void cmd_thread(void) {
    char *p;
    if((p = checkstring(cmdline, "START")) != NULL) {
        getargs(&p, 3, ",");
        if(argc != 3) error("Syntax");
        ThreadStart(getint(argv[0], 1, MAXTHREADS), argv[2]);
    } else if((p = checkstring(cmdline, "YIELD")) != NULL) {
        checkend(p);
        if(InterruptReturn != NULL) error("Invalid in an interrupt routine");
        if(ThreadCount) {                                           // the statement tail makes the switch (ThreadSchedule)
            uint32_t primask = __get_PRIMASK();
            __disable_irq();
            IntReady.tick |= INT_TICK_SLICE;
            __set_PRIMASK(primask);
        }
    } else if((p = checkstring(cmdline, "EXIT")) != NULL) {
        checkend(p);
        if(Cur == NULL || Cur->id == 0) error("Not in a thread");
        ClearVars(1);                                               // its locals at every level, its loops and its temporary memory
        LocalIndex = 0;
        gosubindex = forindex = doindex = 0;
        ClearTempMemory();
        ThreadFinish();
    } else
        error("Syntax");
}
