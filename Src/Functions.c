/***************************************************************************
CMM2 MMBasic
functions.c

Handles all the functions in MMBasic.

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

//#include <math.h>
#include <float.h>
#include <stdlib.h>
#include "errno.h"
#include "MMBasic_Includes.h"

#include "Hardware_Includes.h"
//#include "xregex.h"
#include "re.h"

extern RNG_HandleTypeDef hrng;
extern char runcmd[STRINGSIZE];
const char* overlaid_functions[]={
    "MM.HRES",
    "MM.VRES",
    "MM.VER",
	"MM.I2C",
	"MM.FONTHEIGHT",
	"MM.FONTWIDTH",
	"MM.HPOS",
	"MM.VPOS",
	"MM.ERRNO",
	"MM.ERRMSG$",
	"MM.WATCHDOG",
	"MM.DEVICE$",
	"MM.CMDLINE$",
	"MM.WIDTH",
	"MM.HEIGHT",
	"MM.ONEWIRE",
	"MM.FLAGS",
	"MM.ERRLINE",
	"MM.CODE",                                                      // (before MM.END, which ends the table: the ~() codes stay)
	"MM.USB",
	"MM.DISPLAY",
	"MM.END"
};

/********************************************************************************************************************************************
 basic functions
 each function is responsible for decoding a basic function
 all function names are in the form fun_xxxx() so, if you want to search for the function responsible for the ASC() function look for fun_asc

 There are 4 globals used by these functions:

 char *ep       This is a pointer to the argument of the function
                Eg, in the case of INT(35/7) ep would point to "35/7)"

 fret           Is the return value for a basic function that returns a float

 iret           Is the return value for a basic function that returns an integer

 sret           Is the return value for a basic function that returns a string

 tret           Is the type of the return value.  normally this is set by the caller and is not changed by the function

 ********************************************************************************************************************************************/
/*

DOCUMENTATION
=============
 FIELD$( str$, field)
 or
 FIELD$( str$, field, delim$)
 or
 FIELD$( str$, field, delim$, quote$)
 
 Extract a substring (ie, field) from 'str$'.  Each is separated by any one of 
 the characters in the string 'delim$' and the field number to return is specified 
 by 'field' (the first field is field number 1).  Any leading and trailing 
 spaces will be trimmed from the returned string.
 
 Note that 'delim$' can contain a number of characters and the fields 
 will then be separated by any one of these characters.  if delim$ is not
 provided it will default to a comma (,) character.
 
 'quote$' is the set of characters that might be used to quote text.  Typically
 it is the double quote character (") and any text that is surrounded by the quote
 character(s) will be treated as a block and any 'delim$' characters within that
 block will not be used as delimiters.
 
 This function is useful for splitting apart comma-separated-values (CSV) in data 
 streams produced by GPS modules and test equipment.  For example:
  PRINT FIELD$("aaa,bbb,ccc", 2, ",")
  Will print the string: bbb
 
  PRINT FIELD$("text1, 'quoted, text', text3", 2, ",", "'")
  will print the string: 'quoted, text'

 */

// return true if the char 'c' is contained in the string 'srch$'
// used only by scan_for_delimiter()  below
// Note: this operates on MMBasic strings
static int MInStr(char *srch, char c) {
    int i;
    for(i = 1; i <= *(unsigned char *)srch; i++)
        if(c == srch[i]) 
            return true;
    return false;
}


// scan through string p and return p if it points to any char in delims
// this will skip any quoted text (quote delimiters in quotes)
// used only by fun_field() below
// Note: this operates on MMBasic strings
static int scan_for_delimiter(int start, char *p, char *delims, char *quotes) {
    int i;
    char qidx;
    for(i = start; i <= *(unsigned char *)p && !MInStr(delims, p[i]); i++) {
        if(MInStr(quotes, p[i])) {                                  // if we have a quote
            qidx = p[i];
            i++;                                                    // step over the opening quote
            while(i < *(unsigned char *)p && p[i] != qidx) i++;    // skip the quoted text
        }
    }
    return i;
}
// function (which looks like a pre defined variable) to return MM.CMDLINE$
// it uses the command line for a shortcut RUN (the + symbol) which was stored in tknbuf[]
void fun_cmdline(void){
	char *p=runcmd;
	sret = GetTempStrMemory();									// this buffer is automatically zeroed so the string is zero size
	skipspace(p);
	if(*p==34){
		do{
			p++;
		} while(*p && *p!=34);                                    // (no closing quote: no arguments)
		if(*p) p++;
		skipspace(p);
		if(*p==','){
			p++;
			skipspace(p);
		}
	}
	strcpy(sret, p);                                   // copy the string (the line's source is in its header, not its text)
	CtoM(sret);
	targ=T_STR;
}
// function (which looks like a pre defined variable) to return the version number
// it pulls apart the VERSION string to generate the number
void fun_version(void){
	char *p;
	/* Each step skips the separator that stopped the one before it: a '.', the 'b' of a beta, or the "RC" of a
	   release candidate.  A release has no fourth field, so strtol stops on the terminating NUL and p + 1 would read
	   past the end of the string literal: check there is something to step over.  A release candidate's number is
	   offset by 50, so "6.04.00RC1" is 6.040051, above every beta of the same release (the PicoMite's). */
	fret = strtol(VERSION, &p, 10);
	if(*p) fret += (MMFLOAT)strtol(p + 1, &p, 10) / 100;
	if(*p) fret += (MMFLOAT)strtol(p + 1, &p, 10) / 10000;
	if(*p) {
		int offset = 0;
		if((p[0] == 'R' || p[0] == 'r') && (p[1] == 'C' || p[1] == 'c')) {
			p++;                                                    // ("C1" would read as 0)
			offset = 50;
		}
		fret += (MMFLOAT)(strtol(p + 1, &p, 10) + offset) / 1000000;
	}
	targ = T_NBR;
}

extern int USBcode;                                                 // (KeyboardMap.c)
void fun_tilde(void){
	targ=T_INT;
/*
typedef enum {
    MMHRES,
    MMVRES,
    MMVER,
    MMI2C,
	MMFONTHEIGHT,
	MMFONTWIDTH,
	MMHPOS,
	MMVPOS,
    MMERRNO,
    MMERRMSG,
	MMWATCHDOG,
	MMDEVICE,
	MMCMDLINE,
    MMWIDTH,
    MMHEIGHT,
    MMEND
} Operation;
*/
	switch(*ep-'A'){
		case MMCODE: {                                              // the stepper's G-code buffer's free slots (stepper.c)
			extern volatile uint16_t stepper_gcode_buffer_space;
			iret = (int64_t)stepper_gcode_buffer_space;
			break;
		}
		case MMONEWIRE:
			iret = mmOWvalue;
			break;
		case MMUSB:                                                 // the HID code of the last key pressed on a USB keyboard, -1
			iret = USBcode;                                         // before the first
			break;
		case MMDISPLAY:                                             // the PicoMite's: 1 if it has a display (Peter, 26 Oct: 1,
			iret = 1;                                               // always)
			break;
		case MMFLAGS:
			iret = g_flag;
			break;
		case MMHRES:
		    iret = PageTable[WritePage].xmax;
			break;
		case MMVRES:
		    iret = PageTable[WritePage].ymax;
			break;
		case MMVER:
			fun_version();
			break;
		case MMI2C:
			iret=mmI2Cvalue;
			break;
		case MMFONTHEIGHT:
			iret = FontTable[gui_font >> 4][1] * (gui_font & 0b1111);
			break;
		case MMFONTWIDTH:
			iret = FontTable[gui_font >> 4][0] * (gui_font & 0b1111);
			break;
		case MMHPOS:
			iret = CurrentX;
			break;
		case MMVPOS:
			iret = CurrentY;
			break;
		case MMERRNO:
		    iret = MMerrno;
			break;
		case MMERRLINE:                                             // the line the error message gives, 0 if none
		    iret = MMerrline;
			break;
		case MMERRMSG:
			sret = GetTempMemory(STRINGSIZE);                                        // this will last for the life of the command
			if(MMErrMsg[0]==13)strcpy(sret, &MMErrMsg[2]);
			else strcpy(sret, MMErrMsg);
			CtoM(sret);
			targ=T_STR;
			break;
		case MMWATCHDOG:
			iret = WatchdogSet;
			break;
		case MMDEVICE:
			fun_device();
			break;
		case MMCMDLINE:
			fun_cmdline();
			break;
		case  MMWIDTH:
			iret=HRes/(short)(FontTable[gui_font >> 4][0] * (gui_font & 0b1111));
			break;
		case  MMHEIGHT:
			iret=VRes/(short)(FontTable[gui_font >> 4][1] * (gui_font & 0b1111));
			break;
		default:
			iret=-1;
	}
}

void fun_bound(void){
	int which=1;
	getargs(&ep, 3,",");
	if(argc==3)which=getint(argv[2],0,MAXDIM);
    findvar(argv[0], V_FIND | V_EMPTY_OK | V_NOFIND_ERR);
	if(which==0)iret=OptionBase;
	else {
		// A single element dimension has an upper bound of 0 under OPTION BASE 0,
		// so 0 can no longer double as "no such dimension" - asking for one the
		// variable does not have has to be an error rather than a silent 0.
		if(DimIsScalar(VREC(VarIndex)->dimtbl[0]))error("Expected an array");
		if(DimIsEnd(VREC(VarIndex)->dimtbl[which-1]))error("Dimensions");
		iret=DimUpper(VREC(VarIndex)->dimtbl[which-1]);
		if(iret==-1)iret=0;                                         // unbound empty array parameter
	}
	targ=T_INT;
}
void fun_call(void){
	int i;
    long long int i64 = 0;
    char *s = NULL;
    MMFLOAT f;
	char *q;
	char *p=getCstring(ep); //get the command we want to call
	q=p;
	while(*q){ //convert to upper case for the match
		*q=toupper(*q);
		q++;
	}
	q=ep;
	while(*q){
		if(*q==',' || *q=='\'')break;
		q++;
	}
	if(*q==',')q++;
	i = FindSubFun(p, true);                    // CALL() runs a FUNCTION
	if(strlen(p) + 1 + strlen(q) >= STRINGSIZE) error("Line is too long");   // p is a STRINGSIZE temporary
	strcat(p," ");
	strcat(p,q);
    targ= T_NOTYPE;
	if(i >= 0) {                                // >= 0 means it is a user defined command
		DefinedSubFun(true, p, i, &f, &i64, &s, &targ);
	} else error("Unknown user function");
    if(targ & T_STR) {
    	sret=GetTempStrMemory();
        Mstrcpy(sret, s);                                             // if it is a string then save it
    }
    if(targ & T_INT)iret=i64;
    if(targ & T_NBR)fret=f;
}
// syntax:  str$ = FIELD$(string1, nbr, string2, string3)
//          find field nbr in string1 using the delimiters in string2 to separate the fields
//          if string3 is present any chars quoted by chars in string3 will not be searched for delimiters
// Note: this operates on MMBasic strings
void fun_field(void) {
	char *p, *delims = "\1,", *quotes = "\0";
    int fnbr, i, j, k;
	getargs(&ep, 7,",");
	if(!(argc == 3 || argc == 5 || argc == 7)) error("Syntax");
    p = getstring(argv[0]);                                         // the string containing the fields
    fnbr = getint(argv[2], 1, MAXSTRLEN);                           // field nbr to return
    if(argc > 3 && *argv[4]) delims = getstring(argv[4]);           // delimiters for fields
    if(argc == 7) quotes = getstring(argv[6]);                      // delimiters for quoted text
	sret = GetTempStrMemory();                                      // this will last for the life of the command
    targ = T_STR;
    i = 1;
    while(--fnbr > 0) {
        i = scan_for_delimiter(i, p, delims, quotes);
        if(i > *p) return;
        i++;                                                        // step over the delimiter
    }
    while(p[i] == ' ') i++;                                         // trim leading spaces
    j = scan_for_delimiter(i, p, delims, quotes);                   // find the end of the field
    *sret = k = j - i;
    for(j = 1; j <= k; j++, i++) sret[j] = p[i];                    // copy to the return string
    for(k = *sret; k > 0 && sret[k] == ' '; k--);                   // trim trailing spaces
    *sret = k;
}

void fun_str2bin(void){
    union binmap{
    	int8_t c[8];
    	uint8_t uc[8];
    	float f;
    	double d;
    	int64_t l;
    	uint64_t ul;
    	int i;
    	uint32_t ui;
    	short s;
    	uint16_t us;
    }map;
    int j;
	getargs(&ep, 5,",");
	if(!(argc==3 || argc==5))error("Syntax");
	if(argc==5 && !checkstring(argv[4],"BIG"))error("Syntax");
	char *p;
	p = getstring(argv[2]);
	int len=p[0];
	if(len>8)error("String length");                               // (map is 8 bytes: a longer string overwrote the stack)
	map.l=0;
    for(j=0;j<len;j++)map.c[j]=p[j+1];
    if(argc==5){ // big endian so swap byte order
    	char k;
    	int m;
    	for(j=0;j<(len>>1);j++){
    		m=len-j-1;
    		k=map.c[j];
    		map.c[j]=map.c[m];
    		map.c[m]=k;
    	}
    }
    if(checkstring(argv[0],"DOUBLE")){
        if(len!=8)error("String length");
	targ=T_NBR;
	fret=(MMFLOAT)map.d;
    } else if(checkstring(argv[0],"SINGLE")){
        if(len!=4)error("String length");
	targ=T_NBR;
	fret=(MMFLOAT)map.f;
    } else if(checkstring(argv[0],"INT64")){
        if(len!=8)error("String length");
    	targ=T_INT;
    	iret=(int64_t)map.l;
    } else if( checkstring(argv[0],"INT32")){
        if(len!=4)error("String length");
    	targ=T_INT;
    	iret=(int64_t)map.i;
    } else if(checkstring(argv[0],"INT16")){
        if(len!=2)error("String length");
    	targ=T_INT;
    	iret=(int64_t)map.s;
    } else if(checkstring(argv[0],"INT8")){
        if(len!=1)error("String length");
    	targ=T_INT;
    	iret=(int64_t)map.c[0];
    } else if(checkstring(argv[0],"UINT64")){
        if(len!=8)error("String length");
    	targ=T_INT;
    	iret=(int64_t)map.ul;
    } else if(checkstring(argv[0],"UINT32")){
        if(len!=4)error("String length");
    	targ=T_INT;
    	iret=(int64_t)map.ui;
    } else if(checkstring(argv[0],"UINT16")){
        if(len!=2)error("String length");
    	targ=T_INT;
    	iret=(int64_t)map.us;
    } else if(checkstring(argv[0],"UINT8")){
        if(len!=1)error("String length");
    	targ=T_INT;
    	iret=(int64_t)map.uc[0];
    } else error("Syntax");

}


void fun_bin2str(void){
	int j, len=0;
    union binmap{
    	int8_t c[8];
    	uint8_t uc[8];
    	float f;
    	double d;
    	int64_t l;
    	uint64_t ul;
    	int i;
    	uint32_t ui;
    	short s;
    	uint16_t us;
    }map;
    int64_t i64;
	getargs(&ep, 5,",");
	if(!(argc==3 || argc==5))error("Syntax");
    if(argc==5 && !(checkstring(argv[4],"BIG")))error("Syntax");
	sret = GetTempStrMemory();									// this will last for the life of the command
    if(checkstring(argv[0],"DOUBLE")){
		len=8;
		map.d=(double)getnumber(argv[2]);
    } else if(checkstring(argv[0],"SINGLE")){
		len=4;
    	map.f=(float)getnumber(argv[2]);
    } else {
    	i64=getinteger(argv[2]);
    	if(checkstring(argv[0],"INT64")){
    		len=8;
    		map.l=(int64_t)i64;
    	} else if( checkstring(argv[0],"INT32")){
    		len=4;
    		if(i64 > 2147483647 || i64 < -2147483648)error("Overflow");
    		map.i=(int32_t)i64;
    	} else if(checkstring(argv[0],"INT16")){
    		len=2;
    		if(i64 > 32767 || i64 < -32768)error("Overflow");
    		map.s=(int16_t)i64;
    	} else if(checkstring(argv[0],"INT8")){
    		len=1;
    		if(i64 > 127 || i64 < -128)error("Overflow");
    		map.c[0]=(int8_t)i64;
    	} else if(checkstring(argv[0],"UINT64")){
    		len=8;
    		map.ul=(uint64_t)i64;
    	} else if(checkstring(argv[0],"UINT32")){
    		len=4;
    		if(i64 > 4294967295 || i64 < 0)error("Overflow");
    		map.ui=(uint32_t)i64;
    	} else if(checkstring(argv[0],"UINT16")){
    		len=2;
    		if(i64 > 65535 || i64 < 0)error("Overflow");
    		map.us=(uint16_t)i64;
    	} else if(checkstring(argv[0],"UINT8")){
    		len=1;
    		if(i64 > 255 || i64 < 0)error("Overflow");
    		map.uc[0]=(uint8_t)i64;
    	} else error("Syntax");
    }


	for(j=0;j<len;j++)sret[j]=map.c[j];

    if(argc==5){ // big endian so swap byte order
    	char k;
    	int m;
    	for(j=0;j<(len>>1);j++){
    		m=len-j-1;
    		k=sret[j];
    		sret[j]=sret[m];
    		sret[m]=k;
    	}
    }
    // convert from c type string but it can contain zeroes
    char *p1, *p2;
    j=len;
    p1 = sret + len; p2 = sret + len - 1;
    while(j--) *p1-- = *p2--;
    *sret = len;
	targ = T_STR;
}


// return the absolute value of a number (ie, without the sign)
// a = ABS(nbr)
void fun_abs(void) {
    char *s;
    MMFLOAT f;
    long long int i64;

    targ = T_NOTYPE;                                            // not T_INT: that converts a float, "Number too large" from 2^63
    evaluate(ep, &f, &i64, &s, &targ, false);                   // get the value and type of the argument
    if(targ & T_STR) error("Expected a number");
    if(targ & T_NBR)
        fret = fabs(f);
    else
        iret = FnAbsI(i64);
}

long long int FnAbsI(long long int i64) {
    return i64 < 0 ? -i64 : i64;
}

// Helper function to check if a character is in the mask
int is_in_mask(char c, const char *mask)
{
	while (*mask)
	{
		if (*mask == c)
		{
			return 1;
		}
		mask++;
	}
	return 0;
}

char *trim(char *source, char *mask, char where)
{
	char *result = GetTempMemory(STRINGSIZE);

	if (!source || !mask || !result)
	{
		if (result)
			result[0] = '\0';
		return result;
	}

	int len = strlen(source);
	int start = 0;
	int end = len - 1;

	// Trim from the left
	if (where == 'L' || where == 'B')
	{
		while (start <= end && is_in_mask(source[start], mask))
		{
			start++;
		}
	}

	// Trim from the right
	if (where == 'R' || where == 'B')
	{
		while (end >= start && is_in_mask(source[end], mask))
		{
			end--;
		}
	}

	// Copy the trimmed string to result
	int result_len = end - start + 1;
	if (result_len > 0 && result_len < STRINGSIZE)
	{
		strncpy(result, source + start, result_len);
		result[result_len] = '\0';
	}
	else
	{
		result[0] = '\0';
	}

	return result;
}
void fun_trim(void)
{
	char defaultmask[2] = " ";
	char *mask = NULL;
	char where = 'L';
	getargs(&ep, 5, ",");
	char *instring = (char *)getCstring(argv[0]);
	if (argc >= 3 && *argv[2])
		mask = (char *)getCstring(argv[2]);
	else
		mask = defaultmask;
	if (argc == 5)
	{
		if (checkword(argv[4], "L"))                       // (the letter alone: the PicoMite's #112)
			where = 'L';
		else if (checkword(argv[4], "R"))
			where = 'R';
		else if (checkword(argv[4], "B"))
			where = 'B';
		else
		{
			where = (char)toupper(*getCstring(argv[4]));
			if (!(where == 'L' || where == 'R' || where == 'B'))
				error("Syntax");
		}
	}
	targ = T_STR;
	sret = CtoM(trim(instring, mask, where));
}


// return the ASCII value of the first character in a string (ie, its number value)
// a = ASC(str$)
void fun_asc(void) {
	char *s;

	s = getstring(ep);
	if(*s == 0)
	    iret = 0;
	else
    	iret = *(s + 1);
    targ = T_INT;
}



// return the arctangent of a number in radians
// the pure numeric functions on a value: their fun_ handlers and the compiler's code (Stream.c) call them alike
MMFLOAT FnAtn(MMFLOAT x) {
    return atan(x) * optionangle;
}

void fun_atn(void) {
	fret = FnAtn(getnumber(ep));
    targ = T_NBR;
}

void fun_atan2(void) {
    MMFLOAT y,x,z;
    getargs(&ep, 3,",");
    if(argc != 3)error("Syntax");
    y=getnumber(argv[0]);
    x=getnumber(argv[2]);
    z=atan2(y,x);
    fret=z*optionangle;
    targ = T_NBR;
}

// convert a number into a one character string
// s$ = CHR$(nbr)
void fun_chr(void) {
	int i;

	i = getint(ep, 0, 0xff);
	sret = StrChar(GetTempStrMemory(), i);                      // this will last for the life of the command
    targ = T_STR;
}



// Round numbers with fractional portions up or down to the next whole number or integer.
void fun_cint(void) {
	iret = getinteger(ep);
    targ = T_INT;
}



// return the cosine of a number in radians
MMFLOAT FnCos(MMFLOAT t) {
    return cos(t / optionangle);
}

void fun_cos(void) {
	fret = FnCos(getnumber(ep));
    targ = T_NBR;
}


// convert radians to degrees.  Thanks to Alan Williams for the contribution
MMFLOAT FnDeg(MMFLOAT x) {
    return (MMFLOAT)(x * RADCONV);
}

void fun_deg(void) {
	fret = FnDeg(getnumber(ep));
    targ = T_NBR;
}



// Returns the exponential value of a number.
MMFLOAT FnExp(MMFLOAT x) {
    return exp(x);
}

void fun_exp(void) {
	fret = FnExp(getnumber(ep));
    targ = T_NBR;
}


// utility function used by HEX$(), OCT$() and BIN$()
//void DoHexOctBin(char *tp,int base) {
void fun_base(void) {
	unsigned long long int i;
    int j = 1;
//	getargs(&tp, 3, ",");
	getargs(&ep, 5, ",");
	int base=getint(argv[0],2,36);
	i = (unsigned long long int)getinteger(argv[2]);                // get the number
    if(argc == 5) j = getint(argv[4], 0, MAXSTRLEN);                // get the optional number of chars to return
	sret = GetTempStrMemory();                                      // this will last for the life of the command
    IntToStrPad(sret, (signed long long int)i, '0', j, base);
	CtoM(sret);
    targ = T_STR;
}



// return the hexadecimal representation of a number
// s$ = HEX$(nbr)
//void fun_hex(void) {
//    DoHexOctBin(ep,16);
//}



// return the octal representation of a number
// s$ = OCT$(nbr)
//void fun_oct(void) {
 //   DoHexOctBin(ep,8);
//}



// return the binary representation of a number
// s$ = BIN$(nbr)
//void fun_bin(void) {
//    DoHexOctBin(ep,2);
//}


// syntax:  nbr = INSTR([start,] string1, string2)
//          find the position of string2 in string1 starting at start chars in string1
// returns an integer
//19/10/2024   Fix from picomite for error if start is an expression

void fun_instr(void) {
	char *s1 = NULL, *s2 = NULL;
	int t, start = 0, n = 0 ;
    char *ss;
    MMFLOAT f;
    long long int  i64;
    getargs(&ep, 7, (char *)",");
	if(!(argc==3 || argc==5 || argc==7))error("Syntax");
    t = T_NOTYPE;
    evaluate(argv[0], &f, &i64, &ss, &t, false);                   // get the value and type of the argument
    if(t & T_NBR){
        n=2;
		start=getint(argv[0],0,255)-1;
    } else if(t & T_INT){
		n=2;
		start=getint(argv[0],0,255)-1;
    } else if(t & T_STR){
		n=0;
	} else error("Syntax");
	if(argc < (n==2 ? 7 : 5)){
		s1 = getstring(argv[0+n]);
		s2 = getstring(argv[2+n]);
		targ = T_INT;
		iret = StrInstr(s1, s2, start);                             // find s2 in s1 using MMBasic strings
		return;
	} else {  //its a regex expression

		int match_length;

		//int  re_match(const char* pattern, const char* text, int* matchlength);

		int64_t *tempi = NULL;                                      // the match's length goes to an integer or a float variable
		MMFLOAT *tempf = NULL;
		char *s=GetTempMemory(STRINGSIZE), *p=GetTempMemory(STRINGSIZE);
		strcpy(s,getCstring(argv[0+n]));
		{
			int tmp = OptionEscape;                                 // the pattern's \ sequences are the regex's, not OPTION ESCAPE's
			OptionEscape = 0;
			strcpy(p,getCstring(argv[2+n]));
			OptionEscape = tmp;
		}
		if(argc==5+n){  // This is the passed variable to return the match length
			void *temp = findvar(argv[4+n], V_FIND);
			if(!(FoundType() & (T_NBR | T_INT))) error("Invalid variable");
			if(FoundType() & T_INT) tempi = temp; else tempf = temp;
		}
		targ=T_INT;

		int match_idx = re_match(p, &s[start], &match_length);
		if (match_idx != -1){
		    if(tempi)*tempi=match_length;
		    if(tempf)*tempf=(MMFLOAT)(match_length);
		    iret=match_idx+1+start;
		}else{
			iret=0;
			if(tempi)*tempi=0;
			if(tempf)*tempf=0.0;
		}


	}
	targ=T_INT;
}

/*
// syntax:  nbr = INSTR([start,] string1, string2)
//          find the position of string2 in string1 starting at start chars in string1
// returns an integer
void fun_instr(void) {
	unsigned char *s1 = NULL, *s2 = NULL;
	int start = 0, n = 0 ;
    char *ss;
    MMFLOAT f;
    long long int  i64;
	getargs(&ep, 7, ",");
	if(!(argc==3 || argc==5 || argc==7))error("Syntax");
    targ = T_NOTYPE;
    evaluate(argv[0], &f, &i64, &ss, &targ, false);                   // get the value and type of the argument
    if(targ & T_NBR){
        n=2;
		start=getint(argv[0],0,255)-1;
    } else if(targ & T_INT){
		n=2;
		start=getint(argv[0],0,255)-1;
    } else if(targ & T_STR){

	} else error("Syntax");
	if(argc < (n==2 ? 7 : 5)){
		s1 = (unsigned char *)getstring(argv[0+n]);
		s2 = (unsigned char *)getstring(argv[2+n]);
		targ = T_INT;
		if(start > *s1 - *s2 + 1 || *s2 == 0)
			iret = 0;
		else {
			// find s2 in s1 using MMBasic strings
			int i;
			for(i = start; i < *s1 - *s2 + 1; i++) {
				if(memcmp(s1 + i + 1, s2 + 1, *s2) == 0) {
					iret = i + 1;
					return;
				}
			}
		}
		iret = 0;
	} else {
		regex_t regex;
		int reti;
		regmatch_t pmatch;
		MMFLOAT *temp=NULL;
		temp = findvar(argv[6], V_FIND);
		if(!(FoundType() & T_NBR)) error("Invalid variable");
		char *s=GetTempMemory(STRINGSIZE), *p=GetTempMemory(STRINGSIZE);
		strcpy(s,getCstring(argv[0+n]));
		strcpy(p,getCstring(argv[2+n]));
		if(argc==5+n){
			temp = findvar(argv[4+n], V_FIND);
			if(!(FoundType() & T_NBR)) error("Invalid variable");
		}
		reti = regcomp(&regex, p, 0);
		if( reti ) error("Could not compile regex");
		reti = regexec(&regex, &s[start], 1, &pmatch, 0);
		targ=T_INT;
		if( !reti ){
			iret=pmatch.rm_so+1+start;
			if(temp)*temp=(MMFLOAT)(pmatch.rm_eo-pmatch.rm_so);
		}
		else if( reti == REG_NOMATCH ){
			iret=0;
			if(temp)*temp=0.0;
		}
		else{
			error("Regex execution error");
		}

	}
	targ=T_INT;
}
*/





// Truncate an expression to the next whole number less than or equal to the argument.
long long int FnInt(MMFLOAT x) {
    return FloatToInt64(floor(x));                                  // range-checked: a plain cast turned NaN, INF and 1e30 into 9223372036854775807
}

void fun_int(void) {
	iret = FnInt(getnumber(ep));
    targ = T_INT;
}


// Truncate a number to a whole number by eliminating the decimal point and all characters
// to the right of the decimal point.
long long int FnFix(MMFLOAT x) {
    return FloatToInt64(trunc(x));                                  // range-checked, as INT
}

void fun_fix(void) {
	iret = FnFix(getnumber(ep));
    targ = T_INT;
}



/* The string functions' cores (the PicoMite's, Functions.c): each is the body of an MMBasic function with the
   argument parsing left behind in its fun_ wrapper, so the interpreter and a CSUB (through the CallTable that
   vector slot 7 points at, CFunctions.c) run the same code and cannot disagree.  Each writes into a destination
   of STRINGSIZE, GetTempStrMemory(), and clamps what it is given rather than raising an error: a CSUB that calls
   error() is abandoned by its longjmp. */
char *StrLeft(char *dst, const char *s, int n) {
    Mstrcpy(dst, (char *)s);
    if(n < 0) n = 0;
    if(n < (unsigned char)*dst) *dst = n;                           // truncate if it is less than the current string length
    return dst;
}

char *StrRight(char *dst, const char *s, int n) {
    char *p1 = dst;
    const char *p2;
    if(n < 0) n = 0;
    if(n > (unsigned char)*s) n = (unsigned char)*s;                // the number of chars to copy
    p2 = s + ((unsigned char)*s - n) + 1;
    *p1++ = n;                                                      // the length of the returned string
    while(n--) *p1++ = *p2++;                                       // and the characters
    return dst;
}

char *StrCase(char *dst, const char *s, int upper) {
    char *p = dst;
    int i = *p++ = *s++;                                            // the length, copied across first
    if(upper)
        while(i--) *p++ = toupper(*s++);
    else
        while(i--) *p++ = tolower(*s++);
    return dst;
}

char *StrMid(char *dst, const char *s, int spos, int nbr) {
    char *p1;
    const char *p2;
    int i;
    *dst = 0;                                                       // a null string when the numeric arguments
    if(spos < 1) spos = 1;
    if(spos > (unsigned char)*s || nbr <= 0) return dst;            // are not in the string
    i = (unsigned char)*s - spos + 1;                               // how many chars remain in the string
    if(i > nbr) i = nbr;                                            // reduced if we don't need that many
    p1 = dst; p2 = s + spos;
    *p1++ = i;                                                      // the length of the MMBasic string
    while(i--) *p1++ = *p2++;                                       // and the chars required
    return dst;
}

char *StrChar(char *dst, int c) {
    dst[0] = 1;
    dst[1] = c;
    return dst;
}

char *StrFill(char *dst, int ch, int n) {
    if(n < 0) n = 0;
    if(n > MAXSTRLEN) n = MAXSTRLEN;
    memset(dst + 1, ch, n);
    *dst = n;
    return dst;
}

// INSTR without a regular expression: the position of s2 in s1 from start (counted from 0), or 0
int StrInstr(const char *s1, const char *s2, int start) {
    int i;
    if(start < 0) start = 0;
    if(start > (unsigned char)*s1 - (unsigned char)*s2 + 1 || *s2 == 0) return 0;
    for(i = start; i < (unsigned char)*s1 - (unsigned char)*s2 + 1; i++)    // find s2 in s1 using MMBasic strings
        if(memcmp(s1 + i + 1, s2 + 1, (unsigned char)*s2) == 0) return i + 1;
    return 0;
}

// STR$ once its arguments are evaluated: isint picks the integer path, m is the digits before the point, n those
// after it (STR_AUTO_PRECISION for the default) and ch the padding character
char *StrFormat(char *dst, MMFLOAT f, long long int i64, int isint, int m, int n, int ch) {
    if(!isint)
        FloatToStr(dst, f, m, n, ch);                               // convert the float
    else {
        if(n < 0)
            FloatToStr(dst, i64, m, n, ch);                         // convert as a float
        else {
            IntToStrPad(dst, i64, ch, m, 10);                       // convert the integer
            if(n != STR_AUTO_PRECISION && n > 0) {
                strcat(dst, ".");
                while(n--) strcat(dst, "0");                        // and add on any zeros after the point
            }
        }
    }
    return CtoM(dst);
}

// VAL on a C string: non-zero when the answer is a float (in *f), zero when it is an integer (in *i)
int StrVal(const char *p, MMFLOAT *f, long long int *i) {
    char *t1, *t2;
    *f = 0;
    *i = 0;
    if(*p == '&') {
        p++;
        switch(toupper(*p++)) {
            case 'H': while(IsxDigit(*p)) {
                          *i = (*i << 4) | ((toupper(*p) >= 'A') ? toupper(*p) - 'A' + 10 : *p - '0');
                          p++;
                      }
                      break;
            case 'O': while(*p >= '0' && *p <= '7') *i = (*i << 3) | (*p++ - '0');
                      break;
            case 'B': while(*p == '0' || *p == '1') *i = (*i << 1) | (*p++ - '0');
                      break;
            default : *i = 0;
        }
        return 0;
    }
    *f = (MMFLOAT)strtod(p, &t1);
    *i = strtoll(p, &t2, 10);
    return t1 > t2;
}

// RND: from the hardware's random number generator
MMFLOAT RndVal(void) {
    uint32_t aRandom32bit;
    HAL_RNG_GenerateRandomNumber(&hrng, &aRandom32bit);
    return (MMFLOAT)aRandom32bit/(MMFLOAT)pow(2, 32);
}



// BIT(var%, n), BYTE(s$, pos) and FLAG(n) are one token: tokenise() writes them BITBYTEFLAG(1, 2 or 3 and a comma
// before the arguments (SubstFold), and LIST turns them back (ListUnfold).  The PicoMite's fun_bit, fun_byte and fun_flag.
// n = BIT( var%, bit )           bit 0-63 of the integer variable
// n = BYTE( s$, pos )            the byte at pos (1 to the variable's size) of the string variable
// n = FLAG( bit )                bit 0-63 of the flags (FLAGS, MM.FLAGS)
void fun_bitbyteflag(void) {
	int which, spos;
	getargs(&ep, 5, ",");
	which = argc ? getint(argv[0], 1, 3) : 0;
	if(!((which == 3 && argc == 3) || (which != 3 && argc == 5))) error("Argument count");
	if(which == 1) {
		uint64_t *s = (uint64_t *)findvar(argv[2], V_NOFIND_ERR);
		if(!(FoundType() & T_INT)) error("Not an integer");
		spos = getint(argv[4], 0, 63);
		iret = (*s >> spos) & 1;
	} else if(which == 2) {
		unsigned char *s = (unsigned char *)findvar(argv[2], V_NOFIND_ERR);
		if(!(FoundType() & T_STR)) error("Not a string");
		iret = s[getint(argv[4], 1, FoundSize())];
	} else {
		spos = getint(argv[2], 0, 63);
		iret = (g_flag >> spos) & 1;
	}
	targ = T_INT;
}

// LEFT$(, RIGHT$(, UCASE$( and LCASE$( are one token, as on the PicoMite: tokenise() writes them SCHANGE$( with
// the letter E, R, U or L and a comma before the arguments, and LIST turns them back (ListUnfold)
// s$ = LEFT$( string$, nbr )     the first nbr characters
// s$ = RIGHT$( string$, nbr )    the last nbr characters
// s$ = UCASE$( string$ )         in capitals
// s$ = LCASE$( string$ )         in small letters
void fun_schange(void) {
	int i;
	char *s;
	getargs(&ep, 5, ",");
	switch(argc ? toupper(*argv[0]) : 0) {
		case 'E':
		case 'R':
			if(argc != 5) error("Argument count");
			s = getstring(argv[2]);
			i = getint(argv[4], 0, MAXSTRLEN);
			sret = toupper(*argv[0]) == 'E' ? StrLeft(GetTempStrMemory(), s, i) : StrRight(GetTempStrMemory(), s, i);
			break;
		case 'U':
		case 'L':
			if(argc != 3) error("Argument count");
			s = getstring(argv[2]);
			sret = StrCase(GetTempStrMemory(), s, toupper(*argv[0]) == 'U');
			break;
		default:
			error("Syntax");
	}
    targ = T_STR;
}



// return the length of a string
// nbr = LEN( string$ )
void fun_len(void) {
	iret = *(unsigned char *)getstring(ep);                         // first byte is the length
    targ = T_INT;
}



// Return the natural logarithm of the argument 'number'.
// n = LOG( number )
MMFLOAT FnLog(MMFLOAT f) {
    if(f == 0) error("Divide by zero");
	if(f < 0) error("Negative argument");
	return log(f);
}

void fun_log(void) {
	fret = FnLog(getnumber(ep));
    targ = T_NBR;
}



// Returns a substring of ?string$? beginning at ?start? and continuing for ?nbr? characters.
// S$ = MID$(s, spos [, nbr])
void fun_mid(void) {
	char *s;
	int spos, nbr = 0;
	getargs(&ep, 5, ",");

	if(argc == 5) {													// we have MID$(s, n, m)
		nbr = getint(argv[4], 0, MAXSTRLEN);						// nbr of chars to return
	}
	else if(argc == 3) {											// we have MID$(s, n)
		nbr = MAXSTRLEN;											// default to all chars
	}
	else
		error("Argument count");

	s = getstring(argv[0]);											// the string
	spos = getint(argv[2], 1, MAXSTRLEN);						    // the mid position
	sret = StrMid(GetTempStrMemory(), s, spos, nbr);                // (a null string if the numbers are not in it)
    targ = T_STR;
}



// Return the value of Pi.  Thanks to Alan Williams for the contribution
// n = PI
void fun_pi(void) {
	fret = PI_VALUE;
    targ = T_NBR;
}



// convert degrees to radians.  Thanks to Alan Williams for the contribution
// r = RAD( degrees )
MMFLOAT FnRad(MMFLOAT x) {
    return (MMFLOAT)(x / RADCONV);
}

void fun_rad(void) {
	fret = FnRad(getnumber(ep));
    targ = T_NBR;
}


// generate a random number that is greater than or equal to 0 but less than 1
// n = RND()
void fun_rnd(void) {
	fret = RndVal();
    targ = T_NBR;
}



// Return the sign of the argument
// n = SGN( number )
// SGN of a float (the CMM2's SGN reads its argument as a float: there is no integer form, as the PicoMite has)
long long int FnSgnF(MMFLOAT f) {
	if(f > 0) return +1;
	if(f < 0) return -1;
	return 0;
}

void fun_sgn(void) {
	iret = FnSgnF(getnumber(ep));
    targ = T_INT;
}



// Return the sine of the argument 'number' in radians.
// n = SIN( number )
MMFLOAT FnSin(MMFLOAT t) {
    return sin(t / optionangle);
}

void fun_sin(void) {
	fret = FnSin(getnumber(ep));
    targ = T_NBR;
}



// Return the square root of the argument 'number'.
// n = SQR( number )
MMFLOAT FnSqr(MMFLOAT f) {
	if(f < 0) error("Negative argument");
	return sqrt(f);
}

void fun_sqr(void) {
	fret = FnSqr(getnumber(ep));
    targ = T_NBR;
}



// Return the tangent of the argument 'number' in radians.
// n = TAN( number )
MMFLOAT FnTan(MMFLOAT t) {
    return tan(t / optionangle);
}

void fun_tan(void) {
	fret = FnTan(getnumber(ep));
    targ = T_NBR;
}



// Returns the numerical value of a string.
// n = VAL( string$ )
void fun_val(void) {
	targ = StrVal(getCstring(ep), &fret, &iret) ? T_NBR : T_INT;
}



void fun_eval(void) {
    char *s, *st, *temp_tknbuf;
    int i=0,toggle=0;
    temp_tknbuf = GetTempStrMemory();
    strcpy(temp_tknbuf, tknbuf);                                    // first save the current token buffer in case we are in immediate mode
    // we have to fool the tokeniser into thinking that it is processing a program line entered at the console
    st = GetTempStrMemory();
    strcpy(st, getstring(ep));                                      // then copy the argument
    MtoC(st);                                                       // and convert to a C string
    inpbuf[0] = 'r'; inpbuf[1] = '=';                               // place a dummy assignment in the input buffer to keep the tokeniser happy
    strcpy(inpbuf + 2, st);
    while(inpbuf[i]){
		if(inpbuf[i]==34){
			if(toggle==0)toggle=1;
			else toggle=0;
		}
		if(!toggle)inpbuf[i]=toupper(inpbuf[i]);
		i++;
    }
    tokenise(true);                                                 // and tokenise it (the result is in tknbuf)
    strcpy(st, tknbuf + sizeof(CommandToken) + 2);                 // after the LET token, the r and the = token
    targ = T_NOTYPE;
    evaluate(st, &fret, &iret, &s, &targ, false);                   // get the value and type of the argument
    if(targ & T_STR) {
        Mstrcpy(st, s);                                             // if it is a string then save it
        sret = st;
    }
    strcpy(tknbuf, temp_tknbuf);                                    // restore the saved token buffer
}



/*void fun_errno(void) {
    iret = MMerrno;
    targ = T_INT;
}*/


/*void fun_errmsg(void) {
    sret = GetTempStrMemory();
    strcpy(sret, MMErrMsg);
    CtoM(sret);
//    dump(sret, 32);
    targ = T_STR;
}*/



// Returns a string of blank spaces 'number' bytes long.
// s$ = SPACE$( number )
void fun_space(void) {
	int i;

	i = getint(ep, 0, MAXSTRLEN);
	sret = StrFill(GetTempStrMemory(), ' ', i);                 // this will last for the life of the command
    targ = T_STR;
}



// Returns a string in the decimal (base 10) representation of  'number'.
// s$ = STR$( number, m, n, c$ )
void fun_str(void) {
	char *s;
	MMFLOAT f;
    long long int i64;
	int t;
    int m, n;
    char ch, *p;

    getargs(&ep, 7, ",");
    if((argc & 1) != 1) error("Syntax");
    t = T_NOTYPE;
    p = evaluate(argv[0], &f, &i64, &s, &t, false);                 // get the value and type of the argument
    if(!(t & T_INT || t & T_NBR)) error("Expected a number");
    m = 0; n = STR_AUTO_PRECISION; ch = ' ';
    if(argc > 2) m = getint(argv[2], -128, 128);                    // get the number of digits before the point
    if(argc > 4) n = getint(argv[4], -20, 20);                      // get the number of digits after the point
	if(argc == 7) {
        p = getstring(argv[6]);
        if(*p == 0) error("Zero length argument");
        ch = ((unsigned char)p[1] & 0x7f);
	}

	sret = StrFormat(GetTempStrMemory(), f, i64, !(t & T_NBR), m, n, ch);   // this will last for the life of the command
    targ = T_STR;
}



// Returns a string 'nbr' bytes long
// s$ = STRING$( nbr,  string$ )
// s$ = STRING$( nbr,  number )
void fun_string(void) {
    int i, j, t = T_NOTYPE;
    void *p;

    getargs(&ep, 3, ",");
    if(argc != 3) error("Syntax");

    i = getint(argv[0], 0, MAXSTRLEN);
    p = DoExpression(argv[2], &t);                                  // get the value and type of the argument
    if(t & T_STR) {
        if(!*(char *)p) error("Argument value: $", argv[2]);
        j = *((char *)p + 1);
    } else if(t & T_INT)
        j = *(long long int *)p;
    else
        j = FloatToInt32(*((MMFLOAT *)p));
    if(j < 0 || j > 255) error("Argument value: $", argv[2]);

    sret = StrFill(GetTempStrMemory(), j, i);                       // this will last for the life of the command
    targ = T_STR;
}



// function (which looks like a pre defined variable) to return the version number
// it pulls apart the VERSION string to generate the number
/*void fun_version(void){
	char *p;
    fret = strtol(VERSION, &p, 10);
    fret += (MMFLOAT)strtol(p + 1, &p, 10) / 100;
    fret += (MMFLOAT)strtol(p + 1, &p, 10) / 10000;
    targ = T_NBR;
}*/



// Returns the current cursor position in the line in characters.
// n = POS
void fun_pos(void){
	iret = MMCharPos;
    targ = T_INT;
}



// Outputs spaces until the column indicated by 'number' has been reached.
// PRINT TAB( number )
void fun_tab(void) {
	int i;
	char *p;

	i = getint(ep, 1, 255);
	sret = p = GetTempStrMemory();							    // this will last for the life of the command
	if(MMCharPos > i) {
		i--;
		*p++ = '\r';
		*p++ = '\n';
	}
	else
		i -= MMCharPos;
	mymemset(p, ' ', i);
	p[i] = 0;
	CtoM(sret);
    targ = T_STR;
}



// get a character from the console input queue
// s$ = INKEY$
void fun_inkey(void){
    int i;

	sret = GetTempStrMemory();									// this buffer is automatically zeroed so the string is zero size

	i = MMInkey();
	if(i != -1) {
		sret[0] = 1;												// this is the length
		sret[1] = i;												// and this is the character
	}
    targ = T_STR;
}



// used by ACos() and ASin() below
MMFLOAT arcsinus(MMFLOAT x) {
     return 2.0 * atan(x / (1.0 + sqrt(1.0 - x * x)));
}


// Return the arcsine (in radians) of the argument 'number'.
// n = ASIN(number)
void fun_asin(void) {
     MMFLOAT f = getnumber(ep);
     if(f < -1.0 || f > 1.0) error("Number out of bounds");
     if (f == 1.0) {
          fret = PI_VALUE/2;
     } else if (f == -1.0) {
          fret = -PI_VALUE/2;
     } else {
          fret = arcsinus(f);
     }
     fret *=optionangle;
    targ = T_NBR;
}


// Return the arccosine (in radians) of the argument 'number'.
// n = ACOS(number)
void fun_acos(void) {
     MMFLOAT f = getnumber(ep);
     if(f < -1.0 || f > 1.0) error("Number out of bounds");
     if (f == 1.0) {
          fret = 0.0;
     } else if (f == -1.0) {
          fret = PI_VALUE;
     } else {
          fret = PI_VALUE/2 - arcsinus(f);
     }
     fret *=optionangle;
     targ = T_NBR;
}


// MAX( and MIN( are one token, as on the PicoMite: tokenise() writes them TOPBOTTOM(A, and TOPBOTTOM(I,
// n = MAX( arg1 [, arg2 [, ...]] )   n = MIN( arg1 [, arg2 [, ...]] )
void fun_max_min(void) {
    int i, cmp;
    MMFLOAT nbr, f;
    getargs(&ep, (MAX_ARG_COUNT * 2) - 1, ",");
    if((argc & 1) != 1 || argc < 3) error("Syntax");
    cmp = toupper(*argv[0]);
    if(cmp != 'A' && cmp != 'I') error("Syntax");
    cmp = (cmp == 'A');                                             // true for MAX, false for MIN
    nbr = getnumber(argv[2]);                                       // start from the first value: a starting bound
    for(i = 4; i < argc; i += 2) {                                  // (FLT_MAX) clamped MAX(-1e300, -2e300) at -3.4e38
        f = getnumber(argv[i]);
		if(cmp && f > nbr) nbr = f;
		if(!cmp && f < nbr) nbr = f;
    }
    fret = nbr;
    targ = T_NBR;
}
void fun_ternary(void){
    MMFLOAT f = 0;
    long long int i64 = 0;
    char *s = NULL;
    int t = T_NOTYPE;
	getargs(&ep,5,",");
	if(argc!=5)error("Syntax");
	MMFLOAT which=getnumber(argv[0]);                              // (not truncated: CHOICE(0.5, a, b) is a)
	if(which != 0){
		evaluate(argv[2], &f, &i64, &s, &t, false);
	} else {
		evaluate(argv[4], &f, &i64, &s, &t, false);
	}
	if(t & T_INT){
		iret=i64;
		targ=T_INT;
		return;
	} else if (t & T_NBR){
		fret=f;
		targ=T_NBR;
		return;
	} else if (t & T_STR){
		sret=GetTempStrMemory();
		Mstrcpy(sret, s);                                   // copy the string
		targ=T_STR;
		return;
	} else error("Syntax");
}
