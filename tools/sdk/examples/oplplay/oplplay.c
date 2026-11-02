/* oplplay.c: OPL music rendered to a WAV file on the G2, by any of three OPL emulators, timed (tools/sdk).
   MIT License, as cmm2img.h. Built with Nuked-OPL3 (LGPL-2.1+), emu8950 (MIT) and ymfm (BSD-3), so the .app as a
   whole is LGPL.

     RUN "oplplay.app", in out.wav emulator [seconds [opl2]]

   in        a Standard MIDI File (.mid), an IMF (.imf at 560 Hz, .wlf at 700), a DOSBox capture (.dro, versions 0.1
             and 2.0) or a VGM log of an OPL, OPL2 or OPL3 (.vgm, uncompressed)
   emulator  nuked (OPL3), emu8950 (OPL2) or ymfm (OPL3)
   seconds   how much to render (default 40)
   opl2      MIDI on 9 OPL2 voices, as emu8950 always has, rather than 18 OPL3 ones

   MIDI is played as Doom's DMX driver plays it: each General MIDI program (and each drum, channel 10) is an FM patch
   from a GENMIDI bank (A:/opl/GENMIDI.OP2: Freedoom's, BSD-3), a note takes a free OPL channel (or the oldest), and
   velocity, volume, expression, pan, pitch bend and the sustain pedal are followed. The output is 44.1 kHz 16-bit
   stereo; a dual-OPL2 log has two chips, mixed. The time the emulator takes is reported as a share of the CPU. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmm2.h"
#include "opl3.h"
#include "emu8950.h"

#define RATE 44100
#define BLOCK 1024

/*******************************************************************************************
 the emulators: each makes 44.1 kHz stereo; registers 0-0x1FF (0x100 up: an OPL3's second bank)
*******************************************************************************************/
typedef struct {
    const char *name;
    int opl3;
    void *(*create)(void);
    void (*write)(void *c, int reg, int val);
    void (*gen)(void *c, int16_t *buf, int frames);
} emu_t;

static void *nuked_new(void) { opl3_chip *c = malloc(sizeof(opl3_chip)); if(c) OPL3_Reset(c, RATE); return c; }
static void nuked_write(void *c, int reg, int val) { OPL3_WriteRegBuffered(c, reg, val); }
static void nuked_gen(void *c, int16_t *buf, int frames) { OPL3_GenerateStream(c, buf, frames); }

/* a chip that runs at its own rate (clock / 72 or / 288: 49,716 Hz), taken to 44.1 kHz by linear interpolation as
   Nuked's own resampler does */
typedef struct {
    void *chip;
    void (*one)(void *chip, int32_t lr[2]);
    float step, pos, gain;                                  /* gain: to Nuked's level (emu8950 is half, ymfm twice) */
    int32_t prev[2], cur[2];
} rs_t;
static void rs_gen(rs_t *r, int16_t *buf, int frames) {
    int i, k;
    for(i = 0; i < frames; i++) {
        while(r->pos >= 1.0f) {
            r->prev[0] = r->cur[0];
            r->prev[1] = r->cur[1];
            r->one(r->chip, r->cur);
            r->pos -= 1.0f;
        }
        for(k = 0; k < 2; k++) {
            int32_t v = (int32_t)((r->prev[k] + (r->cur[k] - r->prev[k]) * r->pos) * r->gain);
            buf[2 * i + k] = v > 32767 ? 32767 : v < -32768 ? -32768 : v;
        }
        r->pos += r->step;
    }
}

static void emu8950_one(void *chip, int32_t lr[2]) { lr[0] = lr[1] = OPL_calc(chip); }
static void *emu8950_new(void) {
    rs_t *r = calloc(1, sizeof(rs_t));
    r->chip = OPL_new(3579545, 3579545 / 72);           /* (its own rate: no rate converter) */
    OPL_setChipType(r->chip, 2);                        /* YM3812 */
    r->one = emu8950_one;
    r->gain = 2.0f;
    r->step = (3579545.0f / 72.0f) / RATE;
    r->pos = 1.0f;
    return r;
}
static void emu8950_write(void *c, int reg, int val) { if(reg < 0x100) OPL_writeReg(((rs_t *)c)->chip, reg, val); }
static void emu8950_gen(void *c, int16_t *buf, int frames) { rs_gen(c, buf, frames); }

/* ymfm's YMF262, behind C (ymfm_glue.cpp) */
extern void *ymfm_create(void);
extern void ymfm_write(void *chip, int reg, int val);
extern void ymfm_one(void *chip, int32_t lr[2]);
static void *ymfm_new(void) {
    rs_t *r = calloc(1, sizeof(rs_t));
    r->chip = ymfm_create();
    r->one = ymfm_one;
    r->gain = 0.5f;
    r->step = (14318180.0f / 288.0f) / RATE;
    r->pos = 1.0f;
    return r;
}
static void ymfm_wr(void *c, int reg, int val) { ymfm_write(((rs_t *)c)->chip, reg, val); }
static void ymfm_gen(void *c, int16_t *buf, int frames) { rs_gen(c, buf, frames); }

static const emu_t emus[] = {
    {"nuked", 1, nuked_new, nuked_write, nuked_gen},
    {"emu8950", 0, emu8950_new, emu8950_write, emu8950_gen},
    {"ymfm", 1, ymfm_new, ymfm_wr, ymfm_gen},
};

/*******************************************************************************************
 the player: up to two chips, a WAV file, the emulator's time
*******************************************************************************************/
static const emu_t *emu;
static void *chip[2];
static int nchips = 1;
static FILE *wav;
static uint32_t frames_out, limit;
static uint64_t gen_us;
static int peak;                                            /* the largest sample: a check that something sounded */
static int16_t buf[BLOCK * 2], buf2[BLOCK * 2];

static void opl(int c, int reg, int val) { if(c < nchips && chip[c]) emu->write(chip[c], reg, val); }

static void render(uint32_t n) {
    int i, k;
    while(n) {
        k = n > BLOCK ? BLOCK : n;
        uint64_t t0 = cmm2->micros();
        emu->gen(chip[0], buf, k);
        if(nchips == 2) {
            emu->gen(chip[1], buf2, k);
            for(i = 0; i < 2 * k; i++) {
                int32_t v = buf[i] + buf2[i];
                buf[i] = v > 32767 ? 32767 : v < -32768 ? -32768 : v;
            }
        }
        gen_us += cmm2->micros() - t0;
        for(i = 0; i < 2 * k; i++) if(abs(buf[i]) > peak) peak = abs(buf[i]);
        fwrite(buf, 4, k, wav);
        frames_out += k;
        n -= k;
    }
}

static void wav_header(uint32_t frames) {
    uint32_t data = frames * 4;
    unsigned char h[44] = "RIFF....WAVEfmt ";
    #define W32(o, v) (h[o] = (v) & 0xFF, h[o + 1] = ((v) >> 8) & 0xFF, h[o + 2] = ((v) >> 16) & 0xFF, h[o + 3] = ((v) >> 24) & 0xFF)
    W32(4, 36 + data);
    W32(16, 16);
    h[20] = 1; h[21] = 0; h[22] = 2; h[23] = 0;         /* PCM, stereo */
    W32(24, RATE);
    W32(28, RATE * 4);
    h[32] = 4; h[33] = 0; h[34] = 16; h[35] = 0;
    memcpy(h + 36, "data", 4);
    W32(40, data);
    fseek(wav, 0, SEEK_SET);
    fwrite(h, 1, 44, wav);
}

static unsigned char *file;
static uint32_t flen;
static uint16_t rd16(uint32_t o) { return file[o] | file[o + 1] << 8; }
static uint32_t rd32(uint32_t o) { return rd16(o) | (uint32_t)rd16(o + 2) << 16; }
static uint32_t be32(const unsigned char *p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

/*******************************************************************************************
 IMF, DRO and VGM: register logs. Each step does the writes due now and returns the delay to the next, in samples, or
 a negative number at the end.
*******************************************************************************************/
static uint32_t pos, endpos;
static double tick;                                         /* samples per IMF tick */
static int drover, drochip, dro_short, dro_long, dro_map[128];

static double imf_step(void) {
    while(pos + 4 <= endpos) {
        int reg = file[pos], val = file[pos + 1], d = rd16(pos + 2);
        pos += 4;
        opl(0, reg, val);
        if(d) return d * tick;
    }
    return -1;
}

static double dro_step(void) {
    if(drover == 2) {
        while(pos + 2 <= endpos) {
            int code = file[pos], val = file[pos + 1];
            pos += 2;
            if(code == dro_short) return (val + 1) * 44.1;
            if(code == dro_long) return ((val + 1) << 8) * 44.1;
            opl(nchips == 2 && (code & 0x80) ? 1 : 0, dro_map[code & 0x7F] | (nchips == 1 && (code & 0x80) ? 0x100 : 0), val);
        }
        return -1;
    }
    while(pos < endpos) {
        int code = file[pos++];
        switch(code) {
        case 0: return (file[pos++] + 1) * 44.1;
        case 1: pos += 2; return (rd16(pos - 2) + 1) * 44.1;
        case 2: drochip = 0; break;
        case 3: drochip = 1; break;
        case 4: code = file[pos++];                     /* (an escape: a register 0-4 follows) */
            /* fall through */
        default:
            opl(nchips == 2 ? drochip : 0, code | (nchips == 1 && drochip ? 0x100 : 0), file[pos]);
            pos++;
        }
    }
    return -1;
}

static int vgm_two;                                         /* a second YM3812 (0xAA writes) */
static double vgm_step(void) {
    while(pos < endpos) {
        int c = file[pos];
        if(c == 0x5A || c == 0x5B || c == 0x5C || c == 0x5E) { opl(0, file[pos + 1], file[pos + 2]); pos += 3; }
        else if(c == 0x5F) { opl(0, 0x100 | file[pos + 1], file[pos + 2]); pos += 3; }
        else if(c == 0xAA || c == 0xAB) { opl(1, file[pos + 1], file[pos + 2]); pos += 3; }
        else if(c == 0x61) { pos += 3; return rd16(pos - 2); }
        else if(c == 0x62) { pos++; return 735; }
        else if(c == 0x63) { pos++; return 882; }
        else if(c >= 0x70 && c <= 0x7F) { pos++; return (c & 15) + 1; }
        else if(c >= 0x80 && c <= 0x8F) { pos++; if(c & 15) return c & 15; }
        else if(c == 0x66) return -1;
        else if(c == 0x67) pos += 7 + rd32(pos + 3);
        else if((c >= 0x30 && c <= 0x3F) || c == 0x4F || c == 0x50) pos += 2;
        else if((c >= 0x40 && c <= 0x5F) || (c >= 0xA0 && c <= 0xBF)) pos += 3;
        else if(c >= 0xC0 && c <= 0xDF) pos += 4;
        else if(c >= 0xE0) pos += 5;
        else pos++;
    }
    return -1;
}

/*******************************************************************************************
 MIDI: a Standard MIDI File (types 0 and 1) on an OPL with a GENMIDI bank
*******************************************************************************************/
typedef struct __attribute__((packed)) { uint8_t tremolo, attack, sustain, waveform, scale, level; } gm_op;
typedef struct __attribute__((packed)) { gm_op mod; uint8_t feedback; gm_op car; uint8_t unused; int16_t offset; } gm_voice;
typedef struct __attribute__((packed)) { uint16_t flags; uint8_t fine; uint8_t fixed; gm_voice v[2]; } gm_instr;
static gm_instr *bank;                                      /* 175: 128 programs, then drums 35-81 */

typedef struct { int program, volume, expression, pan, bend, sustain; } mchan_t;
typedef struct {
    int on;                                                 /* key down (or held by the sustain pedal) */
    int held;                                               /* released while the pedal was down */
    int ch, note, vel, sub;                                 /* MIDI channel, note, velocity, the instrument's voice 0/1 */
    const gm_instr *ins;
    uint32_t age;
    int car_tl, mod_tl, additive;
} voice_t;
static mchan_t mch[16];
static voice_t vo[18];
static int nvoices, opl3mode;
static uint32_t agecount;
static const uint8_t opofs[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};

typedef struct { const unsigned char *p, *end; uint32_t next; int running; int done; } mtrack_t;
static mtrack_t trk[64];
static int ntracks, division;
static double us_per_tick, now_ticks_samples;              /* samples per tick */
static uint32_t now_tick;

static uint32_t varlen(const unsigned char **p) {
    uint32_t v = 0;
    int i;
    for(i = 0; i < 4; i++) { v = v << 7 | (**p & 0x7F); if(!(*(*p)++ & 0x80)) break; }
    return v;
}

static int voice_base(int v) { return v < 9 ? 0 : 0x100; }
static int voice_op(int v) { return opofs[v % 9]; }

/* attenuation for velocity, volume and expression, in OPL level steps of 0.75 dB */
static int atten(const voice_t *x) {
    float g = (x->vel / 127.0f) * (mch[x->ch].volume / 127.0f) * (mch[x->ch].expression / 127.0f);
    if(g <= 0.0005f) return 63;
    int a = (int)(-20.0f * log10f(g) / 0.75f + 0.5f);
    return a > 63 ? 63 : a;
}

static void voice_levels(int v) {
    voice_t *x = &vo[v];
    const gm_voice *g = &x->ins->v[x->sub];
    int a = atten(x), b = voice_base(v), op = voice_op(v);
    int car = (g->car.level & 0x3F) + a, mod = g->mod.level & 0x3F;
    if(car > 63) car = 63;
    if(x->additive) { mod += a; if(mod > 63) mod = 63; }
    opl(0, b + 0x43 + op, (g->car.scale & 0xC0) | car);
    opl(0, b + 0x40 + op, (g->mod.scale & 0xC0) | mod);
}

static void voice_pitch(int v, int key) {
    voice_t *x = &vo[v];
    const gm_voice *g = &x->ins->v[x->sub];
    int note = (x->ins->flags & 1) ? x->ins->fixed : x->note, block;
    float semis = note + g->offset - 69 + (mch[x->ch].bend - 8192) * (2.0f / 8192.0f);
    if(x->sub) semis += ((x->ins->fine / 2) - 64) / 32.0f;
    float f = 440.0f * powf(2.0f, semis / 12.0f), fn = 0;
    for(block = 0; block < 8; block++) { fn = f * (float)(1 << (20 - block)) / 49716.0f; if(fn < 1024.0f) break; }
    if(block > 7) { block = 7; fn = 1023; }
    int fnum = (int)(fn + 0.5f);
    if(fnum > 1023) fnum = 1023;
    opl(0, voice_base(v) + 0xA0 + v % 9, fnum & 0xFF);
    opl(0, voice_base(v) + 0xB0 + v % 9, (key ? 0x20 : 0) | block << 2 | fnum >> 8);
}

static void voice_off(int v) {
    vo[v].on = vo[v].held = 0;
    voice_pitch(v, 0);
}

static int voice_find(void) {
    int v, best = -1;
    uint32_t oldest = 0xFFFFFFFF;
    for(v = 0; v < nvoices; v++)
        if(!vo[v].on && vo[v].age < oldest) { oldest = vo[v].age; best = v; }   /* free the longest */
    if(best >= 0) return best;
    for(v = 0; v < nvoices; v++)
        if(vo[v].age < oldest) { oldest = vo[v].age; best = v; }               /* or the oldest note */
    voice_off(best);
    return best;
}

static void note_on(int ch, int note, int vel) {
    const gm_instr *ins;
    int sub, nsub, v;
    if(ch == 9) {
        if(note < 35 || note > 81) return;
        ins = &bank[128 + note - 35];
    } else ins = &bank[mch[ch].program];
    nsub = (ins->flags & 4) ? 2 : 1;
    for(sub = 0; sub < nsub; sub++) {
        const gm_voice *g = &ins->v[sub];
        v = voice_find();
        voice_t *x = &vo[v];
        int b = voice_base(v), op = voice_op(v), pan = 0x30;
        x->on = 1; x->held = 0; x->ch = ch; x->note = note; x->vel = vel; x->sub = sub; x->ins = ins;
        x->age = ++agecount;
        x->additive = g->feedback & 1;
        if(opl3mode) pan = mch[ch].pan < 48 ? 0x10 : mch[ch].pan > 80 ? 0x20 : 0x30;
        opl(0, b + 0x20 + op, g->mod.tremolo);
        opl(0, b + 0x60 + op, g->mod.attack);
        opl(0, b + 0x80 + op, g->mod.sustain);
        opl(0, b + 0xE0 + op, g->mod.waveform);
        opl(0, b + 0x23 + op, g->car.tremolo);
        opl(0, b + 0x63 + op, g->car.attack);
        opl(0, b + 0x83 + op, g->car.sustain);
        opl(0, b + 0xE3 + op, g->car.waveform);
        opl(0, b + 0xC0 + v % 9, (opl3mode ? pan : 0) | (g->feedback & 0x0F));
        voice_levels(v);
        voice_pitch(v, 1);
    }
}

static void note_off(int ch, int note) {
    int v;
    for(v = 0; v < nvoices; v++)
        if(vo[v].on && !vo[v].held && vo[v].ch == ch && vo[v].note == note) {
            if(mch[ch].sustain) vo[v].held = 1;
            else voice_off(v);
        }
}

static void midi_event(int st, int d1, int d2) {
    int ch = st & 15, v;
    switch(st & 0xF0) {
    case 0x80: note_off(ch, d1); break;
    case 0x90: if(d2) note_on(ch, d1, d2); else note_off(ch, d1); break;
    case 0xB0:
        switch(d1) {
        case 7: mch[ch].volume = d2; goto levels;
        case 11: mch[ch].expression = d2; goto levels;
        case 10: mch[ch].pan = d2; break;
        case 64:
            mch[ch].sustain = d2 >= 64;
            if(!mch[ch].sustain)
                for(v = 0; v < nvoices; v++) if(vo[v].on && vo[v].held && vo[v].ch == ch) voice_off(v);
            break;
        case 120: case 123:
            for(v = 0; v < nvoices; v++) if(vo[v].on && vo[v].ch == ch) voice_off(v);
            break;
        case 121: mch[ch].volume = 100; mch[ch].expression = 127; mch[ch].bend = 8192; mch[ch].sustain = 0; goto levels;
        }
        break;
    levels:
        for(v = 0; v < nvoices; v++) if(vo[v].on && vo[v].ch == ch) voice_levels(v);
        break;
    case 0xC0: mch[ch].program = d1; break;
    case 0xE0:
        mch[ch].bend = d1 | d2 << 7;
        for(v = 0; v < nvoices; v++) if(vo[v].on && vo[v].ch == ch) voice_pitch(v, 1);
        break;
    }
}

static void midi_start(void) {
    int i, v;
    const unsigned char *p = file + 8 + be32(file + 4);
    division = file[12] << 8 | file[13];
    ntracks = 0;
    while(p + 8 <= file + flen && ntracks < 64) {
        uint32_t n = be32(p + 4);
        if(memcmp(p, "MTrk", 4) == 0) {
            trk[ntracks].p = p + 8;
            trk[ntracks].end = p + 8 + n > file + flen ? file + flen : p + 8 + n;
            trk[ntracks].next = varlen(&trk[ntracks].p);
            trk[ntracks].done = 0;
            ntracks++;
        }
        p += 8 + n;
    }
    for(i = 0; i < 16; i++) { mch[i].program = 0; mch[i].volume = 100; mch[i].expression = 127; mch[i].pan = 64; mch[i].bend = 8192; }
    nvoices = opl3mode ? 18 : 9;
    for(v = 0; v < 18; v++) memset(&vo[v], 0, sizeof(vo[v]));
    opl(0, 0x01, 0x20);                                     /* waveform select */
    if(opl3mode) { opl(0, 0x105, 0x01); opl(0, 0x104, 0x00); }
    opl(0, 0xBD, 0x00);
    us_per_tick = 500000.0 / division;
    now_tick = 0;
}

static double midi_step(void) {
    for(;;) {
        int i, t = -1;
        for(i = 0; i < ntracks; i++)
            if(!trk[i].done && (t < 0 || trk[i].next < trk[t].next)) t = i;
        if(t < 0) return -1;
        if(trk[t].next > now_tick) {
            double d = (trk[t].next - now_tick) * us_per_tick * RATE / 1e6;
            now_tick = trk[t].next;
            return d;
        }
        mtrack_t *k = &trk[t];
        if(k->p >= k->end) { k->done = 1; continue; }
        int st = *k->p;
        if(st & 0x80) k->p++; else st = k->running;
        if(st == 0xFF) {
            int type = *k->p++;
            uint32_t n = varlen(&k->p);
            if(type == 0x51 && n == 3) us_per_tick = (double)(k->p[0] << 16 | k->p[1] << 8 | k->p[2]) / division;
            if(type == 0x2F) k->done = 1;
            k->p += n;
        } else if(st == 0xF0 || st == 0xF7) {
            k->p += varlen(&k->p);
        } else {
            int d1 = *k->p++, d2 = 0;
            if((st & 0xF0) != 0xC0 && (st & 0xF0) != 0xD0) d2 = *k->p++;
            k->running = st;
            midi_event(st, d1, d2);
        }
        if(k->p < k->end && !k->done) k->next += varlen(&k->p);
        else k->done = 1;
    }
}

/*******************************************************************************************
 main
*******************************************************************************************/
static int load(const char *name, unsigned char **data, uint32_t *len) {
    FILE *f = fopen(name, "rb");
    if(!f) return 0;
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    fseek(f, 0, SEEK_SET);
    *data = malloc(*len + 16);
    if(!*data || fread(*data, 1, *len, f) != *len) { fclose(f); return 0; }
    memset(*data + *len, 0, 16);
    fclose(f);
    return 1;
}

int main(int argc, char **argv) {
    double (*step)(void) = NULL, now = 0, d;
    const char *kind = "?";
    int i, secs = argc > 4 ? atoi(argv[4]) : 40;
    if(argc < 4) { printf("RUN \"oplplay.app\", in out.wav nuked|emu8950|ymfm [seconds [opl2]]\n"); return 1; }
    for(i = 0, emu = NULL; i < 3; i++) if(strcmp(argv[3], emus[i].name) == 0) emu = &emus[i];
    if(!emu) { printf("emulator: nuked, emu8950 or ymfm\n"); return 1; }
    if(!load(argv[1], &file, &flen)) { printf("cannot read %s\n", argv[1]); return 1; }
    limit = (secs < 1 ? 1 : secs) * RATE;
    const char *ext = strrchr(argv[1], '.');
    ext = ext ? ext + 1 : "";

    if(memcmp(file, "MThd", 4) == 0) {
        unsigned char *b;
        uint32_t bl;
        if(!load("A:/opl/GENMIDI.OP2", &b, &bl) || bl < 8 + 175 * 36 || memcmp(b, "#OPL_II#", 8)) { printf("no A:/opl/GENMIDI.OP2\n"); return 1; }
        bank = (gm_instr *)(b + 8);
        opl3mode = emu->opl3 && !(argc > 5 && strcmp(argv[5], "opl2") == 0);
        step = midi_step;                                   /* (midi_start writes registers: once the chip is made) */
        kind = opl3mode ? "MIDI, 18 OPL3 voices" : "MIDI, 9 OPL2 voices";
    } else if(memcmp(file, "DBRAWOPL", 8) == 0) {
        drover = rd16(8);
        if(drover == 2) {
            int hw = file[20], cml = file[25];
            dro_short = file[23]; dro_long = file[24];
            for(i = 0; i < cml && i < 128; i++) dro_map[i] = file[26 + i];
            pos = 26 + cml;
            endpos = pos + rd32(12) * 2;
            nchips = hw == 1 ? 2 : 1;
        } else {
            nchips = file[20] == 1 ? 2 : 1;                 /* 0 OPL2, 1 dual OPL2, 2 OPL3 */
            pos = (file[21] | file[22] | file[23]) ? 21 : 24;   /* (a 1-byte or a 4-byte hardware field) */
            endpos = pos + rd32(16);
        }
        if(endpos > flen) endpos = flen;
        step = dro_step;
        kind = nchips == 2 ? "DRO, two OPL2s" : "DRO";
    } else if(memcmp(file, "Vgm ", 4) == 0) {
        uint32_t ver = rd32(8), off = (ver >= 0x150 && rd32(0x34)) ? 0x34 + rd32(0x34) : 0x40;
        vgm_two = ver >= 0x151 && (rd32(0x50) & 0x40000000);
        nchips = vgm_two ? 2 : 1;
        pos = off;
        endpos = flen;
        step = vgm_step;
        kind = vgm_two ? "VGM, two OPL2s" : "VGM";
    } else if(strcasecmp(ext, "imf") == 0 || strcasecmp(ext, "wlf") == 0) {
        uint32_t n0 = rd16(0);
        tick = (double)RATE / (strcasecmp(ext, "wlf") == 0 ? 700 : 560);
        if(n0 && n0 + 2 <= flen && n0 % 4 == 0) { pos = 2; endpos = 2 + n0; } else { pos = 0; endpos = flen; }
        step = imf_step;
        kind = "IMF";
    } else { printf("%s: not a format this plays\n", argv[1]); return 1; }

    for(i = 0; i < nchips; i++) chip[i] = emu->create();
    if(!chip[0] || (nchips == 2 && !chip[1])) { printf("no room for the emulator\n"); return 1; }
    if(step == midi_step) midi_start();
    if(!(wav = fopen(argv[2], "wb"))) { printf("cannot write %s\n", argv[2]); return 1; }
    fwrite(buf, 1, 44, wav);                                /* (the header, written at the end) */
    printf("%s: %s, by %s, up to %d s\n", argv[1], kind, emu->name, secs);
    while(frames_out < limit) {
        if((d = step()) < 0) {                              /* the end: two seconds for the releases */
            uint32_t tail = limit - frames_out < 2 * RATE ? limit - frames_out : 2 * RATE;
            render(tail);
            break;
        }
        now += d;
        uint32_t target = now > limit ? limit : (uint32_t)now;
        if(target > frames_out) render(target - frames_out);
        if(cmm2->poll()) { printf("stopped\n"); break; }
    }
    wav_header(frames_out);
    fclose(wav);
    printf("  %.1f s of audio, peak %d; the emulator took %lu ms: %.1f%% of the CPU\n", frames_out / (double)RATE, peak,
           (unsigned long)(gen_us / 1000), gen_us / (frames_out / (double)RATE) / 1e4);
    return 0;
}
