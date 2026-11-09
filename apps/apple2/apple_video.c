/* apple_video.c: the Apple //e's screen at its full resolution, for the Colour Maximite 2 (Peter, 9 Nov 2026). frank-apple draws the Apple's 560 x 192 into
   320 x 240 for an RP2350's HDMI, which halves 80 columns and double hi-res; this draws them as MII's own renderer does on a PC (mii_video.c, its
   !MII_RP2350 part, by Michel Pollet, MIT licence: text, lo-res, double lo-res, hi-res with its colour artifacts, double hi-res in colour and in mono, mixed
   mode, and MII's colour sets and green and amber screens), each line from the Apple's memory as it is when a frame ends, into 8-bit pixels, 560 across.
   Double hi-res is in colour, as on a //e with a colour monitor, but on the green and amber screens: MII (and frank-apple) show it in black and white
   when the mode register of a Video-7 RGB card says so, and the //e's ROM moves that register at each reset, so that one Ctrl+Reset turned double
   hi-res black and white for good (9 Nov 2026; a2audit's video tests want colour too).
   A pixel is the place in MII's colour tables (video->clut, and clut_low for the dimmer pixels MII puts where a colour changes) of its colour, which
   apple_cmm2.c makes the palette; so a colour set is a palette change. */
#include <stddef.h>
#include <string.h>
#include "mii.h"
#include "mii_sw.h"
#include "apple_video.h"

#define MIXED_LINE (192 - 4 * 8)                /* mii_video.c's MII_VIDEO_MIXED_LINE */
#define FLASH_MASK 0x10                         /* MII_VIDEO_FLASH_FRAME_MASK */
#define IX(f) (int)(offsetof(mii_video_clut_t, f) / sizeof(mii_color_t))

static inline uint8_t reverse4(uint8_t b) {
    return (b & 1) << 3 | (b & 2) << 1 | (b & 4) >> 1 | (b & 8) >> 3;
}

static inline uint8_t get1(const uint8_t *buffer, int bit) {
    return (buffer[bit >> 3] >> (7 - (bit & 7))) & 1;
}

/* the line's address in the text and lo-res page */
static uint16_t text_addr(uint32_t sw, int line) {
    bool page2 = SWW_GETSTATE(sw, SW80STORE) ? 0 : SWW_GETSTATE(sw, SWPAGE2);
    int i = line >> 3;
    return (0x400 + 0x400 * page2) + (((i & 0x07) << 7) | ((i >> 3) << 5) | ((i >> 3) << 3));
}

static void line_text(mii_video_t *video, uint32_t sw, mii_bank_t *main, mii_bank_t *aux, int line, uint8_t *d) {
    uint16_t a = text_addr(sw, line);
    const uint8_t *rom = video->rom->rom;
    bool col80 = SWW_GETSTATE(sw, SW80COL), altset = SWW_GETSTATE(sw, SWALTCHARSET);
    int flash = video->frame_count & FLASH_MASK ? -0x40 : 0x40;
    uint8_t m[40], x40[40];
    const uint8_t on = IX(mono) + 1, off = IX(mono);
    rom += video->rom->len > (4 * 1024) && video->rom_bank ? (4 * 1024) : 0;
    mii_bank_read(main, a, m, 40);
    if(col80) mii_bank_read(aux, a, x40, 40);
    for(int x = 0; x < 40 + 40 * col80; x++) {
        uint8_t c = col80 ? (x & 1 ? m[x >> 1] : x40[x >> 1]) : m[x];
        if(!altset && c >= 0x40 && c <= 0x7f) c = (int)c + flash;
        uint8_t bits = rom[(c << 3) + (line & 7)];
        for(int pi = 0; pi < 7; pi++) {
            uint8_t col = (bits >> pi) & 1 ? off : on;
            *d++ = col;
            if(!col80) *d++ = col;
        }
    }
}

static void line_lores(mii_video_t *video, uint32_t sw, mii_bank_t *main, mii_bank_t *aux, int line, uint8_t *d) {
    uint16_t a = text_addr(sw, line);
    bool col80 = SWW_GETSTATE(sw, SW80COL);
    uint8_t m[40], x40[40];
    uint32_t lastcolor = 0;
    mii_bank_read(main, a, m, 40);
    if(col80) mii_bank_read(aux, a, x40, 40);
    for(int x = 0; x < 40 + 40 * col80; x++) {
        uint16_t c = col80 ? (x & 1 ? m[x >> 1] : x40[x >> 1]) : m[x];
        c = (c >> (((line / 4) & 1) * 4)) & 0xf;
        uint8_t color = IX(lores) + ((x & col80) ^ col80) * 16 + c, dim = color + LOW;
        if(!video->monochrome) {
            for(int pi = 0; pi < 7; pi++) {
                uint8_t pixel = color;
                if(video->clut.colors[color] != lastcolor) {
                    pixel = dim;
                    lastcolor = video->clut.colors[color];
                }
                *d++ = pixel;
                if(!col80) *d++ = pixel;
            }
        } else {
            c = reverse4(c);
            c |= c << 4;
            c |= c << 8;
            if(col80) {
                for(int pi = 0; pi < 7; pi++) *d++ = (c >> pi) & 1 ? color : dim;
            } else {
                if(x & 1) c >>= 2;
                for(int pi = 0; pi < 14; pi++) *d++ = (c >> pi) & 1 ? color : dim;
            }
        }
    }
}

/* the hi-res and double hi-res line's address */
static uint16_t hires_addr(uint32_t sw, int line) {
    bool page2 = SWW_GETSTATE(sw, SW80STORE) ? 0 : SWW_GETSTATE(sw, SWPAGE2);
    return _mii_line_to_video_addr(0x2000 + 0x2000 * page2, (uint8_t)line);
}

static void line_hires(mii_video_t *video, uint32_t sw, mii_bank_t *main, int line, uint8_t *d) {
    uint8_t src[41];
    uint8_t b0 = 0, b1;
    uint32_t lastcol = 0;
    mii_bank_read(main, hires_addr(sw, line), src, 40);
    src[40] = 0;                                /* (the last column is clear: no wrap round) */
    b1 = src[0];
    for(int x = 0; x < 40; x++) {
        uint8_t b2 = src[x + 1];
        uint16_t run = ((b0 & 0x60) >> 5) | ((b1 & 0x7f) << 2) | ((b2 & 0x03) << 9);   /* the last 2 pixels, these 7 and the next 2 */
        int odd = (x & 1) << 1;
        int offset = (b1 & 0x80) >> 5;
        if(!video->monochrome) {
            for(int i = 0; i < 7; i++) {
                uint8_t left = (run >> (1 + i)) & 1, pixel = (run >> (2 + i)) & 1, right = (run >> (3 + i)) & 1;
                int idx = 0;
                if(pixel) idx = left || right ? 9 : offset + odd + (i & 1) + 1;
                else if(left && right) idx = offset + odd + 1 - (i & 1) + 1;
                uint8_t col = IX(hires) + idx;
                if(video->clut.colors[col] != lastcol) {
                    d[0] = d[1] = col + LOW;
                    lastcol = video->clut.colors[col];
                } else d[0] = d[1] = col;
                d += 2;
            }
        } else {
            for(int i = 0; i < 7; i++) {
                uint8_t pixel = (run >> (2 + i)) & 1, col = IX(mono) + pixel;
                if(video->clut.colors[col] != lastcol) {
                    *d++ = MASKED + pixel;          /* (MII's C_SCANLINE_MASK of that colour) */
                    lastcol = video->clut.colors[col];
                } else *d++ = col;
                *d++ = col;
            }
        }
        b0 = b1;
        b1 = b2;
    }
}

static void line_dhires(mii_video_t *video, uint32_t sw, mii_bank_t *main, mii_bank_t *aux, int line, bool colour, uint8_t *d) {
    uint16_t a = hires_addr(sw, line);
    uint8_t m[40], x40[40];
    mii_bank_read(main, a, m, 40);
    mii_bank_read(aux, a, x40, 40);
    if(!colour) {
        for(int x = 0; x < 40; x++) {
            uint32_t ext = (x40[x] & 0x7f) | ((m[x] & 0x7f) << 7);
            for(int bi = 0; bi < 14; bi++) *d++ = IX(mono) + ((ext >> bi) & 1);
        }
        return;
    }
    uint8_t bits[71] = { 0 };
    for(int x = 0; x < 80; x++) {               /* each byte's 7 bits, the other way round, into the bit buffer */
        uint8_t b = x & 1 ? m[x / 2] : x40[x / 2];
        for(int i = 0; i < 7; i++) {
            int out = 2 + x * 7 + i;
            bits[out / 8] |= ((b >> i) & 1) << (7 - (out % 8));
        }
    }
    for(int i = 0, k = 2; i < 560; i++, k++) {  /* (2 pixels on, as MII has it: a little artifact each side) */
        uint8_t pixel = (get1(bits, i + 3) << (3 - ((k + 3) % 4))) + (get1(bits, i + 2) << (3 - ((k + 2) % 4))) +
                        (get1(bits, i + 1) << (3 - ((k + 1) % 4))) + (get1(bits, i) << (3 - (k % 4)));
        *d++ = IX(dhires) + pixel;
    }
}

/* MII's colours (its tables hold R, G and B in their low three bytes) as the palette's first entries: the colours' places, then LOW on the dimmer ones, and
   MASKED the two mono colours as MII's C_SCANLINE_MASK makes them */
void apple_video_palette(const mii_video_t *video, uint8_t *rgb) {
    const int n = sizeof(video->clut.colors) / sizeof(video->clut.colors[0]);
    for(int i = 0; i < n; i++) {
        uint32_t c = video->clut.colors[i], l = video->clut_low.colors[i];
        rgb[i * 3] = c;
        rgb[i * 3 + 1] = c >> 8;
        rgb[i * 3 + 2] = c >> 16;
        rgb[(LOW + i) * 3] = l;
        rgb[(LOW + i) * 3 + 1] = l >> 8;
        rgb[(LOW + i) * 3 + 2] = l >> 16;
    }
    for(int i = 0; i < 2; i++) {
        uint32_t c = video->clut.mono[i] & 0xc0c0c0;
        rgb[(MASKED + i) * 3] = c;
        rgb[(MASKED + i) * 3 + 1] = c >> 8;
        rgb[(MASKED + i) * 3 + 2] = c >> 16;
    }
}

void apple_video_render(mii_t *mii, uint8_t *frame, int pitch) {
    mii_video_t *video = &mii->video;
    mii_bank_t *main = &mii->bank[MII_BANK_MAIN], *aux = &mii->bank[MII_BANK_AUX_BASE];
    uint32_t sw = mii->sw_state;
    bool text = SWW_GETSTATE(sw, SWTEXT), col80 = SWW_GETSTATE(sw, SW80COL), hires = SWW_GETSTATE(sw, SWHIRES),
         dhires = SWW_GETSTATE(sw, SWDHIRES), mixed = SWW_GETSTATE(sw, SWMIXED);
    if(video->rom == NULL || video->rom->rom == NULL) return;
    for(int line = 0; line < 192; line++, frame += pitch) {
        if(text || (mixed && line >= MIXED_LINE)) line_text(video, sw, main, aux, line, frame);
        else if(hires && col80 && dhires) line_dhires(video, sw, main, aux, line, !video->monochrome, frame);
        else if(hires) line_hires(video, sw, main, line, frame);
        else line_lores(video, sw, main, aux, line, frame);
    }
}
