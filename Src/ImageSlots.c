/***********************************************************************************************************************
Flash and image slots (the PicoMite's FLASH commands) and the writer that draws an image's 4-bit pixels into a CMM2
page.

A slot holds what the PicoMite's does: a program (FLASH SAVE), an image (FLASH LOAD IMAGE: an 8-byte header, its width
and height as two 32-bit words, then the pixels as 4-bit RGB121 values packed two to a byte, the left pixel in the low
nibble, each row (width + 1) / 2 bytes) or a file's bytes as they are (FLASH DISK LOAD). Programs PEEK the header
(MM.INFO(FLASH ADDRESS n)) to see whether a slot is loaded, so the layout is the PicoMite's exactly.

Slots 1 to 3 are the PicoMite's flash slots, kept in RAM and on the SD card (Peter, 20 Oct): each is a file,
A:/.flash/slot1 to slot3 (a hidden folder), and nothing is written to the CMM2's flash. A slot is read into a heap block
the first time it is wanted after a heap wipe (RUN, NEW, a program load), so it costs no memory until then; an empty one
reads as erased flash (FF), as the PicoMite's does. A program is the image program memory held, tokens and all (Peter,
20 Oct: tokenised, not its source; a firmware of another IMAGE_FORMAT refuses it, naming its file to be loaded and
saved again), but for its CSUB records' address words: the file keeps them as offsets from the image's start (a font's
word is its number, below FONT_TABLE_SIZE) and the block as addresses in it, the block in the executable window if
the program has a CSUB. FLASH RUN and FLASH CHAIN run the program there in place, as the PicoMite runs a slot's program
from flash: nothing is tokenised and no flash is written. ProgMemory points at the block while it runs (FlashLoad says
which slot: MM.INFO(FLASH)), and at program memory (ProgFlash) again at the prompt and before a program is loaded
(SlotProgramLeave).

  FLASH SAVE n                        the program in memory into slot n ("Already programmed" if it holds anything)
  FLASH OVERWRITE n                   the same, whatever slot n holds
  FLASH LOAD n                        slot n's program into program memory (RUN runs it as it is, not its file again)
  FLASH RUN n                         slot n's program run (0: program memory's)
  FLASH CHAIN n                       the same, the global variables kept (in a program)
  FLASH LIST [n [, ALL]]              what the slots hold, or slot n's program listed
  FLASH ERASE n / FLASH ERASE ALL     slot n emptied, or slots 1 to 3 (ALL: at the prompt)

Slots 4 to 8 are the PicoMite's RAM slots 1 to 5, which its games use where there is PSRAM (MM.INFO(PSRAM SIZE) says
the CMM2 has). Here each is a heap block (Peter, 18 Oct: no fixed SDRAM area) that every heap wipe keeps (20 Oct, for
the RAM commands: RAM RUN and CHAIN run a program from one), so it goes only with RAM ERASE, an overwrite or a restart,
as the PicoMite's go with a reset. An empty one reads as a zero header, as an empty PicoMite RAM slot does. The RAM
commands (cmd_ram, below) are FLASH's program commands for them.

  FLASH LOAD IMAGE n, f$ [, O | OVERWRITE]     a BMP file into slot n ("Already programmed" if it holds one, unless O)
  FLASH DISK LOAD n, f$ [, O | OVERWRITE]      a file's bytes into slot n as they are (18 Oct: PETSCII Robots' sprites)
  MM.INFO(FLASH ADDRESS n)                     the slot's address

Drawing (ImageSlotBlit): each 4-bit value becomes a pixel of the page's own format through the PicoMite's 16 RGB121
colours (its colours[16]), converted once a call as DrawPixel8/16/32 convert a colour; a page that stores each row twice
(MODE 7 and the like) gets both; OPTION Y AXIS INVERT is honoured. A 4-bit page (MODE n,4: Rgb121.c) takes the values
themselves, as the PicoMite's blit121 does, and MAP acts on them there through the CLUT.

  BLIT FLASH n, dest, x1, y1, x2, y2, w, h [, colour]   the PicoMite's: from slot n onto a framebuffer or page
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "ImageSlots.h"

#define SLOTDIR "A:/.flash"                                         // the flash slots' files
#define SLOT_DATA 0                                                 // what a flash slot's block holds: an image or a file's bytes,
#define SLOT_PROGRAM 1                                              // a program of this firmware's (its CSUBs' addresses made),
#define SLOT_OLD 2                                                  // a program of another IMAGE_FORMAT's,
#define SLOT_BAD 3                                                  // or one whose layout is not sound
extern bool (*linecallback)(int *imagewidth, int *imageheight, uint32_t *linedata, int *linenumber);
extern char runcmd[STRINGSIZE];

int FlashLoad = 0;                                                  // the slot (1 to 8) whose program is running (FLASH RUN and
                                                                    // CHAIN, RAM RUN and CHAIN), or 0
char *ProgFlash = (char *)FLASH_PROGRAM_ADDR;                       // program memory, where ProgMemory points but while a slot's
                                                                    // program runs (main.c sets it with ProgMemory at boot)
int ProgFromSlot = 0;                                               // FLASH LOAD made program memory: RUN runs it as it is
static uint8_t *ImageSlot[MAXIMAGESLOTS + 1];                       // a slot's heap block, or NULL
static uint8_t SlotKind[MAXIMAGESLOTS + 1];                         // what a slot's block holds (SLOT_DATA...)
static const uint32_t ImageSlotEmpty[4] = {0, 0, 0, 0};             // what an empty RAM slot reads as,
static const uint32_t ImageSlotErased[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};   // and an empty flash slot

// the PicoMite's RGB121 colours (graphics/Draw.c colours[16]): bit 3 red, bits 2-1 green, bit 0 blue; its HDMI ones
// (graphics/HDMI.c MAP16DEF), and the ones in use (the VGA's, or the HDMI's under OPTION PICOMITE HDMI: Rgb121Clut)
const int Rgb121Vga[16] = {0x00, 0xFF, 0x4000, 0x40ff, 0x8000, 0x80ff, 0xff00, 0xffff,
                           0xff0000, 0xff00FF, 0xff4000, 0xff40ff, 0xff8000, 0xff80ff, 0xffff00, 0xffffff};
const int Rgb121Hdmi[16] = {0x00, 0xFF, 0x5500, 0x55ff, 0xAA00, 0xAAff, 0xff00, 0xffff,
                            0xff0000, 0xff00FF, 0xff5500, 0xff55ff, 0xffAA00, 0xffAAff, 0xffff00, 0xffffff};
int Rgb121Colours[16] = {0x00, 0xFF, 0x4000, 0x40ff, 0x8000, 0x80ff, 0xff00, 0xffff,
                         0xff0000, 0xff00FF, 0xff4000, 0xff40ff, 0xff8000, 0xff80ff, 0xffff00, 0xffffff};

// the RAM slots' blocks, which every heap wipe keeps (HeapWipe, Memory.c), each with its size, read before the wipe:
// how many. They go only with RAM ERASE, an overwrite or a restart, as the PicoMite's PSRAM slots go with a reset.
int RamSlotBlocks(void **blk, int *size) {
    int n = 0;
    for(int slot = MAXFLASHSLOTS + 1; slot <= MAXIMAGESLOTS; slot++)
        if(ImageSlot[slot] != NULL) {
            blk[n] = ImageSlot[slot];
            size[n++] = MemSize(ImageSlot[slot]);
        }
    return n;
}

// the heap was wiped (HeapWipe): the flash slots' blocks went with it (they are read again when they are wanted)
void ImageSlotsForget(void) {
    for(int slot = 1; slot <= MAXFLASHSLOTS; slot++) ImageSlot[slot] = NULL;
}

// slot n's block let go: freed, but for one a program runs from or a tilemap draws from (the next heap wipe frees it)
static void SlotDrop(int slot) {
    if(ImageSlot[slot] == NULL) return;
    if(ImageSlot[slot] == (uint8_t *)ProgMemory || TileMapUses(ImageSlot[slot])) ImageSlot[slot] = NULL;
    else {
        memset(ImageSlot[slot], 0, 8);                              // (an address kept from MM.INFO(FLASH ADDRESS n) reads
        FreeMemorySafe((void *)&ImageSlot[slot]);                   // no image now, as the PicoMite's erased RAM slot does)
    }
}

/* The flash slots' files. Each is the slot's bytes; none, or none in it, is an empty slot. The SD card is made ready
   whatever the current drive (InitACard), and the folder is made, hidden from FILES and FM, at the first save. */
static void SlotPath(char *path, int slot) {
    sprintf(path, SLOTDIR "/slot%d", slot);
}

static void SlotCard(void) {
    if(!InitACard()) error("SD card not found");
}

// a FatFs call's result: an error if it failed (f, if not NULL, closed first)
static void SlotCheck(FIL *f, FRESULT r) {
    if(r == FR_OK) return;
    if(f != NULL) f_close(f);
    FSerror = r;
    ErrorCheck(0);
    error("Flash slot file");
}

// the size of flash slot n's file: 0 for an empty slot
static int SlotSize(int slot) {
    FILINFO fno;
    char path[24];
    SlotCard();
    SlotPath(path, slot);
    if(f_stat(path, &fno) != FR_OK || (fno.fattrib & AM_DIR)) return 0;
    return (int)fno.fsize;
}

// up to n bytes from the start of flash slot n's file into buf: how many (0: the slot is empty)
static int SlotHead(int slot, void *buf, int n) {
    FIL f;
    UINT got = 0;
    char path[24];
    SlotCard();
    SlotPath(path, slot);
    if(f_open(&f, path, FA_READ) != FR_OK) return 0;
    SlotCheck(&f, f_read(&f, buf, n, &got));
    f_close(&f);
    return (int)got;
}

// flash slot n's file made the len bytes at data, and its block let go
static void SlotWrite(int slot, const void *data, int len) {
    FIL f;
    FILINFO fno;
    UINT done = 0;
    FRESULT r;
    char path[24];
    SlotCard();
    if(f_stat(SLOTDIR, &fno) != FR_OK) {
        SlotCheck(NULL, f_mkdir(SLOTDIR));
        f_chmod(SLOTDIR, AM_HID, AM_HID);
    }
    SlotPath(path, slot);
    SlotCheck(NULL, f_open(&f, path, FA_WRITE | FA_CREATE_ALWAYS));
    r = f_write(&f, data, len, &done);
    if(r == FR_OK) r = f_close(&f);
    else f_close(&f);
    if(r != FR_OK || (int)done != len) {
        f_unlink(path);                                             // not half a slot
        if(r == FR_OK) error("Not enough space on the SD card");
        SlotCheck(NULL, r);
    }
    SlotDrop(slot);
}

// flash slot n emptied: its file deleted, its block let go
static void SlotErase(int slot) {
    FRESULT r;
    char path[24];
    SlotCard();
    SlotPath(path, slot);
    r = f_unlink(path);
    if(r != FR_NO_FILE && r != FR_NO_PATH) SlotCheck(NULL, r);
    SlotDrop(slot);
}

// what the n bytes of a slot's image at p are: SLOT_PROGRAM, SLOT_OLD (main.c's test for an older image), SLOT_DATA
static int SlotKindOf(const uint8_t *p, int n) {
    if(n > 8 && ImageIsCurrentAt((const char *)p)) return SLOT_PROGRAM;
    if(n > 8 && p[0] == T_NEWLINE && (p[1] == '\'' || p[5] == '\'')) return SLOT_OLD;
    return SLOT_DATA;
}

// the file name in the first line of a program's image (n bytes at p, an older format's too) into name (FF_MAX_LFN):
// after the line header (with IMAGE_TAG and the format, or before 15 Oct without), or before there was a header
// straight after T_NEWLINE (the header first: its skip byte, p[1], can be a ')
static void SlotName(const uint8_t *p, int n, char *name) {
    const uint8_t *nm = NULL;
    int k = 0;
    if(p[5] == '\'') nm = p + T_NEWLINE_HDR + (p[6] == IMAGE_TAG ? IMAGE_NAME : 1);
    else if(p[1] == '\'') nm = p + 2;
    if(nm != NULL)
        while(k < FF_MAX_LFN - 1 && nm + k < p + n && nm[k] >= ' ' && nm[k] < 0x7f && nm[k] != '"') {
            name[k] = nm[k];
            k++;
        }
    name[k] = 0;
}

// why slot n, of that kind (its first n bytes at p), has no program to run, as an error
static void SlotRefuse(int slot, int kind, const uint8_t *p, int n) {
    char name[FF_MAX_LFN];
    if(kind == SLOT_BAD) error("Flash slot % is corrupt", slot);
    if(kind == SLOT_OLD) {
        SlotName(p, n, name);
        error("Flash slot % holds $ from another firmware version: load it and save it again", slot, name);
    }
    error(slot <= MAXFLASHSLOTS ? "Flash slot empty" : "RAM slot empty");   // (the PicoMite's)
}

/* The CSUB records of the program's image at img (len bytes): each address word moved by delta (a font's, its number,
   stays). -1 if the image is not sound, else 1 if it has a CSUB (delta 0: only the question). */
static int ImageRebase(uint8_t *img, int len, int32_t delta) {
    const unsigned int *rec;
    unsigned int *cfp;
    int csubs = 0;
    if(!ImageLayout(img, len, &rec)) return -1;
    cfp = (unsigned int *)rec;
    while(*cfp != 0xffffffff) {
        if(*cfp >= FONT_TABLE_SIZE) {
            *cfp += delta;
            csubs = 1;
        }
        cfp++;
        cfp += (*cfp + 4) / sizeof(unsigned int);
    }
    return csubs;
}

// flash slot n's block, read from its file the first time it is wanted after a heap wipe; NULL for an empty slot
static uint8_t *SlotGet(int slot) {
    FIL f;
    UINT got = 0;
    FRESULT r;
    char path[24];
    int len, csubs;
    uint8_t *b, *x;
    if(ImageSlot[slot] != NULL) return ImageSlot[slot];
    if((len = SlotSize(slot)) == 0) return NULL;
    SlotPath(path, slot);
    SlotCheck(NULL, f_open(&f, path, FA_READ));
    b = GetMemory(len);
    r = f_read(&f, b, len, &got);
    f_close(&f);
    if(r != FR_OK || (int)got != len) {
        FreeMemorySafe((void *)&b);
        SlotCheck(NULL, r != FR_OK ? r : FR_INT_ERR);
    }
    SlotKind[slot] = SlotKindOf(b, len);
    if(SlotKind[slot] == SLOT_PROGRAM) {
        if((csubs = ImageRebase(b, len, 0)) < 0) SlotKind[slot] = SLOT_BAD;
        else {
            if(csubs) {                                             // a CSUB's code runs from it
                x = GetExecMemory(len);
                memcpy(x, b, len);
                FreeMemorySafe((void *)&b);
                b = x;
            }
            ImageRebase(b, len, (int32_t)b);
            if(csubs) {
                SCB_CleanDCache();
                SCB_InvalidateICache();
            }
        }
    }
    ImageSlot[slot] = b;
    return b;
}

// a copy of the program's image at img (len bytes, its CSUB records' words addresses relative to base) in a new block,
// in the executable window if it has a CSUB, its records made addresses in the block (a RAM slot's program)
static uint8_t *ImagePlace(const uint8_t *img, int len, const uint8_t *base) {
    int csubs = ImageRebase((uint8_t *)img, len, 0);
    uint8_t *b;
    if(csubs < 0) error("Invalid program");
    b = csubs ? GetExecMemory(len) : GetMemory(len);
    memcpy(b, img, len);
    ImageRebase(b, len, (int32_t)b - (int32_t)base);
    if(csubs) {
        SCB_CleanDCache();                                          // a CSUB's code runs from it
        SCB_InvalidateICache();
    }
    return b;
}

// FLASH (RAM) RUN, CHAIN and LOAD, before anything is cleared: an error if slot n has no program of this firmware's
// (0: program memory's). A flash slot's file is checked from its start (its layout when it is read: SlotProgram).
void SlotProgramCheck(int slot) {
    uint8_t head[T_NEWLINE_HDR + IMAGE_NAME + FF_MAX_LFN];
    int n, kind;
    if(slot == 0) {
        if(!ImageIsCurrentAt(ProgFlash)) error("Nothing to run");
        return;
    }
    if(slot > MAXFLASHSLOTS) {
        if(ImageSlot[slot] == NULL || SlotKind[slot] != SLOT_PROGRAM) SlotRefuse(slot, SLOT_DATA, NULL, 0);
        return;
    }
    n = SlotHead(slot, head, sizeof(head));
    if((kind = SlotKindOf(head, n)) != SLOT_PROGRAM) SlotRefuse(slot, kind, head, n);
}

// slot n's program in its block, to run in place: an error if it holds none of this firmware's
char *SlotProgram(int slot) {
    uint8_t *b = slot <= MAXFLASHSLOTS ? SlotGet(slot) : ImageSlot[slot];
    if(b == NULL) SlotRefuse(slot, SLOT_DATA, NULL, 0);
    if(SlotKind[slot] != SLOT_PROGRAM) SlotRefuse(slot, SlotKind[slot], b, MemSize(b));
    return (char *)b;
}

// ProgMemory back at program memory (the prompt, and before a program is loaded), and the tables made for the slot's
// program forgotten, to be made again for program memory's (as after a heap wipe: their blocks go with the next one)
void SlotProgramLeave(void) {
    FlashLoad = 0;
    if(ProgMemory == ProgFlash) return;
    ProgMemory = ProgFlash;
    ProgramCachesForget();
    findlabel(NULL);
}

// the address of slot n's header (an empty flash slot: FF, an empty RAM slot: a zero header)
uint8_t *ImageSlotAddress(int slot) {
    uint8_t *b;
    if(slot < 1 || slot > MAXIMAGESLOTS) error("Invalid slot %", slot);
    if(slot <= MAXFLASHSLOTS) return (b = SlotGet(slot)) != NULL ? b : (uint8_t *)ImageSlotErased;
    if(ImageSlot[slot] == NULL) return (uint8_t *)ImageSlotEmpty;
    return ImageSlot[slot];
}

// the image in slot n: its pixels (past the header), width and height; the error 'none' if there is none (the
// PicoMite's: BLIT FLASH's "Invalid Image", TILEMAP's "No image in slot %")
uint8_t *ImageSlotPixels(int slot, int *w, int *h, const char *none) {
    uint32_t *hdr;
    hdr = (uint32_t *)ImageSlotAddress(slot);
    *w = (int)hdr[0];
    *h = (int)hdr[1];
    if(*w < 1 || *w > 3840 || *h < 1 || *h > 2160) error((char *)none, slot);
    return (uint8_t *)hdr + 8;
}

// FLASH LOAD IMAGE: a decoded row (RGB888, four bytes a pixel) packed to RGB121 nibbles in the slot
static uint8_t *SlotLoading;
static bool SlotLineCallback(int *imagewidth, int *imageheight, uint32_t *linedata, int *linenumber) {
    int w = *imagewidth, stride = (w + 1) / 2;
    uint8_t *s = (uint8_t *)linedata, *d;
    if(*linenumber < 0 || *linenumber >= *imageheight) return true;
    d = SlotLoading + 8 + *linenumber * stride;
    for(int i = 0; i < w; i += 2) {                                 // the PicoMite's flashpackline
        uint32_t c = s[0] | (s[1] << 8) | (s[2] << 16);
        uint8_t b = ((c & 0x800000) >> 20) | ((c & 0xC000) >> 13) | ((c & 0x80) >> 7);
        s += 4;
        if(i + 1 < w) {
            c = s[0] | (s[1] << 8) | (s[2] << 16);
            b |= (((c & 0x800000) >> 20) | ((c & 0xC000) >> 13) | ((c & 0x80) >> 7)) << 4;
            s += 4;
        }
        *d++ = b;
    }
    return true;
}

// does slot n hold anything ("Already programmed")?
static int SlotInUse(int slot) {
    return slot <= MAXFLASHSLOTS ? SlotSize(slot) != 0 : ImageSlot[slot] != NULL;
}

// FLASH SAVE n and FLASH OVERWRITE n: the program in memory into flash slot n, its CSUBs' addresses made offsets
static void SlotSave(int slot, int overwrite) {
    int len;
    uint8_t *img;
    if(CurrentLinePtr) error("Invalid in a program");
    if(!overwrite && SlotSize(slot)) error("Already programmed");
    if(!ImageIsCurrentAt(ProgMemory) || (len = ImageLayout((unsigned char *)ProgMemory, PROG_FLASH_SIZE, NULL)) == 0)
        error("Nothing to save");
    img = GetMemory(len);
    memcpy(img, ProgMemory, len);
    ImageRebase(img, len, -(int32_t)ProgMemory);
    SlotWrite(slot, img, len);
    FreeMemorySafe((void *)&img);
}

// RAM SAVE n and RAM OVERWRITE n: the program in memory (in a program, the one running) into RAM slot n
static void RamSave(int slot, int overwrite) {
    int len;
    uint8_t *b;
    if(!overwrite && ImageSlot[slot] != NULL) error("Already programmed");
    if(!ImageIsCurrentAt(ProgMemory) || (len = ImageLayout((unsigned char *)ProgMemory, PROG_FLASH_SIZE, NULL)) == 0)
        error("Nothing to save");
    b = ImagePlace((uint8_t *)ProgMemory, len, (uint8_t *)ProgMemory);
    SlotDrop(slot);
    ImageSlot[slot] = b;
    SlotKind[slot] = SLOT_PROGRAM;
}

// FLASH LOAD n and RAM LOAD n: slot n's program into program memory, its CSUBs' addresses made there, as LOAD loads a
// file's; RUN then runs it as it is, not its file again (ProgFromSlot)
static void SlotLoadProgram(int slot) {
    char *img, *copy;
    int len;
    if(CurrentLinePtr) error("Invalid in a program");
    SlotProgramCheck(slot);
    ClearProgram();
    img = SlotProgram(slot);
    len = ImageLayout((unsigned char *)img, MemSize(img), NULL);
    copy = GetMemory(len);
    memcpy(copy, img, len);
    ImageRebase((uint8_t *)copy, len, (int32_t)ProgMemory - (int32_t)img);
    ProgramImageToFlash(copy, len);
    FreeMemorySafe((void *)&copy);
    ProgFromSlot = 1;
}

// FLASH LIST n [, ALL] and RAM LIST n [, ALL]: slot n's program listed as it is held (as LIST FLASH lists program
// memory), its names from its own symbol table
static void SlotListProgram(int slot, int all) {
    char *img = SlotProgram(slot);
    SymSetProgram(img);
    ListProgramFlash(img, all);
    SymSetProgram(ProgMemory);
}

// FLASH LIST and RAM LIST: what each flash slot (1 to 3) or RAM slot (4 to 8, RAM's 1 to 5) holds (the PicoMite's)
static void SlotListAll(int ram) {
    uint8_t head[T_NEWLINE_HDR + IMAGE_NAME + FF_MAX_LFN];
    char name[FF_MAX_LFN];
    uint32_t wh[2];
    int n, kind;
    for(int slot = ram ? MAXFLASHSLOTS + 1 : 1; slot <= (ram ? MAXIMAGESLOTS : MAXFLASHSLOTS); slot++) {
        if(!ram) n = SlotHead(slot, head, sizeof(head));
        else if(ImageSlot[slot] == NULL) n = 0;
        else {
            n = MemSize(ImageSlot[slot]);
            if(n > (int)sizeof(head)) n = sizeof(head);
            memcpy(head, ImageSlot[slot], n);
        }
        MMPrintString(ram ? " RAM Slot " : " Slot ");
        PInt(ram ? slot - MAXFLASHSLOTS : slot);
        if(n == 0) {
            MMPrintString(" available\r\n");
            continue;
        }
        MMPrintString(" in use");
        kind = SlotKindOf(head, n);
        if(kind != SLOT_DATA) {
            SlotName(head, n, name);
            MMPrintString(": \"");
            MMPrintString(name);
            MMPrintString("\"");
            if(kind == SLOT_OLD) MMPrintString(", saved by another firmware version");
        } else if(n >= 8) {
            memcpy(wh, head, 8);
            if(wh[0] >= 1 && wh[0] <= 3840 && wh[1] >= 1 && wh[1] <= 2160) {
                MMPrintString(": image ");
                PInt(wh[0]);
                MMPrintString(" x ");
                PInt(wh[1]);
            }
        }
        MMPrintString("\r\n");
    }
}

void cmd_flash(void) {
    char *p, fp[FF_MAX_LFN];
    int slot;
    if((p = checkstring(cmdline, "LOAD IMAGE"))) {
        int width, height, len, overwrite = 0;
        getargs(&p, 5, ",");
        if(!(argc == 3 || argc == 5)) error("Syntax");
        slot = getint(argv[0], 1, MAXIMAGESLOTS);
        if(argc == 5) {
            if(checkstring(argv[4], "O") || checkstring(argv[4], "OVERWRITE")) overwrite = 1;
            else error("Syntax");
        }
        if(SlotInUse(slot) && !overwrite) error("Already programmed");
        if(!InitSDCard()) return;
        strcpy(fp, getFstring(argv[2]));
        if(strchr(&fp[strlen(fp) > 4 ? strlen(fp) - 4 : 0], '.') == NULL) strcat(fp, ".bmp");
        BMPfnbr = FindFreeFileNbr();
        if(!BasicFileOpen(fp, BMPfnbr, FA_READ)) return;
        decodeBMPheader(&width, &height);
        if(width < 1 || width > 3840 || height < 1 || height > 2160) {
            FileClose(BMPfnbr);
            error("Invalid BMP file");
        }
        if(slot > MAXFLASHSLOTS) SlotDrop(slot);                    // (a flash slot keeps its image until the new one is saved)
        len = 8 + ((width + 1) / 2) * height;
        SlotLoading = GetTempMemory(len);                           // (temporary: the decoder's errors are raised inside
        memset(SlotLoading, 0, len);                                // decodeBMP, and it is freed after the statement)
        ((uint32_t *)SlotLoading)[0] = width;
        ((uint32_t *)SlotLoading)[1] = height;
        if(!CurrentLinePtr) {
            MMPrintString("Saving ");
            MMPrintString(fp);
            MMPrintString(slot <= MAXFLASHSLOTS ? " to flash slot " : " to RAM slot ");
            PInt(slot <= MAXFLASHSLOTS ? slot : slot - MAXFLASHSLOTS);
            MMPrintString("\r\nImage is ");
            PInt(width);
            MMPrintString(" by ");
            PInt(height);
            MMPrintString(" RGB121 pixels\r\n");
        }
        linecallback = SlotLineCallback;
        BMP_Result r = decodeBMP(true);
        linecallback = NULL;
        FileClose(BMPfnbr);
        if(!r.success) error(r.errorMsg);
        uint8_t *kept = GetMemory(len);
        memcpy(kept, SlotLoading, len);
        if(slot <= MAXFLASHSLOTS) SlotWrite(slot, kept, len);
        ImageSlot[slot] = kept;
        SlotKind[slot] = SLOT_DATA;
        SlotLoading = NULL;
        return;
    }
    if((p = checkstring(cmdline, "DISK LOAD"))) {                  // FLASH DISK LOAD n, f$ [, O]: a file's bytes into slot n, as
        int overwrite = 0, fnbr;                                    // they are (the PicoMite's, for its flash slots; PETSCII Robots
        unsigned int size, got = 0;                                 // keeps its sprites so, read at MM.INFO(FLASH ADDRESS n))
        getargs(&p, 5, ",");
        if(!(argc == 3 || argc == 5)) error("Syntax");
        slot = getint(argv[0], 1, MAXIMAGESLOTS);
        if(argc == 5) {
            if(checkstring(argv[4], "O") || checkstring(argv[4], "OVERWRITE")) overwrite = 1;
            else error("Syntax");
        }
        if(SlotInUse(slot) && !overwrite) error("Already programmed");
        if(!InitSDCard()) return;
        strcpy(fp, getFstring(argv[2]));
        fnbr = FindFreeFileNbr();
        if(!BasicFileOpen(fp, fnbr, FA_READ)) return;
        size = f_size(FileTable[fnbr].fptr);
        if(size == 0) {
            FileClose(fnbr);
            error("Empty file");
        }
        if(slot > MAXFLASHSLOTS) SlotDrop(slot);
        SlotLoading = GetMemory(size);
        FileGetData(fnbr, SlotLoading, size, &got);
        FileClose(fnbr);
        if(got != size) {
            FreeMemorySafe((void *)&SlotLoading);
            error("File read error");
        }
        if(slot <= MAXFLASHSLOTS) {                                 // a flash slot: its file, read again when it is wanted
            SlotWrite(slot, SlotLoading, size);                     // (a program's image is made ready to run then: SlotGet)
            FreeMemorySafe((void *)&SlotLoading);
            return;
        }
        ImageSlot[slot] = SlotLoading;
        SlotKind[slot] = SLOT_DATA;
        SlotLoading = NULL;
        return;
    }
    if((p = checkstring(cmdline, "ERASE ALL"))) {
        if(CurrentLinePtr) error("Invalid in a program");
        checkend(p);
        for(slot = 1; slot <= MAXFLASHSLOTS; slot++) SlotErase(slot);
        return;
    }
    if((p = checkstring(cmdline, "ERASE"))) {
        SlotErase(getint(p, 1, MAXFLASHSLOTS));
        return;
    }
    if((p = checkstring(cmdline, "SAVE"))) {
        SlotSave(getint(p, 1, MAXFLASHSLOTS), 0);
        return;
    }
    if((p = checkstring(cmdline, "OVERWRITE"))) {
        SlotSave(getint(p, 1, MAXFLASHSLOTS), 1);
        return;
    }
    if((p = checkstring(cmdline, "LOAD"))) {
        SlotLoadProgram(getint(p, 1, MAXFLASHSLOTS));
        return;
    }
    if((p = checkstring(cmdline, "RUN"))) {
        slot = getint(p, 0, MAXFLASHSLOTS);
        mymemset(runcmd, 0, STRINGSIZE);                            // (MM.CMDLINE$ is empty)
        FlashRunSlot(slot, 0);
        return;
    }
    if((p = checkstring(cmdline, "CHAIN"))) {
        if(!CurrentLinePtr) error("Invalid at command prompt");
        slot = getint(p, 0, MAXFLASHSLOTS);
        mymemset(runcmd, 0, STRINGSIZE);
        FlashRunSlot(slot, 1);
        return;
    }
    if((p = checkstring(cmdline, "LIST"))) {
        getargs(&p, 3, ",");
        if(argc == 0) {
            SlotListAll(0);
            return;
        }
        if(!(argc == 1 || (argc == 3 && checkstring(argv[2], "ALL")))) error("Syntax");
        SlotListProgram(getint(argv[0], 1, MAXFLASHSLOTS), argc == 3);
        return;
    }
    error("Syntax");
}

/* RAM (the PicoMite's, for its PSRAM slots): FLASH's program commands for the RAM slots 1 to 5, which are the image
   slots 4 to 8, and RAM FILE LOAD. A RAM slot is a heap block that every heap wipe keeps (RamSlotBlocks): it goes
   with RAM ERASE, an overwrite or a restart, as the PicoMite's PSRAM slots go with a reset. A program is held there
   as program memory holds one, its CSUBs' addresses made in the block (the executable window if it has a CSUB), and
   runs there in place; MM.INFO(FLASH) gives its RAM slot's number, as the PicoMite's does.

  RAM SAVE n / RAM OVERWRITE n          the program in memory (in a program, the one running) into RAM slot n
  RAM FILE LOAD n, f$ [, O | OVERWRITE]  a program file tokenised into RAM slot n, the running program untouched
  RAM LOAD n                            RAM slot n's program into program memory (at the prompt)
  RAM RUN n / RAM CHAIN n               its program run in place (0: program memory's); CHAIN keeps the globals
  RAM LIST [n [, ALL]]                  what the RAM slots hold, or slot n's program listed
  RAM ERASE n / RAM ERASE ALL           emptied
*/
static int RamSlot(char *p) {
    return getint(p, 1, MAXIMAGESLOTS - MAXFLASHSLOTS) + MAXFLASHSLOTS;
}

void cmd_ram(void) {
    char *p;
    int slot;
    if((p = checkstring(cmdline, "ERASE ALL"))) {
        checkend(p);
        for(slot = MAXFLASHSLOTS + 1; slot <= MAXIMAGESLOTS; slot++) SlotDrop(slot);
        return;
    }
    if((p = checkstring(cmdline, "ERASE"))) {
        SlotDrop(RamSlot(p));
        return;
    }
    if((p = checkstring(cmdline, "FILE LOAD"))) {
        int len, overwrite = 0;
        char *img;
        uint8_t *b;
        getargs(&p, 5, ",");
        if(!(argc == 3 || argc == 5)) error("Syntax");
        slot = RamSlot(argv[0]);
        if(argc == 5) {
            if(checkstring(argv[4], "O") || checkstring(argv[4], "OVERWRITE")) overwrite = 1;
            else error("Syntax");
        }
        if(ImageSlot[slot] != NULL && !overwrite) error("Already programmed");
        if((img = ProgramFileImage(getFstring(argv[2]), &len)) == NULL) return;   // (FileIO.c)
        b = ImagePlace((uint8_t *)img, len, (uint8_t *)img);
        FreeMemorySafe((void *)&img);
        SlotDrop(slot);
        ImageSlot[slot] = b;
        SlotKind[slot] = SLOT_PROGRAM;
        return;
    }
    if((p = checkstring(cmdline, "SAVE"))) {
        RamSave(RamSlot(p), 0);
        return;
    }
    if((p = checkstring(cmdline, "OVERWRITE"))) {
        RamSave(RamSlot(p), 1);
        return;
    }
    if((p = checkstring(cmdline, "LOAD"))) {
        SlotLoadProgram(RamSlot(p));
        return;
    }
    if((p = checkstring(cmdline, "RUN"))) {
        slot = getint(p, 0, MAXIMAGESLOTS - MAXFLASHSLOTS);
        mymemset(runcmd, 0, STRINGSIZE);                            // (MM.CMDLINE$ is empty)
        FlashRunSlot(slot ? slot + MAXFLASHSLOTS : 0, 0);
        return;
    }
    if((p = checkstring(cmdline, "CHAIN"))) {
        if(!CurrentLinePtr) error("Invalid at command prompt");
        slot = getint(p, 0, MAXIMAGESLOTS - MAXFLASHSLOTS);
        mymemset(runcmd, 0, STRINGSIZE);
        FlashRunSlot(slot ? slot + MAXFLASHSLOTS : 0, 1);
        return;
    }
    if((p = checkstring(cmdline, "LIST"))) {
        getargs(&p, 3, ",");
        if(argc == 0) {
            SlotListAll(1);
            return;
        }
        if(!(argc == 1 || (argc == 3 && checkstring(argv[2], "ALL")))) error("Syntax");
        SlotListProgram(RamSlot(argv[0]), argc == 3);
        return;
    }
    error("Syntax");
}

// a colour as page 'page' stores it (DrawPixel8, DrawPixel16 and DrawPixel32's conversions; opaque in 12-bit)
static uint32_t ImageNative(int c) {
    if(VideoColour == 8) return ((c & 0xE00000) >> 16) | ((c & 0xE000) >> 11) | ((c & 0xC0) >> 6);
    if(VideoColour == 16) return (BIT_5[((c & 0xFF0000) >> 19)] << 11) | (BIT_6[((c & 0xFF00) >> 10)] << 5) | BIT_5[((c & 0xFF) >> 3)];
    if(VideoColour == 12) return (BIT_4[((c & 0xFF0000) >> 20)] << 8) | (BIT_4[((c & 0xFF00) >> 12)] << 4) | BIT_4[((c & 0xFF) >> 4)] | 0xF000;
    return (uint32_t)c | 0xFF000000;
}

/* Draw the w x h pixels at (sx, sy) of a slot image (pixels 'src', srcw x srch) into page 'page' at (dx, dy), clipped
   to both; a pixel of value 'transparent' (0-15) is left alone, -1 for none. The PicoMite's blit121. */
void ImageSlotBlit(const uint8_t *src, int srcw, int srch, int sx, int sy, int w, int h, int page, int dx, int dy, int transparent) {
    uint32_t native[16];
    int stride = (srcw + 1) / 2, maxW, maxH, expand, nb;
    uint8_t *base;
    if(page < 0 || page > LastPage || PageTable[page].address == NULL) error("Invalid page");
    maxW = PageTable[page].xmax;
    maxH = PageTable[page].ymax;
    expand = PageTable[page].expand;
    base = (uint8_t *)PageTable[page].address;
    nb = VideoColour == 8 ? 1 : (VideoColour == 32 ? 4 : 2);
    if(sx < 0) { dx -= sx; w += sx; sx = 0; }                       // clipped to the image
    if(sy < 0) { dy -= sy; h += sy; sy = 0; }
    if(sx + w > srcw) w = srcw - sx;
    if(sy + h > srch) h = srch - sy;
    if(dx < 0) { sx -= dx; w += dx; dx = 0; }                       // and to the page
    if(dy < 0) { sy -= dy; h += dy; dy = 0; }
    if(dx + w > maxW) w = maxW - dx;
    if(dy + h > maxH) h = maxH - dy;
    if(w <= 0 || h <= 0) return;
    if(VideoColour == 4) {                                          // a 4-bit page: the values themselves (Rgb121.c)
        for(int y = 0; y < h; y++) {
            int py = dy + y;
            if(optiony) py = maxH - 1 - py;
            Rgb121Row(base + py * ((maxW + 1) >> 1), dx, src + (sy + y) * stride, sx, w, transparent);
        }
        return;
    }
    if(VideoColour == 1) {                                          // a 1-bit page: a value not 0 sets the pixel (Mono.c;
        for(int y = 0; y < h; y++) {                                // the PicoMite's MODE 1 writes 4-bit rows over it)
            const uint8_t *s = src + (sy + y) * stride;
            int py = dy + y;
            if(optiony) py = maxH - 1 - py;
            uint8_t *d = base + py * (maxW >> 3);
            for(int x = 0; x < w; x++) {
                int sxx = sx + x, dxx = dx + x;
                int v = (s[sxx >> 1] >> ((sxx & 1) << 2)) & 0xF;
                if(v == transparent) continue;
                if(v) d[dxx >> 3] |= 1 << (dxx & 7);
                else d[dxx >> 3] &= ~(1 << (dxx & 7));
            }
        }
        MonoDirty = 1;
        return;
    }
    for(int i = 0; i < 16; i++) native[i] = ImageNative(Rgb121Colours[i]);
    for(int y = 0; y < h; y++) {
        const uint8_t *s = src + (sy + y) * stride;
        int py = dy + y;
        if(optiony) py = maxH - 1 - py;
        int row = expand ? py * 2 : py;
        uint8_t *d0 = base + (row * maxW + dx) * nb, *d1 = expand ? d0 + maxW * nb : NULL;
        for(int x = 0; x < w; x++) {
            int sxx = sx + x;
            int v = (s[sxx >> 1] >> ((sxx & 1) << 2)) & 0xF;
            if(v != transparent) {
                uint32_t c = native[v];
                if(nb == 1) { d0[x] = c; if(d1) d1[x] = c; }
                else if(nb == 2) { ((uint16_t *)d0)[x] = c; if(d1) ((uint16_t *)d1)[x] = c; }
                else { ((uint32_t *)d0)[x] = c; if(d1) ((uint32_t *)d1)[x] = c; }
            }
        }
    }
}

/* BLIT FLASH n, dest, x1, y1, x2, y2, w, h [, colour]: the w x h pixels at x1, y1 of slot n's image onto a framebuffer
   (N, L, F, 2, T: Rgb121Letter) or a page at x2, y2, a pixel of value 'colour' (0-15) left alone. The PicoMite's. */
void BlitFlash(char *p) {
    int sw, sh, x1, y1, x2, y2, w, h, page, blank = -1;
    uint8_t *s;
    getargs(&p, 17, ",");
    if(!(argc == 15 || argc == 17)) error("Syntax");
    s = ImageSlotPixels(getint(argv[0], 1, MAXIMAGESLOTS), &sw, &sh, "Invalid Image");
    if(VideoColour < 8 && checkstring(argv[2], "2") && !(Rgb121Buffers & (1 << RGB121_2))) error("Frame buffer 2 not created");
    if((page = Rgb121Letter(argv[2])) < 0) page = getint(argv[2], 0, LastPage);
    x1 = getinteger(argv[4]);
    y1 = getinteger(argv[6]);
    x2 = getinteger(argv[8]);
    y2 = getinteger(argv[10]);
    w = getinteger(argv[12]);
    h = getinteger(argv[14]);
    if(x1 < 0 || y1 < 0 || x1 + w > sw || y1 + h > sh) error("Number out of bounds");
    if(argc == 17) blank = getint(argv[16], -1, 15);
    ImageSlotBlit(s, sw, sh, x1, y1, w, h, page, x2, y2, blank);
}
