/***********************************************************************************************************************
The firmware's OPL3 synth: ymfm's YMF262 (Src/ymfm, BSD-3-Clause, Aaron Giles; ymfm/LICENSE) behind C for
PLAY OPL (OPL.c) and the opl_ services of loadable programs (Loader.c). Peter chose it on 2 Nov 2026 after hearing it
against Nuked-OPL3 and emu8950 on the same music (tests/opl): it sounded the same at about half Nuked's CPU.

The chip runs at its own rate, 14,318,180 / 288 = 49,716 Hz, and is taken to the output's by linear interpolation, as
Nuked-OPL3 did; its level is halved to Nuked's, which PLAY OPL had. A chip is built in memory the caller provides (the
heap) by placement new, so nothing here uses the C++ heap; it holds no other memory, so OPLChipDone only ends it. The
C++ is built without exceptions, RTTI or unwind tables (CMakeLists.txt); this file and ymfm are in bank 2.
***********************************************************************************************************************/
#include <new>
#include "OPLChip.h"
#include "ymfm/ymfm_opl.h"

/* Writes are queued and applied one per chip sample (20 us), as a real chip takes them (an OPL2 wants 23 us after a
   data write). ymfm acts on a key on or off only at its next sample, so a key off and on written together, as music
   with the rhythm section re-strikes a drum, would never reach it: Peter heard YsBattle.vgm's drums differ from
   Nuked-OPL3's, whose own write buffer spaces them (2 Nov 2026). A burst over OPL_QUEUE writes long has its oldest
   applied at once. */
#define OPL_QUEUE 256
#define OPL_CHIPRATE (14318180.0f / 288.0f)

/* A lower output rate (OPLChipDivide: a loadable program may ask for one, 2 Nov 2026): the chip is stepped n times for
   each output it makes, so its pitch, envelopes and LFO are as at the full rate, and only the output is made at
   49,716 / n Hz, which loses the treble above half that. Full rate is the default (Peter). Doom's music took 50% of the
   CPU at the full rate and 37% at half (the step is the larger part). A subclass, as ymfm's own generate() is a step
   and an output together. ymfm's one change is for this: ymfm_opl.cpp makes the YMF262's engine explicitly (as it
   makes the OPL4's), so that the engine's clock() can be called from here. */
namespace {
struct ymf262_steps : ymfm::ymf262 {
    using ymfm::ymf262::ymf262;
    void step() { m_fm.clock(fm_engine::ALL_CHANNELS); }    // a chip sample stepped, not heard
};

struct chip_t {
    ymfm::ymfm_interface intf;                          // (its defaults: no timers, no interrupts, no external memory)
    ymf262_steps opl;
    float step, pos;                                    // chip outputs per output sample; where between prev and cur
    int32_t prev[2], cur[2];
    uint16_t qreg[OPL_QUEUE];                           // the writes waiting, from head to tail
    uint8_t qval[OPL_QUEUE];
    uint16_t head, tail;
    uint32_t rate;
    int div;                                            // chip samples per output the chip makes (OPLChipDivide)
    explicit chip_t(uint32_t r) : opl(intf), step(OPL_CHIPRATE / r), pos(1.0f), prev{0, 0}, cur{0, 0}, head(0), tail(0),
                                  rate(r), div(1) {
        opl.reset();
    }
    void apply(uint32_t reg, uint32_t value) {
        if(reg & 0x100) opl.write_address_hi(reg & 0xFF);
        else opl.write_address(reg & 0xFF);
        opl.write_data(value);
    }
    void apply_next() {
        apply(qreg[head], qval[head]);
        head = (head + 1) & (OPL_QUEUE - 1);
    }
};
}

extern "C" uint32_t OPLChipSize(void) { return sizeof(chip_t); }

extern "C" void OPLChipInit(void *chip, uint32_t rate) { new (chip) chip_t(rate); }

extern "C" void OPLChipDone(void *chip) { static_cast<chip_t *>(chip)->~chip_t(); }

extern "C" void OPLChipDivide(void *chip, int n) {
    chip_t *c = static_cast<chip_t *>(chip);
    c->div = n < 1 ? 1 : n > 4 ? 4 : n;
    c->step = OPL_CHIPRATE / c->div / c->rate;
}

extern "C" void OPLChipWrite(void *chip, uint32_t reg, uint32_t value) {
    chip_t *c = static_cast<chip_t *>(chip);
    uint16_t next = (c->tail + 1) & (OPL_QUEUE - 1);
    if(next == c->head) c->apply_next();                // (full: the oldest now)
    c->qreg[c->tail] = reg & 0x1FF;
    c->qval[c->tail] = value;
    c->tail = next;
}

extern "C" void OPLChipGenerate(void *chip, int16_t *buf, uint32_t frames) {
    chip_t *c = static_cast<chip_t *>(chip);
    ymfm::ymf262::output_data out;
    while(frames--) {
        while(c->pos >= 1.0f) {                         // the chip's outputs this output sample needs
            c->prev[0] = c->cur[0];
            c->prev[1] = c->cur[1];
            for(int k = 1; k < c->div; k++) {           // (a lower rate: samples stepped first, unheard)
                if(c->head != c->tail) c->apply_next();
                c->opl.step();
            }
            if(c->head != c->tail) c->apply_next();     // (one write a chip sample)
            c->opl.generate(&out, 1);
            c->cur[0] = out.data[0];                    // (outputs A and B: left and right)
            c->cur[1] = out.data[1];
            c->pos -= 1.0f;
        }
        for(int k = 0; k < 2; k++) {
            int32_t v = (int32_t)((c->prev[k] + (c->cur[k] - c->prev[k]) * c->pos) * 0.5f);
            *buf++ = v > 32767 ? 32767 : v < -32768 ? -32768 : v;
        }
        c->pos += c->step;
    }
}
