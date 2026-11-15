/* cmm2user.h: what a USER module sees of MMBasic (tools/sdk): the interpreter's routines, MMBasic's types and the
   module's table, all in Inc/cmm2user.h. cmm2_start->basic is the table (cmm2.h), set before module_main() is called.
   Build a module with build.py --module --api 20; examples/user/proforma.c is one to start from. MIT License, as
   cmm2img.h. */
#ifndef CMM2USER_SDK_H
#define CMM2USER_SDK_H
#include "cmm2.h"
#include "../../Inc/cmm2user.h"         /* (by its path, as cmm2.h includes cmm2img.h) */
#endif
