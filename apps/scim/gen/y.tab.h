/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

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

#line 604 "y.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
