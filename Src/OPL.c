/***********************************************************************************************************************
PLAY OPL (G2 only, 2 Nov 2026: Peter): an OPL3 (Yamaha YMF262) FM synth in the firmware, played as MOD is.

  PLAY OPL                                      the synth on: silent until its registers are written (as PLAY MIDI
                                                turns on the PicoMite's VS1053)
  PLAY OPL WRITE reg, value [, reg, value ...]  register writes, reg 0 to &H1FF (&H100 up: the OPL3's second bank), in
                                                order and with the chip's own timing
  PLAY OPL RESET                                every register back to its reset value, which silences it
  PLAY OPL TEST [seconds [, voices]]            the synth timed: seconds (1-60, default 1) of 44.1 kHz stereo made but
                                                not played, with voices (0-18, default 18) sounding, its state first in
                                                the SDRAM heap, then in internal RAM
  PLAY STOP, PAUSE and RESUME as for any sound; MM.INFO(SOUND) is "OPL" ("PAUSED OPL")

The synth is Nuked-OPL3 (opl3.c and opl3.h as published: LGPL-2.1-or-later, Src/opl3.LICENSE). It runs the chip at its
own 49,716 Hz and resamples to the output's rate. PLAY OPL's buffers are MOD's (sbuff1 and sbuff2, WAV_BUFFER_SIZE / 4
frames of 16-bit stereo each), refilled by checkWAVinput between statements (OPLRefill) and played by the MOD branch of
the sample interrupt, so a write is heard one to two buffers (46-93 ms) later. Loadable programs have the synth too,
chips of their own (Loader.c's opl_ services). This file and opl3.c are in bank 2 (the linker script's .bank2).
***********************************************************************************************************************/
#include "MMBasic_Includes.h"
#include "Hardware_Includes.h"
#include "opl3.h"
#include "Loader.h"

#define PSpeedDiv (PeripheralBusSpeed)/2                            // (Audio.c's: the audio timer's input clock)
#define OPL_RATE 44100
#define OPL_TEST_FRAMES 2048
extern TIM_HandleTypeDef htim4;
extern volatile int swingbuf, nextbuf, ppos, wav_filesize;
extern uint16_t *ibuff1, *ibuff2;
extern int nchannels;
extern void ConfigSoundOutputs(void);
extern void i2convert(int16_t *ibuff, int16_t *sbuff, int count);

static opl3_chip *oplchip = NULL;                                   // PLAY OPL's synth, on the heap; NULL when off
static uint32_t oplrate;                                            // the rate the audio timer gives

// buffer 1 or 2 made and counted, then taken to the DACs' scale in place, as MOD's (checkWAVinput)
void OPLRefill(int b) {
    int16_t *s = (int16_t *)(b == 1 ? sbuff1 : sbuff2);
    OPL3_GenerateStream(oplchip, s, WAV_BUFFER_SIZE / 4);
    bcount[b] = WAV_BUFFER_SIZE / 2;
    i2convert(s, s, WAV_BUFFER_SIZE / 2);
}

// CloseAudio's: the synth's memory released (sbuff1 and sbuff2 are CloseAudio's own)
void OPLClose(void) {
    FreeMemorySafe((void **)&oplchip);
}

// PLAY OPL TEST's sound: OPL3 mode, and voices of the 18 two-operator channels keyed on with a sustained FM tone, its
// tremolo and vibrato on, each at its own pitch
static void OPLTestVoices(opl3_chip *c, int voices) {
    static const uint8_t mod[9] = {0, 1, 2, 8, 9, 10, 16, 17, 18};  // each channel's modulator; its carrier is 3 on
    int v;
    OPL3_WriteReg(c, 0x105, 0x01);                                  // OPL3 mode
    OPL3_WriteReg(c, 0x104, 0x00);                                  // no 4-operator channels
    OPL3_WriteReg(c, 0xBD, 0xC0);                                   // the deeper tremolo and vibrato, no rhythm
    for(v = 0; v < voices; v++) {
        uint16_t bank = v < 9 ? 0 : 0x100, ch = v % 9, op = mod[ch], f = 0x157 + v * 23;
        OPL3_WriteReg(c, bank + 0x20 + op, 0xE1);                   // AM, VIB, sustained, multiple 1
        OPL3_WriteReg(c, bank + 0x23 + op, 0xE1);
        OPL3_WriteReg(c, bank + 0x40 + op, 0x18);                   // the modulator's level
        OPL3_WriteReg(c, bank + 0x43 + op, 0x08);                   // the carrier's
        OPL3_WriteReg(c, bank + 0x60 + op, 0xF4);                   // attack 15, decay 4
        OPL3_WriteReg(c, bank + 0x63 + op, 0xF4);
        OPL3_WriteReg(c, bank + 0x80 + op, 0x25);                   // sustain level 2, release 5
        OPL3_WriteReg(c, bank + 0x83 + op, 0x25);
        OPL3_WriteReg(c, bank + 0xE0 + op, 0x00);                   // sine
        OPL3_WriteReg(c, bank + 0xE3 + op, 0x01);                   // half sine
        OPL3_WriteReg(c, bank + 0xC0 + ch, 0x36);                   // left and right, feedback 3, FM
        OPL3_WriteReg(c, bank + 0xA0 + ch, f & 0xFF);
        OPL3_WriteReg(c, bank + 0xB0 + ch, 0x20 | (4 << 2) | (f >> 8));   // key on, block 4
    }
}

static void OPLTest(char *p) {
    getargs(&p, 3, ",");
    int secs = (argc >= 1 && *argv[0]) ? getint(argv[0], 1, 60) : 1;
    int voices = (argc == 3) ? getint(argv[2], 0, 18) : 18;
    int16_t *buf;
    opl3_chip *c;
    uint64_t t, t0;
    uint32_t frames, k;
    char b[STRINGSIZE];
    int where;
    if(CurrentlyPlaying != P_NOTHING) error("Sound output in use");     // (its refills would be timed with it)
    buf = GetMemory(OPL_TEST_FRAMES * 4);
    snprintf(b, sizeof(b), "OPL3, %d voice%s, %d s of %d Hz stereo, code at &H%08X\r\n", voices, voices == 1 ? "" : "s",
             secs, OPL_RATE, (unsigned)(uint32_t)OPL3_Generate);
    MMPrintString(b);
    for(where = 0; where < 2; where++) {
        c = where ? GetInternalMemoryNull(sizeof(opl3_chip)) : GetMemory(sizeof(opl3_chip));
        if(c == NULL) {
            MMPrintString("  state in internal RAM: no room\r\n");
            continue;
        }
        OPL3_Reset(c, OPL_RATE);
        OPLTestVoices(c, voices);
        for(t = 0, frames = secs * OPL_RATE; frames; frames -= k) {
            k = frames > OPL_TEST_FRAMES ? OPL_TEST_FRAMES : frames;
            t0 = MicrosNow();
            OPL3_GenerateStream(c, buf, k);
            t += MicrosNow() - t0;
            if(MMAbort) { FreeMemory(c); FreeMemory(buf); CheckAbort(); }   // (Ctrl-C: the blocks freed first)
            routinechecks(1);                                       // (the USB devices seen to, outside the timing)
        }
        snprintf(b, sizeof(b), "  state in %s at &H%08X: %d.%03d ms, %d.%d%% of the CPU\r\n",
                 where ? "internal RAM" : "the SDRAM heap", (unsigned)(uint32_t)c, (int)(t / 1000), (int)(t % 1000),
                 (int)(t / (secs * 10000ULL)), (int)(t / (secs * 1000ULL) % 10));
        MMPrintString(b);
        FreeMemory(c);
    }
    FreeMemory(buf);
}

void cmd_opl(char *tp) {
    char *p;
    if(G1Hardware) error("Only available on a G2");
    if((p = checkstring(tp, "WRITE"))) {
        getargs(&p, MAX_ARG_COUNT, ",");
        int i;
        if(CurrentlyPlaying != P_OPL && CurrentlyPlaying != P_PAUSE_OPL) error("OPL is not on");
        if(argc < 3 || (argc + 1) % 4) error("Argument count");
        for(i = 0; i < argc; i += 4)
            OPL3_WriteRegBuffered(oplchip, getint(argv[i], 0, 0x1FF), getint(argv[i + 2], 0, 255));
        return;
    }
    if(checkstring(tp, "RESET")) {
        if(CurrentlyPlaying != P_OPL && CurrentlyPlaying != P_PAUSE_OPL) error("OPL is not on");
        OPL3_Reset(oplchip, oplrate);
        return;
    }
    if((p = checkstring(tp, "TEST"))) {
        OPLTest(p);
        return;
    }
    if(*tp && *tp != '\'') SyntaxError();
    if(CurrentlyPlaying == P_OPL || CurrentlyPlaying == P_PAUSE_OPL) return;   // (on already)
    if(CurrentlyPlaying != P_NOTHING) error("Sound output in use");
    oplchip = GetMemory(sizeof(opl3_chip));
    sbuff1 = GetInternalMemory(WAV_BUFFER_SIZE);
    sbuff2 = GetInternalMemory(WAV_BUFFER_SIZE);
    ibuff1 = (uint16_t *)sbuff1;
    ibuff2 = (uint16_t *)sbuff2;
    htim4.Init.Prescaler = PSpeedDiv / OPL_RATE - 1;
    htim4.Instance->PSC = PSpeedDiv / OPL_RATE - 1;
    oplrate = PSpeedDiv / (PSpeedDiv / OPL_RATE);                   // (the timer's rate, so the pitch is the chip's)
    OPL3_Reset(oplchip, oplrate);
    WAVInterrupt = NULL;
    WAVcomplete = 0;
    playreadcomplete = 0;
    OPLRefill(1);                                                   // (silence, to start)
    bcount[2] = 0;
    wav_filesize = WAV_BUFFER_SIZE / 2;
    nchannels = 2;
    CurrentlyPlaying = P_OPL;
    swingbuf = 1;
    nextbuf = 2;
    ppos = 0;
    ConfigSoundOutputs();
}
