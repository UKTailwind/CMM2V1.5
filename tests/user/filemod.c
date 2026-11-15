/* filemod.c: a test USER module that keeps a file open between calls (the loader's file ownership: SQL CLOSE must
   leave it open, USER CLOSE must close it). tests/user/usersql.bas uses it. Build from tools/sdk:
   python build.py out/filemod.mod ../../tests/user/filemod.c --module --api 20 --stack 4096 */
#include <stdio.h>
#include <string.h>
#include "cmm2user.h"
static const struct cmm2_basic *B;
static FILE *f;
static void command(char *line) {
    char *p;
    if((p = B->checkstring(line, "FOPEN"))) { if((f = fopen(B->get_cstring(p), "w")) == NULL) B->error("fopen failed"); }
    else if((p = B->checkstring(line, "FWRITE"))) {
        if(!f) B->error("Not open");
        if(fputs(B->get_cstring(p), f) < 0 || fputs("\r\n", f) < 0 || fflush(f) != 0) B->error("Write failed");
    }
    else if(B->checkstring(line, "FCLOSE")) { if(f && fclose(f) != 0) { f = NULL; B->error("Close failed"); } f = NULL; }
    else B->error("Unknown USER command");
}
static void function(char *a, cmm2_value *r) { (void)a; r->type = CMM2_T_INT; r->i = f != NULL; }
static void forget(void) { f = NULL; }          /* (the loader closes the module's files after this) */
static const cmm2user_head table = { {CMM2MOD_MAGIC, CMM2USER_VERSION, forget}, CMM2USER_MAGIC, command, function };
const void *module_main(void) { B = cmm2_start->basic; return B ? &table : NULL; }
