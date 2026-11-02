/***********************************************************************************************************************
PLAY OPL and PLAY MIDI (G2 only, 2 Nov 2026: Peter): an OPL3 (Yamaha YMF262) FM synth in the firmware, played as MOD
is, and what plays on it.

  PLAY OPL                                      the synth on for register writes: silent until written
  PLAY OPL file$ [, interrupt]                  an OPL music file: IMF (.imf at 560 Hz, .wlf at 700), a DOSBox capture
                                                (.dro, versions 0.1 and 2.0) or a VGM log of an OPL, OPL2 (one or two)
                                                or OPL3 (.vgm; not compressed .vgz); the SUB runs when it ends
  PLAY OPL WRITE reg, value [, reg, value ...]  register writes, reg 0 to &H1FF (&H100 up: the OPL3's second bank)
  PLAY OPL RESET                                every register back to its reset value, which silences it
  PLAY OPL TEST [seconds [, voices]]            the synth timed: seconds (1-60, default 1) of 44.1 kHz stereo made but
                                                not played, with voices (0-18, default 18) sounding
  PLAY MIDI                                     MIDI in real time (the PicoMite's, which needs a VS1053): the synth on
                                                as a General MIDI instrument, silent until notes come
  PLAY NOTE ON channel, note, velocity          channel 0-15 (9: the drums), note and velocity 0-127
  PLAY NOTE OFF channel, note [, velocity]
  PLAY MIDI CMD cmd, data1 [, data2]            a MIDI channel message: &H80-&HEF (program change &HC0, controller
                                                &HB0, pitch bend &HE0...)
  PLAY MIDI TEST [n]                            the tune kept in flash (Freedoom's FreeDM title, 11 s); n (1-3) as on
                                                the PicoMite, all three the same tune (Peter, 2 Nov)
  PLAY MIDIFILE file$ [, interrupt]             a Standard MIDI File (types 0 and 1); the SUB runs when it ends
  PLAY STOP, PAUSE and RESUME as for any sound; MM.INFO(SOUND) is "OPL" or "MIDI" ("PAUSED OPL", "PAUSED MIDI")

MIDI is played as Doom's DMX driver plays it on an OPL: each General MIDI program, and each drum of channel 10, is an
FM patch of the GENMIDI bank in flash (OPLData.c: Freedoom's, BSD-3; Peter chose flash over a file); a note takes a
free voice of the 18 (OPL3 mode), or the oldest; velocity, volume (7), expression (11), pan (10), the sustain pedal
(64), pitch bend (2 semitones) and the all-notes-off controllers are followed.

The synth is ymfm's YMF262 (OPLChip.cpp, Src/ymfm: BSD-3), which Peter chose over Nuked-OPL3 on hearing both. Its
buffers are MOD's (sbuff1 and sbuff2, WAV_BUFFER_SIZE / 4 frames of 16-bit stereo each), refilled by checkWAVinput
between statements (OPLRefill) and played by the MOD branch of the sample interrupt. A file's events are timed to the
sample within each buffer; PLAY OPL WRITE, PLAY NOTE and PLAY MIDI CMD are heard at the next buffer (46-93 ms later). A
file played to its end, with two seconds for the releases, finishes as a module played once does (audio_checks). The
whole file is read into the heap first. This file, OPLData.c and the synth are in bank 2 (the linker script's .bank2).
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "OPLChip.h"
#include "Loader.h"

#define PSpeedDiv (PeripheralBusSpeed)/2                            // (Audio.c's: the audio timer's input clock)
#define OPL_RATE 44100
#define OPL_TEST_FRAMES 2048
#define OPL_MAXTRK 64                                               // the tracks of a MIDI file that are played
extern TIM_HandleTypeDef htim4;
extern volatile int swingbuf, nextbuf, ppos, wav_filesize;
extern uint16_t *ibuff1, *ibuff2;
extern int nchannels;
extern void ConfigSoundOutputs(void);
extern void i2convert(int16_t *ibuff, int16_t *sbuff, int count);
extern char WAVfilename[FF_MAX_LFN];                                // the file playing, for MM.INFO(TRACK) (Audio.c)
extern const unsigned char OPLGenMidi[175 * 36];                    // OPLData.c
extern const unsigned char OPLTestTune[];
extern const unsigned int OPLTestTuneSize;

enum { OM_RAW, OM_LOG, OM_LIVE, OM_SONG };                          // register writes, an OPL file, MIDI live, a MIDI file
enum { LF_IMF, LF_DRO, LF_VGM };

typedef struct { const unsigned char *p, *end; uint32_t next; int running, done; } mtrack_t;
typedef struct { int program, volume, expression, pan, bend, sustain; } mchan_t;
typedef struct {
    int on, held;                                                   // key down; released while the pedal was down
    int ch, note, vel, sub, additive;                               // MIDI channel, note, velocity, the patch's voice 0/1
    const unsigned char *ins;                                       // the GENMIDI instrument (36 bytes)
    uint32_t age;
} voice_t;
typedef struct {                                                    // what plays: on the heap while the synth is on
    mchan_t ch[16];
    voice_t vo[18];
    uint32_t agecount;
    mtrack_t trk[OPL_MAXTRK];
    int ntrk, division;
    double us_per_tick;
    uint32_t now_tick;
    int fmt;                                                        // an OPL file's: LF_...
    uint32_t pos, endpos;
    double unit;                                                    // samples per IMF tick, DRO millisecond, VGM sample
    int drover, drochip, dro_short, dro_long;
    uint8_t dro_map[128];
} ostate_t;

static void *chip[2];                                               // the synth; [1]: a dual-OPL2 file's second chip
static int nchips;
static int16_t *mixbuf;                                             // the second chip's buffer, to mix
static uint32_t oplrate;                                            // the rate the audio timer gives
static int omode;
static ostate_t *os;
static unsigned char *song;                                         // the file playing: on the heap, or OPLTestTune
static uint32_t slen;
static int songheap;
static double due;                                                  // samples to the next event
static int seqdone;                                                 // the file's last event done
static uint32_t tail;                                               // samples of releases still to make after it
static const unsigned char *genmidi = OPLGenMidi;                   // the bank: the flash's, or a program's (OPLStreamPlay)
static int sactive, sloop, svol = 127, spaused;                     // a program's music (OPLStreamPlay)
static int song_tracks(void);

static void ow(int c, int reg, int val) { if(c < nchips && chip[c]) OPLChipWrite(chip[c], reg, val); }
static uint32_t opl_rate(void) { return PSpeedDiv / (PSpeedDiv / OPL_RATE); }   // the audio timer's: the chip's pitch
static uint16_t rd16(uint32_t o) { return song[o] | song[o + 1] << 8; }
static uint32_t rd32(uint32_t o) { return rd16(o) | (uint32_t)rd16(o + 2) << 16; }
static uint32_t be32(const unsigned char *p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }

/*******************************************************************************************
 OPL files: each step does the writes due now and returns the samples to the next, or -1 at the end
*******************************************************************************************/
static double log_step(void) {
    ostate_t *s = os;
    if(s->fmt == LF_IMF) {
        while(s->pos + 4 <= s->endpos) {
            int reg = song[s->pos], val = song[s->pos + 1], d = rd16(s->pos + 2);
            s->pos += 4;
            ow(0, reg, val);
            if(d) return d * s->unit;
        }
        return -1;
    }
    if(s->fmt == LF_DRO && s->drover == 2) {
        while(s->pos + 2 <= s->endpos) {
            int code = song[s->pos], val = song[s->pos + 1];
            s->pos += 2;
            if(code == s->dro_short) return (val + 1) * s->unit;
            if(code == s->dro_long) return ((val + 1) << 8) * s->unit;
            ow(nchips == 2 && (code & 0x80) ? 1 : 0, s->dro_map[code & 0x7F] | (nchips == 1 && (code & 0x80) ? 0x100 : 0), val);
        }
        return -1;
    }
    if(s->fmt == LF_DRO) {
        while(s->pos < s->endpos) {
            int code = song[s->pos++];
            switch(code) {
            case 0: return (song[s->pos++] + 1) * s->unit;
            case 1: s->pos += 2; return (rd16(s->pos - 2) + 1) * s->unit;
            case 2: s->drochip = 0; break;
            case 3: s->drochip = 1; break;
            case 4: code = song[s->pos++];                          // (an escape: a register 0-4 follows)
                /* fall through */
            default:
                ow(nchips == 2 ? s->drochip : 0, code | (nchips == 1 && s->drochip ? 0x100 : 0), song[s->pos]);
                s->pos++;
            }
        }
        return -1;
    }
    while(s->pos < s->endpos) {                                     // VGM
        int c = song[s->pos];
        if(c == 0x5A || c == 0x5B || c == 0x5C || c == 0x5E) { ow(0, song[s->pos + 1], song[s->pos + 2]); s->pos += 3; }
        else if(c == 0x5F) { ow(0, 0x100 | song[s->pos + 1], song[s->pos + 2]); s->pos += 3; }
        else if(c == 0xAA || c == 0xAB) { ow(1, song[s->pos + 1], song[s->pos + 2]); s->pos += 3; }
        else if(c == 0x61) { s->pos += 3; return rd16(s->pos - 2) * s->unit; }
        else if(c == 0x62) { s->pos++; return 735 * s->unit; }
        else if(c == 0x63) { s->pos++; return 882 * s->unit; }
        else if(c >= 0x70 && c <= 0x7F) { s->pos++; return ((c & 15) + 1) * s->unit; }
        else if(c >= 0x80 && c <= 0x8F) { s->pos++; if(c & 15) return (c & 15) * s->unit; }
        else if(c == 0x66) return -1;
        else if(c == 0x67) s->pos += 7 + rd32(s->pos + 3);
        else if((c >= 0x30 && c <= 0x3F) || c == 0x4F || c == 0x50) s->pos += 2;
        else if((c >= 0x40 && c <= 0x5F) || (c >= 0xA0 && c <= 0xBF)) s->pos += 3;
        else if(c >= 0xC0 && c <= 0xDF) s->pos += 4;
        else if(c >= 0xE0) s->pos += 5;
        else s->pos++;
    }
    return -1;
}

/*******************************************************************************************
 MIDI on the OPL3 with the GENMIDI bank: an instrument is 2 flag bytes, fine tuning, fixed note, then two voices of 16
 bytes (modulator: tremolo, attack, sustain, waveform, scale, level; feedback; carrier the same; unused; note offset)
*******************************************************************************************/
static const uint8_t opofs[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
#define GM_FLAGS(i)      ((i)[0] | (i)[1] << 8)
#define GM_FINE(i)       ((i)[2])
#define GM_FIXED(i)      ((i)[3])
#define GM_V(i, v)       ((i) + 4 + (v) * 16)                       // a voice: mod at +0, feedback +6, car +7, offset +14
#define GM_OFFSET(g)     ((int16_t)((g)[14] | (g)[15] << 8))

static int vbase(int v) { return v < 9 ? 0 : 0x100; }

// attenuation for velocity, volume and expression, in OPL level steps of 0.75 dB
static int atten(const voice_t *x) {
    float g = (x->vel / 127.0f) * (os->ch[x->ch].volume / 127.0f) * (os->ch[x->ch].expression / 127.0f);
    if(g <= 0.0005f) return 63;
    int a = (int)(-20.0f * log10f(g) / 0.75f + 0.5f);
    return a > 63 ? 63 : a;
}

static void voice_levels(int v) {
    voice_t *x = &os->vo[v];
    const unsigned char *g = GM_V(x->ins, x->sub);
    int a = atten(x), b = vbase(v), op = opofs[v % 9];
    int car = (g[7 + 5] & 0x3F) + a, mod = g[5] & 0x3F;
    if(car > 63) car = 63;
    if(x->additive) { mod += a; if(mod > 63) mod = 63; }
    ow(0, b + 0x43 + op, (g[7 + 4] & 0xC0) | car);
    ow(0, b + 0x40 + op, (g[4] & 0xC0) | mod);
}

static void voice_pitch(int v, int key) {
    voice_t *x = &os->vo[v];
    const unsigned char *g = GM_V(x->ins, x->sub);
    int note = (GM_FLAGS(x->ins) & 1) ? GM_FIXED(x->ins) : x->note, block, fnum;
    float semis = note + GM_OFFSET(g) - 69 + (os->ch[x->ch].bend - 8192) * (2.0f / 8192.0f), f, fn = 0;
    if(x->sub) semis += ((GM_FINE(x->ins) / 2) - 64) / 32.0f;
    f = 440.0f * powf(2.0f, semis / 12.0f);
    for(block = 0; block < 8; block++) { fn = f * (float)(1 << (20 - block)) / 49716.0f; if(fn < 1024.0f) break; }
    if(block > 7) { block = 7; fn = 1023; }
    fnum = (int)(fn + 0.5f);
    if(fnum > 1023) fnum = 1023;
    ow(0, vbase(v) + 0xA0 + v % 9, fnum & 0xFF);
    ow(0, vbase(v) + 0xB0 + v % 9, (key ? 0x20 : 0) | block << 2 | fnum >> 8);
}

static void voice_off(int v) {
    os->vo[v].on = os->vo[v].held = 0;
    voice_pitch(v, 0);
}

static int voice_find(void) {
    int v, best = -1;
    uint32_t oldest = 0xFFFFFFFF;
    for(v = 0; v < 18; v++)
        if(!os->vo[v].on && os->vo[v].age < oldest) { oldest = os->vo[v].age; best = v; }   // the longest free
    if(best >= 0) return best;
    for(v = 0; v < 18; v++)
        if(os->vo[v].age < oldest) { oldest = os->vo[v].age; best = v; }                    // or the oldest note
    voice_off(best);
    return best;
}

static void note_on(int ch, int note, int vel) {
    const unsigned char *ins;
    int sub, nsub, v;
    if(ch == 9) {
        if(note < 35 || note > 81) return;
        ins = genmidi + (128 + note - 35) * 36;
    } else ins = genmidi + os->ch[ch].program * 36;
    nsub = (GM_FLAGS(ins) & 4) ? 2 : 1;
    for(sub = 0; sub < nsub; sub++) {
        const unsigned char *g = GM_V(ins, sub);
        voice_t *x;
        int b, op, pan;
        v = voice_find();
        x = &os->vo[v];
        b = vbase(v);
        op = opofs[v % 9];
        pan = os->ch[ch].pan < 48 ? 0x10 : os->ch[ch].pan > 80 ? 0x20 : 0x30;
        x->on = 1; x->held = 0; x->ch = ch; x->note = note; x->vel = vel; x->sub = sub; x->ins = ins;
        x->age = ++os->agecount;
        x->additive = g[6] & 1;
        ow(0, b + 0x20 + op, g[0]);
        ow(0, b + 0x60 + op, g[1]);
        ow(0, b + 0x80 + op, g[2]);
        ow(0, b + 0xE0 + op, g[3]);
        ow(0, b + 0x23 + op, g[7]);
        ow(0, b + 0x63 + op, g[8]);
        ow(0, b + 0x83 + op, g[9]);
        ow(0, b + 0xE3 + op, g[10]);
        ow(0, b + 0xC0 + v % 9, pan | (g[6] & 0x0F));
        voice_levels(v);
        voice_pitch(v, 1);
    }
}

static void note_off(int ch, int note) {
    int v;
    for(v = 0; v < 18; v++)
        if(os->vo[v].on && !os->vo[v].held && os->vo[v].ch == ch && os->vo[v].note == note) {
            if(os->ch[ch].sustain) os->vo[v].held = 1;
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
        case 7: os->ch[ch].volume = d2; goto levels;
        case 11: os->ch[ch].expression = d2; goto levels;
        case 10: os->ch[ch].pan = d2; break;
        case 64:
            os->ch[ch].sustain = d2 >= 64;
            if(!os->ch[ch].sustain)
                for(v = 0; v < 18; v++) if(os->vo[v].on && os->vo[v].held && os->vo[v].ch == ch) voice_off(v);
            break;
        case 120: case 123:
            for(v = 0; v < 18; v++) if(os->vo[v].on && os->vo[v].ch == ch) voice_off(v);
            break;
        case 121:
            os->ch[ch].volume = 100; os->ch[ch].expression = 127; os->ch[ch].bend = 8192; os->ch[ch].sustain = 0;
            goto levels;
        }
        break;
    levels:
        for(v = 0; v < 18; v++) if(os->vo[v].on && os->vo[v].ch == ch) voice_levels(v);
        break;
    case 0xC0: os->ch[ch].program = d1 & 0x7F; break;
    case 0xE0:
        os->ch[ch].bend = d1 | d2 << 7;
        for(v = 0; v < 18; v++) if(os->vo[v].on && os->vo[v].ch == ch) voice_pitch(v, 1);
        break;
    }
}

// the channels at their defaults and the chip in OPL3 mode, every voice free
static void midi_reset(void) {
    int i;
    for(i = 0; i < 16; i++) {
        os->ch[i].program = 0; os->ch[i].volume = 100; os->ch[i].expression = 127;
        os->ch[i].pan = 64; os->ch[i].bend = 8192; os->ch[i].sustain = 0;
    }
    memset(os->vo, 0, sizeof(os->vo));
    os->agecount = 0;
    ow(0, 0x105, 0x01);                                             // OPL3 mode: 18 voices, stereo
    ow(0, 0x104, 0x00);
    ow(0, 0x01, 0x20);
    ow(0, 0xBD, 0x00);
}

static uint32_t varlen(const unsigned char **p, const unsigned char *end) {
    uint32_t v = 0;
    int i;
    for(i = 0; i < 4 && *p < end; i++) { v = v << 7 | (**p & 0x7F); if(!(*(*p)++ & 0x80)) break; }
    return v;
}

static double midi_step(void) {
    ostate_t *s = os;
    for(;;) {
        int i, t = -1;
        for(i = 0; i < s->ntrk; i++)
            if(!s->trk[i].done && (t < 0 || s->trk[i].next < s->trk[t].next)) t = i;
        if(t < 0) return -1;
        if(s->trk[t].next > s->now_tick) {
            double d = (s->trk[t].next - s->now_tick) * s->us_per_tick * oplrate / 1e6;
            s->now_tick = s->trk[t].next;
            return d;
        }
        mtrack_t *k = &s->trk[t];
        if(k->p >= k->end) { k->done = 1; continue; }
        int st = *k->p;
        if(st & 0x80) k->p++; else st = k->running;
        if(st == 0xFF) {
            int type = *k->p++;
            uint32_t n = varlen(&k->p, k->end);
            if(type == 0x51 && n == 3 && k->p + 3 <= k->end) s->us_per_tick = (double)(k->p[0] << 16 | k->p[1] << 8 | k->p[2]) / s->division;
            if(type == 0x2F) k->done = 1;
            k->p += n;
        } else if(st == 0xF0 || st == 0xF7) {
            k->p += varlen(&k->p, k->end);
        } else if(st & 0x80) {
            int d1 = *k->p++ & 0x7F, d2 = 0;
            if((st & 0xF0) != 0xC0 && (st & 0xF0) != 0xD0) d2 = *k->p++ & 0x7F;
            k->running = st;
            midi_event(st, d1, d2);
        } else k->done = 1;                                         // (data with no status: a damaged track)
        if(!k->done && k->p < k->end) k->next += varlen(&k->p, k->end);
        else k->done = 1;
    }
}

/*******************************************************************************************
 the synth's buffers
*******************************************************************************************/
// buffer 1 or 2 made and counted, then taken to the DACs' scale in place, as MOD's (checkWAVinput). A file's events
// fall between the chip's calls at their samples; once its releases are made the rest of the buffer is silence and
// the end is flagged, as a module played once flags it
void OPLRefill(int b) {
    int16_t *s = (int16_t *)(b == 1 ? sbuff1 : sbuff2), *s0 = s;
    uint32_t left = WAV_BUFFER_SIZE / 4, n, i;
    if(playreadcomplete) { bcount[b] = 0; return; }
    while(left) {
        n = left;
        if(omode == OM_LOG || omode == OM_SONG) {
            if(!seqdone) {
                while(due < 1.0) {                                  // the events due now
                    double d = omode == OM_SONG ? midi_step() : log_step();
                    if(d < 0) { seqdone = 1; tail = 2 * oplrate; break; }
                    due += d;
                }
                if(!seqdone && due < n) n = (uint32_t)due;
            }
            if(seqdone) {
                if(tail == 0) {
                    memset(s, 0, left * 4);
                    playreadcomplete = 1;
                    break;
                }
                if(n > tail) n = tail;
            }
        }
        OPLChipGenerate(chip[0], s, n);
        if(nchips == 2) {
            OPLChipGenerate(chip[1], mixbuf, n);
            for(i = 0; i < 2 * n; i++) {
                int32_t v = s[i] + mixbuf[i];
                s[i] = v > 32767 ? 32767 : v < -32768 ? -32768 : v;
            }
        }
        s += 2 * n;
        left -= n;
        if(omode == OM_LOG || omode == OM_SONG) {
            if(seqdone) tail -= n;
            else due -= n;
        }
    }
    bcount[b] = WAV_BUFFER_SIZE / 2;
    i2convert(s0, s0, WAV_BUFFER_SIZE / 2);
}

// CloseAudio's and the end of a file's: the synth ended, its memory released (sbuff1 and sbuff2 are Audio.c's)
void OPLClose(void) {
    int i;
    for(i = 0; i < 2; i++) {
        if(chip[i]) OPLChipDone(chip[i]);
        FreeMemorySafe((void **)&chip[i]);
    }
    FreeMemorySafe((void **)&mixbuf);
    FreeMemorySafe((void **)&os);
    if(songheap) FreeMemorySafe((void **)&song);
    song = NULL;
    songheap = 0;
    nchips = 0;
    sactive = 0;
    genmidi = OPLGenMidi;
}

// MM.INFO(SOUND)'s word for the synth, and MM.INFO(TRACK)'s file (NULL for register writes and real-time MIDI)
const char *OPLSoundName(void) { return (omode == OM_LIVE || omode == OM_SONG) ? "MIDI" : "OPL"; }
const char *OPLTrack(void) { return (omode == OM_LOG || omode == OM_SONG) ? WAVfilename : NULL; }
uint32_t OPLRate(void) { return opl_rate(); }

/*******************************************************************************************
 a loadable program's music (Loader.c's sound stream): a MIDI file in the program's memory, played on the synth with
 the flash's bank or the program's GENMIDI, and added to each buffer of the stream after the program's own sound. Set
 going and stopped by the program (with the stream's refill held off: Loader.c), made in the refill (PendSV)
*******************************************************************************************/
// 0, or negative: -1 not a MIDI file, -2 its timing not supported, -3 no room
int OPLStreamPlay(const void *midi, uint32_t size, int loop, const unsigned char *bank, uint32_t frames) {
    int r;
    sactive = 0;
    oplrate = opl_rate();
    if(!chip[0] && (chip[0] = GetMemoryNull(OPLChipSize())) != NULL) OPLChipInit(chip[0], oplrate);   // (made at once:
    if(!os) os = GetMemoryNull(sizeof(ostate_t));                   // OPLClose ends it)
    if(!mixbuf) mixbuf = GetMemoryNull(frames * 4);
    if(!chip[0] || !os || !mixbuf) { OPLClose(); return -3; }
    song = (unsigned char *)midi;
    slen = size;
    songheap = 0;
    if((r = song_tracks()) < 0) return r;
    genmidi = bank ? bank : OPLGenMidi;
    OPLChipDone(chip[0]);                                           // (reset: no note of the last song left sounding)
    OPLChipInit(chip[0], oplrate);
    nchips = 1;
    omode = OM_SONG;
    midi_reset();
    due = 0;
    seqdone = 0;
    sloop = loop;
    spaused = 0;
    sactive = 1;
    return 0;
}
void OPLStreamStop(void) { sactive = 0; }
void OPLStreamVolume(int v) { svol = v < 0 ? 0 : v > 127 ? 127 : v; }
void OPLStreamPause(int p) { spaused = p; }
int OPLStreamPlaying(void) { return sactive; }

// the music's next frames added to buf (16-bit stereo); at its end it starts again if it loops, or plays its releases
// for two seconds and stops (the synth costs as much silent as sounding)
void OPLStreamMix(int16_t *buf, uint32_t frames) {
    int16_t *m = mixbuf;
    uint32_t left = frames, n, i;
    int again = 0;
    if(!sactive || spaused) return;
    while(left) {
        n = left;
        if(!seqdone) {
            while(due < 1.0) {
                double d = midi_step();
                if(d < 0) {
                    if(sloop && !again++ && song_tracks() == 0) continue;   // (once a buffer: an empty file ends)
                    seqdone = 1;
                    tail = 2 * oplrate;
                    break;
                }
                due += d;
            }
            if(!seqdone && due < n) n = (uint32_t)due;
        }
        if(seqdone) {
            if(tail == 0) { sactive = 0; break; }
            if(n > tail) n = tail;
            tail -= n;
        } else due -= n;
        OPLChipGenerate(chip[0], m, n);
        m += 2 * n;
        left -= n;
    }
    n = (uint32_t)(m - mixbuf);
    for(i = 0; i < n; i++) {
        int32_t v = buf[i] + mixbuf[i] * svol / 127;
        buf[i] = v > 32767 ? 32767 : v < -32768 ? -32768 : v;
    }
}

// the synth on in a mode, with n chips, its first buffer made and the audio timer started
static void opl_start(int mode, int n, char *interrupt) {
    int i;
    for(i = 0; i < n; i++) chip[i] = GetMemory(OPLChipSize());
    if(n == 2) mixbuf = GetMemory(WAV_BUFFER_SIZE);
    if(!os) os = GetMemory(sizeof(ostate_t));
    sbuff1 = GetInternalMemory(WAV_BUFFER_SIZE);
    sbuff2 = GetInternalMemory(WAV_BUFFER_SIZE);
    ibuff1 = (uint16_t *)sbuff1;
    ibuff2 = (uint16_t *)sbuff2;
    htim4.Init.Prescaler = PSpeedDiv / OPL_RATE - 1;
    htim4.Instance->PSC = PSpeedDiv / OPL_RATE - 1;
    oplrate = opl_rate();
    for(i = 0; i < n; i++) OPLChipInit(chip[i], oplrate);
    nchips = n;
    omode = mode;
    due = 0;
    seqdone = 0;
    tail = 0;
    WAVInterrupt = interrupt;
    WAVcomplete = 0;
    if(interrupt) IntSignal();
    playreadcomplete = 0;
    if(mode == OM_LIVE || mode == OM_SONG) midi_reset();
    OPLRefill(1);
    bcount[2] = 0;
    wav_filesize = WAV_BUFFER_SIZE / 2;
    nchannels = 2;
    CurrentlyPlaying = P_OPL;
    swingbuf = 1;
    nextbuf = 2;
    ppos = 0;
    ConfigSoundOutputs();
}

// a file into the heap, whole
static void opl_load(char *name, char *ext) {
    int fnbr;
    UINT got;
    char *p = getCstring(name);
    if(strchr(p, '.') == NULL) strcat(p, ext);
    if(!InitSDCard()) error("SD card not found");
    fnbr = FindFreeFileNbr();
    if(!BasicFileOpen(p, fnbr, FA_READ)) error("Could not open the file");
    slen = f_size(FileTable[fnbr].fptr);
    song = GetMemory(slen + 16);
    songheap = 1;
    if(f_read(FileTable[fnbr].fptr, song, slen, &got) != FR_OK || got != slen) { FileClose(fnbr); error("Could not read the file"); }
    FileClose(fnbr);
    memset(song + slen, 0, 16);
    strncpy(WAVfilename, p, sizeof(WAVfilename) - 1);               // (MM.INFO(TRACK))
}

// song's tracks found and set at their starts (os allocated): 0, -1 not a MIDI file, -2 its timing not supported
static int song_tracks(void) {
    const unsigned char *p;
    if(slen < 14 || memcmp(song, "MThd", 4)) return -1;
    os->division = song[12] << 8 | song[13];
    if(os->division == 0 || (os->division & 0x8000)) return -2;
    p = song + 8 + be32(song + 4);
    os->ntrk = 0;
    while(p + 8 <= song + slen && os->ntrk < OPL_MAXTRK) {
        uint32_t n = be32(p + 4);
        if(memcmp(p, "MTrk", 4) == 0) {
            mtrack_t *k = &os->trk[os->ntrk++];
            k->p = p + 8;
            k->end = (n > slen || p + 8 + n > song + slen) ? song + slen : p + 8 + n;
            k->running = 0;
            k->done = 0;
            k->next = varlen(&k->p, k->end);
        }
        if(n > slen) break;
        p += 8 + n;
    }
    os->us_per_tick = 500000.0 / os->division;
    os->now_tick = 0;
    return 0;
}

// a MIDI file (the heap's, or the tune in flash) set going
static void opl_song(char *interrupt) {
    int r;
    if(slen < 14 || memcmp(song, "MThd", 4)) error("Not a MIDI file");
    if(!os) os = GetMemory(sizeof(ostate_t));
    if((r = song_tracks()) == -1) error("Not a MIDI file");
    if(r == -2) error("MIDI timing not supported");
    opl_start(OM_SONG, 1, interrupt);
}

static char *opl_interrupt(char *arg) {
    char *ia = GetIntAddress(arg);
    if(!CurrentLinePtr) error("No program running");
    return ia;
}

// PLAY OPL file$: the format from its first bytes, or IMF by its extension
static void opl_file(char *tp) {
    getargs(&tp, 3, ",");
    char *ia = NULL;
    int n = 1;
    if(!(argc == 1 || argc == 3)) error("Argument count");
    if(CurrentlyPlaying != P_NOTHING) error("Sound output in use");
    if(argc == 3) ia = opl_interrupt(argv[2]);
    opl_load(argv[0], ".VGM");
    if(!os) os = GetMemory(sizeof(ostate_t));
    char *e = strrchr(WAVfilename, '.');
    double rate = opl_rate();                                       // (the units are samples: set before the first buffer)
    if(slen >= 2 && song[0] == 0x1F && song[1] == 0x8B) error("A compressed VGM: unpack it first");
    if(slen >= 26 && memcmp(song, "DBRAWOPL", 8) == 0) {
        os->fmt = LF_DRO;
        os->drover = rd16(8);
        os->drochip = 0;
        os->unit = rate / 1000.0;                                   // (a millisecond)
        if(os->drover == 2) {
            int i, cml = song[25];
            os->dro_short = song[23];
            os->dro_long = song[24];
            for(i = 0; i < cml && i < 128; i++) os->dro_map[i] = song[26 + i];
            os->pos = 26 + cml;
            os->endpos = os->pos + rd32(12) * 2;
            n = song[20] == 1 ? 2 : 1;                              // 0 OPL2, 1 two OPL2s, 2 OPL3
        } else {
            n = song[20] == 1 ? 2 : 1;
            os->pos = (song[21] | song[22] | song[23]) ? 21 : 24;   // (a hardware field of 1 byte or of 4)
            os->endpos = os->pos + rd32(16);
        }
    } else if(slen >= 0x40 && memcmp(song, "Vgm ", 4) == 0) {
        uint32_t ver = rd32(8);
        os->fmt = LF_VGM;
        os->unit = rate / 44100.0;                                  // (a VGM sample)
        os->pos = (ver >= 0x150 && rd32(0x34)) ? 0x34 + rd32(0x34) : 0x40;
        os->endpos = slen;
        n = (ver >= 0x151 && (rd32(0x50) & 0x40000000)) ? 2 : 1;   // two YM3812s
    } else if(e && (strcasecmp(e, ".IMF") == 0 || strcasecmp(e, ".WLF") == 0)) {
        uint32_t n0 = rd16(0);
        os->fmt = LF_IMF;
        os->unit = rate / (strcasecmp(e, ".WLF") == 0 ? 700 : 560);   // (a tick)
        if(n0 && n0 + 2 <= slen && n0 % 4 == 0) { os->pos = 2; os->endpos = 2 + n0; }
        else { os->pos = 0; os->endpos = slen; }
    } else error("Not an OPL file it can play (IMF, DRO or VGM)");
    if(os->endpos > slen) os->endpos = slen;
    opl_start(OM_LOG, n, ia);
}

/*******************************************************************************************
 the commands
*******************************************************************************************/
static void OPLTestVoices(void *c, int voices) {                   // PLAY OPL TEST's sound: voices held, each its pitch
    int v;
    OPLChipWrite(c, 0x105, 0x01);
    OPLChipWrite(c, 0x104, 0x00);
    OPLChipWrite(c, 0xBD, 0xC0);
    for(v = 0; v < voices; v++) {
        uint16_t bank = v < 9 ? 0 : 0x100, ch = v % 9, op = opofs[ch], f = 0x157 + v * 23;
        OPLChipWrite(c, bank + 0x20 + op, 0xE1); OPLChipWrite(c, bank + 0x23 + op, 0xE1);
        OPLChipWrite(c, bank + 0x40 + op, 0x18); OPLChipWrite(c, bank + 0x43 + op, 0x08);
        OPLChipWrite(c, bank + 0x60 + op, 0xF4); OPLChipWrite(c, bank + 0x63 + op, 0xF4);
        OPLChipWrite(c, bank + 0x80 + op, 0x25); OPLChipWrite(c, bank + 0x83 + op, 0x25);
        OPLChipWrite(c, bank + 0xE0 + op, 0x00); OPLChipWrite(c, bank + 0xE3 + op, 0x01);
        OPLChipWrite(c, bank + 0xC0 + ch, 0x36);
        OPLChipWrite(c, bank + 0xA0 + ch, f & 0xFF);
        OPLChipWrite(c, bank + 0xB0 + ch, 0x20 | (4 << 2) | (f >> 8));
    }
}

static void OPLTest(char *p) {
    getargs(&p, 3, ",");
    int secs = (argc >= 1 && *argv[0]) ? getint(argv[0], 1, 60) : 1;
    int voices = (argc == 3) ? getint(argv[2], 0, 18) : 18;
    int16_t *buf;
    void *c;
    uint64_t t, t0;
    uint32_t frames, k;
    char b[STRINGSIZE];
    int where;
    if(CurrentlyPlaying != P_NOTHING) error("Sound output in use");     // (its refills would be timed with it)
    buf = GetMemory(OPL_TEST_FRAMES * 4);
    snprintf(b, sizeof(b), "OPL3 (ymfm), %d voice%s, %d s of %d Hz stereo, code at &H%08X\r\n", voices,
             voices == 1 ? "" : "s", secs, OPL_RATE, (unsigned)(uint32_t)OPLChipGenerate);
    MMPrintString(b);
    for(where = 0; where < 2; where++) {
        c = where ? GetInternalMemoryNull(OPLChipSize()) : GetMemory(OPLChipSize());
        if(c == NULL) {
            MMPrintString("  state in internal RAM: no room\r\n");
            continue;
        }
        OPLChipInit(c, OPL_RATE);
        OPLTestVoices(c, voices);
        for(t = 0, frames = secs * OPL_RATE; frames; frames -= k) {
            k = frames > OPL_TEST_FRAMES ? OPL_TEST_FRAMES : frames;
            t0 = MicrosNow();
            OPLChipGenerate(c, buf, k);
            t += MicrosNow() - t0;
            if(MMAbort) { OPLChipDone(c); FreeMemory(c); FreeMemory(buf); CheckAbort(); }   // (Ctrl-C: freed first)
            routinechecks(1);                                       // (the USB devices seen to, outside the timing)
        }
        snprintf(b, sizeof(b), "  state in %s at &H%08X: %d.%03d ms, %d.%d%% of the CPU\r\n",
                 where ? "internal RAM" : "the SDRAM heap", (unsigned)(uint32_t)c, (int)(t / 1000), (int)(t % 1000),
                 (int)(t / (secs * 10000ULL)), (int)(t / (secs * 1000ULL) % 10));
        MMPrintString(b);
        OPLChipDone(c);
        FreeMemory(c);
    }
    FreeMemory(buf);
}

static int opl_on(void) { return CurrentlyPlaying == P_OPL || CurrentlyPlaying == P_PAUSE_OPL; }
static int midi_on(void) { return opl_on() && (omode == OM_LIVE || omode == OM_SONG); }

static void cmd_opl(char *tp) {
    char *p;
    int i;
    if((p = checkstring(tp, "WRITE"))) {
        getargs(&p, MAX_ARG_COUNT, ",");
        if(!opl_on()) error("OPL is not on");
        if(argc < 3 || (argc + 1) % 4) error("Argument count");
        for(i = 0; i < argc; i += 4) ow(0, getint(argv[i], 0, 0x1FF), getint(argv[i + 2], 0, 255));
        return;
    }
    if(checkstring(tp, "RESET")) {
        if(!opl_on()) error("OPL is not on");
        for(i = 0; i < nchips; i++) { OPLChipDone(chip[i]); OPLChipInit(chip[i], oplrate); }
        if(omode == OM_LIVE || omode == OM_SONG) midi_reset();
        return;
    }
    if((p = checkstring(tp, "TEST"))) {
        OPLTest(p);
        return;
    }
    if(*tp && *tp != '\'') {                                        // PLAY OPL file$ [, interrupt]
        opl_file(tp);
        return;
    }
    if(opl_on() && omode == OM_RAW) return;                         // (on already)
    if(CurrentlyPlaying != P_NOTHING) error("Sound output in use");
    opl_start(OM_RAW, 1, NULL);
}

static void cmd_midi(char *tp) {
    char *p;
    if((p = checkstring(tp, "CMD"))) {
        getargs(&p, 5, ",");
        if(!midi_on()) error("Midi output not enabled");
        if(!(argc == 3 || argc == 5)) error("Argument count");
        int st = getint(argv[0], 128, 255), d1 = getint(argv[2], 0, 127), d2 = argc == 5 ? getint(argv[4], 0, 127) : 0;
        if(st < 0xF0) midi_event(st, d1, d2);                       // (system messages: nothing to do on an OPL)
        return;
    }
    if((p = checkstring(tp, "TEST"))) {
        getargs(&p, 1, ",");
        if(argc == 1) getint(argv[0], 1, 3);                        // (the PicoMite's 1-3: the one tune here)
        if(opl_on() && omode == OM_LIVE) CloseAudio(1);             // (real-time MIDI gives way to the tune)
        if(CurrentlyPlaying != P_NOTHING) error("Sound output in use");
        song = (unsigned char *)OPLTestTune;
        slen = OPLTestTuneSize;
        songheap = 0;
        strcpy(WAVfilename, "MIDI TEST");
        opl_song(NULL);
        return;
    }
    if(*tp && *tp != '\'') SyntaxError();
    if(midi_on() && omode == OM_LIVE) return;                       // (on already)
    if(CurrentlyPlaying != P_NOTHING) error("Sound output in use");
    opl_start(OM_LIVE, 1, NULL);
}

static void cmd_note(char *tp) {
    char *p;
    int on = 0;
    if((p = checkstring(tp, "ON"))) on = 1;
    else if(!(p = checkstring(tp, "OFF"))) SyntaxError();
    getargs(&p, 5, ",");
    if(!midi_on()) error("Midi output not enabled");
    if(on ? argc != 5 : !(argc == 3 || argc == 5)) error("Argument count");
    int ch = getint(argv[0], 0, 15), note = getint(argv[2], 0, 127), vel = argc == 5 ? getint(argv[4], 0, 127) : 0;
    if(on) note_on(ch, note, vel);
    else note_off(ch, note);
}

static void cmd_midifile(char *tp) {
    getargs(&tp, 3, ",");
    char *ia = NULL;
    if(!(argc == 1 || argc == 3)) error("Argument count");
    if(CurrentlyPlaying != P_NOTHING) error("Sound output in use");
    if(argc == 3) ia = opl_interrupt(argv[2]);
    opl_load(argv[0], ".MID");
    opl_song(ia);
}

// PLAY's OPL and MIDI commands: 1 if cmdline was one of them (Audio.c's cmd_play)
int cmd_play_opl(char *cmdline) {
    char *tp;
    if((tp = checkstring(cmdline, "OPL"))) { if(G1Hardware) error("Only available on a G2"); cmd_opl(tp); return 1; }
    if((tp = checkstring(cmdline, "MIDIFILE"))) { if(G1Hardware) error("Only available on a G2"); cmd_midifile(tp); return 1; }
    if((tp = checkstring(cmdline, "MIDI"))) { if(G1Hardware) error("Only available on a G2"); cmd_midi(tp); return 1; }
    if((tp = checkstring(cmdline, "NOTE"))) { if(G1Hardware) error("Only available on a G2"); cmd_note(tp); return 1; }
    return 0;
}
