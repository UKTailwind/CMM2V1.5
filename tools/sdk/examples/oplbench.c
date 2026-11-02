/* oplbench.c: Nuked-OPL3 timed in a loadable program (tools/sdk), against the firmware's copy. MIT License, as
   cmm2img.h; it is built with Src/opl3.c (LGPL-2.1-or-later), so the .app as a whole is LGPL.

     RUN "oplbench.app" [, seconds [, voices]]

   The same sound as PLAY OPL TEST (OPL3 mode, voices two-operator channels keyed on, tremolo and vibrato on), made at
   44.1 kHz in 2048-frame blocks: first by the program's own opl3.c, code and state in the SDRAM's 8 MB, as a ported
   game's synth would run; then by the firmware's (opl_new, opl_generate), its code in flash and its state on the
   MMBasic heap. */
#include <stdio.h>
#include <stdlib.h>
#include "cmm2.h"
#include "opl3.h"

#define RATE 44100
#define BLOCK 2048
static int16_t buf[BLOCK * 2];

typedef void (*write_fn)(void *chip, uint32_t reg, uint32_t value);
static void own_write(void *chip, uint32_t reg, uint32_t value) { OPL3_WriteReg(chip, reg, value); }

static void voices_on(void *c, write_fn w, int voices) {    /* OPL.c's OPLTestVoices */
    static const uint8_t mod[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
    int v;
    w(c, 0x105, 0x01);
    w(c, 0x104, 0x00);
    w(c, 0xBD, 0xC0);
    for(v = 0; v < voices; v++) {
        uint32_t bank = v < 9 ? 0 : 0x100, ch = v % 9, op = mod[ch], f = 0x157 + v * 23;
        w(c, bank + 0x20 + op, 0xE1); w(c, bank + 0x23 + op, 0xE1);
        w(c, bank + 0x40 + op, 0x18); w(c, bank + 0x43 + op, 0x08);
        w(c, bank + 0x60 + op, 0xF4); w(c, bank + 0x63 + op, 0xF4);
        w(c, bank + 0x80 + op, 0x25); w(c, bank + 0x83 + op, 0x25);
        w(c, bank + 0xE0 + op, 0x00); w(c, bank + 0xE3 + op, 0x01);
        w(c, bank + 0xC0 + ch, 0x36);
        w(c, bank + 0xA0 + ch, f & 0xFF);
        w(c, bank + 0xB0 + ch, 0x20 | (4 << 2) | (f >> 8));
    }
}

static void report(const char *what, uint64_t us, int secs) {
    printf("  %s: %lu.%03lu ms, %lu.%lu%% of the CPU\n", what, (unsigned long)(us / 1000), (unsigned long)(us % 1000),
           (unsigned long)(us / (secs * 10000ULL)), (unsigned long)(us / (secs * 1000ULL) % 10));
}

int main(int argc, char **argv) {
    int secs = argc > 1 ? atoi(argv[1]) : 1, voices = argc > 2 ? atoi(argv[2]) : 18;
    uint32_t frames, k;
    uint64_t t, t0;
    if(secs < 1) secs = 1;
    if(voices < 0 || voices > 18) voices = 18;
    printf("OPL3, %d voice%s, %d s of %d Hz stereo\n", voices, voices == 1 ? "" : "s", secs, RATE);

    opl3_chip *own = malloc(sizeof(opl3_chip));
    OPL3_Reset(own, RATE);
    voices_on(own, own_write, voices);
    for(t = 0, frames = secs * RATE; frames; frames -= k) {
        k = frames > BLOCK ? BLOCK : frames;
        t0 = cmm2->micros();
        OPL3_GenerateStream(own, buf, k);
        t += cmm2->micros() - t0;
        if(cmm2->poll()) return 1;
    }
    printf("  (the program's code at %p, its state at %p)\n", (void *)OPL3_GenerateStream, (void *)own);
    report("the program's opl3.c, in the SDRAM", t, secs);
    free(own);

    void *fw = cmm2->opl_new(RATE);
    if(fw == NULL) { printf("opl_new: no room\n"); return 2; }
    voices_on(fw, cmm2->opl_write, voices);
    for(t = 0, frames = secs * RATE; frames; frames -= k) {
        k = frames > BLOCK ? BLOCK : frames;
        t0 = cmm2->micros();
        cmm2->opl_generate(fw, buf, k);
        t += cmm2->micros() - t0;
        if(cmm2->poll()) return 1;
    }
    printf("  (the firmware's code at %p, its state at %p)\n", (void *)cmm2->opl_generate, fw);
    report("the firmware's, in flash", t, secs);
    cmm2->opl_free(fw);
    return 0;
}
