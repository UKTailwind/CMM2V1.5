/***********************************************************************************************************************
The firmware's OPL3 synth (G2 only): ymfm's YMF262 (Src/ymfm, BSD-3-Clause, Aaron Giles; ymfm/LICENSE) behind C for
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

namespace {
struct chip_t {
    ymfm::ymfm_interface intf;                          // (its defaults: no timers, no interrupts, no external memory)
    ymfm::ymf262 opl;
    float step, pos;                                    // chip samples per output sample; where between prev and cur
    int32_t prev[2], cur[2];
    uint16_t qreg[OPL_QUEUE];                           // the writes waiting, from head to tail
    uint8_t qval[OPL_QUEUE];
    uint16_t head, tail;
    explicit chip_t(uint32_t rate) : opl(intf), step((14318180.0f / 288.0f) / rate), pos(1.0f), prev{0, 0}, cur{0, 0},
                                     head(0), tail(0) {
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
        while(c->pos >= 1.0f) {                         // the chip's samples this output sample needs
            c->prev[0] = c->cur[0];
            c->prev[1] = c->cur[1];
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
