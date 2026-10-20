/***************************************************************************

CMM2 MMBasic
FileIO.c

Does all the SD Card related file I/O in MMBasic.

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

------------------------------------------------------------------------------
  * In addition the software components from STMicroelectronics are provided
  * subject to the license as detailed below:
------------------------------------------------------------------------------
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
*******************************************************************************/


#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"

//** SD CARD INCLUDES ***********************************************************
#include "ff.h"
#include "diskio.h"
//#include "integer.h"
#include "gifdec.h"
#include "ltdc.h"
#include "picojpeg.h"

int OptionFileErrorAbort = true;
extern void Display_Refresh(void);
extern BYTE MDD_SDSPI_CardDetectState(void);
// FatFs definitions
FATFS FatFs;
FATFS FatFs_USB;                                                    // a USB flash drive, C: (USBDrive.c)
bool usb_msc_is_mounted(void);
extern volatile BYTE USBDriveStat;
char FileGetChar(int fnbr);
char FilePutChar(char c, int fnbr);
int FileEOF(int fnbr);
void File_CloseAll(void);
int InitSDCard(void);
char *ChangeToDir(char *p);
void LoadImage(char *p);
void LoadPNG(char *p);
void LoadGIF(char *p);
void LoadFont(char *p);
void cmd_LoadJPGImage(char *p);
int dirflags;
FRESULT FSerror;
volatile BYTE SDCardStat = STA_NOINIT | STA_NODISK;
volatile int diskcheckrate = 0;
extern 	volatile unsigned int checkSD;
#define SDbufferSize 512
extern unsigned int __attribute__((section(".my_section"))) _excep_cause;
extern int as_strcmpi (const char *s1, const char *s2);
int jxpos=0, jypos=0;
extern uint8_t *FontBuffer;
extern RTC_HandleTypeDef hrtc;
gd_GIF *gif=NULL;
unsigned char *frame;
int giffnbr=0,gifxOrigin, gifyOrigin;
extern volatile uint8_t pagesetdone;
char filepath[FF_MAX_LFN]="A:/";
void DrivePath(char *out);
void DriveCurrent(void);
extern int docheck;
extern LTDC_LayerCfgTypeDef pLayerCfg;
char fullpathname[FF_MAX_LFN];
char fullfilepathname[FF_MAX_LFN];
typedef struct sa_dlist {
    char from[STRINGSIZE];
    char to[STRINGSIZE];
} a_dlist;
a_dlist *dlist;

int nDefines;
int LineCount=0;
// 8*8*4 bytes * 3 = 768
int16_t *gCoeffBuf;

// 8*8*4 bytes * 3 = 768
uint8_t *gMCUBufR;
uint8_t *gMCUBufG;
uint8_t *gMCUBufB;

// 256 bytes
int16_t *gQuant0;
int16_t *gQuant1;
uint8_t *gHuffVal2;
uint8_t *gHuffVal3;
uint8_t *gInBuf;
#define BLOCK_SIZE 4096
/*****************************************************************************************
Mapping of errors reported by the file system to MMBasic file errors
*****************************************************************************************/
const int ErrorMap[] = {        0,                                  // 0
                                1,                                  // Assertion failed
                                2,                                  // Low level I/O error
                                3,                                  // No response from SDcard
                                4,                                  // Could not find the file
                                5,                                  // Could not find the path
                                6,                                  // The path name format is invalid
                                7,                                  // Prohibited access or directory full
                                8,                                  // Directory exists or path to it cannot be found
                                9,                                  // The file/directory object is invalid
                               10,                                  // SD card is write protected
                               11,                                  // The logical drive number is invalid
                               12,                                  // The volume has no work area
                               13,                                  // Not a FAT volume
                               14,                                  // Format aborted
                               15,                                  // Could not access volume
                               16,                                  // File sharing policy
                               17,                                  // Buffer could not be allocated
                               18,                                  // Too many open files
                               19,                                  // Parameter is invalid
							   20									// Not present
                            };

/******************************************************************************************
Text for the file related error messages reported by MMBasic
******************************************************************************************/
const char *FErrorMsg[] = {	"",
		"A hard error occurred in the low level disk I/O layer",
		"Assertion failed",
		"SD Card not found",
		"Could not find the file",
		"Could not find the path",
		"The path name format is invalid",
		"Access denied due to prohibited access or directory full",
		"Access denied due to prohibited access",
		"The file/directory object is invalid",
		"The physical drive is write protected",
		"The logical drive number is invalid",
		"The volume has no work area",
		"There is no valid FAT volume",
		"The f_mkfs() aborted due to any problem",
		"Could not get a grant to access the volume within defined period",
		"The operation is rejected according to the file sharing policy",
		"LFN working buffer could not be allocated",
		"Number of open files > FF_FS_LOCK",
		"Given parameter is invalid",
		"SD card not present",
		"USB drive not present"                                     // 21 (InitUSBDrive)
};




//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// MMBASIC COMMANDS & FUNCTIONS FOR THE SDCARD /////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup the SD Card based on the settings saved in flash



void cmd_save(void) {
    int fnbr;
    unsigned int nbr;
    char *p, *pp, *flinebuf;
    int x,y,w,h, filesize;
    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
    unsigned char bmppad[3] = {0,0,0};
    int i,j;
    if(!InitSDCard()) return;
	int maxH=PageTable[ReadPage].ymax;
    int maxW=PageTable[ReadPage].xmax;
    if((p = checkstring(cmdline, "CONTEXT")) != NULL) {            // SAVE CONTEXT [CLEAR]: the globals, to A:/.vars
        SaveContext();
        if(checkstring(p, "CLEAR")) ClearGlobals();
        else checkend(p);
        return;
    }
    fnbr = FindFreeFileNbr();
    if((p = checkstring(cmdline, "IMAGE")) !=NULL){
    	getargs(&p,9,",");
        if((void *)ReadBuffer == (void *)DisplayNotSet) error("SAVE IMAGE not available on this display");
        pp = getFstring(argv[0]);
        if(argc!=1 && argc!=9)error("Syntax");
        if(strchr(pp, '.') == NULL) strcat(pp, ".BMP");
        if(!BasicFileOpen(pp, fnbr, FA_WRITE | FA_CREATE_ALWAYS)) return;
        if(argc==1){
        	x=0; y=0; h=maxH; w=maxW;
        } else {
        	x=getint(argv[2],0,maxW-1);
        	y=getint(argv[4],0,maxH-1);
        	w=getint(argv[6],1,maxW-x);
        	h=getint(argv[8],1,maxH-y);
    		if(optiony) y=maxH-y-h;                                 // Y axis up: the area from y up, as BOX draws it (x was mirrored
    		                                                        // too, and the area below y saved)
        }
    	int savey=optiony;
    	optiony=0;
        filesize=54 + 3*w*h;
        bmpfileheader[ 2] = (unsigned char)(filesize    );
        bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
        bmpfileheader[ 4] = (unsigned char)(filesize>>16);
        bmpfileheader[ 5] = (unsigned char)(filesize>>24);

        bmpinfoheader[ 4] = (unsigned char)(       w    );
        bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
        bmpinfoheader[ 6] = (unsigned char)(       w>>16);
        bmpinfoheader[ 7] = (unsigned char)(       w>>24);
        bmpinfoheader[ 8] = (unsigned char)(       h    );
        bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
        bmpinfoheader[10] = (unsigned char)(       h>>16);
        bmpinfoheader[11] = (unsigned char)(       h>>24);
		f_write(FileTable[fnbr].fptr, bmpfileheader, 14, &nbr);
		f_write(FileTable[fnbr].fptr, bmpinfoheader, 40, &nbr);
        flinebuf = GetTempMemory(maxW * 4);
        for(i = y+h-1; i >= y; i--){
           ReadBuffer(x, i, x+w-1, i, flinebuf);
           pp=flinebuf;
           if(VideoColour==12){
        	   for(j=0;j<maxW*4;j++){
        		   if(j % 4 != 3)*pp++=flinebuf[j];
        	   }
           }
           f_write(FileTable[fnbr].fptr, flinebuf, w*3, &nbr);
           if((w*3) % 4 !=0) f_write(FileTable[fnbr].fptr, bmppad, 4-((w*3) % 4) , &nbr);
           if(CurrentlyPlaying != P_NOTHING){
			   f_sync(FileTable[fnbr].fptr);
			   MM_Delay(1);
           }
        }
        optiony=savey;
    } else if((p = checkstring(cmdline, "DATA")) !=NULL){
		getargs(&p,5,",");
		if(argc!=5)error("Syntax");
		pp = getFstring(argv[0]);
		if(strchr(pp, '.') == NULL) strcat(pp, ".DAT");
		uint32_t address=(GetPeekAddr(argv[2]) & 0b11111111111111111111111111111100);
		uint32_t size=getint(argv[4],1,0x7FFFFFFF);
		for(uint32_t i=address;i<address+size;i++)if(!PEEKRANGE(i)) error("Address");
		if(!BasicFileOpen(pp, fnbr, FA_WRITE | FA_CREATE_ALWAYS)) return;
		f_write(FileTable[fnbr].fptr,  (char *)address, size, &nbr);
		if(nbr!=size)error("File write error");
    } else {
    	error("Syntax");
    }

    FileClose(fnbr);
}
int cmpstr(char *s1,char *s2)
{
  unsigned char *p1 = (unsigned char *) s1;
  unsigned char *p2 = (unsigned char *) s2;
  unsigned char c1, c2;

  if (p1 == p2)
    return 0;

  do
    {
      c1 = tolower (*p1++);
      c2 = tolower (*p2++);
      if (c1 == '\0') return 0;
    }
  while (c1 == c2);

  return c1 - c2;
}

int massage(char *buff){
	int i=nDefines;
	while(i--){
		char *p=dlist[i].from;
		while(*p){
			*p=toupper(*p);
			p++;
		}
		p=dlist[i].to;
		while(*p){
			*p=toupper(*p);
			p++;
		}
		STR_REPLACE(buff,dlist[i].from,dlist[i].to,false);
	}
	STR_REPLACE(buff,"=<","<=",3);
	STR_REPLACE(buff,"=>",">=",3);
	STR_REPLACE(buff," ,",",",3);
	STR_REPLACE(buff,", ",",",3);
	STR_REPLACE(buff," *","*",3);
	STR_REPLACE(buff,"* ","*",3);
	STR_REPLACE(buff,"- ","-",3);
	STR_REPLACE(buff," /","/",3);
	STR_REPLACE(buff,"/ ","/",3);
	STR_REPLACE(buff,"= ","=",3);
	STR_REPLACE(buff,"+ ","+",3);
	STR_REPLACE(buff," )",")",3);
	STR_REPLACE(buff,") ",")",3);
	STR_REPLACE(buff,"( ","(",3);
	STR_REPLACE(buff,"> ",">",3);
	STR_REPLACE(buff,"< ","<",3);
	STR_REPLACE(buff," '","'",3);
	return strlen(buff);
}
void importfile(char *pp, char *tp, char **p, uint32_t buf, int convertdebug){
    int blockcomment = 0;                                           // within /* ... */ (a block comment)
    int fnbr;
    char buff[256];
    char qq[FF_MAX_LFN] = {0};
    char num[10];
    int importlines=0;
    int ignore=0;
	char *fname, *sbuff, *op, *ip;
    int c, f, slen, data;
    fnbr = FindFreeFileNbr();
    char  *q;
    if((q=strchr(tp,34)) == 0) error("Syntax");
    q++;
    if((q=strchr(q,34)) == 0) error("Syntax");
    fname = getFstring(tp);
	if(strchr(&fname[strlen(fname)-4], '.') == NULL) strcat(fname, ".INC");
    f=strlen(fname);
	q=&fname[strlen(fname)-4];
	if(strcasecmp(q,".inc")!=0)error("must be a .inc file");
	if(!(fname[1]==':' || fname[0] == 92 || fname[0]==47)) {strcpy(qq,pp);strcat(qq,fname);}
	else strcpy(qq,fname);
	BasicFileOpen(qq, fnbr, FA_READ);
    while(!FileEOF(fnbr)) {
    	int toggle=0, len=0;// while waiting for the end of file
    	sbuff=buff;
        if(((uint32_t)*p - buf) >= EDIT_BUFFER_SIZE - 256*6) error("Not enough memory");
        mymemset(buff,0,256);
		MMgetline(fnbr, (char *)buff);									    // get the input line
		data=0;
		importlines++;
		LineCount++;
		routinechecks(1);
		len=strlen(buff);
		toggle=0;
		for(c=0;c<strlen(buff);c++){
			if(buff[c] == TAB) buff[c] = ' ';
		}
		while(*sbuff==' '){
			sbuff++;
			len--;
		}
		if(blockcomment || (sbuff[0] == '/' && sbuff[1] == '*')) {   // a /* ... */ block comment (the PicoMite's): its lines
			if(!blockcomment) blockcomment = (strstr(&sbuff[2], "*/") == NULL);   // dropped, from the /* line to the */ line
			else if(sbuff[0] == '*' && sbuff[1] == '/') blockcomment = 0;     // (a /* line with its own */ is a line)
			continue;
		}
		if(ignore && !(sbuff[0]=='#' && cmpstr("COMMENT END",&sbuff[1])==0))*sbuff='\'';   // (# lines too, but its END)
		if(strncasecmp(sbuff,"rem ",4)==0 || (len==3 && strncasecmp(sbuff,"rem",3)==0 )){
			sbuff+=2;
			*sbuff='\'';
			continue;
		}
		if(strncasecmp(sbuff,"mmdebug",7)==0 && !isnamechar(sbuff[7]) && convertdebug==1){   // the word (not MMDEBUGGER), as the program's own
			sbuff+=6;
			*sbuff='\'';
			continue;
		}
		if(strncasecmp(sbuff,"data ",5)==0)data=1;
		slen=len;
		op=sbuff;
		ip=sbuff;
		while(*ip){
			if(*ip==34){
				if(toggle==0)toggle=1;
				else toggle=0;
			}
			if(!toggle && (*ip==' ' || *ip==':')){
				*op++=*ip++; //copy the first space
				while(*ip==' '){
					ip++;
					len--;
				}
			}
			else *op++=*ip++;
		}
		slen=len;
		if(!(toupper(sbuff[0])=='R' && toupper(sbuff[1])=='U' && toupper(sbuff[2])=='N' && (strlen(sbuff)==3 || sbuff[3]==' '))){
			toggle=0;
			for(c=0;c<slen;c++){
				if(!(toggle || data))sbuff[c]=toupper(sbuff[c]);
				if(sbuff[c]==34){
					if(toggle==0)toggle=1;
					else toggle=0;
				}
			}
		}
		toggle=0;
		for(c=0;c<slen;c++){
			if(sbuff[c]==34){
				if(toggle==0)toggle=1;
				else toggle=0;
			}
			if(!toggle && sbuff[c]==39 && len==slen){
				len=c;//get rid of comments
				break;
			}
		}
		if(sbuff[0]=='#'){
			char *tp=checkstring(&sbuff[1], "DEFINE");
			if(tp){
				getargs(&tp,3,",");
				if(nDefines>=MAXDEFINES){
				    FreeMemorySafe((void *)&buf);
				    FreeMemorySafe((void *)&dlist);
					error("Too many #DEFINE statements");
				}
				strcpy(dlist[nDefines].from,getFstring(argv[0]));
				strcpy(dlist[nDefines].to,getFstring(argv[2]));
				nDefines++;
			} else {
				if(cmpstr("COMMENT END",&sbuff[1])==0)ignore=0;
				if(cmpstr("COMMENT START",&sbuff[1])==0)ignore=1;
				if(cmpstr("MMDEBUG ON",&sbuff[1])==0)convertdebug=0;
				if(cmpstr("MMDEBUG OFF",&sbuff[1])==0)convertdebug=1;
				if(cmpstr("INCLUDE ",&sbuff[1])==0){
					error("Can't import from an import");
				}
			}
		} else {
			// the line and its '|file,line marker must fit in buff: check before appending (this used to
			// write the marker first and check after), using the whole buffer (leading spaces were stepped over)
			if(sbuff != buff){ memmove(buff, sbuff, len); sbuff = buff; }
			IntToStr(num,importlines,10);
			if(len + (toggle ? 1 : 0) + 3 + f + (int)strlen(num) > 254){
				FreeMemorySafe((void *)&buf);
				FreeMemorySafe((void *)&dlist);
				error("Line % of $ is too long", importlines, fname);
			}
			if(toggle)sbuff[len++]=34;
			sbuff[len++]=39;
			sbuff[len++]='|';
			memcpy(&sbuff[len],fname,f);
			len+=strlen(fname);
			sbuff[len++]=',';
			strcpy(&sbuff[len],num);
			len+=strlen(num);
			sbuff[len]=0;
			strreplace_overflow = 0;
			len=massage(sbuff); //can't risk crushing lines with a quote in them
			if(strreplace_overflow){                                 // a #DEFINE made the line longer than buff
				FreeMemorySafe((void *)&buf);
				FreeMemorySafe((void *)&dlist);
				error("Line % of $ is too long", importlines, fname);
			}
			if((sbuff[0]!=39) || (sbuff[0]==39 && sbuff[1]==39)){
				memcpy(*p,sbuff,len);
				*p+=len;
				**p='\n';
				*p+=1;
			}
		}
    }
    FileClose(fnbr);
    if(blockcomment) error("No matching */ in $", fname);
    return ;
}
// load a file into program memory
int FileLoadProgram(char *fname, int mode) {
    int fnbr, size=0;
    char *p,*op, *ip, *buf, *sbuff, name[FF_MAX_LFN]={0},buff[STRINGSIZE];
    char pp[FF_MAX_LFN] = {0};
    char num[10];
    int c;
    int convertdebug=1;
    int ignore=0;
    int blockcomment = 0;                                           // within /* ... */ (a block comment)
    nDefines=0;
    LineCount=0;
    int i,importlines=0, data;
	if(mode & 1){                                                   // (1: fname is an expression, 2: CHAIN, the globals kept)
		strcpy(buff,getFstring(fname));
	} else strcpy(buff,fname);
	if(strchr(&buff[strlen(buff)-4], '.') == NULL) strcat(buff, ".BAS");
	if(mode & 2) {                                                  // as ClearProgram, but the globals stay
		ClearRuntimeKeepGlobals();
		TraceOn = false;
		sendCRLF = 3;
	} else
    ClearProgram();												    // clear any leftovers from the previous program
    if(!InitSDCard()) return false;
    fnbr = FindFreeFileNbr();
	if(!BasicFileOpen(buff, fnbr, FA_READ)) return false;
	strcpy(name,fullfilepathname);
    i=strlen(name)-1;
    while(i>0 && !(name[i] == 92 || name[i]==47))i--;
    memcpy(pp,name,i+1);
    p = buf = GetMemory(EDIT_BUFFER_SIZE);
    dlist=GetMemory(sizeof(a_dlist)*MAXDEFINES);

    while(!FileEOF(fnbr)) {                                     // while waiting for the end of file
    	int toggle=0, len=0, slen;// while waiting for the end of file
    	sbuff=buff;
        if((p - buf) >= EDIT_BUFFER_SIZE - 256*6) error("Not enough memory");
        mymemset(buff,0,256);
		MMgetline(fnbr, (char *)buff);									    // get the input line
		data=0;
		importlines++;
		LineCount++;
    	routinechecks(1);
		len=strlen(buff);
		toggle=0;
		for(c=0;c<strlen(buff);c++){
			if(buff[c] == TAB) buff[c] = ' ';
		}
		while(sbuff[0]==' '){ //strip leading spaces
			sbuff++;
			len--;
		}
		if(blockcomment || (sbuff[0] == '/' && sbuff[1] == '*')) {   // a /* ... */ block comment (the PicoMite's): its lines
			if(!blockcomment) blockcomment = (strstr(&sbuff[2], "*/") == NULL);   // dropped, from the /* line to the */ line
			else if(sbuff[0] == '*' && sbuff[1] == '/') blockcomment = 0;     // (a /* line with its own */ is a line)
			continue;
		}
		if(ignore && !(sbuff[0]=='#' && cmpstr("COMMENT END",&sbuff[1])==0))*sbuff='\'';   // (# lines too, but its END)
		if(strncasecmp(sbuff,"rem ",4)==0 || (len==3 && strncasecmp(sbuff,"rem",3)==0 )){
			sbuff+=2;
			*sbuff='\'';
			continue;
		}
		if(strncasecmp(sbuff,"mmdebug",7)==0 && !isnamechar(sbuff[7]) && convertdebug==1){   // (the word: not MMDEBUGGER)
			sbuff+=6;
			*sbuff='\'';
			continue;
		}
		if(strncasecmp(sbuff,"data ",5)==0)data=1;
		slen=len;
		op=sbuff;
		ip=sbuff;
		while(*ip){
			if(*ip==34){
				if(toggle==0)toggle=1;
				else toggle=0;
			}
			if(!toggle && (*ip==' ' || *ip==':')){
				*op++=*ip++; //copy the first space
				while(*ip==' '){
					ip++;
					len--;
				}
			} else *op++=*ip++;
		}
		slen=len;
		if(sbuff[0]=='#'){
			char *tp=checkstring(&sbuff[1], "DEFINE");
			if(tp){
				getargs(&tp,3,",");
				if(nDefines>=MAXDEFINES){
				    FreeMemorySafe((void *)&buf);
				    FreeMemorySafe((void *)&dlist);
					error("Too many #DEFINE statements");
				}
				strcpy(dlist[nDefines].from,getFstring(argv[0]));
				strcpy(dlist[nDefines].to,getFstring(argv[2]));
				nDefines++;
			} else {
				if(cmpstr("COMMENT END",&sbuff[1])==0)ignore=0;
				if(cmpstr("COMMENT START",&sbuff[1])==0)ignore=1;
				if(cmpstr("MMDEBUG ON",&sbuff[1])==0)convertdebug=0;
				if(cmpstr("MMDEBUG OFF",&sbuff[1])==0)convertdebug=1;
				if(cmpstr("INCLUDE",&sbuff[1])==0){
					importfile(pp, &sbuff[8],&p, (uint32_t)buf, convertdebug);
				}
			}
		} else {
			if(!(toupper(sbuff[0])=='R' && toupper(sbuff[1])=='U' && toupper(sbuff[2])=='N' && (strlen(sbuff)==3 || sbuff[3]==' '))){
				toggle=0;
				for(c=0;c<slen;c++){
					if(!(toggle || data))sbuff[c]=toupper(sbuff[c]);
					if(sbuff[c]==34){
						if(toggle==0)toggle=1;
						else toggle=0;
					}
				}
			}
			toggle=0;
			for(c=0;c<slen;c++){
				if(sbuff[c]==34){
					if(toggle==0)toggle=1;
					else toggle=0;
				}
				if(!toggle && sbuff[c]==39 && len==slen){
					len=c;//get rid of comments
					break;
				}
			}
			// the line and its '|line marker must fit in buff: check before appending (this used to write
			// the marker first and check after), using the whole buffer (leading spaces were stepped over)
			if(sbuff != buff){ memmove(buff, sbuff, len); sbuff = buff; }
			IntToStr(num,importlines,10);
			if(len + (toggle ? 1 : 0) + 2 + (int)strlen(num) > 254){
			    FreeMemorySafe((void *)&buf);
			    FreeMemorySafe((void *)&dlist);
				error("Line % is too long", importlines);
			}
			if(toggle)sbuff[len++]=34;
			sbuff[len++]=39;
			sbuff[len++]='|';
			strcpy(&sbuff[len],num);
			len+=strlen(num);
			sbuff[len]=0;
			strreplace_overflow = 0;
			len=massage(sbuff); //can't risk crushing lines with a quote in them
			if(strreplace_overflow){                                 // a #DEFINE made the line longer than buff
			    FreeMemorySafe((void *)&buf);
			    FreeMemorySafe((void *)&dlist);
				error("Line % is too long", importlines);
			}
			if((sbuff[0]!=39) || (sbuff[0]==39 && sbuff[1]==39)){
				mycpy(p,sbuff,len);
				p+=len;
				*p++='\n';
			}
		}

    }
    *p = 0;                                                         // terminate the string in RAM
    FileClose(fnbr);
    if(blockcomment) {
        FreeMemorySafe((void *)&buf);
        FreeMemorySafe((void *)&dlist);
        error("No matching */");
    }
	int load=0;
    if(Option.ProgramStartCode>=0){
		size=SaveProgramToMemory(buf, false, name);
    	uint32_t *top=(uint32_t *)((uint32_t)SDMemory+512*1024-4);
    	top--;
		i=0;
		ip=(char *)SDMemory;
		op=(char *)ProgMemory;
		while(i<size){
			i++;
			if(*ip != *op){
				load=1;
				break;
			}
			op++;
			ip++;
		}
		while(*top-- == 0xFFFFFFFF){};
		size=(uint32_t)top - (uint32_t)SDMemory+256;
		FreeMemorySafe((void *)&SDMemory);
    }
    if(load || Option.ProgramStartCode<0)SaveProgramToFlash(buf, false, name, size);
    FreeMemorySafe((void *)&buf);
    FreeMemorySafe((void *)&dlist);
    return true;
}
#define skipzero(x)    while(*x == 0) x++
void LoadFONT(char *p){
    char buff[256], *pp;
    int i, f, n, y, x, fnbr, bcount;
    uint32_t c;
	getargs(&p, 1, ",");                                            // this MUST be the first executable line in the function
    if(argc == 0) error("Argument count");
    if(!InitSDCard()) return;

    p = getFstring(argv[0]);                                        // get the file name

	// open the file
	if(strchr(p, '.') == NULL) strcat(p, ".FNT");
	fnbr = FindFreeFileNbr();
    if(!BasicFileOpen(p, fnbr, FA_READ)) return;
	MMgetline(fnbr, (char *)buff);									    // get the input line
	for(c=0;c<strlen(buff);c++){
		if(buff[c] == TAB) buff[c] = 0;
		if(buff[c] == ' ') buff[c] = 0;
	}
	pp=buff+10;
	if(!mem_equal(buff,"DefineFont",10))error("Syntax");
	skipzero(pp);
	if(*pp!='#')error("Syntax");
	pp++;
	f = (strtoul(pp, NULL, 10)-1);
	if(f!=7)error("Fonts load into #8");
	MMgetline(fnbr, (char *)buff);									    // get the input line
	for(c=0;c<strlen(buff);c++){
		if(buff[c] == TAB) buff[c] = ' ';
	}
	pp=buff;
	c = strtoul(pp, NULL, 16);
	n=c>>24;
	if(((c>>16) & 0xff) < 0x20)error("Can't define non-printing characters");
	y=(c>>8)&0xff;
	x=c & 0xff;
	if((x*y)%8)error("Height x width not divisible by 8");
	bcount=(n*x*y)>>3;
	FontTable[7]=FontBuffer=GetMemory(bcount+4); //don't forget the header!!!
	uint32_t *dcopy=(uint32_t *)FontBuffer;
	*dcopy++=c; //store the font header;
    while(!FileEOF(fnbr)) {
		MMgetline(fnbr, (char *)buff);									    // get the input line
		for(c=0;c<strlen(buff);c++){
			if(buff[c] == TAB) buff[c] = ' ';
		}
		pp=buff;
		for(i=0; i<8; i++){
			if(bcount>0){
				skipspace(pp);
				*dcopy++ = strtoul(pp, &pp, 16);
				bcount-=4;
			}
		}
    }
    FileClose(fnbr);
}

void cmd_load(void) {
    char *p;

    p = checkstring(cmdline, "CONTEXT");                            // LOAD CONTEXT [KEEP]: the globals SAVE CONTEXT wrote
	if(p) {
        int keep = checkstring(p, "KEEP") != NULL;
        if(!keep) checkend(p);
        RestoreContext(keep);
        return;
    }
    p = checkstring(cmdline, "BMP");
	if(!p) p = checkstring(cmdline, "IMAGE");                       // (the PicoMite's name for it: file$ [, x, y])
	if(p) {
        LoadImage(p);
       return;
    }
    p = checkstring(cmdline, "JPG");
	if(p) {
		cmd_LoadJPGImage(p);
       return;
    }
    p = checkstring(cmdline, "GIF");
	if(p) {
        LoadGIF(p);
       return;
    }
    p = checkstring(cmdline, "PNG");
	if(p) {
        LoadPNG(p);
       return;
    }
    p = checkstring(cmdline, "FONT");
	if(p) {
        LoadFONT(p);
       return;
    }
	p = checkstring(cmdline, "DATA");
	if(p) {
	    int fnbr;
	    unsigned int nbr;
		static FILINFO fnod;
	    char *pp;
		getargs(&p,3,",");
		if(argc!=3)error("Syntax");
		if(!InitSDCard()) error((char *)FErrorMsg[20]);					// setup the SD card
		pp = getFstring(argv[0]);
		if(strchr(pp, '.') == NULL) strcat(pp, ".DAT");
		uint32_t address=(GetPokeAddr(argv[2]) & 0b11111111111111111111111111111100);
		FSerror = f_stat(pp, &fnod);
		if(FSerror != FR_OK)error((char *)FErrorMsg[4]);
		if((fnod.fattrib & AM_DIR))error((char *)FErrorMsg[4]);
		uint32_t size=fnod.fsize;
		for(uint32_t i=address;i<address+size;i++)if(!POKERANGE(i)) error("Address");
	    fnbr = FindFreeFileNbr();
		if(!BasicFileOpen(pp, fnbr, FA_READ)) return;
		f_read(FileTable[fnbr].fptr,  (char *)address, size, &nbr);
		if(nbr!=size)error("File read error");
	    FileClose(fnbr);
	    return;
	}
	error("Syntax");
}



// search for a volume label, directory or file
// s$ = DIR$(fspec, DIR|FILE|ALL)       will return the first entry
// s$ = DIR$()                          will return the next
// If s$ is empty then no (more) files found
void fun_dir(void) {
    static DIR djd;
    char *p;
    static FILINFO fnod;
    static char pp[FF_MAX_LFN];
    static char drive = 'A';                                        // the drive listed: the SD card's state is A:'s only
    char q[FF_MAX_LFN]={0};
    getargs(&ep, 3, ",");
    if(argc != 0) dirflags = -1;
    if(!(argc <= 3)) error("Syntax");

    if(argc == 3) {
        if(checkstring(argv[2], "DIR"))
            dirflags = AM_DIR;
        else if(checkstring(argv[2], "FILE"))
            dirflags = -1;
        else if(checkstring(argv[2], "ALL"))
            dirflags = 0;
        else
            error("Invalid flag specification");
    }


    if(argc != 0) {
    	int i, j;
        // this must be the first call eg:  DIR$("*.*", FILE)
        p = getFstring(argv[0]);
        i=strlen(p)-1;
        while(i>0 && !(p[i] == 92 || p[i]==47))i--;
        if(i>0){
        	memcpy(q,p,i);
             for(j=0;j<strlen(q);j++)if(q[j]=='\\')q[j]='/';  //allow backslash for the DOS oldies
        	i++;
        }
        strcpy(pp,&p[i]);
        if((pp[0]==47 || pp[0]==92) && i==0){
        	strcpy(q,&pp[1]);
        	strcpy(pp,q);
        	q[0] = filepath[0]; q[1] = ':'; q[2] = '/'; q[3] = 0;   // (the current drive)
        }
		if(pp[0]==0)strcpy(pp,"*");
		if(!InitSDCard()) error((char *)FErrorMsg[20]);					// setup the SD card
		fullpath(q);
		drive = toupper(fullpathname[1] == ':' ? fullpathname[0] : filepath[0]);
        if(pp[0]==0)strcpy(pp,"*");
        djd.pat = pp;
        if(!InitSDCard()) return;                                   // setup the SD card
        FSerror = f_opendir(&djd, fullpathname);
        ErrorCheck(0);
    }
        if(drive != 'C' && (disk_status(0) & STA_NOINIT)){           // (on C: it said "SD card not found" with no card)
           f_closedir(&djd);
            error("SD card not found");
        }
        if(dirflags == AM_DIR){
            for (;;) {
                FSerror = f_readdir(&djd, &fnod);		// Get a directory item
                if (FSerror != FR_OK || !fnod.fname[0]) break;	// Terminate if any error or end of directory
                if (pattern_matching(pp, fnod.fname, 0, 0) && (fnod.fattrib & AM_DIR) && !(fnod.fattrib & AM_SYS)) break;		// Test for the file name
            }
        }
        else if(dirflags == -1){
            for (;;) {
                FSerror = f_readdir(&djd, &fnod);		// Get a directory item
                if (FSerror != FR_OK || !fnod.fname[0]) break;	// Terminate if any error or end of directory
                if (pattern_matching(pp, fnod.fname, 0, 0) && !(fnod.fattrib & AM_DIR)&& !(fnod.fattrib & AM_SYS)) break;		// Test for the file name
            }
        }
        else {
            for (;;) {
                FSerror = f_readdir(&djd, &fnod);		// Get a directory item
                if (FSerror != FR_OK || !fnod.fname[0]) break;	// Terminate if any error or end of directory
                if (pattern_matching(pp, fnod.fname, 0, 0) && !(fnod.fattrib & AM_SYS)) break;		// Test for the file name
            }
        }

    if (FSerror != FR_OK || !fnod.fname[0])f_closedir(&djd);
    sret = GetTempStrMemory();                                    // this will last for the life of the command
    strcpy(sret, fnod.fname);
    CtoM(sret);                                                     // convert to a MMBasic style string
    targ = T_STR;
}
void getfullpath(char *p, char *q){
	int j;
    strcpy(q,p);
    for(j=0;j<strlen(q);j++)if(q[j]=='\\')q[j]='/';  //allow backslash for the DOS oldies
    fullpath(q);
    strcpy(q,fullpathname);
}

// a path as FatFs takes it, with its drive always given: the drive it names, or the current one ("C:/dir/name")
void DrivePath(char *out) {
    if(out[1] != ':') {
        memmove(out + 2, out, strlen(out) + 1);
        out[0] = filepath[0];
        out[1] = ':';
    }
}

// "dir/pattern" split (the directory is optional): the directory as FatFs's path, with its drive, and the pattern
static void SplitPattern(char *p, char *dir, char *pat) {
    char d[FF_MAX_LFN] = {0};
    char *s = strrchr(p, '/');
    if(s == NULL && p[0] && p[1] == ':') s = p + 1;                 // ("C:*.BAS")
    if(s != NULL) {
        memcpy(d, p, s - p);
        if(d[0] == 0 || (d[1] == ':' && d[2] == 0)) strcat(d, "/"); // the root, of this drive or of the one named
        strcpy(pat, s + 1);
    } else strcpy(pat, p);
    getfullpath(d, dir);
    DrivePath(dir);
    if(pat[0] == 0) strcpy(pat, "*");
}

// is the FatFs path a directory (a root is)
static int IsDir(char *path) {
    DIR dj;
    if(f_opendir(&dj, path) != FR_OK) return false;
    f_closedir(&dj);
    return true;
}

// one file copied, through buff (len bytes)
static void CopyOneFile(char *from, char *to, char *buff, int len) {
    unsigned int nbr = 0, bw;
    int fnbr1, fnbr2;
    fnbr1 = FindFreeFileNbr();
    if(!BasicFileOpen(from, fnbr1, FA_READ)) return;
    fnbr2 = FindFreeFileNbr();
    if(!BasicFileOpen(to, fnbr2, FA_WRITE | FA_CREATE_ALWAYS)) {
        FileClose(fnbr1);
        return;
    }
    while(!f_eof(FileTable[fnbr1].fptr)) {
        FSerror = f_read(FileTable[fnbr1].fptr, buff, len, &nbr);
        ErrorCheck(fnbr1);
        FSerror = f_write(FileTable[fnbr2].fptr, buff, nbr, &bw);
        ErrorCheck(fnbr2);
        routinechecks(1);
    }
    FileClose(fnbr1);
    FileClose(fnbr2);
}

// COPY from$ TO to$: from$ may hold wildcards (at the prompt), when to$ is a directory; to$ ending in / is a
// directory the file keeps its name in.  Either drive to either (the PicoMite's, both drives being FatFs here).
void cmd_copy(void){
	char ss[2];														// this will be used to split up the argument line
	char *fromfile, *tofile;
	ss[0] = tokenTO;
	ss[1] = 0;
	char *buff = GetTempMemory(4096);
	char a[FF_MAX_LFN] = {0}, b[FF_MAX_LFN] = {0};
	getargs(&cmdline,3,ss);
	if(argc!=3)error("Syntax");
	fromfile = getFstring(argv[0]);
	tofile = getFstring(argv[2]);
	for(char *t = fromfile; *t; t++) if(*t == '\\') *t = '/';
	for(char *t = tofile; *t; t++) if(*t == '\\') *t = '/';
	if(!InitSDCard()) error((char *)FErrorMsg[20]);
	if(strchr(fromfile, '*') || strchr(fromfile, '?')) {           // a bulk copy
		char dir[FF_MAX_LFN] = {0}, pat[FF_MAX_LFN] = {0}, todir[FF_MAX_LFN] = {0};
		DIR djd;
		FILINFO fnod;
		if(CurrentLinePtr) error("Invalid in a program");
		SplitPattern(fromfile, dir, pat);
		getfullpath(tofile, todir);
		DrivePath(todir);
		if(!IsDir(todir)) error("$ not a directory", tofile);
		if(strcasecmp(dir, todir) == 0) error("Source and destination are the same");
		mymemset(&djd, 0, sizeof(DIR));
		mymemset(&fnod, 0, sizeof(FILINFO));
		FSerror = f_findfirst(&djd, &fnod, dir, pat);
		ErrorCheck(0);
		while(FSerror == FR_OK && fnod.fname[0]) {
			if(!(fnod.fattrib & (AM_SYS | AM_HID | AM_DIR))) {
				MMPrintString("Copying ");
				MMPrintString(fnod.fname);
				PRet();
				strcpy(a, dir);
				if(a[strlen(a) - 1] != '/') strcat(a, "/");
				strcat(a, fnod.fname);
				strcpy(b, todir);
				if(b[strlen(b) - 1] != '/') strcat(b, "/");
				strcat(b, fnod.fname);
				CopyOneFile(a, b, buff, 4096);
			}
			FSerror = f_findnext(&djd, &fnod);
		}
		f_closedir(&djd);
		return;
	}
	if(tofile[0] && tofile[strlen(tofile) - 1] == '/') {           // into a directory, under its own name
		char *name = strrchr(fromfile, '/');
		name = (name != NULL) ? name + 1 : (fromfile[0] && fromfile[1] == ':') ? fromfile + 2 : fromfile;
		if(*name == 0) error("No filename specified");
		getfullpath(tofile, b);
		DrivePath(b);
		if(!IsDir(b)) error("$ not a directory", tofile);
		if(b[strlen(b) - 1] != '/') strcat(b, "/");
		strcat(b, name);
	} else {
		getfullfilepath(tofile, b);
		DrivePath(b);
	}
	getfullfilepath(fromfile, a);
	DrivePath(a);
	if(strcasecmp(a, b) == 0) error("Source and destination are the same");
	CopyOneFile(fromfile, b, buff, 4096);
}

void cmd_mkdir(void) {
    char *p;
    char q[FF_MAX_LFN]={0};
    p = getFstring(cmdline);                                        // get the directory name and convert to a standard C string
    getfullpath(p,q);
    if(!InitSDCard()) return;
    FSerror = f_mkdir(q);
    ErrorCheck(0);
}



void cmd_rmdir(void){
    char *p;
    char q[FF_MAX_LFN]={0};
    p = getFstring(cmdline);                                        // get the directory name and convert to a standard C string
    getfullpath(p,q);
    if(!InitSDCard()) return;
    FSerror = f_unlink(q);
    ErrorCheck(0);
}

void rmSubstr(char *str, const char *toRemove)
{
    size_t length = strlen(toRemove);
    char *found,
         *next = strstr(str, toRemove);

    for (size_t bytesRemoved = 0; (found = next); bytesRemoved += length)
    {
        char *rest = found + length;
        next = strstr(rest, toRemove);
        memmove(found - bytesRemoved,
                rest,
                next ? next - rest: strlen(rest) + 1);
    }
}
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
int resolve_path(char *path,char *result,char *pos)
{
    if (*path == '/') {
	*result = '/';
	pos = result+1;
	path++;
    }
    *pos = 0;
    if (!*path) return 0;
    while (1) {
	char *slash;
	struct stat st;
	st.st_mode=0;
	slash = *path ? strchr(path,'/') : NULL;
	if (slash) *slash = 0;
	if (!path[0] || (path[0] == '.' &&
	  (!path[1] || (path[1] == '.' && !path[2])))) {
	    pos--;
	    if (pos != result && path[0] && path[1])
		while (*--pos != '/');
	}
	else {
	    strcpy(pos,path);
//	    if (lstat(result,&st) < 0) return -1;
	    if (S_ISLNK(st.st_mode)) {
		char buf[PATH_MAX];
//		if (readlink(result,buf,sizeof(buf)) < 0) return -1;
		*pos = 0;
		if (slash) {
		    *slash = '/';
		    strcat(buf,slash);
		}
		strcpy(path,buf);
		if (*path == '/') result[1] = 0;
		pos = strchr(result,0);
		continue;
	    }
	    pos = strchr(result,0);
	}
	if (slash) {
	    *pos++ = '/';
	    path = slash+1;
	}
	*pos = 0;
	if (!slash) break;
    }
    return 0;
}
// the drive a path names (A: the SD card, C: a USB flash drive; 0: and B: are A:), or else the current one (filepath's).
// B: is the PicoMite's name for its SD card (A: is its flash), so PicoMite programs reach the card by either (19 Oct).
char PathDrive(const char *p) {
    char d = filepath[0];
    if(p[0] && p[1] == ':') {
        d = toupper(p[0]);
        if(d == '0' || d == 'B') d = 'A';
        if(d != 'A' && d != 'C') error("Invalid drive");
    }
    return d;
}

// each drive's current directory while the other drive is current
static char DrivePaths[2][FF_MAX_LFN] = { "A:/", "C:/" };

// drive d's current directory, "A:/DIR" (A: or C:; FM's panels start there)
const char *DriveDir(char d) {
    return (d == filepath[0]) ? filepath : DrivePaths[d == 'C'];
}

// the current drive made d (A or C), with the directory it had (the PicoMite's DRIVE)
void SetDrive(char d) {
    char old[FF_MAX_LFN];
    if(d == 'B') d = 'A';                                           // (the PicoMite's name for the SD card)
    if(d != 'A' && d != 'C') error("Invalid drive");
    if(d == filepath[0]) return;
    if(d == 'C' && !usb_msc_is_mounted()) error((char *)FErrorMsg[21]);
    if(d == 'A' && !MDD_SDSPI_CardDetectState()) error((char *)FErrorMsg[20]);
    strcpy(old, filepath);
    strcpy(DrivePaths[old[0] == 'C'], old);
    strcpy(filepath, DrivePaths[d == 'C']);
    if(!InitSDCard()) {
        strcpy(filepath, old);
        DriveCurrent();
        return;
    }
    if(f_chdir(&filepath[2]) != FR_OK) {                            // (that directory has gone: the drive's root)
        filepath[3] = 0;
        f_chdir("/");
    }
}

// DRIVE "A:" | "B:" | "C:": the SD card (A: or B:) or a USB flash drive
void cmd_drive(void) {
    char *p = getFstring(cmdline);
    if(strlen(p) != 2 || p[1] != ':') error("Invalid drive");
    SetDrive(toupper(p[0]));
}

// FatFs's current drive back to filepath's (after a path on the other drive was looked at)
void DriveCurrent(void) {
    f_chdrive(filepath[0] == 'C' ? "C:" : "A:");
}

void fullpath(char *q){
	char *p=GetTempStrMemory();
	char *rp=GetTempStrMemory();
	int i;
	strcpy(p,q);
	mymemset(fullpathname,0,sizeof(fullpathname));
	strcpy(fullpathname,filepath);
    for(i=0;i<strlen(p);i++)if(p[i]=='\\')p[i]='/';  //allow backslash for the DOS oldies
    if(strcmp(p,".")==0 || strlen(p)==0){
    	memmove(fullpathname, &fullpathname[2],strlen(fullpathname));
//    	MMPrintString("Now: ");MMPrintString(fullpathname);PRet();
    	return; //nothing to do
    }
    char drv = PathDrive(p);
    if(p[1]==':'){ //modify the requested path so that if the disk is specified the pathname is absolute and starts with /
    	if(p[2]=='/')p+=2;
    	else {
    		p[1]='/';
    		p++;
    	}
    }
    if (*p=='/'){ //absolute path specified
    	rp[0] = drv; rp[1] = ':'; rp[2] = 0;
    	strcat(rp,p);
    } else { // relative path specified
    	strcpy(rp,fullpathname); //copy the current pathname
        if(rp[strlen(rp)-1]!='/')  strcat(rp,"/"); //make sure the previous pathname ends in slash, will only be the case at root
    	strcat(rp,p); //append the new pathname
    }
	strcpy(fullpathname,rp); //set the new pathname
	resolve_path(fullpathname,rp,rp); //resolve to single absolute path
	if(rp[1]==':' && rp[2]==0)strcat(rp,"/"); //if root append the slash
	strcpy(fullpathname,rp); //store this back to the filepath variable
	if(fullpathname[0] == filepath[0])                              // on the current drive: FatFs's current drive (InitSDCard)
		memmove(fullpathname, &fullpathname[2],strlen(fullpathname));
//	MMPrintString("Now: ");MMPrintString(fullpathname);PRet();
}
void cmd_chdir(void){
	int i;
    char *p;
    char rp[STRINGSIZE],oldfilepath[STRINGSIZE];
    p = strupr(getFstring(cmdline));  // get the directory name and convert to a standard C string
    for(i=0;i<strlen(p);i++)if(p[i]=='\\')p[i]='/';  //allow backslash for the DOS oldies
    if(strcmp(p,".")==0)return; //nothing to do
    if(strlen(p)==0)return;//nothing to do
    strcpy(oldfilepath,filepath); //save the path in case the change of directory fails
    char drv = PathDrive(p);                                        // (A: or C:)
    if(drv == 'C' && !usb_msc_is_mounted()) error((char *)FErrorMsg[21]);   // (before the path changes)
    if(p[1]==':'){ //modify the requested path so that if the disk is specified the pathname is absolute and starts with /
    	if(p[2]=='/')p+=2;
    	else {
    		p[1]='/';
    		p++;
    	}
    }
    if (*p=='/'){ //absolute path specified
    	rp[0] = drv; rp[1] = ':'; rp[2] = 0;
    	strcat(rp,p);
    } else { // relative path specified
    	strcpy(rp,filepath); //copy the current pathname
        if(rp[strlen(rp)-1]!='/')  strcat(rp,"/"); //make sure the previous pathname ends in slash, will only be the case at root
    	strcat(rp,p); //append the new pathname
    }
	strcpy(filepath,rp); //set the new pathname
	resolve_path(filepath,rp,rp); //resolve to single absolute path
	if(rp[1]==':' && rp[2]==0)strcat(rp,"/"); //if root append the slash
	strcpy(filepath,rp); //store this back to the filepath variable
    if(!InitSDCard()) { //If no disk restore the old path and return
    	strcpy(filepath,oldfilepath);
    	DriveCurrent();
    	return;
    }
	FSerror = f_chdir(&filepath[2]); //finally change directory always using an absolute pathname
	if(FSerror) {                                                   // if it didn't work restore the original path
		strcpy(filepath,oldfilepath);
		DriveCurrent();
	} else if(filepath[0] != oldfilepath[0])                        // (another drive: the old one keeps its directory)
		strcpy(DrivePaths[oldfilepath[0] == 'C'], oldfilepath);
	ErrorCheck(0); // error if the pathname was invalid

}


void fun_cwd(void) {
    sret = GetTempStrMemory();                                    // this will last for the life of the command
	strcpy(sret,filepath);
    {   // the names as the card has them (f_getcwd), as the PicoMite gives them: CHDIR keeps filepath in capitals
        char b[STRINGSIZE];
        if(f_getcwd(b, sizeof(b)) == FR_OK && toupper(b[0]) == toupper(filepath[0]) && b[1] == ':' && strlen(b) <= MAXSTRLEN)
            strcpy(sret, b);
    }
    sret = CtoM(sret);
    targ = T_STR;
}

void getfullfilepath(char *p, char *q){
	int i,j;
    char pp[FF_MAX_LFN] = {0};
    i=strlen(p)-1;
    while(i>0 && !(p[i] == 92 || p[i]==47))i--;
    if(i>0){
    	memcpy(q,p,i);
    	 for(j=0;j<strlen(q);j++)if(q[j]=='\\')q[j]='/';  //allow backslash for the DOS oldies
    	i++;
    }
    strcpy(pp,&p[i]);
    if((pp[0]==47 || pp[0]==92) && i==0){
    	strcpy(q,&pp[1]);
    	strcpy(pp,q);
    	q[0] = filepath[0]; q[1] = ':'; q[2] = '/'; q[3] = 0;   // (the current drive)
    }
    fullpath(q);
    strcpy(q,fullpathname);
    if(q[strlen(q)-1]!=47)strcat(q,"/");
    strcat(q,pp);
}

// KILL file$, or at the prompt KILL pattern$ [, ALL]: each file the pattern matches is deleted after a Y/N, or with
// ALL after one for them all (the PicoMite's)
void cmd_kill(void){
    char q[FF_MAX_LFN]={0};
    getargs(&cmdline, 3, ",");
    if(argc != 1 && argc != 3) error("Syntax");
    char *p = getFstring(argv[0]);
    for(char *t = p; *t; t++) if(*t == '\\') *t = '/';
    if(!InitSDCard()) error((char *)FErrorMsg[20]);
    if(strchr(p, '*') || strchr(p, '?')) {
        char dir[FF_MAX_LFN] = {0}, pat[FF_MAX_LFN] = {0};
        DIR djd;
        FILINFO fnod;
        int all = 0, k = 'Y';
        if(argc == 3) {
            if(!checkstring(argv[2], "ALL")) error("Syntax");
            all = 1;
        }
        if(CurrentLinePtr) error("Invalid in a program");
        SplitPattern(p, dir, pat);
        if(all) {
            MMPrintString("Deleting "); MMPrintString(pat); MMPrintString(" from "); MMPrintString(dir);
            MMPrintString("\r\nAre you sure ? (Y/N) ");
            do k = toupper(MMInkey()); while(k != 'Y' && k != 'N');
            putConsole(k);
            PRet();
            if(k == 'N') return;
        }
        mymemset(&djd, 0, sizeof(DIR));
        mymemset(&fnod, 0, sizeof(FILINFO));
        FSerror = f_findfirst(&djd, &fnod, dir, pat);
        ErrorCheck(0);
        while(FSerror == FR_OK && fnod.fname[0]) {
            if(!(fnod.fattrib & (AM_SYS | AM_HID | AM_DIR))) {
                MMPrintString("Deleting ");
                MMPrintString(fnod.fname);
                if(!all) {
                    MMPrintString(" ? (Y/N) ");
                    do k = toupper(MMInkey()); while(k != 'Y' && k != 'N');
                    putConsole(k);
                }
                PRet();
                if(k == 'Y') {
                    strcpy(q, dir);
                    if(q[strlen(q) - 1] != '/') strcat(q, "/");
                    strcat(q, fnod.fname);
                    FSerror = f_unlink(q);
                    if(FSerror) f_closedir(&djd);
                    ErrorCheck(0);
                }
            }
            FSerror = f_findnext(&djd, &fnod);
        }
        f_closedir(&djd);
        docheck=1;
        return;
    }
    getfullfilepath(p,q);
    FSerror = f_unlink(q);
    ErrorCheck(0);
    docheck=1;
}



void cmd_seek(void) {
    int fnbr, idx;
    getargs(&cmdline, 5, ",");
    if(argc != 3) error("Syntax");
    if(*argv[0] == '#') argv[0]++;
    fnbr = getinteger(argv[0]);
    if(fnbr < 1 || fnbr > MAXOPENFILES || FileTable[fnbr].com <= MAXCOMPORTS) error("File number");
    if(FileTable[fnbr].com == 0) error("File number #% is not open", fnbr);
    if(!InitSDCard()) return;
    idx = getint(argv[2],1,0x7FFFFFFF) - 1;
    if(idx < 0) idx = 0;
        FSerror = f_lseek(FileTable[fnbr].fptr,idx);
        ErrorCheck(fnbr);

}


void cmd_name(void) {
    char *old, *new, ss[2];
    ss[0] = tokenAS;                                                // this will be used to split up the argument line
    ss[1] = 0;
    char qold[FF_MAX_LFN]={0};
    char qnew[FF_MAX_LFN]={0};
    {                                                               // start a new block
        getargs(&cmdline, 3, ss);                                   // getargs macro must be the first executable stmt in a block
        if(argc != 3) error("Syntax");
        old = getFstring(argv[0]);                                  // get the old name
        getfullfilepath(old,qold);
        new = getFstring(argv[2]);                                  // get the new name
        getfullfilepath(new,qnew);
        DrivePath(qold);
        DrivePath(qnew);
        if(qold[0] != qnew[0]) error("Cannot rename across drives");    // (FatFs would rename on the old one's)
        if(!InitSDCard()) return;
        FSerror = f_rename(qold, qnew);
        ErrorCheck(0);
    }
}


int BMPfnbr;                                                        // the file LOAD BMP reads (BmpDecoder.c)
bool (*linecallback)(int *imagewidth, int *imageheight, uint32_t *linedata, int *linenumber) = NULL;
// LOAD BMP file$ [, x] [, y] [, mode] [, xoffset] [, yoffset]: the image with its top left at x, y, from xoffset,
// yoffset of it (the PicoMite's LOAD IMAGE arguments).  The PicoMite's mode chooses a dither for its few-colour
// displays; the CMM2 draws in the page's colour depth and does not dither, so mode is checked and not used.
void LoadImage(char *p) {
	int xOrigin = 0, yOrigin = 0, xRead = 0, yRead = 0;
	getargs(&p, 11, ",");                                           // this MUST be the first executable line in the function
	if(argc == 0) error("Argument count");
	if(!InitSDCard()) return;
	p = getFstring(argv[0]);                                        // get the file name
	int maxH=PageTable[WritePage].ymax;
	if(argc >= 3 && *argv[2]) xOrigin = getinteger(argv[2]);        // get the x origin (optional) argument
	if(argc >= 5 && *argv[4]) {
		yOrigin = getinteger(argv[4]);                              // get the y origin (optional) argument
		if(optiony) yOrigin=maxH-1-yOrigin;
	}
	if(argc >= 7 && *argv[6]) getint(argv[6], -1, 7);              // the PicoMite's dither mode
	if(argc >= 9 && *argv[8]) xRead = getint(argv[8], 0, 3839);
	if(argc >= 11 && *argv[10]) yRead = getint(argv[10], 0, 2159);
	// open the file
	if(strchr(p, '.') == NULL) strcat(p, ".BMP");
	BMPfnbr = FindFreeFileNbr();
	if(!BasicFileOpen(p, BMPfnbr, FA_READ)) return;
	int savey=optiony;
	optiony=0;
	ReadAndDisplayBMP(BMPfnbr, -1, xRead, yRead, xOrigin, yOrigin);
	optiony=savey;
	FileClose(BMPfnbr);
}
// LOAD PNG file$ [, x [, y [, t [, cutoff [, ximage [, yimage]]]]]]: decoded a row at a time (the PicoMite's streaming
// decoder) and drawn through DrawBuffer as RGBA, so the 12-bit pages get the alpha. t is the CMM2's: 0 opaque, 1 the
// default cut, 2-15 the cut t * 16 (pixels with less alpha are not drawn); cutoff, 1-254, sets the cut itself; ximage
// and yimage are the first column and row of the image shown (both the PicoMite's). The image is clipped to the page.
typedef struct {
	int x, y, x0, x1, yOffset, maxH, flags;
} s_pngdraw;
static int pngdrawrow(void *v, unsigned y, const unsigned char *rgba)
{
	s_pngdraw *c = (s_pngdraw *)v;
	int sy = c->y + (int)y - c->yOffset;
	routinechecks(1);                                               // (keeps sound playing during a large image)
	if (sy >= c->maxH)
		return 0;                                                   // below the page: stop decoding
	if ((int)y < c->yOffset || sy < 0)
		return 1;
	DrawBuffer(c->x, sy, c->x + c->x1 - c->x0 - 1, sy, (char *)rgba + c->x0 * 4, c->flags);
	return 1;
}
void LoadPNG(char *p) {
	int xOrigin, yOrigin, w, h, transparent = 0, force = 0, xOffset = 0, yOffset = 0;
	int maxW = PageTable[WritePage].xmax;
	int maxH = PageTable[WritePage].ymax;
	upng_t *upng;
	s_pngdraw c;
	// get the command line arguments
	getargs(&p, 13, ",");                                           // this MUST be the first executable line in the function
	if(argc == 0) error("Argument count");
	if(!InitSDCard()) return;

	p = getFstring(argv[0]);                                        // get the file name

	xOrigin = yOrigin = 0;
	if(argc >= 3 && *argv[2]) xOrigin = getinteger(argv[2]);        // get the x origin (optional) argument
	if(argc >= 5 && *argv[4]){
		yOrigin = getinteger(argv[4]);                              // get the y origin (optional) argument
		if(optiony) yOrigin=maxH-1-yOrigin;
	}
	if(argc >= 7 && *argv[6]) transparent=getint(argv[6],0,15);
	if(transparent){
		force=transparent<<4;                                       // (t = 1 too: an alpha below 16 is cut; it cut nothing)
		transparent=4;
	}
	if(argc >= 9 && *argv[8]) { force = getint(argv[8], 1, 254) & 0xF0; transparent = 4; }   // the cut itself (DrawBuffer takes its top four bits)
	if(argc >= 11 && *argv[10]) xOffset = getinteger(argv[10]);      // the first image column shown
	if(argc >= 13 && *argv[12]) yOffset = getinteger(argv[12]);      // the first image row shown
	if(xOffset < 0 || yOffset < 0) error("Offset below 0");
	// open the file
	if(strchr(p, '.') == NULL) strcat(p, ".PNG");
	upng = upng_new_from_file(p);
	if(upng == NULL) return;
	upng_header(upng);
	upng_error_check(upng);                                         // (frees the decoder and reports, as "Invalid format" did)
	w = upng_get_width(upng);
	h = upng_get_height(upng);
	if(xOffset >= w || yOffset >= h){
		upng_free(upng);
		error("Offset outside the image");
	}
	// the image columns that land on the page: from xOffset, or further right when x is negative
	c.x0 = xOffset + (xOrigin < 0 ? -xOrigin : 0);
	c.x1 = (xOrigin + w - xOffset > maxW) ? xOffset + maxW - xOrigin : w;
	c.x = xOrigin < 0 ? 0 : xOrigin;
	c.y = yOrigin;
	c.yOffset = yOffset;
	c.maxH = maxH;
	c.flags = 3 | transparent | force;
	int savey=optiony;
	optiony=0;
	if(c.x1 > c.x0 && c.x < maxW) {
		upng_decode_rows(upng, pngdrawrow, &c);
		optiony=savey;
		upng_error_check(upng);
	}
	optiony=savey;
	upng_free(upng);
	clearrepeat();
}
void GIFcallback(void){
	int ret;
	int savey=optiony;
	optiony=0;
	gd_render_frame(gif,frame);
	DrawBuffer(gifxOrigin, gifyOrigin, gifxOrigin+gif->width-1, gifyOrigin+gif->height-1,(char *)frame,2);
	ret=gd_get_frame(gif);
    GifTimer=gif->gce.delay*10;
    if (ret == 0) {
    	gd_rewind(gif);
//    	ret=gd_get_frame(gif);
//        GifTimer=gif->gce.delay*10;

    }
    optiony=savey;
}
void LoadGIF(char *p) {
	int ret;
	// get the command line arguments
	getargs(&p, 5, ",");                                            // this MUST be the first executable line in the function
    if(argc == 0 || gif!=NULL) {
    	if(gif!=NULL){
    		GifTimer=5000;
    		FileClose(giffnbr);
    		FreeMemorySafe((void*)&frame);
    		gd_close_gif(gif);
    		gif=NULL;
    		GifTimer=0;
    		giffnbr=0;
    	}
		if(argc==0)return;
    }
    if(!InitSDCard()) return;
	int maxH=PageTable[WritePage].ymax;

    p = getFstring(argv[0]);                                        // get the file name

    gifxOrigin = gifyOrigin = 0;
	if(argc >= 3) gifxOrigin = getinteger(argv[2]);                    // get the x origin (optional) argument
	if(argc == 5) {
		gifyOrigin = getinteger(argv[4]);                    // get the y origin (optional) argument
		if(optiony) gifyOrigin=maxH-1-gifyOrigin;
	}

	// open the file
	if(strchr(p, '.') == NULL) strcat(p, ".GIF");
	giffnbr = FindFreeFileNbr();
    if(!BasicFileOpen(p, giffnbr, FA_READ)) return;
    GifTimer=0x5000;
    gif = gd_open_gif(giffnbr);
    frame = GetMemory(gif->width * gif->height * 3);
	ret=gd_get_frame(gif);
	routinechecks(1);
	int savey=optiony;
	optiony=0;
	if(ret!=-1){
		gd_render_frame(gif,frame);
    	routinechecks(1);
		DrawBuffer(gifxOrigin, gifyOrigin, gifxOrigin+gif->width-1, gifyOrigin+gif->height-1,(char *)frame,2);
    	routinechecks(1);
    	ret=gd_get_frame(gif);
	}
	if(ret<=0){
		FileClose(giffnbr);
		FreeMemorySafe((void*)&frame);
		gd_close_gif(gif);
		gif=NULL;
		GifTimer=0;
		giffnbr=0;
		return;
	}
    optiony=savey;
    GifTimer=gif->gce.delay*10;
}
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
static uint g_nInFileSize;
static uint g_nInFileOfs;
static int jpgfnbr;
// Dithering modes - bit layout: [method(bit2)][format(bits 1-0)]
// Bit 2: 0=Floyd-Steinberg, 1=Atkinson
// Bits 1-0: 00=RGB121, 01=RGB222, 10=RGB332, 11=reserved for RGB565
#define DITHER_NONE -1
#define DITHER_FS_RGB121 0       // 0b000
#define DITHER_FS_RGB222 1       // 0b001
#define DITHER_FS_RGB332 2       // 0b010
#define DITHER_ATKINSON_RGB121 4 // 0b100
#define DITHER_ATKINSON_RGB222 5 // 0b101
#define DITHER_ATKINSON_RGB332 6 // 0b110

// Extract mode components
#define DITHER_METHOD(mode) ((mode) >> 2)  // 0=Floyd-Steinberg, 1=Atkinson
#define DITHER_FORMAT(mode) ((mode) & 0x3) // 0=RGB121, 1=RGB222, 2=RGB332, 3=RGB565

// Format identifiers
#define FORMAT_RGB121 0
#define FORMAT_RGB222 1
#define FORMAT_RGB332 2
#define FORMAT_RGB565 3

// Convert RGB888 to RGB121 with dithering
uint8_t rgb888_to_rgb121_dither(int16_t r, int16_t g, int16_t b)
{
    r = (r < 0) ? 0 : (r > 255) ? 255
                                : r;
    g = (g < 0) ? 0 : (g > 255) ? 255
                                : g;
    b = (b < 0) ? 0 : (b > 255) ? 255
                                : b;
    uint8_t r1 = (r >= 128) ? 1 : 0;
    uint8_t g2 = (g * 3 + 127) / 255;
    uint8_t b1 = (b >= 128) ? 1 : 0;
    return (r1 << 3) | (g2 << 1) | b1;
}

// Convert RGB888 to RGB222 with dithering
uint8_t rgb888_to_rgb222_dither(int16_t r, int16_t g, int16_t b)
{
    r = (r < 0) ? 0 : (r > 255) ? 255
                                : r;
    g = (g < 0) ? 0 : (g > 255) ? 255
                                : g;
    b = (b < 0) ? 0 : (b > 255) ? 255
                                : b;
    uint8_t r2 = (r * 3 + 127) / 255;
    uint8_t g2 = (g * 3 + 127) / 255;
    uint8_t b2 = (b * 3 + 127) / 255;
    return (r2 << 4) | (g2 << 2) | b2;
}

// Convert RGB888 to RGB332 with dithering
uint8_t rgb888_to_rgb332_dither(int16_t r, int16_t g, int16_t b)
{
    r = (r < 0) ? 0 : (r > 255) ? 255
                                : r;
    g = (g < 0) ? 0 : (g > 255) ? 255
                                : g;
    b = (b < 0) ? 0 : (b > 255) ? 255
                                : b;
    uint8_t r3 = (r * 7 + 127) / 255;
    uint8_t g3 = (g * 7 + 127) / 255;
    uint8_t b2 = (b * 3 + 127) / 255;
    return (r3 << 5) | (g3 << 2) | b2;
}

// Convert quantized color back to RGB888
static inline void unpack_rgb121(uint8_t packed, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t r1 = (packed >> 3) & 1;
    uint8_t g2 = (packed >> 1) & 3;
    uint8_t b1 = packed & 1;
    *r = r1 ? 255 : 0;
    *g = (g2 * 255) / 3;
    *b = b1 ? 255 : 0;
}

static inline void unpack_rgb222(uint8_t packed, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t r2 = (packed >> 4) & 3;
    uint8_t g2 = (packed >> 2) & 3;
    uint8_t b2 = packed & 3;
    *r = (r2 * 255) / 3;
    *g = (g2 * 255) / 3;
    *b = (b2 * 255) / 3;
}

static inline void unpack_rgb332(uint8_t packed, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t r3 = (packed >> 5) & 7;
    uint8_t g3 = (packed >> 2) & 7;
    uint8_t b2 = packed & 3;
    *r = (r3 * 255) / 7;
    *g = (g3 * 255) / 7;
    *b = (b2 * 255) / 3;
}

// Dither a complete image row (across all MCUs)
static void dither_image_row(uint8_t *row_buffer, int row_width,
                             int16_t *curr_error, int16_t *next_error, int mode)
{
    int method = DITHER_METHOD(mode); // 0=Floyd-Steinberg, 1=Atkinson
    int format = DITHER_FORMAT(mode); // 0=RGB121, 1=RGB222, 2=RGB332

    int is_fs = (method == 0);

    for (int x = 0; x < row_width; x++)
    {
        uint8_t *pDst = row_buffer + (x * 3);

        // Apply error
        int16_t old_r = pDst[2] + curr_error[x * 3 + 0];
        int16_t old_g = pDst[1] + curr_error[x * 3 + 1];
        int16_t old_b = pDst[0] + curr_error[x * 3 + 2];

        // Quantize based on format
        uint8_t packed, new_r, new_g, new_b;

        switch (format)
        {
        case FORMAT_RGB121:
            packed = rgb888_to_rgb121_dither(old_r, old_g, old_b);
            unpack_rgb121(packed, &new_r, &new_g, &new_b);
            break;
        case FORMAT_RGB222:
            packed = rgb888_to_rgb222_dither(old_r, old_g, old_b);
            unpack_rgb222(packed, &new_r, &new_g, &new_b);
            break;
        case FORMAT_RGB332:
            packed = rgb888_to_rgb332_dither(old_r, old_g, old_b);
            unpack_rgb332(packed, &new_r, &new_g, &new_b);
            break;
        default: // FORMAT_RGB565 - not yet implemented
            new_r = old_r;
            new_g = old_g;
            new_b = old_b;
            break;
        }

        // Clamp for error calculation
        if (old_r < 0)
            old_r = 0;
        if (old_r > 255)
            old_r = 255;
        if (old_g < 0)
            old_g = 0;
        if (old_g > 255)
            old_g = 255;
        if (old_b < 0)
            old_b = 0;
        if (old_b > 255)
            old_b = 255;

        // Calculate errors
        int16_t err_r = old_r - new_r;
        int16_t err_g = old_g - new_g;
        int16_t err_b = old_b - new_b;

        // Write result
        pDst[2] = new_r;
        pDst[1] = new_g;
        pDst[0] = new_b;

        // Distribute error
        if (is_fs)
        {
            // Floyd-Steinberg distribution
            if (x + 1 < row_width)
            {
                curr_error[(x + 1) * 3 + 0] += (err_r * 7) / 16;
                curr_error[(x + 1) * 3 + 1] += (err_g * 7) / 16;
                curr_error[(x + 1) * 3 + 2] += (err_b * 7) / 16;
            }

            if (x > 0)
            {
                next_error[(x - 1) * 3 + 0] += (err_r * 3) / 16;
                next_error[(x - 1) * 3 + 1] += (err_g * 3) / 16;
                next_error[(x - 1) * 3 + 2] += (err_b * 3) / 16;
            }

            next_error[x * 3 + 0] += (err_r * 5) / 16;
            next_error[x * 3 + 1] += (err_g * 5) / 16;
            next_error[x * 3 + 2] += (err_b * 5) / 16;

            if (x + 1 < row_width)
            {
                next_error[(x + 1) * 3 + 0] += (err_r * 1) / 16;
                next_error[(x + 1) * 3 + 1] += (err_g * 1) / 16;
                next_error[(x + 1) * 3 + 2] += (err_b * 1) / 16;
            }
        }
        else
        {
            // Atkinson distribution
            if (x + 1 < row_width)
            {
                curr_error[(x + 1) * 3 + 0] += err_r / 8;
                curr_error[(x + 1) * 3 + 1] += err_g / 8;
                curr_error[(x + 1) * 3 + 2] += err_b / 8;
            }

            if (x + 2 < row_width)
            {
                curr_error[(x + 2) * 3 + 0] += err_r / 8;
                curr_error[(x + 2) * 3 + 1] += err_g / 8;
                curr_error[(x + 2) * 3 + 2] += err_b / 8;
            }

            if (x > 0)
            {
                next_error[(x - 1) * 3 + 0] += err_r / 8;
                next_error[(x - 1) * 3 + 1] += err_g / 8;
                next_error[(x - 1) * 3 + 2] += err_b / 8;
            }

            next_error[x * 3 + 0] += err_r / 8;
            next_error[x * 3 + 1] += err_g / 8;
            next_error[x * 3 + 2] += err_b / 8;

            if (x + 1 < row_width)
            {
                next_error[(x + 1) * 3 + 0] += err_r / 8;
                next_error[(x + 1) * 3 + 1] += err_g / 8;
                next_error[(x + 1) * 3 + 2] += err_b / 8;
            }
        }
    }
}

unsigned char pjpeg_need_bytes_callback(unsigned char *pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data)
{
    uint n, n_read;
//    pCallback_data;

    n = min(g_nInFileSize - g_nInFileOfs, buf_size);
    f_read(FileTable[jpgfnbr].fptr, pBuf, n, &n_read);
    if (n != n_read)
        return PJPG_STREAM_READ_ERROR;
    *pBytes_actually_read = (unsigned char)(n);
    g_nInFileOfs += n;
    return 0;
}
// LOAD JPG file$ [, x [, y [, dither [, xoffset [, yoffset [, scale]]]]]]: the PicoMite's (decoded a row of MCUs at a time;
// xoffset and yoffset crop the source, scale 2, 4 or 8 averages blocks of pixels, dither 0-2 and 4-6 diffuse the error
// to RGB121, RGB222 or RGB332, Floyd-Steinberg or Atkinson; RGB332 suits the 8-bit modes)
void cmd_LoadJPGImage(char *p)
{
    pjpeg_image_info_t image_info;
    int mcu_x = 0;
    int mcu_y = 0;
    uint8_t status;
    int dither_mode = -1;

    gCoeffBuf = (int16_t *)GetTempMemory(8 * 8 * sizeof(int16_t));
    gMCUBufR = (uint8_t *)GetTempMemory(256);
    gMCUBufG = (uint8_t *)GetTempMemory(256);
    gMCUBufB = (uint8_t *)GetTempMemory(256);
    gQuant0 = (int16_t *)GetTempMemory(8 * 8 * sizeof(int16_t));
    gQuant1 = (int16_t *)GetTempMemory(8 * 8 * sizeof(int16_t));
    gHuffVal2 = (uint8_t *)GetTempMemory(256);
    gHuffVal3 = (uint8_t *)GetTempMemory(256);
    gInBuf = (uint8_t *)GetTempMemory(PJPG_MAX_IN_BUF_SIZE);
    g_nInFileSize = g_nInFileOfs = 0;

    int xOrigin, yOrigin;
    int xOffset, yOffset;
    int scale = 1; // pixel-binning factor: 1=full, 2=50%, 4=25%, 8=12.5%

    getargs(&p, 13, ",");
    if (argc == 0)
        StandardError(2);
    if (!InitSDCard())
        return;

    p = (char *)getFstring(argv[0]);

    xOrigin = yOrigin = 0;
    xOffset = yOffset = 0;

    if (argc >= 3 && *argv[2])
        xOrigin = getint(argv[2], 0, HRes - 1);
    if (argc >= 5 && *argv[4])
        yOrigin = getint(argv[4], 0, VRes - 1);
    if (argc >= 7 && *argv[6])
        dither_mode = getint(argv[6], -1, 7);
    if (dither_mode == 3 || dither_mode == 7)
        error("RGB565 dithering not yet supported");
    // the offsets and the scale are read before the file is opened, so an error in them leaves nothing open (the
    // PicoMite's read them after, with getint, whose error left the file open); their ranges are checked below
    if (argc >= 9 && *argv[8])
        xOffset = getinteger(argv[8]);
    if (argc >= 11 && *argv[10])
        yOffset = getinteger(argv[10]);
    if (argc >= 13 && *argv[12])
    {
        scale = getint(argv[12], 1, 8);
        if (scale != 1 && scale != 2 && scale != 4 && scale != 8)
            error("Scale must be 1, 2, 4 or 8");
    }

    if (strchr((char *)p, '.') == NULL)
        strcat((char *)p, ".jpg");
    jpgfnbr = FindFreeFileNbr();
    if (!BasicFileOpen((char *)p, jpgfnbr, FA_READ))
        return;
    g_nInFileSize = f_size(FileTable[jpgfnbr].fptr);
    status = pjpeg_decode_init(&image_info, pjpeg_need_bytes_callback, NULL, 0);

    if (status)
    {
        if (status == PJPG_UNSUPPORTED_MODE)
        {
            FileClose(jpgfnbr);
            error("Progressive JPEG files are not supported");
        }
        FileClose(jpgfnbr);
        error("pjpeg_decode_init() failed with status %", status);
    }

    if (xOffset < 0 || xOffset > image_info.m_width - 1)
    {
        FileClose(jpgfnbr);
        error("% is invalid (valid is 0 to %)", xOffset, image_info.m_width - 1);
    }
    if (yOffset < 0 || yOffset > image_info.m_height - 1)
    {
        FileClose(jpgfnbr);
        error("% is invalid (valid is 0 to %)", yOffset, image_info.m_height - 1);
    }

    int mcu_row_width = image_info.m_width * image_info.m_comps;
    int mcu_row_height = image_info.m_MCUHeight;
    unsigned char *mcu_row_buffer = GetTempMemory(mcu_row_height * mcu_row_width);

    // When binning, decoded source lines are averaged into this one output line.
    // MCU height is always a multiple of 8, so it divides evenly by 2, 4 or 8 and
    // a bin never straddles an MCU-row boundary.
    unsigned char *scaled_line_buffer = NULL;
    if (scale > 1)
        scaled_line_buffer = GetTempMemory((image_info.m_width / scale + 1) * 3);

    // Allocate error buffers for full image width
    int16_t *error_buffer_0 = NULL;
    int16_t *error_buffer_1 = NULL;

    if (dither_mode >= 0)
    {
        int err_size = image_info.m_width * 3 * sizeof(int16_t);
        error_buffer_0 = (int16_t *)GetTempMemory(err_size);
        error_buffer_1 = (int16_t *)GetTempMemory(err_size);

        for (int i = 0; i < image_info.m_width * 3; i++)
        {
            error_buffer_0[i] = 0;
            error_buffer_1[i] = 0;
        }
    }

    int16_t *curr_error = error_buffer_0;
    int16_t *next_error = error_buffer_1;

    // Process MCU row by MCU row
    for (mcu_y = 0; mcu_y < image_info.m_MCUSPerCol; mcu_y++)
    {
        // Calculate the actual Y position in the image for this MCU row
        int image_y = mcu_y * image_info.m_MCUHeight;

        // We can stop decoding if we're past the visible region
        // but we must decode all previous MCU rows sequentially.
        // With binning each screen row consumes "scale" source rows, so the
        // visible source height grows by the same factor.
        int src_visible_limit = yOffset + scale * (VRes - yOrigin);
        bool should_display = (image_y + image_info.m_MCUHeight > yOffset) &&
                              (image_y < src_visible_limit);

        if (image_y >= src_visible_limit)
            break;

        // Decode all MCUs in this row into the buffer
        // (must decode sequentially even if we won't display this row)
        for (mcu_x = 0; mcu_x < image_info.m_MCUSPerRow; mcu_x++)
        {
            routinechecks(1);
            status = pjpeg_decode_mcu();

            if (status)
            {
                if (status != PJPG_NO_MORE_BLOCKS)
                {
                    FileClose(jpgfnbr);
                    error("pjpeg_decode_mcu() failed with status %", status);
                }
                FileClose(jpgfnbr);
                return;
            }

            // Copy this MCU into the row buffer
            int mcu_x_offset = mcu_x * image_info.m_MCUWidth;

            for (int y = 0; y < image_info.m_MCUHeight; y += 8)
            {
                const int by_limit = min(8, image_info.m_height - (mcu_y * image_info.m_MCUHeight + y));
                for (int x = 0; x < image_info.m_MCUWidth; x += 8)
                {
                    uint src_ofs = (x * 8U) + (y * 16U);
                    const uint8_t *pSrcR = image_info.m_pMCUBufR + src_ofs;
                    const uint8_t *pSrcG = image_info.m_pMCUBufG + src_ofs;
                    const uint8_t *pSrcB = image_info.m_pMCUBufB + src_ofs;

                    const int bx_limit = min(8, image_info.m_width - (mcu_x * image_info.m_MCUWidth + x));

                    for (int by = 0; by < by_limit; by++)
                    {
                        uint8_t *pDst = mcu_row_buffer + (y + by) * mcu_row_width + (mcu_x_offset + x) * 3;

                        for (int bx = 0; bx < bx_limit; bx++)
                        {
                            pDst[2] = *pSrcR++;
                            pDst[1] = *pSrcG++;
                            pDst[0] = *pSrcB++;
                            pDst += 3;
                        }

                        pSrcR += (8 - bx_limit);
                        pSrcG += (8 - bx_limit);
                        pSrcB += (8 - bx_limit);
                    }
                }
            }
        }

        // Now dither this entire MCU row, one image row at a time
        // We MUST process ALL rows to maintain correct error diffusion state
        if (dither_mode >= 0)
        {
            for (int y = 0; y < mcu_row_height; y++)
            {
                int img_y = mcu_y * mcu_row_height + y;
                if (img_y >= image_info.m_height)
                    break;

                uint8_t *row_ptr = mcu_row_buffer + (y * mcu_row_width);
                routinechecks(1);
                dither_image_row(row_ptr, image_info.m_width, curr_error, next_error, dither_mode);

                // Swap buffers for next row
                int16_t *temp = curr_error;
                curr_error = next_error;
                next_error = temp;

                // Clear next buffer
                for (int i = 0; i < image_info.m_width * 3; i++)
                    next_error[i] = 0;
            }
        }

        // Display this MCU row line by line
        // Only display lines that are within the yOffset region
        if (should_display && scale == 1)
        {
            // Draw each line individually to prevent overflow
            for (int line = 0; line < image_info.m_MCUHeight; line++)
            {
                int image_line_y = image_y + line;

                // Check if this line is within the image bounds
                if (image_line_y >= image_info.m_height)
                    break;

                // Skip lines before yOffset
                if (image_line_y < yOffset)
                    continue;

                // Calculate screen Y position (relative to yOffset)
                int screen_y = yOrigin + (image_line_y - yOffset);

                // Check if this line is within vertical display bounds
                if (screen_y < 0 || screen_y >= VRes)
                    continue;

                // Calculate horizontal display parameters
                int screen_x = xOrigin;
                if (screen_x >= HRes)
                    continue;

                // Calculate how much of this line to display
                // We need to skip xOffset pixels from the start of the line
                int source_start_x = xOffset;
                int available_width = image_info.m_width - source_start_x;
                int display_width = min(available_width, HRes - screen_x);

                if (display_width <= 0)
                    continue;

                // Draw just this line from the buffer, starting at xOffset
                uint8_t *line_ptr = mcu_row_buffer + (line * mcu_row_width) + (source_start_x * 3);
                DrawBuffer(screen_x, screen_y, screen_x + display_width - 1, screen_y, (char *)line_ptr, 0);
                routinechecks(1);
            }
        }
        else if (should_display)
        {
            // Scaled output: bin each scale x scale block of source pixels into
            // a single averaged output pixel. Bins step "scale" source lines at
            // a time; MCU height is a multiple of scale so bins stay inside this
            // MCU row buffer.
            for (int line = 0; line < image_info.m_MCUHeight; line += scale)
            {
                int image_line_y = image_y + line; // top source row of this bin

                if (image_line_y >= image_info.m_height)
                    break;

                // Skip bins that start before yOffset
                if (image_line_y < yOffset)
                    continue;

                // Screen Y maps the cropped, binned source row
                int screen_y = yOrigin + (image_line_y - yOffset) / scale;
                if (screen_y < 0 || screen_y >= VRes)
                    continue;

                int screen_x = xOrigin;
                if (screen_x >= HRes)
                    continue;

                int source_start_x = xOffset;
                int available_out = (image_info.m_width - source_start_x) / scale;
                int display_width = min(available_out, HRes - screen_x);

                if (display_width <= 0)
                    continue;

                for (int ox = 0; ox < display_width; ox++)
                {
                    int sx0 = source_start_x + ox * scale;
                    uint32_t sumB = 0, sumG = 0, sumR = 0;
                    int count = 0;

                    for (int dy = 0; dy < scale; dy++)
                    {
                        if (image_y + line + dy >= image_info.m_height)
                            break;
                        uint8_t *sp = mcu_row_buffer + ((line + dy) * mcu_row_width) + sx0 * 3;
                        for (int dx = 0; dx < scale; dx++)
                        {
                            if (sx0 + dx >= image_info.m_width)
                                break;
                            sumB += sp[0];
                            sumG += sp[1];
                            sumR += sp[2];
                            sp += 3;
                            count++;
                        }
                    }

                    // Average the sub-area, rounding to nearest
                    uint8_t *op = scaled_line_buffer + ox * 3;
                    op[0] = (uint8_t)((sumB + count / 2) / count);
                    op[1] = (uint8_t)((sumG + count / 2) / count);
                    op[2] = (uint8_t)((sumR + count / 2) / count);
                }

                DrawBuffer(screen_x, screen_y, screen_x + display_width - 1, screen_y, (char *)scaled_line_buffer, 0);
                routinechecks(1);
            }
        }
    }

    FileClose(jpgfnbr);
    clearrepeat();
}




int strcicmp(char const *a, char const *b)
{
    for (;; a++, b++) {
        int d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a)
            return d;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// ERROR HANDLING ////////////////////////////////////////////


void ErrorThrow(int e) {
    MMerrno = e;
    FSerror = e;
    MMErrMsg = (char *)FErrorMsg[e];
    if(e && OptionFileErrorAbort) error(MMErrMsg);
    return;
}


void ErrorCheck(int fnbr) {                                         //checks for an error, if fnbr is specified frees up the filehandle before sending error
    int e;
    e = (int)FSerror;
    if(fnbr != 0 && e != 0) ForceFileClose(fnbr);
    if(e >= 1 && e <= 19) ErrorThrow(ErrorMap[e]);
    return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// GENERAL I/O ////////////////////////////////////////////


void FileOpen(char *fname, char *fmode, char *ffnbr) {
    int fnbr;
    BYTE mode = 0;
    if(str_equal(fmode, "OUTPUT"))
        mode = FA_WRITE | FA_CREATE_ALWAYS;
    else if(str_equal(fmode, "APPEND"))
        mode = FA_WRITE | FA_OPEN_APPEND;
    else if(str_equal(fmode, "INPUT"))
        mode = FA_READ;
    else if(str_equal(fmode, "RANDOM"))
        mode = FA_WRITE | FA_OPEN_APPEND | FA_READ;
    else
        error("File access mode");

    if(*ffnbr == '#') ffnbr++;
    fnbr = getinteger(ffnbr);
    BasicFileOpen(fname, fnbr, mode);
}


// this performs the basic duties of opening a file, all file opens in MMBasic should use this
// it will open the file, set the FileTable[] entry and populate the file descriptor
// it returns with true if successful or false if an error
int BasicFileOpen(char *fname, int fnbr, int mode) {
    char pp[FF_MAX_LFN] = {0};
    char q[FF_MAX_LFN]={0};
    char *p=fname;
    int i,j;
    if(fnbr < 1 || fnbr > MAXOPENFILES) error("File number");
    if(FileTable[fnbr].com != 0) error("File number already open");
    if(!InitSDCard()) return false;
    	FileTable[fnbr].fptr = GetInternalMemory(sizeof(FIL));              // allocate the file descriptor
        i=strlen(p)-1;
        while(i>0 && !(p[i] == 92 || p[i]==47))i--;
        if(i>0){
        	memcpy(q,p,i);
             for(j=0;j<strlen(q);j++)if(q[j]=='\\')q[j]='/';  //allow backslash for the DOS oldies
        	i++;
        }
        strcpy(pp,&p[i]);
        if((pp[0]==47 || pp[0]==92) && i==0){
        	strcpy(q,&pp[1]);
        	strcpy(pp,q);
        	q[0] = filepath[0]; q[1] = ':'; q[2] = '/'; q[3] = 0;   // (the current drive)
        }
        fullpath(q);
//       	MMPrintString("Was: ");MMPrintString(fname);PRet();
//       	MMPrintString("Path: ");MMPrintString(fullpathname);PRet();
//       	MMPrintString("File: ");MMPrintString(pp);PRet();
       	strcpy(q,fullpathname);
       	if(fullpathname[strlen(fullpathname)-1]!='/')strcat(q,"/");
       	strcat(q,pp);
//       	MMPrintString("Full: ");MMPrintString(q);PRet();
       	strcpy(fullfilepathname,q);
       	DrivePath(fullfilepathname);                                // (with its drive: the program's name line keeps it)
//       	MMPrintString(fullfilepathname);PRet();
        FSerror = f_open(FileTable[fnbr].fptr, q, mode);        // open it
        FileCharPos[fnbr] = 1;                                  // (PRINT's column in its line)
        ErrorCheck(fnbr);
        clearrepeat();

    if(FSerror) {
        ForceFileClose(fnbr);
        return false;
    } else
        return true;
}


//close the file and free up the file handle
// it will generate an error if needed
void FileClose(int fnbr) {
    ForceFileClose(fnbr);
    ErrorThrow(FSerror);
}


//close the file and free up the file handle
// it will NOT generate an error
void ForceFileClose(int fnbr) {
    if(fnbr && FileTable[fnbr].fptr != NULL){
        FSerror = f_close(FileTable[fnbr].fptr);
        FreeMemorySafe((void *)(char *)&FileTable[fnbr].fptr);
//        FreeMemorySafe((void *)&SDbuffer[fnbr]);
//        buffpointer[fnbr]=0;
//        lastfptr[fnbr]=-1;
//        bw[fnbr]=-1;
//        fmode[fnbr]=0;
    }
}



char FileGetChar(int fnbr) {
    char ch;
    unsigned int i;
    FSerror = f_read(FileTable[fnbr].fptr, &ch,1, &i);
    		ErrorCheck(fnbr);
    return ch;
}

void FilePutStr(int count, char *c, int fnbr){
    unsigned int bw;
    InitSDCard();
    FSerror = f_write(FileTable[fnbr].fptr, c, count, &bw);
    ErrorCheck(fnbr);
}


char FilePutChar(char c, int fnbr) {
    static char t;
    unsigned int bw;
    t = c;
    if(!InitSDCard()) return 0;
    FSerror = f_write(FileTable[fnbr].fptr, &t, 1, &bw);
    ErrorCheck(fnbr);
    return t;
}



// n bytes of file fnbr into buff; *read is how many came (the PicoMite's; LINPUT( and LONGSTRING LOAD use it)
int FileGetData(int fnbr, void *buff, int count, unsigned int *read) {
    if(!InitSDCard()) return FSerror;
    FSerror = f_read(FileTable[fnbr].fptr, buff, count, (UINT *)read);
    ErrorCheck(fnbr);
    return FSerror;
}
void FilePutData(char *c, int fnbr, int n) {
    unsigned int bw;
    if(!InitSDCard()) return;
    FSerror = f_write(FileTable[fnbr].fptr, c, n, &bw);
    ErrorCheck(fnbr);
}
int FileEOF(int fnbr) {
	return f_eof(FileTable[fnbr].fptr);
}

// the current drive's volume ready (filepath's drive: A: the SD card, C: a USB flash drive), and FatFs's current drive
// set to it; the other drive's volume is registered, so that a path naming it mounts it when it is used (the PicoMite's
// InitSDCard and InitUSBDrive)
static int InitUSBDrive(void) {
    int i;
    ErrorThrow(0);
    if(!usb_msc_is_mounted()) { ErrorThrow(21); return false; }
    if(!(USBDriveStat & STA_NOINIT)) { f_chdrive("C:"); return 1; }
    for(i = 0; i < MAXOPENFILES; i++)                               // only the files open on C: are stale
        if(FileTable[i].com > MAXCOMPORTS && FileTable[i].fptr != NULL && FileTable[i].fptr->obj.fs == &FatFs_USB)
            ForceFileClose(i);
    i = f_mount(&FatFs_USB, "C:", 1);
    if(i) { ErrorThrow(ErrorMap[i]); return false; }
    f_mount(&FatFs, "A:", 0);
    f_chdrive("C:");
    return 2;
}

int InitSDCard(void) {
    int i;
    if(filepath[0] == 'C') return InitUSBDrive();
    ErrorThrow(0);    // reset mm.errno to zero
    if(!MDD_SDSPI_CardDetectState()) { ErrorThrow(20); return false; }  // error if the card is not present
    if(!(SDCardStat & STA_NOINIT)) {
    	f_chdrive("A:");
    	return 1;                     // if the card is present and has been initialised we have nothing to do
    }
    for(i = 0; i < MAXOPENFILES; i++)                               // only the files open on A: are stale
        if(FileTable[i].com > MAXCOMPORTS)
            if(FileTable[i].fptr != NULL && FileTable[i].fptr->obj.fs != &FatFs_USB)
                ForceFileClose(i);
    i = f_mount(&FatFs, "A:", 1);
    if(i) { ErrorThrow(ErrorMap[i]); return false; }
    f_mount(&FatFs_USB, "C:", 0);
    f_chdrive("A:");
    return 2;
}



// finds the first available free file number.  Throws an error if no free file numbers
int FindFreeFileNbr(void) {
    int i;
    for(i = MAXOPENFILES; i > 0; i--)
        if(FileTable[i].com == 0) return i;
    error("Too many files open");
    return 0;
}


// check the SD card to see if it has been removed.  Also check WAV playback
// this is called from cmd_pause(), the main ExecuteProgram() loop and the console's MMgetchar()
extern volatile int audiofinished;
void CheckSDCard(void) {
	if(gif==NULL && CurrentlyPlaying == P_NOTHING && CurrentLinePtr)return;
	if(!MDD_SDSPI_CardDetectState()) {
		if(!SDCardStat & STA_NOINIT){
			mymemset(&FatFs,0,sizeof(FATFS));
			SDCardStat = STA_NOINIT | STA_NODISK;
			if(filepath[0] == 'A') {                                // (a current C: is not the card's)
				mymemset(filepath,0,sizeof(filepath));
				strcpy(filepath,"A:/");
			}
		}
	} else {
		if(CurrentlyPlayinge == P_WAV || CurrentlyPlaying == P_WAV || CurrentlyPlaying == P_FLAC || CurrentlyPlaying == P_MP3 || CurrentlyPlaying == P_MOD || CurrentlyPlaying == P_ARRAY || CurrentlyPlaying == P_SAMPLE || CurrentlyPlaying == P_BBC || audiofinished)
			checkWAVinput();
		if(GifTimer==0 && gif!=NULL){
			GifTimer=5000;
			GIFcallback();
		}
	}
}



