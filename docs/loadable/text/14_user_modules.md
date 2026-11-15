# USER Modules

A loadable program runs and then hands the machine back to BASIC. A USER module is the other kind of loadable code: it adds commands and functions of your own to MMBasic, which a BASIC program calls as it calls the built-in ones. The SQL command works this way, with SQLite as its module; USER does the same for code you write.

```
USER LOAD "A:/mymod.mod"
USER LED ON
PRINT USER(TEMPERATURE 3)
```

MMBasic hands the module everything after USER, or everything inside the brackets of USER(, and the module reads its own keywords and arguments with the interpreter's own routines, as a built-in command does. A module is written in C with this SDK, built with build.py --user, and copied to the SD card. tools/sdk/examples/user/proforma.c is a working module to start from: copy it, rename it and change its tables.

## Using a Module from BASIC

{widths: 34,66}
| Statement | What it does |
|---|---|
| USER LOAD file$ | Loads the module, unloading any loaded before. .mod is added to a name with no extension. Loading the same file again reads it afresh, so a module can be rebuilt and reloaded without a restart. |
| USER CLOSE | Unloads the module. |
| USER anything | The module's command: it is given the rest of the statement. |
| USER(anything) | The module's function: it is given the text inside the brackets, and returns an integer, a float or a string. |

With no module loaded, USER and USER( give the error "USER LOAD first". LOAD and CLOSE are USER's own words, so a module cannot have commands of those names.

A module stays loaded until USER CLOSE, through any number of program runs at the command prompt. RUN, NEW and CHAIN take it away with the rest of the program's memory, but MMBasic remembers its file (as a full path, so a later CHDIR does not matter), and the next USER or USER( loads it again. The module then starts afresh: its variables have their initial values, and any count it kept starts again.

One USER module is loaded at a time, beside the SQL command's engine if that is open. A module is the same file on both generations.

## Building a Module

From tools/sdk:

```
python build.py out/proforma.mod examples/user/proforma.c --user
```

--user makes a USER module:

- a module rather than a program (--module: crt0.c calls module_main(), not main());
- services version 20, the one that brings the interpreter's routines, so older firmware refuses it with "needs a newer firmware";
- a 4KB stack, which it needs only while it starts, since its commands and functions run on MMBasic's stack (below);
- snprintf(), sprintf(), vsnprintf(), vsprintf() and printf() that are the firmware's own (userlib.c), floats included, so the module carries no printf of its own. The pro-forma is 3KB; with newlib's printf it was 17KB.

printf(), puts() and putchar() print through MMBasic's PRINT, on the console and the screen, a newline as CR LF. fprintf() and the rest of stdio on a FILE are newlib's own and have no floats in a --user build: to write a float to a file, snprintf() it into a buffer and fputs() that. --heap sets the memory malloc() has; with none the module has 64KB.

The module's code goes into the top 512KB of MMBasic's heap, the part that can hold code (where CSUBs run), from a 64KB boundary; its heap for malloc() goes into the ordinary heap.

## The Module's Table

module_main() is called once, when the module is loaded, and returns the module's table:

```
#include "cmm2user.h"

static const struct cmm2_basic *B;              /* the interpreter's routines */

static const cmm2user_head table = {
    {CMM2MOD_MAGIC, CMM2USER_VERSION, forget},  /* any module's header, and its forget() */
    CMM2USER_MAGIC,
    command,                                    /* USER ...  (or NULL) */
    function,                                   /* USER(...) (or NULL) */
};

const void *module_main(void) {
    B = cmm2_start->basic;
    if(B == NULL || B->version < CMM2USER_VERSION || B->size < sizeof(struct cmm2_basic))
        return NULL;                            /* a firmware older than the header: "not a module" */
    return &table;
}
```

{widths: 34,66}
| Entry | Called |
|---|---|
| void command(char *line) | For each USER command, with the statement after USER. |
| void function(char *args, cmm2_value *r) | For each USER(, with the text inside the brackets. It sets r->type and the value (below). |
| void forget(void) | Before the module goes: USER CLOSE, another USER LOAD, RUN, NEW or CHAIN. It gives back anything that would outlive the module. Files the module left open are closed after it returns. |

cmm2user.h (tools/sdk) brings in cmm2.h and the firmware's Inc/cmm2user.h, which defines everything in this chapter.

## What the Module Is Given

The text is the statement as MMBasic stores it. MMBasic's functions and operators are each one byte, 0x80 or more: =, +, AND, NOT, TO, STEP, THEN, ELSE, LEN( and the rest. Everything else is text, with the spaces MMBasic takes out gone and the names of variables spelt out. So USER LED ON gives the module "LED ON", and USER SET x = 5 gives "SET x", the byte for =, then "5".

A module's keywords should therefore be plain words that are not MMBasic functions or operators: B->checkstring() finds a word that is text, and B->token("=") names an operator's byte when the module needs to look for one. A table of keywords and routines, as in the pro-forma, keeps this simple:

```
static void command(char *line) {
    char *p;
    if((p = B->checkstring(line, "HELLO")) != NULL) { c_hello(p); return; }
    if((p = B->checkstring(line, "SWAP")) != NULL)  { c_swap(p); return; }
    B->error("Unknown USER command");
}
```

## The Interpreter's Routines

cmm2_start->basic points to a table of the interpreter's own routines. Each works exactly as it does for MMBasic's built-in commands.

{widths: 44,56}
| Routine | What it does |
|---|---|
| char *checkstring(p, word) | If p, after its spaces, starts with word (in any case, and not as the start of a longer name), the text after it; else NULL. |
| void getargs(&p, max, delims, argv, &argc) | Splits the text at any of the characters in delims, outside brackets and strings. argv[0], argv[2] ... are the items (an empty one is ""), argv[1], argv[3] ... the delimiters between them, and argc counts both: 2n - 1 for n items. More than max is the error "Argument count". |
| int token(name) | The byte of a function or an operator ("=", "TO", "SIN("), or 0. |
| char *close_bracket(p) | For p just after a (, its matching ). |
| char *evaluate(p, &v) | The expression at p: its value and type in v, and where it ends. |
| int64_t get_integer(p) | The expression as an integer (a float is rounded). |
| int64_t get_int(p, min, max) | The same, with an error outside min to max. |
| double get_number(p) | The expression as a float. |
| char *get_string(p) | The expression as an MMBasic string. |
| char *get_cstring(p) | The expression as a C string. |
| void *var(p, mode, &v) | The variable p names: its data, and what it is in v (below). |
| void assign(p, &v) | The variable p names given the value v, as LET gives it. |
| void *temp(bytes) | Zeroed memory, freed when the statement ends. |
| void error(msg, ...) | MMBasic's error. In msg, $ is replaced by the next argument (a char *) and % by the next (an int). |
| void check_abort(void) | MMBasic's break if Ctrl-C has been pressed: call it in a long loop. |
| void print(s) | Prints a C string on the console and the screen, as PRINT does. |
| void int_to_str(out, n, base) | An integer as text in base 2 to 16. |
| void float_to_str(out, f, m, n, pad) | A float as text: m digits before the point (0: as many as it needs), n after it (999: as PRINT shows it), padded before with pad. |
| char *c_to_m(s), char *m_to_c(s) | A C string made an MMBasic string, or back, in place. |
| int in_program(void) | 1 in a program, 0 at the command prompt. |
| int option_base(void) | OPTION BASE: 0 or 1. |
| int vsnprintf(out, n, fmt, ap) | The firmware's own vsnprintf, floats included. A --user build's snprintf(), sprintf() and printf() call it. A loadable program cannot have it: the firmware's printf may take memory from the firmware's heap for a float, which only code that runs inside a BASIC statement, as a module's does, can allow. |

Strings that these routines return are in temporary memory, freed when the statement ends: copy anything the module wants to keep. A string that evaluate() gives can be a variable's own, so a module must not change it.

## Values

An MMBasic string is a length byte (0 to 255) followed by its characters, with no zero at the end. A value is:

```
typedef struct {
    int      type;          /* CMM2_T_NBR (a double), CMM2_T_INT (an int64_t) or CMM2_T_STR */
    double   f;
    int64_t  i;
    char    *s;             /* an MMBasic string */
} cmm2_value;
```

A function sets r->type and the value. For a string, r->s already points to room for one (CMM2_STRINGSIZE bytes) for the module to write into, or the module can point r->s at a string of its own that lasts until the statement ends.

## Variables

var() finds the variable a piece of text names, as x, n%, s$, a(3), rec.field or a whole array a(), and returns where its data is. CMM2_VAR_READ asks for one that is already there; CMM2_VAR_WRITE makes it if it is not (with OPTION EXPLICIT's error, as an assignment would), and refuses a constant.

```
typedef struct {
    int type;               /* CMM2_T_NBR, CMM2_T_INT, CMM2_T_STR or CMM2_T_STRUCT */
    int size;               /* a string's longest length; a structure's bytes; 8 for a number */
    int constant;           /* 1: a CONST */
    int dims;               /* 0: one variable, element or member; 1-5: a whole array, a() */
    int bound[5];           /* each dimension's elements, OPTION BASE allowed for */
    int elements;           /* all its elements: 1 unless dims */
} cmm2_var;
```

A whole array's elements follow each other in memory, the first dimension changing fastest: a number takes 8 bytes, a string size + 1 (its length byte first), a structure size. The pro-forma's FILL and TOTAL commands show both.

To give a variable a value, assign() is simplest: it converts a float to an integer or the other way as LET does, and gives MMBasic's errors for a string too long or a string given to a number.

## Errors, Memory and the Stack

B->error() is MMBasic's error. It ends the statement at once, so nothing after it in the module runs: the program stops with the message, or goes on as its ON ERROR says. Any of the routines above can end the module's command the same way, on a bad argument. Free anything malloc() gave before calling error(), or keep it in a static variable to reuse.

The module's commands and functions run on MMBasic's own stack, as a CSUB does. That is what lets them call the interpreter, and lets a USER( in a USER command's arguments call the module again. The stack is 18KB shared with the interpreter, so keep large buffers out of local variables: use B->temp() for memory needed only during the statement, and malloc() or static variables for memory kept between calls.

A module can use the C library (strings, maths, snprintf() and printf(), malloc, files with fopen()) and the file and time services. Files it opens are its own: they are closed when the module goes, not when a program run at the prompt ends. The services that take over the screen, the sound or the console are for programs that run and end, and a module should leave them alone.

USER LOAD and USER CLOSE cannot be used while one of the module's commands or functions is running, such as from a BASIC FUNCTION called in its arguments.

## The Pro-forma

tools/sdk/examples/user/proforma.c has four commands and four functions, each a few lines:

{widths: 38,62}
| BASIC | What it shows |
|---|---|
| USER HELLO [name$] | An optional argument that is an expression. |
| USER SWAP a, b | Two variables found by var() and changed in place. |
| USER FILL a(), value | A whole array of numbers or strings. |
| USER TOTAL a(), t | Reading an array, and assign() to give a variable the result. |
| USER(ADD a, b) | Two values by evaluate(): an integer result or a float. |
| USER(REVERSE$ s$) | A string result. |
| USER(FIXED$ x, n) | snprintf() with a float, the firmware's own. |
| USER(CALLS) | State kept in the module between calls. |

Its comments describe the rest. To make a module of your own, copy it, keep module_main(), the two dispatch routines and forget(), and replace the commands and functions in the two tables.
