/***********************************************************************************************************************
Image slots (the PicoMite's FLASH LOAD IMAGE, 18 Oct: for TILEMAP, and BLIT FLASH to come) and the writer that draws
their 4-bit pixels into a CMM2 page.

A slot holds what the PicoMite's does: an 8-byte header (its width and height, two 32-bit words), then the pixels as
4-bit RGB121 values packed two to a byte, the left pixel in the low nibble, each row (width + 1) / 2 bytes. Programs PEEK
the header (MM.INFO(FLASH ADDRESS n)) to see whether a slot is loaded, so the layout is the PicoMite's exactly.

Slots 4 to 8 are the PicoMite's RAM slots, which its games use where there is PSRAM (MM.INFO(PSRAM SIZE) says the CMM2
has). Here each is a heap block made by FLASH LOAD IMAGE (Peter, 18 Oct: no fixed SDRAM area), freed with the heap by
RUN, NEW and a program load, so a program loads its images at every start, as the PicoMite's games do. An empty slot
reads as a zero header, as an empty PicoMite RAM slot does. Slots 1 to 3, the PicoMite's flash slots, are not here yet.

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

#define MAXFLASHSLOTS 3                                             // the PicoMite's flash slots (not here yet); then its RAM slots, 4 to 8
extern bool (*linecallback)(int *imagewidth, int *imageheight, uint32_t *linedata, int *linenumber);

static uint8_t *ImageSlot[MAXIMAGESLOTS + 1];                       // a RAM slot's heap block, or NULL
static const uint32_t ImageSlotEmpty[4] = {0, 0, 0, 0};             // what an empty slot reads as

// the PicoMite's RGB121 colours (graphics/Draw.c colours[16]): bit 3 red, bits 2-1 green, bit 0 blue
const int Rgb121Colours[16] = {0x00, 0xFF, 0x4000, 0x40ff, 0x8000, 0x80ff, 0xff00, 0xffff,
                               0xff0000, 0xff00FF, 0xff4000, 0xff40ff, 0xff8000, 0xff80ff, 0xffff00, 0xffffff};

// the heap was wiped (HeapWipe): the blocks went with it
void ImageSlotsForget(void) {
    memset(ImageSlot, 0, sizeof(ImageSlot));
}

// the address of slot n's header (an empty slot: a zero header)
uint8_t *ImageSlotAddress(int slot) {
    if(slot < 1 || slot > MAXIMAGESLOTS) error("Invalid slot %", slot);
    if(slot <= MAXFLASHSLOTS || ImageSlot[slot] == NULL) return (uint8_t *)ImageSlotEmpty;
    return ImageSlot[slot];
}

// the image in slot n: its pixels (past the header), width and height; an error if there is none
uint8_t *ImageSlotPixels(int slot, int *w, int *h) {
    uint32_t *hdr;
    if(slot >= 1 && slot <= MAXFLASHSLOTS) error("Flash image slots are not provided: use slots 4 to 8");
    hdr = (uint32_t *)ImageSlotAddress(slot);
    *w = (int)hdr[0];
    *h = (int)hdr[1];
    if(*w < 1 || *w > 3840 || *h < 1 || *h > 2160) error("No image in slot %", slot);
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

// FLASH LOAD IMAGE n, f$ [, O] (the PicoMite's other FLASH commands are its program slots: not here)
void cmd_flash(void) {
    char *p, fp[FF_MAX_LFN];
    if((p = checkstring(cmdline, "LOAD IMAGE"))) {
        int slot, width, height, overwrite = 0;
        getargs(&p, 5, ",");
        if(!(argc == 3 || argc == 5)) error("Syntax");
        slot = getint(argv[0], 1, MAXIMAGESLOTS);
        if(argc == 5) {
            if(checkstring(argv[4], "O") || checkstring(argv[4], "OVERWRITE")) overwrite = 1;
            else error("Syntax");
        }
        if(slot <= MAXFLASHSLOTS) error("Flash image slots are not provided: use slots 4 to 8");
        if(ImageSlot[slot] != NULL && !overwrite) error("Already programmed");
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
        if(ImageSlot[slot] != NULL) FreeMemorySafe((void *)&ImageSlot[slot]);
        SlotLoading = GetMemory(8 + ((width + 1) / 2) * height);    // (zeroed by the allocator)
        ((uint32_t *)SlotLoading)[0] = width;
        ((uint32_t *)SlotLoading)[1] = height;
        if(!CurrentLinePtr) {
            MMPrintString("Saving ");
            MMPrintString(fp);
            MMPrintString(" to RAM slot ");
            PInt(slot - MAXFLASHSLOTS);
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
        if(!r.success) {
            FreeMemorySafe((void *)&SlotLoading);
            error(r.errorMsg);
        }
        ImageSlot[slot] = SlotLoading;
        SlotLoading = NULL;
        return;
    }
    if((p = checkstring(cmdline, "DISK LOAD"))) {                  // FLASH DISK LOAD n, f$ [, O]: a file's bytes into slot n, as
        int slot, overwrite = 0, fnbr;                              // they are (the PicoMite's, for its flash slots; PETSCII Robots
        unsigned int size, got = 0;                                 // keeps its sprites so, read at MM.INFO(FLASH ADDRESS n))
        getargs(&p, 5, ",");
        if(!(argc == 3 || argc == 5)) error("Syntax");
        slot = getint(argv[0], 1, MAXIMAGESLOTS);
        if(argc == 5) {
            if(checkstring(argv[4], "O") || checkstring(argv[4], "OVERWRITE")) overwrite = 1;
            else error("Syntax");
        }
        if(slot <= MAXFLASHSLOTS) error("Flash slots are not provided: use slots 4 to 8");
        if(ImageSlot[slot] != NULL && !overwrite) error("Already programmed");
        if(!InitSDCard()) return;
        strcpy(fp, getFstring(argv[2]));
        fnbr = FindFreeFileNbr();
        if(!BasicFileOpen(fp, fnbr, FA_READ)) return;
        size = f_size(FileTable[fnbr].fptr);
        if(size == 0) {
            FileClose(fnbr);
            error("Empty file");
        }
        if(ImageSlot[slot] != NULL) FreeMemorySafe((void *)&ImageSlot[slot]);
        SlotLoading = GetMemory(size);
        FileGetData(fnbr, SlotLoading, size, &got);
        FileClose(fnbr);
        if(got != size) {
            FreeMemorySafe((void *)&SlotLoading);
            error("File read error");
        }
        ImageSlot[slot] = SlotLoading;
        SlotLoading = NULL;
        return;
    }
    error("Only FLASH LOAD IMAGE and FLASH DISK LOAD are provided on the CMM2");
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
    s = ImageSlotPixels(getint(argv[0], 1, MAXIMAGESLOTS), &sw, &sh);
    if(VideoColour == 4 && checkstring(argv[2], "2") && !(Rgb121Buffers & (1 << RGB121_2))) error("Frame buffer 2 not created");
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
