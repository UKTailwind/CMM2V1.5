/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "gram.y"

#include <stdlib.h>
#include <string.h>

#include "sc.h"
#include "cmds/cmds.h"
#include "interp.h"
#include "macros.h"
#include "actions/sort.h"
#include "actions/filter.h"
#include "maps.h"
#include "marks.h"
#include "xmalloc.h" // for scxfree
#include "actions/hide_show.h"
#include "cmds/cmds_normal.h"
#include "conf.h"
#include "pipe.h"
#include "main.h"
#include "file.h"
#include "tui.h"
#include "undo.h"
#include "yank.h"
#include "graph.h"
#include "utils/dictionary.h"
#include "trigger.h"
#include "actions/shift.h"
#include "clipboard.h"
#include "actions/plot.h"
#include "actions/subtotal.h"
#include "actions/freeze.h"
#include "sheet.h"
#include "vmtbl.h"
#include "cmds/cmds_command.h"

void yyerror(char *err);               // error routine for yacc (gram.y)
int yylex();
extern struct session * session;

#ifdef USELOCALE
#include <locale.h>
#endif

#ifndef MSDOS
#include <unistd.h>
#endif

#define ENULL (struct enode *) 0

#line 120 "y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    STRING = 258,                  /* STRING  */
    COL = 259,                     /* COL  */
    NUMBER = 260,                  /* NUMBER  */
    FNUMBER = 261,                 /* FNUMBER  */
    RANGE = 262,                   /* RANGE  */
    VAR = 263,                     /* VAR  */
    WORD = 264,                    /* WORD  */
    MAPWORD = 265,                 /* MAPWORD  */
    PLUGIN = 266,                  /* PLUGIN  */
    S_SHOW = 267,                  /* S_SHOW  */
    S_HIDE = 268,                  /* S_HIDE  */
    S_SHOWROW = 269,               /* S_SHOWROW  */
    S_HIDEROW = 270,               /* S_HIDEROW  */
    S_SHOWCOL = 271,               /* S_SHOWCOL  */
    S_HIDECOL = 272,               /* S_HIDECOL  */
    S_FREEZE = 273,                /* S_FREEZE  */
    S_UNFREEZE = 274,              /* S_UNFREEZE  */
    S_MARK = 275,                  /* S_MARK  */
    S_AUTOFIT = 276,               /* S_AUTOFIT  */
    S_PAD = 277,                   /* S_PAD  */
    S_DELETECOL = 278,             /* S_DELETECOL  */
    S_DATEFMT = 279,               /* S_DATEFMT  */
    S_SUBTOTAL = 280,              /* S_SUBTOTAL  */
    S_RSUBTOTAL = 281,             /* S_RSUBTOTAL  */
    S_FORMAT = 282,                /* S_FORMAT  */
    S_FMT = 283,                   /* S_FMT  */
    S_LET = 284,                   /* S_LET  */
    S_LABEL = 285,                 /* S_LABEL  */
    S_LEFTSTRING = 286,            /* S_LEFTSTRING  */
    S_RIGHTSTRING = 287,           /* S_RIGHTSTRING  */
    S_LEFTJUSTIFY = 288,           /* S_LEFTJUSTIFY  */
    S_RIGHTJUSTIFY = 289,          /* S_RIGHTJUSTIFY  */
    S_CENTER = 290,                /* S_CENTER  */
    S_SORT = 291,                  /* S_SORT  */
    S_FILTERON = 292,              /* S_FILTERON  */
    S_GOTO = 293,                  /* S_GOTO  */
    S_GOTOB = 294,                 /* S_GOTOB  */
    S_CCOPY = 295,                 /* S_CCOPY  */
    S_CPASTE = 296,                /* S_CPASTE  */
    S_PLOT = 297,                  /* S_PLOT  */
    S_LOCK = 298,                  /* S_LOCK  */
    S_UNLOCK = 299,                /* S_UNLOCK  */
    S_DEFINE = 300,                /* S_DEFINE  */
    S_UNDEFINE = 301,              /* S_UNDEFINE  */
    S_DETAIL = 302,                /* S_DETAIL  */
    S_EVAL = 303,                  /* S_EVAL  */
    S_SEVAL = 304,                 /* S_SEVAL  */
    S_ERROR = 305,                 /* S_ERROR  */
    S_FILL = 306,                  /* S_FILL  */
    S_STRTONUM = 307,              /* S_STRTONUM  */
    S_DELETEROW = 308,             /* S_DELETEROW  */
    S_VALUEIZEALL = 309,           /* S_VALUEIZEALL  */
    S_SHIFT = 310,                 /* S_SHIFT  */
    S_GETNUM = 311,                /* S_GETNUM  */
    S_YANKAREA = 312,              /* S_YANKAREA  */
    S_PASTEYANKED = 313,           /* S_PASTEYANKED  */
    S_GETSTRING = 314,             /* S_GETSTRING  */
    S_GETEXP = 315,                /* S_GETEXP  */
    S_GETFMT = 316,                /* S_GETFMT  */
    S_GETFORMAT = 317,             /* S_GETFORMAT  */
    S_RECALC = 318,                /* S_RECALC  */
    S_EXECUTE = 319,               /* S_EXECUTE  */
    S_QUIT = 320,                  /* S_QUIT  */
    S_EXPORT = 321,                /* S_EXPORT  */
    S_REBUILD_GRAPH = 322,         /* S_REBUILD_GRAPH  */
    S_PRINT_GRAPH = 323,           /* S_PRINT_GRAPH  */
    S_SYNCREFS = 324,              /* S_SYNCREFS  */
    S_REDO = 325,                  /* S_REDO  */
    S_UNDO = 326,                  /* S_UNDO  */
    S_IMAP = 327,                  /* S_IMAP  */
    S_EMAP = 328,                  /* S_EMAP  */
    S_CMAP = 329,                  /* S_CMAP  */
    S_NEWSHEET = 330,              /* S_NEWSHEET  */
    S_NEXTSHEET = 331,             /* S_NEXTSHEET  */
    S_PREVSHEET = 332,             /* S_PREVSHEET  */
    S_DELSHEET = 333,              /* S_DELSHEET  */
    S_MOVETOSHEET = 334,           /* S_MOVETOSHEET  */
    S_MOVESHEET = 335,             /* S_MOVESHEET  */
    S_RENAMESHEET = 336,           /* S_RENAMESHEET  */
    S_NMAP = 337,                  /* S_NMAP  */
    S_VMAP = 338,                  /* S_VMAP  */
    S_INOREMAP = 339,              /* S_INOREMAP  */
    S_ENOREMAP = 340,              /* S_ENOREMAP  */
    S_CNOREMAP = 341,              /* S_CNOREMAP  */
    S_NNOREMAP = 342,              /* S_NNOREMAP  */
    S_VNOREMAP = 343,              /* S_VNOREMAP  */
    S_NUNMAP = 344,                /* S_NUNMAP  */
    S_IUNMAP = 345,                /* S_IUNMAP  */
    S_EUNMAP = 346,                /* S_EUNMAP  */
    S_CUNMAP = 347,                /* S_CUNMAP  */
    S_VUNMAP = 348,                /* S_VUNMAP  */
    S_COLOR = 349,                 /* S_COLOR  */
    S_CELLCOLOR = 350,             /* S_CELLCOLOR  */
    S_UNFORMAT = 351,              /* S_UNFORMAT  */
    S_REDEFINE_COLOR = 352,        /* S_REDEFINE_COLOR  */
    S_DEFINE_COLOR = 353,          /* S_DEFINE_COLOR  */
    S_SET = 354,                   /* S_SET  */
    S_FCOPY = 355,                 /* S_FCOPY  */
    S_FSUM = 356,                  /* S_FSUM  */
    S_TRIGGER = 357,               /* S_TRIGGER  */
    S_UNTRIGGER = 358,             /* S_UNTRIGGER  */
    S_OFFSCR_SC_COLS = 359,        /* S_OFFSCR_SC_COLS  */
    S_OFFSCR_SC_ROWS = 360,        /* S_OFFSCR_SC_ROWS  */
    S_NB_FROZEN_ROWS = 361,        /* S_NB_FROZEN_ROWS  */
    S_NB_FROZEN_COLS = 362,        /* S_NB_FROZEN_COLS  */
    S_NB_FROZEN_SCREENROWS = 363,  /* S_NB_FROZEN_SCREENROWS  */
    S_NB_FROZEN_SCREENCOLS = 364,  /* S_NB_FROZEN_SCREENCOLS  */
    K_AUTOBACKUP = 365,            /* K_AUTOBACKUP  */
    K_NOAUTOBACKUP = 366,          /* K_NOAUTOBACKUP  */
    K_AUTOCALC = 367,              /* K_AUTOCALC  */
    K_NOAUTOCALC = 368,            /* K_NOAUTOCALC  */
    K_DEBUG = 369,                 /* K_DEBUG  */
    K_NODEBUG = 370,               /* K_NODEBUG  */
    K_TRG = 371,                   /* K_TRG  */
    K_NOTRG = 372,                 /* K_NOTRG  */
    K_EXTERNAL_FUNCTIONS = 373,    /* K_EXTERNAL_FUNCTIONS  */
    K_NOEXTERNAL_FUNCTIONS = 374,  /* K_NOEXTERNAL_FUNCTIONS  */
    K_EXEC_LUA = 375,              /* K_EXEC_LUA  */
    K_NOEXEC_LUA = 376,            /* K_NOEXEC_LUA  */
    K_HALF_PAGE_SCROLL = 377,      /* K_HALF_PAGE_SCROLL  */
    K_NOHALF_PAGE_SCROLL = 378,    /* K_NOHALF_PAGE_SCROLL  */
    K_HISTORY_FILE = 379,          /* K_HISTORY_FILE  */
    K_NOHISTORY_FILE = 380,        /* K_NOHISTORY_FILE  */
    K_NOCURSES = 381,              /* K_NOCURSES  */
    K_CURSES = 382,                /* K_CURSES  */
    K_NUMERIC = 383,               /* K_NUMERIC  */
    K_NONUMERIC = 384,             /* K_NONUMERIC  */
    K_NUMERIC_DECIMAL = 385,       /* K_NUMERIC_DECIMAL  */
    K_NONUMERIC_DECIMAL = 386,     /* K_NONUMERIC_DECIMAL  */
    K_NUMERIC_ZERO = 387,          /* K_NUMERIC_ZERO  */
    K_NONUMERIC_ZERO = 388,        /* K_NONUMERIC_ZERO  */
    K_OVERLAP = 389,               /* K_OVERLAP  */
    K_NOOVERLAP = 390,             /* K_NOOVERLAP  */
    K_INPUT_BAR_BOTTOM = 391,      /* K_INPUT_BAR_BOTTOM  */
    K_IGNORE_HIDDEN = 392,         /* K_IGNORE_HIDDEN  */
    K_NOIGNORE_HIDDEN = 393,       /* K_NOIGNORE_HIDDEN  */
    K_INPUT_EDIT_MODE = 394,       /* K_INPUT_EDIT_MODE  */
    K_UNDERLINE_GRID = 395,        /* K_UNDERLINE_GRID  */
    K_TRUNCATE = 396,              /* K_TRUNCATE  */
    K_NOTRUNCATE = 397,            /* K_NOTRUNCATE  */
    K_AUTOWRAP = 398,              /* K_AUTOWRAP  */
    K_NOAUTOWRAP = 399,            /* K_NOAUTOWRAP  */
    K_QUIET = 400,                 /* K_QUIET  */
    K_NOQUIET = 401,               /* K_NOQUIET  */
    K_QUIT_AFTERLOAD = 402,        /* K_QUIT_AFTERLOAD  */
    K_NOQUIT_AFTERLOAD = 403,      /* K_NOQUIT_AFTERLOAD  */
    K_XLSX_READFORMULAS = 404,     /* K_XLSX_READFORMULAS  */
    K_NOXLSX_READFORMULAS = 405,   /* K_NOXLSX_READFORMULAS  */
    K_DEFAULT_COPY_TO_CLIPBOARD_CMD = 406, /* K_DEFAULT_COPY_TO_CLIPBOARD_CMD  */
    K_DEFAULT_PASTE_FROM_CLIPBOARD_CMD = 407, /* K_DEFAULT_PASTE_FROM_CLIPBOARD_CMD  */
    K_COPY_TO_CLIPBOARD_DELIMITED_TAB = 408, /* K_COPY_TO_CLIPBOARD_DELIMITED_TAB  */
    K_NOCOPY_TO_CLIPBOARD_DELIMITED_TAB = 409, /* K_NOCOPY_TO_CLIPBOARD_DELIMITED_TAB  */
    K_COPY_TO_CLIPBOARD_WYSIWYG = 410, /* K_COPY_TO_CLIPBOARD_WYSIWYG  */
    K_NOCOPY_TO_CLIPBOARD_WYSIWYG = 411, /* K_NOCOPY_TO_CLIPBOARD_WYSIWYG  */
    K_DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD = 412, /* K_DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD  */
    K_IMPORT_DELIMITED_TO_TEXT = 413, /* K_IMPORT_DELIMITED_TO_TEXT  */
    K_IGNORECASE = 414,            /* K_IGNORECASE  */
    K_NOIGNORECASE = 415,          /* K_NOIGNORECASE  */
    K_TM_GMTOFF = 416,             /* K_TM_GMTOFF  */
    K_COMMAND_TIMEOUT = 417,       /* K_COMMAND_TIMEOUT  */
    K_MAPPING_TIMEOUT = 418,       /* K_MAPPING_TIMEOUT  */
    K_NEWLINE_ACTION = 419,        /* K_NEWLINE_ACTION  */
    K_SHOW_CURSOR = 420,           /* K_SHOW_CURSOR  */
    K_NOSHOW_CURSOR = 421,         /* K_NOSHOW_CURSOR  */
    K_ERROR = 422,                 /* K_ERROR  */
    K_INVALID = 423,               /* K_INVALID  */
    K_FIXED = 424,                 /* K_FIXED  */
    K_SUM = 425,                   /* K_SUM  */
    K_PROD = 426,                  /* K_PROD  */
    K_AVG = 427,                   /* K_AVG  */
    K_STDDEV = 428,                /* K_STDDEV  */
    K_COUNT = 429,                 /* K_COUNT  */
    K_ROWS = 430,                  /* K_ROWS  */
    K_COLS = 431,                  /* K_COLS  */
    K_ABS = 432,                   /* K_ABS  */
    K_FROW = 433,                  /* K_FROW  */
    K_FCOL = 434,                  /* K_FCOL  */
    K_ACOS = 435,                  /* K_ACOS  */
    K_ASIN = 436,                  /* K_ASIN  */
    K_ATAN = 437,                  /* K_ATAN  */
    K_ATAN2 = 438,                 /* K_ATAN2  */
    K_CEIL = 439,                  /* K_CEIL  */
    K_COS = 440,                   /* K_COS  */
    K_EXP = 441,                   /* K_EXP  */
    K_FABS = 442,                  /* K_FABS  */
    K_FLOOR = 443,                 /* K_FLOOR  */
    K_HYPOT = 444,                 /* K_HYPOT  */
    K_LN = 445,                    /* K_LN  */
    K_LOG = 446,                   /* K_LOG  */
    K_PI = 447,                    /* K_PI  */
    K_POW = 448,                   /* K_POW  */
    K_SIN = 449,                   /* K_SIN  */
    K_SQRT = 450,                  /* K_SQRT  */
    K_TAN = 451,                   /* K_TAN  */
    K_DTR = 452,                   /* K_DTR  */
    K_RTD = 453,                   /* K_RTD  */
    K_MAX = 454,                   /* K_MAX  */
    K_MIN = 455,                   /* K_MIN  */
    K_RND = 456,                   /* K_RND  */
    K_ROUND = 457,                 /* K_ROUND  */
    K_IF = 458,                    /* K_IF  */
    K_PV = 459,                    /* K_PV  */
    K_FV = 460,                    /* K_FV  */
    K_PMT = 461,                   /* K_PMT  */
    K_HOUR = 462,                  /* K_HOUR  */
    K_MINUTE = 463,                /* K_MINUTE  */
    K_SECOND = 464,                /* K_SECOND  */
    K_MONTH = 465,                 /* K_MONTH  */
    K_DAY = 466,                   /* K_DAY  */
    K_YEAR = 467,                  /* K_YEAR  */
    K_NOW = 468,                   /* K_NOW  */
    K_DATE = 469,                  /* K_DATE  */
    K_DTS = 470,                   /* K_DTS  */
    K_TTS = 471,                   /* K_TTS  */
    K_FMT = 472,                   /* K_FMT  */
    K_REPLACE = 473,               /* K_REPLACE  */
    K_SUBSTR = 474,                /* K_SUBSTR  */
    K_UPPER = 475,                 /* K_UPPER  */
    K_LOWER = 476,                 /* K_LOWER  */
    K_CAPITAL = 477,               /* K_CAPITAL  */
    K_STON = 478,                  /* K_STON  */
    K_SLEN = 479,                  /* K_SLEN  */
    K_EQS = 480,                   /* K_EQS  */
    K_EXT = 481,                   /* K_EXT  */
    K_EVALUATE = 482,              /* K_EVALUATE  */
    K_SEVALUATE = 483,             /* K_SEVALUATE  */
    K_LUA = 484,                   /* K_LUA  */
    K_NVAL = 485,                  /* K_NVAL  */
    K_SVAL = 486,                  /* K_SVAL  */
    K_LOOKUP = 487,                /* K_LOOKUP  */
    K_HLOOKUP = 488,               /* K_HLOOKUP  */
    K_VLOOKUP = 489,               /* K_VLOOKUP  */
    K_INDEX = 490,                 /* K_INDEX  */
    K_STINDEX = 491,               /* K_STINDEX  */
    K_GETENT = 492,                /* K_GETENT  */
    K_TBLSTYLE = 493,              /* K_TBLSTYLE  */
    K_TBL = 494,                   /* K_TBL  */
    K_LATEX = 495,                 /* K_LATEX  */
    K_SLATEX = 496,                /* K_SLATEX  */
    K_TEX = 497,                   /* K_TEX  */
    K_FRAME = 498,                 /* K_FRAME  */
    K_RNDTOEVEN = 499,             /* K_RNDTOEVEN  */
    K_FILENAME = 500,              /* K_FILENAME  */
    K_MYROW = 501,                 /* K_MYROW  */
    K_MYCOL = 502,                 /* K_MYCOL  */
    K_COLORIZE = 503,              /* K_COLORIZE  */
    K_LASTROW = 504,               /* K_LASTROW  */
    K_LASTCOL = 505,               /* K_LASTCOL  */
    K_COLTOA = 506,                /* K_COLTOA  */
    K_CRACTION = 507,              /* K_CRACTION  */
    K_CRROW = 508,                 /* K_CRROW  */
    K_CRCOL = 509,                 /* K_CRCOL  */
    K_ROWLIMIT = 510,              /* K_ROWLIMIT  */
    K_COLLIMIT = 511,              /* K_COLLIMIT  */
    K_PAGESIZE = 512,              /* K_PAGESIZE  */
    K_ERR = 513,                   /* K_ERR  */
    K_REF = 514,                   /* K_REF  */
    K_LOCALE = 515,                /* K_LOCALE  */
    K_SET8BIT = 516,               /* K_SET8BIT  */
    K_ASCII = 517,                 /* K_ASCII  */
    K_CHR = 518,                   /* K_CHR  */
    K_FACT = 519                   /* K_FACT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define STRING 258
#define COL 259
#define NUMBER 260
#define FNUMBER 261
#define RANGE 262
#define VAR 263
#define WORD 264
#define MAPWORD 265
#define PLUGIN 266
#define S_SHOW 267
#define S_HIDE 268
#define S_SHOWROW 269
#define S_HIDEROW 270
#define S_SHOWCOL 271
#define S_HIDECOL 272
#define S_FREEZE 273
#define S_UNFREEZE 274
#define S_MARK 275
#define S_AUTOFIT 276
#define S_PAD 277
#define S_DELETECOL 278
#define S_DATEFMT 279
#define S_SUBTOTAL 280
#define S_RSUBTOTAL 281
#define S_FORMAT 282
#define S_FMT 283
#define S_LET 284
#define S_LABEL 285
#define S_LEFTSTRING 286
#define S_RIGHTSTRING 287
#define S_LEFTJUSTIFY 288
#define S_RIGHTJUSTIFY 289
#define S_CENTER 290
#define S_SORT 291
#define S_FILTERON 292
#define S_GOTO 293
#define S_GOTOB 294
#define S_CCOPY 295
#define S_CPASTE 296
#define S_PLOT 297
#define S_LOCK 298
#define S_UNLOCK 299
#define S_DEFINE 300
#define S_UNDEFINE 301
#define S_DETAIL 302
#define S_EVAL 303
#define S_SEVAL 304
#define S_ERROR 305
#define S_FILL 306
#define S_STRTONUM 307
#define S_DELETEROW 308
#define S_VALUEIZEALL 309
#define S_SHIFT 310
#define S_GETNUM 311
#define S_YANKAREA 312
#define S_PASTEYANKED 313
#define S_GETSTRING 314
#define S_GETEXP 315
#define S_GETFMT 316
#define S_GETFORMAT 317
#define S_RECALC 318
#define S_EXECUTE 319
#define S_QUIT 320
#define S_EXPORT 321
#define S_REBUILD_GRAPH 322
#define S_PRINT_GRAPH 323
#define S_SYNCREFS 324
#define S_REDO 325
#define S_UNDO 326
#define S_IMAP 327
#define S_EMAP 328
#define S_CMAP 329
#define S_NEWSHEET 330
#define S_NEXTSHEET 331
#define S_PREVSHEET 332
#define S_DELSHEET 333
#define S_MOVETOSHEET 334
#define S_MOVESHEET 335
#define S_RENAMESHEET 336
#define S_NMAP 337
#define S_VMAP 338
#define S_INOREMAP 339
#define S_ENOREMAP 340
#define S_CNOREMAP 341
#define S_NNOREMAP 342
#define S_VNOREMAP 343
#define S_NUNMAP 344
#define S_IUNMAP 345
#define S_EUNMAP 346
#define S_CUNMAP 347
#define S_VUNMAP 348
#define S_COLOR 349
#define S_CELLCOLOR 350
#define S_UNFORMAT 351
#define S_REDEFINE_COLOR 352
#define S_DEFINE_COLOR 353
#define S_SET 354
#define S_FCOPY 355
#define S_FSUM 356
#define S_TRIGGER 357
#define S_UNTRIGGER 358
#define S_OFFSCR_SC_COLS 359
#define S_OFFSCR_SC_ROWS 360
#define S_NB_FROZEN_ROWS 361
#define S_NB_FROZEN_COLS 362
#define S_NB_FROZEN_SCREENROWS 363
#define S_NB_FROZEN_SCREENCOLS 364
#define K_AUTOBACKUP 365
#define K_NOAUTOBACKUP 366
#define K_AUTOCALC 367
#define K_NOAUTOCALC 368
#define K_DEBUG 369
#define K_NODEBUG 370
#define K_TRG 371
#define K_NOTRG 372
#define K_EXTERNAL_FUNCTIONS 373
#define K_NOEXTERNAL_FUNCTIONS 374
#define K_EXEC_LUA 375
#define K_NOEXEC_LUA 376
#define K_HALF_PAGE_SCROLL 377
#define K_NOHALF_PAGE_SCROLL 378
#define K_HISTORY_FILE 379
#define K_NOHISTORY_FILE 380
#define K_NOCURSES 381
#define K_CURSES 382
#define K_NUMERIC 383
#define K_NONUMERIC 384
#define K_NUMERIC_DECIMAL 385
#define K_NONUMERIC_DECIMAL 386
#define K_NUMERIC_ZERO 387
#define K_NONUMERIC_ZERO 388
#define K_OVERLAP 389
#define K_NOOVERLAP 390
#define K_INPUT_BAR_BOTTOM 391
#define K_IGNORE_HIDDEN 392
#define K_NOIGNORE_HIDDEN 393
#define K_INPUT_EDIT_MODE 394
#define K_UNDERLINE_GRID 395
#define K_TRUNCATE 396
#define K_NOTRUNCATE 397
#define K_AUTOWRAP 398
#define K_NOAUTOWRAP 399
#define K_QUIET 400
#define K_NOQUIET 401
#define K_QUIT_AFTERLOAD 402
#define K_NOQUIT_AFTERLOAD 403
#define K_XLSX_READFORMULAS 404
#define K_NOXLSX_READFORMULAS 405
#define K_DEFAULT_COPY_TO_CLIPBOARD_CMD 406
#define K_DEFAULT_PASTE_FROM_CLIPBOARD_CMD 407
#define K_COPY_TO_CLIPBOARD_DELIMITED_TAB 408
#define K_NOCOPY_TO_CLIPBOARD_DELIMITED_TAB 409
#define K_COPY_TO_CLIPBOARD_WYSIWYG 410
#define K_NOCOPY_TO_CLIPBOARD_WYSIWYG 411
#define K_DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD 412
#define K_IMPORT_DELIMITED_TO_TEXT 413
#define K_IGNORECASE 414
#define K_NOIGNORECASE 415
#define K_TM_GMTOFF 416
#define K_COMMAND_TIMEOUT 417
#define K_MAPPING_TIMEOUT 418
#define K_NEWLINE_ACTION 419
#define K_SHOW_CURSOR 420
#define K_NOSHOW_CURSOR 421
#define K_ERROR 422
#define K_INVALID 423
#define K_FIXED 424
#define K_SUM 425
#define K_PROD 426
#define K_AVG 427
#define K_STDDEV 428
#define K_COUNT 429
#define K_ROWS 430
#define K_COLS 431
#define K_ABS 432
#define K_FROW 433
#define K_FCOL 434
#define K_ACOS 435
#define K_ASIN 436
#define K_ATAN 437
#define K_ATAN2 438
#define K_CEIL 439
#define K_COS 440
#define K_EXP 441
#define K_FABS 442
#define K_FLOOR 443
#define K_HYPOT 444
#define K_LN 445
#define K_LOG 446
#define K_PI 447
#define K_POW 448
#define K_SIN 449
#define K_SQRT 450
#define K_TAN 451
#define K_DTR 452
#define K_RTD 453
#define K_MAX 454
#define K_MIN 455
#define K_RND 456
#define K_ROUND 457
#define K_IF 458
#define K_PV 459
#define K_FV 460
#define K_PMT 461
#define K_HOUR 462
#define K_MINUTE 463
#define K_SECOND 464
#define K_MONTH 465
#define K_DAY 466
#define K_YEAR 467
#define K_NOW 468
#define K_DATE 469
#define K_DTS 470
#define K_TTS 471
#define K_FMT 472
#define K_REPLACE 473
#define K_SUBSTR 474
#define K_UPPER 475
#define K_LOWER 476
#define K_CAPITAL 477
#define K_STON 478
#define K_SLEN 479
#define K_EQS 480
#define K_EXT 481
#define K_EVALUATE 482
#define K_SEVALUATE 483
#define K_LUA 484
#define K_NVAL 485
#define K_SVAL 486
#define K_LOOKUP 487
#define K_HLOOKUP 488
#define K_VLOOKUP 489
#define K_INDEX 490
#define K_STINDEX 491
#define K_GETENT 492
#define K_TBLSTYLE 493
#define K_TBL 494
#define K_LATEX 495
#define K_SLATEX 496
#define K_TEX 497
#define K_FRAME 498
#define K_RNDTOEVEN 499
#define K_FILENAME 500
#define K_MYROW 501
#define K_MYCOL 502
#define K_COLORIZE 503
#define K_LASTROW 504
#define K_LASTCOL 505
#define K_COLTOA 506
#define K_CRACTION 507
#define K_CRROW 508
#define K_CRCOL 509
#define K_ROWLIMIT 510
#define K_COLLIMIT 511
#define K_PAGESIZE 512
#define K_ERR 513
#define K_REF 514
#define K_LOCALE 515
#define K_SET8BIT 516
#define K_ASCII 517
#define K_CHR 518
#define K_FACT 519

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 50 "gram.y"

    int ival;
    double fval;
    struct ent_ptr ent;
    struct enode * enode;
    char * sval;
    struct range_s rval;

#line 710 "y.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_STRING = 3,                     /* STRING  */
  YYSYMBOL_COL = 4,                        /* COL  */
  YYSYMBOL_NUMBER = 5,                     /* NUMBER  */
  YYSYMBOL_FNUMBER = 6,                    /* FNUMBER  */
  YYSYMBOL_RANGE = 7,                      /* RANGE  */
  YYSYMBOL_VAR = 8,                        /* VAR  */
  YYSYMBOL_WORD = 9,                       /* WORD  */
  YYSYMBOL_MAPWORD = 10,                   /* MAPWORD  */
  YYSYMBOL_PLUGIN = 11,                    /* PLUGIN  */
  YYSYMBOL_S_SHOW = 12,                    /* S_SHOW  */
  YYSYMBOL_S_HIDE = 13,                    /* S_HIDE  */
  YYSYMBOL_S_SHOWROW = 14,                 /* S_SHOWROW  */
  YYSYMBOL_S_HIDEROW = 15,                 /* S_HIDEROW  */
  YYSYMBOL_S_SHOWCOL = 16,                 /* S_SHOWCOL  */
  YYSYMBOL_S_HIDECOL = 17,                 /* S_HIDECOL  */
  YYSYMBOL_S_FREEZE = 18,                  /* S_FREEZE  */
  YYSYMBOL_S_UNFREEZE = 19,                /* S_UNFREEZE  */
  YYSYMBOL_S_MARK = 20,                    /* S_MARK  */
  YYSYMBOL_S_AUTOFIT = 21,                 /* S_AUTOFIT  */
  YYSYMBOL_S_PAD = 22,                     /* S_PAD  */
  YYSYMBOL_S_DELETECOL = 23,               /* S_DELETECOL  */
  YYSYMBOL_S_DATEFMT = 24,                 /* S_DATEFMT  */
  YYSYMBOL_S_SUBTOTAL = 25,                /* S_SUBTOTAL  */
  YYSYMBOL_S_RSUBTOTAL = 26,               /* S_RSUBTOTAL  */
  YYSYMBOL_S_FORMAT = 27,                  /* S_FORMAT  */
  YYSYMBOL_S_FMT = 28,                     /* S_FMT  */
  YYSYMBOL_S_LET = 29,                     /* S_LET  */
  YYSYMBOL_S_LABEL = 30,                   /* S_LABEL  */
  YYSYMBOL_S_LEFTSTRING = 31,              /* S_LEFTSTRING  */
  YYSYMBOL_S_RIGHTSTRING = 32,             /* S_RIGHTSTRING  */
  YYSYMBOL_S_LEFTJUSTIFY = 33,             /* S_LEFTJUSTIFY  */
  YYSYMBOL_S_RIGHTJUSTIFY = 34,            /* S_RIGHTJUSTIFY  */
  YYSYMBOL_S_CENTER = 35,                  /* S_CENTER  */
  YYSYMBOL_S_SORT = 36,                    /* S_SORT  */
  YYSYMBOL_S_FILTERON = 37,                /* S_FILTERON  */
  YYSYMBOL_S_GOTO = 38,                    /* S_GOTO  */
  YYSYMBOL_S_GOTOB = 39,                   /* S_GOTOB  */
  YYSYMBOL_S_CCOPY = 40,                   /* S_CCOPY  */
  YYSYMBOL_S_CPASTE = 41,                  /* S_CPASTE  */
  YYSYMBOL_S_PLOT = 42,                    /* S_PLOT  */
  YYSYMBOL_S_LOCK = 43,                    /* S_LOCK  */
  YYSYMBOL_S_UNLOCK = 44,                  /* S_UNLOCK  */
  YYSYMBOL_S_DEFINE = 45,                  /* S_DEFINE  */
  YYSYMBOL_S_UNDEFINE = 46,                /* S_UNDEFINE  */
  YYSYMBOL_S_DETAIL = 47,                  /* S_DETAIL  */
  YYSYMBOL_S_EVAL = 48,                    /* S_EVAL  */
  YYSYMBOL_S_SEVAL = 49,                   /* S_SEVAL  */
  YYSYMBOL_S_ERROR = 50,                   /* S_ERROR  */
  YYSYMBOL_S_FILL = 51,                    /* S_FILL  */
  YYSYMBOL_S_STRTONUM = 52,                /* S_STRTONUM  */
  YYSYMBOL_S_DELETEROW = 53,               /* S_DELETEROW  */
  YYSYMBOL_S_VALUEIZEALL = 54,             /* S_VALUEIZEALL  */
  YYSYMBOL_S_SHIFT = 55,                   /* S_SHIFT  */
  YYSYMBOL_S_GETNUM = 56,                  /* S_GETNUM  */
  YYSYMBOL_S_YANKAREA = 57,                /* S_YANKAREA  */
  YYSYMBOL_S_PASTEYANKED = 58,             /* S_PASTEYANKED  */
  YYSYMBOL_S_GETSTRING = 59,               /* S_GETSTRING  */
  YYSYMBOL_S_GETEXP = 60,                  /* S_GETEXP  */
  YYSYMBOL_S_GETFMT = 61,                  /* S_GETFMT  */
  YYSYMBOL_S_GETFORMAT = 62,               /* S_GETFORMAT  */
  YYSYMBOL_S_RECALC = 63,                  /* S_RECALC  */
  YYSYMBOL_S_EXECUTE = 64,                 /* S_EXECUTE  */
  YYSYMBOL_S_QUIT = 65,                    /* S_QUIT  */
  YYSYMBOL_S_EXPORT = 66,                  /* S_EXPORT  */
  YYSYMBOL_S_REBUILD_GRAPH = 67,           /* S_REBUILD_GRAPH  */
  YYSYMBOL_S_PRINT_GRAPH = 68,             /* S_PRINT_GRAPH  */
  YYSYMBOL_S_SYNCREFS = 69,                /* S_SYNCREFS  */
  YYSYMBOL_S_REDO = 70,                    /* S_REDO  */
  YYSYMBOL_S_UNDO = 71,                    /* S_UNDO  */
  YYSYMBOL_S_IMAP = 72,                    /* S_IMAP  */
  YYSYMBOL_S_EMAP = 73,                    /* S_EMAP  */
  YYSYMBOL_S_CMAP = 74,                    /* S_CMAP  */
  YYSYMBOL_S_NEWSHEET = 75,                /* S_NEWSHEET  */
  YYSYMBOL_S_NEXTSHEET = 76,               /* S_NEXTSHEET  */
  YYSYMBOL_S_PREVSHEET = 77,               /* S_PREVSHEET  */
  YYSYMBOL_S_DELSHEET = 78,                /* S_DELSHEET  */
  YYSYMBOL_S_MOVETOSHEET = 79,             /* S_MOVETOSHEET  */
  YYSYMBOL_S_MOVESHEET = 80,               /* S_MOVESHEET  */
  YYSYMBOL_S_RENAMESHEET = 81,             /* S_RENAMESHEET  */
  YYSYMBOL_S_NMAP = 82,                    /* S_NMAP  */
  YYSYMBOL_S_VMAP = 83,                    /* S_VMAP  */
  YYSYMBOL_S_INOREMAP = 84,                /* S_INOREMAP  */
  YYSYMBOL_S_ENOREMAP = 85,                /* S_ENOREMAP  */
  YYSYMBOL_S_CNOREMAP = 86,                /* S_CNOREMAP  */
  YYSYMBOL_S_NNOREMAP = 87,                /* S_NNOREMAP  */
  YYSYMBOL_S_VNOREMAP = 88,                /* S_VNOREMAP  */
  YYSYMBOL_S_NUNMAP = 89,                  /* S_NUNMAP  */
  YYSYMBOL_S_IUNMAP = 90,                  /* S_IUNMAP  */
  YYSYMBOL_S_EUNMAP = 91,                  /* S_EUNMAP  */
  YYSYMBOL_S_CUNMAP = 92,                  /* S_CUNMAP  */
  YYSYMBOL_S_VUNMAP = 93,                  /* S_VUNMAP  */
  YYSYMBOL_S_COLOR = 94,                   /* S_COLOR  */
  YYSYMBOL_S_CELLCOLOR = 95,               /* S_CELLCOLOR  */
  YYSYMBOL_S_UNFORMAT = 96,                /* S_UNFORMAT  */
  YYSYMBOL_S_REDEFINE_COLOR = 97,          /* S_REDEFINE_COLOR  */
  YYSYMBOL_S_DEFINE_COLOR = 98,            /* S_DEFINE_COLOR  */
  YYSYMBOL_S_SET = 99,                     /* S_SET  */
  YYSYMBOL_S_FCOPY = 100,                  /* S_FCOPY  */
  YYSYMBOL_S_FSUM = 101,                   /* S_FSUM  */
  YYSYMBOL_S_TRIGGER = 102,                /* S_TRIGGER  */
  YYSYMBOL_S_UNTRIGGER = 103,              /* S_UNTRIGGER  */
  YYSYMBOL_S_OFFSCR_SC_COLS = 104,         /* S_OFFSCR_SC_COLS  */
  YYSYMBOL_S_OFFSCR_SC_ROWS = 105,         /* S_OFFSCR_SC_ROWS  */
  YYSYMBOL_S_NB_FROZEN_ROWS = 106,         /* S_NB_FROZEN_ROWS  */
  YYSYMBOL_S_NB_FROZEN_COLS = 107,         /* S_NB_FROZEN_COLS  */
  YYSYMBOL_S_NB_FROZEN_SCREENROWS = 108,   /* S_NB_FROZEN_SCREENROWS  */
  YYSYMBOL_S_NB_FROZEN_SCREENCOLS = 109,   /* S_NB_FROZEN_SCREENCOLS  */
  YYSYMBOL_K_AUTOBACKUP = 110,             /* K_AUTOBACKUP  */
  YYSYMBOL_K_NOAUTOBACKUP = 111,           /* K_NOAUTOBACKUP  */
  YYSYMBOL_K_AUTOCALC = 112,               /* K_AUTOCALC  */
  YYSYMBOL_K_NOAUTOCALC = 113,             /* K_NOAUTOCALC  */
  YYSYMBOL_K_DEBUG = 114,                  /* K_DEBUG  */
  YYSYMBOL_K_NODEBUG = 115,                /* K_NODEBUG  */
  YYSYMBOL_K_TRG = 116,                    /* K_TRG  */
  YYSYMBOL_K_NOTRG = 117,                  /* K_NOTRG  */
  YYSYMBOL_K_EXTERNAL_FUNCTIONS = 118,     /* K_EXTERNAL_FUNCTIONS  */
  YYSYMBOL_K_NOEXTERNAL_FUNCTIONS = 119,   /* K_NOEXTERNAL_FUNCTIONS  */
  YYSYMBOL_K_EXEC_LUA = 120,               /* K_EXEC_LUA  */
  YYSYMBOL_K_NOEXEC_LUA = 121,             /* K_NOEXEC_LUA  */
  YYSYMBOL_K_HALF_PAGE_SCROLL = 122,       /* K_HALF_PAGE_SCROLL  */
  YYSYMBOL_K_NOHALF_PAGE_SCROLL = 123,     /* K_NOHALF_PAGE_SCROLL  */
  YYSYMBOL_K_HISTORY_FILE = 124,           /* K_HISTORY_FILE  */
  YYSYMBOL_K_NOHISTORY_FILE = 125,         /* K_NOHISTORY_FILE  */
  YYSYMBOL_K_NOCURSES = 126,               /* K_NOCURSES  */
  YYSYMBOL_K_CURSES = 127,                 /* K_CURSES  */
  YYSYMBOL_K_NUMERIC = 128,                /* K_NUMERIC  */
  YYSYMBOL_K_NONUMERIC = 129,              /* K_NONUMERIC  */
  YYSYMBOL_K_NUMERIC_DECIMAL = 130,        /* K_NUMERIC_DECIMAL  */
  YYSYMBOL_K_NONUMERIC_DECIMAL = 131,      /* K_NONUMERIC_DECIMAL  */
  YYSYMBOL_K_NUMERIC_ZERO = 132,           /* K_NUMERIC_ZERO  */
  YYSYMBOL_K_NONUMERIC_ZERO = 133,         /* K_NONUMERIC_ZERO  */
  YYSYMBOL_K_OVERLAP = 134,                /* K_OVERLAP  */
  YYSYMBOL_K_NOOVERLAP = 135,              /* K_NOOVERLAP  */
  YYSYMBOL_K_INPUT_BAR_BOTTOM = 136,       /* K_INPUT_BAR_BOTTOM  */
  YYSYMBOL_K_IGNORE_HIDDEN = 137,          /* K_IGNORE_HIDDEN  */
  YYSYMBOL_K_NOIGNORE_HIDDEN = 138,        /* K_NOIGNORE_HIDDEN  */
  YYSYMBOL_K_INPUT_EDIT_MODE = 139,        /* K_INPUT_EDIT_MODE  */
  YYSYMBOL_K_UNDERLINE_GRID = 140,         /* K_UNDERLINE_GRID  */
  YYSYMBOL_K_TRUNCATE = 141,               /* K_TRUNCATE  */
  YYSYMBOL_K_NOTRUNCATE = 142,             /* K_NOTRUNCATE  */
  YYSYMBOL_K_AUTOWRAP = 143,               /* K_AUTOWRAP  */
  YYSYMBOL_K_NOAUTOWRAP = 144,             /* K_NOAUTOWRAP  */
  YYSYMBOL_K_QUIET = 145,                  /* K_QUIET  */
  YYSYMBOL_K_NOQUIET = 146,                /* K_NOQUIET  */
  YYSYMBOL_K_QUIT_AFTERLOAD = 147,         /* K_QUIT_AFTERLOAD  */
  YYSYMBOL_K_NOQUIT_AFTERLOAD = 148,       /* K_NOQUIT_AFTERLOAD  */
  YYSYMBOL_K_XLSX_READFORMULAS = 149,      /* K_XLSX_READFORMULAS  */
  YYSYMBOL_K_NOXLSX_READFORMULAS = 150,    /* K_NOXLSX_READFORMULAS  */
  YYSYMBOL_K_DEFAULT_COPY_TO_CLIPBOARD_CMD = 151, /* K_DEFAULT_COPY_TO_CLIPBOARD_CMD  */
  YYSYMBOL_K_DEFAULT_PASTE_FROM_CLIPBOARD_CMD = 152, /* K_DEFAULT_PASTE_FROM_CLIPBOARD_CMD  */
  YYSYMBOL_K_COPY_TO_CLIPBOARD_DELIMITED_TAB = 153, /* K_COPY_TO_CLIPBOARD_DELIMITED_TAB  */
  YYSYMBOL_K_NOCOPY_TO_CLIPBOARD_DELIMITED_TAB = 154, /* K_NOCOPY_TO_CLIPBOARD_DELIMITED_TAB  */
  YYSYMBOL_K_COPY_TO_CLIPBOARD_WYSIWYG = 155, /* K_COPY_TO_CLIPBOARD_WYSIWYG  */
  YYSYMBOL_K_NOCOPY_TO_CLIPBOARD_WYSIWYG = 156, /* K_NOCOPY_TO_CLIPBOARD_WYSIWYG  */
  YYSYMBOL_K_DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD = 157, /* K_DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD  */
  YYSYMBOL_K_IMPORT_DELIMITED_TO_TEXT = 158, /* K_IMPORT_DELIMITED_TO_TEXT  */
  YYSYMBOL_K_IGNORECASE = 159,             /* K_IGNORECASE  */
  YYSYMBOL_K_NOIGNORECASE = 160,           /* K_NOIGNORECASE  */
  YYSYMBOL_K_TM_GMTOFF = 161,              /* K_TM_GMTOFF  */
  YYSYMBOL_K_COMMAND_TIMEOUT = 162,        /* K_COMMAND_TIMEOUT  */
  YYSYMBOL_K_MAPPING_TIMEOUT = 163,        /* K_MAPPING_TIMEOUT  */
  YYSYMBOL_K_NEWLINE_ACTION = 164,         /* K_NEWLINE_ACTION  */
  YYSYMBOL_K_SHOW_CURSOR = 165,            /* K_SHOW_CURSOR  */
  YYSYMBOL_K_NOSHOW_CURSOR = 166,          /* K_NOSHOW_CURSOR  */
  YYSYMBOL_K_ERROR = 167,                  /* K_ERROR  */
  YYSYMBOL_K_INVALID = 168,                /* K_INVALID  */
  YYSYMBOL_K_FIXED = 169,                  /* K_FIXED  */
  YYSYMBOL_K_SUM = 170,                    /* K_SUM  */
  YYSYMBOL_K_PROD = 171,                   /* K_PROD  */
  YYSYMBOL_K_AVG = 172,                    /* K_AVG  */
  YYSYMBOL_K_STDDEV = 173,                 /* K_STDDEV  */
  YYSYMBOL_K_COUNT = 174,                  /* K_COUNT  */
  YYSYMBOL_K_ROWS = 175,                   /* K_ROWS  */
  YYSYMBOL_K_COLS = 176,                   /* K_COLS  */
  YYSYMBOL_K_ABS = 177,                    /* K_ABS  */
  YYSYMBOL_K_FROW = 178,                   /* K_FROW  */
  YYSYMBOL_K_FCOL = 179,                   /* K_FCOL  */
  YYSYMBOL_K_ACOS = 180,                   /* K_ACOS  */
  YYSYMBOL_K_ASIN = 181,                   /* K_ASIN  */
  YYSYMBOL_K_ATAN = 182,                   /* K_ATAN  */
  YYSYMBOL_K_ATAN2 = 183,                  /* K_ATAN2  */
  YYSYMBOL_K_CEIL = 184,                   /* K_CEIL  */
  YYSYMBOL_K_COS = 185,                    /* K_COS  */
  YYSYMBOL_K_EXP = 186,                    /* K_EXP  */
  YYSYMBOL_K_FABS = 187,                   /* K_FABS  */
  YYSYMBOL_K_FLOOR = 188,                  /* K_FLOOR  */
  YYSYMBOL_K_HYPOT = 189,                  /* K_HYPOT  */
  YYSYMBOL_K_LN = 190,                     /* K_LN  */
  YYSYMBOL_K_LOG = 191,                    /* K_LOG  */
  YYSYMBOL_K_PI = 192,                     /* K_PI  */
  YYSYMBOL_K_POW = 193,                    /* K_POW  */
  YYSYMBOL_K_SIN = 194,                    /* K_SIN  */
  YYSYMBOL_K_SQRT = 195,                   /* K_SQRT  */
  YYSYMBOL_K_TAN = 196,                    /* K_TAN  */
  YYSYMBOL_K_DTR = 197,                    /* K_DTR  */
  YYSYMBOL_K_RTD = 198,                    /* K_RTD  */
  YYSYMBOL_K_MAX = 199,                    /* K_MAX  */
  YYSYMBOL_K_MIN = 200,                    /* K_MIN  */
  YYSYMBOL_K_RND = 201,                    /* K_RND  */
  YYSYMBOL_K_ROUND = 202,                  /* K_ROUND  */
  YYSYMBOL_K_IF = 203,                     /* K_IF  */
  YYSYMBOL_K_PV = 204,                     /* K_PV  */
  YYSYMBOL_K_FV = 205,                     /* K_FV  */
  YYSYMBOL_K_PMT = 206,                    /* K_PMT  */
  YYSYMBOL_K_HOUR = 207,                   /* K_HOUR  */
  YYSYMBOL_K_MINUTE = 208,                 /* K_MINUTE  */
  YYSYMBOL_K_SECOND = 209,                 /* K_SECOND  */
  YYSYMBOL_K_MONTH = 210,                  /* K_MONTH  */
  YYSYMBOL_K_DAY = 211,                    /* K_DAY  */
  YYSYMBOL_K_YEAR = 212,                   /* K_YEAR  */
  YYSYMBOL_K_NOW = 213,                    /* K_NOW  */
  YYSYMBOL_K_DATE = 214,                   /* K_DATE  */
  YYSYMBOL_K_DTS = 215,                    /* K_DTS  */
  YYSYMBOL_K_TTS = 216,                    /* K_TTS  */
  YYSYMBOL_K_FMT = 217,                    /* K_FMT  */
  YYSYMBOL_K_REPLACE = 218,                /* K_REPLACE  */
  YYSYMBOL_K_SUBSTR = 219,                 /* K_SUBSTR  */
  YYSYMBOL_K_UPPER = 220,                  /* K_UPPER  */
  YYSYMBOL_K_LOWER = 221,                  /* K_LOWER  */
  YYSYMBOL_K_CAPITAL = 222,                /* K_CAPITAL  */
  YYSYMBOL_K_STON = 223,                   /* K_STON  */
  YYSYMBOL_K_SLEN = 224,                   /* K_SLEN  */
  YYSYMBOL_K_EQS = 225,                    /* K_EQS  */
  YYSYMBOL_K_EXT = 226,                    /* K_EXT  */
  YYSYMBOL_K_EVALUATE = 227,               /* K_EVALUATE  */
  YYSYMBOL_K_SEVALUATE = 228,              /* K_SEVALUATE  */
  YYSYMBOL_K_LUA = 229,                    /* K_LUA  */
  YYSYMBOL_K_NVAL = 230,                   /* K_NVAL  */
  YYSYMBOL_K_SVAL = 231,                   /* K_SVAL  */
  YYSYMBOL_K_LOOKUP = 232,                 /* K_LOOKUP  */
  YYSYMBOL_K_HLOOKUP = 233,                /* K_HLOOKUP  */
  YYSYMBOL_K_VLOOKUP = 234,                /* K_VLOOKUP  */
  YYSYMBOL_K_INDEX = 235,                  /* K_INDEX  */
  YYSYMBOL_K_STINDEX = 236,                /* K_STINDEX  */
  YYSYMBOL_K_GETENT = 237,                 /* K_GETENT  */
  YYSYMBOL_K_TBLSTYLE = 238,               /* K_TBLSTYLE  */
  YYSYMBOL_K_TBL = 239,                    /* K_TBL  */
  YYSYMBOL_K_LATEX = 240,                  /* K_LATEX  */
  YYSYMBOL_K_SLATEX = 241,                 /* K_SLATEX  */
  YYSYMBOL_K_TEX = 242,                    /* K_TEX  */
  YYSYMBOL_K_FRAME = 243,                  /* K_FRAME  */
  YYSYMBOL_K_RNDTOEVEN = 244,              /* K_RNDTOEVEN  */
  YYSYMBOL_K_FILENAME = 245,               /* K_FILENAME  */
  YYSYMBOL_K_MYROW = 246,                  /* K_MYROW  */
  YYSYMBOL_K_MYCOL = 247,                  /* K_MYCOL  */
  YYSYMBOL_K_COLORIZE = 248,               /* K_COLORIZE  */
  YYSYMBOL_K_LASTROW = 249,                /* K_LASTROW  */
  YYSYMBOL_K_LASTCOL = 250,                /* K_LASTCOL  */
  YYSYMBOL_K_COLTOA = 251,                 /* K_COLTOA  */
  YYSYMBOL_K_CRACTION = 252,               /* K_CRACTION  */
  YYSYMBOL_K_CRROW = 253,                  /* K_CRROW  */
  YYSYMBOL_K_CRCOL = 254,                  /* K_CRCOL  */
  YYSYMBOL_K_ROWLIMIT = 255,               /* K_ROWLIMIT  */
  YYSYMBOL_K_COLLIMIT = 256,               /* K_COLLIMIT  */
  YYSYMBOL_K_PAGESIZE = 257,               /* K_PAGESIZE  */
  YYSYMBOL_K_ERR = 258,                    /* K_ERR  */
  YYSYMBOL_K_REF = 259,                    /* K_REF  */
  YYSYMBOL_K_LOCALE = 260,                 /* K_LOCALE  */
  YYSYMBOL_K_SET8BIT = 261,                /* K_SET8BIT  */
  YYSYMBOL_K_ASCII = 262,                  /* K_ASCII  */
  YYSYMBOL_K_CHR = 263,                    /* K_CHR  */
  YYSYMBOL_K_FACT = 264,                   /* K_FACT  */
  YYSYMBOL_265_ = 265,                     /* ';'  */
  YYSYMBOL_266_ = 266,                     /* '?'  */
  YYSYMBOL_267_ = 267,                     /* ':'  */
  YYSYMBOL_268_ = 268,                     /* '|'  */
  YYSYMBOL_269_ = 269,                     /* '&'  */
  YYSYMBOL_270_ = 270,                     /* '<'  */
  YYSYMBOL_271_ = 271,                     /* '='  */
  YYSYMBOL_272_ = 272,                     /* '>'  */
  YYSYMBOL_273_ = 273,                     /* '!'  */
  YYSYMBOL_274_ = 274,                     /* '+'  */
  YYSYMBOL_275_ = 275,                     /* '-'  */
  YYSYMBOL_276_ = 276,                     /* '#'  */
  YYSYMBOL_277_ = 277,                     /* '*'  */
  YYSYMBOL_278_ = 278,                     /* '/'  */
  YYSYMBOL_279_ = 279,                     /* '%'  */
  YYSYMBOL_280_ = 280,                     /* '^'  */
  YYSYMBOL_281_ = 281,                     /* '{'  */
  YYSYMBOL_282_ = 282,                     /* '}'  */
  YYSYMBOL_283_ = 283,                     /* '@'  */
  YYSYMBOL_284_ = 284,                     /* '('  */
  YYSYMBOL_285_ = 285,                     /* ')'  */
  YYSYMBOL_286_ = 286,                     /* ','  */
  YYSYMBOL_287_ = 287,                     /* '.'  */
  YYSYMBOL_288_ = 288,                     /* '~'  */
  YYSYMBOL_289_ = 289,                     /* '$'  */
  YYSYMBOL_YYACCEPT = 290,                 /* $accept  */
  YYSYMBOL_command = 291,                  /* command  */
  YYSYMBOL_term = 292,                     /* term  */
  YYSYMBOL_e = 293,                        /* e  */
  YYSYMBOL_expr_list = 294,                /* expr_list  */
  YYSYMBOL_range = 295,                    /* range  */
  YYSYMBOL_var = 296,                      /* var  */
  YYSYMBOL_var_or_range = 297,             /* var_or_range  */
  YYSYMBOL_num = 298,                      /* num  */
  YYSYMBOL_strarg = 299,                   /* strarg  */
  YYSYMBOL_setlist = 300,                  /* setlist  */
  YYSYMBOL_setitem = 301                   /* setitem  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  231
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   3395

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  290
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  12
/* YYNRULES -- Number of rules.  */
#define YYNRULES  379
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  979

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   519


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int16 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   273,     2,   276,   289,   279,   269,     2,
     284,   285,   277,   274,   286,   275,   287,   278,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   267,   265,
     270,   271,   272,   266,   283,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,   280,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   281,   268,   282,   288,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   441,   441,   447,   488,   494,   499,   504,   508,   513,
     517,   524,   530,   536,   537,   538,   539,   542,   547,   554,
     556,   558,   560,   562,   564,   566,   580,   595,   600,   613,
     619,   627,   636,   646,   652,   654,   658,   662,   666,   670,
     674,   678,   682,   687,   691,   696,   710,   711,   713,   715,
     719,   723,   724,   729,   735,   737,   742,   747,   751,   756,
     761,   767,   772,   777,   782,   789,   790,   791,   792,   797,
     802,   840,   862,   879,   893,   908,   914,   921,   930,   936,
     941,   946,   951,   956,   962,   967,   972,   977,   983,   988,
     992,   996,  1001,  1005,  1012,  1032,  1043,  1048,  1053,  1058,
    1063,  1068,  1073,  1079,  1083,  1093,  1101,  1109,  1113,  1117,
    1118,  1119,  1121,  1129,  1145,  1146,  1148,  1150,  1156,  1180,
    1187,  1191,  1196,  1197,  1199,  1208,  1218,  1223,  1229,  1237,
    1239,  1242,  1244,  1253,  1262,  1268,  1272,  1273,  1286,  1317,
    1319,  1322,  1326,  1328,  1330,  1332,  1334,  1336,  1338,  1340,
    1342,  1344,  1346,  1348,  1350,  1352,  1354,  1356,  1358,  1360,
    1361,  1362,  1363,  1364,  1365,  1366,  1368,  1369,  1370,  1371,
    1372,  1373,  1375,  1376,  1377,  1379,  1380,  1381,  1382,  1383,
    1384,  1385,  1387,  1389,  1391,  1393,  1395,  1396,  1397,  1398,
    1399,  1400,  1401,  1402,  1404,  1408,  1410,  1411,  1412,  1414,
    1415,  1417,  1419,  1420,  1421,  1422,  1424,  1426,  1428,  1430,
    1432,  1434,  1436,  1438,  1440,  1442,  1444,  1446,  1448,  1457,
    1458,  1460,  1461,  1462,  1463,  1465,  1467,  1470,  1471,  1472,
    1474,  1475,  1477,  1478,  1479,  1480,  1481,  1482,  1483,  1485,
    1486,  1487,  1488,  1489,  1490,  1491,  1492,  1493,  1497,  1498,
    1499,  1500,  1501,  1502,  1503,  1504,  1505,  1506,  1507,  1508,
    1509,  1510,  1511,  1512,  1514,  1515,  1516,  1519,  1520,  1523,
    1527,  1532,  1539,  1546,  1553,  1560,  1570,  1581,  1591,  1601,
    1611,  1617,  1618,  1621,  1622,  1623,  1624,  1627,  1628,  1640,
    1641,  1646,  1648,  1650,  1651,  1652,  1653,  1656,  1660,  1663,
    1667,  1669,  1672,  1674,  1676,  1677,  1679,  1681,  1682,  1684,
    1685,  1689,  1690,  1691,  1693,  1694,  1695,  1697,  1698,  1699,
    1701,  1702,  1703,  1706,  1707,  1708,  1711,  1712,  1713,  1716,
    1718,  1719,  1722,  1724,  1727,  1728,  1729,  1730,  1733,  1734,
    1735,  1738,  1739,  1740,  1744,  1745,  1746,  1748,  1749,  1750,
    1752,  1753,  1754,  1757,  1758,  1759,  1762,  1763,  1764,  1770,
    1777,  1785,  1787,  1791,  1793,  1796,  1797,  1798,  1801,  1802,
    1810,  1812,  1813,  1819,  1820,  1826,  1827,  1833,  1835,  1836
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "STRING", "COL",
  "NUMBER", "FNUMBER", "RANGE", "VAR", "WORD", "MAPWORD", "PLUGIN",
  "S_SHOW", "S_HIDE", "S_SHOWROW", "S_HIDEROW", "S_SHOWCOL", "S_HIDECOL",
  "S_FREEZE", "S_UNFREEZE", "S_MARK", "S_AUTOFIT", "S_PAD", "S_DELETECOL",
  "S_DATEFMT", "S_SUBTOTAL", "S_RSUBTOTAL", "S_FORMAT", "S_FMT", "S_LET",
  "S_LABEL", "S_LEFTSTRING", "S_RIGHTSTRING", "S_LEFTJUSTIFY",
  "S_RIGHTJUSTIFY", "S_CENTER", "S_SORT", "S_FILTERON", "S_GOTO",
  "S_GOTOB", "S_CCOPY", "S_CPASTE", "S_PLOT", "S_LOCK", "S_UNLOCK",
  "S_DEFINE", "S_UNDEFINE", "S_DETAIL", "S_EVAL", "S_SEVAL", "S_ERROR",
  "S_FILL", "S_STRTONUM", "S_DELETEROW", "S_VALUEIZEALL", "S_SHIFT",
  "S_GETNUM", "S_YANKAREA", "S_PASTEYANKED", "S_GETSTRING", "S_GETEXP",
  "S_GETFMT", "S_GETFORMAT", "S_RECALC", "S_EXECUTE", "S_QUIT", "S_EXPORT",
  "S_REBUILD_GRAPH", "S_PRINT_GRAPH", "S_SYNCREFS", "S_REDO", "S_UNDO",
  "S_IMAP", "S_EMAP", "S_CMAP", "S_NEWSHEET", "S_NEXTSHEET", "S_PREVSHEET",
  "S_DELSHEET", "S_MOVETOSHEET", "S_MOVESHEET", "S_RENAMESHEET", "S_NMAP",
  "S_VMAP", "S_INOREMAP", "S_ENOREMAP", "S_CNOREMAP", "S_NNOREMAP",
  "S_VNOREMAP", "S_NUNMAP", "S_IUNMAP", "S_EUNMAP", "S_CUNMAP", "S_VUNMAP",
  "S_COLOR", "S_CELLCOLOR", "S_UNFORMAT", "S_REDEFINE_COLOR",
  "S_DEFINE_COLOR", "S_SET", "S_FCOPY", "S_FSUM", "S_TRIGGER",
  "S_UNTRIGGER", "S_OFFSCR_SC_COLS", "S_OFFSCR_SC_ROWS",
  "S_NB_FROZEN_ROWS", "S_NB_FROZEN_COLS", "S_NB_FROZEN_SCREENROWS",
  "S_NB_FROZEN_SCREENCOLS", "K_AUTOBACKUP", "K_NOAUTOBACKUP", "K_AUTOCALC",
  "K_NOAUTOCALC", "K_DEBUG", "K_NODEBUG", "K_TRG", "K_NOTRG",
  "K_EXTERNAL_FUNCTIONS", "K_NOEXTERNAL_FUNCTIONS", "K_EXEC_LUA",
  "K_NOEXEC_LUA", "K_HALF_PAGE_SCROLL", "K_NOHALF_PAGE_SCROLL",
  "K_HISTORY_FILE", "K_NOHISTORY_FILE", "K_NOCURSES", "K_CURSES",
  "K_NUMERIC", "K_NONUMERIC", "K_NUMERIC_DECIMAL", "K_NONUMERIC_DECIMAL",
  "K_NUMERIC_ZERO", "K_NONUMERIC_ZERO", "K_OVERLAP", "K_NOOVERLAP",
  "K_INPUT_BAR_BOTTOM", "K_IGNORE_HIDDEN", "K_NOIGNORE_HIDDEN",
  "K_INPUT_EDIT_MODE", "K_UNDERLINE_GRID", "K_TRUNCATE", "K_NOTRUNCATE",
  "K_AUTOWRAP", "K_NOAUTOWRAP", "K_QUIET", "K_NOQUIET", "K_QUIT_AFTERLOAD",
  "K_NOQUIT_AFTERLOAD", "K_XLSX_READFORMULAS", "K_NOXLSX_READFORMULAS",
  "K_DEFAULT_COPY_TO_CLIPBOARD_CMD", "K_DEFAULT_PASTE_FROM_CLIPBOARD_CMD",
  "K_COPY_TO_CLIPBOARD_DELIMITED_TAB",
  "K_NOCOPY_TO_CLIPBOARD_DELIMITED_TAB", "K_COPY_TO_CLIPBOARD_WYSIWYG",
  "K_NOCOPY_TO_CLIPBOARD_WYSIWYG", "K_DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD",
  "K_IMPORT_DELIMITED_TO_TEXT", "K_IGNORECASE", "K_NOIGNORECASE",
  "K_TM_GMTOFF", "K_COMMAND_TIMEOUT", "K_MAPPING_TIMEOUT",
  "K_NEWLINE_ACTION", "K_SHOW_CURSOR", "K_NOSHOW_CURSOR", "K_ERROR",
  "K_INVALID", "K_FIXED", "K_SUM", "K_PROD", "K_AVG", "K_STDDEV",
  "K_COUNT", "K_ROWS", "K_COLS", "K_ABS", "K_FROW", "K_FCOL", "K_ACOS",
  "K_ASIN", "K_ATAN", "K_ATAN2", "K_CEIL", "K_COS", "K_EXP", "K_FABS",
  "K_FLOOR", "K_HYPOT", "K_LN", "K_LOG", "K_PI", "K_POW", "K_SIN",
  "K_SQRT", "K_TAN", "K_DTR", "K_RTD", "K_MAX", "K_MIN", "K_RND",
  "K_ROUND", "K_IF", "K_PV", "K_FV", "K_PMT", "K_HOUR", "K_MINUTE",
  "K_SECOND", "K_MONTH", "K_DAY", "K_YEAR", "K_NOW", "K_DATE", "K_DTS",
  "K_TTS", "K_FMT", "K_REPLACE", "K_SUBSTR", "K_UPPER", "K_LOWER",
  "K_CAPITAL", "K_STON", "K_SLEN", "K_EQS", "K_EXT", "K_EVALUATE",
  "K_SEVALUATE", "K_LUA", "K_NVAL", "K_SVAL", "K_LOOKUP", "K_HLOOKUP",
  "K_VLOOKUP", "K_INDEX", "K_STINDEX", "K_GETENT", "K_TBLSTYLE", "K_TBL",
  "K_LATEX", "K_SLATEX", "K_TEX", "K_FRAME", "K_RNDTOEVEN", "K_FILENAME",
  "K_MYROW", "K_MYCOL", "K_COLORIZE", "K_LASTROW", "K_LASTCOL", "K_COLTOA",
  "K_CRACTION", "K_CRROW", "K_CRCOL", "K_ROWLIMIT", "K_COLLIMIT",
  "K_PAGESIZE", "K_ERR", "K_REF", "K_LOCALE", "K_SET8BIT", "K_ASCII",
  "K_CHR", "K_FACT", "';'", "'?'", "':'", "'|'", "'&'", "'<'", "'='",
  "'>'", "'!'", "'+'", "'-'", "'#'", "'*'", "'/'", "'%'", "'^'", "'{'",
  "'}'", "'@'", "'('", "')'", "','", "'.'", "'~'", "'$'", "$accept",
  "command", "term", "e", "expr_list", "range", "var", "var_or_range",
  "num", "strarg", "setlist", "setitem", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-264)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-283)

#define yytable_value_is_error(Yyn) \
  ((Yyn) == YYTABLE_NINF)

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    1144,  -264,    15,    23,     6,    11,    17,    29,    26,    40,
      44,    51,    18,    52,     2,   251,   251,    42,   251,   251,
     251,   251,   251,   251,   251,   251,   251,   251,    65,   107,
     251,  -264,    54,   251,   251,   126,   251,   313,   144,   144,
      55,   389,   251,    57,  -264,   251,   290,  -222,  -218,   251,
     251,   251,    61,  -264,    63,  -264,    64,  -264,  -264,  -264,
    -264,  -264,    71,    72,    74,    75,  -264,  -264,    76,    77,
      80,    86,    87,    88,    91,    92,    98,   108,   111,   112,
     113,   114,   115,   116,   123,    89,   251,   124,   125,  -264,
     126,  -264,   251,   251,   127,   134,   140,   141,   146,   151,
     131,  -264,  -264,  -264,  -264,  -129,  -123,   -99,   -97,   -96,
     -88,   -85,   -84,   133,   -81,   303,  -264,  -264,     7,  -264,
    -264,   184,   -46,   188,  -264,   -66,   206,   207,   -66,   208,
     205,   210,   213,   -54,   -53,   -48,   -45,  -264,  -264,  -264,
     224,  -264,  -264,  -264,  -264,    82,    82,   225,   227,   251,
    -264,  -264,   230,   231,  -264,  -264,   251,  -264,  -264,  -264,
    -264,   251,  -264,  -264,  -264,   -52,  -264,  -264,  -264,   558,
     558,   144,   810,   271,   558,  -264,  3115,  -264,  3115,  -264,
      82,    82,  -264,  -264,   235,   241,  -264,   250,   262,  -264,
    -264,  -264,  -264,  -264,   270,   275,   277,   278,  -264,  -264,
    -264,  -264,  -264,   279,   281,   284,   289,   315,   335,   339,
    -264,  -264,  -264,  -264,  -264,  -264,  -264,   340,  -264,   342,
     345,   796,  -264,   352,  -264,  -264,  -264,  -264,  -264,  -264,
    -264,  -264,   353,   355,   341,   359,   360,   363,   361,   364,
     251,   251,   369,    19,  -264,  -264,   372,   102,    90,    10,
     313,  -264,   384,   385,   394,  -264,  -264,   144,   144,   144,
     144,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,
    -264,   -66,   399,  -264,  -264,  -237,   558,   128,   129,   137,
     142,   147,   154,   162,   163,   164,   243,   244,   247,   249,
     252,   253,   254,   255,   263,   272,   273,   274,   283,  -264,
     285,   286,   288,   296,   297,   298,   299,   301,   304,   305,
     306,   307,   309,   311,   314,   317,   325,   333,   334,   351,
    -264,   354,   356,   357,   362,   365,   367,   368,   373,   377,
     378,   381,   382,   383,   390,   391,   392,   393,   400,   402,
     403,   404,   405,   406,   407,  -264,  -264,   408,  -264,  -264,
     409,  -264,  -264,   412,   414,   419,   420,  1155,  1764,  -264,
     144,   144,   144,   144,    31,   144,    78,   144,   144,   144,
     144,   144,   144,   144,    82,  -264,  -264,   148,   226,   237,
    -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,
    -264,  -264,   520,   530,   294,  -264,   316,  -264,   326,  -264,
     328,  -264,   397,  -264,   398,  -264,   429,  -264,   435,  -264,
     436,  -264,   437,  -264,   438,  -264,   439,  -264,   440,  -264,
     441,   442,  -264,   443,   444,   445,  -264,   447,  -264,   449,
    -264,   455,  -264,   457,  -264,   458,   459,   460,  -264,   461,
    -264,   462,   463,   464,  -264,   465,   469,   470,   473,   474,
    -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,
    -264,   251,  -264,  -264,   564,  -264,   135,   144,  -264,   621,
    -264,   632,   635,   639,  3115,  3115,  3115,  3115,   366,  -264,
     251,   251,   251,   251,   251,   251,   251,   144,   144,   144,
     144,   144,   144,   144,   144,   144,   144,   144,   144,   144,
     144,   144,   144,   144,   144,   144,   144,   144,   117,   117,
     144,   144,   144,   144,   144,   144,   144,   144,   144,   144,
     144,   144,   144,   144,   144,   144,   144,   144,   144,   144,
     144,   144,   144,   144,   144,   144,   144,   144,   144,   144,
     117,   117,   117,   117,   117,   144,   251,   144,   144,   144,
     144,   144,   545,  -264,  3115,  3099,   165,  -172,   144,   144,
     336,   336,   144,   336,  -237,  -237,  -237,   130,   130,   130,
    -264,  -264,   138,   475,   651,   668,   742,   744,   745,   746,
     747,   749,   750,   752,   759,   761,   762,   763,   764,   765,
     772,   773,   774,   775,   776,   779,   780,   784,   786,   126,
     126,   788,   790,   126,   792,   794,    82,    82,    82,     9,
     797,  -264,  -264,     4,   856,  -264,  -264,  -264,  -264,   800,
     515,   521,   522,   524,   525,   527,   528,   535,   529,   536,
     537,   538,  1782,  1800,  1818,  1836,  1854,  1872,   989,  1890,
    1908,  1926,  1944,  1962,  1008,  1980,  1998,  1027,  2016,  2034,
    2052,  2070,  2088,  1156,   539,  -263,   542,  1175,   549,   551,
    2106,  1194,  1213,  1232,  1251,  1270,  2124,  2142,  2160,  2178,
    2196,  2214,   -72,  1289,  1308,  1327,  1346,  1365,  2232,  2250,
    2268,  2286,  2304,  1384,  1403,  2322,  2340,  1422,  1441,  1460,
    1479,   552,   -66,  1498,   554,  1517,   557,  1536,   559,  1555,
     562,  2358,   565,  2376,  2394,  2412,  2430,  2448,   558,   144,
     336,   336,   336,   416,   251,   804,  -264,  -264,  -264,  -264,
    -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,
    -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,
    -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,
    -264,  -264,  -264,    12,   806,   144,  -264,   144,  -264,   144,
    -264,   144,  -264,   144,  -264,   144,  -264,  -264,  -264,  -264,
    -264,  -264,  -264,  -264,  -264,   144,  -264,  -264,  -264,  -264,
    -264,   144,  -264,  -264,   144,  -264,  -264,  -264,  -264,  -264,
     144,   144,  -264,   144,   144,  -264,  -264,   144,   144,   144,
     144,   144,  -264,  -264,  -264,  -264,  -264,  -264,  -264,   144,
     144,   144,   144,   144,   144,  -264,  -264,  -264,  -264,  -264,
     144,   144,  -264,  -264,   144,   144,   144,   251,   144,   251,
     144,   251,   144,   251,   144,   251,   144,  -264,   144,  -264,
    -264,  -264,  -264,  -264,  -264,    -8,    20,   809,  -264,   821,
    -264,  -264,   832,    24,  2466,  2484,  2502,  2520,  2538,  2556,
    2574,  2592,  2610,  3115,  -234,  2628,  -232,  2646,  2664,  1574,
    1593,  1612,  1631,  2682,  1650,  1669,  2700,  1688,  1707,  2718,
    2736,  2754,  2772,  2790,   569,  2808,   570,  1726,   571,  1745,
     573,   -29,   574,   834,  2826,  -264,   839,    27,  -264,  -264,
    -264,   850,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,
    -264,  -264,   144,  -264,  -264,  -264,  -264,   144,   144,   144,
     144,  -264,   144,   144,  -264,   144,   144,  -264,  -264,  -264,
    -264,  -264,  -264,  -264,   144,   144,   144,   144,  -264,  -264,
     144,  -264,  -264,   144,  -264,  -264,  -264,   855,  -264,  3115,
    2844,  2862,  2880,  2898,  2916,  2934,  2952,  2970,  2988,  3006,
    3024,  3042,  3060,  3078,  -264,  -264,  -264,  -264,  -264,  -264,
    -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264,  -264
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       0,   137,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    67,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    27,     0,     0,     0,     0,     0,
       0,     0,     0,   126,     0,   120,     0,   121,   122,   123,
     125,   124,     0,     0,     0,     0,    73,    74,    72,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   106,     0,     0,   289,
     109,   111,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    15,    16,    13,    14,    22,    21,    20,    19,    38,
      36,    42,    40,     0,    47,     0,    17,    12,     0,   270,
     280,     0,     0,     0,   281,   282,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     7,     8,     9,
       0,    54,    58,   283,   284,     0,     0,     0,     0,    56,
      57,    62,     0,     0,    61,    65,     0,    68,    69,   287,
     288,     0,   116,    94,   235,   232,   233,   244,   246,     0,
       0,     0,     0,     0,     0,   255,   117,   138,   134,   135,
       0,     0,    66,    18,     0,     0,   127,     0,     0,   129,
     130,   131,    51,   118,     0,     0,     0,     0,    70,    71,
      75,    76,    77,     0,     0,     0,     0,     0,     0,     0,
      88,    89,    90,    92,    91,    93,   104,     0,   105,     0,
       0,   113,   110,     0,   103,    97,    98,    99,   100,   101,
     102,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    29,     0,    49,    50,   271,     0,     0,     0,     0,
       0,    11,     0,     0,     0,    53,    10,     3,     0,     0,
       0,    43,   286,   285,    59,    60,    55,    63,    64,   112,
     114,   115,     0,   237,   231,   253,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   234,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     192,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   219,   221,     0,   222,   223,
       0,   243,   245,     0,     0,     0,     0,     0,     0,   236,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    34,    28,     0,     0,     0,
     119,    79,    80,    82,    78,    81,    84,    85,    87,    83,
      86,    95,     0,     0,   309,   311,   312,   314,   315,   317,
     318,   320,   321,   323,   324,   326,   327,   329,   293,   294,
     342,   344,   345,   347,   351,   353,   354,   356,   295,   302,
     297,   330,   332,   299,   301,   304,   305,   307,   308,   334,
     335,   336,   338,   339,   341,     0,     0,   363,   365,   366,
     368,     0,   361,   348,   350,   375,   371,   373,   357,   378,
     379,   290,    96,    24,    26,    23,    25,    37,    35,    41,
      39,    31,    30,    46,     0,   273,     0,     0,   272,     0,
     269,     0,     0,     0,     2,     4,     5,     6,     0,   139,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   230,   257,     0,   262,   261,     0,     0,
     258,   259,     0,   260,   248,   249,   266,   250,   251,   252,
     254,    33,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    32,    48,     0,     0,   274,    44,    45,    52,     0,
     281,     0,   281,     0,   281,     0,   281,     0,   281,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   281,   138,     0,     0,   281,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   138,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     263,   264,   265,     0,     0,     0,   107,   108,   310,   313,
     316,   319,   322,   325,   328,   292,   343,   346,   352,   355,
     291,   296,   331,   298,   300,   303,   306,   333,   337,   340,
     359,   360,   364,   367,   369,   362,   349,   376,   372,   374,
     370,   358,   377,     0,     0,     0,   194,     0,   141,     0,
     143,     0,   145,     0,   147,     0,   149,   157,   158,   159,
     160,   161,   162,   163,   164,     0,   166,   167,   168,   169,
     170,     0,   172,   173,     0,   175,   176,   177,   178,   179,
       0,     0,   151,     0,     0,   154,   180,     0,     0,     0,
       0,     0,   186,   187,   188,   189,   190,   191,   199,     0,
       0,     0,     0,     0,     0,   202,   203,   204,   196,   197,
       0,     0,   227,   228,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   238,     0,   239,
     241,   240,   242,   247,   140,   256,     0,     0,   128,     0,
     133,   276,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   267,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   271,     0,     0,   132,   278,
     277,     0,   275,   142,   144,   146,   148,   150,   165,   171,
     174,   153,     0,   152,   156,   155,   181,     0,     0,     0,
       0,   200,     0,     0,   201,     0,     0,   198,   217,   218,
     224,   225,   209,   208,     0,     0,     0,     0,   206,   205,
       0,   215,   214,     0,   220,   273,   272,     0,   279,   268,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   274,   182,   183,   184,   185,   193,
     195,   226,   229,   211,   210,   213,   212,   207,   216
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -264,  -264,  -167,   -38,    69,    34,   139,   609,    -3,   -77,
    -264,  -264
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,   100,   175,   863,   864,   124,   177,   126,   150,   161,
     221,   451
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     176,   178,   273,   274,   250,   117,   118,   359,   753,   119,
     120,   105,   245,   222,   750,   468,   106,   851,   751,   101,
     102,   107,  -282,   115,   245,   895,   154,   103,   104,   900,
     109,   110,   946,   108,   164,   118,   165,   166,   181,   120,
     370,   371,   372,   373,   111,   112,   130,   131,   113,   127,
     129,   911,   912,   914,   912,   114,   116,   156,   179,   187,
     140,   141,   183,   188,   155,   192,   193,   194,   142,   118,
     143,   144,   119,   120,   195,   196,   182,   197,   198,   199,
     200,   164,   118,   165,   166,   201,   120,   143,   144,   202,
     203,   204,   216,   118,   205,   206,   119,   120,   364,   365,
     366,   207,   367,   368,   369,   370,   371,   372,   373,   479,
     151,   208,   143,   144,   209,   210,   211,   212,   213,   214,
     164,   118,   165,   166,   119,   120,   215,   219,   220,   159,
     118,   231,   225,   275,   120,   358,   240,   118,   232,   226,
     119,   120,   262,   263,   233,   227,   228,   164,   118,   165,
     166,   229,   120,   125,   128,   128,   230,   125,   125,   125,
     125,   125,   125,   125,   125,   128,   128,   125,   234,   128,
     235,   236,   125,   125,   160,   125,   163,   374,   375,   237,
     125,   128,   238,   239,   125,   125,   242,   247,   125,   125,
     125,   248,   249,   360,   361,   270,   362,   363,   364,   365,
     366,   250,   367,   368,   369,   370,   371,   372,   373,   251,
     254,   252,   253,   808,   809,   255,   256,   257,   258,   474,
     475,   476,   477,   259,   125,   125,   260,   261,   264,   160,
     265,   125,   125,   267,   268,   272,   360,   361,   376,   362,
     363,   364,   365,   366,   377,   367,   368,   369,   370,   371,
     372,   373,   125,   378,   125,   118,   939,   940,   119,   120,
     362,   363,   364,   365,   366,   379,   367,   368,   369,   370,
     371,   372,   373,   380,   164,   118,   165,   166,   381,   120,
     382,   383,   384,   121,   385,   122,   464,   386,   125,   167,
     168,   123,   387,   754,   118,   125,   246,   119,   120,   469,
     271,   852,   558,   559,   169,   170,   171,   243,   246,   896,
     119,   120,   121,   901,   172,   173,   947,   118,   388,   174,
     123,   120,   554,   555,   556,   557,   560,   561,   563,   564,
     565,   566,   567,   568,   569,   570,   167,   168,   389,   145,
     146,   147,   390,   391,   148,   455,   121,   392,   122,   562,
     393,   169,   170,   171,   123,   452,   145,   146,   453,   121,
     454,   172,   173,   456,   457,   459,   174,   123,   458,   460,
     121,   571,   122,   463,   467,   167,   168,   465,   123,   125,
     125,   145,   146,   152,   466,   479,   153,   471,   472,   470,
     169,   170,   171,   118,   143,   144,   119,   120,   121,   473,
     172,   173,   167,   168,   478,   174,   123,   121,   613,   122,
     373,   713,   480,   481,   121,   123,   122,   169,   170,   171,
     846,   482,   123,   119,   120,   121,   483,   172,   173,   614,
     572,   484,   174,   123,   363,   364,   365,   366,   485,   367,
     368,   369,   370,   371,   372,   373,   486,   487,   488,   632,
     633,   634,   635,   636,   637,   638,   639,   640,   641,   642,
     643,   644,   645,   646,   647,   648,   649,   650,   651,   652,
     653,   657,   660,   661,   662,   663,   664,   665,   666,   667,
     668,   669,   670,   671,   672,   673,   674,   675,   676,   677,
     678,   679,   680,   681,   682,   683,   684,   685,   686,   687,
     688,   689,   690,   693,   695,   697,   699,   701,   573,   703,
     704,   705,   706,   707,   620,   622,   624,   626,   628,   574,
     710,   711,   740,   741,   712,   575,   744,   489,   490,   167,
     168,   491,   121,   492,   122,   576,   493,   494,   495,   496,
     123,   844,   654,   658,   169,   170,   171,   497,   164,   118,
     165,   166,   121,   120,   357,   173,   498,   499,   500,   174,
     123,   164,   118,   165,   166,   577,   120,   501,   612,   502,
     503,   185,   504,   122,   691,   694,   696,   698,   700,   123,
     505,   506,   507,   508,   121,   509,   122,   578,   510,   511,
     512,   513,   123,   514,   121,   515,   122,   579,   516,   580,
     125,   517,   123,   747,   748,   749,  -283,  -283,  -283,   518,
     367,   368,   369,   370,   371,   372,   373,   519,   520,   125,
     125,   125,   125,   125,   125,   125,   615,   132,   133,   134,
     135,   136,   137,   138,   139,   521,   616,   149,   522,   617,
     523,   524,   157,   158,   618,   162,   525,   655,   655,   526,
     180,   527,   528,   619,   184,   186,   715,   529,   189,   190,
     191,   530,   531,   145,   146,   532,   533,   534,   581,   582,
     121,   845,   122,   716,   535,   536,   537,   538,   123,   692,
     692,   692,   692,   692,   539,   125,   540,   541,   542,   543,
     544,   545,   546,   547,   217,   218,   548,   121,   549,   122,
     583,   223,   224,   550,   551,   847,   584,   585,   586,   587,
     588,   589,   590,   591,   592,   593,   594,   854,   595,   855,
     596,   856,   241,   857,   244,   858,   597,   859,   598,   599,
     600,   601,   602,   603,   604,   605,   606,   860,   160,   160,
     607,   608,   160,   861,   609,   610,   862,   717,   714,   718,
     719,   720,   721,   865,   722,   723,   867,   724,   266,   868,
     869,   870,   871,   872,   725,   269,   726,   727,   728,   729,
     730,   873,   874,   875,   876,   877,   878,   731,   732,   733,
     734,   735,   879,   880,   736,   737,   881,   882,   883,   738,
     885,   739,   887,   742,   889,   743,   891,   745,   893,   746,
     894,   757,   752,   167,   168,   756,   758,   850,   759,   760,
     853,   761,   762,   897,   763,   765,   167,   168,   169,   170,
     764,   766,   767,   768,   898,   791,   121,   792,   172,   173,
     708,   169,   170,   174,   123,   794,   795,   899,   828,   121,
     830,   172,   173,   832,   945,   834,   174,   123,   836,   461,
     462,   838,   125,   125,   932,   948,   934,   936,   938,   941,
     964,   884,   866,   886,     0,   888,     0,   890,     0,   892,
       0,     0,     0,     0,   949,     0,     0,     0,     0,   950,
     951,   952,   953,     0,   954,   955,     0,   956,   957,     0,
       0,     0,     0,     0,     0,     0,   958,   959,   960,   961,
       0,     0,   962,     0,     0,   963,   394,   395,   396,   397,
     398,   399,   400,   401,   402,   403,   404,   405,   406,   407,
     408,   409,   410,   411,   412,   413,   414,   415,   416,   417,
     418,   419,   420,   421,   422,   423,   424,   425,   426,   427,
     428,   429,   430,   431,   432,   433,   434,   435,   436,   437,
     438,   439,   440,   441,   442,   443,   444,   445,   446,   447,
     448,   449,   450,     0,     0,     0,   128,     0,   128,     0,
     128,     0,   128,     0,   128,     0,     0,     0,     0,   276,
     277,   278,   279,   280,   281,   282,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,   300,   301,   302,   303,   304,   305,   306,
     307,   308,   309,   310,   311,   312,   313,   314,   315,   316,
     317,   318,   319,   320,   321,   322,   323,   324,   325,   326,
     327,   328,   329,   330,   331,   332,   333,   334,   335,   336,
     337,   338,   339,   340,   341,   342,   343,   248,     0,     0,
       0,     0,     0,     0,     0,   344,   345,   346,   347,   348,
     349,   350,     0,     0,     0,     0,     0,     0,   351,   352,
     611,   353,   354,   355,   356,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   621,
     623,   625,   627,   629,   630,   631,     0,     0,     0,   360,
     361,     0,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   656,   659,   942,
     943,   360,   361,     0,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,     0,
       0,     0,   755,     0,  -136,     1,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   702,     2,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   360,   361,     0,   362,   363,   364,
     365,   366,     0,   367,   368,   369,   370,   371,   372,   373,
       0,     0,     0,   360,   361,   775,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,     0,   360,   361,   781,   362,   363,   364,   365,   366,
       0,   367,   368,   369,   370,   371,   372,   373,     0,     0,
       0,     0,     0,   784,     0,     0,     0,     0,     0,     0,
       0,     0,   848,   849,   552,   277,   278,   279,   280,   281,
     282,   283,   284,   285,   286,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   248,     0,     0,     0,     0,     0,     0,     0,
     344,   345,   346,   347,   348,   349,   350,     0,     0,     0,
       0,     0,     0,   351,   352,     0,   353,   354,   355,   356,
       0,   360,   361,     0,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,     0,
     360,   361,   790,   362,   363,   364,   365,   366,     0,   367,
     368,   369,   370,   371,   372,   373,     0,     0,     0,   360,
     361,   793,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,     0,   360,   361,
     797,   362,   363,   364,   365,   366,     0,   367,   368,   369,
     370,   371,   372,   373,     0,     0,     0,   360,   361,   798,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,     0,   360,   361,   799,   362,
     363,   364,   365,   366,     0,   367,   368,   369,   370,   371,
     372,   373,     0,     0,     0,   360,   361,   800,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,     0,   360,   361,   801,   362,   363,   364,
     365,   366,     0,   367,   368,   369,   370,   371,   372,   373,
       0,     0,     0,   360,   361,   810,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,     0,   360,   361,   811,   362,   363,   364,   365,   366,
       0,   367,   368,   369,   370,   371,   372,   373,     0,     0,
       0,   360,   361,   812,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,     0,
     360,   361,   813,   362,   363,   364,   365,   366,     0,   367,
     368,   369,   370,   371,   372,   373,     0,     0,     0,   360,
     361,   814,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,     0,   360,   361,
     820,   362,   363,   364,   365,   366,     0,   367,   368,   369,
     370,   371,   372,   373,     0,     0,     0,   360,   361,   821,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,     0,   360,   361,   824,   362,
     363,   364,   365,   366,     0,   367,   368,   369,   370,   371,
     372,   373,     0,     0,     0,   360,   361,   825,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,     0,   360,   361,   826,   362,   363,   364,
     365,   366,     0,   367,   368,   369,   370,   371,   372,   373,
       0,     0,     0,   360,   361,   827,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,     0,   360,   361,   829,   362,   363,   364,   365,   366,
       0,   367,   368,   369,   370,   371,   372,   373,     0,     0,
       0,   360,   361,   831,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,     0,
     360,   361,   833,   362,   363,   364,   365,   366,     0,   367,
     368,   369,   370,   371,   372,   373,     0,     0,     0,   360,
     361,   835,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,     0,   360,   361,
     917,   362,   363,   364,   365,   366,     0,   367,   368,   369,
     370,   371,   372,   373,     0,     0,     0,   360,   361,   918,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,     0,   360,   361,   919,   362,
     363,   364,   365,   366,     0,   367,   368,   369,   370,   371,
     372,   373,     0,     0,     0,   360,   361,   920,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,     0,   360,   361,   922,   362,   363,   364,
     365,   366,     0,   367,   368,   369,   370,   371,   372,   373,
       0,     0,     0,   360,   361,   923,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,     0,   360,   361,   925,   362,   363,   364,   365,   366,
       0,   367,   368,   369,   370,   371,   372,   373,     0,     0,
       0,   360,   361,   926,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,     0,
     360,   361,   935,   362,   363,   364,   365,   366,     0,   367,
     368,   369,   370,   371,   372,   373,     0,     0,     0,   360,
     361,   937,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   553,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   769,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   770,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   771,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   772,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   773,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   774,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   776,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   777,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   778,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   779,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   780,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   782,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   783,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   785,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   786,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   787,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   788,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   789,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   796,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   802,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   803,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   804,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   805,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   806,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   807,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   815,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   816,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   817,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   818,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   819,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   822,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   823,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   837,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   839,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   840,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   841,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   842,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   843,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   902,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   903,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   904,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   905,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   906,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   907,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   908,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   909,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   910,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   913,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   915,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   916,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   921,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   924,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   927,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   928,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   929,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   930,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   931,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   933,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   944,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   965,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   966,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   967,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   968,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   969,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   970,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   971,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   972,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,   360,   361,   973,   362,   363,   364,   365,   366,     0,
     367,   368,   369,   370,   371,   372,   373,     0,     0,   360,
     361,   974,   362,   363,   364,   365,   366,     0,   367,   368,
     369,   370,   371,   372,   373,     0,     0,   360,   361,   975,
     362,   363,   364,   365,   366,     0,   367,   368,   369,   370,
     371,   372,   373,     0,     0,   360,   361,   976,   362,   363,
     364,   365,   366,     0,   367,   368,   369,   370,   371,   372,
     373,     0,     0,   360,   361,   977,   362,   363,   364,   365,
     366,     0,   367,   368,   369,   370,   371,   372,   373,     0,
       0,     0,     0,   978,   360,   361,   709,   362,   363,   364,
     365,   366,     0,   367,   368,   369,   370,   371,   372,   373,
     360,   361,     0,   362,   363,   364,   365,   366,     0,   367,
     368,   369,   370,   371,   372,   373
};

static const yytype_int16 yycheck[] =
{
      38,    39,   169,   170,   267,     3,     4,   174,     4,     7,
       8,     5,     5,    90,     5,     5,     5,     5,     9,     4,
       5,     4,   285,     5,     5,     5,    29,     4,     5,     5,
       4,     5,     5,     4,     3,     4,     5,     6,    41,     8,
     277,   278,   279,   280,     4,     5,     4,     5,     4,    15,
      16,   285,   286,   285,   286,     4,     4,     3,     3,   281,
      26,    27,     5,   281,    30,     4,     3,     3,     3,     4,
       5,     6,     7,     8,     3,     3,    42,     3,     3,     3,
       3,     3,     4,     5,     6,     5,     8,     5,     6,     3,
       3,     3,     3,     4,     3,     3,     7,     8,   270,   271,
     272,     3,   274,   275,   276,   277,   278,   279,   280,   276,
       3,     3,     5,     6,     3,     3,     3,     3,     3,     3,
       3,     4,     5,     6,     7,     8,     3,     3,     3,     3,
       4,     0,     5,   171,     8,   173,     3,     4,   267,     5,
       7,     8,   145,   146,   267,     5,     5,     3,     4,     5,
       6,     5,     8,    14,    15,    16,     5,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,   267,    30,
     267,   267,    33,    34,    35,    36,    37,   180,   181,   267,
      41,    42,   267,   267,    45,    46,   267,     3,    49,    50,
      51,   237,     4,   265,   266,   161,   268,   269,   270,   271,
     272,   267,   274,   275,   276,   277,   278,   279,   280,     3,
       5,     4,     4,   285,   286,     5,     3,   271,   271,   257,
     258,   259,   260,   271,    85,    86,   271,     3,     3,    90,
       3,    92,    93,     3,     3,   287,   265,   266,     3,   268,
     269,   270,   271,   272,     3,   274,   275,   276,   277,   278,
     279,   280,   113,     3,   115,     4,   285,   286,     7,     8,
     268,   269,   270,   271,   272,     3,   274,   275,   276,   277,
     278,   279,   280,     3,     3,     4,     5,     6,     3,     8,
       3,     3,     3,   281,     3,   283,   267,     3,   149,   258,
     259,   289,     3,   289,     4,   156,   289,     7,     8,   289,
     161,   289,   271,   272,   273,   274,   275,     4,   289,   289,
       7,     8,   281,   289,   283,   284,   289,     4,     3,   288,
     289,     8,   360,   361,   362,   363,   364,   365,   366,   367,
     368,   369,   370,   371,   372,   373,   258,   259,     3,   274,
     275,   276,     3,     3,   279,     4,   281,     5,   283,   271,
       5,   273,   274,   275,   289,     3,   274,   275,     5,   281,
       5,   283,   284,     4,     4,     4,   288,   289,     5,     5,
     281,   374,   283,     4,   284,   258,   259,     5,   289,   240,
     241,   274,   275,   276,   282,   552,   279,     3,     3,   250,
     273,   274,   275,     4,     5,     6,     7,     8,   281,     5,
     283,   284,   258,   259,     5,   288,   289,   281,   273,   283,
     280,   273,   284,   284,   281,   289,   283,   273,   274,   275,
       4,   284,   289,     7,     8,   281,   284,   283,   284,   467,
     282,   284,   288,   289,   269,   270,   271,   272,   284,   274,
     275,   276,   277,   278,   279,   280,   284,   284,   284,   487,
     488,   489,   490,   491,   492,   493,   494,   495,   496,   497,
     498,   499,   500,   501,   502,   503,   504,   505,   506,   507,
     508,   509,   510,   511,   512,   513,   514,   515,   516,   517,
     518,   519,   520,   521,   522,   523,   524,   525,   526,   527,
     528,   529,   530,   531,   532,   533,   534,   535,   536,   537,
     538,   539,   540,   541,   542,   543,   544,   545,   282,   547,
     548,   549,   550,   551,   480,   481,   482,   483,   484,   282,
     558,   559,   599,   600,   562,     5,   603,   284,   284,   258,
     259,   284,   281,   284,   283,     5,   284,   284,   284,   284,
     289,   708,   508,   509,   273,   274,   275,   284,     3,     4,
       5,     6,   281,     8,   283,   284,   284,   284,   284,   288,
     289,     3,     4,     5,     6,   271,     8,   284,     4,   284,
     284,   281,   284,   283,   540,   541,   542,   543,   544,   289,
     284,   284,   284,   284,   281,   284,   283,   271,   284,   284,
     284,   284,   289,   284,   281,   284,   283,   271,   284,   271,
     461,   284,   289,   606,   607,   608,   270,   271,   272,   284,
     274,   275,   276,   277,   278,   279,   280,   284,   284,   480,
     481,   482,   483,   484,   485,   486,     5,    18,    19,    20,
      21,    22,    23,    24,    25,   284,     4,    28,   284,     4,
     284,   284,    33,    34,     5,    36,   284,   508,   509,   284,
      41,   284,   284,   287,    45,    46,     5,   284,    49,    50,
      51,   284,   284,   274,   275,   284,   284,   284,   271,   271,
     281,   709,   283,     5,   284,   284,   284,   284,   289,   540,
     541,   542,   543,   544,   284,   546,   284,   284,   284,   284,
     284,   284,   284,   284,    85,    86,   284,   281,   284,   283,
     271,    92,    93,   284,   284,   289,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   755,   271,   757,
     271,   759,   113,   761,   115,   763,   271,   765,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   775,   599,   600,
     271,   271,   603,   781,   271,   271,   784,     5,   273,     5,
       5,     5,     5,   791,     5,     5,   794,     5,   149,   797,
     798,   799,   800,   801,     5,   156,     5,     5,     5,     5,
       5,   809,   810,   811,   812,   813,   814,     5,     5,     5,
       5,     5,   820,   821,     5,     5,   824,   825,   826,     5,
     828,     5,   830,     5,   832,     5,   834,     5,   836,     5,
     838,   286,     5,   258,   259,     5,   285,     3,   286,   285,
       4,   286,   285,     4,   286,   286,   258,   259,   273,   274,
     285,   285,   285,   285,     3,   286,   281,   285,   283,   284,
     285,   273,   274,   288,   289,   286,   285,     5,   286,   281,
     286,   283,   284,   286,     5,   286,   288,   289,   286,   240,
     241,   286,   713,   714,   285,     5,   286,   286,   285,   285,
       5,   827,   793,   829,    -1,   831,    -1,   833,    -1,   835,
      -1,    -1,    -1,    -1,   912,    -1,    -1,    -1,    -1,   917,
     918,   919,   920,    -1,   922,   923,    -1,   925,   926,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   934,   935,   936,   937,
      -1,    -1,   940,    -1,    -1,   943,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
     144,   145,   146,   147,   148,   149,   150,   151,   152,   153,
     154,   155,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,    -1,    -1,    -1,   827,    -1,   829,    -1,
     831,    -1,   833,    -1,   835,    -1,    -1,    -1,    -1,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   222,   223,   224,   225,   226,   227,   228,   229,
     230,   231,   232,   233,   234,   235,   236,   237,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   245,   246,   247,   248,   249,
     250,   251,    -1,    -1,    -1,    -1,    -1,    -1,   258,   259,
     461,   261,   262,   263,   264,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   480,
     481,   482,   483,   484,   485,   486,    -1,    -1,    -1,   265,
     266,    -1,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   508,   509,   285,
     286,   265,   266,    -1,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,    -1,
      -1,    -1,   286,    -1,     0,     1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   546,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   265,   266,    -1,   268,   269,   270,
     271,   272,    -1,   274,   275,   276,   277,   278,   279,   280,
      -1,    -1,    -1,   265,   266,   286,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,    -1,   265,   266,   286,   268,   269,   270,   271,   272,
      -1,   274,   275,   276,   277,   278,   279,   280,    -1,    -1,
      -1,    -1,    -1,   286,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   713,   714,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     245,   246,   247,   248,   249,   250,   251,    -1,    -1,    -1,
      -1,    -1,    -1,   258,   259,    -1,   261,   262,   263,   264,
      -1,   265,   266,    -1,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,    -1,
     265,   266,   286,   268,   269,   270,   271,   272,    -1,   274,
     275,   276,   277,   278,   279,   280,    -1,    -1,    -1,   265,
     266,   286,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,    -1,   265,   266,
     286,   268,   269,   270,   271,   272,    -1,   274,   275,   276,
     277,   278,   279,   280,    -1,    -1,    -1,   265,   266,   286,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,    -1,   265,   266,   286,   268,
     269,   270,   271,   272,    -1,   274,   275,   276,   277,   278,
     279,   280,    -1,    -1,    -1,   265,   266,   286,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,    -1,   265,   266,   286,   268,   269,   270,
     271,   272,    -1,   274,   275,   276,   277,   278,   279,   280,
      -1,    -1,    -1,   265,   266,   286,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,    -1,   265,   266,   286,   268,   269,   270,   271,   272,
      -1,   274,   275,   276,   277,   278,   279,   280,    -1,    -1,
      -1,   265,   266,   286,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,    -1,
     265,   266,   286,   268,   269,   270,   271,   272,    -1,   274,
     275,   276,   277,   278,   279,   280,    -1,    -1,    -1,   265,
     266,   286,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,    -1,   265,   266,
     286,   268,   269,   270,   271,   272,    -1,   274,   275,   276,
     277,   278,   279,   280,    -1,    -1,    -1,   265,   266,   286,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,    -1,   265,   266,   286,   268,
     269,   270,   271,   272,    -1,   274,   275,   276,   277,   278,
     279,   280,    -1,    -1,    -1,   265,   266,   286,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,    -1,   265,   266,   286,   268,   269,   270,
     271,   272,    -1,   274,   275,   276,   277,   278,   279,   280,
      -1,    -1,    -1,   265,   266,   286,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,    -1,   265,   266,   286,   268,   269,   270,   271,   272,
      -1,   274,   275,   276,   277,   278,   279,   280,    -1,    -1,
      -1,   265,   266,   286,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,    -1,
     265,   266,   286,   268,   269,   270,   271,   272,    -1,   274,
     275,   276,   277,   278,   279,   280,    -1,    -1,    -1,   265,
     266,   286,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,    -1,   265,   266,
     286,   268,   269,   270,   271,   272,    -1,   274,   275,   276,
     277,   278,   279,   280,    -1,    -1,    -1,   265,   266,   286,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,    -1,   265,   266,   286,   268,
     269,   270,   271,   272,    -1,   274,   275,   276,   277,   278,
     279,   280,    -1,    -1,    -1,   265,   266,   286,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,    -1,   265,   266,   286,   268,   269,   270,
     271,   272,    -1,   274,   275,   276,   277,   278,   279,   280,
      -1,    -1,    -1,   265,   266,   286,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,    -1,   265,   266,   286,   268,   269,   270,   271,   272,
      -1,   274,   275,   276,   277,   278,   279,   280,    -1,    -1,
      -1,   265,   266,   286,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,    -1,
     265,   266,   286,   268,   269,   270,   271,   272,    -1,   274,
     275,   276,   277,   278,   279,   280,    -1,    -1,    -1,   265,
     266,   286,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   265,   266,   285,   268,   269,   270,   271,   272,    -1,
     274,   275,   276,   277,   278,   279,   280,    -1,    -1,   265,
     266,   285,   268,   269,   270,   271,   272,    -1,   274,   275,
     276,   277,   278,   279,   280,    -1,    -1,   265,   266,   285,
     268,   269,   270,   271,   272,    -1,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   265,   266,   285,   268,   269,
     270,   271,   272,    -1,   274,   275,   276,   277,   278,   279,
     280,    -1,    -1,   265,   266,   285,   268,   269,   270,   271,
     272,    -1,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,    -1,    -1,   285,   265,   266,   267,   268,   269,   270,
     271,   272,    -1,   274,   275,   276,   277,   278,   279,   280,
     265,   266,    -1,   268,   269,   270,   271,   272,    -1,   274,
     275,   276,   277,   278,   279,   280
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,     1,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     291,     4,     5,     4,     5,     5,     5,     4,     4,     4,
       5,     4,     5,     4,     4,     5,     4,     3,     4,     7,
       8,   281,   283,   289,   295,   296,   297,   295,   296,   295,
       4,     5,   297,   297,   297,   297,   297,   297,   297,   297,
     295,   295,     3,     5,     6,   274,   275,   276,   279,   297,
     298,     3,   276,   279,   298,   295,     3,   297,   297,     3,
     296,   299,   297,   296,     3,     5,     6,   258,   259,   273,
     274,   275,   283,   284,   288,   292,   293,   296,   293,     3,
     297,   298,   295,     5,   297,   281,   297,   281,   281,   297,
     297,   297,     4,     3,     3,     3,     3,     3,     3,     3,
       3,     5,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,   297,   297,     3,
       3,   300,   299,   297,   297,     5,     5,     5,     5,     5,
       5,     0,   267,   267,   267,   267,   267,   267,   267,   267,
       3,   297,   267,     4,   297,     5,   289,     3,   237,     4,
     267,     3,     4,     4,     5,     5,     3,   271,   271,   271,
     271,     3,   298,   298,     3,     3,   297,     3,     3,   297,
     295,   296,   287,   292,   292,   293,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   189,   190,   191,   192,
     193,   194,   195,   196,   197,   198,   199,   200,   201,   202,
     203,   204,   205,   206,   207,   208,   209,   210,   211,   212,
     213,   214,   215,   216,   217,   218,   219,   220,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   245,   246,   247,   248,   249,   250,
     251,   258,   259,   261,   262,   263,   264,   283,   293,   292,
     265,   266,   268,   269,   270,   271,   272,   274,   275,   276,
     277,   278,   279,   280,   298,   298,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     5,     5,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   301,     3,     5,     5,     4,     4,     4,     5,     4,
       5,   297,   297,     4,   267,     5,   282,   284,     5,   289,
     296,     3,     3,     5,   293,   293,   293,   293,     5,   292,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   284,   284,   284,   284,   284,   284,   284,   284,
     284,   284,   169,   285,   293,   293,   293,   293,   271,   272,
     293,   293,   271,   293,   293,   293,   293,   293,   293,   293,
     293,   298,   282,   282,   282,     5,     5,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   271,   271,   271,   271,   271,   271,   271,   271,   271,
     271,   297,     4,   273,   293,     5,     4,     4,     5,   287,
     295,   297,   295,   297,   295,   297,   295,   297,   295,   297,
     297,   297,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   295,   296,   297,   293,   295,   297,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   295,   296,   293,   295,   293,   295,   293,   295,   293,
     295,   293,   297,   293,   293,   293,   293,   293,   285,   267,
     293,   293,   293,   273,   273,     5,     5,     5,     5,     5,
       5,     5,     5,     5,     5,     5,     5,     5,     5,     5,
       5,     5,     5,     5,     5,     5,     5,     5,     5,     5,
     299,   299,     5,     5,   299,     5,     5,   298,   298,   298,
       5,     9,     5,     4,   289,   286,     5,   286,   285,   286,
     285,   286,   285,   286,   285,   286,   285,   285,   285,   285,
     285,   285,   285,   285,   285,   286,   285,   285,   285,   285,
     285,   286,   285,   285,   286,   285,   285,   285,   285,   285,
     286,   286,   285,   286,   286,   285,   285,   286,   286,   286,
     286,   286,   285,   285,   285,   285,   285,   285,   285,   286,
     286,   286,   286,   286,   286,   285,   285,   285,   285,   285,
     286,   286,   285,   285,   286,   286,   286,   286,   286,   286,
     286,   286,   286,   286,   286,   286,   286,   285,   286,   285,
     285,   285,   285,   285,   292,   293,     4,   289,   297,   297,
       3,     5,   289,     4,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   294,   293,   294,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   295,   293,   295,   293,   295,   293,
     295,   293,   295,   293,   293,     5,   289,     4,     3,     5,
       5,   289,   285,   285,   285,   285,   285,   285,   285,   285,
     285,   285,   286,   285,   285,   285,   285,   286,   286,   286,
     286,   285,   286,   286,   285,   286,   286,   285,   285,   285,
     285,   285,   285,   285,   286,   286,   286,   286,   285,   285,
     286,   285,   285,   286,   285,     5,     5,   289,     5,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,     5,   285,   285,   285,   285,   285,
     285,   285,   285,   285,   285,   285,   285,   285,   285
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   290,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   291,   291,
     291,   291,   291,   291,   291,   291,   291,   291,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   292,   292,
     292,   292,   292,   292,   292,   292,   292,   292,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   293,   293,   293,
     293,   293,   293,   293,   293,   293,   293,   294,   294,   295,
     295,   296,   296,   296,   296,   296,   296,   296,   296,   296,
     296,   297,   297,   298,   298,   298,   298,   299,   299,   300,
     300,   301,   301,   301,   301,   301,   301,   301,   301,   301,
     301,   301,   301,   301,   301,   301,   301,   301,   301,   301,
     301,   301,   301,   301,   301,   301,   301,   301,   301,   301,
     301,   301,   301,   301,   301,   301,   301,   301,   301,   301,
     301,   301,   301,   301,   301,   301,   301,   301,   301,   301,
     301,   301,   301,   301,   301,   301,   301,   301,   301,   301,
     301,   301,   301,   301,   301,   301,   301,   301,   301,   301,
     301,   301,   301,   301,   301,   301,   301,   301,   301,   301,
     301,   301,   301,   301,   301,   301,   301,   301,   301,   301
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     4,     3,     4,     4,     4,     2,     2,     2,
       3,     3,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     4,     4,     4,     4,     1,     3,     3,
       4,     4,     5,     4,     3,     4,     2,     4,     2,     4,
       2,     4,     2,     3,     5,     5,     4,     2,     5,     3,
       3,     2,     5,     3,     2,     3,     2,     2,     2,     3,
       3,     2,     2,     3,     3,     2,     2,     1,     2,     2,
       2,     2,     1,     1,     1,     2,     2,     2,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       2,     2,     2,     2,     2,     3,     3,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     5,     5,     1,
       2,     1,     3,     2,     3,     3,     2,     2,     2,     3,
       1,     1,     1,     1,     1,     1,     1,     2,     6,     2,
       2,     2,     7,     6,     2,     2,     0,     1,     1,     3,
       5,     5,     7,     5,     7,     5,     7,     5,     7,     5,
       7,     5,     7,     7,     5,     7,     7,     5,     5,     5,
       5,     5,     5,     5,     5,     7,     5,     5,     5,     5,
       5,     7,     5,     5,     7,     5,     5,     5,     5,     5,
       5,     7,     9,     9,     9,     9,     5,     5,     5,     5,
       5,     5,     2,     9,     5,     9,     5,     5,     7,     5,
       7,     7,     5,     5,     5,     7,     7,     9,     7,     7,
       9,     9,     9,     9,     7,     7,     9,     7,     7,     2,
       7,     2,     2,     2,     7,     7,     9,     5,     5,     9,
       3,     2,     1,     1,     2,     1,     2,     2,     5,     5,
       5,     5,     5,     2,     1,     2,     1,     5,     3,     3,
       3,     3,     3,     2,     3,     1,     5,     3,     3,     3,
       3,     3,     3,     4,     4,     4,     3,     1,     3,     3,
       1,     2,     3,     3,     4,     7,     6,     7,     7,     8,
       1,     1,     1,     1,     1,     2,     2,     1,     1,     0,
       2,     3,     3,     1,     1,     1,     3,     1,     3,     1,
       3,     1,     1,     3,     1,     1,     3,     1,     1,     1,
       3,     1,     1,     3,     1,     1,     3,     1,     1,     3,
       1,     1,     3,     1,     1,     3,     1,     1,     3,     1,
       1,     3,     1,     3,     1,     1,     1,     3,     1,     1,
       3,     1,     1,     3,     1,     1,     3,     1,     1,     3,
       1,     1,     3,     1,     1,     3,     1,     1,     3,     3,
       3,     1,     3,     1,     3,     1,     1,     3,     1,     3,
       3,     1,     3,     1,     3,     1,     3,     3,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* command: S_LET var_or_range '=' e  */
#line 441 "gram.y"
                                  {
                                  struct roman * roman = session->cur_doc;
                                  struct sheet * sh = roman->cur_sh;
                                  let(roman, sh, (yyvsp[-2].rval).left.vp, (yyvsp[0].enode));
                                  }
#line 3145 "y.tab.c"
    break;

  case 3: /* command: S_LET var_or_range '='  */
#line 448 "gram.y"
                                  {
                                  // TODO get this code out of gram.y - reeval cells that depends on $2
                                  extern graphADT graph;
                                  struct roman * roman = session->cur_doc;
                                  struct sheet * sh = roman->cur_sh;
#ifdef UNDO
                                  // here we save in undostruct, all the ents that depends on the deleted one (before change)
                                  ents_that_depends_on_range(sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col);
                                  create_undo_action();
                                  copy_to_undostruct(sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, UNDO_DEL, HANDLE_DEPS, NULL);
#endif

                                  if (getVertex(graph, sh, lookat(sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col), 0) != NULL) destroy_vertex(sh, lookat(sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col));

                                  (yyvsp[-1].rval).left.vp->v = (double) 0.0;
                                  if ((yyvsp[-1].rval).left.vp->expr && !((yyvsp[-1].rval).left.vp->flags & is_strexpr)) {
                                      efree((yyvsp[-1].rval).left.vp->expr);
                                      (yyvsp[-1].rval).left.vp->expr = NULL;
                                  }
                                  (yyvsp[-1].rval).left.vp->cellerror = CELLOK;
                                  (yyvsp[-1].rval).left.vp->flags &= ~is_valid;
                                  (yyvsp[-1].rval).left.vp->flags |= is_changed;
                                  roman->modflg++;

                                  // clearing the value counts as a write, so run write triggers
                                  if (( (yyvsp[-1].rval).left.vp->trigger  ) && (((yyvsp[-1].rval).left.vp->trigger->flag & TRG_WRITE) == TRG_WRITE))
                                      do_trigger((yyvsp[-1].rval).left.vp, TRG_WRITE);

#ifdef UNDO
                                  // here we save in undostruct, all the ents that depends on the deleted one (after change)
                                  copy_to_undostruct(sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, UNDO_ADD, HANDLE_DEPS, NULL);
                                  extern struct ent_ptr * deps;
                                  if (deps != NULL) {
                                      free(deps);
                                      deps = NULL;
                                  }
                                  end_undo_action();
#endif
                                  }
#line 3189 "y.tab.c"
    break;

  case 4: /* command: S_LABEL var_or_range '=' e  */
#line 488 "gram.y"
                                          {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            slet(roman, sh, (yyvsp[-2].rval).left.vp, (yyvsp[0].enode), 0);
                                          }
#line 3199 "y.tab.c"
    break;

  case 5: /* command: S_LEFTSTRING var_or_range '=' e  */
#line 494 "gram.y"
                                          {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            slet(roman, sh, (yyvsp[-2].rval).left.vp, (yyvsp[0].enode), -1);
                                          }
#line 3209 "y.tab.c"
    break;

  case 6: /* command: S_RIGHTSTRING var_or_range '=' e  */
#line 499 "gram.y"
                                          {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            slet(roman, sh, (yyvsp[-2].rval).left.vp, (yyvsp[0].enode), 1);
                                          }
#line 3219 "y.tab.c"
    break;

  case 7: /* command: S_LEFTJUSTIFY var_or_range  */
#line 504 "gram.y"
                                     {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            ljustify(sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 3228 "y.tab.c"
    break;

  case 8: /* command: S_RIGHTJUSTIFY var_or_range  */
#line 508 "gram.y"
                                     {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            rjustify(sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 3237 "y.tab.c"
    break;

  case 9: /* command: S_CENTER var_or_range  */
#line 513 "gram.y"
                                     {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            center(sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 3246 "y.tab.c"
    break;

  case 10: /* command: S_FMT var_or_range STRING  */
#line 517 "gram.y"
                                     {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            format_cell(sh, (yyvsp[-1].rval).left.vp, (yyvsp[-1].rval).right.vp, (yyvsp[0].sval));
                                            scxfree((yyvsp[0].sval));
                                     }
#line 3257 "y.tab.c"
    break;

  case 11: /* command: S_DATEFMT var_or_range STRING  */
#line 524 "gram.y"
                                       {
                                            struct roman * roman = session->cur_doc;
                                            struct sheet * sh = roman->cur_sh;
                                            dateformat(sh, (yyvsp[-1].rval).left.vp, (yyvsp[-1].rval).right.vp, (yyvsp[0].sval));
                                            scxfree((yyvsp[0].sval));
                                       }
#line 3268 "y.tab.c"
    break;

  case 12: /* command: S_DATEFMT STRING  */
#line 530 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       dateformat(sh, lookat(sh, sh->currow, sh->curcol), lookat(sh, sh->currow, sh->curcol), (yyvsp[0].sval));
                                       scxfree((yyvsp[0].sval)); }
#line 3278 "y.tab.c"
    break;

  case 13: /* command: S_HIDE COL  */
#line 536 "gram.y"
                                     { hide_col((yyvsp[0].ival), 1); }
#line 3284 "y.tab.c"
    break;

  case 14: /* command: S_HIDE NUMBER  */
#line 537 "gram.y"
                                     { hide_row((yyvsp[0].ival), 1); }
#line 3290 "y.tab.c"
    break;

  case 15: /* command: S_SHOW COL  */
#line 538 "gram.y"
                                     { show_col((yyvsp[0].ival), 1); }
#line 3296 "y.tab.c"
    break;

  case 16: /* command: S_SHOW NUMBER  */
#line 539 "gram.y"
                                     { show_row((yyvsp[0].ival), 1); }
#line 3302 "y.tab.c"
    break;

  case 17: /* command: S_DELETECOL COL  */
#line 542 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       deletecol(sh, (yyvsp[0].ival), 1);
                                     }
#line 3312 "y.tab.c"
    break;

  case 18: /* command: S_DELETEROW NUMBER  */
#line 547 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       deleterow(sh, (yyvsp[0].ival), 1);
                                     }
#line 3322 "y.tab.c"
    break;

  case 19: /* command: S_HIDECOL COL  */
#line 554 "gram.y"
                                     {
                                       hide_col((yyvsp[0].ival), 1); }
#line 3329 "y.tab.c"
    break;

  case 20: /* command: S_SHOWCOL COL  */
#line 556 "gram.y"
                                     {
                                       show_col((yyvsp[0].ival), 1); }
#line 3336 "y.tab.c"
    break;

  case 21: /* command: S_HIDEROW NUMBER  */
#line 558 "gram.y"
                                     {
                                       hide_row((yyvsp[0].ival), 1); }
#line 3343 "y.tab.c"
    break;

  case 22: /* command: S_SHOWROW NUMBER  */
#line 560 "gram.y"
                                     {
                                       show_row((yyvsp[0].ival), 1); }
#line 3350 "y.tab.c"
    break;

  case 23: /* command: S_SHOWCOL COL ':' COL  */
#line 562 "gram.y"
                                     {
                                       show_col((yyvsp[-2].ival), (yyvsp[0].ival)-(yyvsp[-2].ival)+1); }
#line 3357 "y.tab.c"
    break;

  case 24: /* command: S_SHOWROW NUMBER ':' NUMBER  */
#line 564 "gram.y"
                                     {
                                       show_row((yyvsp[-2].ival), (yyvsp[0].ival)-(yyvsp[-2].ival)+1); }
#line 3364 "y.tab.c"
    break;

  case 25: /* command: S_HIDECOL COL ':' COL  */
#line 566 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       int c = sh->curcol, arg;
                                       if ((yyvsp[-2].ival) < (yyvsp[0].ival)) {
                                            sh->curcol = (yyvsp[-2].ival);
                                            arg = (yyvsp[0].ival) - (yyvsp[-2].ival) + 1;
                                       } else {
                                            sh->curcol = (yyvsp[0].ival);
                                            arg = (yyvsp[-2].ival) - (yyvsp[0].ival) + 1;
                                       }
                                       hide_col((yyvsp[-2].ival), arg);        // hide de un rango de columnas
                                       sh->curcol = c < sh->curcol ? c : c < sh->curcol + arg ? sh->curcol : c - arg;
                                     }
#line 3383 "y.tab.c"
    break;

  case 26: /* command: S_HIDEROW NUMBER ':' NUMBER  */
#line 580 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       int r = sh->currow, arg;      // hide de un rango de filas
                                       if ((yyvsp[-2].ival) < (yyvsp[0].ival)) {
                                           sh->currow = (yyvsp[-2].ival);
                                           arg = (yyvsp[0].ival) - (yyvsp[-2].ival) + 1;
                                       } else {
                                           sh->currow = (yyvsp[0].ival);
                                           arg = (yyvsp[-2].ival) - (yyvsp[0].ival) + 1;
                                       }
                                       hide_row((yyvsp[-2].ival), arg);
                                       sh->currow = r < sh->currow ? r : r < sh->currow + arg ? sh->currow : r - arg;
                                     }
#line 3402 "y.tab.c"
    break;

  case 27: /* command: S_VALUEIZEALL  */
#line 595 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       valueize_area(sh, 0, 0, sh->maxrow, sh->maxcol); }
#line 3411 "y.tab.c"
    break;

  case 28: /* command: S_SHIFT var_or_range STRING  */
#line 600 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       if (strlen((yyvsp[0].sval)) != 1 || ((yyvsp[0].sval)[0] != 'h' && (yyvsp[0].sval)[0] != 'j' && (yyvsp[0].sval)[0] != 'k' && (yyvsp[0].sval)[0] != 'l')) {
                                           sc_error("wrong parameter for shift command");
                                       } else {
                                           wchar_t wstr[2] = L"";
                                           swprintf(wstr, BUFFERSIZE, L"%c", (yyvsp[0].sval)[0]);
                                           shift(sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).right.vp->row, (yyvsp[-1].rval).right.vp->col, wstr[0]);
                                       }
                                       scxfree((yyvsp[0].sval));
                                     }
#line 3428 "y.tab.c"
    break;

  case 29: /* command: S_MARK COL var_or_range  */
#line 613 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       set_cell_mark((yyvsp[-1].ival) + 97, sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col);
                                     }
#line 3438 "y.tab.c"
    break;

  case 30: /* command: S_MARK COL var_or_range var_or_range  */
#line 619 "gram.y"
                                              {
                                          struct roman * roman = session->cur_doc;
                                          struct sheet * sh = roman->cur_sh;
                                          srange * sr = create_range('\0', '\0', (yyvsp[-1].rval).left.vp, (yyvsp[0].rval).left.vp);
                                          unselect_ranges();
                                          set_range_mark((yyvsp[-2].ival) + 97, sh, sr);
                                     }
#line 3450 "y.tab.c"
    break;

  case 31: /* command: S_MARK COL STRING var_or_range  */
#line 627 "gram.y"
                                            {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh;
                                       if ((sh = search_sheet(roman, (yyvsp[-1].sval))) != NULL ) {
                                           set_cell_mark((yyvsp[-2].ival) + 97, sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col);
                                       }
                                       scxfree((yyvsp[-1].sval));
                                     }
#line 3463 "y.tab.c"
    break;

  case 32: /* command: S_MARK COL STRING var_or_range var_or_range  */
#line 636 "gram.y"
                                                     {
                                          struct roman * roman = session->cur_doc;
                                          struct sheet * sh;
                                          if ((sh = search_sheet(roman, (yyvsp[-2].sval))) != NULL ) {
                                              srange * sr = create_range('\0', '\0', (yyvsp[-1].rval).left.vp, (yyvsp[0].rval).left.vp);
                                              unselect_ranges();
                                              set_range_mark((yyvsp[-3].ival) + 97, sh, sr);
                                          }
                                          scxfree((yyvsp[-2].sval));
                                     }
#line 3478 "y.tab.c"
    break;

  case 33: /* command: S_FILL var_or_range num num  */
#line 646 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       fill(sh, (yyvsp[-2].rval).left.vp, (yyvsp[-2].rval).right.vp, (yyvsp[-1].fval), (yyvsp[0].fval));
                                     }
#line 3488 "y.tab.c"
    break;

  case 34: /* command: S_FILL num num  */
#line 652 "gram.y"
                                     { sc_error("Not enough parameters for fill command"); }
#line 3494 "y.tab.c"
    break;

  case 35: /* command: S_FREEZE NUMBER ':' NUMBER  */
#line 654 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, (yyvsp[-2].ival), 0), lookat(sh, (yyvsp[0].ival), 0), 1, 'r'); }
#line 3503 "y.tab.c"
    break;

  case 36: /* command: S_FREEZE NUMBER  */
#line 658 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, (yyvsp[0].ival), 0), lookat(sh, (yyvsp[0].ival), 0), 1, 'r'); }
#line 3512 "y.tab.c"
    break;

  case 37: /* command: S_FREEZE COL ':' COL  */
#line 662 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, 0, (yyvsp[-2].ival)), lookat(sh, 0, (yyvsp[0].ival)), 1, 'c'); }
#line 3521 "y.tab.c"
    break;

  case 38: /* command: S_FREEZE COL  */
#line 666 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, 0, (yyvsp[0].ival)), lookat(sh, 0, (yyvsp[0].ival)), 1, 'c'); }
#line 3530 "y.tab.c"
    break;

  case 39: /* command: S_UNFREEZE NUMBER ':' NUMBER  */
#line 670 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, (yyvsp[-2].ival), 0), lookat(sh, (yyvsp[0].ival), 0), 0, 'r'); }
#line 3539 "y.tab.c"
    break;

  case 40: /* command: S_UNFREEZE NUMBER  */
#line 674 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, (yyvsp[0].ival), 0), lookat(sh, (yyvsp[0].ival), 0), 0, 'r'); }
#line 3548 "y.tab.c"
    break;

  case 41: /* command: S_UNFREEZE COL ':' COL  */
#line 678 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, 0, (yyvsp[-2].ival)), lookat(sh, 0, (yyvsp[0].ival)), 0, 'c'); }
#line 3557 "y.tab.c"
    break;

  case 42: /* command: S_UNFREEZE COL  */
#line 682 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       handle_freeze(sh, lookat(sh, 0, (yyvsp[0].ival)), lookat(sh, 0, (yyvsp[0].ival)), 0, 'c'); }
#line 3566 "y.tab.c"
    break;

  case 43: /* command: S_SORT range STRING  */
#line 687 "gram.y"
                                     { sortrange(session->cur_doc->cur_sh, (yyvsp[-1].rval).left.vp, (yyvsp[-1].rval).right.vp, (yyvsp[0].sval));
                                       //scxfree($3);
                                       //do not free here
                                     }
#line 3575 "y.tab.c"
    break;

  case 44: /* command: S_SUBTOTAL range COL STRING COL  */
#line 691 "gram.y"
                                         {
                                       subtotal((yyvsp[-3].rval).left.vp->row, (yyvsp[-3].rval).left.vp->col, (yyvsp[-3].rval).right.vp->row,
                                                (yyvsp[-3].rval).right.vp->col, (yyvsp[-2].ival), (yyvsp[-1].sval), (yyvsp[0].ival), 0);
                                       scxfree((yyvsp[-1].sval));
                                     }
#line 3585 "y.tab.c"
    break;

  case 45: /* command: S_RSUBTOTAL range COL STRING COL  */
#line 696 "gram.y"
                                          {
                                       subtotal((yyvsp[-3].rval).left.vp->row, (yyvsp[-3].rval).left.vp->col, (yyvsp[-3].rval).right.vp->row,
                                                 (yyvsp[-3].rval).right.vp->col, (yyvsp[-2].ival), (yyvsp[-1].sval), (yyvsp[0].ival), 1);
                                       scxfree((yyvsp[-1].sval));
                                     }
#line 3595 "y.tab.c"
    break;

  case 46: /* command: S_AUTOFIT COL ':' COL  */
#line 710 "gram.y"
                                     { auto_fit(session->cur_doc->cur_sh, (yyvsp[-2].ival), (yyvsp[0].ival), DEFWIDTH); }
#line 3601 "y.tab.c"
    break;

  case 47: /* command: S_AUTOFIT COL  */
#line 711 "gram.y"
                                     { auto_fit(session->cur_doc->cur_sh, (yyvsp[0].ival), (yyvsp[0].ival), DEFWIDTH); }
#line 3607 "y.tab.c"
    break;

  case 48: /* command: S_PAD NUMBER COL ':' COL  */
#line 713 "gram.y"
                                   {
                                       pad(session->cur_doc->cur_sh, (yyvsp[-3].ival), 0, (yyvsp[-2].ival), session->cur_doc->cur_sh->maxrow, (yyvsp[0].ival)); }
#line 3614 "y.tab.c"
    break;

  case 49: /* command: S_PAD NUMBER COL  */
#line 715 "gram.y"
                                   {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       pad(sh, (yyvsp[-1].ival), 0, (yyvsp[0].ival), sh->maxrow, (yyvsp[0].ival)); }
#line 3623 "y.tab.c"
    break;

  case 50: /* command: S_PAD NUMBER var_or_range  */
#line 719 "gram.y"
                                   {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       pad(sh, (yyvsp[-1].ival), (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 3632 "y.tab.c"
    break;

  case 51: /* command: S_GETFORMAT COL  */
#line 723 "gram.y"
                                   {   getformat((yyvsp[0].ival), fdoutput); }
#line 3638 "y.tab.c"
    break;

  case 52: /* command: S_FORMAT COL NUMBER NUMBER NUMBER  */
#line 724 "gram.y"
                                           {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       doformat(sh, (yyvsp[-3].ival),(yyvsp[-3].ival),(yyvsp[-2].ival),(yyvsp[-1].ival),(yyvsp[0].ival));
                                       }
#line 3648 "y.tab.c"
    break;

  case 53: /* command: S_FORMAT NUMBER NUMBER  */
#line 729 "gram.y"
                                     {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       dorowformat(sh, (yyvsp[-1].ival), (yyvsp[0].ival));
                                     }
#line 3658 "y.tab.c"
    break;

  case 54: /* command: S_FILTERON range  */
#line 735 "gram.y"
                                     { enable_filters((yyvsp[0].rval).left.vp, (yyvsp[0].rval).right.vp);
                                     }
#line 3665 "y.tab.c"
    break;

  case 55: /* command: S_GOTO var_or_range var_or_range  */
#line 737 "gram.y"
                                          {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       moveto(sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).right.vp->row, (yyvsp[-1].rval).right.vp->col, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col);
                                     }
#line 3675 "y.tab.c"
    break;

  case 56: /* command: S_GOTO var_or_range  */
#line 742 "gram.y"
                                 {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       moveto(sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, -1, -1);
                                 }
#line 3685 "y.tab.c"
    break;

  case 57: /* command: S_GOTO num  */
#line 747 "gram.y"
                                 {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       num_search(sh, (yyvsp[0].fval), 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 0, 1); }
#line 3694 "y.tab.c"
    break;

  case 58: /* command: S_GOTO STRING  */
#line 751 "gram.y"
                                 {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, (yyvsp[0].sval), 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 0, 1); }
#line 3703 "y.tab.c"
    break;

  case 59: /* command: S_GOTO '#' STRING  */
#line 756 "gram.y"
                                 {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, (yyvsp[0].sval), 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 1, 1); }
#line 3712 "y.tab.c"
    break;

  case 60: /* command: S_GOTO '%' STRING  */
#line 761 "gram.y"
                                 {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, (yyvsp[0].sval), 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 2, 1); }
#line 3721 "y.tab.c"
    break;

  case 61: /* command: S_GOTOB num  */
#line 767 "gram.y"
                                  {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       num_search(sh, (yyvsp[0].fval), 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 0, 0); }
#line 3730 "y.tab.c"
    break;

  case 62: /* command: S_GOTOB STRING  */
#line 772 "gram.y"
                                  {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, (yyvsp[0].sval), 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 0, 0); }
#line 3739 "y.tab.c"
    break;

  case 63: /* command: S_GOTOB '#' STRING  */
#line 777 "gram.y"
                                  {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, (yyvsp[0].sval), 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 1, 0); }
#line 3748 "y.tab.c"
    break;

  case 64: /* command: S_GOTOB '%' STRING  */
#line 782 "gram.y"
                                  {
                                       struct roman * roman = session->cur_doc;
                                       struct sheet * sh = roman->cur_sh;
                                       str_search(sh, (yyvsp[0].sval), 0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol, 2, 0); }
#line 3757 "y.tab.c"
    break;

  case 65: /* command: S_CCOPY range  */
#line 789 "gram.y"
                                 { copy_to_clipboard((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 3763 "y.tab.c"
    break;

  case 66: /* command: S_STRTONUM range  */
#line 790 "gram.y"
                                 { convert_string_to_number((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col); }
#line 3769 "y.tab.c"
    break;

  case 67: /* command: S_CPASTE  */
#line 791 "gram.y"
                                 { paste_from_clipboard(); }
#line 3775 "y.tab.c"
    break;

  case 68: /* command: S_LOCK var_or_range  */
#line 792 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   lock_cells(sh, (yyvsp[0].rval).left.vp, (yyvsp[0].rval).right.vp);
                                 }
#line 3785 "y.tab.c"
    break;

  case 69: /* command: S_UNLOCK var_or_range  */
#line 797 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   unlock_cells(sh, (yyvsp[0].rval).left.vp, (yyvsp[0].rval).right.vp);
                                 }
#line 3795 "y.tab.c"
    break;

  case 70: /* command: S_NEWSHEET STRING  */
#line 802 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh;

                                   // do not need to alloc a new 'Sheet1'
                                   // just reuse the just allocated 'Sheet1' in load_file();
                                   if (! strcmp((yyvsp[0].sval), "Sheet1") && (sh = search_sheet(roman, (yyvsp[0].sval))) != NULL && sh->flags & is_allocated) {
                                       sh->flags &= ~is_allocated;
                                       sh->flags |= is_empty;
                                       scxfree((yyvsp[0].sval));
                                       chg_mode('.');

                                   // if a sheet already exists with the name we are trying to create
                                   } else if ((sh = search_sheet(roman, (yyvsp[0].sval))) != NULL ) {
                                       sc_info("sheet already exist with that name");
                                       scxfree((yyvsp[0].sval));
                                       chg_mode('.');

                                   // if a just allocated 'Sheet1' exists, reuse it and do not malloc a new one.
                                   } else if ((sh = search_sheet(roman, "Sheet1")) != NULL && sh->flags & is_allocated) {
                                       sh->flags &= ~is_allocated;
                                       sh->flags |= is_empty;
                                       free(sh->name);
                                       sh->name = (yyvsp[0].sval);
                                       chg_mode('.');
                                       ui_update(TRUE);

                                   // if reached here, now yes malloc a new one
                                   } else {
                                       roman->cur_sh = new_sheet(roman, (yyvsp[0].sval));
                                       growtbl(roman->cur_sh, GROWNEW, 0, 0);
                                       erasedb(roman->cur_sh, 0);
                                       scxfree((yyvsp[0].sval));
                                       roman->modflg++;
                                       chg_mode('.');
                                       ui_update(TRUE);
                                   }
                                 }
#line 3838 "y.tab.c"
    break;

  case 71: /* command: S_DELSHEET STRING  */
#line 840 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh;
                                   if ((sh = search_sheet(roman, (yyvsp[0].sval))) == NULL ) {
                                       sc_info("No sheet exists with that name");
                                       scxfree((yyvsp[0].sval));
                                   } else if (roman->cur_sh == sh && sh->next == NULL && sh->prev == NULL) {
                                       sc_info("Cannot delete the only sheet of document");
                                       scxfree((yyvsp[0].sval));
                                   } else {
                                       if (roman->cur_sh == sh && sh->next != NULL)
                                           roman->cur_sh = sh->next;
                                       else if (roman->cur_sh == sh)
                                           roman->cur_sh = sh->prev;
                                       delete_sheet(roman, sh, 0);
                                       sh = NULL;
                                       roman->modflg++;
                                       scxfree((yyvsp[0].sval));
                                       chg_mode('.');
                                       ui_update(TRUE);
                                   }
                                 }
#line 3865 "y.tab.c"
    break;

  case 72: /* command: S_DELSHEET  */
#line 862 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   if (sh->next == NULL && sh->prev == NULL) {
                                       sc_info("Cannot delete the only sheet of document");
                                   } else {
                                       if (roman->cur_sh == sh && sh->next != NULL)
                                           roman->cur_sh = sh->next;
                                       else if (roman->cur_sh == sh)
                                           roman->cur_sh = sh->prev;
                                       delete_sheet(roman, sh, 0);
                                       sh = NULL;
                                       roman->modflg++;
                                       chg_mode('.');
                                       ui_update(TRUE);
                                   }
                                 }
#line 3887 "y.tab.c"
    break;

  case 73: /* command: S_NEXTSHEET  */
#line 879 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   if (roman->cur_sh->next != NULL) {
                                       roman->cur_sh = roman->cur_sh->next;
                                   } else if (roman->next != NULL) {
                                       session->cur_doc = roman->next;
                                       session->cur_doc->cur_sh = session->cur_doc->first_sh;
                                   } else {
                                       session->cur_doc = session->first_doc;
                                       session->cur_doc->cur_sh = session->cur_doc->first_sh;
                                   }
                                   chg_mode('.');
                                   ui_update(TRUE);
                                 }
#line 3906 "y.tab.c"
    break;

  case 74: /* command: S_PREVSHEET  */
#line 893 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   if (roman->cur_sh->prev != NULL) {
                                       roman->cur_sh = roman->cur_sh->prev;
                                   } else if (roman->prev != NULL) {
                                       session->cur_doc = roman->prev;
                                       session->cur_doc->cur_sh = session->cur_doc->last_sh;
                                   } else {
                                       session->cur_doc = session->last_doc;
                                       session->cur_doc->cur_sh = session->cur_doc->last_sh;
                                   }
                                   chg_mode('.');
                                   ui_update(TRUE);
                                 }
#line 3925 "y.tab.c"
    break;

  case 75: /* command: S_MOVETOSHEET STRING  */
#line 908 "gram.y"
                                  {
                                    struct sheet * sh;
                                    if ((sh = search_sheet(session->cur_doc, (yyvsp[0].sval))) != NULL )
                                        session->cur_doc->cur_sh = sh;
                                    scxfree((yyvsp[0].sval));
                                  }
#line 3936 "y.tab.c"
    break;

  case 76: /* command: S_MOVESHEET NUMBER  */
#line 914 "gram.y"
                                   {
                                    struct roman * roman = session->cur_doc;
                                    move_sheet_to_position(roman, (yyvsp[0].ival));
                                    roman->modflg++;
                                    chg_mode('.');
                                    ui_update(TRUE);
                                  }
#line 3948 "y.tab.c"
    break;

  case 77: /* command: S_RENAMESHEET STRING  */
#line 921 "gram.y"
                                  {
                                   struct sheet * sh = session->cur_doc->cur_sh;
                                   if (sh->name != NULL) free(sh->name);
                                   session->cur_doc->modflg++;
                                   sh->name = (yyvsp[0].sval);
                                   chg_mode('.');
                                   ui_show_header();
                                 }
#line 3961 "y.tab.c"
    break;

  case 78: /* command: S_NMAP STRING STRING  */
#line 930 "gram.y"
                                 {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), NORMAL_MODE, 1);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                 }
#line 3971 "y.tab.c"
    break;

  case 79: /* command: S_IMAP STRING STRING  */
#line 936 "gram.y"
                                 {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), INSERT_MODE, 1);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                 }
#line 3981 "y.tab.c"
    break;

  case 80: /* command: S_EMAP STRING STRING  */
#line 941 "gram.y"
                                 {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), EDIT_MODE, 1);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                 }
#line 3991 "y.tab.c"
    break;

  case 81: /* command: S_VMAP STRING STRING  */
#line 946 "gram.y"
                                 {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), VISUAL_MODE, 1);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                 }
#line 4001 "y.tab.c"
    break;

  case 82: /* command: S_CMAP STRING STRING  */
#line 951 "gram.y"
                                 {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), COMMAND_MODE, 1);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                 }
#line 4011 "y.tab.c"
    break;

  case 83: /* command: S_NNOREMAP STRING STRING  */
#line 956 "gram.y"
                                   {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), NORMAL_MODE, 0);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4021 "y.tab.c"
    break;

  case 84: /* command: S_INOREMAP STRING STRING  */
#line 962 "gram.y"
                                   {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), INSERT_MODE, 0);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4031 "y.tab.c"
    break;

  case 85: /* command: S_ENOREMAP STRING STRING  */
#line 967 "gram.y"
                                   {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), EDIT_MODE, 0);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4041 "y.tab.c"
    break;

  case 86: /* command: S_VNOREMAP STRING STRING  */
#line 972 "gram.y"
                                   {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), VISUAL_MODE, 0);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4051 "y.tab.c"
    break;

  case 87: /* command: S_CNOREMAP STRING STRING  */
#line 977 "gram.y"
                                   {
                                   add_map((yyvsp[-1].sval), (yyvsp[0].sval), COMMAND_MODE, 0);
                                   scxfree((yyvsp[-1].sval));
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4061 "y.tab.c"
    break;

  case 88: /* command: S_NUNMAP STRING  */
#line 983 "gram.y"
                                   {
                                   del_map((yyvsp[0].sval), NORMAL_MODE);
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4070 "y.tab.c"
    break;

  case 89: /* command: S_IUNMAP STRING  */
#line 988 "gram.y"
                                   {
                                   del_map((yyvsp[0].sval), INSERT_MODE);
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4079 "y.tab.c"
    break;

  case 90: /* command: S_EUNMAP STRING  */
#line 992 "gram.y"
                                   {
                                   del_map((yyvsp[0].sval), EDIT_MODE);
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4088 "y.tab.c"
    break;

  case 91: /* command: S_VUNMAP STRING  */
#line 996 "gram.y"
                                   {
                                   del_map((yyvsp[0].sval), VISUAL_MODE);
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4097 "y.tab.c"
    break;

  case 92: /* command: S_CUNMAP STRING  */
#line 1001 "gram.y"
                                   {
                                   del_map((yyvsp[0].sval), COMMAND_MODE);
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4106 "y.tab.c"
    break;

  case 93: /* command: S_COLOR STRING  */
#line 1005 "gram.y"
                                   {
#ifdef USECOLORS
                                   chg_color((yyvsp[0].sval));
#endif
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4117 "y.tab.c"
    break;

  case 94: /* command: S_DETAIL var  */
#line 1012 "gram.y"
                                   {
                                   char det[BUFFERSIZE] = "";
                                   struct ent * e = (yyvsp[0].ent).vp;

                                   sprintf(det + strlen(det), "row: %d\n", e->row);
                                   sprintf(det + strlen(det), "col: %d\n", e->col);
                                   sprintf(det + strlen(det), "cellerror: %d\n"   , e->cellerror);
                                   sprintf(det + strlen(det), "flags:\n");
                                   sprintf(det + strlen(det), "is_valid: %d\n"    , e->flags & is_valid );
                                   sprintf(det + strlen(det), "is_deleted: %d\n"  , e->flags & is_deleted);
                                   sprintf(det + strlen(det), "is_changed: %d\n"  , e->flags & is_changed);
                                   sprintf(det + strlen(det), "is_strexpr: %d\n"  , e->flags & is_strexpr);
                                   sprintf(det + strlen(det), "is_leftflush: %d\n", e->flags & is_leftflush);
                                   sprintf(det + strlen(det), "is_locked: %d\n"   , e->flags & is_locked);
                                   sprintf(det + strlen(det), "is_label: %d\n"    , e->flags & is_label);
                                   sprintf(det + strlen(det), "iscleared: %d\n"   , e->flags & iscleared);
                                   sprintf(det + strlen(det), "may_sync: %d\n"    , e->flags & may_sync);
                                   ui_show_text((char *) &det);
                                   }
#line 4141 "y.tab.c"
    break;

  case 95: /* command: S_CELLCOLOR var_or_range STRING  */
#line 1033 "gram.y"
                                   {
#ifdef USECOLORS
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   if ( ! get_conf_int("nocurses"))
                                       color_cell(sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).right.vp->row, (yyvsp[-1].rval).right.vp->col, (yyvsp[0].sval));
#endif
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4155 "y.tab.c"
    break;

  case 96: /* command: S_TRIGGER var_or_range STRING  */
#line 1043 "gram.y"
                                        {
                                          set_trigger(session->cur_doc->cur_sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).right.vp->row, (yyvsp[-1].rval).right.vp->col, (yyvsp[0].sval));
                                          scxfree((yyvsp[0].sval));
                                        }
#line 4164 "y.tab.c"
    break;

  case 97: /* command: S_OFFSCR_SC_COLS NUMBER  */
#line 1048 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->offscr_sc_cols = (yyvsp[0].ival);
                                 }
#line 4174 "y.tab.c"
    break;

  case 98: /* command: S_OFFSCR_SC_ROWS NUMBER  */
#line 1053 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->offscr_sc_rows = (yyvsp[0].ival);
                                 }
#line 4184 "y.tab.c"
    break;

  case 99: /* command: S_NB_FROZEN_ROWS NUMBER  */
#line 1058 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->nb_frozen_rows = (yyvsp[0].ival);
                                 }
#line 4194 "y.tab.c"
    break;

  case 100: /* command: S_NB_FROZEN_COLS NUMBER  */
#line 1063 "gram.y"
                                 {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->nb_frozen_cols = (yyvsp[0].ival);
                                 }
#line 4204 "y.tab.c"
    break;

  case 101: /* command: S_NB_FROZEN_SCREENROWS NUMBER  */
#line 1068 "gram.y"
                                       {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->nb_frozen_screenrows = (yyvsp[0].ival);
                                 }
#line 4214 "y.tab.c"
    break;

  case 102: /* command: S_NB_FROZEN_SCREENCOLS NUMBER  */
#line 1073 "gram.y"
                                       {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
                                   sh->nb_frozen_screencols = (yyvsp[0].ival);
                                 }
#line 4224 "y.tab.c"
    break;

  case 103: /* command: S_UNTRIGGER var_or_range  */
#line 1079 "gram.y"
                                   {
                                   del_trigger(session->cur_doc->cur_sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col);
                                   }
#line 4232 "y.tab.c"
    break;

  case 104: /* command: S_CELLCOLOR STRING  */
#line 1083 "gram.y"
                                   {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
#ifdef USECOLORS
                                   if ( ! get_conf_int("nocurses"))
                                       color_cell(sh, sh->currow, sh->curcol, sh->currow, sh->curcol, (yyvsp[0].sval));
#endif
                                   scxfree((yyvsp[0].sval));
                                   }
#line 4246 "y.tab.c"
    break;

  case 105: /* command: S_UNFORMAT var_or_range  */
#line 1093 "gram.y"
                                   {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
#ifdef USECOLORS
                                   if ( ! get_conf_int("nocurses")) unformat(sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col);
#endif
                                   }
#line 4258 "y.tab.c"
    break;

  case 106: /* command: S_UNFORMAT  */
#line 1101 "gram.y"
                                   {
                                   struct roman * roman = session->cur_doc;
                                   struct sheet * sh = roman->cur_sh;
#ifdef USECOLORS
                                   if ( ! get_conf_int("nocurses")) unformat(sh, sh->currow, sh->curcol, sh->currow, sh->curcol);
#endif
                                   }
#line 4270 "y.tab.c"
    break;

  case 107: /* command: S_REDEFINE_COLOR STRING NUMBER NUMBER NUMBER  */
#line 1109 "gram.y"
                                                      {
                                         redefine_color((yyvsp[-3].sval), (yyvsp[-2].ival), (yyvsp[-1].ival), (yyvsp[0].ival));
                                         scxfree((yyvsp[-3].sval)); }
#line 4278 "y.tab.c"
    break;

  case 108: /* command: S_DEFINE_COLOR STRING NUMBER NUMBER NUMBER  */
#line 1113 "gram.y"
                                                    {
                                         define_color((yyvsp[-3].sval), (yyvsp[-2].ival), (yyvsp[-1].ival), (yyvsp[0].ival));
                                         scxfree((yyvsp[-3].sval)); }
#line 4286 "y.tab.c"
    break;

  case 109: /* command: S_FCOPY  */
#line 1117 "gram.y"
                                   { fcopy(session->cur_doc->cur_sh, ""); }
#line 4292 "y.tab.c"
    break;

  case 110: /* command: S_FCOPY strarg  */
#line 1118 "gram.y"
                                   { fcopy(session->cur_doc->cur_sh, (yyvsp[0].sval)); }
#line 4298 "y.tab.c"
    break;

  case 111: /* command: S_FSUM  */
#line 1119 "gram.y"
                                   { fsum(session->cur_doc->cur_sh);  }
#line 4304 "y.tab.c"
    break;

  case 112: /* command: S_PLOT STRING var_or_range  */
#line 1121 "gram.y"
                                          {
                                     plot((yyvsp[-1].sval), (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col);
                                     scxfree((yyvsp[-1].sval));
                                   }
#line 4313 "y.tab.c"
    break;

  case 113: /* command: S_SET setlist  */
#line 1129 "gram.y"
                                   { //if (! loading) sc_debug("INT: Config value changed");
                                   }
#line 4320 "y.tab.c"
    break;

  case 114: /* command: S_DEFINE strarg range  */
#line 1145 "gram.y"
                                   { add_range((yyvsp[-1].sval), (yyvsp[0].rval).left, (yyvsp[0].rval).right, 1); }
#line 4326 "y.tab.c"
    break;

  case 115: /* command: S_DEFINE strarg var  */
#line 1146 "gram.y"
                                   { add_range((yyvsp[-1].sval), (yyvsp[0].ent), (yyvsp[0].ent), 0); }
#line 4332 "y.tab.c"
    break;

  case 116: /* command: S_UNDEFINE var_or_range  */
#line 1148 "gram.y"
                                   { del_range((yyvsp[0].rval).left.vp, (yyvsp[0].rval).right.vp); }
#line 4338 "y.tab.c"
    break;

  case 117: /* command: S_EVAL e  */
#line 1150 "gram.y"
                                   {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh = roman->cur_sh;
                                     eval_result = eval(sh, NULL, (yyvsp[0].enode), 0);
                                     efree((yyvsp[0].enode));
                                   }
#line 4349 "y.tab.c"
    break;

  case 118: /* command: S_EXECUTE STRING  */
#line 1156 "gram.y"
                                   {

                                     inputline[0]=L'\0';

                                     #ifdef HISTORY_FILE
                                            commandline_history = (struct history *) create_history(':');
                                            load_history(commandline_history, ':'); // load the command history file
                                     #endif
                                     #ifdef INS_HISTORY_FILE
                                            insert_history = (struct history *) create_history('=');
                                            load_history(insert_history, '='); // load the insert history file
                                     #endif

                                     (void) swprintf(inputline, BUFFERSIZE, L"%s", (yyvsp[0].sval));

                                     struct block * auxb = (struct block *) create_buf();
                                     addto_buf(auxb, OKEY_ENTER);
                                     do_commandmode(auxb);
                                     flush_buf(auxb);
                                     erase_buf(auxb);
                                     auxb = NULL;
                                     inputline[0]=L'\0';
                                     scxfree((yyvsp[0].sval));
                                   }
#line 4378 "y.tab.c"
    break;

  case 119: /* command: S_EXPORT STRING STRING  */
#line 1180 "gram.y"
                                   {
                                     struct roman * roman = session->cur_doc;
                                     swprintf(inputline, BUFFERSIZE, L"e! %s %s", (yyvsp[-1].sval), (yyvsp[0].sval));
                                     do_export(0, 0, roman->cur_sh->maxrow, roman->cur_sh->maxcol);
                                     scxfree((yyvsp[-1].sval));
                                     scxfree((yyvsp[0].sval));
                                   }
#line 4390 "y.tab.c"
    break;

  case 120: /* command: S_QUIT  */
#line 1187 "gram.y"
                                   {
                                     printf("quitting. unsaved changes will be lost.\n");
                                     shall_quit = 2; // unsaved changes are lost!
                                   }
#line 4399 "y.tab.c"
    break;

  case 121: /* command: S_REBUILD_GRAPH  */
#line 1191 "gram.y"
                                   {
                                     rebuild_graph();
                                     ui_update(FALSE);
                                   }
#line 4408 "y.tab.c"
    break;

  case 122: /* command: S_PRINT_GRAPH  */
#line 1196 "gram.y"
                                   { print_vertexs(); }
#line 4414 "y.tab.c"
    break;

  case 123: /* command: S_SYNCREFS  */
#line 1197 "gram.y"
                                   { sync_refs(session->cur_doc->cur_sh); }
#line 4420 "y.tab.c"
    break;

  case 124: /* command: S_UNDO  */
#line 1199 "gram.y"
                                   {
#ifdef UNDO
                                     do_undo();
                                     // sync_refs(session->cur_doc->cur_sh);
                                     EvalAll();
                                     ui_update(TRUE);
#endif
                                   }
#line 4433 "y.tab.c"
    break;

  case 125: /* command: S_REDO  */
#line 1208 "gram.y"
                                   {
#ifdef UNDO
                                     do_redo();
                                     // sync_refs(session->cur_doc->cur_sh);
                                     EvalAll();
                                     ui_update(TRUE);
#endif
                                   }
#line 4446 "y.tab.c"
    break;

  case 126: /* command: S_RECALC  */
#line 1218 "gram.y"
                                   {
                                     EvalAll();
                                     //ui_update(1);
                                     //changed = 0;
                                   }
#line 4456 "y.tab.c"
    break;

  case 127: /* command: S_GETNUM var_or_range  */
#line 1223 "gram.y"
                                   {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh = roman->cur_sh;
                                     getnum(sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, fdoutput);
                                   }
#line 4466 "y.tab.c"
    break;

  case 128: /* command: S_GETNUM '{' STRING '}' '!' var_or_range  */
#line 1229 "gram.y"
                                                  {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh;
                                     if ((sh = search_sheet(roman, (yyvsp[-3].sval))) == NULL )
                                         sh = roman->cur_sh;
                                     getnum(sh, (yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, fdoutput);
                                     scxfree((yyvsp[-3].sval));
                                   }
#line 4479 "y.tab.c"
    break;

  case 129: /* command: S_GETSTRING var_or_range  */
#line 1237 "gram.y"
                                   { getstring((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, fdoutput); }
#line 4485 "y.tab.c"
    break;

  case 130: /* command: S_GETEXP var_or_range  */
#line 1239 "gram.y"
                                   { getexp((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, fdoutput); }
#line 4491 "y.tab.c"
    break;

  case 131: /* command: S_GETFMT var_or_range  */
#line 1242 "gram.y"
                                   { getfmt((yyvsp[0].rval).left.vp->row, (yyvsp[0].rval).left.vp->col, (yyvsp[0].rval).right.vp->row, (yyvsp[0].rval).right.vp->col, fdoutput); }
#line 4497 "y.tab.c"
    break;

  case 132: /* command: S_YANKAREA '{' STRING '}' '!' var_or_range STRING  */
#line 1244 "gram.y"
                                                           {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh;
                                     if ((sh = search_sheet(roman, (yyvsp[-4].sval))) == NULL )
                                         sh = roman->cur_sh;
                                     yank_area(sh, (yyvsp[-1].rval).left.vp->row, (yyvsp[-1].rval).left.vp->col, (yyvsp[-1].rval).right.vp->row, (yyvsp[-1].rval).right.vp->col, (yyvsp[0].sval)[0], 1);
                                     scxfree((yyvsp[-4].sval));
                                     scxfree((yyvsp[0].sval));
                                   }
#line 4511 "y.tab.c"
    break;

  case 133: /* command: S_PASTEYANKED '{' STRING '}' NUMBER STRING  */
#line 1253 "gram.y"
                                                    {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh;
                                     if ((sh = search_sheet(roman, (yyvsp[-3].sval))) == NULL )
                                         sh = roman->cur_sh;
                                     paste_yanked_ents(sh, (yyvsp[-1].ival), (yyvsp[0].sval)[0]);
                                     scxfree((yyvsp[-3].sval));
                                     scxfree((yyvsp[0].sval));
                                   }
#line 4525 "y.tab.c"
    break;

  case 134: /* command: S_SEVAL e  */
#line 1262 "gram.y"
                                   {
                                     struct roman * roman = session->cur_doc;
                                     struct sheet * sh = roman->cur_sh;
                                     seval_result = seval(sh, NULL, (yyvsp[0].enode), 0); // always make sure this seval_result is always freed afterwards
                                     efree((yyvsp[0].enode));
                                   }
#line 4536 "y.tab.c"
    break;

  case 135: /* command: S_ERROR STRING  */
#line 1268 "gram.y"
                                   { sc_error((yyvsp[0].sval));
                                     //free $2
                                   }
#line 4544 "y.tab.c"
    break;

  case 137: /* command: error  */
#line 1273 "gram.y"
                 {
                     sc_error("syntax error: %s", line);
                     YYABORT;

                     //linelim = 0;
                     //yyparse();

                     //line[0]='\0';
                     //linelim = -1;
                     //yyclearin;
                     //yyerrok;
                 }
#line 4561 "y.tab.c"
    break;

  case 138: /* term: var  */
#line 1286 "gram.y"
                                  {
                                    if ((yyvsp[0].ent).vf & GET_ENT)
                                        (yyval.enode) = (yyvsp[0].ent).expr;
                                    else {
                                       (yyval.enode) = new_var(O_VAR, (yyvsp[0].ent));
                                       (yyval.enode)->e.r.left.expr = NULL;
                                       (yyval.enode)->e.r.right.expr = NULL;
                                    }
                                  }
#line 4575 "y.tab.c"
    break;

  case 139: /* term: '@' K_FIXED term  */
#line 1317 "gram.y"
                                  { (yyval.enode) = new('f', (yyvsp[0].enode), ENULL); }
#line 4581 "y.tab.c"
    break;

  case 140: /* term: '(' '@' K_FIXED ')' term  */
#line 1320 "gram.y"
                                  { (yyval.enode) = new('F', (yyvsp[0].enode), ENULL); }
#line 4587 "y.tab.c"
    break;

  case 141: /* term: '@' K_SUM '(' var_or_range ')'  */
#line 1323 "gram.y"
                                  { (yyval.enode) = new(SUM, new_range(REDUCE | SUM, (yyvsp[-1].rval)), ENULL); }
#line 4593 "y.tab.c"
    break;

  case 142: /* term: '@' K_SUM '(' range ',' e ')'  */
#line 1327 "gram.y"
                                  { (yyval.enode) = new(SUM, new_range(REDUCE | SUM, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 4599 "y.tab.c"
    break;

  case 143: /* term: '@' K_PROD '(' var_or_range ')'  */
#line 1329 "gram.y"
                                  { (yyval.enode) = new(PROD, new_range(REDUCE | PROD, (yyvsp[-1].rval)), ENULL); }
#line 4605 "y.tab.c"
    break;

  case 144: /* term: '@' K_PROD '(' range ',' e ')'  */
#line 1331 "gram.y"
                                  { (yyval.enode) = new(PROD, new_range(REDUCE | PROD, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 4611 "y.tab.c"
    break;

  case 145: /* term: '@' K_AVG '(' var_or_range ')'  */
#line 1333 "gram.y"
                                  { (yyval.enode) = new(AVG, new_range(REDUCE | AVG, (yyvsp[-1].rval)), ENULL); }
#line 4617 "y.tab.c"
    break;

  case 146: /* term: '@' K_AVG '(' range ',' e ')'  */
#line 1335 "gram.y"
                                  { (yyval.enode) = new(AVG, new_range(REDUCE | AVG, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 4623 "y.tab.c"
    break;

  case 147: /* term: '@' K_STDDEV '(' var_or_range ')'  */
#line 1337 "gram.y"
                                  { (yyval.enode) = new(STDDEV, new_range(REDUCE | STDDEV, (yyvsp[-1].rval)), ENULL); }
#line 4629 "y.tab.c"
    break;

  case 148: /* term: '@' K_STDDEV '(' range ',' e ')'  */
#line 1339 "gram.y"
                                  { (yyval.enode) = new(STDDEV, new_range(REDUCE | STDDEV, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 4635 "y.tab.c"
    break;

  case 149: /* term: '@' K_COUNT '(' var_or_range ')'  */
#line 1341 "gram.y"
                                  { (yyval.enode) = new(COUNT, new_range(REDUCE | COUNT, (yyvsp[-1].rval)), ENULL); }
#line 4641 "y.tab.c"
    break;

  case 150: /* term: '@' K_COUNT '(' range ',' e ')'  */
#line 1343 "gram.y"
                                  { (yyval.enode) = new(COUNT, new_range(REDUCE | COUNT, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 4647 "y.tab.c"
    break;

  case 151: /* term: '@' K_MAX '(' var_or_range ')'  */
#line 1345 "gram.y"
                                  { (yyval.enode) = new(MAX, new_range(REDUCE | MAX, (yyvsp[-1].rval)), ENULL); }
#line 4653 "y.tab.c"
    break;

  case 152: /* term: '@' K_MAX '(' range ',' e ')'  */
#line 1347 "gram.y"
                                  { (yyval.enode) = new(MAX, new_range(REDUCE | MAX, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 4659 "y.tab.c"
    break;

  case 153: /* term: '@' K_MAX '(' e ',' expr_list ')'  */
#line 1349 "gram.y"
                                  { (yyval.enode) = new(LMAX, (yyvsp[-1].enode), (yyvsp[-3].enode)); }
#line 4665 "y.tab.c"
    break;

  case 154: /* term: '@' K_MIN '(' var_or_range ')'  */
#line 1351 "gram.y"
                                  { (yyval.enode) = new(MIN, new_range(REDUCE | MIN, (yyvsp[-1].rval)), ENULL); }
#line 4671 "y.tab.c"
    break;

  case 155: /* term: '@' K_MIN '(' range ',' e ')'  */
#line 1353 "gram.y"
                                  { (yyval.enode) = new(MIN, new_range(REDUCE | MIN, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 4677 "y.tab.c"
    break;

  case 156: /* term: '@' K_MIN '(' e ',' expr_list ')'  */
#line 1355 "gram.y"
                                  { (yyval.enode) = new(LMIN, (yyvsp[-1].enode), (yyvsp[-3].enode)); }
#line 4683 "y.tab.c"
    break;

  case 157: /* term: '@' K_ROWS '(' var_or_range ')'  */
#line 1357 "gram.y"
                                  { (yyval.enode) = new_range(REDUCE | 'R', (yyvsp[-1].rval)); }
#line 4689 "y.tab.c"
    break;

  case 158: /* term: '@' K_COLS '(' var_or_range ')'  */
#line 1359 "gram.y"
                                  { (yyval.enode) = new_range(REDUCE | 'C', (yyvsp[-1].rval)); }
#line 4695 "y.tab.c"
    break;

  case 159: /* term: '@' K_ABS '(' e ')'  */
#line 1360 "gram.y"
                                  { (yyval.enode) = new(ABS, (yyvsp[-1].enode), ENULL); }
#line 4701 "y.tab.c"
    break;

  case 160: /* term: '@' K_FROW '(' e ')'  */
#line 1361 "gram.y"
                                  { (yyval.enode) = new(FROW, (yyvsp[-1].enode), ENULL); }
#line 4707 "y.tab.c"
    break;

  case 161: /* term: '@' K_FCOL '(' e ')'  */
#line 1362 "gram.y"
                                  { (yyval.enode) = new(FCOL, (yyvsp[-1].enode), ENULL); }
#line 4713 "y.tab.c"
    break;

  case 162: /* term: '@' K_ACOS '(' e ')'  */
#line 1363 "gram.y"
                                  { (yyval.enode) = new(ACOS, (yyvsp[-1].enode), ENULL); }
#line 4719 "y.tab.c"
    break;

  case 163: /* term: '@' K_ASIN '(' e ')'  */
#line 1364 "gram.y"
                                  { (yyval.enode) = new(ASIN, (yyvsp[-1].enode), ENULL); }
#line 4725 "y.tab.c"
    break;

  case 164: /* term: '@' K_ATAN '(' e ')'  */
#line 1365 "gram.y"
                                  { (yyval.enode) = new(ATAN, (yyvsp[-1].enode), ENULL); }
#line 4731 "y.tab.c"
    break;

  case 165: /* term: '@' K_ATAN2 '(' e ',' e ')'  */
#line 1367 "gram.y"
                                  { (yyval.enode) = new(ATAN2, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 4737 "y.tab.c"
    break;

  case 166: /* term: '@' K_CEIL '(' e ')'  */
#line 1368 "gram.y"
                                  { (yyval.enode) = new(CEIL, (yyvsp[-1].enode), ENULL); }
#line 4743 "y.tab.c"
    break;

  case 167: /* term: '@' K_COS '(' e ')'  */
#line 1369 "gram.y"
                                  { (yyval.enode) = new(COS, (yyvsp[-1].enode), ENULL); }
#line 4749 "y.tab.c"
    break;

  case 168: /* term: '@' K_EXP '(' e ')'  */
#line 1370 "gram.y"
                                  { (yyval.enode) = new(EXP, (yyvsp[-1].enode), ENULL); }
#line 4755 "y.tab.c"
    break;

  case 169: /* term: '@' K_FABS '(' e ')'  */
#line 1371 "gram.y"
                                  { (yyval.enode) = new(FABS, (yyvsp[-1].enode), ENULL); }
#line 4761 "y.tab.c"
    break;

  case 170: /* term: '@' K_FLOOR '(' e ')'  */
#line 1372 "gram.y"
                                  { (yyval.enode) = new(FLOOR, (yyvsp[-1].enode), ENULL); }
#line 4767 "y.tab.c"
    break;

  case 171: /* term: '@' K_HYPOT '(' e ',' e ')'  */
#line 1374 "gram.y"
                                  { (yyval.enode) = new(HYPOT, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 4773 "y.tab.c"
    break;

  case 172: /* term: '@' K_LN '(' e ')'  */
#line 1375 "gram.y"
                                  { (yyval.enode) = new(LOG, (yyvsp[-1].enode), ENULL); }
#line 4779 "y.tab.c"
    break;

  case 173: /* term: '@' K_LOG '(' e ')'  */
#line 1376 "gram.y"
                                  { (yyval.enode) = new(LOG10, (yyvsp[-1].enode), ENULL); }
#line 4785 "y.tab.c"
    break;

  case 174: /* term: '@' K_POW '(' e ',' e ')'  */
#line 1378 "gram.y"
                                  { (yyval.enode) = new(POW, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 4791 "y.tab.c"
    break;

  case 175: /* term: '@' K_SIN '(' e ')'  */
#line 1379 "gram.y"
                                  { (yyval.enode) = new(SIN, (yyvsp[-1].enode), ENULL); }
#line 4797 "y.tab.c"
    break;

  case 176: /* term: '@' K_SQRT '(' e ')'  */
#line 1380 "gram.y"
                                  { (yyval.enode) = new(SQRT, (yyvsp[-1].enode), ENULL); }
#line 4803 "y.tab.c"
    break;

  case 177: /* term: '@' K_TAN '(' e ')'  */
#line 1381 "gram.y"
                                  { (yyval.enode) = new(TAN, (yyvsp[-1].enode), ENULL); }
#line 4809 "y.tab.c"
    break;

  case 178: /* term: '@' K_DTR '(' e ')'  */
#line 1382 "gram.y"
                                  { (yyval.enode) = new(DTR, (yyvsp[-1].enode), ENULL); }
#line 4815 "y.tab.c"
    break;

  case 179: /* term: '@' K_RTD '(' e ')'  */
#line 1383 "gram.y"
                                  { (yyval.enode) = new(RTD, (yyvsp[-1].enode), ENULL); }
#line 4821 "y.tab.c"
    break;

  case 180: /* term: '@' K_RND '(' e ')'  */
#line 1384 "gram.y"
                                  { (yyval.enode) = new(RND, (yyvsp[-1].enode), ENULL); }
#line 4827 "y.tab.c"
    break;

  case 181: /* term: '@' K_ROUND '(' e ',' e ')'  */
#line 1386 "gram.y"
                                  { (yyval.enode) = new(ROUND, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 4833 "y.tab.c"
    break;

  case 182: /* term: '@' K_IF '(' e ',' e ',' e ')'  */
#line 1388 "gram.y"
                                  { (yyval.enode) = new(IF,  (yyvsp[-5].enode),new(',',(yyvsp[-3].enode),(yyvsp[-1].enode))); }
#line 4839 "y.tab.c"
    break;

  case 183: /* term: '@' K_PV '(' e ',' e ',' e ')'  */
#line 1390 "gram.y"
                                  { (yyval.enode) = new(PV,  (yyvsp[-5].enode),new(':',(yyvsp[-3].enode),(yyvsp[-1].enode))); }
#line 4845 "y.tab.c"
    break;

  case 184: /* term: '@' K_FV '(' e ',' e ',' e ')'  */
#line 1392 "gram.y"
                                  { (yyval.enode) = new(FV,  (yyvsp[-5].enode),new(':',(yyvsp[-3].enode),(yyvsp[-1].enode))); }
#line 4851 "y.tab.c"
    break;

  case 185: /* term: '@' K_PMT '(' e ',' e ',' e ')'  */
#line 1394 "gram.y"
                                  { (yyval.enode) = new(PMT, (yyvsp[-5].enode),new(':',(yyvsp[-3].enode),(yyvsp[-1].enode))); }
#line 4857 "y.tab.c"
    break;

  case 186: /* term: '@' K_HOUR '(' e ')'  */
#line 1395 "gram.y"
                                  { (yyval.enode) = new(HOUR, (yyvsp[-1].enode), ENULL); }
#line 4863 "y.tab.c"
    break;

  case 187: /* term: '@' K_MINUTE '(' e ')'  */
#line 1396 "gram.y"
                                  { (yyval.enode) = new(MINUTE, (yyvsp[-1].enode), ENULL); }
#line 4869 "y.tab.c"
    break;

  case 188: /* term: '@' K_SECOND '(' e ')'  */
#line 1397 "gram.y"
                                  { (yyval.enode) = new(SECOND, (yyvsp[-1].enode), ENULL); }
#line 4875 "y.tab.c"
    break;

  case 189: /* term: '@' K_MONTH '(' e ')'  */
#line 1398 "gram.y"
                                  { (yyval.enode) = new(MONTH, (yyvsp[-1].enode), ENULL); }
#line 4881 "y.tab.c"
    break;

  case 190: /* term: '@' K_DAY '(' e ')'  */
#line 1399 "gram.y"
                                  { (yyval.enode) = new(DAY, (yyvsp[-1].enode), ENULL); }
#line 4887 "y.tab.c"
    break;

  case 191: /* term: '@' K_YEAR '(' e ')'  */
#line 1400 "gram.y"
                                  { (yyval.enode) = new(YEAR, (yyvsp[-1].enode), ENULL); }
#line 4893 "y.tab.c"
    break;

  case 192: /* term: '@' K_NOW  */
#line 1401 "gram.y"
                                  { (yyval.enode) = new(NOW, ENULL, ENULL);}
#line 4899 "y.tab.c"
    break;

  case 193: /* term: '@' K_DTS '(' e ',' e ',' e ')'  */
#line 1403 "gram.y"
                                  { (yyval.enode) = new(DTS, (yyvsp[-5].enode), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode)));}
#line 4905 "y.tab.c"
    break;

  case 194: /* term: NUMBER '.' NUMBER '.' NUMBER  */
#line 1405 "gram.y"
                                  { (yyval.enode) = new(DTS, new_const(O_CONST, (double) (yyvsp[-4].ival)),
                                         new(',', new_const(O_CONST, (double) (yyvsp[-2].ival)),
                                         new_const(O_CONST, (double) (yyvsp[0].ival))));}
#line 4913 "y.tab.c"
    break;

  case 195: /* term: '@' K_TTS '(' e ',' e ',' e ')'  */
#line 1409 "gram.y"
                                  { (yyval.enode) = new(TTS, (yyvsp[-5].enode), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode)));}
#line 4919 "y.tab.c"
    break;

  case 196: /* term: '@' K_STON '(' e ')'  */
#line 1410 "gram.y"
                                  { (yyval.enode) = new(STON, (yyvsp[-1].enode), ENULL); }
#line 4925 "y.tab.c"
    break;

  case 197: /* term: '@' K_SLEN '(' e ')'  */
#line 1411 "gram.y"
                                  { (yyval.enode) = new(SLEN, (yyvsp[-1].enode), ENULL); }
#line 4931 "y.tab.c"
    break;

  case 198: /* term: '@' K_EQS '(' e ',' e ')'  */
#line 1413 "gram.y"
                                  { (yyval.enode) = new(EQS, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 4937 "y.tab.c"
    break;

  case 199: /* term: '@' K_DATE '(' e ')'  */
#line 1414 "gram.y"
                                  { (yyval.enode) = new(DATE, (yyvsp[-1].enode), ENULL); }
#line 4943 "y.tab.c"
    break;

  case 200: /* term: '@' K_DATE '(' e ',' e ')'  */
#line 1416 "gram.y"
                                  { (yyval.enode) = new(DATE, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 4949 "y.tab.c"
    break;

  case 201: /* term: '@' K_FMT '(' e ',' e ')'  */
#line 1418 "gram.y"
                                  { (yyval.enode) = new(FMT, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 4955 "y.tab.c"
    break;

  case 202: /* term: '@' K_UPPER '(' e ')'  */
#line 1419 "gram.y"
                                  { (yyval.enode) = new(UPPER, (yyvsp[-1].enode), ENULL); }
#line 4961 "y.tab.c"
    break;

  case 203: /* term: '@' K_LOWER '(' e ')'  */
#line 1420 "gram.y"
                                  { (yyval.enode) = new(LOWER, (yyvsp[-1].enode), ENULL); }
#line 4967 "y.tab.c"
    break;

  case 204: /* term: '@' K_CAPITAL '(' e ')'  */
#line 1421 "gram.y"
                                  { (yyval.enode) = new(CAPITAL, (yyvsp[-1].enode), ENULL); }
#line 4973 "y.tab.c"
    break;

  case 205: /* term: '@' K_INDEX '(' range ',' e ')'  */
#line 1423 "gram.y"
                                  { (yyval.enode) = new(INDEX, new_range(REDUCE | INDEX, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 4979 "y.tab.c"
    break;

  case 206: /* term: '@' K_INDEX '(' e ',' range ')'  */
#line 1425 "gram.y"
                                  { (yyval.enode) = new(INDEX, new_range(REDUCE | INDEX, (yyvsp[-1].rval)), (yyvsp[-3].enode)); }
#line 4985 "y.tab.c"
    break;

  case 207: /* term: '@' K_INDEX '(' range ',' e ',' e ')'  */
#line 1427 "gram.y"
                                  { (yyval.enode) = new(INDEX, new_range(REDUCE | INDEX, (yyvsp[-5].rval)), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 4991 "y.tab.c"
    break;

  case 208: /* term: '@' K_LOOKUP '(' range ',' e ')'  */
#line 1429 "gram.y"
                                  { (yyval.enode) = new(LOOKUP, new_range(REDUCE | LOOKUP, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 4997 "y.tab.c"
    break;

  case 209: /* term: '@' K_LOOKUP '(' e ',' range ')'  */
#line 1431 "gram.y"
                                  { (yyval.enode) = new(LOOKUP, new_range(REDUCE | LOOKUP, (yyvsp[-1].rval)), (yyvsp[-3].enode)); }
#line 5003 "y.tab.c"
    break;

  case 210: /* term: '@' K_HLOOKUP '(' range ',' e ',' e ')'  */
#line 1433 "gram.y"
                                  { (yyval.enode) = new(HLOOKUP, new_range(REDUCE | HLOOKUP, (yyvsp[-5].rval)), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 5009 "y.tab.c"
    break;

  case 211: /* term: '@' K_HLOOKUP '(' e ',' range ',' e ')'  */
#line 1435 "gram.y"
                                  { (yyval.enode) = new(HLOOKUP, new_range(REDUCE | HLOOKUP, (yyvsp[-3].rval)), new(',', (yyvsp[-5].enode), (yyvsp[-1].enode))); }
#line 5015 "y.tab.c"
    break;

  case 212: /* term: '@' K_VLOOKUP '(' range ',' e ',' e ')'  */
#line 1437 "gram.y"
                                  { (yyval.enode) = new(VLOOKUP, new_range(REDUCE | VLOOKUP, (yyvsp[-5].rval)), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 5021 "y.tab.c"
    break;

  case 213: /* term: '@' K_VLOOKUP '(' e ',' range ',' e ')'  */
#line 1439 "gram.y"
                                  { (yyval.enode) = new(VLOOKUP, new_range(REDUCE | VLOOKUP, (yyvsp[-3].rval)), new(',', (yyvsp[-5].enode), (yyvsp[-1].enode))); }
#line 5027 "y.tab.c"
    break;

  case 214: /* term: '@' K_STINDEX '(' range ',' e ')'  */
#line 1441 "gram.y"
                                  { (yyval.enode) = new(STINDEX, new_range(REDUCE | STINDEX, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 5033 "y.tab.c"
    break;

  case 215: /* term: '@' K_STINDEX '(' e ',' range ')'  */
#line 1443 "gram.y"
                                  { (yyval.enode) = new(STINDEX, new_range(REDUCE | STINDEX, (yyvsp[-1].rval)), (yyvsp[-3].enode)); }
#line 5039 "y.tab.c"
    break;

  case 216: /* term: '@' K_STINDEX '(' range ',' e ',' e ')'  */
#line 1445 "gram.y"
                                  { (yyval.enode) = new(STINDEX, new_range(REDUCE | STINDEX, (yyvsp[-5].rval)), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 5045 "y.tab.c"
    break;

  case 217: /* term: '@' K_EXT '(' e ',' e ')'  */
#line 1447 "gram.y"
                                  { (yyval.enode) = new(EXT, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 5051 "y.tab.c"
    break;

  case 218: /* term: '@' K_LUA '(' e ',' e ')'  */
#line 1449 "gram.y"
                                  {
                                  #ifdef XLUA
                                  (yyval.enode) = new(LUA, (yyvsp[-3].enode), (yyvsp[-1].enode));
                                  #else
                                  sc_error("@lua: SC-IM was not compiled with LUA support");
                                  (yyval.enode) = new(ERR_, ENULL, ENULL);
                                  #endif
                                  }
#line 5064 "y.tab.c"
    break;

  case 219: /* term: '@' K_MYROW  */
#line 1457 "gram.y"
                                  { (yyval.enode) = new(MYROW, ENULL, ENULL);}
#line 5070 "y.tab.c"
    break;

  case 220: /* term: '@' K_COLORIZE '(' var_or_range ',' e ')'  */
#line 1459 "gram.y"
                                  { (yyval.enode) = new(COLORIZE, new_range(REDUCE | COLORIZE, (yyvsp[-3].rval)), (yyvsp[-1].enode)); }
#line 5076 "y.tab.c"
    break;

  case 221: /* term: '@' K_MYCOL  */
#line 1460 "gram.y"
                                  { (yyval.enode) = new(MYCOL, ENULL, ENULL);}
#line 5082 "y.tab.c"
    break;

  case 222: /* term: '@' K_LASTROW  */
#line 1461 "gram.y"
                                  { (yyval.enode) = new(LASTROW, ENULL, ENULL);}
#line 5088 "y.tab.c"
    break;

  case 223: /* term: '@' K_LASTCOL  */
#line 1462 "gram.y"
                                  { (yyval.enode) = new(LASTCOL, ENULL, ENULL);}
#line 5094 "y.tab.c"
    break;

  case 224: /* term: '@' K_NVAL '(' e ',' e ')'  */
#line 1464 "gram.y"
                                  { (yyval.enode) = new(NVAL, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 5100 "y.tab.c"
    break;

  case 225: /* term: '@' K_SVAL '(' e ',' e ')'  */
#line 1466 "gram.y"
                                  { (yyval.enode) = new(SVAL, (yyvsp[-3].enode), (yyvsp[-1].enode)); }
#line 5106 "y.tab.c"
    break;

  case 226: /* term: '@' K_REPLACE '(' e ',' e ',' e ')'  */
#line 1468 "gram.y"
                                  { (yyval.enode) = new(REPLACE, (yyvsp[-5].enode), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 5112 "y.tab.c"
    break;

  case 227: /* term: '@' K_EVALUATE '(' e ')'  */
#line 1470 "gram.y"
                                    { (yyval.enode) = new(EVALUATE, (yyvsp[-1].enode), ENULL); }
#line 5118 "y.tab.c"
    break;

  case 228: /* term: '@' K_SEVALUATE '(' e ')'  */
#line 1471 "gram.y"
                                    { (yyval.enode) = new(SEVALUATE, (yyvsp[-1].enode), ENULL); }
#line 5124 "y.tab.c"
    break;

  case 229: /* term: '@' K_SUBSTR '(' e ',' e ',' e ')'  */
#line 1473 "gram.y"
                                  { (yyval.enode) = new(SUBSTR, (yyvsp[-5].enode), new(',', (yyvsp[-3].enode), (yyvsp[-1].enode))); }
#line 5130 "y.tab.c"
    break;

  case 230: /* term: '(' e ')'  */
#line 1474 "gram.y"
                                  { (yyval.enode) = (yyvsp[-1].enode); }
#line 5136 "y.tab.c"
    break;

  case 231: /* term: '+' term  */
#line 1475 "gram.y"
                                  { (yyval.enode) = (yyvsp[0].enode); }
#line 5142 "y.tab.c"
    break;

  case 232: /* term: NUMBER  */
#line 1477 "gram.y"
                                  { (yyval.enode) = new_const(O_CONST, (double) (yyvsp[0].ival)); }
#line 5148 "y.tab.c"
    break;

  case 233: /* term: FNUMBER  */
#line 1478 "gram.y"
                                  { (yyval.enode) = new_const(O_CONST, (yyvsp[0].fval)); }
#line 5154 "y.tab.c"
    break;

  case 234: /* term: '@' K_PI  */
#line 1479 "gram.y"
                                  { (yyval.enode) = new(PI_, ENULL, ENULL); }
#line 5160 "y.tab.c"
    break;

  case 235: /* term: STRING  */
#line 1480 "gram.y"
                                  { (yyval.enode) = new_str((yyvsp[0].sval)); }
#line 5166 "y.tab.c"
    break;

  case 236: /* term: '~' term  */
#line 1481 "gram.y"
                                  { (yyval.enode) = new('!', (yyvsp[0].enode), ENULL); }
#line 5172 "y.tab.c"
    break;

  case 237: /* term: '!' term  */
#line 1482 "gram.y"
                                  { (yyval.enode) = new('!', (yyvsp[0].enode), ENULL); }
#line 5178 "y.tab.c"
    break;

  case 238: /* term: '@' K_FILENAME '(' e ')'  */
#line 1484 "gram.y"
                                  { (yyval.enode) = new(FILENAME, (yyvsp[-1].enode), ENULL); }
#line 5184 "y.tab.c"
    break;

  case 239: /* term: '@' K_COLTOA '(' e ')'  */
#line 1485 "gram.y"
                                  { (yyval.enode) = new(COLTOA, (yyvsp[-1].enode), ENULL);}
#line 5190 "y.tab.c"
    break;

  case 240: /* term: '@' K_ASCII '(' e ')'  */
#line 1486 "gram.y"
                                  { (yyval.enode) = new(ASCII, (yyvsp[-1].enode), ENULL); }
#line 5196 "y.tab.c"
    break;

  case 241: /* term: '@' K_SET8BIT '(' e ')'  */
#line 1487 "gram.y"
                                  { (yyval.enode) = new(SET8BIT, (yyvsp[-1].enode), ENULL); }
#line 5202 "y.tab.c"
    break;

  case 242: /* term: '@' K_CHR '(' e ')'  */
#line 1488 "gram.y"
                                  { (yyval.enode) = new(CHR, (yyvsp[-1].enode), ENULL);}
#line 5208 "y.tab.c"
    break;

  case 243: /* term: '@' K_ERR  */
#line 1489 "gram.y"
                                  { (yyval.enode) = new(ERR_, ENULL, ENULL); }
#line 5214 "y.tab.c"
    break;

  case 244: /* term: K_ERR  */
#line 1490 "gram.y"
                                  { (yyval.enode) = new(ERR_, ENULL, ENULL); }
#line 5220 "y.tab.c"
    break;

  case 245: /* term: '@' K_REF  */
#line 1491 "gram.y"
                                  { (yyval.enode) = new(REF_, ENULL, ENULL); }
#line 5226 "y.tab.c"
    break;

  case 246: /* term: K_REF  */
#line 1492 "gram.y"
                                  { (yyval.enode) = new(REF_, ENULL, ENULL); }
#line 5232 "y.tab.c"
    break;

  case 247: /* term: '@' K_FACT '(' e ')'  */
#line 1493 "gram.y"
                                  { (yyval.enode) = new(FACT, (yyvsp[-1].enode), ENULL); }
#line 5238 "y.tab.c"
    break;

  case 248: /* e: e '+' e  */
#line 1497 "gram.y"
                                  { (yyval.enode) = new('+', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5244 "y.tab.c"
    break;

  case 249: /* e: e '-' e  */
#line 1498 "gram.y"
                                  { (yyval.enode) = new('-', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5250 "y.tab.c"
    break;

  case 250: /* e: e '*' e  */
#line 1499 "gram.y"
                                  { (yyval.enode) = new('*', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5256 "y.tab.c"
    break;

  case 251: /* e: e '/' e  */
#line 1500 "gram.y"
                                  { (yyval.enode) = new('/', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5262 "y.tab.c"
    break;

  case 252: /* e: e '%' e  */
#line 1501 "gram.y"
                                  { (yyval.enode) = new('%', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5268 "y.tab.c"
    break;

  case 253: /* e: '-' e  */
#line 1502 "gram.y"
                                  { (yyval.enode) = new('m', (yyvsp[0].enode), ENULL); }
#line 5274 "y.tab.c"
    break;

  case 254: /* e: e '^' e  */
#line 1503 "gram.y"
                                  { (yyval.enode) = new('^', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5280 "y.tab.c"
    break;

  case 256: /* e: e '?' e ':' e  */
#line 1505 "gram.y"
                                  { (yyval.enode) = new('?', (yyvsp[-4].enode), new(':', (yyvsp[-2].enode), (yyvsp[0].enode))); }
#line 5286 "y.tab.c"
    break;

  case 257: /* e: e ';' e  */
#line 1506 "gram.y"
                                  { (yyval.enode) = new(';', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5292 "y.tab.c"
    break;

  case 258: /* e: e '<' e  */
#line 1507 "gram.y"
                                  { (yyval.enode) = new('<', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5298 "y.tab.c"
    break;

  case 259: /* e: e '=' e  */
#line 1508 "gram.y"
                                  { (yyval.enode) = new('=', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5304 "y.tab.c"
    break;

  case 260: /* e: e '>' e  */
#line 1509 "gram.y"
                                  { (yyval.enode) = new('>', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5310 "y.tab.c"
    break;

  case 261: /* e: e '&' e  */
#line 1510 "gram.y"
                                  { (yyval.enode) = new('&', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5316 "y.tab.c"
    break;

  case 262: /* e: e '|' e  */
#line 1511 "gram.y"
                                  { (yyval.enode) = new('|', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5322 "y.tab.c"
    break;

  case 263: /* e: e '<' '=' e  */
#line 1512 "gram.y"
                                  { (yyval.enode) = new('!', new('>', (yyvsp[-3].enode), (yyvsp[0].enode)), ENULL); }
#line 5328 "y.tab.c"
    break;

  case 264: /* e: e '<' '>' e  */
#line 1514 "gram.y"
                                  { (yyval.enode) = new('!', new('=', (yyvsp[-3].enode), (yyvsp[0].enode)), ENULL); }
#line 5334 "y.tab.c"
    break;

  case 265: /* e: e '>' '=' e  */
#line 1515 "gram.y"
                                  { (yyval.enode) = new('!', new('<', (yyvsp[-3].enode), (yyvsp[0].enode)), ENULL); }
#line 5340 "y.tab.c"
    break;

  case 266: /* e: e '#' e  */
#line 1516 "gram.y"
                                  { (yyval.enode) = new('#', (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5346 "y.tab.c"
    break;

  case 267: /* expr_list: e  */
#line 1519 "gram.y"
                                  { (yyval.enode) = new(ELIST, ENULL, (yyvsp[0].enode)); }
#line 5352 "y.tab.c"
    break;

  case 268: /* expr_list: expr_list ',' e  */
#line 1520 "gram.y"
                                  { (yyval.enode) = new(ELIST, (yyvsp[-2].enode), (yyvsp[0].enode)); }
#line 5358 "y.tab.c"
    break;

  case 269: /* range: var ':' var  */
#line 1523 "gram.y"
                                  {
                                    (yyval.rval).left = (yyvsp[-2].ent);
                                    (yyval.rval).right = (yyvsp[0].ent);
                                  }
#line 5367 "y.tab.c"
    break;

  case 270: /* range: RANGE  */
#line 1527 "gram.y"
                                  { (yyval.rval) = (yyvsp[0].rval); }
#line 5373 "y.tab.c"
    break;

  case 271: /* var: COL NUMBER  */
#line 1532 "gram.y"
                                  {
                                    struct roman * roman = session->cur_doc;
                                    (yyval.ent).vp = lookat(roman->cur_sh, (yyvsp[0].ival), (yyvsp[-1].ival));
                                    (yyval.ent).vf = 0;
                                    (yyval.ent).sheet = roman->cur_sh;
                                  }
#line 5384 "y.tab.c"
    break;

  case 272: /* var: '$' COL NUMBER  */
#line 1539 "gram.y"
                                  {
                                    struct roman * roman = session->cur_doc;
                                    (yyval.ent).vp = lookat(roman->cur_sh, (yyvsp[0].ival), (yyvsp[-1].ival));
                                    (yyval.ent).vf = FIX_COL;
                                    (yyval.ent).sheet = roman->cur_sh;
                                  }
#line 5395 "y.tab.c"
    break;

  case 273: /* var: COL '$' NUMBER  */
#line 1546 "gram.y"
                                  {
                                    struct roman * roman = session->cur_doc;
                                    (yyval.ent).vp = lookat(roman->cur_sh, (yyvsp[0].ival), (yyvsp[-2].ival));
                                    (yyval.ent).vf = FIX_ROW;
                                    (yyval.ent).sheet = roman->cur_sh;
                                  }
#line 5406 "y.tab.c"
    break;

  case 274: /* var: '$' COL '$' NUMBER  */
#line 1553 "gram.y"
                                  {
                                    struct roman * roman = session->cur_doc;
                                    (yyval.ent).vp = lookat(roman->cur_sh, (yyvsp[0].ival), (yyvsp[-2].ival));
                                    (yyval.ent).vf = FIX_ROW | FIX_COL;
                                    (yyval.ent).sheet = roman->cur_sh;
                                  }
#line 5417 "y.tab.c"
    break;

  case 275: /* var: '@' K_GETENT '(' e ',' e ')'  */
#line 1560 "gram.y"
                                   {
                                    struct roman * roman = session->cur_doc;
                                    struct sheet * sh = roman->cur_sh;
                                    (yyval.ent).vp = lookat(sh, eval(sh, NULL, (yyvsp[-3].enode), 0), eval(sh, NULL, (yyvsp[-1].enode), 0));
                                    (yyval.ent).vf = GET_ENT;
                                    if ((yyval.ent).expr != NULL) efree((yyval.ent).expr);
                                    (yyval.ent).expr = new(GETENT, (yyvsp[-3].enode), (yyvsp[-1].enode));
                                    (yyval.ent).sheet = sh;
                                  }
#line 5431 "y.tab.c"
    break;

  case 276: /* var: '{' STRING '}' '!' COL NUMBER  */
#line 1571 "gram.y"
                                  {
                                    struct roman * roman = session->cur_doc;
                                    struct sheet * sh;
                                    if ((sh = search_sheet(roman, (yyvsp[-4].sval))) == NULL ) sh = roman->cur_sh;
                                    (yyval.ent).vp = lookat(sh, (yyvsp[0].ival), (yyvsp[-1].ival));
                                    (yyval.ent).vf = 0;
                                    (yyval.ent).sheet = sh;
                                    scxfree((yyvsp[-4].sval));
                                  }
#line 5445 "y.tab.c"
    break;

  case 277: /* var: '{' STRING '}' '!' '$' COL NUMBER  */
#line 1581 "gram.y"
                                           {
                                    struct roman * roman = session->cur_doc;
                                    struct sheet * sh;
                                    if ((sh = search_sheet(roman, (yyvsp[-5].sval))) == NULL ) sh = roman->cur_sh;
                                    (yyval.ent).vp = lookat(sh, (yyvsp[0].ival), (yyvsp[-1].ival));
                                    (yyval.ent).vf = FIX_COL;
                                    (yyval.ent).sheet = sh;
                                    scxfree((yyvsp[-5].sval));
                                  }
#line 5459 "y.tab.c"
    break;

  case 278: /* var: '{' STRING '}' '!' COL '$' NUMBER  */
#line 1591 "gram.y"
                                           {
                                    struct roman * roman = session->cur_doc;
                                    struct sheet * sh;
                                    if ((sh = search_sheet(roman, (yyvsp[-5].sval))) == NULL ) sh = roman->cur_sh;
                                    (yyval.ent).vp = lookat(sh, (yyvsp[0].ival), (yyvsp[-2].ival));
                                    (yyval.ent).vf = FIX_ROW;
                                    (yyval.ent).sheet = sh;
                                    scxfree((yyvsp[-5].sval));
                                  }
#line 5473 "y.tab.c"
    break;

  case 279: /* var: '{' STRING '}' '!' '$' COL '$' NUMBER  */
#line 1601 "gram.y"
                                               {
                                    struct roman * roman = session->cur_doc;
                                    struct sheet * sh;
                                    if ((sh = search_sheet(roman, (yyvsp[-6].sval))) == NULL ) sh = roman->cur_sh;
                                    (yyval.ent).vp = lookat(sh, (yyvsp[0].ival), (yyvsp[-2].ival));
                                    (yyval.ent).vf = FIX_ROW | FIX_COL;
                                    (yyval.ent).sheet = sh;
                                    scxfree((yyvsp[-6].sval));
                                  }
#line 5487 "y.tab.c"
    break;

  case 280: /* var: VAR  */
#line 1611 "gram.y"
                                  {
                                    (yyval.ent) = (yyvsp[0].rval).left;
                                  }
#line 5495 "y.tab.c"
    break;

  case 281: /* var_or_range: range  */
#line 1617 "gram.y"
                                  { (yyval.rval) = (yyvsp[0].rval); }
#line 5501 "y.tab.c"
    break;

  case 282: /* var_or_range: var  */
#line 1618 "gram.y"
                                  { (yyval.rval).left = (yyvsp[0].ent); (yyval.rval).right = (yyvsp[0].ent); }
#line 5507 "y.tab.c"
    break;

  case 283: /* num: NUMBER  */
#line 1621 "gram.y"
                                  { (yyval.fval) = (double) (yyvsp[0].ival); }
#line 5513 "y.tab.c"
    break;

  case 284: /* num: FNUMBER  */
#line 1622 "gram.y"
                                  { (yyval.fval) = (yyvsp[0].fval); }
#line 5519 "y.tab.c"
    break;

  case 285: /* num: '-' num  */
#line 1623 "gram.y"
                                  { (yyval.fval) = -(yyvsp[0].fval); }
#line 5525 "y.tab.c"
    break;

  case 286: /* num: '+' num  */
#line 1624 "gram.y"
                                  { (yyval.fval) = (yyvsp[0].fval); }
#line 5531 "y.tab.c"
    break;

  case 287: /* strarg: STRING  */
#line 1627 "gram.y"
                                  { (yyval.sval) = (yyvsp[0].sval); }
#line 5537 "y.tab.c"
    break;

  case 288: /* strarg: var  */
#line 1628 "gram.y"
                                  {
                                    char *s, *s1;
                                    s1 = (yyvsp[0].ent).vp->label;
                                    if (!s1)
                                    s1 = "NULL_STRING";
                                    s = scxmalloc((unsigned)strlen(s1)+1);
                                    (void) strcpy(s, s1);
                                    (yyval.sval) = s;
                                  }
#line 5551 "y.tab.c"
    break;

  case 291: /* setitem: K_OVERLAP '=' NUMBER  */
#line 1646 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "overlap=0", TRUE);
                                     else         parse_str(user_conf_d, "overlap=1", TRUE); }
#line 5558 "y.tab.c"
    break;

  case 292: /* setitem: K_HISTORY_FILE '=' NUMBER  */
#line 1648 "gram.y"
                                   { if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "history_file=0", TRUE);
                                    else         parse_str(user_conf_d, "history_file=1", TRUE); }
#line 5565 "y.tab.c"
    break;

  case 293: /* setitem: K_HISTORY_FILE  */
#line 1650 "gram.y"
                                  {              parse_str(user_conf_d, "history_file=1", TRUE); }
#line 5571 "y.tab.c"
    break;

  case 294: /* setitem: K_NOHISTORY_FILE  */
#line 1651 "gram.y"
                                  {              parse_str(user_conf_d, "history_file=0", TRUE); }
#line 5577 "y.tab.c"
    break;

  case 295: /* setitem: K_OVERLAP  */
#line 1652 "gram.y"
                                  {               parse_str(user_conf_d, "overlap=1", TRUE); }
#line 5583 "y.tab.c"
    break;

  case 296: /* setitem: K_INPUT_BAR_BOTTOM '=' NUMBER  */
#line 1653 "gram.y"
                                         {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "input_bar_bottom=0", TRUE);
                                     else         parse_str(user_conf_d, "input_bar_bottom=1", TRUE);
                                                  ui_mv_bottom_bar(); }
#line 5591 "y.tab.c"
    break;

  case 297: /* setitem: K_INPUT_BAR_BOTTOM  */
#line 1656 "gram.y"
                                  {               parse_str(user_conf_d, "input_bar_bottom=1", TRUE);
                                                  ui_mv_bottom_bar();
                                  }
#line 5599 "y.tab.c"
    break;

  case 298: /* setitem: K_INPUT_EDIT_MODE '=' NUMBER  */
#line 1660 "gram.y"
                                        {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "input_edit_mode=0", TRUE);
                                     else         parse_str(user_conf_d, "input_edit_mode=1", TRUE);
                                                  ui_mv_bottom_bar(); }
#line 5607 "y.tab.c"
    break;

  case 299: /* setitem: K_INPUT_EDIT_MODE  */
#line 1663 "gram.y"
                                  {               parse_str(user_conf_d, "input_edit_mode=1", TRUE);
                                                  ui_mv_bottom_bar();
                                  }
#line 5615 "y.tab.c"
    break;

  case 300: /* setitem: K_UNDERLINE_GRID '=' NUMBER  */
#line 1667 "gram.y"
                                     {            if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "underline_grid=0", TRUE);
                                                  else parse_str(user_conf_d, "underline_grid=1", TRUE); }
#line 5622 "y.tab.c"
    break;

  case 301: /* setitem: K_UNDERLINE_GRID  */
#line 1669 "gram.y"
                                  {               parse_str(user_conf_d, "underline_grid=1", TRUE);
                                  }
#line 5629 "y.tab.c"
    break;

  case 302: /* setitem: K_NOOVERLAP  */
#line 1672 "gram.y"
                                  {               parse_str(user_conf_d, "overlap=0", TRUE); }
#line 5635 "y.tab.c"
    break;

  case 303: /* setitem: K_TRUNCATE '=' NUMBER  */
#line 1674 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "truncate=0", TRUE);
                                     else         parse_str(user_conf_d, "truncate=1", TRUE); }
#line 5642 "y.tab.c"
    break;

  case 304: /* setitem: K_TRUNCATE  */
#line 1676 "gram.y"
                                  {               parse_str(user_conf_d, "truncate=1", TRUE); }
#line 5648 "y.tab.c"
    break;

  case 305: /* setitem: K_NOTRUNCATE  */
#line 1677 "gram.y"
                                  {               parse_str(user_conf_d, "truncate=0", TRUE); }
#line 5654 "y.tab.c"
    break;

  case 306: /* setitem: K_AUTOWRAP '=' NUMBER  */
#line 1679 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "autowrap=0", TRUE);
                                     else         parse_str(user_conf_d, "autowrap=1", TRUE); }
#line 5661 "y.tab.c"
    break;

  case 307: /* setitem: K_AUTOWRAP  */
#line 1681 "gram.y"
                                  {               parse_str(user_conf_d, "autowrap=1", TRUE); }
#line 5667 "y.tab.c"
    break;

  case 308: /* setitem: K_NOAUTOWRAP  */
#line 1682 "gram.y"
                                  {               parse_str(user_conf_d, "autowrap=0", TRUE); }
#line 5673 "y.tab.c"
    break;

  case 309: /* setitem: K_AUTOBACKUP  */
#line 1684 "gram.y"
                                  {               parse_str(user_conf_d, "autobackup=1", TRUE); }
#line 5679 "y.tab.c"
    break;

  case 310: /* setitem: K_AUTOBACKUP '=' NUMBER  */
#line 1685 "gram.y"
                                  {
                                                  char cmd[MAXCMD];
                                                  sprintf(cmd, "autobackup=%d", (yyvsp[0].ival));
                                                  parse_str(user_conf_d, cmd, TRUE); }
#line 5688 "y.tab.c"
    break;

  case 311: /* setitem: K_NOAUTOBACKUP  */
#line 1689 "gram.y"
                                  {               parse_str(user_conf_d, "autobackup=0", TRUE); }
#line 5694 "y.tab.c"
    break;

  case 312: /* setitem: K_AUTOCALC  */
#line 1690 "gram.y"
                                  {               parse_str(user_conf_d, "autocalc=1", TRUE); }
#line 5700 "y.tab.c"
    break;

  case 313: /* setitem: K_AUTOCALC '=' NUMBER  */
#line 1691 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "autocalc=0", TRUE);
                                     else         parse_str(user_conf_d, "autocalc=1", TRUE); }
#line 5707 "y.tab.c"
    break;

  case 314: /* setitem: K_NOAUTOCALC  */
#line 1693 "gram.y"
                                  {               parse_str(user_conf_d, "autocalc=0", TRUE); }
#line 5713 "y.tab.c"
    break;

  case 315: /* setitem: K_DEBUG  */
#line 1694 "gram.y"
                                  {               parse_str(user_conf_d, "debug=1", TRUE); }
#line 5719 "y.tab.c"
    break;

  case 316: /* setitem: K_DEBUG '=' NUMBER  */
#line 1695 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "debug=0", TRUE);
                                     else         parse_str(user_conf_d, "debug=1", TRUE); }
#line 5726 "y.tab.c"
    break;

  case 317: /* setitem: K_NODEBUG  */
#line 1697 "gram.y"
                                  {               parse_str(user_conf_d, "debug=0", TRUE); }
#line 5732 "y.tab.c"
    break;

  case 318: /* setitem: K_TRG  */
#line 1698 "gram.y"
                                  {               parse_str(user_conf_d, "trigger=1", TRUE); }
#line 5738 "y.tab.c"
    break;

  case 319: /* setitem: K_TRG '=' NUMBER  */
#line 1699 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "trigger=0", TRUE);
                                     else         parse_str(user_conf_d, "trigger=1", TRUE); }
#line 5745 "y.tab.c"
    break;

  case 320: /* setitem: K_NOTRG  */
#line 1701 "gram.y"
                                  {               parse_str(user_conf_d, "trigger=0", TRUE); }
#line 5751 "y.tab.c"
    break;

  case 321: /* setitem: K_EXTERNAL_FUNCTIONS  */
#line 1702 "gram.y"
                                  {               parse_str(user_conf_d, "external_functions=1", TRUE); }
#line 5757 "y.tab.c"
    break;

  case 322: /* setitem: K_EXTERNAL_FUNCTIONS '=' NUMBER  */
#line 1703 "gram.y"
                                         {
                                     if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "external_functions=0", TRUE);
                                     else         parse_str(user_conf_d, "external_functions=1", TRUE); }
#line 5765 "y.tab.c"
    break;

  case 323: /* setitem: K_NOEXTERNAL_FUNCTIONS  */
#line 1706 "gram.y"
                                  {               parse_str(user_conf_d, "external_functions=0", TRUE); }
#line 5771 "y.tab.c"
    break;

  case 324: /* setitem: K_EXEC_LUA  */
#line 1707 "gram.y"
                                  {               parse_str(user_conf_d, "exec_lua=1", TRUE); }
#line 5777 "y.tab.c"
    break;

  case 325: /* setitem: K_EXEC_LUA '=' NUMBER  */
#line 1708 "gram.y"
                                  {
                                     if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "exec_lua=0", TRUE);
                                     else         parse_str(user_conf_d, "exec_lua=1", TRUE); }
#line 5785 "y.tab.c"
    break;

  case 326: /* setitem: K_NOEXEC_LUA  */
#line 1711 "gram.y"
                                  {               parse_str(user_conf_d, "exec_lua=0", TRUE); }
#line 5791 "y.tab.c"
    break;

  case 327: /* setitem: K_HALF_PAGE_SCROLL  */
#line 1712 "gram.y"
                                  {               parse_str(user_conf_d, "half_page_scroll=1", TRUE); }
#line 5797 "y.tab.c"
    break;

  case 328: /* setitem: K_HALF_PAGE_SCROLL '=' NUMBER  */
#line 1714 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "half_page_scroll=0", TRUE);
                                     else         parse_str(user_conf_d, "half_page_scroll=1", TRUE); }
#line 5804 "y.tab.c"
    break;

  case 329: /* setitem: K_NOHALF_PAGE_SCROLL  */
#line 1716 "gram.y"
                                  {               parse_str(user_conf_d, "half_page_scroll=0", TRUE); }
#line 5810 "y.tab.c"
    break;

  case 330: /* setitem: K_IGNORE_HIDDEN  */
#line 1718 "gram.y"
                                  {               parse_str(user_conf_d, "ignore_hidden=1", TRUE); }
#line 5816 "y.tab.c"
    break;

  case 331: /* setitem: K_IGNORE_HIDDEN '=' NUMBER  */
#line 1720 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "ignore_hidden=0", TRUE);
                                     else         parse_str(user_conf_d, "ignore_hidden=1", TRUE); }
#line 5823 "y.tab.c"
    break;

  case 332: /* setitem: K_NOIGNORE_HIDDEN  */
#line 1722 "gram.y"
                                  {               parse_str(user_conf_d, "ignore_hidden=0", TRUE); }
#line 5829 "y.tab.c"
    break;

  case 333: /* setitem: K_QUIET '=' NUMBER  */
#line 1724 "gram.y"
                                  {
                                     if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "quiet=0", TRUE);
                                     else         parse_str(user_conf_d, "quiet=1", TRUE); }
#line 5837 "y.tab.c"
    break;

  case 334: /* setitem: K_QUIET  */
#line 1727 "gram.y"
                                  {               parse_str(user_conf_d, "quiet=1", TRUE); }
#line 5843 "y.tab.c"
    break;

  case 335: /* setitem: K_NOQUIET  */
#line 1728 "gram.y"
                                  {               parse_str(user_conf_d, "quiet=0", TRUE); }
#line 5849 "y.tab.c"
    break;

  case 336: /* setitem: K_QUIT_AFTERLOAD  */
#line 1729 "gram.y"
                                  {               parse_str(user_conf_d, "quit_afterload=1", TRUE); }
#line 5855 "y.tab.c"
    break;

  case 337: /* setitem: K_QUIT_AFTERLOAD '=' NUMBER  */
#line 1731 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "quit_afterload=0", TRUE);
                                     else         parse_str(user_conf_d, "quit_afterload=1", TRUE); }
#line 5862 "y.tab.c"
    break;

  case 338: /* setitem: K_NOQUIT_AFTERLOAD  */
#line 1733 "gram.y"
                                  {               parse_str(user_conf_d, "quit_afterload=0", TRUE); }
#line 5868 "y.tab.c"
    break;

  case 339: /* setitem: K_XLSX_READFORMULAS  */
#line 1734 "gram.y"
                                  {               parse_str(user_conf_d, "xlsx_readformulas=1", TRUE); }
#line 5874 "y.tab.c"
    break;

  case 340: /* setitem: K_XLSX_READFORMULAS '=' NUMBER  */
#line 1736 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "xlsx_readformulas=0", TRUE);
                                     else         parse_str(user_conf_d, "xlsx_readformulas=1", TRUE); }
#line 5881 "y.tab.c"
    break;

  case 341: /* setitem: K_NOXLSX_READFORMULAS  */
#line 1738 "gram.y"
                                  {               parse_str(user_conf_d, "xlsx_readformulas=0", TRUE); }
#line 5887 "y.tab.c"
    break;

  case 342: /* setitem: K_NOCURSES  */
#line 1739 "gram.y"
                                  {  if (! session->cur_doc->loading) parse_str(user_conf_d, "nocurses=1", TRUE); }
#line 5893 "y.tab.c"
    break;

  case 343: /* setitem: K_NOCURSES '=' NUMBER  */
#line 1740 "gram.y"
                                  {  if (! session->cur_doc->loading) {
                                         if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "nocurses=0", TRUE);
                                         else         parse_str(user_conf_d, "nocurses=1", TRUE); }
                                     }
#line 5902 "y.tab.c"
    break;

  case 344: /* setitem: K_CURSES  */
#line 1744 "gram.y"
                                  {  if (! session->cur_doc->loading) parse_str(user_conf_d, "nocurses=0", TRUE); }
#line 5908 "y.tab.c"
    break;

  case 345: /* setitem: K_NUMERIC  */
#line 1745 "gram.y"
                                  {               parse_str(user_conf_d, "numeric=1", TRUE); }
#line 5914 "y.tab.c"
    break;

  case 346: /* setitem: K_NUMERIC '=' NUMBER  */
#line 1746 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "numeric=0", TRUE);
                                     else         parse_str(user_conf_d, "numeric=1", TRUE); }
#line 5921 "y.tab.c"
    break;

  case 347: /* setitem: K_NONUMERIC  */
#line 1748 "gram.y"
                                  {               parse_str(user_conf_d, "numeric=0", TRUE); }
#line 5927 "y.tab.c"
    break;

  case 348: /* setitem: K_IGNORECASE  */
#line 1749 "gram.y"
                                  {               parse_str(user_conf_d, "ignorecase=1", TRUE); }
#line 5933 "y.tab.c"
    break;

  case 349: /* setitem: K_IGNORECASE '=' NUMBER  */
#line 1750 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "ignorecase=0", TRUE);
                                     else         parse_str(user_conf_d, "ignorecase=1", TRUE); }
#line 5940 "y.tab.c"
    break;

  case 350: /* setitem: K_NOIGNORECASE  */
#line 1752 "gram.y"
                                  {               parse_str(user_conf_d, "ignorecase=0", TRUE); }
#line 5946 "y.tab.c"
    break;

  case 351: /* setitem: K_NUMERIC_DECIMAL  */
#line 1753 "gram.y"
                                  {               parse_str(user_conf_d, "numeric_decimal=1", TRUE); }
#line 5952 "y.tab.c"
    break;

  case 352: /* setitem: K_NUMERIC_DECIMAL '=' NUMBER  */
#line 1755 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "numeric_decimal=0", TRUE);
                                     else         parse_str(user_conf_d, "numeric_decimal=1", TRUE); }
#line 5959 "y.tab.c"
    break;

  case 353: /* setitem: K_NONUMERIC_DECIMAL  */
#line 1757 "gram.y"
                                  {               parse_str(user_conf_d, "numeric_decimal=0", TRUE); }
#line 5965 "y.tab.c"
    break;

  case 354: /* setitem: K_NUMERIC_ZERO  */
#line 1758 "gram.y"
                                  {               parse_str(user_conf_d, "numeric_zero=1", TRUE); }
#line 5971 "y.tab.c"
    break;

  case 355: /* setitem: K_NUMERIC_ZERO '=' NUMBER  */
#line 1760 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "numeric_zero=0", TRUE);
                                     else         parse_str(user_conf_d, "numeric_zero=1", TRUE); }
#line 5978 "y.tab.c"
    break;

  case 356: /* setitem: K_NONUMERIC_ZERO  */
#line 1762 "gram.y"
                                  {               parse_str(user_conf_d, "numeric_zero=0", TRUE); }
#line 5984 "y.tab.c"
    break;

  case 357: /* setitem: K_NEWLINE_ACTION  */
#line 1763 "gram.y"
                                  {               parse_str(user_conf_d, "newline_action=0", TRUE); }
#line 5990 "y.tab.c"
    break;

  case 358: /* setitem: K_NEWLINE_ACTION '=' WORD  */
#line 1764 "gram.y"
                                   {
                                  char * s = (char *) (yyvsp[0].sval);
                                  if (s[0] =='j') parse_str(user_conf_d, "newline_action=j", TRUE);
                                  else if (s[0] =='l')
                                  parse_str(user_conf_d, "newline_action=l", TRUE);
                                  }
#line 6001 "y.tab.c"
    break;

  case 359: /* setitem: K_DEFAULT_COPY_TO_CLIPBOARD_CMD '=' strarg  */
#line 1770 "gram.y"
                                                    {
                                  char cmd[MAXCMD];
                                  char * s = (char *) (yyvsp[0].sval);
                                  sprintf(cmd, "default_copy_to_clipboard_cmd=%s", s);
                                  parse_str(user_conf_d, cmd, FALSE);
                                  scxfree(s);
                                  }
#line 6013 "y.tab.c"
    break;

  case 360: /* setitem: K_DEFAULT_PASTE_FROM_CLIPBOARD_CMD '=' strarg  */
#line 1777 "gram.y"
                                                       {
                                  char cmd[MAXCMD];
                                  char * s = (char *) (yyvsp[0].sval);
                                  sprintf(cmd, "default_paste_from_clipboard_cmd=%s", s);
                                  parse_str(user_conf_d, cmd, FALSE);
                                  scxfree(s);
                                  }
#line 6025 "y.tab.c"
    break;

  case 361: /* setitem: K_IMPORT_DELIMITED_TO_TEXT  */
#line 1785 "gram.y"
                                    {      parse_str(user_conf_d, "import_delimited_to_text=1", TRUE); }
#line 6031 "y.tab.c"
    break;

  case 362: /* setitem: K_IMPORT_DELIMITED_TO_TEXT '=' NUMBER  */
#line 1788 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "import_delimited_to_text=0", TRUE);
                                     else         parse_str(user_conf_d, "import_delimited_to_text=1", TRUE); }
#line 6038 "y.tab.c"
    break;

  case 363: /* setitem: K_COPY_TO_CLIPBOARD_DELIMITED_TAB  */
#line 1791 "gram.y"
                                           {      parse_str(user_conf_d, "copy_to_clipboard_delimited_tab=1", TRUE); }
#line 6044 "y.tab.c"
    break;

  case 364: /* setitem: K_COPY_TO_CLIPBOARD_DELIMITED_TAB '=' NUMBER  */
#line 1794 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "copy_to_clipboard_delimited_tab=0", TRUE);
                                     else         parse_str(user_conf_d, "copy_to_clipboard_delimited_tab=1", TRUE); }
#line 6051 "y.tab.c"
    break;

  case 365: /* setitem: K_NOCOPY_TO_CLIPBOARD_DELIMITED_TAB  */
#line 1796 "gram.y"
                                             {    parse_str(user_conf_d, "copy_to_clipboard_delimited_tab=0", TRUE); }
#line 6057 "y.tab.c"
    break;

  case 366: /* setitem: K_COPY_TO_CLIPBOARD_WYSIWYG  */
#line 1797 "gram.y"
                                     {            parse_str(user_conf_d, "copy_to_clipboard_wysiwyg=1", TRUE); }
#line 6063 "y.tab.c"
    break;

  case 367: /* setitem: K_COPY_TO_CLIPBOARD_WYSIWYG '=' NUMBER  */
#line 1799 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "copy_to_clipboard_wysiwyg=0", TRUE);
                                     else         parse_str(user_conf_d, "copy_to_clipboard_wysiwyg=1", TRUE); }
#line 6070 "y.tab.c"
    break;

  case 368: /* setitem: K_NOCOPY_TO_CLIPBOARD_WYSIWYG  */
#line 1801 "gram.y"
                                       {          parse_str(user_conf_d, "copy_to_clipboard_wysiwyg=0", TRUE); }
#line 6076 "y.tab.c"
    break;

  case 369: /* setitem: K_DEFAULT_OPEN_FILE_UNDER_CURSOR_CMD '=' strarg  */
#line 1802 "gram.y"
                                                         {
                                  char cmd[MAXCMD];
                                  char * s = (char *) (yyvsp[0].sval);
                                  sprintf(cmd, "default_open_file_under_cursor_cmd=%s", s);
                                  parse_str(user_conf_d, cmd, FALSE);
                                  scxfree(s);
                                  }
#line 6088 "y.tab.c"
    break;

  case 370: /* setitem: K_NEWLINE_ACTION '=' NUMBER  */
#line 1810 "gram.y"
                                     {
                                     if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "newline_action=0", TRUE); }
#line 6095 "y.tab.c"
    break;

  case 371: /* setitem: K_COMMAND_TIMEOUT  */
#line 1812 "gram.y"
                                  {               parse_str(user_conf_d, "command_timeout=3000", TRUE); }
#line 6101 "y.tab.c"
    break;

  case 372: /* setitem: K_COMMAND_TIMEOUT '=' num  */
#line 1813 "gram.y"
                                     {
                                     char * s = scxmalloc((unsigned) BUFFERSIZE);
                                     sprintf(s, "command_timeout=%d", (int) (yyvsp[0].fval));
                                     parse_str(user_conf_d, s, TRUE);
                                     scxfree(s);
                                     }
#line 6112 "y.tab.c"
    break;

  case 373: /* setitem: K_MAPPING_TIMEOUT  */
#line 1819 "gram.y"
                                  {               parse_str(user_conf_d, "mapping_timeout=1500", TRUE); }
#line 6118 "y.tab.c"
    break;

  case 374: /* setitem: K_MAPPING_TIMEOUT '=' num  */
#line 1820 "gram.y"
                                     {
                                     char * s = scxmalloc((unsigned) BUFFERSIZE);
                                     sprintf(s, "mapping_timeout=%d", (int) (yyvsp[0].fval));
                                     parse_str(user_conf_d, s, TRUE);
                                     scxfree(s);
                                     }
#line 6129 "y.tab.c"
    break;

  case 375: /* setitem: K_TM_GMTOFF  */
#line 1826 "gram.y"
                                  {               parse_str(user_conf_d, "tm_gmtoff=-10800", TRUE); }
#line 6135 "y.tab.c"
    break;

  case 376: /* setitem: K_TM_GMTOFF '=' num  */
#line 1827 "gram.y"
                                  {
                                     char * s = scxmalloc((unsigned) BUFFERSIZE);
                                     sprintf(s, "tm_gmtoff=%d", (int) (yyvsp[0].fval));
                                     parse_str(user_conf_d, s, TRUE);
                                     scxfree(s);
                                  }
#line 6146 "y.tab.c"
    break;

  case 377: /* setitem: K_SHOW_CURSOR '=' NUMBER  */
#line 1833 "gram.y"
                                  {  if ((yyvsp[0].ival) == 0) parse_str(user_conf_d, "show_cursor=0", TRUE);
                                     else         parse_str(user_conf_d, "show_cursor=1", TRUE); }
#line 6153 "y.tab.c"
    break;

  case 378: /* setitem: K_SHOW_CURSOR  */
#line 1835 "gram.y"
                                  {               parse_str(user_conf_d, "show_cursor=1", TRUE); }
#line 6159 "y.tab.c"
    break;

  case 379: /* setitem: K_NOSHOW_CURSOR  */
#line 1836 "gram.y"
                                  {               parse_str(user_conf_d, "show_cursor=0", TRUE); }
#line 6165 "y.tab.c"
    break;


#line 6169 "y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

