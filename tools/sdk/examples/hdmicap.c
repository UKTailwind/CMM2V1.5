/* hdmicap.c: the bit-for-bit check of a G3's sound through HDMI (docs/G3_HDMI_Audio_Plan.md, step 2). A 44.1 kHz
   16-bit WAV is played by BASIC at PLAY VOLUME v; the frames SAI2's DMA sends are copied from its buffer as
   HdmiAudio.c makes them, and compared with the file's samples as the firmware scales them, (s * mapping[v]) / 2000
   (at 100, the file's own). There is no change to the firmware: the buffer is read where HdmiAudio.c keeps it, the top
   of the D2 SRAM. MIT License, as cmm2img.h.

     RUN "hdmicap.app", file [, volume [, frames [, every]]]   (sound!) volume 30 if not given; frames 65536 (1.5 s),
                                                     compared from half a second into the file; poll() every
                                                     'every' halves (1)

   Each half of the DMA buffer (128 frames) is made again just after the DMA leaves it, and stays as it is for 5.8 ms,
   until the DMA leaves it again; it is copied 100 us after the DMA leaves it. BASIC's WAV is refilled by poll(), called
   after each half by default (every eighth, 23 ms, ended a WAV after 0.6 s: a refill read nothing). A half missed
   would shift what follows, which the comparison finds. Built with --api 12 (basic_cmd). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"
#include "stm32h743xx.h"

#define BUF   ((const volatile uint32_t *)0x30047000)                // HdmiAudio.c's HA_BUF: two halves of HALF frames
#define HALF  128
#define SKIP  22050                                                 // frames of the file before the comparison
#define MATCH 32                                                    // frames that place the capture against the file

static const int mapping[101] = {0,4,11,18,25,33,41,49,57,66,75,84,93,103,113,123,134,145,156,167,179,191,203,216,228,
    241,255,268,282,296,311,325,340,355,371,387,403,419,436,453,470,487,505,523,541,560,578,597,617,636,656,676,697,
    718,738,760,781,803,825,847,870,893,916,940,963,987,1012,1036,1061,1086,1111,1137,1163,1189,1216,1242,1269,1297,
    1324,1352,1380,1408,1437,1466,1495,1525,1554,1584,1615,1645,1676,1707,1739,1770,1802,1834,1867,1900,1933,1966,
    2000};                                                          // (Audio.c's: PLAY VOLUME's levels)

static uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

int main(int argc, char **argv) {
    const char *name = argc > 1 ? argv[1] : "";
    int vol = argc > 2 ? atoi(argv[2]) : 30, frames = argc > 3 ? atoi(argv[3]) : 65536;
    int every = argc > 4 ? atoi(argv[4]) : 1;
    uint8_t h[12];
    uint32_t rate = 0, chans = 0, bits = 0, dataat = 0, datalen = 0;
    char cmd[300], st[256];

    if(!*name || vol < 0 || vol > 100 || frames < 1024 || every < 1) {
        printf("RUN \"hdmicap.app\", file [, volume [, frames [, every]]]\n");
        return 1;
    }
    if(!(DMA1_Stream1->CR & DMA_SxCR_EN) || DMA1_Stream1->M0AR != (uint32_t)BUF) {
        printf("no HDMI sound running: not a G3, or a firmware without HdmiAudio.c\n");
        return 1;
    }
    FILE *f = fopen(name, "rb");                                    // the WAV's format and its samples' place
    if(!f) { printf("%s: cannot open\n", name); return 1; }
    if(fread(h, 1, 12, f) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) { printf("not a WAV\n"); return 1; }
    while(fread(h, 1, 8, f) == 8) {
        uint32_t n = rd32(h + 4);
        if(!memcmp(h, "fmt ", 4)) {
            uint8_t fm[16];
            if(fread(fm, 1, 16, f) != 16) break;
            chans = fm[2] | fm[3] << 8; rate = rd32(fm + 4); bits = fm[14] | fm[15] << 8;
            fseek(f, (long)(n - 16 + (n & 1)), SEEK_CUR);
        } else if(!memcmp(h, "data", 4)) {
            dataat = (uint32_t)ftell(f); datalen = n;
            break;
        } else fseek(f, (long)(n + (n & 1)), SEEK_CUR);
    }
    printf("%s: %lu Hz, %lu bits, %lu channel(s), %lu frames\n", name, (unsigned long)rate, (unsigned long)bits,
           (unsigned long)chans, chans && bits ? (unsigned long)(datalen / (chans * bits / 8)) : 0UL);
    if(rate != 44100 || bits != 16 || chans < 1 || chans > 2 || !dataat) { printf("a 44.1 kHz 16-bit WAV is needed\n"); return 1; }
    uint32_t want = SKIP + (uint32_t)frames + 16384, have = datalen / (chans * 2);   // (room for 100 halves missed)
    if(have < want) { printf("the file is too short: %lu frames, %lu needed\n", (unsigned long)have, (unsigned long)want); return 1; }

    int16_t *exp = malloc(want * 4);                                // the file's frames as the firmware scales them
    uint32_t capn = SKIP + (uint32_t)frames + 8192, n = 0;          // (and the capture: the start's silence too)
    uint32_t *cap = malloc(capn * 4);
    if(!exp || !cap) { printf("no memory\n"); return 1; }
    fseek(f, (long)dataat, SEEK_SET);
    for(uint32_t i = 0; i < want; i++) {
        int16_t s[2];
        if(fread(s, 2, chans, f) != chans) { printf("read error\n"); return 1; }
        if(chans == 1) s[1] = s[0];
        exp[2 * i] = (int16_t)((s[0] * mapping[vol]) / 2000);
        exp[2 * i + 1] = (int16_t)((s[1] * mapping[vol]) / 2000);
    }
    fclose(f);

    snprintf(cmd, sizeof(cmd), "PLAY VOLUME %d,%d", vol, vol);
    if(cmm2->basic_cmd(cmd)) { printf("%s failed\n", cmd); return 1; }
    snprintf(cmd, sizeof(cmd), "PLAY WAV \"%s\"", name);
    if(cmm2->basic_cmd(cmd)) { printf("%s failed\n", cmd); return 1; }
    int region = DMA1_Stream1->NDTR > 2 * HALF, halves = 0;         // (NDTR: half-words left; over 256, the first half)
    while(n < capn) {
        int now = DMA1_Stream1->NDTR > 2 * HALF;
        if(now == region) continue;
        region = now;                                               // the DMA has left a half: made again by now + 100 us
        cmm2->delay_us(100);
        const volatile uint32_t *src = now ? BUF + HALF : BUF;
        for(int i = 0; i < HALF && n < capn; i++) cap[n++] = src[i];
        if(++halves % every == 0 && cmm2->poll()) break;            // (BASIC's WAV refilled; Ctrl-C ends it)
    }
    if(cmm2->basic_eval("MM.INFO(SOUND)", st)) strcpy(st, "?");
    printf("MM.INFO(SOUND) at the end: %s\n", st);
    cmm2->basic_cmd("PLAY STOP");
    printf("%lu frames copied from the DMA buffer\n", (unsigned long)n);

    uint32_t k = SKIP, p, j, bad = 0;                               // a place in the file with sound in it
    while(k < want - MATCH - (uint32_t)frames && abs(exp[2 * k]) < 1000) k++;
    for(p = 0; p + MATCH <= n; p++) {
        for(j = 0; j < MATCH && cap[p + j] == ((uint16_t)exp[2 * (k + j)] | (uint32_t)(uint16_t)exp[2 * (k + j) + 1] << 16); j++) {}
        if(j == MATCH) break;
    }
    if(p + MATCH > n) { printf("the file's frames %lu-%lu are not in the capture: no match at all\n", (unsigned long)k,
                               (unsigned long)(k + MATCH - 1)); return 2; }
    /* then half by half: each half the program copied (capture frames hb * HALF on) is the file's frames from its place,
       which moves on a half for each half the program missed (BASIC's refill in poll() can hold it up for longer than a
       half, 2.9 ms: a comparison frame by frame failed there). A half that matches nowhere within 8 halves of its place
       is counted as different */
    uint32_t hb = (p + HALF - 1) / HALF, same = 0, missed = 0;
    int64_t off = (int64_t)k - (int64_t)p;                          // the file's frame = the capture's + off
    for(; (hb + 1) * HALF <= n && same * HALF < (uint32_t)frames; hb++) {
        uint32_t c = hb * HALF, m;
        int64_t fi = 0;
        if(c + off + HALF > want) break;
        for(m = 0; m <= 8; m++) {
            fi = c + off + (int64_t)m * HALF;
            if(fi + HALF > want) { m = 9; break; }
            for(j = 0; j < HALF && cap[c + j] == ((uint16_t)exp[2 * (fi + j)] | (uint32_t)(uint16_t)exp[2 * (fi + j) + 1] << 16); j++) {}
            if(j == HALF) break;
        }
        if(m <= 8) { same++; missed += m; off += (int64_t)m * HALF; continue; }
        fi = c + off;
        for(j = 0; j < HALF && cap[c + j] == ((uint16_t)exp[2 * (fi + j)] | (uint32_t)(uint16_t)exp[2 * (fi + j) + 1] << 16); j++) {}
        if(bad < 5) printf("capture half %lu, file frame %lu: sent %08lX, the file's %08lX\n", (unsigned long)hb,
                           (unsigned long)(fi + j), (unsigned long)cap[c + j],
                           (unsigned long)((uint16_t)exp[2 * (fi + j)] | (uint32_t)(uint16_t)exp[2 * (fi + j) + 1] << 16));
        bad++;
    }
    printf("file frame %lu is capture frame %lu; at PLAY VOLUME %d, %lu halves of %d frames (%lu frames) the same, %lu "
           "different; %lu halves missed by the program\n", (unsigned long)k, (unsigned long)p, vol, (unsigned long)same,
           HALF, (unsigned long)(same * HALF), (unsigned long)bad, (unsigned long)missed);
    if(!bad && same * HALF >= (uint32_t)frames) printf("bit for bit\n");
    return bad || same * HALF < (uint32_t)frames ? 2 : 0;
}
