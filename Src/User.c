/***************************************************************************
CMM2 MMBasic

User.c

USER and USER( (Peter, 15 Nov 2026): a module of the user's own, built with the SDK (Inc/cmm2user.h, with a module to
start from in tools/sdk/examples/user), is handed each USER command and each USER( function. It reads its part of the
statement itself, with the interpreter's own routines (userbasic, below), and follows MMBasic's rules as a built-in
command does. USER LOAD puts it in the heap's executable window (Loader.c's LOADER_MOD_USER slot, beside SQL's), and
its functions are called on the main stack, as a CSUB's are.

    USER LOAD file$         the module loaded afresh (.mod added to a name with no extension), the last one closed
                            first. Its name is kept, so after RUN, NEW or CHAIN, which take the module with the heap,
                            the next USER loads it again (afresh: its variables start again)
    USER CLOSE              the module closed (its forget() first) and its name forgotten
    USER anything else      the module's command, given the rest of the statement
    USER(anything)          the module's function, given the text inside the brackets: an integer, a float or a string

With no module, USER and USER( give the error "USER LOAD first". LOAD and CLOSE are the firmware's words, so a
module's own command cannot be called either of them. In the bank 2 flash with the loader (the link script).

Copyright 2026 Peter Mather. (The firmware's licence: see the other files.)
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "Loader.h"
#include "cmm2user.h"

_Static_assert(sizeof(MMFLOAT) == sizeof(double), "cmm2user.h's floats are doubles");
_Static_assert(T_NBR == CMM2_T_NBR && T_STR == CMM2_T_STR && T_INT == CMM2_T_INT && T_STRUCT == CMM2_T_STRUCT,
               "cmm2user.h's types are MMBasic's");
_Static_assert(STRINGSIZE == CMM2_STRINGSIZE && MAXDIM == 5, "cmm2user.h's sizes are MMBasic's");

extern void getfullpath(char *p, char *q);                          // (FileIO.c)
extern void DrivePath(char *out);                                    // (FileIO.c: the drive put in front)

static char username[FF_MAX_LFN + 8];                               // USER LOAD's file as a full path: kept when the heap goes
static int userdepth;                                               // the module's calls under way: more than one when a USER(
                                                                    // is in a USER's arguments; USER LOAD and CLOSE wait for none

/* ---- the interpreter's routines, as cmm2user.h's table has them ---- */
static char *u_checkstring(char *p, const char *word) { return checkstring(p, (char *)word); }
static void u_getargs(char **p, int max, const char *delims, char **argv, int *argc) {
    makeargs(p, max, GetTempMemory(STRINGSIZE + STRINGSIZE / 2), argv, argc, (char *)delims);   // (getargs' room)
}
static int u_token(const char *name) {                              // (GetTokenValue's search, in any case, with 0 for a name
    for(int i = 0; i < TokenTableSize - 1; i++)                     // it has not)
        if(strcasecmp(name, (const char *)tokentbl[i].name) == 0) return i + C_BASETOKEN;
    return 0;
}
static char *u_closebracket(char *p) { return getclosebracket(p); }
static char *u_evaluate(char *p, cmm2_value *v) {
    MMFLOAT f = 0;
    long long int i = 0;
    char *s = NULL;
    int t = T_NOTYPE;
    p = evaluate(p, &f, &i, &s, &t, false);
    v->type = t & (T_NBR | T_INT | T_STR);
    v->f = f;
    v->i = i;
    v->s = s;
    return p;
}
static int64_t u_getinteger(char *p) { return getinteger(p); }
static int64_t u_getint(char *p, int64_t min, int64_t max) { return getint(p, min, max); }
static double u_getnumber(char *p) { return getnumber(p); }
static char *u_getstring(char *p) { return getstring(p); }
static char *u_getcstring(char *p) { return getCstring(p); }

// the variable p names: its data, and what it is in v. A whole array is one named with empty brackets, a()
static void *u_var(char *p, int mode, cmm2_var *v) {
    void *d;
    struct s_vartbl *r;
    char *e;
    int i, ty, whole = 0;
    memset(v, 0, sizeof(*v));
    skipspace(p);
    d = findvar(p, (mode == CMM2_VAR_WRITE ? V_FIND : V_NOFIND_ERR) | V_EMPTY_OK);
    r = VREC(VarIndex);
    v->constant = (r->type & T_CONST) != 0;
    if(mode == CMM2_VAR_WRITE && v->constant) error("Cannot change a constant");
    ty = TypeMask(FoundType());
    v->type = ty;
    if(ty & T_STR) v->size = FoundSize();
    else if(ty & T_STRUCT) v->size = g_structtbl[g_StructMemberType ? g_StructMemberSize : r->size]->total_size;
    else v->size = 8;
    e = p + strlen(p);
    while(e > p && e[-1] == ' ') e--;
    if(e > p && e[-1] == ')') {
        e--;
        while(e > p && e[-1] == ' ') e--;
        whole = e > p && e[-1] == '(';
    }
    v->elements = 1;
    if(g_StructMemberType) {
        if(strstr(p, "()")) error("Struct member arrays not supported for this command");
    } else if(whole && DimIsRealArray(RAW_DIM((*r), 0))) {
        for(i = 0; i < MAXDIM && !DimIsEnd(RAW_DIM((*r), i)); i++) {
            v->bound[i] = DimElements(RAW_DIM((*r), i));
            v->elements *= v->bound[i];
        }
        v->dims = i;
    }
    return d;
}

// the variable p names given the value v, as an assignment gives it
static void u_assign(char *p, const cmm2_value *val) {
    cmm2_var v;
    char *d = u_var(p, CMM2_VAR_WRITE, &v);
    if(v.dims) error("Expected a variable, not an array");
    if(v.type & T_STR) {
        int n;
        if(!(val->type & T_STR)) error("Expected a string");
        n = (unsigned char)*val->s;
        if(n > v.size) error("String too long");
        memmove(d, val->s, n + 1);
    } else if(v.type & (T_INT | T_NBR)) {
        if(!(val->type & (T_INT | T_NBR))) error("Expected a number");
        if(v.type & T_INT) *(long long int *)d = (val->type & T_INT) ? val->i : FloatToInt64(val->f);
        else *(MMFLOAT *)d = (val->type & T_NBR) ? val->f : (MMFLOAT)val->i;
    } else error("Expected a number or a string variable");
}
static void *u_temp(int bytes) { return GetTempMemory(bytes); }
static void u_print(const char *s) { MMPrintString((char *)s); }
static void u_inttostr(char *out, int64_t n, int base) { IntToStr(out, n, base); }
static void u_floattostr(char *out, double f, int m, int n, char pad) { FloatToStr(out, f, m, n, pad); }
static char *u_ctom(char *s) { return CtoM(s); }
static char *u_mtoc(char *s) { return MtoC(s); }
static int u_inprogram(void) { return CurrentLinePtr != NULL; }
static int u_optionbase(void) { return OptionBase; }

static const struct cmm2_basic userbasic = {
    .version = CMM2USER_VERSION,
    .size = sizeof(struct cmm2_basic),
    .checkstring = u_checkstring,
    .getargs = u_getargs,
    .token = u_token,
    .close_bracket = u_closebracket,
    .evaluate = u_evaluate,
    .get_integer = u_getinteger,
    .get_int = u_getint,
    .get_number = u_getnumber,
    .get_string = u_getstring,
    .get_cstring = u_getcstring,
    .var = u_var,
    .assign = u_assign,
    .temp = u_temp,
    .error = (void (*)(const char *, ...))error,
    .check_abort = CheckAbort,
    .print = u_print,
    .int_to_str = u_inttostr,
    .float_to_str = u_floattostr,
    .c_to_m = u_ctom,
    .m_to_c = u_mtoc,
    .vsnprintf = vsnprintf,                                         // (newlib's, with floats: the firmware's own)
    .in_program = u_inprogram,
    .option_base = u_optionbase,
};

/* ---- the module ---- */

// name loaded in the USER slot, afresh: its table, or an error (one that is not a USER module unloaded)
static const cmm2user_head *userload(char *name) {
    const cmm2user_head *t;
    LoaderModuleDrop(LOADER_MOD_USER);                              // (the same file too: a new build of it is read)
    t = LoaderModuleLoad(LOADER_MOD_USER, name, 0, &userbasic);
    if(t->magic != CMM2USER_MAGIC) { LoaderModuleDrop(LOADER_MOD_USER); error("$ is not a USER module", name); }
    if(t->head.version > CMM2USER_VERSION) { LoaderModuleDrop(LOADER_MOD_USER); error("$ needs a newer firmware", name); }
    return t;
}

// the module: loaded again if the heap has gone since USER LOAD, an error if there is none
static const cmm2user_head *usernow(void) {
    const cmm2user_head *t = LoaderModuleTable(LOADER_MOD_USER);
    if(t) return t;
    if(!username[0]) error("USER LOAD first");
    return userload(username);
}

// the module's command (v NULL) or function called, counted in userdepth while it runs. An error inside it (to the
// prompt, or to a program's ON ERROR SKIP or IGNORE), Ctrl-C or an END in a FUNCTION its arguments call comes through
// here first, to be counted out, then goes on where it was going
static void usercall(const cmm2user_head *t, char *text, cmm2_value *v) {
    jmp_buf savemark, savenext;
    volatile int depth = userdepth;
    memcpy(savemark, mark, sizeof(jmp_buf));
    memcpy(savenext, ErrNext, sizeof(jmp_buf));
    if(setjmp(mark)) {
        userdepth = depth;
        memcpy(ErrNext, savenext, sizeof(jmp_buf));
        memcpy(mark, savemark, sizeof(jmp_buf));
        longjmp(mark, 1);
    }
    if(setjmp(ErrNext)) {
        userdepth = depth;
        memcpy(mark, savemark, sizeof(jmp_buf));
        memcpy(ErrNext, savenext, sizeof(jmp_buf));
        longjmp(ErrNext, 1);
    }
    userdepth = depth + 1;
    if(v) t->function(text, v);
    else t->command(text);
    userdepth = depth;
    memcpy(ErrNext, savenext, sizeof(jmp_buf));
    memcpy(mark, savemark, sizeof(jmp_buf));
}

void cmd_user(void) {
    char *p;
    const cmm2user_head *t;
    if((p = checkstring(cmdline, "LOAD"))) {
        char name[FF_MAX_LFN + 8], *s = getCstring(p), *dot, *slash;
        if(userdepth) error("USER LOAD inside a USER command or function");
        if(strlen(s) >= FF_MAX_LFN - 4) error("Invalid file name");
        getfullpath(s, name);                                       // (a full path with its drive: the same file after a
        DrivePath(name);                                            // CHDIR, to another drive too)
        slash = strrchr(name, '/');
        dot = strrchr(name, '.');
        if(dot == NULL || (slash != NULL && dot < slash)) strcat(name, ".mod");
        username[0] = 0;
        userload(name);
        strcpy(username, name);
        return;
    }
    if(checkstring(cmdline, "CLOSE")) {
        if(userdepth) error("USER CLOSE inside a USER command or function");
        username[0] = 0;
        LoaderModuleDrop(LOADER_MOD_USER);
        return;
    }
    t = usernow();
    if(t->command == NULL) error("The USER module has no command");
    usercall(t, cmdline, NULL);
}

void fun_user(void) {
    const cmm2user_head *t = usernow();
    cmm2_value v;
    if(t->function == NULL) error("The USER module has no function");
    memset(&v, 0, sizeof(v));
    v.s = GetTempMemory(STRINGSIZE);                                // (the string's room: the module may point s elsewhere)
    usercall(t, ep, &v);
    if(v.type == T_STR) { sret = v.s; targ = T_STR; }
    else if(v.type == T_INT) { iret = v.i; targ = T_INT; }
    else if(v.type == T_NBR) { fret = v.f; targ = T_NBR; }
    else error("The USER function gave no value");
}
