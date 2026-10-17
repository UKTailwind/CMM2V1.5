/***************************************************************************

CMM2 MMBasic
commands.c

Handles all the commands in MMBasic

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
#include "help.h"
#include "turtle.h"

void flist(int, int, int);
void clearprog(void);
void execute_one_command(char *p);
void MIPS16 ListProgramFlash(char *p, int all);
void MIPS16 ListFile(char *pp, int all);
// stack to keep track of nested FOR/NEXT loops
struct s_forstack forstack[MAXFORLOOPS + 1];
int forindex;
extern unsigned long long int statement_time;
extern char *firststmt;
extern volatile uint64_t FastTimer;
// stack to keep track of nested DO/LOOP loops
struct s_dostack dostack[MAXDOLOOPS];
int doindex;                                						// counts the number of nested DO/LOOP loops
char *KeyInterrupt=NULL;
volatile int Keycomplete=0;
int keyselect=0;
void ListNewLine(int *ListCnt, int all);
// stack to keep track of GOSUBs, SUBs and FUNCTIONs
char *gosubstack[MAXGOSUB];
char *errorstack[MAXGOSUB];
char *substack[MAXGOSUB];                   // the SUB/FUNCTION definition each frame runs, NULL for a GOSUB: STATIC names its variable after the innermost one
int gosubindex;
char runcmd[STRINGSIZE];

char DimUsed = false;						// used to catch OPTION BASE after DIM has been used
struct sa_data datastore[MAXRESTORE];
int restorepointer = 0;

int TraceOn;                                // used to track the state of TRON/TROFF
  char *TraceBuff[TRACE_BUFF_SIZE];
  int TraceBuffIndex;                       // used for listing the contents of the trace buffer

int OptionErrorSkip;                                               // how to handle an error
int MMerrno=0;                                                        // the error number
int MMerrline=0;                                                      // MM.ERRLINE: the line the error message gave, 0 if none
int ErrorSkipDepth=0;                                                 // the ExecuteProgram depth ON ERROR SKIP was given at
uint64_t g_flag = 0;                                                // FLAGS, FLAG(n) and MM.FLAGS: 64 flags a program keeps (the PicoMite's)
char *MMErrMsg = "";                                                // the error message
void command_print(char *cmd);
void execute(char *mycmd);
void cmd_null(void) {
	// do nothing (this is just a placeholder for commands that have no action)
}
void display_help(int current, char *buff){
	   MMPrintString("\r\n");
	   CurrentX=3;
	   int k;
	   for(k=0;k<Option.Width-1;k++)putConsole(k<strlen(help[current+1])? help[current+1][k] : ' ');
	   PRet();
	   CurrentX=3;
	   k=0;
	   for(int j=0;j<strlen(buff);j++)if(toupper(buff[j])!=help[current+2][j])k=1;
	   if(k==0){
		   for(k=0;k<Option.Width-1;k++)putConsole(k<strlen(help[current+3])? help[current+3][k] : ' ');
	   } else {
		 for(k=0;k<Option.Width-1;k++)putConsole(' ');
	   }
	   PRet();
	   CurrentX=3;
	   k=0;
	   for(int j=0;j<strlen(buff);j++)if(toupper(buff[j])!=help[current+4][j])k=1;
	   if(k==0){
		   for(k=0;k<Option.Width-1;k++)putConsole(k<strlen(help[current+5])? help[current+5][k] : ' ');
	   } else {
		 for(k=0;k<Option.Width-1;k++)putConsole(' ');
	   }
}
void do_help(char *p){
    if(CurrentLinePtr) error("Invalid in a program");
    char buff[STRINGSIZE]={0};
    strcpy(buff,p);
    int i,currsave=CurrentY, up=1;
    int current=0;
    int firstin=1;
 	MMPrintString("\r\n\n\n");
    while(1){
    	if(up)SerUSBPutS("\033[3A");
		ShowCursor(false);
		putConsole('\r');
    	CurrentY=currsave;CurrentX=3;
        putConsole('?');putConsole(' ');putConsole(' ');
        CurrentX-=(FontTable[gui_font >> 4][0] * (gui_font & 0b1111));
        MMPrintString(buff);
    	int c=0;
    	if(firstin==0){
			do {
				ShowCursor(true);
				c=MMInkey();

			}while(c==-1);
    	}
    	firstin=0;
    	if(c==0x1b || c==F12){
    		ShowCursor(false);
    		MMPrintString("\r\n\n\n\n");
    		strcpy(p,help[current+1]);
    		break;
    	}
    	if(c=='\b' && strlen(buff)){
    		ShowCursor(false);
    		buff[strlen(buff)-1]=0;
    		CurrentX-=(FontTable[gui_font >> 4][0] * (gui_font & 0b1111));
    		SerUSBPutS("\033[1K");
    		DisplayPutC(' ');
    		putConsole('\r');
    	} else if(c==UP){
    		int k;
    	    current-=2;
    	    if(current==-2)current=0;
		    k=0;
		    for(int j=0;j<strlen(buff);j++)if(toupper(buff[j])!=help[current][j])k=1;
		    if(k==0){
			   display_help(current, buff);
			   up=1;
			   continue;
		    } else {
			   current+=2;
		    }
    	} else if(c==DOWN){
    		int k;
    	    current+=2;
    	    if(*help[current]=='Z')current-=2;
 		    k=0;
 		    for(int j=0;j<strlen(buff);j++)if(toupper(buff[j])!=help[current][j])k=1;
 		    if(k==0){
 			   display_help(current, buff);
 			   up=1;
 			   continue;
 		    } else {
 			   current-=2;
 		    }
      	} else if(IsPrint(c)){
    		ShowCursor(false);
    		buff[strlen(buff)]=c;
    		buff[strlen(buff)]=0;
    	}
    	routinechecks(1);
		ShowCursor(false);
    	for(i=0;*help[i]!='Z';i+=2){
    	   int k=0;
    	   for(int j=0;j<strlen(buff);j++)if(toupper(buff[j])!=help[i][j])k=1;
  	       if(k==0 && strlen(buff)){
  	    	   current=i;
  	    	   display_help(current, buff);
  	    	   up=1;
  	    	   break;
  	       } else {
  	    	   up=0;
  	       }
    	}
    }
    ShowCursor(false);
}
// a line of help.txt without its CR LF in buff; 0 at the end of the file
static int HelpLine(int fnbr, char *buff) {
	char *in = buff;
	if(FileEOF(fnbr)) return 0;
	memset(buff, 0, STRINGSIZE);
	while(!FileEOF(fnbr)) {
		char c = FileGetChar(fnbr);
		if(c == '\n') break;
		if(c == '\r') continue;
		if(in < buff + STRINGSIZE - 1) *in++ = c;
	}
	return 1;
}

// text in lines of the console's width, broken at spaces (the PicoMite's)
static int printWrappedText(const char *text, int screenWidth, int listcnt, int all) {
	int length = strlen(text), start = 0;
	char buff[STRINGSIZE];
	if(length == 0) ListNewLine(&listcnt, all);
	while(start < length) {
		int end = start + screenWidth, lastSpace = -1;
		if(end >= length) {
			MMPrintString((char *)text + start);
			ListNewLine(&listcnt, all);
			break;
		}
		for(int i = start; i < end; i++) if(text[i] == ' ') lastSpace = i;
		int n = lastSpace != -1 ? lastSpace - start : screenWidth;
		memcpy(buff, text + start, n); buff[n] = 0;
		MMPrintString(buff);
		ListNewLine(&listcnt, all);
		start += n + (lastSpace != -1);                             // (over the space it broke at)
	}
	return listcnt;
}

// HELP topic with help.txt: each topic is a line ~NAME and the lines of text after it; every topic whose name matches
// (* and ? are wildcards) is printed.  The PicoMite's, which rewinds over the next topic's line when a topic's text
// ends; Gerry's b14 has no rewind, so of two matching topics in a row the second was missed.  Here the line is kept.
static void HelpFile(char *topic) {
	int fnbr, ListCnt, have = 0;
	char *buff = GetTempStrMemory();
	skipspace(topic);
	if(!*topic) {
		MMPrintString("Enter help and the name of the command or function\r\nUse * for multicharacter wildcard or ? for single character wildcard\r\n");
		return;
	}
	fnbr = FindFreeFileNbr();
	if(!BasicFileOpen("/help.txt", fnbr, FA_READ)) return;
	ListCnt = CurrentY / (FontTable[gui_font >> 4][1] * (gui_font & 0b1111)) + 2;
	while(have || HelpLine(fnbr, buff)) {
		have = 0;
		if(buff[0] != '~' || !pattern_matching(topic, &buff[1], 0, 0)) continue;
		while(HelpLine(fnbr, buff)) {                               // its text, up to the next topic's line
			if(buff[0] == '~') {
				have = 1;
				ListNewLine(&ListCnt, false);
				break;
			}
			ListCnt = printWrappedText(buff, Option.Width - 1, ListCnt, false);
		}
	}
	FileClose(fnbr);
}

// HELP [topic]: the topics matching from A:/help.txt when the card has one, the built-in help otherwise.  At the
// prompt the topic is made a string (tokenise), so HELP PRINT is HELP "PRINT" and its keyword is not tokenised.
void cmd_help(void){
	char topic[STRINGSIZE] = "";
	FILINFO fnod;
	if(CurrentLinePtr) error("Invalid in a program");
	getargs(&cmdline, 1, ",");
	if(argc) strcpy(topic, getCstring(argv[0]));
	if(InitSDCard() && f_stat("0:/help.txt", &fnod) == FR_OK && !(fnod.fattrib & AM_DIR)) {
		HelpFile(topic);
		return;
	}
	do_help(topic);
	memset(cmdline,0,STRINGSIZE);
	memset(inpbuf,0,STRINGSIZE);
}

void cmd_execute(void){
	execute(cmdline);
}
void execute(char *mycmd){
//    char *temp_tknbuf;
	char *ttp;
	int i=0,toggle=0;
//    temp_tknbuf = GetTempStrMemory();
//    strcpy(temp_tknbuf, tknbuf);
    // first save the current token buffer in case we are in immediate mode
    // we have to fool the tokeniser into thinking that it is processing a program line entered at the console
    skipspace(mycmd);
	strcpy(inpbuf, getCstring(mycmd));                                      // then copy the argument
	if(!(toupper(inpbuf[0])=='R' && toupper(inpbuf[1])=='U' && toupper(inpbuf[2])=='N')){ //convert the string to upper case
		 while(inpbuf[i]){
			if(inpbuf[i]==34){
				if(toggle==0)toggle=1;
				else toggle=0;
			}
			if(!toggle){
				if(inpbuf[i]==':')error("Only single statements allowed");
				inpbuf[i]=toupper(inpbuf[i]);
			}
			i++;
		  }
		 tokenise(true);                                                 // and tokenise it (the result is in tknbuf)
		 mymemset(inpbuf,0,STRINGSIZE);
		 tknbuf[strlen(tknbuf)]=0;
		 tknbuf[strlen(tknbuf)+1]=0;
		 ttp = nextstmt;                                                 // save the globals used by commands
		 ScrewUpTimer=1000;
		 ExecuteProgram(tknbuf);                                              // execute the function's code
		 ScrewUpTimer=0;
// TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
		 nextstmt = ttp;
		 return;
    } else {
    	char *p=inpbuf;
		char *q, *s;
		char fn[FF_MAX_LFN];
		FRESULT fr;
		FILINFO fno;
		int len = strlen(p);
		char *a = commandtbl_put(p, GetCommandValue("RUN"));     // the RUN token takes the place of "RUN "
		if(len > 4) memmove(a, &p[4], len - 4); else len = 4;   // (a bare RUN has nothing to move)
		if((q=strchr(p,':'))){
			q--;
			*q='0';
		}
		p[len - 4 + sizeof(CommandToken)]=0;
		if((q=strchr(p,'\"')) != 0){
			if((s=strchr(&q[1],'\"')) != 0)*s=0;
			strcpy(fn,&q[1]);
			strcpy(fn,q);
			if(strchr(fn, '.') == NULL){
			  strcat(fn, ".BAS");
			}
			if(!InitSDCard()) {}; //make sure the SDcard is there
			fr = f_stat(&fn[1], &fno); //check for the file in the current directory
			*s='\"';
			if(fr != FR_OK || (fno.fattrib & AM_DIR)){ // file was not in the current directory so insert the path
			  int slen=strlen(p) - (uint32_t)p + (uint32_t)&q[1];
			  memmove(&q[1+strlen((char *)Option.path)],&q[1],slen+1);
			  memcpy(&q[1],(char *)Option.path,strlen((char *)Option.path));
			}
		}
		CloseAudio(1);
		strcpy(tknbuf,inpbuf);
		longjmp(run,1);
    }
}
// length of string now checked
void cmd_inc(void){
	char *p,*q;
    int vtype;
	getargs(&cmdline,3,",");
	if(argc==1){
		p = findvar(argv[0], V_FIND);
		if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
        vtype = TypeMask(FoundType());
        if(vtype & T_STR) error("Invalid variable");                // sanity check
		if(vtype & T_NBR)
            (*(MMFLOAT *)p) = (*(MMFLOAT *)p) + 1.0;
		else if(vtype & T_INT)*(long long int *)p = *(long long int *)p + 1;
		else error("Syntax");
	} else {
		p = findvar(argv[0], V_FIND);
		if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
        vtype = TypeMask(FoundType());
        if(vtype & T_STR){
        	int size=FoundSize();
        	q=getstring(argv[2]);
        	//if(*p + *q > MAXSTRLEN) error("String too long");
        	if(*p + *q > size) error("String too long");
			//Mstrcat(p, getstring(argv[2]));
        	Mstrcat(p, q);
        } else if(vtype & T_NBR){
        	 (*(MMFLOAT *)p) = (*(MMFLOAT *)p)+getnumber(argv[2]);
        } else if(vtype & T_INT){
        	*(long long int *)p = *(long long int *)p+getinteger(argv[2]);
        } else error("syntax");
 	}
}
/*
void cmd_inc(void){
	char *p, *q;
    int vtype;
	getargs(&cmdline,3,",");
	if(argc==1){
		p = findvar(argv[0], V_FIND);
		if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
        vtype = TypeMask(FoundType());
        if(vtype & T_STR) error("Invalid variable");                // sanity check
		if(vtype & T_NBR)
            (*(MMFLOAT *)p) = (*(MMFLOAT *)p) + 1.0;
		else if(vtype & T_INT)*(long long int *)p = *(long long int *)p + 1;
		else error("Syntax");
	} else {
		p = findvar(argv[0], V_FIND);
		if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
        vtype = TypeMask(FoundType());
        if(vtype & T_STR){
        	q=getstring(argv[2]);
    		if(*p + *q > MAXSTRLEN) error("String too long");
			Mstrcat(p, q);
        } else if(vtype & T_NBR){
        	 (*(MMFLOAT *)p) = (*(MMFLOAT *)p)+getnumber(argv[2]);
        } else if(vtype & T_INT){
        	*(long long int *)p = *(long long int *)p+getinteger(argv[2]);
        } else error("syntax");
 	}
}
*/
void cmd_debug(void){
	char *p;
		if((p = checkstring(cmdline, "BREAK"))) {
		char *q=GetStringMemory();
		while(1){
			MMPrintString("DEBUG> ");
			MMgetline(0, &q[1]);
			if(strncasecmp(&q[1], "CONTINUE",8)==0)break;
			*q='\"';
			strcat(q,"\"");
			execute(q);
			memset(inpbuf,0,STRINGSIZE);
		}
		FreeMemorySafe((void *)&q);
		return;
		}
		command_print(cmdline);
}
// the PRINT command
void cmd_print(void) {
	command_print(cmdline);
}
void command_print(char *cmd) {
	char *s, *p;
    unsigned char *ss;
	MMFLOAT f;
    long long int i64;
	int i, t, fnbr;
	int docrlf;														// this is used to suppress the cr/lf if needed

	getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, ";,");				// this is a macro and must be the first executable stmt

//    s = 0; *s = 56;											    // for testing the exception handler

	docrlf = true;

	if(argc > 0 && *argv[0] == '#') {								// check if the first arg is a file number
		argv[0]++;
         if(SymNameEqual(argv[0], "GPS")) {                         // #GPS (GPS may be a symbol, Symbols.h)
            if(!GPSchannel) error("GPS not activated");
            if(argc!=3) error("Only a single string parameter allowed");
            p = argv[2];
			t = T_NOTYPE;
			p = evaluate(p, &f, &i64, &s, &t, true);			// get the value and type of the argument
            ss=(unsigned char *)s;
            if(!(t & T_STR)) error("Only a single string parameter allowed");
            int i,xsum=0;
            if(ss[1]!='$' || ss[ss[0]]!='*')error("GPS command must start with dollar and end with star");
            for(i=1;i<=ss[0];i++){
                SerialPutchar(GPSchannel, s[i]);
                if(s[i]=='$')xsum=0;                                // (NMEA's checksum is of what is between the $ and the *:
                else if(s[i]!='*')xsum ^=s[i];                      // the $ was in it, so every sentence sent had a wrong one)
            }
            i=xsum/16;
            i=i+'0';
            if(i>'9')i=i-'0'+'A';
            SerialPutchar(GPSchannel, i);
            i=xsum % 16;
            i=i+'0';
            if(i>'9')i=i-'0'+'A';
            SerialPutchar(GPSchannel, i);
            SerialPutchar(GPSchannel, 13);
            SerialPutchar(GPSchannel, 10);
            return;
        } else {
            fnbr = getinteger(argv[0]);									// get the number
            i = 1;
            if(argc >= 2 && *argv[1] == ',') i = 2;						// and set the next argument to be looked at
        }
	} else {
		fnbr = 0;													// no file number so default to the standard output
		i = 0;
	}

	for(; i < argc; i++) {											// step through the arguments
		if(*argv[i] == ',') {
			MMfputc('\t', fnbr);									// print a tab for a comma
			docrlf = false;                                         // a trailing comma should suppress CR/LF
		}
		else if(*argv[i] == ';') {
			docrlf = false;											// other than suppress cr/lf do nothing for a semicolon
		}
		else {														// we have a normal expression
			p = argv[i];
			while(*p) {
				t = T_NOTYPE;
				p = evaluate(p, &f, &i64, &s, &t, true);			// get the value and type of the argument
                if(t & T_NBR) {
                    *inpbuf = ' ';                                  // preload a space
                    FloatToStr(inpbuf + ((f >= 0) ? 1:0), f, 0, STR_AUTO_PRECISION, ' ');// if positive output a space instead of the sign
					MMfputs(CtoM(inpbuf), fnbr);					// convert to a MMBasic string and output
				} else if(t & T_INT) {
                    *inpbuf = ' ';                                  // preload a space
                    IntToStr(inpbuf + ((i64 >= 0) ? 1:0), i64, 10); // if positive output a space instead of the sign
					MMfputs(CtoM(inpbuf), fnbr);					// convert to a MMBasic string and output
				} else if(t & T_STR) {
					MMfputs(s, fnbr);								// print if a string (s is a MMBasic string)
				} else error("Attempt to print reserved word");								// trap things like PRINT AS
			}
			docrlf = true;
		}
	}
	if(docrlf && AutoLineWrap) MMfputs("\2\r\n", fnbr);								// print the terminating cr/lf unless it has been suppressed
	if(PrintPixelMode!=0)SerUSBPutS("\033[m");
	PrintPixelMode=0;
	AutoLineWrap=true;
}



// the LET command
// because the LET is implied (ie, line does not have a recognisable command)
// it ends up as the place where mistyped commands are discovered.  This is why
// the error message is "Unknown command"
// a() = b(): both existing whole arrays of the same type (and string LENGTH, or structure type) with the same
// number of elements (not necessarily the same shape: a 2x3 can fill a 6), copied in one go (the PicoMite's);
// b() can be a FUNCTION that returns an array, a() = f(...), the one place such a call is allowed
static void LetArrayCopy(char *p_equals) {
	void *dst, *src;
	int dstidx, dstcount, srccount, srctype, srcsize = 0, esize, d, fidx = -1;
	char *p, *tq;

	// the destination must exist: plain V_FIND would make an undeclared a() a parameter list's empty array
	dst = findvar(cmdline, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
	if(g_StructMemberType) error("Struct member arrays not supported for this command");
	if(!emptyarray) error("Syntax");
	dstidx = VarIndex;
	if(VREC(dstidx)->type & T_CONST) error("Cannot change a constant");

	p = p_equals + 1;
	skipspace(p);
	if(!isnamestartsym(*p)) error("Expected an array");
	if(issymbol(*p)) tq = p + symbolsize(*p);                       // (a symbol, Symbols.h)
	else for(tq = p + 1; isnamechar(*tq); tq++);
	if(*tq == '$' || *tq == '%' || *tq == '!') tq++;
	if(*tq == '(') fidx = FindSubFun(p, 1);                         // the same order as getvalue: a FUNCTION before a variable
	if(fidx >= 0) {                                                 // an array FUNCTION: run it and take its result
		MMFLOAT f;
		long long int i64;
		char *s = NULL;
		int t = T_NOTYPE;
		FunReturnArrayCount = -1;                                   // (sanctions the array return: DefinedSubFun)
		DefinedSubFun(true, p, fidx, &f, &i64, &s, &t);
		if(FunReturnArrayCount <= 0) error("Expected an array");
		srccount = FunReturnArrayCount;
		FunReturnArrayCount = 0;
		src = s;
		srctype = t;                                                // (INTEGER or FLOAT)
	} else {
		src = findvar(p, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
		if(g_StructMemberType) error("Struct member arrays not supported for this command");
		if(!emptyarray) error("Expected an array");
		srctype = VREC(VarIndex)->type;
		srcsize = VREC(VarIndex)->size;
		for(srccount = 1, d = 0; d < MAXDIM && !DimIsEnd(VREC(VarIndex)->dimtbl[d]); d++) srccount *= DimElements(VREC(VarIndex)->dimtbl[d]);
	}

	// the same element type (no conversion), string LENGTH or structure type, and number of elements
	if(TypeMask(VREC(dstidx)->type) != TypeMask(srctype)) error("Incompatible type");
	if(VREC(dstidx)->type & T_STR) {
		if(VREC(dstidx)->size != srcsize) error("String length");
		esize = VREC(dstidx)->size + 1;
	} else if(VREC(dstidx)->type & T_STRUCT) {
		if(VREC(dstidx)->size != srcsize) error("Structure types must match");
		esize = g_structtbl[VREC(dstidx)->size]->total_size;
	} else
		esize = 8;
	for(dstcount = 1, d = 0; d < MAXDIM && !DimIsEnd(VREC(dstidx)->dimtbl[d]); d++) dstcount *= DimElements(VREC(dstidx)->dimtbl[d]);
	if(dstcount != srccount) error("Array size mismatch");
	if(dst != src) memcpy(dst, src, (size_t)dstcount * esize);  // (a() = a(), or a parameter its caller's array)
	checkend(skipvar(p, false));
}

void cmd_let(void) {
	int t, size, vartype;
	MMFLOAT f;
    long long int i64;
	char *s;
	char *p1, *p2;

	p1 = cmdline;

	// search through the line looking for the equals sign
	while(*p1 && tokenfunction(*p1) != op_equal) p1++;
	if(!*p1) error("Unknown command");

	// check that we have a straight forward variable
	p2 = skipvar(cmdline, false);
	skipspace(p2);
	if(p1 != p2) error("Syntax");

	// a whole array, a() = b(): only empty brackets put ( against the ) before the = (the byte before cmdline is the
	// command's token, so the look back cannot match it)
	{
		char *q = p1;
		while(*(q - 1) == ' ') q--;
		if(*(q - 1) == ')' && *(q - 2) == '(') {
			LetArrayCopy(p1);
			return;
		}
	}

	// create the variable and get the length if it is a string
	p2 = findvar(cmdline, V_FIND);
    size = VREC(VarIndex)->size;
    if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
    vartype = VREC(VarIndex)->type;
    if(g_StructMemberType != 0) {                                   // a structure's member
        vartype = g_StructMemberType;
        size = g_StructMemberSize;                                  // a string's length, a nested structure's type
    }

	// step over the equals sign, evaluate the rest of the command and save in the variable
	p1++;
	if(vartype & T_STRUCT) {                                        // a whole structure, from one of the same type
		t = T_NOTYPE;
		p1 = evaluate(p1, &f, &i64, &s, &t, false);
		if(!(t & T_STRUCT)) error("Expected a structure value");
		if(g_ExprStructType != size) error("Structure types must match");
		memmove(p2, s, g_structtbl[size]->total_size);
	}
	else if(vartype & T_STR) {
		t = T_STR;
		p1 = evaluate(p1, &f, &i64, &s, &t, false);
		if(*s > size) error("String too long");
		Mstrcpy(p2, s);
	}
	else if(vartype & T_NBR) {
		t = T_NBR;
		p1 = evaluate(p1, &f, &i64, &s, &t, false);
		if(t & T_NBR)
            (*(MMFLOAT *)p2) = f;
        else
            (*(MMFLOAT *)p2) = (MMFLOAT)i64;
	} else {
		t = T_INT;
		p1 = evaluate(p1, &f, &i64, &s, &t, false);
		if(t & T_INT)
            (*(long long int *)p2) = i64;
        else
            (*(long long int *)p2) = FloatToInt64(f);
	}
	checkend(p1);
}
int as_strcmpi (const char *s1, const char *s2)
{
  const unsigned char *p1 = (const unsigned char *) s1;
  const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;

  if (p1 == p2)
    return 0;

  do
    {
      c1 = tolower (*p1++);
      c2 = tolower (*p2++);
      if (c1 == '\0')
	break;
    }
  while (c1 == c2);

  return c1 - c2;
}

void sortStrings(char **arr, int n)
{
    char temp[16];
    int i,j;
    // Sorting strings using bubble sort
    for (j=0; j<n-1; j++)
    {
        for (i=j+1; i<n; i++)
        {
            if (as_strcmpi(arr[j], arr[i]) > 0)
            {
                strcpy(temp, arr[j]);
                strcpy(arr[j], arr[i]);
                strcpy(arr[i], temp);
            }
        }
    }
}

void do_listfiles(char *ccmdline){
	int i, j, dirs, ListCnt, currentsize;
	uint32_t currentdate;
	char *p, /**q,*/ extension[8];
	int fcnt, sortorder=0;
	char ts[FF_MAX_LFN] = {0};
    char pp[FF_MAX_LFN] = {0};
    char q[FF_MAX_LFN]={0};
	s_flist *flist;
    static DIR djd;
    static FILINFO fnod;
	mymemset(&djd,0,sizeof(DIR));
	mymemset(&fnod,0,sizeof(FILINFO));
    fcnt = 0;
    if(*ccmdline){
    	getargs(&ccmdline,3,",");
    	if(!(argc==1 || argc==3))error("Syntax");
    	p = getCstring(argv[0]);
        i=strlen(p)-1;
        while(i>0 && !(p[i] == 92 || p[i]==47))i--;
        if(i>0){
        	memcpy(q,p,i);
             for(j=0;j<strlen(q);j++)if(q[j]=='\\')q[j]='/';  //allow backslash for the DOS oldies
        	if(q[1]==':')q[0]='0';
        	i++;
        }
        strcpy(pp,&p[i]);
        if((pp[0]==47 || pp[0]==92) && i==0){
        	strcpy(q,&pp[1]);
        	strcpy(pp,q);
        	strcpy(q,"0:/");
        }
    	if(argc==3){
    		if(checkstring(argv[2], "NAME"))sortorder=0;
    		else if(checkstring(argv[2], "TIME"))sortorder=1;
    		else if(checkstring(argv[2], "SIZE"))sortorder=2;
    		else if(checkstring(argv[2], "TYPE"))sortorder=3;
    		else error("Syntax");
    	}
    }
    if(pp[0]==0)strcpy(pp,"*");
    if(CurrentLinePtr) error("Invalid in a program");
    if(!InitSDCard()) error((char *)FErrorMsg[20]);					// setup the SD card
    flist=GetMemory(sizeof(s_flist)*MAXFILES);
//    	    q = GetCWD();
   	fullpath(q);
	MMPrintString("A:");
	MMPrintString(fullpathname);
    MMPrintString("\r\n");

    // search for the first file/dir
    FSerror = f_findfirst(&djd, &fnod, fullpathname, pp);
    ErrorCheck(0);
    // add the file to the list, search for the next and keep looping until no more files
    while(FSerror == FR_OK && fnod.fname[0]) {
        if(fcnt >= MAXFILES) {
        		FreeMemorySafe((void *)&flist);
            	f_closedir(&djd);
                error("Too many files to list");
        }
        if(!(fnod.fattrib & (AM_SYS | AM_HID))){
            // add a prefix to each line so that directories will sort ahead of files
            if(fnod.fattrib & AM_DIR){
                ts[0] = 'D';
                currentdate=0xFFFFFFFF;
                fnod.fdate=0xFFFF;
                fnod.ftime=0xFFFF;
                mymemset(extension,'+',sizeof(extension));
            	extension[sizeof(extension)-1]=0;
            } else {
                ts[0] = 'F';
                currentdate=(fnod.fdate<<16) | fnod.ftime;
                if(fnod.fname[strlen(fnod.fname)-1]=='.') strcpy(extension,&fnod.fname[strlen(fnod.fname)-1]);
                else if(fnod.fname[strlen(fnod.fname)-2]=='.') strcpy(extension,&fnod.fname[strlen(fnod.fname)-2]);
                else if(fnod.fname[strlen(fnod.fname)-3]=='.') strcpy(extension,&fnod.fname[strlen(fnod.fname)-3]);
                else if(fnod.fname[strlen(fnod.fname)-4]=='.') strcpy(extension,&fnod.fname[strlen(fnod.fname)-4]);
                else if(fnod.fname[strlen(fnod.fname)-5]=='.') strcpy(extension,&fnod.fname[strlen(fnod.fname)-5]);
                else {
                	mymemset(extension,'.',sizeof(extension));
                	extension[sizeof(extension)-1]=0;
                }
           }
            currentsize=fnod.fsize;
            // and concatenate the filename found
            strcpy(&ts[1], fnod.fname);
            // sort the file name into place in the array
            if(sortorder==0){
            	for(i = fcnt; i > 0; i--) {
            		if( strcicmp((flist[i - 1].fn), (ts)) > 0)
            			flist[i] = flist[i - 1];
            		else
            			break;
            	}
            } else if(sortorder==2){
            	for(i = fcnt; i > 0; i--) {
            		if( (flist[i - 1].fs) > currentsize)
            			flist[i] = flist[i - 1];
            		else
            			break;
            	}
            } else if(sortorder==3){
            	for(i = fcnt; i > 0; i--) {
            		char e2[8];
                    if(flist[i - 1].fn[strlen(flist[i - 1].fn)-1]=='.') strcpy(e2,&flist[i - 1].fn[strlen(flist[i - 1].fn)-1]);
                    else if(flist[i - 1].fn[strlen(flist[i - 1].fn)-2]=='.') strcpy(e2,&flist[i - 1].fn[strlen(flist[i - 1].fn)-2]);
                    else if(flist[i - 1].fn[strlen(flist[i - 1].fn)-3]=='.') strcpy(e2,&flist[i - 1].fn[strlen(flist[i - 1].fn)-3]);
                    else if(flist[i - 1].fn[strlen(flist[i - 1].fn)-4]=='.') strcpy(e2,&flist[i - 1].fn[strlen(flist[i - 1].fn)-4]);
                    else if(flist[i - 1].fn[strlen(flist[i - 1].fn)-5]=='.') strcpy(e2,&flist[i - 1].fn[strlen(flist[i - 1].fn)-5]);
                    else {
                    	if(flist[i - 1].fn[0]=='D'){
                    		mymemset(e2,'+',sizeof(e2));
                        	e2[sizeof(e2)-1]=0;
                    	} else {
                    		mymemset(e2,'.',sizeof(e2));
                        	e2[sizeof(e2)-1]=0;
                    	}
                    }
            		if( strcicmp((e2), (extension)) > 0)
            			flist[i] = flist[i - 1];
            		else
            			break;
            	}
        	} else {
            	for(i = fcnt; i > 0; i--) {
            		if( ((flist[i - 1].fd<<16) |flist[i - 1].ft)  < currentdate)
            			flist[i] = flist[i - 1];
            		else
            			break;
            	}

        	}
            strcpy(flist[i].fn, ts);
            flist[i].fs = fnod.fsize;
            flist[i].fd = fnod.fdate;
            flist[i].ft = fnod.ftime;
            fcnt++;
        }
        FSerror = f_findnext(&djd, &fnod);
   }
    // list the files with a pause every screen full
	ListCnt = 2;
	for(i = dirs = 0; i < fcnt; i++) {
    	routinechecks(1);
		if(MMAbort) {
        	FreeMemorySafe((void *)&flist);
            f_closedir(&djd);
            WDTimer = 0;                                                // turn off the watchdog timer
            mymemset(inpbuf,0,STRINGSIZE);
        	SCB_CleanInvalidateDCache();
            longjmp(mark, 1);
        }
		if(flist[i].fn[0] == 'D') {
    		dirs++;
            MMPrintString("   <DIR>  ");
		}
		else {
		    IntToStrPad(ts, (flist[i].ft>>11)&0x1F, '0', 2, 10);
		    ts[2] = ':'; IntToStrPad(ts + 3, (flist[i].ft >>5)&0x3F, '0', 2, 10);
		    ts[5]=' ';
		    IntToStrPad(ts + 6, flist[i].fd & 0x1F, '0', 2, 10);
		    ts[8] = '-'; IntToStrPad(ts + 9, (flist[i].fd >> 5)&0xF, '0', 2, 10);
		    ts[11] = '-'; IntToStr(ts + 12 , ((flist[i].fd >> 9)& 0x7F )+1980, 10);
		    ts[16] =' ';
		    IntToStrPad(ts+17, flist[i].fs, ' ', 10, 10); MMPrintString(ts);
            MMPrintString("  ");
        }
        MMPrintString(flist[i].fn + 1);
		MMPrintString("\r\n");
		// check if it is more than a screen full
		if(++ListCnt >= Option.Height && i < fcnt) {
			MMPrintString("PRESS ANY KEY ...");
			MMgetchar(1);
			MMPrintString("\r                 \r");
			ListCnt = 1;
		}
	}
    // display the summary
    IntToStr(ts, dirs, 10); MMPrintString(ts);
    MMPrintString(" director"); MMPrintString(dirs==1?"y, ":"ies, ");
    IntToStr(ts, fcnt - dirs, 10); MMPrintString(ts);
    MMPrintString(" file"); MMPrintString((fcnt-dirs)==1?"":"s");
	MMPrintString("\r\n");
	FreeMemorySafe((void *)&flist);
    f_closedir(&djd);
    mymemset(inpbuf,0,STRINGSIZE);
	SCB_CleanInvalidateDCache();
    longjmp(mark, 1);
}
void cmd_listfiles(void){
	do_listfiles(cmdline);
}
void PIntFixed(int64_t n, int m) {
    char s[20];
    IntToStr(s, (int64_t)n, 10);
    for(int i=strlen(s); i<m; i++)putConsole(' ');
    MMPrintString(s);
    MMPrintString(",");
}
void PFltFixed(MMFLOAT flt){
	   char s[20];
	   FloatToStr(s, flt, 10,1, ' ');
	    MMPrintString(s);
	    MMPrintString(",");
}
void PIntFixedFile(int64_t n, int m, int fnbr) {
    char s[20];
    IntToStr(s, (int64_t)n, 10);
    for(int i=strlen(s); i<m; i++)FilePutChar(' ',fnbr);
    FilePutStr(strlen(s), s, fnbr);
    FilePutStr(1, ",", fnbr);
}
void PFltFixedFile(MMFLOAT flt, int fnbr){
	   char s[20];
	   FloatToStr(s, flt, 10,1, ' ');
	   FilePutStr(strlen(s), s, fnbr);
	   FilePutStr(1, ",", fnbr);
}


void MIPS16 cmd_list(void) {
	char *p, *tp;
	int i,j,k,m, step=5;
	hidecursor(0);
	FreeMemorySafe((void *)&cursorsave);
	cursorenable=0;
    int maxW=PageTable[WritePage].xmax;
    if((p = checkstring(cmdline, "ALL"))) {
        if(!(*p == 0 || *p == '\'')) {
        	getargs(&p,1,",");
			char *qq=GetTempStrMemory();
        	char *buff=GetTempStrMemory();
        	strcpy(buff,getCstring(argv[0]));
			getfullfilepath(buff,qq);
    		if(strchr(qq, '.') == NULL) strcat(buff, ".BAS");
            ListFile(qq, true);
        } else {
        	ListProgram((char *)ProgMemory, true);
        	checkend(p);
        }
        cleanend();
    } else if((p = checkstring(cmdline, "FLASH ALL"))) {
        ListProgramFlash((char *)ProgMemory, true);
        checkend(p);
    } else if(checkstring(cmdline, "PROFILE CSV")) {
    	if(Option.profile==0)error("Profiling not enabled");
        static uint32_t *lastprofile=NULL;
    	char b[STRINGSIZE],fn[FF_MAX_LFN]={0};
    	char *pp;
    	p=ProgMemory;
    	if(!ImageIsCurrent())return;
        int fnbr=0;
    	while(!(*p == 0 || *p == 0xff)) {                               // normally a LIST ends at the break so this is a safety precaution
            if(*p == T_NEWLINE) {
				uint32_t *profileadd=(uint32_t *)((uint32_t)(p-firststmt+(G1Hardware? 0xD0300000 : 0xD0700000)) & (~3));
				if((uint32_t)profileadd-(uint32_t)lastprofile==4)profileadd++;
				lastprofile=profileadd;
				if(p!=firststmt && LHDR_LINE(p)==0){                // an #INCLUDE file's name line
					p = NextLine(p);
					continue;
				}
				int lfile = LHDR_FILE(p), lline = LHDR_LINE(p);
				if(p!=firststmt){
					PIntFixedFile(profileadd[0],10, fnbr);
					PFltFixedFile((MMFLOAT)profileadd[1]/(MMFLOAT)profileadd[0], fnbr);
					FilePutStr(3,"  \"",fnbr);
				} else {
					strcpy(fn,ProgName());
					fn[strlen(fn)-3]='C';
					fn[strlen(fn)-2]='S';
					fn[strlen(fn)-1]='V';
					if(!CurrentLinePtr){
						MMPrintString("Creating ");
						MMPrintString(fn);
						PRet();
					}
				    fnbr = FindFreeFileNbr();
			        if(!BasicFileOpen(fn, fnbr, FA_WRITE | FA_CREATE_ALWAYS)) return;
				}
    			p = llist(b, p);                                        // otherwise expand the line
    			pp = b;
				while(*pp) {
					if((*pp=='\'' && pp[1]=='/')){
						pp++;
					}
					else FilePutChar(*pp++,fnbr);
				}
				if(lline){                                          // the line's source: "text",,line or "text",file,line
					FilePutChar('"',fnbr);
					FilePutChar(',',fnbr);
					if(lfile > 1) FilePutStr(strlen(LineFileName(lfile)),LineFileName(lfile),fnbr);
					FilePutChar(',',fnbr);
					IntToStr(b,lline,10);
					FilePutStr(strlen(b),b,fnbr);
				}
				FilePutStr(2,"\r\n",fnbr);
				routinechecks(1);
    			if(p[0] == 0 && p[1] == 0) break;                       // end of the listing ?
    		}
    	}
        FileClose(fnbr);
        checkend(p);
    } else if(checkstring(cmdline, "PROFILE")) {
        if(CurrentLinePtr) error("Invalid in a program");
    	if(Option.profile==0)error("Profiling not enabled");
        static uint32_t *lastprofile=NULL;
    	char b[STRINGSIZE];
    	char *pp;
        int ListCnt = 1;
    	p=ProgMemory;
    	if(!ImageIsCurrent())return;
    	while(!(*p == 0 || *p == 0xff)) {                               // normally a LIST ends at the break so this is a safety precaution
            if(*p == T_NEWLINE) {
				uint32_t *profileadd=(uint32_t *)((uint32_t)(p-firststmt+(G1Hardware? 0xD0300000 : 0xD0700000)) & (~3));
				if((uint32_t)profileadd-(uint32_t)lastprofile==4)profileadd++;
				lastprofile=profileadd;
				if(p!=firststmt && LHDR_LINE(p)==0){                // an #INCLUDE file's name line
					p = NextLine(p);
					continue;
				}
				int lfile = LHDR_FILE(p), lline = LHDR_LINE(p);
				if(p!=firststmt){
					PIntFixed(profileadd[0],10);
					PFltFixed((MMFLOAT)profileadd[1]/(MMFLOAT)profileadd[0]);
					MMPrintString("  \"");
				}
    			p = llist(b, p);                                        // otherwise expand the line
    			pp = b;
				while(*pp) {
					if(MMCharPos >= Option.Width) ListNewLine(&ListCnt, 0);
					if((*pp=='\'' && pp[1]=='/')){
						pp++;
					}
					else MMputchar(*pp++);
				}
				if(lline){                                          // the line's source: "text",,line or "text",file,line
					char src[FF_MAX_LFN + 16];
					strcpy(src, "\",");
					if(lfile > 1) strncat(src, LineFileName(lfile), FF_MAX_LFN - 1);
					strcat(src, ",");
					IntToStr(src + strlen(src), lline, 10);
					for(pp = src; *pp; ) {
						if(MMCharPos >= Option.Width) ListNewLine(&ListCnt, 0);
						MMputchar(*pp++);
					}
				}
				routinechecks(1);
                ListNewLine(&ListCnt, 0);
    			if(p[0] == 0 && p[1] == 0) break;                       // end of the listing ?
    		}
    	}
        checkend(p);
    } else if((tp = checkstring(cmdline,"FILES"))){
    	do_listfiles(tp);
    } else if((tp = checkstring(cmdline,"PAGES"))){
    	PO("MODE ",3);PInt(VideoMode);putConsole(',');PInt(VideoColour);MMPrintString(" has ");PInt(LastPage+1);MMPrintString(" pages\r\n");
    	MMPrintString("Page no.   Page Address   Width   Height   Size   Lines");PRet();
    	for(int i=0;i<=LastPage;i++){
    		putConsole(' ');
    		if(i<10)putConsole(' ');
    		PInt(i);
    		MMPrintString("         &H");
    		PIntH((uint32_t)PageTable[i].address);
    		if(PageTable[i].xmax<1000)MMPrintString("     ");
    		else MMPrintString("    ");
    		PInt((uint32_t)PageTable[i].xmax);
    		MMPrintString("     ");
    		PInt((uint32_t)PageTable[i].ymax);
    		if(PageTable[i].size<0xFFFF)putConsole(' ');
    		MMPrintString("    &H");
    		PIntH((uint32_t)PageTable[i].size);
    		if(PageTable[i].size<0xFFFFF)MMPrintString("   ");
    		else MMPrintString("  ");
    		putConsole(PageTable[i].expand ? '2' : '1');
    		PRet();
    	}
    } else if((p = checkstring(cmdline, "TYPE"))) {
        ListTypes(p);                                               // LIST TYPE [name] (Struct.c)
    } else if((p = checkstring(cmdline, "FLASH"))) {
        ListProgramFlash((char *)ProgMemory, false);
        checkend(p);
    } else if((p = checkstring(cmdline, "COMMANDS"))) {
    	step=maxW/gui_font_width/20;
    	m=0;
    	static const char *const extra[] = {"ELSE IF", "END IF", "Exit Do", "Cat", "Sprite", "Bit(", "Byte(", "Flag("};   // the spellings the tokeniser turns into other commands
    	int x = sizeof(extra) / sizeof(extra[0]);
		char** c=GetTempMemory((CommandTableSize+x)*sizeof(*c)+(CommandTableSize+x)*20);
		for(i=0;i<CommandTableSize+x;i++){
				if(i<CommandTableSize && str_equal((char *)commandtbl[i].name, "BitByteFlag(")) continue;   // the fold's token: its three names are listed instead
				c[m]= (char *)((int)c + sizeof(char *) * (CommandTableSize+x) + m*20);
    			if(i<CommandTableSize)strcpy(c[m],commandtbl[i].name);
    			else strcpy(c[m],extra[i-CommandTableSize]);
    			m++;
		}
    	sortStrings(c,m);
    	for(i=1;i<m;i+=step){
    		for(k=0;k<step;k++){
        		if(i+k<m){
        			MMPrintString(c[i+k]);
        			if(k!=(step-1))for(j=strlen(c[i+k]);j<19;j++)putConsole(' ');
        		}
    		}
    		MMPrintString("\r\n");
    	}
		MMPrintString("Total of ");PInt(m-1);MMPrintString(" commands\r\n");
		if (checkstring(cmdline, "COMMANDS V")){
			 MMPrintString("Total of ");PInt(m-1-x);MMPrintString(" tokens\r\n");
		}

      } else if((p = checkstring(cmdline, "FUNCTIONS"))) {
       	step=maxW/gui_font_width/20;
     	m=0;
     	int x=MMEND+FoldCount;                                      // the MM. names and the functions that share a token (MMBasic.c)

		char** c=GetTempMemory((TokenTableSize+x)*sizeof(*c)+(TokenTableSize+x)*20);
		for(i=0;i<TokenTableSize+x;i++){
				if(i<TokenTableSize && (str_equal((char *)tokentbl[i].name, "SChange$(") || str_equal((char *)tokentbl[i].name, "TopBottom(")
				                       || str_equal((char *)tokentbl[i].name, "BitByteFlag(")))
					continue;                                       // the folds' tokens: their functions are listed instead
				c[m]= (char *)((int)c + sizeof(char *) * (TokenTableSize+x) + m*20);
    			if(i<TokenTableSize)strcpy(c[m],tokentbl[i].name);
    			else if(i<TokenTableSize+MMEND)strcpy(c[m],overlaid_functions[i-TokenTableSize]);
    			else strcpy(c[m],FoldList[i-TokenTableSize-MMEND]);
				m++;
		}
    	sortStrings(c,m);
    	for(i=1;i<m-1;i+=step){
    		for(k=0;k<step;k++){
        		if(i+k<m-1){
        			MMPrintString(c[i+k]);
        			if(k!=(step-1))for(j=strlen(c[i+k]);j<19;j++)putConsole(' ');
        		}
    		}
    		MMPrintString("\r\n");
    	}
		MMPrintString("Total of ");PInt(m-2);MMPrintString(" functions and operators\r\n");
		if (checkstring(cmdline, "FUNCTIONS V")){
			  MMPrintString("Total of ");PInt(TokenTableSize-1);MMPrintString(" tokens\r\n");
		}

    } else {
        if(!(*cmdline == 0 || *cmdline == '\'')) {
        	getargs(&cmdline,1,",");
			char *qq=GetTempStrMemory();
        	char *buff=GetTempStrMemory();
        	strcpy(buff,getCstring(argv[0]));
			getfullfilepath(buff,qq);
    		if(strchr(qq, '.') == NULL) strcat(buff, ".BAS");
			ListFile(qq, false);
        } else {
        	ListProgram((char *)ProgMemory, false);
        	checkend(cmdline);
        }
        cleanend();
    }
}


void ListNewLine(int *ListCnt, int all) {
	MMPrintString("\r\n");
	(*ListCnt)++;
    if(!all && *ListCnt >= Option.Height-(Option.showstatus ? 2:0)) {
    	MMPrintString("PRESS ANY KEY ...");
    	MMgetchar(1);
    	MMPrintString("\r                 \r");
    	*ListCnt = 1;
    }
}
void MIPS16 ListProgramFlash(char *p, int all) {
	char b[STRINGSIZE];
	char *pp;
    int ListCnt = 1;

	while(!(*p == 0 || *p == 0xff)) {                               // normally a LIST ends at the break so this is a safety precaution
        if(*p == T_NEWLINE) {
			p = llist(b, p);                                        // otherwise expand the line
			pp = b;
			while(*pp) {
				if(MMCharPos >= Option.Width) ListNewLine(&ListCnt, all);
				MMputchar(*pp++);
			}
        	routinechecks(1);
            ListNewLine(&ListCnt, all);
			if(p[0] == 0 && p[1] == 0) break;                       // end of the listing ?
		}
	}
}



void MIPS16 ListProgram(char *pp, int all) {
	char buff[STRINGSIZE];
    FRESULT fr;
    FILINFO fno;
    int fnbr;
    int i,ListCnt = 1;
	if(pp != (char *)ProgMemory || (pp = ProgName()) == NULL)return;   // the program's own file
    fr = f_stat(pp, &fno);
    if(fr == FR_OK  && !(fno.fattrib & AM_DIR)){
    	fnbr = FindFreeFileNbr();
    	if(!BasicFileOpen(pp, fnbr, FA_READ)) return;
    	while(!FileEOF(fnbr)) {                                     // while waiting for the end of file
        	routinechecks(1);
    		mymemset(buff,0,256);
    		MMgetline(fnbr, (char *)buff);									    // get the input line
    		for(i=0;i<strlen(buff);i++)if(buff[i] == TAB) buff[i] = ' ';
    		MMPrintString(buff);
    		ListCnt+=strlen(buff)/Option.Width;
            ListNewLine(&ListCnt, all);
    	}
    	FileClose(fnbr);
    } else error("File not found");
}

void MIPS16 ListFile(char *pp, int all) {
	char buff[STRINGSIZE];
    FRESULT fr;
    FILINFO fno;
    int fnbr;
    int i,ListCnt = 1;
    fr = f_stat(pp, &fno);
    if(fr == FR_OK  && !(fno.fattrib & AM_DIR)){
    	fnbr = FindFreeFileNbr();
    	if(!BasicFileOpen(pp, fnbr, FA_READ)) return;
    	while(!FileEOF(fnbr)) {                                     // while waiting for the end of file
    		mymemset(buff,0,256);
    		MMgetline(fnbr, (char *)buff);									    // get the input line
    		for(i=0;i<strlen(buff);i++)if(buff[i] == TAB) buff[i] = ' ';
    		MMPrintString(buff);
    		ListCnt+=strlen(buff)/Option.Width;
        	routinechecks(1);
            ListNewLine(&ListCnt, all);
    	}
    	FileClose(fnbr);
    } else error("File not found");
}


static void MIPS16 RunStart(void);
void MIPS16 cmd_run(void) {
	char *c=(char *)ProgMemory;
	int maxH=PageTable[WritePage].ymax;
    int maxW=PageTable[WritePage].xmax;
	SerUSBPutS("\033[?25h");
	SerUSBPutS("\033[37m");
	SerUSBPutS("\033[m");
	MM_Delay(3);
    ScrewUpTimer=0;
	while(MMInkey()!=-1){}
	if(Option.showstatus){
	    ShortScroll=0;
		ShowCursor(false);
		int lastx=CurrentX,lasty=CurrentY;
		DrawRectangle( 0,maxH-gui_font_height-1,maxW-1,maxH-1,BLACK);
		CurrentY=lasty;
		CurrentX=lastx;
	}

	skipspace(cmdline);
	mymemset(runcmd,0,STRINGSIZE);
	memcpy(runcmd,cmdline,strlen(cmdline));
	if(*cmdline == 34){
		if(!FileLoadProgram(cmdline,1)) return;
	} else {
		if((c = ProgName()) == NULL)error("Nothing to run");
		if(!FileLoadProgram(c,0)) return;
	}
	ClearRuntime();
	RunStart();
}

// RUN and CHAIN, the program loaded and the runtime cleared: prepare it and start at its first line
static void MIPS16 RunStart(void) {
    WatchdogSet = false;
	PrepareProgram(true);
    IgnorePIN = false;
    if(*ProgMemory != T_NEWLINE) return;                             // no program to run
    setmode(Option.mode,DEFCOLOUR,0,Option.mode==12?1:0);
    if((uint32_t)PageTable[0].address==0xD0000000){
    	mymemset((char *)0x24000000,0,512*1024);
    	mymemset((char *)0xD0000000+PageTable[0].size,0,512*6*1024-PageTable[0].size);
    } else {
    	mymemset((char *)0xD0000000,0,512*6*1024);
    	mymemset((char *)0x24000000+PageTable[0].size,0,512*1024-PageTable[0].size);
    }
    if(Option.profile){
    	if(G1Hardware)mymemset((char *)0xD0300000, 0, 512*1024);
    	else mymemset((char *)0xD0700000, 0, 512*1024);
    }
    turtle_init();  //turtle
    statement_time= ReadCoreTimer() + FastTimer;
    firststmt= (char *)ProgMemory;
    nextstmt = (char *)ProgMemory;
}

// CHAIN fname$ [, cmdline$] (the PicoMite's): run another program, keeping the global variables. Everything else
// goes as on RUN (the locals, the loops, files, interrupts, pins, sprites, the display mode); the globals stay where
// they are, the heap wiped but for the blocks they own (ClearRuntimeKeepGlobals). cmdline$ is the new program's
// MM.CMDLINE$. A kept variable of a structure type refers to its type by number: both programs must define the same
// TYPEs in the same order.
void MIPS16 cmd_chain(void) {
	char fn[STRINGSIZE], args[STRINGSIZE];
	getargs(&cmdline, 3, ",");
	if(!(argc == 1 || argc == 3)) error("Argument count");
	strcpy(fn, getCstring(argv[0]));                                // (copied: the heap goes before the program is loaded)
	*args = 0;
	if(argc == 3) strcpy(args, getCstring(argv[2]));
	if(strlen(fn) + strlen(args) + 4 > STRINGSIZE - 1) error("Line is too long");
	{                                                               // the file opens, before this program is cleared away
		char t[STRINGSIZE];
		int fnbr = FindFreeFileNbr();
		strcpy(t, fn);
		if(strchr(&t[strlen(t) > 4 ? strlen(t) - 4 : 0], '.') == NULL) strcat(t, ".BAS");   // (as FileLoadProgram)
		if(!InitSDCard() || !BasicFileOpen(t, fnbr, FA_READ)) return;
		FileClose(fnbr);
	}
	mymemset(runcmd, 0, STRINGSIZE);                                // MM.CMDLINE$ reads what follows the quoted name
	strcpy(runcmd, "\"");
	strcat(runcmd, fn);
	strcat(runcmd, "\",");
	strcat(runcmd, args);
	if(!FileLoadProgram(fn, 2)) return;
	ClearRuntimeKeepGlobals();
	RunStart();
}


void MIPS16 cmd_continue(void) {
    if(*cmdline == tokenFOR) {
        if(forindex == 0) error("No FOR loop is in effect");
        nextstmt = forstack[forindex - 1].nextptr;
        return;
    }
    if(checkstring(cmdline, "DO")) {
        if(doindex == 0) error("No DO loop is in effect");
        nextstmt = dostack[doindex - 1].loopptr;
        return;
    }
    // must be a normal CONTINUE
	checkend(cmdline);
	if(CurrentLinePtr) error("Invalid in a program");
	if(ContinuePoint == NULL) error("Cannot continue");
    IgnorePIN = false;
	nextstmt = ContinuePoint;
}

void cmd_goto(void) {
	if(isnamestartsym(*cmdline))
		nextstmt = findlabel(cmdline);								// must be a label (a symbol or text)
	else
		nextstmt = findline(getinteger(cmdline), true);				// try for a line number
    IgnorePIN = false;
}

void cmd_call(void){
	int i;
	char *q;
	char *p=getCstring(cmdline); //get the command we want to call
	q=p;
	while(*q){ //convert to upper case for the match
		*q=toupper(*q);
		q++;
	}
	q=cmdline;
	while(*q){
		if(*q==',' || *q=='\'')break;
		q++;
	}
	if(*q==',')q++;
	i = FindSubFun(p, false);                   // it could be a defined command
	if(strlen(p) + 1 + strlen(q) >= STRINGSIZE) error("Line is too long");   // p is a STRINGSIZE temporary
	strcat(p," ");
	strcat(p,q);
//	MMPrintString(p);PRet();
	if(i >= 0) {                                // >= 0 means it is a user defined command
		DefinedSubFun(false, p, i, NULL, NULL, NULL, NULL);
	}
	else
		error("Unknown user subroutine");
}

/* ============================================================================
 * IF / ELSEIF / ELSE / ENDIF jump table (the PicoMite's)
 *
 * Built by PrepareProgram so that cmd_if's false branch and cmd_else can locate the matching arm or ENDIF by a binary
 * search instead of the linear GetNextCommand walk. Single-line IFs are not in it (cmd_if resolves them from argv[]);
 * only IFs whose THEN ends the line. A malformed program (ELSE without IF, an unclosed IF) leaves next_arm or
 * endif_tok NULL and the run-time scan raises the error as before. The CMM2 has the program region only.
 * ============================================================================ */
#define IFTAB_MAX_NEST 64
#define IFTAB_INITIAL_CAPACITY 32

static struct iftab_entry *iftab = NULL;
static int iftab_count = 0;
static int iftab_capacity = 0;

void IfTableFree(void) {
    if(iftab) FreeMemorySafe((void **)&iftab);
    iftab_count = 0;
    iftab_capacity = 0;
}

// Forget the table WITHOUT freeing it: InitHeap wiped the heap and its page map together, so the pointer is dangling
// and its pages already free (the PicoMite found a later free handing back pages a TYPE definition had by then)
void IfTableForget(void) {
    iftab = NULL;
    iftab_count = 0;
    iftab_capacity = 0;
}

static int iftab_grow(void) {
    int newcap = iftab_capacity ? iftab_capacity * 2 : IFTAB_INITIAL_CAPACITY;
    struct iftab_entry *n = (struct iftab_entry *)GetMemoryNull(newcap * sizeof(struct iftab_entry));
    if(!n) return 0;
    if(iftab) {
        memcpy(n, iftab, iftab_count * sizeof(struct iftab_entry));
        FreeMemorySafe((void **)&iftab);
    }
    iftab = n;
    iftab_capacity = newcap;
    return 1;
}

// append an entry, returning its index (or -1 when the heap is full)
static int iftab_append(char *tok, char *line_ptr) {
    if(iftab_count >= iftab_capacity && !iftab_grow()) return -1;
    iftab[iftab_count].tok = tok;
    iftab[iftab_count].next_arm = NULL;
    iftab[iftab_count].endif_tok = NULL;
    iftab[iftab_count].line_ptr = line_ptr;
    return iftab_count++;
}

// whether the IF at p is multi-line: its THEN is the last token on the line (cmd_if's test)
static int iftab_is_multiline_if(char *p) {
    char *tp = p + sizeof(CommandToken);
    while(*tp && *tp != tokenTHEN) tp++;
    if(*tp) tp++;
    skipspace(tp);
    return (*tp == 0 || *tp == '\'');
}

// keep prev_arm[] as long as iftab: 0 when the heap is full
static int iftab_prev_fit(int **prev_arm, int *prev_cap, int count) {
    if(iftab_capacity == *prev_cap) return 1;
    int *np = (int *)GetMemoryNull(iftab_capacity * sizeof(int));
    if(!np) return 0;
    if(*prev_arm) {
        memcpy(np, *prev_arm, count * sizeof(int));
        FreeMemorySafe((void **)prev_arm);
    }
    *prev_arm = np;
    *prev_cap = iftab_capacity;
    return 1;
}

// the entries of one region: a parallel "previous arm" chain (prev_arm[], by iftab index) lets ENDIF patch endif_tok
// for every arm of its IF without walking the closed IFs nested in it. 0 if the heap ran out.
static int iftab_build_region(char *prog) {
    if(!prog || *prog == (char)0xff || *prog == 0) return 1;
    int last_arm[IFTAB_MAX_NEST];                                   // iftab index of the latest arm of each open IF
    int sp = 0, sp_extra = 0;                                       // (beyond the cap: depth only, the scan does the rest)
    int prev_cap = 0, *prev_arm = NULL;
    char *p = prog, *line_ptr = NULL;
    while(*p != (char)0xff) {
        p = GetNextCommand(p, &line_ptr, NULL);
        if(*p == 0) break;
        CommandToken tkn = commandtbl_at(p);
        if(tkn == cmdIF) {
            if(!iftab_is_multiline_if(p)) continue;                 // a single-line IF
            if(sp >= IFTAB_MAX_NEST) { sp_extra++; continue; }
            int count = iftab_count, idx = iftab_append(p, line_ptr);
            if(idx < 0 || !iftab_prev_fit(&prev_arm, &prev_cap, count)) { FreeMemorySafe((void **)&prev_arm); return 0; }
            prev_arm[idx] = -1;
            last_arm[sp++] = idx;
        } else if(tkn == cmdELSEIF || tkn == cmdELSE) {
            if(sp_extra > 0 || sp == 0) continue;                   // malformed or beyond the cap: the scan handles it
            int prev = last_arm[sp - 1];
            int count = iftab_count, idx = iftab_append(p, line_ptr);
            if(idx < 0 || !iftab_prev_fit(&prev_arm, &prev_cap, count)) { FreeMemorySafe((void **)&prev_arm); return 0; }
            iftab[prev].next_arm = p;
            iftab[prev].line_ptr = line_ptr;                        // the line of next_arm: an error in this ELSEIF's condition names it
            prev_arm[idx] = prev;
            last_arm[sp - 1] = idx;
        } else if(tkn == cmdENDIF) {
            if(sp_extra > 0) sp_extra--;
            else if(sp > 0) {
                int last = last_arm[--sp];
                iftab[last].next_arm = p;
                iftab[last].line_ptr = line_ptr;
                for(int i = last; i >= 0; i = prev_arm[i]) iftab[i].endif_tok = p;
            }
        }
    }
    FreeMemorySafe((void **)&prev_arm);
    return 1;                                                       // an unclosed IF keeps NULLs: the scan raises "No matching ENDIF"
}

// with too little heap for the table there is no table: every IF scans, as before it existed (an error here would
// send the prompt, which rebuilds it each time it comes back, round that loop for ever)
void MIPS16 IfTableBuild(void) {
    IfTableFree();
    if(!iftab_build_region((char *)ProgMemory)) IfTableFree();     // (built in program order: already sorted)
}

struct iftab_entry *IfTableLookup(char *tok) {
    if(iftab_count == 0 || tok == NULL) return NULL;
    int lo = 0, hi = iftab_count;
    while(lo < hi) {
        int mid = (lo + hi) >> 1;
        char *m = iftab[mid].tok;
        if(m == tok) return &iftab[mid];
        if(m < tok) lo = mid + 1;
        else hi = mid;
    }
    return NULL;
}

void cmd_if(void) {
	int r, i, testgoto, testelseif;
	char ss[3];														// this will be used to split up the argument line
	char *p, *tp;
	char *rp = NULL;
	// the IF (then ELSEIF) token, the jump table's key: the statement loop's CmdTokenPtr, if that is this IF
	char *if_token_addr = (CmdTokenPtr && commandtbl_at(CmdTokenPtr) == cmdIF) ? CmdTokenPtr : NULL;

	ss[0] = tokenTHEN;
	ss[1] = tokenELSE;
	ss[2] = 0;

	testgoto = false;
	testelseif = false;

retest_an_if:
	{																// start a new block
		getargs(&cmdline, 20, ss);									// getargs macro must be the first executable stmt in a block

		if(testelseif && argc > 2) error("Unexpected text");

		// if there is no THEN token retry the test with a GOTO.  If that fails flag an error
		if(argc < 2 || *argv[1] != ss[0]) {
			if(testgoto) error("IF without THEN");
			ss[0] = tokenGOTO;
			testgoto = true;
			goto retest_an_if;
		}


		// allow for IF statements embedded inside this IF
		if(argc >= 3 && commandtbl_at(argv[2]) == cmdIF) argc = 3;  // this is IF xx=yy THEN IF ... so we want to evaluate only the first 3
		if(argc >= 5 && commandtbl_at(argv[4]) == cmdIF) argc = 5;  // this is IF xx=yy THEN cmd ELSE IF ... so we want to evaluate only the first 5

		if(argc == 4 || (argc == 5 && *argv[3] != ss[1])) error("Syntax");

		r = (getnumber(argv[0]) != 0);								// evaluate the expression controlling the if statement

		if(r) {
			// the test returned TRUE
			// first check if it is a multiline IF (ie, only 2 args)
			if(argc == 2) {
				// if multiline do nothing, control will fall through to the next line (which is what we want to execute next)
				;
			}
			else {
				// This is a standard single line IF statement
				// Because the test was TRUE we are just interested in the THEN cmd stage.
				if(*argv[1] == tokenGOTO) {
					cmdline = argv[2];
					cmd_goto();
					return;
				} else if(IsDigitinline(*argv[2])) {
					nextstmt = findline(getinteger(argv[2]), true);
				} else {
					if(argc == 5) {
						// this is a full IF THEN ELSE and the statement we want to execute is between the THEN & ELSE
						// this is handled by a special routine
						execute_one_command(argv[2]);
					} else {
						// easy - there is no ELSE clause so just point the next statement pointer to the byte after the THEN token
						for(p = cmdline; *p && *p != ss[0]; p++);	// search for the token
						nextstmt = p + 1;							// and point to the byte after
					}
				}
			}
		} else {
			// the test returned FALSE so we are just interested in the ELSE stage (if present)
			// first check if it is a multiline IF (ie, only 2 args)
			struct iftab_entry *ife = (argc == 2) ? IfTableLookup(if_token_addr) : NULL;
			if(ife && ife->next_arm) {
				// the jump table has this IF's next arm: an ELSEIF is evaluated as the scan below would, an ELSE or
				// ENDIF is stepped over
				char *target = ife->next_arm;
				if(commandtbl_at(target) == cmdELSEIF) {
					p = target + sizeof(CommandToken);
					skipspace(p);
					CurrentLinePtr = ife->line_ptr;
					if(*p == 0) error("Syntax");                    // there must be a test after the elseif
					cmdline = p;
					skipelement(p);
					nextstmt = p;
					testgoto = false;
					testelseif = true;
					if_token_addr = target;
					goto retest_an_if;
				}
				skipelement(target);
				nextstmt = target;
			} else if(argc == 2) {
				// search for the next ELSE, or ENDIF and pass control to the following line
				// if an ELSEIF is found re execute this function to evaluate the condition following the ELSEIF
				i = 1; p = nextstmt;
				while(1) {
                    p = GetNextCommand(p, &rp, "No matching ENDIF");
					CommandToken tkn = commandtbl_at(p);
					if(tkn == cmdtoken) {
						// found a nested IF command, we now need to determine if it is a single or multiline IF
						// search for a THEN, then check if only white space follows.  If so, it is multiline.
						tp = p + sizeof(CommandToken);
						while(*tp && *tp != ss[0]) tp++;
						if(*tp) tp++;								// step over the THEN
						skipspace(tp);
						if(*tp == 0 || *tp == '\'')					// yes, only whitespace follows
							i++;									// count it as a nested IF
						else										// no, it is a single line IF
							skipelement(p);							// skip to the end so that we avoid an ELSE
						continue;
					}

					if(tkn == cmdELSE && i == 1) {
						// found an ELSE at the same level as this IF.  Step over it and continue with the statement after it
						skipelement(p);
						nextstmt = p;
						break;
					}

					if(tkn == cmdELSEIF && i == 1) {
						// we have found an ELSEIF statement at the same level as our IF statement
						// setup the environment to make this function evaluate the test following ELSEIF and jump back
						// to the start of the function.  This is not very clean (it uses the dreaded goto for a start) but it works
						if_token_addr = p;                          // (its token: the key for the jump table)
						p += sizeof(CommandToken);                  // step over the token
						skipspace(p);
						CurrentLinePtr = rp;
						if(*p == 0) error("Syntax");                // there must be a test after the elseif
						cmdline = p;
						skipelement(p);
						nextstmt = p;
						testgoto = false;
						testelseif = true;
						goto retest_an_if;
					}

					if(tkn == cmdENDIF) i--;						// found an ENDIF so decrement our nested counter
					if(i == 0) {
						// found our matching ENDIF stmt.  Step over it and continue with the statement after it
						skipelement(p);
						nextstmt = p;
						break;
					}
				}
			}
			else {
				// this must be a single line IF statement
				// check if there is an ELSE on the same line
				if(argc == 5) {
					// there is an ELSE command
					if(IsDigitinline(*argv[4]))
						// and it is just a number, so get it and find the line
						nextstmt = findline(getinteger(argv[4]), true);
					else {
						// there is a statement after the ELSE clause  so just point to it (the byte after the ELSE token)
						// Find THEN, step over the command token the THEN clause starts with, then look for ELSE:
						// scanning from the start stopped at a command whose token has the ELSE token's value
						// (ERROR), so IF 0 THEN ERROR "x" ELSE ... ran "x" ELSE ... as a statement.
						for(p = cmdline; *p && *p != ss[0]; p++);	// find the THEN
						if(*p) p++;									// step over it
						skipspace(p);
						if(commandtbl_at(p) != CMD_NOTOKEN) p += sizeof(CommandToken);	// and over the command token
						for(; *p && *p != ss[1]; p++);				// then search for the ELSE
						nextstmt = p + 1;							// and point to the byte after
					}
                } else {
                    // no ELSE on a single line IF statement, so just continue with the next statement
                    skipline(cmdline);
                    nextstmt = cmdline;
                }
			}
		}
	}
}



void cmd_else(void) {
	int i;
	char *p, *tp;

	if(cmdtoken ==  cmdELSE) checkend(cmdline);
	// the jump table has this ELSE's or ELSEIF's ENDIF (the statement loop's CmdTokenPtr is its token)
	if(CmdTokenPtr && (commandtbl_at(CmdTokenPtr) == cmdELSE || commandtbl_at(CmdTokenPtr) == cmdELSEIF)) {
		struct iftab_entry *ife = IfTableLookup(CmdTokenPtr);
		if(ife && ife->endif_tok) {
			p = ife->endif_tok;
			skipelement(p);
			nextstmt = p;
			return;
		}
	}
	// search for the next ENDIF and pass control to the following line
	i = 1; p = nextstmt;

	while(1) {
        p = GetNextCommand(p, NULL, "No matching ENDIF");
		CommandToken tkn = commandtbl_at(p);
		if(tkn == cmdIF) {
			// found a nested IF command, we now need to determine if it is a single or multiline IF
			// search for a THEN, then check if only white space follows.  If so, it is multiline.
			tp = p + sizeof(CommandToken);
			while(*tp && *tp != tokenTHEN) tp++;
			if(*tp) tp++;											// step over the THEN
			skipspace(tp);
			if(*tp == 0 || *tp == '\'')								// yes, only whitespace follows
				i++;												// count it as a nested IF
		}
		if(tkn == cmdENDIF) i--;				    					// found an ENDIF so decrement our nested counter
		if(i == 0) break;											// found our matching ENDIF stmt
	}
	// found a matching ENDIF.  Step over it and continue with the statement after it
	skipelement(p);
	nextstmt = p;
}



// END [NOEND | cmd$] (the PicoMite's): a SUB MM.END runs first unless NOEND says not; without one, cmd$ runs as though
// typed at the prompt once the program has ended
char EndLine[STRINGSIZE];
int InMMEND = false;                                                // MM.END is running: an END in it ends there (RUN clears it)
void cmd_end(void) {
	getargs(&cmdline, 1, ",");
	int noend = (argc == 1 && checkstring(argv[0], "NOEND") != NULL);
	if(!noend && !InMMEND && FindSubFun("MM.END", 0) >= 0) {
		InMMEND = true;
		ExecuteProgram("MM.END\0");
	} else if(argc == 1 && !noend && !InMMEND) {
		char *s = getCstring(argv[0]);
		strcpy(EndLine, s);
	}
	InMMEND = false;
	cleanend();
}

#if !defined(LITE)

void cmd_select(void) {
    int i, type;
    char *p, *rp = NULL, *SaveCurrentLinePtr;
    void *v;
    MMFLOAT f = 0;
    long long int i64 = 0;
    char s[STRINGSIZE];

    // these are the tokens that we will be searching for
    // they are cached the first time this command is called

    type = T_NOTYPE;
    v = DoExpression(cmdline, &type);                               // evaluate the select case value
    type = TypeMask(type);
    if(type & T_NBR) f = *(MMFLOAT *)v;
    if(type & T_INT) i64 = *(long long int *)v;
    if(type & T_STR) Mstrcpy(s, (char *)v);

    // now search through the program looking for a matching CASE statement
    // i tracks the nesting level of any nested SELECT CASE commands
    SaveCurrentLinePtr = CurrentLinePtr;                            // save where we are because we will have to fake CurrentLinePtr to get errors reported correctly
    i = 1; p = nextstmt;
    while(1) {
        p = GetNextCommand(p, &rp, "No matching END SELECT");
        CommandToken tkn = commandtbl_at(p);

        if(tkn == cmdSELECT_CASE) i++;                              // found a nested SELECT CASE command, increase the nested count and carry on searching

        // is this a CASE stmt at the same level as this SELECT CASE.
        if(tkn == cmdCASE && i == 1) {
            int t;
            MMFLOAT ft, ftt;
            long long int i64t, i64tt;
            char *st, *stt;

            CurrentLinePtr = rp;                                    // and report errors at the line we are on

            // loop through the comparison elements on the CASE line.  Each element is separated by a comma.
            // The p++ at the top of the loop steps over the comma, or the first time the last byte of the
            // CASE token: step over the rest of the token here.
            p += sizeof(CommandToken) - 1;
            do {
                p++;
                skipspace(p);
                t = type;
                // check for CASE IS,  eg  CASE IS > 5  -or-  CASE > 5  and process it if it is
                // an operator can be >, <>, etc but it can also be a prefix + or - so we must not catch them
                char *isp = checkstring(p, "IS");                   // (it was kept in SaveCurrentLinePtr, which every
                                                                    // later error then reported as the line: the PicoMite's e3298b9)
                if(isp || ((tokentype(*p) & T_OPER) && !(*p == GetTokenValue("+") || *p == GetTokenValue("-")))) {
                    int o;
                    if(isp) p = isp;                                // (IS may be a 2-byte symbol)
                    skipspace(p);
                    if(tokentype(*p) & T_OPER)
                        o = *p++ - C_BASETOKEN;                     // get the operator
                    else
                        error("Syntax");
                    if(type & T_NBR) ft = f;
                    if(type & T_INT) i64t = i64;
                    if(type & T_STR) st = s;
                    while(o != E_END) p = doexpr(p, &ft, &i64t, &st, &o, &t); // get the right hand side of the expression and evaluate the operator in o
                    if(!(t & T_INT)) error("Syntax");     			// comparisons must always return an integer
                    if(i64t) {                                      // evaluates to true
                        skipelement(p);
                        nextstmt = p;
                        CurrentLinePtr = SaveCurrentLinePtr;
                        return;                                     // if we have a match just return to the interpreter and let it execute the code
                    } else {                                        // evaluates to false
                        skipspace(p);
                        continue;
                    }
                }

                // it must be either a single value (eg, "foo") or a range (eg, "foo" TO "zoo")
                // evaluate the first value
                p = evaluate(p, &ft, &i64t, &st, &t, true);
                skipspace(p);
                if(*p == tokenTO) {                      			// is there is a TO keyword?
                    p++;
                    t = type;
                    p = evaluate(p, &ftt, &i64tt, &stt, &t, false); // evaluate the right hand side of the TO expression
                    if(((type & T_NBR) && f >= ft && f <= ftt) || ((type & T_INT) && i64 >= i64t && i64 <= i64tt) || (((type & T_STR) && Mstrcmp(s, st) >= 0) && (Mstrcmp(s, stt) <= 0))) {
                        skipelement(p);
                        nextstmt = p;
                        CurrentLinePtr = SaveCurrentLinePtr;
                        return;                                     // if we have a match just return to the interpreter and let it execute the code
                    } else {
                        skipspace(p);
                        continue;                                   // otherwise continue searching
                    }
                }

                // if we got to here the element must be just a single match.  So make the test
                if(((type & T_NBR) && f == ft) ||  ((type & T_INT) && i64 == i64t) ||  ((type & T_STR) && Mstrcmp(s, st) == 0)) {
                    skipelement(p);
                    nextstmt = p;
                    CurrentLinePtr = SaveCurrentLinePtr;
                    return;                                         // if we have a match just return to the interpreter and let it execute the code
                }
                skipspace(p);
            } while(*p == ',');                                     // keep looping through the elements on the CASE line
            checkend(p);
            CurrentLinePtr = SaveCurrentLinePtr;
        }

        // test if we have found a CASE ELSE statement at the same level as this SELECT CASE
        // if true it means that we did not find a matching CASE - so execute this code
        if(tkn == cmdCASE_ELSE && i == 1) {
            p += sizeof(CommandToken);                              // step over the token
            checkend(p);
            skipelement(p);
            nextstmt = p;
            CurrentLinePtr = SaveCurrentLinePtr;
            return;
        }

        if(tkn == cmdEND_SELECT) i--;                               // found an END SELECT so decrement our nested counter

        if(i == 0) {
            // found our matching END SELECT stmt.  Step over it and continue with the statement after it
            skipelement(p);
            nextstmt = p;
            CurrentLinePtr = SaveCurrentLinePtr;
            return;
        }
    }
}


// if we have hit a CASE or CASE ELSE we must search for a END SELECT at this level and resume at that point
void cmd_case(void) {
    int i;
    char *p;

    // search through the program looking for a END SELECT statement
    // i tracks the nesting level of any nested SELECT CASE commands
    i = 1; p = nextstmt;
    while(1) {
        p = GetNextCommand(p, NULL, "No matching END SELECT");
        CommandToken tkn = commandtbl_at(p);

        if(tkn == cmdSELECT_CASE) i++;                              // found a nested SELECT CASE command, we now need to search for its END CASE

        if(tkn == cmdEND_SELECT) i--;                               // found an END SELECT so decrement our nested counter
        if(i == 0) {
            // found our matching END SELECT stmt.  Step over it and continue with the statement after it
            skipelement(p);
            nextstmt = p;
            break;
        }
    }
}

#endif

void cmd_input(void) {
	char s[STRINGSIZE];
	char *p, *sp, *tp;
	int i, fnbr;
	getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, ",;");				// this is a macro and must be the first executable stmt

	// is the first argument a file number specifier?  If so, get it
	if(argc >= 3 && *argv[0] == '#') {
		argv[0]++;
		fnbr = getinteger(argv[0]);
		i = 2;
	}
	else {
		fnbr = 0;
		// is the first argument a prompt?
		// if so, print it followed by an optional question mark
		if(argc >= 3 && *argv[0] == '"' && (*argv[1] == ',' || *argv[1] == ';')) {
			*(argv[0] + strlen(argv[0]) - 1) = 0;
			argv[0]++;
			MMPrintString(argv[0]);
			if(*argv[1] == ';') MMPrintString("? ");
			i = 2;
		} else {
			MMPrintString("? ");									// no prompt?  then just print the question mark
			i = 0;
		}
	}

	if(argc - i < 1) error("Syntax");						        // no variable to input to

	MMgetline(fnbr, inpbuf);									    // get the line
	p = inpbuf;

	// step through the variables listed for the input statement
	// and find the next item on the line and assign it to the variable
	for(; i < argc; i++) {
		sp = s;														// sp is a temp pointer into s[]
		if(*argv[i] == ',' || *argv[i] == ';') continue;
		skipspace(p);
		if(*p != 0) {
			if(*p == '"') {											// if it is a quoted string
				p++;												// step over the quote
				while(*p && *p != '"')  *sp++ = *p++;				// and copy everything upto the next quote
				while(*p && *p != ',') p++;							// then find the next comma
			} else {												// otherwise it is a normal string of characters
				while(*p && *p != ',') *sp++ = *p++;				// copy up to the comma
				while(sp > s && sp[-1] == ' ') sp--;				// and trim trailing whitespace
			}
		}
		*sp = 0;													// terminate the string
		tp = findvar(argv[i], V_FIND);								// get the variable and save its new value
        if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
		if(FoundType() & T_STR) {
    		if(strlen(s) > FoundSize()) error("String too long");
			strcpy(tp, s);
			CtoM(tp);												// convert to a MMBasic string
		} else
		if(FoundType() & T_INT) {
    		*((long long int *)tp) = strtoll(s, &sp, 10);			// convert to an integer
		}
		else
			*((MMFLOAT *)tp) = (MMFLOAT)atof(s);
		if(*p == ',') p++;
	}
}


void cmd_trace(void) {
    if(checkstring(cmdline, "ON"))
    	TraceOn = true;
    else if(checkstring(cmdline, "OFF"))
        TraceOn = false;
    else if(checkstring(cmdline, "LIST")) {
        int i;
        cmdline += 4;
        skipspace(cmdline);
        if(*cmdline == 0 || *cmdline =='\'')  //'
        	i = TRACE_BUFF_SIZE - 1;
        else
        	i = getint(cmdline, 0, TRACE_BUFF_SIZE - 1);
        i = TraceBuffIndex - i;
        if(i < 0) i += TRACE_BUFF_SIZE;
        while(i != TraceBuffIndex) {
        	TraceLines(TraceBuff[i]);
            if(++i >= TRACE_BUFF_SIZE) i = 0;
        }
    }
    else
        error("Unknown command");
}



// FOR command
void cmd_for(void) {
    int i, t, test;
    char ss[4];                                                     // this will be used to split up the argument line
    char *p, *tp;
    void *vptr;
    char *vname, vtype;
//    static char fortoken, nexttoken;

      // cache these tokens for speed
//    if(!fortoken) fortoken = cmdFOR;
//    if(!nexttoken) nexttoken = cmdNEXT;

    ss[0] = tokenEQUAL;
    ss[1] = tokenTO;
    ss[2] = tokenSTEP;
    ss[3] = 0;

    {                                                               // start a new block
        getargs(&cmdline, 7, ss);                                   // getargs macro must be the first executable stmt in a block
        if(argc < 5 || argc == 6 || *argv[1] != ss[0] || *argv[3] != ss[1]) error("FOR with misplaced = or TO");
        if(argc == 6 || (argc == 7 && *argv[5] != ss[2])) error("Syntax");

        // get the variable name and trim any spaces
        vname = argv[0];
        if(*vname && *vname == ' ') vname++;
        while(*vname && vname[strlen(vname) - 1] == ' ') vname[strlen(vname) - 1] = 0;
        vptr = findvar(argv[0], V_FIND);                            // create the variable
        if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
        vtype = TypeMask(FoundType());
        if(vtype & T_STR) error("Invalid variable");                // sanity check

        // check if the FOR variable is already in the stack and remove it if it is
        // this is necessary as the program can jump out of the loop without hitting
        // the NEXT statement and this will eventually result in a stack overflow
        for(i = 0; i < forindex ;i++) {
            if(forstack[i].var == vptr && forstack[i].level == LocalIndex) {
                while(i < forindex - 1) {
                    forstack[i].forptr = forstack[i+1].forptr;
                    forstack[i].nextptr = forstack[i+1].nextptr;
                    forstack[i].var = forstack[i+1].var;
                    forstack[i].vartype = forstack[i+1].vartype;
                    forstack[i].level = forstack[i+1].level;
                    forstack[i].tovalue.i = forstack[i+1].tovalue.i;
                    forstack[i].stepvalue.i = forstack[i+1].stepvalue.i;
                    i++;
                }
                forindex--;
                break;
            }
        }

        if(forindex == MAXFORLOOPS) error("Too many nested FOR loops");

        forstack[forindex].var = vptr;                              // save the variable index
        forstack[forindex].vartype = vtype;                         // save the type of the variable
        forstack[forindex].level = LocalIndex;                      // save the level of the variable in terms of sub/funs
        forindex++;                                                 // incase functions use for loops
        if(vtype & T_NBR) {
            *(MMFLOAT *)vptr = getnumber(argv[2]);                  // get the starting value for a float and save
            forstack[forindex - 1].tovalue.f = getnumber(argv[4]);  // get the to value and save
            if(argc == 7)
                forstack[forindex - 1].stepvalue.f = getnumber(argv[6]);// get the step value for a float and save
            else
                forstack[forindex - 1].stepvalue.f = 1.0;           // default is +1
        } else {
            *(long long int *)vptr = getinteger(argv[2]);           // get the starting value for an integer and save
            forstack[forindex - 1].tovalue.i = getinteger(argv[4]); // get the to value and save
            if(argc == 7)
                forstack[forindex - 1].stepvalue.i = getinteger(argv[6]);// get the step value for an integer and save
            else
                forstack[forindex - 1].stepvalue.i = 1;             // default is +1
        }
        forindex--;

        forstack[forindex].forptr = nextstmt + 1;                   // return to here when looping

        // now find the matching NEXT command
        t = 1; p = nextstmt;
        while(1) {
              p = GetNextCommand(p, &tp, "No matching NEXT");
            CommandToken tkn = commandtbl_at(p);
            if(tkn == cmdFOR) t++;                                  // count the FOR
            if(tkn == cmdNEXT) {                                    // is it NEXT
                // our variable as a whole name in the NEXT statement, symbol or text (NEXT JI is not NEXT I; a symbol's
                // payload is not letters, so its bytes are not compared ignoring case)
                if(SymFindName(p + sizeof(CommandToken), vname))     // found
                    t = 0;                                          // yes, found the matching NEXT
                else
                    t--;                                            // no luck, just decrement our stack counter
            }
            if(t == 0) {                                            // found the matching NEXT
                forstack[forindex].nextptr = p;                     // pointer to the start of the NEXT command
                break;
            }
        }

          // test the loop value at the start
          if(forstack[forindex].vartype & T_INT)
              test = (forstack[forindex].stepvalue.i >= 0 && *(long long int *)vptr > forstack[forindex].tovalue.i) || (forstack[forindex].stepvalue.i < 0 && *(long long int *)vptr < forstack[forindex].tovalue.i) ;
          else
              test = (forstack[forindex].stepvalue.f >= 0 && *(MMFLOAT *)vptr > forstack[forindex].tovalue.f) || (forstack[forindex].stepvalue.f < 0 && *(MMFLOAT *)vptr < forstack[forindex].tovalue.f) ;

          if(test) {
            // loop is invalid at the start, so go to the end of the NEXT command
            skipelement(p);                                         // find the command after the NEXT command
            nextstmt = p;                                           // this is where we will continue
        } else {
            forindex++;                                             // save the loop data and continue on with the command after the FOR statement
          }
    }
}

void cmd_new(void){
	int i=3;
	LineCount=0;
    if(CurrentLinePtr) error("Invalid in a program");
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
    mymemset(lastcmd,0,STRINGSIZE*4);
	SCB_CleanInvalidateDCache();
    longjmp(mark, 1);
}


void cmd_next(void) {
    int i, vindex, test;
    void *vtbl[MAXFORLOOPS];
    int vcnt;
    char *p;
    getargs(&cmdline, MAXFORLOOPS * 2, ",");                        // getargs macro must be the first executable stmt in a block

    vindex = 0;                                                     // keep lint happy

    for(vcnt = i = 0; i < argc; i++) {
        if(i & 0x01) {
            if(*argv[i] != ',') error("Syntax");
        } else
            vtbl[vcnt++] = findvar(argv[i], V_FIND | V_NOFIND_ERR); // find the variable and error if not found
    }

    loopback:
    // first search the for stack for a loop with the same variable specified on the NEXT's line
    if(vcnt) {
        for(i = forindex - 1; i >= 0; i--)
            for(vindex = vcnt - 1; vindex >= 0 ; vindex--)
                if(forstack[i].var == vtbl[vindex])
                    goto breakout;
    } else {
        // if no variables specified search the for stack looking for an entry with the same program position as
        // this NEXT statement. This cheats by using the cmdline as an identifier and may not work inside an IF THEN ELSE
        for(i = 0; i < forindex; i++) {
            p = forstack[i].nextptr + sizeof(CommandToken);         // nextptr points at the NEXT token
            skipspace(p);
            if(p == cmdline) goto breakout;
        }
    }

    error("Cannot find a matching FOR");

    breakout:

    // found a match
    // apply the STEP value to the variable and test against the TO value
    if(forstack[i].vartype & T_INT) {
        *(long long int *)forstack[i].var += forstack[i].stepvalue.i;
        test = (forstack[i].stepvalue.i >= 0 && *(long long int *)forstack[i].var > forstack[i].tovalue.i) || (forstack[i].stepvalue.i < 0 && *(long long int *)forstack[i].var < forstack[i].tovalue.i) ;
    } else {
        *(MMFLOAT *)forstack[i].var += forstack[i].stepvalue.f;
        test = (forstack[i].stepvalue.f >= 0 && *(MMFLOAT *)forstack[i].var > forstack[i].tovalue.f) || (forstack[i].stepvalue.f < 0 && *(MMFLOAT *)forstack[i].var < forstack[i].tovalue.f) ;
    }

    if(test) {
        // the loop has terminated
        // remove the entry in the table, then skip forward to the next element and continue on from there
        while(i < forindex - 1) {
            forstack[i].forptr = forstack[i+1].forptr;
            forstack[i].nextptr = forstack[i+1].nextptr;
            forstack[i].var = forstack[i+1].var;
            forstack[i].vartype = forstack[i+1].vartype;
            forstack[i].level = forstack[i+1].level;
            forstack[i].tovalue.i = forstack[i+1].tovalue.i;
            forstack[i].stepvalue.i = forstack[i+1].stepvalue.i;
            i++;
        }
        forindex--;
        if(vcnt > 0) {
            // remove that entry from our FOR stack
            for(; vindex < vcnt - 1; vindex++) vtbl[vindex] = vtbl[vindex + 1];
            vcnt--;
            if(vcnt > 0)
                goto loopback;
            else
                return;
        }

    } else {
        // we have not reached the terminal value yet, so go back and loop again
        nextstmt = forstack[i].forptr;
    }
}




/* ---------------------------------------------------------------------------
   DO fast path (the PicoMite's). A WHILE/UNTIL condition of the form "var OP number" or "number OP var" - var a
   numeric scalar, OP one of < > <= >= = <> - is resolved once to a pointer at the variable's value, so each LOOP
   compares directly instead of running the expression evaluator. The compare repeats the evaluator's own (compare()
   in Operators.c): as integers only when both sides are integers, otherwise in float, by the sign of the difference.
   The number is read by the evaluator itself, so its value and type are exactly what the normal path gets. A DO
   WHILE condition is compiled when the DO starts and a LOOP WHILE one at its first LOOP, where the evaluator would
   first read it, so an error is reported where it always was. The pointer stays valid while the DO entry exists: a
   local cannot go before its frame (which also drops the entry), and erase() calls DoFastForget for a global.
   --------------------------------------------------------------------------- */
#define DOFAST_LT 1
#define DOFAST_GT 2
#define DOFAST_LTE 3
#define DOFAST_GTE 4
#define DOFAST_EQ 5
#define DOFAST_NE 6
#define DOFAST_VARINT 1                                             // the variable is an integer
#define DOFAST_INTCMP 2                                             // both sides are integers: compare as integers
#define DOFAST_LITFIRST 4                                           // "number OP var": the difference is number - var

// a scalar name the fast path can take: no struct member, string, array or call
static char *DoFastSkipName(char *p) {
    if(issymbol(*p)) {
        const char *s;
        int l;
        p = NameView(p, &s, &l);
        if(memchr(s, '.', l)) return NULL;
    } else {
        if(!isnamestart(*p)) return NULL;
        while(isnamechar(*p)) {
            if(*p == '.') return NULL;
            p++;
        }
    }
    if(*p == '$') return NULL;
    if(*p == '%' || *p == '!') p++;
    char *q = p;
    skipspace(q);
    if(*q == '(') return NULL;
    return p;
}

// a numeric literal: the characters getvalue() takes as a number
static char *DoFastSkipNumber(char *p) {
    if(*p == '&') {
        int c = toupper((unsigned char)p[1]);
        if(c != 'H' && c != 'O' && c != 'B') return NULL;
        p += 2;
        if(!isxdigit((unsigned char)*p)) return NULL;
        while(isxdigit((unsigned char)*p)) p++;
        return p;
    }
    if(!((*p >= '0' && *p <= '9') || *p == '.')) return NULL;
    while((*p >= '0' && *p <= '9') || *p == '.' || *p == '+' || *p == '-' || *p == 'E' || *p == 'e') p++;
    return p;
}

static int DoFastOp(unsigned char c) {
    if(c < C_BASETOKEN || !(tokentype(c) & T_OPER)) return 0;
    void (*fp)(void) = tokenfunction(c);
    if(fp == op_lt) return DOFAST_LT;
    if(fp == op_gt) return DOFAST_GT;
    if(fp == op_lte) return DOFAST_LTE;
    if(fp == op_gte) return DOFAST_GTE;
    if(fp == op_equal) return DOFAST_EQ;
    if(fp == op_ne) return DOFAST_NE;
    return 0;
}

// compile the condition at p for the DO entry ds; leaves fast_state ON or OFF
static void DoFastCompile(char *p, struct s_dostack *ds, int until) {
    char *name, *lit, *litend, *q;
    int op, flags = 0;
    ds->fast_state = DOFAST_OFF;
    skipspace(p);
    if(isnamestartsym(*p)) {                                        // var OP number
        name = p;
        if((q = DoFastSkipName(p)) == NULL) return;
        skipspace(q);
        if((op = DoFastOp((unsigned char)*q)) == 0) return;
        q++;
        skipspace(q);
        lit = q;
        if((litend = DoFastSkipNumber(q)) == NULL) return;
        q = litend;
    } else {                                                        // number OP var
        lit = p;
        if((litend = DoFastSkipNumber(p)) == NULL) return;
        q = litend;
        skipspace(q);
        if((op = DoFastOp((unsigned char)*q)) == 0) return;
        q++;
        skipspace(q);
        name = q;
        if((q = DoFastSkipName(q)) == NULL) return;
        flags |= DOFAST_LITFIRST;
    }
    skipspace(q);
    if(!(*q == 0 || *q == '\'')) return;                            // more to the condition than that
    char buf[40];
    int n = litend - lit;
    if(n >= (int)sizeof(buf)) return;
    memcpy(buf, lit, n);
    buf[n] = 0;
    MMFLOAT f;
    long long int i64;
    char *s;
    int t = T_NOTYPE;
    evaluate(buf, &f, &i64, &s, &t, false);                         // the number, read exactly as the evaluator reads it
    void *var = findvar(name, V_NOFIND_NULL);
    if(var == NULL) return;                                         // not created yet: the normal path will do that
    int vt = VREC(VarIndex)->type;
    if(g_StructMemberType || (vt & T_STRUCT)) return;
    if((vt & (T_STR | T_CONST)) || !(vt & (T_INT | T_NBR)) || DimIsAllocated(RAW_DIM((*VREC(VarIndex)), 0))) return;
    if(vt & T_INT) flags |= DOFAST_VARINT;
    if((vt & T_INT) && (t & T_INT)) {
        flags |= DOFAST_INTCMP;
        ds->fast_limit.i = i64;
    } else
        ds->fast_limit.f = (t & T_INT) ? (MMFLOAT)i64 : f;
    ds->fast_var = var;
    ds->fast_varindex = VarIndex;
    ds->fast_op = op;
    ds->fast_until = until;
    ds->fast_flags = flags;
    ds->fast_state = DOFAST_ON;
}

// the condition's value (before any UNTIL), computed as the evaluator would
static inline int __attribute__((always_inline)) DoFastTest(struct s_dostack *ds) {
    long long int r;
    if(ds->fast_flags & DOFAST_INTCMP) {
        long long int v = *(long long int *)ds->fast_var, l = ds->fast_limit.i;
        if(ds->fast_op == DOFAST_EQ) return v == l;
        if(ds->fast_op == DOFAST_NE) return v != l;
        r = (ds->fast_flags & DOFAST_LITFIRST) ? l - v : v - l;
    } else {
        MMFLOAT v = (ds->fast_flags & DOFAST_VARINT) ? (MMFLOAT) * (long long int *)ds->fast_var : *(MMFLOAT *)ds->fast_var;
        MMFLOAT d = (ds->fast_flags & DOFAST_LITFIRST) ? ds->fast_limit.f - v : v - ds->fast_limit.f;
        r = (d > 0) ? 1 : (d < 0) ? -1 : 0;
    }
    switch(ds->fast_op) {
        case DOFAST_LT: return r < 0;
        case DOFAST_GT: return r > 0;
        case DOFAST_LTE: return r <= 0;
        case DOFAST_GTE: return r >= 0;
        case DOFAST_EQ: return r == 0;
        default: return r != 0;
    }
}

// ERASE/REDIM freed the global in this slot: DO conditions using it evaluate normally
void DoFastForget(int slot) {
    for(int i = 0; i < doindex; i++)
        if(dostack[i].fast_state == DOFAST_ON && dostack[i].fast_varindex == slot) dostack[i].fast_state = DOFAST_OFF;
}

void cmd_do(void) {
    int i, doUntil;
    char *p, *evalp;
    if(cmdtoken==cmdWHILE)error("Unknown command");
    // if it is a DO loop find the WHILE/UNTIL token and (if found) get a pointer to its expression (the CMM2 took only
    // WHILE: DO UNTIL x ... LOOP ignored the UNTIL and looped for ever)
    while(*cmdline && *cmdline != tokenWHILE && *cmdline != tokenUNTIL) cmdline++;
    if(*cmdline == tokenWHILE || *cmdline == tokenUNTIL) {
        doUntil = (*cmdline == tokenUNTIL);
        evalp = ++cmdline;
    } else {
        doUntil = 0;
        evalp = NULL;
    }
    // check if this loop is already in the stack and remove it if it is: the program can jump out of the loop
    // without hitting the LOOP and this would eventually overflow the stack. Only at this SUB level: a recursive call
    // reaches the same DO with the caller's entry still live (it was dropped, and the caller's LOOP found no DO)
    for(i = 0; i < doindex ;i++) {
        if(dostack[i].doptr == nextstmt && dostack[i].level == LocalIndex) {
            while(i < doindex - 1) {
                dostack[i] = dostack[i + 1];
                i++;
            }
            doindex--;
            break;
        }
    }

    // add our pointers to the top of the stack
    if(doindex == MAXDOLOOPS) error("Too many nested DO or WHILE loops");
    dostack[doindex].evalptr = evalp;
    dostack[doindex].doptr = nextstmt;
    dostack[doindex].level = LocalIndex;
    dostack[doindex].untiltest = doUntil;

    // now find the matching LOOP command
    p = DoFindLoop(nextstmt, cmdtoken, "No matching LOOP");
    dostack[doindex].loopptr = p;

    // the LOOP statement's arguments (for the conflict check and the fast path)
    char *loop_arg = p + sizeof(CommandToken);
    while(*loop_arg && (unsigned char)*loop_arg < 0x80) loop_arg++;
    if(dostack[doindex].evalptr != NULL) {
        // if this is a DO WHILE ... LOOP statement the LOOP must not have a test of its own
        if(*loop_arg == tokenWHILE) error("LOOP has a WHILE test");
        if(*loop_arg == tokenUNTIL) error("LOOP has an UNTIL test");
    }

    // the DO fast path: a DO WHILE/UNTIL condition is compiled here, a LOOP WHILE/UNTIL one at the first LOOP (where
    // the evaluator first reads it)
    dostack[doindex].fast_state = DOFAST_UNTRIED;
    if(evalp != NULL)
        DoFastCompile(evalp, &dostack[doindex], doUntil);
    else if(*loop_arg != tokenWHILE && *loop_arg != tokenUNTIL)
        dostack[doindex].fast_state = DOFAST_OFF;                   // DO ... LOOP with no condition

    doindex++;

    // do the evaluation (if there is something to evaluate) and if false go straight to the command after the LOOP
    if(dostack[doindex - 1].evalptr != NULL) {
        int condval = (getnumber(dostack[doindex - 1].evalptr) != 0);
        if(dostack[doindex - 1].untiltest) condval = !condval;      // DO UNTIL: skip the body if the condition is already true
        if(!condval) {
            doindex--;                                              // remove the entry in the table
            nextstmt = dostack[doindex].loopptr;                    // point to the LOOP statement
            skipelement(nextstmt);                                  // skip to the next command
        }
    }
}

// The LOOP that closes the DO (command token dotoken) whose nextstmt is p.
char *DoFindLoop(char *p, CommandToken dotoken, char *errmsg) {
    int i = 1;
    char *tp;
    while(1) {
        p = GetNextCommand(p, &tp, errmsg);
        if(*p == 0) return NULL;                                    // the end of the program (only when errmsg is NULL)
        CommandToken tkn = commandtbl_at(p);
        if(tkn == dotoken) i++;                                     // entered a nested DO or WHILE loop
        if(tkn == cmdLOOP) i--;                                     // exited a nested loop
        if(i == 0) return p;                                        // found our matching LOOP
    }
}

void cmd_loop(void) {
    char *p;
    int tst = 0;                                                    // initialise tst to stop the compiler from complaining
    int i;

    // search the do table for the entry of this LOOP statement, the most recent first: in a recursive SUB each call
    // has its own entry for the same LOOP, and the bottom one was the caller's (its condition never ended the loop)
    for(i = doindex - 1; i >= 0 ;i--) {
        p = dostack[i].loopptr + sizeof(CommandToken);              // loopptr points at the LOOP token
        skipspace(p);
        if(p == cmdline) {
            // found a match
            struct s_dostack *ds = &dostack[i];
            if(ds->fast_state == DOFAST_UNTRIED) {                  // a LOOP WHILE/UNTIL condition, read for the first time
                ds->fast_state = DOFAST_OFF;
                if(*cmdline == tokenWHILE || *cmdline == tokenUNTIL)
                    DoFastCompile(cmdline + 1, ds, *cmdline == tokenUNTIL);
            }
            if(ds->fast_state == DOFAST_ON) {
                tst = DoFastTest(ds);
                if(ds->fast_until) tst = !tst;
                goto do_loop_branch;
            }
            // first check if the DO statement had a WHILE/UNTIL component; if not find the test here and evaluate it
            if(dostack[i].evalptr == NULL) {                        // if it was a DO without a WHILE/UNTIL
                if((unsigned char)*cmdline >= 0x80) {               // if there is something
                    if(*cmdline != tokenWHILE && *cmdline != tokenUNTIL) error("Syntax");
                    int isUntil = (*cmdline == tokenUNTIL);
                    int condval = (getnumber(++cmdline) != 0);
                    tst = isUntil ? !condval : condval;
                } else {
                    tst = 1;                                        // and loop forever
                    checkend(cmdline);                              // make sure that there is nothing else
                }
            } else {                                                // if it was DO WHILE or DO UNTIL
                int condval = (getnumber(dostack[i].evalptr) != 0);
                tst = dostack[i].untiltest ? !condval : condval;    // UNTIL inverts: loop while the condition is false
                checkend(cmdline);                                  // make sure that there is nothing else
            }
        do_loop_branch:
            // test the expression value and reset the program pointer if we are still looping
            // otherwise remove this entry from the do stack
            if(tst)
                nextstmt = dostack[i].doptr;                        // loop again
            else
                doindex = i;                                        // the loop has ended: remove its entry, carry on after it
            return;
        }
    }
    error("LOOP without a matching DO");
}



void cmd_exitfor(void) {
	if(forindex == 0) error("No FOR loop is in effect");
	nextstmt = forstack[--forindex].nextptr;
	checkend(cmdline);
	skipelement(nextstmt);
}



void cmd_exit(void) {
	if(doindex == 0) error("No DO loop is in effect");
	nextstmt = dostack[--doindex].loopptr;
	checkend(cmdline);
	skipelement(nextstmt);
}



void cmd_error(void) {
	char *s, p[STRINGSIZE];
	if(*cmdline && *cmdline != '\'') {
		s = getCstring(cmdline);

		strcpy(p,s);
		error(p);
	}
	else
		error("");
}




// this is the Sub or Fun command
// it simply skips over text until it finds the end of it
void cmd_subfun(void) {
	char *p;
	CommandToken returntoken, errtoken;

    if(gosubindex != 0) error("No matching END declaration");       // we have hit a SUB/FUN while in another SUB or FUN
	if(cmdtoken == cmdSUB) {
	    returntoken = cmdENDSUB;
	    errtoken = cmdENDFUNCTION;
	} else {
	    returntoken = cmdENDFUNCTION;
	    errtoken = cmdENDSUB;
    }
	p = nextstmt;
	while(1) {
        p = GetNextCommand(p, NULL, "No matching END declaration");
        CommandToken tkn = commandtbl_at(p);
        if(tkn == cmdSUB || tkn == cmdFUN || tkn == errtoken) error("No matching END declaration");
		if(tkn == returntoken) {                                    // found the next return
    		skipelement(p);
    		nextstmt = p;                                           // point to the next command
    		break;
        }
    }
}




void cmd_gosub(void) {
	if(gosubindex >= MAXGOSUB) error("Too many nested GOSUB");
    errorstack[gosubindex] = CurrentLinePtr;
    substack[gosubindex] = NULL;                                    // a GOSUB runs in the SUB it was called from
	gosubstack[gosubindex++] = nextstmt;
	LocalIndex++;
	routinechecks(1);
	if(isnamestartsym(*cmdline))
		nextstmt = findlabel(cmdline);								// must be a label (a symbol or text)
	else
		nextstmt = findline(getinteger(cmdline), true);				// try for a line number
    IgnorePIN = false;
}

// BIT(var%, n) = 0|1, BYTE(s$, pos) = 0..255 and FLAG(n) = 0|1 are one token: tokenise() writes them BITBYTEFLAG(1, 2
// or 3 and a comma before the arguments (SubstFold), and LIST turns them back (ListUnfold).  The PicoMite's cmd_bit,
// cmd_byte and cmd_flag.
void cmd_bitbyteflag(void) {
	int which, value;
	getargs(&cmdline, 5, ",");
	which = argc ? getint(argv[0], 1, 3) : 0;
	if(!((which == 3 && argc == 3) || (which != 3 && argc == 5))) error("Argument count");
	while(*cmdline && tokenfunction(*cmdline) != op_equal) cmdline++;
	if(!*cmdline) error("Syntax");
	++cmdline;
	if(!*cmdline) error("Syntax");
	if(which == 1) {                                                // BIT(var%, n) = 0|1
		uint64_t *source = (uint64_t *)findvar(argv[2], V_NOFIND_ERR);
		if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
		if(!(FoundType() & T_INT)) error("Not an integer");
		uint64_t bit = (uint64_t)1 << (uint64_t)getint(argv[4], 0, 63);
		value = getint(cmdline, 0, 1);
		if(value) *source |= bit; else *source &= ~bit;
	} else if(which == 2) {                                         // BYTE(s$, pos) = 0..255
		findvar(argv[2], V_NOFIND_ERR);
		if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
		if(!(FoundType() & T_STR)) error("Not a string");
		unsigned char *sourcestring = (unsigned char *)getstring(argv[2]);
		int start = getint(argv[4], 1, sourcestring[0]);
		value = getint(cmdline, 0, 255);
		sourcestring[start] = value;
	} else {                                                        // FLAG(n) = 0|1
		uint64_t bit = (uint64_t)1 << (uint64_t)getint(argv[2], 0, 63);
		value = getint(cmdline, 0, 1);
		if(value) g_flag |= bit; else g_flag &= ~bit;
	}
}

// FLAGS = n sets all 64 flags; MM.FLAGS reads them
void cmd_flags(void) {
	while(*cmdline && tokenfunction(*cmdline) != op_equal) cmdline++;
	if(!*cmdline) error("Syntax");
	g_flag = getinteger(++cmdline);
}

// LMID(ls%(), pos [, n]) = x$: MID$ on a LONGSTRING (the PicoMite's cmd_lmid)
void cmd_lmid(void){
	unsigned char *p;
	int num = -1;
	char *lsStart = NULL;
	int64_t *dest = NULL;
	getargs(&cmdline, 5, ",");
	if(!(argc == 5 || argc == 3)) error("Argument count");
	int totalsize = (parseintegerarray(argv[0], &dest, 1, 1, NULL, true, NULL) - 1) * 8;   // the room in the long string, in bytes
	lsStart = (char *)&dest[1];
	int currentlength = dest[0];
	int start = getint(argv[2], 1, currentlength);                  // a starting point in the string
	if(argc == 5) num = getint(argv[4], 0, currentlength);
	if(start + (num < 0 ? 0 : num - 1) > currentlength) error("Selection exceeds length of string");
	start--;                                                        // position 1 is array position 0
	while(*cmdline && tokenfunction(*cmdline) != op_equal) cmdline++;
	if(!*cmdline) error("Syntax");
	++cmdline;
	if(!*cmdline) error("Syntax");
	char *value = getstring(cmdline);
	if(num == -1) num = value[0];
	p = (unsigned char *)&value[1];
	if(num == value[0]) memcpy(&lsStart[start], p, num);            // a simple substitution
	else {
		int change = value[0] - num;
		if(currentlength + change > totalsize) error("String too long");
		memmove(&lsStart[start + value[0]], &lsStart[start + num], currentlength - (start + num));   // the rest to its new place
		dest[0] += change;
		memcpy(&lsStart[start], p, value[0]);                       // then the new text
	}
}

void cmd_mid(void){
	char *p;
	getargs(&cmdline,5,",");
	findvar(argv[0], V_NOFIND_ERR);
    if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
	if(!(FoundType() & T_STR)) error("Not a string");
	int size=FoundSize();
	char *sourcestring=getstring(argv[0]);
	int start=getint(argv[2],1,sourcestring[0]);
	int num=-1;                                                     // -1: the value's own length; 0 inserts (the PicoMite's)
	if(argc==5)num=getint(argv[4],0,sourcestring[0]);
	if(start+(num<0 ? 0 : num-1)>sourcestring[0])error("Selection exceeds length of string");
	while(*cmdline && tokenfunction(*cmdline) != op_equal) cmdline++;
	if(!*cmdline) error("Syntax");
	++cmdline;
	if(!*cmdline) error("Syntax");
	char *value = getstring(cmdline);
	if(num==-1)num=value[0];
	p=&value[1];
	if(num==value[0]) memcpy(&sourcestring[start],p,num);
	else {
		int change=value[0]-num;
		if(sourcestring[0]+change>size)error("String too long");
		memmove(&sourcestring[start+value[0]],&sourcestring[start+num],sourcestring[0]-(start+num-1));
		sourcestring[0]+=change;
		memcpy(&sourcestring[start],p,value[0]);
    }
}
/*
void cmd_mid(void){
	unsigned char *p;
	getargs(&cmdline,5,",");
	findvar(argv[0], V_NOFIND_ERR);
    if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
	if(!(FoundType() & T_STR)) error("Not a string");
	char *sourcestring=getstring(argv[0]);
	int start=getint(argv[2],1,sourcestring[0]);
	int num=0;
	if(argc==5)num=getint(argv[4],1,sourcestring[0]);
	if(start+num-1>sourcestring[0])error("Selection exceeds length of string");
	while(*cmdline && tokenfunction(*cmdline) != op_equal) cmdline++;
	if(!*cmdline) error("Syntax");
	++cmdline;
	if(!*cmdline) error("Syntax");
	char *value = getstring(cmdline);
	if(num==0)num=value[0];
	p=(unsigned char *)&value[1];
	if(num==value[0]) memcpy(&sourcestring[start],p,num);
	else {
		int change=value[0]-num;
		if(sourcestring[0]+change>255)error("String too long");
		memmove(&sourcestring[start+value[0]],&sourcestring[start+num],sourcestring[0]-(start+num-1));
		sourcestring[0]+=change;
		memcpy(&sourcestring[start],p,value[0]);
	}
}
*/

void cmd_return(void) {
 	checkend(cmdline);
	if(gosubindex == 0 || gosubstack[gosubindex - 1] == NULL) error("Nothing to return to");
    ClearVars(LocalIndex--);                                        // delete any local variables
    TempMemoryIsChanged = true;                                     // signal that temporary memory should be checked
	nextstmt = gosubstack[--gosubindex];                            // return to the caller
    CurrentLinePtr = errorstack[gosubindex];
}

void cmd_endfun(void) {
 	checkend(cmdline);
	if(gosubindex == 0 || gosubstack[gosubindex - 1] != NULL) error("Nothing to return to");
	nextstmt = "\0\0\0";                                            // now terminate this run of ExecuteProgram()
}

//Updated for Optionescape
void cmd_read(void) {
    int i, j, k, len, card;
    char *p, *lineptr = NULL, *ptr;
    CommandToken datatoken;
    int vcnt, vidx, num_to_read=0;
    //Add READ SAVE and READ RESTORE
	if (checkstring(cmdline, (char*)"SAVE")) {
		if(restorepointer== MAXRESTORE - 1)error((char*)"Too many saves");
		datastore[restorepointer].SaveNextDataLine = NextDataLine;
		datastore[restorepointer].SaveNextData = NextData;
		restorepointer++;
		return;
	}
	if (checkstring(cmdline, (char*)"RESTORE")) {
		if (!restorepointer)error((char*)"Nothing to restore");
		restorepointer--;
		NextDataLine = datastore[restorepointer].SaveNextDataLine;
		NextData = datastore[restorepointer].SaveNextData;
		return;
	}

    getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, ",");                // getargs macro must be the first executable stmt in a block

    if(argc == 0) error("Syntax");
	// first count the elements and do the syntax checking
    for(vcnt = i = 0; i < argc; i++) {
        if(i & 0x01) {
            if(*argv[i] != ',') error("Syntax");
        } else {
			findvar(argv[i], V_FIND | V_EMPTY_OK);
			if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
			if(emptyarray && g_StructMemberType) error("Struct member arrays not supported for this command");   // (each element's is in a structure of its own)
			card=1;
			if(emptyarray){ //empty array
				for(k=0;k<MAXDIM;k++){
					j=(DimUpper(VREC(VarIndex)->dimtbl[k]) - OptionBase + 1);
					if(j)card *= j;
				}
			}
			num_to_read+=card;
		}
	}
    char **vtbl=GetTempMemory(num_to_read * sizeof (char *));
    int *vtype=GetTempMemory(num_to_read * sizeof (int));
    int *vsize=GetTempMemory(num_to_read * sizeof (int));
    // step through the arguments and save the pointer and type
    for(vcnt = i = 0; i < argc; i+=2) {
		ptr = vtbl[vcnt] = findvar(argv[i], V_FIND | V_EMPTY_OK);
		card=1;
		if(emptyarray){
			for(k=0;k<MAXDIM;k++){
				j=(DimUpper(VREC(VarIndex)->dimtbl[k]) - OptionBase + 1);
				if(j)card *= j;
			}
		}
		for(k=0;k<card;k++){
			if(k){
				if(FoundType() & (T_INT | T_NBR))ptr+=8;
				else ptr+=FoundSize()+1;
				vtbl[vcnt]=ptr;
			}
			vtype[vcnt] = TypeMask(FoundType());
			vsize[vcnt] = FoundSize();
			vcnt++;
		}
    }

    // setup for a search through the whole memory
    vidx = 0;
    datatoken = GetCommandValue("Data");
    p = lineptr = NextDataLine;
    if(*p == 0xff) error("No DATA to read");                        // error if there is no program

  // search looking for a DATA statement.  We keep returning to this point until all the data is found
search_again:
    while(1) {
        if(*p == 0) p++;                                            // if it is at the end of an element skip the zero marker
        if(*p == 0 /*|| *p == 0xff*/) error("No DATA to read");     // 2nd 0 so end of the program and we still need more data
        if(*p == T_NEWLINE) { lineptr = p; p += T_NEWLINE_HDR; }   // and step over the line's header
        if(*p == T_LINENBR) p += 3;
        skipspace(p);
        if(*p == T_LABEL) {                                         // if there is a label here
            p += p[1] + 2;                                          // skip over the label
            skipspace(p);                                           // and any following spaces
        }
        if(commandtbl_at(p) == datatoken) break;                    // found a DATA statement
        while(*p) p++;                                              // look for the zero marking the start of the next element
    }
    NextDataLine = lineptr;
    p += sizeof(CommandToken);                                      // step over the token
    skipspace(p);
    if(!*p || *p == '\'') { CurrentLinePtr = lineptr; error("No DATA to read"); }

        // we have a DATA statement, first split the line into arguments
        {                                                           // new block, the getargs macro must be the first executable stmt in a block
        getargs(&p, (MAX_ARG_COUNT * 2) - 1, ",");
        if((argc & 1) == 0) { CurrentLinePtr = lineptr; error("Syntax"); }
        // now step through the variables on the READ line and get their new values from the argument list
        // we set the line number to the number of the DATA stmt so that any errors are reported correctly
        while(vidx < vcnt) {
            // check that there is some data to read if not look for another DATA stmt
            if(NextData > argc) {
                skipline(p);
                NextData = 0;
                goto search_again;
            }
            CurrentLinePtr = lineptr;
            if(vtype[vidx] & T_STR) {
                char *p1, *p2;
                if(*argv[NextData] == '"') {                               // if quoted string
                  	int toggle=0;
                    for(len = 0, p1 = vtbl[vidx], p2 = argv[NextData] + 1; *p2 && *p2 != '"'; len++) {
                    	if(*p2=='\\' && p2[1]!='"' && OptionEscape)toggle^=1;
	                    if(toggle){
	                        if(*p2=='\\' && IsDigit(p2[1]) && IsDigit(p2[2]) && IsDigit(p2[3])){
	                            p2++;
	                            i=(*p2++)-48;
	                            i*=10;
	                            i+=(*p2++)-48;
	                            i*=10;
	                            i+=(*p2++)-48;
	                            if(i==0)error("Illegal escape sequence, use CHR$(0) for the Null character","$");
	                            *p1++=i;
	                        } else {
	                            p2++;
	                            switch(*p2){
	                                case '\\':
	                                    *p1++='\\';
	                                    p2++;
	                                    break;
	                                case 'a':
	                                    *p1++='\a';
	                                    p2++;
	                                    break;
	                                case 'b':
	                                    *p1++='\b';
	                                    p2++;
	                                    break;
	                                case 'e':
	                                    *p1++='\e';
	                                    p2++;
	                                    break;
	                                case 'f':
	                                    *p1++='\f';
	                                    p2++;
	                                    break;
	                                case 'n':
	                                    *p1++='\n';
	                                    p2++;
	                                    break;
	                                case 'q':
	                                    *p1++='\"';
	                                    p2++;
	                                    break;
	                                case 'r':
	                                    *p1++='\r';
	                                    p2++;
	                                    break;
	                                case 't':
	                                    *p1++='\t';
	                                    p2++;
	                                    break;
	                                case 'v':
	                                    *p1++='\v';
	                                    p2++;
	                                    break;
	                                case '&':
	                                    p2++;
	                                    if(IsxDigit(*p2) && IsxDigit(p2[1])){
	                                        i=0;
	                                        i = (i << 4) | ((toupper(*p2) >= 'A') ? toupper(*p2) - 'A' + 10 : *p2 - '0');
	                                        p++;
	                                        i = (i << 4) | ((toupper(*p2) >= 'A') ? toupper(*p2) - 'A' + 10 : *p2 - '0');
	                                        if(i==0)error("Illegal escape sequence, use CHR$(0) for the Null character","$");
	                                        p2++;
	                                        *p1++=i;
	                                    } else *p1++='x';
	                                    break;
	                                default:
	                                    *p1++=*p2++;
	                            }
	                        }
	                        toggle=0;
	                    } else *p1++ = *p2++;
                    }
                } else {                                            // else if not quoted
                    for(len = 0, p1 = vtbl[vidx], p2 = argv[NextData]; *p2 && *p2 != '\'' ; len++, p1++, p2++) {
                        if(*p2 < 0x20 || *p2 >= 0x7f) error("Invalid character");
                        *p1 = *p2;                                  // copy up to the comma
                    }
                }
                if(len > vsize[vidx]) error("String too long");
                *p1 = 0;                                            // terminate the string
                CtoM(vtbl[vidx]);                                   // convert to a MMBasic string
            }
            else if(vtype[vidx] & T_INT)
                *((long long int *)vtbl[vidx]) = getinteger(argv[NextData]); // much easier if integer variable
            else
                *((MMFLOAT *)vtbl[vidx]) = getnumber(argv[NextData]);      // same for numeric variable

            vidx++;
            NextData += 2;
        }
    }
}

#ifdef OLD
void cmd_read(void) {
    int i, j, k, len, card;
    char *p, datatoken, *lineptr = NULL, *ptr;
    int vcnt, vidx, num_to_read=0;
	if (checkstring(cmdline, "SAVE")) {
		if(restorepointer== MAXRESTORE - 1)error((char*)"Too many saves");
		datastore[restorepointer].SaveNextDataLine = NextDataLine;
		datastore[restorepointer].SaveNextData = NextData;
		restorepointer++;
		return;
	}
	if (checkstring(cmdline, "RESTORE")) {
		if (!restorepointer)error((char*)"Nothing to restore");
		restorepointer--;
		NextDataLine = datastore[restorepointer].SaveNextDataLine;
		NextData = datastore[restorepointer].SaveNextData;
		return;
	}

    getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, ",");                // getargs macro must be the first executable stmt in a block
    if(argc == 0) error("Syntax");
	// first count the elements and do the syntax checking
    for(vcnt = i = 0; i < argc; i++) {
        if(i & 0x01) {
            if(*argv[i] != ',') error("Syntax");
        } else {
			findvar(argv[i], V_FIND | V_EMPTY_OK);
			if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
			card=1;
//			PInt((uint32_t)ptr);PIntComma((uint32_t)VREC(VarIndex)->val.s);PIntComma((uint32_t)VREC(VarIndex)->dimtbl[0]);PRet();
			if(emptyarray){ //empty array
				for(k=0;k<MAXDIM;k++){
					j=(DimUpper(VREC(VarIndex)->dimtbl[k]) - OptionBase + 1);
					if(j)card *= j;
				}
			}
			num_to_read+=card;
		}
	}
    char **vtbl=GetTempMemory(num_to_read * sizeof (char *));
    int *vtype=GetTempMemory(num_to_read * sizeof (int));
    int *vsize=GetTempMemory(num_to_read * sizeof (int));

    // step through the arguments and save the pointer and type
    for(vcnt = i = 0; i < argc; i+=2) {
		ptr = vtbl[vcnt] = findvar(argv[i], V_FIND | V_EMPTY_OK);
		card=1;
		if(emptyarray){
			for(k=0;k<MAXDIM;k++){
				j=(DimUpper(VREC(VarIndex)->dimtbl[k]) - OptionBase + 1);
				if(j)card *= j;
			}
		}
		for(k=0;k<card;k++){
			if(k){
				if(VREC(VarIndex)->type & (T_INT | T_NBR))ptr+=8;
				else ptr+=VREC(VarIndex)->size+1;
				vtbl[vcnt]=ptr;
			}
			vtype[vcnt] = TypeMask(VREC(VarIndex)->type);
			vsize[vcnt] = VREC(VarIndex)->size;
			vcnt++;
		}
    }

    // setup for a search through the whole memory
    vidx = 0;
    datatoken = GetCommandValue("Data");
    p = lineptr = NextDataLine;
    if(*p == 0xff) error("No DATA to read");                        // error if there is no program

  // search looking for a DATA statement.  We keep returning to this point until all the data is found
search_again:
    while(1) {
        if(*p == 0) p++;                                            // if it is at the end of an element skip the zero marker
        if(*p == 0 || *p == 0xff) error("No DATA to read");         // end of the program and we still need more data
        if(*p == T_NEWLINE) lineptr = p++;
        if(*p == T_LINENBR) p += 3;
        skipspace(p);
        if(*p == T_LABEL) {                                         // if there is a label here
            p += p[1] + 2;                                          // skip over the label
            skipspace(p);                                           // and any following spaces
        }
        if(*p == datatoken) break;                                  // found a DATA statement
        while(*p) p++;                                              // look for the zero marking the start of the next element
    }
    NextDataLine = lineptr;
    p++;                                                            // step over the token
    skipspace(p);
    if(!*p || *p == '\'') { CurrentLinePtr = lineptr; error("No DATA to read"); }

        // we have a DATA statement, first split the line into arguments
        {                                                           // new block, the getargs macro must be the first executable stmt in a block
        getargs(&p, (MAX_ARG_COUNT * 2) - 1, ",");
        if((argc & 1) == 0) { CurrentLinePtr = lineptr; error("Syntax"); }
        // now step through the variables on the READ line and get their new values from the argument list
        // we set the line number to the number of the DATA stmt so that any errors are reported correctly
        while(vidx < vcnt) {
            // check that there is some data to read if not look for another DATA stmt
            if(NextData > argc) {
                skipline(p);
                NextData = 0;
                goto search_again;
            }
            CurrentLinePtr = lineptr;
            if(vtype[vidx] & T_STR) {
                char *p1, *p2;
                if(*argv[NextData] == '"') {                               // if quoted string
                  	int toggle=0;
                    for(len = 0, p1 = vtbl[vidx], p2 = argv[NextData] + 1; *p2 && *p2 != '"'; len++) {
						if(*p2=='|')toggle^=1;
						if(*p2=='|' && isdigit((unsigned char)p2[1])){
							if(toggle){
							p2++;
							i=(*p2++)-48;
							while(isdigit((unsigned char)*p2)){
								i*=10;
								i+=(*p2++)-48;
							}
							*p1++=i;
							if(*p2=='|')p2++;
							} else {
								p2++;
							}
							toggle=0;
						} else *p1++ = *p2++;
					}
                } else {
					int toggle=0;                                            // else if not quoted
                    for(len = 0, p1 = vtbl[vidx], p2 = argv[NextData]; *p2 && *p2 != '\'' ; len++) {
                        if(*p2 < 0x20 || *p2 >= 0x7f) error("Invalid character");
						if(*p2=='|')toggle^=1;
						if(*p2=='|' && isdigit((unsigned char)p2[1])){
							if(toggle){
							p2++;
							i=(*p2++)-48;
							while(isdigit((unsigned char)*p2)){
								i*=10;
								i+=(*p2++)-48;
							}
							*p1++=i;
							if(*p2=='|')p2++;
							} else {
								p2++;
							}
							toggle=0;
						} else *p1++ = *p2++;
                    }
                }
                if(len > vsize[vidx]) error("String too long");
                *p1 = 0;                                            // terminate the string
                CtoM(vtbl[vidx]);                                   // convert to a MMBasic string
            }
            else {
                char *p = argv[NextData];
                while(*p)  *p++ = toupper(*p);                               // all expressions must be in uppercase
				if(vtype[vidx] & T_INT)
					*((long long int *)vtbl[vidx]) = getinteger(argv[NextData]); // much easier if integer variable
				else
					*((MMFLOAT *)vtbl[vidx]) = getnumber(argv[NextData]);      // same for numeric variable
				}
            vidx++;
            NextData += 2;
        }
    }
}
#endif

void cmd_restore(void) {
	if(*cmdline == 0 || *cmdline == '\'') {
        NextDataLine = ProgMemory;
		NextData = 0;
	} else {
		skipspace(cmdline);
		if(*cmdline=='"') {
			NextDataLine = findlabel(getCstring(cmdline));
			NextData = 0;
		}
		else if(IsDigit(*cmdline) || *cmdline==GetTokenValue( (char *)"+") || *cmdline==GetTokenValue( (char *)"-")  || *cmdline=='.'){
				NextDataLine = findline(getinteger(cmdline), true);		// try for a line number
				NextData = 0;
		} else {
			void *ptr=findvar(cmdline,V_NOFIND_NULL);
			if(ptr){
				if(VREC(VarIndex)->type & T_NBR) {
					if(DimIsRealArray(VREC(VarIndex)->dimtbl[0])) {		// Not an array
						error("Syntax");
					}
					NextDataLine = findline(getinteger(cmdline), true);
				} else if(VREC(VarIndex)->type & T_INT) {
					if(DimIsRealArray(VREC(VarIndex)->dimtbl[0])) {		// Not an array
						error("Syntax");
					}
					NextDataLine = findline(getinteger(cmdline), true);
				} else {
					char *c=getCstring(cmdline);
					char b[STRINGSIZE]={0};
					char *d=b;
					while(*c){
						*d++=toupper(*c++);
					}
					NextDataLine = findlabel(b);					    // must be a label
				}
			} else if(isnamestart(*cmdline)) {
				NextDataLine = findlabel(cmdline);					    // must be a label
			}
			NextData = 0;
		}
	}
}


void cmd_lineinput(void) {
	char *vp;
	int i, fnbr;
	getargs(&cmdline, 3, ",;");										// this is a macro and must be the first executable stmt
	if(argc == 0 || argc == 2) error("Syntax");

	i = 0;
	fnbr = 0;
	if(argc == 3) {
		// is the first argument a file number specifier?  If so, get it
		if(*argv[0] == '#' && *argv[1] == ',') {
			argv[0]++;
			fnbr = getinteger(argv[0]);
		}
		else {
			// is the first argument a prompt?  if so, print it otherwise there are too many arguments
			if(*argv[1] != ',' && *argv[1] != ';') error("Syntax");
			MMfputs(getstring(argv[0]), 0);
		}
	i = 2;
	}

	if(argc - i != 1) error("Syntax");
	vp = findvar(argv[i], V_FIND);
    if(VREC(VarIndex)->type & T_CONST) error("Cannot change a constant");
	if(!(FoundType() & T_STR)) error("Invalid variable");
	MMgetline(fnbr, inpbuf);									    // get the input line
	if(strlen(inpbuf) > FoundSize()) error("String too long");
	strcpy(vp, inpbuf);
	CtoM(vp);														// convert to a MMBasic string
}



void cmd_on(void) {
	int r;
	char ss[4];													    // this will be used to split up the argument line
    char *p;

	// first check if this is:   ON KEY location
	if((p = checkstring(cmdline, "KEY")) != NULL) {
		getargs(&p,3,",");
		if(argc==1){
			if(*argv[0] == '0' && !IsDigitinline(*(argv[0]+1))){
				OnKeyGOSUB = NULL;                                      // the program wants to turn the interrupt off
			} else {
				OnKeyGOSUB = GetIntAddress(argv[0]);						    // get a pointer to the interrupt routine
				IntSignal();
			}
			return;
		} else {
			keyselect=getint(argv[0],0,255);
			if(keyselect==0){
				KeyInterrupt = NULL;                                      // the program wants to turn the interrupt off
			} else {
				if(*argv[2] == '0' && !IsDigitinline(*(argv[2]+1))){
					KeyInterrupt = NULL;                                      // the program wants to turn the interrupt off
				} else {
					KeyInterrupt = GetIntAddress(argv[2]);						    // get a pointer to the interrupt routine
					IntSignal();
				}
			}
			return;
		}
	}
    p = checkstring(cmdline, "ERROR");
    if(p) {
        if(checkstring(p, "ABORT")) {
            OptionErrorSkip = 0;
            return;
        }
        MMerrno = 0;                                                // clear the error flags
        MMerrline = 0;
        *MMErrMsg = 0;
        if(checkstring(p, "CLEAR")) return;
        if(checkstring(p, "IGNORE")) {
            OptionErrorSkip = -1;
            return;
        }
        if(checkstring(p, "RESTART")) {                             // an error resets the CMM2 (the PicoMite's)
            OptionErrorSkip = 999999;
            return;
        }
        if((p = checkstring(p, "SKIP"))) {
            // n + 1: the count drops after every statement of this loop, this one included (ExecuteProgram)
            if(*p == 0 || *p == '\'')
                OptionErrorSkip = 2;
            else
                OptionErrorSkip = getint(p, 1, 10000) + 1;
            ErrorSkipDepth = ExecDepth;
            return;
        }
        error("Syntax");
	}

	// if we got here the command must be the traditional:  ON nbr GOTO|GOSUB line1, line2,... etc

	ss[0] = tokenGOTO;
	ss[1] = tokenGOSUB;
	ss[2] = ',';
	ss[3] = 0;
	{																// start a new block
		getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, ss);				// getargs macro must be the first executable stmt in a block
		if(argc < 3 || !(*argv[1] == ss[0] || *argv[1] == ss[1])) error("Syntax");
		if(argc%2 == 0) error("Syntax");

		r = getint(argv[0], 0, 255);									// evaluate the expression controlling the statement
		if(r == 0 || r > argc/2) return;							// microsoft say that we just go on to the next line

		if(*argv[1] == ss[1]) {
			// this is a GOSUB, same as a GOTO but we need to first push the return pointer
			if(gosubindex >= MAXGOSUB) error("Too many nested GOSUB");
            errorstack[gosubindex] = CurrentLinePtr;
            substack[gosubindex] = NULL;                            // a GOSUB runs in the SUB it was called from
			gosubstack[gosubindex++] = nextstmt;
        	LocalIndex++;
		}

		if(isnamestart(*argv[r*2]))
			nextstmt = findlabel(argv[r*2]);						// must be a label
		else
			nextstmt = findline(getinteger(argv[r*2]), true);		// try for a line number
	}
    IgnorePIN = false;
}


// utility routine used by DoDim() below and other places in the interpreter
// checks if the type has been explicitly specified as in DIM FLOAT A, B, ... etc
char *CheckIfTypeSpecified(char *p, int *type, int AllowDefaultType) {
    char *tp;

    g_StructArg = -1;
    if((tp = checkstring(p, "INTEGER")) != NULL)
        *type = T_INT | T_IMPLIED;
    else if((tp = checkstring(p, "STRING")) != NULL)
        *type = T_STR | T_IMPLIED;
    else if((tp = checkstring(p, "FLOAT")) != NULL)
        *type = T_NBR | T_IMPLIED;
    else {
        tp = p;
        skipspace(tp);
        if(g_structcnt > 0 && (g_StructArg = FindStructType(tp)) >= 0) {
            *type = T_STRUCT | T_IMPLIED;                           // a TYPE's name: g_StructArg is its number
            if(issymbol(*tp)) tp += symbolsize(*tp);                // (a symbol, Symbols.h)
            else while(isnamechar(*tp)) tp++;
            skipspace(tp);
            return tp;
        }
        if(!AllowDefaultType) error("Variable type");
        tp = p;
        *type = DefaultType;                                        // if the type is not specified use the default
    }
    return tp;
}



// a DIM's initialising value at p into v, of type t: a string no longer than size (it ran into what followed)
char *SetValue(char *p, int t, void *v, int size) {
    MMFLOAT f;
    long long int i64;
    char *s;
	if(t & T_STR) {
		p = evaluate(p, &f, &i64, &s, &t, true);
		if(*s > size) error("String too long");
		Mstrcpy(v, s);
	}
	else if(t & T_NBR) {
		p = evaluate(p, &f, &i64, &s, &t, false);
		if(t & T_NBR)
            (*(MMFLOAT *)v) = f;
        else
            (*(MMFLOAT *)v) = (MMFLOAT)i64;
	} else {
		p = evaluate(p, &f, &i64, &s, &t, false);
		if(t & T_INT)
            (*(long long int *)v) = i64;
        else
            (*(long long int *)v) = FloatToInt64(f);
	}
    return p;
}



// define a variable
// DIM [AS INTEGER|FLOAT|STRING] var[(d1 [,d2,...]] [AS INTEGER|FLOAT|STRING] [, ..., ...]
// LOCAL also uses this function the routines only differ in that LOCAL can only be used in a sub/fun
// STATIC's hidden global: the name of the SUB or FUNCTION it is in, 0x1E, then the variable's own text (its
// bounds and LENGTH too).  The SUB is the innermost call frame that has a definition (a GOSUB has none), so
// neither a call made earlier in the SUB nor an interrupt changes it.  0x1E is a name character that no source
// holds (tokenise makes control characters spaces): without it STATIC C in SUB AB and STATIC BC in SUB A shared
// the global ABC.  VarName holds STRINGSIZE bytes, so the bounds and LENGTH of a long declaration fit (the
// PicoMite's holds 65, and the CMM2's copy into 65 was unchecked); findvar limits the name.  The PicoMite's
// StaticVarName (74ed383, b0f448e).
static void StaticVarName(char *VarName, char *arg) {
    char *def = NULL;
    int k, n;
    for(k = gosubindex - 1; k >= 0 && def == NULL; k--) def = substack[k];
    if(def == NULL) error("Invalid here");
    def += sizeof(CommandToken);                                    // the name as the definition spells it, without a type suffix
    skipspace(def);
    CopyName(def, VarName, &k);                                     // (a symbol or text: the name is given to findvar as text)
    VarName[k++] = 0x1e;
    n = SymExpand(VarName + k, arg, strlen(arg), STRINGSIZE - 1 - k);    // the variable's text, its symbols spelt out
    if(n < 0) error("Line is too long");
    VarName[k + n] = 0;
}

void MIPS16 cmd_dim(void) {
    int i, j, k, type, typeSave, ImpliedType = 0, VIndexSave, StaticVar = false, ImpliedStruct, StructArg;
    char *p, chSave, *chPosit, *vn;
    char VarName[STRINGSIZE];                                       // STATIC's hidden name
    void *v, *tv;

    if(*cmdline == tokenAS) cmdline++;                              // this means that we can use DIM AS INTEGER a, b, etc
    p = CheckIfTypeSpecified(cmdline, &type, true);                 // check for DIM FLOAT A, B, ...
    ImpliedType = type;
    ImpliedStruct = g_StructArg;
    {                                                               // getargs macro must be the first executable stmt in a block
        getargs(&p, (MAX_ARG_COUNT * 2) - 1, ",");
        if((argc & 0x01) == 0) error("Syntax");

        for(i = 0; i < argc; i += 2) {
            p = skipvar(argv[i], false);                            // point to after the variable
            while(!(*p == 0 || *p == tokenAS || *p == '\'' || *p == tokenEQUAL))
                p++;                                                // skip over a LENGTH keyword if there and see if we can find "AS"
            chSave = *p; chPosit = p; *p = 0;                       // save the char then terminate the string so that LENGTH is evaluated correctly
            if(chSave == tokenAS) {                                 // are we using Microsoft syntax (eg, AS INTEGER)?
                if(ImpliedType & T_IMPLIED) error("Type specified twice");
                p++;                                                // step over the AS token
                p = CheckIfTypeSpecified(p, &type, true);           // and get the type
                if(!(type & T_IMPLIED)) error("Variable type");
                StructArg = g_StructArg;
            } else
                StructArg = ImpliedStruct;

            if(cmdtoken == cmdLOCAL) {
                if(LocalIndex == 0) error("Invalid here");
                type |= V_LOCAL;                                    // local if defined in a sub/fun
            }

            if(cmdtoken == cmdSTATIC) {
                if(LocalIndex == 0) error("Invalid here");
                StaticVarName(VarName, argv[i]);
                vn = VarName;
                StaticVar = true;
            } else
                vn = argv[i];                                       // (it was copied into VarName, unchecked)

            g_StructArg = StructArg;
            v = findvar(vn, type | V_NOFIND_NULL);               // check if the variable exists
            typeSave = type;
            VIndexSave = VarIndex;
            if(v == NULL) {                                         // if not found
                g_StructArg = StructArg;
                v = findvar(vn, type | V_FIND | V_DIM_VAR);      // create the variable
                type = TypeMask(VREC(VarIndex)->type);
                VIndexSave = VarIndex;
                *chPosit = chSave;                                  // restore the char previously removed
                if(DimIsEmptyParam(VREC(VarIndex)->dimtbl[0])) error("Array dimensions");
                if(DimIsRealArray(VREC(VarIndex)->dimtbl[0])) {
                    DimUsed = true;                                 // prevent OPTION BASE from being used
                    v = VREC(VarIndex)->val.s;
                }
                while(*p && *p != '\'' && tokenfunction(*p) != op_equal) p++;   // search through the line looking for the equals sign
                if(tokenfunction(*p) == op_equal) {
                    p++;                                            // step over the equals sign
                    skipspace(p);
                    if(VREC(VIndexSave)->type & T_STRUCT) {              // DIM p AS type = (v1, v2 ...), an array's elements in turn (Struct.c)
                        for(j = 1, k = 0; k < MAXDIM && DimIsRealArray(VREC(VIndexSave)->dimtbl[k]); k++)
                            j *= DimElements(VREC(VIndexSave)->dimtbl[k]);
                        StructInit(p, VREC(VIndexSave)->size, v, j);
                    } else if(DimIsRealArray(VREC(VarIndex)->dimtbl[0]) && *p == '(') {
                        // calculate the overall size of the array
                        for(j = 1, k = 0; k < MAXDIM && !DimIsEnd(VREC(VIndexSave)->dimtbl[k]); k++) {
                            j *= (DimUpper(VREC(VIndexSave)->dimtbl[k]) + 1 - OptionBase);
                        }
                        do {
                            p++;                                    // step over the opening bracket or terminating comma
                            p = SetValue(p, type, v, VREC(VIndexSave)->size);
                            if(type & T_STR) v = (char *)v + VREC(VIndexSave)->size + 1;
                            if(type & T_NBR) v = (char *)v + sizeof(MMFLOAT);
                            if(type & T_INT) v = (char *)v + sizeof(long long int);
                            skipspace(p); j--;
                        } while(j > 0 && *p == ',');
                        if(*p != ')') error("Number of initialising values");
                        if(j != 0) error("Number of initialising values");
                    } else
                        SetValue(p, type, v, VREC(VIndexSave)->size);
                }
                type = ImpliedType;
            } else {
                if(!StaticVar) error("$ already declared", vn);
            }


            // if it is a STATIC var create a local var pointing to the global var
            if(StaticVar) {
                *chPosit = 0;                                       // the declaration without "= value", which was put back to set the
                                                                    // global ("Incompatible types": the value read as part of LENGTH;
                                                                    // the PicoMite's 74ed383)
                g_StructArg = StructArg;
                tv = findvar(argv[i], typeSave | V_LOCAL | V_NOFIND_NULL);                        // check if the local variable exists
                if(tv != NULL) error("$ already declared", argv[i]);
                g_StructArg = StructArg;
                tv = findvar(argv[i], typeSave | V_LOCAL | V_FIND | V_DIM_VAR);                   // create the variable
                if(DimIsRealArray(VREC(VIndexSave)->dimtbl[0]) || (VREC(VIndexSave)->type & (T_STR | T_STRUCT))) {
                    if(!VAR_INLINE_STR(VREC(VarIndex))) FreeMemory(tv);                           // we don't need the memory allocated to the local
                    VREC(VarIndex)->val.s = VREC(VIndexSave)->val.s;                            // point to the memory of the global variable
                } else
                    VREC(VarIndex)->val.ia = &(VREC(VIndexSave)->val.i);                        // point to the data of the variable
                VREC(VarIndex)->type = VREC(VIndexSave)->type | T_PTR;                          // set the type to a pointer
                VREC(VarIndex)->size = VREC(VIndexSave)->size;                                  // just in case it is a string copy the size
                for(j = 0; j < MAXDIM; j++) VREC(VarIndex)->dimtbl[j] = VREC(VIndexSave)->dimtbl[j];// just in case it is an array copy the dimensions
            }
        }
    }
}

void MIPS16 cmd_const(void) {
    char *p;
    void *v;
    int i, type;

	getargs(&cmdline, (MAX_ARG_COUNT * 2) - 1, ",");				// getargs macro must be the first executable stmt in a block
	if((argc & 0x01) == 0) error("Syntax");

    for(i = 0; i < argc; i += 2) {
        p = skipvar(argv[i], false);                                // point to after the variable
        skipspace(p);
        if(tokenfunction(*p) != op_equal) error("Syntax");  // must be followed by an equals sign
        p++;                                                        // step over the equals sign
        type = T_NOTYPE;
        v = DoExpression(p, &type);                                 // evaluate the constant's value
        type = TypeMask(type);
        type |= V_FIND | V_DIM_VAR | T_CONST | T_IMPLIED;
        if(LocalIndex != 0) type |= V_LOCAL;                        // local if defined in a sub/fun
        findvar(argv[i], type);                                     // create the variable
        if(DimIsAllocated(VREC(VarIndex)->dimtbl[0])) error("Invalid constant");
        if(TypeMask(VREC(VarIndex)->type) != TypeMask(type)) error("Invalid constant");
        else {
            if(type & T_NBR) VREC(VarIndex)->val.f = *(MMFLOAT *)v;           // and set its value
            if(type & T_INT) VREC(VarIndex)->val.i = *(long long int *)v;
            //if(type & T_STR) Mstrcpy(VREC(VarIndex)->val.s, (char *)v);
            // CONST string from Picomite 6.00.02RC5
            if(type & T_STR) {
 				if((char)*(char *)v<(MAXDIM-1)*sizeof(VREC(VarIndex)->dimtbl[1])){
 					if(!VAR_INLINE_STR(VREC(VarIndex))) FreeMemorySafe((void **)&VREC(VarIndex)->val.s);
 					VREC(VarIndex)->val.s=(void *)&VREC(VarIndex)->dimtbl[1];
 			   }
 			   Mstrcpy(VREC(VarIndex)->val.s, (char *)v);
            }
        }
    }
}




// ERASE's work for one name: the variable's record goes, and its memory with it unless nofree, when the old data's
// address is returned for the caller to copy from and then free (REDIM PRESERVE).  The PicoMite's erase().
uint32_t erase(char *p, int nofree) {
	int j, k, len;
	char *s, *x;
	uint32_t addr = 0;
	while(strlen(p) && !isnamechar(p[strlen(p) - 1])) p[strlen(p) - 1] = 0;   // the name without its () or dimensions
	makeupper(p);                                                   // all variables are stored as uppercase
	for(j = MAXLOCALVARS; j < MAXVARS; j++) {
		s = p;  x = VREC(j)->name; len = strlen(p);
		while(len > 0 && *s == *x) {                                // compare the variable to the name that we have
			len--; s++; x++;
		}
		if(!(len == 0 && (*x == 0 || strlen(p) == MAXVARLEN))) continue;
		// found the variable
		if(((VREC(j)->type & (T_STR | T_STRUCT)) || DimIsAllocated(VREC(j)->dimtbl[0])) && !(VREC(j)->type & T_PTR) && !VAR_INLINE_STR(VREC(j))) {
			addr = (uint32_t)VREC(j)->val.s;
			if(!nofree) FreeMemory(VREC(j)->val.s);                 // free any memory (if allocated)
			VREC(j)->val.s = NULL;
		}
		k = j + 1;
		if(k == MAXVARS) k = MAXLOCALVARS;
		VarRecordRelease(j, VREC(k)->type != T_NOTYPE);             // its record to the free list; the slot blocked if a probe may have passed it
		SymBindForgetSlot(j);                                       // and no symbol bound to it (Symbols.h)
		DoFastForget(j);                                            // a DO condition that pointed at this variable must evaluate again
		Globalvarcnt--;
		return addr;
	}
	error("Cannot find $", p);
	return 0;
}

void cmd_erasevar(char *erase_args) {
	int i;
	char p[STRINGSIZE];
	getargs(&erase_args, (MAX_ARG_COUNT * 2) - 1, ",");            // getargs macro must be the first executable stmt in a block
	if((argc & 0x01) == 0) error("Argument count");
	for(i = 0; i < argc; i += 2) {
		strcpy(p, argv[i]);
		erase(p, false);
	}
}

// REDIM's: the dimension expressions of "name(e1, e2)" are evaluated into dims, and the text becomes "name()" for
// findvar (the PicoMite's parse_and_strip, closing at the last bracket so that an index may hold a function call)
static void parse_and_strip(char *string, int *dims) {
	char *open = strchr(string, '('), *close = strrchr(string, ')');
	for(int i = 0; i < MAXDIM; i++) dims[i] = 0;
	if(open && close && close > open) {
		char buffer[STRINGSIZE], *p = buffer, *start, saved;
		int n = close - open - 1, idx = 0, depth;
		if(n >= (int)sizeof(buffer)) error("Line is too long");
		memcpy(buffer, open + 1, n); buffer[n] = 0;
		while(*p && idx < MAXDIM) {
			skipspace(p);
			if(*p == 0) break;
			start = p; depth = 0;                                   // this expression, up to a comma outside brackets
			while(*p && !(*p == ',' && depth == 0)) {
				if(*p == '(') depth++; else if(*p == ')') depth--;
				p++;
			}
			saved = *p; *p = 0;
			dims[idx++] = DimEncode(getinteger(start));
			*p = saved;
			if(*p == ',') p++;
		}
		strcpy(open + 1, ")");                                      // name()
	}
}

// true when the two sets of bounds differ only in the last one (the PicoMite's array_comp): the first index varies
// fastest, so the earlier bounds fix where each element lies and PRESERVE can keep them in place
static int array_comp(int in[MAXDIM], int out[MAXDIM]) {
	int last_in = -1, last_out = -1;
	for(int i = MAXDIM - 1; i >= 0; i--) {
		if(last_in == -1 && in[i] != 0) last_in = i;
		if(last_out == -1 && out[i] != 0) last_out = i;
	}
	if(last_in != last_out) return false;
	if(last_in == -1) return true;
	for(int i = 0; i < MAXDIM; i++)
		if(i != last_in && in[i] != out[i]) return false;
	return true;
}

// REDIM [PRESERVE] array(bounds) [, ...]: the array is made again with the new bounds, of its declared type and (a
// string's) length; with PRESERVE its contents are kept, and only the last bound may change.  The PicoMite's cmd_redim,
// with each argument copied whole (its strncpy left one of 32 characters unterminated: issue 21).
void cmd_redim(void) {
	int dims[MAXDIM], newdims[MAXDIM];
	char *oldmemory = NULL, *newmemory, *tp, old[STRINGSIZE];
	int oldsize = 0, newsize, length = -1, structIdx = -1;
	int preserve = ((tp = checkstring(cmdline, "PRESERVE")) != NULL);
	if(tp == NULL) tp = cmdline;
	{
		getargs(&tp, MAX_ARG_COUNT, ",");
		for(int i = 0; i < argc; i += 2) {
			strcpy(old, argv[i]);
			parse_and_strip(old, newdims);
			findvar(old, V_FIND | V_NOFIND_ERR | V_EMPTY_OK);
			if(VREC(VarIndex)->type & T_STR) length = VREC(VarIndex)->size;
			if(VREC(VarIndex)->type & T_STRUCT) structIdx = (int)VREC(VarIndex)->size;   // its TYPE
			if(DimIsScalar(VREC(VarIndex)->dimtbl[0])) error("$ is not an array", argv[i]);
			int type = TypeMask(VREC(VarIndex)->type);
			if(DimIsRealArray(VREC(VarIndex)->dimtbl[0])) {
				oldmemory = VREC(VarIndex)->val.s;
				oldsize = MemSize(oldmemory);
				for(int k = 0; k < MAXDIM; k++) dims[k] = VREC(VarIndex)->dimtbl[k];
				if(!array_comp(dims, newdims) && preserve) error("Only the last array index can be changed");
			}
			uint32_t addr = erase(old, preserve);                   // the record goes; with PRESERVE the data stays for the copy
			if(type & T_STR) {
				char *newstring = GetTempStrMemory();
				strcpy(newstring, argv[i]);
				strcat(newstring, " LENGTH ");
				IntToStr(&newstring[strlen(newstring)], length, 10);
				findvar(newstring, type | T_IMPLIED | V_FIND | V_DIM_VAR);   // (T_IMPLIED: the type as declared, not DefaultType)
			} else if(type & T_STRUCT) {
				char *vararg = GetTempStrMemory(), *asp;
				strcpy(vararg, argv[i]);
				asp = skipvar(vararg, false);
				while(*asp && *asp != tokenAS) asp++;
				if(*asp == tokenAS) *asp = 0;                       // without an AS: the TYPE is structIdx
				g_StructArg = structIdx;
				findvar(vararg, type | T_IMPLIED | V_FIND | V_DIM_VAR);
				g_StructArg = -1;
			} else
				findvar(argv[i], type | T_IMPLIED | V_FIND | V_DIM_VAR);
			newmemory = VREC(VarIndex)->val.s;
			newsize = MemSize(newmemory);
			if(preserve) {
				if(newsize < oldsize) oldsize = newsize;
				memcpy(newmemory, oldmemory, oldsize);
				if(addr) FreeMemory((void *)addr);
			}
		}
	}
}

void cmd_erase(void) {                                              // ERASE var [, var ...]: its own command (CLEAR VARS stays)
	cmd_erasevar(cmdline);
}

void cmd_clear(void) {
	char *p;
	if((p=checkstring(cmdline,"VARS"))){
		cmd_erasevar(p);
		return;
	}
	checkend(cmdline);
	if(LocalIndex)error("Invalid in a subroutine");
	ClearVars(0);
}


/***********************************************************************************************
utility functions used by the various commands
************************************************************************************************/


// utility function used by llist() below
// it copys a command or function honouring the case selected by the user
void strCopyWithCase(char *d, char *s) {
	if(Option.Listcase == CONFIG_LOWER) {
		while(*s) *d++ = tolower(*s++);
	} else if(Option.Listcase == CONFIG_UPPER) {
		while(*s) *d++ = toupper(*s++);
	} else {
		while(*s) *d++ = *s++;
	}
	*d = 0;
}

void replaceAlpha(char *str, const char *replacements[MMEND]){
    char buffer[STRINGSIZE]; // Buffer to store the modified string
    int bufferIndex = 0;
    int len = strlen(str);
    int i = 0;

    while (i < len) {
        // Check for the pattern "~(X)" where X is an uppercase letter
        if (i<len-3 && str[i] == '~' && str[i + 1] == '(' && isupper((int)str[i + 2]) && str[i + 3] == ')') {
            char alpha = str[i + 2]; // Extract the letter 'alpha'
            const char *replacement = replacements[alpha - 'A']; // Get the replacement string

            // Copy the replacement string into the buffer
            strcpy(&buffer[bufferIndex], replacement);
            bufferIndex += strlen(replacement);

            i += 4; // Move past "~(X)"
        } else {
            // Copy the current character to the buffer
            buffer[bufferIndex++] = str[i++];
        }
    }

    buffer[bufferIndex] = '\0'; // Null-terminate the buffer
    strcpy(str,  buffer); // Copy the buffer back into the original string
}

// list a line into a buffer (b) given a pointer to the beginning of the line (p).
// the returned string is a C style string (terminated with a zero)
// this is used by cmd_list(), cmd_edit() and cmd_xmodem()
char MIPS16 *llist(char *b, char *p) {
    int i, firstnonwhite = true;
    char *b_start = b;

    while(1) {
        if(*p == T_NEWLINE) {
            p += T_NEWLINE_HDR;                                     // step over the line's header
            firstnonwhite = true;
            continue;
        }

        if(*p == T_LINENBR) {
            i = (((p[1]) << 8) | (p[2]));                           // get the line number
            p += 3;                                                 // and step over the number
            IntToStr(b, i, 10);
            b += strlen(b);
            if(*p != ' ') *b++ = ' ';
            }

        if(*p == T_LABEL) {                                         // got a label
            for(i = p[1], p += 2; i > 0; i--)
                *b++ = *p++;                                        // copy to the buffer
            *b++ = ':';                                             // terminate with a colon
            if(*p && *p != ' ') *b++ = ' ';                         // and a space if necessary
            firstnonwhite = true;
            }                                                       // this deliberately drops through in case the label is the only thing on the line

        if(*p >= C_BASETOKEN) {
            if(firstnonwhite && commandtbl_at(p) != CMD_NOTOKEN) {  // a command token starts a statement
                CommandToken tkn = commandtbl_at(p);
                if(tkn == GetCommandValue("Let"))
                    *b = 0;                                         // use nothing if it LET
                else {
                    strCopyWithCase(b, commandname(tkn));           // expand the command (if it is not LET)
                    b += strlen(b);                                 // update pointer to the end of the buffer
                    if(IsAlpha(*(b - 1))) *b++ = ' ';               // add a space to the end of the command name
                }
                firstnonwhite = false;
                p += sizeof(CommandToken);
            } else {                                                // not a command so must be a token
                strCopyWithCase(b, tokenname(*p));                  // expand the token
                b += strlen(b);                                     // update pointer to the end of the buffer
                if(*p == tokenTHEN || *p == tokenELSE)
                    firstnonwhite = true;
                else
                    firstnonwhite = false;
                p++;
            }
            continue;
        }

        // a name stored as a symbol: its spelling (Symbols.h; '?' if its table does not hold it)
        if(issymbol(*p)) {
            b += SymExpand(b, p, symbolsize(*p), STRINGSIZE);
            p += symbolsize(*p);
            firstnonwhite = false;
            continue;
        }

        // hey, an ordinary char, just copy it to the output
        if(*p) {
            *b = *p;                                                // place the char in the buffer
            if(*p != ' ') firstnonwhite = false;
            p++;  b++;                                              // move the pointers
            continue;
        }

        // at this point the char must be a zero
        // zero char can mean both a separator or end of line
        if(!(p[1] == T_NEWLINE || p[1] == 0)) {
            *b++ = ':';                                             // just a separator
            firstnonwhite = true;
            p++;
            continue;
        }

        // must be the end of a line - so return to the caller
        while(*(b-1) == ' ' && b > b_start) --b;                    // eat any spaces on the end of the line
        *b = 0;                                                     // terminate the output buffer
		replaceAlpha((char *)b_start, overlaid_functions) ;  //replace the user version of all the MM. functions
		ListUnfold(b_start);                                        // and BIN$( LEFT$( MAX( and the rest (MMBasic.c)
        return ++p;
    } // end while
}



void execute_one_command(char *p) {
    int i;

    CheckAbort();
    targ = T_CMD;
    skipspace(p);                                                   // skip any whitespace
    CommandToken tkn = commandtbl_at(p);
    if(tkn != CMD_NOTOKEN && tkn < CommandTableSize - 1 && (commandtbl[tkn].type & T_CMD)) {
        if(tkn == cmdWHILE || tkn == cmdDO || tkn == cmdFOR) error("Invalid inside THEN ... ELSE") ;
        cmdtoken = tkn;
        CmdTokenPtr = p;
        cmdline = p + sizeof(CommandToken);
        skipspace(cmdline);
        if(!SymAwareCommand(tkn)) cmdline = SymExpandStatement(cmdline);   // its arguments as text (Symbols.c)
        commandtbl[tkn].fptr();                                     // execute the command
    } else {
        if(!isnamestartsym(*p)) error("Invalid character");
        i = FindSubFun(p, false);                                   // it could be a defined command
        if(i >= 0)                                                  // >= 0 means it is a user defined command
            DefinedSubFun(false, p, i, NULL, NULL, NULL, NULL);
        else
            error("Unknown command");
    }
    ClearTempMemory();                                              // at the end of each command we need to clear any temporary string vars
}

// ARRAY SET, ARRAY ADD, ARRAY INSERT and ARRAY SLICE, and MATH SET, ADD, INSERT and SLICE, which are the same
// commands: the PicoMite's (core/Commands.c), for numbers (a structure member array too: pts().x) and strings
void cmd_arrayset(void) { array_set(cmdline); }
void cmd_arrayadd(void) { array_add(cmdline); }
void cmd_arrayinsert(void) { array_insert(cmdline); }
void cmd_arrayslice(void) { array_slice(cmdline); }
void array_set(char *tp)
{
	MMFLOAT f;
	long long int i64;
	char *s;
	int dims[MAXDIM] = {0};
	int i, t, copy, card1 = 1;
	unsigned char size = 0;
	MMFLOAT *a1float = NULL;
	int64_t *a1int = NULL;
	char *a1str = NULL;
	int s1; // stride for numeric arrays
	getargs(&tp, 3, ",");
	if (!(argc == 3))
		StandardError(2);
	findvar(argv[2], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
	t = VREC(VarIndex)->type;
	evaluate(argv[0], &f, &i64, &s, &t, false);
	if (t & T_STR)
	{
		card1 = parsestringarray(argv[2], &a1str, 2, 0, dims, true, &size);
		copy = (int)size + 1;
		memset(a1str, 0, copy * card1);
		if (*s)
		{
			for (i = 0; i < card1; i++)
			{
				Mstrcpy(&a1str[i * copy], s);
			}
		}
	}
	else
	{
		card1 = parsenumberarray(argv[2], &a1float, &a1int, 2, 0, dims, true, &s1);
		if (t & T_STR)
			SyntaxError();
		;

		if (a1float != NULL)
		{
			for (i = 0; i < card1; i++)
				STRIDE_FLOAT(a1float, i, s1) = ((t & T_INT) ? (MMFLOAT)i64 : f);
		}
		else
		{
			for (i = 0; i < card1; i++)
				STRIDE_INT(a1int, i, s1) = ((t & T_INT) ? i64 : FloatToInt64(f));
		}
	}
}
void cmd_add(void)
{
	array_add(cmdline);
}

void array_add(char *tp)
{
	MMFLOAT f;
	long long int i64;
	char *s;
	int dims[MAXDIM] = {0};
	int i, t, card1 = 1, card2 = 1;
	MMFLOAT *a1float = NULL, *a2float = NULL, scale;
	int64_t *a1int = NULL, *a2int = NULL;
	char *a1str = NULL, *a2str = NULL;
	int s1, s2;
	getargs(&tp, 5, ",");
	if (!(argc == 5))
		StandardError(2);
	findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
	t = VREC(VarIndex)->type;
	if (t & T_STR)
	{
		unsigned char size = 0, size2 = 0;
		char *toadd;
		card1 = parsestringarray(argv[0], &a1str, 1, 0, dims, false, &size);
		evaluate(argv[2], &f, &i64, &s, &t, false);
		if (!(t & T_STR))
			SyntaxError();
		;
		toadd = getstring(argv[2]);
		card2 = parsestringarray(argv[4], &a2str, 3, 0, dims, true, &size2);
		if (card1 != card2)
			StandardError(16);
		char *buff = GetTempStrMemory(); // this will last for the life of the command
		int copy = size + 1;
		int copy2 = size2 + 1;
		for (i = 0; i < card1; i++)
		{
			char *sarg1 = a1str + i * copy;
			char *sarg2 = a2str + i * copy2;
			if (*sarg1 + *toadd > size2)
				error("String too long");
			Mstrcpy(buff, sarg1);
			Mstrcat(buff, toadd);
			Mstrcpy(sarg2, buff);
		}
	}
	else
	{
		card1 = parsenumberarray(argv[0], &a1float, &a1int, 1, 0, dims, false, &s1);
		evaluate(argv[2], &f, &i64, &s, &t, false);
		if (t & T_STR)
			SyntaxError();
		;
		scale = getnumber(argv[2]);
		card2 = parsenumberarray(argv[4], &a2float, &a2int, 3, 0, dims, true, &s2);
		if (card1 != card2)
			StandardError(16);
		if (scale != 0.0)
		{
			if (a2float != NULL && a1float != NULL)
			{
				for (i = 0; i < card1; i++)
					STRIDE_FLOAT(a2float, i, s2) = ((t & T_INT) ? (MMFLOAT)i64 : f) + STRIDE_FLOAT(a1float, i, s1);
			}
			else if (a2float != NULL && a1float == NULL)
			{
				for (i = 0; i < card1; i++)
					STRIDE_FLOAT(a2float, i, s2) = ((t & T_INT) ? (MMFLOAT)i64 : f) + ((MMFLOAT)STRIDE_INT(a1int, i, s1));
			}
			else if (a2float == NULL && a1float != NULL)
			{
				for (i = 0; i < card1; i++)
					STRIDE_INT(a2int, i, s2) = FloatToInt64(((t & T_INT) ? (MMFLOAT)i64 : f) + STRIDE_FLOAT(a1float, i, s1));
			}
			else
			{
				for (i = 0; i < card1; i++)
					STRIDE_INT(a2int, i, s2) = ((t & T_INT) ? i64 : FloatToInt64(f)) + STRIDE_INT(a1int, i, s1);
			}
		}
		else
		{
			if (a2float != NULL && a1float != NULL)
			{
				for (i = 0; i < card1; i++)
					STRIDE_FLOAT(a2float, i, s2) = STRIDE_FLOAT(a1float, i, s1);
			}
			else if (a2float != NULL && a1float == NULL)
			{
				for (i = 0; i < card1; i++)
					STRIDE_FLOAT(a2float, i, s2) = ((MMFLOAT)STRIDE_INT(a1int, i, s1));
			}
			else if (a2float == NULL && a1float != NULL)
			{
				for (i = 0; i < card1; i++)
					STRIDE_INT(a2int, i, s2) = FloatToInt64(STRIDE_FLOAT(a1float, i, s1));
			}
			else
			{
				for (i = 0; i < card1; i++)
					STRIDE_INT(a2int, i, s2) = STRIDE_INT(a1int, i, s1);
			}
		}
	}
}
void cmd_insert(void)
{
	array_insert(cmdline);
}
void array_insert(char *tp)
{
	int i, j, t, start, increment, dim[MAXDIM], pos[MAXDIM], off[MAXDIM], dimcount = 0, target = -1;
	int64_t *a1int = NULL, *a2int = NULL;
	MMFLOAT *a1float = NULL, *a2float = NULL;
	char *a1str = NULL, *a2str = NULL;
	unsigned char size = 0, size2 = 0;
	int dims[MAXDIM] = {0};
	getargs(&tp, 15, ",");
	if (argc < 7)
		StandardError(2);
	findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
	t = VREC(VarIndex)->type;
	if (t & T_STR)
	{
		parsestringarray(argv[0], &a1str, 1, 0, dims, false, &size);
	}
	else
	{
		parsenumberarray(argv[0], &a1float, &a1int, 1, 0, dims, false, NULL);
	}
	if (!DimIsRealArray(dims[1]))
		error("Argument 1 must be a 2D or more array");
	for (i = 0; i < MAXDIM; i++)
	{
		if (DimIsRealArray(dims[i]))
		{
			dimcount++;
			dim[i] = DimUpper(dims[i]) - OptionBase;
		}
		else
			dim[i] = 0;
	}
	if (((argc - 1) / 2 - 1) != dimcount)
		StandardError(2);
	for (i = 0; i < dimcount; i++)
	{
		if (*argv[i * 2 + 2])
			pos[i] = getint(argv[i * 2 + 2], OptionBase, dim[i] + OptionBase) - OptionBase;
		else
		{
			if (target != -1)
				error("Only one index can be omitted");
			target = i;
			pos[i] = 1;
		}
	}
	if (t & T_STR)
	{
		parsestringarray(argv[i * 2 + 2], &a2str, i + 1, 1, dims, true, &size2);
	}
	else
	{
		parsenumberarray(argv[i * 2 + 2], &a2float, &a2int, i + 1, 1, dims, true, NULL);
	}
	if (target == -1)
		error("One index must be omitted"); // the omitted one is the dimension inserted into
	if (dim[target] + OptionBase != DimUpper(dims[0]))
		error("Size mismatch between insert and target array");
	if (size != size2)
		error("String arrays differ in string length");
	i = dimcount - 1;
	while (i >= 0)
	{
		off[i] = 1;
		for (j = 0; j < i; j++)
			off[i] *= (dim[j] + 1);
		i--;
	}
	start = 1;
	for (i = 0; i < dimcount; i++)
	{
		start += (pos[i] * off[i]);
	}
	start--;
	increment = off[target];
	start -= increment;
	if (t & T_STR)
	{
		int copy = (int)size + 1;
		for (i = 0; i <= dim[target]; i++)
		{
			char *p = a2str + i * copy;
			char *q = &a1str[(start + i * increment) * copy];
			memcpy(q, p, copy);
		}
	}
	else
	{
		for (i = 0; i <= dim[target]; i++) // the types may differ: convert, as LET does
		{
			int k = start + i * increment;
			if (a1float != NULL && a2float != NULL)
				a1float[k] = a2float[i];
			else if (a1float != NULL)
				a1float[k] = (MMFLOAT)a2int[i];
			else if (a2float != NULL)
				a1int[k] = FloatToInt64(a2float[i]);
			else
				a1int[k] = a2int[i];
		}
	}
	return;
}
void cmd_slice(void)
{
	array_slice(cmdline);
}
void array_slice(char *tp)
{
	int i, j, t, start, increment, dim[MAXDIM], pos[MAXDIM], off[MAXDIM], dimcount = 0, target = -1, toarray = 0;
	int64_t *a1int = NULL, *a2int = NULL;
	MMFLOAT *a1float = NULL, *a2float = NULL;
	char *a1str = NULL, *a2str = NULL;
	unsigned char size = 0, size2 = 0;
	int dims[MAXDIM] = {0};
	getargs(&tp, 15, ",");
	if (argc < 7)
		StandardError(2);
	findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
	t = VREC(VarIndex)->type;
	if (t & T_STR)
	{
		parsestringarray(argv[0], &a1str, 1, 0, dims, false, &size);
	}
	else
	{
		parsenumberarray(argv[0], &a1float, &a1int, 1, 0, dims, false, NULL);
	}
	if (!DimIsRealArray(dims[1]))
		error("Argument 1 must be a 2D or more array");
	for (i = 0; i < MAXDIM; i++)
	{
		if (DimIsRealArray(dims[i]))
		{
			dimcount++;
			dim[i] = DimUpper(dims[i]) - OptionBase;
		}
		else
			dim[i] = 0;
	}
	if (((argc - 1) / 2 - 1) != dimcount)
		StandardError(2);
	for (i = 0; i < dimcount; i++)
	{
		if (*argv[i * 2 + 2])
			pos[i] = getint(argv[i * 2 + 2], OptionBase, dim[i] + OptionBase) - OptionBase;
		else
		{
			if (target != -1)
				error("Only one index can be omitted");
			target = i;
			pos[i] = 1;
		}
	}
	if (target == -1)
		error("One index must be omitted"); // the omitted one is the dimension sliced
	if (t & T_STR)
	{
		toarray = parsestringarray(argv[i * 2 + 2], &a2str, i + 1, 1, dims, true, &size2) - 1;
	}
	else
	{
		toarray = parsenumberarray(argv[i * 2 + 2], &a2float, &a2int, i + 1, 1, dims, true, NULL) - 1;
	}
	if (dim[target] != toarray)
		error("Size mismatch between slice and target array");
	if (size != size2)
		error("String arrays differ in string length");
	i = dimcount - 1;
	while (i >= 0)
	{
		off[i] = 1;
		for (j = 0; j < i; j++)
			off[i] *= (dim[j] + 1);
		i--;
	}
	start = 1;
	for (i = 0; i < dimcount; i++)
	{
		start += (pos[i] * off[i]);
	}
	start--;
	increment = off[target];
	start -= increment;
	if (t & T_STR)
	{
		int copy = (int)size + 1; // allow for the length character of the string
		for (i = 0; i <= dim[target]; i++)
		{
			char *p = a2str + i * copy;
			char *q = &a1str[(start + i * increment) * copy];
			memcpy(p, q, copy);
		}
	}
	else
	{
		for (i = 0; i <= dim[target]; i++) // the types may differ: convert, as LET does
		{
			int k = start + i * increment;
			if (a1float != NULL && a2float != NULL)
				a2float[i] = a1float[k];
			else if (a1float != NULL)
				a2int[i] = FloatToInt64(a1float[k]);
			else if (a2float != NULL)
				a2float[i] = (MMFLOAT)a1int[k];
			else
				a2int[i] = a1int[k];
		}
	}
	return;
}
