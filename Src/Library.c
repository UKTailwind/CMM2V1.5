/***********************************************************************************************************************
LIBRARY (the PicoMite's, 17 Oct: Peter). A second program image, whose SUBs, FUNCTIONs, CSUBs, fonts, labels, DATA and
top level every program shares.

  LIBRARY LOAD fname$ [, fname$ ...] [, O | OVERWRITE] [, RAM]   up to eight files made one library, in that order;
                                     the program's first statement, which restarts it once the library is in
  LIBRARY SAVE                       the program in memory added to the flash library, then NEW (at the prompt)
  LIBRARY DELETE                     the flash library erased (at the prompt)
  LIBRARY LIST [ALL]                 its listing (at the prompt)
  LIBRARY DISK SAVE fname$           its image to a file (".lib" if none is given)
  LIBRARY DISK LOAD fname$           an image from a file into the flash library

Where it is.  The flash library is bank 2 sector 6, 128 KB, the last program slot (OPTION FLASH 6), as the PicoMite's
is its last flash slot.  Option.libflash says it holds a library and Option.libhash which files LIBRARY LOAD made it
from, so loading the same files again touches nothing (the hash is 0 after LIBRARY SAVE or DISK LOAD, which come from
no files).  A RAM library (LIBRARY LOAD ..., RAM) is a heap block (Peter, 17 Oct: no fixed SDRAM area).  It shadows the
flash library until the next RUN, NEW or program load, which release it, as the PicoMite's RAM slot is released;
LIBRARY LOAD's own restart keeps it (RamLibKeep).  So a program loads it at every start.  The PicoMite keeps its RAM
library across a RUN and only re-attaches it; here it is read and tokenised again.

The image is laid out as a program's:
  - a name line for each source file (line 0: ' IMAGE_TAG IMAGE_FORMAT name), then the tokenised lines, each header
    naming its file and line, so that an error in the library names them;
  - two zeros, a word of zeros, then, word aligned, FF FF FF FF;
  - the library's symbol table (Symbols.h: library symbols, SymLibSave);
  - the CSUB and font records, ended by FFFFFFFF.
A CSUB record's address word is the offset of its CSUB token from the start of the image, where a program's holds the
address, so an image can be copied between RAM, flash and a .lib file unchanged; a font's is its number - 1 with bit 31
set.  As on the PicoMite only a CSUB's binary is kept, not its hex text, and blank and comment-only lines are dropped.
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"

#define LIB_FLASH_ADDR  ADDR_FLASH_SECTOR_6_BANK2
#define LIB_FLASH_SLOT  6
#define LIB_MAX         0x20000                                     // the image's limit: one flash sector
#define LIB_FLAG        0x4C                                        // Option.libflash: 'L' when sector 6 holds a library
#define MAXLIBFILES     8
#define MAXLIBNAMES     64                                          // the files a LIBRARY SAVE may gather
#define LIB_HASH_SEED   2166136261u                                 // FNV-1a, across every file in order
#define LIB_FONT        0x80000000u                                 // a font record's address word: its number - 1 | this

char *LibMemory = NULL;                                             // the library in use (RAM or flash), NULL for none
char *RamLibMemory = NULL;                                          // a RAM library's heap block
static int RamLibSize = 0;                                          // its MemSize, for the heap wipe that keeps it
static uint32_t RamLibHash = 0;                                     // the files it came from
int RamLibKeep = 0;                                                 // ClearRuntime keeps the RAM library (LIBRARY LOAD's restart)
unsigned char *CFunctionLibrary = NULL;                             // its CSUB and font records (PrepareProgram)
extern volatile uint8_t FlashDone;
extern char tknerrmsg[];
extern char fullfilepathname[];
extern void ListNewLine(int *ListCnt, int all);
extern char *LineFileName(int file);

int LibFlashPresent(void) {
    return Option.libflash == LIB_FLAG;
}

static uint32_t LibFlashHash(void) {
    return (uint32_t)Option.libhash[0] | ((uint32_t)Option.libhash[1] << 8) | ((uint32_t)Option.libhash[2] << 16) | ((uint32_t)Option.libhash[3] << 24);
}

static void LibSetFlash(int present, uint32_t hash) {
    Option.libflash = present ? LIB_FLAG : 0;
    for(int i = 0; i < 4; i++) Option.libhash[i] = (hash >> (8 * i)) & 0xFF;
    SaveOptions(1);
}

// LibMemory: the RAM library while there is one, else the flash library, else none
void LibAttach(void) {
    LibMemory = RamLibMemory != NULL ? RamLibMemory : (LibFlashPresent() ? (char *)LIB_FLASH_ADDR : NULL);
}

// the RAM library gone (ClearRuntime: RUN, NEW, a program load): its block went with the heap
void RamLibRelease(void) {
    RamLibMemory = NULL;
    RamLibSize = 0;
    RamLibHash = 0;
    CFunctionLibrary = NULL;
    LibAttach();
}

// the RAM library's block, for a heap wipe that keeps it (ClearRuntimeAll)
int RamLibBlock(void **blk, int *size) {
    if(!RamLibKeep || RamLibMemory == NULL) return 0;
    *blk = RamLibMemory;
    *size = RamLibSize;
    return 1;
}

// is p in the library's image?
int InLibrary(const char *p) {
    if(LibMemory == NULL || p == NULL) return 0;
    return p >= LibMemory && p < LibMemory + (RamLibMemory != NULL ? RamLibSize : LIB_MAX);
}

// the name of source file 'file' of the image at base (its name lines come first), or "?"
static char *ImageFileName(char *base, int file) {
    char *p = base;
    while(*p == T_NEWLINE && LHDR_LINE(p) == 0) {
        if(LHDR_FILE(p) == file) return p + T_NEWLINE_HDR + IMAGE_NAME;
        if(LHDR_SKIP(p) > LHDR_SKIPMAX) break;
        p += LHDR_SKIP(p);
    }
    return "?";
}

// the name of the file the line at p (a T_NEWLINE) came from: the library's or the program's
char *LineFileNameAt(char *p) {
    if(InLibrary(p)) return ImageFileName(LibMemory, LHDR_FILE(p));
    return LineFileName(LHDR_FILE(p));
}

/* Is the library image at lib (limit bytes at most) sound?  The PicoMite's LibraryImageValid: a corrupt image hangs
   PrepareProgram before any input is possible, so it is checked at boot and before a .lib file is written; the walk
   is PrepareProgramExt's: the text, the zeros, the FF, the symbol table, then the records, each inside the limit. */
int LibraryImageValid(const unsigned char *lib, int limit) {
    const unsigned char *p = lib, *end = lib + limit;
    const unsigned int *cfp, *cfpend;
    if(lib == NULL || *p != T_NEWLINE) return 0;                    // it starts with its first name line
    while(p + 1 < end && !(p[0] == 0 && p[1] == 0)) p++;
    if(p + 1 >= end) return 0;                                      // the text never ended
    p += 2;
    while(p < end && *p == 0) p++;
    if(p >= end || *p != 0xff) return 0;                            // the marker before the records
    p++;
    cfp = (const unsigned int *)(((uint32_t)p + 3) & ~3);
    cfpend = (const unsigned int *)end;
    if((const unsigned char *)cfp + sizeof(symtab_t) <= end && *cfp == SYM_MAGIC) {
        const symtab_t *st = (const symtab_t *)cfp;
        if(st->textlen != (uint32_t)((const unsigned char *)cfp - lib) || (st->size & 3) ||
           st->size > (uint32_t)(end - (const unsigned char *)cfp)) return 0;
        cfp += st->size / 4;
    }
    while(cfp < cfpend && *cfp != 0xffffffffu) {
        unsigned int words;
        cfp++;                                                      // over the address word
        if(cfp >= cfpend) return 0;
        words = (*cfp + 4) / sizeof(unsigned int);
        if(words == 0 || words > (unsigned int)(cfpend - cfp)) return 0;
        cfp += words;
    }
    if(cfp >= cfpend) return 0;
    return (int)((const unsigned char *)(cfp + 1) - lib);           // its length, the terminator included
}

// boot: a flash library that is not sound is discarded (reflashing the firmware leaves sector 6 as it was)
void LibBoot(void) {
    RamLibMemory = NULL;
    if(LibFlashPresent() && !LibraryImageValid((unsigned char *)LIB_FLASH_ADDR, LIB_MAX)) {
        LibSetFlash(0, 0);
        MMPrintString("Library was corrupt and has been discarded\r\n");
    }
    LibAttach();
}

// a flash program of 'size' bytes may not reach the flash library's sector (SaveProgramToFlash)
void LibCheckProgramFlash(int size) {
    if(Option.ProgramStartCode < 0 || !LibFlashPresent()) return;
    uint32_t end = (uint32_t)ProgMemory + ((size + 0x1FFFF) & ~0x1FFFF);
    if(Option.ProgramStartCode == LIB_FLASH_SLOT || end > LIB_FLASH_ADDR)
        error("Library is using flash slot %", LIB_FLASH_SLOT);
}

/*********************************************************************************************************************
 The flash sector: erased and written here (the program's writer, flash.c, writes to the SDRAM under OPTION RAM)
*********************************************************************************************************************/

static int LibFlashErased(void) {
    for(uint32_t *j = (uint32_t *)LIB_FLASH_ADDR; j < (uint32_t *)(LIB_FLASH_ADDR + LIB_MAX); j++)
        if(*j != 0xFFFFFFFF) return 0;
    return 1;
}

static void LibFlashErase(void) {
    FLASH_EraseInitTypeDef e;
    int ok;
    SCB_DisableICache();
    SCB_DisableDCache();
    HAL_FLASH_Unlock();
    e.TypeErase = FLASH_TYPEERASE_SECTORS;
    e.Banks = FLASH_BANK_2;
    e.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    e.Sector = GetSector(LIB_FLASH_ADDR);
    e.NbSectors = 1;
    FlashDone = 0;
    ok = (HAL_FLASHEx_Erase_IT(&e) == HAL_OK);
    if(ok) {
        while(FlashDone == 0) routinechecks(0);
        ok = LibFlashErased();
    }
    SCB_EnableICache();
    SCB_EnableDCache();
    if(!ok) error("Flash erase problem");
}

// the image (len bytes) into the erased sector, 32 bytes a flash word
static void LibFlashProgram(const unsigned char *img, int len) {
    static uint32_t w[8] __attribute__((aligned(32)));
    for(int off = 0; off < len; off += 32) {
        int tries = 0;
        for(int k = 0; k < 32; k++) ((uint8_t *)w)[k] = (off + k < len) ? img[off + k] : 0xFF;
        while(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, LIB_FLASH_ADDR + off, (uint32_t)w) != HAL_OK)
            if(++tries >= 10) error("Flash write fail");
        if((off & 0x7FF) == 0) routinechecks(1);
    }
    SCB_CleanInvalidateDCache();
    if(memcmp((void *)LIB_FLASH_ADDR, img, len)) error("Flash write fail");
}

/*********************************************************************************************************************
 Reading the source (the PicoMite's lib_scan): the BASIC kept as text, each line "file,line;text", CSUB and font hex
 bodies turned into their records' binary as they are read
*********************************************************************************************************************/

typedef struct {
    int fnbr;                                                       // a file being read
    const char *mem;                                                // or text in memory (LIBRARY SAVE), "file,line;" kept
} libsrc_t;

static int LibEOF(libsrc_t *s) {
    return s->mem != NULL ? *s->mem == 0 : FileEOF(s->fnbr);
}

static int LibGet(libsrc_t *s) {
    return s->mem != NULL ? (unsigned char)*s->mem++ : (unsigned char)FileGetChar(s->fnbr);
}

static void LibFail(libsrc_t *s, char *msg) {
    if(s->mem == NULL) FileClose(s->fnbr);
    error(msg);
}

// is this line's first word 'word' (any case)?  What follows it, or NULL
static char *LibFirstWord(char *s, const char *word) {
    int n = strlen(word);
    while(*s == ' ') s++;
    if(strncasecmp(s, word, n)) return NULL;
    if(isnamechar((unsigned char)s[n])) return NULL;                // a longer name that merely starts with it
    s += n;
    while(*s == ' ') s++;
    return s;
}

// a CSUB or font body line: eight hex digits a word, then perhaps a comment.  1 if it was one (its words added)
static int LibHexLine(char *s, unsigned char *bin, uint32_t *len, uint32_t max) {
    char *p = s;
    while(1) {
        uint32_t n = 0;
        int i;
        while(*p == ' ') p++;
        if(*p == 0 || *p == '\'') return 1;
        for(i = 0; i < 8; i++) if(!isxdigit((unsigned char)p[i])) return 0;
        for(i = 0; i < 8; i++) {
            n <<= 4;
            n |= isdigit((unsigned char)p[i]) ? (uint32_t)(p[i] - '0') : (uint32_t)(toupper(p[i]) - 'A' + 10);
        }
        p += 8;
        if(*p && *p != ' ' && *p != '\'') return 0;
        if(*len + 4 > max) error("Library too big");
        if(bin) memcpy(bin + *len, &n, 4);
        *len += 4;
    }
}

/* One source: its lines measured (text and bin NULL) or kept.  hash, nfix and nrec are running totals across the
   files; textlen and binlen are this source's share.  file numbers the lines of a file; text in memory carries its own. */
static void LibScan(libsrc_t *src, int file, uint32_t *hash, char *text, uint32_t *textlen, unsigned char *bin,
                    uint32_t *binlen, uint32_t binmax, int *nfixout, int *nrecout) {
    int c, prevchar = 0, incsub = 0, isfont, nrec = *nrecout, nfix = *nfixout, line = 0, lf = file, ll;
    char *p, *t = text, pre[24];
    uint32_t tl = 0, bl = 0, recstart = 0, h = *hash;
    while(!LibEOF(src)) {
        p = inpbuf;
        while(1) {                                                  // a line into inpbuf: printable characters only
            if(LibEOF(src)) break;
            c = LibGet(src) & 0x7f;
            h ^= (uint32_t)c;
            h *= 16777619u;
            if(c == '\r' || (c == '\n' && prevchar != '\r')) { prevchar = c; break; }
            prevchar = c;
            if(c == '\n') continue;
            if(c == TAB) c = ' ';
            if(IsPrint(c)) {
                *p++ = c;
                if((p - inpbuf) >= MAXSTRLEN) LibFail(src, "Line is too long");
            }
        }
        *p = 0;
        line++;
        p = inpbuf;
        ll = line;
        if(src->mem != NULL) {                                      // text in memory: "file,line;" then the line
            lf = strtol(p, &p, 10);
            if(*p == ',') p++;
            ll = strtol(p, &p, 10);
            if(*p == ';') p++;
        }
        if(ll > LHDR_MAXLINE) LibFail(src, "Too many lines in a library file");
        if(incsub) {
            if(LibFirstWord(p, "end")) {
                if(bin) {
                    uint32_t len = bl - recstart - 8;
                    memcpy(bin + recstart + 4, &len, 4);            // its size: what follows the size word
                }
                incsub = 0;                                         // (the END line itself is kept)
            } else {
                if(!LibHexLine(p, bin, &bl, binmax)) LibFail(src, "Invalid hex word");
                continue;                                           // the hex text is not kept
            }
        } else {
            isfont = (LibFirstWord(p, "definefont") != NULL);
            if(LibFirstWord(p, "csub") || LibFirstWord(p, "cfunction") || isfont) {
                uint32_t addr = 0xFFFFFFFF;                         // a CSUB's: its token's offset, once the text is down
                if(nrec >= MAXCFUNCTION) LibFail(src, "Too many embedded routines");
                if(isfont) {
                    char *q = LibFirstWord(p, "definefont");
                    if(*q == '#') q++;
                    addr = 0;
                    while(*q >= '0' && *q <= '9') addr = addr * 10 + (uint32_t)(*q++ - '0');
                    if(addr < 1 || addr > FONT_TABLE_SIZE) LibFail(src, "Font number");
                    if(addr == 1 || addr == 6 || addr == 7) LibFail(src, "Cannot redefine fonts 1, 6, or 7");
                    addr = (addr - 1) | LIB_FONT;
                } else nfix++;
                recstart = bl;
                if(bin) memcpy(bin + bl, &addr, 4);
                bl += 8;                                            // the address word, and the size word to come
                nrec++;
                incsub = 1;
            } else {
                char *q = p;
                while(*q == ' ') q++;
                if(*q == 0 || *q == '\'') continue;                 // blank or a comment: only library space
            }
        }
        sprintf(pre, "%d,%d;", lf, ll);
        tl += strlen(pre) + strlen(p) + 1;
        if(t) {
            strcpy(t, pre); t += strlen(pre);
            strcpy(t, p); t += strlen(p);
            *t++ = '\n';
        }
    }
    if(incsub) LibFail(src, "Missing END declaration");
    if(t) *t = 0;
    *hash = h;
    *textlen = tl;
    *binlen = bl;
    *nfixout = nfix;
    *nrecout = nrec;
}

/* The files read: the text and the records' binary in temporary memory, the hash of the files, and their names (as
   opened, with the drive).  The PicoMite's FileLoadLibrary: a first pass measures and hashes, and when the hash is
   skiphash (what the library holds already) nothing more is done and 0 is returned; nothing is written anywhere. */
static int LibLoadFiles(char *fnames[], int nfiles, uint32_t *hashout, char **textout, unsigned char **binout,
                        uint32_t *binlenout, int *nfixout, uint32_t skiphash, char names[][FF_MAX_LFN]) {
    int i, nfix = 0, nrec = 0;
    uint32_t textlen = 0, binlen = 0, tl, bl, used = 0, h = LIB_HASH_SEED;
    char *text, *t, fp[FF_MAX_LFN];
    unsigned char *bin = NULL, *b;
    libsrc_t src = {0, NULL};
    if(!InitSDCard()) return 0;
    for(int pass = 0; pass < 2; pass++) {
        if(pass == 1) {
            *hashout = h;
            if(skiphash && skiphash == h) return 0;                 // already exactly these files
            text = GetTempMemory(textlen + 4);
            if(binlen) bin = GetTempMemory(binlen);
            t = text; b = bin;
            h = LIB_HASH_SEED;
            nfix = nrec = 0;
        }
        for(i = 0; i < nfiles; i++) {
            strcpy(fp, getFstring(fnames[i]));
            if(strchr(&fp[strlen(fp) > 4 ? strlen(fp) - 4 : 0], '.') == NULL) strcat(fp, ".bas");
            src.fnbr = FindFreeFileNbr();
            if(!BasicFileOpen(fp, src.fnbr, FA_READ)) return 0;
            if(pass == 0) {
                FSIZE_t sz = f_size(FileTable[src.fnbr].fptr);
                strcpy(names[i], fullfilepathname);
                if(sz == 0) { FileClose(src.fnbr); error("$ is empty", fp); }
            }
            tl = 0;
            bl = pass ? binlen - used : 0;
            LibScan(&src, i + 1, &h, pass ? t : NULL, &tl, pass ? b : NULL, &bl, pass ? bl : 0xFFFFFFFF, &nfix, &nrec);
            FileClose(src.fnbr);
            if(pass == 0) { textlen += tl; binlen += bl; }
            else { t += tl; if(b) { b += bl; used += bl; } }
        }
    }
    *textout = text;
    *binout = bin;
    *binlenout = binlen;
    *nfixout = nfix;
    return 1;
}

/*********************************************************************************************************************
 The image: built in RAM, then copied to a heap block (RAM) or written to the sector (flash)
*********************************************************************************************************************/

static unsigned char *LibW, *LibWStart, *LibWEnd;
static void LibPut(unsigned char c) {
    if(LibW >= LibWEnd) { SymEnd(); error("Library too big"); }
    *LibW++ = c;
}

// the next "file,line;text" of the source text: the text into inpbuf, the file and line into tknfile and tknline
static char *LibNextLine(char *s) {
    char *d = inpbuf;
    if(s == NULL || *s == 0) return NULL;
    tknfile = strtol(s, &s, 10);
    if(*s == ',') s++;
    tknline = strtol(s, &s, 10);
    if(*s == ';') s++;
    while(*s && *s != '\n') *d++ = *s++;
    *d = 0;
    if(*s) s++;
    tknfilename = NULL;
    return s;
}

// tokenise() made a line in tknbuf: it into the image, to its terminating zero
static void LibPutLine(void) {
    char *p = tknbuf;
    while(!(p[0] == 0 && p[1] == 0)) LibPut(*p++);
    LibPut(0);
}

/* The image of the library: name lines for names[0..nnames-1], the lines of text, the symbol table, then the records
   (bin), each CSUB's address word set to its token's offset.  Into out (max bytes); its length. */
static int LibBuildImage(char *text, unsigned char *bin, uint32_t binlen, int nfix, char **names, int nnames,
                         unsigned char *out, int max) {
    char buf[TKNBUFSIZE], *s;
    unsigned char *p;
    int i, savefile = tknfile, saveline = tknline;
    memcpy(buf, tknbuf, TKNBUFSIZE);                                // tokenise() writes through tknbuf
    clearrepeat();
    LibW = LibWStart = out;
    LibWEnd = out + max;
    if(SymBegin(text)) {                                            // the names counted over exactly the lines written
        SymLibSave = 1;
        SymRawBlock = 0;
        for(s = text; (s = LibNextLine(s)) != NULL; ) tokenise(false);
        SymRank();
    }
    for(i = 0; i < nnames; i++) {                                   // the name lines
        inpbuf[0] = '\''; inpbuf[1] = IMAGE_TAG; inpbuf[2] = IMAGE_FORMAT;
        strncpy(&inpbuf[IMAGE_NAME], names[i], STRINGSIZE - IMAGE_NAME - 1);
        inpbuf[STRINGSIZE - 1] = 0;
        tknfile = i + 1; tknline = 0; tknfilename = NULL;
        if(tokenise(false)) { SymEnd(); error("$", tknerrmsg); }
        LibPutLine();
    }
    SymRawBlock = 0;
    for(s = text; (s = LibNextLine(s)) != NULL; ) {
        if(tokenise(false)) { SymEnd(); error("$", tknerrmsg); }
        LibPutLine();
        routinechecks(1);
    }
    LibPut(0);
    for(i = 0; i < 4; i++) LibPut(0);                               // a word of zeros after the end, as a program's
    while((LibW - out) & 3) LibPut(0);
    for(i = 0; i < 4; i++) LibPut(0xFF);                            // the marker before the records
    if(LibW + SymTableSize() + binlen + 4 > LibWEnd) { SymEnd(); error("Library too big"); }
    SymTableWrite(LibPut, (uint32_t)(LibW - out));
    SymEnd();
    p = out;                                                        // each CSUB's record gets its token's offset
    i = 0;
    while(!(p[0] == 0 && p[1] == 0) && i < nfix) {
        if(*p == T_NEWLINE) p += T_NEWLINE_HDR;
        if(*p == T_LINENBR) p += 3;
        skipspace(p);
        if(*p == T_LABEL) {
            p += p[1] + 2;
            skipspace(p);
        }
        if(commandtbl_at((char *)p) == cmdCSUB) {
            uint32_t addr = (uint32_t)(p - out), k = 0, a;
            while(k < binlen) {                                     // the first record still waiting for one
                memcpy(&a, bin + k, 4);
                if(a == 0xFFFFFFFF) { memcpy(bin + k, &addr, 4); break; }
                memcpy(&a, bin + k + 4, 4);
                k += a + 8;
            }
            i++;
        }
        while(*p) p++;
        p++;
    }
    for(i = 0; i < (int)binlen; i++) LibPut(bin[i]);
    for(i = 0; i < 4; i++) LibPut(0xFF);                            // the records' terminator
    memcpy(tknbuf, buf, TKNBUFSIZE);
    tknfile = savefile; tknline = saveline;
    return LibW - out;
}

// the image (len bytes at img) made the flash library; hash: the files it came from (0: none)
static void LibWriteFlash(unsigned char *img, int len, uint32_t hash) {
    if(len > LIB_MAX) error("Library too big");
    LibSetFlash(0, 0);                                              // (an erase that fails part way leaves no library)
    LibFlashErase();
    LibFlashProgram(img, len);
    LibSetFlash(1, hash);
}

// the first statement of the program (after its name lines, comments, blank lines and OPTION SYMBOLS OFF)
static char *LibFirstStatement(void) {
    char *q = (char *)ProgMemory;
    while(*q == T_NEWLINE) {
        char *body = q + T_NEWLINE_HDR;
        if(*body == T_LINENBR) body += 3;
        while(*body == ' ') body++;
        if(*body != 0 && *body != '\'' && !SymIsOffStatement(body)) break;
        q = NextLine(q);
    }
    return q;
}

// after the library changed: the program prepared again over it, and in a program its top level run and the
// program restarted at its first line (where LIBRARY LOAD now finds the library it asked for)
static void LibRestart(char *savedline) {
    CurrentLinePtr = savedline;
    ClearRuntime();                                                 // as RUN: a RAM library is kept only through RamLibKeep,
    RamLibKeep = 0;                                                 // for this wipe alone (an error in the library's top level
    LibAttach();                                                    // below must not leave it set for every RUN after)
    PrepareProgram(true);
    if(savedline) {
        ExecuteProgram(LibMemory);
        nextstmt = (char *)ProgMemory;
    }
}

static void LibLoad(char *tp) {
    int overwrite = 0, wantram = 0, nfiles = 0, nfix = 0, len, k;
    uint32_t hash = 0, binlen = 0, skiphash = 0;
    char *text = NULL, *files[MAXLIBFILES], *np[MAXLIBFILES], *savedline = CurrentLinePtr;
    static char names[MAXLIBFILES][FF_MAX_LFN];
    unsigned char *bin = NULL, *img;
    getargs(&tp, MAXLIBFILES * 2 + 3, ",");
    if(argc == 0 || (argc & 1) == 0) error("Syntax");
    for(k = 0; k < argc; k += 2) {
        if(checkstring(argv[k], "O") || checkstring(argv[k], "OVERWRITE")) overwrite = 1;
        else if(checkstring(argv[k], "RAM")) wantram = 1;
        else {
            if(nfiles >= MAXLIBFILES) error("No more than % files", MAXLIBFILES);
            files[nfiles++] = argv[k];
        }
    }
    if(nfiles == 0) error("Syntax");
    if(savedline && savedline != LibFirstStatement()) error("Must be the first statement in the program");
    if(wantram) {
        if(RamLibMemory != NULL) skiphash = RamLibHash;
    } else {
        if(!LibFlashPresent()) {
            if(Option.ProgramStartCode == LIB_FLASH_SLOT || !LibFlashErased()) error("Flash slot % already in use", LIB_FLASH_SLOT);
        } else skiphash = LibFlashHash();
    }
    if(!LibLoadFiles(files, nfiles, &hash, &text, &bin, &binlen, &nfix, skiphash, names)) {
        CurrentLinePtr = savedline;
        return;                                                     // this library already (or a file would not open)
    }
    for(k = 0; k < nfiles; k++) np[k] = names[k];
    img = GetMemory(LIB_MAX);
    len = LibBuildImage(text, bin, binlen, nfix, np, nfiles, img, LIB_MAX);
    if(wantram) {
        char *blk = GetExecMemory(len);                             // (in the executable window: its CSUBs run from it)
        memcpy(blk, img, len);
        FreeMemorySafe((void *)&img);
        SCB_CleanDCache();                                          // a CSUB's code runs from it
        SCB_InvalidateICache();
        RamLibMemory = blk;
        RamLibSize = MemSize(blk);
        RamLibHash = hash;
        RamLibKeep = 1;
        LibRestart(savedline);
        return;
    }
    if(LibFlashPresent() && !overwrite) {
        int c;
        MMPrintString("\r\nThis replaces the library already in flash - are you sure (Y/N) ? ");
        while((c = MMInkey()) == -1) routinechecks(1);
        MMputchar(c);
        MMPrintString("\r\n");
        if(toupper(c) != 'Y') error("Library not replaced");
    }
    LibWriteFlash(img, len, hash);
    FreeMemorySafe((void *)&img);
    LibRestart(savedline);
}

/*********************************************************************************************************************
 LIBRARY SAVE: the flash library and the program turned back into text (llist, a CSUB's hex from its record), then one
 image made of the two, as LIBRARY LOAD makes one from files.  The PicoMite appends the program's tokens to the image;
 here each line's header names its file, so the files are numbered afresh: the library's, then the program's.
*********************************************************************************************************************/

static char *LibTextPut(char *t, char *tend, int file, int line, char *s) {
    char pre[24];
    sprintf(pre, "%d,%d;", file, line);
    if(t + strlen(pre) + strlen(s) + 2 >= tend) error("Not enough memory");
    strcpy(t, pre); t += strlen(pre);
    strcpy(t, s); t += strlen(s);
    *t++ = '\n';
    *t = 0;
    return t;
}

// the record of the CSUB whose token is at offset off of the image, or of font n (bit 31 set), in the records at cfp
static unsigned int *LibRecord(unsigned int *cfp, uint32_t key) {
    while(cfp != NULL && *cfp != 0xffffffff) {
        if(*cfp == key) return cfp;
        cfp++;
        cfp += (*cfp + 4) / sizeof(unsigned int);
    }
    return NULL;
}

// the lines of the image at base (not its name lines) into the text, its files numbered from firstfile; a library's
// CSUBs and fonts get their hex from their records (a program keeps its hex as text)
static char *LibImageText(char *t, char *tend, char *base, int firstfile, unsigned int *records, int islib) {
    char b[STRINGSIZE * 2], *p = base;
    while(*p == T_NEWLINE) {
        char *line = p, *q;
        int file = LHDR_FILE(p) + firstfile - 1, ln = LHDR_LINE(p);
        p = NextLine(p);
        if(ln == 0) continue;                                       // a name line
        llist(b, line);
        t = LibTextPut(t, tend, file, ln, b);
        if(!islib) continue;
        q = line + T_NEWLINE_HDR;
        if(*q == T_LINENBR) q += 3;
        skipspace(q);
        if(*q == T_LABEL) { q += q[1] + 2; skipspace(q); }
        CommandToken tkn = commandtbl_at(q);
        uint32_t key;
        if(tkn == cmdCSUB) key = (uint32_t)(q - base);
        else if(tkn == GetCommandValue("DefineFont")) {
            q += sizeof(CommandToken);
            skipspace(q);
            if(*q == '#') q++;
            key = ((uint32_t)atoi(q) - 1) | LIB_FONT;
        } else continue;
        unsigned int *r = LibRecord(records, key);
        if(r == NULL) error("Library record missing");
        int words = r[1] / 4;
        unsigned int *w = r + 2;
        while(words > 0) {                                          // its words, eight a line, as the hex was written
            char *o = b;
            for(int k = 0; k < 8 && words > 0; k++, words--) o += sprintf(o, "%08X ", *w++);
            o[-1] = 0;
            t = LibTextPut(t, tend, file, ln, b);
        }
    }
    return t;
}

static void LibSave(void) {
    char *text, *t, *tend, *text2, *names[MAXLIBNAMES], *p;
    int nnames = 0, libfiles = 0, nfix = 0, nrec = 0, len;
    uint32_t tl = 0, bl = 0, hash = LIB_HASH_SEED;
    unsigned char *bin, *img;
    libsrc_t src = {0, NULL};
    if(!ImageIsCurrent() || *ProgMemory != T_NEWLINE) error("No program to save");
    ClearRuntime();                                                 // (a RAM library goes: SAVE is to the flash library)
    PrepareProgram(false);                                          // CFunctionFlash and CFunctionLibrary
    if(!LibFlashPresent() && (Option.ProgramStartCode == LIB_FLASH_SLOT || !LibFlashErased()))
        error("Flash slot % already in use", LIB_FLASH_SLOT);
    if(LibFlashPresent()) {                                         // the library's files first, then the program's
        for(p = LibMemory; *p == T_NEWLINE && LHDR_LINE(p) == 0 && nnames < MAXLIBNAMES; p = NextLine(p))
            names[nnames++] = p + T_NEWLINE_HDR + IMAGE_NAME;
        libfiles = nnames;
    }
    for(p = (char *)ProgMemory; *p == T_NEWLINE && LHDR_LINE(p) == 0; p = NextLine(p)) {
        if(nnames >= MAXLIBNAMES) error("Too many files");
        names[nnames++] = p + T_NEWLINE_HDR + IMAGE_NAME;
    }
    t = text = GetMemory(4 * LIB_MAX + PROG_FLASH_SIZE);
    tend = text + 4 * LIB_MAX + PROG_FLASH_SIZE;
    *t = 0;
    if(LibFlashPresent()) t = LibImageText(t, tend, LibMemory, 1, (unsigned int *)CFunctionLibrary, 1);
    t = LibImageText(t, tend, (char *)ProgMemory, libfiles + 1, NULL, 0);
    src.mem = text;                                                 // measured, then kept: the text and the records
    LibScan(&src, 0, &hash, NULL, &tl, NULL, &bl, 0xFFFFFFFF, &nfix, &nrec);
    text2 = GetMemory(tl + 4);
    bin = bl ? GetMemory(bl) : NULL;
    src.mem = text; nfix = nrec = 0;
    LibScan(&src, 0, &hash, text2, &tl, bin, &bl, bl, &nfix, &nrec);
    img = GetMemory(LIB_MAX);
    len = LibBuildImage(text2, bin, bl, nfix, names, nnames, img, LIB_MAX);
    LibWriteFlash(img, len, 0);                                     // (from no files: the next LIBRARY LOAD writes)
    if(MMCharPos > 1) MMPrintString("\r\n");
    MMPrintString("Library Saved ");
    PInt(len);
    MMPrintString(" bytes\r\n");
    cmdline = "";
    CurrentLinePtr = NULL;
    cmd_new();                                                      // the program goes, and the heap with these buffers
}

/*********************************************************************************************************************
 The command
*********************************************************************************************************************/

static void LibList(int all) {
    char b[STRINGSIZE * 2], *p = LibMemory, *pp;
    int ListCnt = 1;
    while(p != NULL && *p == T_NEWLINE) {
        char *line = p;
        p = NextLine(p);
        if(LHDR_LINE(line) == 0) continue;                          // a name line
        llist(b, line);
        for(pp = b; *pp; pp++) {
            if(MMCharPos >= Option.Width) ListNewLine(&ListCnt, all);
            MMputchar(*pp);
        }
        routinechecks(1);
        ListNewLine(&ListCnt, all);
    }
}

void cmd_library(void) {
    char *tp;
    if((tp = checkstring(cmdline, "LOAD"))) {
        LibLoad(tp);
        return;
    }
    if(checkstring(cmdline, "SAVE")) {
        if(CurrentLinePtr) error("Invalid in a program");
        LibSave();
        return;
    }
    if(checkstring(cmdline, "DELETE")) {
        if(CurrentLinePtr) error("Invalid in a program");
        if(!LibFlashPresent()) return;
        LibSetFlash(0, 0);
        LibFlashErase();
        ClearRuntime();                                             // the SUB tables and the name index forget it
        LibAttach();
        return;
    }
    if(checkstring(cmdline, "LIST ALL")) {
        if(CurrentLinePtr) error("Invalid in a program");
        LibList(true);
        return;
    }
    if(checkstring(cmdline, "LIST")) {
        if(CurrentLinePtr) error("Invalid in a program");
        LibList(false);
        return;
    }
    if((tp = checkstring(cmdline, "DISK SAVE"))) {
        char fp[FF_MAX_LFN];
        int fnbr, len;
        if(CurrentLinePtr) error("Invalid in a program");
        if(LibMemory == NULL) error("No library to store");
        len = LibraryImageValid((unsigned char *)LibMemory, RamLibMemory != NULL ? RamLibSize : LIB_MAX);
        if(!len) error("Library is not valid");
        strcpy(fp, getFstring(tp));
        if(strchr(&fp[strlen(fp) > 4 ? strlen(fp) - 4 : 0], '.') == NULL) strcat(fp, ".lib");
        if(!InitSDCard()) return;
        fnbr = FindFreeFileNbr();
        if(!BasicFileOpen(fp, fnbr, FA_WRITE | FA_CREATE_ALWAYS)) return;
        for(int i = 0; i < len; i++) FilePutChar(LibMemory[i], fnbr);
        FileClose(fnbr);
        return;
    }
    if((tp = checkstring(cmdline, "DISK LOAD"))) {
        char fp[FF_MAX_LFN];
        unsigned char *img;
        int fnbr, len;
        if(CurrentLinePtr) error("Invalid in a program");
        if(!LibFlashPresent() && (Option.ProgramStartCode == LIB_FLASH_SLOT || !LibFlashErased()))
            error("Flash slot % already in use", LIB_FLASH_SLOT);
        strcpy(fp, getFstring(tp));
        if(strchr(&fp[strlen(fp) > 4 ? strlen(fp) - 4 : 0], '.') == NULL) strcat(fp, ".lib");
        if(!InitSDCard()) return;
        fnbr = FindFreeFileNbr();
        if(!BasicFileOpen(fp, fnbr, FA_READ)) return;
        len = f_size(FileTable[fnbr].fptr);
        if(len <= 0 || len > LIB_MAX) { FileClose(fnbr); error("File size % cannot exceed %", len, LIB_MAX); }
        img = GetMemory(LIB_MAX);
        memset(img, 0xFF, LIB_MAX);
        for(int i = 0; i < len; i++) img[i] = FileGetChar(fnbr);
        FileClose(fnbr);
        if(!LibraryImageValid(img, LIB_MAX)) error("Not a library file");
        ClearRuntime();                                             // (a RAM library goes)
        LibWriteFlash(img, len, 0);
        LibAttach();
        return;
    }
    error("Syntax");
}
